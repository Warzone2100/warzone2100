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
 * Multiplay.c
 *
 * Alex Lee, Sep97, Pumpkin Studios
 *
 * Contains the day to day networking stuff, and received message handler.
 */
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/framework/strres.h"
#include "map.h"

#include "stats.h"									// for templates.
#include "game.h"									// for loading maps
#include "hci.h"

#include <time.h>									// for recording ping times.
#include "research.h"
#include "display3d.h"								// for changing the viewpoint
#include "console.h"								// for screen messages
#include "power.h"
#include "cmddroid.h"								//  for commanddroidupdatekills
#include "wrappers.h"								// for game over
#include "component.h"
#include "frontend.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "levels.h"
#include "selection.h"

#include "init.h"
#include "warcam.h"	// these 4 for fireworks
#include "effects.h"
#include "lib/gamelib/gtime.h"
#include "keybind.h"

#include "lib/script/script.h"				//Because of "ScriptTabs.h"
#include "scripttabs.h"			//because of CALL_AI_MSG
#include "scriptcb.h"			//for console callback
#include "scriptfuncs.h"

#include "lib/netplay/netplay.h"								// the netplay library.
#include "multiplay.h"								// warzone net stuff.
#include "multijoin.h"								// player management stuff.
#include "multirecv.h"								// incoming messages stuff
#include "multistat.h"
#include "multigifts.h"								// gifts and alliances.

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// globals.
BOOL						bMultiPlayer				= FALSE;	// true when more than 1 player.
char						sForceName[256]				= "Default";
SDWORD						player2dpid[MAX_PLAYERS]	={0,0,0,0,0,0,0,0};		//stores dpids of each player. FILTHY HACK (ASSUMES 8 players)
//UDWORD						arenaPlayersReceived=0;
BOOL						openchannels[MAX_PLAYERS]={TRUE};
UBYTE						bDisplayMultiJoiningStatus;

MULTIPLAYERGAME				game;									//info to describe game.
MULTIPLAYERINGAME			ingame;

BOOL						bSendingMap					= FALSE;	// map broadcasting.

char						tempString[12];
char						beaconReceiveMsg[MAX_PLAYERS][MAX_CONSOLE_STRING_LENGTH];	//beacon msg for each player
char								playerName[MAX_PLAYERS][MAX_NAME_SIZE];	//Array to store all player names (humans and AIs)

/////////////////////////////////////
/* multiplayer message stack stuff */
/////////////////////////////////////
#define MAX_MSG_STACK	50
#define MAX_STR			255

static char msgStr[MAX_MSG_STACK][MAX_STR];
static SDWORD msgPlFrom[MAX_MSG_STACK];
static SDWORD msgPlTo[MAX_MSG_STACK];
static SDWORD callbackType[MAX_MSG_STACK];
static SDWORD locx[MAX_MSG_STACK];
static SDWORD locy[MAX_MSG_STACK];
static DROID *msgDroid[MAX_MSG_STACK];
static SDWORD msgStackPos = -1;				//top element pointer

// ////////////////////////////////////////////////////////////////////////////
// Remote Prototypes
extern RESEARCH*			asResearch;							//list of possible research items.
extern PLAYER_RESEARCH*		asPlayerResList[MAX_PLAYERS];

// ////////////////////////////////////////////////////////////////////////////
// Local Prototypes

static BOOL recvBeacon();
static BOOL recvDestroyTemplate(void);
static BOOL recvResearch(void);

