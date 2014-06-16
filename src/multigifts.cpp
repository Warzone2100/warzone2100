/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
 * MultiGift.c
 * gifts one player can give to another..
 * Also home to Deathmatch hardcoded RULES.
 */

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/widget/widget.h"
#include "objmem.h"
#include "console.h"
#include "map.h"
#include "research.h"
#include "power.h"
#include "group.h"
#include "anim_id.h"
#include "hci.h"
#include "scriptfuncs.h"		// for objectinrange.
#include "lib/gamelib/gtime.h"
#include "effects.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"			// for samples.
#include "wrappers.h"			// for gameover..
#include "lib/script/script.h"
#include "scripttabs.h"
#include "scriptcb.h"
#include "loop.h"
#include "transporter.h"
#include "mission.h" // for INVALID_XY

#include "lib/netplay/netplay.h"
#include "multiplay.h"
#include "multigifts.h"
#include "multiint.h"			// for force name.
#include "multimenu.h"			// for multimenu
#include "multistat.h"
#include "random.h"
#include "keymap.h"

///////////////////////////////////////////////////////////////////////////////
// prototypes

static void		recvGiftDroids					(uint8_t from, uint8_t to, uint32_t droidID);
static void		sendGiftDroids					(uint8_t from, uint8_t to);
static void		giftResearch					(uint8_t from, uint8_t to, bool send);
static void	giftAutoGame(uint8_t from, uint8_t to, bool send);
///////////////////////////////////////////////////////////////////////////////
// gifts..

bool recvGift(NETQUEUE queue)
{
	uint8_t	type, from, to;
	int		audioTrack;
	uint32_t droidID;

	NETbeginDecode(queue, GAME_GIFT);
		NETuint8_t(&type);
		NETuint8_t(&from);
		NETuint8_t(&to);
		NETuint32_t(&droidID);
	NETend();

	if (!canGiveOrdersFor(queue.index, from))
	{
		debug(LOG_WARNING, "Gift (%d) from %d, to %d, queue.index %d", (int)type, (int)from, (int)to, (int)queue.index);
		syncDebug("Wrong player.");
		return false;
	}

	// Handle the gift depending on what it is
	switch (type)
	{
		case RADAR_GIFT:
			audioTrack = ID_SENSOR_DOWNLOAD;
			giftRadar(from, to, false);
			break;
		case DROID_GIFT:
			audioTrack = ID_UNITS_TRANSFER;
			recvGiftDroids(from, to, droidID);
			break;
		case RESEARCH_GIFT:
			audioTrack = ID_TECHNOLOGY_TRANSFER;
			giftResearch(from, to, false);
			break;
		case POWER_GIFT:
			audioTrack = ID_POWER_TRANSMIT;
			giftPower(from, to, droidID, false);
			break;
		case AUTOGAME_GIFT:
			audioTrack = ID_SOUND_NEXUS_SYNAPTIC_LINK;
			giftAutoGame(from, to, false);
			break;
		default:
			debug(LOG_ERROR, "recvGift: Unknown Gift recvd");
			return false;
			break;
	}

	// If we are on the recieving end play an audio alert
	if (to == selectedPlayer)
	{
		audio_QueueTrack(audioTrack);
	}
	return true;
}

bool sendGift(uint8_t type, uint8_t to)
{
	int audioTrack;

	switch (type)
	{
		case RADAR_GIFT:
			audioTrack = ID_SENSOR_DOWNLOAD;
			giftRadar(selectedPlayer, to, true);
			break;
		case DROID_GIFT:
			audioTrack = ID_UNITS_TRANSFER;
			sendGiftDroids(selectedPlayer, to);
			break;
		case RESEARCH_GIFT:
			audioTrack = ID_TECHNOLOGY_TRANSFER;
			giftResearch(selectedPlayer, to, true);
			break;
		case POWER_GIFT:
			audioTrack = ID_POWER_TRANSMIT;
			giftPower(selectedPlayer, to, 0, true);
			break;
		case AUTOGAME_GIFT:
			giftAutoGame(selectedPlayer, to, true);
			return true;
			break;
		default:
			debug( LOG_ERROR, "Unknown Gift sent" );

			return false;
			break;
	}

	// Play the appropriate audio track
	audio_QueueTrack(audioTrack);

	return true;
}

