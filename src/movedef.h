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
/** \file
 *  Definitions for movement tracking.
 */

#ifndef __INCLUDED_MOVEDEF_H__
#define __INCLUDED_MOVEDEF_H__

#include "lib/framework/vector.h"
#include "formationdef.h"

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

	/// Counts route replacements, so a reader that memoized an answer about this droid's path
	/// can tell that the path it read is gone.
	/// Not saved: it only decides whether a memo is reused, and a miss recomputes the same answer.
	uint32_t routeGeneration = 0;

	void setRoute(std::vector<Vector2i> route, int index = 0)
	{
		asPath = std::move(route);
		pathIndex = index;
		++routeGeneration;
	}

	void setRoute(Vector2i point)
	{
		asPath.assign(1, point);
		pathIndex = 0;
		++routeGeneration;
	}

	void clearRoute()
	{
		asPath.clear();
		pathIndex = 0;
		++routeGeneration;
	}

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
	unsigned tolerance = 0;               ///< Increases until unit gives up and goes to the next waypoint, if close enough
	unsigned shuffleStart = 0;            ///< When a shuffle started
	unsigned settleTime = 0;              ///< When this droid last closed on its destination
	int32_t settleBest = 0;               ///< How close it has come, 0 until it has been measured
	Vector2i backoffPos = Vector2i(0, 0); ///< Where this droid last gained real ground
	unsigned backoffTime = 0;             ///< When it was there, 0 until it has been measured
	unsigned backoffUntil = 0;            ///< End of the running backoff episode, 0 outside one

	FORMATION *psFormation = nullptr;     ///< formation the droid is currently a member of

	int iVertSpeed = 0;                   ///< VTOL movement

	/// The corridor layer's last answer about whether this droid is held, with the state it was taken against.
	/// Not saved: a miss recomputes the same answer.
	struct HoldMemo
	{
		uint32_t stamp = 0;            ///< zero while empty
		uint32_t gameTime = 0;
		uint32_t routeGeneration = 0;
		uint32_t bumpTime = 0;
		Vector2i pos = Vector2i(0, 0);
		Vector2i target = Vector2i(0, 0);
		int pathIndex = 0;
		uint8_t flags = 0;
		uint8_t value = 0;
	} holdMemo;
};

#endif // __INCLUDED_MOVEDEF_H__
