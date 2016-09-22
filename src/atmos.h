/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_ATMOS_H__
#define __INCLUDED_SRC_ATMOS_H__

#include "lib/framework/vector.h"
#include "lib/ivis_opengl/ivisdef.h"

struct ATPART
{
	UBYTE		status;
	UBYTE		type;
	UDWORD		size;
	Vector3f	position;
	Vector3f	velocity;
	iIMDShape	*imd;
};

enum WT_CLASS
{
	WT_RAINING,
	WT_SNOWING,
	WT_NONE
};

void atmosInitSystem(void);
void atmosUpdateSystem(void);
void renderParticle(ATPART *psPart, const glm::mat4 &viewMatrix);
void atmosDrawParticles(const glm::mat4 &viewMatrix);
void atmosSetWeatherType(WT_CLASS type);
WT_CLASS atmosGetWeatherType(void);

#endif // __INCLUDED_SRC_ATMOS_H__
