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
/** @file
 *  Definitions for multi but form functions.
 */

#ifndef __INCLUDED_LIB_WIDGET_MULTIBUTFORM_H__
#define __INCLUDED_LIB_WIDGET_MULTIBUTFORM_H__

#include "lib/ivis_opengl/ivisdef.h"

#include "widget.h"
#include "widgbase.h"
#include "form.h"
#include "label.h"
#include <map>
#include <functional>
#include <string>

class MultibuttonWidget : public W_FORM
{
public:
	enum class ButtonAlignment {
		RIGHT_ALIGN,
		CENTER_ALIGN
	};

public:
	MultibuttonWidget(int value = -1);

	virtual void display(int xOffset, int yOffset);
	virtual void geometryChanged();

	void setLabel(char const *text);
	void addButton(int value, const std::shared_ptr<W_BUTTON>& button);
	void setButtonMinClickInterval(UDWORD interval);
	void setButtonAlignment(ButtonAlignment alignment);
	void enable(bool enabled = true);
	void disable()
	{
		enable(false);
	}
	void setGap(int gap);
	int currentValue() const
	{
		return currentValue_;
	}

	/* The optional "onChoose" callback function */
	typedef std::function<void (MultibuttonWidget& widget, int newValue)> W_ON_CHOOSE_FUNC;

	void addOnChooseHandler(const W_ON_CHOOSE_FUNC& onChooseFunc);

public:
	void choose(int value);

private:
	void stateChanged();

protected:
	std::shared_ptr<W_LABEL> label;
	std::vector<std::pair<std::shared_ptr<W_BUTTON>, int>> buttons;
	int currentValue_;
	bool disabled;
	int gap_;
	bool lockCurrent;
	ButtonAlignment butAlign = ButtonAlignment::RIGHT_ALIGN;
	std::vector<W_ON_CHOOSE_FUNC> onChooseHandlers;
};

class MultichoiceWidget : public MultibuttonWidget
{

public:
	MultichoiceWidget(int value = -1);
};


#endif // __INCLUDED_LIB_WIDGET_MULTIBUTFORM_H__
