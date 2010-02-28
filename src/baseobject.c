/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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

#include "lib/framework/frame.h"
#include "lib/framework/debug.h"
#include "baseobject.h"
#include "droid.h"
#include "projectile.h"
#include "structure.h"

static inline float interpolateFloat(float v1, float v2, uint32_t t1, uint32_t t2, uint32_t t)
{
	int32_t numer = t - t1, denom = t2 - t1;
	return v1 + (v2 - v1) * numer/denom;
}

Vector3i interpolatePos(Vector3i p1, Vector3i p2, uint32_t t1, uint32_t t2, uint32_t t)
{
	Vector3i ret = { interpolateInt(p1.x, p2.x, t1, t2, t),
	                 interpolateInt(p1.y, p2.y, t1, t2, t),
	                 interpolateInt(p1.z, p2.z, t1, t2, t)
	               };
	return ret;
}

float interpolateDirection(float v1, float v2, uint32_t t1, uint32_t t2, uint32_t t)
{
	if (v1 > v2 + 180)
	{
		v2 += 360;
	}
	else if(v2 > v1 + 180)
	{
		v1 += 360;
	}
	return interpolateFloat(v1, v2, t1, t2, t);
}

int16_t interpolateIntAngle(int16_t v1, int16_t v2, uint32_t t1, uint32_t t2, uint32_t t)
{
	int delta = (v2 - v1 + 360000 + 180)%360 - 180;  // delta: [-180; 179].
	return (interpolateInt(v1, v1 + delta, t1, t2, t) + 360000)%360;  // [0; 359]
}

SPACETIME interpolateSpacetime(SPACETIME st1, SPACETIME st2, uint32_t t)
{
	if (st1.time == st2.time)
	{
		debug(LOG_WARNING, "st1.time = %u, st2.time = %u, t = %u\n", (unsigned)st1.time, (unsigned)st2.time, (unsigned)t);
		return st2;
	}
	return constructSpacetime(interpolatePos(st1.pos, st2.pos, st1.time, st2.time, t),
	                          interpolateDirection(st1.direction, st2.direction, st1.time, st2.time, t),
	                          interpolateIntAngle(st1.pitch, st2.pitch, st1.time, st2.time, t),
	                          interpolateIntAngle(st1.roll, st2.roll, st1.time, st2.time, t),
	                          t
	                         );
}

SPACETIME interpolateObjectSpacetime(SIMPLE_OBJECT *obj, uint32_t t)
{
	switch (obj->type)
	{
		default:
			return GET_SPACETIME(obj);
		case OBJ_DROID:
			return interpolateSpacetime(((DROID *)obj)->prevSpacetime, GET_SPACETIME(obj), t);
	}
}

void checkObject(const BASE_OBJECT* psObject, const char * const location_description, const char * function, const int recurse)
{
	if (recurse < 0)
		return;

	ASSERT(psObject != NULL, "NULL pointer");

	switch (psObject->type)
	{
		case OBJ_DROID:
			checkDroid((const DROID *)psObject, location_description, function, recurse - 1);
			break;

		case OBJ_STRUCTURE:
			checkStructure((const STRUCTURE *)psObject, location_description, function, recurse - 1);
			break;

		case OBJ_PROJECTILE:
			checkProjectile((const PROJECTILE *)psObject, location_description, function, recurse - 1);
			break;

		case OBJ_FEATURE:
		case OBJ_TARGET:
			break;

		default:
			ASSERT_HELPER(!"invalid object type", location_description, function, "CHECK_OBJECT: Invalid object type (type num %u)", (unsigned int)psObject->type);
			break;
	}

	ASSERT_HELPER(psObject->type == OBJ_FEATURE
	    || psObject->type == OBJ_TARGET
	    || psObject->player < MAX_PLAYERS,
	       location_description, function, "CHECK_OBJECT: Out of bound owning player number (%u)", (unsigned int)psObject->player);

	ASSERT_HELPER(psObject->direction <= 360.0f
	    && psObject->direction >= 0.0f,
	       location_description, function, "CHECK_OBJECT: Out of range direction (%f)", (float)psObject->direction);
}