static void giftAutoGame(uint8_t from, uint8_t to, bool send)
{
	uint32_t dummy = 0;

	if (send)
	{
		uint8_t subType = AUTOGAME_GIFT;

		NETbeginEncode(NETgameQueue(selectedPlayer), GAME_GIFT);
			NETuint8_t(&subType);
			NETuint8_t(&from);
			NETuint8_t(&to);
			NETuint32_t(&dummy);
		NETend();
		debug(LOG_SYNC, "We (%d) are telling %d we want to enable/disable a autogame", from, to);
	}
	// If we are recieving the "gift"
	else
	{
		if (to == selectedPlayer)
		{
			NetPlay.players[from].autoGame = !NetPlay.players[from].autoGame ;
			CONPRINTF(ConsoleString, (ConsoleString, "%s has %s the autoGame command", getPlayerName(from), NetPlay.players[from].autoGame ? "Enabled" : "Disabled"));
			debug(LOG_SYNC, "We (%d) are being told that %d has %s autogame", selectedPlayer, from, NetPlay.players[from].autoGame ? "Enabled" : "Disabled");
		}
	}
}


// ////////////////////////////////////////////////////////////////////////////
// give radar information
void giftRadar(uint8_t from, uint8_t to, bool send)
{
	uint32_t dummy = 0;

	if (send)
	{
		uint8_t subType = RADAR_GIFT;

		NETbeginEncode(NETgameQueue(selectedPlayer), GAME_GIFT);
			NETuint8_t(&subType);
			NETuint8_t(&from);
			NETuint8_t(&to);
			NETuint32_t(&dummy);
		NETend();
	}
	// If we are recieving the gift
	else
	{
		hqReward(from, to);
		if (to == selectedPlayer)
		{
			CONPRINTF(ConsoleString, (ConsoleString, _("%s Gives You A Visibility Report"), getPlayerName(from)));
		}
	}
}

// recvGiftDroid()
// We received a droid gift from another player.
// NOTICE: the packet is already set-up for decoding via recvGift()
//
// \param from  :player that sent us the droid
// \param to    :player that should be getting the droid
static void recvGiftDroids(uint8_t from, uint8_t to, uint32_t droidID)
{
	DROID *psDroid = IdToDroid(droidID, from);

	if (psDroid)
	{
		syncDebugDroid(psDroid, '<');
		giftSingleDroid(psDroid, to);
		syncDebugDroid(psDroid, '>');
		if (to == selectedPlayer)
		{
			CONPRINTF(ConsoleString, (ConsoleString, _("%s Gives you a %s"), getPlayerName(from), psDroid->aName));
		}
	}
	else
	{
		debug(LOG_ERROR, "Bad droid id %u, from %u to %u", droidID, from, to);
	}
}

// sendGiftDroids()
// We give selected droid(s) as a gift to another player.
//
// \param from  :player that sent us the droid
// \param to    :player that should be getting the droid
static void sendGiftDroids(uint8_t from, uint8_t to)
{
	DROID        *psD;
	uint8_t      giftType = DROID_GIFT;
	uint8_t      totalToSend;

	if (apsDroidLists[from] == NULL)
	{
		return;
	}

	/*
	 * Work out the number of droids to send. As well as making sure they are
	 * selected we also need to make sure they will NOT put the receiving player
	 * over their droid limit.
	 */

	for (totalToSend = 0, psD = apsDroidLists[from];
	     psD && getNumDroids(to) + totalToSend < getMaxDroids(to) && totalToSend != UINT8_MAX;
	     psD = psD->psNext)
	{
		if (psD->selected)
				++totalToSend;
	}
	/*
	 * We must send one droid at a time, due to the fact that giftSingleDroid()
	 * does its own net calls.
	 */

	for (psD = apsDroidLists[from]; psD && totalToSend != 0; psD = psD->psNext)
	{
		if ((psD->droidType == DROID_TRANSPORTER || psD->droidType == DROID_SUPERTRANSPORTER)
		 && !transporterIsEmpty(psD))
		{
			CONPRINTF(ConsoleString, (ConsoleString, _("Tried to give away a non-empty %s - but this is not allowed."), psD->aName));
			continue;
		}
		if (psD->selected)
		{
			NETbeginEncode(NETgameQueue(selectedPlayer), GAME_GIFT);
			NETuint8_t(&giftType);
			NETuint8_t(&from);
			NETuint8_t(&to);
			// Add the droid to the packet
			NETuint32_t(&psD->id);
			NETend();

			// Decrement the number of droids left to send
			--totalToSend;
		}
	}
}


