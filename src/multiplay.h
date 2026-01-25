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
#include "lib/netplay/nettypes.h"
#include "multiplaydefs.h"
#include "orderdef.h"
#include "stringdef.h"
#include "messagedef.h"
#include "levels.h"
#include "console.h"
#include "multirecv.h"
#include "objmem.h"
#include <vector>
#include <string>
#include <chrono>
#include <array>

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

#include <nlohmann/json_fwd.hpp>

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
	LEVEL_TYPE	type;						// DMATCH/CAMPAIGN/SKIRMISH/TEAMPLAY etc...
	uint8_t		scavengers;					// ultimate scavengers, scavengers, no scavengers
	char		map[128];					// name of multiplayer map being used.
	uint8_t		maxPlayers;					// max players to allow
	char		name[128];					// game name   (to be used)
	Sha256      hash;                       ///< Hash of map file. Zero if built in.
	std::vector<Sha256> modHashes;          // Hash of mods.
	uint32_t    power;						// power level for arena game
	uint8_t		base;						// clean/base/base&defence
	uint8_t		alliance;					// no/yes/locked
	bool		mapHasScavengers;
	bool		isMapMod;					// if a map has mods
	bool        isRandom;                   // If a map is non-static.
	uint32_t	techLevel;					// what technology level is being used
	uint32_t	inactivityMinutes;			// The number of minutes without active play before a player should be considered "inactive". (0 = disable activity alerts)
	uint32_t	gameTimeLimitMinutes;		// The number of minutes before the game automatically ends (0 = disable time limit)
	PLAYER_LEAVE_MODE	playerLeaveMode;	// The behavior used for when players leave a game
	BLIND_MODE	blindMode = BLIND_MODE::NONE;

	// NOTE: If adding to this struct, a lot of things probably require changing
	// (send/recvOptions? loadMainFile/writeMainFile? to/from_json in multiint.h.cpp?)
};

struct MULTISTRUCTLIMITS
{
	uint32_t        id;
	uint32_t        limit;

	inline bool operator==(const MULTISTRUCTLIMITS& a) const
	{
		return (id == a.id && limit == a.limit);
	}
};

// The side we are *configured* as. Used to indicate whether we are the server or the client. Note that when
// playing singleplayer, we are running as a host, not a client. Values are booleans as this replaces the old
// `bool bHostSetup`.
enum class InGameSide : bool {
	HOST_OR_SINGLEPLAYER = true,
	MULTIPLAYER_CLIENT = false
};

// info used inside games.
struct MULTIPLAYERINGAME
{
	UDWORD				PingTimes[MAX_CONNECTED_PLAYERS];				// store for pings.
	int 				LagCounter[MAX_CONNECTED_PLAYERS];
	int 				DesyncCounter[MAX_CONNECTED_PLAYERS];
	bool				VerifiedIdentity[MAX_CONNECTED_PLAYERS];		// if the multistats identity has been verified.
	bool				localOptionsReceived;							// used to show if we have game options yet..
	bool				localJoiningInProgress;							// used before we know our player number.
	bool				JoiningInProgress[MAX_CONNECTED_PLAYERS];
	bool				PendingDisconnect[MAX_CONNECTED_PLAYERS];		// used to mark players who have disconnected after the game has "fired up" but before it actually starts (i.e. pre-game / loading phase) - UI only
	bool				DataIntegrity[MAX_CONNECTED_PLAYERS];
	std::array<bool, MAX_CONNECTED_PLAYERS> hostChatPermissions;		// the *host*-set free chat permission status for players (true if free chat is allowed, false if only Quick Chat is allowed)

	// Used by the host to track players lingering in lobby without checking ready
	std::array<optional<std::chrono::steady_clock::time_point>, MAX_CONNECTED_PLAYERS> joinTimes;
	std::array<optional<std::chrono::steady_clock::time_point>, MAX_CONNECTED_PLAYERS> lastReadyTimes;
	std::array<optional<std::chrono::steady_clock::time_point>, MAX_CONNECTED_PLAYERS> lastNotReadyTimes;
	std::array<uint64_t, MAX_CONNECTED_PLAYERS> secondsNotReady; // updated when player status switches to ready
	std::array<optional<uint32_t>, MAX_CONNECTED_PLAYERS> playerLeftGameTime; // records when the player leaves the game (as a player)
	//

