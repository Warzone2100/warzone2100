/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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

#include "lib/ivis_opengl/ivisdef.h"

#include "widget.h"
#include "widgbase.h"
#include <map>
#include <functional>
#include <string>


class W_BUTTON : public WIDGET
{

public:
	struct Images
	{
		Images() {}
		Images(Image normal, Image down, Image highlighted, Image disabled = Image()) : normal(normal), down(down), highlighted(highlighted), disabled(disabled) {}

		Image normal;       ///< The image for the button.
		Image down;         ///< The image for the button, when down. Is overlaid over image.
		Image highlighted;  ///< The image for the button, when highlighted. Is overlaid over image.
		Image disabled;     ///< The image for the button, when disabled. Is overlaid over image.
	};

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
	WzString getString() const;
	void setString(WzString string);
	void setTip(std::string string);
	void unlock();

	void setImages(Images const &images);
	void setImages(Image image, Image imageDown, Image imageHighlight, Image imageDisabled = Image());

	using WIDGET::setString;
	using WIDGET::setTip;

	/* The optional "onClick" callback function */
	typedef std::function<void (W_BUTTON& button)> W_BUTTON_ONCLICK_FUNC;

	void addOnClickHandler(const W_BUTTON_ONCLICK_FUNC& onClickFunc);

public:
	UDWORD		state;				// The current button state
	WzString        pText;                          // The text for the button
	Images          images;                         ///< The images for the button.
	std::string     pTip;                           // The tool tip for the button
	SWORD HilightAudioID;				// Audio ID for form clicked sound
	SWORD ClickedAudioID;				// Audio ID for form hilighted sound
	WIDGET_AUDIOCALLBACK AudioCallback;	// Pointer to audio callback function
	iV_fonts        FontID;
private:
	std::vector<W_BUTTON_ONCLICK_FUNC> onClickHandlers;
};

class StateButton : public W_BUTTON
{

public:
	StateButton(WIDGET *parent) : W_BUTTON(parent), currentState(-1) {}
	void setState(int newStateValue);
	void setTip(int stateValue, const std::string& string);
	void setTip(int stateValue, char const *stringUtf8);
	void setImages(int stateValue, Images const &stateImages);

	using WIDGET::setTip;

private:
	int currentState;
	std::map<int, std::string> tips;
	std::map<int, Images> imageSets;
};

#endif // __INCLUDED_LIB_WIDGET_BUTTON_H__
