// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version, as published
	by the Warzone 2100 Project. They are provided in COPYING.SECTION7 in
	the top-level directory of the source distribution. This notice must
	be kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/

#include "intelpanel.h"
#include "intelview.h"
#include "intelkit.h"
#include "playersview.h"
#include "teamstrategyview.h"

#include "../hudskin.h"
#include "../hudtoast.h"

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/ivis_opengl/textdraw.h"
#include "lib/widget/button.h"
#include "lib/widget/editbox.h"
#include "lib/widget/form.h"
#include "lib/widget/widget.h"
#include "lib/netplay/netplay.h"

#include "../../hci.h"
#include "../../intdisplay.h"
#include "../../intelmap.h"
#include "../../loop.h"
#include "../../mission.h"
#include "../../multiplay.h"
#include "../../qtscript.h"
#include "../../screens/researchtreescreen.h"
#include "../../hci/teamstrategy.h"
#include "../../display.h"
#include "../../input/context.h"
#include "../../input/manager.h"

#include <algorithm>

namespace
{

constexpr int FULL_WIDTH = 930;
constexpr int FULL_HEIGHT = 588;
constexpr int COMPACT_BELOW_WIDTH = 960;
constexpr int COMPACT_BELOW_HEIGHT = 600;
constexpr int COMPACT_MARGIN = 4;
constexpr int TITLE_HEIGHT = 44;
constexpr int TITLE_HEIGHT_COMPACT = 44;
constexpr int CLOSE_BUTTON = 22;
constexpr int CLOSE_BUTTON_COMPACT = 32;

std::shared_ptr<W_SCREEN> overlay;
bool pausedGame = false;

bool compactLayout()
{
	return screenWidth < COMPACT_BELOW_WIDTH || screenHeight < COMPACT_BELOW_HEIGHT;
}

/// The Classic screen's pause (setIntelligencePauseState): the game's updates, its time, scripts and console stop (not
/// the tutorial's), while the in-game interface goes on, so the reticule's buttons still close the screen and open
/// their panels. Without its backdrop: the loop draws the paused world behind this screen.
void pauseGame()
{
	if (bMultiPlayer)
	{
		return;
	}
	clearMissionWidgets();
	gameTimeStop();
	setGameUpdatePause(true);
	if (!bInTutorial)
	{
		setScriptPause(true);
		setConsolePause(true);
	}
	setScrollPause(true);
	pausedGame = true;
}

void resumeGame()
{
	if (!pausedGame)
	{
		return;
	}
	resetMissionWidgets();
	setGameUpdatePause(false);
	if (!bInTutorial)
	{
		setScriptPause(false);
	}
	setScrollPause(false);
	setConsolePause(false);
	gameTimeStart();
	pausedGame = false;
}

enum class PanelView : uint8_t
{
	Players,
	Intelligence,
	Strategy
};

constexpr int PLAYERS_WIDTH = 1140;
constexpr int TAB_HEIGHT = 30;
constexpr int TAB_HEIGHT_COMPACT = 32;
/// The room the panels with other players leave under them, for the reticule, the power bar and the radar
constexpr int RESERVED_BOTTOM = 110;

/// (With other players) A view's tab in the header: its icon and name, and the Intelligence view's unread count
class IntelTab : public W_BUTTON
{
public:
	static std::shared_ptr<IntelTab> make(PanelView view, bool compact)
	{
		auto tab = std::make_shared<IntelTab>();
		tab->view = view;
		tab->compact = compact;
		switch (view)
		{
		case PanelView::Players:
			tab->label = compact ? _("Players") : _("Players & diplomacy");
			tab->icon = "players";
			break;
		case PanelView::Intelligence:
			tab->label = _("Intelligence");
			tab->icon = "intelligence";
			break;
		case PanelView::Strategy:
			tab->label = compact ? _("Strategy") : _("Team strategy");
			tab->icon = "team-strategy";
			break;
		}
		return tab;
	}

