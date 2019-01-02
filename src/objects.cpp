/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2019  Warzone 2100 Project

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
/*
 * Objects.c
 *
 * The object system.
 *
 */


#include "lib/framework/frame.h"
#include "objects.h"


/* Initialise the object system */
bool objInitialise()
{
	if (!objmemInitialise())
	{
		return false;
	}

	return true;
}


/* Shutdown the object system */
bool objShutdown()
{
	objmemShutdown();

	return true;
}


/*goes thru' the list passed in reversing the order so the first entry becomes
the last and the last entry becomes the first!*/
void reverseObjectList(BASE_OBJECT **ppsList)
{
	BASE_OBJECT *psPrev = nullptr;
	BASE_OBJECT *psCurrent = *ppsList;

	while (psCurrent != nullptr)
	{
		BASE_OBJECT *psNext = psCurrent->psNext;
		psCurrent->psNext = psPrev;
		psPrev = psCurrent;
		psCurrent = psNext;
	}
	//set the list passed in to point to the new top
	*ppsList = psPrev;
}

const char *objInfo(const BASE_OBJECT *psObj)
{
	static char	info[PATH_MAX];

	if (!psObj)
	{
		return "null";
	}

	switch (psObj->type)
	{
	case OBJ_DROID:
		{
			const DROID *psDroid = (const DROID *)psObj;
			return droidGetName(psDroid);
		}
	case OBJ_STRUCTURE:
		{
			const STRUCTURE *psStruct = (const STRUCTURE *)psObj;
			sstrcpy(info, getName(psStruct->pStructureType));
			break;
		}
	case OBJ_FEATURE:
		{
			const FEATURE *psFeat = (const FEATURE *)psObj;
			sstrcpy(info, getName(psFeat->psStats));
			break;
		}
	case OBJ_PROJECTILE:
		sstrcpy(info, "Projectile");	// TODO
		break;
	case OBJ_TARGET:
		sstrcpy(info, "Target");	// TODO
		break;
	default:
		sstrcpy(info, "Unknown object type");
		break;
	}
	return info;
}
