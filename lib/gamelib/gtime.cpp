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
/*
 * GTime.c
 *
 * Provide a game clock that runs only when the game runs.
 *
 */

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/math_ext.h"
#include "gtime.h"
#include "src/multiplay.h"
#include "lib/netplay/netplay.h"


#include <time.h>


/* See header file for documentation */
UDWORD gameTime = 0, deltaGameTime = 0, graphicsTime = 0, deltaGraphicsTime = 0, realTime = 0, deltaRealTime = 0;
float graphicsTimeFraction = 0.0, realTimeFraction = 0.0;

UDWORD extraGameTicksProcessed = 0;

/** The current clock modifier. Set to speed up the game. */
static Rational modifier;

/// The real time, the last time graphicsTime updated.
static uint32_t prevRealTime;

/**
  * Count how many times gameTimeStop has been called without a game time start.
  * We use this to ensure that we can properly nest stop commands.
  **/
static UDWORD	stopCount;

static uint32_t gameQueueTime[MAX_GAMEQUEUE_SLOTS];
static uint32_t gameQueueCheckTime[MAX_GAMEQUEUE_SLOTS];
static uint32_t gameQueueCheckCrc[MAX_GAMEQUEUE_SLOTS];
static bool     crcError = false;

static uint32_t updateReadyTime = 0;
static uint32_t updateWantedTime = 0;
static uint16_t chosenLatency = GAME_TICKS_PER_UPDATE;
static uint16_t discreteChosenLatency = GAME_TICKS_PER_UPDATE;
static uint16_t wantedLatency = GAME_TICKS_PER_UPDATE;
static uint16_t wantedLatencies[MAX_GAMEQUEUE_SLOTS];

static optional<uint32_t> waitingOnPlayersStartTime;
#define MIN_WAITONPLAYERS_DISPLAYTIME_FOR_SPECTATORS GAME_TICKS_PER_UPDATE

static void updateLatency(void);

static std::string listToString(char const *format, char const *separator, uint32_t const *begin, uint32_t const *end)
{
	std::string ret;
	uint32_t const *i = begin;
	while (i != end)
	{
		char tmp[100];
		ssprintf(tmp, format, *i);
		ret += tmp;

		if (++i != end)
		{
			ret += separator;
		}
	}
	return ret;
}

/* Initialise the game clock */
void gameTimeInit(void)
{
	unsigned player;

	/* Start the timer off at 2 so that when the scripts strip the map of objects
	 * for multiPlayer they will be processed as if they died. */
	// Setting game and graphics time.
	setGameTime(2);

	// Setting real time.
	realTime = wzGetTicks();
	deltaRealTime = 0;
	prevRealTime = realTime;
	realTimeFraction = 0.f;

	modifier = 1;

	stopCount = 0;

	chosenLatency = GAME_TICKS_PER_UPDATE * 2;
	discreteChosenLatency = GAME_TICKS_PER_UPDATE * 2;
	wantedLatency = GAME_TICKS_PER_UPDATE * 2;
	for (player = 0; player < MAX_GAMEQUEUE_SLOTS; ++player)
	{
		gameQueueCheckTime[player] = 0;
		gameQueueCheckCrc[player] = 0;
		wantedLatencies[player] = 0;
	}

	// Don't let syncDebug from previous games cause a desynch dump at gameTime 102.
	crcError = false;
	resetSyncDebug();
}

void setGameTime(uint32_t newGameTime)
{
	// Setting game time.
	gameTime = newGameTime;
	setPlayerGameTime(NET_ALL_PLAYERS, newGameTime);
	deltaGameTime = 0;

	// Setting graphics time to game time.
	graphicsTime = gameTime;
	deltaGraphicsTime = 0;
	graphicsTimeFraction = 0.f;

	// Not setting real time.
}

UDWORD getModularScaledGameTime(UDWORD timePeriod, UDWORD requiredRange)
{
	return gameTime % timePeriod * requiredRange / MAX(1, timePeriod);
}

UDWORD getModularScaledGraphicsTime(UDWORD timePeriod, UDWORD requiredRange)
{
	return graphicsTime % MAX(1, timePeriod) * requiredRange / MAX(1, timePeriod);
}

UDWORD getModularScaledRealTime(UDWORD timePeriod, UDWORD requiredRange)
{
	return realTime % MAX(1, timePeriod) * requiredRange / MAX(1, timePeriod);
}

void gameTimeUpdateBegin()
{
	deltaGameTime = 0;
	deltaGraphicsTime = 0;
	extraGameTicksProcessed = 0;
}

