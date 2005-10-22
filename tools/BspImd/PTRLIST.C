/***************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "frame.h"
#include "imd.h"
#include "ivi.h"	// for the typedefs
#include "bspimd.h"

#include "ptrlist.h"
#include "mem.h"



/***************************************************************************/
//  Initialize list to empty
/***************************************************************************/

void
list_Create( PSBSPPTRLIST *ppList )
{
	(*ppList) = (PSBSPPTRLIST) MALLOC( sizeof(BSPPTRLIST) );

	(*ppList)->pHead = NULL;
	(*ppList)->pTail = NULL;
	(*ppList)->pCurPosition = NULL;
	(*ppList)->iNumNodes = 0;
}

/***************************************************************************/
//  Free each node. Does not free the app data pointed to by each node.
/***************************************************************************/

void
list_Empty( PSBSPPTRLIST pList )
{
    PSNODE  pCur, pNext;

    pCur = pList->pHead;
    pList->pHead = pList->pTail = pList->pCurPosition = NULL;

    // Go through list and free each node
    while (pCur != NULL)
    {
        pNext = pCur->pNext;
		FREE( pCur->pData );
		FREE( pCur );
        pCur = pNext;
    }

	// update counter
	pList->iNumNodes = 0;
}   


void list_Free( PSBSPPTRLIST pList )
{
    PSNODE  pCur, pNext;

    pCur = pList->pHead;
    pList->pHead = pList->pTail = pList->pCurPosition = NULL;

    // Go through list and free each node
    while (pCur != NULL)
    {
        pNext = pCur->pNext;
		FREE( pCur );
        pCur = pNext;
    }

	// update counter
	pList->iNumNodes = 0;
}   


/***************************************************************************/
//  Find - find the node with the specified app data attached to it.
/***************************************************************************/

PSNODE
list_Find( PSBSPPTRLIST pList, void  *pData)
{
    PSNODE  pCur;

    // go through list until we reach end or we find the right node.
	for ( pCur = pList->pHead; (pCur != NULL) && (pCur->pData != pData); pCur = pCur->pNext );

    return(pCur);
}

/***************************************************************************/
//  GetFirst - return app data for first entry in list and make it
//  the current node.
/***************************************************************************/

void *
list_GetFirst( PSBSPPTRLIST pList )
{
    pList->pCurPosition = pList->pHead;

    if (pList->pCurPosition == NULL)
    {
        return(NULL);
    }

    return(pList->pCurPosition->pData);
}

/***************************************************************************/
//  GetLast - return app data to last entry in list and make it
//  the current node.
/***************************************************************************/

void *
list_GetLast( PSBSPPTRLIST pList )
{
    pList->pCurPosition = pList->pTail;
    if (pList->pCurPosition == NULL)
    {
        return(NULL);
    }

    return(pList->pCurPosition->pData);
}

/***************************************************************************/
//  GetNext - return next app data entry in list and make it
//  the current node.
/***************************************************************************/

void *
list_GetNext( PSBSPPTRLIST pList, void *pData )
{
	PSNODE	pNext;

    pList->pCurPosition = list_Find( pList, pData);

    // check for empty list or already at end of list.
    if ((pList->pCurPosition == NULL) || (pList->pCurPosition->pNext == NULL))
    {
        return(NULL);
    }

    pNext = pList->pCurPosition->pNext;
    pList->pCurPosition = pNext;
    return(pNext->pData);
}

/***************************************************************************/
//  GetPrev - return app data for previous entry in list and make it
//  the current node.
/***************************************************************************/

void *
list_GetPrev( PSBSPPTRLIST pList, void *pData)
{
    PSNODE  pPrev;

    pList->pCurPosition = list_Find( pList, pData );

    // check for empty list or already at start
    if ((pList->pCurPosition == NULL) || (pList->pCurPosition->pPrev == NULL))
    {
        return(NULL);
    }

    pPrev = pList->pCurPosition->pPrev;
    pList->pCurPosition = pPrev;
    return(pPrev->pData);
}

/***************************************************************************/
//  Add - create a new node and put it at the start of the list and
//  make it the current node.
/***************************************************************************/

void
list_Add( PSBSPPTRLIST pList, void *pData )
{
    PSNODE  pNew = (PSNODE) MALLOC( sizeof(NODE) );

    // setup node and prepare it for its role as the new head of the list
    pNew->pData = pData;
    pNew->pNext = pList->pHead;
    pNew->pPrev = NULL;

    // The old head of list (if any) should point to new node)
    if (pList->pHead != NULL)
        pList->pHead->pPrev = pNew;

    // Make new node the head and current position
    pList->pHead = pNew;
    pList->pCurPosition = pNew;

    // Check to see if new node is also the tail (ie. only list entry)
    if (pList->pTail == NULL)
        pList->pTail = pNew;

	// update counter
	pList->iNumNodes++;
}

/***************************************************************************/
//  Append - create a new node and put it at the end of the list.
/***************************************************************************/

