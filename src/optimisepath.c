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
	25th September, 1998. Path Optimisation.
	An attempt to optimise the final path found for routing
	in order to stop units getting snagged on corners.
	Alex McLean. Will only be executed on my machine for now...
*/

// -------------------------------------------------------------------------
#include "lib/framework/frame.h"
#include "lib/framework/trig.h"

#include "base.h"
#include "move.h"
#include "map.h"
#include "weapons.h"
#include "statsdef.h"
#include "droiddef.h"
#include "lib/ivis_common/pietypes.h"
#include "optimisepath.h"
#include "raycast.h"
#include "fpath.h"
#include "effects.h"

// -------------------------------------------------------------------------
/* Where to slide a path point according to bisecting angle */
signed char	markerSteps[8][2] =
{
	{0,0},
	{1,-1},
	{1,0},
	{1,1},
	{0,1},
	{-1,1},
	{-1,0},
	{-1,-1}
};


// -------------------------------------------------------------------------
/* Returns the index into the above table for a given angle */
UDWORD	getStepIndexFromAngle(UDWORD angle);


/*
	Attempts to make the droid's path more 'followable' by moving
	waypoints away from the nearest blocking tile. The direction
	in which the waypoint is moved is dependant on where the previous
	and next way points lie. Essentially, the angle that bisects the two
	vectors to the previous and next waypoint is found. We then
	calculate which way to make this angle (line) face. It needs to face into
	the larger of the two arcs, thereby moving _away_ from the blocking
	tile
*/
void	optimisePathForDroid(DROID *psDroid)
{
#if 0
	// didn't bother to get this to compile when I changed the PATH_POINT structure
	// I'll sort it out later - John.
	UDWORD	index,stepIndex;
	UDWORD	angleA,angleB;
	SDWORD	x1,y1,x2,y2;
	UDWORD	bisectingAngle;
	SDWORD	xAlt,yAlt;
	UDWORD	tileX,tileY;
	Vector3i pos;
	SDWORD	xDif,yDif;
	SDWORD	xRec,yRec;
	PATH_POINT	*moveList;

	index = 0;
	/* Get a pointer to the movement list */
	moveList = psDroid->sMove.MovementList;

	/* Go through the movement list */
	while(moveList[index].XCoordinate != -1)
	{
		/* If it's not the start point and not the end point */
		if(index!=0 && moveList[index+1].XCoordinate != -1	&& moveList[index+1].YCoordinate != -1)
		{
			// ------------------------
			// IMPORTANT
			// Maybe only do this waypoint movement
			// if the distance between this one
			// and the last is beyond a certain threshold.
			// To be found by experiment, or maybe vehicle speed.
			// ------------------------


			/* Get the existing waypoint */
			x1 = moveList[index].XCoordinate;
			y1 = moveList[index].YCoordinate;
//			xRec = x1;
//			yRec = y1;
			// - add a fire so we get to see where it is! */
				pos.x = moveList[index].XCoordinate;
				pos.z = moveList[index].YCoordinate;
				pos.y = map_Height(pos.x,pos.z);
				effectGiveAuxVar(8);
	  			effectGiveAuxVarSec(100000);
//				addEffect(&pos,EFFECT_FIRE,FIRE_TYPE_LOCALISED,FALSE,NULL,0);

			/* Get the coordinates of the previous waypoint */
			x2 = moveList[index-1].XCoordinate;
			y2 = moveList[index-1].YCoordinate;

			/* Get the angle to it */
			angleA = rayPointsToAngle(x2,y2,x1,y1);

			/* Get the coordinates of the next waypoint */
			x2 = moveList[index+1].XCoordinate;	// x1,y1 stay the same
			y2 = moveList[index+1].YCoordinate;

			/* Get the angle to it, notice switch in relative order */
			angleB = rayPointsToAngle(x2,y2,x1,y1);

			/* Establish the bisecting angle */
			bisectingAngle = getBisectingDirectionAway(angleA,angleB);

			/* Find correct index to use into offset table */
			stepIndex = getStepIndexFromAngle(bisectingAngle);

		 //	DBPRINTF(("First Angle %d, Second Angle %d, Bisecting %d, Index %d\n",angleA,angleB,bisectingAngle,stepIndex));

			/* Get the alteration in terms of tile coordinates */
			xAlt = markerSteps[stepIndex][0];
			yAlt = markerSteps[stepIndex][1];

		 //	DBPRINTF(("XShift %d, yShift %d\n",xAlt,yAlt));

			/* Get the tile we're trying to move to */
			tileX = x1>>TILE_SHIFT;
			tileY = y1>>TILE_SHIFT;
			tileX+=xAlt;
			tileY+=yAlt;

			/* Could we move the waypoint there? */
			if(!fpathBlockingTile(tileX,tileY))
			{
		 //		DBPRINTF(("Moved marker\n"));
				/* Move it */
				moveList[index].XCoordinate = (x1 + (xAlt*TILE_UNITS));
				moveList[index].YCoordinate = (y1 + (yAlt*TILE_UNITS));

				/*
				xDif = xRec - (x1 + (xAlt*TILE_UNITS));
				yDif = yRec - (y1 + (yAlt*TILE_UNITS));
				if(abs(xDif)>128 || abs(yDif)>128)
				{
					effectGiveAuxVar(4);
				}
				*/

				pos.x = moveList[index].XCoordinate;
				pos.z = moveList[index].YCoordinate;
				pos.y = map_Height(pos.x,pos.z) + 256;
				effectGiveAuxVar(8);
	  			effectGiveAuxVarSec(150000);
				addEffect(&pos,EFFECT_FIRE,FIRE_TYPE_SMOKY_BLUE,FALSE,NULL,0);

			}
			else
			{
		   //		DBPRINTF(("Could NOT move marker - blocking tile\n"));
				/*
				xDif = xRec - (x1 + (xAlt*TILE_UNITS));
				yDif = yRec - (y1 + (yAlt*TILE_UNITS));
				if(abs(xDif)>128 || abs(yDif)>128)
				{
					effectGiveAuxVar(4);
				}
				pos.x = moveList[index].XCoordinate + (xAlt*TILE_UNITS);
				pos.z = moveList[index].YCoordinate + (yAlt*TILE_UNITS);
				pos.y = map_Height(pos.x,pos.z) + 512;
				effectGiveAuxVar(2);
	  			effectGiveAuxVarSec(150000);
				addEffect(&pos,EFFECT_FIRE,FIRE_TYPE_LOCALISED,FALSE,NULL,0);
				*/
			}

		}
		index++;
	}
#endif
}

