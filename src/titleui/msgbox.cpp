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
 * titleui/msgbox.cpp
 *
 * My own little UI, this shows you the console so that you can work out what went wrong
 *  after (for example) a join.
 * This prevents various problems that came up with using the lobby-based-console,
 *  including (for example) the big tangling-up of everything...
 * -- 20kdc
 */

#include "titleui.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/netplay/netplay.h"
#include "lib/widget/label.h"
#include "lib/widget/paragraph.h"
#include "../multiplay.h"
#include "../intdisplay.h"
#include "../hci.h"
#include "../multiint.h"
#include "../warzoneconfig.h"
#include "../frend.h"

WzMsgBoxTitleUI::WzMsgBoxTitleUI(WzString title, WzString text, std::shared_ptr<WzTitleUI> next) : title(title), text(text), next(next)
{
}

void WzMsgBoxTitleUI::start()
{
	addBackdrop();
	addTopForm(false);
	addBottomForm();

	addMultiBut(psWScreen, FRONTEND_BOTFORM, WZ_MSGBOX_TUI_LEAVE, FRONTEND_BOTFORMW - (MULTIOP_OKW * 2), FRONTEND_BOTFORMH - (MULTIOP_OKH * 2), MULTIOP_OKW, MULTIOP_OKH, _("Continue"), IMAGE_OK, IMAGE_OK, true);

	auto psBotForm = widgGetFromID(psWScreen, FRONTEND_BOTFORM);

	if (psBotForm)
	{
		auto msgboxTitle = std::make_shared<W_LABEL>();
		if (msgboxTitle) // silence a GCC warning ...
		{
			msgboxTitle->setFont(font_medium_bold, WZCOL_TEXT_BRIGHT);
			msgboxTitle->setString(title);
			msgboxTitle->setGeometry(MULTIOP_OKW, MULTIOP_OKH, msgboxTitle->getMaxLineWidth(), iV_GetTextLineSize(font_regular_bold));
			psBotForm->attach(msgboxTitle);
		}

		auto paragraph = std::make_shared<Paragraph>();
		const int msgboxTitleContentsPadding = 20;
		int msgboxTitleBottomY = msgboxTitle->y() + msgboxTitle->height();
		int msgboxContentsY = msgboxTitleBottomY + msgboxTitleContentsPadding;
		paragraph->setGeometry(MULTIOP_OKW, msgboxContentsY, FRONTEND_BOTFORMW - (MULTIOP_OKW * 2), FRONTEND_BOTFORMH - msgboxContentsY - (MULTIOP_OKH * 2));
		paragraph->setFontColour(WZCOL_TEXT_BRIGHT);
		paragraph->setLineSpacing(5);
		paragraph->setFont(font_regular);
		paragraph->addText(text.toUtf8());
		psBotForm->attach(paragraph);
	}
	else
	{
		debug(LOG_FATAL, "No bottom form?");
	}
}

TITLECODE WzMsgBoxTitleUI::run()
{
	screen_disableMapPreview();
	WidgetTriggers const &triggers = widgRunScreen(psWScreen);
	unsigned id = triggers.empty() ? 0 : triggers.front().widget->id;
	if (id == WZ_MSGBOX_TUI_LEAVE)
		changeTitleUI(next);
	widgDisplayScreen(psWScreen);
	return TITLECODE_CONTINUE;
}

