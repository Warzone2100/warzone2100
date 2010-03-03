/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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
/*! \file gtime.h
 * \brief Interface to the game clock.
 *
 */
#ifndef _gtime_h
#define _gtime_h


/// The number of time units per second of the game clock.
#define GAME_TICKS_PER_SEC 1000
/// The number of game state updates per second of the game clock.
#define GAME_UPDATES_PER_SEC 10
/// The number of time units per game tick.
#define GAME_TICKS_PER_UPDATE (GAME_TICKS_PER_SEC/GAME_UPDATES_PER_SEC)

/** The maximum time for one frame (stops the clock running away when debugging)
 * changed to /6 by ajl. if this needs to go back to ticks/10 then tell me. */
#define GTIME_MAXFRAME (GAME_TICKS_PER_SEC/6)

/// The current time in the game world.
/// Changes in GAME_UNITS_PER_TICK increments.
extern UDWORD gameTime;
/// The current time in the graphical display of the game world.
/// Should be close to gameTime, up to GAME_UNITS_PER_TICK behind.
extern UDWORD graphicsTime;
/** The current time in the game world - never stops.
 * FIXME Then isn't it the real time, not the game time? Rename from gameTime2 to realTime?
 * Think it did stop (opposite of what the above comment says), if the graphics rendering couldn't keep up. If needed, that feature can be added back.
 * And it was previously affected by the game speed modifier, which explains how the game speed modifier affected the GUI speed...
 */
extern UDWORD realTime;
#define gameTime2 realTime

/// The difference between the previous and current gameTime.
/// FIXME This should be renamed from frameTime to deltaGameTime.
extern UDWORD deltaGameTime;
#define frameTime deltaGameTime
/// The difference between the previous and current graphicsTime.
extern UDWORD deltaGraphicsTime;
/// The difference between the previous and current gameTime2 (FIXME gameTime2 should be called realTime).
/// FIXME This should be renamed from frameTime2 to deltaRealTime.
extern UDWORD deltaRealTime;
#define frameTime2 deltaRealTime

/** Initialise the game clock. */
extern void gameTimeInit(void);

/// Changes the game (and graphics) time.
extern void setGameTime(uint32_t newGameTime);

/** Call this each loop to update the timers.
 * @param sane If true, then the game time increases in GAME_UNITS_PER_TICK increments, and deltaGameTime is either 0 or GAME_UNITS_PER_TICK. If false, the game time is equal to the graphics time, and the game always ticks.
 * @returns true iff the game time ticked.
 */
extern void gameTimeUpdate(void);
extern bool logicalUpdates;  ///< Separate logical from graphical updates. FIXME Should be constant true. But first it needs to work perfectly.

/* Returns true if gameTime is stopped. */
extern BOOL gameTimeIsStopped(void);

/** Call this to stop the game timer. */
extern void gameTimeStop(void);

/** Call this to restart the game timer after a call to gameTimeStop. */
extern void gameTimeStart(void);

/** Call this to set the game time and to reset the time modifier, and to update the real time, setting the delta to 0. */
extern void gameTimeReset(UDWORD time);

/**
 *	Reset the game time modifiers.
 *	@see gameTimeSetMod 
 */
void gameTimeResetMod(void);

/** Set the time modifier. Used to speed up the game. */
void gameTimeSetMod(float mod);

/** Get the current time modifier. */
void gameTimeGetMod(float *pMod);

/**
 * Returns the game time, modulo the time period, scaled to 0..requiredRange.
 * For instance getModularScaledGameTime(4096,256) will return a number that cycles through the values
 * 0..256 every 4.096 seconds...
 * Useful for periodic stuff.
 *
 * Operates on game time, which can be paused, and increases in GAME_UNITS_PER_TICK increments.
 * NOTE Currently unused – turns out only getModularScaledGraphicsTime was appropriate in the places this was previously used.
 */
extern UDWORD getModularScaledGameTime(UDWORD timePeriod, UDWORD requiredRange);
/**
 * Returns the graphics time, modulo the time period, scaled to 0..requiredRange.
 * For instance getModularScaledGraphicsTime(4096,256) will return a number that cycles through the values
 * 0..256 every 4.096 seconds...
 * Useful for periodic stuff.
 *
 * Operates on graphics time, which can be paused.
 */
extern UDWORD getModularScaledGraphicsTime(UDWORD timePeriod, UDWORD requiredRange);
/**
 * Returns the real time, modulo the time period, scaled to 0..requiredRange.
 * For instance getModularScaledRealTime(4096,256) will return a number that cycles through the values
 * 0..256 every 4.096 seconds...
 * Useful for periodical stuff.
 *
 * Operates on real time, which can't be paused.
 */
extern UDWORD getModularScaledRealTime(UDWORD timePeriod, UDWORD requiredRange);


/** Break down given time into its constituent components. */
extern void	getTimeComponents(UDWORD time, UDWORD *hours, UDWORD *minutes, UDWORD *seconds);



extern float gameTimeFraction;  ///< Private performance calculation. Do not use.
extern float graphicsTimeFraction;  ///< Private performance calculation. Do not use.
extern float realTimeFraction;  ///< Private performance calculation. Do not use.

/// Returns the value times deltaGameTime, converted to seconds.
static inline float gameTimeAdjustedIncrement(float value)
{
	return value * gameTimeFraction;
}
/// Returns the value times deltaGraphicsTime, converted to seconds.
static inline float graphicsTimeAdjustedIncrement(float value)
{
	return value * graphicsTimeFraction;
}
/// Returns the value times deltaGraphicsTime, converted to seconds.
static inline float realTimeAdjustedIncrement(float value)
{
	return value * realTimeFraction;
}

/**
 * Returns value times deltaGameTime (pauseTime = true) or deltaRealTime (pauseTime = false), converted to seconds.
 * @param value Amount to change something in a second.
 * @param pauseTime If true, adjust also for pause of game time. Generally use true in-game, false for GUI.
 * @return Amount to change this frame.
 * TODO Replace all calls to this function with gameTimeAdjustedIncrement or realTimeAdjustedIncrement, and delete this function.
 */
static inline float timeAdjustedIncrement(float value, BOOL pauseTime)
{
	return (pauseTime ? gameTimeAdjustedIncrement : realTimeAdjustedIncrement)(value);
}

#endif
