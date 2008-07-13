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
 * Multijoin.c
 *
 * Alex Lee, pumpkin studios, bath,
 *
 * Stuff to handle the comings and goings of players.
 */
#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/framework/math-help.h"

#include "lib/gamelib/gtime.h"
#include "lib/ivis_common/textdraw.h"
#include "lib/netplay/netplay.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "lib/script/script.h"

#include "multijoin.h"

#include "objmem.h"
#include "statsdef.h"
#include "droiddef.h"
#include "game.h"
#include "projectile.h"
#include "droid.h"
#include "map.h"
#include "power.h"
#include "game.h"					// for loading maps
#include "message.h"				// for clearing game messages
#include "order.h"
#include "console.h"
#include "orderdef.h"				// for droid_order_data
#include "hci.h"
#include "component.h"
#include "research.h"
#include "wrappers.h"
#include "intimage.h"
#include "data.h"
#include "scripttabs.h"

#include "multiplay.h"
#include "multirecv.h"
#include "multiint.h"
#include "multistat.h"
#include "multigifts.h"
#include "scriptcb.h"


// ////////////////////////////////////////////////////////////////////////////
// External Variables
extern BOOL		bHosted;
extern BOOL		multiRequestUp;
// ////////////////////////////////////////////////////////////////////////////
//external functions

// ////////////////////////////////////////////////////////////////////////////
// Local Functions

static void resetMultiVisibility(UDWORD player);

// ////////////////////////////////////////////////////////////////////////////
// Wait For Players

