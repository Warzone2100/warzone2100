/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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
#include "lib/framework/strres.h"

#include "stats.h"
#include "lib/gamelib/gtime.h"
#include "map.h"
#include "objects.h"
#include "display.h"								// for checking if droid in view.
#include "order.h"
#include "action.h"
#include "hci.h"									// for byte packing funcs.
#include "display3ddef.h"							// tile size constants.
#include "console.h"
#include "geometry.h"								// for gettilestructure
#include "mapgrid.h"								// for move droids directly.
#include "lib/netplay/netplay.h"
#include "multiplay.h"
#include "frontend.h"								// for titlemode
#include "multistat.h"
#include "power.h"									// for power checks
#include "multirecv.h"
#include "random.h"

static void NETauto(PACKAGED_CHECK *v)
{
	NETauto(&v->player);
	NETauto(&v->droidID);
	NETauto(&v->order);
	NETauto(&v->secondaryOrder);
	NETauto(&v->body);
	NETauto(&v->experience);
	NETauto(&v->pos);
	NETauto(&v->rot);
	if (v->order == DORDER_ATTACK)
	{
		NETauto(&v->targetID);
	}
	else if (v->order == DORDER_MOVE)
	{
		NETauto(&v->orderX);
		NETauto(&v->orderY);
	}
}

// ////////////////////////////////////////////////////////////////////////////
// function definitions

static bool sendStructureCheck	(void);							//Structure
static PACKAGED_CHECK packageCheck(const DROID *pD);
static bool sendDroidCheck		(void);							//droids

static bool sendPowerCheck(void);
static UDWORD averagePing(void);

#define AV_PING_FREQUENCY       20000                           // how often to update average pingtimes. in approx millisecs.
#define PING_FREQUENCY          4000                            // how often to update pingtimes. in approx millisecs.
#define STRUCT_PERIOD           4000                            // how often (ms) to send a structure check.
#define DROID_PERIOD            315                             // how often (ms) to send droid checks
#define POWER_PERIOD            5000                            // how often to send power levels
#define SCORE_FREQUENCY         108000                          // how often to update global score.

static UDWORD				PingSend[MAX_PLAYERS];	//stores the time the ping was called.

// ////////////////////////////////////////////////////////////////////////////
// test traffic level.
static bool okToSend(void)
{
	// Update checks and go no further if any exceeded.
	// removing the received check again ... add NETgetRecentBytesRecvd() to left hand side of equation if this works badly
	if (NETgetRecentBytesSent() >= MAX_BYTESPERSEC)
	{
		return false;
	}

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// Droid checking info. keep position and damage in sync.
void sendCheck()
{
	NETgetBytesSent();			// update stats.
	NETgetBytesRecvd();
	NETgetPacketsSent();
	NETgetPacketsRecvd();

	// dont send checks till all players are present.
	for (unsigned i = 0; i < MAX_PLAYERS; ++i)
	{
		if(isHumanPlayer(i) && ingame.JoiningInProgress[i])
		{
			return;
		}
	}

	// send Checks. note each send has it's own send criteria, so might not send anything.
	// Priority is droids -> structures -> power -> score -> ping

	sendDroidCheck();
	sync_counter.sentDroidCheck++;
	sendStructureCheck();
	sync_counter.sentStructureCheck++;
	sendPowerCheck();
	sync_counter.sentPowerCheck++;
	if(okToSend())
	{
		sendScoreCheck();
		sync_counter.sentScoreCheck++;
	}
	else
	{
		sync_counter.unsentScoreCheck++;
	}
	if(okToSend())
	{
		sendPing();
		sync_counter.sentPing++;
	}
	else
	{
		sync_counter.unsentPing++;
	}
}

// ////////////////////////////////////////////////////////////////////////////
// pick a droid to send, NULL otherwise.
static DROID* pickADroid(void)
{
	DROID *pD, *ret = NULL;  // ret: dummy initialisation.
	unsigned player = MAX_PLAYERS;
	unsigned i;

	// Pick a random player who has at least one droid.
	for (i = 0; i < 200; ++i)
	{
		unsigned p = gameRand(MAX_PLAYERS);
		if (apsDroidLists[p] != NULL)
		{
			player = p;
			break;
		}
	}

	if (player == MAX_PLAYERS)
	{
		return NULL;  // No players have any droids, with high probability...
	}

	// O(n) where n is number of droids. Slow, but hard to beat on a linked list. (One call of a pick n droids function would be just as fast.)
	i = 0;
	for (pD = apsDroidLists[player]; pD != NULL; pD = pD->psNext)
	{
		if (gameRand(++i) == 0)
		{
			ret = pD;
		}
	}

	return ret;
}

/** Force a droid to be synced
 *
 *  Call this when you need to update the given droid right now.
 */
bool ForceDroidSync(const DROID* droidToSend)
{
	uint8_t count = 1;		// *always* one
	PACKAGED_CHECK pc = packageCheck(droidToSend);

	ASSERT(droidToSend != NULL, "NULL pointer passed");

	debug(LOG_SYNC, "Force sync of droid %u from player %u", droidToSend->id, droidToSend->player);

	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_CHECK_DROID);
		NETuint8_t(&count);
		NETuint32_t(&gameTime);  // Send game time.
		NETauto(&pc);
	return NETend();
}

