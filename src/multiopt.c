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
 * MultiOpt.c
 *
 * Alex Lee,97/98, Pumpkin Studios
 *
 * Routines for setting the game options and starting the init process.
 */
#include "lib/framework/frame.h"			// for everything
#include "map.h"
#include "game.h"			// for loading maps
#include "message.h"		// for clearing messages.
#include "main.h"
#include "display3d.h"		// for changing the viewpoint
#include "power.h"
#include "lib/widget/widget.h"
#include "lib/gamelib/gtime.h"
#include "lib/netplay/netplay.h"
#include "hci.h"
#include "configuration.h"			// lobby cfg.
#include "clparse.h"
#include "lib/ivis_common/piestate.h"

#include "component.h"
#include "console.h"
#include "multiplay.h"
#include "lib/sound/audio.h"
#include "multijoin.h"
#include "frontend.h"
#include "levels.h"
#include "multistat.h"
#include "multiint.h"
#include "multilimit.h"
#include "multigifts.h"
#include "aiexperience.h"	//for beacon messages
#include "multiint.h"
#include "multirecv.h"

// ////////////////////////////////////////////////////////////////////////////
// External Variables

extern char	buildTime[8];

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////

// send complete game info set!
// dpid == 0 for no new players.
void sendOptions(uint32_t dest, uint32_t play)
{
	int i, j;

	NETbeginEncode(NET_OPTIONS, NET_ALL_PLAYERS);

	// First send information about the game
	NETuint8_t(&game.type);
	NETstring(game.map, 128);
	NETstring(game.version, 8);
	NETuint8_t(&game.maxPlayers);
	NETstring(game.name, 128);
	NETbool(&game.fog);
	NETuint32_t(&game.power);
	NETuint8_t(&game.base);
	NETuint8_t(&game.alliance);

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		NETuint8_t(&game.skDiff[i]);
	}

	// Now the dpid array
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		// We should probably re-define player2dpid as a uint8_t
		NETint32_t(&player2dpid[i]);
	}

	// Send the list of who is still joining
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		NETbool(&ingame.JoiningInProgress[i]);
	}

	// Send dest and play
	NETuint32_t(&dest);
	NETuint32_t(&play);

	// Send over the list of player colours
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		NETuint8_t(&PlayerColour[i]);
	}

	// Same goes for the alliances
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		for (j = 0; j < MAX_PLAYERS; j++)
		{
			NETuint8_t(&alliances[i][j]);
		}
	}

	// Send their team (this is apparently not related to alliances
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		// This should also be a uint8_t
		NETint32_t(&playerTeamGUI[i]);
	}

	// Send the number of structure limits to expect
	NETuint32_t(&ingame.numStructureLimits);

	// Send the structures changed
	for (i = 0; i < ingame.numStructureLimits; i++)
	{
		NETuint8_t(&ingame.pStructureLimits[i].id);
		NETuint8_t(&ingame.pStructureLimits[i].limit);
	}

	NETend();
}

// ////////////////////////////////////////////////////////////////////////////
/*!
 * check the wdg files that are being used.
 */
static BOOL checkGameWdg(const char *nm)
{
	LEVEL_DATASET *lev;

	for (lev = psLevels; lev; lev = lev->psNext)
	{
		if (strcmp(lev->pName, nm) == 0)
		{
			return true;
		}
	}

	return false;
}

