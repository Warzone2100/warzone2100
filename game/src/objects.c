/*
 * Objects.c
 *
 * The object system.
 *
 */


#include "frame.h"
#include "objects.h"


/* Initialise the object system */
BOOL objInitialise(void)
{
	if (!objmemInitialise())
	{
		return FALSE;
	}

	return TRUE;
}


/* Shutdown the object system */
BOOL objShutdown(void)
{
	objmemShutdown();
	
	return TRUE;
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
