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
/*
 * FindPath.h
 *
 * Routing functions interface.
 *
 */
#ifndef _findpath_h
#define _findpath_h

#define MAXMOVES			50
#define WALLHUGSIZE			150

#define PIXELMOVEMENT		0x80000000
#define PIXELMOVEMASK		0x7FFFFFFF
#define BLOCKMOVEMENT		0x00000000

#define MOVEINACTIVE		0
#define MOVENAVIGATE		1
#define MOVETURN			2
#define MOVEPAUSE			3
#define MOVEPOINTTOPOINT	4
#define MOVETURNSTOP		5
#define MOVETURNTOTARGET	6
#define MOVEROUTE			7
#define MOVEHOVER			8
#define MOVEDRIVE			9
#define MOVEDRIVEFOLLOW		10
#define MOVEWAITROUTE		11
#define MOVESHUFFLE			12
#define MOVEROUTESHUFFLE	13

//bit operators for map movement
#if 0
#define SUPER_BIT	0x01
#define CHAR_BIT	0x02
#define BUILD_BIT	0x04
#define VECH_BIT	0x08

#define ALL_BIT		SUPER_BIT+CHAR_BIT+BUILD_BIT+VECH_BIT
#endif

extern BOOL blockingTile(UDWORD x, UDWORD y, UDWORD mask);

extern void FindPath (SDWORD StartX, SDWORD StartY, SDWORD DestinationX, 
					  SDWORD DestinationY, MOVE_CONTROL *MoveData,	
					  UBYTE Mask);
extern SDWORD WallHug (PATH_POINT *WallPoints, SBYTE DirectionAdjustment,
				SDWORD StartX, SDWORD StartY, SDWORD EndX, SDWORD EndY, SDWORD Direction,
				UBYTE Mask);
extern BOOL LineOfSight (SDWORD StartX, SDWORD StartY, SDWORD EndX, SDWORD EndY, 
						 UBYTE Mask);
extern BOOL MovementUpdate(BASE_OBJECT *Obj, MOVE_CONTROL *MoveData);
extern BOOL NewMovementUpdate(BASE_OBJECT *Obj, MOVE_CONTROL *MoveData);
/* Turn towards a target */
extern void TurnToTarget(BASE_OBJECT *Obj, MOVE_CONTROL *MoveData,
						 UDWORD tarX, UDWORD tarY);
/* Return the difference in directions */
extern UDWORD dirDiff(SDWORD start, SDWORD end);

#define POS	0
#define NEG 1
#define NIL 2
#define COMPASS_POINTS 8

#define NORTH		0
#define NORTHEAST	1
#define EAST		2
#define SOUTHEAST	3
#define SOUTH		4
#define SOUTHWEST	5
#define WEST		6
#define NORTHWEST	7

#endif // findpath_h
