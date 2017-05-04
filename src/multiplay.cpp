/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
#include "research.h"
#include "init.h"
#include "warcam.h"	// these 4 for fireworks
#include "mission.h"
#include "effects.h"
#include "lib/gamelib/gtime.h"
#include "keybind.h"
#include "qtscript.h"
#include "design.h"

#include "lib/script/script.h"				//Because of "ScriptTabs.h"
#include "scripttabs.h"			//because of CALL_AI_MSG
#include "scriptcb.h"			//for console callback
#include "scriptfuncs.h"
#include "template.h"
#include "lib/netplay/netplay.h"								// the netplay library.
#include "modding.h"
#include "multiplay.h"								// warzone net stuff.
#include "multijoin.h"								// player management stuff.
#include "multirecv.h"								// incoming messages stuff
#include "multistat.h"
#include "multigifts.h"								// gifts and alliances.
#include "multiint.h"
#include "keymap.h"
#include "cheat.h"

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// globals.
bool						bMultiPlayer				= false;	// true when more than 1 player.
bool						bMultiMessages				= false;	// == bMultiPlayer unless multimessages are disabled
bool						openchannels[MAX_PLAYERS] = {true};
UBYTE						bDisplayMultiJoiningStatus;

MULTIPLAYERGAME				game;									//info to describe game.
MULTIPLAYERINGAME			ingame;

char						beaconReceiveMsg[MAX_PLAYERS][MAX_CONSOLE_STRING_LENGTH];	//beacon msg for each player
char								playerName[MAX_PLAYERS][MAX_STR_LENGTH];	//Array to store all player names (humans and AIs)

/////////////////////////////////////
/* multiplayer message stack stuff */
/////////////////////////////////////
#define MAX_MSG_STACK	100				// must be *at least* 64

static char msgStr[MAX_MSG_STACK][MAX_STR_LENGTH];
static SDWORD msgPlFrom[MAX_MSG_STACK];
static SDWORD msgPlTo[MAX_MSG_STACK];
static SDWORD callbackType[MAX_MSG_STACK];
static SDWORD locx[MAX_MSG_STACK];
static SDWORD locy[MAX_MSG_STACK];
static DROID *msgDroid[MAX_MSG_STACK];
static SDWORD msgStackPos = -1;				//top element pointer

// ////////////////////////////////////////////////////////////////////////////
// Local Prototypes

static bool recvBeacon(NETQUEUE queue);
static bool recvResearch(NETQUEUE queue);
static bool sendAIMessage(char *pStr, UDWORD player, UDWORD to);

