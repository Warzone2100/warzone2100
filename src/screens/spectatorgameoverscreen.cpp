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
 *  The overlay screen displayed when the game is over for a spectator (including when a replay ends)
 */

#include "spectatorgameoverscreen.h"

#include "lib/framework/frame.h"
#include "lib/widget/widget.h"
#include "lib/widget/button.h"
#include "lib/widget/label.h"
#include "lib/widget/paneltabbutton.h"
#include "lib/ivis_opengl/screen.h"
#include "../intdisplay.h"
#include "../hci.h"
#include "../frontend.h"	// for displayTextOption / DisplayTextOptionCache
#include "../levels.h"		// for LEVEL_DATASET (needed by multimenu.h)
#include "../multimenu.h"	// for WzMultiMenuTabs
#include "../playerstatsgraph.h"
#include "../researchlogviewer.h"
#include "../mission.h"

// layout within a centered 640x480 design space (mirrors the mission results screen)
constexpr int SPECGAMEOVER_BACKFORM_W = 640;
constexpr int SPECGAMEOVER_BACKFORM_H = 480;
constexpr int SPECGAMEOVER_TITLE_X = 20;
constexpr int SPECGAMEOVER_TITLE_Y = 0;
constexpr int SPECGAMEOVER_TITLE_W = 600;
constexpr int SPECGAMEOVER_TITLE_H = 40;
constexpr int SPECGAMEOVER_TABS_Y = 44;
constexpr int SPECGAMEOVER_TABS_H = 20;
constexpr int SPECGAMEOVER_STATS_X = 20;
constexpr int SPECGAMEOVER_STATS_Y = 64;
constexpr int SPECGAMEOVER_STATS_W = 600;
constexpr int SPECGAMEOVER_STATS_H = 300;
constexpr int SPECGAMEOVER_RESEARCH_PADDING = 10;
constexpr int SPECGAMEOVER_BUTTONFORM_X = 20;
constexpr int SPECGAMEOVER_BUTTONFORM_Y = 380;
constexpr int SPECGAMEOVER_BUTTONFORM_W = 600;
constexpr int SPECGAMEOVER_BUTTONFORM_H = 80;
constexpr int SPECGAMEOVER_BUTTON_TEXT_H = 16;

static std::shared_ptr<W_SCREEN> spectatorGameOverScreen;
static std::shared_ptr<WIDGET> spectatorGameOverBackForm;