// -------------------------------------------------------------------------
/*
	Gets the angle that bisects the given to angles. the angle
	is given such that it points into the larger arc. This is
	of course ambiguous in the case where the two given angles
	are 180 degrees apart
*/
UDWORD	getBisectingDirectionAway(UDWORD angleA,UDWORD angleB)
{
FRACT	xVec,yVec;
FRACT	angle;
UDWORD	retVal;

	/* Get the component vectors */
	xVec = trigSin(angleA) + trigSin(angleB);
	yVec = trigCos(angleA) + trigCos(angleB);

	/* Get the angle between them */
	angle = RAD_TO_DEG(atan2(xVec,yVec));
	angle+=360;
	/* Get it as an integer */
	retVal = (MAKEINT(angle))%360;

	/* And make it point the other way - into larger arc */
	retVal = (retVal + 180)%360;
	ASSERT( retVal<360,"Weird angle found" );

	return(retVal);
}

// -------------------------------------------------------------------------
/*
	A hack function - could be done by dividing the angle by 45
	and establishing the right quadrant
*/
UDWORD	getStepIndexFromAngle(UDWORD angle)
{
FRACT	accA;
UDWORD	retVal = 0;

	accA = MAKEFRACT(angle);

	ASSERT( angle<360,"Angle's too big!!!" );

	if(accA<=22.5 || accA>337.0)
	{
		retVal = 0;
	}
	else if(accA>22.5 && accA <= 67.5)
	{
		retVal = 1;

	}
	else if(accA>67.5 && accA <= 112.5)
	{
		retVal = 2;

	}
	else if(accA>112.5 && accA <= 157.5)
	{
		retVal = 3;

	}
	else if(accA>157.5 && accA <= 202.5)
	{
		retVal = 4;

	}
	else if(accA>202.5 && accA <= 247.5)
	{
		retVal = 5;

	}
	else if(accA>247.5 && accA <= 292.5)
	{
		retVal = 6;

	}

	else if(accA>292.5 && accA <= 337.5)
	{
		retVal = 7;

	}

	return(retVal);

}
// -------------------------------------------------------------------------
