/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
/*
 * titleui/passbox.cpp
 *
 * A separated version of the password box that used to be in multiint.
 * The old password modal hides all the UI under normal conditions.
 * This version does that and also prevents any cross-talk
 *  between the Lobby-related code and the general network game code.
 * -- 20kdc
 *
 * As this comes from multiint.cpp, see that for original origin details of the UI code
 */

#include "titleui.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/screen.h"

#include "lib/widget/editbox.h"
#include "lib/widget/button.h"
#include "lib/widget/widget.h"
#include "lib/widget/widgint.h"
#include "lib/widget/label.h"

#include "lib/netplay/netplay.h"
#include "../multiplay.h"
#include "../intdisplay.h"
#include "../hci.h"
#include "../multiint.h"
#include "../warzoneconfig.h"
#include "../frend.h"
#include "../init.h"

WzPassBoxTitleUI::WzPassBoxTitleUI(std::function<void(const char *)> next) : next(next)
{
}

void WzPassBoxTitleUI::start()
{
	addBackdrop();

	WIDGET *parent = widgGetFromID(psWScreen, FRONTEND_BACKDROP);

	// draws the background of the password box
	auto passwordForm = std::make_shared<IntFormAnimated>();
	parent->attach(passwordForm);
	passwordForm->id = FRONTEND_PASSWORDFORM;
	passwordForm->setCalcLayout(LAMBDA_CALCLAYOUT_SIMPLE({
		psWidget->setGeometry(FRONTEND_BOTFORMX, 160, FRONTEND_TOPFORMW, FRONTEND_TOPFORMH - 40);
	}));

	// password label.
	auto enterPasswordLabel = std::make_shared<W_LABEL>();
	passwordForm->attach(enterPasswordLabel);
	enterPasswordLabel->setTextAlignment(WLAB_ALIGNCENTRE);
	enterPasswordLabel->setGeometry(130, 0, 280, 40);
	enterPasswordLabel->setFont(font_large, WZCOL_FORM_TEXT);
	enterPasswordLabel->setString(_("Enter Password:"));

	// and finally draw the password entry box
	auto passwordBox = std::make_shared<W_EDITBOX>();
	passwordForm->attach(passwordBox);
	passwordBox->id = CON_PASSWORD;
	passwordBox->setGeometry(130, 40, 280, 20);
	passwordBox->setBoxColours(WZCOL_MENU_BORDER, WZCOL_MENU_BORDER, WZCOL_MENU_BACKGROUND);

	auto buttonYes = std::make_shared<W_BUTTON>();
	passwordForm->attach(buttonYes);
	buttonYes->id = CON_PASSWORDYES;
	buttonYes->setImages(AtlasImage(FrontImages, IMAGE_OK), AtlasImage(FrontImages, IMAGE_OK), mpwidgetGetFrontHighlightImage(AtlasImage(FrontImages, IMAGE_OK)));
	buttonYes->move(180, 65);
	buttonYes->setTip(_("OK"));
	auto buttonNo = std::make_shared<W_BUTTON>();
	passwordForm->attach(buttonNo);
	buttonNo->id = CON_PASSWORDNO;
	buttonNo->setImages(AtlasImage(FrontImages, IMAGE_NO), AtlasImage(FrontImages, IMAGE_NO), mpwidgetGetFrontHighlightImage(AtlasImage(FrontImages, IMAGE_NO)));
	buttonNo->move(230, 65);
	buttonNo->setTip(_("Cancel"));

	// auto click in the password box
	W_CONTEXT sContext = W_CONTEXT::ZeroContext();
	widgGetFromID(psWScreen, CON_PASSWORD)->clicked(&sContext);
}

TITLECODE WzPassBoxTitleUI::run()
{
	screen_disableMapPreview();
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id;
	if (id == CON_PASSWORDYES || (keyPressed(KEY_RETURN) || keyPressed(KEY_KPENTER))) {
		next(widgGetString(psWScreen, CON_PASSWORD));
	} else if ((id == CON_PASSWORDNO) || CancelPressed()) {
		next(nullptr);
	}
	widgDisplayScreen(psWScreen);
	return TITLECODE_CONTINUE;
}