	PanelView view = PanelView::Players;
	bool selected = false;
	std::function<size_t ()> count;

	int32_t idealWidth() override
	{
		const iV_fonts font = selected ? font_regular_bold : font_regular;
		text.setText(WzString::fromUtf8(label), font);
		int w = 12 + 16 + 8 + text.width() + 12;
		const size_t n = count ? count() : 0;
		if (n > 0)
		{
			w += 8 + countWidth(n);
		}
		return w;
	}

protected:
	void display(int xOffset, int yOffset) override
	{
		HudSkin &skin = currentHudSkin();
		const HudColors &colors = skin.colors();
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		if (selected)
		{
			PIELIGHT fill = colors.accent;
			fill.byte.a = 40;
			pie_DrawRoundedBox(static_cast<float>(rect.x()), static_cast<float>(rect.y()), static_cast<float>(rect.right()), static_cast<float>(rect.bottom()), fill,
			                   skin.metrics().cardRadius, colors.accent, 1.f);
		}
		else if (isHighlighted())
		{
			skin.drawCard(rect, HudCardState::Highlighted);
		}
		const int iconSize = 16;
		skin.drawNamedIcon(icon, WzRect(rect.x() + 12, rect.y() + (rect.height() - iconSize) / 2, iconSize, iconSize), selected ? colors.accent : colors.textSecondary);
		const iV_fonts font = selected ? font_regular_bold : font_regular;
		text.setText(WzString::fromUtf8(label), font);
		const float center = static_cast<float>(rect.y()) + rect.height() / 2.f;
		const int textX = rect.x() + 12 + iconSize + 8;
		text.render(static_cast<float>(textX), std::round(center - text.lineSize() / 2.f - text.aboveBase()), selected ? colors.textPrimary : colors.textSecondary);
		const size_t n = count ? count() : 0;
		if (n > 0)
		{
			const int w = countWidth(n);
			const int h = 18;
			const int bx = textX + text.width() + 8;
			const float top = std::round(center - h / 2.f);
			pie_DrawRoundedBox(static_cast<float>(bx), top, static_cast<float>(bx + w), top + h, colors.accent, h / 2.f);
			countText.render(static_cast<float>(bx + (w - countText.width()) / 2), std::round(center - countText.lineSize() / 2.f - countText.aboveBase()), colors.textPrimary);
		}
	}

private:
	int countWidth(size_t n)
	{
		countText.setText(WzString::fromUtf8(std::to_string(n)), font_small);
		return std::max(18, countText.width() + 10);
	}

	std::string label;
	std::string icon;
	bool compact = false;
	WzText text;
	WzText countText;
};

/// The panel: its frame, the title row (single player: the title, the count of unread items, "Paused", and with other
/// players: the views' tabs), close, and the current view
class IntelPanel : public W_FORM
{
public:
	void initialize(nonstd::optional<PanelView> first)
	{
		compact = compactLayout();
		withOthers = bMultiPlayer;
		const bool spectator = selectedPlayer >= MAX_PLAYERS || NetPlay.players[selectedPlayer].isSpectator;
		if (withOthers)
		{
			views.push_back(PanelView::Players);
		}
		if (!spectator)
		{
			views.push_back(PanelView::Intelligence);
		}
		if (withOthers && teamStrategyViewAvailable(!NetPlay.bComms))
		{
			views.push_back(PanelView::Strategy);
		}
		if (std::find(views.begin(), views.end(), PanelView::Intelligence) != views.end())
		{
			view = IntelView::make(compact);
			view->screen = []() { return overlay; };
		}

		std::weak_ptr<IntelPanel> weakSelf = std::static_pointer_cast<IntelPanel>(shared_from_this());
		if (withOthers)
		{
			for (PanelView which : views)
			{
				auto tab = IntelTab::make(which, compact);
				if (which == PanelView::Intelligence)
				{
					std::weak_ptr<IntelView> weakView = view;
					tab->count = [weakView]() -> size_t {
						auto strongView = weakView.lock();
						return strongView ? strongView->unreadCount() : 0;
					};
				}
				tab->addOnClickHandler([weakSelf, which](W_BUTTON &) {
					widgScheduleTask([weakSelf, which]() {
						if (auto strong = weakSelf.lock())
						{
							strong->showView(which);
						}
					});
				});
				tabs.push_back(tab);
				attach(tab);
			}
		}

		W_BUTINIT init;
		init.pTip = _("Close");
		init.pDisplay = intDisplayHudCloseButton;
		closeButton = std::make_shared<W_BUTTON>(&init);
		closeButton->addOnClickHandler([](W_BUTTON &) {
			widgScheduleTask([]() { intResetScreen(false); });
		});
		attach(closeButton);
		showView(first.has_value() && std::find(views.begin(), views.end(), *first) != views.end() ? *first : views.front());
	}

