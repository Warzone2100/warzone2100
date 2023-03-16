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
 * Multiplay.c
 *
 * Alex Lee, Sep97, Pumpkin Studios
 *
 * Contains the day to day networking stuff, and received message handler.
 */
#include <string.h>
#include <algorithm>
#include <chrono>

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/framework/strres.h"
#include "lib/framework/physfs_ext.h"
#include "lib/ivis_opengl/piepalette.h" // for pal_Init()
#include "map.h"

#include "game.h"									// for loading maps
#include "hci.h"

#include <time.h>									// for recording ping times.
#include "research.h"
#include "display3d.h"								// for changing the viewpoint
#include "console.h"								// for screen messages
#include "clparse.h"
#include "data.h"
#include "power.h"
#include "cmddroid.h"								//  for commanddroidupdatekills
#include "wrappers.h"								// for game over
#include "component.h"
#include "frontend.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "levels.h"
#include "selection.h"
#include "research.h"
#include "init.h"
#include "warcam.h"	// these 4 for fireworks
#include "mission.h"
#include "effects.h"
#include "lib/gamelib/gtime.h"
#include "keybind.h"
#include "qtscript.h"
#include "design.h"
#include "advvis.h"
#include "lighting.h" // for reInitPaletteAndFog()

#include "template.h"
#include "lib/netplay/netplay.h"								// the netplay library.
#include "modding.h"
#include "multiplay.h"								// warzone net stuff.
#include "multijoin.h"								// player management stuff.
#include "multirecv.h"								// incoming messages stuff
#include "multistat.h"
#include "multigifts.h"								// gifts and alliances.
#include "multiint.h"
#include "cheat.h"
#include "main.h"								// for gamemode
#include "multiint.h"
#include "activity.h"
#include "lib/framework/wztime.h"
#include "chat.h" // for InGameChatMessage
#include "warzoneconfig.h"
#include "stdinreader.h"
#include "spectatorwidgets.h"
#include "challenge.h"

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// globals.
bool						bMultiPlayer				= false;	// true when more than 1 player.
bool						bMultiMessages				= false;	// == bMultiPlayer unless multimessages are disabled
bool						openchannels[MAX_CONNECTED_PLAYERS] = {true};
UBYTE						bDisplayMultiJoiningStatus;

MULTIPLAYERGAME				game;									//info to describe game.
MULTIPLAYERINGAME			ingame;

char						beaconReceiveMsg[MAX_PLAYERS][MAX_CONSOLE_STRING_LENGTH];	//beacon msg for each player

#define DATACHECK2_INTERVAL_MS 10000

// ////////////////////////////////////////////////////////////////////////////
// Local Prototypes

static bool recvBeacon(NETQUEUE queue);
static bool recvResearch(NETQUEUE queue);
static bool sendDataCheck2();

void startMultiplayerGame();

// ////////////////////////////////////////////////////////////////////////////
// Auto Lag Kick Handling

#define LAG_INITIAL_LOAD_GRACEPERIOD 60
#define LAG_CHECK_INTERVAL 1000
const std::chrono::milliseconds LagCheckInterval(LAG_CHECK_INTERVAL);

static void sendTextMessage(const char* msg)
{
	auto message = InGameChatMessage(selectedPlayer, msg);
	message.send();
}

static void autoLagKickRoutine()
{
	if (!NetPlay.isHost)
	{
		return;
	}

	int LagAutoKickSeconds = war_getAutoLagKickSeconds();
	if (LagAutoKickSeconds <= 0)
	{
		return;
	}

	const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	if (std::chrono::duration_cast<std::chrono::milliseconds>(now - ingame.lastLagCheck) < LagCheckInterval)
	{
		return;
	}

	ingame.lastLagCheck = now;
	uint32_t playerCheckLimit = (!ingame.TimeEveryoneIsInGame.has_value()) ? MAX_CONNECTED_PLAYERS : MAX_PLAYERS;
	for (uint32_t i = 0; i < playerCheckLimit; ++i)
	{
		if (!isHumanPlayer(i))
		{
			continue;
		}
		if (i > MAX_PLAYERS && !gtimeShouldWaitForPlayer(i))
		{
			continue;
		}
		bool isLagging = (ingame.PingTimes[i] >= PING_LIMIT);
		bool isWaitingForInitialLoad = !ingame.TimeEveryoneIsInGame.has_value() && ingame.JoiningInProgress[i];
		if (isWaitingForInitialLoad && (std::chrono::duration_cast<std::chrono::seconds>(now - ingame.startTime) < std::chrono::seconds(LAG_INITIAL_LOAD_GRACEPERIOD)))
		{
			isLagging = false;
			isWaitingForInitialLoad = false;
		}
		if (!isLagging && !isWaitingForInitialLoad)
		{
			if(ingame.LagCounter[i] > 0)
			{
				ingame.LagCounter[i]--;
			}
			continue;
		}

		ingame.LagCounter[i]++;
		if (ingame.LagCounter[i] >= LagAutoKickSeconds) {
			std::string msg = astringf("Auto-kicking player %" PRIu32 " (\"%s\") because of ping issues. (Timeout: %u seconds)", i, getPlayerName(i), LagAutoKickSeconds);
			debug(LOG_INFO, "%s", msg.c_str());
			sendTextMessage(msg.c_str());
			wz_command_interface_output("WZEVENT: lag-kick: %u %s\n", i, NetPlay.players[i].IPtextAddress);
			kickPlayer(i, "Your connection was too laggy.", ERROR_CONNECTION, false);
			ingame.LagCounter[i] = 0;
		}
		else if (ingame.LagCounter[i] >= (LagAutoKickSeconds - 3)) {
			std::string msg = astringf("Auto-kicking player %" PRIu32 " (\"%s\") in %u seconds.", i, getPlayerName(i), (LagAutoKickSeconds - ingame.LagCounter[i]));
			debug(LOG_INFO, "%s", msg.c_str());
			sendTextMessage(msg.c_str());
		}
		else if (ingame.LagCounter[i] % 15 == 0) { // every 15 seconds
			std::string msg = astringf("Auto-kicking player %" PRIu32 " (\"%s\") in %u seconds.", i, getPlayerName(i), (LagAutoKickSeconds - ingame.LagCounter[i]));
			debug(LOG_INFO, "%s", msg.c_str());
			sendTextMessage(msg.c_str());
		}
	}
}

// ////////////////////////////////////////////////////////////////////////////
// temporarily disable multiplayer mode.
void turnOffMultiMsg(bool bDoit)
{
	if (!bMultiPlayer)
	{
		return;
	}

	bMultiMessages = !bDoit;
	return;
}


// ////////////////////////////////////////////////////////////////////////////
// throw a party when you win!
bool multiplayerWinSequence(bool firstCall)
{
	static Position pos = Position(0, 0, 0);
	static UDWORD last = 0;
	float		rotAmount;
	STRUCTURE	*psStruct;

	if (selectedPlayer >= MAX_PLAYERS)
	{
		return false;
	}

	if (firstCall)
	{
		pos  = cameraToHome(selectedPlayer, true);			// pan the camera to home if not already doing so
		last = 0;

		// stop all research
		CancelAllResearch(selectedPlayer);

		// stop all manufacture.
		for (psStruct = apsStructLists[selectedPlayer]; psStruct; psStruct = psStruct->psNext)
		{
			if (StructIsFactory(psStruct))
			{
				if (((FACTORY *)psStruct->pFunctionality)->psSubject)//check if active
				{
					cancelProduction(psStruct, ModeQueue);
				}
			}
		}
	}

	// rotate world
	if (MissionResUp && !getWarCamStatus())
	{
		rotAmount = graphicsTimeAdjustedIncrement(MAP_SPIN_RATE / 12);
		playerPos.r.y = static_cast<int>(playerPos.r.y + rotAmount);
	}

	if (last > gameTime)
	{
		last = 0;
	}
	if ((gameTime - last) < 500)							// only  if not done recently.
	{
		return true;
	}
	last = gameTime;

	if (rand() % 3 == 0)
	{
		Position pos2 = pos;
		pos2.x += (rand() % world_coord(8)) - world_coord(4);
		pos2.z += (rand() % world_coord(8)) - world_coord(4);

		if (pos2.x < 0)
		{
			pos2.x = 128;
		}

		if ((unsigned)pos2.x > world_coord(mapWidth))
		{
			pos2.x = world_coord(mapWidth);
		}

		if (pos2.z < 0)
		{
			pos2.z = 128;
		}

		if ((unsigned)pos2.z > world_coord(mapHeight))
		{
			pos2.z = world_coord(mapHeight);
		}

		addEffect(&pos2, EFFECT_FIREWORK, FIREWORK_TYPE_LAUNCHER, false, nullptr, 0);	// throw up some fire works.
	}

	// show the score..


	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// MultiPlayer main game loop code.
bool multiPlayerLoop()
{
	UDWORD		i;
	UBYTE		joinCount;

	joinCount = 0;
	for (i = 0; i < MAX_CONNECTED_PLAYERS; i++)
	{
		if (isHumanPlayer(i) && ingame.JoiningInProgress[i])
		{
			joinCount++;
		}
	}
	if (joinCount)
	{
		setWidgetsStatus(false);
		bDisplayMultiJoiningStatus = joinCount;	// someone is still joining! say So

		// deselect anything selected.
		selDroidDeselect(selectedPlayer);

		if (keyPressed(KEY_ESC))// check for cancel
		{
			bDisplayMultiJoiningStatus = 0;
			clearDisplayMultiJoiningStatusCache();
			setWidgetsStatus(true);
			setPlayerHasLost(true);
		}
	}
	else		//everyone is in the game now!
	{
		if (bDisplayMultiJoiningStatus)
		{
			bDisplayMultiJoiningStatus = 0;
			clearDisplayMultiJoiningStatusCache();
			setWidgetsStatus(true);
		}
		if (!ingame.TimeEveryoneIsInGame.has_value())
		{
			ingame.TimeEveryoneIsInGame = gameTime;
			debug(LOG_NET, "I have entered the game @ %d", ingame.TimeEveryoneIsInGame.value());
			if (!NetPlay.isHost)
			{
				debug(LOG_NET, "=== Sending hash to host ===");
				sendDataCheck();
			}
			ingame.lastPlayerDataCheck2 = std::chrono::steady_clock::now();
		}
		if (NetPlay.bComms)
		{
			sendPing();
		}
		if (NetPlay.isHost && NetPlay.bComms)
		{
			sendDataCheck2();
		}
		// Only have to do this on a true MP game
		if (NetPlay.isHost && !ingame.isAllPlayersDataOK && NetPlay.bComms)
		{
			if (gameTime - ingame.TimeEveryoneIsInGame.value() > GAME_TICKS_PER_SEC * 60)
			{
				// we waited 60 secs to make sure people didn't bypass the data integrity checks
				int index;
				for (index = 0; index < MAX_CONNECTED_PLAYERS; index++)
				{
					if (ingame.DataIntegrity[index] == false && isHumanPlayer(index) && index != NetPlay.hostPlayer)
					{
						char msg[256] = {'\0'};

						snprintf(msg, sizeof(msg), _("Kicking player %s, because they tried to bypass data integrity check!"), getPlayerName(index));
						sendInGameSystemMessage(msg);
						addConsoleMessage(msg, LEFT_JUSTIFY, NOTIFY_MESSAGE);
						NETlogEntry(msg, SYNC_FLAG, index);

#ifndef DEBUG
						kickPlayer(index, _("Invalid data!"), ERROR_INVALID, false);
#endif
						debug(LOG_WARNING, "Kicking Player %s (%u), they tried to bypass data integrity check!", getPlayerName(index), index);
					}
				}
				ingame.isAllPlayersDataOK = true;
			}
		}
	}

	if (NetPlay.isHost)
	{
		autoLagKickRoutine();
	}

	// if player has won then process the win effects...
	if (testPlayerHasWon())
	{
		multiplayerWinSequence(false);
	}
	return true;
}


// ////////////////////////////////////////////////////////////////////////////
// quikie functions.

// to get droids ...
DROID *IdToDroid(UDWORD id, UDWORD player)
{
	if (player == ANYPLAYER)
	{
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			for (DROID *d = apsDroidLists[i]; d; d = d->psNext)
			{
				if (d->id == id)
				{
					return d;
				}
			}
		}
	}
	else if (player < MAX_PLAYERS)
	{
		for (DROID *d = apsDroidLists[player]; d; d = d->psNext)
		{
			if (d->id == id)
			{
				return d;
			}
		}
	}
	return nullptr;
}

