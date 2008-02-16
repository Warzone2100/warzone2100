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

/**
 * @file target.c
 * Screen coordinate based targeting system. Maintains a list of on screen objects
 * and provides various functions for aquiring targets from this list.
 */

#include <stdio.h>

#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h"
#include "lib/gamelib/animobj.h"

#include "statsdef.h"
#include "basedef.h"
#include "featuredef.h"
#include "weapondef.h"
#include "structuredef.h"
#include "display.h"
#include "visibility.h"
#include "objectdef.h"
#include "droid.h"

// FIXME Direct iVis implementation include!
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/piepalette.h"

#include "target.h"

#define MAX_TARGETS	32

static BOOL FoundCurrent;
static UWORD NumTargets;
static UWORD TargetCurrent;
static UDWORD TargetCurrentID;
static BASE_OBJECT *TargetList[MAX_TARGETS];
static UDWORD TargetEndTime;
static BASE_OBJECT *TargetingObject;

// Initialise the targeting system.
//
void targetInitialise(void)
{
	TargetCurrent = 0;
	TargetCurrentID = UDWORD_MAX;
}

// Reset the target list, call once per frame.
//
void targetOpenList(BASE_OBJECT *psTargeting)
{
	NumTargets = 0;
	FoundCurrent = FALSE;
	TargetingObject = psTargeting;
}

void targetCloseList(void)
{
}

void	targetAdd(BASE_OBJECT *psObj)
{
}

// Aquire the target nearest the vector from x,y to the top of the screen.
//
static BASE_OBJECT *targetAquireNearestView(SWORD x,SWORD y)
{
	UWORD i;
	UWORD Nearesti = 0;
	UDWORD NearestDx = UDWORD_MAX;
	UDWORD dx,dy;
	BASE_OBJECT *NearestObj = NULL;
	BASE_OBJECT *psObj;

	for(i=0; i<NumTargets; i++) {
		psObj = TargetList[i];
		dx = abs(psObj->sDisplay.screenX - x);
		dy = abs(psObj->sDisplay.screenY - y);
		dx += dy/2;
		if(dx < NearestDx) {
			NearestDx = dx;
			Nearesti = i;
			NearestObj = psObj;
		}
	}

	if(NearestObj != NULL) {
		TargetCurrent = Nearesti;
		if(TargetCurrentID != NearestObj->id) {
			TargetCurrentID = NearestObj->id;
			targetStartAnim();
		}
	} else {
		TargetCurrentID = UDWORD_MAX;
	}

	return NearestObj;
}

// Aquire the target nearest to the specified object.
//
BASE_OBJECT *targetAquireNearestObjView(BASE_OBJECT *psObj)
{
	if(psObj != NULL) {
		return targetAquireNearestView((SWORD)(psObj->sDisplay.screenX), (SWORD)(psObj->sDisplay.screenY));
	} else {
		return NULL;
	}
}

// Get the currently targeted object.
//
BASE_OBJECT *targetGetCurrent(void)
{
	if(TargetCurrentID != UDWORD_MAX) {
		return TargetList[TargetCurrent];
	}

	return NULL;
}

// Start the box zoom animation.
//
void targetStartAnim(void)
{
	TargetEndTime = gameTime+GAME_TICKS_PER_SEC/2;
}

// Display a marker over the current target.
//
void targetMarkCurrent(void)
{
	SWORD x,y;
	SWORD Offset;
	SWORD x0,y0,x1,y1;

	if(TargetCurrentID == UDWORD_MAX) {
		return;
	}

	x = (SWORD)(TargetList[TargetCurrent]->sDisplay.screenX);
	y = (SWORD)(TargetList[TargetCurrent]->sDisplay.screenY);

	// Make it zoom in.
	if(TargetEndTime > gameTime) {
		Offset =(SWORD)(16+(TargetEndTime-gameTime)/2);
	} else {
		Offset = 16;
	}

	x0 = (SWORD)(x-Offset);
	y0 = (SWORD)(y-Offset);
	x1 = (SWORD)(x+Offset);
	y1 = (SWORD)(y+Offset);

	iV_Line(x0, y0, x0 + 8, y0, WZCOL_YELLOW);
	iV_Line(x0, y0, x0, y0 + 8, WZCOL_YELLOW);

	iV_Line(x1, y0, x1 - 8, y0, WZCOL_YELLOW);
	iV_Line(x1, y0, x1, y0 + 8, WZCOL_YELLOW);

	iV_Line(x1, y1, x1 - 8, y1, WZCOL_YELLOW);
	iV_Line(x1, y1, x1, y1 - 8, WZCOL_YELLOW);

	iV_Line(x0, y1, x0 + 8, y1, WZCOL_YELLOW);
	iV_Line(x0, y1, x0, y1 - 8, WZCOL_YELLOW);
}
