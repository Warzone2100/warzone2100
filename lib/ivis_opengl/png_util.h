/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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

struct IMGSaveError
{
	IMGSaveError()
	{ }

	IMGSaveError(std::string error)
	: text(error)
	{ }

	inline bool noError() const
	{
		return text.empty();
	}
	std::string text;

	static IMGSaveError None;
};

/*!
 * Load a PNG from file into image
 *
 * \param fileName input file to load from
 * \param image Sprite to read into
 * \return true on success, false otherwise
 */
bool iV_loadImage_PNG(const char *fileName, iV_Image *image);

/*!
 * Save a PNG from image into file
 *
 * This function is safe to call from any thread
 *
 * \param fileName output file to save to
 * \param image Texture to read from
 * \return an IMGSaveError struct. On failure, its "text" contains a description of the error.
 */
IMGSaveError iV_saveImage_PNG(const char *fileName, const iV_Image *image);

/*!
 * Save a PNG from image into file using grayscale colour model.
 *
 * This function is safe to call from any thread
 *
 * \param fileName output file to save to
 * \param image Texture to read from
 * \return an IMGSaveError struct. On failure, its "text" contains a description of the error.
 */
IMGSaveError iV_saveImage_PNG_Gray(const char *fileName, const iV_Image *image);

void iV_saveImage_JPEG(const char *fileName, const iV_Image *image);

#endif // _LIBIVIS_COMMON_PNG_H_
