/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
#ifndef _imd_
#define _imd_

#include "ivisdef.h"

#define PIE_NAME				"PIE"  // Pumpkin image export data file
#define PIE_VER				2
#define PIE_FLOAT_VER		3

//*************************************************************************

// PIE model flags
// premultiplied implies additive
#define iV_IMD_NO_ADDITIVE     0x00000001
#define iV_IMD_ADDITIVE        0x00000002
#define iV_IMD_PREMULTIPLIED   0x00000004
// pitch to camera implies roll to camera
#define iV_IMD_ROLL_TO_CAMERA  0x00000010
#define iV_IMD_PITCH_TO_CAMERA 0x00000020
#define iV_IMD_NOSTRETCH       0x00001000
#define iV_IMD_TCMASK          0x00010000

// polygon flags	b0..b7: col, b24..b31: anim index

#define iV_IMD_TEX 0x00000200		// this is both a polygon and pie flag
#define iV_IMD_TEXANIM 0x00004000 // iV_IMD_TEX must be set also

//*************************************************************************

void modelShutdown();
iIMDShape *modelGet(const QString &filename);

#endif