bool		multiplayPlayersReady(bool bNotifyStatus);
void		startMultiplayerGame(void);

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
	static Position pos;
	Position pos2;
	static UDWORD last = 0;
	float		rotAmount;
	STRUCTURE	*psStruct;

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
		player.r.y += rotAmount;
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
		pos2 = pos;
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
bool multiPlayerLoop(void)
{
	UDWORD		i;
	UBYTE		joinCount;

	joinCount = 0;
	for (i = 0; i < MAX_PLAYERS; i++)
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
			setWidgetsStatus(true);
			setPlayerHasLost(true);
		}
	}
	else		//everyone is in the game now!
	{
		if (bDisplayMultiJoiningStatus)
		{
			bDisplayMultiJoiningStatus = 0;
			setWidgetsStatus(true);
		}
		if (!ingame.TimeEveryoneIsInGame)
		{
			ingame.TimeEveryoneIsInGame = gameTime;
			debug(LOG_NET, "I have entered the game @ %d", ingame.TimeEveryoneIsInGame);
			if (!NetPlay.isHost)
			{
				debug(LOG_NET, "=== Sending hash to host ===");
				sendDataCheck();
			}
		}
		if (NetPlay.bComms)
		{
			sendPing();
		}
		// Only have to do this on a true MP game
		if (NetPlay.isHost && !ingame.isAllPlayersDataOK && NetPlay.bComms)
		{
			if (gameTime - ingame.TimeEveryoneIsInGame > GAME_TICKS_PER_SEC * 60)
			{
				// we waited 60 secs to make sure people didn't bypass the data integrity checks
				int index;
				for (index = 0; index < MAX_PLAYERS; index++)
				{
					if (ingame.DataIntegrity[index] == false && isHumanPlayer(index) && index != NET_HOST_ONLY)
					{
						char msg[256] = {'\0'};

						sprintf(msg, _("Kicking player %s, because they tried to bypass data integrity check!"), getPlayerName(index));
						sendTextMessage(msg, true);
						addConsoleMessage(msg, LEFT_JUSTIFY, NOTIFY_MESSAGE);
						NETlogEntry(msg, SYNC_FLAG, index);

#ifndef DEBUG
						kickPlayer(index, "invalid data!", ERROR_INVALID);
#endif
						debug(LOG_WARNING, "Kicking Player %s (%u), they tried to bypass data integrity check!", getPlayerName(index), index);
					}
				}
				ingame.isAllPlayersDataOK = true;
			}
		}
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
		if (droidTemplates[player].count(tempId) > 0)
		{
			return droidTemplates[player][tempId];
		}
		return nullptr;
	}

	// It could be a AI template...or that of another player
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (droidTemplates[i].count(tempId) > 0)
		{
			return droidTemplates[i][tempId];
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
	ASSERT_OR_RETURN(nullptr, player < MAX_PLAYERS , "Wrong player index: %u", player);

	if (game.type != CAMPAIGN)
	{
		if (strcmp(playerName[player], "") != 0)
		{
			return (char *)&playerName[player];
		}
	}

	if (strlen(NetPlay.players[player].name) == 0)
	{
		// make up a name for this player.
		return getPlayerColourName(player);
	}
	else if (NetPlay.players[player].ai >= 0 && !NetPlay.players[player].allocated)
	{
		static char names[MAX_PLAYERS][StringSize];  // Must be static, since the getPlayerName() return value is used in tool tips... Long live the widget system.
		// Add colour to player name.
		sstrcpy(names[player], getPlayerColourName(player));
		sstrcat(names[player], "-");
		sstrcat(names[player], NetPlay.players[player].name);
		return names[player];
	}

	return NetPlay.players[player].name;
}