BOOL intDisplayMultiJoiningStatus(UBYTE joinCount)
{
	UDWORD			x,y,w,h;
	char			sTmp[6];

	w = RET_FORMWIDTH;
	h = RET_FORMHEIGHT;
	x = RET_X;
	y = RET_Y;

//	cameraToHome(selectedPlayer);				// home the camera to the player.
	RenderWindowFrame(FRAME_NORMAL, x, y ,w, h);		// draw a wee blu box.

	// display how far done..
	iV_DrawText(_("Players Still Joining"),
					x+(w/2)-(iV_GetTextWidth(_("Players Still Joining"))/2),
					y+(h/2)-8 );
	sprintf(sTmp,"%d%%", PERCENT((NetPlay.playercount-joinCount),NetPlay.playercount) );
	iV_DrawText(sTmp ,x + (w / 2) - 10, y + (h / 2) + 10);

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// when a remote player leaves an arena game do this!
void clearPlayer(UDWORD player,BOOL quietly,BOOL removeOil)
{
	UDWORD			i;
	BOOL			bTemp;
	STRUCTURE		*psStruct,*psNext;

	player2dpid[player] = 0;					// remove player, make computer controlled
	ingame.JoiningInProgress[player] = false;	// if they never joined, reset the flag

	(void)setPlayerName(player,"");				//clear custom player name (will use default instead)

	for(i = 0;i<MAX_PLAYERS;i++)				// remove alliances
	{
		alliances[player][i]	= ALLIANCE_BROKEN;
		alliances[i][player]	= ALLIANCE_BROKEN;
	}

	debug(LOG_DEATH, "clearPlayer: killing off all droids");
	while(apsDroidLists[player])				// delete all droids
	{
		if(quietly)
		{
			killDroid(apsDroidLists[player]);
		}else{
			destroyDroid(apsDroidLists[player]);
		}
	}

	psStruct = apsStructLists[player];
	while(psStruct)				// delete all structs
	{
		psNext = psStruct->psNext;
		bTemp = false;

		if(removeOil)
		{
			if (psStruct->pStructureType->type == REF_RESOURCE_EXTRACTOR)
			{
				bTemp =  true;
			}
		}

		if(quietly)
		{
			removeStruct(psStruct, true);
		}
		else
		{
			if(	(psStruct->pStructureType->type != REF_WALL &&
				 psStruct->pStructureType->type != REF_WALLCORNER ) )
			{
				destroyStruct(psStruct);
			}
		}

		if(bTemp)
		{
			if(apsFeatureLists[0]->psStats->subType == FEAT_OIL_RESOURCE)
			{
				removeFeature(apsFeatureLists[0]);
			}
		}
		psStruct = psNext;
	}

	return;
}

// Reset visibilty, so a new player can't see the old stuff!!
static void resetMultiVisibility(UDWORD player)
{
	UDWORD		owned;
	DROID		*pDroid;
	STRUCTURE	*pStruct;

	for(owned = 0 ; owned <MAX_PLAYERS ;owned++)		// for each player
	{
		if(owned != player)								// done reset own stuff..
		{
			//droids
			for(pDroid = apsDroidLists[owned];pDroid;pDroid=pDroid->psNext)
			{
				pDroid->visible[player] = false;
			}

			//structures
			for(pStruct= apsStructLists[owned];pStruct;pStruct=pStruct->psNext)
			{
				pStruct->visible[player] = false;
			}

		}
	}
	return;
}

// ////////////////////////////////////////////////////////////////////////////
// A remote player has left the game
BOOL MultiPlayerLeave( UDWORD dp)
{
	unsigned int i = 0;
	char	buf[255];

	while((player2dpid[i] != dp) && (i<MAX_PLAYERS) )i++;	// find out which!

	debug(LOG_NET, "Player %u is leaving (dpid=%u)", i, dp);

	if(i != MAX_PLAYERS)									// player not already removed
	{
		NETlogEntry("Player Unexpectedly leaving, came from directplay...",0,dp);

		sprintf( buf,_("%s has Left the Game"),getPlayerName(i) );

		turnOffMultiMsg(true);
		clearPlayer(i,false,false);
		game.skDiff[dp-1] = (DIFF_SLIDER_STOPS / 2);

		turnOffMultiMsg(false);

		addConsoleMessage(buf,DEFAULT_JUSTIFY, SYSTEM_MESSAGE);

		if(widgGetFromID(psWScreen,IDRET_FORM))
		{
			audio_QueueTrack( ID_CLAN_EXIT );
		}
	}

	NETplayerInfo();									// update the player info stuff


	// fire script callback to reassign skirmish players.
	CBPlayerLeft = i;
	eventFireCallbackTrigger((TRIGGER_TYPE)CALL_PLAYERLEFT);


	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// A Remote Player has joined the game.
BOOL MultiPlayerJoin(UDWORD dpid)
{
	UDWORD i;

	if(widgGetFromID(psWScreen,IDRET_FORM))	// if ingame.
	{
		audio_QueueTrack( ID_CLAN_ENTER );
	}

	if(widgGetFromID(psWScreen,MULTIOP_PLAYERS))	// if in multimenu.
	{
		if (!multiRequestUp && (bHosted || ingame.localJoiningInProgress))
		{
			addPlayerBox(true);	// update the player box.
		}
	}

	if(NetPlay.bHost)		// host responsible for welcoming this player.
	{
		// if we've already received a request from this player don't reallocate.
		for(i=0; (i<MAX_PLAYERS) ;i++)
		{
			if((player2dpid[i] == dpid) && ingame.JoiningInProgress[i] )
			{
				return true;
			}
		}
		ASSERT( NetPlay.playercount<=MAX_PLAYERS,"Too many players!" );

		// setup data for this player, then broadcast it to the other players.
		do
		{
			// Randomly allocate a player to this new machine
			i = rand() % game.maxPlayers;
		} while (player2dpid[i] != 0);

		setPlayerColour(i,MAX_PLAYERS);				// force a colourchoice
		chooseColour(i);							// pick an unused colour.

		setupNewPlayer(dpid,i);						// setup all the guff for that player.
		sendOptions(dpid,i);

		bPlayerReadyGUI[dpid] = false;

		// if skirmish and game full, then kick...
		if(game.type == SKIRMISH && NetPlay.playercount > game.maxPlayers )
		{
			kickPlayer(dpid);
		}

	}
	return true;
}


// ////////////////////////////////////////////////////////////////////////////
// Setup Stuff for a new player.
void setupNewPlayer(UDWORD dpid, UDWORD player)
{
	UDWORD i;//,col;
	char buf[255];

	player2dpid[player] = dpid;							// assign them a player too.
	ingame.PingTimes[player] =0;						// reset ping time
	ingame.JoiningInProgress[player] = true;			// note that player is now joining.

	for(i=0;i<MAX_PLAYERS;i++)							// set all alliances to broken.
	{
		alliances[selectedPlayer][i] = ALLIANCE_BROKEN;
		alliances[i][selectedPlayer] = ALLIANCE_BROKEN;
	}

	resetMultiVisibility(player);						// set visibility flags.
	NETplayerInfo();								// update the net info stuff

	setMultiStats(player2dpid[player],getMultiStats(player,false),true);  // get the players score from the ether.

	sprintf( buf,_("%s is Joining the Game"),getPlayerName(player) );
	addConsoleMessage(buf,DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
}

// ////////////////////////////////////////////////////////////////////////////
// reduce the amount of oil that can be extracted.
void modifyResources(POWER_GEN_FUNCTION* psFunction)
{
	switch(game.power)
	{
	case LEV_LOW:
		psFunction->powerMultiplier = psFunction->powerMultiplier * 3/4 ;	// careful with the brackets! (do mul before div)
		break;
	case LEV_MED:
		psFunction->powerMultiplier = psFunction->powerMultiplier * 1;
		break;
	case LEV_HI:
		psFunction->powerMultiplier = psFunction->powerMultiplier * 5/4  ;
		break;
	default:
		break;
	}
	return;
}
