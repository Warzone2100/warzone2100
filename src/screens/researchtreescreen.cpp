/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/** @file
 *  Full screen host for the research tree canvas. See researchtreescreen.h.
 */

#include "researchtreescreen.h"

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/widget/widgbase.h"
#include "lib/widget/form.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"

#include "../researchtree/researchtreeview.h"
#include "../hci.h"
#include "../intdisplay.h"
#include "../multiplay.h"
#include "../keybind.h"
#include "lib/netplay/netplay.h"
#include "lib/gamelib/gtime.h"
#include "../loop.h"
#include "../activity.h"
#include "../display.h"		// for gInputManager
#include "../input/context.h"
#include "../intimage.h"
#include "../spectatorwidgets.h"
#include "lib/widget/button.h"

#include <algorithm>

// Weak, so leaving a game tears the canvas down with the widget system rather than
// holding its cached label textures past graphics shutdown
static std::weak_ptr<W_SCREEN> researchTreeOverlay;
static std::weak_ptr<WzResearchTreeView> researchTreeView;

// The same numbers the in-game options screen lays its own title bar out with. The
// frame padding is a single pixel there, and anything more shows the frame's fill
// above the bar, which reads as part of it, so whatever is centered comes out sitting
// low in the band the eye sees.
static const int FRAME_PAD = 1;
static const int TITLE_BAR_HEIGHT = 34;
static const int TITLE_BAR_PAD = 5;
// The bar carries the same controls the toolbar under it does, so they take the height
// they have down there rather than whatever is left after the bar's own padding. The
// bar is a little taller than the options screen's to give them room.
static const int TITLE_BAR_CONTROL_HEIGHT = 20;

// Dims whatever is behind, swallows the click that lands on the backdrop, and
// watches for the key that closes the whole thing
class WzResearchTreeRootForm : public W_CLICKFORM
{
public:
	static std::shared_ptr<WzResearchTreeRootForm> make()
	{
		class make_shared_enabler : public WzResearchTreeRootForm {};
		auto form = std::make_shared<make_shared_enabler>();
		// Taken again whenever the screen changes size. A resize walks the tree calling
		// each widget's own callback, and one without a callback keeps the size it was made
		// at. Nothing looks wrong, a form not clipping what it holds, so the tree goes on
		// being drawn at the new size while a click on it is tested against this and misses.
		form->setCalcLayout([](WIDGET *widget) {
			widget->setGeometry(0, 0, screenWidth, screenHeight);
		});
		return form;
	}

	void display(int xOffset, int yOffset) override
	{
		const int x0 = xOffset + x();
		const int y0 = yOffset + y();
		pie_UniTransBoxFill(static_cast<float>(x0), static_cast<float>(y0),
		                    static_cast<float>(x0 + width()), static_cast<float>(y0 + height()),
		                    pal_RGBA(0, 0, 0, 170));
	}

	void setView(const std::shared_ptr<WzResearchTreeView>& view) { m_view = view; }

	void run(W_CONTEXT *) override
	{
		if (keyPressed(KEY_ESC))
		{
			// One press gives up one thing, so thinking better of a search or a traced path
			// does not also shut the view
			auto view = m_view.lock();
			const bool closing = (view == nullptr || !view->backOut());
			if (closing)
			{
				closeResearchTreeScreen();
			}
			// Clear the input only once this key has been dealt with. Overlays run top-down,
			// so anything registered beneath would otherwise act on the same press, and
			// clearing unconditionally would take the other keys from the mappings, which
			// run later in the frame.
			inputLoseFocus();
			if (closing)
			{
				return;
			}
		}

	}

	void geometryChanged() override
	{
		for (auto& child : children())
		{
			child->callCalcLayout();
		}
	}

private:
	std::weak_ptr<WzResearchTreeView> m_view;
};

// The close button at the right of the title bar. Drawn the way the in-game options
// screen draws its own, so the two read as the same control.
class WzResearchTreeCloseButton : public W_BUTTON
{
public:
	static std::shared_ptr<WzResearchTreeCloseButton> make()
	{
		class make_shared_enabler : public WzResearchTreeCloseButton {};
		auto button = std::make_shared<make_shared_enabler>();
		button->m_text.setText(WzString::fromUtf8("\u2715"), font_regular);
		button->setTip(_("Close"));
		return button;
	}

