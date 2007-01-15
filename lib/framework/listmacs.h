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
/*! \file listmacs.h
 * \brief Macros for some simple list operations
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
