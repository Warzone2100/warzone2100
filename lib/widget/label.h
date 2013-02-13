/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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


struct W_LABEL : public WIDGET
{
	W_LABEL(W_LABINIT const *init);

	void highlight(W_CONTEXT *psContext);
	void highlightLost();

	QString getString() const;
	void setString(QString string);
	void setTip(QString string);

	QString  aText;         // Text on the label
	iV_fonts FontID;
	QString  pTip;          // The tool tip for the button
};

/* label display function */
extern void labelDisplay(WIDGET *psWidget, UDWORD xOffset, UDWORD yOffset, PIELIGHT *pColours);

#endif // __INCLUDED_LIB_WIDGET_LABEL_H__
