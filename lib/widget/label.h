/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
 *  Definitions for the label widget.
 */

#ifndef __INCLUDED_LIB_WIDGET_LABEL_H__
#define __INCLUDED_LIB_WIDGET_LABEL_H__

#include "widget.h"
#include "widgbase.h"
#include "lib/ivis_opengl/textdraw.h"


class W_LABEL : public WIDGET
{
	Q_OBJECT

public:
	W_LABEL(W_LABINIT const *init);
	W_LABEL(WIDGET *parent);

	void highlight(W_CONTEXT *psContext);
	void highlightLost();
	void display(int xOffset, int yOffset);

	QString getString() const;
	void setString(QString string);
	void setTip(QString string);

	void setFont(iV_fonts font) { FontID = font; }
	void setFontColour(PIELIGHT colour) { fontColour = colour; }
	void setFont(iV_fonts font, PIELIGHT colour) { setFont(font); setFontColour(colour); }
	void setTextAlignment(WzTextAlignment align);

	void setString(char const *stringUtf8) { WIDGET::setString(stringUtf8); }  // Unhide the WIDGET::setString(char const *) function...
	void setTip(char const *stringUtf8) { WIDGET::setTip(stringUtf8); }  // Unhide the WIDGET::setTip(char const *) function...

	QString  aText;         // Text on the label
	iV_fonts FontID;
	QString  pTip;          // The tool tip for the button

private:
	PIELIGHT fontColour;
};

#endif // __INCLUDED_LIB_WIDGET_LABEL_H__