// ////////////////////////////////////////////////////////////////////////////
// options for a game. (usually recvd in frontend)
void recvOptions()
{
	int		i, j;
	UDWORD	play;
	UDWORD	newPl;

	NETbeginDecode(NET_OPTIONS);

	// Get general information about the game
	NETuint8_t(&game.type);
	NETstring(game.map, 128);
	NETstring(game.version, 8);
	NETuint8_t(&game.maxPlayers);
	NETstring(game.name, 128);
	NETbool(&game.fog);
	NETuint32_t(&game.power);
	NETuint8_t(&game.base);
	NETuint8_t(&game.alliance);

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		NETuint8_t(&game.skDiff[i]);
	}

	// Check the version
	if (strncmp(game.version, buildTime, 8) != 0)
	{
		debug(LOG_ERROR, "Host is running a different version of Warzone2100.");
	}

	// Now the dpid array
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		NETint32_t(&player2dpid[i]);
	}

	// Send the list of who is still joining
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		NETbool(&ingame.JoiningInProgress[i]);
	}

	NETuint32_t(&newPl);
	NETuint32_t(&play);

	// Player colours
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		NETuint8_t(&PlayerColour[i]);
	}

	// Alliances
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		for (j = 0; j < MAX_PLAYERS; j++)
		{
			NETuint8_t(&alliances[i][j]);
		}
	}

	// Teams
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		NETint32_t(&playerTeamGUI[i]);
	}

	// Free any structure limits we may have in-place
	if (ingame.numStructureLimits)
	{
		ingame.numStructureLimits = 0;
		free(ingame.pStructureLimits);
		ingame.pStructureLimits = NULL;
	}

	// Get the number of structure limits to expect
	NETuint32_t(&ingame.numStructureLimits);

	// If there were any changes allocate memory for them
	if (ingame.numStructureLimits)
	{
		ingame.pStructureLimits = malloc(ingame.numStructureLimits * sizeof(MULTISTRUCTLIMITS));
	}

	for (i = 0; i < ingame.numStructureLimits; i++)
	{
		NETuint8_t(&ingame.pStructureLimits[i].id);
		NETuint8_t(&ingame.pStructureLimits[i].limit);
	}

	NETend();

	// Post process
	if (newPl != 0)
	{
		// If we are the new player
		if (newPl == NetPlay.dpidPlayer)
 		{
			selectedPlayer = play;		// Select player
			NETplayerInfo();			// Get player info
			powerCalculated = false;	// Turn off any power requirements
		}
		// Someone else is joining.
		else
		{
			setupNewPlayer(newPl, play);
 		}
 	}

	// Do the skirmish slider settings if they are up
	for (i = 0; i < MAX_PLAYERS; i++)
 	{
		if (widgGetFromID(psWScreen, MULTIOP_SKSLIDE + i))
		{
			widgSetSliderPos(psWScreen, MULTIOP_SKSLIDE + i, game.skDiff[i]);
		}
	}

	// See if we have the map or not
	if (!checkGameWdg(game.map))
	{
		// Request the map from the host
		NETbeginEncode(NET_REQUESTMAP, NET_ALL_PLAYERS);

		NETuint32_t(&NetPlay.dpidPlayer);

		NETend();

		addConsoleMessage("MAP REQUESTED!",DEFAULT_JUSTIFY, CONSOLE_SYSTEM);
	}
	else
	{
		loadMapPreview();
	}
}


// ////////////////////////////////////////////////////////////////////////////
// Host Campaign.
BOOL hostCampaign(char *sGame, char *sPlayer)
{
	PLAYERSTATS playerStats;
	UDWORD		pl,numpl,i,j;

	debug(LOG_WZ, "Hosting campaign: '%s', player: '%s'", sGame, sPlayer);

	freeMessages();

	// If we had a single player (i.e. campaign) game before this value will
	// have been set to 0. So revert back to the default value.
	if (game.maxPlayers == 0)
	{
		game.maxPlayers = 4;
	}

	NEThostGame(sGame, sPlayer, game.type, 0, 0, 0, game.maxPlayers); // temporary stuff

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		player2dpid[i] = 0;
		setPlayerName(i, ""); //Clear custom names (use default ones instead)
	}


	pl = rand()%game.maxPlayers;						//pick a player

	player2dpid[pl] = NetPlay.dpidPlayer;				// add ourselves to the array.
	selectedPlayer = pl;

	ingame.localJoiningInProgress = true;
	ingame.JoiningInProgress[selectedPlayer] = true;
	bMultiPlayer = true;								// enable messages

	loadMultiStats(sPlayer,&playerStats);				// stats stuff
	setMultiStats(NetPlay.dpidPlayer,playerStats,false);
	setMultiStats(NetPlay.dpidPlayer,playerStats,true);

	if(!NetPlay.bComms)
	{
		NETplayerInfo();
		strcpy(NetPlay.players[0].name,sPlayer);
		numpl = 1;
	}
	else
	{
		numpl = NETplayerInfo();
	}

	// may be more than one player already. check and resolve!
	if(numpl >1)
	{
		for(j = 0;j<MAX_PLAYERS;j++)
		{
			if(NetPlay.players[j].dpid && (NetPlay.players[j].dpid != NetPlay.dpidPlayer))
			{
				for(i = 0;player2dpid[i] != 0;i++);
				player2dpid[i] = NetPlay.players[j].dpid;
			}
		}
	}

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// Join Campaign

