/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project

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

#ifndef _LIBIVIS_COMMON_JPEG_UTIL_H_
#define _LIBIVIS_COMMON_JPEG_UTIL_H_

#include "pietypes.h"
#include <vector>

constexpr uint32_t WZ_JPEG_MAX_IMAGE_DIMENSIONS = 4096;

struct JPEGResult
{
	JPEGResult()
	{ }

	JPEGResult(std::string error)
	: text(error)
	{ }

	inline bool noError() const
	{
		return text.empty();
	}
	std::string text;

	static JPEGResult NoError() { return JPEGResult(); }
};

/*!
 * Load a JPEG from a memory buffer into an iV_Image
 *
 * \param memoryBuffer input memory buffer to load from
 * \param image Sprite to read into
 * \return true on success, false otherwise
 */
JPEGResult iV_loadImage_JPEG(std::vector<unsigned char>&& memoryBuffer, iV_Image *image);

#endif // _LIBIVIS_COMMON_JPEG_UTIL_H_