bool setPlayerName(int player, const char *sName)
{
	ASSERT_OR_RETURN(false, player < MAX_PLAYERS && player >= 0, "Player index (%u) out of range", player);
	sstrcpy(playerName[player], sName);
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// to determine human/computer players and responsibilities of each..
bool isHumanPlayer(int player)
{
	if (player >= MAX_PLAYERS || player < 0)
	{
		return false;	// obvious, really
	}
	return NetPlay.players[player].allocated;
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
		return NET_HOST_ONLY;	// host responsible for all AIs
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
	return playerInQuestion >= 0 && playerInQuestion < MAX_PLAYERS &&
	       (player == playerInQuestion || responsibleFor(player, playerInQuestion) || getDebugMappingStatus());
}

int scavengerSlot()
{
	// Scavengers used to always be in position 7, when scavengers were only supported in less than 8 player maps.
	// Scavengers should be in position N in N-player maps, where N ≥ 8.
	return MAX(game.maxPlayers, 7);
}

int scavengerPlayer()
{
	return game.scavengers ? scavengerSlot() : -1;
}

// ////////////////////////////////////////////////////////////////////////////
// probably temporary. Places the camera on the players 1st droid or struct.
Vector3i cameraToHome(UDWORD player, bool scroll)
{
	Vector3i res;
	UDWORD x, y;
	STRUCTURE	*psBuilding;

	for (psBuilding = apsStructLists[player]; psBuilding && (psBuilding->pStructureType->type != REF_HQ); psBuilding = psBuilding->psNext) {}

	if (psBuilding)
	{
		x = map_coord(psBuilding->pos.x);
		y = map_coord(psBuilding->pos.y);
	}
	else if (apsDroidLists[player])				// or first droid
	{
		x = map_coord(apsDroidLists[player]->pos.x);
		y =	map_coord(apsDroidLists[player]->pos.y);
	}
	else if (apsStructLists[player])							// center on first struct
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

static void sendObj(BASE_OBJECT *psObj)
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

void sendSyncRequest(int32_t req_id, int32_t x, int32_t y, BASE_OBJECT *psObj, BASE_OBJECT *psObj2)
{
	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_SYNC_REQUEST);
	NETint32_t(&req_id);
	NETint32_t(&x);
	NETint32_t(&y);
	sendObj(psObj);
	sendObj(psObj2);
	NETend();
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Recv Messages. Get a message and dispatch to relevant function.
bool recvMessage(void)
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
				recvTextMessage(queue);
				break;
			case NET_DATA_CHECK:
				recvDataCheck(queue);
				break;
			case NET_AITEXTMSG:					//multiplayer AI text message
				recvTextMessageAI(queue);
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
		case NET_OPTIONS:
			recvOptions(queue);
			break;
		case NET_PLAYER_DROPPED:				// remote player got disconnected
			{
				uint32_t player_id;

				NETbeginDecode(queue, NET_PLAYER_DROPPED);
				{
					NETuint32_t(&player_id);
				}
				NETend();

				if (whosResponsible(player_id) != queue.index && queue.index != NET_HOST_ONLY)
				{
					HandleBadParam("NET_PLAYER_DROPPED given incorrect params.", player_id, queue.index);
					break;
				}

				debug(LOG_INFO, "** player %u has dropped!", player_id);

				if (NetPlay.players[player_id].allocated)
				{
					MultiPlayerLeave(player_id);		// get rid of their stuff
					NET_InitPlayer(player_id, false);
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
				if (player_id >= MAX_PLAYERS)
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
		// FIXME: the next 4 cases might not belong here --check (we got two loops for this)
		case NET_COLOURREQUEST:
			recvColourRequest(queue);
			break;
		case NET_POSITIONREQUEST:
			recvPositionRequest(queue);
			break;
		case NET_TEAMREQUEST:
			recvTeamRequest(queue);
			break;
		case NET_READY_REQUEST:
			recvReadyRequest(queue);

			// if hosting try to start the game if everyone is ready
			if (NetPlay.isHost && multiplayPlayersReady(false))
			{
				startMultiplayerGame();
			}
			break;
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

				if (player_id == NET_HOST_ONLY)
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
					debug(LOG_ERROR, "You were kicked because %s", reason);
					setPlayerHasLost(true);
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
		ssprintf(buf, "Auto kicking player %s, invalid command received.", NetPlay.players[actual].name);
		sendTextMessage(buf, true);
		kickPlayer(actual, buf, KICK_TYPE);
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

	if (!getDebugMappingStatus() && bMultiPlayer)
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
	if (game.type == SKIRMISH)
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
bool sendResearchStatus(STRUCTURE *psBuilding, uint32_t index, uint8_t player, bool bStart)
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
		NETuint32_t(&psBuilding->id);
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
		    && ((RESEARCH_FACILITY *)psBuilding->pFunctionality)->psSubject->ref - REF_RESEARCH_START == index)
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
	if (aiCheckAlliances(selectedPlayer, player))
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
				debug(LOG_ERROR, "Player %d researching impossible topic \"%s\".", player, getName(&asResearch[index]));
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
			cancelResearch(psBuilding, ModeImmediate);
			popStatusPending(*(RESEARCH_FACILITY *)psBuilding->pFunctionality);  // Research cancellation is no longer pending, as it's actually cancelling now.
		}
	}

	if (aiCheckAlliances(selectedPlayer, player))
	{
		intAlliedResearchChanged();
		intNotifyResearchButton(prevResearchState);
	}

	return true;
}