BOOL joinCampaign(UDWORD gameNumber, char *sPlayer)
{
	PLAYERSTATS	playerStats;

	if(!ingame.localJoiningInProgress)
	{
		NETjoinGame(gameNumber, sPlayer);	// join
		ingame.localJoiningInProgress	= true;

		loadMultiStats(sPlayer,&playerStats);
		setMultiStats(NetPlay.dpidPlayer,playerStats,false);
		setMultiStats(NetPlay.dpidPlayer,playerStats,true);
		return false;
	}

	bMultiPlayer = true;
	return true;
}

#if 0	// unused - useful?
// ////////////////////////////////////////////////////////////////////////////
// Lobby launched. fires the correct routine when the game was lobby launched.
BOOL LobbyLaunched(void)
{
	UDWORD i;
	PLAYERSTATS pl = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	// set the player info as soon as possible to avoid screwy scores appearing elsewhere.
	NETplayerInfo();
	NETfindGame();

	for (i = 0; i < MAX_PLAYERS && NetPlay.players[i].dpid != NetPlay.dpidPlayer; i++);

	if(!loadMultiStats(NetPlay.players[i].name, &pl) )
	{
		return false; // cheating was detected, so fail.
	}

	setMultiStats(NetPlay.dpidPlayer, pl, false);
	setMultiStats(NetPlay.dpidPlayer, pl, true);

	// setup text boxes on multiplay screen.
	strcpy((char*) sPlayer, NetPlay.players[i].name);
	strcpy((char*) game.name, NetPlay.games[0].name);

	return true;
}
#endif

