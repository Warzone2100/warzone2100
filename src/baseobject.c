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
