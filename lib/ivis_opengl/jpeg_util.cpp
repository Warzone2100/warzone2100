// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