void
list_Append( PSBSPPTRLIST pList, void *pData)
{
    PSNODE  pNew = (PSNODE) MALLOC( sizeof(NODE) );

    // setup node and prepare it for its role as the new tail of the list
    pNew->pData = pData;
    pNew->pPrev = pList->pTail;
    pNew->pNext = NULL;

    // The old tail of list (if any) should point to new node.
    if (pList->pTail != NULL)
        pList->pTail->pNext = pNew;

    // Make new node the tail
    pList->pTail = pNew;

    // Check to see if new node is also the head (ie. only list entry)
    if (pList->pHead == NULL)
    {
        pList->pHead = pNew;
        pList->pCurPosition = pNew;
    }

	// update counter
	pList->iNumNodes++;
}

/***************************************************************************/
//  Insert - create a new node and put it in front of the current
//  position node and make it the current position.
/***************************************************************************/

void
list_InsertAtCurrentPos( PSBSPPTRLIST pList, void *pData)
{
    PSNODE  pNext, pPrev;
    PSNODE  pNew = (PSNODE) MALLOC( sizeof(NODE) );

    pNew->pData = pData;

    // check to be sure that there is a current node
    if (pList->pCurPosition != NULL)
    {
        
        // get pointers of current position
        pPrev = pList->pCurPosition->pPrev;
        pNext = pList->pCurPosition->pNext;

        // set new nodes pointers for insertion into the list
        pNew->pPrev = pPrev;
        pNew->pNext = pList->pCurPosition;

        // Set the node in front of new node (if any) to point to it
        if (pPrev != NULL)
        {
            pPrev->pNext = pNew;

        // No node in front -> new node is at head
        } else {
            pList->pHead = pNew;
        }

        // make new node the current node
        pList->pCurPosition = pNew;

    // No current node, just Add to front
    } else {
		list_Add( pList, pData );
    }

	// update counter
	pList->iNumNodes++;
}

/***************************************************************************/
//  Insert - create a new node and put it in front of the specified
//  node and make it the current position.
/***************************************************************************/

void
list_InsertInFrontOfNode( PSBSPPTRLIST pList, void *pData, void *pBefore )
{
    // simply make the specified node current and insert the new
    // node.
	pList->pCurPosition = list_Find( pList, pBefore );
	list_InsertAtCurrentPos( pList, pData );

	// update counter
	pList->iNumNodes++;
}

/***************************************************************************/
//  Remove - remove a specified node from the list.
//  Note: we do not delete the app data attached to the node!
/***************************************************************************/

void
list_Remove( PSBSPPTRLIST pList, void *pData )
{
    PSNODE  pCur, pNext, pPrev;

	pCur = pList->pCurPosition = list_Find( pList, pData );

    if (pCur != NULL)
    {
        // save a copy of the links
        pPrev = pCur->pPrev;
        pNext = pCur->pNext;

        // Is there a node ahead of us?
        if (pPrev != NULL)
        {
            // yes -> update it to not point to us.
            pPrev->pNext = pNext;
        } else {
            // no -> update head to not point to us.
            pList->pHead = pNext;
            pList->pCurPosition = pNext;
        }

        // Is there a node behind us?
        if (pNext != NULL)
        {
            // yes -> update it to not point to us.
            pNext->pPrev = pPrev;
            pList->pCurPosition = pNext;
        } else {
            // no -> update tail to not point to us.
            pList->pTail = pPrev;
            pList->pCurPosition = pPrev;
        }

        FREE(pCur);

		// update counter
		pList->iNumNodes--;
    }
}   

/***************************************************************************/
//  RemoveFirst - remove the first node in the list and return the
//  app data associated with it.
/***************************************************************************/

void *
list_RemoveFirst( PSBSPPTRLIST pList )
{
    PSNODE  pCur, pNext;
    void    *pData = NULL;

    pCur = pList->pHead;

    // is there a node at the head?
    if (pCur != NULL)
    {
        // take first node out of list.
        pNext = pCur->pNext;
        pList->pHead = pNext;
        pList->pCurPosition = pNext;

        // are there any nodes after us?
        if (pNext != NULL)
        {
            // yes -> make it the new head
            pNext->pPrev = NULL;
        } else {
            // no -> the list is now empty
            pList->pTail = NULL;
        }

        // get app data for node and then delete it
        pData = pCur->pData;
        FREE(pCur);

		// update counter
		pList->iNumNodes--;
    }

    return(pData);
}

/***************************************************************************/
//  RemoveLast - remove the last node in the list and return the
//  app data associated with it.
/***************************************************************************/

void *
list_RemoveLast( PSBSPPTRLIST pList )
{
    PSNODE  pCur, pPrev;
    void    *pData = NULL;

    pCur = pList->pTail;

    // is there a node at the tail?
    if (pCur != NULL)
    {
        // take last node out of list.
        pPrev = pCur->pPrev;
        pList->pTail = pPrev;

        // are there any nodes ahead of us?
        if (pPrev != NULL)
        {
            // yes -> make it the new tail node
            pPrev->pNext = NULL;
        } else {
            // no -> list is now empty
            pList->pHead = NULL;
            pList->pCurPosition = NULL;
        }

        // get app data for node and then delete it
        pData = pCur->pData;
        FREE(pCur);

		// update counter
		pList->iNumNodes--;
    }

    return(pData);
}

/***************************************************************************/
//  NumNodes - return number of nodes currently in list
/***************************************************************************/

int
list_NumNodes( PSBSPPTRLIST pList )
{
	return pList->iNumNodes;
}

/***************************************************************************/