// find off-world droids
DROID *IdToMissionDroid(UDWORD id, UDWORD player)
{
	if (player == ANYPLAYER)
	{
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			for (DROID *d = mission.apsDroidLists[i]; d; d = d->psNext)
			{
				if (d->id == id)
				{
					return d;
				}
			}
		}
	}
	else if (player < MAX_PLAYERS)
	{
		for (DROID *d = mission.apsDroidLists[player]; d; d = d->psNext)
		{
			if (d->id == id)
			{
				return d;
			}
		}
	}
	return nullptr;
}

static STRUCTURE* _IdToStruct(UDWORD id, UDWORD beginPlayer, UDWORD endPlayer)
{
	STRUCTURE **lists[2] = {apsStructLists, mission.apsStructLists};
	for (int j = 0; j < 2; ++j)
	{
		for (int i = beginPlayer; i < endPlayer; ++i)
		{
			for (STRUCTURE *d = lists[j][i]; d; d = d->psNext)
			{
				if (d->id == id)
				{
					return d;
				}
			}
		}
	}
	return nullptr;
}
// ////////////////////////////////////////////////////////////////////////////
// find a structure
STRUCTURE *IdToStruct(UDWORD id, UDWORD player)
{
	int beginPlayer = 0, endPlayer = MAX_PLAYERS;
	if (player != ANYPLAYER)
	{
		beginPlayer = player;
		endPlayer = std::min<int>(player + 1, MAX_PLAYERS);
	}
	STRUCTURE *out = nullptr;
	out = _IdToStruct(id, beginPlayer, endPlayer);
	if (out)
	{
		return out;
	}
	return nullptr;
}

// ////////////////////////////////////////////////////////////////////////////
// find a feature
FEATURE *IdToFeature(UDWORD id, UDWORD player)
{
	(void)player;	// unused, all features go into player 0
	for (FEATURE *d = apsFeatureLists[0]; d; d = d->psNext)
	{
		if (d->id == id)
		{
			return d;
		}
	}
	return nullptr;
}

// ////////////////////////////////////////////////////////////////////////////

DROID_TEMPLATE *IdToTemplate(UDWORD tempId, UDWORD player)
{
	// Check if we know which player this is from, in that case, assume it is a player template
	// FIXME: nuke the ANYPLAYER hack
	if (player != ANYPLAYER && player < MAX_PLAYERS)
	{
		return findPlayerTemplateById(player, tempId);
	}

	// It could be a AI template...or that of another player
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		auto psTempl = findPlayerTemplateById(i, tempId);
		if (psTempl)
		{
			return psTempl;
		}
	}

	// no error, since it is possible that we don't have this template defined yet.
	return nullptr;
}

/////////////////////////////////////////////////////////////////////////////////
//  Returns a pointer to base object, given an id and optionally a player.
BASE_OBJECT *IdToPointer(UDWORD id, UDWORD player)
{
	DROID		*pD;
	STRUCTURE	*pS;
	FEATURE		*pF;
	// droids.

	pD = IdToDroid(id, player);
	if (pD)
	{
		return (BASE_OBJECT *)pD;
	}

	// structures
	pS = IdToStruct(id, player);
	if (pS)
	{
		return (BASE_OBJECT *)pS;
	}

	// features
	pF = IdToFeature(id, player);
	if (pF)
	{
		return (BASE_OBJECT *)pF;
	}

	return nullptr;
}


// ////////////////////////////////////////////////////////////////////////////
// return a players name.
const char *getPlayerName(int player)
{
	ASSERT_OR_RETURN(nullptr, player >= 0, "Wrong player index: %d", player);

	const bool aiPlayer = (static_cast<size_t>(player) < NetPlay.players.size()) && (NetPlay.players[player].ai >= 0) && !NetPlay.players[player].allocated;

	if (aiPlayer && GetGameMode() == GS_NORMAL && !challengeActive)
	{
		ASSERT_OR_RETURN("", player < MAX_PLAYERS, "invalid player: %d", player);
		static char names[MAX_PLAYERS][StringSize];  // Must be static, since the getPlayerName() return value is used in tool tips... Long live the widget system.
		// Add colour to player name.
		sstrcpy(names[player], getPlayerColourName(player));
		sstrcat(names[player], "-");
		sstrcat(names[player], NetPlay.players[player].name);
		return names[player];
	}

	if (static_cast<size_t>(player) >= NetPlay.players.size() || strlen(NetPlay.players[player].name) == 0)
	{
		// for campaign and tutorials
		return _("Commander");
	}

	return NetPlay.players[player].name;
}

bool setPlayerName(int player, const char *sName)
{
	ASSERT_OR_RETURN(false, player < MAX_CONNECTED_PLAYERS && player >= 0, "Player index (%u) out of range", player);
	sstrcpy(NetPlay.players[player].name, sName);
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// to determine human/computer players and responsibilities of each.
bool isHumanPlayer(int player)
{
	if (player >= MAX_CONNECTED_PLAYERS || player < 0)
	{
		return false;	// obvious, really
	}
	return NetPlay.players[player].allocated;
}

// Clear player name data after game quit.
void clearPlayerName(unsigned int player)
{
	ASSERT_OR_RETURN(, player < MAX_CONNECTED_PLAYERS, "Player index (%u) out of range", player);
	NetPlay.players[player].name[0] = '\0';
}

// returns player responsible for 'player'
int whosResponsible(int player)
{
	if (isHumanPlayer(player))
	{
		return player;			// Responsible for him or her self
	}
	else if (player == selectedPlayer)
	{
		return player;			// We are responsibly for ourselves
	}
	else
	{
		return NetPlay.hostPlayer;	// host responsible for all AIs
	}
}

//returns true if selected player is responsible for 'player'
bool myResponsibility(int player)
{
	return (whosResponsible(player) == selectedPlayer || whosResponsible(player) == realSelectedPlayer);
}

//returns true if 'player' is responsible for 'playerinquestion'
bool responsibleFor(int player, int playerinquestion)
{
	return whosResponsible(playerinquestion) == player;
}

bool canGiveOrdersFor(int player, int playerInQuestion)
{
	const DebugInputManager& dbgInputManager = gInputManager.debugManager();
	return playerInQuestion >= 0 && playerInQuestion < MAX_PLAYERS &&
	       (player == playerInQuestion || responsibleFor(player, playerInQuestion) || dbgInputManager.debugMappingsAllowed());
}

int scavengerSlot()
{
	// Scavengers used to always be in position 7, when scavengers were only supported in less than 8 player maps.
	// Scavengers should be in position N in N-player maps, where N ≥ 8.
	return MAX(game.maxPlayers, 7);
}

int scavengerPlayer()
{
	return (game.scavengers != NO_SCAVENGERS) ? scavengerSlot() : -1;
}

// ////////////////////////////////////////////////////////////////////////////
// probably temporary. Places the camera on the players 1st droid or struct.
Vector3i cameraToHome(UDWORD player, bool scroll)
{
	UDWORD x, y;
	STRUCTURE	*psBuilding = nullptr;

	if (player < MAX_PLAYERS)
	{
		for (psBuilding = apsStructLists[player]; psBuilding && (psBuilding->pStructureType->type != REF_HQ); psBuilding = psBuilding->psNext) {}
	}

	if (psBuilding)
	{
		x = map_coord(psBuilding->pos.x);
		y = map_coord(psBuilding->pos.y);
	}
	else if ((player < MAX_PLAYERS) && apsDroidLists[player])				// or first droid
	{
		x = map_coord(apsDroidLists[player]->pos.x);
		y =	map_coord(apsDroidLists[player]->pos.y);
	}
	else if ((player < MAX_PLAYERS) && apsStructLists[player])				// center on first struct
	{
		x = map_coord(apsStructLists[player]->pos.x);
		y = map_coord(apsStructLists[player]->pos.y);
	}
	else														//or map center.
	{
		x = mapWidth / 2;
		y = mapHeight / 2;
	}


	if (scroll)
	{
		requestRadarTrack(world_coord(x), world_coord(y));
	}
	else
	{
		setViewPos(x, y, true);
	}

	Vector3i res;
	res.x = world_coord(x);
	res.y = map_TileHeight(x, y);
	res.z = world_coord(y);
	return res;
}

static void recvSyncRequest(NETQUEUE queue)
{
	int32_t req_id, x, y, obj_id, obj_id2, player_id, player_id2;
	BASE_OBJECT *psObj = nullptr, *psObj2 = nullptr;

	NETbeginDecode(queue, GAME_SYNC_REQUEST);
	NETint32_t(&req_id);
	NETint32_t(&x);
	NETint32_t(&y);
	NETint32_t(&obj_id);
	NETint32_t(&player_id);
	NETint32_t(&obj_id2);
	NETint32_t(&player_id2);
	NETend();

	syncDebug("sync request received from%d req_id%d x%u y%u %obj1 %obj2", queue.index, req_id, x, y, obj_id, obj_id2);
	if (obj_id)
	{
		psObj = IdToPointer(obj_id, player_id);
	}
	if (obj_id2)
	{
		psObj2 = IdToPointer(obj_id2, player_id2);
	}
	triggerEventSyncRequest(queue.index, req_id, x, y, psObj, psObj2);
}

static void sendObj(const BASE_OBJECT *psObj)
{
	if (psObj)
	{
		int32_t obj_id = psObj->id;
		int32_t player = psObj->player;
		NETint32_t(&obj_id);
		NETint32_t(&player);
	}
	else
	{
		int32_t dummy = 0;
		NETint32_t(&dummy);
		NETint32_t(&dummy);
	}
}

void sendSyncRequest(int32_t req_id, int32_t x, int32_t y, const BASE_OBJECT *psObj, const BASE_OBJECT *psObj2)
{
	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_SYNC_REQUEST);
	NETint32_t(&req_id);
	NETint32_t(&x);
	NETint32_t(&y);
	sendObj(psObj);
	sendObj(psObj2);
	NETend();
}

