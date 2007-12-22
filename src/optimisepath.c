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

#include "basedef.h"
#include "move.h"
#include "map.h"
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


// -------------------------------------------------------------------------
/*
	Gets the angle that bisects the given to angles. the angle
	is given such that it points into the larger arc. This is
	of course ambiguous in the case where the two given angles
	are 180 degrees apart
*/
UDWORD	getBisectingDirectionAway(UDWORD angleA,UDWORD angleB)
{
float	xVec,yVec;
float	angle;
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
float	accA;
UDWORD	retVal = 0;

	accA = (float)angle;

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