	std::shared_ptr<IntelView> view;
	std::shared_ptr<HudPlayersView> players;
	std::shared_ptr<HudTeamStrategyView> strategy;
	bool compact = false;
	bool withOthers = false;
	PanelView current = PanelView::Intelligence;
	std::function<void ()> onSizeChanged;
	/// The item it opens on (the first unread one otherwise)
	const MESSAGE *firstMessage = nullptr;
	nonstd::optional<std::pair<HudIntelKind, std::string>> firstEntry;

	bool hasView(PanelView which) const
	{
		return std::find(views.begin(), views.end(), which) != views.end();
	}

	void showView(PanelView which)
	{
		if (!hasView(which))
		{
			return;
		}
		if (content)
		{
			detach(content);
		}
		if (auto accessory = view ? view->headerAccessory() : nullptr)
		{
			if (accessory->parent())
			{
				detach(accessory);
			}
		}
		current = which;
		switch (which)
		{
		case PanelView::Intelligence:
			content = view;
			// (Selecting an item marks it read, so not before the view is shown)
			if (!view->hasSelection())
			{
				if (!firstEntry.has_value() || !view->selectEntry(firstEntry->first, firstEntry->second))
				{
					view->selectMessage(firstMessage);
				}
				firstMessage = nullptr;
				firstEntry.reset();
			}
			break;
		case PanelView::Players:
			if (!players)
			{
				players = HudPlayersView::make(compact, hasView(PanelView::Intelligence));
				std::weak_ptr<IntelPanel> weakSelf = std::static_pointer_cast<IntelPanel>(shared_from_this());
				players->onOpenIntelligence = [weakSelf](HudIntelKind kind, const std::string &key) {
					if (auto strong = weakSelf.lock())
					{
						if (!key.empty() && strong->view)
						{
							strong->view->selectEntry(kind, key);
						}
						strong->showView(PanelView::Intelligence);
					}
				};
			}
			content = players;
			break;
		case PanelView::Strategy:
			if (!strategy)
			{
				strategy = HudTeamStrategyView::make(compact);
			}
			content = strategy;
			break;
		}
		attach(content, ChildZPos::Back);
		if (which == PanelView::Intelligence && view && view->headerAccessory())
		{
			attach(view->headerAccessory());
		}
		for (auto &tab : tabs)
		{
			tab->selected = tab->view == which;
		}
		if (onSizeChanged)
		{
			onSizeChanged();
		}
		geometryChanged();
	}

	void nextView(int delta)
	{
		auto it = std::find(views.begin(), views.end(), current);
		const int index = static_cast<int>(it - views.begin());
		const int count = static_cast<int>(views.size());
		showView(views[static_cast<size_t>(((index + delta) % count + count) % count)]);
	}