// ////////////////////////////////////////////////////////////////////////////
// give technologies.
static void giftResearch(uint8_t from, uint8_t to, bool send)
{
	int		i;
	uint32_t	dummy = 0;

	if (send)
	{
		uint8_t giftType = RESEARCH_GIFT;

		NETbeginEncode(NETgameQueue(selectedPlayer), GAME_GIFT);
			NETuint8_t(&giftType);
			NETuint8_t(&from);
			NETuint8_t(&to);
			NETuint32_t(&dummy);
		NETend();
	}
	else
	{
		if (to == selectedPlayer)
		{
			CONPRINTF(ConsoleString, (ConsoleString, _("%s Gives You Technology Documents"), getPlayerName(from)));
		}
		// For each topic
		for (i = 0; i < asResearch.size(); i++)
		{
			// If they have it and we don't research it
			if (IsResearchCompleted(&asPlayerResList[from][i])
			&& !IsResearchCompleted(&asPlayerResList[to][i]))
			{
				MakeResearchCompleted(&asPlayerResList[to][i]);
				researchResult(i, to, false, NULL, true);
			}
		}
	}
}


// ////////////////////////////////////////////////////////////////////////////
// give Power
void giftPower(uint8_t from, uint8_t to, uint32_t amount, bool send)
{
	if (send)
	{
		uint8_t giftType = POWER_GIFT;

		NETbeginEncode(NETgameQueue(selectedPlayer), GAME_GIFT);
			NETuint8_t(&giftType);
			NETuint8_t(&from);
			NETuint8_t(&to);
			NETuint32_t(&amount);
		NETend();
	}
	else
	{
		uint32_t gifval;

		if (from == ANYPLAYER || amount != 0)
		{
			gifval = amount;
		}
		else
		{
			// Give 1/3 of our power away
			gifval = getPower(from) / 3;
			usePower(from, gifval);
		}

		addPower(to, gifval);

		if (from != ANYPLAYER && to == selectedPlayer)
		{
			CONPRINTF(ConsoleString,(ConsoleString,_("%s Gives You %u Power"),getPlayerName(from),gifval));
		}
	}
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// alliance code......

void requestAlliance(uint8_t from, uint8_t to, bool prop, bool allowAudio)
{
	if (prop && bMultiMessages)
	{
		sendAlliance(from, to, ALLIANCE_REQUESTED, false);
		return;  // Wait for our message.
	}

	syncDebug("Request alliance %d %d", from, to);
	alliances[from][to] = ALLIANCE_REQUESTED;	// We've asked
	alliances[to][from] = ALLIANCE_INVITATION;	// They've been invited


	CBallFrom = from;
	CBallTo = to;
	eventFireCallbackTrigger((TRIGGER_TYPE) CALL_ALLIANCEOFFER);

	if (to == selectedPlayer)
	{
		CONPRINTF(ConsoleString,(ConsoleString,_("%s Requests An Alliance With You"),getPlayerName(from)));

		if (allowAudio)
		{
			audio_QueueTrack(ID_ALLIANCE_OFF);
		}
	}
	else if (from == selectedPlayer)
	{
		CONPRINTF(ConsoleString,(ConsoleString,_("You Invite %s To Form An Alliance"),getPlayerName(to)));
		if (allowAudio)
		{
			audio_QueueTrack(ID_ALLIANCE_OFF);
		}
	}
}

void breakAlliance(uint8_t p1, uint8_t p2, bool prop, bool allowAudio)
{
	char	tm1[128];

	if (prop && bMultiMessages)
	{
		sendAlliance(p1, p2, ALLIANCE_BROKEN, false);
		return;  // Wait for our message.
	}

	if (alliances[p1][p2] == ALLIANCE_FORMED)
	{
		sstrcpy(tm1, getPlayerName(p1));
		CONPRINTF(ConsoleString,(ConsoleString,_("%s Breaks The Alliance With %s"),tm1,getPlayerName(p2) ));

		if (allowAudio && (p1 == selectedPlayer || p2 == selectedPlayer))
		{
			audio_QueueTrack(ID_ALLIANCE_BRO);
		}
	}

	syncDebug("Break alliance %d %d", p1, p2);
	alliances[p1][p2] = ALLIANCE_BROKEN;
	alliances[p2][p1] = ALLIANCE_BROKEN;
	alliancebits[p1] &= ~(1 << p2);
	alliancebits[p2] &= ~(1 << p1);
}

void formAlliance(uint8_t p1, uint8_t p2, bool prop, bool allowAudio, bool allowNotification)
{
	DROID	*psDroid;
	char	tm1[128];

	if (bMultiMessages && prop)
	{
		sendAlliance(p1, p2, ALLIANCE_FORMED, false);
		return;  // Wait for our message.
	}

	// Don't add message if already allied
	if (bMultiPlayer && alliances[p1][p2] != ALLIANCE_FORMED && allowNotification)
	{
		sstrcpy(tm1, getPlayerName(p1));
		CONPRINTF(ConsoleString,(ConsoleString,_("%s Forms An Alliance With %s"),tm1,getPlayerName(p2)));
	}

	syncDebug("Form alliance %d %d", p1, p2);
	alliances[p1][p2] = ALLIANCE_FORMED;
	alliances[p2][p1] = ALLIANCE_FORMED;
	if (game.alliance == ALLIANCES_TEAMS)	// this is for shared vision only
	{
		alliancebits[p1] |= 1 << p2;
		alliancebits[p2] |= 1 << p1;
	}

	if (allowAudio && (p1 == selectedPlayer || p2== selectedPlayer))
	{
		audio_QueueTrack(ID_ALLIANCE_ACC);
	}

	// Not campaign and alliances are transitive
	if (game.alliance == ALLIANCES_TEAMS)
	{
		giftRadar(p1, p2, false);
		giftRadar(p2, p1, false);
	}

	// Clear out any attacking orders
	for (psDroid = apsDroidLists[p1]; psDroid; psDroid = psDroid->psNext)	// from -> to
	{
		if (psDroid->order.type == DORDER_ATTACK
		 && psDroid->order.psObj
		 && psDroid->order.psObj->player == p2)
		{
			orderDroid(psDroid, DORDER_STOP, ModeImmediate);
		}
	}
	for (psDroid = apsDroidLists[p2]; psDroid; psDroid = psDroid->psNext)	// to -> from
	{
		if (psDroid->order.type == DORDER_ATTACK
		 && psDroid->order.psObj
 		 && psDroid->order.psObj->player == p1)
		{
			orderDroid(psDroid, DORDER_STOP, ModeImmediate);
		}
	}
}



void sendAlliance(uint8_t from, uint8_t to, uint8_t state, int32_t value)
{
	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_ALLIANCE);
		NETuint8_t(&from);
		NETuint8_t(&to);
		NETuint8_t(&state);
		NETint32_t(&value);
	NETend();
}

