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
 *  Definitions for formations.
 */

#ifndef __INCLUDED_FORMATIONDEF_H__
#define __INCLUDED_FORMATIONDEF_H__

#include "droiddef.h"

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

// maximum number of lines in a formation
#define F_MAXLINES		4
// maximum number of unit members of a formation (cannot be more that 128)
#define F_MAXMEMBERS	20

// information about a formation line
// a linked list of the formation members on this line is maintained
// using their index in the asMembers array.  (-1 == 'NULL')
// (cuts down the memory use over proper pointers)
typedef struct _f_line
{
	SWORD		xoffset,yoffset;	// position relative to center
	uint16_t	direction;		// orientation of line
	SBYTE		member;				// first member in the 'linked list' of members
} F_LINE;

// information about a formation member
typedef struct _f_member
{
	SBYTE			line;			// which line this member is on
	SBYTE			next;			// the next member on this line
	SWORD			dist;			// distance along the line
	DROID                   *psDroid;               // the member unit
} F_MEMBER;

// information about a formation
typedef struct _formation
{
	SWORD		refCount;	// number of units using the formation

	SWORD		size;	// maximum length of the lines
	SWORD		rankDist;	// seperation between the ranks
	uint16_t	direction;	// direction of the formation
	SDWORD		x,y;	// position of the front of the formation

	// the lines that make up a formation
	F_LINE		asLines[F_MAXLINES];
	SWORD		numLines;
	UBYTE		maxRank;

	// the units that have a position allocated in the formation
	SBYTE		free;
	F_MEMBER	asMembers[F_MAXMEMBERS];

	// formation speed (currently speed of slowest member) - GJ - sorry.
	UDWORD		iSpeed;

	struct _formation	*psNext;
} FORMATION;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __INCLUDED_FORMATIONDEF_H__
