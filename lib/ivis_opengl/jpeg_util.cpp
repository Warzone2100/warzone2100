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

#include "jpeg_util.h"

#include <turbojpeg.h>

JPEGResult iV_loadImage_JPEG(std::vector<unsigned char>&& memoryBuffer, iV_Image *image)
{
#if SIZE_MAX > ULONG_MAX
	if (memoryBuffer.size() > std::numeric_limits<unsigned long>::max())
	{
		return JPEGResult("memoryBuffer size is too big");
	}
#endif

	// Initialize
	tjhandle decompressor = tjInitDecompress();

	// Get image info
	int width, height, subsamp;
	tjDecompressHeader2(decompressor, memoryBuffer.data(), static_cast<unsigned long>(memoryBuffer.size()), &width, &height, &subsamp);

	if (width <= 0 || height <= 0)
	{
		return JPEGResult("Invalid width and/or height");
	}
	if ((static_cast<uint32_t>(width) > WZ_JPEG_MAX_IMAGE_DIMENSIONS) || (static_cast<uint32_t>(height) > WZ_JPEG_MAX_IMAGE_DIMENSIONS))
	{
		return JPEGResult("Too big width and/or height");
	}

	// Decompress to 8-bit RGB
	if (!image->allocate(width, height, 3, true))
	{
		tjDestroy(decompressor);
		return JPEGResult("Failed to allocate memory");
	}

	tjDecompress2(decompressor, memoryBuffer.data(), static_cast<unsigned long>(memoryBuffer.size()), image->bmp_w(), width, 0, height, TJPF_RGB, TJFLAG_FASTDCT);

	// Cleanup
	tjDestroy(decompressor);

	return JPEGResult::NoError();
}