	/// The panel's size for the current view, at most `maxWidth` by `maxHeight`
	WzSize preferredSize(int maxWidth, int maxHeight) const
	{
		const int titleHeight = compact ? TITLE_HEIGHT_COMPACT : TITLE_HEIGHT;
		// (With other players, the panel keeps its width from view to view)
		const int w = std::min(withOthers ? PLAYERS_WIDTH : FULL_WIDTH, maxWidth);
		if (current == PanelView::Intelligence)
		{
			return WzSize(w, std::min(FULL_HEIGHT, maxHeight));
		}
		const int contentHeight = current == PanelView::Players ? players->heightFor(w) : strategy->heightFor(w);
		return WzSize(w, std::min(titleHeight + contentHeight, maxHeight));
	}

	void geometryChanged() override
	{
		if (!closeButton)
		{
			return;
		}
		const int titleHeight = compact ? TITLE_HEIGHT_COMPACT : TITLE_HEIGHT;
		const int close = compact ? CLOSE_BUTTON_COMPACT : CLOSE_BUTTON;
		closeButton->setGeometry(width() - close - (compact ? 6 : 16), (titleHeight - close) / 2 + (compact ? 0 : 1), close, close);
		int x0 = compact ? 6 : 12;
		const int tabHeight = compact ? TAB_HEIGHT_COMPACT : TAB_HEIGHT;
		for (auto &tab : tabs)
		{
			const int w = tab->idealWidth();
			tab->setGeometry(x0, (titleHeight - tabHeight) / 2, w, tabHeight);
			x0 += w + (compact ? 4 : 10);
		}
		if (content)
		{
			content->setGeometry(0, titleHeight, width(), height() - titleHeight);
		}
		if (current == PanelView::Intelligence && view && view->headerAccessory())
		{
			auto accessory = view->headerAccessory();
			const int gap = compact ? 6 : 10;
			accessory->setGeometry(closeButton->x() - gap - accessory->width(), (titleHeight - accessory->height()) / 2 + (compact ? 0 : 1),
			                       accessory->width(), accessory->height());
		}
	}

	void display(int xOffset, int yOffset) override
	{
		HudSkin &skin = currentHudSkin();
		const HudColors &colors = skin.colors();
		const WzRect rect(xOffset + x(), yOffset + y(), width(), height());
		skin.drawPanelFrame(rect, rect);
		const int titleHeight = compact ? TITLE_HEIGHT_COMPACT : TITLE_HEIGHT;
		if (compact || withOthers)
		{
			// (Single player's full layout has its divider under the filters, in the view)
			const bool underFilters = !compact && current == PanelView::Intelligence;
			if (!underFilters)
			{
				pie_UniTransBoxFill(static_cast<float>(rect.x() + 1), static_cast<float>(rect.y() + titleHeight), static_cast<float>(rect.right() - 1),
				                    static_cast<float>(rect.y() + titleHeight + 1), pal_RGBA(120, 150, 190, 48));
			}
		}
		if (withOthers)
		{
			return;
		}
		int x0 = rect.x() + (compact ? 10 : 16);
		const int iconSize = 18;
		skin.drawNamedIcon("intelligence", WzRect(x0, rect.y() + (titleHeight - iconSize) / 2, iconSize, iconSize), colors.accent);
		x0 += iconSize + 8;
		title.setText(WzString::fromUtf8(_("Intelligence")), compact ? font_regular_bold : font_medium_bold);
		const float center = static_cast<float>(rect.y()) + titleHeight / 2.f;
		title.render(static_cast<float>(x0), std::round(center - title.lineSize() / 2.f - title.aboveBase()), colors.textPrimary);
		x0 += title.width() + 10;
		const size_t unread = view->unreadCount();
		if (unread > 0)
		{
			badge.setText(WzString::fromUtf8(astringf(_("%zu new"), unread)), font_small);
			const int w = badge.width() + 16;
			const int h = 18;
			const WzRect pill(x0, static_cast<int>(center) - h / 2, w, h);
			skin.drawPill(pill, HudTone::Accent, true);
			badge.render(static_cast<float>(x0 + 8), std::round(center - badge.lineSize() / 2.f - badge.aboveBase()), colors.textPrimary);
			x0 += w + 10;
		}
		if (!compact)
		{
			paused.setText(WzString::fromUtf8(_("Paused")), font_small);
			paused.render(static_cast<float>(x0), std::round(center - paused.lineSize() / 2.f - paused.aboveBase()), colors.textSecondary);
		}
	}

private:
	std::vector<PanelView> views;
	std::vector<std::shared_ptr<IntelTab>> tabs;
	std::shared_ptr<WIDGET> content;
	std::shared_ptr<W_BUTTON> closeButton;
	WzText title;
	WzText badge;
	WzText paused;
};

/// The whole screen: in single player the world dimmed (and out of reach), the panel over it, the keys
class IntelRoot : public W_FORM
{
public:
	void initialize(nonstd::optional<PanelView> first, const MESSAGE *message, nonstd::optional<std::pair<HudIntelKind, std::string>> entry)
	{
		// (As with the Classic screen, clicks outside the panel reach the interface under it, ex. the reticule's buttons.
		// The world takes no clicks while the game's updates are paused)
		setTransparentToMouse(true);
		panel = std::make_shared<IntelPanel>();
		attach(panel);
		std::weak_ptr<IntelRoot> weakSelf = std::static_pointer_cast<IntelRoot>(shared_from_this());
		panel->onSizeChanged = [weakSelf]() {
			if (auto strong = weakSelf.lock())
			{
				strong->layoutPanel();
			}
		};
		panel->firstMessage = message;
		panel->firstEntry = std::move(entry);
		panel->initialize(first);
		setCalcLayout([](WIDGET *widget) {
			widget->setGeometry(0, 0, static_cast<int>(screenWidth), static_cast<int>(screenHeight));
		});
	}

