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
 *  Frontend Image Button
 */

#pragma once

#include "lib/widget/widget.h"
#include "lib/widget/button.h"

class WzFrontendImageButton : public W_BUTTON
{
protected:
	WzFrontendImageButton();
public:
	static std::shared_ptr<WzFrontendImageButton> make(optional<UWORD> frontendImgID);
	void setImage(optional<UWORD> frontendImgID);
	void setImageDimensions(int imageSize);
	void setPadding(int horizontalPadding, int verticalPadding);
	void setImageHorizontalOffset(int xOffset);
	void setCustomTextColors(optional<PIELIGHT> textColor, optional<PIELIGHT> highlightedTextColor);
	void setCustomImageColor(optional<PIELIGHT> color);
	void setBackgroundColor(optional<PIELIGHT> color);
	enum class BorderDrawMode
	{
		Never,
		Highlighted,
		Always
	};
	void setBorderDrawMode(BorderDrawMode mode);
public:
	void setString(WzString string) override;
	int32_t idealWidth() override;
	int32_t idealHeight() override;
protected:
	void display(int xOffset, int yOffset) override;
private:
	void recalcIdealWidth();
	bool shouldDisplayImage() const;
private:
	WzText wzText;
	optional<UWORD> frontendImgID = nullopt;
	optional<PIELIGHT> customTextColor = nullopt;
	optional<PIELIGHT> customTextHighlightColor = nullopt;
	optional<PIELIGHT> customImgColor = nullopt;
	optional<PIELIGHT> customBackgroundColor = nullopt;
	bool missingImage = false;
	int imageDimensions = 16;
	int cachedIdealWidth = 0;
	int lastWidgetWidth = 0;
	int horizontalPadding = 5;
	int verticalPadding = 3;
	int imageHorizontalOffset = 0;
	BorderDrawMode borderDrawMode = BorderDrawMode::Always;
};