	int32_t idealWidth() override { return m_text.width() + TITLE_BAR_PAD * 2; }

	void display(int xOffset, int yOffset) override
	{
		const int x0 = xOffset + x();
		const int y0 = yOffset + y();
		const bool down = (getState() & (WBUT_DOWN | WBUT_LOCK | WBUT_CLICKLOCK)) != 0;
		const bool over = (getState() & WBUT_HIGHLIGHT) != 0;
		if (down || over)
		{
			pie_UniTransBoxFill(static_cast<float>(x0), static_cast<float>(y0),
			                    static_cast<float>(x0 + width()), static_cast<float>(y0 + height()),
			                    down ? WZCOL_MENU_SCORE_BUILT : pal_RGBA(255, 255, 255, 50));
		}
		m_text.render(x0 + (width() - m_text.width()) / 2,
		              y0 + (height() - m_text.lineSize()) / 2 - m_text.aboveBase(),
		              down ? WZCOL_FORM_TEXT : (over ? WZCOL_TEXT_BRIGHT : WZCOL_TEXT_MEDIUM));
	}

private:
	WzText m_text;
};

// What the tree is and how to leave it, over the frame's top edge. The tree fills
// everything below.
class WzResearchTreeTitleBar : public WIDGET
{
public:
	static std::shared_ptr<WzResearchTreeTitleBar> make(const std::shared_ptr<WzResearchTreeView>& view)
	{
		class make_shared_enabler : public WzResearchTreeTitleBar {};
		auto bar = std::make_shared<make_shared_enabler>();
		bar->m_title.setText(WzString::fromUtf8(_("Research Tree")), font_regular);
		bar->m_close = WzResearchTreeCloseButton::make();
		bar->attach(bar->m_close);
		bar->m_close->addOnClickHandler([](W_BUTTON&) {
			closeResearchTreeScreen();
		});
		// Whose tree is on show belongs up here with the window's own controls, which keeps
		// the toolbar below to one row however many readings a game offers. The view makes
		// these and goes on driving them.
		if (view != nullptr)
		{
			bar->m_switcher = view->seatSwitcher();
			if (bar->m_switcher)
			{
				bar->attach(bar->m_switcher);
			}
			bar->m_ownSeat = view->ownSeatChip();
			if (bar->m_ownSeat)
			{
				bar->attach(bar->m_ownSeat);
			}
			// The switcher is as wide as the seat it names and the chip comes and goes with
			// which seat that is, neither of which resizes the bar, so the bar is told when
			// the seat changes and places them again.
			std::weak_ptr<WzResearchTreeTitleBar> weakBar = bar;
			view->setOnSeatChanged([weakBar]() {
				if (auto self = weakBar.lock())
				{
					self->geometryChanged();
				}
			});
		}
		return bar;
	}

	int32_t idealHeight() override { return TITLE_BAR_HEIGHT; }

	void geometryChanged() override
	{
		const int controlHeight = std::min(TITLE_BAR_CONTROL_HEIGHT, std::max(1, height() - TITLE_BAR_PAD * 2));
		const int buttonWidth = m_close->idealWidth();
		const int controlY = (height() - controlHeight) / 2;
		m_close->setGeometry(width() - (buttonWidth + TITLE_BAR_PAD), controlY, buttonWidth, controlHeight);

		// Lined up with the toolbar's contents below rather than the bar's own padding. The
		// switcher keeps its place as the chip comes and goes, so the chip follows it.
		//
		// The bar and the view are both children of the window at the same x, so an offset
		// that lines up inside one lines up inside the other.
		int at = researchTreeContentInset();
		if (m_switcher)
		{
			// Back by what the switcher writes its own text in by, so the text lands on the
			// inset and reads as one line down the left of the view
			const int room = std::max(10, width() / 3);
			m_switcher->setGeometry(at - researchTreeSwitcherTextInset(), controlY,
			                        std::min(m_switcher->idealWidth(), room), controlHeight);
			at += m_switcher->width() - researchTreeSwitcherTextInset() + TITLE_BAR_PAD;
		}
		if (m_ownSeat && m_ownSeat->visible())
		{
			m_ownSeat->setGeometry(at, controlY, WzResearchTreeView::ownSeatChipWidth(), controlHeight);
			at += WzResearchTreeView::ownSeatChipWidth() + TITLE_BAR_PAD;
		}
		m_leadingEnd = at;
	}

