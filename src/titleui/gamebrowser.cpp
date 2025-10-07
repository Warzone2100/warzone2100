// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2025  Warzone 2100 Project

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
/** \file
 *  Game Browser Title UI.
 */

#include "gamebrowser.h"
#include "lib/widget/form.h"
#include "widgets/titleformwrapper.h"
#include "widgets/gamebrowserform.h"

// MARK: -

WzGameBrowserTitleUI::WzGameBrowserTitleUI(std::shared_ptr<WzTitleUI> parent)
	: parent(parent)
{
	screen = W_SCREEN::make();
}

WzGameBrowserTitleUI::~WzGameBrowserTitleUI()
{
	// currently a no-op
}

void WzGameBrowserTitleUI::screenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight)
{
	screen->screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
}

void WzGameBrowserTitleUI::start()
{
	if (bAlreadyStarted)
	{
		refreshLobbyBrowser(gameBrowserForm);
		return;
	}

	bAlreadyStarted = true;

	addBackdrop(screen);

	auto weakSelf = std::weak_ptr<WzGameBrowserTitleUI>(std::dynamic_pointer_cast<WzGameBrowserTitleUI>(shared_from_this()));

	gameBrowserForm = makeLobbyBrowser([weakSelf](){
		auto strongSelf = weakSelf.lock();
		ASSERT_OR_RETURN(, strongSelf != nullptr, "No parent");
		changeTitleUI(strongSelf->parent); // go back
	});

	auto newRootForm = makeTitleFormWrapper([](){ return _("GAMES"); }, gameBrowserForm);
	newRootForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(0, 0, screenWidth, screenHeight);
	}));
	screen->psForm->attach(newRootForm, WIDGET::ChildZPos::Front);
}

std::shared_ptr<WzTitleUI> WzGameBrowserTitleUI::getParentTitleUI()
{
	return parent;
}

TITLECODE WzGameBrowserTitleUI::run()
{
	screen_disableMapPreview();

	widgRunScreen(screen);

	widgDisplayScreen(screen); // show the widgets currently running

	if (CancelPressed())
	{
		changeTitleUI(parent);
	}

	return TITLECODE_CONTINUE;
}
