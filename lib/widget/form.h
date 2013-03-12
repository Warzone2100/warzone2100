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
 *  Definitions for the form functions.
 */

#ifndef __INCLUDED_LIB_WIDGET_FORM_H__
#define __INCLUDED_LIB_WIDGET_FORM_H__

#include "lib/widget/widget.h"


/* The standard form */
class W_FORM : public WIDGET
{
	Q_OBJECT

public:
	W_FORM(W_FORMINIT const *init);
	W_FORM(WIDGET *parent);

	void clicked(W_CONTEXT *psContext, WIDGET_KEY key);
	void highlightLost();
	void display(int xOffset, int yOffset);

	bool            disableChildren;        ///< Disable all child widgets if true
};

/* The clickable form data structure */
class W_CLICKFORM : public W_FORM
{
	Q_OBJECT

public:
	W_CLICKFORM(W_FORMINIT const *init);
	W_CLICKFORM(WIDGET *parent);

	void clicked(W_CONTEXT *psContext, WIDGET_KEY key);
	void released(W_CONTEXT *psContext, WIDGET_KEY key);
	void highlight(W_CONTEXT *psContext);
	void highlightLost();
	void display(int xOffset, int yOffset);

	unsigned getState();
	void setState(unsigned state);
	void setFlash(bool enable);
	void setTip(QString string);

	void setString(char const *stringUtf8) { WIDGET::setString(stringUtf8); }  // Unhide the WIDGET::setString(char const *) function...
	void setTip(char const *stringUtf8) { WIDGET::setTip(stringUtf8); }  // Unhide the WIDGET::setTip(char const *) function...

	UDWORD		state;					// Button state of the form
	QString         pTip;                   // Tip for the form
	SWORD HilightAudioID;				// Audio ID for form clicked sound
	SWORD ClickedAudioID;				// Audio ID for form hilighted sound
	WIDGET_AUDIOCALLBACK AudioCallback;	// Pointer to audio callback function
};

#endif // __INCLUDED_LIB_WIDGET_FORM_H__
