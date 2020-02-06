/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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

struct LIGHT
{
	Vector3i position = Vector3i(0, 0, 0);
	UDWORD range;
	PIELIGHT colour;
};

void setTheSun(Vector3f newSun);
Vector3f getTheSun();

void processLight(LIGHT *psLight);
void initLighting(UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2);
void lightValueForTile(UDWORD tileX, UDWORD tileY);
void doBuildingLights();
void UpdateFogDistance(float distance);
void calcDroidIllumination(DROID *psDroid);

#endif // __INCLUDED_SRC_LIGHTNING_H__
