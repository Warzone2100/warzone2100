/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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

#include "structuredef.h"

struct BRIDGE_INFO
{
	int	startX, startY, endX, endY;			// Copy of coordinates of bridge.
	int	heightChange;					// How much to raise lowest end by.
	int	bridgeHeight;					// How high are the sections?
	int	bridgeLength;					// How many tiles long?
	bool	bConstantX, startHighest;			// Which axis is it on and which end is highest?
};

/* Establishes whether a bridge could be built along the coordinates given */
bool bridgeValid(int startX, int startY, int endX, int endY);

/* Draws a wall section - got to be in world matrix context though! */
bool renderBridgeSection(STRUCTURE *psStructure);

/* Will provide you with everything you ever wanted to know about your bridge but were afraid to ask */
void getBridgeInfo(int startX, int startY, int endX, int endY, BRIDGE_INFO *info);

/* FIX ME - this is used in debug to test the bridge build code */
void testBuildBridge(UDWORD startX, UDWORD startY, UDWORD endX, UDWORD endY);

#endif // __INCLUDED_SRC_BRIDGE_H__
