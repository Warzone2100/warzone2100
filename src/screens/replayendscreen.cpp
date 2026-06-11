/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project

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
 *  The overlay screen displayed when a replay ends (currently: a "Player Stats" graph panel)
 */

#include "replayendscreen.h"

#include "lib/framework/frame.h"
#include "lib/widget/widget.h"
#include "lib/widget/paneltabbutton.h"
#include "lib/ivis_opengl/screen.h"
#include "../intdisplay.h"
#include "../hci.h"
#include "../levels.h"		// for LEVEL_DATASET (needed by multimenu.h)
#include "../multimenu.h"	// for WzMultiMenuTabs
#include "../playerstatsgraph.h"

// layout within a centered 640x480 design space (mirrors the mission results screen)
constexpr int REPLAYEND_BACKFORM_W = 640;
constexpr int REPLAYEND_BACKFORM_H = 480;
constexpr int REPLAYEND_TABS_H = 20;
constexpr int REPLAYEND_STATS_X = 20;
constexpr int REPLAYEND_STATS_Y = 64;
constexpr int REPLAYEND_STATS_W = 600;
constexpr int REPLAYEND_STATS_H = 300;

static std::shared_ptr<W_SCREEN> replayEndScreen;
static std::shared_ptr<WIDGET> replayEndBackForm;

void showReplayEndScreen()
{
	if (replayEndScreen)
	{
		return; // already showing
	}

	auto screen = W_SCREEN::make();
	screen->psForm->hide(); // the hidden root form still displays children, but does not accept mouse-over itself

	// backform: centered 640x480 design space, mirroring the mission results screen
	auto backForm = std::make_shared<WIDGET>();
	screen->psForm->attach(backForm);
	backForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry((static_cast<int>(screenWidth) - REPLAYEND_BACKFORM_W) / 2, (static_cast<int>(screenHeight) - REPLAYEND_BACKFORM_H) / 2, REPLAYEND_BACKFORM_W, REPLAYEND_BACKFORM_H);
	}));

	// player stats panel
	auto statsForm = std::make_shared<IntFormAnimated>();
	backForm->attach(statsForm);
	statsForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(REPLAYEND_STATS_X, REPLAYEND_STATS_Y, REPLAYEND_STATS_W, REPLAYEND_STATS_H);
	}));

	auto statsGraphForm = PlayerStatsGraphForm::make();
	statsForm->attach(statsGraphForm);
	statsGraphForm->setGeometry(0, 0, REPLAYEND_STATS_W, REPLAYEND_STATS_H);

	// single "Player Stats" tab, above the panel
	auto tabs = std::make_shared<WzMultiMenuTabs>(0);
	backForm->attach(tabs);
	tabs->setButtonAlignment(MultibuttonWidget::ButtonAlignment::CENTER_ALIGN);
	tabs->addButton(0, WzPanelTabButton::make(_("Player Stats")));
	tabs->choose(0);
	tabs->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psParent = psWidget->parent();
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		int tabsWidth = psWidget->idealWidth();
		psWidget->setGeometry((psParent->width() - tabsWidth) / 2, 0, tabsWidth, REPLAYEND_TABS_H);
	}));

	widgRegisterOverlayScreenOnTopOfScreen(screen, psWScreen);
	replayEndScreen = screen;
	replayEndBackForm = backForm;
}

void closeReplayEndScreen()
{
	if (replayEndScreen)
	{
		widgRemoveOverlayScreen(replayEndScreen);
		replayEndScreen = nullptr;
	}
	replayEndBackForm = nullptr;
}

void setReplayEndScreenVisible(bool visible)
{
	if (replayEndBackForm)
	{
		replayEndBackForm->show(visible);
	}
}