/* Call this each loop to update the game timer */
GameTimeUpdateResult gameTimeUpdate(bool mayUpdate, bool forceTryGameTickUpdate)
{
	GameTimeUpdateResult result = GameTimeUpdateResult::NO_UPDATE;

	uint32_t currTime = wzGetTicks();

	if (currTime < prevRealTime)
	{
		// Warzone 2100, the first relativistic computer game!
		// Exhibit A: Time travel
		// force a rebase
		debug(LOG_WARNING, "Time travel is occurring! Clock went back in time a bit from %d to %d!\n", prevRealTime, currTime);
		prevRealTime = currTime;
	}

	// Do not update the game time if gameTimeStop has been called
	if (stopCount != 0)
	{
		return GameTimeUpdateResult::NO_UPDATE;
	}

	// Calculate the new game time
	int newDeltaGraphicsTime = quantiseFraction(modifier.n, modifier.d, currTime, prevRealTime);
	ASSERT(newDeltaGraphicsTime >= 0, "Something very wrong.");

	uint32_t newGraphicsTime = graphicsTime + newDeltaGraphicsTime;

	if (newGraphicsTime > gameTime && !mayUpdate)
	{
		newGraphicsTime = gameTime;
		newDeltaGraphicsTime = newGraphicsTime - graphicsTime;
	}

	if (updateWantedTime == 0 && newGraphicsTime >= gameTime)
	{
		updateWantedTime = currTime;  // This is the time that we wanted to tick.
	}

	bool timeForGameTickUpdate = newGraphicsTime > gameTime;

	if ((timeForGameTickUpdate || forceTryGameTickUpdate) && !checkPlayerGameTime(NET_ALL_PLAYERS))
	{
		// Pause time at current game time, since we are waiting GAME_GAME_TIME from other players.
		newGraphicsTime = gameTime;
		newDeltaGraphicsTime = newGraphicsTime - graphicsTime;
		if (!waitingOnPlayersStartTime.has_value())
		{
			waitingOnPlayersStartTime = realTime;
		}

		bool shouldDisplayWaitingStatus = true;
		if (NetPlay.players[selectedPlayer].isSpectator && !NetPlay.isHost)
		{
			shouldDisplayWaitingStatus = (realTime - waitingOnPlayersStartTime.value_or(0)) > MIN_WAITONPLAYERS_DISPLAYTIME_FOR_SPECTATORS;
		}
		if (timeForGameTickUpdate && shouldDisplayWaitingStatus)
		{
			debug(LOG_SYNC, "Waiting for other players. gameTime = %u, player times are {%s}", gameTime, listToString("%u", ", ", gameQueueTime, gameQueueTime + MAX_CONNECTED_PLAYERS).c_str());

			for (unsigned player = 0; player < MAX_CONNECTED_PLAYERS; ++player)
			{
				if (!checkPlayerGameTime(player))
				{
					NETsetPlayerConnectionStatus(CONNECTIONSTATUS_WAITING_FOR_PLAYER, player);
					break;  // GAME_GAME_TIME is processed serially, so don't know if waiting for more players.
				}
			}
		}

		timeForGameTickUpdate = false;
		forceTryGameTickUpdate = false;
	}

	// Adjust deltas.
	if (timeForGameTickUpdate || forceTryGameTickUpdate)
	{
		// Update the game time.
		deltaGameTime = GAME_TICKS_PER_UPDATE;
		gameTime += deltaGameTime;

		waitingOnPlayersStartTime = nullopt;

		result = (timeForGameTickUpdate) ? GameTimeUpdateResult::GAME_TIME_UPDATED : GameTimeUpdateResult::GAME_TIME_UPDATED_FORCED;

		if (!timeForGameTickUpdate && forceTryGameTickUpdate)
		{
			extraGameTicksProcessed++;
		}

		updateLatency();
		if (crcError)
		{
			debug(LOG_ERROR, "Synch error, gameTimes were: {%s}", listToString("%7u", ", ", gameQueueCheckTime, gameQueueCheckTime + MAX_GAMEQUEUE_SLOTS).c_str());
			debug(LOG_ERROR, "Synch error, CRCs were:      {%s}", listToString(" 0x%04X", ", ", gameQueueCheckCrc, gameQueueCheckCrc + MAX_GAMEQUEUE_SLOTS).c_str());
			crcError = false;
		}
	}
	else
	{
		if (extraGameTicksProcessed > 0)
		{
			// If extra game ticks have been processed, newGraphicsTime/newDeltaGraphicsTime must be "fast-forwarded" to appropriate values based on gameTime
			newGraphicsTime = gameTime;
			newDeltaGraphicsTime = newGraphicsTime - graphicsTime;
		}

		// Update the graphics time.
		graphicsTime      = newGraphicsTime;
		deltaGraphicsTime = newDeltaGraphicsTime;

		// Update prevRealTime, since graphicsTime changed.
		prevRealTime      = currTime;
	}

	// Pre-calculate fraction used in timeAdjustedIncrement
	graphicsTimeFraction = (float)deltaGraphicsTime / (float)GAME_TICKS_PER_SEC;

	ASSERT(graphicsTime <= gameTime, "Trying to see the future.");
	return result;
}

