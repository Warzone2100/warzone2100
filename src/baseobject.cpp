/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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

#include "baseobject.h"
#include "droid.h"
#include "projectile.h"
#include "structure.h"

static inline int32_t interpolateInt(int32_t v1, int32_t v2, uint32_t t1, uint32_t t2, uint32_t t)
{
	int32_t numer = t - t1, denom = t2 - t1;
	return v1 + (v2 - v1) * numer/denom;
}

static inline uint16_t interpolateAngle(uint16_t v1, uint16_t v2, uint32_t t1, uint32_t t2, uint32_t t)
{
	int32_t numer = t - t1, denom = t2 - t1;
	return v1 + angleDelta(v2 - v1) * numer/denom;
}

static Position interpolatePos(Position p1, Position p2, uint32_t t1, uint32_t t2, uint32_t t)
{
	Position ret = { interpolateInt(p1.x, p2.x, t1, t2, t),
	                 interpolateInt(p1.y, p2.y, t1, t2, t),
	                 interpolateInt(p1.z, p2.z, t1, t2, t)
	               };
	return ret;
}

Rotation interpolateRot(Rotation v1, Rotation v2, uint32_t t1, uint32_t t2, uint32_t t)
{
	Rotation rot = { interpolateAngle(v1.direction, v2.direction, t1, t2, t),
	                 interpolateAngle(v1.pitch,     v2.pitch,     t1, t2, t),
	                 interpolateAngle(v1.roll,      v2.roll,      t1, t2, t)
	               };
	return rot;
}

static SPACETIME interpolateSpacetime(SPACETIME st1, SPACETIME st2, uint32_t t)
{
	return constructSpacetime(interpolatePos(st1.pos, st2.pos, st1.time, st2.time, t), interpolateRot(st1.rot, st2.rot, st1.time, st2.time, t), t);
}

SPACETIME interpolateObjectSpacetime(const SIMPLE_OBJECT *obj, uint32_t t)
{
	switch (obj->type)
	{
		default:
			return GET_SPACETIME(obj);
		case OBJ_DROID:
			return interpolateSpacetime(((DROID *)obj)->prevSpacetime, GET_SPACETIME(obj), t);
		case OBJ_PROJECTILE:
			return interpolateSpacetime(((PROJECTILE *)obj)->prevSpacetime, GET_SPACETIME(obj), t);
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
}
