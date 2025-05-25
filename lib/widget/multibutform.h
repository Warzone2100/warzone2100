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

	virtual void display(int xOffset, int yOffset) override;
	virtual void geometryChanged() override;
	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;

	void setLabel(char const *text);
	void setLabel(const std::shared_ptr<WIDGET>& widget);
	void addButton(int value, const std::shared_ptr<W_BUTTON>& button);
	void clearButtons();
	size_t numButtons() const;
	void setButtonMinClickInterval(UDWORD interval);
	void setButtonAlignment(ButtonAlignment alignment);
	void enable(bool enabled = true);
	void disable()
	{
		enable(false);
	}
	bool isDisabled() { return disabled; }
	void setGap(int gap);
	int getGap() const { return gap_; }
	void setOuterPaddingX(int left, int right);
	int currentValue() const
	{
		return currentValue_;
	}

	std::shared_ptr<W_BUTTON> getButtonByValue(int value);

	/* The optional "canChoose" callback function */
	typedef std::function<bool (MultibuttonWidget& widget, int newValue)> W_CAN_CHOOSE_FUNC;

	void setCanChooseHandler(const W_CAN_CHOOSE_FUNC& canChooseFunc);

	/* The optional "onChoose" callback function */
	typedef std::function<void (MultibuttonWidget& widget, int newValue)> W_ON_CHOOSE_FUNC;

	void addOnChooseHandler(const W_ON_CHOOSE_FUNC& onChooseFunc);

protected:
	virtual void stateChanged();

public:
	void choose(int value);

private:
	int32_t widthOfAllButtons() const;
	void internalHandleButtonClick(int value);

protected:
	std::shared_ptr<WIDGET> label;
	std::vector<std::pair<std::shared_ptr<W_BUTTON>, int>> buttons;
	int currentValue_;
	bool disabled;
	int gap_;
	int outerPaddingLeft_;
	int outerPaddingRight_;
	bool lockCurrent;
	ButtonAlignment butAlign = ButtonAlignment::RIGHT_ALIGN;
	std::vector<W_ON_CHOOSE_FUNC> onChooseHandlers;
	W_CAN_CHOOSE_FUNC canChooseHandler;
};

class MultichoiceWidget : public MultibuttonWidget
{

public:
	MultichoiceWidget(int value = -1);
};


#endif // __INCLUDED_LIB_WIDGET_MULTIBUTFORM_H__
