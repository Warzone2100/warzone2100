/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Freddie Witherden
	Copyright (C) 2008-2013  Warzone 2100 Project

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

#ifndef FONT_H_
#define FONT_H_

#include "internal-cairo.h"

/*
 * Forward declarations
 */
typedef struct _font font;

/**
 * The weight of a font. This can be either normal or bold.
 */
typedef enum
{
	FONT_WEIGHT_NORMAL,
	FONT_WEIGHT_BOLD
} fontWeight;

/**
 * The shape of a font. This can be either normal or italic.
 */
typedef enum
{
	FONT_SHAPE_NORMAL,
	FONT_SHAPE_ITALIC
} fontSHape;

/**
 * Represents a font.
 */
struct _font
{
	/// The name of the font
	const char *family;
	
	/// The size of the font, this should be in points (pt)
	float ptSize;
	
	/// The weight of the font (normal or bold)
	fontWeight weight;
	
	/// The shape of the font (normal or italic)
	fontSHape shape;
};

/**
 * Sets the font for cr to font.
 *
 * @param cr    The cairo context to set the font for.
 * @param font  The font to use.
 */
void widgetSetFont(cairo_t *cr, const font *font);

#endif /*FONT_H_*/