	void display(int xOffset, int yOffset) override
	{
		const int x0 = xOffset + x();
		const int y0 = yOffset + y();
		// Centered on the bar, in the same ink as everything else on it. Held clear of the
		// controls on the left, which on a narrow window would have the title across them.
		const int titleX = std::max(x0 + m_leadingEnd, x0 + (width() - m_title.width()) / 2);
		m_title.render(titleX,
		               y0 + (height() - m_title.lineSize()) / 2 - m_title.aboveBase(), WZCOL_TEXT_MEDIUM);
		iV_Line(x0, y0 + height(), x0 + width(), y0 + height(), pal_RGBA(0, 0, 0, 150));
	}

private:
	WzText m_title;
	std::shared_ptr<W_BUTTON> m_close;
	std::shared_ptr<WIDGET> m_switcher;		// null where a game offers one reading
	std::shared_ptr<W_BUTTON> m_ownSeat;
	int m_leadingEnd = 0;					// right edge of the controls on the left
};

// The window the tree lives in. Draws the frame the rest of the interface uses and
// hands everything inside it to the title bar and the view.
class WzResearchTreeWindow : public WIDGET
{
public:
	static std::shared_ptr<WzResearchTreeWindow> make(const std::shared_ptr<WzResearchTreeView>& view)
	{
		class make_shared_enabler : public WzResearchTreeWindow {};
		auto window = std::make_shared<make_shared_enabler>();
		window->m_bar = WzResearchTreeTitleBar::make(view);
		window->attach(window->m_bar);
		window->m_view = view;
		window->attach(view);
		return window;
	}

	void geometryChanged() override
	{
		const int inner = width() - FRAME_PAD * 2;
		m_bar->setGeometry(FRAME_PAD, FRAME_PAD, std::max(10, inner), TITLE_BAR_HEIGHT);
		const int top = FRAME_PAD + TITLE_BAR_HEIGHT;
		m_view->setGeometry(FRAME_PAD, top, std::max(10, inner), std::max(10, height() - top - FRAME_PAD));
	}

	void display(int xOffset, int yOffset) override
	{
		const int x0 = xOffset + x();
		const int y0 = yOffset + y();
		// Behind the frame, which is translucent. The options screen is a small window over
		// a dimmed game and gets away with what shows through. This one covers the screen,
		// so the reticule ends up behind the title bar and reads through the close button.
		pie_UniTransBoxFill(static_cast<float>(x0), static_cast<float>(y0),
		                    static_cast<float>(x0 + width()), static_cast<float>(y0 + height()),
		                    pal_RGBA(8, 10, 14, 255));
		RenderWindowFrame(FRAME_NORMAL, x0, y0, width(), height());
	}

private:
	std::shared_ptr<WzResearchTreeTitleBar> m_bar;
	std::shared_ptr<WIDGET> m_view;
};

bool researchTreeScreenIsUp()
{
	return researchTreeOverlay.lock() != nullptr;
}

// The campaign is one player's to take at their own pace, and the other screens it
// offers stop the clock while up. Skirmish and multiplayer carry on.
static bool researchTreePauses()
{
	return ActivityManager::instance().getCurrentGameMode() == ActivitySink::GameMode::CAMPAIGN
		&& !runningMultiplayer();
}

// Only ever undone by whoever did it. The in-game options screen may already have
// stopped the clock before this one opened, and restarting it on the way out would
// leave that screen up over a running game. Same two flags it keeps, for that reason.
static bool treeStoppedGameTime = false;
static bool treeSetPauseStates = false;

static void pauseForResearchTree()
{
	if (!researchTreePauses())
	{
		return;
	}
	if (!gameTimeIsStopped())
	{
		gameTimeStop();
		treeStoppedGameTime = true;
	}
	if (!gamePaused())
	{
		setGamePauseStatus(true);
		setConsolePause(true);
		setScriptPause(true);
		setAudioPause(true);
		setScrollPause(true);
		treeSetPauseStates = true;
	}
}

static void unpauseAfterResearchTree()
{
	if (treeStoppedGameTime)
	{
		if (gameTimeIsStopped())
		{
			gameTimeStart();
		}
		treeStoppedGameTime = false;
	}
	if (treeSetPauseStates)
	{
		setGamePauseStatus(false);
		setConsolePause(false);
		setScriptPause(false);
		setAudioPause(false);
		setScrollPause(false);
		treeSetPauseStates = false;
	}
}

