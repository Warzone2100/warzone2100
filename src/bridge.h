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

#ifndef __INCLUDED_SRC_BRIDGE_H__
#define __INCLUDED_SRC_BRIDGE_H__

/* Shift these out into a header file */
#define MINIMUM_BRIDGE_SPAN 2
#define MAXIMUM_BRIDGE_SPAN	12
#define BRIDGE_END_HORIZ 1
#define BRIDGE_END_VERT  2
#define BRIDGE_MID_HORIZ 3
#define BRIDGE_MID_VERT  4

typedef struct _bridge_info
{
UDWORD	startX,startY,endX,endY;			// Copy of coordinates of bridge.
UDWORD	heightChange;						// How much to raise lowest end by.
UDWORD	bridgeHeight;						// How high are the sections?
UDWORD	bridgeLength;						// How many tiles long?
BOOL	bConstantX,startHighest;			// Which axis is it on and which end is highest?
} BRIDGE_INFO;

/* Establishes whether a bridge could be built along the coordinates given */
extern BOOL	bridgeValid(UDWORD startX,UDWORD startY, UDWORD endX, UDWORD endY);
/* Draws a wall section - got to be in world matrix context thogh! */
extern BOOL	renderBridgeSection(STRUCTURE *psStructure);
/* Will provide you with everything you ever wanted to know about your bridge but were afraid to ask */
extern void	getBridgeInfo(UDWORD startX,UDWORD startY,UDWORD endX, UDWORD endY, BRIDGE_INFO *info);

/* FIX ME - this is used in debug to test the bridge build code */
extern void	testBuildBridge(UDWORD startX,UDWORD startY,UDWORD endX,UDWORD endY);

#endif // __INCLUDED_SRC_BRIDGE_H__