static void printchatmsg(const char *text, int from, bool team = false)
{
	char msg[MAX_CONSOLE_STRING_LENGTH];

	sstrcpy(msg, NetPlay.players[from].name);		// name
	sstrcat(msg, ": ");					// seperator
	sstrcat(msg, text);					// add message
	addConsoleMessage(msg, DEFAULT_JUSTIFY, from, team);		// display
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Text Messaging between team.
void sendTeamMessage(const char *pStr, uint32_t from)
{
	char	display[MAX_CONSOLE_STRING_LENGTH];
	bool team = true;

	sstrcpy(display, pStr);
	// This is for local display
	if (from == selectedPlayer)
	{
		printchatmsg(display, from, team);
	}

	for (int i = 0; i < game.maxPlayers; i++)
	{
		if (i != from && aiCheckAlliances(from, i))
		{
			if (i == selectedPlayer)
			{
				printchatmsg(display, from); // also display it
			}
			if (isHumanPlayer(i))
			{
				NETbeginEncode(NETnetQueue(i), NET_TEXTMSG);
				NETuint32_t(&from);				// who this msg is from
				NETbool(&team);
				NETstring(display, MAX_CONSOLE_STRING_LENGTH);	// the message to send
				NETend();
			}
			else if (myResponsibility(i))
			{
				msgStackPush(CALL_AI_MSG, from, i, display, -1, -1, nullptr);
				triggerEventChat(from, i, display);
			}
			else	//also send to AIs now (non-humans), needed for AI
			{
				sendAIMessage(display, from, i);
			}
		}
	}
}

// Text Messaging between players. proceed string with players to send to.
// eg "123hi there" sends "hi there" to players 1,2 and 3.
bool sendTextMessage(const char *pStr, bool all, uint32_t from)
{
	bool				normal = true, team = false;
	bool				sendto[MAX_PLAYERS];
	int					posTable[MAX_PLAYERS];
	UDWORD				i;
	char				display[MAX_CONSOLE_STRING_LENGTH];
	char				msg[MAX_CONSOLE_STRING_LENGTH];
	char				*curStr = (char *)pStr;

	memset(display, 0x0, sizeof(display));	//clear buffer
	memset(msg, 0x0, sizeof(msg));		//clear buffer
	memset(sendto, 0x0, sizeof(sendto));		//clear private flag
	memset(posTable, 0x0, sizeof(posTable));		//clear buffer
	sstrcpy(msg, curStr);

	// This is for local display
	if (from == selectedPlayer)
	{
		printchatmsg(curStr, from);
	}

	triggerEventChat(from, from, pStr); // send to self

	if (!all)
	{
		// create a position table
		for (i = 0; i < game.maxPlayers; i++)
		{
			posTable[NetPlay.players[i].position] = i;
		}

		if (curStr[0] == '.')
		{
			curStr++;
			for (i = 0; i < game.maxPlayers; i++)
			{
				if (i != from && aiCheckAlliances(from, i))
				{
					sendto[i] = true;
				}
			}
			normal = false;
			if (!all)
			{
				sstrcpy(display, _("(allies"));
			}
		}
		for (; curStr[0] >= '0' && curStr[0] <= '9'; ++curStr)  // for each 0..9 numeric char encountered
		{
			i = posTable[curStr[0] - '0'];
			if (normal)
			{
				sstrcpy(display, _("(private to "));
			}
			else
			{
				sstrcat(display, ", ");
			}
			if ((isHumanPlayer(i) || (game.type == SKIRMISH && i < game.maxPlayers && game.skDiff[i])))
			{
				sstrcat(display, getPlayerName(posTable[curStr[0] - '0']));
				sendto[i] = true;
			}
			else
			{
				sstrcat(display, _("[invalid]"));
			}
			normal = false;
		}

		if (!normal)	// lets user know it is a private message
		{
			if (curStr[0] == ' ')
			{
				curStr++;
			}
			sstrcat(display, ") ");
			sstrcat(display, curStr);
		}
	}

	if (all)	//broadcast
	{
		NETbeginEncode(NETbroadcastQueue(), NET_TEXTMSG);
		NETuint32_t(&from);		// who this msg is from
		NETbool(&team);			// team specific?
		NETstring(msg, MAX_CONSOLE_STRING_LENGTH);	// the message to send
		NETend();
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (i == selectedPlayer && from != i)
			{
				printchatmsg(display, from); // also display it
			}
			if (i != from && !isHumanPlayer(i) && myResponsibility(i))
			{
				msgStackPush(CALL_AI_MSG, from, i, msg, -1, -1, nullptr);
				triggerEventChat(from, i, msg);
			}
			else if (i != from && !isHumanPlayer(i) && !myResponsibility(i))
			{
				sendAIMessage(msg, from, i);
			}
		}
	}
	else if (normal)
	{
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (i != from && openchannels[i])
			{
				if (i == selectedPlayer)
				{
					printchatmsg(curStr, from); // also display it
				}
				if (isHumanPlayer(i))
				{
					NETbeginEncode(NETnetQueue(i), NET_TEXTMSG);
					NETuint32_t(&from);		// who this msg is from
					NETbool(&team);			// team specific?
					NETstring(msg, MAX_CONSOLE_STRING_LENGTH);	// the message to send
					NETend();
				}
				else if (myResponsibility(i))
				{
					msgStackPush(CALL_AI_MSG, from, i, msg, -1, -1, nullptr);
					triggerEventChat(from, i, msg);
				}
				else	// send to AIs on different host
				{
					sendAIMessage(msg, from, i);
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
				if (i == selectedPlayer)
				{
					printchatmsg(display, from); // also display it
				}
				if (isHumanPlayer(i))
				{
					NETbeginEncode(NETnetQueue(i), NET_TEXTMSG);
					NETuint32_t(&from);				// who this msg is from
					NETbool(&team); // team specific?
					NETstring(display, MAX_CONSOLE_STRING_LENGTH);	// the message to send
					NETend();
				}
				else if (myResponsibility(i))
				{
					msgStackPush(CALL_AI_MSG, from, i, curStr, -1, -1, nullptr);
					triggerEventChat(from, i, curStr);
				}
				else	//also send to AIs now (non-humans), needed for AI
				{
					sendAIMessage(curStr, from, i);
				}
			}
		}
	}

	return true;
}