// ////////////////////////////////////////////////////////////////////////////
// say goodbye to everyone else
BOOL sendLeavingMsg(void)
{
	/*
	 * Send a leaving message, This resolves a problem with tcpip which
	 * occasionally doesn't automatically notice a player leaving
	 */
	NETbeginEncode(NET_LEAVING, NET_ALL_PLAYERS);
	{
		uint32_t player_id = player2dpid[selectedPlayer];
		BOOL host = NetPlay.bHost;

		NETuint32_t(&player_id);
		NETbool(&host);
	}
	NETend();

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// called in Init.c to shutdown the whole netgame gubbins.
BOOL multiShutdown(void)
{
	// shut down netplay lib.
	debug(LOG_MAIN, "shutting down networking");
  	NETshutdown();

	debug(LOG_MAIN, "free game data (structure limits)");
	if(ingame.numStructureLimits)
	{
		ingame.numStructureLimits = 0;
		free(ingame.pStructureLimits);
		ingame.pStructureLimits = NULL;
	}

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// copy tempates from one player to another.
BOOL addTemplate(UDWORD player, DROID_TEMPLATE *psNew)
{
	DROID_TEMPLATE *psTempl = malloc(sizeof(DROID_TEMPLATE));

	if (psTempl == NULL)
	{
		debug(LOG_ERROR, "addTemplate: Out of memory");
		return false;
	}
	memcpy(psTempl, psNew, sizeof(DROID_TEMPLATE));
	psTempl->pName = NULL;

	if (psNew->pName)
	{
		psTempl->pName = strdup(psNew->pName);
	}

	psTempl->psNext = apsDroidTemplates[player];
	apsDroidTemplates[player] = psTempl;

	return true;
}

BOOL addTemplateSet(UDWORD from,UDWORD to)
{
	DROID_TEMPLATE	*psCurr;

	if(from == to)
	{
		return true;
	}

	for(psCurr = apsDroidTemplates[from];psCurr;psCurr= psCurr->psNext)
	{
		addTemplate(to, psCurr);
	}

	return true;
}

BOOL copyTemplateSet(UDWORD from,UDWORD to)
{
	DROID_TEMPLATE	*psTempl;

	if(from == to)
	{
		return true;
	}

	while(apsDroidTemplates[to])				// clear the old template out.
	{
		psTempl = apsDroidTemplates[to]->psNext;
		free(apsDroidTemplates[to]);
		apsDroidTemplates[to] = psTempl;
	}

	return 	addTemplateSet(from,to);
}

// ////////////////////////////////////////////////////////////////////////////
// setup templates
BOOL multiTemplateSetup(void)
{
	UDWORD player, pcPlayer = 0;

	switch (game.type)
	{
	case CAMPAIGN:
		for(player=0;player<game.maxPlayers;player++)
		{
			copyTemplateSet(CAMPAIGNTEMPLATES,player);
		}
		break;

	case SKIRMISH:
		// create the pc player list in deathmatch set.
		addTemplateSet(CAMPAIGNTEMPLATES,DEATHMATCHTEMPLATES);
		addTemplateSet(6,DEATHMATCHTEMPLATES);
		addTemplateSet(2,DEATHMATCHTEMPLATES);

		//choose which way to do this.
		if(isHumanPlayer(CAMPAIGNTEMPLATES))
		{
			//pc first
			for(player=0;player<game.maxPlayers;player++)
			{
				if(!isHumanPlayer(player))
				{
					copyTemplateSet(DEATHMATCHTEMPLATES,player);
				}
			}
			//now players.
			for(player=0;player<game.maxPlayers;player++)
			{
				if(isHumanPlayer(player))
				{
					copyTemplateSet(CAMPAIGNTEMPLATES,player);
				}
			}
		}
		else
		{
			// ensure a copy of pc templates to a pc player.
			if(isHumanPlayer(DEATHMATCHTEMPLATES))
			{

				for(player=0;player<MAX_PLAYERS && isHumanPlayer(player);player++);
				if(!isHumanPlayer(player))
				{
					pcPlayer = player;
					copyTemplateSet(DEATHMATCHTEMPLATES,pcPlayer);
				}
			}
			else
			{
				pcPlayer = DEATHMATCHTEMPLATES;
			}
			//players first
			for(player=0;player<game.maxPlayers;player++)
			{
				if(isHumanPlayer(player))
				{
					copyTemplateSet(CAMPAIGNTEMPLATES,player);
				}
			}
			//now pc
			for(player=0;player<game.maxPlayers;player++)
			{
				if(!isHumanPlayer(player))
				{
					copyTemplateSet(pcPlayer,player);
				}
			}
		}
		break;

	default:
		break;
	}

	return true;
}



// ////////////////////////////////////////////////////////////////////////////
// remove structures from map before campaign play.
static BOOL cleanMap(UDWORD player)
{
	DROID		*psD,*psD2;
	STRUCTURE	*psStruct;
	BOOL		firstFact,firstRes;

	bMultiPlayer = false;

	firstFact = true;
	firstRes = true;

	// reverse so we always remove the last object. re-reverse afterwards.
//	reverseObjectList((BASE_OBJECT**)&apsStructLists[player]);

	switch(game.base)
	{
	case CAMP_CLEAN:									//clean map
		while(apsStructLists[player])					//strip away structures.
		{
			removeStruct(apsStructLists[player], true);
		}
		psD = apsDroidLists[player];					// remove all but construction droids.
		while(psD)
		{
			psD2=psD->psNext;
			//if(psD->droidType != DROID_CONSTRUCT)
            if (!(psD->droidType == DROID_CONSTRUCT ||
                psD->droidType == DROID_CYBORG_CONSTRUCT))
			{
				killDroid(psD);
			}
			psD = psD2;
		}
		break;

	case CAMP_BASE:												//just structs, no walls
		psStruct = apsStructLists[player];
		while(psStruct)
		{
			if ( (psStruct->pStructureType->type == REF_WALL)
			   ||(psStruct->pStructureType->type == REF_WALLCORNER)
			   ||(psStruct->pStructureType->type == REF_DEFENSE)
			   ||(psStruct->pStructureType->type == REF_BLASTDOOR)
			   ||(psStruct->pStructureType->type == REF_CYBORG_FACTORY)
			   ||(psStruct->pStructureType->type == REF_COMMAND_CONTROL)
			   )
			{
				removeStruct(psStruct, true);
				psStruct= apsStructLists[player];			//restart,(list may have changed).
			}

			else if( (psStruct->pStructureType->type == REF_FACTORY)
				   ||(psStruct->pStructureType->type == REF_RESEARCH)
				   ||(psStruct->pStructureType->type == REF_POWER_GEN))
			{
				if(psStruct->pStructureType->type == REF_FACTORY )
				{
					if(firstFact == true)
					{
						firstFact = false;
						removeStruct(psStruct, true);
						psStruct= apsStructLists[player];
					}
					else	// don't delete, just rejig!
					{
						if(((FACTORY*)psStruct->pFunctionality)->capacity != 0)
						{
							((FACTORY*)psStruct->pFunctionality)->capacity = 0;
							((FACTORY*)psStruct->pFunctionality)->productionOutput = (UBYTE)((PRODUCTION_FUNCTION*)psStruct->pStructureType->asFuncList[0])->productionOutput;

							psStruct->sDisplay.imd	= psStruct->pStructureType->pIMD;
							psStruct->body			= (UWORD)(structureBody(psStruct));

						}
						psStruct				= psStruct->psNext;
					}
				}
				else if(psStruct->pStructureType->type == REF_RESEARCH)
				{
					if(firstRes == true)
					{
						firstRes = false;
						removeStruct(psStruct, true);
						psStruct= apsStructLists[player];
					}
					else
					{
						if(((RESEARCH_FACILITY*)psStruct->pFunctionality)->capacity != 0)
						{	// downgrade research
							((RESEARCH_FACILITY*)psStruct->pFunctionality)->capacity = 0;
							((RESEARCH_FACILITY*)psStruct->pFunctionality)->researchPoints = ((RESEARCH_FUNCTION*)psStruct->pStructureType->asFuncList[0])->researchPoints;
							psStruct->sDisplay.imd	= psStruct->pStructureType->pIMD;
							psStruct->body			= (UWORD)(structureBody(psStruct));
						}
						psStruct=psStruct->psNext;
					}
				}
				else if(psStruct->pStructureType->type == REF_POWER_GEN)
				{
						if(((POWER_GEN*)psStruct->pFunctionality)->capacity != 0)
						{	// downgrade powergen.
							((POWER_GEN*)psStruct->pFunctionality)->capacity = 0;
							((POWER_GEN*)psStruct->pFunctionality)->power = ((POWER_GEN_FUNCTION*)psStruct->pStructureType->asFuncList[0])->powerOutput;
							((POWER_GEN*)psStruct->pFunctionality)->multiplier += ((POWER_GEN_FUNCTION*)psStruct->pStructureType->asFuncList[0])->powerMultiplier;

							psStruct->sDisplay.imd	= psStruct->pStructureType->pIMD;
							psStruct->body			= (UWORD)(structureBody(psStruct));
						}
						psStruct=psStruct->psNext;
				}
			}

			else
			{
				psStruct=psStruct->psNext;
			}
		}
		break;


	case CAMP_WALLS:												//everything.
		break;
	default:
		debug( LOG_ERROR, "Unknown Campaign Style" );
		abort();
		break;
	}

	// rerev list to get back to normal.
//	reverseObjectList((BASE_OBJECT**)&apsStructLists[player]);

	bMultiPlayer = true;
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// setup a campaign game
static BOOL campInit(void)
{
	UDWORD			player;
	UBYTE		newPlayerArray[MAX_PLAYERS];
	UDWORD		i,j,lastAI;
	SDWORD		newPlayerTeam[MAX_PLAYERS] = {-1,-1,-1,-1,-1,-1,-1,-1};

// if this is from a savegame, stop here!
	if((getSaveGameType() == GTYPE_SAVE_START)
	|| (getSaveGameType() == GTYPE_SAVE_MIDMISSION)	)
	{
		// these two lines are the biggest hack in the world.
		// the reticule seems to get detached from 'reticuleup'
		// this forces it back in sync...
		intRemoveReticule();
		intAddReticule();

		return true;
	}

	//Convert skirmish GUI player ids to in-game ids
	if(game.type == SKIRMISH)
	{
		lastAI = 0;		//last used AI slot
		memset(newPlayerArray,1,MAX_PLAYERS * sizeof(newPlayerArray[0]));		//'1' for humans
		for(i=0;i<MAX_PLAYERS;i++)
		{
			if(game.skDiff[i] < UBYTE_MAX )		//slot with enabled or disabled AI
			{
				//find first unused slot
				for(j=lastAI;j<MAX_PLAYERS && isHumanPlayer(j);j++);	//skip humans

				ASSERT(j<MAX_PLAYERS,"campInit: couldn't find free slot while assigning AI %d , lastAI=%d", i, lastAI);

				newPlayerArray[j] = game.skDiff[i];		//copy over
				newPlayerTeam[j] = playerTeamGUI[i];

				//remove player if it was disabled in menus
				if(game.skDiff[i] == 0)
					clearPlayer(j,true,false);

				lastAI = j;
				lastAI++;
			}
			else if(game.skDiff[i] == UBYTE_MAX)	//human player
			{
				//find player net id
				for(j=0;(j < MAX_PLAYERS) && (player2dpid[j] != NetPlay.players[i].dpid);j++);

				ASSERT(j<MAX_PLAYERS,"campInit: couldn't find player id for GUI id %d", i);

				newPlayerTeam[j] = playerTeamGUI[i];
			}

		}

		memcpy(game.skDiff,newPlayerArray,MAX_PLAYERS  * sizeof(newPlayerArray[0]));
		memcpy(playerTeam,newPlayerTeam,MAX_PLAYERS * sizeof(newPlayerTeam[0]));
	}

	for(player = 0;player<game.maxPlayers;player++)			// clean up only to the player limit for this map..
	{
		if( (!isHumanPlayer(player)) && game.type != SKIRMISH)	// strip away unused players
		{
			clearPlayer(player,true,true);
		}

		cleanMap(player);
	}

	// optionally remove other computer players.
	// HACK: if actual number of players is 8, then drop baba player to avoid
	// exceeding player number (babas need a player, too!) - Per
	if ((game.type == CAMPAIGN && game.maxPlayers > 7) || game.type == SKIRMISH)
	{
		for(player=game.maxPlayers;player<MAX_PLAYERS;player++)
		{
			clearPlayer(player,true,false);
		}
	}

	// add free research gifts..
	if(NetPlay.bHost)
	{
		addOilDrum( NetPlay.playercount*2 );		// add some free power.
	}

	playerResponding();			// say howdy!

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// say hi to everyone else....
void playerResponding(void)
{
	ingame.startTime = gameTime;
	ingame.localJoiningInProgress = false; // No longer joining.
	ingame.JoiningInProgress[selectedPlayer] = false;

	// Home the camera to the player
	cameraToHome(selectedPlayer, false);

	// Tell the world we're here
	NETbeginEncode(NET_PLAYERRESPONDING, NET_ALL_PLAYERS);
	NETuint32_t(&selectedPlayer);
	NETend();
}

// ////////////////////////////////////////////////////////////////////////////
//called when the game finally gets fired up.
BOOL multiGameInit(void)
{
	UDWORD player;

	for (player = 0; player < MAX_PLAYERS; player++)
	{
		openchannels[player] =true;								//open comms to this player.
	}

	campInit();

	InitializeAIExperience();
	msgStackReset();	//for multiplayer msgs, reset message stack


	return true;
}

////////////////////////////////
// at the end of every game.
BOOL multiGameShutdown(void)
{
	PLAYERSTATS	st;

	sendLeavingMsg();							// say goodbye
	updateMultiStatsGames();					// update games played.

	st = getMultiStats(selectedPlayer, true);	// save stats

	saveMultiStats(getPlayerName(selectedPlayer), getPlayerName(selectedPlayer), &st);

	// close game
	NETclose();

	if (ingame.numStructureLimits)
	{
		ingame.numStructureLimits = 0;
		free(ingame.pStructureLimits);
		ingame.pStructureLimits = NULL;
	}

	ingame.localJoiningInProgress = false; // Clean up
	ingame.localOptionsReceived = false;
	ingame.bHostSetup = false;	// Dont attempt a host
	NetPlay.bHost					= false;
	bMultiPlayer					= false;	// Back to single player mode
	selectedPlayer					= 0;		// Back to use player 0 (single player friendly)

	return true;
}
