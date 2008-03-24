/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
BOOL objInitialise(void)
{
	if (!objmemInitialise())
	{
		return false;
	}

	return true;
}


/* Shutdown the object system */
BOOL objShutdown(void)
{
	objmemShutdown();

	return true;
}


/*goes thru' the list passed in reversing the order so the first entry becomes
the last and the last entry becomes the first!*/
void reverseObjectList(BASE_OBJECT **ppsList)
{
    BASE_OBJECT     *psPrev, *psNext, *psCurrent, *psObjList;

    //initialise the pointers
    psObjList = *ppsList;
    psPrev = psNext = NULL;
    psCurrent = psObjList;

    while(psCurrent != NULL)
    {
        psNext = psCurrent->psNext;
        psCurrent->psNext = psPrev;
        psPrev = psCurrent;
        psCurrent = psNext;
    }
    //set the list passed in to point to the new top
    *ppsList = psPrev;
}
