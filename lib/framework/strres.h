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
/*! \file strres.h
 *  * \brief String resource interface functions
 */
#ifndef _strres_h
#define _strres_h

// Forward declaration to allow pointers to this type
struct STR_RES;

/* Create a string resource object */
extern struct STR_RES* strresCreate(size_t init, size_t ext);

/* Release a string resource object */
extern void strresDestroy(struct STR_RES *psRes);

/* Return the ID number for an ID string */
extern BOOL strresGetIDNum(struct STR_RES *psRes, const char *pIDStr, UDWORD *pIDNum);

/**
 * @return The stored ID string that matches the string passed in, or NULL if
 *         no ID string could be found.
 */
extern const char* strresGetIDString(struct STR_RES *psRes, const char *pIDStr);

/* Get the string from an ID number */
extern const char* strresGetString(const struct STR_RES *psRes, UDWORD id);

/* Load a string resource file */
extern BOOL strresLoad(struct STR_RES* psRes, const char* fileName);

/* Get the ID number for a string*/
extern UDWORD strresGetIDfromString(struct STR_RES *psRes, const char *pString);

#endif
