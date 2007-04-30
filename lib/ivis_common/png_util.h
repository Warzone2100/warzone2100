/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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

#ifndef _LIBIVIS_COMMON_PNG_H_
#define _LIBIVIS_COMMON_PNG_H_

#include "pietypes.h"

/*!
 * Load a PNG from file into texture
 *
 * \param fileName input file to load from
 * \param sprite Sprite to read into
 * \return TRUE on success, FALSE otherwise
 */
BOOL pie_PNGLoadFile(const char *fileName, iTexture *s);

/*!
 * Save a PNG from texture into file
 *
 * \param fileName output file to save to
 * \param sprite Texture to read from
 * \return TRUE on success, FALSE otherwise
 */
void pie_PNGSaveFile(const char *fileName, iTexture *s);

#endif // _LIBIVIS_COMMON_PNG_H_
