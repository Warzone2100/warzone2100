/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2019  Warzone 2100 Project

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

#include <vector>

enum MOVE_STATUS
{
	MOVEINACTIVE,
	MOVENAVIGATE,
	MOVETURN,
	MOVEPAUSE,
	MOVEPOINTTOPOINT,
	MOVETURNTOTARGET,
	MOVEHOVER,
	MOVEWAITROUTE,
	MOVESHUFFLE,
};

struct MOVE_CONTROL
{
	MOVE_STATUS Status = MOVEINACTIVE;    ///< Inactive, Navigating or moving point to point status
	int pathIndex = 0;                    ///< Position in asPath
	std::vector<Vector2i> asPath;         ///< Pointer to list of block X,Y map coordinates.

	Vector2i destination = Vector2i(0, 0);                 ///< World coordinates of movement destination
	Vector2i src = Vector2i(0, 0);
	Vector2i target = Vector2i(0, 0);
	int speed = 0;                        ///< Speed of motion

	uint16_t moveDir = 0;                 ///< Direction of motion (not the direction the droid is facing)
	uint16_t bumpDir = 0;                 ///< Direction at last bump
	unsigned bumpTime = 0;                ///< Time of first bump with something
	uint16_t lastBump = 0;                ///< Time of last bump with a droid - relative to bumpTime
	uint16_t pauseTime = 0;               ///< When MOVEPAUSE started - relative to bumpTime
	Position bumpPos = Position(0, 0, 0); ///< Position of last bump

	unsigned shuffleStart = 0;            ///< When a shuffle started

	int iVertSpeed = 0;                   ///< VTOL movement
};

#endif // __INCLUDED_MOVEDEF_H__