bool recvAlliance(NETQUEUE queue, bool allowAudio)
{
	uint8_t to, from, state;
	int32_t value;

	NETbeginDecode(queue, GAME_ALLIANCE);
		NETuint8_t(&from);
		NETuint8_t(&to);
		NETuint8_t(&state);
		NETint32_t(&value);
	NETend();

	if (!canGiveOrdersFor(queue.index, from))
	{
		return false;
	}

	switch (state)
	{
		case ALLIANCE_NULL:
			break;
		case ALLIANCE_REQUESTED:
			requestAlliance(from, to, false, allowAudio);
			break;
		case ALLIANCE_FORMED:
			formAlliance(from, to, false, allowAudio, true);
			break;
		case ALLIANCE_BROKEN:
			breakAlliance(from, to, false, allowAudio);
			break;
		default:
			debug(LOG_ERROR, "Unknown alliance state recvd.");
			return false;
			break;
	}

	return true;
}


// ////////////////////////////////////////////////////////////////////////////
// add an artifact on destruction if required.
void  technologyGiveAway(const STRUCTURE *pS)
{
	// If a fully built factory (or with modules under construction) which is our responsibility got destroyed
	if (pS->pStructureType->type == REF_FACTORY && (pS->status == SS_BUILT || pS->currentBuildPts >= pS->body))
	{
		syncDebug("Adding artefact.");
	}
	else
	{
		syncDebug("Not adding artefact.");
		return;
	}

	int featureIndex;
	for (featureIndex = 0; featureIndex < numFeatureStats && asFeatureStats[featureIndex].subType != FEAT_GEN_ARTE; ++featureIndex) {}
	if (featureIndex >= numFeatureStats)
	{
		debug(LOG_WARNING, "No artefact feature!");
		return;
	}

	uint32_t x = map_coord(pS->pos.x), y = map_coord(pS->pos.y);
	if (!pickATileGen(&x, &y, LOOK_FOR_EMPTY_TILE, zonedPAT))
	{
		syncDebug("Did not find location for oil drum.");
		debug(LOG_FEATURE, "Unable to find a free location.");
		return;
	}
	FEATURE *pF = buildFeature(&asFeatureStats[featureIndex], world_coord(x), world_coord(y), false);
	if (pF)
	{
		pF->player = pS->player;
		syncDebugFeature(pF, '+');
	}
	else
	{
		debug(LOG_ERROR, "Couldn't build artefact?");
	}
}

/** Sends a build order for the given feature type to all players
 *  \param subType the type of feature to build
 *  \param x,y the coordinates to place the feature at
 */