static inline std::chrono::seconds maxDataCheck2WaitSeconds()
{
	return std::chrono::seconds(std::max(war_getAutoLagKickSeconds() + 3, 60));
}

static bool sendDataCheck2()
{
	if (NetPlay.isHost)
	{
		const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		if (std::chrono::duration_cast<std::chrono::milliseconds>(now - ingame.lastPlayerDataCheck2) < std::chrono::milliseconds(DATACHECK2_INTERVAL_MS))
		{
			return true;
		}
		// Send a request to all active players
		const auto maxWaitSeconds = maxDataCheck2WaitSeconds();
		for (uint32_t player = 0; player < std::min<uint32_t>(game.maxPlayers, MAX_PLAYERS); ++player)
		{
			if (player == NetPlay.hostPlayer || !isHumanPlayer(player) || NetPlay.players[player].isSpectator)
			{
				continue;
			}

			// Check when the last unanswered request was sent
			if (ingame.lastSentPlayerDataCheck2[player].has_value()
				&& (std::chrono::duration_cast<std::chrono::seconds>(now - ingame.lastSentPlayerDataCheck2[player].value()) >= maxWaitSeconds))
			{
				// If it's after the allowed time, kick the player
				std::string msg = astringf(_("%s (%u) has an incompatible mod, and has been kicked."), getPlayerName(player), player);
				sendInGameSystemMessage(msg.c_str());
				addConsoleMessage(msg.c_str(), LEFT_JUSTIFY, NOTIFY_MESSAGE);

				kickPlayer(player, _("Your data doesn't match the host's!"), ERROR_WRONGDATA, false);
				debug(LOG_INFO, "%s (%u) did not respond with a NET_DATA_CHECK2 within the required timeframe (%s seconds), and has been kicked", getPlayerName(player), player, std::to_string(maxWaitSeconds.count()).c_str());
				ingame.lastSentPlayerDataCheck2[player].reset();
				continue;
			}

			NETbeginEncode(NETnetQueue(player), NET_DATA_CHECK2);
			NETuint32_t(&NetPlay.hostPlayer);
			NETend();
			if (!ingame.lastSentPlayerDataCheck2[player].has_value())
			{
				ingame.lastSentPlayerDataCheck2[player] = now;
			}
		}
		ingame.lastPlayerDataCheck2 = now;
		return true;
	}

	// For a player, respond to the host
	NETbeginEncode(NETnetQueue(NetPlay.hostPlayer), NET_DATA_CHECK2);		// only need to send to HOST
	NETuint32_t(&selectedPlayer);
	NETuint32_t(&realSelectedPlayer);
	std::unordered_map<uint16_t, uint32_t> layers;
	widgForEachOverlayScreen([&layers](const std::shared_ptr<W_SCREEN> &pScreen, uint16_t zOrder) -> bool {
		layers[zOrder]++;
		return true;
	});
	uint32_t layersSize = static_cast<uint32_t>(layers.size());
	NETuint32_t(&layersSize);
	for (auto& layer : layers)
	{
		uint16_t zOrder = layer.first;
		NETuint16_t(&zOrder);
		NETuint32_t(&layer.second);
	}
	for (size_t i = 0; i < DATA_MAXDATA; i++)
	{
		NETuint32_t(&DataHash[i]);
	}
	int8_t aiIndex = NetPlay.players[realSelectedPlayer].ai;
	NETint8_t(&aiIndex);
	bool bValue = godMode;
	NETbool(&bValue);
	NETend();
	return true;
}

static bool recvDataCheck2(NETQUEUE queue)
{
	uint32_t player = queue.index;
	uint32_t recvSelectedPlayer = 0;
	uint32_t recvRealSelectedPlayer = 0;
	std::unordered_map<uint16_t, uint32_t> layers;
	uint32_t tempBuffer[DATA_MAXDATA] = {0};
	int8_t aiIndex = 0;
	bool recvGM;

	if (!NetPlay.isHost) // the host can send NET_DATA_CHECK2 messages to clients to request a check
	{
		ASSERT_OR_RETURN(false, NetPlay.hostPlayer == queue.index, "Non-host player (%u) is sending NET_DATA_CHECK2 to us??", queue.index);
		NETbeginDecode(queue, NET_DATA_CHECK2);
		NETuint32_t(&recvSelectedPlayer);
		NETend();
		ASSERT_OR_RETURN(false, NetPlay.hostPlayer == recvSelectedPlayer, "Non-host player (selectedPlayer: %u) is sending NET_DATA_CHECK2 to us??", recvSelectedPlayer);
		sendDataCheck2();
		return true;
	}

	NETbeginDecode(queue, NET_DATA_CHECK2);
	NETuint32_t(&recvSelectedPlayer);
	NETuint32_t(&recvRealSelectedPlayer);
	uint32_t layersSize = 0;
	uint16_t zOrder = 0;
	uint32_t layerCount = 0;
	NETuint32_t(&layersSize);
	for (uint32_t i = 0; i < layersSize; ++i)
	{
		NETuint16_t(&zOrder);
		NETuint32_t(&layerCount);
		layers[zOrder] = layerCount;
	}
	for (size_t i = 0; i < DATA_MAXDATA; ++i)
	{
		NETuint32_t(&tempBuffer[i]);
	}
	NETint8_t(&aiIndex);
	NETbool(&recvGM);
	NETend();

	if (player >= MAX_CONNECTED_PLAYERS) // invalid player number.
	{
		debug(LOG_ERROR, "invalid player number (%u) detected.", player);
		return false;
	}

	if (whosResponsible(player) != queue.index)
	{
		HandleBadParam("NET_DATA_CHECK2 given incorrect params.", player, queue.index);
		return false;
	}

	if (recvRealSelectedPlayer >= NetPlay.players.size())
	{
		HandleBadParam("NET_DATA_CHECK2 given invalid param.", recvRealSelectedPlayer, queue.index);
		return false;
	}

	if (!isHumanPlayer(player) || NetPlay.players[player].kick)
	{
		// Ignoring
		return false;
	}

	debug(LOG_NET, "** Received NET_DATA_CHECK2 from player %u", player);
	ingame.lastSentPlayerDataCheck2[player].reset();

	bool hasWrongData = false;

	if (!NetPlay.players[player].isSpectator && (recvSelectedPlayer != player || recvRealSelectedPlayer != player))
	{
		debug(LOG_INFO, "%s (%u) has a corrupted player index. (selectedPlayer: %" PRIu32 ", realSelectedPlayer: %" PRIu32 ")", getPlayerName(player), player, recvSelectedPlayer, recvRealSelectedPlayer);
		hasWrongData = true;
	}

	if (layersSize > 1024)
	{
		debug(LOG_INFO, "%s (%u) has a very high layersSize - something is probably wrong. (layersSize: %" PRIu32 ")", getPlayerName(player), player, layersSize);
		hasWrongData = true;
	}

	if (!NetPlay.players[player].isSpectator)
	{
		for (uint16_t zCheck = 65530; zOrder < std::numeric_limits<uint16_t>::max() - 2; ++zOrder)
		{
			auto it = layers.find(zCheck);
			if (it != layers.end())
			{
				debug(LOG_INFO, "%s (%u) has an unexpected display layer. (layer: %" PRIu16 ", count: %" PRIu32 ")", getPlayerName(player), player, zCheck, it->second);
			}
		}
		uint16_t zCheck = std::numeric_limits<uint16_t>::max() - 2;
		if (layers.count(zCheck) > 0 && !gInputManager.debugManager().debugMappingsAllowed())
		{
			debug(LOG_INFO, "%s (%u) has an unexpected display layer (script debugger).", getPlayerName(player), player);
			hasWrongData = true;
		}
		zCheck = std::numeric_limits<uint16_t>::max();
		auto it = layers.find(zCheck);
		if (it != layers.end() && it->second > 1)
		{
			debug(LOG_INFO, "%s (%u) has an unexpected number of notification layers. (count: %" PRIu32 ")", getPlayerName(player), player, it->second);
		}
	}

	if (memcmp(DataHash, tempBuffer, sizeof(DataHash)))
	{
		int i = 0;
		for (; DataHash[i] == tempBuffer[i]; ++i)
		{
		}

		debug(LOG_INFO, "%s (%u) has an incompatible mod. ([%d] got %x, expected %x)", getPlayerName(player), player, i, tempBuffer[i], DataHash[i]);
		hasWrongData = true;
	}

	if (aiIndex != NetPlay.players[player].ai)
	{
		debug(LOG_INFO, "%s (%u) has a corrupted player state value. (ai: %" PRIi8 "; should be: %" PRIi8 ")", getPlayerName(player), player, aiIndex, NetPlay.players[player].ai);
		hasWrongData = true;
	}

	if (!NetPlay.players[player].isSpectator && recvGM)
	{
		debug(LOG_INFO, "%s (%u) has a corrupted global state value. (godMode: %s)", getPlayerName(player), player, (recvGM) ? "true" : "false");
		hasWrongData = true;
	}

	if (hasWrongData)
	{
		ASSERT_HOST_ONLY(return false);
		std::string msg = astringf(_("%s (%u) has an incompatible mod, and has been kicked."), getPlayerName(player), player);
		sendInGameSystemMessage(msg.c_str());
		addConsoleMessage(msg.c_str(), LEFT_JUSTIFY, NOTIFY_MESSAGE);

		kickPlayer(player, _("Your data doesn't match the host's!"), ERROR_WRONGDATA, false);
		return false;
	}

	return true;
}


