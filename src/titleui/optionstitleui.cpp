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
 *  Options Title UI.
 */

#include "optionstitleui.h"
#include "../frontend.h"
#include "../multiint.h"
#include "../hci.h"
#include "../frend.h"
#include "../intdisplay.h"
#include "lib/widget/margin.h"
#include "widgets/titleformwrapper.h"

// MARK: -

WzOptionsTitleUI::WzOptionsTitleUI(std::shared_ptr<WzTitleUI> parent)
	: parent(parent)
{
	screen = W_SCREEN::make();
}

WzOptionsTitleUI::~WzOptionsTitleUI()
{
	// currently a no-op
}

void WzOptionsTitleUI::screenSizeDidChange(unsigned int oldWidth, unsigned int oldHeight, unsigned int newWidth, unsigned int newHeight)
{
	screen->screenSizeDidChange(oldWidth, oldHeight, newWidth, newHeight);
}

void WzOptionsTitleUI::start()
{
	if (bAlreadyStarted)
	{
		return;
	}

	bAlreadyStarted = true;

	addBackdrop(screen);

	auto weakSelf = std::weak_ptr<WzOptionsTitleUI>(std::dynamic_pointer_cast<WzOptionsTitleUI>(shared_from_this()));

	auto psQuitButton = std::make_shared<WzMultiButton>();
	psQuitButton->setGeometry(0, 0, 30, 29);
	psQuitButton->setTip(P_("menu", "Return"));
	psQuitButton->imNormal = AtlasImage(FrontImages, IMAGE_RETURN);
	psQuitButton->imDown = AtlasImage(FrontImages, IMAGE_RETURN_HI);
	psQuitButton->doHighlight = IMAGE_RETURN_HI;
	psQuitButton->tc = MAX_PLAYERS;
	psQuitButton->alpha = 255;
	psQuitButton->addOnClickHandler([weakSelf](W_BUTTON& button) {
		widgScheduleTask([weakSelf]() {
			auto strongSelf = weakSelf.lock();
			ASSERT_OR_RETURN(, strongSelf != nullptr, "No parent");
			changeTitleUI(strongSelf->parent); // go back
		});
	});

	optionsBrowserForm = createOptionsBrowser(false, Margin(8, 10).wrap(psQuitButton));

	auto newRootForm = makeTitleFormWrapper([](){ return _("OPTIONS"); }, optionsBrowserForm);
	newRootForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(0, 0, screenWidth, screenHeight);
	}));
	screen->psForm->attach(newRootForm, WIDGET::ChildZPos::Front);
}

std::shared_ptr<WzTitleUI> WzOptionsTitleUI::getParentTitleUI()
{
	return parent;
}

TITLECODE WzOptionsTitleUI::run()
{
	widgRunScreen(screen);

	widgDisplayScreen(screen); // show the widgets currently running

	if (CancelPressed())
	{
		changeTitleUI(parent);
	}

	return TITLECODE_CONTINUE;
}
