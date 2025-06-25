/*
	This file is part of Warzone 2100.
	Copyright (C) 2024-2025  Warzone 2100 Project

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
 *  Advanced Fancy Checkbox
 */

#pragma once

#include "lib/widget/widget.h"
#include "lib/widget/button.h"
#include "lib/widget/paragraph.h"

class WzAdvCheckbox : public W_BUTTON
{
protected:
	WzAdvCheckbox();
	void initialize(const WzString& displayName, const WzString& description);
public:
	static std::shared_ptr<WzAdvCheckbox> make(const WzString& displayName, const WzString& description);
	void setImageDimensions(int imageSize);
	void setOuterHorizontalPadding(int padding);
	void setInnerHorizontalPadding(int padding);
	void setOuterVerticalPadding(int padding);
	void setInnerVerticalPadding(int padding);
	bool isChecked() const;
	void setIsChecked(bool val);
public:
	void setString(WzString string) override;
	void setDescription(const WzString& description);
	int32_t idealWidth() override;
	int32_t idealHeight() override;
protected:
	void display(int xOffset, int yOffset) override;
	void geometryChanged() override;
	void highlight(W_CONTEXT *psContext) override;
	void highlightLost() override;
	void released(W_CONTEXT *, WIDGET_KEY key) override;
private:
	void recalcIdealWidth();
private:
	std::shared_ptr<Paragraph> descriptionWidget;
	WzText wzText;
	int imageDimensions = 16;
	int cachedIdealWidth = 0;
	int lastWidgetWidth = 0;
	int outerHorizontalPadding = 5;
	int innerHorizontalPadding = 10;
	int outerVerticalPadding = 5;
	int innerVerticalPadding = 5;
	bool checked = false;
};
