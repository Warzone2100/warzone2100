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

#ifndef __INCLUDED_SRC_EDIT3D_H__
#define __INCLUDED_SRC_EDIT3D_H__

#include "map.h"

#define TILE_RAISE	1
#define TILE_LOWER	-1
#define MAX_TILE_HEIGHT 255
#define MIN_TILE_HEIGHT	0

typedef void (*BUILDCALLBACK)(UDWORD xPos, UDWORD yPos, void *UserData);

void Edit3DInitVars();
bool found3DBuilding(UDWORD *x, UDWORD *y);
bool found3DBuildLocTwo(UDWORD *px1, UDWORD *py1, UDWORD *px2, UDWORD *py2);
void init3DBuilding(BASE_STATS *psStats, BUILDCALLBACK CallBack, void *UserData);
void kill3DBuilding();
bool process3DBuilding();

void adjustTileHeight(MAPTILE *psTile, SDWORD adjust);
void raiseTile(int tile3dX, int tile3dY);
void lowerTile(int tile3dX, int tile3dY);
bool inHighlight(UDWORD realX, UDWORD realY);

struct HIGHLIGHT
{
	UWORD	xTL, yTL;		// Top left of box to highlight
	UWORD	xBR, yBR;		// Bottom right of box to highlight
};

extern HIGHLIGHT	buildSite;


#define BUILD3D_NONE		99
#define BUILD3D_POS			100
#define BUILD3D_FINISHED	101
#define BUILD3D_VALID		102


struct BUILDDETAILS
{
	BUILDCALLBACK	CallBack;
	void 			*UserData;  //this holds the OBJECT_POSITION pointer for a Deliv Point
	UDWORD			x, y;
	UDWORD			width, height;
	BASE_STATS		*psStats;
};

extern BUILDDETAILS	sBuildDetails;

extern UDWORD buildState;
extern UDWORD temp;
extern int brushSize;
extern bool quickQueueMode;

/*returns true if the build state is not equal to BUILD3D_NONE*/
bool   tryingToGetLocation();

#endif // __INCLUDED_SRC_EDIT3D_H__
