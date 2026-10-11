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

#include "../hudskin.h"

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

#include "../../hci.h"
#include "../../intdisplay.h"
#include "../../intelmap.h"
#include "../../loop.h"
#include "../../mission.h"
#include "../../multiplay.h"
#include "../../qtscript.h"
#include "../../screens/researchtreescreen.h"

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

/// The panel: its frame, the title row (the title, the count of unread items, "Paused", close) and the view
class IntelPanel : public W_FORM
{
public:
	void initialize()
	{
		compact = compactLayout();
		view = IntelView::make(compact);
		std::weak_ptr<IntelPanel> weakSelf = std::static_pointer_cast<IntelPanel>(shared_from_this());
		view->screen = []() { return overlay; };
		attach(view);
		if (auto accessory = view->headerAccessory())
		{
			attach(accessory);
		}

		W_BUTINIT init;
		init.pTip = _("Close");
		init.pDisplay = intDisplayHudCloseButton;
		closeButton = std::make_shared<W_BUTTON>(&init);
		closeButton->addOnClickHandler([](W_BUTTON &) {
			widgScheduleTask([]() { intResetScreen(false); });
		});
		attach(closeButton);
	}

	std::shared_ptr<IntelView> view;
	bool compact = false;

	void geometryChanged() override
	{
		const int titleHeight = compact ? TITLE_HEIGHT_COMPACT : TITLE_HEIGHT;
		const int close = compact ? CLOSE_BUTTON_COMPACT : CLOSE_BUTTON;
		closeButton->setGeometry(width() - close - (compact ? 6 : 16), (titleHeight - close) / 2 + (compact ? 0 : 1), close, close);
		view->setGeometry(0, titleHeight, width(), height() - titleHeight);
		if (auto accessory = view->headerAccessory())
		{
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
		if (compact)
		{
			// (The full layout's divider is under the filters, in the view)
			pie_UniTransBoxFill(static_cast<float>(rect.x() + 1), static_cast<float>(rect.y() + titleHeight), static_cast<float>(rect.right() - 1),
			                    static_cast<float>(rect.y() + titleHeight + 1), pal_RGBA(120, 150, 190, 48));
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
		if (!bMultiPlayer && !compact)
		{
			paused.setText(WzString::fromUtf8(_("Paused")), font_small);
			paused.render(static_cast<float>(x0), std::round(center - paused.lineSize() / 2.f - paused.aboveBase()), colors.textSecondary);
		}
	}

private:
	std::shared_ptr<W_BUTTON> closeButton;
	WzText title;
	WzText badge;
	WzText paused;
};

/// The whole screen: the world dimmed (and out of reach), the panel over it, the keys
class IntelRoot : public W_FORM
{
public:
	void initialize()
	{
		// (As with the Classic screen, clicks outside the panel reach the interface under it, ex. the reticule's buttons.
		// The world takes no clicks while the game's updates are paused)
		setTransparentToMouse(true);
		panel = std::make_shared<IntelPanel>();
		attach(panel);
		panel->initialize();
		setCalcLayout([](WIDGET *widget) {
			widget->setGeometry(0, 0, static_cast<int>(screenWidth), static_cast<int>(screenHeight));
		});
	}

	std::shared_ptr<IntelPanel> panel;

	void geometryChanged() override
	{
		if (panel->compact != compactLayout())
		{
			// (The layout is chosen as the screen opens: a resize between the two is rare enough to start again)
			widgScheduleTask([]() {
				hudIntelScreenClose();
				hudIntelScreenOpen(nullptr, false);
			});
			return;
		}
		if (panel->compact)
		{
			panel->setGeometry(COMPACT_MARGIN, COMPACT_MARGIN, width() - 2 * COMPACT_MARGIN, height() - 2 * COMPACT_MARGIN);
		}
		else
		{
			const int w = std::min(FULL_WIDTH, width() - 24);
			const int h = std::min(FULL_HEIGHT, height() - 24);
			panel->setGeometry((width() - w) / 2, (height() - h) / 2, w, h);
		}
	}

	void display(int xOffset, int yOffset) override
	{
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
		IntelView &view = *panel->view;
		const bool editing = view.searchIsEditing();
		if (keyPressed(KEY_ESC))
		{
			// (An edit box gives up its text and its focus on Escape itself)
			if (!editing && !wasEditing && !view.backOut())
			{
				widgScheduleTask([]() { intResetScreen(false); });
			}
			inputLoseFocus();
		}
		else if (!editing)
		{
			if (keyPressed(KEY_UPARROW))
			{
				view.moveSelection(-1);
			}
			else if (keyPressed(KEY_DOWNARROW))
			{
				view.moveSelection(1);
			}
			else if (keyPressed(KEY_RETURN) || keyPressed(KEY_KPENTER))
			{
				view.playSelected();
			}
			else if ((keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL)) && keyPressed(KEY_F))
			{
				view.focusSearch();
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
	return !bMultiPlayer && selectedPlayer < MAX_PLAYERS && currentHudSkin().drawsIntelligenceScreen();
}

void hudIntelScreenOpen(MESSAGE *message, bool play)
{
	if (overlay)
	{
		return;
	}
	pauseGame();
	overlay = W_SCREEN::make();
	overlay->psForm->setTransparentToMouse(true);
	auto root = std::make_shared<IntelRoot>();
	overlay->psForm->attach(root);
	root->initialize();
	root->callCalcLayout();
	root->panel->view->selectMessage(message);
	if (play)
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

void hudIntelScreenClose()
{
	if (!overlay)
	{
		return;
	}
	intStopResearchReportAudio();
	// (The research tree, if a report opened it over the screen, goes with it)
	if (researchTreeScreenIsUp())
	{
		closeResearchTreeScreen();
	}
	widgRemoveOverlayScreen(overlay);
	overlay.reset();
	resumeGame();
}

bool hudIntelScreenIsOpen()
{
	return overlay != nullptr;
}

bool hudIntelScreenApplyForTests(const std::string &setting)
{
	if (!overlay || overlay->psForm->children().empty())
	{
		return false;
	}
	auto root = std::dynamic_pointer_cast<IntelRoot>(overlay->psForm->children().front());
	const size_t equals = setting.find('=');
	return root != nullptr && equals != std::string::npos && root->panel->view->applyForTests(setting.substr(0, equals), setting.substr(equals + 1));
}