	std::shared_ptr<IntelPanel> panel;

	void geometryChanged() override
	{
		layoutPanel();
	}

	void layoutPanel()
	{
		if (width() == 0 || height() == 0)
		{
			return;
		}
		if (panel->compact != compactLayout())
		{
			// (The layout is chosen as the screen opens: a resize between the two is rare enough to start again)
			const PanelView current = panel->current;
			widgScheduleTask([current]() {
				hudIntelScreenClose();
				hudIntelScreenOpenOn(static_cast<int>(current));
			});
			return;
		}
		if (panel->compact)
		{
			panel->setGeometry(COMPACT_MARGIN, COMPACT_MARGIN, width() - 2 * COMPACT_MARGIN, height() - 2 * COMPACT_MARGIN);
			return;
		}
		const bool withOthers = panel->withOthers;
		const WzSize size = panel->preferredSize(width() - 24, height() - 24 - (withOthers ? RESERVED_BOTTOM / 2 : 0));
		const int top = withOthers ? std::max(12, (height() - RESERVED_BOTTOM - size.height()) / 2) : (height() - size.height()) / 2;
		panel->setGeometry((width() - size.width()) / 2, top, size.width(), size.height());
	}

	void display(int xOffset, int yOffset) override
	{
		if (bMultiPlayer)
		{
			// (The game goes on: the world isn't dimmed)
			return;
		}
		// The world dimmed, but not the reticule, whose buttons still work
		const PIELIGHT dimColor = pal_RGBA(0, 0, 0, 110);
		const float x0 = static_cast<float>(xOffset + x());
		const float y0 = static_cast<float>(yOffset + y());
		const float x1 = x0 + static_cast<float>(width());
		const float y1 = y0 + static_cast<float>(height());
		WIDGET *reticule = psWScreen ? widgGetFromID(psWScreen, IDRET_FORM) : nullptr;
		if (reticule == nullptr || !reticule->visible())
		{
			pie_UniTransBoxFill(x0, y0, x1, y1, dimColor);
			return;
		}
		const float rx0 = static_cast<float>(reticule->screenPosX());
		const float ry0 = static_cast<float>(reticule->screenPosY());
		const float rx1 = rx0 + static_cast<float>(reticule->width());
		const float ry1 = ry0 + static_cast<float>(reticule->height());
		pie_UniTransBoxFill(x0, y0, x1, ry0, dimColor);
		pie_UniTransBoxFill(x0, ry1, x1, y1, dimColor);
		pie_UniTransBoxFill(x0, ry0, rx0, ry1, dimColor);
		pie_UniTransBoxFill(rx1, ry0, x1, ry1, dimColor);
	}