// ////////////////////////////////////////////////////////////////////////////
// temporarily disable multiplayer mode.
BOOL turnOffMultiMsg(BOOL bDoit)
{
	static BOOL bTemp;

	if(bDoit)	// turn off msgs.
	{
		if(bTemp == TRUE)
		{
			debug(LOG_NET, "turnOffMultiMsg: multiple calls to turn off");
		}
		if(bMultiPlayer)
		{
			bMultiPlayer = FALSE;
			bTemp = TRUE;
		}
	}
	else	// turn on msgs.
	{
		if(bTemp)
		{
			bMultiPlayer = TRUE;
			bTemp = FALSE;
		}
	}
	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// throw a pary when you win!
BOOL multiplayerWinSequence(BOOL firstCall)
{
	static Vector3i pos;
	Vector3i pos2;
	static UDWORD last=0;
	float		rotAmount;
	STRUCTURE	*psStruct;

	if(firstCall)
	{
		pos  = cameraToHome(selectedPlayer,TRUE);			// pan the camera to home if not already doing so
		last =0;

		// stop all research
		CancelAllResearch(selectedPlayer);

		// stop all manufacture.
		for(psStruct=apsStructLists[selectedPlayer];psStruct;psStruct = psStruct->psNext)
		{
			if (StructIsFactory(psStruct))
			{
				if (((FACTORY *)psStruct->pFunctionality)->psSubject)//check if active
				{
					cancelProduction(psStruct);
				}
			}
		}
	}

	// rotate world
	if(!getWarCamStatus())
	{
		rotAmount = timeAdjustedIncrement(MAP_SPIN_RATE / 12, TRUE);
		player.r.y += rotAmount;
	}

	if(last > gameTime)last= 0;
	if((gameTime-last) < 500 )							// only  if not done recently.
	{
		return TRUE;
	}
	last = gameTime;

	if(rand()%3 == 0)
	{
		pos2=pos;
		pos2.x +=  (rand() % world_coord(8)) - world_coord(4);
		pos2.z +=  (rand() % world_coord(8)) - world_coord(4);

		if (pos2.x < 0)
			pos2.x = 128;

		if ((unsigned)pos2.x > world_coord(mapWidth))
			pos2.x = world_coord(mapWidth);

		if (pos2.z < 0)
			pos2.z = 128;

		if ((unsigned)pos2.z > world_coord(mapHeight))
			pos2.z = world_coord(mapHeight);

		addEffect(&pos2,EFFECT_FIREWORK,FIREWORK_TYPE_LAUNCHER,FALSE,NULL,0);	// throw up some fire works.
	}

	// show the score..


	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// MultiPlayer main game loop code.
BOOL multiPlayerLoop(void)
{
	UDWORD		i;
	UBYTE		joinCount;

	sendCheck();						// send some checking info if possible
	processMultiPlayerArtifacts();		// process artifacts

		joinCount =0;
		for(i=0;i<MAX_PLAYERS;i++)
		{
			if(isHumanPlayer(i) && ingame.JoiningInProgress[i] )
			{
				joinCount++;
			}
		}
		if(joinCount)
		{
			setWidgetsStatus(FALSE);
			bDisplayMultiJoiningStatus = joinCount;	// someone is still joining! say So

			// deselect anything selected.
			selDroidDeselect(selectedPlayer);

			if(keyPressed(KEY_ESC) )// check for cancel
			{
				bDisplayMultiJoiningStatus = 0;
				setWidgetsStatus(TRUE);
				setPlayerHasLost(TRUE);
			}
		}
		else		//everyone is in the game now!
		{
			if(bDisplayMultiJoiningStatus)
			{
				sendVersionCheck();
				bDisplayMultiJoiningStatus = 0;
				setWidgetsStatus(TRUE);
			}
		}

	recvMessage();						// get queued messages


	// if player has won then process the win effects...
	if(testPlayerHasWon())
	{
		multiplayerWinSequence(FALSE);
	}
	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// quikie functions.

// to get droids ...
BOOL IdToDroid(UDWORD id, UDWORD player, DROID **psDroid)
{
	UDWORD i;
	DROID *d;

	if(player == ANYPLAYER)
	{
		for(i=0;i<MAX_PLAYERS;i++)			// find the droid to order form them all
		{
			d = apsDroidLists[i];
			while((d != NULL )&&(d->id !=id) )d=d->psNext;
			if(d)
			{
				*psDroid = d;
				return TRUE;
			}
		}
		return FALSE;
	}
	else									// find the droid, given player
	{
		d = apsDroidLists[player];
		while( (d != NULL ) && (d->id !=id))d=d->psNext;
		if(d)
		{
			*psDroid = d;
			return TRUE;
		}
		return FALSE;
	}
}

// ////////////////////////////////////////////////////////////////////////////
// find a structure
STRUCTURE *IdToStruct(UDWORD id,UDWORD player)
{
	STRUCTURE	*psStr = NULL;
	UDWORD		i;

	if(player == ANYPLAYER)
	{
		for(i=0;i<MAX_PLAYERS;i++)
		{
			for(psStr=apsStructLists[i];( (psStr != NULL) && (psStr->id != id)); psStr=psStr->psNext);
			if(psStr)
			{
				return psStr;
			}
		}
	}
	else
	{
		for(psStr=apsStructLists[player];((psStr != NULL )&&(psStr->id != id) );psStr=psStr->psNext);
	}
	return psStr;
}

// ////////////////////////////////////////////////////////////////////////////
// find a feature
FEATURE *IdToFeature(UDWORD id,UDWORD player)
{
	FEATURE	*psF =NULL;
	UDWORD	i;

	if(player == ANYPLAYER)
	{
		for(i=0;i<MAX_PLAYERS;i++)
		{
			for(psF=apsFeatureLists[i];( (psF != NULL) && (psF->id != id)); psF=psF->psNext);
			if(psF)
			{
				return psF;
			}
		}
	}
	else
	{
		for(psF=apsFeatureLists[player];((psF != NULL )&&(psF->id != id) );psF=psF->psNext);
	}
	return psF;
}

// ////////////////////////////////////////////////////////////////////////////

DROID_TEMPLATE *IdToTemplate(UDWORD tempId,UDWORD player)
{
	DROID_TEMPLATE *psTempl = NULL;
	UDWORD		i;
	if(player != ANYPLAYER)
	{
		for (psTempl = apsDroidTemplates[player];			// follow templates
		(psTempl && (psTempl->multiPlayerID != tempId ));
		 psTempl = psTempl->psNext);
	}
	else
	{
		// REALLY DANGEROUS!!! ID's are NOT assumed to be unique for TEMPLATES.
		debug( LOG_NEVER, "Really Dodgy Check performed for a template" );
		for(i=0;i<MAX_PLAYERS;i++)
		{
			for (psTempl = apsDroidTemplates[i];			// follow templates
			(psTempl && (psTempl->multiPlayerID != tempId ));
			 psTempl = psTempl->psNext);
			if(psTempl)
			{
				return psTempl;
			}
		}
	}
	return psTempl;
}

// the same as above, but only checks names in similarity.
DROID_TEMPLATE *NameToTemplate(const char *sName,UDWORD player)
{
	DROID_TEMPLATE *psTempl = NULL;

	for (psTempl = apsDroidTemplates[player];			// follow templates
		(psTempl && (strcmp(psTempl->aName,sName) != 0) );
		 psTempl = psTempl->psNext);

	 return psTempl;
}

/////////////////////////////////////////////////////////////////////////////////
//  Returns a pointer to base object, given an id and optionally a player.
BASE_OBJECT *IdToPointer(UDWORD id,UDWORD player)
{
	DROID		*pD;
	STRUCTURE	*pS;
	FEATURE		*pF;
	// droids.
	if (IdToDroid(id,player,&pD))
	{
		return (BASE_OBJECT*)pD;
	}

	// structures
	pS = IdToStruct(id,player);
	if(pS)
	{
		return (BASE_OBJECT*)pS;
	}

	// features
	pF = IdToFeature(id,player);
	if(pF)
	{
		return (BASE_OBJECT*)pF;
	}

	return NULL;
}


// ////////////////////////////////////////////////////////////////////////////
// return a players name.
char *getPlayerName(UDWORD player)
{
	UDWORD i;

	ASSERT( player < MAX_PLAYERS , "getPlayerName: wrong player index: %d", player);

	//Try NetPlay.playerName first (since supports AIs)
	if(game.type != CAMPAIGN)
		if(strcmp(playerName[player], "") != 0)
			return (char*)&playerName[player];

	//Use the ordinary way if failed
	for(i=0;i<MAX_PLAYERS;i++)
	{
		if(player2dpid[player] == (unsigned int)NetPlay.players[i].dpid)
		{
			if(strcmp(NetPlay.players[i].name,"") == 0)
			{
				// make up a name for this player.
				return getPlayerColourName(player);
			}

			return (char*)&NetPlay.players[i].name;
		}
	}

	return NetPlay.players[0].name;
}

BOOL setPlayerName(UDWORD player, const char *sName)
{
	if(player > MAX_PLAYERS)
	{
		ASSERT(FALSE, "setPlayerName: wrong player index (%d)", player);
		return FALSE;
	}

	strcpy(playerName[player],sName);

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// to determine human/computer players and responsibilities of each..
BOOL isHumanPlayer(UDWORD player)
{
	if (player >= MAX_PLAYERS)
		return FALSE;

	return (BOOL) (player2dpid[player] != 0);
}

// returns player responsible for 'player'
UDWORD  whosResponsible(UDWORD player)
{
	UDWORD c;
	SDWORD i;

    c = ANYPLAYER;
	if (isHumanPlayer(player))
	{
		c = player;
	}

	else if(player == selectedPlayer)
	{
		c = player;
	}

	else
	{
		// crawl down array to find a responsible fellow,
		for(i=player; i>=0; i--)
		{
			if(isHumanPlayer(i))
			{
				c = i;
			}
		}
		// else crawl up to find a responsible fellow
		if(c == ANYPLAYER)
		{
			for(i=player; i<MAX_PLAYERS; i++)
			{
				if(isHumanPlayer(i))
				{
					c = i;
				}
			}
		}

	}
	if(c == ANYPLAYER)
	{
		debug( LOG_NEVER, "failed to find a player for %d \n", player );
	}
	return c;
}

//returns true if selected player is responsible for 'player'
BOOL myResponsibility(UDWORD player)
{
	if(whosResponsible(player) == selectedPlayer)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

//returns true if 'player' is responsible for 'playerinquestion'
BOOL responsibleFor(UDWORD player, UDWORD playerinquestion)
{

	if(whosResponsible(playerinquestion) == player)
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}


// ////////////////////////////////////////////////////////////////////////////
// probably temporary. Places the camera on the players 1st droid or struct.
Vector3i cameraToHome(UDWORD player,BOOL scroll)
{
	Vector3i res;
	UDWORD x,y;
	STRUCTURE	*psBuilding;

	for (psBuilding = apsStructLists[player];
		 psBuilding && (psBuilding->pStructureType->type != REF_HQ);
		 psBuilding= psBuilding->psNext);

	if(psBuilding)
	{
		x= map_coord(psBuilding->pos.x);
		y= map_coord(psBuilding->pos.y);
	}
	else if (apsDroidLists[player])				// or first droid
	{
		 x= map_coord(apsDroidLists[player]->pos.x);
		 y=	map_coord(apsDroidLists[player]->pos.y);
	}
	else if (apsStructLists[player])							// center on first struct
	{
		x= map_coord(apsStructLists[player]->pos.x);
		y= map_coord(apsStructLists[player]->pos.y);
	}
	else														//or map center.
	{
		x= mapWidth/2;
		y= mapHeight/2;
	}


	if(scroll)
	{
		requestRadarTrack(world_coord(x), world_coord(y));
	}
	else
	{
		setViewPos(x,y,TRUE);
	}

	res.x = world_coord(x);
	res.y = map_TileHeight(x,y);
	res.z = world_coord(y);
	return res;
}


// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Recv Messages. Get a message and dispatch to relevant function.
BOOL recvMessage(void)
{
	NETMSG msg;

	while(NETrecv(&msg) == TRUE)			// for all incoming messages.
	{
		// Cocpy the message to the global one used by the new NET API
		NetMsg = msg;

		// messages only in game.
		if(!ingame.localJoiningInProgress)
		{
			switch(msg.type)
			{
			case NET_DROID:						// new droid of known type
				recvDroid();
				break;
			case NET_DROIDINFO:					//droid update info
				recvDroidInfo();
				break;
			case NET_DROIDDEST:					// droid destroy
				recvDestroyDroid();
				break;
			case NET_DROIDMOVE:					// move a droid to x,y command.
				recvDroidMove();
				break;
			case NET_GROUPORDER:				// an order for more than 1 droid.
				recvGroupOrder();
				break;
			case NET_CHECK_DROID:				// droid damage and position checks
				recvDroidCheck();
				break;
			case NET_CHECK_STRUCT:				// structure damage checks.
				recvStructureCheck();
				break;
			case NET_CHECK_POWER:				// Power level syncing.
				recvPowerCheck();
				break;
			case NET_TEXTMSG:					// simple text message
				recvTextMessage();
				break;
			case NET_AITEXTMSG:					//multiplayer AI text message
				recvTextMessageAI();
				break;
			case NET_BEACONMSG:					//beacon (blip) message
				recvBeacon();
				break;
			case NET_BUILD:						// a build order has been sent.
				recvBuildStarted();
				break;
			case NET_BUILDFINISHED:				// a building is complete
				recvBuildFinished();
				break;
			case NET_STRUCTDEST:				// structure destroy
				recvDestroyStructure();
				break;
			case NET_SECONDARY:					// set a droids secondary order level.
				recvDroidSecondary();
				break;
			case NET_SECONDARY_ALL:					// set a droids secondary order level.
				recvDroidSecondaryAll();
				break;
			case NET_DROIDEMBARK:
				recvDroidEmbark();              //droid has embarked on a Transporter
				break;
			case NET_DROIDDISEMBARK:
				recvDroidDisEmbark();           //droid has disembarked from a Transporter
				break;
			case NET_REQUESTDROID:				// player requires a droid that they dont have.
				recvRequestDroid();
				break;
//			case NET_REQUESTPLAYER:				// a new player requires information
//				multiPlayerRequest(&msg);
//				break;
			case NET_GIFT:						// an alliance gift from one player to another.
				recvGift(&msg);
				break;
			case NET_SCORESUBMIT:				//  a score update from another player
				recvScoreSubmission();
				break;
			case NET_VTOL:
				recvHappyVtol();
				break;
			case NET_LASSAT:
				recvLasSat();
				break;
			default:
				break;
			}
		}

		// messages usable all the time
		switch(msg.type)
		{
		case NET_TEMPLATE:					// new template
			recvTemplate(&msg);
			break;
		case NET_TEMPLATEDEST:				// template destroy
			recvDestroyTemplate();
			break;
		case NET_FEATUREDEST:				// feature destroy
			recvDestroyFeature();
			break;
		case NET_PING:						// diagnostic ping msg.
			recvPing();
			break;
		case NET_DEMOLISH:					// structure demolished.
			recvDemolishFinished();
			break;
		case NET_RESEARCH:					// some research has been done.
			recvResearch();
			break;
		case NET_LEAVING:					// player leaving nicely
		{
			uint32_t player_id;
			BOOL host;

			NETbeginDecode();
				NETuint32_t(&player_id);
				NETbool(&host);                 // Added to check for host quit here -- Buggy
			NETend();
			MultiPlayerLeave(player_id);
			if (host)                               // host has quit, need to quit too.
			{
				//stopJoining();		//NOT defined here, checking if we need it or not.
				debug(LOG_NET, "***Need to call stopJoining()");
				addConsoleMessage(_("The host has left the game!"), LEFT_JUSTIFY);
			}
			break;
		}
		case NET_WHOLEDROID:				// a complete droid description has arrived.
			receiveWholeDroid(&msg);
			break;
		case NET_OPTIONS:
			recvOptions();
			break;
		case NET_VERSION:
			recvVersionCheck(&msg);
			break;
		case NET_PLAYERRESPONDING:			// remote player is now playing
		{
			uint32_t player_id;

			NETbeginDecode();
				// the player that has just responded
				NETuint32_t(&player_id);
			NETend();

			// This player is now with us!
			ingame.JoiningInProgress[player_id] = FALSE;
			break;
		}
		case NET_COLOURREQUEST:
			recvColourRequest(&msg);
			break;
		case NET_TEAMREQUEST:
			recvTeamRequest(&msg);
			break;
		case NET_ARTIFACTS:
			recvMultiPlayerRandomArtifacts();
			break;
		case NET_FEATURES:
			recvMultiPlayerFeature();
			break;
		case NET_ALLIANCE:
			recvAlliance(TRUE);
			break;
		case NET_KICK:
		{
			uint32_t player_id;

			NETbeginDecode();
				NETuint32_t(&player_id);
			NETend();

			if (NetPlay.dpidPlayer == player_id)  // we've been told to leave.
			{
				setPlayerHasLost(TRUE);
			}
			break;
		}
		case NET_FIREUP:				// frontend only
			break;
		case NET_RESEARCHSTATUS:
			recvResearchStatus();
			break;
		default:
			break;
		}
	}

	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// Research Stuff. Nat games only send the result of research procedures.
BOOL SendResearch(uint8_t player, uint32_t index)
{
	UBYTE i;
	PLAYER_RESEARCH *pPlayerRes;

	// Send the player that is researching the topic and the topic itself
	NETbeginEncode(NET_RESEARCH, NET_ALL_PLAYERS);
		NETuint8_t(&player);
		NETuint32_t(&index);
	NETend();

	/*
	 * Since we are called when the state of research changes (completed,
	 * stopped &c) we also need to update our onw local copy of what our allies
	 * are doing/have done.
	 */
	if (game.type == SKIRMISH)
	{		
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (alliances[i][player] == ALLIANCE_FORMED)
			{
				pPlayerRes = asPlayerResList[i] + index;
				
				// If we have it but they don't
				if (!IsResearchCompleted(pPlayerRes))
				{
					// Do the research for that player
					MakeResearchCompleted(pPlayerRes);
					researchResult(index, i, FALSE, NULL);
				}
			}
		}
	}

	return TRUE;
}

// recv a research topic that is now complete.
static BOOL recvResearch()
{
	uint8_t			player;
	uint32_t		index;
	int				i;
	PLAYER_RESEARCH	*pPlayerRes;
	RESEARCH		*pResearch;

	NETbeginDecode();
		NETuint8_t(&player);
		NETuint32_t(&index);
	NETend();

	pPlayerRes = asPlayerResList[player] + index;
	
	// If they have completed the research
	if (IsResearchCompleted(pPlayerRes))
	{
		MakeResearchCompleted(pPlayerRes);
		researchResult(index, player, FALSE, NULL);

		// Take off the power if available
		pResearch = asResearch + index;
		usePower(player, pResearch->researchPower);
	}

	// Update allies research accordingly
	if (game.type == SKIRMISH)
	{
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (alliances[i][player] == ALLIANCE_FORMED)
			{
				pPlayerRes = asPlayerResList[i] + index;
				
				if (!IsResearchCompleted(pPlayerRes))
				{
					// Do the research for that player
					MakeResearchCompleted(pPlayerRes);
					researchResult(index, i, FALSE, NULL);
				}
			}
		}
	}

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// New research stuff, so you can see what others are up to!
// inform others that I'm researching this.
BOOL sendReseachStatus(STRUCTURE *psBuilding, uint32_t index, uint8_t player, BOOL bStart)
{	
	if (!myResponsibility(player) || gameTime < 5)
	{
		return TRUE;
	}
	
	NETbeginEncode(NET_RESEARCHSTATUS, NET_ALL_PLAYERS);
		NETuint8_t(&player);
		NETbool(&bStart);
		
		// If we know the building researching it then send its ID
		if (psBuilding)
		{
			NETuint32_t(&psBuilding->id);
		}
		else
		{
			NETnull();
		}
		
		// Finally the topic in question
		NETuint32_t(&index);
	NETend();

	return TRUE;
}

BOOL recvResearchStatus()
{
	STRUCTURE			*psBuilding;
	PLAYER_RESEARCH		*pPlayerRes;
	RESEARCH_FACILITY	*psResFacilty;
	RESEARCH			*pResearch;
	uint8_t				player;
	BOOL				bStart;
	uint32_t			index, structRef;

	NETbeginDecode();
		NETuint8_t(&player);
		NETbool(&bStart);
		NETuint32_t(&structRef);
		NETuint32_t(&index);
	NETend();

	pPlayerRes = asPlayerResList[player] + index;

	// psBuilding may be null if finishing
	if (bStart)							// Starting research
	{
		psBuilding = IdToStruct(structRef, player);
		
		// Set that facility to research
		if (psBuilding)
		{
			psResFacilty = (RESEARCH_FACILITY *) psBuilding->pFunctionality;
			
			if (psResFacilty->psSubject)
			{
				cancelResearch(psBuilding);
			}
			
			// Set the subject up
			pResearch				= asResearch + index;
			psResFacilty->psSubject = (BASE_STATS *) pResearch;

			// If they have previously started but cancelled there is no need to accure power
			if (IsResearchCancelled(pPlayerRes))
			{
				psResFacilty->powerAccrued	= pResearch->researchPower;
			}
			else
			{
				psResFacilty->powerAccrued	= 0;
			}

			// Start the research
			MakeResearchStarted(pPlayerRes);
			psResFacilty->timeStarted		= ACTION_START_TIME;
			psResFacilty->timeStartHold		= 0;
			psResFacilty->timeToResearch	= pResearch->researchPoints / psResFacilty->researchPoints;
			
			// A failsafe of some sort
			if (psResFacilty->timeToResearch == 0)
			{
				psResFacilty->timeToResearch = 1;
			}
		}

	}
	// Finished/cancelled research
	else
	{
		// If they completed the research, we're done
		if (IsResearchCompleted(pPlayerRes))
		{
			return TRUE;
		}
		
		// If they did not say what facility it was, look it up orselves
		if (!structRef)
		{
			// Go through the structs to find the one doing this topic
			for (psBuilding = apsStructLists[player]; psBuilding; psBuilding = psBuilding->psNext)
			{
				if (psBuilding->pStructureType->type == REF_RESEARCH
				 && psBuilding->status == SS_BUILT
				 && ((RESEARCH_FACILITY *) psBuilding->pFunctionality)->psSubject
				 && ((RESEARCH_FACILITY *) psBuilding->pFunctionality)->psSubject->ref - REF_RESEARCH_START == index)
				{
					break;
				}
			}
		}
		else
		{
			psBuilding = IdToStruct(structRef, player);
		}
		
		// Stop the facility doing any research
		if (psBuilding)
		{
			cancelResearch(psBuilding);
		}
	}
	
	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Text Messaging between players. proceed string with players to send to.
// eg "123 hi there" sends "hi there" to players 1,2 and 3.
BOOL sendTextMessage(const char *pStr, BOOL all)
{
	BOOL	normal = TRUE;
	BOOL	sendto[MAX_PLAYERS];
	UDWORD	i;
	char	display[MAX_CONSOLE_STRING_LENGTH];
	char	msg[MAX_CONSOLE_STRING_LENGTH];
	uint8_t netplayer = 0;

	if (!ingame.localOptionsReceived)
	{
		return TRUE;
	}

	memset(display,0x0, sizeof(display));	//clear buffer
	memset(msg,0x0, sizeof(display));		//clear buffer
	memset(sendto,0x0, sizeof(sendto));		//clear private flag
	strlcpy(msg, pStr, sizeof(msg));
	for(i = 0; ((pStr[i] >= '0' ) && (pStr[i] <= '8' )  && (i < MAX_PLAYERS )); i++)		// for each numeric char encountered..
	{
		sendto[ pStr[i]-'0' ] = TRUE;
		normal = FALSE;
	}

	if (!all && !normal)	// lets user know it is a private message
	{
		strlcpy(display,"(private)",sizeof(display));
		strlcat(display,pStr,sizeof(display));
	}

	if (all)	//broadcast
	{
		NETbeginEncode(NET_TEXTMSG, NET_ALL_PLAYERS);
			NETuint32_t(&NetPlay.dpidPlayer);		// who this msg is from
			NETstring(msg,MAX_CONSOLE_STRING_LENGTH);	// the message to send
		NETend();
	}
	else if (normal)
	{
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (i != selectedPlayer && openchannels[i])
			{
				if (isHumanPlayer(i))
				{
					netplayer = player2dpid[i];
					NETbeginEncode(NET_TEXTMSG,netplayer);
						NETuint32_t(&NetPlay.dpidPlayer);		// who this msg is from
						NETstring(msg,MAX_CONSOLE_STRING_LENGTH);	// the message to send
					NETend();
				}
				else	//also send to AIs now (non-humans), needed for AI
				{
					sendAIMessage(msg, selectedPlayer, i);
				}
			}
		}
	}
	else	//private msg
	{
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (sendto[i])
			{
				if (isHumanPlayer(i))
				{
					netplayer = player2dpid[i];
						NETbeginEncode(NET_TEXTMSG, netplayer);
						NETuint32_t(&NetPlay.dpidPlayer);				// who this msg is from
						NETstring(display, MAX_CONSOLE_STRING_LENGTH);	// the message to send
					NETend();
				}
				else	//also send to AIs now (non-humans), needed for AI
				{
					sendAIMessage(display, selectedPlayer, i);
				}
			}
		}
		sprintf(display,"(private)");		 
	}

	//This is for local display
	for(i = 0; NetPlay.players[i].dpid != NetPlay.dpidPlayer; i++);	//findplayer
	strlcat(display, NetPlay.players[i].name, sizeof(display));		// name
	strlcat(display, ": ", sizeof(display));						// seperator
	strlcat(display, pStr, sizeof(display));						// add message
	addConsoleMessage(display, DEFAULT_JUSTIFY);					// display

	return TRUE;
}

//AI multiplayer message, send from a certain player index to another player index
BOOL sendAIMessage(char *pStr, UDWORD player, UDWORD to)
{
	UDWORD	sendPlayer;		//dpidPlayer is a uint32_t, not int32_t!
	uint8_t netplayer=0;

	//check if this is one of the local players, don't need net send then
	if (to == selectedPlayer || myResponsibility(to))	//(the only) human on this machine or AI on this machine
	{
		//Just show "him" the message
		displayAIMessage(pStr, player, to);

		//Received a console message from a player callback
		//store and call later
		//-------------------------------------------------
		if (!msgStackPush(CALL_AI_MSG,player,to,pStr,-1,-1,NULL))
		{
			debug(LOG_ERROR, "sendAIMessage() - msgStackPush - stack failed");
			return FALSE;
		}
	}
	else		//not a local player (use multiplayer mode)
	{
		if (!ingame.localOptionsReceived)
		{
			return TRUE;
		}

		//find machine that is hosting this human or AI
		sendPlayer = whosResponsible(to);

		if (sendPlayer >= MAX_PLAYERS)
		{
			debug(LOG_ERROR, "sendAIMessage() - sendPlayer >= MAX_PLAYERS");
			return FALSE;
		}

		if (!isHumanPlayer(sendPlayer))		//NETsend can't send to non-humans
		{
			debug(LOG_ERROR, "sendAIMessage() - player is not human.");
			return FALSE;
		}
		netplayer = player2dpid[sendPlayer];
		
		//send to the player who is hosting 'to' player (might be himself if human and not AI)
		NETbeginEncode(NET_AITEXTMSG, netplayer);
			NETuint32_t(&player);			//save the actual sender
			//save the actual player that is to get this msg on the source machine (source can host many AIs)
			NETuint32_t(&to);				//save the actual receiver (might not be the same as the one we are actually sending to, in case of AIs)
			NETstring(pStr, MAX_CONSOLE_STRING_LENGTH);		// copy message in.
		NETend();
	}

	return TRUE;
}

//
// At this time, we do NOT support messages for beacons
//
BOOL sendBeacon(int32_t locX, int32_t locY, int32_t forPlayer, int32_t sender, const char* pStr)
{
	int sendPlayer;
	//debug(LOG_WZ, "sendBeacon: '%s'",pStr);

	//find machine that is hosting this human or AI
	sendPlayer = whosResponsible(forPlayer);

	if(sendPlayer >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "sendAIMessage() - whosResponsible() failed.");
		return FALSE;
	}

	// I assume this is correct, looks like it sends it to ONLY that person, and the routine
	// kf_AddHelpBlip() itterates for each player it needs.
	NETbeginEncode(NET_BEACONMSG, player2dpid[sendPlayer]);     // send to the player who is hosting 'to' player (might be himself if human and not AI)
		NETint32_t(&sender);                                // save the actual sender

		// save the actual player that is to get this msg on the source machine (source can host many AIs)
		NETint32_t(&forPlayer);                             // save the actual receiver (might not be the same as the one we are actually sending to, in case of AIs)
		NETint32_t(&locX);                                  // save location
		NETint32_t(&locY);

		// const_cast: need to cast away constness because of the const-incorrectness of NETstring (const-incorrect when sending/encoding a packet)
		NETstring((char*)pStr, MAX_CONSOLE_STRING_LENGTH);  // copy message in.
	NETend();

	return TRUE;
}

void displayAIMessage(char *pStr, SDWORD from, SDWORD to)
{
	char tmp[255];

	if (isHumanPlayer(to))		//display text only if receiver is the (human) host machine itself
	{
		strcpy(tmp, getPlayerName(from));
		strcat(tmp, ": ");											// seperator
		strcat(tmp, pStr);											// add message
		addConsoleMessage(tmp, DEFAULT_JUSTIFY);
	}
}

// Write a message to the console.
BOOL recvTextMessage()
{
	UDWORD	dpid;		
	UDWORD	i;
	char	msg[MAX_CONSOLE_STRING_LENGTH];
	char newmsg[MAX_CONSOLE_STRING_LENGTH];
	UDWORD  player=MAX_PLAYERS,j;		//console callback - player who sent the message

	memset(msg, 0x0, sizeof(msg));
	memset(newmsg, 0x0, sizeof(newmsg));

	NETbeginDecode();
		// Who this msg is from
		NETuint32_t(&dpid);
		// The message to send
		NETstring(newmsg, MAX_CONSOLE_STRING_LENGTH);
	NETend();


	for (i = 0; NetPlay.players[i].dpid != dpid; i++);		//findplayer

	//console callback - find real number of the player
	for (j = 0; i < MAX_PLAYERS; j++)
	{
		if (dpid == player2dpid[j])
		{
			player = j;
			break;
		}
	}

	ASSERT(player != MAX_PLAYERS, "recvTextMessage: failed to find owner of dpid %d", dpid);

	strlcpy(msg, NetPlay.players[i].name, sizeof(msg));
	// Seperator
	strlcat(msg, ": ", sizeof(msg));
	// Add message
	strlcat(msg, newmsg, sizeof(msg));
	
	addConsoleMessage(msg, DEFAULT_JUSTIFY);

	// Multiplayer message callback
	// Received a console message from a player, save
	MultiMsgPlayerFrom = player;
	MultiMsgPlayerTo = selectedPlayer;

	strlcpy(MultiplayMsg, newmsg, sizeof(MultiplayMsg));
	eventFireCallbackTrigger((TRIGGER_TYPE)CALL_AI_MSG);

	// make some noise!
	if (titleMode == MULTIOPTION || titleMode == MULTILIMIT)
	{
		audio_PlayTrack(FE_AUDIO_MESSAGEEND);
	}
	else if (!ingame.localJoiningInProgress)
	{
		audio_PlayTrack(ID_SOUND_MESSAGEEND);
	}

	return TRUE;
}

//AI multiplayer message - received message from AI (from scripts)
BOOL recvTextMessageAI()
{
	UDWORD	sender, receiver;
	char	msg[MAX_CONSOLE_STRING_LENGTH];
	char	newmsg[MAX_CONSOLE_STRING_LENGTH];

	NETbeginDecode();
		NETuint32_t(&sender);			//in-game player index ('normal' one)
		NETuint32_t(&receiver);			//in-game player index
		NETstring(newmsg,MAX_CONSOLE_STRING_LENGTH);		
	NETend();

	//strlcpy(msg, getPlayerName(sender), sizeof(msg));  // name
	//strlcat(msg, " : ", sizeof(msg));                  // seperator
	strlcpy(msg, newmsg, sizeof(msg));

	//Display the message and make the script callback
	displayAIMessage(msg, sender, receiver);

	//Received a console message from a player callback
	//store and call later
	//-------------------------------------------------
	if(!msgStackPush(CALL_AI_MSG,sender,receiver,msg,-1,-1,NULL))
	{
		debug(LOG_ERROR, "recvTextMessageAI() - msgStackPush - stack failed");
		return FALSE;
	}

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// Templates

// send a newly created template to other players
BOOL sendTemplate(DROID_TEMPLATE *pTempl)
{
	NETMSG m;
	UDWORD count = 0, i;

	ASSERT(pTempl != NULL, "sendTemplate: Old Pumpkin bug");
	if (!pTempl) return TRUE; /* hack */

	// I hate adding more of this hideous code, but it is necessary for now - Per
	NetAddUint8(m, count, selectedPlayer);			count += sizeof(Uint8);
	NetAddUint32(m, count, pTempl->ref);			count += sizeof(Uint32);
	NetAdd(m, count, pTempl->aName);			count += DROID_MAXNAME;
	NetAddUint8(m, count, pTempl->NameVersion);		count += sizeof(Uint8);
	for (i = 0; i < DROID_MAXCOMP; i++)
	{
		// signed, but sent as a bunch of bits...
		NetAddUint32(m, count, pTempl->asParts[i]);	count += sizeof(Uint32);
	}
	NetAddUint32(m, count, pTempl->buildPoints);		count += sizeof(Uint32);
	NetAddUint32(m, count, pTempl->powerPoints);		count += sizeof(Uint32);
	NetAddUint32(m, count, pTempl->storeCount);		count += sizeof(Uint32);
	NetAddUint32(m, count, pTempl->numWeaps);		count += sizeof(Uint32);
	for (i = 0; i < DROID_MAXWEAPS; i++)
	{
		NetAddUint32(m, count, pTempl->asWeaps[i]);	count += sizeof(Uint32);
	}
	NetAddUint32(m, count, pTempl->droidType);		count += sizeof(Uint32);
	NetAddUint32(m, count, pTempl->multiPlayerID);		count += sizeof(Uint32);

	m.type = NET_TEMPLATE;
	m.size = count;
	return(  NETbcast(&m,FALSE)	);
}

// receive a template created by another player
BOOL recvTemplate(NETMSG * m)
{
	UBYTE			player;
	DROID_TEMPLATE	*psTempl;
	DROID_TEMPLATE	t, *pT = &t;
	unsigned int i;
	unsigned int count = 0;

	NetGetUint8(m, count, player);				count += sizeof(Uint8);
	ASSERT( player < MAX_PLAYERS, "recvtemplate: invalid player size: %d", player );

	NetGetUint32(m, count, pT->ref);			count += sizeof(Uint32);
	NetGet(m, count, pT->aName);				count += DROID_MAXNAME;
	NetGetUint8(m, count, pT->NameVersion);			count += sizeof(Uint8);
	for (i = 0; i < DROID_MAXCOMP; i++)
	{
		// signed, but sent as a bunch of bits...
		NetGetUint32(m, count, pT->asParts[i]);		count += sizeof(Uint32);
	}
	NetGetUint32(m, count, pT->buildPoints);		count += sizeof(Uint32);
	NetGetUint32(m, count, pT->powerPoints);		count += sizeof(Uint32);
	NetGetUint32(m, count, pT->storeCount);			count += sizeof(Uint32);
	NetGetUint32(m, count, pT->numWeaps);			count += sizeof(Uint32);
	for (i = 0; i < DROID_MAXWEAPS; i++)
	{
		NetGetUint32(m, count, pT->asWeaps[i]);		count += sizeof(Uint32);
	}
	NetGetUint32(m, count, pT->droidType);			count += sizeof(Uint32);
	NetGetUint32(m, count, pT->multiPlayerID);		count += sizeof(Uint32);

	t.psNext = NULL;

	psTempl = IdToTemplate(t.multiPlayerID,player);
	if(psTempl)												// already exists.
	{
		t.psNext = psTempl->psNext;
		memcpy(psTempl, &t, sizeof(DROID_TEMPLATE));
	}
	else
	{
		addTemplate(player,&t);
		apsDroidTemplates[player]->ref = REF_TEMPLATE_START;	// templates are the odd one out!
	}
	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// inform others that you no longer have a template

BOOL SendDestroyTemplate(DROID_TEMPLATE *t)
{
	uint8_t player = selectedPlayer;
	
	NETbeginEncode(NET_TEMPLATEDEST, NET_ALL_PLAYERS);
		NETuint8_t(&player);
		NETuint32_t(&t->multiPlayerID);
	NETend();

	return TRUE;
}

// acknowledge another player no longer has a template
static BOOL recvDestroyTemplate()
{
	uint8_t			player;
	uint32_t		templateID;
	DROID_TEMPLATE	*psTempl, *psTempPrev = NULL;
	
	NETbeginDecode();
		NETuint8_t(&player);
		NETuint32_t(&templateID);
	NETend();
	
	// Find the template in the list
	for (psTempl = apsDroidTemplates[player]; psTempl; psTempl = psTempl->psNext)
	{
		if (psTempl->multiPlayerID == templateID)
		{
			break;
		}
		
		psTempPrev = psTempl;
	}

	// If we found it then delete it
	if (psTempl)
	{
		// Update the linked list
		if (psTempPrev)
		{
			psTempPrev->psNext = psTempl->psNext;
		}
		else
		{
			apsDroidTemplates[player] = psTempl->psNext;
		}
		
		// Delete the template
		free(psTempl);
	}
	
	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Features

// send a destruct feature message.
BOOL SendDestroyFeature(FEATURE *pF)
{
	// Only send if it is our responsibility
	if (myResponsibility(pF->player))
		return TRUE;

	NETbeginEncode(NET_FEATUREDEST, NET_ALL_PLAYERS);
		NETuint32_t(&pF->id);
	return NETend();
}

// process a destroy feature msg.
BOOL recvDestroyFeature()
{
	FEATURE *pF;
	uint32_t	id;
	
	NETbeginDecode();
		NETuint32_t(&id);
	NETend();

	pF = IdToFeature(id,ANYPLAYER);

	if (pF == NULL)
	{
		return FALSE;
	}

	// Remove the feature locally
	turnOffMultiMsg(TRUE);
	removeFeature(pF);
	turnOffMultiMsg(FALSE);

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// Network File packet processor.
BOOL recvMapFileRequested(NETMSG *pMsg)
{
	char mapStr[256],mapName[256],fixedname[256];

	// another player is requesting the map
	if(!NetPlay.bHost)
	{
		return TRUE;
	}

	// start sending the map to the other players.
	if(!bSendingMap)
	{
		memset(mapStr,0,256);
		memset(mapName,0,256);
		memset(fixedname,0,256);
		bSendingMap = TRUE;
		addConsoleMessage("SENDING MAP!",DEFAULT_JUSTIFY);
		addConsoleMessage("FIX FOR LINUX!",DEFAULT_JUSTIFY);

		strlcpy(mapName, game.map, sizeof(mapName));
		// chop off the -T1
		mapName[strlen(game.map)-3] = 0;		// chop off the -T1 etc..

		// chop off the sk- if required.
		if(strncmp(mapName,"Sk-",3) == 0)
		{
			strlcpy(mapStr, &(mapName[3]), sizeof(mapStr));
			strlcpy(mapName, mapStr, sizeof(mapName));
		}

		snprintf(mapStr, sizeof(mapStr), "%dc-%s.wz", game.maxPlayers, mapName);
		snprintf(fixedname, sizeof(fixedname), "maps/%s", mapStr);		//We know maps are in /maps dir...now. fix for linux -Q
		strlcpy(mapStr, fixedname, sizeof(mapStr));
		NETsendFile(TRUE,mapStr,0);
	}

	return TRUE;
}

// continue sending the map
UBYTE sendMap(void)
{
	UBYTE done;
	static UDWORD lastCall;


	if(lastCall > gameTime)lastCall= 0;
	if ( (gameTime - lastCall) <200)
	{
		return 0;
	}
	lastCall = gameTime;


	done = NETsendFile(FALSE,game.map,0);

	if(done == 100)
	{
		addConsoleMessage("MAP SENT!",DEFAULT_JUSTIFY);
		bSendingMap = FALSE;
	}

	return done;
}

// another player is broadcasting a map, recv a chunk
BOOL recvMapFileData(NETMSG *pMsg)
{
	UBYTE done;

	done =  NETrecvFile(pMsg);
	if(done == 100)
	{
		addConsoleMessage("MAP DOWNLOADED!",DEFAULT_JUSTIFY);
		sendTextMessage("MAP DOWNLOADED",TRUE);					//send

		// clear out the old level list.
		levShutDown();
		levInitialise();
		if (!buildMapList()) {
			return FALSE;
		}
	}
	else
	{
		flushConsoleMessages();
		CONPRINTF(ConsoleString,(ConsoleString,"MAP:%d%%",done));
	}

	return TRUE;
}


//------------------------------------------------------------------------------------------------//

/* multiplayer message stack */
void msgStackReset(void)
{
	msgStackPos = -1;		//Beginning of the stack
}

UDWORD msgStackPush(SDWORD CBtype, SDWORD plFrom, SDWORD plTo, const char *tStr, SDWORD x, SDWORD y, DROID *psDroid)
{
	debug(LOG_WZ, "msgStackPush: pushing message type %d to pos %d", CBtype, msgStackPos + 1);

	if (msgStackPos >= MAX_MSG_STACK)
	{
		debug(LOG_ERROR, "msgStackPush() - stack full");
		return FALSE;
	}

	//make point to the last valid element
	msgStackPos++;

	//remember values
	msgPlFrom[msgStackPos] = plFrom;
	msgPlTo[msgStackPos] = plTo;

	callbackType[msgStackPos] = CBtype;
	locx[msgStackPos] = x;
	locy[msgStackPos] = y;

	strcpy(msgStr[msgStackPos], tStr);

	msgDroid[msgStackPos] = psDroid;

	return TRUE;
}

BOOL isMsgStackEmpty(void)
{
	if(msgStackPos <= (-1)) return TRUE;
	return FALSE;
}

BOOL msgStackGetFrom(SDWORD  *psVal)
{
	if(msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackGetFrom: msgStackPos < 0");
		return FALSE;
	}

	*psVal = msgPlFrom[0];

	return TRUE;
}

BOOL msgStackGetTo(SDWORD  *psVal)
{
	if(msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackGetTo: msgStackPos < 0");
		return FALSE;
	}

	*psVal = msgPlTo[0];

	return TRUE;
}

static BOOL msgStackGetCallbackType(SDWORD  *psVal)
{
	if(msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackGetCallbackType: msgStackPos < 0");
		return FALSE;
	}

	*psVal = callbackType[0];

	return TRUE;
}

static BOOL msgStackGetXY(SDWORD  *psValx, SDWORD  *psValy)
{
	if(msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackGetXY: msgStackPos < 0");
		return FALSE;
	}

	*psValx = locx[0];
	*psValy = locy[0];

	return TRUE;
}


BOOL msgStackGetMsg(char  *psVal)
{
	if(msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackGetMsg: msgStackPos < 0");
		return FALSE;
	}

	strcpy(psVal, msgStr[0]);
	//*psVal = msgPlTo[msgStackPos];

	return TRUE;
}

static BOOL msgStackSort(void)
{
	SDWORD i;

	//go through all-1 elements (bottom-top)
	for(i=0;i<msgStackPos;i++)
	{
		msgPlFrom[i] = msgPlFrom[i+1];
		msgPlTo[i] = msgPlTo[i+1];

		callbackType[i] = callbackType[i+1];
		locx[i] = locx[i+1];
		locy[i] = locy[i+1];

		strcpy(msgStr[i], msgStr[i+1]);
	}

	//erase top element
	msgPlFrom[msgStackPos] = -2;
	msgPlTo[msgStackPos] = -2;

	callbackType[msgStackPos] = -2;
	locx[msgStackPos] = -2;
	locy[msgStackPos] = -2;

	strcpy(msgStr[msgStackPos], "ERROR char!!!!!!!!");

	msgStackPos--;		//since removed the top element

	return TRUE;
}

BOOL msgStackPop(void)
{
	debug(LOG_WZ, "msgStackPop: stack size %d", msgStackPos);

	if(msgStackPos < 0 || msgStackPos > MAX_MSG_STACK)
	{
		debug(LOG_ERROR, "msgStackPop: wrong msgStackPos index: %d", msgStackPos);
		return FALSE;
	}

	return msgStackSort();		//move all elements 1 pos lower
}

BOOL msgStackGetDroid(DROID **ppsDroid)
{
	if(msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackGetDroid: msgStackPos < 0");
		return FALSE;
	}

	*ppsDroid = msgDroid[0];

	return TRUE;
}

SDWORD msgStackGetCount(void)
{
	return msgStackPos + 1;
}

BOOL msgStackFireTop(void)
{
	SDWORD		_callbackType;
	char		msg[255];

	if(msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackFireTop: msgStackPos < 0");
		return FALSE;
	}

	if(!msgStackGetCallbackType(&_callbackType))
		return FALSE;

	switch(_callbackType)
	{
		case CALL_VIDEO_QUIT:
			debug(LOG_SCRIPT, "msgStackFireTop: popped CALL_VIDEO_QUIT");
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_VIDEO_QUIT);
			break;

		case CALL_DORDER_STOP:
			ASSERT(FALSE,"CALL_DORDER_STOP is currently disabled");

			debug(LOG_SCRIPT, "msgStackFireTop: popped CALL_DORDER_STOP");

			if(!msgStackGetDroid(&psScrCBOrderDroid))
				return FALSE;

			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DORDER_STOP);
			break;

		case CALL_BEACON:

			if(!msgStackGetXY(&beaconX, &beaconY))
				return FALSE;

			if(!msgStackGetFrom(&MultiMsgPlayerFrom))
				return FALSE;

			if(!msgStackGetTo(&MultiMsgPlayerTo))
				return FALSE;

			if(!msgStackGetMsg(msg))
				return FALSE;

			strcpy(MultiplayMsg, msg);

			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_BEACON);
			break;

		case CALL_AI_MSG:
			if(!msgStackGetFrom(&MultiMsgPlayerFrom))
				return FALSE;

			if(!msgStackGetTo(&MultiMsgPlayerTo))
				return FALSE;

			if(!msgStackGetMsg(msg))
				return FALSE;

			strcpy(MultiplayMsg, msg);

			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_AI_MSG);
			break;

		default:
			debug(LOG_ERROR, "msgStackFireTop: unknown callback type");
			return FALSE;
			break;
	}

	if(!msgStackPop())
		return FALSE;

	return TRUE;
}

static BOOL recvBeacon()
{
	int32_t sender, receiver,locX, locY;
	char    msg[MAX_CONSOLE_STRING_LENGTH];

	NETbeginDecode();
	    NETint32_t(&sender);            // the actual sender
	    NETint32_t(&receiver);          // the actual receiver (might not be the same as the one we are actually sending to, in case of AIs)
	    NETint32_t(&locX);
	    NETint32_t(&locY);
	    NETstring(msg, sizeof(msg));    // Receive the actual message
	NETend();

	debug(LOG_WZ, "Received beacon for player: %d, from: %d",receiver, sender);

	strlcat(msg, NetPlay.players[sender].name, sizeof(msg));    // name
	strlcpy(beaconReceiveMsg[sender], msg, sizeof(beaconReceiveMsg));

	return addHelpBlip(locX, locY, receiver, sender, beaconReceiveMsg[sender]);
}

static const char* playerColors[] =
{
	N_("Green"),
	N_("Orange"),
	N_("Grey"),
	N_("Black"),
	N_("Red"),
	N_("Blue"),
	N_("Pink"),
	N_("Cyan")
};

char *getPlayerColourName(SDWORD player)
{
	static const unsigned int end = sizeof(playerColors) / sizeof(const char*);

	ASSERT(player < end,
	       "getPlayerColourName: player number (%d) exceeds maximum (%d)\n", player, end - 1);

	if (player < end)
	{
		strcpy(tempString, _(playerColors[ getPlayerColour(player) ]));
	}
	else
	{
		tempString[0] = '0';
	}

	return tempString;
}

/*
 * returns player in-game index from a dpid or -1 if player not found (should not happen!)
 */
SDWORD dpidToPlayer(SDWORD dpid)
{
	UDWORD i;

	for(i=0;(i<MAX_PLAYERS) && (dpid != player2dpid[i]); i++);

	if(i >= MAX_PLAYERS)
	{
		ASSERT(i< MAX_PLAYERS, "dpidToPlayer: failed to find player with dpid %d", dpid);
		return -1;
	}

	return i;
}
