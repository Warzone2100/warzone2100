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
 * GTime.c
 *
 * Provide a game clock that runs only when the game runs.
 *
 */

#include <SDL_timer.h>
#include <time.h>

#include "lib/framework/frame.h"
#include "gtime.h"

#define GTIME_MINFRAME	(GAME_TICKS_PER_SEC/80)

/* See header file for documentation */
UDWORD gameTime, frameTime, gameTime2, frameTime2;
float frameTimeFraction, frameTimeFraction2;

/** The current clock modifier. Set to speed up the game. */
static float modifier;

/** The amount of game time before the last time clock speed was set. */
static UDWORD	timeOffset;

/** The amount of game time before the last time clock speed was set. */
static UDWORD	timeOffset2;

/** The tick count the last time the clock speed was set. */
static UDWORD	baseTime;

/** The tick count the last time the clock speed was set. */
static UDWORD	baseTime2;

/** When the game paused, so that gameTime can be adjusted when the game restarts. */
static SDWORD	pauseStart;

/** 
  * Count how many times gameTimeStop has been called without a game time start. 
  * We use this to ensure that we can properly nest stop commands. 
  **/
static UDWORD	stopCount;

/* Initialise the game clock */
BOOL gameTimeInit(void)
{
    	/* Start the timer off at 2 so that when the scripts strip the map of objects
	 * for multiPlayer they will be processed as if they died. */
	gameTime = 2;
	timeOffset = 0;
	baseTime = SDL_GetTicks();

	gameTime2 = 0;
	timeOffset2 = 0;
	baseTime2 = baseTime;

	modifier = 1.0f;

	stopCount = 0;

	return TRUE;
}

UDWORD	getTimeValueRange(UDWORD tickFrequency, UDWORD requiredRange)
{
	UDWORD	div1, div2;

	div1 = gameTime2%tickFrequency;
	div2 = tickFrequency/requiredRange;
	return(div1/div2);
}

UDWORD	getStaticTimeValueRange(UDWORD tickFrequency, UDWORD requiredRange)
{
	UDWORD	div1, div2;

	div1 = gameTime%tickFrequency;
	div2 = tickFrequency/requiredRange;
	return(div1/div2);
}

/* Call this each loop to update the game timer */
void gameTimeUpdate(void)
{
	unsigned int currTime = SDL_GetTicks();
	unsigned long long newTime;

	// Do not update the game time if gameTimeStop has been called
	if (stopCount == 0)
	{
		// Calculate the new game time
		newTime = ( currTime - baseTime ) * modifier + timeOffset;

		// Calculate the time for this frame
		frameTime = (newTime - gameTime);

		// Limit the frame time
		if (frameTime > GTIME_MAXFRAME)
		{
			baseTime += ( frameTime - GTIME_MAXFRAME ) / modifier; // adjust the addition to base time
			newTime = gameTime + GTIME_MAXFRAME;
			frameTime = GTIME_MAXFRAME;
		}

		// Store the game time
		gameTime = newTime;
	}

	// now update gameTime2 which does not pause
	newTime = currTime - baseTime2 + timeOffset;

	// Calculate the time for this frame
	frameTime2 = newTime - gameTime2;

	// Limit the frame time
	if (frameTime2 > GTIME_MAXFRAME)
	{
		baseTime2 += frameTime2 - GTIME_MAXFRAME;
		newTime = gameTime2 + GTIME_MAXFRAME;
		frameTime2 = GTIME_MAXFRAME;
	}

	// Store the game time
	gameTime2 = newTime;

	// Pre-calculate fraction used in timeAdjustedIncrement
	frameTimeFraction = (float)frameTime / (float)GAME_TICKS_PER_SEC;
	frameTimeFraction2 = (float)frameTime / (float)GAME_TICKS_PER_SEC;
}

// reset the game time modifiers
void gameTimeResetMod(void)
{
	timeOffset = gameTime;
	timeOffset2 = gameTime2;

	baseTime = SDL_GetTicks();
	baseTime2 = baseTime;

	modifier = 1.0f;
}

// set the time modifier
void gameTimeSetMod(float mod)
{
	gameTimeResetMod();
	modifier = mod;
}

// get the current time modifier
void gameTimeGetMod(float *pMod)
{
	*pMod = modifier;
}

BOOL gameTimeIsStopped(void)
{
	return (stopCount != 0);
}

/* Call this to stop the game timer */
void gameTimeStop(void)
{
	if (stopCount == 0)
	{
		pauseStart = SDL_GetTicks();
		debug( LOG_NEVER, "Clock paused at %d\n", pauseStart);
	}
	stopCount += 1;
}

/* Call this to restart the game timer after a call to gameTimeStop */
void gameTimeStart(void)
{
	if (stopCount == 1)
	{
		// shift the base time to now
		timeOffset = gameTime;
		baseTime = SDL_GetTicks();
	}

	if (stopCount > 0)
	{
		stopCount --;
	}
}

/* Call this to reset the game timer */
void gameTimeReset(UDWORD time)
{
	// reset the game timers
	gameTime = time;
	timeOffset = time;

	gameTime2 = time;
	timeOffset2 = time;

	baseTime = SDL_GetTicks();
	baseTime2 = baseTime;

	modifier = 1.0f;
}

void	getTimeComponents(UDWORD time, UDWORD *hours, UDWORD *minutes, UDWORD *seconds)
{
	UDWORD	h, m, s;
	UDWORD	ticks_per_hour, ticks_per_minute;

	/* Ticks in a minute */
	ticks_per_minute = GAME_TICKS_PER_SEC * 60;

	/* Ticks in an hour */
	ticks_per_hour = ticks_per_minute * 60;

	h = time / ticks_per_hour;
	m = (time - (h * ticks_per_hour)) / ticks_per_minute;
	s = (time - ((h * ticks_per_hour) + (m * ticks_per_minute))) / GAME_TICKS_PER_SEC;

	*hours = h;
	*minutes = m;
	*seconds = s;
}
