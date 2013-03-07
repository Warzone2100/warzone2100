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
 *  Definitions for edit box functions.
 */

#ifndef __INCLUDED_LIB_WIDGET_BUTTON_H__
#define __INCLUDED_LIB_WIDGET_BUTTON_H__

#include "widget.h"
#include "widgbase.h"
#include "lib/ivis_opengl/textdraw.h"


class W_BUTTON : public WIDGET
{
	Q_OBJECT

public:
	W_BUTTON(W_BUTINIT const *init);
	W_BUTTON(WIDGET *parent);

	void clicked(W_CONTEXT *psContext, WIDGET_KEY key);
	void released(W_CONTEXT *psContext, WIDGET_KEY key);
	void highlight(W_CONTEXT *psContext);
	void highlightLost();
	void display(int xOffset, int yOffset);

	unsigned getState();
	void setState(unsigned state);
	void setFlash(bool enable);
	QString getString() const;
	void setString(QString string);
	void setTip(QString string);

	void setImages(Image image, Image imageDown, Image imageHighlight, Image imageDisabled = Image());

	void setString(char const *stringUtf8) { WIDGET::setString(stringUtf8); }  // Unhide the WIDGET::setString(char const *) function...
	void setTip(char const *stringUtf8) { WIDGET::setTip(stringUtf8); }  // Unhide the WIDGET::setString(char const *) function...

signals:
	void clicked();

public:
	UDWORD		state;				// The current button state
	QString         pText;                          // The text for the button
	Image           image;                          ///< The image for the button.
	Image           imageDown;                      ///< The image for the button, when down. Is overlaid over image.
	Image           imageDisabled;                  ///< The image for the button, when disabled. Is overlaid over image.
	Image           imageHighlight;                 ///< The image for the button, when highlighted. Is overlaid over image.
	QString         pTip;                           // The tool tip for the button
	SWORD HilightAudioID;				// Audio ID for form clicked sound
	SWORD ClickedAudioID;				// Audio ID for form hilighted sound
	WIDGET_AUDIOCALLBACK AudioCallback;	// Pointer to audio callback function
	iV_fonts        FontID;
};

#endif // __INCLUDED_LIB_WIDGET_BUTTON_H__
