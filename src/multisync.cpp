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
#include "main.h"								// for gamemode
#include "multistat.h"
#include "multirecv.h"
#include "stdinreader.h"

#include <array>

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

// ////////////////////////////////////////////////////////////////////////////
// function definitions

static UDWORD averagePing();

#define AV_PING_FREQUENCY       20000                           // how often to update average pingtimes. in approx millisecs.
#define PING_FREQUENCY          4000                            // how often to update pingtimes. in approx millisecs.

static UDWORD				PingSend[MAX_CONNECTED_PLAYERS];	//stores the time the ping was called.
#define PING_CHALLENGE_BYTES 32
static_assert(PING_CHALLENGE_BYTES % 8 == 0, "Must be a multiple of 8 bytes");
typedef std::array<uint8_t, PING_CHALLENGE_BYTES> PingChallengeBytes;
static std::array<optional<PingChallengeBytes>, MAX_CONNECTED_PLAYERS> pingChallenges;  // Random data sent with the last ping.


// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Score
// We use setMultiStats() to broadcast the score when needed.
bool sendScoreCheck()
{
	// Broadcast any changes in other players, but not in FRONTEND!!!
	// Detection for this no longer uses title mode, but instead game mode, because that makes more sense
	if (GetGameMode() == GS_NORMAL)
	{
		uint8_t			i;

		for (i = 0; i < MAX_CONNECTED_PLAYERS; i++)
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

static UDWORD averagePing()
{
	UDWORD i, count = 0, total = 0;

	for (i = 0; i < MAX_CONNECTED_PLAYERS; i++)
	{
		if (isHumanPlayer(i))
		{
			total += ingame.PingTimes[i];
			count ++;
		}
	}
	return total / MAX(count, 1);
}

void multiSyncResetAllChallenges()
{
	for (auto& challenge : pingChallenges)
	{
		challenge.reset();
	}
}

void multiSyncResetPlayerChallenge(uint32_t playerIdx)
{
	if (!NetPlay.isHost) { return; }
	pingChallenges[playerIdx].reset();
}

void multiSyncPlayerSwap(uint32_t playerIndexA, uint32_t playerIndexB)
{
	if (!NetPlay.isHost) { return; }
	std::swap(pingChallenges[playerIndexA], pingChallenges[playerIndexB]);
}

static inline PingChallengeBytes generatePingChallenge(uint8_t playerIdx)
{
	PingChallengeBytes bytes;
	if (NetPlay.isHost && !ingame.VerifiedIdentity[playerIdx])
	{
		// generate secure random challenges until the player verifies their identity
		genSecRandomBytes(bytes.data(), bytes.size());
	}
	else
	{
		uint8_t *pBytesDst = bytes.data();
		uint8_t *pBytesEnd = bytes.data() + bytes.size();
		for (; pBytesDst <= (pBytesEnd - sizeof(uint64_t)); )
		{
			uint64_t pingChallengei = (uint64_t)rand() << 32 | rand();
			memcpy(pBytesDst, &pingChallengei, sizeof(uint64_t));
			pBytesDst += sizeof(uint64_t);
		}
	}
	return bytes;
}

bool sendPing()
{
	bool			isNew = true;
	uint8_t			player = selectedPlayer;
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

	for (int i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
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

	if (NetPlay.isHost)
	{
		// Generate a unique ping challenge for each player, and send them individually
		for (int i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
		{
			if (isHumanPlayer(i)
				&& i != selectedPlayer)
			{
				pingChallenges[i] = generatePingChallenge(i);

				NETbeginEncode(NETnetQueue(i), NET_PING);
				NETuint8_t(&player);
				NETbool(&isNew);
				NETbin(pingChallenges[i].value().data(), PING_CHALLENGE_BYTES);
				NETend();

				// Note when we sent the ping
				PingSend[i] = realTime;
			}
		}
	}
	else
	{
		// Just generate and broadcast the same ping challenge to all other players
		pingChallenges[0] = generatePingChallenge(0);

		NETbeginEncode(NETbroadcastQueue(), NET_PING);
		NETuint8_t(&player);
		NETbool(&isNew);
		NETbin(pingChallenges[0].value().data(), PING_CHALLENGE_BYTES);
		NETend();

		// Note when we sent the ping
		for (int i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
		{
			PingSend[i] = realTime;
		}
	}

	return true;
}

inline optional<PingChallengeBytes>& expectedPingChallengeBytes(uint8_t playerIdx)
{
	return pingChallenges[(NetPlay.isHost) ? playerIdx : 0];
}

// accept and process incoming ping messages.
bool recvPing(NETQUEUE queue)
{
	bool	isNew = false;
	uint8_t	sender, us = selectedPlayer;
	uint8_t challenge[PING_CHALLENGE_BYTES];
	EcKey::Sig challengeResponse;

	NETbeginDecode(queue, NET_PING);
	NETuint8_t(&sender);
	NETbool(&isNew);
	if (isNew)
	{
		NETbin(challenge, PING_CHALLENGE_BYTES);
	}
	else
	{
		NETbytes(&challengeResponse);
	}
	NETend();

	if (sender >= MAX_CONNECTED_PLAYERS)
	{
		debug(LOG_ERROR, "Bad NET_PING packet, sender is %d", (int)sender);
		return false;
	}

	if (whosResponsible(sender) != queue.index)
	{
		HandleBadParam("NET_PING given incorrect params.", sender, queue.index);
		return false;
	}

	// If this is a new ping, respond to it
	if (isNew)
	{
		challengeResponse = getMultiStats(us).identity.sign(&challenge, PING_CHALLENGE_BYTES);

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
		auto& expectedPingChallenge = expectedPingChallengeBytes(sender);
		if (!expectedPingChallenge.has_value())
		{
			// Someone is sending a NET_PING but we do not have a valid challenge for them... ignore
			return false;
		}

		bool verifiedResponse = false;
		const auto& senderIdentity = getMultiStats(sender).identity;
		if (!senderIdentity.empty())
		{
			verifiedResponse = senderIdentity.verify(challengeResponse, expectedPingChallenge.value().data(), PING_CHALLENGE_BYTES);
		}
		if (!verifiedResponse)
		{
			// Either bad signature, or we sent more than one ping packet and this response is to an older one than the latest.
			debug(LOG_NEVER, "Bad and/or old NET_PING packet, alleged sender is %d", (int)sender);
			return false;
		}

		// Work out how long it took them to respond
		ingame.PingTimes[sender] = (realTime - PingSend[sender]) / 2;

		if (!ingame.VerifiedIdentity[sender])
		{
			// Record that they've verified the identity
			ingame.VerifiedIdentity[sender] = true;

			// Output to stdinterface, if enabled
			std::string senderPublicKeyB64 = base64Encode(senderIdentity.toBytes(EcKey::Public));
			std::string senderIdentityHash = senderIdentity.publicHashString();
			wz_command_interface_output("WZEVENT: player identity VERIFIED: %" PRIu32 " %s %s\n", sender, senderPublicKeyB64.c_str(), senderIdentityHash.c_str());
		}

		// Note that we have received it
		PingSend[sender] = 0;
		multiSyncResetPlayerChallenge(sender);
	}

	return true;
}