HandleMessageAction getMessageHandlingAction(NETQUEUE& queue, uint8_t type)
{
	if (queue.index == NetPlay.hostPlayer)
	{
		// host gets access to all messages
		return HandleMessageAction::Process_Message;
	}

	bool senderIsSpectator = NetPlay.players[queue.index].isSpectator;

	if (type > NET_MIN_TYPE && type < NET_MAX_TYPE)
	{
		switch (type)
		{
			case NET_OPTIONS:
			case NET_PLAYER_INFO:
			case NET_PLAYER_JOINED:
			case NET_FILE_PAYLOAD:
			case NET_VOTE_REQUEST:
				// only the host is allowed to send these messages
				if (queue.index != NetPlay.hostPlayer)
				{
					return HandleMessageAction::Disallow_And_Kick_Sender;
				}
				break;
			case NET_KICK:
			case NET_AITEXTMSG:
			case NET_BEACONMSG:
			case NET_TEAMREQUEST: // spectators should not be allowed to request a team / non-spectator slot status
			case NET_POSITIONREQUEST:
				if (senderIsSpectator)
				{
					return HandleMessageAction::Disallow_And_Kick_Sender;
				}
				break;
			case NET_TEXTMSG:
				// Normal chat messages are available to spectators in the game room / lobby chat, but *not* in-game
				if (senderIsSpectator && GetGameMode() == GS_NORMAL)
				{
					if (gameInitialised && !bDisplayMultiJoiningStatus)
					{
						// If the game is actually initialized and everyone has joined the game, treat this as a kickable offense
						return HandleMessageAction::Disallow_And_Kick_Sender;
					}
					else
					{
						// Otherwise just silently ignore it
						return HandleMessageAction::Silently_Ignore;
					}
				}
				break;
			case NET_SPECTEXTMSG:
				if (!senderIsSpectator)
				{
					return HandleMessageAction::Silently_Ignore;
				}
				break;
			case NET_VOTE:
				if (senderIsSpectator)
				{
					return HandleMessageAction::Silently_Ignore;
				}
				break;
			case NET_COLOURREQUEST:
				// for now, *must* be allowed
				return HandleMessageAction::Process_Message;
			case NET_DATA_CHECK2:
				if (senderIsSpectator)
				{
					return HandleMessageAction::Silently_Ignore;
				}
				break;
			default:
				// certain messages are always allowed, no matter who it is
				return HandleMessageAction::Process_Message;
		}
	}

	if (type > GAME_MIN_TYPE && type < GAME_MAX_TYPE)
	{
		switch (type)
		{
			case GAME_GAME_TIME:
			case GAME_PLAYER_LEFT:
				// always allowed
				return HandleMessageAction::Process_Message;
			case GAME_SYNC_REQUEST:
				if (senderIsSpectator)
				{
					return HandleMessageAction::Silently_Ignore;
				}
				break;
			case GAME_DEBUG_MODE:
			case GAME_DEBUG_ADD_DROID:
			case GAME_DEBUG_ADD_STRUCTURE:
			case GAME_DEBUG_ADD_FEATURE:
			case GAME_DEBUG_REMOVE_DROID:
			case GAME_DEBUG_REMOVE_STRUCTURE:
			case GAME_DEBUG_REMOVE_FEATURE:
			case GAME_DEBUG_FINISH_RESEARCH:
				if (senderIsSpectator)
				{
					return HandleMessageAction::Disallow_And_Kick_Sender;
				}
				break;
			default:
				if (senderIsSpectator)
				{
					return HandleMessageAction::Disallow_And_Kick_Sender;
				}
				break;
		}
	}

	if (type == REPLAY_ENDED)
	{
		return HandleMessageAction::Silently_Ignore;
	}

	return HandleMessageAction::Process_Message;
}