void printConsoleNameChange(const char *oldName, const char *newName)
{
	char msg[MAX_CONSOLE_STRING_LENGTH];

	// Player changed name.
	sstrcpy(msg, oldName);                               // Old name.
	sstrcat(msg, " → ");                                 // Separator
	sstrcat(msg, newName);  // New name.

	addConsoleMessage(msg, DEFAULT_JUSTIFY, selectedPlayer);  // display
}


//AI multiplayer message, send from a certain player index to another player index
static bool sendAIMessage(char *pStr, UDWORD player, UDWORD to)
{
	UDWORD	sendPlayer;

	if (!ingame.localOptionsReceived)
	{
		return true;
	}

	// find machine that is hosting this human or AI
	sendPlayer = whosResponsible(to);

	if (sendPlayer >= MAX_PLAYERS)
	{
		debug(LOG_ERROR, "sendAIMessage() - sendPlayer >= MAX_PLAYERS");
		return false;
	}

	if (!isHumanPlayer(sendPlayer))		//NETsend can't send to non-humans
	{
		debug(LOG_ERROR, "sendAIMessage() - player is not human.");
		return false;
	}

	//send to the player who is hosting 'to' player (might be himself if human and not AI)
	NETbeginEncode(NETnetQueue(sendPlayer), NET_AITEXTMSG);
	NETuint32_t(&player);			//save the actual sender
	//save the actual player that is to get this msg on the source machine (source can host many AIs)
	NETuint32_t(&to);				//save the actual receiver (might not be the same as the one we are actually sending to, in case of AIs)
	NETstring(pStr, MAX_CONSOLE_STRING_LENGTH);		// copy message in.
	NETend();
	return true;
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
		debug(LOG_ERROR, "sendAIMessage() - whosResponsible() failed.");
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

	// const_cast: need to cast away constness because of the const-incorrectness of NETstring (const-incorrect when sending/encoding a packet)
	NETstring((char *)pStr, MAX_CONSOLE_STRING_LENGTH); // copy message in.
	NETend();

	return true;
}

