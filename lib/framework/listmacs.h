/*
 * ListMacs.h
 *
 * Macros for some simple list operations
 *
 * The parameters for the macros are :
 *
 * psHead		the pointer to the start of the list
 * psEntry		the pointer to the list entry
 * TYPE			the type of the list structure (not a pointer to it)
 *
 */
#ifndef _listmacs_h
#define _listmacs_h

// initialise a list
#define LIST_INIT(psHead) psHead = NULL

// add an entry to the start of a list
#define LIST_ADD(psHead, psEntry) \
	(psEntry)->psNext = (psHead); \
	(psHead) = (psEntry)

// add an entry to a list
#define LIST_ADDEND(psHead, psEntry, TYPE) \
{ \
	TYPE	*psPrev, *psCurr; \
\
	psPrev = NULL; \
	for(psCurr = (psHead); psCurr; psCurr = psCurr->psNext) \
	{ \
		psPrev = psCurr; \
	} \
	if (psPrev) \
	{ \
		psPrev->psNext = (psEntry); \
	} \
	else \
	{ \
		(psEntry)->psNext = NULL; \
		(psHead) = (psEntry); \
	} \
}

// remove an entry from a list
#define LIST_REMOVE(psHead, psEntry, TYPE) \
{ \
	TYPE	*psPrev, *psCurr; \
\
	psPrev = NULL; \
	for(psCurr = (psHead); psCurr; psCurr = psCurr->psNext) \
	{ \
		if (psCurr == (psEntry)) \
		{ \
			break; \
		} \
		psPrev = psCurr; \
	} \
	ASSERT( psCurr!=NULL, "LIST_REMOVE: " __FILE__ "(%d): entry not found", __LINE__ ); \
	if (psPrev == NULL) \
	{ \
		(psHead) = (psHead)->psNext; \
	} \
	else if (psCurr != NULL) \
	{ \
		psPrev->psNext = psCurr->psNext; \
	} \
}


#endif
