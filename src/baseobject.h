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
/** \file
 *  Functions for the base object type.
 */

#ifndef __INCLUDED_BASEOBJECT_H__
#define __INCLUDED_BASEOBJECT_H__

#include "basedef.h"

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

static const unsigned int max_check_object_recursion = 4;

static inline unsigned interpolateInt(int32_t v1, int32_t v2, uint32_t t1, uint32_t t2, uint32_t t)
{
	int32_t numer = t - t1, denom = t2 - t1;
	return v1 + (v2 - v1) * numer/denom;
}

/// Get interpolated position at time t.
Vector3uw interpolatePos(Vector3uw p1, Vector3uw p2, uint32_t t1, uint32_t t2, uint32_t t);
/// Get interpolated direction at time t.
float interpolateDirection(float v1, float v2, uint32_t t1, uint32_t t2, uint32_t t);
/// Get interpolated pitch or roll at time t.
int16_t interpolateIntAngle(int16_t v1, int16_t v2, uint32_t t1, uint32_t t2, uint32_t t);
/// Get interpolated spacetime at time t.
SPACETIME interpolateSpacetime(SPACETIME st1, SPACETIME st2, uint32_t t);
/// Get interpolated object spacetime at time t.
SPACETIME interpolateObjectSpacetime(SIMPLE_OBJECT *obj, uint32_t t);

void checkObject(const BASE_OBJECT* psObject, const char * const location_description, const char * function, const int recurse);

/* assert if object is bad */
#define CHECK_OBJECT(object) checkObject((object), AT_MACRO, __FUNCTION__, max_check_object_recursion)

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __INCLUDED_BASEOBJECT_H__
