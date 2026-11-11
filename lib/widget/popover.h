// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2025-2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/
/**
 * @file
 * Definitions for popover widget
 */

#ifndef __INCLUDED_LIB_WIDGET_POPOVER_H__
#define __INCLUDED_LIB_WIDGET_POPOVER_H__

#include "widget.h"
#include <functional>

class PopoverWidget : public WIDGET
{
protected:
	PopoverWidget();

public:
	typedef std::function<void ()> OnPopoverCloseHandlerFunc;

	enum class Style
	{
		Interactive,	// covers the whole screen: everything beneath stops receiving mouse and keyboard events
		// Accepts events on its own contents, ex. buttons, while everything
		// outside it falls through to whatever is underneath. Nothing dismisses it
		// on a click elsewhere or on Esc, so whoever opened it has to close it.
		MouseInteractive,
		NonInteractive	// akin to a tooltip (accepts no mouse-over or other events, expects that something else will call close() on it at some point)
	};

	enum class Alignment
	{
		LeftOfParent,
		RightOfParent
	};

	static std::shared_ptr<PopoverWidget> makePopover(const std::shared_ptr<WIDGET>& parent, std::shared_ptr<WIDGET> popoverContents, Style style = Style::NonInteractive, Alignment align = Alignment::LeftOfParent, Vector2i positionOffset = Vector2i(0, 0), const OnPopoverCloseHandlerFunc& onCloseHandler = nullptr);

	virtual ~PopoverWidget();

	virtual void display(int xOffset, int yOffset) override;
	virtual void geometryChanged() override;
	void close();

	virtual int32_t idealWidth() override;
	virtual int32_t idealHeight() override;

private:
	void open(const std::shared_ptr<WIDGET>& parent);
	void closeImpl();
	void repositionOnScreenRelativeToParent(const std::shared_ptr<WIDGET>& parent);

private:
	std::weak_ptr<W_SCREEN> overlayScreen;
	std::shared_ptr<WIDGET> popoverContents;
	OnPopoverCloseHandlerFunc onCloseHandler;
	Style style;
	Alignment align;
	Vector2i positionOffset;
};

#endif // __INCLUDED_LIB_WIDGET_POPOVER_H__