bool shouldProcessMessage(NETQUEUE& queue, uint8_t type)
{
	auto action = getMessageHandlingAction(queue, type);
	switch (action)
	{
		case HandleMessageAction::Process_Message:
			return true;
		case HandleMessageAction::Silently_Ignore:
			NETpop(queue); // remove message from queue
			return false;
		case HandleMessageAction::Disallow_And_Kick_Sender:
		{
			NETpop(queue); // remove message from queue
			if (NetPlay.isHost)
			{
				// kick sender for sending unauthorized message
				char buf[255];
				auto senderPlayerIdx = queue.index;
				debug(LOG_INFO, "Auto kicking player %s, invalid command received: %s", NetPlay.players[senderPlayerIdx].name, messageTypeToString(type));
				ssprintf(buf, _("Auto kicking player %s, invalid command received: %u"), NetPlay.players[senderPlayerIdx].name, type);
				sendInGameSystemMessage(buf);
				kickPlayer(queue.index, _("Unauthorized network command"), ERROR_INVALID, false);
			}
			return false;
		}
	}
	return false; // silence warnings
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Recv Messages. Get a message and dispatch to relevant function.
bool recvMessage()
{
	NETQUEUE queue;
	uint8_t type;

	while (NETrecvNet(&queue, &type) || NETrecvGame(&queue, &type))          // for all incoming messages.
	{
		bool processedMessage1 = false;
		bool processedMessage2 = false;

		if (queue.queueType == QUEUE_GAME)
		{
			syncDebug("Processing player %d, message %s", queue.index, messageTypeToString(type));
		}

		if (!shouldProcessMessage(queue, type))
		{
			continue;
		}

		// messages only in game.
		if (!ingame.localJoiningInProgress)
		{
			processedMessage1 = true;
			switch (type)
			{
			case GAME_DROIDINFO:					//droid update info
				recvDroidInfo(queue);
				break;
			case NET_TEXTMSG:					// simple text message
				receiveInGameTextMessage(queue);
				break;
			case NET_DATA_CHECK:
				recvDataCheck(queue);
				break;
			case NET_DATA_CHECK2:
				recvDataCheck2(queue);
				break;
			case NET_AITEXTMSG:					//multiplayer AI text message
				recvTextMessageAI(queue);
				break;
			case NET_SPECTEXTMSG:
				recvSpecInGameTextMessage(queue); // multiplayer spectator text message
				break;
			case NET_BEACONMSG:					//beacon (blip) message
				recvBeacon(queue);
				break;
			case GAME_SYNC_REQUEST:
				recvSyncRequest(queue);
				break;
			case GAME_DROIDDISEMBARK:
				recvDroidDisEmbark(queue);           //droid has disembarked from a Transporter
				break;
			case GAME_GIFT:						// an alliance gift from one player to another.
				recvGift(queue);
				break;
			case GAME_LASSAT:
				recvLasSat(queue);
				break;
			case GAME_DEBUG_MODE:
				recvProcessDebugMappings(queue);
				break;
			case GAME_DEBUG_ADD_DROID:
				recvDroid(queue);
				break;
			case GAME_DEBUG_ADD_STRUCTURE:
				recvBuildFinished(queue);
				break;
			case GAME_DEBUG_ADD_FEATURE:
				recvMultiPlayerFeature(queue);
				break;
			case GAME_DEBUG_REMOVE_DROID:
				recvDestroyDroid(queue);
				break;
			case GAME_DEBUG_REMOVE_STRUCTURE:
				recvDestroyStructure(queue);
				break;
			case GAME_DEBUG_REMOVE_FEATURE:
				recvDestroyFeature(queue);
				break;
			case GAME_DEBUG_FINISH_RESEARCH:
				recvResearch(queue);
				break;
			case REPLAY_ENDED:
				if (!NETisReplay())
				{
					// ignore
					break;
				}
				addConsoleMessage(_("REPLAY HAS ENDED"), CENTRE_JUSTIFY, SYSTEM_MESSAGE, false, MAX_CONSOLE_MESSAGE_DURATION);
				addConsoleMessage(_("(Press ESC to quit.)"), CENTRE_JUSTIFY, SYSTEM_MESSAGE, false, MAX_CONSOLE_MESSAGE_DURATION);
				break;
			default:
				processedMessage1 = false;
				break;
			}
		}

		// messages usable all the time
		processedMessage2 = true;
		switch (type)
		{
		case NET_PING:						// diagnostic ping msg.
			recvPing(queue);
			break;
		case NET_PLAYER_DROPPED:				// remote player got disconnected
			{
				uint32_t player_id;

				NETbeginDecode(queue, NET_PLAYER_DROPPED);
				{
					NETuint32_t(&player_id);
				}
				NETend();

				if (player_id >= MAX_CONNECTED_PLAYERS)
				{
					debug(LOG_INFO, "** player %u has dropped - huh?", player_id);
					break;
				}

				if (whosResponsible(player_id) != queue.index && queue.index != NetPlay.hostPlayer)
				{
					HandleBadParam("NET_PLAYER_DROPPED given incorrect params.", player_id, queue.index);
					break;
				}

				debug(LOG_INFO, "** player %u has dropped!", player_id);

				if (NetPlay.players[player_id].allocated)
				{
					MultiPlayerLeave(player_id);		// get rid of their stuff
					NET_InitPlayer(player_id, false);
					ActivityManager::instance().updateMultiplayGameData(game, ingame, NETGameIsLocked());
				}
				NETsetPlayerConnectionStatus(CONNECTIONSTATUS_PLAYER_DROPPED, player_id);
				break;
			}
		case NET_PLAYERRESPONDING:			// remote player is now playing
			{
				uint32_t player_id;

				resetReadyStatus(false);

				NETbeginDecode(queue, NET_PLAYERRESPONDING);
				// the player that has just responded
				NETuint32_t(&player_id);
				NETend();
				if (player_id >= MAX_CONNECTED_PLAYERS)
				{
					debug(LOG_ERROR, "Bad NET_PLAYERRESPONDING received, ID is %d", (int)player_id);
					break;
				}
				// This player is now with us!
				if (ingame.JoiningInProgress[player_id])
				{
					addKnownPlayer(NetPlay.players[player_id].name, getMultiStats(player_id).identity);
				}
				ingame.JoiningInProgress[player_id] = false;
				break;
			}
		case GAME_ALLIANCE:
			recvAlliance(queue, true);
			break;
		case NET_KICK:	// in-game kick message
			{
				uint32_t player_id;
				char reason[MAX_KICK_REASON];
				LOBBY_ERROR_TYPES KICK_TYPE = ERROR_NOERROR;

				NETbeginDecode(queue, NET_KICK);
				NETuint32_t(&player_id);
				NETstring(reason, MAX_KICK_REASON);
				NETenum(&KICK_TYPE);
				NETend();

				if (player_id == NetPlay.hostPlayer)
				{
					char buf[250] = {'\0'};

					ssprintf(buf, "Player %d (%s : %s) tried to kick %u", (int) queue.index, NetPlay.players[queue.index].name, NetPlay.players[queue.index].IPtextAddress, player_id);
					NETlogEntry(buf, SYNC_FLAG, 0);
					debug(LOG_ERROR, "%s", buf);
					if (NetPlay.isHost)
					{
						NETplayerKicked((unsigned int) queue.index);
					}
					break;
				}
				else if (selectedPlayer == player_id)  // we've been told to leave.
				{
					debug(LOG_INFO, "You were kicked because %s", reason);
					displayKickReasonPopup(reason);
					setPlayerHasLost(true);
					ActivityManager::instance().wasKickedByPlayer(NetPlay.players[queue.index], KICK_TYPE, reason);
				}
				else
				{
					debug(LOG_NET, "Player %d was kicked: %s", player_id, reason);
					NETplayerKicked(player_id);
				}
				break;
			}
		case GAME_RESEARCHSTATUS:
			recvResearchStatus(queue);
			break;
		case GAME_STRUCTUREINFO:
			recvStructureInfo(queue);
			break;
		case NET_PLAYER_STATS:
			recvMultiStats(queue);
			break;
		case GAME_PLAYER_LEFT:
			recvPlayerLeft(queue);
			break;
		default:
			processedMessage2 = false;
			break;
		}

		if (processedMessage1 && processedMessage2)
		{
			debug(LOG_ERROR, "Processed %s message twice!", messageTypeToString(type));
		}
		if (!processedMessage1 && !processedMessage2)
		{
			debug(LOG_ERROR, "Didn't handle %s message!", messageTypeToString(type));
		}

		NETpop(queue);
	}

	return true;
}

void HandleBadParam(const char *msg, const int from, const int actual)
{
	char buf[255];
	LOBBY_ERROR_TYPES KICK_TYPE = ERROR_INVALID;

	ssprintf(buf, "!!>Msg: %s, Actual: %d, Bad: %d", msg, actual, from);
	NETlogEntry(buf, SYNC_FLAG, actual);
	if (NetPlay.isHost)
	{
		ssprintf(buf, _("Auto kicking player %s, invalid command received."), NetPlay.players[actual].name);
		sendInGameSystemMessage(buf);
		kickPlayer(actual, buf, KICK_TYPE, false);
	}
}

// ////////////////////////////////////////////////////////////////////////////
// Research Stuff. Nat games only send the result of research procedures.
bool SendResearch(uint8_t player, uint32_t index, bool trigger)
{
	// Send the player that is researching the topic and the topic itself
	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_DEBUG_FINISH_RESEARCH);
	NETuint8_t(&player);
	NETuint32_t(&index);
	NETend();

	return true;
}

// recv a research topic that is now complete.
static bool recvResearch(NETQUEUE queue)
{
	uint8_t			player;
	uint32_t		index;
	int				i;
	PLAYER_RESEARCH	*pPlayerRes;

	NETbeginDecode(queue, GAME_DEBUG_FINISH_RESEARCH);
	NETuint8_t(&player);
	NETuint32_t(&index);
	NETend();

	const DebugInputManager& dbgInputManager = gInputManager.debugManager();
	if (!dbgInputManager.debugMappingsAllowed() && bMultiPlayer)
	{
		debug(LOG_WARNING, "Failed to finish research for player %u.", NetPlay.players[queue.index].position);
		return false;
	}

	syncDebug("player%d, index%u", player, index);

	if (player >= MAX_PLAYERS || index >= asResearch.size())
	{
		debug(LOG_ERROR, "Bad GAME_DEBUG_FINISH_RESEARCH received, player is %d, index is %u", (int)player, index);
		return false;
	}

	pPlayerRes = &asPlayerResList[player][index];
	syncDebug("research status = %d", pPlayerRes->ResearchStatus & RESBITS);

	if (!IsResearchCompleted(pPlayerRes))
	{
		MakeResearchCompleted(pPlayerRes);
		researchResult(index, player, false, nullptr, true);
	}

	// Update allies research accordingly
	if (game.type == LEVEL_TYPE::SKIRMISH)
	{
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (alliances[i][player] == ALLIANCE_FORMED)
			{
				pPlayerRes = &asPlayerResList[i][index];

				if (!IsResearchCompleted(pPlayerRes))
				{
					// Do the research for that player
					MakeResearchCompleted(pPlayerRes);
					researchResult(index, i, false, nullptr, true);
				}
			}
		}
	}

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// New research stuff, so you can see what others are up to!
// inform others that I'm researching this.
bool sendResearchStatus(const STRUCTURE *psBuilding, uint32_t index, uint8_t player, bool bStart)
{
	if (!myResponsibility(player) || gameTime < 5)
	{
		return true;
	}

	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_RESEARCHSTATUS);
	NETuint8_t(&player);
	NETbool(&bStart);

	// If we know the building researching it then send its ID
	if (psBuilding)
	{
		uint32_t buildingID = psBuilding->id;
		NETuint32_t(&buildingID);
	}
	else
	{
		uint32_t zero = 0;
		NETuint32_t(&zero);
	}

	// Finally the topic in question
	NETuint32_t(&index);
	NETend();

	// Tell UI to remove from the list of available research.
	MakeResearchStartedPending(&asPlayerResList[player][index]);

	return true;
}

STRUCTURE *findResearchingFacilityByResearchIndex(unsigned player, unsigned index)
{
	// Go through the structs to find the one doing this topic
	for (STRUCTURE *psBuilding = apsStructLists[player]; psBuilding; psBuilding = psBuilding->psNext)
	{
		if (psBuilding->pStructureType->type == REF_RESEARCH
		    && ((RESEARCH_FACILITY *)psBuilding->pFunctionality)->psSubject
		    && ((RESEARCH_FACILITY *)psBuilding->pFunctionality)->psSubject->ref - STAT_RESEARCH == index)
		{
			return psBuilding;
		}
	}
	return nullptr;  // Not found.
}

