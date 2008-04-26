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

#include "lib/netplay/netplay.h"
#include "multiplay.h"
#include "multigifts.h"
#include "multiint.h"			// for force name.
#include "multimenu.h"			// for multimenu
#include "multistat.h"

///////////////////////////////////////////////////////////////////////////////
// prototypes

static void		recvGiftDroids					(uint8_t from, uint8_t to, uint32_t droidID);
static void		sendGiftDroids					(uint8_t from, uint8_t to);
static void		giftResearch					(uint8_t from, uint8_t to, BOOL send);

///////////////////////////////////////////////////////////////////////////////
// gifts..

BOOL recvGift(void)
{
	uint8_t	type, from, to;
	int		audioTrack;
	uint32_t droidID;

	NETbeginDecode(NET_GIFT);
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
			giftPower(from, to, false);
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

BOOL sendGift(uint8_t type, uint8_t to)
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
			giftPower(selectedPlayer, to, true);
			break;
		default:
			debug( LOG_ERROR, "Unknown Gift sent" );
			abort();
			return false;
			break;
	}

	// Play the appropriate audio track
	audio_QueueTrack(audioTrack);

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// give radar information
void giftRadar(uint8_t from, uint8_t to, BOOL send)
{
	uint32_t dummy = 0;

	hqReward(from, to);

	if (send)
	{
		uint8_t subType = RADAR_GIFT;

		NETbeginEncode(NET_GIFT, NET_ALL_PLAYERS);
			NETuint8_t(&subType);
			NETuint8_t(&from);
			NETuint8_t(&to);
			NETuint32_t(&dummy);
		NETend();
	}
	// If we are recieving the gift
	else if (to == selectedPlayer)
	{
		CONPRINTF(ConsoleString,(ConsoleString,_("%s Gives You A Visibility Report"),
		          getPlayerName(from)));
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
	DROID		*psDroid;

	if (IdToDroid(droidID, from, &psDroid))
	{
		giftSingleDroid(psDroid, to);
		if (to == selectedPlayer)
		{
			CONPRINTF(ConsoleString, (ConsoleString, _("%s Gives you a %s"), getPlayerName(from), psDroid->aName));
		}
	}
	else
	{
		debug(LOG_ERROR, "recvGiftDroids: Bad droid id %d", (int)droidID);
	}
}

// sendGiftDroids()
// We give selected droid(s) as a gift to another player.
//
// \param from  :player that sent us the droid
// \param to    :player that should be getting the droid
static void sendGiftDroids(uint8_t from, uint8_t to)
{
	DROID        *next, *psD;
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

	for (psD = apsDroidLists[from]; psD && totalToSend != 0; psD = next)
	{
		// Store the next droid in the list as the list may change
		next = psD->psNext;

		if (psD->droidType == DROID_TRANSPORTER)
		{
			CONPRINTF(ConsoleString, (ConsoleString, _("Tried to give away a %s - but this is not allowed."), psD->aName));
			continue;
		}
		if (psD->selected)
		{
			NETbeginEncode(NET_GIFT, NET_ALL_PLAYERS);
			NETuint8_t(&giftType);
			NETuint8_t(&from);
			NETuint8_t(&to);
			// Add the droid to the packet
			NETuint32_t(&psD->id);
			NETend();

			// Hand over the droid on our sidde
			giftSingleDroid(psD, to);

			// Decrement the number of droids left to send
			--totalToSend;
		}
	}
}


// ////////////////////////////////////////////////////////////////////////////
// give technologies.
static void giftResearch(uint8_t from, uint8_t to, BOOL send)
{
	PLAYER_RESEARCH	*pR, *pRto;
	int		i;
	uint32_t	dummy = 0;

	pR	 = asPlayerResList[from];
	pRto = asPlayerResList[to];

	// For each topic
	for (i = 0; i < numResearch; i++)
	{
		// If they have it and we don't research it
		if (IsResearchCompleted(&pR[i])
		 && !IsResearchCompleted(&pRto[i]))
		{
			MakeResearchCompleted(&pRto[i]);
			researchResult(i, to, false, NULL);
		}
	}

	if (send)
	{
		uint8_t giftType = RESEARCH_GIFT;

		NETbeginEncode(NET_GIFT, NET_ALL_PLAYERS);
			NETuint8_t(&giftType);
			NETuint8_t(&from);
			NETuint8_t(&to);
			NETuint32_t(&dummy);
		NETend();
	}
	else if (to == selectedPlayer)
	{
		CONPRINTF(ConsoleString,(ConsoleString,_("%s Gives You Technology Documents"),getPlayerName(from) ));
	}
}


// ////////////////////////////////////////////////////////////////////////////
// give Power
void giftPower(uint8_t from, uint8_t to, BOOL send)
{
	UDWORD gifval;
	uint32_t dummy = 0;

	if (from == ANYPLAYER)
	{
		gifval = OILDRUM_POWER;
	}
	else
	{
		// Give 1/3 of our power away
		gifval = getPower(from) / 3;
		usePower(from, gifval);
	}

	addPower(to, gifval);

	if (send)
	{
		uint8_t giftType = POWER_GIFT;

		NETbeginEncode(NET_GIFT, NET_ALL_PLAYERS);
			NETuint8_t(&giftType);
			NETuint8_t(&from);
			NETuint8_t(&to);
			NETuint32_t(&dummy);
		NETend();
	}
	else if (to == selectedPlayer)
	{
		CONPRINTF(ConsoleString,(ConsoleString,_("%s Gives You Power"),getPlayerName(from)));
	}
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// alliance code......

void requestAlliance(uint8_t from, uint8_t to, BOOL prop, BOOL allowAudio)
{
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

	if (prop)
	{
		sendAlliance(from, to, ALLIANCE_REQUESTED, false);
	}
}

void breakAlliance(uint8_t p1, uint8_t p2, BOOL prop, BOOL allowAudio)
{
	char	tm1[128];

	if (alliances[p1][p2] == ALLIANCE_FORMED)
	{
		strlcpy(tm1, getPlayerName(p1), sizeof(tm1));
		CONPRINTF(ConsoleString,(ConsoleString,_("%s Breaks The Alliance With %s"),tm1,getPlayerName(p2) ));

		if (allowAudio && (p1 == selectedPlayer || p2 == selectedPlayer))
		{
			audio_QueueTrack(ID_ALLIANCE_BRO);
		}
	}

	alliances[p1][p2] = ALLIANCE_BROKEN;
	alliances[p2][p1] = ALLIANCE_BROKEN;

	if (prop)
	{
		sendAlliance(p1, p2, ALLIANCE_BROKEN, false);
	}
}

void formAlliance(uint8_t p1, uint8_t p2, BOOL prop, BOOL allowAudio, BOOL allowNotification)
{
	DROID	*psDroid;
	char	tm1[128];

	// Don't add message if already allied
	if (bMultiPlayer && alliances[p1][p2] != ALLIANCE_FORMED && allowNotification)
	{
		strlcpy(tm1, getPlayerName(p1), sizeof(tm1));
		CONPRINTF(ConsoleString,(ConsoleString,_("%s Forms An Alliance With %s"),tm1,getPlayerName(p2)));
	}

	alliances[p1][p2] = ALLIANCE_FORMED;
	alliances[p2][p1] = ALLIANCE_FORMED;


	if (allowAudio && (p1 == selectedPlayer || p2== selectedPlayer))
	{
		audio_QueueTrack(ID_ALLIANCE_ACC);
	}

	if (bMultiPlayer && prop)
	{
		sendAlliance(p1, p2, ALLIANCE_FORMED, false);
	}

	// Not campaign and alliances are transitive
	if ((bMultiPlayer || game.type == SKIRMISH) && game.alliance == ALLIANCES_TEAMS)
	{
		giftRadar(p1, p2, false);
		giftRadar(p2, p1, false);
	}

	// Clear out any attacking orders
	turnOffMultiMsg(true);

	for (psDroid = apsDroidLists[p1]; psDroid; psDroid = psDroid->psNext)	// from -> to
	{
		if (psDroid->order == DORDER_ATTACK
		 && psDroid->psTarget
		 && psDroid->psTarget->player == p2)
		{
			orderDroid(psDroid, DORDER_STOP);
		}
	}
	for (psDroid = apsDroidLists[p2]; psDroid; psDroid = psDroid->psNext)	// to -> from
	{
		if (psDroid->order == DORDER_ATTACK
		 && psDroid->psTarget
 		 && psDroid->psTarget->player == p1)
		{
			orderDroid(psDroid,DORDER_STOP);
		}
	}

	turnOffMultiMsg(false);
}



void sendAlliance(uint8_t from, uint8_t to, uint8_t state, int32_t value)
{
	NETbeginEncode(NET_ALLIANCE, NET_ALL_PLAYERS);
		NETuint8_t(&from);
		NETuint8_t(&to);
		NETuint8_t(&state);
		NETint32_t(&value);
	NETend();
}

BOOL recvAlliance(BOOL allowAudio)
{
	uint8_t to, from, state;
	int32_t value;

	NETbeginDecode(NET_ALLIANCE);
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
	int				i;
	uint8_t			count = 1;
	uint32_t		x, y;
	FEATURE			*pF = NULL;
	FEATURE_TYPE	type = FEAT_GEN_ARTE;

	// If a factory which is our responsibility got destroyed
	if (pS->pStructureType->type == REF_FACTORY
	 && myResponsibility(pS->player))
	{
		x = map_coord(pS->pos.x);
		y = map_coord(pS->pos.y);

		// Pick a tile to place the artifact
		if (!pickATileGen(&x, &y, LOOK_FOR_EMPTY_TILE, zonedPAT))
		{
			ASSERT(false, "technologyGiveAway: Unable to find a free location");
		}

		// Get the feature offset
		for(i = 0; i < numFeatureStats && asFeatureStats[i].subType != FEAT_GEN_ARTE; i++);

		// 'Build' the artifact
		pF = buildFeature((asFeatureStats + i), world_coord(x), world_coord(y), false);
		if (pF)
		{
			pF->player = pS->player;
		}

		NETbeginEncode(NET_ARTIFACTS, NET_ALL_PLAYERS);
		{
			/* Make sure that we don't have to violate the constness of pS.
			 * Since the nettype functions aren't const correct when sending
			 */
			uint8_t player = pS->player;

			NETuint8_t(&count);
			NETenum(&type);
			NETuint32_t(&x);
			NETuint32_t(&y);
			NETuint32_t(&pF->id);
			NETuint8_t(&player);
		}
		NETend();
	}

	return;
}



///////////////////////////////////////////////////////////////////////////////
// loooosseeeerrr  gifts
// if the player is losing then give a simple gift.

#define GIFTFREQ (1000*(60*5))			// every 5 mins tops..

static void addLoserGifts(void)
{
	static UDWORD	lastgift = 0;
	int				count, i;
	uint8_t			player = ONEPLAYER;
	uint8_t			quantity;
	uint32_t		x, y;
	FEATURE			*pF;
	FEATURE_TYPE	type = FEAT_OIL_DRUM;
	STRUCTURE		*psStruct;

	if (lastgift > gameTime)
		lastgift = 0;	// might be a restart

	// Player has no power, so give the player some oil
	if (apsStructLists[selectedPlayer] && getPower(selectedPlayer) < 10)
	{
		// Only proceed if it's been a while
		if (gameTime - lastgift < GIFTFREQ)
		{
			return;
		}

		// Only proceed if no powergen
		for (psStruct = apsStructLists[selectedPlayer];
			 psStruct && psStruct->pStructureType->type != REF_POWER_GEN;
			 psStruct = psStruct->psNext);
		if (psStruct)
		{
			return;
		}

		lastgift = gameTime;

		for (i = 0;
			 i < numFeatureStats && asFeatureStats[i].subType != FEAT_OIL_DRUM;
			 i++);

		quantity = rand() % 5 + 1;

		NETbeginEncode(NET_ARTIFACTS, NET_ALL_PLAYERS);
			NETuint8_t(&quantity);
			NETenum(&type);

			for (count = 0; count < quantity; count++)
			{
				x = map_coord(apsStructLists[selectedPlayer]->pos.x);
				y = map_coord(apsStructLists[selectedPlayer]->pos.y);

				if (!pickATileGen(&x, &y, LOOK_FOR_EMPTY_TILE, zonedPAT))
				{
					ASSERT(false, "addlosergifts: Unable to find a free location");
				}

				NETlogEntry("gift", 0, 0);

				pF = buildFeature((asFeatureStats + i), world_coord(x), world_coord(y), false);

				NETuint32_t(&x);
				NETuint32_t(&y);
				NETuint32_t(&pF->id);
				NETuint8_t(&player);

				if (pF)
				{
					// Flag for multiplayer artifacts
					pF->player = player;
				}
			}

		NETend();
		audio_QueueTrack(ID_GIFT);
	}
}

/** Sends a build order for the given feature type to all players
 *  \param subType the type of feature to build
 *  \param x,y the coordinates to place the feature at
 */
void sendMultiPlayerFeature(FEATURE_TYPE subType, uint32_t x, uint32_t y)
{
	NETbeginEncode(NET_FEATURES, NET_ALL_PLAYERS);
	{
		NETenum(&subType);
		NETuint32_t(&x);
		NETuint32_t(&y);
	}
	NETend();
}

void recvMultiPlayerFeature()
{
	FEATURE_TYPE subType;
	uint32_t     x, y;
	unsigned int i;

	NETbeginDecode(NET_FEATURES);
	{
		NETenum(&subType);
		NETuint32_t(&x);
		NETuint32_t(&y);
	}
	NETend();

	// Find the feature stats list that contains the feature type we want to build
	for (i = 0; i < numFeatureStats; ++i)
	{
		// If we found the correct feature type
		if (asFeatureStats[i].subType == subType)
		{
			// Create a feature of the specified type at the given location
			buildFeature(&asFeatureStats[i], x, y, false);

			break;
		}
	}
}
///////////////////////////////////////////////////////////////////////////////
// splatter artifact gifts randomly about.
void  addMultiPlayerRandomArtifacts(uint8_t quantity, FEATURE_TYPE type)
{
	FEATURE		*pF;
	int			i, count;
	uint32_t	x, y;
	uint8_t		player = ANYPLAYER;

	NETbeginEncode(NET_ARTIFACTS, NET_ALL_PLAYERS);
		NETuint8_t(&quantity);
		NETenum(&type);

		for(i = 0; i < numFeatureStats && asFeatureStats[i].subType != type; i++);

		ASSERT(mapWidth > 20, "map not big enough");
		ASSERT(mapHeight > 20, "map not big enough");

		for (count = 0; count < quantity; count++)
		{
			// Between 10 and mapwidth - 10
			x = (rand() % (mapWidth - 20)) + 10;
			y = (rand() % (mapHeight - 20)) + 10;

			if (!pickATileGen(&x, &y, LOOK_FOR_EMPTY_TILE, zonedPAT))
			{
				ASSERT(false, "addMultiPlayerRandomArtifacts: Unable to find a free location");
			}

			pF = buildFeature(asFeatureStats + i, world_coord(x), world_coord(y), false);

			NETuint32_t(&x);
			NETuint32_t(&y);
			NETuint32_t(&pF->id);
			NETuint8_t(&player);

			if (pF)
			{
				pF->player = player;
			}
		}

	NETend();
}

// ///////////////////////////////////////////////////////////////
BOOL addOilDrum(uint8_t count)
{
	addMultiPlayerRandomArtifacts(count, FEAT_OIL_DRUM);
	return true;
}

// ///////////////////////////////////////////////////////////////
// receive splattered artifacts
void recvMultiPlayerRandomArtifacts()
{
	int				count, i;
	uint8_t			quantity, player;
	uint32_t		tx,ty;
	uint32_t		ref;
	FEATURE_TYPE	type;
	FEATURE 		*pF;

	NETbeginDecode(NET_ARTIFACTS);
		NETuint8_t(&quantity);
		NETenum(&type);

	for (i = 0; i < numFeatureStats && asFeatureStats[i].subType != type; i++);

	for (count = 0; count < quantity; count++)
	{
		MAPTILE *psTile;

		NETuint32_t(&tx);
		NETuint32_t(&ty);
		NETuint32_t(&ref);
		NETuint8_t(&player);

		if (!tileOnMap(tx, ty))
		{
			debug(LOG_ERROR, "recvMultiPlayerRandomArtifacts: Bad tile coordinates (%u,%u)", tx, ty);
			continue;
		}
		psTile = mapTile(tx, ty);
		if (!psTile || psTile->psObject != NULL)
		{
			debug(LOG_ERROR, "recvMultiPlayerRandomArtifacts: Already something at (%u,%u)!", tx, ty);
			continue;
		}

		pF = buildFeature((asFeatureStats + i), world_coord(tx), world_coord(ty), false);
		if (pF)
		{
			pF->id		= ref;
			pF->player	= player;
		}
	}
	NETend();
}

// ///////////////////////////////////////////////////////////////
void giftArtifact(UDWORD owner, UDWORD x, UDWORD y)
{
	PLAYER_RESEARCH *pO,*pR;
	UDWORD	topic=0;
	pR   = asPlayerResList[selectedPlayer];

	if (owner < MAX_PLAYERS)
	{
		pO	 = asPlayerResList[owner];

		for (topic = numResearch; topic > 0; topic--)
		{
			if (IsResearchCompleted(&pO[topic])
			 && !IsResearchPossible(&pR[topic]))
			{
				MakeResearchPossible(&pR[topic]);
				CONPRINTF(ConsoleString,(ConsoleString,_("You Discover Blueprints For %s"),
					getName(asResearch[topic].pName)));
				
				break;
			}
		}
	}
}

// ///////////////////////////////////////////////////////////////
void processMultiPlayerArtifacts(void)
{
	static UDWORD lastCall;
	FEATURE	*pF,*pFN;
	UDWORD	x,y,pl;
	Vector3i position;
	BOOL	found=false;

	// only do this every now and again.
	if(lastCall > gameTime)lastCall= 0;
	if ( (gameTime - lastCall) <2000)
	{
		return;
	}
	lastCall = gameTime;

	addLoserGifts();

	for(pF = apsFeatureLists[0]; pF ; pF = pFN)
	{
		pFN = pF->psNext;
		// artifacts
		if(pF->psStats->subType == FEAT_GEN_ARTE)
		{
			found = objectInRange((BASE_OBJECT *)apsDroidLists[selectedPlayer], pF->pos.x, pF->pos.y, (TILE_UNITS+(TILE_UNITS/3))  );
			if(found)
			{
				position.x = pF->pos.x;				// Add an effect
				position.z = pF->pos.y;
				position.y = pF->pos.z;
				addEffect(&position,EFFECT_EXPLOSION,EXPLOSION_TYPE_DISCOVERY,false,NULL,false);

				x = pF->pos.x;
				y = pF->pos.y;
				pl= pF->player;
				removeFeature(pF);			// remove artifact+ send info.
				giftArtifact(pl,x,y);		// reward player.
				pF->player = 0;
				audio_QueueTrack( ID_SOUND_ARTIFACT_RECOVERED );
			}
		}
	}
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
			 && playerTeam[i] == playerTeam[j]	// Wto different players belonging to the same team
			 && !aiCheckAlliances(i, j)
			 && playerTeam[i] >= 0
			 && game.skDiff[i]
			 && game.skDiff[j])	// Not allied and not ignoring teams
			{
				// Create silently
				formAlliance(i, j, false, false, false);
			}
		}
	}
}