// ///////////////////////////////////////////////////////////////////////////
// send a droid info packet.
static bool sendDroidCheck(void)
{
	DROID			*pD, **ppD;
	uint8_t			i, count;
	static UDWORD	lastSent = 0;		// Last time a struct was sent.
	UDWORD			toSend = 12;

	if (lastSent > gameTime)
	{
		lastSent= 0;
	}

	// Only send a struct send if not done recently
	if (gameTime - lastSent < DROID_PERIOD)
	{
		return true;
	}

	lastSent = gameTime;


	if (!isInSync())  // Don't really send anything, unless out of synch.
	{
		NETbeginEncode(NETgameQueue(selectedPlayer), GAME_CHECK_DROID);
	}

		// Allocate space for the list of droids to send
		ppD = (DROID **)alloca(sizeof(DROID *) * toSend);

		// Get the list of droids to sent
		for (i = 0, count = 0; i < toSend; i++)
		{
			pD = pickADroid();

			if (pD == NULL || (pD->gameCheckDroid != NULL && pD->gameCheckDroid->gameTime > gameTime))
			{
				continue;  // Didn't find a droid, or droid was synched recently.
			}

			// If the droid is ours add it to the list
			if (myResponsibility(pD->player))
			{
				ppD[count++] = pD;
			}
			delete pD->gameCheckDroid;
			pD->gameCheckDroid = new PACKAGED_CHECK(packageCheck(pD));
		}

		if (!isInSync())  // Don't really send anything, unless out of synch.
		{
			// Send the number of droids to expect
			NETuint8_t(&count);
			NETuint32_t(&gameTime);  // Send game time.

			// Add the droids to the packet
			for (i = 0; i < count; i++)
			{
				NETauto(ppD[i]->gameCheckDroid);
			}
		}

	if (!isInSync())  // Don't really send anything, unless out of synch.
	{
		return NETend();
	}

	return true;
}

#define MIN_DELAY_BETWEEN_DROID_SYNCHS 5000  // Must be longer than maximum possible latency.
// ////////////////////////////////////////////////////////////////////////////
// Send a Single Droid Check message
static PACKAGED_CHECK packageCheck(const DROID *pD)
{
	PACKAGED_CHECK pc;
	memset(&pc, 0xFF, sizeof(pc));  // Dummy initialisation.
	pc.gameTime = gameTime + MIN_DELAY_BETWEEN_DROID_SYNCHS;

	pc.player = pD->player;
	pc.droidID = pD->id;
	pc.order = pD->order;
	pc.secondaryOrder = pD->secondaryOrder;
	pc.body = pD->body;
	if (pD->body > pD->originalBody)
	{
		ASSERT(false, "Droid %u body is too high before synch, is %u, which is more than %u.", pc.droidID, pD->body, pD->originalBody);
	}
	pc.experience = pD->experience;
	pc.pos = droidGetPrecisePosition(pD);
	pc.rot = pD->rot;
	if (pD->order == DORDER_ATTACK)
	{
		pc.targetID = pD->psTarget->id;
	}
	else if (pD->order == DORDER_MOVE)
	{
		pc.orderX = pD->orderX;
		pc.orderY = pD->orderY;
	}
	return pc;
}