	InGameSide			side;
	optional<int32_t>	TimeEveryoneIsInGame;
	bool				isAllPlayersDataOK;
	std::chrono::steady_clock::time_point startTime;
	optional<std::chrono::steady_clock::time_point> endTime;
	std::chrono::steady_clock::time_point lastLagCheck;
	std::chrono::steady_clock::time_point lastDesyncCheck;
	std::chrono::steady_clock::time_point lastNotReadyCheck;
	optional<std::chrono::steady_clock::time_point> lastSentPlayerDataCheck2[MAX_CONNECTED_PLAYERS] = {};
	std::chrono::steady_clock::time_point lastPlayerDataCheck2;
	bool				muteChat[MAX_CONNECTED_PLAYERS] = {false};		// the local client-set mute status for this player
	std::vector<MULTISTRUCTLIMITS> structureLimits;
	uint8_t				flags;  ///< Bitmask, shows which structures are disabled.
#define MPFLAGS_NO_TANKS	0x01  		///< Flag for tanks disabled
#define MPFLAGS_NO_CYBORGS	0x02  		///< Flag for cyborgs disabled
#define MPFLAGS_NO_VTOLS	0x04  		///< Flag for VTOLs disabled
#define MPFLAGS_NO_UPLINK	0x08  		///< Flag for Satellite Uplink disabled
#define MPFLAGS_NO_LASSAT	0x10  		///< Flag for Laser Satellite Command Post disabled
#define MPFLAGS_FORCELIMITS	0x20  		///< Flag to force structure limits
#define MPFLAGS_MAX		0x3f
	std::vector<MULTISTRUCTLIMITS> lastAppliedStructureLimits;	// a bit of a hack to retain the structureLimits used when loading / starting a game
};

struct NetworkTextMessage
{
	/**
	 * Sender can be a player index, SYSTEM_MESSAGE or NOTIFY_MESSAGE.
	 **/
	int32_t sender;
	char text[MAX_CONSOLE_STRING_LENGTH];
	bool teamSpecific = false;

	NetworkTextMessage() {}
	NetworkTextMessage(int32_t messageSender, char const *messageText);
	void enqueue(NETQUEUE queue);
	bool receive(NETQUEUE queue);
	bool decode(const NetMessage& message, uint8_t senderIdx);
private:
	bool decode(MessageReader& r, uint8_t senderIdx);
};

optional<NetworkTextMessage> decodeSpecInGameTextMessage(MessageReader& r, uint8_t senderIdx);

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
extern bool openchannels[MAX_CONNECTED_PLAYERS]; // outgoing message channels (i.e. who you are willing to send messages to)
extern UBYTE bDisplayMultiJoiningStatus;	// draw load progress?

#define RUN_ONLY_ON_SIDE(_side) \
	if (ingame.side != _side) \
	{ \
		return; \
	}


// ////////////////////////////////////////////////////////////////////////////
// defines

// Bitmask for client lobby

#define NO_VTOLS  1
#define NO_TANKS 2
#define NO_BORGS 4


#define ANYPLAYER				99

enum ScavType
{
	NO_SCAVENGERS = 0,
	SCAVENGERS = 1,
	ULTIMATE_SCAVENGERS = 2
};
constexpr ScavType SCAV_TYPE_MAX = ScavType::ULTIMATE_SCAVENGERS;

// campaign subtypes
enum CampType
{
	CAMP_CLEAN = 0,
	CAMP_BASE = 1,
	CAMP_WALLS = 2
};
constexpr CampType CAMP_TYPE_MAX = CampType::CAMP_WALLS;

#define PING_LIMIT				4000		// If ping is bigger than this, then worry and panic, and don't even try showing the ping.

enum PowerSetting
{
	LEV_LOW = 0,
	LEV_MED = 1,
	LEV_HI = 2
};
constexpr PowerSetting POWER_SETTING_MAX = PowerSetting::LEV_HI;

enum TechLevel
{
	TECH_1 = 1,
	TECH_2 = 2,
	TECH_3 = 3,
	TECH_4 = 4
};
constexpr TechLevel TECH_LEVEL_MIN = TechLevel::TECH_1;
constexpr TechLevel TECH_LEVEL_MAX = TechLevel::TECH_4;

#define MAX_KICK_REASON			1024		// max array size for the reason your kicking someone
#define MAX_JOIN_REJECT_REASON	2048		// max array size for the reason a join was rejected (custom host message provided by wzcmd interface)

// functions

