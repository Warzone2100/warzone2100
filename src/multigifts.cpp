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

///////////////////////////////////////////////////////////////////////////////
// prototypes

static void		recvGiftDroids					(uint8_t from, uint8_t to, uint32_t droidID);
static void		sendGiftDroids					(uint8_t from, uint8_t to);
static void		giftResearch					(uint8_t from, uint8_t to, bool send);

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
		default:
			debug( LOG_ERROR, "Unknown Gift sent" );

			return false;
			break;
	}

	// Play the appropriate audio track
	audio_QueueTrack(audioTrack);

	return true;
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
		if (psD->droidType == DROID_TRANSPORTER
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
	uint8_t			count = 1;
	FEATURE_TYPE	type = FEAT_GEN_ARTE;

	// If a fully built factory (or with modules under construction) which is our responsibility got destroyed
	if (pS->pStructureType->type == REF_FACTORY && (pS->status == SS_BUILT || pS->currentBuildPts >= pS->body)
	 && myResponsibility(pS->player))
	{
		uint32_t x = map_coord(pS->pos.x);
		uint32_t y = map_coord(pS->pos.y);
		uint32_t id = generateNewObjectId();

		// Pick a tile to place the artifact
		if (!pickATileGen(&x, &y, LOOK_FOR_EMPTY_TILE, zonedPAT))
		{
			ASSERT(false, "technologyGiveAway: Unable to find a free location");
		}

		NETbeginEncode(NETgameQueue(selectedPlayer), GAME_ARTIFACTS);
		{
			/* Make sure that we don't have to violate the constness of pS.
			 * Since the nettype functions aren't const correct when sending
			 */
			uint8_t player = pS->player;

			NETuint8_t(&count);
			NETenum(&type);
			NETuint32_t(&x);
			NETuint32_t(&y);
			NETuint32_t(&id);
			NETuint8_t(&player);
		}
		NETend();
	}

	return;
}

/** Sends a build order for the given feature type to all players
 *  \param subType the type of feature to build
 *  \param x,y the coordinates to place the feature at
 */
void sendMultiPlayerFeature(FEATURE_TYPE subType, uint32_t x, uint32_t y, uint32_t id)
{
	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_FEATURES);
	{
		NETenum(&subType);
		NETuint32_t(&x);
		NETuint32_t(&y);
		NETuint32_t(&id);
	}
	NETend();
}

void recvMultiPlayerFeature(NETQUEUE queue)
{
	FEATURE_TYPE subType = FEAT_TREE;  // Dummy initialisation.
	uint32_t     x, y, id;
	unsigned int i;

	NETbeginDecode(queue, GAME_FEATURES);
	{
		NETenum(&subType);
		NETuint32_t(&x);
		NETuint32_t(&y);
		NETuint32_t(&id);
	}
	NETend();

	// Find the feature stats list that contains the feature type we want to build
	for (i = 0; i < numFeatureStats; ++i)
	{
		// If we found the correct feature type
		if (asFeatureStats[i].subType == subType)
		{
			// Create a feature of the specified type at the given location
			FEATURE *result = buildFeature(&asFeatureStats[i], x, y, false);
			result->id = id;
			break;
		}
	}
}
// must match _feature_type in featuredef.h
static const char *feature_names[] =
{
	"FEAT_UNUSED",
	"FEAT_HOVER",
	"FEAT_TANK",
	"FEAT_GEN_ARTE",
	"FEAT_OIL_RESOURCE",
	"FEAT_BOULDER",
	"FEAT_VEHICLE",
	"FEAT_BUILDING",
	"FEAT_UNUSED",
	"FEAT_LOS_OBJ",
	"FEAT_OIL_DRUM",
	"FEAT_TREE",
	"FEAT_SKYSCRAPER",
};
///////////////////////////////////////////////////////////////////////////////
// splatter artifact gifts randomly about.
void  addMultiPlayerRandomArtifacts(uint8_t quantity, FEATURE_TYPE type)
{
	int             i, count;
	uint32_t	x, y;
	uint8_t		player = ANYPLAYER;

	debug(LOG_FEATURE, "Sending %u artifact(s) type: (%s)", quantity, feature_names[type]);
	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_ARTIFACTS);
		NETuint8_t(&quantity);
		NETenum(&type);

		ASSERT(mapWidth > 20, "map not big enough");
		ASSERT(mapHeight > 20, "map not big enough");

		for (count = 0; count < quantity; count++)
		{
			uint32_t id = generateNewObjectId();

			for (i = 0; i < 3; i++) // try three times
			{
				// Between 10 and mapwidth - 10
				x = (rand()%(mapWidth - 20)) + 10;
				y = (rand()%(mapHeight - 20)) + 10;

				if (pickATileGen(&x, &y, LOOK_FOR_EMPTY_TILE, zonedPAT))
				{
					break;
				}
				else if (i == 2)
				{
					debug(LOG_FEATURE, "Unable to find a free location after 3 tries; giving up.");
					x = INVALID_XY;
				}
			}

			NETuint32_t(&x);
			NETuint32_t(&y);
			NETuint32_t(&id);
			NETuint8_t(&player);
		}

	NETend();
}

// ///////////////////////////////////////////////////////////////
bool addOilDrum(uint8_t count)
{
	addMultiPlayerRandomArtifacts(count, FEAT_OIL_DRUM);
	return true;
}

// ///////////////////////////////////////////////////////////////
// receive splattered artifacts
void recvMultiPlayerRandomArtifacts(NETQUEUE queue)
{
	int				count, i;
	uint8_t			quantity, player;
	uint32_t		tx,ty;
	uint32_t		ref;
	FEATURE_TYPE            type = FEAT_TREE;  // Dummy initialisation.
	FEATURE 		*pF;

	NETbeginDecode(queue, GAME_ARTIFACTS);
		NETuint8_t(&quantity);
		NETenum(&type);

	debug(LOG_FEATURE, "receiving %u artifact(s) type: (%s)", quantity, feature_names[type]);
	for (i = 0; i < numFeatureStats && asFeatureStats[i].subType != type; i++) {}

	for (count = 0; count < quantity; count++)
	{
		MAPTILE *psTile;

		NETuint32_t(&tx);
		NETuint32_t(&ty);
		NETuint32_t(&ref);
		NETuint8_t(&player);

		if (tx == INVALID_XY)
		{
			continue;
		}
		else if (!tileOnMap(tx, ty))
		{
			debug(LOG_ERROR, "Bad tile coordinates (%u,%u)", tx, ty);
			continue;
		}
		psTile = mapTile(tx, ty);
		if (!psTile || psTile->psObject != NULL)
		{
			debug(LOG_ERROR, "Already something at (%u,%u)!", tx, ty);
			continue;
		}

		pF = buildFeature((asFeatureStats + i), world_coord(tx), world_coord(ty), false);
		if (pF)
		{
			pF->id		= ref;
			pF->player	= player;
			syncDebugFeature(pF, '+');
		}
		else
		{
			debug(LOG_ERROR, "Couldn't build feature %u for player %u ?", ref, player);
		}
	}
	NETend();
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

		audio_QueueTrack(ID_SOUND_ARTIFACT_RECOVERED);

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