void closeResearchTreeScreen()
{
	// Before the screen goes, a popover of its own not being held by it and so left
	// standing over the game with nothing able to reach it
	if (auto view = researchTreeView.lock())
	{
		view->closePopovers();
	}
	if (auto screen = researchTreeOverlay.lock())
	{
		widgRemoveOverlayScreen(screen);
		// The view is the only thing the tree's keys are for, and everything else was
		// switched off for it
		gInputManager.contexts().popState();
	}
	researchTreeOverlay.reset();
	researchTreeView.reset();
	unpauseAfterResearchTree();
}

void researchTreeScreenBack()
{
	auto view = researchTreeView.lock();
	if (view == nullptr || !view->backOut())
	{
		closeResearchTreeScreen();
	}
}

void researchTreeScreenTracePath()
{
	if (auto view = researchTreeView.lock())
	{
		view->toggleFocus();
	}
}

void researchTreeScreenFocusSearch()
{
	if (auto view = researchTreeView.lock())
	{
		view->focusSearch();
	}
}

void researchTreeScreenToggleNames()
{
	if (auto view = researchTreeView.lock())
	{
		view->toggleLabeled();
	}
}

void researchTreeScreenStepSelection(int delta)
{
	if (auto view = researchTreeView.lock())
	{
		view->stepSelection(delta);
	}
}

void researchTreeScreenCyclePerspective(int delta)
{
	if (auto view = researchTreeView.lock())
	{
		view->cyclePerspective(delta);
	}
}

bool toggleResearchTreeScreen()
{
	if (researchTreeScreenIsUp())
	{
		closeResearchTreeScreen();
		return false;
	}
	const bool watching = (selectedPlayer >= MAX_PLAYERS)
		|| (bMultiPlayer && selectedPlayer < NetPlay.players.size() && NetPlay.players[selectedPlayer].isSpectator);
	// The interface is switched off, so a screen opened here would take the keys without
	// ever being drawn, including the one that switches it back on
	if (!getWidgetsStatus())
	{
		return false;
	}

	ResearchTreeContext context;
	context.source = ResearchTreeContext::Source::LivePlayerState;
	context.viewer = selectedPlayer;
	context.player = selectedPlayer;
	if (selectedPlayer >= MAX_PLAYERS)
	{
		// Someone holding a spectator slot has no seat of their own, so the view opens
		// on the first seat they may read and the switcher offers the rest
		const auto seats = researchPerspectivesFor(selectedPlayer);
		const auto seat = std::find_if(seats.begin(), seats.end(), [](const ResearchPerspective& p) {
			return p.kind == ResearchPerspective::Kind::Seat && p.player < MAX_PLAYERS;
		});
		if (seat == seats.end())
		{
			return false;	// nothing to look at
		}
		context.player = seat->player;
	}
	// Only a seat still being played can be acted on.
	// A finished game, a teammate's view and an eliminated player's own tree are for reading.
	context.allowAssignment = !watching;
	// The spectator widgets are an overlay of their own, so this must be placed on top
	showResearchTreeScreen(context, watching ? specOverlayScreen() : nullptr);
	return true;
}

void showResearchTreeScreen(const ResearchTreeContext& context, const std::shared_ptr<W_SCREEN>& aboveScreen)
{
	closeResearchTreeScreen();

	auto screen = W_SCREEN::make();
	researchTreeOverlay = screen;
	auto root = WzResearchTreeRootForm::make();
	screen->psForm->attach(root);

	auto view = WzResearchTreeView::make(context);
	auto window = WzResearchTreeWindow::make(view);
	root->attach(window);
	root->setView(view);
	researchTreeView = view;
	pauseForResearchTree();

	// A full screen view over the game has no business letting the game act on the same
	// keys, so nothing but the tree's own bindings stay live
	gInputManager.contexts().pushState();
	gInputManager.contexts().makeAllInactive();
	gInputManager.contexts().set(InputContext::RESEARCH_TREE, InputContext::State::PRIORITIZED);
	window->setCalcLayout([](WIDGET *widget) {
		const int inset = 16;
		widget->setGeometry(inset, inset, screenWidth - inset * 2, screenHeight - inset * 2);
	});

	widgRegisterOverlayScreenOnTopOfScreen(screen, aboveScreen ? aboveScreen : psWScreen);
}