WZ_DECL_WARN_UNUSED_RESULT BASE_OBJECT		*IdToPointer(UDWORD id, UDWORD player);
WZ_DECL_WARN_UNUSED_RESULT STRUCTURE		*IdToStruct(UDWORD id, UDWORD player);
WZ_DECL_WARN_UNUSED_RESULT DROID			*IdToDroid(UDWORD id, UDWORD player);
WZ_DECL_WARN_UNUSED_RESULT DROID			*IdToMissionDroid(UDWORD id, UDWORD player);
WZ_DECL_WARN_UNUSED_RESULT FEATURE		*IdToFeature(UDWORD id, UDWORD player);
WZ_DECL_WARN_UNUSED_RESULT DROID_TEMPLATE	*IdToTemplate(UDWORD tempId, UDWORD player);

const char *getPlayerName(uint32_t player, bool treatAsNonHost = false);
bool setPlayerName(int player, const char *sName);
void clearPlayerName(unsigned int player);
const char *getPlayerColourName(int player);
bool isHumanPlayer(int player);				//to tell if the player is a computer or not.
bool myResponsibility(int player);
bool responsibleFor(int player, int playerinquestion);
int whosResponsible(int player);
bool canGiveOrdersFor(int player, int playerInQuestion);
int scavengerSlot();    // Returns the player number that scavengers would have if they were enabled.
int scavengerPlayer();  // Returns the player number that the scavengers have, or -1 if disabled.
Vector3i cameraToHome(UDWORD player, bool scroll, bool fromSave);

bool multiPlayerLoop();							// for loop.c

bool isBlindPlayerInfoState();
// return a "generic" player name that is fixed based on the player idx (useful for blind mode games)
const char *getPlayerGenericName(int player);

enum class HandleMessageAction
{
	Process_Message,
	Silently_Ignore,
	Disallow_And_Kick_Sender
};
HandleMessageAction getMessageHandlingAction(NETQUEUE& queue, uint8_t type);
bool shouldProcessMessage(NETQUEUE& queue, uint8_t type);
bool recvMessage();
bool SendResearch(uint8_t player, uint32_t index, bool trigger);
void printInGameTextMessage(NetworkTextMessage const &message);
void sendInGameSystemMessage(const char *text);
int32_t findPlayerIndexByPosition(uint32_t position);
void printConsoleNameChange(const char *oldName, const char *newName);  ///< Print message to console saying a name changed.

void turnOffMultiMsg(bool bDoit);

void autoLagKickRoutine(std::chrono::steady_clock::time_point now);
void autoLobbyNotReadyKickRoutine(std::chrono::steady_clock::time_point now);
uint64_t calculateSecondsNotReadyForPlayer(size_t i, std::chrono::steady_clock::time_point now);

void sendMap();
bool multiplayerWinSequence(bool firstCall);

/////////////////////////////////////////////////////////
// definitions of functions in multiplay's other c files.

// Buildings . multistruct
bool SendDestroyStructure(const STRUCTURE *s);
bool SendBuildFinished(const STRUCTURE *psStruct);
bool sendLasSat(UBYTE player, const STRUCTURE *psStruct, const BASE_OBJECT *psObj);
void sendStructureInfo(const STRUCTURE *psStruct, STRUCTURE_INFO structureInfo, const DROID_TEMPLATE *psTempl);

// droids . multibot
bool SendDroid(const DROID_TEMPLATE *pTemplate, uint32_t x, uint32_t y, uint8_t player, uint32_t id, const INITIAL_DROID_ORDERS *initialOrders);
bool SendDestroyDroid(const DROID *psDroid);
void sendQueuedDroidInfo();  ///< Actually sends the droid orders which were queued by SendDroidInfo.
void sendDroidInfo(DROID *psDroid, DroidOrder const &order, bool add);

bool sendDroidSecondary(const DROID *psDroid, SECONDARY_ORDER sec, SECONDARY_STATE state);
bool sendDroidDisembark(const DROID *psTransporter, DROID const *psDroid);

// Startup. mulitopt
bool multiShutdown();
bool sendLeavingMsg();

bool hostCampaign(const char *SessionName, char *hostPlayerName, bool spectatorHost, bool skipResetAIs);
struct JoinConnectionDescription
{
public:
	JoinConnectionDescription()
	{ }
public:
	enum class JoinConnectionType
	{
		TCP_DIRECT,
#ifdef WZ_GNS_NETWORK_BACKEND_ENABLED
		GNS_DIRECT,
#endif
	};
public:
	JoinConnectionDescription(const std::string& host, uint32_t port)
	: host(host)
	, port(port)
	, type(JoinConnectionType::TCP_DIRECT)
	{ }
	JoinConnectionDescription(JoinConnectionType t, const std::string& host, uint32_t port)
	: host(host)
	, port(port)
	, type(t)
	{ }
public:
	static std::string connectiontype_to_string(JoinConnectionType type);
	static optional<JoinConnectionType> connectiontype_from_string(const std::string& str);
public:
	std::string host;
	uint32_t port = 0;
	JoinConnectionType type = JoinConnectionType::TCP_DIRECT;
};
void to_json(nlohmann::json& j, const JoinConnectionDescription::JoinConnectionType& v);
void from_json(const nlohmann::json& j, JoinConnectionDescription::JoinConnectionType& v);
void to_json(nlohmann::json& j, const JoinConnectionDescription& v);
void from_json(const nlohmann::json& j, JoinConnectionDescription& v);

