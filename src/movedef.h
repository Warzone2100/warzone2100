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
 *  Definitions for movement tracking.
 */

#ifndef __INCLUDED_MOVEDEF_H__
#define __INCLUDED_MOVEDEF_H__

#include "lib/framework/vector.h"

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

//Watermelon:num of VTOL weapons should be same as DROID_MAXWEAPS
#define VTOL_MAXWEAPS		3

typedef enum _move_status
{
MOVEINACTIVE,
MOVENAVIGATE,
MOVETURN,
MOVEPAUSE,
MOVEPOINTTOPOINT,
MOVETURNSTOP,
MOVETURNTOTARGET,
MOVEROUTE,		// unused
MOVEHOVER,
MOVEDRIVE,
MOVEWAITROUTE,
MOVESHUFFLE,
MOVEROUTESHUFFLE,	// unused
} MOVE_STATUS;

/// Extra precision added to movement calculations
#define EXTRA_BITS				8
#define EXTRA_PRECISION				((1 << EXTRA_BITS) - 1)
#define EXTRA_MASK				0xff

typedef struct _move_control
{
	MOVE_STATUS	Status;					// Inactive, Navigating or moving point to point status
	uint16_t	Position;				// Position in asPath
	uint16_t	numPoints;				// number of points in asPath
	Vector2i	 *asPath;				// Pointer to list of block X,Y map coordinates.

	SDWORD	DestinationX, DestinationY;			// World coordinates of movement destination
	SDWORD	srcX,srcY,targetX,targetY;
	float	speed;						// Speed of motion
	SWORD	boundX,boundY;				// Vector for the end of path boundary
	int32_t	eBitX, eBitY;					// extra bits stored in a temporary bit bucket

	uint16_t moveDir;					// direction of motion (not the direction the droid is facing)
	uint16_t bumpDir;					// direction at last bump
	UDWORD	bumpTime;					// time of first bump with something
	UWORD	lastBump;					// time of last bump with a droid - relative to bumpTime
	UWORD	pauseTime;					// when MOVEPAUSE started - relative to bumpTime
	UWORD	bumpX,bumpY;				// position of last bump

	UDWORD	shuffleStart;				// when a shuffle started

	struct _formation	*psFormation;			// formation the droid is currently a member of

	/* vtol movement - GJ */
	SWORD	iVertSpeed;

	// iAttackRuns tracks the amount of ammunition a VTOL has remaining for each weapon
	UDWORD	iAttackRuns[VTOL_MAXWEAPS];
} MOVE_CONTROL;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __INCLUDED_MOVEDEF_H__
