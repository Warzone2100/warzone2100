/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
/** @file
 *  String management functions
 */

#ifndef __INCLUDED_SRC_TEXT_H__
#define __INCLUDED_SRC_TEXT_H__

// Forward declaration to allow pointers to this type
struct STR_RES;

/* The string resource object */
extern struct STR_RES *psStringRes;

/* Initialise the string system */
bool stringsInitialise();

/* Shut down the string system */
void stringsShutDown();

#endif // __INCLUDED_SRC_TEXT_H__
