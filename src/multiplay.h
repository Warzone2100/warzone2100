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
 *  Alex Lee 1997/98, Pumpkin Studios, Bath.
 */

#ifndef __INCLUDED_SRC_MULTIPLAY_H__
#define __INCLUDED_SRC_MULTIPLAY_H__

#include "lib/framework/frame.h"
#include "lib/framework/types.h"
#include "lib/framework/vector.h"
#include "lib/framework/crc.h"
#include "orderdef.h"
#include "stringdef.h"
#include "messagedef.h"
#include <vector>
#include <string>

class DROID_GROUP;
struct BASE_OBJECT;
struct DROID;
struct DROID_TEMPLATE;
struct FEATURE;
struct INITIAL_DROID_ORDERS;
struct STRUCTURE;

// /////////////////////////////////////////////////////////////////////////////////////////////////
// Game Options Structure. Enough info to completely describe the static stuff in a multiplayer game.
// Also used for skirmish games. And sometimes for campaign. Should be moved somewhere else.
struct MULTIPLAYERGAME
{
	uint8_t		type;						// DMATCH/CAMPAIGN/SKIRMISH/TEAMPLAY etc...
	bool		scavengers;					// whether scavengers are on or off
	char		map[128];					// name of multiplayer map being used.
	uint8_t		maxPlayers;					// max players to allow
	char		name[128];					// game name   (to be used)
	Sha256      hash;                       ///< Hash of map file. Zero if built in.
	std::vector<Sha256> modHashes;          // Hash of mods.
	uint32_t    power;						// power level for arena game
	uint8_t		base;						// clean/base/base&defence
	uint8_t		alliance;					// no/yes/locked
	uint8_t		skDiff[MAX_PLAYERS];		// skirmish game difficulty settings. 0x0=OFF 0xff=HUMAN
	bool		mapHasScavengers;
	bool		isMapMod;					// if a map has mods
	uint32_t	techLevel;					// what technology level is being used
};

struct MULTISTRUCTLIMITS
{
	uint32_t        id;
	uint32_t        limit;
};

// info used inside games.
struct MULTIPLAYERINGAME
{
	UDWORD				PingTimes[MAX_PLAYERS];				// store for pings.
	bool				localOptionsReceived;				// used to show if we have game options yet..
	bool				localJoiningInProgress;				// used before we know our player number.
	bool				JoiningInProgress[MAX_PLAYERS];
	bool				DataIntegrity[MAX_PLAYERS];
	bool				bHostSetup;
	int32_t				TimeEveryoneIsInGame;
	bool				isAllPlayersDataOK;
	UDWORD				startTime;
	std::vector<MULTISTRUCTLIMITS> structureLimits;
	uint8_t				flags;  ///< Bitmask, shows which structures are disabled.
#define MPFLAGS_NO_TANKS	0x01  		///< Flag for tanks disabled
#define MPFLAGS_NO_CYBORGS	0x02  		///< Flag for cyborgs disabled
#define MPFLAGS_NO_VTOLS	0x04  		///< Flag for VTOLs disabled
#define MPFLAGS_NO_UPLINK	0x08  		///< Flag for Satellite Uplink disabled
#define MPFLAGS_NO_LASSAT	0x10  		///< Flag for Laser Satellite Command Post disabled
#define MPFLAGS_FORCELIMITS	0x20  		///< Flag to force structure limits
#define MPFLAGS_MAX		0x3f
	SDWORD		skScores[MAX_PLAYERS][2];			// score+kills for local skirmish players.
	char		phrases[5][255];					// 5 favourite text messages.
};

enum STRUCTURE_INFO
{
	STRUCTUREINFO_MANUFACTURE,
	STRUCTUREINFO_CANCELPRODUCTION,
	STRUCTUREINFO_HOLDPRODUCTION,
	STRUCTUREINFO_RELEASEPRODUCTION,
	STRUCTUREINFO_HOLDRESEARCH,
	STRUCTUREINFO_RELEASERESEARCH
};

// ////////////////////////////////////////////////////////////////////////////
// Game Options and stats.
extern MULTIPLAYERGAME		game;						// the game description.
extern MULTIPLAYERINGAME	ingame;						// the game description.

extern bool bMultiPlayer;				// true when more than 1 player.
extern bool bMultiMessages;				// == bMultiPlayer unless multi messages are disabled
extern bool openchannels[MAX_PLAYERS];
extern UBYTE bDisplayMultiJoiningStatus;	// draw load progress?

// ////////////////////////////////////////////////////////////////////////////
// defines

// Bitmask for client lobby

#define NO_VTOLS  1
#define NO_TANKS 2
#define NO_BORGS 4


#define ANYPLAYER				99

#define CAMP_CLEAN				0			// campaign subtypes
#define CAMP_BASE				1
#define CAMP_WALLS				2

#define PING_LIMIT                              4000                    // If ping is bigger than this, then worry and panic, and don't even try showing the ping.

#define LEV_LOW					0
#define LEV_MED					1
#define LEV_HI					2

#define DIFF_SLIDER_STOPS		20			//max number of stops for the multiplayer difficulty slider

#define MAX_KICK_REASON			80			// max array size for the reason your kicking someone
// functions