// Write a message to the console.
bool recvTextMessage(NETQUEUE queue)
{
	UDWORD	playerIndex;
	char	msg[MAX_CONSOLE_STRING_LENGTH];
	char newmsg[MAX_CONSOLE_STRING_LENGTH];
	bool team = false;

	memset(msg, 0x0, sizeof(msg));
	memset(newmsg, 0x0, sizeof(newmsg));

	NETbeginDecode(queue, NET_TEXTMSG);
	NETuint32_t(&playerIndex);	// Who this msg is from
	NETbool(&team);	// team specific?
	NETstring(newmsg, MAX_CONSOLE_STRING_LENGTH);	// The message to receive
	NETend();

	if (whosResponsible(playerIndex) != queue.index)
	{
		playerIndex = queue.index;  // Fix corrupted playerIndex.
	}

	if (playerIndex >= MAX_PLAYERS || (!NetPlay.players[playerIndex].allocated && NetPlay.players[playerIndex].ai == AI_OPEN))
	{
		return false;
	}

	sstrcpy(msg, NetPlay.players[playerIndex].name);
	// Seperator
	sstrcat(msg, ": ");
	// Add message
	sstrcat(msg, newmsg);

	addConsoleMessage(msg, DEFAULT_JUSTIFY, playerIndex, team);

	// Multiplayer message callback
	// Received a console message from a player, save
	MultiMsgPlayerFrom = playerIndex;
	MultiMsgPlayerTo = selectedPlayer;

	sstrcpy(MultiplayMsg, newmsg);
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

	//Received a console message from a player callback
	//store and call later
	//-------------------------------------------------
	if (!msgStackPush(CALL_AI_MSG, sender, receiver, msg, -1, -1, nullptr))
	{
		debug(LOG_ERROR, "recvTextMessageAI() - msgStackPush - stack failed");
		return false;
	}

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Features

// send a destruct feature message.
bool SendDestroyFeature(FEATURE *pF)
{
	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_DEBUG_REMOVE_FEATURE);
	NETuint32_t(&pF->id);
	return NETend();
}

