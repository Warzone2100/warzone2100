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
/*
 * MultiSync.c
 *
 * synching issues
 * This file handles the constant backstream of net info, checking that recvd info
 * is concurrent with the local world, and correcting as required. Magic happens here.
 *
 * All conflicts due to non-guaranteed messaging are detected/resolved here.
 *
 * Alex Lee, pumpkin Studios, bath.
 */

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/gamelib/gtime.h"
#include "lib/netplay/netplay.h"
#include "multiplay.h"
#include "frontend.h"								// for titlemode
#include "multistat.h"
#include "multirecv.h"


// ////////////////////////////////////////////////////////////////////////////
// function definitions

static UDWORD averagePing(void);

#define AV_PING_FREQUENCY       20000                           // how often to update average pingtimes. in approx millisecs.
#define PING_FREQUENCY          4000                            // how often to update pingtimes. in approx millisecs.

static UDWORD				PingSend[MAX_PLAYERS];	//stores the time the ping was called.
static uint8_t pingChallenge[8];                                // Random data sent with the last ping.


// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Score
// We use setMultiStats() to broadcast the score when needed.
bool sendScoreCheck(void)
{
	// Broadcast any changes in other players, but not in FRONTEND!!!
	if (titleMode != MULTIOPTION && titleMode != MULTILIMIT)
	{
		uint8_t			i;

		for (i = 0; i < game.maxPlayers; i++)
		{
			// Host controls AI's scores + his own...
			if (myResponsibility(i))
			{
				// Send score to everyone else
				setMultiStats(i, getMultiStats(i), false);
			}
		}
	}

	return true;
}

// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Pings

static UDWORD averagePing(void)
{
	UDWORD i, count = 0, total = 0;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (isHumanPlayer(i))
		{
			total += ingame.PingTimes[i];
			count ++;
		}
	}
	return total / MAX(count, 1);
}

bool sendPing(void)
{
	bool			isNew = true;
	uint8_t			player = selectedPlayer;
	int				i;
	static UDWORD	lastPing = 0;	// Last time we sent a ping
	static UDWORD	lastav = 0;		// Last time we updated average

	// Only ping every so often
	if (lastPing > realTime)
	{
		lastPing = 0;
	}

	if (realTime - lastPing < PING_FREQUENCY)
	{
		return true;
	}

	lastPing = realTime;

	// If host, also update the average ping stat for joiners
	if (NetPlay.isHost)
	{
		if (lastav > realTime)
		{
			lastav = 0;
		}

		if (realTime - lastav > AV_PING_FREQUENCY)
		{
			NETsetGameFlags(2, averagePing());
			lastav = realTime;
		}
	}

	/*
	 * Before we send the ping, if any player failed to respond to the last one
	 * we should re-enumerate the players.
	 */

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (isHumanPlayer(i)
		    && PingSend[i]
		    && ingame.PingTimes[i]
		    && i != selectedPlayer)
		{
			ingame.PingTimes[i] = PING_LIMIT;
		}
		else if (!isHumanPlayer(i)
		         && PingSend[i]
		         && ingame.PingTimes[i]
		         && i != selectedPlayer)
		{
			ingame.PingTimes[i] = 0;
		}
	}

	uint64_t pingChallengei = (uint64_t)rand() << 32 | rand();
	memcpy(pingChallenge, &pingChallengei, sizeof(pingChallenge));

	NETbeginEncode(NETbroadcastQueue(), NET_PING);
	NETuint8_t(&player);
	NETbool(&isNew);
	NETbin(pingChallenge, sizeof(pingChallenge));
	NETend();

	// Note when we sent the ping
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		PingSend[i] = realTime;
	}

	return true;
}

// accept and process incoming ping messages.
bool recvPing(NETQUEUE queue)
{
	bool	isNew;
	uint8_t	sender, us = selectedPlayer;
	uint8_t challenge[sizeof(pingChallenge)];
	EcKey::Sig challengeResponse;

	NETbeginDecode(queue, NET_PING);
	NETuint8_t(&sender);
	NETbool(&isNew);
	if (isNew)
	{
		NETbin(challenge, sizeof(pingChallenge));
	}
	else
	{
		NETbytes(&challengeResponse);
	}
	NETend();

	if (sender >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "Bad NET_PING packet, sender is %d", (int)sender);
		return false;
	}

	// If this is a new ping, respond to it
	if (isNew)
	{
		challengeResponse = getMultiStats(us).identity.sign(&challenge, sizeof(pingChallenge));

		NETbeginEncode(NETnetQueue(sender), NET_PING);
		// We are responding to a new ping
		isNew = false;

		NETuint8_t(&us);
		NETbool(&isNew);
		NETbytes(&challengeResponse);
		NETend();
	}
	// They are responding to one of our pings
	else
	{
		if (!getMultiStats(sender).identity.empty() && !getMultiStats(sender).identity.verify(challengeResponse, pingChallenge, sizeof(pingChallenge)))
		{
			debug(LOG_ERROR, "Bad NET_PING packet, alleged sender is %d", (int)sender);
			return false;
		}

		// Work out how long it took them to respond
		ingame.PingTimes[sender] = (realTime - PingSend[sender]) / 2;

		// Note that we have received it
		PingSend[sender] = 0;
	}

	return true;
}