bool recvResearchStatus(NETQUEUE queue)
{
	STRUCTURE			*psBuilding;
	PLAYER_RESEARCH		*pPlayerRes;
	RESEARCH_FACILITY	*psResFacilty;
	RESEARCH			*pResearch;
	uint8_t				player;
	bool bStart = false;
	uint32_t			index, structRef;

	NETbeginDecode(queue, GAME_RESEARCHSTATUS);
	NETuint8_t(&player);
	NETbool(&bStart);
	NETuint32_t(&structRef);
	NETuint32_t(&index);
	NETend();

	syncDebug("player%d, bStart%d, structRef%u, index%u", player, bStart, structRef, index);

	if (player >= MAX_PLAYERS || index >= asResearch.size())
	{
		debug(LOG_ERROR, "Bad GAME_RESEARCHSTATUS received, player is %d, index is %u", (int)player, index);
		return false;
	}
	if (!canGiveOrdersFor(queue.index, player))
	{
		debug(LOG_WARNING, "Droid order for wrong player.");
		syncDebug("Wrong player.");
		return false;
	}

	int prevResearchState = 0;
	if (selectedPlayer < MAX_PLAYERS && aiCheckAlliances(selectedPlayer, player))
	{
		prevResearchState = intGetResearchState();
	}

	pPlayerRes = &asPlayerResList[player][index];

	// psBuilding may be null if finishing
	if (bStart)							// Starting research
	{
		ResetPendingResearchStatus(pPlayerRes);  // Reset pending state, even if research state is not changed due to the structure being destroyed.

		psBuilding = IdToStruct(structRef, player);

		// Set that facility to research
		if (psBuilding && psBuilding->pFunctionality)
		{
			if (!psBuilding->pStructureType || psBuilding->pStructureType->type != REF_RESEARCH)
			{
				debug(LOG_INFO, "Structure is not a research facility: \"%s\".", (psBuilding->pStructureType) ? psBuilding->pStructureType->id.toUtf8().c_str() : "");
				return false;
			}

			psResFacilty = (RESEARCH_FACILITY *) psBuilding->pFunctionality;

			popStatusPending(*psResFacilty);  // Research is no longer pending, as it's actually starting now.

			if (psResFacilty->psSubject)
			{
				cancelResearch(psBuilding, ModeImmediate);
			}

			if (IsResearchStarted(pPlayerRes))
			{
				STRUCTURE *psOtherBuilding = findResearchingFacilityByResearchIndex(player, index);
				ASSERT(psOtherBuilding != nullptr, "Something researched but no facility.");
				if (psOtherBuilding != nullptr)
				{
					cancelResearch(psOtherBuilding, ModeImmediate);
				}
			}

			if (!researchAvailable(index, player, ModeImmediate) && bMultiPlayer)
			{
				debug(LOG_ERROR, "Player %d researching impossible topic \"%s\".", player, getStatsName(&asResearch[index]));
				return false;
			}

			// Set the subject up
			pResearch				= &asResearch[index];
			psResFacilty->psSubject = pResearch;

			// Start the research
			MakeResearchStarted(pPlayerRes);
			psResFacilty->timeStartHold		= 0;
		}
	}
	// Finished/cancelled research
	else
	{
		// If they completed the research, we're done
		if (IsResearchCompleted(pPlayerRes))
		{
			return true;
		}

		// If they did not say what facility it was, look it up orselves
		if (!structRef)
		{
			psBuilding = findResearchingFacilityByResearchIndex(player, index);
		}
		else
		{
			psBuilding = IdToStruct(structRef, player);
		}

		// Stop the facility doing any research
		if (psBuilding)
		{
			if (!psBuilding->pStructureType || psBuilding->pStructureType->type != REF_RESEARCH)
			{
				debug(LOG_INFO, "Structure is not a research facility: \"%s\".", (psBuilding->pStructureType) ? psBuilding->pStructureType->id.toUtf8().c_str() : "");
				return false;
			}

			cancelResearch(psBuilding, ModeImmediate);
			popStatusPending(*(RESEARCH_FACILITY *)psBuilding->pFunctionality);  // Research cancellation is no longer pending, as it's actually cancelling now.
		}
	}

	if (selectedPlayer < MAX_PLAYERS && aiCheckAlliances(selectedPlayer, player))
	{
		intAlliedResearchChanged();
		intNotifyResearchButton(prevResearchState);
	}

	return true;
}

void setPlayerMuted(uint32_t playerIdx, bool muted)
{
	ASSERT_OR_RETURN(, playerIdx < MAX_CONNECTED_PLAYERS, "Invalid playerIdx: %" PRIu32, playerIdx);
	if (muted == ingame.muteChat[playerIdx])
	{
		// no change
		return;
	}
	ingame.muteChat[playerIdx] = muted;
	if (isHumanPlayer(playerIdx))
	{
		storePlayerMuteOption(NetPlay.players[playerIdx].name, getMultiStats(playerIdx).identity, muted);
	}
}

bool isPlayerMuted(uint32_t sender)
{
	ASSERT_OR_RETURN(false, sender < MAX_CONNECTED_PLAYERS, "Invalid sender: %" PRIu32, sender);
	return ingame.muteChat[sender];
}

NetworkTextMessage::NetworkTextMessage(int32_t messageSender, char const *messageText)
{
	sender = messageSender;
	sstrcpy(text, messageText);
}

void NetworkTextMessage::enqueue(NETQUEUE queue)
{
	NETbeginEncode(queue, NET_TEXTMSG);
	NETint32_t(&sender);
	NETbool(&teamSpecific);
	NETstring(text, MAX_CONSOLE_STRING_LENGTH);
	NETend();
}

bool NetworkTextMessage::receive(NETQUEUE queue)
{
	memset(text, 0x0, sizeof(text));

	NETbeginDecode(queue, NET_TEXTMSG);
	NETint32_t(&sender);
	NETbool(&teamSpecific);
	NETstring(text, MAX_CONSOLE_STRING_LENGTH);
	NETend();

	if (whosResponsible(sender) != queue.index)
	{
		sender = queue.index;  // Fix corrupted sender.
	}

	if (sender >= MAX_CONNECTED_PLAYERS || (sender >= 0 && (!NetPlay.players[sender].allocated && NetPlay.players[sender].ai == AI_OPEN)))
	{
		return false;
	}

	return true;
}

void printInGameTextMessage(NetworkTextMessage const &message)
{
	switch (message.sender)
	{
	case SYSTEM_MESSAGE:
	case NOTIFY_MESSAGE:
		addConsoleMessage(message.text, DEFAULT_JUSTIFY, message.sender, message.teamSpecific);
		break;

	default:
		char formatted[MAX_CONSOLE_STRING_LENGTH];
		ssprintf(formatted, "[%s] %s", formatLocalDateTime("%H:%M").c_str(), message.text);
		addConsoleMessage(formatted, DEFAULT_JUSTIFY, message.sender, message.teamSpecific);
		break;
	}
}

void sendInGameSystemMessage(const char *text)
{
	NetworkTextMessage message(SYSTEM_MESSAGE, text);
	printInGameTextMessage(message);
	if (NetPlay.isHost || !NetPlay.players[selectedPlayer].isSpectator || GetGameMode() != GS_NORMAL)
	{
		// host + players can broadcast these at any time
		// spectators can only broadcast in-game system messages before the game has started (i.e. in lobby)
		message.enqueue(NETbroadcastQueue());
	}
}

void printConsoleNameChange(const char *oldName, const char *newName)
{
	char msg[MAX_CONSOLE_STRING_LENGTH];
	ssprintf(msg, "%s → %s", oldName, newName);
	displayRoomSystemMessage(msg);
}

//
// At this time, we do NOT support messages for beacons
//
bool sendBeacon(int32_t locX, int32_t locY, int32_t forPlayer, int32_t sender, const char *pStr)
{
	int sendPlayer;
	//debug(LOG_WZ, "sendBeacon: '%s'",pStr);

	//find machine that is hosting this human or AI
	sendPlayer = whosResponsible(forPlayer);

	if (sendPlayer >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "sendBeacon() - whosResponsible() failed.");
		return false;
	}

	// I assume this is correct, looks like it sends it to ONLY that person, and the routine
	// kf_AddHelpBlip() iterates for each player it needs.
	NETbeginEncode(NETnetQueue(sendPlayer), NET_BEACONMSG);    // send to the player who is hosting 'to' player (might be himself if human and not AI)
	NETint32_t(&sender);                                // save the actual sender

	// save the actual player that is to get this msg on the source machine (source can host many AIs)
	NETint32_t(&forPlayer);                             // save the actual receiver (might not be the same as the one we are actually sending to, in case of AIs)
	NETint32_t(&locX);                                  // save location
	NETint32_t(&locY);

	NETstring(pStr, MAX_CONSOLE_STRING_LENGTH); // copy message in.
	NETend();

	return true;
}

/**
 * Read a message from the queue, and write it to the console.
 *
 * This message can be:
 * - In game chat message
 * - In game system message (player got kicked, player used cheat, etc.)
 **/
bool receiveInGameTextMessage(NETQUEUE queue)
{
	NetworkTextMessage message;
	if (!message.receive(queue)) {
		return false;
	}

	if (message.sender >= 0 && isPlayerMuted(message.sender))
	{
		return false;
	}

	printInGameTextMessage(message);

	// make some noise!
	if (GetGameMode() != GS_NORMAL)
	{
		audio_PlayTrack(FE_AUDIO_MESSAGEEND);
	}
	else if (!ingame.localJoiningInProgress)
	{
		audio_PlayTrack(ID_SOUND_MESSAGEEND);
	}

	return true;
}

//AI multiplayer message - received message for AI (for hosted scripts)
bool recvTextMessageAI(NETQUEUE queue)
{
	UDWORD	sender, receiver;
	char	msg[MAX_CONSOLE_STRING_LENGTH];
	char	newmsg[MAX_CONSOLE_STRING_LENGTH];

	NETbeginDecode(queue, NET_AITEXTMSG);
	NETuint32_t(&sender);			//in-game player index ('normal' one)
	NETuint32_t(&receiver);			//in-game player index
	NETstring(newmsg, MAX_CONSOLE_STRING_LENGTH);
	NETend();

	if (whosResponsible(sender) != queue.index)
	{
		sender = queue.index;  // Fix corrupted sender.
	}

	sstrcpy(msg, newmsg);
	triggerEventChat(sender, receiver, newmsg);

	return true;
}

