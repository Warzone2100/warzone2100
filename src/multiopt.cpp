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
#include "lib/ivis_opengl/piestate.h"

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
#include "multiint.h"
#include "multirecv.h"
#include "scriptfuncs.h"
#include "template.h"
#include "lib/framework/wzapp.h"


// send complete game info set!
void sendOptions()
{
	bool dummy = true;
	unsigned int i;

	NETbeginEncode(NETbroadcastQueue(), NET_OPTIONS);

	// First send information about the game
	NETuint8_t(&game.type);
	NETstring(game.map, 128);
	NETuint8_t(&game.maxPlayers);
	NETstring(game.name, 128);
	NETbool(&dummy);
	NETuint32_t(&game.power);
	NETuint8_t(&game.base);
	NETuint8_t(&game.alliance);
	NETbool(&game.scavengers);

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		NETuint8_t(&game.skDiff[i]);
	}

	// Send the list of who is still joining
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		NETbool(&ingame.JoiningInProgress[i]);
	}

	// Same goes for the alliances
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		unsigned int j;

		for (j = 0; j < MAX_PLAYERS; j++)
		{
			NETuint8_t(&alliances[i][j]);
		}
	}

	// Send the number of structure limits to expect
	NETuint32_t(&ingame.numStructureLimits);
	debug(LOG_NET, "(Host) Structure limits to process on client is %u", ingame.numStructureLimits);
	// Send the structures changed
	for (i = 0; i < ingame.numStructureLimits; i++)
	{
		NETuint32_t(&ingame.pStructureLimits[i].id);
		NETuint32_t(&ingame.pStructureLimits[i].limit);
	}
	updateLimitFlags();
	NETuint8_t(&ingame.flags);

	NETend();
}

// ////////////////////////////////////////////////////////////////////////////
/*!
 * check the wdg files that are being used.
 */
static bool checkGameWdg(const char *nm)
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
void recvOptions(NETQUEUE queue)
{
	unsigned int i;
	bool dummy = true;

	debug(LOG_NET, "Receiving options from host");
	NETbeginDecode(queue, NET_OPTIONS);

	// Get general information about the game
	NETuint8_t(&game.type);
	NETstring(game.map, 128);
	NETuint8_t(&game.maxPlayers);
	NETstring(game.name, 128);
	NETbool(&dummy);
	NETuint32_t(&game.power);
	NETuint8_t(&game.base);
	NETuint8_t(&game.alliance);
	NETbool(&game.scavengers);

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		NETuint8_t(&game.skDiff[i]);
	}

	// Send the list of who is still joining
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		NETbool(&ingame.JoiningInProgress[i]);
	}

	// Alliances
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		unsigned int j;

		for (j = 0; j < MAX_PLAYERS; j++)
		{
			NETuint8_t(&alliances[i][j]);
		}
	}
	netPlayersUpdated = true;

	// Free any structure limits we may have in-place
	if (ingame.numStructureLimits)
	{
		ingame.numStructureLimits = 0;
		free(ingame.pStructureLimits);
		ingame.pStructureLimits = NULL;
	}

	// Get the number of structure limits to expect
	NETuint32_t(&ingame.numStructureLimits);
	debug(LOG_NET, "Host is sending us %u structure limits", ingame.numStructureLimits);
	// If there were any changes allocate memory for them
	if (ingame.numStructureLimits)
	{
		ingame.pStructureLimits = (MULTISTRUCTLIMITS *)malloc(ingame.numStructureLimits * sizeof(MULTISTRUCTLIMITS));
	}

	for (i = 0; i < ingame.numStructureLimits; i++)
	{
		NETuint32_t(&ingame.pStructureLimits[i].id);
		NETuint32_t(&ingame.pStructureLimits[i].limit);
	}
	NETuint8_t(&ingame.flags);

	NETend();

	// Do the skirmish slider settings if they are up
	for (i = 0; i < MAX_PLAYERS; i++)
 	{
		if (widgGetFromID(psWScreen, MULTIOP_SKSLIDE + i))
		{
			widgSetSliderPos(psWScreen, MULTIOP_SKSLIDE + i, game.skDiff[i]);
		}
	}
	debug(LOG_NET, "Rebuilding map list");
	// clear out the old level list.
	levShutDown();
	levInitialise();
	rebuildSearchPath(mod_multiplay, true);	// MUST rebuild search path for the new maps we just got!
	buildMapList();
	// See if we have the map or not
	if (!checkGameWdg(game.map))
	{
		uint32_t player = selectedPlayer;

		debug(LOG_NET, "Map was not found, requesting map %s from host.", game.map);
		// Request the map from the host
		NETbeginEncode(NETnetQueue(NET_HOST_ONLY), NET_FILE_REQUESTED);
		NETuint32_t(&player);
		NETend();

		addConsoleMessage("MAP REQUESTED!",DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	}
	else
	{
		loadMapPreview(false);
	}
}