void sendMultiPlayerFeature(uint32_t ref, uint32_t x, uint32_t y, uint32_t id)
{
	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_DEBUG_ADD_FEATURE);
	{
		NETuint32_t(&ref);
		NETuint32_t(&x);
		NETuint32_t(&y);
		NETuint32_t(&id);
	}
	NETend();
}

void recvMultiPlayerFeature(NETQUEUE queue)
{
	uint32_t ref = 0xff, x =0, y = 0, id = 0;
	unsigned int i;

	NETbeginDecode(queue, GAME_DEBUG_ADD_FEATURE);
	{
		NETuint32_t(&ref);
		NETuint32_t(&x);
		NETuint32_t(&y);
		NETuint32_t(&id);
	}
	NETend();

	if (!getDebugMappingStatus() && bMultiPlayer)
	{
		debug(LOG_WARNING, "Failed to add feature for player %u.", NetPlay.players[queue.index].position);
		return;
	}

	// Find the feature stats list that contains the feature type we want to build
	for (i = 0; i < numFeatureStats; ++i)
	{
		// If we found the correct feature type
		if (asFeatureStats[i].ref == ref)
		{
			// Create a feature of the specified type at the given location
			FEATURE *result = buildFeature(&asFeatureStats[i], x, y, false);
			result->id = id;
			break;
		}
	}
}

bool addOilDrum(uint8_t count)
{
	syncDebug("Adding %d oil drums.", count);

	int featureIndex;
	for (featureIndex = 0; featureIndex < numFeatureStats && asFeatureStats[featureIndex].subType != FEAT_OIL_DRUM; ++featureIndex) {}
	if (featureIndex >= numFeatureStats)
	{
		debug(LOG_WARNING, "No oil drum feature!");
		return false;  // Return value ignored.
	}

	for (unsigned n = 0; n < count; ++n)
	{
		uint32_t x, y;
		for (int i = 0; i < 3; ++i)  // try three times
		{
			// Between 10 and mapwidth - 10
			x = gameRand(mapWidth - 20) + 10;
			y = gameRand(mapHeight - 20) + 10;

			if (pickATileGen(&x, &y, LOOK_FOR_EMPTY_TILE, zonedPAT))
			{
				break;
			}
			x = INVALID_XY;
		}
		if (x == INVALID_XY)
		{
			syncDebug("Did not find location for oil drum.");
			debug(LOG_FEATURE, "Unable to find a free location.");
			continue;
		}
		FEATURE *pF = buildFeature(&asFeatureStats[featureIndex], world_coord(x), world_coord(y), false);
		if (pF)
		{
			pF->player = ANYPLAYER;
			syncDebugFeature(pF, '+');
		}
		else
		{
			debug(LOG_ERROR, "Couldn't build oil drum?");
		}
	}
	return true;
}

// ///////////////////////////////////////////////////////////////
bool pickupArtefact(int toPlayer, int fromPlayer)
{
	if (fromPlayer < MAX_PLAYERS && bMultiPlayer)
	{
		for (int topic = asResearch.size() - 1; topic >= 0; topic--)
		{
			if (IsResearchCompleted(&asPlayerResList[fromPlayer][topic])
			 && !IsResearchPossible(&asPlayerResList[toPlayer][topic]))
			{
				// Make sure the topic can be researched
				if (asResearch[topic].researchPower && asResearch[topic].researchPoints)
				{
					MakeResearchPossible(&asPlayerResList[toPlayer][topic]);
					if (toPlayer == selectedPlayer)
					{
						CONPRINTF(ConsoleString,(ConsoleString,_("You Discover Blueprints For %s"), getName(asResearch[topic].pName)));
					}
					break;
				}
				// Invalid topic
				else
				{
					debug(LOG_WARNING, "%s is a invalid research topic?", getName(asResearch[topic].pName));
				}
			}
		}

		if (toPlayer == selectedPlayer)
		{
			audio_QueueTrack(ID_SOUND_ARTIFACT_RECOVERED);
		}

		return true;
	}

	return false;
}

/* Ally team members with each other */
void createTeamAlliances(void)
{
	int i, j;

	debug(LOG_WZ, "Creating teams");

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		for (j = 0; j < MAX_PLAYERS; j++)
		{
			if (i != j
			 && NetPlay.players[i].team == NetPlay.players[j].team	// two different players belonging to the same team
			 && !aiCheckAlliances(i, j)
			 && game.skDiff[i]
			 && game.skDiff[j])	// Not allied and not ignoring teams
			{
				// Create silently
				formAlliance(i, j, false, false, false);
			}
		}
	}
}
