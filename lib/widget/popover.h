// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2025  Warzone 2100 Project

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
		Interactive,
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