// ////////////////////////////////////////////////////////////////////////////
// Host Campaign.
bool hostCampaign(char *sGame, char *sPlayer)
{
	PLAYERSTATS playerStats;
	UDWORD		i;

	debug(LOG_WZ, "Hosting campaign: '%s', player: '%s'", sGame, sPlayer);

	freeMessages();

	// If we had a single player (i.e. campaign) game before this value will
	// have been set to 0. So revert back to the default value.
	if (game.maxPlayers == 0)
	{
		game.maxPlayers = 4;
	}

	if (!NEThostGame(sGame, sPlayer, game.type, 0, 0, 0, game.maxPlayers))
	{
		return false;
	}

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		if (NetPlay.bComms)
		{
			game.skDiff[i] = 0;     	// disable AI
		}
		setPlayerName(i, ""); //Clear custom names (use default ones instead)
	}

	NetPlay.players[selectedPlayer].ready = false;

	ingame.localJoiningInProgress = true;
	ingame.JoiningInProgress[selectedPlayer] = true;
	bMultiPlayer = true;
	bMultiMessages = true; // enable messages

	loadMultiStats(sPlayer,&playerStats);				// stats stuff
	setMultiStats(selectedPlayer, playerStats, false);
	setMultiStats(selectedPlayer, playerStats, true);

	if(!NetPlay.bComms)
	{
		strcpy(NetPlay.players[0].name,sPlayer);
	}

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// Tell the host we are leaving the game 'nicely', (we wanted to) and not
// because we have some kind of error. (dropped or disconnected)
bool sendLeavingMsg(void)
{
	debug(LOG_NET, "We are leaving 'nicely'");
	NETbeginEncode(NETnetQueue(NET_HOST_ONLY), NET_PLAYER_LEAVING);
	{
		bool host = NetPlay.isHost;
		uint32_t id = selectedPlayer;

		NETuint32_t(&id);
		NETbool(&host);
	}
	NETend();

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// called in Init.c to shutdown the whole netgame gubbins.
bool multiShutdown(void)
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
// copy templates from one player to another.
bool addTemplateToList(DROID_TEMPLATE *psNew, DROID_TEMPLATE **ppList)
{
	DROID_TEMPLATE *psTempl = new DROID_TEMPLATE(*psNew);
	psTempl->pName = NULL;

	if (psNew->pName)
	{
		psTempl->pName = strdup(psNew->pName);
	}

	psTempl->psNext = *ppList;
	*ppList = psTempl;

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// copy templates from one player to another.
bool addTemplate(UDWORD player, DROID_TEMPLATE *psNew)
{
	return addTemplateToList(psNew, &apsDroidTemplates[player]);
}

void addTemplateBack(unsigned player, DROID_TEMPLATE *psNew)
{
	DROID_TEMPLATE **ppList = &apsDroidTemplates[player];
	while (*ppList != NULL)
	{
		ppList = &(*ppList)->psNext;
	}
	addTemplateToList(psNew, ppList);
}

// ////////////////////////////////////////////////////////////////////////////
// setup templates
bool multiTemplateSetup(void)
{
	// do nothing now
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
static bool gameInit(void)
{
	UDWORD			player;

	// If this is from a savegame, stop here!
	if (getSaveGameType() == GTYPE_SAVE_START || getSaveGameType() == GTYPE_SAVE_MIDMISSION)
	{
		// these two lines are the biggest hack in the world.
		// the reticule seems to get detached from 'reticuleup'
		// this forces it back in sync...
		intRemoveReticule();
		intAddReticule();

		return true;
	}

	for (player = 1; player < MAX_PLAYERS; player++)
	{
		// we want to remove disabled AI & all the other players that don't belong
		if ((game.skDiff[player] == 0 || player >= game.maxPlayers) && player != scavengerPlayer())
		{
			clearPlayer(player, true);			// do this quietly
			debug(LOG_NET, "removing disabled AI (%d) from map.", player);
		}
	}

	if (game.scavengers)	// FIXME - not sure if we still need this hack - Per
	{
		// ugly hack for now
		game.skDiff[scavengerPlayer()] = DIFF_SLIDER_STOPS / 2;
	}

	if (NetPlay.isHost)	// add oil drums
	{
		addOilDrum(NetPlay.playercount * 2);
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
	NETbeginEncode(NETbroadcastQueue(), NET_PLAYERRESPONDING);
	NETuint32_t(&selectedPlayer);
	NETend();
}

// ////////////////////////////////////////////////////////////////////////////
//called when the game finally gets fired up.
bool multiGameInit(void)
{
	UDWORD player;

	for (player = 0; player < MAX_PLAYERS; player++)
	{
		openchannels[player] =true;								//open comms to this player.
	}

	gameInit();
	msgStackReset();	//for multiplayer msgs, reset message stack

	return true;
}

////////////////////////////////
// at the end of every game.
bool multiGameShutdown(void)
{
	PLAYERSTATS	st;
	uint32_t        time;

	debug(LOG_NET,"%s is shutting down.",getPlayerName(selectedPlayer));

	sendLeavingMsg();							// say goodbye
	updateMultiStatsGames();					// update games played.

	st = getMultiStats(selectedPlayer);	// save stats

	saveMultiStats(getPlayerName(selectedPlayer), getPlayerName(selectedPlayer), &st);

	// if we terminate the socket too quickly, then, it is possible not to get the leave message
	time = wzGetTicks();
	while (wzGetTicks() - time < 1000)
	{
		wzYieldCurrentThread();  // TODO Make a wzDelay() function?
	}
	// close game
	NETclose();
	NETremRedirects();

	if (ingame.numStructureLimits)
	{
		ingame.numStructureLimits = 0;
		free(ingame.pStructureLimits);
		ingame.pStructureLimits = NULL;
	}
	ingame.flags = 0;

	ingame.localJoiningInProgress = false; // Clean up
	ingame.localOptionsReceived = false;
	ingame.bHostSetup = false;	// Dont attempt a host
	ingame.TimeEveryoneIsInGame = 0;
	ingame.startTime = 0;
	NetPlay.isHost					= false;
	bMultiPlayer					= false;	// Back to single player mode
	bMultiMessages					= false;
	selectedPlayer					= 0;		// Back to use player 0 (single player friendly)

	return true;
}
