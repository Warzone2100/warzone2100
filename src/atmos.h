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

#ifndef __INCLUDED_SRC_ATMOS_H__
#define __INCLUDED_SRC_ATMOS_H__

#include "map.h"
#include "lib/framework/vector.h"
#include "lib/ivis_opengl/ivisdef.h"

/* Roughly one particle per tile */
#define MAX_ATMOS_PARTICLES     (MAP_MAXWIDTH * MAP_MAXHEIGHT)

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
    WT_NONE,
	WT_RAINING,
	WT_SNOWING
};

enum AP_TYPE
{
    AP_RAIN,
    AP_SNOW
};

class Atmosphere
{
public:
    static   Atmosphere& getInstance();
    void     renderParticle(ATPART *psPart);
    void     updateSystem();
    void     drawParticles(void);

    static   void     setWeatherType(WT_CLASS type);
    static   WT_CLASS getWeatherType(void);

private:
    ATPART asAtmosParts[MAX_ATMOS_PARTICLES];
    UDWORD freeParticle;

    Atmosphere();
    Atmosphere(Atmosphere const&);      // Don't Implement
    void operator=(Atmosphere const&);  // Don't implement

    void     testParticleWrap(ATPART *psPart);
    void     addParticle(const Vector3f &pos, AP_TYPE type);
    void     processParticle(ATPART *psPart);
};

#endif // __INCLUDED_SRC_ATMOS_H__