void gameTimeUpdateEnd()
{
	deltaGameTime = 0;
}

void realTimeUpdate(void)
{
	uint32_t currTime = wzGetTicks();

	// now update realTime which does not pause
	// Store the real time
	deltaRealTime = currTime - realTime;
	realTime += deltaRealTime;

	deltaRealTime = MIN(deltaRealTime, GTIME_MAXFRAME);  // Don't scroll across the map suddenly, if computer freezes for a moment.

	// Pre-calculate fraction used in timeAdjustedIncrement
	realTimeFraction = (float)deltaRealTime / (float)GAME_TICKS_PER_SEC;
}

// reset the game time modifiers
void gameTimeResetMod(void)
{
	prevRealTime = wzGetTicks();

	modifier = 1;
}

// set the time modifier
void gameTimeSetMod(Rational mod)
{
	gameTimeResetMod();
	modifier = mod;
}

// get the current time modifier
Rational gameTimeGetMod()
{
	return modifier;
}

bool gameTimeIsStopped(void)
{
	return stopCount != 0;
}

/* Call this to stop the game timer */
void gameTimeStop(void)
{
	stopCount += 1;

	graphicsTimeFraction = 0.f;
}

/* Call this to restart the game timer after a call to gameTimeStop */
void gameTimeStart(void)
{
//	ASSERT(stopCount > 0, "Game started too many times.");

	prevRealTime = wzGetTicks();
	stopCount = std::max<int>(stopCount - 1, 0);
}

/* Call this to reset the game timer */
void gameTimeReset(UDWORD time)
{
	// reset the game timers
	setGameTime(time);
	gameTimeResetMod();
	realTime = wzGetTicks();
	deltaRealTime = 0;
}

void getTimeComponents(unsigned time, int *hours, int *minutes, int *seconds, int *milliseconds)
{
	*milliseconds = time % GAME_TICKS_PER_SEC;
	time /= GAME_TICKS_PER_SEC;

	*seconds = time % 60;
	time /= 60;

	*minutes = time % 60;
	time /= 60;

	*hours = time;
}

static void updateLatency()
{
	uint16_t maxWantedLatency = 0;
	unsigned player;
	uint16_t prevDiscreteChosenLatency = discreteChosenLatency;

	// Find out what latency has been agreed on, next.
	for (player = 0; player < MAX_CONNECTED_PLAYERS; ++player)
	{
		if (NetPlay.players[player].allocated  // Don't wait for dropped/kicked players.
			&& (NetPlay.players[player].position < game.maxPlayers) // should always be a valid map / player position
			&& (!NetPlay.players[player].isSpectator || player == NetPlay.hostPlayer)) // Don't wait for spectators (that are not the host)
		{
			//minWantedLatency = MIN(minWantedLatency, wantedLatencies[player]);  // Minimum, so the clients don't increase the latency to try to make one slow computer run faster.
			maxWantedLatency = MAX(maxWantedLatency, wantedLatencies[player]);  // Maximum, since the host experiences lower latency than everyone else.
		}
	}
	// Adjust the agreed latency. (Can maximum decrease by 5ms or increase by 30ms per update.
	chosenLatency = chosenLatency + clip(maxWantedLatency - chosenLatency, -5, 60);
	// Round the chosen latency to an integer number of updates, up to 10.
	discreteChosenLatency = clip((chosenLatency + GAME_TICKS_PER_UPDATE / 2) / GAME_TICKS_PER_UPDATE * GAME_TICKS_PER_UPDATE, GAME_TICKS_PER_UPDATE, GAME_TICKS_PER_UPDATE * GAME_UPDATES_PER_SEC);
	if (prevDiscreteChosenLatency != discreteChosenLatency)
	{
		debug(LOG_SYNC, "Adjusting latency %d -> %d", prevDiscreteChosenLatency, discreteChosenLatency);
	}

	// We want the chosen latency to increase by how much our update was delayed waiting for others, or to decrease by how long after we got the messages from others that it was time to tick. Plus a tiny 10ms buffer.
	// We will send this number to others.
	wantedLatency = static_cast<uint16_t>(clip<int>((int)(discreteChosenLatency + updateReadyTime - updateWantedTime + 10), 0, UINT16_MAX));

	// Reset the times, ready to be set again.
	updateReadyTime = 0;
	updateWantedTime = 0;
}

