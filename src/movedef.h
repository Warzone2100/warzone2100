/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
MOVEHOVER = 8,  // = 8 for savegame compatibility.
MOVEDRIVE,
MOVEWAITROUTE,
MOVESHUFFLE,
} MOVE_STATUS;

/// Extra precision added to movement calculations, stored in ebitX, ebitY.
#define EXTRA_BITS                              8
#define EXTRA_PRECISION                         (1 << EXTRA_BITS)
#define EXTRA_MASK                              (EXTRA_PRECISION - 1)

struct MOVE_CONTROL
{
	MOVE_STATUS	Status;					// Inactive, Navigating or moving point to point status
	uint16_t	Position;				// Position in asPath
	uint16_t	numPoints;				// number of points in asPath
	Vector2i	 *asPath;				// Pointer to list of block X,Y map coordinates.

	SDWORD	DestinationX, DestinationY;			// World coordinates of movement destination
	SDWORD	srcX,srcY,targetX,targetY;
	int	speed;						// Speed of motion
	uint8_t  eBitX, eBitY;                                  // extra bits stored in a temporary bit bucket

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
};

#endif // __INCLUDED_MOVEDEF_H__