void showSpectatorGameOverScreen()
{
	ASSERT_OR_RETURN(, !MissionResUp && intMode != INT_MISSIONRES, "Mission results screen is already up");

	if (spectatorGameOverScreen)
	{
		return; // already showing
	}

	auto screen = W_SCREEN::make();
	screen->psForm->hide(); // the hidden root form still displays children, but does not accept mouse-over itself

	// backform: centered 640x480 design space, mirroring the mission results screen
	auto backForm = std::make_shared<WIDGET>();
	screen->psForm->attach(backForm);
	backForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry((static_cast<int>(screenWidth) - SPECGAMEOVER_BACKFORM_W) / 2, (static_cast<int>(screenHeight) - SPECGAMEOVER_BACKFORM_H) / 2, SPECGAMEOVER_BACKFORM_W, SPECGAMEOVER_BACKFORM_H);
	}));

	// "Game is over" title, above the tabs
	auto titleForm = std::make_shared<IntFormAnimated>();
	backForm->attach(titleForm);
	titleForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(SPECGAMEOVER_TITLE_X, SPECGAMEOVER_TITLE_Y, SPECGAMEOVER_TITLE_W, SPECGAMEOVER_TITLE_H);
	}));

	auto titleLabel = std::make_shared<W_LABEL>();
	titleForm->attach(titleLabel);
	titleLabel->setFont(font_regular, WZCOL_TEXT_BRIGHT);
	titleLabel->setString(_("Game is over"));
	titleLabel->setTextAlignment(WLAB_ALIGNCENTRE);
	titleLabel->setGeometry(0, 12, SPECGAMEOVER_TITLE_W, 16);

	// player stats panel
	auto statsForm = std::make_shared<IntFormAnimated>();
	backForm->attach(statsForm);
	statsForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(SPECGAMEOVER_STATS_X, SPECGAMEOVER_STATS_Y, SPECGAMEOVER_STATS_W, SPECGAMEOVER_STATS_H);
	}));

	auto statsGraphForm = PlayerStatsGraphForm::make();
	statsForm->attach(statsGraphForm);
	statsGraphForm->setGeometry(0, 0, SPECGAMEOVER_STATS_W, SPECGAMEOVER_STATS_H);

	// research log panel, shown when the "Research" tab is selected
	auto researchForm = std::make_shared<IntFormAnimated>(false);
	backForm->attach(researchForm);
	researchForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(SPECGAMEOVER_STATS_X, SPECGAMEOVER_STATS_Y, SPECGAMEOVER_STATS_W, SPECGAMEOVER_STATS_H);
	}));
	researchForm->hide();

	auto researchLogViewer = GameResearchLogViewerWidget::make();
	researchForm->attach(researchLogViewer);
	researchLogViewer->setGeometry(SPECGAMEOVER_RESEARCH_PADDING, SPECGAMEOVER_RESEARCH_PADDING,
	                               SPECGAMEOVER_STATS_W - SPECGAMEOVER_RESEARCH_PADDING * 2, SPECGAMEOVER_STATS_H - SPECGAMEOVER_RESEARCH_PADDING * 2);

	// "Player Stats" / "Research" tabs, above the panel
	auto tabs = std::make_shared<WzMultiMenuTabs>(0);
	backForm->attach(tabs);
	tabs->setButtonAlignment(MultibuttonWidget::ButtonAlignment::CENTER_ALIGN);
	tabs->addButton(0, WzPanelTabButton::make(_("Player Stats")));
	tabs->addButton(1, WzPanelTabButton::make(_("Research")));
	tabs->choose(0);
	std::weak_ptr<WIDGET> weakStatsForm = statsForm;
	std::weak_ptr<WIDGET> weakResearchForm = researchForm;
	tabs->addOnChooseHandler([weakStatsForm, weakResearchForm](MultibuttonWidget& widget, int newValue) {
		// Switch actively-displayed tab
		widgScheduleTask([weakStatsForm, weakResearchForm, newValue]() {
			auto strongStatsForm = weakStatsForm.lock();
			auto strongResearchForm = weakResearchForm.lock();
			ASSERT_OR_RETURN(, strongStatsForm != nullptr && strongResearchForm != nullptr, "No forms?");
			strongStatsForm->show(newValue == 0);
			strongResearchForm->show(newValue == 1);
		});
	});
	tabs->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		auto psParent = psWidget->parent();
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent");
		int tabsWidth = psWidget->idealWidth();
		psWidget->setGeometry((psParent->width() - tabsWidth) / 2, SPECGAMEOVER_TABS_Y, tabsWidth, SPECGAMEOVER_TABS_H);
	}));

	// "Quit To Main Menu" button, below the panel
	auto buttonForm = std::make_shared<IntFormAnimated>();
	backForm->attach(buttonForm);
	buttonForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(SPECGAMEOVER_BUTTONFORM_X, SPECGAMEOVER_BUTTONFORM_Y, SPECGAMEOVER_BUTTONFORM_W, SPECGAMEOVER_BUTTONFORM_H);
	}));

	auto quitButton = std::make_shared<W_BUTTON>();
	buttonForm->attach(quitButton);
	quitButton->setString(_("Quit To Main Menu"));
	quitButton->FontID = font_regular;
	quitButton->style |= WBUT_TXTCENTRE;
	quitButton->displayFunction = displayTextOption;
	quitButton->pUserData = new DisplayTextOptionCache();
	quitButton->setOnDelete([](WIDGET *psWidget) {
		assert(psWidget->pUserData != nullptr);
		delete static_cast<DisplayTextOptionCache *>(psWidget->pUserData);
		psWidget->pUserData = nullptr;
	});
	quitButton->setGeometry(5, (SPECGAMEOVER_BUTTONFORM_H - SPECGAMEOVER_BUTTON_TEXT_H) / 2, SPECGAMEOVER_BUTTONFORM_W - 10, SPECGAMEOVER_BUTTON_TEXT_H);
	quitButton->addOnClickHandler([](W_BUTTON&) {
		widgScheduleTask([]() {
			intRequestQuitToMainMenu();
		});
	});

	widgRegisterOverlayScreenOnTopOfScreen(screen, psWScreen);
	spectatorGameOverScreen = screen;
	spectatorGameOverBackForm = backForm;
}

void closeSpectatorGameOverScreen()
{
	if (spectatorGameOverScreen)
	{
		widgRemoveOverlayScreen(spectatorGameOverScreen);
		spectatorGameOverScreen = nullptr;
	}
	spectatorGameOverBackForm = nullptr;
}

void setSpectatorGameOverScreenVisible(bool visible)
{
	if (spectatorGameOverBackForm)
	{
		spectatorGameOverBackForm->show(visible);
	}
}
