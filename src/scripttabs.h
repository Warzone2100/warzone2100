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
/** @file
 *  All the tables for the script compiler
 */

#ifndef __INCLUDED_SRC_SCRIPTTABS_H__
#define __INCLUDED_SRC_SCRIPTTABS_H__

#include "lib/script/parse.h"

// How many game ticks for one event tick
#define SCR_TICKRATE	100

// The table of user types for the compiler
extern TYPE_SYMBOL asTypeTable[];

// The table of script callable functions
extern FUNC_SYMBOL asFuncTable[];

// The table of external variables
extern VAR_SYMBOL asExternTable[];

// The table of object variables
extern VAR_SYMBOL asObjTable[];

// The table of constant values
extern CONST_SYMBOL asConstantTable[];

// Initialise the script system
extern bool scrTabInitialise(void);

// Shut down the script system
extern void scrShutDown(void);

#endif // __INCLUDED_SRC_SCRIPTTABS_H__
