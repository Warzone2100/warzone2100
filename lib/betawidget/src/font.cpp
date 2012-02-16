/*
	This file is part of Warzone 2100.
	Copyright (C) 2008  Freddie Witherden
	Copyright (C) 2008-2012  Warzone 2100 Project

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

#include "font.h"

#include <assert.h>

void widgetSetFont(cairo_t *cr, const font *font)
{
	const char *family = font->family;
	float ptSize = font->ptSize;
	cairo_font_weight_t weight;
	cairo_font_slant_t shape;
	
	// Convert from fontWeight to cairo_font_weight_t
	switch (font->weight)
	{
		case FONT_WEIGHT_NORMAL:
			weight = CAIRO_FONT_WEIGHT_NORMAL;
			break;
		case FONT_WEIGHT_BOLD:
			weight = CAIRO_FONT_WEIGHT_BOLD;
			break;
		default:
			assert(!"Invalid font weight");
			break;
	}
	
	// Convert from fontShape to cairo_font_slant_t
	switch (font->shape)
	{
		case FONT_SHAPE_NORMAL:
			shape = CAIRO_FONT_SLANT_NORMAL;
			break;
		case FONT_SHAPE_ITALIC:
			shape = CAIRO_FONT_SLANT_ITALIC;
			break;
		default:
			assert(!"Invalid font shape");
			break;
	}
	
	// Set the face
	cairo_select_font_face(cr, family, shape, weight);
	
	// Set the size
	cairo_set_font_size(cr, ptSize);
}
