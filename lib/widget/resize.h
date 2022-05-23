/*
	This file is part of Warzone 2100.
	Copyright (C) 2020  Warzone 2100 Project

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

#ifndef __INCLUDED_LIB_WIDGET_RESIZE_H__
#define __INCLUDED_LIB_WIDGET_RESIZE_H__

#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "widget.h"
#include <nonstd/optional.hpp>

class ResizeWidget;
struct Resize
{
	static Resize width(nonstd::optional<int32_t> min, nonstd::optional<int32_t> max = nonstd::nullopt)
	{
		Resize resize;
		resize.minWidth = min;
		resize.maxWidth = max;
		return resize;
	}

	static Resize height(nonstd::optional<int32_t> min, nonstd::optional<int32_t> max = nonstd::nullopt)
	{
		Resize resize;
		resize.minHeight = min;
		resize.maxHeight = max;
		return resize;
	}

	static Resize fixed(int32_t width, int32_t height)
	{
		return Resize{width, width, height, height};
	}

    std::shared_ptr<ResizeWidget> wrap(std::shared_ptr<WIDGET> widget);

    nonstd::optional<int32_t> minWidth;
    nonstd::optional<int32_t> maxWidth;
    nonstd::optional<int32_t> minHeight;
    nonstd::optional<int32_t> maxHeight;
};

class ResizeWidget: public WIDGET
{
public:
    explicit ResizeWidget(Resize resize): resize(resize)
	{
		setTransparentToMouse(true);
	}

protected:
    void geometryChanged() override;
    int32_t idealWidth() override;
    int32_t idealHeight() override;

private:
    Resize resize;
};

#endif // __INCLUDED_LIB_WIDGET_RESIZE_H__
