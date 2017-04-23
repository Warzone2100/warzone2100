/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
struct STR_RES *strresCreate();

/* Release a string resource object */
WZ_DECL_NONNULL(1) void strresDestroy(struct STR_RES *psRes);

/**
 * Retrieve a resource string from its identifier string.
 *
 * @param psRes the string resource to search
 * @param ID the string ID to search the matching string for
 * @return the string associated with the given @c ID string, or NULL if none
 *         could be found.
 */
WZ_DECL_NONNULL(1, 2) const char *strresGetString(const struct STR_RES *psRes, const char *ID);

/* Load a string resource file */
WZ_DECL_NONNULL(1, 2) bool strresLoad(struct STR_RES *psRes, const char *fileName);

/* Get the ID string for a string */
WZ_DECL_NONNULL(1, 2) const char *strresGetIDfromString(struct STR_RES *psRes, const char *pString);

#endif
