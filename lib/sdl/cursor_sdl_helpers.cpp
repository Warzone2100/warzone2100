// SPDX-License-Identifier: Zlib
/*
 Adapted version of some code from SDL_mouse.c
	Modifications: Copyright (C) 2022 Warzone 2100 Project

 License:

	Copyright (C) 1997-2022 Sam Lantinga <slouken@libsdl.org>

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	claim that you wrote the original software. If you use this software
	in a product, an acknowledgment in the product documentation would be
	appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.

*/

#include "cursor_sdl_helpers.h"

// Implement the same algorithm from: https://github.com/libsdl-org/SDL/blob/main/src/events/SDL_mouse.c
bool ivImageFromMonoCursorDataSDLCompat(const uint8_t* data, const uint8_t* mask, unsigned int w, unsigned int h, iV_Image& image)
{
	unsigned char* pWritePtr = nullptr;
	uint8_t datab = 0, maskb = 0;
	const uint32_t black = 0xFF000000;
	const uint32_t white = 0xFFFFFFFF;
	const uint32_t transparent = 0x00000000;

	if (!image.allocate(w, h, 4, true))
	{
		return false;
	}

	auto pBmpData = image.bmp_w();
	for (unsigned int y = 0; y < h; ++y) {
		pWritePtr = pBmpData + (y * w * 4);
		for (unsigned int x = 0; x < w; ++x) {
			if ((x % 8) == 0) {
				datab = *data++;
				maskb = *mask++;
			}
			const uint32_t* pColor = &black;
			if (maskb & 0x80) {
				pColor = (datab & 0x80) ? &black : &white;
			} else {
				pColor = (datab & 0x80) ? &black : &transparent;
			}
			pWritePtr[0] = reinterpret_cast<const unsigned char*>(pColor)[0];
			pWritePtr[1] = reinterpret_cast<const unsigned char*>(pColor)[1];
			pWritePtr[2] = reinterpret_cast<const unsigned char*>(pColor)[2];
			pWritePtr[3] = reinterpret_cast<const unsigned char*>(pColor)[3];
			pWritePtr += 4;

			datab <<= 1;
			maskb <<= 1;
		}
	}

	return true;
}