std::vector<JoinConnectionDescription> findLobbyGame(const std::string& lobbyAddress, unsigned int lobbyPort, uint32_t lobbyGameId);
void joinGame(const char *connectionString, bool asSpectator = false);
void joinGame(const char *host, uint32_t port, bool asSpectator = false);
void joinGame(const std::vector<JoinConnectionDescription>& connection_list, bool asSpectator = false);
void playerResponding();
bool multiGameInit();
bool multiGameShutdown();
bool multiStartScreenInit();

// kick-redirect
struct KickRedirectInfo
{
	std::vector<JoinConnectionDescription> connList; // a list of connection options - probably just one with a different port and an empty or "=" host (which reuses the current host address)
	optional<std::string> gamePassword = nullopt;
	bool asSpectator = false;
};
void to_json(nlohmann::json& j, const KickRedirectInfo& v);
void from_json(const nlohmann::json& j, KickRedirectInfo& v);
optional<KickRedirectInfo> parseKickRedirectInfo(const std::string& redirectJSONString, const std::string& currHostAddress);

bool kickRedirectPlayer(uint32_t player_id, const KickRedirectInfo& redirectInfo);
bool kickRedirectPlayer(uint32_t player_id, JoinConnectionDescription::JoinConnectionType connectionType, uint16_t newPort, bool asSpectator, optional<std::string> gamePassword);

// syncing.
bool sendScoreCheck();							//score check only(frontend)
void multiSyncResetAllChallenges();
void multiSyncResetPlayerChallenge(uint32_t playerIdx);
void multiSyncPlayerSwap(uint32_t playerIndexA, uint32_t playerIndexB);
bool sendPing();							// allow game to request pings.
void HandleBadParam(const char *msg, const int from, const int actual);
// multijoin
bool sendResearchStatus(const STRUCTURE *psBuilding, UDWORD index, UBYTE player, bool bStart);

bool sendBeacon(int32_t locX, int32_t locY, int32_t forPlayer, int32_t sender, const char *pStr);

void startMultiplayerGame();
void resetReadyStatus(bool bSendOptions, bool ignoreReadyReset = false);
bool shouldSkipReadyResetOnPlayerJoinLeaveEvent();

STRUCTURE *findResearchingFacilityByResearchIndex(const PerPlayerStructureLists& pList, unsigned player, unsigned index);
STRUCTURE *findResearchingFacilityByResearchIndex(unsigned player, unsigned index); // checks apsStructLists

void sendSyncRequest(int32_t req_id, int32_t x, int32_t y, const BASE_OBJECT *psObj, const BASE_OBJECT *psObj2);


bool sendBeaconToPlayer(SDWORD locX, SDWORD locY, SDWORD forPlayer, SDWORD sender, const char *beaconMsg);
MESSAGE *findBeaconMsg(UDWORD player, SDWORD sender);
VIEWDATA *CreateBeaconViewData(SDWORD sender, UDWORD LocX, UDWORD LocY);

void setPlayerMuted(uint32_t playerIdx, bool muted);
bool isPlayerMuted(uint32_t sender);

bool setGameStoryLogPlayerDataValue(uint32_t playerIndex, const std::string& key_str, const std::string& value_str);
bool makePlayerSpectator(uint32_t player_id, bool removeAllStructs = false, bool quietly = false);

class WZGameReplayOptionsHandler : public ReplayOptionsHandler
{
	virtual bool saveOptions(nlohmann::json& object) const override;
	virtual bool optionsUpdatePlayerInfo(nlohmann::json& object) const override;
	virtual bool saveMap(EmbeddedMapData& mapData) const override;
	virtual bool restoreOptions(const nlohmann::json& object, EmbeddedMapData&& embeddedMapData, uint32_t replay_netcodeMajor, uint32_t replay_netcodeMinor) override;
	virtual size_t desiredBufferSize() const override;
	virtual size_t maximumEmbeddedMapBufferSize() const override;
};

#endif // __INCLUDED_SRC_MULTIPLAY_H__
