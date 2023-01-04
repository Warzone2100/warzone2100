/*
	This file is part of Warzone 2100.
	Copyright (C) 2021  Warzone 2100 Project

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

#ifndef __INCLUDED_LIB_WIDGET_IMAGE_H__
#define __INCLUDED_LIB_WIDGET_IMAGE_H__

#include "lib/ivis_opengl/bitimage.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "widget.h"

class ImageWidget: public WIDGET
{
public:
    ImageWidget(const AtlasImageDef *image): image(image)
    {
    }

    int32_t idealWidth() override
    {
        return image ? image->Width : 0;
    }

    int32_t idealHeight() override
    {
        return image ? image->Height : 0;
    }

protected:
    void display(int xOffset, int yOffset) override
    {
        if (image && width() > 0 && height() > 0)
        {
            auto x0 = xOffset + x();
            auto y0 = yOffset + y();
            iV_DrawImage2(image, x0, y0, width(), height());
        }
    }

private:
    const AtlasImageDef *image;
};

#endif // __INCLUDED_LIB_WIDGET_IMAGE_H__