// process a destroy feature msg.
bool recvDestroyFeature(NETQUEUE queue)
{
	FEATURE *pF;
	uint32_t	id;

	NETbeginDecode(queue, GAME_DEBUG_REMOVE_FEATURE);
	NETuint32_t(&id);
	NETend();

	if (!getDebugMappingStatus() && bMultiPlayer)
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

	debug(LOG_FEATURE, "p%d feature id %d destroyed (%s)", pF->player, pF->id, getName(pF->psStats));
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

	auto &files = NetPlay.players[player].wzFiles;
	if (std::any_of(files.begin(), files.end(), [&](WZFile const &file) { return file.hash == hash; }))
	{
		return true;  // Already sending this file, do nothing.
	}

	netPlayersUpdated = true;  // Show download icon on player.

	std::string filename;
	if (hash == game.hash)
	{
		addConsoleMessage(_("Map was requested: SENDING MAP!"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);

		LEVEL_DATASET *mapData = levFindDataSet(game.map, &game.hash);
		filename = mapData->realFileName;
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
		debug(LOG_ERROR, "Failed to open %s for reading: %s", filename.c_str(), PHYSFS_getLastError());
		debug(LOG_FATAL, "You have a map (%s) that can't be located.\n\nMake sure it is in the correct directory and or format! (No map packs!)", filename.c_str());
		// NOTE: if we get here, then the game is basically over, The host can't send the file for whatever reason...
		// Which also means, that we can't continue.
		debug(LOG_NET, "***Host has a file issue, and is being forced to quit!***");
		NETbeginEncode(NETbroadcastQueue(), NET_HOST_DROPPED);
		NETend();
		abort();
	}

	debug(LOG_INFO, "File is valid, sending [directory: %s] %s to client %u", PHYSFS_getRealDir(filename.c_str()), filename.c_str(), player);

	PHYSFS_sint64 fileSize_64 = PHYSFS_fileLength(pFileHandle);
	ASSERT_OR_RETURN(false, fileSize_64 <= 0xFFFFFFFF, "File too big!");

	// Schedule file to be sent.
	files.emplace_back(pFileHandle, hash, fileSize_64);

	return true;
}

// Continue sending maps and mods.
void sendMap()
{
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		auto &files = NetPlay.players[i].wzFiles;
		for (auto &file : files)
		{
			int done = NETsendFile(file, i);
			if (done == 100)
			{
				netPlayersUpdated = true;  // Remove download icon from player.
				addConsoleMessage(_("FILE SENT!"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
				debug(LOG_INFO, "=== File has been sent to player %d ===", i);
			}
		}
		files.erase(std::remove_if(files.begin(), files.end(), [](WZFile const &file) { return file.handle == nullptr; }), files.end());
	}
}

// Another player is broadcasting a map, recv a chunk. Returns false if not yet done.
bool recvMapFileData(NETQUEUE queue)
{
	NETrecvFile(queue);
	if (NetPlay.wzFiles.empty())
	{
		netPlayersUpdated = true;  // Remove download icon from ourselves.
		addConsoleMessage("MAP DOWNLOADED!", DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
		sendTextMessage("MAP DOWNLOADED", true);					//send
		debug(LOG_INFO, "=== File has been received. ===");

		// clear out the old level list.
		levShutDown();
		levInitialise();
		rebuildSearchPath(mod_multiplay, true);	// MUST rebuild search path for the new maps we just got!
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
				ssprintf(buf, _("Warning, this is a map-mod, it could alter normal gameplay."));
			}
			else
			{
				ssprintf(buf, _("Warning, HOST has altered the game code, and can't be trusted!"));
			}
			addConsoleMessage(buf,  DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
			game.isMapMod = true;
			widgReveal(psWScreen, MULTIOP_MAP_MOD);
		}

		loadMapPreview(false);
		return true;
	}

	return false;
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

	if (msgStackPos + 1 >= MAX_MSG_STACK)
	{
		debug(LOG_ERROR, "msgStackPush() - stack full");
		return false;
	}

	//make point to the last valid element
	msgStackPos++;

	//remember values
	msgPlFrom[msgStackPos] = plFrom;
	msgPlTo[msgStackPos] = plTo;

	callbackType[msgStackPos] = CBtype;
	locx[msgStackPos] = x;
	locy[msgStackPos] = y;

	sstrcpy(msgStr[msgStackPos], tStr);

	msgDroid[msgStackPos] = psDroid;

	return true;
}

bool isMsgStackEmpty(void)
{
	if (msgStackPos <= (-1))
	{
		return true;
	}
	return false;
}

bool msgStackGetFrom(SDWORD  *psVal)
{
	if (msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackGetFrom: msgStackPos < 0");
		return false;
	}

	*psVal = msgPlFrom[0];

	return true;
}

bool msgStackGetTo(SDWORD  *psVal)
{
	if (msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackGetTo: msgStackPos < 0");
		return false;
	}

	*psVal = msgPlTo[0];

	return true;
}

static bool msgStackGetCallbackType(SDWORD  *psVal)
{
	if (msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackGetCallbackType: msgStackPos < 0");
		return false;
	}

	*psVal = callbackType[0];

	return true;
}

static bool msgStackGetXY(SDWORD  *psValx, SDWORD  *psValy)
{
	if (msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackGetXY: msgStackPos < 0");
		return false;
	}

	*psValx = locx[0];
	*psValy = locy[0];

	return true;
}


bool msgStackGetMsg(char  *psVal)
{
	if (msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackGetMsg: msgStackPos < 0");
		return false;
	}

	strcpy(psVal, msgStr[0]);
	//*psVal = msgPlTo[msgStackPos];

	return true;
}

static bool msgStackSort(void)
{
	SDWORD i;

	//go through all-1 elements (bottom-top)
	for (i = 0; i < msgStackPos; i++)
	{
		msgPlFrom[i] = msgPlFrom[i + 1];
		msgPlTo[i] = msgPlTo[i + 1];

		callbackType[i] = callbackType[i + 1];
		locx[i] = locx[i + 1];
		locy[i] = locy[i + 1];

		sstrcpy(msgStr[i], msgStr[i + 1]);
	}

	//erase top element
	msgPlFrom[msgStackPos] = -2;
	msgPlTo[msgStackPos] = -2;

	callbackType[msgStackPos] = -2;
	locx[msgStackPos] = -2;
	locy[msgStackPos] = -2;

	sstrcpy(msgStr[msgStackPos], "ERROR char!!!!!!!!");

	msgStackPos--;		//since removed the top element

	return true;
}

bool msgStackPop(void)
{
	debug(LOG_WZ, "msgStackPop: stack size %d", msgStackPos);

	if (msgStackPos < 0 || msgStackPos > MAX_MSG_STACK)
	{
		debug(LOG_ERROR, "msgStackPop: wrong msgStackPos index: %d", msgStackPos);
		return false;
	}

	return msgStackSort();		//move all elements 1 pos lower
}

bool msgStackGetDroid(DROID **ppsDroid)
{
	if (msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackGetDroid: msgStackPos < 0");
		return false;
	}

	*ppsDroid = msgDroid[0];

	return true;
}

SDWORD msgStackGetCount(void)
{
	return msgStackPos + 1;
}

bool msgStackFireTop(void)
{
	SDWORD		_callbackType;
	char		msg[255];

	if (msgStackPos < 0)
	{
		debug(LOG_ERROR, "msgStackFireTop: msgStackPos < 0");
		return false;
	}

	if (!msgStackGetCallbackType(&_callbackType))
	{
		return false;
	}

	switch (_callbackType)
	{
	case CALL_VIDEO_QUIT:
		debug(LOG_SCRIPT, "msgStackFireTop: popped CALL_VIDEO_QUIT");
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_VIDEO_QUIT);
		break;

	case CALL_DORDER_STOP:
		ASSERT(false, "CALL_DORDER_STOP is currently disabled");

		debug(LOG_SCRIPT, "msgStackFireTop: popped CALL_DORDER_STOP");

		if (!msgStackGetDroid(&psScrCBOrderDroid))
		{
			return false;
		}

		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DORDER_STOP);
		break;

	case CALL_BEACON:

		if (!msgStackGetXY(&beaconX, &beaconY))
		{
			return false;
		}

		if (!msgStackGetFrom(&MultiMsgPlayerFrom))
		{
			return false;
		}

		if (!msgStackGetTo(&MultiMsgPlayerTo))
		{
			return false;
		}

		if (!msgStackGetMsg(msg))
		{
			return false;
		}

		sstrcpy(MultiplayMsg, msg);

		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_BEACON);
		break;

	case CALL_AI_MSG:
		if (!msgStackGetFrom(&MultiMsgPlayerFrom))
		{
			return false;
		}

		if (!msgStackGetTo(&MultiMsgPlayerTo))
		{
			return false;
		}

		if (!msgStackGetMsg(msg))
		{
			return false;
		}

		sstrcpy(MultiplayMsg, msg);

		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_AI_MSG);
		break;

	default:
		debug(LOG_ERROR, "msgStackFireTop: unknown callback type");
		return false;
		break;
	}

	if (!msgStackPop())
	{
		return false;
	}

	return true;
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
void resetReadyStatus(bool bSendOptions)
{
	// notify all clients if needed
	if (bSendOptions)
	{
		sendOptions();
	}
	netPlayersUpdated = true;
}
