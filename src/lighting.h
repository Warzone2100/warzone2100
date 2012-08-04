/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_LIGHTNING_H__
#define __INCLUDED_SRC_LIGHTNING_H__

#include "lib/ivis_opengl/pietypes.h"

#define FOG_FLAGS		7
#define FOG_BACKGROUND	1
#define FOG_DISTANCE	2
#define FOG_GROUND		4
#define FOG_ENABLED		8

extern UDWORD fogStatus;

enum LIGHT_COLOUR
{
LIGHT_RED,
LIGHT_GREEN,
LIGHT_BLUE,
LIGHT_YELLOW,
LIGHT_WHITE
};

struct LIGHT
{
Vector3i	position;
UBYTE	type;
UDWORD	range;
LIGHT_COLOUR	colour;
};

extern void setTheSun(Vector3f newSun);
extern Vector3f getTheSun(void);

extern void	processLight(LIGHT *psLight);
extern void initLighting(UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2);
extern void	lightValueForTile(UDWORD tileX, UDWORD tileY);
extern void	doBuildingLights( void );
extern void UpdateFogDistance(float distance);
extern void	calcDroidIllumination(DROID *psDroid);

#endif // __INCLUDED_SRC_LIGHTNING_H__