bool recvSpecInGameTextMessage(NETQUEUE queue)
{
	UDWORD	sender;
	char	newmsg[MAX_CONSOLE_STRING_LENGTH] = {};

	NETbeginDecode(queue, NET_SPECTEXTMSG);
	NETuint32_t(&sender);			//in-game player index ('normal' one)
	NETstring(newmsg, MAX_CONSOLE_STRING_LENGTH);
	NETend();

	if (whosResponsible(sender) != queue.index)
	{
		sender = queue.index;  // Fix corrupted sender.
	}

	if (sender >= MAX_CONNECTED_PLAYERS || (!NetPlay.players[sender].allocated && NetPlay.players[sender].ai == AI_OPEN))
	{
		return false;
	}

	if (!NetPlay.players[selectedPlayer].isSpectator)
	{
		return false; // ignore
	}

	auto message = NetworkTextMessage(SPECTATOR_MESSAGE, newmsg);

	if (isPlayerMuted(sender))
	{
		return false;
	}

	printInGameTextMessage(message);

	// make some noise!
	if (GetGameMode() != GS_NORMAL)
	{
		audio_PlayTrack(FE_AUDIO_MESSAGEEND);
	}
	else if (!ingame.localJoiningInProgress)
	{
		audio_PlayTrack(ID_SOUND_MESSAGEEND);
	}

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Features

// process a destroy feature msg.
bool recvDestroyFeature(NETQUEUE queue)
{
	FEATURE *pF;
	uint32_t	id;

	NETbeginDecode(queue, GAME_DEBUG_REMOVE_FEATURE);
	NETuint32_t(&id);
	NETend();

	const DebugInputManager& dbgInputManager = gInputManager.debugManager();
	if (!dbgInputManager.debugMappingsAllowed() && bMultiPlayer)
	{
		debug(LOG_WARNING, "Failed to remove feature for player %u.", NetPlay.players[queue.index].position);
		return false;
	}

	pF = IdToFeature(id, ANYPLAYER);
	if (pF == nullptr)
	{
		debug(LOG_FEATURE, "feature id %d not found (probably already destroyed)", id);
		return false;
	}

	debug(LOG_FEATURE, "p%d feature id %d destroyed (%s)", pF->player, pF->id, getStatsName(pF->psStats));
	// Remove the feature locally
	turnOffMultiMsg(true);
	destroyFeature(pF, gameTime - deltaGameTime + 1);  // deltaGameTime is actually 0 here, since we're between updates. However, the value of gameTime - deltaGameTime + 1 will not change when we start the next tick.
	turnOffMultiMsg(false);

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// Network File packet processor.
bool recvMapFileRequested(NETQUEUE queue)
{
	ASSERT_OR_RETURN(false, NetPlay.isHost, "Host only routine detected for client!");

	uint32_t player = queue.index;

	Sha256 hash;
	hash.setZero();
	NETbeginDecode(queue, NET_FILE_REQUESTED);
	NETbin(hash.bytes, hash.Bytes);
	NETend();

	auto files = NetPlay.players[player].wzFiles;
	ASSERT_OR_RETURN(false, files != nullptr, "wzFiles is uninitialized?? (Player: %" PRIu32 ")", player);
	if (std::any_of(files->begin(), files->end(), [&](WZFile const &file) { return file.hash == hash; }))
	{
		return true;  // Already sending this file, do nothing.
	}

	netPlayersUpdated = true;  // Show download icon on player.

	std::string filename;
	if (hash == game.hash)
	{
		addConsoleMessage(_("Map was requested: SENDING MAP!"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);

		LEVEL_DATASET *mapData = levFindDataSet(game.map, &game.hash);
		ASSERT_OR_RETURN(false, mapData, "levFindDataSet failed for game.map: %s", game.map);
		ASSERT_OR_RETURN(false, mapData->realFileName != nullptr, "levFindDataSet found game.map: %s; but realFileName is empty - requesting a built-in map??", game.map);
		filename = mapData->realFileName;
		if (filename.empty())
		{
			debug(LOG_INFO, "Unknown map requested by %u.", player);
			return false;
		}
		debug(LOG_INFO, "Map was requested. Looking for %s", filename.c_str());
	}
	else
	{
		filename = getModFilename(hash);
		if (filename.empty())
		{
			debug(LOG_INFO, "Unknown file requested by %u.", player);
			return false;
		}

		addConsoleMessage(_("Mod was requested: SENDING MOD!"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	}

	// Checking to see if file is available...
	PHYSFS_file *pFileHandle = PHYSFS_openRead(filename.c_str());
	if (pFileHandle == nullptr)
	{
		debug(LOG_ERROR, "Failed to open %s for reading: %s", filename.c_str(), WZ_PHYSFS_getLastError());
		debug(LOG_FATAL, "You have a map (%s) that can't be located.\n\nMake sure it is in the correct directory and or format! (No map packs!)", filename.c_str());
		// NOTE: if we get here, then the game is basically over, The host can't send the file for whatever reason...
		// Which also means, that we can't continue.
		debug(LOG_NET, "***Host has a file issue, and is being forced to quit!***");
		NETbeginEncode(NETbroadcastQueue(), NET_HOST_DROPPED);
		NETend();
		abort();
	}

	PHYSFS_sint64 fileSize_64 = PHYSFS_fileLength(pFileHandle);
	ASSERT_OR_RETURN(false, fileSize_64 <= 0xFFFFFFFF, "File is too big!");
	ASSERT_OR_RETURN(false, fileSize_64 >= 0, "Filesize < 0; can't be determined");
	uint32_t fileSize_u32 = (uint32_t)fileSize_64;
	ASSERT_OR_RETURN(false, fileSize_u32 <= MAX_NET_TRANSFERRABLE_FILE_SIZE, "Filesize is too large; (size: %" PRIu32")", fileSize_u32);

	// Schedule file to be sent.
	debug(LOG_INFO, "File is valid, sending [directory: %s] %s to client %u", WZ_PHYSFS_getRealDir_String(filename.c_str()).c_str(), filename.c_str(), player);
	files->emplace_back(pFileHandle, filename, hash, fileSize_u32);

	return true;
}

// Continue sending maps and mods.
void sendMap()
{
	// maximum "budget" in time per call to sendMap
	// (at 60fps, total frame budget is ~16ms - allocate 4ms max for each call to sendMap)
	const uint64_t maxMicroSecondsPerSendMapCall = (4 * 1000);

	// calculate the time budget per file
	uint64_t totalFilesToSend = 0;
	for (int i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
	{
		totalFilesToSend += (NetPlay.players[i].wzFiles) ? NetPlay.players[i].wzFiles->size() : 0;
	}
	const uint64_t maxMicroSecondsPerFile = maxMicroSecondsPerSendMapCall / std::max((uint64_t)1, totalFilesToSend);

	using microDuration = std::chrono::duration<uint64_t, std::micro>;
	auto file_startTime = std::chrono::high_resolution_clock::now();
	microDuration file_currentDuration;

	for (int i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
	{
		auto pFiles = NetPlay.players[i].wzFiles;
		if (pFiles == nullptr)
		{
			continue;
		}
		auto &files = *pFiles;
		for (auto &file : files)
		{
			int done = 0;
			file_startTime = std::chrono::high_resolution_clock::now();
			do
			{
				done = NETsendFile(file, i);
				file_currentDuration = std::chrono::duration_cast<microDuration>(std::chrono::high_resolution_clock::now() - file_startTime);
			}
			while (done < 100 && (file_currentDuration.count() < maxMicroSecondsPerFile));
			if (done == 100)
			{
				netPlayersUpdated = true;  // Remove download icon from player.
				addConsoleMessage(_("FILE SENT!"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
				debug(LOG_INFO, "=== File has been sent to player %d ===", i);
			}
		}
		files.erase(std::remove_if(files.begin(), files.end(), [](WZFile const &file) { return file.handle() == nullptr; }), files.end());
	}
}

// Another player is broadcasting a map, recv a chunk. Returns false if not yet done.
bool recvMapFileData(NETQUEUE queue)
{
	NETrecvFile(queue);
	if (NET_getDownloadingWzFiles().empty())
	{
		netPlayersUpdated = true;  // Remove download icon from ourselves.
		addConsoleMessage(_("MAP DOWNLOADED!"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
		sendInGameSystemMessage("MAP DOWNLOADED");
		debug(LOG_INFO, "=== File has been received. ===");

		// clear out the old level list.
		levShutDown();
		levInitialise();
		rebuildSearchPath(mod_multiplay, true);	// MUST rebuild search path for the new maps we just got!
		pal_Init(); //Update palettes.
		if (!buildMapList())
		{
			return false;
		}

		LEVEL_DATASET *mapData = levFindDataSet(game.map, &game.hash);
		if (mapData && CheckForMod(mapData->realFileName))
		{
			char buf[256];
			if (game.isMapMod)
			{
				ssprintf(buf, "%s", _("Warning, this is a map-mod, it could alter normal gameplay."));
			}
			else
			{
				ssprintf(buf, "%s", _("Warning, HOST has altered the game code, and can't be trusted!"));
			}
			addConsoleMessage(buf,  DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
			game.isMapMod = true;
			widgReveal(psWScreen, MULTIOP_MAP_MOD);
		}
		if (mapData && CheckForRandom(mapData->realFileName, mapData->apDataFiles[0]))
		{
			game.isRandom = true;
			widgReveal(psWScreen, MULTIOP_MAP_RANDOM);
		}

		loadMapPreview(false);
		return true;
	}

	return false;
}


//prepare viewdata for help blip
VIEWDATA *CreateBeaconViewData(SDWORD sender, UDWORD LocX, UDWORD LocY)
{
	UDWORD height;
	VIEWDATA *psViewData = nullptr;
	SDWORD audioID;
	char name[255];

	//allocate message space
	psViewData = new VIEWDATA;

	//store name
	snprintf(name, sizeof(name), _("Beacon %d"), sender);
	psViewData->name = name;

	//store text message, hardcoded for now
	psViewData->textMsg.push_back(WzString::fromUtf8(getPlayerName(sender)));

	//store message type
	psViewData->type = VIEW_BEACON;

	//allocate memory for blip location etc
	psViewData->pData = new VIEW_PROXIMITY;

	//store audio
	audioID = NO_SOUND;
	((VIEW_PROXIMITY *)psViewData->pData)->audioID = audioID;

	//store blip location
	((VIEW_PROXIMITY *)psViewData->pData)->x = (UDWORD)LocX;
	((VIEW_PROXIMITY *)psViewData->pData)->y = (UDWORD)LocY;

	//check the z value is at least the height of the terrain
	height = map_Height(LocX, LocY);

	((VIEW_PROXIMITY *)psViewData->pData)->z = height;

	//store prox message type
	((VIEW_PROXIMITY *)psViewData->pData)->proxType = PROX_ENEMY; //PROX_ENEMY for now

	//remember who sent this msg, so we could remove this one, when same player sends a new help-blip msg
	((VIEW_PROXIMITY *)psViewData->pData)->sender = sender;

	//remember when the message was created so can remove it after some time
	((VIEW_PROXIMITY *)psViewData->pData)->timeAdded = gameTime;
	debug(LOG_MSG, "Added message");

	return psViewData;
}

/* Looks through the players list of messages to find VIEW_BEACON (one per player!) pointer */
MESSAGE *findBeaconMsg(UDWORD player, SDWORD sender)
{
	ASSERT_OR_RETURN(nullptr, player < MAX_PLAYERS, "Unsupported player: %" PRIu32 "", player);

	for (MESSAGE *psCurr = apsMessages[player]; psCurr != nullptr; psCurr = psCurr->psNext)
	{
		//look for VIEW_BEACON, should only be 1 per player
		if (psCurr->dataType == MSG_DATA_BEACON)
		{
			if (psCurr->pViewData->type == VIEW_BEACON)
			{
				debug(LOG_WZ, "findBeaconMsg: %d ALREADY HAS A MESSAGE STORED", player);
				if (((VIEW_PROXIMITY *)psCurr->pViewData->pData)->sender == sender)
				{
					debug(LOG_WZ, "findBeaconMsg: %d ALREADY HAS A MESSAGE STORED from %d", player, sender);
					return psCurr;
				}
			}
		}
	}

	//not found the message so return NULL
	return nullptr;
}

/* Add a beacon (blip) */
bool addBeaconBlip(SDWORD locX, SDWORD locY, SDWORD forPlayer, SDWORD sender, const char *textMsg)
{
	MESSAGE *psMessage;

	if (forPlayer >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "addBeaconBlip: player number is too high");
		return false;
	}

	//find the message if was already added previously
	psMessage = findBeaconMsg(forPlayer, sender);
	if (psMessage)
	{
		//remove it
		removeMessage(psMessage, forPlayer);
	}

	//create new message
	psMessage = addBeaconMessage(MSG_PROXIMITY, false, forPlayer);
	if (psMessage)
	{
		VIEWDATA *pTempData = CreateBeaconViewData(sender, locX, locY);
		ASSERT_OR_RETURN(false, pTempData != nullptr, "Empty help data for radar beacon");
		psMessage->pViewData = pTempData;
		debug(LOG_MSG, "blip added, pViewData=%p", static_cast<void *>(psMessage->pViewData));
		jsDebugMessageUpdate();
	}
	else
	{
		debug(LOG_WARNING, "call failed");
	}

	//Received a blip message from a player callback
	//store and call later
	//-------------------------------------------------
	//call beacon callback only if not adding for ourselves
	if (forPlayer != sender)
	{
		triggerEventBeacon(sender, forPlayer, textMsg, locX, locY);

		if (selectedPlayer == forPlayer)
		{
			// show console message
			CONPRINTF(_("Beacon received from %s!"), getPlayerName(sender));
			// play audio
			audio_QueueTrackPos(ID_SOUND_BEACON, locX, locY, 0);
		}
	}

	return true;
}

bool sendBeaconToPlayer(SDWORD locX, SDWORD locY, SDWORD forPlayer, SDWORD sender, const char *beaconMsg)
{
	bool retval;
	if (sender == forPlayer || myResponsibility(forPlayer))	//if destination player is on this machine
	{
		debug(LOG_WZ, "sending beacon to player %d (local player) from %d", forPlayer, sender);
		retval = addBeaconBlip(locX, locY, forPlayer, sender, beaconMsg);
	}
	else
	{
		debug(LOG_WZ, "sending beacon to player %d (remote player) from %d", forPlayer, sender);
		retval = sendBeacon(locX, locY, forPlayer, sender, beaconMsg);
	}
	jsDebugMessageUpdate();
	return retval;
}

static bool recvBeacon(NETQUEUE queue)
{
	int32_t sender, receiver, locX, locY;
	char    msg[MAX_CONSOLE_STRING_LENGTH];

	NETbeginDecode(queue, NET_BEACONMSG);
	NETint32_t(&sender);            // the actual sender
	NETint32_t(&receiver);          // the actual receiver (might not be the same as the one we are actually sending to, in case of AIs)
	NETint32_t(&locX);
	NETint32_t(&locY);
	NETstring(msg, sizeof(msg));    // Receive the actual message
	NETend();

	debug(LOG_WZ, "Received beacon for player: %d, from: %d", receiver, sender);

	sstrcat(msg, NetPlay.players[sender].name);    // name
	sstrcpy(beaconReceiveMsg[sender], msg);

	return addBeaconBlip(locX, locY, receiver, sender, beaconReceiveMsg[sender]);
}

const char *getPlayerColourName(int player)
{
	static const char *playerColors[] =
	{
		N_("Green"),
		N_("Orange"),
		N_("Grey"),
		N_("Black"),
		N_("Red"),
		N_("Blue"),
		N_("Pink"),
		N_("Cyan"),
		N_("Yellow"),
		N_("Purple"),
		N_("White"),
		N_("Bright blue"),
		N_("Neon green"),
		N_("Infrared"),
		N_("Ultraviolet"),
		N_("Brown"),
	};
	STATIC_ASSERT(MAX_PLAYERS <= ARRAY_SIZE(playerColors));

	ASSERT(player < ARRAY_SIZE(playerColors), "player number (%d) exceeds maximum (%lu)", player, (unsigned long) ARRAY_SIZE(playerColors));

	if (player >= ARRAY_SIZE(playerColors))
	{
		return "";
	}

	return gettext(playerColors[getPlayerColour(player)]);
}

/* Reset ready status for all players */
void resetReadyStatus(bool bSendOptions, bool ignoreReadyReset)
{
	// notify all clients if needed
	if (bSendOptions)
	{
		sendOptions();
	}
	netPlayersUpdated = true;

	//Really reset ready status
	if (NetPlay.isHost && !ignoreReadyReset)
	{
		for (unsigned int i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
		{
			//Ignore for autohost launch option.
			if (selectedPlayer == i && getHostLaunch() == HostLaunch::Autohost)
			{
				continue;
			}

			if (isHumanPlayer(i) && ingame.JoiningInProgress[i])
			{
				changeReadyStatus(i, false);
			}
		}
	}
}

int32_t findPlayerIndexByPosition(uint32_t position)
{
	for (auto playerIndex = 0; playerIndex < game.maxPlayers; playerIndex++)
	{
		if (NetPlay.players[playerIndex].position == position) {
			return playerIndex;
		}
	}

	return -1;
}

bool makePlayerSpectator(uint32_t playerIndex, bool removeAllStructs, bool quietly)
{
	// Remove objects quietly if the player is starting off as a spectator
	quietly = quietly || NetPlay.players[playerIndex].isSpectator;

	turnOffMultiMsg(true);

	if (playerIndex < MAX_PLAYERS)
	{
		setPower(playerIndex, 0);

		// Destroy HQ
		std::vector<STRUCTURE *> hqStructs;
		for (STRUCTURE *psStruct = apsStructLists[playerIndex]; psStruct; psStruct = psStruct->psNext)
		{
			if (REF_HQ == psStruct->pStructureType->type)
			{
				hqStructs.push_back(psStruct);
			}
		}
		for (auto psStruct : hqStructs)
		{
			if (quietly)
			{
				removeStruct(psStruct, true);
			}
			else			// show effects
			{
				destroyStruct(psStruct, gameTime);
			}
		}

		// Destroy all droids
		debug(LOG_DEATH, "killing off all droids for player %d", playerIndex);
		while (apsDroidLists[playerIndex])				// delete all droids
		{
			if (quietly)			// don't show effects
			{
				killDroid(apsDroidLists[playerIndex]);
			}
			else				// show effects
			{
				destroyDroid(apsDroidLists[playerIndex], gameTime);
			}
		}

		// Destroy structs
		debug(LOG_DEATH, "killing off structures for player %d", playerIndex);
		STRUCTURE *psStruct = apsStructLists[playerIndex];
		while (psStruct)				// delete structs
		{
			STRUCTURE * psNext = psStruct->psNext;

			if (removeAllStructs
				|| psStruct->pStructureType->type == REF_POWER_GEN
				|| psStruct->pStructureType->type == REF_RESEARCH
				|| psStruct->pStructureType->type == REF_COMMAND_CONTROL
				|| StructIsFactory(psStruct))
			{
				// FIXME: look why destroyStruct() doesn't put back the feature like removeStruct() does
				if (quietly || psStruct->pStructureType->type == REF_RESOURCE_EXTRACTOR)		// don't show effects
				{
					removeStruct(psStruct, true);
				}
				else			// show effects
				{
					destroyStruct(psStruct, gameTime);
				}
			}

			psStruct = psNext;
		}
	}

	if (!quietly)
	{
		debug(LOG_INFO, "player: %" PRIu32 " (gameTime: %" PRIu32 ")", playerIndex, gameTime);
	}
	if (!NETisReplay() || playerIndex != realSelectedPlayer)
	{
		syncDebug("player%u", (unsigned)playerIndex);
	}
	NetPlay.players[playerIndex].isSpectator = true; // must come before enableGodMode

	if (playerIndex == selectedPlayer)
	{
		// reset the widget screen to just the reticule (close all panels)
		auto savedIntMode = intMode;
		intResetScreen(false, true);
		intMode = savedIntMode; // restore intMode from before intResetScreen call (or it may not be possible to click "continue game" on the mission results screen)

		// disable various reticule buttons
		std::array<UDWORD, 5> reticuleButtonsToDisable{IDRET_MANUFACTURE, IDRET_RESEARCH, IDRET_BUILD, IDRET_DESIGN, IDRET_COMMAND};
		for (UDWORD buttonID : reticuleButtonsToDisable)
		{
			if (intCheckReticuleButEnabled(buttonID))
			{
				setReticuleStats(buttonID, "", "", "");
			}
		}

		// hide the power bar
		forceHidePowerBar(true);

		if (!headlessGameMode())
		{
			// enable "god mode" for map + object visibility (+ minimap)
			enableGodMode();
		}

		// add spectator mode message
		bool lowUISpectatorMode = streamer_spectator_mode() || NETisReplay();
		addConsoleMessage(_("Spectator Mode"), CENTRE_JUSTIFY, SYSTEM_MESSAGE, false, (!lowUISpectatorMode) ? MAX_CONSOLE_MESSAGE_DURATION : 15);
		addConsoleMessage(_("You are a spectator. Enjoy watching the game!"), CENTRE_JUSTIFY, SYSTEM_MESSAGE, false, (!lowUISpectatorMode) ? 30 : 15);

		specLayerInit(!streamer_spectator_mode());
	}

	turnOffMultiMsg(false);

	return true;
}
