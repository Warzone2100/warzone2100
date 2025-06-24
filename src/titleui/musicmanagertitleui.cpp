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
 *  Music Manager Title UI.
 */

#include "musicmanagertitleui.h"

#include "../frontend.h"
#include "../multiint.h"
#include "../hci.h"
#include "../frend.h"
#include "../intdisplay.h"

#include "../musicmanager.h"

WzMusicManagerTitleUI::WzMusicManagerTitleUI(std::shared_ptr<WzTitleUI> parent)
	: parent(parent)
{
}

WzMusicManagerTitleUI::~WzMusicManagerTitleUI()
{
	// currently a no-op
}

void WzMusicManagerTitleUI::screenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight)
{
}

static void addOptionsBaseForm()
{
	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);

	auto botForm = std::make_shared<IntFormAnimated>();
	parent->attach(botForm);
	botForm->id = FRONTEND_BOTFORM;

	botForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(FRONTEND_BOTFORM_WIDEX, FRONTEND_TOPFORM_WIDEY, 590, 460);
	}));
}

void WzMusicManagerTitleUI::start()
{
	addBackdrop();
	addOptionsBaseForm();

	WIDGET *psBotForm = widgGetFromID(psWScreen, FRONTEND_BOTFORM);
	ASSERT(psBotForm != nullptr, "Unable to find FRONTEND_BOTFORM?");

	addSideText(FRONTEND_SIDETEXT, FRONTEND_SIDEX, FRONTEND_TOPFORM_WIDEY, _("MUSIC MANAGER"));

	auto weakSelf = std::weak_ptr<WzMusicManagerTitleUI>(std::dynamic_pointer_cast<WzMusicManagerTitleUI>(shared_from_this()));

	auto psQuitButton = addMultiBut(psWScreen, FRONTEND_BOTFORM, FRONTEND_QUIT, 10, 10, 30, 29, _("Return To Previous Screen"), IMAGE_RETURN, IMAGE_RETURN_HI, IMAGE_RETURN_HI);
	psQuitButton->addOnClickHandler([weakSelf](W_BUTTON& button) {
		widgScheduleTask([weakSelf]() {
			auto strongSelf = weakSelf.lock();
			ASSERT_OR_RETURN(, strongSelf != nullptr, "No parent");
			changeTitleUI(strongSelf->parent); // go back
		});
	});

	musicManagerForm = makeMusicManagerForm(false);
	psBotForm->attach(musicManagerForm);
	musicManagerForm->setCalcLayout([](WIDGET *psWidget){
		auto psParent = psWidget->parent();
		ASSERT_OR_RETURN(, psParent != nullptr, "No parent?");
		int w = psParent->width();
		int h = psParent->height();
		int y0 = 49;
		psWidget->setGeometry(0, y0, w, h - y0 - 1);
	});
}

std::shared_ptr<WzTitleUI> WzMusicManagerTitleUI::getParentTitleUI()
{
	return parent;
}

TITLECODE WzMusicManagerTitleUI::run()
{
	widgRunScreen(psWScreen);

	widgDisplayScreen(psWScreen); // show the widgets currently running

	if (CancelPressed())
	{
		changeTitleUI(parent);
	}

	return TITLECODE_CONTINUE;
}
