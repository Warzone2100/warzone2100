/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
#ifndef __INCLUDED_LIB_FRAMEWORK_LISTMACS_H__
#define __INCLUDED_LIB_FRAMEWORK_LISTMACS_H__

/**
 * Append an entry to the end of a linked list
 */
#define LIST_APPEND(psHead, psEntry, TYPE) \
	{ \
		TYPE	*psPrev, *psCurr; \
		\
		/* Find the end of the list */ \
		psPrev = NULL; \
		for(psCurr = (psHead); psCurr; psCurr = psCurr->psNext) \
		{ \
			psPrev = psCurr; \
		} \
		\
		(psEntry)->psNext = NULL; \
		\
		if (psPrev) \
		{ \
			/* Append to the end */ \
			psPrev->psNext = (psEntry); \
		} \
		else \
		{ \
			/* If a NULL list got passed, make this item the list */ \
			(psHead) = (psEntry); \
		} \
	}

/**
 * Remove an entry from a linked list
 */
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
		ASSERT(psCurr != NULL, "LIST_REMOVE: entry not found"); \
		if (psPrev == NULL) \
		{ \
			(psHead) = (psHead)->psNext; \
		} \
		else if (psCurr != NULL) \
		{ \
			psPrev->psNext = psCurr->psNext; \
		} \
	}

#endif // __INCLUDED_LIB_FRAMEWORK_LISTMACS_H__