// ////////////////////////////////////////////////////////////////////////////
// receive a check and update the local world state accordingly
bool recvDroidCheck(NETQUEUE queue)
{
	uint8_t		count;
	int		i;
	uint32_t        synchTime;

	NETbeginDecode(queue, GAME_CHECK_DROID);

		// Get the number of droids to expect
		NETuint8_t(&count);
		NETuint32_t(&synchTime);  // Get game time.

		for (i = 0; i < count; i++)
		{
			DROID *         pD;
			PACKAGED_CHECK  pc;
			Position        precPos;

			NETauto(&pc);

			// Find the droid in question
			pD = IdToDroid(pc.droidID, pc.player);
			if (!pD)
			{
				NETlogEntry("Recvd Unknown droid info. val=player", SYNC_FLAG, pc.player);
				debug(LOG_SYNC, "Received checking info for an unknown (as yet) droid. player:%d ref:%d", pc.player, pc.droidID);
				continue;
			}

			syncDebugDroid(pD, '<');

			if (pD->gameCheckDroid == NULL)
			{
				debug(LOG_SYNC, "We got a droid %u synch, but we couldn't find the droid!", pc.droidID);
				continue;  // Can't synch, since we didn't save data to be able to calculate a delta.
			}

			PACKAGED_CHECK const pc2 = *pD->gameCheckDroid;

			if (pc2.gameTime != synchTime + MIN_DELAY_BETWEEN_DROID_SYNCHS)
			{
				debug(LOG_SYNC, "We got a droid %u synch, but we didn't choose the same droid to synch.", pc.droidID);
				pD->gameCheckDroid->gameTime = synchTime + MIN_DELAY_BETWEEN_DROID_SYNCHS;  // Get droid synch time back in synch.
				continue;  // Can't synch, since we didn't save data to be able to calculate a delta.
			}

#define MERGECOPYSYNC(x, y, z)  if (pc.y != pc2.y) { debug(LOG_SYNC, "Droid %u out of synch, changing "#x" from %"z" to %"z".", pc.droidID, x, pc.y);             x = pc.y; }
#define MERGEDELTA(x, y, z) if (pc.y != pc2.y) { debug(LOG_SYNC, "Droid %u out of synch, changing "#x" from %"z" to %"z".", pc.droidID, x, x + pc.y - pc2.y); x += pc.y - pc2.y; }
			// player not synched here...
			precPos = droidGetPrecisePosition(pD);
			MERGEDELTA(precPos.x, pos.x, "d");
			MERGEDELTA(precPos.y, pos.y, "d");
			MERGEDELTA(precPos.z, pos.z, "d");
			droidSetPrecisePosition(pD, precPos);
			MERGEDELTA(pD->rot.direction, rot.direction, "d");
			MERGEDELTA(pD->rot.pitch, rot.pitch, "d");
			MERGEDELTA(pD->rot.roll, rot.roll, "d");
			MERGEDELTA(pD->body, body, "u");
			if (pD->body > pD->originalBody)
			{
				pD->body = pD->originalBody;
				debug(LOG_SYNC, "Droid %u body was too high after synch, reducing to %u.", pc.droidID, pD->body);
			}
			MERGEDELTA(pD->experience, experience, "u");

			if (pc.pos.x != pc2.pos.x || pc.pos.y != pc2.pos.y)
			{
				// snap droid(if on ground) to terrain level at x,y.
				if ((asPropulsionStats + pD->asBits[COMP_PROPULSION].nStat)->propulsionType != PROPULSION_TYPE_LIFT)  // if not airborne.
				{
					pD->pos.z = map_Height(pD->pos.x, pD->pos.y);
				}
			}

			// Doesn't cover all cases, but at least doesn't actively break stuff randomly.
			switch (pc.order)
			{
				case DORDER_MOVE:
					if (pc.order != pc2.order || pc.orderX != pc2.orderX || pc.orderY != pc2.orderY)
					{
						debug(LOG_SYNC, "Droid %u out of synch, changing order from %s to %s(%d, %d).", pc.droidID, getDroidOrderName((DROID_ORDER)pc2.order), getDroidOrderName((DROID_ORDER)pc.order), pc.orderX, pc.orderY);
						// reroute the droid.
						orderDroidLoc(pD, (DROID_ORDER)pc.order, pc.orderX, pc.orderY, ModeImmediate);
					}
					break;
				case DORDER_ATTACK:
					if (pc.order != pc2.order || pc.targetID != pc2.targetID)
					{
						BASE_OBJECT *obj = IdToPointer(pc.targetID, ANYPLAYER);
						if (obj != NULL)
						{
							debug(LOG_SYNC, "Droid %u out of synch, changing order from %s to %s(%u).", pc.droidID, getDroidOrderName((DROID_ORDER)pc2.order), getDroidOrderName((DROID_ORDER)pc.order), pc.targetID);
							// remote droid is attacking, not here tho!
							orderDroidObj(pD, (DROID_ORDER)pc.order, IdToPointer(pc.targetID, ANYPLAYER), ModeImmediate);
						}
						else
						{
							debug(LOG_SYNC, "Droid %u out of synch, would change order from %s to %s(%u), but can't find target.", pc.droidID, getDroidOrderName((DROID_ORDER)pc2.order), getDroidOrderName((DROID_ORDER)pc.order), pc.targetID);
						}
					}
					break;
				case DORDER_NONE:
				case DORDER_GUARD:
					if (pc.order != pc2.order)
					{
						DROID_ORDER_DATA sOrder;
						memset(&sOrder, 0, sizeof(DROID_ORDER_DATA));
						sOrder.order = (DROID_ORDER)pc.order;

						debug(LOG_SYNC, "Droid %u out of synch, changing order from %s to %s.", pc.droidID, getDroidOrderName((DROID_ORDER)pc2.order), getDroidOrderName((DROID_ORDER)pc.order));
						turnOffMultiMsg(true);
						moveStopDroid(pD);
						orderDroidBase(pD, &sOrder);
						turnOffMultiMsg(false);
					}
					break;
				default:
					break;  // Don't know what to do, but at least won't be actively breaking anything.
			}

			MERGECOPYSYNC(pD->secondaryOrder, secondaryOrder, "u");  // The old code set this after changing orders, so doing that in case.
#undef MERGECOPYSYNC
#undef MERGEDELTA

			syncDebugDroid(pD, '>');

			// ...and repeat!
		}

	NETend();

	return true;
}

// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Structure Checking, to ensure smoke and stuff is consistent across machines.
// this func is recursive!
static STRUCTURE *pickAStructure(unsigned player)
{
	STRUCTURE *pS, *ret = NULL;
	unsigned i;

	// O(n) where n is number of structures. Slow, but hard to beat on a linked list.
	i = 0;
	for (pS = apsStructLists[player]; pS != NULL; pS = pS->psNext)
	{
		if (gameRand(++i) == 0)
		{
			ret = pS;
		}
	}

	return ret;
}

static uint32_t structureCheckLastSent = 0;  // Last time a struct was sent
static uint32_t structureCheckLastId[MAX_PLAYERS];
static uint32_t structureCheckLastBody[MAX_PLAYERS];
static Rotation structureCheckLastDirection[MAX_PLAYERS];
static uint32_t structureCheckLastType[MAX_PLAYERS];

// ////////////////////////////////////////////////////////////////////////
// Send structure information.
static bool sendStructureCheck(void)
{
	uint8_t         player;

	if (structureCheckLastSent > gameTime)
	{
		structureCheckLastSent = 0;
	}
	// Only send a struct send if not done recently
	if (gameTime - structureCheckLastSent < STRUCT_PERIOD)
	{
		return true;
	}

	structureCheckLastSent = gameTime;

	for (player = 0; player < MAX_PLAYERS; ++player)
	{
		STRUCTURE       *pS = pickAStructure(player);
		bool            hasCapacity = true;
		uint8_t         capacity;

		// Only send info about complete buildings
		if (pS == NULL || pS->status != SS_BUILT)
		{
			structureCheckLastId[player] = 0;
			continue;
		}

		switch (pS->pStructureType->type)
		{
			case REF_RESEARCH:
				capacity = pS->pFunctionality->researchFacility.capacity;
				break;
			case REF_FACTORY:
			case REF_VTOL_FACTORY:
				capacity = pS->pFunctionality->factory.capacity;
				break;
			case REF_POWER_GEN:
				capacity = pS->pFunctionality->powerGenerator.capacity;
			default:
				hasCapacity = false;
				break;
		}
		structureCheckLastId[player] = pS->id;
		structureCheckLastBody[player] = pS->body;
		structureCheckLastDirection[player] = pS->rot;
		structureCheckLastType[player] = pS->pStructureType->type;

		if (myResponsibility(player))
		{
			if (!isInSync())  // Don't really send anything, unless out of synch.
			{
				NETbeginEncode(NETgameQueue(selectedPlayer), GAME_CHECK_STRUCT);
					NETuint8_t(&player);
					NETuint32_t(&gameTime);
					NETuint32_t(&pS->id);
					NETuint32_t(&pS->body);
					NETenum(&pS->pStructureType->type);
					NETRotation(&pS->rot);
					if (hasCapacity)
					{
						NETuint8_t(&capacity);
					}
				NETend();
			}
		}
	}

	return true;
}