WZ_DECL_WARN_UNUSED_RESULT BASE_OBJECT		*IdToPointer(UDWORD id, UDWORD player);
WZ_DECL_WARN_UNUSED_RESULT STRUCTURE		*IdToStruct(UDWORD id, UDWORD player);
WZ_DECL_WARN_UNUSED_RESULT DROID			*IdToDroid(UDWORD id, UDWORD player);
WZ_DECL_WARN_UNUSED_RESULT DROID			*IdToMissionDroid(UDWORD id, UDWORD player);
WZ_DECL_WARN_UNUSED_RESULT FEATURE		*IdToFeature(UDWORD id, UDWORD player);
WZ_DECL_WARN_UNUSED_RESULT DROID_TEMPLATE	*IdToTemplate(UDWORD tempId, UDWORD player);

const char *getPlayerName(int player);
bool setPlayerName(int player, const char *sName);
const char *getPlayerColourName(int player);
bool isHumanPlayer(int player);				//to tell if the player is a computer or not.
bool myResponsibility(int player);
bool responsibleFor(int player, int playerinquestion);
int whosResponsible(int player);
bool canGiveOrdersFor(int player, int playerInQuestion);
int scavengerSlot();    // Returns the player number that scavengers would have if they were enabled.
int scavengerPlayer();  // Returns the player number that the scavengers have, or -1 if disabled.
Vector3i cameraToHome(UDWORD player, bool scroll);

bool multiPlayerLoop();							// for loop.c

bool recvMessage();
bool SendResearch(uint8_t player, uint32_t index, bool trigger);
bool SendDestroyFeature(FEATURE *pF);					// send a destruct feature message.
bool sendTextMessage(const char *pStr, bool cast, uint32_t from = selectedPlayer);	// send a text message
void sendTeamMessage(const char *pStr, uint32_t from = selectedPlayer);	// send a team chat message
void printConsoleNameChange(const char *oldName, const char *newName);  ///< Print message to console saying a name changed.

void turnOffMultiMsg(bool bDoit);

void sendMap();
bool multiplayerWinSequence(bool firstCall);

/////////////////////////////////////////////////////////
// definitions of functions in multiplay's other c files.

// Buildings . multistruct
bool SendDestroyStructure(STRUCTURE *s);
bool SendBuildFinished(STRUCTURE *psStruct);
bool sendLasSat(UBYTE player, STRUCTURE *psStruct, BASE_OBJECT *psObj);
void sendStructureInfo(STRUCTURE *psStruct, STRUCTURE_INFO structureInfo, DROID_TEMPLATE *psTempl);

// droids . multibot
bool SendDroid(DROID_TEMPLATE *pTemplate, uint32_t x, uint32_t y, uint8_t player, uint32_t id, const INITIAL_DROID_ORDERS *initialOrders);
bool SendDestroyDroid(const DROID *psDroid);
void sendQueuedDroidInfo();  ///< Actually sends the droid orders which were queued by SendDroidInfo.
void sendDroidInfo(DROID *psDroid, DroidOrder const &order, bool add);
bool SendCmdGroup(DROID_GROUP *psGroup, UWORD x, UWORD y, BASE_OBJECT *psObj);

bool sendDroidSecondary(const DROID *psDroid, SECONDARY_ORDER sec, SECONDARY_STATE state);
bool sendDroidDisembark(DROID const *psTransporter, DROID const *psDroid);

// Startup. mulitopt
bool multiShutdown();
bool sendLeavingMsg();

bool hostCampaign(char *sGame, char *sPlayer);
struct JoinConnectionDescription
{
public:
	JoinConnectionDescription() { }
	JoinConnectionDescription(const std::string& host, uint32_t port)
	: host(host)
	, port(port)
	{ }
public:
	std::string host;
	uint32_t port = 0;
};
std::vector<JoinConnectionDescription> findLobbyGame(const std::string& lobbyAddress, unsigned int lobbyPort, uint32_t lobbyGameId);
enum class JoinGameResult {
	FAILED,
	JOINED,
	PENDING_PASSWORD
};
JoinGameResult joinGame(const char *host, uint32_t port);
JoinGameResult joinGame(const std::vector<JoinConnectionDescription>& connection_list);
void playerResponding();
bool multiGameInit();
bool multiGameShutdown();

// syncing.
bool sendScoreCheck();							//score check only(frontend)
bool sendPing();							// allow game to request pings.
void HandleBadParam(const char *msg, const int from, const int actual);
// multijoin
bool sendResearchStatus(STRUCTURE *psBuilding, UDWORD index, UBYTE player, bool bStart);

void displayAIMessage(char *pStr, SDWORD from, SDWORD to);  //make AI process a message


bool sendBeacon(int32_t locX, int32_t locY, int32_t forPlayer, int32_t sender, const char *pStr);

bool multiplayPlayersReady(bool bNotifyStatus);
void startMultiplayerGame();
void resetReadyStatus(bool bSendOptions);

STRUCTURE *findResearchingFacilityByResearchIndex(unsigned player, unsigned index);

void sendSyncRequest(int32_t req_id, int32_t x, int32_t y, BASE_OBJECT *psObj, BASE_OBJECT *psObj2);


bool sendBeaconToPlayer(SDWORD locX, SDWORD locY, SDWORD forPlayer, SDWORD sender, const char *beaconMsg);
MESSAGE *findBeaconMsg(UDWORD player, SDWORD sender);
VIEWDATA *CreateBeaconViewData(SDWORD sender, UDWORD LocX, UDWORD LocY);

#endif // __INCLUDED_SRC_MULTIPLAY_H__