	void run(W_CONTEXT *) override
	{
		if (intelkit::menuIsOpen())
		{
			// (A menu takes the keys, and closes on Escape)
			wasEditing = false;
			return;
		}
		IntelPanel &p = *panel;
		const bool editing = p.current == PanelView::Intelligence && p.view->searchIsEditing();
		const bool shift = keyDown(KEY_LSHIFT) || keyDown(KEY_RSHIFT);
		if (keyPressed(KEY_ESC))
		{
			// (An edit box gives up its text and its focus on Escape itself)
			bool handled = editing || wasEditing;
			if (!handled && p.current == PanelView::Intelligence)
			{
				handled = p.view->backOut();
			}
			if (!handled && p.current == PanelView::Players)
			{
				handled = p.players->backOut();
			}
			if (!handled)
			{
				widgScheduleTask([]() { intResetScreen(false); });
			}
			inputLoseFocus();
		}
		else if (!editing && keyPressed(KEY_TAB) && p.withOthers)
		{
			p.nextView(shift ? -1 : 1);
			inputLoseFocus();
		}
		else if (!editing)
		{
			const bool enter = keyPressed(KEY_RETURN) || (!p.withOthers && keyPressed(KEY_KPENTER));
			switch (p.current)
			{
			case PanelView::Intelligence:
				if (keyPressed(KEY_UPARROW))
				{
					p.view->moveSelection(-1);
				}
				else if (keyPressed(KEY_DOWNARROW))
				{
					p.view->moveSelection(1);
				}
				else if (enter)
				{
					p.view->playSelected();
				}
				else if ((keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL)) && keyPressed(KEY_F))
				{
					p.view->focusSearch();
				}
				break;
			case PanelView::Players:
				if (keyPressed(KEY_UPARROW))
				{
					p.players->moveFocus(-1);
				}
				else if (keyPressed(KEY_DOWNARROW))
				{
					p.players->moveFocus(1);
				}
				else if (enter)
				{
					p.players->actOnFocus();
				}
				break;
			case PanelView::Strategy:
			{
				const int back = p.compact ? KEY_UPARROW : KEY_LEFTARROW;
				const int forward = p.compact ? KEY_DOWNARROW : KEY_RIGHTARROW;
				if (keyPressed(static_cast<KEY_CODE>(back)))
				{
					p.strategy->moveFocus(-1);
				}
				else if (keyPressed(static_cast<KEY_CODE>(forward)))
				{
					p.strategy->moveFocus(1);
				}
				else if (enter)
				{
					p.strategy->stepFocus(!shift);
				}
				else if (keyPressed(KEY_BACKSPACE))
				{
					p.strategy->stepFocus(false);
				}
				break;
			}
			}
		}
		wasEditing = editing;
	}

private:
	bool wasEditing = false;
};

}

bool hudIntelScreenEnabled()
{
	// (With other players: spectators and a replay's viewer too, on the players view)
	return (selectedPlayer < (bMultiPlayer ? MAX_CONNECTED_PLAYERS : MAX_PLAYERS) || NETisReplay()) && currentHudSkin().drawsIntelligenceScreen();
}

