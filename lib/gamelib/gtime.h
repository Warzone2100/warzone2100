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
/*! \file gtime.h
 * \brief Interface to the game clock.
 *
 */
#ifndef _gtime_h
#define _gtime_h

#include "lib/framework/vector.h"


struct NETQUEUE;
struct Rational;

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
/// The current time in the real world - never stops, and not reset between games.
extern UDWORD realTime;

/// The difference between the previous and current gameTime.
extern UDWORD deltaGameTime;
/// The difference between the previous and current graphicsTime.
extern UDWORD deltaGraphicsTime;
/// The difference between the previous and current realTime.
extern UDWORD deltaRealTime;

/** Initialise the game clock. */
extern void gameTimeInit(void);

/// Changes the game (and graphics) time.
extern void setGameTime(uint32_t newGameTime);

/** Call this each loop to update the gameTime, graphicsTime and realTime timers, and corresponding deltaGameTime, deltaGraphicsTime and deltaRealTime.
 * The game time increases in GAME_UNITS_PER_TICK increments, and deltaGameTime is either 0 or GAME_UNITS_PER_TICK.
 * @returns true iff the game time ticked.
 */
void gameTimeUpdate(bool mayUpdate);
/// Call after updating the state, and before processing any net messages that use deltaGameTime. (Sets deltaGameTime = 0.)
void gameTimeUpdateEnd(void);

/// Updates the realTime timer, and corresponding deltaRealTime.
void realTimeUpdate(void);

/* Returns true if gameTime is stopped. */
extern bool gameTimeIsStopped(void);

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
void gameTimeSetMod(Rational mod);

/** Get the current time modifier. */
Rational gameTimeGetMod();

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
void getTimeComponents(unsigned time, int *hours, int *minutes, int *seconds, int *milliseconds);



extern float graphicsTimeFraction;  ///< Private performance calculation. Do not use.
extern float realTimeFraction;  ///< Private performance calculation. Do not use.

/// Returns numerator/denominator * (newTime - oldTime). Rounds up or down such that the average return value is right, if oldTime is always the previous newTime.
static inline WZ_DECL_CONST int quantiseFraction(int numerator, int denominator, int newTime, int oldTime)
{
	int64_t newValue = (int64_t)newTime * numerator / denominator;
	int64_t oldValue = (int64_t)oldTime * numerator / denominator;
	return newValue - oldValue;
}
/// Returns numerator/denominator * (newTime - oldTime). Rounds up or down such that the average return value is right, if oldTime is always the previous newTime.
static inline WZ_DECL_CONST Vector3i quantiseFraction(Vector3i numerator, int denominator, int newTime, int oldTime)
{
	return Vector3i(quantiseFraction(numerator.x, denominator, newTime, oldTime),
	                quantiseFraction(numerator.y, denominator, newTime, oldTime),
	                quantiseFraction(numerator.z, denominator, newTime, oldTime));
}

/// Returns the value times deltaGameTime, converted to seconds. The return value is rounded down.
static inline int32_t gameTimeAdjustedIncrement(int value)
{
	return value * (int)deltaGameTime / GAME_TICKS_PER_SEC;
}
/// Returns the value times deltaGraphicsTime, converted to seconds.
static inline float graphicsTimeAdjustedIncrement(float value)
{
	return value * graphicsTimeFraction;
}

/// Returns the value times deltaGraphicsTime, converted to seconds, as a float.
static inline float realTimeAdjustedIncrement(float value)
{
	return value * realTimeFraction;
}
/// Returns the value times deltaRealTime, converted to seconds. The return value is rounded up or down to the nearest integer, such that it is exactly right on average.
static inline int32_t realTimeAdjustedAverage(int value)
{
	return quantiseFraction(value, GAME_TICKS_PER_SEC, realTime + deltaRealTime, realTime);
}

/// Returns the value times deltaGameTime, converted to seconds. The return value is rounded up or down, such that it is exactly right on average.
static inline int32_t gameTimeAdjustedAverage(int value)
{
	return quantiseFraction(value, GAME_TICKS_PER_SEC, gameTime + deltaGameTime, gameTime);
}
/// Returns the numerator/denominator times deltaGameTime, converted to seconds. The return value is rounded up or down, such that it is exactly right on average.
static inline int32_t gameTimeAdjustedAverage(int numerator, int denominator)
{
	return quantiseFraction(numerator, GAME_TICKS_PER_SEC * denominator, gameTime + deltaGameTime, gameTime);
}
/// Returns the numerator/denominator times deltaGameTime, converted to seconds. The return value is rounded up or down, such that it is exactly right on average.
static inline Vector3i gameTimeAdjustedAverage(Vector3i numerator, int denominator)
{
	return quantiseFraction(numerator, GAME_TICKS_PER_SEC * denominator, gameTime + deltaGameTime, gameTime);
}

void sendPlayerGameTime(void);                            ///< Sends a GAME_GAME_TIME message with gameTime plus latency to our game queues.
void recvPlayerGameTime(NETQUEUE queue);                  ///< Processes a GAME_GAME_TIME message.
bool checkPlayerGameTime(unsigned player);                ///< Checks that we are not waiting for a GAME_GAME_TIME message from this player. (player can be NET_ALL_PLAYERS.)
void setPlayerGameTime(unsigned player, uint32_t time);   ///< Sets the player's time.

#endif