// receive checking info about a structure and update local world state
bool recvStructureCheck(NETQUEUE queue)
{
	uint32_t                synchTime;
	STRUCTURE		*pS;
	bool			hasCapacity = true;
	int                     j;
	Rotation                rot;
	uint8_t			player, ourCapacity;
	uint32_t		body;
	uint32_t                ref;
	STRUCTURE_TYPE          type = REF_HQ;  // Dummy initialisation.

	NETbeginDecode(queue, GAME_CHECK_STRUCT);
		NETuint8_t(&player);
		NETuint32_t(&synchTime);
		NETuint32_t(&ref);
		NETuint32_t(&body);
		NETenum(&type);
		NETRotation(&rot);

		if (player >= MAX_PLAYERS)
		{
			debug(LOG_ERROR, "Bad GAME_CHECK_STRUCT received!");
			NETend();
			return false;
		}

		if (structureCheckLastSent != synchTime)
		{
			debug(LOG_ERROR, "We got a structure synch at the wrong time.");
		}

		if (ref != structureCheckLastId[player])
		{
			debug(LOG_ERROR, "We got a structure %u synch, but had chosen %u instead.", ref, structureCheckLastId[player]);
			NETend();
			return false;
		}

		// If the structure exists our job is easy
		pS = IdToStruct(ref, player);
		if (pS)
		{
			syncDebugStructure(pS, '<');
			if (pS->pStructureType->type != structureCheckLastType[player] || type != structureCheckLastType[player])
			{
				debug(LOG_ERROR, "GAME_CHECK_STRUCT received, wrong structure type!");
				NETend();
				return false;
			}

			// Check its finished
			if (pS->status != SS_BUILT)
			{
				pS->rot = rot;
				pS->id = ref;
				pS->status = SS_BUILT;
				buildingComplete(pS);
			}

			// If the structure has a capacity
			switch (pS->pStructureType->type)
			{
				case REF_RESEARCH:
					ourCapacity = ((RESEARCH_FACILITY *) pS->pFunctionality)->capacity;
					j = researchModuleStat;
					break;
				case REF_FACTORY:
				case REF_VTOL_FACTORY:
					ourCapacity = ((FACTORY *) pS->pFunctionality)->capacity;
					j = factoryModuleStat;
					break;
				case REF_POWER_GEN:
					ourCapacity = ((POWER_GEN *) pS->pFunctionality)->capacity;
					j = powerModuleStat;
					break;
				default:
					hasCapacity = false;
					break;
			}

			// So long as the struct has a capacity fetch it from the packet
			if (hasCapacity)
			{
				uint8_t actualCapacity = 0;

				NETuint8_t(&actualCapacity);

				// If our capacity is different upgrade ourself
				for (; ourCapacity < actualCapacity; ourCapacity++)
				{
					debug(LOG_SYNC, "Structure %u out of synch, adding module.", ref);
					buildStructure(&asStructureStats[j], pS->pos.x, pS->pos.y, pS->player, false);

					// Check it is finished
					if (pS->status != SS_BUILT)
					{
						pS->id = ref;
						pS->status = SS_BUILT;
						buildingComplete(pS);
					}
				}
			}

#define MERGEDELTA(x, y, ya, z) if (y != ya) { debug(LOG_SYNC, "Structure %u out of synch, changing "#x" from %"z" to %"z".", ref, x, x + y - ya); x += y - ya; }
			MERGEDELTA(pS->body, body, structureCheckLastBody[player], "u");
			MERGEDELTA(pS->rot.direction, rot.direction, structureCheckLastDirection[player].direction, "d");
			MERGEDELTA(pS->rot.pitch, rot.pitch, structureCheckLastDirection[player].pitch, "d");
			MERGEDELTA(pS->rot.roll, rot.roll, structureCheckLastDirection[player].roll, "d");
#undef MERGEDELTA

			syncDebugStructure(pS, '>');
		}
		else
		{
			debug(LOG_ERROR, "We got a structure %u synch, but can't find the structure.", ref);
		}

	NETend();
	return true;
}

