// A fixed-up version of the PHYSFS_mountMemory function that properly calls the destructor func
//
// WZ Modifications:
//  - Fix calling the destructor func
//  - Some minor tweaks to use std::atomic, `new` for __PHYSFS_MemoryIoInfo2
//
// Copyright (c) 2021 Warzone 2100 Project
// Licensed under the same terms as the original license (below).
//
// Original License:
//
//	Copyright (c) 2001-2021 Ryan C. Gordon <icculus@icculus.org> and others.
//
//	This software is provided 'as-is', without any express or implied
//	warranty.  In no event will the authors be held liable for any damages
//	arising from the use of this software.
//
//	Permission is granted to anyone to use this software for any purpose,
//	including commercial applications, and to alter it and redistribute it
//	freely, subject to the following restrictions:
//
//	1. The origin of this software must not be misrepresented; you must not
//	   claim that you wrote the original software. If you use this software
//	   in a product, an acknowledgment in the product documentation would be
//	   appreciated but is not required.
//	2. Altered source versions must be plainly marked as such, and must not be
//	   misrepresented as being the original software.
//	3. This notice may not be removed or altered from any source distribution.
//

#pragma once

#include <physfs.h>

#if PHYSFS_VER_MAJOR >= 3 || (PHYSFS_VER_MAJOR == 2 && (PHYSFS_VER_MINOR > 1) || PHYSFS_VER_MINOR == 1 && PHYSFS_VER_PATCH >= 1)
#define HAS_PHYSFS_IO_SUPPORT
#endif

int PHYSFS_mountMemory_fixed(const void *buf, PHYSFS_uint64 len, void (*del)(void *),
						const char *fname, const char *mountPoint,
						int appendToPath);