namespace
{

/// The item the screen opens on (from a toast's View button)
nonstd::optional<std::pair<HudIntelKind, std::string>> requestedEntry;

void openScreen(nonstd::optional<PanelView> first, MESSAGE *message, bool play)
{
	if (overlay)
	{
		return;
	}
	auto entry = std::move(requestedEntry);
	requestedEntry.reset();
	if (entry.has_value())
	{
		first = PanelView::Intelligence;
	}
	// (Any notice would point into the screen, which is now open)
	hudToastClearAll();
	pauseGame();
	// (As with the research tree, the game's keys are held while the screen is up, except the background context's,
	// ex. the multiplayer menu's key, which closes it)
	gInputManager.contexts().pushState();
	gInputManager.contexts().makeAllInactive();
	gInputManager.contexts().set(InputContext::BACKGROUND, InputContext::State::ACTIVE);
	overlay = W_SCREEN::make();
	overlay->psForm->setTransparentToMouse(true);
	auto root = std::make_shared<IntelRoot>();
	overlay->psForm->attach(root);
	root->initialize(first, message, std::move(entry));
	root->callCalcLayout();
	if (play && root->panel->view)
	{
		std::weak_ptr<IntelView> weakView = root->panel->view;
		widgScheduleTask([weakView]() {
			if (auto view = weakView.lock())
			{
				view->playSelected();
			}
		});
	}
	widgRegisterOverlayScreenOnTopOfScreen(overlay, psWScreen);
}

std::shared_ptr<IntelRoot> currentRoot()
{
	if (!overlay || overlay->psForm->children().empty())
	{
		return nullptr;
	}
	return std::dynamic_pointer_cast<IntelRoot>(overlay->psForm->children().front());
}

}

void hudIntelScreenOpen(MESSAGE *message, bool play)
{
	// With other players: on the players view, unless it opens on an item
	openScreen(bMultiPlayer && message == nullptr ? nonstd::optional<PanelView>(PanelView::Players) : nonstd::optional<PanelView>(PanelView::Intelligence),
	           message, play);
}

void hudIntelScreenOpenOn(int view)
{
	openScreen(static_cast<PanelView>(view), nullptr, false);
}

void hudIntelScreenShowEntry(int kind, const std::string &key)
{
	if (overlay)
	{
		hudIntelScreenClose();
		intResetScreen(true);
	}
	requestedEntry = std::make_pair(static_cast<HudIntelKind>(kind), key);
	psCurrentMsg = nullptr;
	addIntelScreen();
	requestedEntry.reset();
}

void hudIntelScreenClose()
{
	if (!overlay)
	{
		return;
	}
	intStopResearchReportAudio();
	intelkit::closeMenu();
	// (The research tree, if a report opened it over the screen, goes with it)
	if (researchTreeScreenIsUp())
	{
		closeResearchTreeScreen();
	}
	widgRemoveOverlayScreen(overlay);
	overlay.reset();
	gInputManager.contexts().popState();
	resumeGame();
}

bool hudIntelScreenIsOpen()
{
	return overlay != nullptr;
}

bool hudIntelScreenApplyForTests(const std::string &setting)
{
	auto root = currentRoot();
	const size_t equals = setting.find('=');
	if (root == nullptr || equals == std::string::npos)
	{
		return false;
	}
	const std::string key = setting.substr(0, equals);
	const std::string value = setting.substr(equals + 1);
	IntelPanel &panel = *root->panel;
	if (key == "view")
	{
		const PanelView which = value == "players" ? PanelView::Players : (value == "strategy" ? PanelView::Strategy : PanelView::Intelligence);
		if (!panel.hasView(which))
		{
			return false;
		}
		panel.showView(which);
		return true;
	}
	switch (panel.current)
	{
	case PanelView::Players:
		return panel.players->applyForTests(key, value);
	case PanelView::Strategy:
		return panel.strategy->applyForTests(key, value);
	case PanelView::Intelligence:
		break;
	}
	return panel.view->applyForTests(key, value);
}