static uint32_t powerCheckLastSent = 0;
static int64_t powerCheckLastPower[MAX_PLAYERS];

// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Power Checking. Send a power level check every now and again.
static bool sendPowerCheck()
{
	uint8_t         player;

	if (powerCheckLastSent > gameTime)
	{
		powerCheckLastSent = 0;
	}

	// Only send if not done recently
	if (gameTime - powerCheckLastSent < POWER_PERIOD)
	{
		return true;
	}

	powerCheckLastSent = gameTime;
	for (player = 0; player < MAX_PLAYERS; ++player)
	{
		powerCheckLastPower[player] = getPrecisePower(player);
		if (myResponsibility(player))
		{
			if (!isInSync())  // Don't really send anything, unless out of synch.
			{
				NETbeginEncode(NETgameQueue(selectedPlayer), GAME_CHECK_POWER);
					NETuint8_t(&player);
					NETuint32_t(&gameTime);
					NETint64_t(&powerCheckLastPower[player]);
				NETend();
			}
		}
	}
	return true;
}

bool recvPowerCheck(NETQUEUE queue)
{
	uint8_t		player;
	uint32_t        synchTime;
	int64_t         power;

	NETbeginDecode(queue, GAME_CHECK_POWER);
		NETuint8_t(&player);
		NETuint32_t(&synchTime);
		NETint64_t(&power);
	NETend();

	if (powerCheckLastSent != synchTime)
	{
		debug(LOG_ERROR, "Power synch check out of synch!");
		return false;
	}

	if (player >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "Bad GAME_CHECK_POWER packet: player is %d : %s",
		      (int)player, isHumanPlayer(player) ? "Human" : "AI");
		return false;
	}

	if (power != powerCheckLastPower[player])
	{
		int64_t powerFrom = getPrecisePower(player);
		int64_t powerTo = powerFrom + power - powerCheckLastPower[player];
		debug(LOG_SYNC, "GAME_CHECK_POWER: Adjusting power for player %d (%s) from %lf to %lf",
		      (int)player, isHumanPlayer(player) ? "Human" : "AI", powerFrom/4294967296., powerTo/4294967296.);
		syncDebug("Adjusting power for player %d (%s) from %"PRId64" to %"PRId64"", (int)player, isHumanPlayer(player) ? "Human" : "AI", powerFrom, powerTo);
		setPrecisePower(player, powerTo);
	}
	return true;
}

// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Score
// We use setMultiStats() to broadcast the score when needed.
bool sendScoreCheck(void)
{
	static UDWORD	lastsent = 0;

	if (lastsent > gameTime)
	{
		lastsent= 0;
	}

	if (gameTime - lastsent < SCORE_FREQUENCY)
	{
		return true;
	}

	lastsent = gameTime;

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

	for(i=0;i<MAX_PLAYERS;i++)
	{
		if(isHumanPlayer(i))
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

	NETbeginEncode(NETbroadcastQueue(), NET_PING);
		NETuint8_t(&player);
		NETbool(&isNew);
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

	NETbeginDecode(queue, NET_PING);
		NETuint8_t(&sender);
		NETbool(&isNew);
	NETend();

	if (sender >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "Bad NET_PING packet, sender is %d", (int)sender);
		return false;
	}

	// If this is a new ping, respond to it
	if (isNew)
	{
		NETbeginEncode(NETnetQueue(sender), NET_PING);
			// We are responding to a new ping
			isNew = false;

			NETuint8_t(&us);
			NETbool(&isNew);
		NETend();
	}
	// They are responding to one of our pings
	else
	{
		// Work out how long it took them to respond
		ingame.PingTimes[sender] = (realTime - PingSend[sender]) / 2;

		// Note that we have received it
		PingSend[sender] = 0;
	}

	return true;
}
