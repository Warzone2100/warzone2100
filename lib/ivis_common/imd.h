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
#ifndef _imd_
#define _imd_


#include "ivisdef.h"

#define IMD_NAME				"IMD"
#define PIE_NAME				"PIE"  // Pumpkin image export data file
#define IMD_VER				1
#define PIE_VER				2



#ifdef BSPIMD

#define BSPPOLYID_MAXPOLYID (65534)
#define BSPPOLYID_TERMINATE (BSPPOLYID_MAXPOLYID+1)	// This is used as a terminator

#include "bspimd.h"
#include "lib/framework/frame.h"

#endif

//*************************************************************************

#define iV_IMD_MAX_POINTS	512 // increased from 256 - Per, May 20th 2006
#define iV_IMD_MAX_POLYS	512 // increased from 256 - Per, May 20th 2006

//*************************************************************************

// polygon flags	b0..b7: col, b24..b31: anim index


#define iV_IMD_TEX			0x00000200
#define iV_IMD_TEXANIM		0x00004000	// iV_IMD_TEX must be set also
#define iV_IMD_PSXTEX		0x00008000	// - use playstation texture allocation method
#define iV_IMD_BSPFRESH		0x00010000	// Freshly created by the BSP
#define iV_IMD_NOHALFPSXTEX 0x00020000

// shape override flags

// Are any of these really used anymore ?

#define iV_IMD_XEFFECT			0x1	// if this is set DO NOT PROCESS as a normal 3d pie ! (its a psx effect)
#define iV_IMD_XEFFECT_VPLANE	0x2	// Sprite effect drawn in view plane ( always facing player )
#define iV_IMD_XEFFECT_WPLANE	0x4	// Sprite effect drawn in world plane ( always parallel to terrain )
#define iV_IMD_XEFFECT_PROJECTILE	0x8	// Projectile effect.
#define iV_IMD_BINARY	0x10		// This Pie file was loaded as binary ... when freeing up do not free the pointers ... treat as one big chunk
#define iV_IMD_NOCULLSOME 0x20		// Some polys in the object are drawn without culling.

#define iV_IMD_XFLAT			0x00000100
#define iV_IMD_XTEX			0x00000200
#define iV_IMD_XWIRE			0x00000400
#define iV_IMD_XTRANS		0x00000800
#define iV_IMD_XGOUR			0x00001000
#define iV_IMD_XNOCUL		0x00002000
#define iV_IMD_XOUTLINE		0x00004000
#define iV_IMD_XFIXVIEW		0x00008000

// extended draw routines flags

#define iV_IMDX_AXES			0x00000001
#define iV_IMDX_BOUNDXZ		0x00000002
#define iV_IMDX_CHECKPOINT	0x00000004
#define iV_IMDX_POLYNUMBER	0x00000008
#define iV_IMDX_SPHERE		0x00000010
#define iV_IMDX_XGRID		0x00000020
#define iV_IMDX_YGRID		0x00000040
#define iV_IMDX_ZGRID		0x00000080
#define iV_IMDX_GRID			(iV_IMDX_XGRID | iV_IMDX_YGRID | iV_IMDX_ZGRID)


//*************************************************************************

extern BOOL iV_setImagePath(char *path);
extern iIMDShape *iV_IMDLoad(char *filename, BOOL palkeep);
extern iIMDShape *iV_ProcessIMD(char **ppFileData, char *FileDataEnd );
iIMDShape *iV_ProcessBPIE(iIMDShape *, UDWORD size);

extern BOOL iV_IMDSave(char *filename, iIMDShape *s, BOOL PieIMD);
extern void iV_IMDDebug(iIMDShape *s);

extern void iV_IMDRelease(iIMDShape *s);

// How high up do we want to stop looking
#define DROID_VIS_UPPER	100

// How low do we stop looking?
#define DROID_VIS_LOWER	10

#endif
