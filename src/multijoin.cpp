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
 * Multijoin.c
 *
 * Alex Lee, pumpkin studios, bath,
 *
 * Stuff to handle the comings and goings of players.
 */

#include <physfs.h>

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/framework/math_ext.h"

#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/textdraw.h"
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

#include "multimenu.h"
#include "multiplay.h"
#include "multirecv.h"
#include "multiint.h"
#include "multistat.h"
#include "multigifts.h"
#include "qtscript.h"
#include "scriptcb.h"

// ////////////////////////////////////////////////////////////////////////////
// Local Functions

static void resetMultiVisibility(UDWORD player);

// ////////////////////////////////////////////////////////////////////////////
// Wait For Players

bool intDisplayMultiJoiningStatus(UBYTE joinCount)
{
	UDWORD			x, y, w, h;
	char			sTmp[6];

	w = RET_FORMWIDTH;
	h = RET_FORMHEIGHT;
	x = RET_X;
	y = RET_Y;

//	cameraToHome(selectedPlayer);				// home the camera to the player.
	RenderWindowFrame(FRAME_NORMAL, x, y , w, h);		// draw a wee blu box.

	// display how far done..
	iV_SetFont(font_regular);
	iV_DrawText(_("Players Still Joining"),
	            x + (w / 2) - (iV_GetTextWidth(_("Players Still Joining")) / 2),
	            y + (h / 2) - 8);
	unsigned playerCount = 0;  // Calculate what NetPlay.playercount should be, which is apparently only non-zero for the host.
	for (unsigned player = 0; player < game.maxPlayers; ++player)
	{
		if (isHumanPlayer(player))
		{
			++playerCount;
		}
	}
	if (!playerCount)
	{
		return true;
	}
	iV_SetFont(font_large);
	sprintf(sTmp, "%d%%", PERCENT(playerCount - joinCount, playerCount));
	iV_DrawText(sTmp , x + (w / 2) - 10, y + (h / 2) + 10);

	iV_SetFont(font_small);
	int yStep = iV_GetTextLineSize();
	int yPos = RET_Y - yStep * game.maxPlayers;

	static const std::string statusStrings[3] = {"☐ ", "☑ ", "☒ "};

	for (unsigned player = 0; player < game.maxPlayers; ++player)
	{
		int status = -1;
		if (isHumanPlayer(player))
		{
			status = ingame.JoiningInProgress[player] ? 0 : 1; // Human player, still joining or joined.
		}
		else if (NetPlay.players[player].ai >= 0)
		{
			status = 2;  // AI player (automatically joined).
		}
		if (status >= 0)
		{
			iV_DrawText((statusStrings[status] + getPlayerName(player)).c_str(), x + 5, yPos + yStep * NetPlay.players[player].position);
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////
/*
** when a remote player leaves an arena game do this!
**
** @param player -- the one we need to clear
** @param quietly -- true means without any visible effects
*/
void clearPlayer(UDWORD player, bool quietly)
{
	UDWORD			i;
	STRUCTURE		*psStruct, *psNext;

	debug(LOG_NET, "R.I.P. %s (%u). quietly is %s", getPlayerName(player), player, quietly ? "true" : "false");

	ingame.JoiningInProgress[player] = false;	// if they never joined, reset the flag
	ingame.DataIntegrity[player] = false;

	(void)setPlayerName(player, "");				//clear custom player name (will use default instead)

	for (i = 0; i < MAX_PLAYERS; i++)				// remove alliances
	{
		alliances[player][i]	= ALLIANCE_BROKEN;
		alliances[i][player]	= ALLIANCE_BROKEN;
	}

	debug(LOG_DEATH, "killing off all droids for player %d", player);
	while (apsDroidLists[player])				// delete all droids
	{
		if (quietly)			// don't show effects
		{
			killDroid(apsDroidLists[player]);
		}
		else				// show effects
		{
			destroyDroid(apsDroidLists[player], gameTime);
		}
	}

	debug(LOG_DEATH, "killing off all structures for player %d", player);
	psStruct = apsStructLists[player];
	while (psStruct)				// delete all structs
	{
		psNext = psStruct->psNext;

		// FIXME: look why destroyStruct() doesn't put back the feature like removeStruct() does
		if (quietly || psStruct->pStructureType->type == REF_RESOURCE_EXTRACTOR)		// don't show effects
		{
			removeStruct(psStruct, true);
		}
		else			// show effects
		{
			destroyStruct(psStruct, gameTime);
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

	for (owned = 0 ; owned < MAX_PLAYERS ; owned++)		// for each player
	{
		if (owned != player)								// done reset own stuff..
		{
			//droids
			for (pDroid = apsDroidLists[owned]; pDroid; pDroid = pDroid->psNext)
			{
				pDroid->visible[player] = false;
			}

			//structures
			for (pStruct = apsStructLists[owned]; pStruct; pStruct = pStruct->psNext)
			{
				pStruct->visible[player] = false;
			}

		}
	}
	return;
}

static void sendPlayerLeft(uint32_t playerIndex)
{
	ASSERT(NetPlay.isHost, "Only host should call this.");

	uint32_t forcedPlayerIndex = whosResponsible(playerIndex);
	NETQUEUE(*netQueueType)(unsigned) = forcedPlayerIndex != selectedPlayer ? NETgameQueueForced : NETgameQueue;
	NETbeginEncode(netQueueType(forcedPlayerIndex), GAME_PLAYER_LEFT);
	NETuint32_t(&playerIndex);
	NETend();
}

void recvPlayerLeft(NETQUEUE queue)
{
	uint32_t playerIndex = 0;
	NETbeginDecode(queue, GAME_PLAYER_LEFT);
	NETuint32_t(&playerIndex);
	NETend();
	if (whosResponsible(playerIndex) != queue.index)
	{
		return;
	}

	turnOffMultiMsg(true);
	clearPlayer(playerIndex, false);  // don't do it quietly
	turnOffMultiMsg(false);
	NetPlay.players[playerIndex].allocated = false;

	char buf[256];
	ssprintf(buf, _("%s has Left the Game"), getPlayerName(playerIndex));
	addConsoleMessage(buf, DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	NETsetPlayerConnectionStatus(CONNECTIONSTATUS_PLAYER_DROPPED, playerIndex);

	debug(LOG_INFO, "** player %u has dropped, in-game!", playerIndex);
}

// ////////////////////////////////////////////////////////////////////////////
// A remote player has left the game
bool MultiPlayerLeave(UDWORD playerIndex)
{
	if (playerIndex >= MAX_PLAYERS)
	{
		ASSERT(false, "Bad player number");
		return false;
	}

	NETlogEntry("Player leaving game", SYNC_FLAG, playerIndex);
	debug(LOG_NET, "** Player %u [%s], has left the game at game time %u.", playerIndex, getPlayerName(playerIndex), gameTime);

	if (ingame.localJoiningInProgress)
	{
		clearPlayer(playerIndex, false);
	}
	else if (NetPlay.isHost)  // If hosting, and game has started (not in pre-game lobby screen, that is).
	{
		sendPlayerLeft(playerIndex);
	}
	game.skDiff[playerIndex] = 0;

	if (NetPlay.players[playerIndex].wzFile.isSending)
	{
		char buf[256];

		ssprintf(buf, _("File transfer has been aborted for %d.") , playerIndex);
		addConsoleMessage(buf, DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
		debug(LOG_INFO, "=== File has been aborted for %d ===", playerIndex);
		NetPlay.players[playerIndex].wzFile.isSending = false;
		NetPlay.players[playerIndex].needFile = false;
	}

	if (widgGetFromID(psWScreen, IDRET_FORM))
	{
		audio_QueueTrack(ID_CLAN_EXIT);
	}

	// fire script callback to reassign skirmish players.
	CBPlayerLeft = playerIndex;
	eventFireCallbackTrigger((TRIGGER_TYPE)CALL_PLAYERLEFT);
	triggerEventPlayerLeft(playerIndex);

	netPlayersUpdated = true;
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// A Remote Player has joined the game.
bool MultiPlayerJoin(UDWORD playerIndex)
{
	if (widgGetFromID(psWScreen, IDRET_FORM))	// if ingame.
	{
		audio_QueueTrack(ID_CLAN_ENTER);
	}

	if (widgGetFromID(psWScreen, MULTIOP_PLAYERS))	// if in multimenu.
	{
		if (!multiRequestUp && (bHosted || ingame.localJoiningInProgress))
		{
			addPlayerBox(true);	// update the player box.
		}
	}

	if (NetPlay.isHost)		// host responsible for welcoming this player.
	{
		// if we've already received a request from this player don't reallocate.
		if (ingame.JoiningInProgress[playerIndex])
		{
			return true;
		}
		ASSERT(NetPlay.playercount <= MAX_PLAYERS, "Too many players!");

		// setup data for this player, then broadcast it to the other players.
		setupNewPlayer(playerIndex);						// setup all the guff for that player.
		if (bHosted)
		{
			sendOptions();
		}
		// if skirmish and game full, then kick...
		if (NetPlay.playercount > game.maxPlayers)
		{
			kickPlayer(playerIndex, "the game is already full.", ERROR_FULL);
		}
		// send everyone's stats to the new guy
		{
			int i;

			for (i = 0; i < MAX_PLAYERS; i++)
			{
				if (NetPlay.players[i].allocated)
				{
					setMultiStats(i, getMultiStats(i), false);
				}
			}
		}
	}
	return true;
}

bool sendDataCheck(void)
{
	int i = 0;

	NETbeginEncode(NETnetQueue(NET_HOST_ONLY), NET_DATA_CHECK);		// only need to send to HOST
	for (i = 0; i < DATA_MAXDATA; i++)
	{
		NETuint32_t(&DataHash[i]);
	}
	NETend();
	debug(LOG_NET, "sent hash to host");
	return true;
}

bool recvDataCheck(NETQUEUE queue)
{
	int i = 0;
	uint32_t player = queue.index;
	uint32_t tempBuffer[DATA_MAXDATA] = {0};

	if (!NetPlay.isHost)				// only host should act
	{
		ASSERT(false, "Host only routine detected for client!");
		return false;
	}

	NETbeginDecode(queue, NET_DATA_CHECK);
	for (i = 0; i < DATA_MAXDATA; i++)
	{
		NETuint32_t(&tempBuffer[i]);
	}
	NETend();

	if (player >= MAX_PLAYERS) // invalid player number.
	{
		debug(LOG_ERROR, "invalid player number (%u) detected.", player);
		return false;
	}

	if (whosResponsible(player) != queue.index)
	{
		HandleBadParam("NET_DATA_CHECK given incorrect params.", player, queue.index);
		return false;
	}

	debug(LOG_NET, "** Received NET_DATA_CHECK from player %u", player);

	if (NetPlay.isHost)
	{
		if (memcmp(DataHash, tempBuffer, sizeof(DataHash)))
		{
			char msg[256] = {'\0'};

			for (i = 0; i < DATA_MAXDATA; i++)
			{
				if (DataHash[i] != tempBuffer[i])
				{
					break;
				}
			}

			sprintf(msg, _("%s (%u) has an incompatible mod, and has been kicked."), getPlayerName(player), player);
			sendTextMessage(msg, true);
			addConsoleMessage(msg, LEFT_JUSTIFY, NOTIFY_MESSAGE);

			kickPlayer(player, "your data doesn't match the host's!", ERROR_WRONGDATA);
			debug(LOG_WARNING, "%s (%u) has an incompatible mod. ([%d] got %x, expected %x)", getPlayerName(player), player, i, tempBuffer[i], DataHash[i]);
			debug(LOG_POPUP, "%s (%u), has an incompatible mod. ([%d] got %x, expected %x)", getPlayerName(player), player, i, tempBuffer[i], DataHash[i]);

			return false;
		}
		else
		{
			debug(LOG_NET, "DataCheck message received and verified for player %s (slot=%u)", getPlayerName(player), player);
			ingame.DataIntegrity[player] = true;
		}
	}
	return true;
}
// ////////////////////////////////////////////////////////////////////////////
// Setup Stuff for a new player.
void setupNewPlayer(UDWORD player)
{
	UDWORD i;

	ingame.PingTimes[player] = 0;					// Reset ping time
	ingame.JoiningInProgress[player] = true;			// Note that player is now joining
	ingame.DataIntegrity[player] = false;

	for (i = 0; i < MAX_PLAYERS; i++)				// Set all alliances to broken
	{
		alliances[selectedPlayer][i] = ALLIANCE_BROKEN;
		alliances[i][selectedPlayer] = ALLIANCE_BROKEN;
	}

	resetMultiVisibility(player);						// set visibility flags.

	setMultiStats(player, getMultiStats(player), true);  // get the players score

	if (selectedPlayer != player)
	{
		char buf[255];
		ssprintf(buf, _("%s is joining the game"), getPlayerName(player));
		addConsoleMessage(buf, DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	}
}


// While not the perfect place for this, it has to do when a HOST joins (hosts) game
// unfortunatly, we don't get the message until after the setup is done.
void ShowMOTD(void)
{
	char buf[250] = { '\0' };
	// when HOST joins the game, show server MOTD message first
	addConsoleMessage(_("Server message:"), DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
	addConsoleMessage(NetPlay.MOTD, DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
	if (NetPlay.HaveUpgrade)
	{
		audio_PlayTrack(ID_SOUND_BUILD_FAIL);
		ssprintf(buf, _("There is an update to the game, please visit http://wz2100.net to download new version."));
		addConsoleMessage(buf, DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
	}
	else
	{
		audio_PlayTrack(FE_AUDIO_MESSAGEEND);
	}

}
