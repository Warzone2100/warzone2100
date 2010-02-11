/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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
/** \file
 *  Definitions for command droids.
 */

#ifndef __INCLUDED_CMDDROIDDEF_H__
#define __INCLUDED_CMDDROIDDEF_H__

#include "statsdef.h"
#include "droiddef.h"

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

// the maximum number of command droids per side
#define MAX_CMDDROIDS	5

typedef struct _command_droid
{
	// define the command droid as a COMPONENT so it fits into the design screen
	STATS_COMPONENT;

	UDWORD          died;
	SWORD           aggression;
	SWORD           survival;
	SWORD           nWeapStat;
	UWORD           kills;
	DROID*          psDroid;
} COMMAND_DROID;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __INCLUDED_CMDDROIDDEF_H__