void sendPlayerGameTime()
{
	unsigned player;
	uint32_t latencyTicks = discreteChosenLatency / GAME_TICKS_PER_UPDATE;
	uint32_t checkTime = gameTime;
	GameCrcType checkCrc = nextDebugSync();

	for (player = 0; player < MAX_CONNECTED_PLAYERS; ++player)
	{
		if (!myResponsibility(player))
		{
			continue;
		}

		NETbeginEncode(NETgameQueue(player), GAME_GAME_TIME);
		NETuint32_t(&latencyTicks);
		NETuint32_t(&checkTime);
		NETuint16_t(&checkCrc);
		NETuint16_t(&wantedLatency);
		NETend();
	}
}

static inline bool shouldWaitForPlayerSlot(unsigned player)
{
	return NetPlay.players[player].allocated  // Don't wait for players that have been kicked/dropped.
	&& (!NetPlay.players[player].isSpectator || !ingame.TimeEveryoneIsInGame.has_value() || player == NetPlay.hostPlayer); // Don't wait for spectators (that are not the host) once the game has started
}

// publicly-exposed function
bool gtimeShouldWaitForPlayer(unsigned player)
{
	return shouldWaitForPlayerSlot(player);
}

static inline bool shouldCheckDebugSyncForPlayerSlot(unsigned player)
{
	return NetPlay.players[player].allocated	// human player
		&& !ingame.endTime.has_value()			// and game hasn't ended
		&& (!NetPlay.players[player].isSpectator || player == NetPlay.hostPlayer);
}

void recvPlayerGameTime(NETQUEUE queue)
{
	ASSERT(queue.index < MAX_GAMEQUEUE_SLOTS, "Unexpected queue.index: %" PRIu8 "", queue.index);

	uint32_t latencyTicks = 0;
	uint32_t checkTime = 0;
	GameCrcType checkCrc = 0;

	NETbeginDecode(queue, GAME_GAME_TIME);
	NETuint32_t(&latencyTicks);
	NETuint32_t(&checkTime);
	NETuint16_t(&checkCrc);
	NETuint16_t(&wantedLatencies[queue.index]);
	NETend();

	gameQueueTime[queue.index] = checkTime + latencyTicks * GAME_TICKS_PER_UPDATE;  // gameTime when future messages shall be processed.

	gameQueueCheckTime[queue.index] = checkTime;
	gameQueueCheckCrc[queue.index] = checkCrc;

	if (shouldCheckDebugSyncForPlayerSlot(queue.index))
	{
		syncDebug("GAME_GAME_TIME p%d;lat%u,ct%u,crc%04X,wlat%u", queue.index, latencyTicks, checkTime, checkCrc, wantedLatencies[queue.index]);
		if (!checkDebugSync(checkTime, checkCrc))
		{
			debug(LOG_ERROR, "Found CRC error when receiving GAME_GAME_TIME for player: %" PRIu8 " (checkTime: %" PRIu32 ", checkCrc: %" PRIu16 ")", queue.index, checkTime, checkCrc);
			crcError = true;
			if (NetPlay.players[queue.index].allocated)
			{
				NETsetPlayerConnectionStatus(CONNECTIONSTATUS_DESYNC, queue.index);
			}
		}
	}

	if (updateReadyTime == 0 && checkPlayerGameTime(NET_ALL_PLAYERS))
	{
		updateReadyTime = wzGetTicks();  // This is the time we were able to tick.
	}
}

bool checkPlayerGameTime(unsigned player)
{
	unsigned begin = player, end = player + 1;
	if (player == NET_ALL_PLAYERS)
	{
		begin = 0;
		end = MAX_CONNECTED_PLAYERS;
	}

	for (player = begin; player < end; ++player)
	{
		if (gameTime > gameQueueTime[player]
			&& shouldWaitForPlayerSlot(player))
		{
			return false;  // Still waiting for this player.
		}
	}

	return true;  // Have GAME_GAME_TIME from all players.
}

void setPlayerGameTime(unsigned player, uint32_t time)
{
	if (player == NET_ALL_PLAYERS)
	{
		for (player = 0; player < MAX_GAMEQUEUE_SLOTS; ++player)
		{
			gameQueueTime[player] = time;
		}
	}
	else
	{
		ASSERT(player < MAX_GAMEQUEUE_SLOTS, "Unexpected player: %u", player);
		gameQueueTime[player] = time;
	}
}
