/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2024  Warzone 2100 Project

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
/**
 * @file netplay.c
 *
 * Basic netcode.
 */

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/string_ext.h"
#include "lib/framework/crc.h"
#include "lib/framework/file.h"
#include "lib/gamelib/gtime.h"
#include "lib/exceptionhandler/dumpinfo.h"
#include "src/console.h"
#include "src/component.h"		// FIXME: we need to handle this better
#include "src/modding.h"		// FIXME: we need to handle this better
#include "src/clparse.h"

#include <time.h>			// for stats
#include <physfs.h>
#include "lib/framework/physfs_ext.h"
#include <string.h>
#include <memory>
#include <thread>
#include <atomic>
#include <limits>
#include <sodium.h>

#include "netplay.h"
#include "netlog.h"
#include "netreplay.h"
#include "netsocket.h"
#include "netpermissions.h"
#include "sync_debug.h"

#include <miniwget.h>
#if (defined(__GNUC__) || defined(__clang__)) && !defined(__INTEL_COMPILER)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wpedantic"
#endif
#include <miniupnpc.h>
#if (defined(__GNUC__) || defined(__clang__)) && !defined(__INTEL_COMPILER)
# pragma GCC diagnostic pop
#endif
#include <upnpcommands.h>

// Enforce minimum MINIUPNPC_API_VERSION
#if !defined(MINIUPNPC_API_VERSION) || (MINIUPNPC_API_VERSION < 9)
	#error lib/netplay requires MINIUPNPC_API_VERSION >= 9
#endif

#include "src/multistat.h"
#include "src/multijoin.h"
#include "src/multiint.h"
#include "src/multiplay.h"
#include "src/hci/quickchat.h"
#include "src/warzoneconfig.h"
#include "src/version.h"
#include "src/loadsave.h"
#include "src/activity.h"
#include "src/stdinreader.h"

#include "3rdparty/GeoIP/libGeoIP/GeoIP.h"
#include <LRUCache11/LRUCache11.hpp>

#if defined (WZ_OS_MAC)
# include "lib/framework/cocoa_wrapper.h"
#endif

// WARNING !!! This is initialised via configuration.c !!!
char masterserver_name[255] = {'\0'};
static unsigned int masterserver_port = 0, gameserver_port = 0;
static bool bJoinPrefTryIPv6First = true;
static bool bDefaultHostFreeChatEnabled = true;

// This is for command line argument override
// Disables port saving and reading from/to config
bool netGameserverPortOverride = false;

#define NET_TIMEOUT_DELAY	2500		// we wait this amount of time for socket activity
#define NET_READ_TIMEOUT	0
/*
*	=== Using new socket code, this might not hold true any longer ===
*	NOTE /rant:  If the buffer size isn't big enough, it will invalidate the socket.
*	Which means that we need to allocate a buffer big enough to handle worst case
*	situations.
*	reference: MaxMsgSize in netplay.h  (currently set to 16K)
*
*/
#define NET_BUFFER_SIZE	(MaxMsgSize)	// Would be 16K

#define UPNP_SUCCESS 1
#define UPNP_ERROR_DEVICE_NOT_FOUND -1
#define UPNP_ERROR_CONTROL_NOT_AVAILABLE -2

// For NET_JOIN messages
enum NET_JOIN_PLAYERTYPE : uint8_t {
	NET_JOIN_PLAYER = 0,
	NET_JOIN_SPECTATOR = 1,
};

// ////////////////////////////////////////////////////////////////////////
// Function prototypes
static void NETplayerCloseSocket(UDWORD index, bool quietSocketClose);
static void NETplayerLeaving(UDWORD player, bool quietSocketClose = false);		// Cleanup sockets on player leaving (nicely)
static void NETplayerDropped(UDWORD player);		// Broadcast NET_PLAYER_DROPPED & cleanup
static void NETallowJoining();
static void NETfixPlayerCount();
/*
 * Network globals, these are part of the new network API
 */
SYNC_COUNTER sync_counter;		// keeps track on how well we are in sync
// ////////////////////////////////////////////////////////////////////////
// Types

static std::atomic_int upnp_status;

struct Statistic
{
	size_t sent;
	size_t received;
};

struct NETSTATS  // data regarding the last one second or so.
{
	Statistic       rawBytes;               // Number of actual bytes, in about 1 sec.
	Statistic       uncompressedBytes;      // Number of bytes sent, before compression, in about 1 sec.
	Statistic       packets;                // Number of calls to writeAll, in about 1 sec.
};

struct NET_PLAYER_DATA
{
	uint16_t        size;
	void           *data;
	size_t          buffer_size;
};

#define SERVER_UPDATE_MIN_INTERVAL 7 * GAME_TICKS_PER_SEC
#define SERVER_UPDATE_MAX_INTERVAL 25 * GAME_TICKS_PER_SEC

class LobbyServerConnectionHandler
{
public:
	bool connect();
	bool disconnect();
	void sendUpdate();
	void run();
private:
	void sendUpdateNow();
	void sendKeepAlive();

	inline bool canSendServerUpdateNow()
	{
		return (currentState == LobbyConnectionState::Connected)
			&& (realTime - lastServerUpdate >= SERVER_UPDATE_MIN_INTERVAL);
	}

	inline bool shouldSendServerKeepAliveNow()
	{
		return (currentState == LobbyConnectionState::Connected)
			&& (realTime - lastServerUpdate >= SERVER_UPDATE_MAX_INTERVAL);
	}
private:
	enum class LobbyConnectionState
	{
		Disconnected,
		Connecting_WaitingForResponse,
		Connected
	};
	LobbyConnectionState currentState = LobbyConnectionState::Disconnected;
	Socket *rs_socket = nullptr;
	SocketSet* waitingForConnectionFinalize = nullptr;
	uint32_t lastConnectionTime = 0;
	uint32_t lastServerUpdate = 0;
	bool queuedServerUpdate = false;
};

class PlayerManagementRecord
{
public:
	void clear()
	{
		identitiesMovedToSpectatorsByHost.clear();
		ipsMovedToSpectatorsByHost.clear();
	}
	void movedPlayerToSpectators(const PLAYER& player, const EcKey::Key& publicIdentity, bool byHost)
	{
		if (!byHost) { return; }
		ipsMovedToSpectatorsByHost.insert(player.IPtextAddress);
		identitiesMovedToSpectatorsByHost.insert(base64Encode(publicIdentity));
	}
	void movedSpectatorToPlayers(const PLAYER& player, const EcKey::Key& publicIdentity, bool byHost)
	{
		if (!byHost) { return; }
		ipsMovedToSpectatorsByHost.erase(player.IPtextAddress);
		identitiesMovedToSpectatorsByHost.erase(base64Encode(publicIdentity));
	}
	bool hostMovedPlayerToSpectators(const std::string& ipAddress)
	{
		return ipsMovedToSpectatorsByHost.count(ipAddress) > 0;
	}
	bool hostMovedPlayerToSpectators(const EcKey::Key& publicIdentity)
	{
		return identitiesMovedToSpectatorsByHost.count(base64Encode(publicIdentity)) > 0;
	}
private:
	std::unordered_set<std::string> identitiesMovedToSpectatorsByHost;
	std::unordered_set<std::string> ipsMovedToSpectatorsByHost;
};

// ////////////////////////////////////////////////////////////////////////
// Variables

NETPLAY	NetPlay;
static bool allow_joining = false;
static bool server_not_there = false;
static bool lobby_disabled = false;
static std::string lobby_disabled_info_link_url;
static GAMESTRUCT	gamestruct;

static std::vector<WZFile> DownloadingWzFiles;

// update flags
bool netPlayersUpdated;

// geo ip object
extern GeoIP *gi;

/**
 * Socket used for these purposes:
 *  * Host a game, be a server.
 *  * Connect to the lobby server.
 *  * Join a server for a game.
 */
static Socket *tcp_socket = nullptr;               ///< Socket used to talk to a lobby server (hosts also seem to temporarily act as lobby servers while the client negotiates joining the game), or to listen for clients (if we're the host, in which case we use rs_socket (declaration hidden somewhere inside a function) to talk to the lobby server).

static Socket *bsocket = nullptr;                  ///< Socket used to talk to the host (clients only). If bsocket != NULL, then tcp_socket == NULL.
static Socket *connected_bsocket[MAX_CONNECTED_PLAYERS] = { nullptr };  ///< Sockets used to talk to clients (host only).
static SocketSet *socket_set = nullptr;

static struct UPNPUrls urls;
static struct IGDdatas data;

// local ip address
static char lanaddr[40];
static char externalIPAddress[40];
/**
 * Used for connections with clients.
 */
#define NET_PING_TMP_PING_CHALLENGE_SIZE 128

struct TmpSocketInfo
{
	std::string ip;
	std::chrono::steady_clock::time_point connectTime;
	char buffer[10] = {'\0'};
	size_t usedBuffer = 0;
	std::vector<uint8_t> connectChallenge;
	enum class TmpConnectState
	{
		None,
		PendingInitialConnect,
		PendingJoinRequest,
		PendingAsyncApproval,
		ProcessJoin
	};
	TmpConnectState connectState;

	struct ReceivedJoinInfo
	{
		// Information received from a join request - needed for later stages
		char name[64] = {'\0'};
		uint8_t playerType = 0;
		EcKey identity;

		void reset()
		{
			name[0] = '\0';
			playerType = 0;
			identity.clear();
		}
	};
	ReceivedJoinInfo receivedJoinInfo;

	// async join approval
	std::string uniqueJoinID;
	optional<LOBBY_ERROR_TYPES> asyncJoinApprovalResult = nullopt;

	void reset()
	{
		ip.clear();
		connectTime = std::chrono::steady_clock::time_point();
		memset(buffer, 0, sizeof(buffer));
		usedBuffer = 0;
		connectChallenge.clear();
		connectState = TmpConnectState::None;
		receivedJoinInfo.reset();
		uniqueJoinID.clear();
		asyncJoinApprovalResult = nullopt;
	}
};

static Socket *tmp_socket[MAX_TMP_SOCKETS] = { nullptr };  ///< Sockets used to talk to clients which have not yet been assigned a player number (host only).
static std::array<TmpSocketInfo, MAX_TMP_SOCKETS> tmp_connectState;
static bool bAsyncJoinApprovalEnabled = false;
static std::unordered_map<std::string, size_t> tmp_pendingIPs;
static lru11::Cache<std::string, size_t> tmp_badIPs(512, 64);

static SocketSet *tmp_socket_set = nullptr;
static int32_t          NetGameFlags[4] = { 0, 0, 0, 0 };
char iptoconnect[PATH_MAX] = "\0"; // holds IP/hostname from command line
bool cliConnectToIpAsSpectator = false; // for cli option

static NETSTATS nStats              = {{0, 0}, {0, 0}, {0, 0}};
static NETSTATS nStatsLastSec       = {{0, 0}, {0, 0}, {0, 0}};
static NETSTATS nStatsSecondLastSec = {{0, 0}, {0, 0}, {0, 0}};
static const NETSTATS nZeroStats    = {{0, 0}, {0, 0}, {0, 0}};
static int nStatsLastUpdateTime = 0;

unsigned NET_PlayerConnectionStatus[CONNECTIONSTATUS_NORMAL][MAX_CONNECTED_PLAYERS];
std::vector<optional<uint32_t>>	NET_waitingForIndexChangeAckSince = std::vector<optional<uint32_t>>(MAX_CONNECTED_PLAYERS, nullopt);	///< If waiting for the client to acknowledge a player index change, this is the realTime we started waiting

static LobbyServerConnectionHandler lobbyConnectionHandler;
static PlayerManagementRecord playerManagementRecord;

// ////////////////////////////////////////////////////////////////////////////
/************************************************************************************
 **  NOTE (!)  Increment NETCODE_VERSION_MINOR on each release.
 **
 ************************************************************************************
**/
static char const *versionString = version_getVersionString();

#include "lib/netplay/netplay_config.h"

const std::vector<WZFile>& NET_getDownloadingWzFiles()
{
	return DownloadingWzFiles;
}

void NET_addDownloadingWZFile(WZFile&& newFile)
{
	DownloadingWzFiles.push_back(std::move(newFile));
}

void NET_clearDownloadingWZFiles()
{
	// if we were in a middle of transferring a file, then close the file handle
	for (auto &file : DownloadingWzFiles)
	{
		debug(LOG_NET, "closing aborted file");
		file.closeFile();
		ASSERT(!file.filename.empty(), "filename must not be empty");
		PHYSFS_delete(file.filename.c_str()); 		// delete incomplete (map) file
	}
	DownloadingWzFiles.clear();
}

NETPLAY::NETPLAY()
{
	players.resize(MAX_CONNECTED_PLAYERS);
	scriptSetPlayerDataStrings.resize(MAX_PLAYERS);
	playerReferences.resize(MAX_CONNECTED_PLAYERS);
	for (auto i = 0; i < MAX_CONNECTED_PLAYERS; i++)
	{
		playerReferences[i] = std::make_shared<PlayerReference>(i);
	}
}

bool NETisCorrectVersion(uint32_t game_version_major, uint32_t game_version_minor)
{
	return (uint32_t)NETCODE_VERSION_MAJOR == game_version_major && (uint32_t)NETCODE_VERSION_MINOR == game_version_minor;
}

bool NETisGreaterVersion(uint32_t game_version_major, uint32_t game_version_minor)
{
	return (game_version_major > NETCODE_VERSION_MAJOR) || ((game_version_major == NETCODE_VERSION_MAJOR) && (game_version_minor > NETCODE_VERSION_MINOR));
}

uint32_t NETGetMajorVersion()
{
	return NETCODE_VERSION_MAJOR;
}

uint32_t NETGetMinorVersion()
{
	return NETCODE_VERSION_MINOR;
}

bool NETGameIsLocked()
{
	return NetPlay.GamePassworded;
}

bool NET_getLobbyDisabled()
{
	return lobby_disabled;
}

const std::string& NET_getLobbyDisabledInfoLinkURL()
{
	return lobby_disabled_info_link_url;
}

void NET_setLobbyDisabled(const std::string& infoLinkURL)
{
	lobby_disabled = true;
	lobby_disabled_info_link_url = infoLinkURL;
}

//	Sets if the game is password protected or not
void NETGameLocked(bool flag)
{
	NetPlay.GamePassworded = flag;
	bool flagChanged = gamestruct.privateGame != (uint32_t)flag;
	gamestruct.privateGame = flag;
	if (allow_joining && NetPlay.isHost && flagChanged)
	{
		debug(LOG_NET, "Updating game locked status.");
		NETregisterServer(WZ_SERVER_UPDATE);
	}
	if (flagChanged)
	{
		ActivityManager::instance().updateMultiplayGameData(game, ingame, NETGameIsLocked());
	}
	NETlogEntry("Password is", SYNC_FLAG, NetPlay.GamePassworded);
	debug(LOG_NET, "Passworded game is %s", NetPlay.GamePassworded ? "TRUE" : "FALSE");
}

SpectatorInfo SpectatorInfo::currentNetPlayState()
{
	SpectatorInfo latestSpecInfo;
	for (const auto& slot : NetPlay.players)
	{
		if (slot.isSpectator)
		{
			latestSpecInfo.totalSpectatorSlots++;
			if (slot.allocated)
			{
				latestSpecInfo.spectatorsJoined++;
			}
		}
	}
	return latestSpecInfo;
}

SpectatorInfo NETGameGetSpectatorInfo()
{
	if (NetPlay.isHost)
	{
		// we have this as part of the host current gamestruct
		return SpectatorInfo::fromUint32(gamestruct.desc.dwUserFlags[1]);
	}
	else
	{
		return SpectatorInfo::currentNetPlayState();
	}
}

void NETsetLobbyOptField(const char *Value, const NET_LOBBY_OPT_FIELD Field)
{
	switch (Field)
	{
		case NET_LOBBY_OPT_FIELD::GNAME:
			sstrcpy(gamestruct.name, Value);
			break;
		case NET_LOBBY_OPT_FIELD::MAPNAME:
			sstrcpy(gamestruct.mapname, formatGameName(Value).toUtf8().c_str());
			break;
		case NET_LOBBY_OPT_FIELD::HOSTNAME:
			sstrcpy(gamestruct.hostname, Value);
			break;
		default:
			debug(LOG_WARNING, "Invalid field specified for NETsetGameOptField()");
			break;
	}
}

//	Sets the game password
void NETsetGamePassword(const char *password)
{
	sstrcpy(NetPlay.gamePassword, password);
	debug(LOG_NET, "Password entered is: [%s]", NetPlay.gamePassword);
	NETGameLocked(true);
}

//	Resets the game password
void NETresetGamePassword()
{
	NetPlay.gamePassword[0] = '\0';
	debug(LOG_NET, "password was reset");
	NETGameLocked(false);
}

void NETsetAsyncJoinApprovalRequired(bool enabled)
{
	bAsyncJoinApprovalEnabled = enabled;
	if (bAsyncJoinApprovalEnabled)
	{
		debug(LOG_INFO, "Async join approval is required");
	}
}

//	NOTE: *MUST* be called from the main thread!
bool NETsetAsyncJoinApprovalResult(const std::string& uniqueJoinID, bool approve, LOBBY_ERROR_TYPES rejectedReason)
{
	if (!approve && rejectedReason == ERROR_NOERROR)
	{
		debug(LOG_INFO, "Rejecting join with NOERROR reason changed to ERROR_KICKED");
		rejectedReason = ERROR_KICKED;
	}
	for (auto& tmp_info : tmp_connectState)
	{
		if (tmp_info.connectState == TmpSocketInfo::TmpConnectState::PendingAsyncApproval
			&& tmp_info.uniqueJoinID == uniqueJoinID)
		{
			// found a match
			tmp_info.asyncJoinApprovalResult = (approve) ? ERROR_NOERROR : rejectedReason;
			return true;
		}
	}

	// no matching pending join - may have dropped in the interim, etc
	return false;
}

// *********** Socket with buffer that read NETMSGs ******************

static size_t NET_fillBuffer(Socket **pSocket, SocketSet *pSocketSet, uint8_t *bufstart, int bufsize)
{
	Socket *socket = *pSocket;
	ssize_t size;

	if (!socketReadReady(socket))
	{
		return 0;
	}

	size_t rawBytes;
	size = readNoInt(socket, bufstart, bufsize, &rawBytes);

	if ((size != 0 || !socketReadDisconnected(socket)) && size != SOCKET_ERROR)
	{
		nStats.rawBytes.received          += rawBytes;
		nStats.uncompressedBytes.received += size;
		nStats.packets.received           += 1;

		return size;
	}
	else
	{
		if (size == 0)
		{
			debug(LOG_NET, "Connection closed from the other side");
			NETlogEntry("Connection closed from the other side..", SYNC_FLAG, selectedPlayer);
		}
		else
		{
			debug(LOG_NET, "%s tcp_socket %p is now invalid", strSockError(getSockErr()), static_cast<void *>(socket));
		}

		// an error occurred, or the remote host has closed the connection.
		if (pSocketSet != nullptr)
		{
			SocketSet_DelSocket(pSocketSet, socket);
		}

		ASSERT(size <= bufsize, "Socket buffer is too small!");

		if (size > bufsize)
		{
			debug(LOG_ERROR, "Fatal connection error: buffer size of (%d) was too small, current byte count was %ld", bufsize, (long)size);
			NETlogEntry("Fatal connection error: buffer size was too small!", SYNC_FLAG, selectedPlayer);
		}
		if (bsocket == socket)
		{
			debug(LOG_NET, "Host connection was lost!");
			NETlogEntry("Host connection was lost!", SYNC_FLAG, selectedPlayer);
			bsocket = nullptr;
			//Game is pretty much over --should just end everything when HOST dies.
			NetPlay.isHostAlive = false;
			ingame.localJoiningInProgress = false;
			setLobbyError(ERROR_NOERROR);
			NETclose();
			return 0;
		}
		socketClose(socket);
		*pSocket = nullptr;
	}

	return 0;
}

static int playersPerTeam()
{
	for (int v = game.maxPlayers - 1; v > 1; --v)
	{
		if (game.maxPlayers%v == 0)
		{
			return v;
		}
	}
	return 1;
}

/**
 * Resets network properties of a player to safe defaults. Player slots should always be in this state
 * before attemtping to assign a connectign player to it.
 *
 * Used to reset the player slot in NET_InitPlayer and to reset players slot without modifying ai/team/
 * position configuration for the players.
 */
static void initPlayerNetworkProps(int playerIndex)
{
	NetPlay.players[playerIndex].allocated = false;
	NetPlay.players[playerIndex].autoGame = false;
	NetPlay.players[playerIndex].heartattacktime = 0;
	NetPlay.players[playerIndex].heartbeat = true;  // we always start with a heartbeat
	NetPlay.players[playerIndex].kick = false;
	NetPlay.players[playerIndex].ready = false;

	if (NetPlay.players[playerIndex].wzFiles)
	{
		NetPlay.players[playerIndex].wzFiles->clear();
	}
	else
	{
		ASSERT(false, "PLAYERS.wzFiles is uninitialized??");
	}
	if (playerIndex < MAX_CONNECTED_PLAYERS)
	{
		ingame.JoiningInProgress[playerIndex] = false;
		ingame.PendingDisconnect[playerIndex] = false;
		ingame.hostChatPermissions[playerIndex] = (NetPlay.bComms) ? NETgetDefaultMPHostFreeChatPreference() : true;
	}
}

void NET_InitPlayer(uint32_t i, bool initPosition, bool initTeams, bool initSpectator)
{
	ASSERT_OR_RETURN(, i < NetPlay.players.size(), "Invalid player_id: (%" PRIu32")",  i);

	initPlayerNetworkProps(i);

	NetPlay.players[i].difficulty = AIDifficulty::DISABLED;
	if (ingame.localJoiningInProgress)
	{
		// only clear name outside of games.
		clearPlayerName(i);
	}
	if (initPosition)
	{
		setPlayerColour(i, i);
		NetPlay.players[i].position = i;
		NetPlay.players[i].team = initTeams && i < game.maxPlayers? i/playersPerTeam() : i;
	}
	if (NetPlay.bComms)
	{
		NetPlay.players[i].ai = AI_OPEN;
	}
	else
	{
		NetPlay.players[i].ai = 0;
	}
	NetPlay.players[i].faction = FACTION_NORMAL;
	if (initSpectator)
	{
		NetPlay.players[i].isSpectator = false;
	}
}

uint8_t NET_numHumanPlayers(void)
{
	uint8_t RetVal = 0;
	for (uint8_t Inc = 0; Inc < MAX_PLAYERS; ++Inc)
	{
		if (NetPlay.players[Inc].allocated && !NetPlay.players[Inc].isSpectator) ++RetVal;
	}

	return RetVal;
}

std::vector<uint8_t> NET_getHumanPlayers(void)
{
	std::vector<uint8_t> RetVal;
	RetVal.reserve(MAX_PLAYERS);

	for (uint8_t Inc = 0; Inc < MAX_PLAYERS; ++Inc)
	{
		if (NetPlay.players[Inc].allocated && !NetPlay.players[Inc].isSpectator) RetVal.push_back(Inc);
	}

	return RetVal;
}

void NET_InitPlayers(bool initTeams, bool initSpectator)
{
	for (unsigned i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
	{
		NET_InitPlayer(i, true, initTeams, initSpectator);
		ingame.muteChat[i] = false;
		clearPlayerName(i);
		NETinitQueue(NETnetQueue(i));
	}
	resetRecentScoreData();
	NETinitQueue(NETbroadcastQueue());

	NetPlay.hostPlayer = NET_HOST_ONLY;	// right now, host starts always at index zero
	NetPlay.playercount = 0;
	DownloadingWzFiles.clear();
	NET_waitingForIndexChangeAckSince = std::vector<optional<uint32_t>>(MAX_CONNECTED_PLAYERS, nullopt);
	debug(LOG_NET, "Players initialized");
}

static void NETSendNPlayerInfoTo(uint32_t *index, uint32_t indexLen, unsigned to)
{
	ASSERT_HOST_ONLY(return);
	NETbeginEncode(NETnetQueue(to), NET_PLAYER_INFO);
	NETuint32_t(&indexLen);
	for (unsigned n = 0; n < indexLen; ++n)
	{
		debug(LOG_NET, "sending player's (%u) info to all players", index[n]);
		NETlogEntry("Sending player's info to all players", SYNC_FLAG, index[n]);
		NETuint32_t(&index[n]);
		NETbool(&NetPlay.players[index[n]].allocated);
		NETbool(&NetPlay.players[index[n]].heartbeat);
		NETbool(&NetPlay.players[index[n]].kick);
		NETstring(NetPlay.players[index[n]].name, sizeof(NetPlay.players[index[n]].name));
		NETuint32_t(&NetPlay.players[index[n]].heartattacktime);
		NETint32_t(&NetPlay.players[index[n]].colour);
		NETint32_t(&NetPlay.players[index[n]].position);
		NETint32_t(&NetPlay.players[index[n]].team);
		NETbool(&NetPlay.players[index[n]].ready);
		NETint8_t(&NetPlay.players[index[n]].ai);
		NETint8_t(reinterpret_cast<int8_t*>(&NetPlay.players[index[n]].difficulty));
		NETuint8_t(reinterpret_cast<uint8_t *>(&NetPlay.players[index[n]].faction));
		NETbool(&NetPlay.players[index[n]].isSpectator);
		const char emptyCode[3] = {0};
		NETstring(getSendGeoIPDataEnable() ? NetPlay.players[index[n]].countryCode : emptyCode, sizeof(NetPlay.players[index[n]].countryCode));
	}
	NETend();
	ActivityManager::instance().updateMultiplayGameData(game, ingame, NETGameIsLocked());
}

static void NETSendPlayerInfoTo(uint32_t index, unsigned to)
{
	NETSendNPlayerInfoTo(&index, 1, to);
}

static void NETSendAllPlayerInfoTo(unsigned to)
{
	static uint32_t indices[MAX_CONNECTED_PLAYERS];
	for (int i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
	{
		indices[i] = i;
	}
	ASSERT_OR_RETURN(, NetPlay.isHost == true, "Invalid call for non-host");

	NETSendNPlayerInfoTo(indices, ARRAY_SIZE(indices), to);
}

void NETBroadcastTwoPlayerInfo(uint32_t index1, uint32_t index2)
{
	ASSERT_HOST_ONLY(return);
	uint32_t indices[2] = {index1, index2};
	NETSendNPlayerInfoTo(indices, 2, NET_ALL_PLAYERS);
}

void NETBroadcastPlayerInfo(uint32_t index)
{
	ASSERT_HOST_ONLY(return);
	NETSendPlayerInfoTo(index, NET_ALL_PLAYERS);
}

// Checks if there are *any* open slots (player *OR* spectator) for an incoming connection
static bool NET_HasAnyOpenSlots()
{
	for (int i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
	{
		if (i == scavengerSlot())
		{
			// do not offer up the scavenger slot (this really needs to be refactored later - why is this a variable slot index?!?)
			continue;
		}
		if (i == PLAYER_FEATURE)
		{
			// do not offer up this "player feature" slot - TODO: maybe remove the need for this slot?
			continue;
		}
		PLAYER const &p = NetPlay.players[i];
		if (!p.allocated && p.ai == AI_OPEN)
		{
			return true;
		}
	}
	return false;
}

static optional<uint32_t> NET_FindOpenSlotForPlayer(bool forceTakeLowestAvailablePlayerNumber = false, optional<bool> asSpectator = false)
{
	int index = -1;
	int position = INT_MAX;
	for (int i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
	{
		if (!forceTakeLowestAvailablePlayerNumber && !asSpectator.value_or(false) && (i >= game.maxPlayers || i >= MAX_PLAYERS || NetPlay.players[i].position >= game.maxPlayers))
		{
			// Player slots are only supported where the player index and the player position is <= game.maxPlayers
			// Skip otherwise
			continue;
		}
		if (i == scavengerSlot())
		{
			// do not offer up the scavenger slot (this really needs to be refactored later - why is this a variable slot index?!?)
			continue;
		}
		if (i == PLAYER_FEATURE)
		{
			// do not offer up this "player feature" slot - TODO: maybe remove the need for this slot?
			continue;
		}
		// find the lowest "position" slot that is available (unless forceTakeLowestAvailablePlayerNumber is set, in which case just take the first available)
		PLAYER const &p = NetPlay.players[i];
		if (!p.allocated && p.ai == AI_OPEN && p.position < position && (!asSpectator.has_value() || asSpectator.value() == p.isSpectator))
		{
			index = i;
			position = p.position;
			if (forceTakeLowestAvailablePlayerNumber)
			{
				break;
			}
		}
	}

	if (index == -1)
	{
		return nullopt;
	}

	return static_cast<uint32_t>(index);
}

static optional<uint32_t> NET_CreatePlayer(char const *name, bool forceTakeLowestAvailablePlayerNumber = false, optional<bool> asSpectator = false)
{
	optional<uint32_t> index = NET_FindOpenSlotForPlayer(forceTakeLowestAvailablePlayerNumber, asSpectator);
	if (!index.has_value())
	{
		debug(LOG_ERROR, "Could not find place for player %s", name);
		NETlogEntry("Could not find a place for player!", SYNC_FLAG, -1);
		return nullopt;
	}

	char buf[250] = {'\0'};

	ssprintf(buf, "A new player has been created. Player, %s, is set to slot %u", name, index.value());
	debug(LOG_NET, "%s", buf);
	NETlogEntry(buf, SYNC_FLAG, index.value());
	NET_InitPlayer(index.value(), false);  // re-init everything
	NetPlay.players[index.value()].allocated = true;
	NetPlay.players[index.value()].difficulty = AIDifficulty::HUMAN;
	setPlayerName(index.value(), name);
	if (!NetPlay.players[index.value()].isSpectator)
	{
		++NetPlay.playercount;
	}
	++sync_counter.joins;
	return index;
}

static void NET_DestroyPlayer(unsigned int index, bool suppressActivityUpdates = false)
{
	ASSERT_OR_RETURN(, index < NetPlay.players.size(), "Invalid player_id: (%" PRIu32")",  index);
	debug(LOG_NET, "Freeing slot %u for a new player", index);
	NETlogEntry("Freeing slot for a new player.", SYNC_FLAG, index);
	bool wasAllocated = NetPlay.players[index].allocated;
	if (NetPlay.players[index].allocated)
	{
		NetPlay.players[index].allocated = false;
		if (!NetPlay.players[index].isSpectator)
		{
			NetPlay.playercount--;
		}
		gamestruct.desc.dwCurrentPlayers = NetPlay.playercount;
		if (allow_joining && NetPlay.isHost)
		{
			// Update player count in the lobby by disconnecting
			// and reconnecting
			NETregisterServer(WZ_SERVER_UPDATE);
		}
	}
	NET_InitPlayer(index, false);  // reinitialize
	if (wasAllocated && !suppressActivityUpdates)
	{
		ActivityManager::instance().updateMultiplayGameData(game, ingame, NETGameIsLocked());
	}
}

bool NETplayerHasConnection(uint32_t index)
{
	ASSERT_HOST_ONLY(return false);
	ASSERT_OR_RETURN(false, index < MAX_CONNECTED_PLAYERS, "Invalid index: %" PRIu32, index);
	return connected_bsocket[index] != nullptr;
}

/**
 * @note Connection dropped. Handle it gracefully.
 * \param index
 */
static void NETplayerClientsDisconnect(std::set<uint32_t> indexes)
{
	if (!NetPlay.isHost)
	{
		ASSERT(false, "Host only routine detected for client!");
		return;
	}

	// 1.) Do a pass just closing the sockets (in case multiple players involved)
	std::vector<uint32_t> playersActuallyDisconnected;
	for (auto index : indexes)
	{
		if (index >= MAX_CONNECTED_PLAYERS)
		{
			ASSERT(index < MAX_CONNECTED_PLAYERS, "Invalid player index: %" PRIu32, index);
			continue;
		}

		if (connected_bsocket[index])
		{
			debug(LOG_NET, "Player (%u) has left unexpectedly, closing socket %p", index, static_cast<void *>(connected_bsocket[index]));

			NETplayerCloseSocket(index, false);
			playersActuallyDisconnected.push_back(index);
		}
		else
		{
			debug(LOG_ERROR, "Player (%u) has left unexpectedly - but socket already closed?", index);
		}
	}

	// 2.) Notify other players about which players were disconnected
	for (auto index : playersActuallyDisconnected)
	{
		if (index >= MAX_CONNECTED_PLAYERS)
		{
			continue;
		}

		NETplayerLeaving(index, true);

		NETlogEntry("Player has left unexpectedly.", SYNC_FLAG, index);
		// Announce to the world. This was really icky, because we may have been calling the send
		// function recursively. We really ought to have had a send queue, and now we finally do...
		if (ingame.localJoiningInProgress)  // Only if game hasn't actually started yet.
		{
			NETbeginEncode(NETbroadcastQueue(), NET_PLAYER_DROPPED);
			NETuint32_t(&index);
			NETend();
		}
	}
}

static void NETplayerCloseSocket(UDWORD index, bool quietSocketClose)
{
	ASSERT_OR_RETURN(, index < MAX_CONNECTED_PLAYERS, "Invalid index: %" PRIu32, index);
	if (connected_bsocket[index])
	{
		debug(LOG_NET, "Player (%u) has left, closing socket %p", index, static_cast<void *>(connected_bsocket[index]));
		NETlogEntry("Player has left nicely.", SYNC_FLAG, index);

		// Although we can get a error result from DelSocket, it don't really matter here.
		SocketSet_DelSocket(socket_set, connected_bsocket[index]);
		socketClose(connected_bsocket[index]);
		connected_bsocket[index] = nullptr;
	}
	else
	{
		debug(LOG_NET, "Player (%u) has left nicely, socket already closed?", index);
	}
}

/**
 * @note When a player leaves nicely (ie, we got a NET_PLAYER_LEAVING
 *       message), we clean up the socket that we used.
 * \param index
 */
static void NETplayerLeaving(UDWORD index, bool quietSocketClose)
{
	ASSERT_OR_RETURN(, index < MAX_CONNECTED_PLAYERS, "Invalid index: %" PRIu32, index);
	NETplayerCloseSocket(index, quietSocketClose);
	sync_counter.left++;
	bool wasSpectator = NetPlay.players[index].isSpectator;
	MultiPlayerLeave(index);		// more cleanup
	if (ingame.localJoiningInProgress)  // Only if game hasn't actually started yet.
	{
		NET_DestroyPlayer(index);       // sets index player's array to false
		if (!wasSpectator)
		{
			resetReadyStatus(false);		// reset ready status for all players
		}
	}
}

/**
 * @note When a player's connection is broken we broadcast the NET_PLAYER_DROPPED
 *       message.
 * \param index
 */
static void NETplayerDropped(UDWORD index)
{
	ASSERT_OR_RETURN(, index < NetPlay.players.size(), "Invalid index: %" PRIu32, index);
	uint32_t id = index;

	if (!NetPlay.isHost)
	{
		ASSERT(false, "Host only routine detected for client!");
		return;
	}

	sync_counter.drops++;
	bool wasSpectator = NetPlay.players[index].isSpectator;
	MultiPlayerLeave(id);			// more cleanup
	if (ingame.localJoiningInProgress)  // Only if game hasn't actually started yet.
	{
		// Send message type specifically for dropped / disconnects
		NETbeginEncode(NETbroadcastQueue(), NET_PLAYER_DROPPED);
		NETuint32_t(&id);
		NETend();
		debug(LOG_INFO, "sending NET_PLAYER_DROPPED for player %d", id);
		NET_DestroyPlayer(id);          // just clears array
		if (!wasSpectator)
		{
			resetReadyStatus(false);		// reset ready status for all players
		}
	}

	NETsetPlayerConnectionStatus(CONNECTIONSTATUS_PLAYER_DROPPED, id);
}

/**
 * @note Cleanup for when a player is kicked.
 * \param index
 */
void NETplayerKicked(UDWORD index)
{
	ASSERT_OR_RETURN(, index < MAX_CONNECTED_PLAYERS, "NETplayerKicked invalid player_id: (%" PRIu32")", index);

	// kicking a player counts as "leaving nicely", since "nicely" in this case
	// simply means "there wasn't a connection error."
	debug(LOG_INFO, "Player %u was kicked.", index);
	sync_counter.kicks++;
	NETlogEntry("Player was kicked.", SYNC_FLAG, index);
	NETplayerLeaving(index);		// need to close socket for the player that left.
	NETsetPlayerConnectionStatus(CONNECTIONSTATUS_PLAYER_LEAVING, index);
}

// ////////////////////////////////////////////////////////////////////////
// rename the local player
bool NETchangePlayerName(UDWORD index, char *newName)
{
	if (!NetPlay.bComms)
	{
		setPlayerName(0, newName);
		return true;
	}
	debug(LOG_NET, "Requesting a change of player name for pid=%u to %s", index, newName);
	NETlogEntry("Player wants a name change.", SYNC_FLAG, index);
	ASSERT_OR_RETURN(false, index < MAX_CONNECTED_PLAYERS, "invalid index: %" PRIu32 "", index);

	setPlayerName(index, newName);
	if (NetPlay.isHost)
	{
		NETSendAllPlayerInfoTo(NET_ALL_PLAYERS);
	}
	else
	{
		ASSERT_OR_RETURN(false, index <= static_cast<uint32_t>(std::numeric_limits<uint8_t>::max()), "index (%" PRIu32 ") exceeds supported bounds", index);
		ASSERT_OR_RETURN(false, index == selectedPlayer, "Clients can only change their own name!");
		uint8_t player = static_cast<uint8_t>(index);
		WzString newNameStr = NetPlay.players[index].name;
		NETbeginEncode(NETnetQueue(NetPlay.hostPlayer), NET_PLAYERNAME_CHANGEREQUEST);
		NETuint8_t(&player);
		NETwzstring(newNameStr);
		NETend();
	}

	return true;
}

void NETfixDuplicatePlayerNames()
{
	char name[StringSize];
	unsigned i, j, pass;
	for (i = 1; i != MAX_CONNECTED_PLAYERS; ++i)
	{
		sstrcpy(name, NetPlay.players[i].name);
		if (name[0] == '\0' || !NetPlay.players[i].allocated)
		{
			continue;  // Ignore empty names.
		}
		for (pass = 0; pass != 101; ++pass)
		{
			if (pass != 0)
			{
				ssprintf(name, "%s_%X", NetPlay.players[i].name, pass + 1);
			}

			for (j = 0; j != i; ++j)
			{
				if (strcmp(name, NetPlay.players[j].name) == 0)
				{
					break;  // Duplicate name.
				}
			}

			if (i == j)
			{
				break;  // Unique name.
			}
		}
		if (pass != 0)
		{
			NETchangePlayerName(i, name);
		}
	}
}

// ////////////////////////////////////////////////////////////////////////
// return one of the four user flags in the current sessiondescription.
SDWORD NETgetGameFlags(UDWORD flag)
{
	if (flag < 1 || flag > 4)
	{
		return 0;
	}
	else
	{
		return NetGameFlags[flag - 1];
	}
}

static void NETsendGameFlags()
{
	ASSERT_HOST_ONLY(return);
	debug(LOG_NET, "sending game flags");
	NETbeginEncode(NETbroadcastQueue(), NET_GAME_FLAGS);
	{
		// Send the amount of game flags we're about to send
		uint8_t i, count = ARRAY_SIZE(NetGameFlags);
		NETuint8_t(&count);

		// Send over all game flags
		for (i = 0; i < count; ++i)
		{
			NETint32_t(&NetGameFlags[i]);
		}
	}
	NETend();
}

// ////////////////////////////////////////////////////////////////////////
// Set a game flag
bool NETsetGameFlags(UDWORD flag, SDWORD value)
{
	if (!NetPlay.bComms)
	{
		return true;
	}

	if (flag > 0 && flag < 5)
	{
		return (NetGameFlags[flag - 1] = value);
	}

	NETsendGameFlags();

	return true;
}

/**
 * @note \c game is being sent to the master server (if hosting)
 *       The implementation of NETsendGAMESTRUCT <em>must</em> guarantee to
 *       pack it in network byte order (big-endian).
 *
 * @return true on success, false when a socket error has occurred
 *
 * @see GAMESTRUCT,NETrecvGAMESTRUCT
 */
static bool NETsendGAMESTRUCT(Socket *sock, const GAMESTRUCT *ourgamestruct)
{
	// A buffer that's guaranteed to have the correct size (i.e. it
	// circumvents struct padding, which could pose a problem).  Initialise
	// to zero so that we can be sure we're not sending any (undefined)
	// memory content across the network.
	char buf[sizeof(ourgamestruct->GAMESTRUCT_VERSION) + sizeof(ourgamestruct->name) + sizeof(ourgamestruct->desc.host) + (sizeof(int32_t) * 8) +
	         sizeof(ourgamestruct->secondaryHosts) + sizeof(ourgamestruct->extra) + sizeof(ourgamestruct->hostPort) + sizeof(ourgamestruct->mapname) + sizeof(ourgamestruct->hostname) + sizeof(ourgamestruct->versionstring) +
	         sizeof(ourgamestruct->modlist) + (sizeof(uint32_t) * 9) ] = { 0 };
	char *buffer = buf;
	unsigned int i;
	ssize_t result;

	auto push32 = [&](uint32_t value) {
		uint32_t swapped = htonl(value);
		memcpy(buffer, &swapped, sizeof(swapped));
		buffer += sizeof(swapped);
	};

	auto push16 = [&](uint16_t value) {
		uint16_t swapped = htons(value);
		memcpy(buffer, &swapped, sizeof(swapped));
		buffer += sizeof(swapped);
	};

	// Now dump the data into the buffer
	// Copy 32bit large big endian numbers
	push32(ourgamestruct->GAMESTRUCT_VERSION);

	// Copy a string
	strlcpy(buffer, ourgamestruct->name, sizeof(ourgamestruct->name));
	buffer += sizeof(ourgamestruct->name);

	// Copy 32bit large big endian numbers
	push32(ourgamestruct->desc.dwSize);
	push32(ourgamestruct->desc.dwFlags);

	// Copy yet another string
	strlcpy(buffer, ourgamestruct->desc.host, sizeof(ourgamestruct->desc.host));
	buffer += sizeof(ourgamestruct->desc.host);

	// Copy 32bit large big endian numbers
	push32(ourgamestruct->desc.dwMaxPlayers);
	push32(ourgamestruct->desc.dwCurrentPlayers);
	for (i = 0; i < ARRAY_SIZE(ourgamestruct->desc.dwUserFlags); ++i)
	{
		push32(ourgamestruct->desc.dwUserFlags[i]);
	}

	// Copy a string
	for (i = 0; i < ARRAY_SIZE(ourgamestruct->secondaryHosts); ++i)
	{
		strlcpy(buffer, ourgamestruct->secondaryHosts[i], sizeof(ourgamestruct->secondaryHosts[i]));
		buffer += sizeof(ourgamestruct->secondaryHosts[i]);
	}

	// Copy a string
	strlcpy(buffer, ourgamestruct->extra, sizeof(ourgamestruct->extra));
	buffer += sizeof(ourgamestruct->extra);

	// Copy 16bit large big endian number
	push16(ourgamestruct->hostPort);

	// Copy a string
	strlcpy(buffer, ourgamestruct->mapname, sizeof(ourgamestruct->mapname));
	buffer += sizeof(ourgamestruct->mapname);

	// Copy a string
	strlcpy(buffer, ourgamestruct->hostname, sizeof(ourgamestruct->hostname));
	buffer += sizeof(ourgamestruct->hostname);

	// Copy a string
	strlcpy(buffer, ourgamestruct->versionstring, sizeof(ourgamestruct->versionstring));
	buffer += sizeof(ourgamestruct->versionstring);

	// Copy a string
	strlcpy(buffer, ourgamestruct->modlist, sizeof(ourgamestruct->modlist));
	buffer += sizeof(ourgamestruct->modlist);

	// Copy 32bit large big endian numbers
	push32(ourgamestruct->game_version_major);

	// Copy 32bit large big endian numbers
	push32(ourgamestruct->game_version_minor);

	// Copy 32bit large big endian numbers
	push32(ourgamestruct->privateGame);

	// Copy 32bit large big endian numbers
	push32(ourgamestruct->pureMap);

	// Copy 32bit large big endian numbers
	push32(ourgamestruct->Mods);

	// Copy 32bit large big endian numbers
	push32(ourgamestruct->gameId);

	// Copy 32bit large big endian numbers
	push32(ourgamestruct->limits);

	// Copy 32bit large big endian numbers
	push32(ourgamestruct->future3);

	// Copy 32bit large big endian numbers
	push32(ourgamestruct->future4);

	debug(LOG_NET, "sending GAMESTRUCT, size: %u", (unsigned int)sizeof(buf));

	// Send over the GAMESTRUCT
	result = writeAll(sock, buf, sizeof(buf));
	if (result == SOCKET_ERROR)
	{
		const int err = getSockErr();

		// If packet could not be sent, we should inform user of the error.
		debug(LOG_ERROR, "Failed to send GAMESTRUCT. Reason: %s", strSockError(getSockErr()));
		debug(LOG_ERROR, "Please make sure TCP ports %u & %u are open!", masterserver_port, gameserver_port);

		setSockErr(err);
		return false;
	}

	return true;
}

/**
 * @note \c game is being retrieved from the master server (if browsing the
 *       lobby). The implementation of NETrecvGAMESTRUCT should assume the data
 *       to be packed in network byte order (big-endian).
 *
 * @see GAMESTRUCT,NETsendGAMESTRUCT
 */
static bool NETrecvGAMESTRUCT(Socket *sock, GAMESTRUCT *ourgamestruct)
{
	// A buffer that's guaranteed to have the correct size (i.e. it
	// circumvents struct padding, which could pose a problem).
	char buf[sizeof(ourgamestruct->GAMESTRUCT_VERSION) + sizeof(ourgamestruct->name) + sizeof(ourgamestruct->desc.host) + (sizeof(int32_t) * 8) +
	         sizeof(ourgamestruct->secondaryHosts) + sizeof(ourgamestruct->extra) + sizeof(ourgamestruct->hostPort) + sizeof(ourgamestruct->mapname) + sizeof(ourgamestruct->hostname) + sizeof(ourgamestruct->versionstring) +
	         sizeof(ourgamestruct->modlist) + (sizeof(uint32_t) * 9) ] = { 0 };
	char *buffer = buf;
	unsigned int i;
	ssize_t result = 0;

	auto pop32 = [&]() -> uint32_t {
		uint32_t value = 0;
		memcpy(&value, buffer, sizeof(value));
		value = ntohl(value);
		buffer += sizeof(value);
		return value;
	};

	auto pop16 = [&]() -> uint16_t {
		uint16_t value = 0;
		memcpy(&value, buffer, sizeof(value));
		value = ntohs(value);
		buffer += sizeof(value);
		return value;
	};

	// Read a GAMESTRUCT from the connection
	result = readAll(sock, buf, sizeof(buf), NET_TIMEOUT_DELAY);
	bool failed = false;
	if (result == SOCKET_ERROR)
	{
		debug(LOG_ERROR, "Lobby server connection error: %s", strSockError(getSockErr()));
		failed = true;
	}
	else if ((unsigned)result != sizeof(buf))
	{
		debug(LOG_ERROR, "GAMESTRUCT recv timed out; received %d bytes; expecting %d", (int)result, (int)sizeof(buf));
		failed = true;
	}
	if (failed)
	{
		// caller handles invalidating and closing tcp_socket
		return false;
	}

	// Now dump the data into the game struct
	// Copy 32bit large big endian numbers
	ourgamestruct->GAMESTRUCT_VERSION = pop32();
	// Copy a string
	sstrcpy(ourgamestruct->name, buffer);
	buffer += sizeof(ourgamestruct->name);

	// Copy 32bit large big endian numbers
	ourgamestruct->desc.dwSize = pop32();
	ourgamestruct->desc.dwFlags = pop32();

	// Copy yet another string
	sstrcpy(ourgamestruct->desc.host, buffer);
	buffer += sizeof(ourgamestruct->desc.host);

	// Copy 32bit large big endian numbers
	ourgamestruct->desc.dwMaxPlayers = pop32();
	ourgamestruct->desc.dwCurrentPlayers = pop32();
	for (i = 0; i < ARRAY_SIZE(ourgamestruct->desc.dwUserFlags); ++i)
	{
		ourgamestruct->desc.dwUserFlags[i] = pop32();
	}

	// Copy a string
	for (i = 0; i < ARRAY_SIZE(ourgamestruct->secondaryHosts); ++i)
	{
		sstrcpy(ourgamestruct->secondaryHosts[i], buffer);
		buffer += sizeof(ourgamestruct->secondaryHosts[i]);
	}

	// Copy a string
	sstrcpy(ourgamestruct->extra, buffer);
	buffer += sizeof(ourgamestruct->extra);

	// Copy 16-bit host port
	ourgamestruct->hostPort = pop16();

	// Copy a string
	sstrcpy(ourgamestruct->mapname, buffer);
	buffer += sizeof(ourgamestruct->mapname);

	// Copy a string
	sstrcpy(ourgamestruct->hostname, buffer);
	buffer += sizeof(ourgamestruct->hostname);

	// Copy a string
	sstrcpy(ourgamestruct->versionstring, buffer);
	buffer += sizeof(ourgamestruct->versionstring);

	// Copy a string
	sstrcpy(ourgamestruct->modlist, buffer);
	buffer += sizeof(ourgamestruct->modlist);

	// Copy 32bit large big endian numbers
	ourgamestruct->game_version_major = pop32();
	ourgamestruct->game_version_minor = pop32();
	ourgamestruct->privateGame = pop32();
	ourgamestruct->pureMap = pop32();
	ourgamestruct->Mods = pop32();
	ourgamestruct->gameId = pop32();
	ourgamestruct->limits = pop32();
	ourgamestruct->future3 = pop32();
	ourgamestruct->future4 = pop32();

	debug(LOG_NET, "received GAMESTRUCT");

	return true;
}

// This function is run in its own thread! Do not call any non-threadsafe functions!
static void upnp_init(std::atomic_int &retval)
{
	struct UPNPDev *devlist;
	struct UPNPDev *dev;
	char *descXML;
	int result = 0;
	int descXMLsize = 0;
	memset(&urls, 0, sizeof(struct UPNPUrls));
	memset(&data, 0, sizeof(struct IGDdatas));

	debug(LOG_NET, "Searching for UPnP devices for automatic port forwarding...");
#if defined(MINIUPNPC_API_VERSION) && (MINIUPNPC_API_VERSION >= 14)
	devlist = upnpDiscover(3000, nullptr, nullptr, 0, 0, 2, &result);
#else
	devlist = upnpDiscover(3000, nullptr, nullptr, 0, 0, &result);
#endif
	debug(LOG_NET, "UPnP device search finished.");

	if (devlist)
	{
		dev = devlist;
		while (dev)
		{
			if (strstr(dev->st, "InternetGatewayDevice"))
			{
				break;
			}
			dev = dev->pNext;
		}
		if (!dev)
		{
			dev = devlist; /* defaulting to first device */
		}

		debug(LOG_NET, "UPnP device found: %s %s\n", dev->descURL, dev->st);

#if defined(MINIUPNPC_API_VERSION) && (MINIUPNPC_API_VERSION >= 16)
		int status_code = -1;
		descXML = (char *)miniwget_getaddr(dev->descURL, &descXMLsize, lanaddr, sizeof(lanaddr), dev->scope_id, &status_code);
		if (status_code != 200)
		{
			if (descXML)
			{
				free(descXML);
				descXML = nullptr;
			}
			debug(LOG_NET, "HTTP error %d fetching: %s", status_code, (dev->descURL) ? dev->descURL : "");
		}
#else
		descXML = (char *)miniwget_getaddr(dev->descURL, &descXMLsize, lanaddr, sizeof(lanaddr), dev->scope_id);
#endif
		debug(LOG_NET, "LAN address: %s", lanaddr);
		if (descXML)
		{
			parserootdesc(descXML, descXMLsize, &data);
			free(descXML); descXML = nullptr;
			GetUPNPUrls(&urls, &data, dev->descURL, dev->scope_id);
		}

		debug(LOG_NET, "UPnP device found: %s %s LAN address %s", dev->descURL, dev->st, lanaddr);
		freeUPNPDevlist(devlist);

		if (!urls.controlURL || urls.controlURL[0] == '\0')
		{
			retval = UPNP_ERROR_CONTROL_NOT_AVAILABLE;
		}
		else
		{
			retval = UPNP_SUCCESS;
		}
	}
	else
	{
		retval = UPNP_ERROR_DEVICE_NOT_FOUND;
	}
}

static bool upnp_add_redirect(int port)
{
	char port_str[16];
	char buf[512] = {'\0'};
	int r;

	debug(LOG_NET, "upnp_add_redir(%d)", port);
	sprintf(port_str, "%d", port);
	r = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
	                        port_str, port_str, lanaddr, "Warzone 2100", "TCP", nullptr, "0");	// "0" = lease time unlimited
	if (r != UPNPCOMMAND_SUCCESS)
	{
		ssprintf(buf, _("Could not open required port (%s) on  (%s)"), port_str, lanaddr);
		debug(LOG_NET, "%s", buf);
		addConsoleMessage(buf, DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
		// beware of writing a line too long, it screws up console line count. \n is safe for line split
		ssprintf(buf, _("You must manually configure your router & firewall to\n open port %d before you can host a game."), NETgetGameserverPort());
		addConsoleMessage(buf, DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
		return false;
	}

	r = UPNP_GetExternalIPAddress(urls.controlURL, data.first.servicetype, externalIPAddress);
	if (r != UPNPCOMMAND_SUCCESS)
	{
		ssprintf(externalIPAddress, "%s", "???");
	}
	ssprintf(buf, _("Game configured port (%s) correctly on (%s)\nYour external IP is %s"), port_str, lanaddr, externalIPAddress);
	addConsoleMessage(buf, DEFAULT_JUSTIFY, NOTIFY_MESSAGE);

	return true;
}

static int upnp_rem_redirect(int port)
{
	char port_str[16];
	debug(LOG_NET, "upnp_rem_redir(%d)", port);
	sprintf(port_str, "%d", port);
	return UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, port_str, "TCP", nullptr);
}

void NETaddRedirects()
{
	if (upnp_add_redirect(gameserver_port))
	{
		debug(LOG_NET, "successful!");
		NetPlay.isUPNP_CONFIGURED = true;
		NetPlay.isUPNP_ERROR = false;
	}
	else
	{
		debug(LOG_NET, "failed!");
		NetPlay.isUPNP_CONFIGURED = false;
		NetPlay.isUPNP_ERROR = true;
	}
}

void NETremRedirects()
{
	debug(LOG_NET, "upnp is %d", NetPlay.isUPNP_CONFIGURED);
	if (NetPlay.isUPNP_CONFIGURED)
	{
		int result = upnp_rem_redirect(gameserver_port);
		if (!result)
		{
			debug(LOG_NET, "removed UPnP entry.");
		}
		else
		{
			debug(LOG_ERROR, "Failed to remove UPnP entry for the game. You must manually remove it from your router. (%d)", result);
		}
	}
}

void NETdiscoverUPnPDevices()
{
	if (!NetPlay.isUPNP_CONFIGURED && NetPlay.isUPNP)
	{
		wz::thread t(upnp_init, std::ref(upnp_status));
		t.detach();
	}
	else if (!NetPlay.isUPNP)
	{
		debug(LOG_INFO, "UPnP detection disabled by user.");
	}
}

// ////////////////////////////////////////////////////////////////////////
// setup stuff
int NETinit(bool bFirstCall)
{
	debug(LOG_NET, "NETinit");
	upnp_status = 0;
	NETlogEntry("NETinit!", SYNC_FLAG, selectedPlayer);
	NET_InitPlayers(true, true);

	SOCKETinit();

	if (bFirstCall)
	{
		debug(LOG_NET, "NETPLAY: Init called, MORNIN'");

		// NOTE NetPlay.isUPNP is already set in configuration.c!
		NetPlay.bComms = true;
		NetPlay.GamePassworded = false;
		NetPlay.ShowedMOTD = false;
		NetPlay.isHostAlive = false;
		NetPlay.HaveUpgrade = false;
		NetPlay.gamePassword[0] = '\0';
		NetPlay.MOTD = nullptr;
		NETstartLogging();
	}

	if (NetPlay.MOTD)
	{
		free(NetPlay.MOTD);
	}
	NetPlay.MOTD = nullptr;
	NetPlay.ShowedMOTD = false;
	NetPlay.GamePassworded = false;
	memset(&sync_counter, 0x0, sizeof(sync_counter));	//clear counters

	return 0;
}

// ////////////////////////////////////////////////////////////////////////
// SHUTDOWN THE CONNECTION.
int NETshutdown()
{
	debug(LOG_NET, "NETshutdown");
	NETlogEntry("NETshutdown", SYNC_FLAG, selectedPlayer);
	if (NetPlay.bComms && NetPlay.isUPNP)
	{
		NETremRedirects();
		NetPlay.isUPNP_CONFIGURED = false;
		NetPlay.isUPNP_ERROR = false;
	}
	NETstopLogging();
	NETpermissionsShutdown();
	if (NetPlay.MOTD)
	{
		free(NetPlay.MOTD);
	}
	NetPlay.MOTD = nullptr;
	NETdeleteQueue();
	SOCKETshutdown();

	// Reset net usage statistics.
	nStats = nZeroStats;
	nStatsLastSec = nZeroStats;
	nStatsSecondLastSec = nZeroStats;

	return 0;
}

// ////////////////////////////////////////////////////////////////////////
//close the open game..
int NETclose()
{
	unsigned int i;

	// reset flag
	NetPlay.ShowedMOTD = false;
	NEThaltJoining();

	debug(LOG_NET, "Terminating sockets.");

	NetPlay.isHost = false;
	server_not_there = false;
	allow_joining = false;

	for (i = 0; i < MAX_CONNECTED_PLAYERS; i++)
	{
		if (connected_bsocket[i])
		{
			debug(LOG_NET, "Closing connected_bsocket[%u], %p", i, static_cast<void *>(connected_bsocket[i]));
			socketClose(connected_bsocket[i]);
			connected_bsocket[i] = nullptr;
		}
		NET_DestroyPlayer(i, true);
	}

	if (tmp_socket_set)
	{
		debug(LOG_NET, "Freeing tmp_socket_set %p", static_cast<void *>(tmp_socket_set));
		deleteSocketSet(tmp_socket_set);
		tmp_socket_set = nullptr;
	}

	for (i = 0; i < MAX_TMP_SOCKETS; i++)
	{
		if (tmp_socket[i])
		{
			// FIXME: need SocketSet_DelSocket() as well, socket_set or tmp_socket_set?
			debug(LOG_NET, "Closing tmp_socket[%d] %p", i, static_cast<void *>(tmp_socket[i]));
			socketClose(tmp_socket[i]);
			tmp_socket[i] = nullptr;
		}
	}

	if (socket_set)
	{
		// checking to make sure tcp_socket is still valid
		if (tcp_socket)
		{
			SocketSet_DelSocket(socket_set, tcp_socket);
		}
		if (bsocket)
		{
			SocketSet_DelSocket(socket_set, bsocket);
		}
		debug(LOG_NET, "Freeing socket_set %p", static_cast<void *>(socket_set));
		deleteSocketSet(socket_set);
		socket_set = nullptr;
	}
	if (tcp_socket)
	{
		debug(LOG_NET, "Closing tcp_socket %p", static_cast<void *>(tcp_socket));
		socketClose(tcp_socket);
		tcp_socket = nullptr;
	}
	if (bsocket)
	{
		debug(LOG_NET, "Closing bsocket %p", static_cast<void *>(bsocket));
		socketClose(bsocket);
		bsocket = nullptr;
	}

	playerManagementRecord.clear();

	return 0;
}


// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Send and Recv functions

// ////////////////////////////////////////////////////////////////////////
// return bytes of data sent recently.
size_t NETgetStatistic(NetStatisticType type, bool sent, bool isTotal)
{
	size_t Statistic::*statisticType = sent ? &Statistic::sent : &Statistic::received;
	Statistic NETSTATS::*statsType;
	switch (type)
	{
	case NetStatisticRawBytes:          statsType = &NETSTATS::rawBytes;          break;
	case NetStatisticUncompressedBytes: statsType = &NETSTATS::uncompressedBytes; break;
	case NetStatisticPackets:           statsType = &NETSTATS::packets;           break;
	default: ASSERT(false, " "); return 0;
	}

	int time = wzGetTicks();
	if ((unsigned)(time - nStatsLastUpdateTime) >= (unsigned)GAME_TICKS_PER_SEC)
	{
		nStatsLastUpdateTime = time;
		nStatsSecondLastSec = nStatsLastSec;
		nStatsLastSec = nStats;
	}

	if (isTotal)
	{
		return nStats.*statsType.*statisticType;
	}
	return nStatsLastSec.*statsType.*statisticType - nStatsSecondLastSec.*statsType.*statisticType;
}

static std::set<uint32_t> netSendPendingDisconnectPlayerIndexes;

void NETsendProcessDelayedActions()
{
	if (netSendPendingDisconnectPlayerIndexes.empty())
	{
		return;
	}

	// "consume" the netSendPendingDisconnectPlayerIndexes up-front, and do *not* reference it again (as NETplayerClientsDisconnect may lead to calls that mutate it)
	std::set<uint32_t> pendingDisconnectPlayers = netSendPendingDisconnectPlayerIndexes;
	netSendPendingDisconnectPlayerIndexes.clear();

	for (auto player : pendingDisconnectPlayers)
	{
		if (player >= MAX_CONNECTED_PLAYERS)
		{
			ASSERT(player < MAX_CONNECTED_PLAYERS, "Invalid player: %" PRIu32, player);
			continue;
		}
		NETlogEntry("client disconnect?", SYNC_FLAG, player);
	}

	NETplayerClientsDisconnect(pendingDisconnectPlayers);
}

// ////////////////////////////////////////////////////////////////////////
// Send a message to a player, option to guarantee message
bool NETsend(NETQUEUE queue, NetMessage const *message)
{
	uint8_t player = queue.index;
	ssize_t result = 0;

	if (!NetPlay.bComms)
	{
		return true;
	}

	Socket **sockets = connected_bsocket;
	bool isTmpQueue = false;
	switch (queue.queueType)
	{
	case QUEUE_BROADCAST:
		ASSERT_OR_RETURN(false, player == NET_ALL_PLAYERS, "Wrong queue index.");
		break;
	case QUEUE_NET:
		ASSERT_OR_RETURN(false, player < MAX_CONNECTED_PLAYERS, "Wrong queue index.");
		break;
	case QUEUE_TMP:
		sockets = tmp_socket;
		isTmpQueue = true;

		ASSERT_OR_RETURN(false, player < MAX_TMP_SOCKETS && NetPlay.isHost, "Wrong queue index.");
		break;
	default:
		ASSERT_OR_RETURN(false, false, "Wrong queue type.");
	}

	if (NetPlay.isHost)
	{
		int firstPlayer = player == NET_ALL_PLAYERS ? 0                         : player;
		int lastPlayer  = player == NET_ALL_PLAYERS ? MAX_CONNECTED_PLAYERS - 1 : player;
		for (player = firstPlayer; player <= lastPlayer; ++player)
		{
			// We are the host, send directly to player.
			if (sockets[player] != nullptr && player != queue.exclude)
			{
				uint8_t *rawData = message->rawDataDup();
				if (!rawData)
				{
					debug(LOG_FATAL, "Failed to allocate raw data (message type: %" PRIu8 ", player: %d)", message->type, player);
					abort();
				}
				ssize_t rawLen   = message->rawLen();
				size_t compressedRawLen;
				result = writeAll(sockets[player], rawData, rawLen, &compressedRawLen);
				delete[] rawData;  // Done with the data.

				if (result == rawLen)
				{
					nStats.rawBytes.sent          += compressedRawLen;
					nStats.uncompressedBytes.sent += rawLen;
					nStats.packets.sent           += 1;
				}
				else if (result == SOCKET_ERROR)
				{
					// Write error, most likely client disconnect.
					debug(LOG_ERROR, "Failed to send message (type: %" PRIu8 ", rawLen: %zu, compressedRawLen: %zu) to %" PRIu8 ": %s", message->type, message->rawLen(), compressedRawLen, player, strSockError(getSockErr()));
					if (!isTmpQueue)
					{
						netSendPendingDisconnectPlayerIndexes.insert(player);
					}
				}
			}
		}
		return true;
	}
	else if (player == NetPlay.hostPlayer)
	{
		// We are a client, send directly to player, who happens to be the host.
		if (bsocket)
		{
			uint8_t *rawData = message->rawDataDup();
			ssize_t rawLen   = message->rawLen();
			size_t compressedRawLen;
			result = writeAll(bsocket, rawData, rawLen, &compressedRawLen);
			delete[] rawData;  // Done with the data.

			if (result == rawLen)
			{
				nStats.rawBytes.sent          += compressedRawLen;
				nStats.uncompressedBytes.sent += rawLen;
				nStats.packets.sent           += 1;
			}
			else if (result == SOCKET_ERROR)
			{
				// Write error, most likely host disconnect.
				debug(LOG_ERROR, "Failed to send message: %s", strSockError(getSockErr()));
				debug(LOG_ERROR, "Host connection was broken, socket %p.", static_cast<void *>(bsocket));
				NETlogEntry("write error--client disconnect.", SYNC_FLAG, player);
				SocketSet_DelSocket(socket_set, bsocket);            // mark it invalid
				socketClose(bsocket);
				bsocket = nullptr;
				NetPlay.players[NetPlay.hostPlayer].heartbeat = false;	// mark host as dead
				//Game is pretty much over --should just end everything when HOST dies.
				NetPlay.isHostAlive = false;
			}

			return result == rawLen;
		}
	}
	else
	{
		// We are a client and can't send the data directly, ask the host to send the data to the player.
		uint8_t sender = selectedPlayer;
		NETbeginEncode(NETnetQueue(NetPlay.hostPlayer), NET_SEND_TO_PLAYER);
		NETuint8_t(&sender);
		NETuint8_t(&player);
		NETnetMessage(&message);
		NETend();
	}

	return false;
}

void NETflush()
{
	if (!NetPlay.bComms)
	{
		return;
	}

	NETflushGameQueues();

	size_t compressedRawLen;
	if (NetPlay.isHost)
	{
		for (int player = 0; player < MAX_CONNECTED_PLAYERS; ++player)
		{
			// We are the host, send directly to player.
			if (connected_bsocket[player] != nullptr)
			{
				socketFlush(connected_bsocket[player], player, &compressedRawLen);
				nStats.rawBytes.sent += compressedRawLen;
			}
		}
		for (int player = 0; player < MAX_TMP_SOCKETS; ++player)
		{
			// We are the host, send directly to player.
			if (tmp_socket[player] != nullptr)
			{
				socketFlush(tmp_socket[player], std::numeric_limits<uint8_t>::max(), &compressedRawLen);
				nStats.rawBytes.sent += compressedRawLen;
			}
		}
	}
	else
	{
		if (bsocket != nullptr)
		{
			socketFlush(bsocket, NetPlay.hostPlayer, &compressedRawLen);
			nStats.rawBytes.sent += compressedRawLen;
		}
	}
}

// Player index swapping

// Warning: This should be used with care, and generally should not be called directly.
// Use higher-level functions like NETmovePlayerToSpectatorOnlySlot & NETrequestSpectatorToPlay instead.
static bool swapPlayerIndexes(uint32_t playerIndexA, uint32_t playerIndexB)
{
	ASSERT_OR_RETURN(false, ingame.localJoiningInProgress, "Only supported in lobby, before game starts");
	ASSERT_OR_RETURN(false, playerIndexA < MAX_CONNECTED_PLAYERS, "playerIndexA out of bound: %" PRIu32 "", playerIndexA);
	ASSERT_OR_RETURN(false, playerIndexB < MAX_CONNECTED_PLAYERS, "playerIndexB out of bound: %" PRIu32 "", playerIndexB);
	ASSERT_OR_RETURN(false, playerIndexA != NetPlay.hostPlayer && playerIndexA != NetPlay.hostPlayer && playerIndexB != NetPlay.hostPlayer && playerIndexB != NetPlay.hostPlayer, "Can't swap host player index: (index A: %" PRIu32 ", index B: %" PRIu32 ")", playerIndexA, playerIndexB);

	// Send the NET_SWAPPING_PLAYER_INDEX message *first*
	NETbeginEncode(NETbroadcastQueue(), NET_PLAYER_SWAP_INDEX);
	NETuint32_t(&playerIndexA);
	NETuint32_t(&playerIndexB);
	NETend();

	// Then swap the networking stuff for these slots
	std::swap(connected_bsocket[playerIndexA], connected_bsocket[playerIndexB]);
	// should be no need to call SocketSet_AddSocket, since should already be in the socket_set
	NETswapQueues(NETnetQueue(playerIndexA), NETnetQueue(playerIndexB));

	// Backup the old NETPLAY PLAYERS data
	std::array<PLAYER, 2> playersData = {std::move(NetPlay.players[playerIndexA]), std::move(NetPlay.players[playerIndexB])};

	// Instead of calling clearPlayer() which has all kinds of unintended effects,
	// hmmm...
	std::array<uint32_t, 2> playerIndexes = {playerIndexA, playerIndexB};
	for (auto playerIndex : playerIndexes)
	{
		// From MultiPlayerLeave()
		//  From clearPlayer() - this is needed to disconnect PlayerReferences so, for example, old chat messages are still associated with the proper player
		NetPlay.playerReferences[playerIndex]->disconnect();
		NetPlay.playerReferences[playerIndex] = std::make_shared<PlayerReference>(playerIndex);

		//
//		if (playerIndex < MAX_PLAYERS)
//		{
//			playerVotes[playerIndex] = 0;
//		}
	}

	// Swap the player multistats / identity info
	swapPlayerMultiStatsLocal(playerIndexA, playerIndexB);

	// Swap the NetPlay PLAYER entries
	NetPlay.players[playerIndexB] = std::move(playersData[0]);
	NetPlay.players[playerIndexA] = std::move(playersData[1]);
	// Just like changePosition(), we should *preserve* the team and "position" from the original index
	std::swap(NetPlay.players[playerIndexA].position, NetPlay.players[playerIndexB].position);
	std::swap(NetPlay.players[playerIndexA].team, NetPlay.players[playerIndexB].team);
	// And also swap spectator flag!
	std::swap(NetPlay.players[playerIndexA].isSpectator, NetPlay.players[playerIndexB].isSpectator);
	std::swap(NetPlay.players[playerIndexA].colour, NetPlay.players[playerIndexB].colour); // And the color!
	// (Essentially all of the above are "slot-associated" more than "player-associated" properties, and so should stay with the slot)

	// Swap certain ingame player-associated entries
	std::swap(ingame.PingTimes[playerIndexA], ingame.PingTimes[playerIndexB]);
	std::swap(ingame.LagCounter[playerIndexA], ingame.LagCounter[playerIndexB]);
	std::swap(ingame.VerifiedIdentity[playerIndexA], ingame.VerifiedIdentity[playerIndexB]);
	std::swap(ingame.JoiningInProgress[playerIndexA], ingame.JoiningInProgress[playerIndexB]);
	std::swap(ingame.PendingDisconnect[playerIndexA], ingame.PendingDisconnect[playerIndexB]);
	std::swap(ingame.DataIntegrity[playerIndexA], ingame.DataIntegrity[playerIndexB]);
	std::swap(ingame.hostChatPermissions[playerIndexA], ingame.hostChatPermissions[playerIndexB]);
	std::swap(ingame.lastSentPlayerDataCheck2[playerIndexA], ingame.lastSentPlayerDataCheck2[playerIndexB]);
	std::swap(ingame.muteChat[playerIndexA], ingame.muteChat[playerIndexB]);
	multiSyncPlayerSwap(playerIndexA, playerIndexB);

	// Ensure we filter messages appropriately waiting for the client ack *at each new index*
	if (NetPlay.players[playerIndexA].allocated)
	{
		NET_waitingForIndexChangeAckSince.at(playerIndexA) = realTime;
	}
	if (NetPlay.players[playerIndexB].allocated)
	{
		NET_waitingForIndexChangeAckSince.at(playerIndexB) = realTime;
	}

	// Fix up any AI players - if they get moved to a spectator slot, get rid of the AI
	for (auto playerIndex : playerIndexes)
	{
		if (!NetPlay.players[playerIndex].allocated
			&& NetPlay.players[playerIndex].ai >= 0
			&& (NetPlay.players[playerIndex].isSpectator || playerIndex > MAX_PLAYERS))
		{
			// remove the AI
			ASSERT(playerIndex != PLAYER_FEATURE, "Not expecting to see PLAYER_FEATURE here!");
			NetPlay.players[playerIndex].difficulty =  AIDifficulty::DISABLED;
			NetPlay.players[playerIndex].ai = AI_OPEN;
			clearPlayerName(playerIndex);
		}
		if (!NetPlay.players[playerIndex].allocated && NetPlay.players[playerIndex].ai < 0)
		{
			ASSERT(NetPlay.players[playerIndex].difficulty == AIDifficulty::DISABLED, "Unexpected difficulty (%d) for player (%d)", (int)NetPlay.players[playerIndex].difficulty, playerIndex);
		}

		if (NetPlay.players[playerIndex].allocated
			&& playerIndex < MAX_PLAYERS)
		{
			if (NetPlay.players[playerIndex].difficulty != AIDifficulty::HUMAN)
			{
				debug(LOG_INFO, "Fixing up human difficulty for player: %d", playerIndex);
				NetPlay.players[playerIndex].difficulty = AIDifficulty::HUMAN;
			}
		}
	}

	if (!NetPlay.players[playerIndexA].allocated || !NetPlay.players[playerIndexB].allocated)
	{
		// Recalculate NetPlay.playercount (etc), as it may have changed
		NETfixPlayerCount();
	}

	return true;
}

static inline bool _NET_isSpectatorOnlySlot(UDWORD playerIdx)
{
	ASSERT_OR_RETURN(false, playerIdx < NetPlay.players.size(), "Invalid playerIdx: %" PRIu32 "", playerIdx);
	return playerIdx >= MAX_PLAYERS || NetPlay.players[playerIdx].position >= game.maxPlayers;
}

static optional<uint32_t> _NET_findSpectatorSlotToOpen()
{
	for (uint32_t i = MAX_PLAYER_SLOTS; i < MAX_CONNECTED_PLAYERS; ++i)
	{
		if (!_NET_isSpectatorOnlySlot(i))
		{
			continue;
		}
		if (NetPlay.players[i].allocated || NetPlay.players[i].isSpectator)
		{
			continue;
		}
		if (game.mapHasScavengers && NetPlay.players[i].position == scavengerSlot())
		{
			continue; // skip it
		}
		if (i == PLAYER_FEATURE)
		{
			continue; // skip it
		}

		return i;
	}
	return nullopt;
}

bool NETcanOpenNewSpectatorSlot()
{
	return _NET_findSpectatorSlotToOpen().has_value();
}

static optional<uint32_t> _NET_openNewSpectatorSlot_internal(bool broadcastUpdate)
{
	ASSERT_HOST_ONLY(return false);
	auto newSpecSlot = _NET_findSpectatorSlotToOpen();
	if (!newSpecSlot.has_value())
	{
		return nullopt;
	}

	uint32_t i = newSpecSlot.value();
	NetPlay.players[i].ai = AI_OPEN;
	NetPlay.players[i].isSpectator = true;
	// common code
	NetPlay.players[i].difficulty = AIDifficulty::DISABLED; // disable AI for this slot
	if (broadcastUpdate)
	{
		NETBroadcastPlayerInfo(i);
		netPlayersUpdated = true;
	}
	return i;
}

bool NETopenNewSpectatorSlot()
{
	ASSERT_HOST_ONLY(return false);
	return _NET_openNewSpectatorSlot_internal(true).has_value();
}

bool NETmovePlayerToSpectatorOnlySlot(uint32_t playerIdx, bool hostOverride /*= false*/)
{
	ASSERT_HOST_ONLY(return false);
	ASSERT_OR_RETURN(false, playerIdx < MAX_CONNECTED_PLAYERS, "playerIdx out of bounds: %" PRIu32 "", playerIdx);
	// Verify it's a human player
	ASSERT_OR_RETURN(false, isHumanPlayer(playerIdx) && !NetPlay.players[playerIdx].isSpectator, "playerIdx is not a currently-connected human player: %" PRIu32 "", playerIdx);

	// Try to grab a new spectator-only slot index
	optional<uint32_t> availableSpectatorIndex = NET_FindOpenSlotForPlayer(false, true);
	if (!availableSpectatorIndex.has_value() && hostOverride)
	{
		// Attempt to open a new spectator slot just for this player
		availableSpectatorIndex = _NET_openNewSpectatorSlot_internal(false);
	}
	if (!availableSpectatorIndex.has_value())
	{
		debug(LOG_ERROR, "No available spectator slots to move player %" PRIu32 " to", playerIdx);
		return false;
	}

	// Backup the player's identity for later recording
	auto playerPublicKeyIdentity = getMultiStats(playerIdx).identity.toBytes(EcKey::Privacy::Public);

	// Swap the player indexes
	if (!swapPlayerIndexes(playerIdx, availableSpectatorIndex.value()))
	{
		debug(LOG_ERROR, "Failed to swap player indexes: %" PRIu32 ", %" PRIu32 "", playerIdx, availableSpectatorIndex.value());
		return false;
	}
	ASSERT(NetPlay.players[availableSpectatorIndex.value()].isSpectator, "New slot doesn't have spectator set??");

	playerManagementRecord.movedPlayerToSpectators(NetPlay.players[availableSpectatorIndex.value()], playerPublicKeyIdentity, hostOverride);

	// Broadcast the swapped player info
	NETBroadcastTwoPlayerInfo(playerIdx, availableSpectatorIndex.value());

	return true;
}

static bool wasAlreadyMovedToSpectatorsByHost(uint32_t playerIdx)
{
	return playerManagementRecord.hostMovedPlayerToSpectators(NetPlay.players[playerIdx].IPtextAddress)
		|| playerManagementRecord.hostMovedPlayerToSpectators(getMultiStats(playerIdx).identity.toBytes(EcKey::Privacy::Public));
}

SpectatorToPlayerMoveResult NETmoveSpectatorToPlayerSlot(uint32_t playerIdx, optional<uint32_t> newPlayerIdx, bool hostOverride /*= false*/)
{
	ASSERT_HOST_ONLY(return SpectatorToPlayerMoveResult::FAILED);
	ASSERT_OR_RETURN(SpectatorToPlayerMoveResult::FAILED, playerIdx < MAX_CONNECTED_PLAYERS, "playerIdx out of bounds: %" PRIu32 "", playerIdx);
	ASSERT_OR_RETURN(SpectatorToPlayerMoveResult::FAILED, newPlayerIdx.value_or(0) < MAX_CONNECTED_PLAYERS, "newPlayerIdx out of bounds: %" PRIu32 "", newPlayerIdx.value_or(0));
	ASSERT_OR_RETURN(SpectatorToPlayerMoveResult::FAILED, !newPlayerIdx.has_value() || newPlayerIdx.value() != NetPlay.hostPlayer, "newPlayerIdx cannot be host player: %" PRIu32 "", newPlayerIdx.value_or(0));
	// Verify it's a human player
	ASSERT_OR_RETURN(SpectatorToPlayerMoveResult::FAILED, isHumanPlayer(playerIdx) && NetPlay.players[playerIdx].isSpectator, "playerIdx is not a currently-connected spectator: %" PRIu32 "", playerIdx);

	// Check if the host has moved this player to a spectator before, if so deny (unless the *host* is the one triggering the move)
	if (!hostOverride && wasAlreadyMovedToSpectatorsByHost(playerIdx))
	{
		return SpectatorToPlayerMoveResult::FAILED;
	}

	if (!newPlayerIdx.has_value())
	{
		// Attempt to find an open slot
		newPlayerIdx = NET_FindOpenSlotForPlayer(false, false);
		if (!newPlayerIdx.has_value())
		{
			return SpectatorToPlayerMoveResult::NEEDS_SLOT_SELECTION;
		}
	}

	// Backup the spectator's identity for later recording
	auto spectatorPublicKeyIdentity = getMultiStats(playerIdx).identity.toBytes(EcKey::Privacy::Public);

	// Swap the player indexes
	if (!swapPlayerIndexes(playerIdx, newPlayerIdx.value()))
	{
		debug(LOG_ERROR, "Failed to swap player indexes: %" PRIu32 ", %" PRIu32 "", playerIdx, newPlayerIdx.value());
		return SpectatorToPlayerMoveResult::FAILED;
	}
	ASSERT(!NetPlay.players[newPlayerIdx.value()].isSpectator, "New slot should not be a spectator??");

	playerManagementRecord.movedSpectatorToPlayers(NetPlay.players[newPlayerIdx.value()], spectatorPublicKeyIdentity, hostOverride);

	// Broadcast the swapped player info
	NETBroadcastTwoPlayerInfo(playerIdx, newPlayerIdx.value());

	return SpectatorToPlayerMoveResult::SUCCESS;
}

static inline bool NETFilterMessageWhileSwappingPlayer(uint8_t sender, uint8_t type)
{
	if (!NetPlay.isHost)
	{
		return false; // only host filters these messages
	}

	if (sender == NetPlay.hostPlayer)
	{
		return false; // never filter host messages
	}

	if (static_cast<size_t>(sender) >= NET_waitingForIndexChangeAckSince.size() || !NET_waitingForIndexChangeAckSince.at(sender).has_value())
	{
		return false; // no filtering - not waiting for player to acknowledge index change
	}

	#define INDEX_CHANGE_ACK_TIMEOUT (5 * GAME_TICKS_PER_SEC)
	if ((realTime - NET_waitingForIndexChangeAckSince.at(sender).value_or(realTime)) > INDEX_CHANGE_ACK_TIMEOUT)
	{
		// this client did not acknowledge the player index change before the timeout - kick them
		char msg[256] = {'\0'};
		if (NETplayerHasConnection(sender) || NetPlay.players[sender].allocated)
		{
			ssprintf(msg, "Auto-kicking player %u, did not ack player index change within required timeframe.", (unsigned int)sender);
			sendInGameSystemMessage(msg);
			debug(LOG_INFO, "Client (player: %u) failed to ack player index swap (ignoring message type: %" PRIu8 ")", sender, type);
			kickPlayer(sender, _("Client failed to ack player index swap"), ERROR_INVALID, false);
		}
		return true; // filter original message, of course
	}

	// Filter certain net messages if player index swap is in progress (until player acknowledges swap)

	switch (type)
	{
	case NET_TEXTMSG:
	case NET_QUICK_CHAT_MSG:
		// Just send a message to the player that this text message was undelivered and to try again - it's easier and this should be quite rare
		{
			WzQuickChatTargeting targeting;
			targeting.specificPlayers.insert(sender);
			sendQuickChat(WzQuickChatMessage::INTERNAL_MSG_DELIVERY_FAILURE_TRY_AGAIN, selectedPlayer, targeting);
			return true; // filter / ignore
		}
	case NET_VOTE:
		// POSSIBLE TODO: Send the player a new VOTE_REQUEST so they can send a new vote? (after they ack the swap)
		return true; // filter / ignore
	case NET_PLAYERNAME_CHANGEREQUEST:	///< non-host human player is changing their name.
		// We can ignore if we force the player to re-send their name when they change slots
		return true; // filter / ignore

	// Client messages to ignore while waiting for a swap player ACK
	// (because they may contain the old player index, and also the necessary messages will be resent after the ack)
	case NET_PING:                       // For now, just drop it - until the player responds with the ack
	case NET_PLAYER_STATS:
	case NET_COLOURREQUEST:              ///< player requests a colour change.
	case NET_FACTIONREQUEST:             ///< player requests a colour change.
	case NET_TEAMREQUEST:                ///< request team membership
	case NET_READY_REQUEST:              ///< player ready to start an mp game
	case NET_POSITIONREQUEST:            ///< position in GUI player list
	case NET_DATA_CHECK:                 ///< Data integrity check
	case NET_DATA_CHECK2:
		return true; // filter / ignore

	// one slot / index change at a time...
	case NET_PLAYER_SLOTTYPE_REQUEST:
		return true; // filter / ignore

	// client messages to be processed normally
	case NET_FILE_REQUESTED:             ///< Player has requested a file (map/mod/?)
	case NET_FILE_CANCELLED:             ///< Player cancelled a file request
		return false; // process normally (do *not* filter)

	// host-only messages
	case NET_OPTIONS:                    ///< welcome a player to a game.
	case NET_KICK:                       ///< kick a player .
	case NET_FIREUP:                     ///< campaign game has started, we can go too.. Shortcut message, not to be used in dmatch.
	case NET_PLAYER_INFO:                ///< basic player info
	case NET_PLAYER_JOINED:              ///< notice about player joining
	case NET_PLAYER_LEAVING:             ///< A player is leaving, (nicely)
	case NET_PLAYER_DROPPED:             ///< notice about player dropped / disconnected
	case NET_GAME_FLAGS:                 ///< game flags
	case NET_HOST_DROPPED:               ///< Host has dropped
	case NET_FILE_PAYLOAD:               ///< sending file to the player that needs it
	case NET_VOTE_REQUEST:               ///< Setup a vote popup
	case NET_PLAYER_SWAP_INDEX:
	case NET_HOST_CONFIG:
		ASSERT(false, "Received unexpected host-only message (%" PRIu8 ") from sender: %" PRIu8 "", type, sender);
		break;

	// only possible with initial join
	case NET_JOIN:                       ///< join a game
	case NET_ACCEPTED:                   ///< accepted into game
	case NET_REJECTED:                   ///< nope, you can't join
		ASSERT(false, "Received unexpected initial-join message (%" PRIu8 ") from sender: %" PRIu8 "", type, sender);
		break;

	// should only be possible once game has started
	case NET_PLAYERRESPONDING:           ///< computer that sent this is now playing warzone!
	case NET_AITEXTMSG:                  ///< chat between AIs
	case NET_BEACONMSG:                  ///< place beacon
	case NET_SHARE_GAME_QUEUE:           ///< Message contains a game message, which should be inserted into a queue.
	case NET_DEBUG_SYNC:                 ///< Synch error messages, so people don't have to use pastebin.
	case NET_SPECTEXTMSG:                ///< chat between spectators
	case NET_TEAM_STRATEGY:              ///< Player is sending an updated strategy notice to team members
		break;

	// just process normally
	case NET_SEND_TO_PLAYER:             ///< Non-host clients aren't directly connected to each other, so they talk via the host using these messages.
		return false; // process normally (do *not* filter)

	case NET_SECURED_NET_MESSAGE:        ///< This just wraps another message, so permit this wrapper message
		return false; // process normally (do *not* filter)

	case NET_PLAYER_SWAP_INDEX_ACK:
		// **MUST** be permitted
		return false; // process normally (do *not* filter)

	default:
		// just process normally
		break;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////
// Check if a message is a system message
static bool NETprocessSystemMessage(NETQUEUE playerQueue, uint8_t *type)
{
	if (*type == NET_SECURED_NET_MESSAGE)
	{
		if (!NETdecryptSecuredNetMessage(playerQueue, *type) || *type == NET_SECURED_NET_MESSAGE) // nested secured messages are not supported
		{
			// remove the invalid secured message, and return true to say we "handled it"
			debug(LOG_INFO, "Ignoring invalid secured message allegedly from player: %u", static_cast<unsigned>(playerQueue.index));
			NETpop(playerQueue);
			return true;
		}
	}

	if (NETFilterMessageWhileSwappingPlayer(playerQueue.index, *type))
	{
		NETpop(playerQueue);
		return true;
	}

	switch (*type)
	{
	case NET_SEND_TO_PLAYER:
		{
			uint8_t sender;
			uint8_t receiver;
			NetMessage const *message = nullptr;
			NETbeginDecode(playerQueue, NET_SEND_TO_PLAYER);
			NETuint8_t(&sender);
			NETuint8_t(&receiver);
			NETnetMessage(&message);  // Must delete message later.
			std::unique_ptr<NetMessage const> deleteLater(message);
			if (!NETend())
			{
				debug(LOG_ERROR, "Incomplete NET_SEND_TO_PLAYER.");
				break;
			}
			if (sender >= MAX_CONNECTED_PLAYERS || (receiver >= MAX_CONNECTED_PLAYERS && receiver != NET_ALL_PLAYERS))
			{
				debug(LOG_ERROR, "Bad NET_SEND_TO_PLAYER.");
				break;
			}
			if ((receiver == selectedPlayer || receiver == NET_ALL_PLAYERS) && playerQueue.index == NetPlay.hostPlayer)
			{
				// Message was sent to us via the host.
				if (sender != selectedPlayer)  // Make sure host didn't send us our own broadcast messages, which shouldn't happen anyway.
				{
					NETinsertMessageFromNet(NETnetQueue(sender), message);
					NETlogPacket(message->type, static_cast<uint32_t>(message->rawLen()), true);
				}
			}
			else if (NetPlay.isHost && sender == playerQueue.index)
			{
				if (((message->type == NET_OPTIONS
					  || message->type == NET_FIREUP
				      || message->type == NET_KICK
				      || message->type == NET_PLAYER_LEAVING
				      || message->type == NET_PLAYER_DROPPED
				      || message->type == NET_REJECTED
					  || message->type == NET_GAME_FLAGS
				      || message->type == NET_PLAYER_JOINED
					  || message->type == NET_PLAYER_INFO
					  || message->type == NET_FILE_PAYLOAD
					  || message->type == NET_PLAYER_SWAP_INDEX
					  || message->type == NET_HOST_CONFIG) && sender != NetPlay.hostPlayer)
				    ||
				    ((message->type == NET_HOST_DROPPED
				      || message->type == NET_FILE_REQUESTED
				      || message->type == NET_READY_REQUEST
				      || message->type == NET_TEAMREQUEST
				      || message->type == NET_COLOURREQUEST
				      || message->type == NET_POSITIONREQUEST
					  || message->type == NET_FACTIONREQUEST
				      || message->type == NET_FILE_CANCELLED
					  || message->type == NET_DATA_CHECK
				      || message->type == NET_JOIN
				      || message->type == NET_PLAYERNAME_CHANGEREQUEST
					  || message->type == NET_PLAYER_SWAP_INDEX_ACK) && receiver != NetPlay.hostPlayer)
					||
					((message->type == NET_PLAYER_SLOTTYPE_REQUEST
					  || message->type == NET_DATA_CHECK2) && (sender != NetPlay.hostPlayer && receiver != NetPlay.hostPlayer)))
				{
					char msg[256] = {'\0'};

					ssprintf(msg, "Auto-kicking player %u, lacked the required access level for command(%d).", (unsigned int)sender, (int)message->type);
					sendRoomSystemMessage(msg);
					NETlogEntry(msg, SYNC_FLAG, sender);
					addIPToBanList(NetPlay.players[sender].IPtextAddress, NetPlay.players[sender].name);
					NETplayerDropped(sender);
					connected_bsocket[sender] = nullptr;
					debug(LOG_ERROR, "%s", msg);
					break;
				}

				// Certain messages should be filtered while we're waiting for the ack of a player index switch
				if (NETFilterMessageWhileSwappingPlayer(sender, message->type))
				{
					break;
				}

				// Certain messages should be filtered due to hostChatPermissions
				if ((message->type == NET_TEXTMSG || message->type == NET_SPECTEXTMSG || message->type == NET_AITEXTMSG)
					&& !ingame.hostChatPermissions[sender])
				{
					// Only allow messages direct to host in this case! (Carve-out to allow /hostmsg commands...)
					if (receiver != NetPlay.hostPlayer)
					{
						break;
					}
				}

				// We are the host, and player is asking us to send the message to receiver.
				NETbeginEncode(NETnetQueue(receiver, sender), NET_SEND_TO_PLAYER);
				NETuint8_t(&sender);
				NETuint8_t(&receiver);
				NETnetMessage(&message);
				NETend();

				if (receiver == NET_ALL_PLAYERS)
				{
					NETinsertMessageFromNet(NETnetQueue(sender), message);  // Message is also for the host.
					NETlogPacket(message->type, static_cast<uint32_t>(message->rawLen()), true);
					// Not sure if flushing here can make a difference, maybe it can:
					//NETflush();  // Send the message to everyone as fast as possible.
				}
			}
			else
			{
				if (NetPlay.isHost && NETFilterMessageWhileSwappingPlayer(playerQueue.index, message->type))
				{
					debug(LOG_NET, "Ignoring message type (%d) from Player %d while swapping player index", (int)message->type, (int)playerQueue.index);
					break;
				}

				debug(LOG_INFO, "Report this: Player %d sent us message type (%d) addressed to %d from %d. We are %d.", (int)playerQueue.index, (int)message->type, (int)receiver, (int)sender, selectedPlayer);
			}

			break;
		}
	case NET_SHARE_GAME_QUEUE:
		{
			if (ingame.localJoiningInProgress)
			{
				// NET_SHARE_GAME_QUEUE should only be processed after the game has started - the game queues probably aren't yet created!
				debug(LOG_ERROR, "Ignoring NET_SHARE_GAME_QUEUE message from %" PRIu8 " (only permitted in-game).", playerQueue.index);
				break;
			}

			uint8_t player = 0;
			uint32_t num = 0, n;
			NetMessage const *message = nullptr;

			// Encoded in NETprocessSystemMessage in nettypes.cpp.
			NETbeginDecode(playerQueue, NET_SHARE_GAME_QUEUE);
			NETuint8_t(&player);
			NETuint32_t(&num);
			bool isSentByCorrectClient = responsibleFor(playerQueue.index, player);
			isSentByCorrectClient = isSentByCorrectClient || (playerQueue.index == NetPlay.hostPlayer && playerQueue.index != selectedPlayer);  // Let host spoof other people's NET_SHARE_GAME_QUEUE messages, but not our own. This allows the host to spoof a GAME_PLAYER_LEFT message (but spoofing any message when the player is still there will fail with desynch).
			if (!isSentByCorrectClient || player >= MAX_CONNECTED_PLAYERS)
			{
				NETend();
				break;
			}
			for (n = 0; n < num; ++n)
			{
				NETnetMessage(&message);

				NETinsertMessageFromNet(NETgameQueue(player), message);
				NETlogPacket(message->type, static_cast<uint32_t>(message->rawLen()), true);

				delete message;
				message = nullptr;
			}
			if (!NETend())
			{
				debug(LOG_ERROR, "Bad NET_SHARE_GAME_QUEUE message.");
				break;
			}
			break;
		}
	case NET_PLAYERNAME_CHANGEREQUEST:
		{
			if (!ingame.localJoiningInProgress)
			{
				// player name change only permitted in the lobby
				debug(LOG_ERROR, "Ignoring NET_PLAYERNAME_CHANGEREQUEST message (only permitted in the lobby).");
				break;
			}
			if (!NetPlay.isHost)
			{
				// Only the host should receive and process these messages
				debug(LOG_ERROR, "Ignoring NET_PLAYERNAME_CHANGEREQUEST message (should only be sent to the host).");
				break;
			}

			uint8_t player = 0;
			WzString oldName;
			WzString newName;

			// Encoded in NETchangePlayerName() in netplay.cpp.
			NETbeginDecode(playerQueue, NET_PLAYERNAME_CHANGEREQUEST);
			NETuint8_t(&player);
			NETwzstring(newName);
			NETend();

			// Bail out if the given ID number is out of range
			if (player >= MAX_CONNECTED_PLAYERS || (playerQueue.index != NetPlay.hostPlayer && (playerQueue.index != player || !NetPlay.players[player].allocated)))
			{
				debug(LOG_ERROR, "NET_PLAYERNAME_CHANGEREQUEST from %u: Player ID (%u) out of range (max %u)", playerQueue.index, player, (unsigned int)MAX_CONNECTED_PLAYERS);
				break;
			}

			oldName = NetPlay.players[player].name;
			setPlayerName(player, newName.toUtf8().c_str());

			if (NetPlay.players[player].allocated && strncmp(oldName.toUtf8().c_str(), NetPlay.players[player].name, sizeof(NetPlay.players[player].name)) != 0)
			{
				printConsoleNameChange(oldName.toUtf8().c_str(), NetPlay.players[player].name);
				// Send the updated data to all other clients as well.
				NETBroadcastPlayerInfo(player); // ultimately triggers updateMultiplayGameData inside NETSendNPlayerInfoTo
				NETfixDuplicatePlayerNames();
				netPlayersUpdated = true;
			}

			break;
		}
	case NET_PLAYER_STATS:
		{
			recvMultiStats(playerQueue);
			netPlayersUpdated = true;
			break;
		}
	case NET_PLAYER_INFO:
		{
			uint32_t indexLen = 0, n;
			uint32_t index = MAX_CONNECTED_PLAYERS;
			int32_t colour = 0;
			int32_t position = 0;
			int32_t team = 0;
			int8_t ai = 0;
			int8_t difficulty = 0;
			uint8_t faction = FACTION_NORMAL;
			bool isSpectator = false;
			bool error = false;
			char countryCode[3] = {0};

			NETbeginDecode(playerQueue, NET_PLAYER_INFO);
			NETuint32_t(&indexLen);
			if (indexLen > MAX_CONNECTED_PLAYERS || (playerQueue.index != NetPlay.hostPlayer))
			{
				debug(LOG_ERROR, "MSG_PLAYER_INFO: Bad number of players updated: %u", indexLen);
				NETend();
				break;
			}

			for (n = 0; n < indexLen; ++n)
			{
				bool wasAllocated = false;
				std::string oldName;

				// Retrieve the player's ID
				NETuint32_t(&index);

				// Bail out if the given ID number is out of range
				if (index >= MAX_CONNECTED_PLAYERS || (playerQueue.index != NetPlay.hostPlayer && (playerQueue.index != index || !NetPlay.players[index].allocated)))
				{
					debug(LOG_ERROR, "MSG_PLAYER_INFO from %u: Player ID (%u) out of range (max %u)", playerQueue.index, index, (unsigned int)MAX_CONNECTED_PLAYERS);
					error = true;
					break;
				}

				// Retrieve the rest of the data
				wasAllocated = NetPlay.players[index].allocated;
				NETbool(&NetPlay.players[index].allocated);
				NETbool(&NetPlay.players[index].heartbeat);
				NETbool(&NetPlay.players[index].kick);
				oldName.clear();
				oldName = NetPlay.players[index].name;
				NETstring(NetPlay.players[index].name, sizeof(NetPlay.players[index].name));
				NETuint32_t(&NetPlay.players[index].heartattacktime);
				NETint32_t(&colour);
				NETint32_t(&position);
				NETint32_t(&team);
				NETbool(&NetPlay.players[index].ready);
				NETint8_t(&ai);
				NETint8_t(&difficulty);
				NETuint8_t(&faction);
				NETbool(&isSpectator);
				NETstring(countryCode, sizeof(NetPlay.players[index].countryCode));

				// The host does not know its IP address, only the client does, so we do not wait for the country code, 
				// but use the one that was determined at connection to host.
				if (playerQueue.index != NetPlay.hostPlayer)
				{
					if (strlen(countryCode) > 0)
					{
						sstrcpy(NetPlay.players[index].countryCode, countryCode);
					}
					else
					{
						NetPlay.players[index].countryCode[0] = '\0';
					}
				}

				auto newFactionId = uintToFactionID(faction);
				if (!newFactionId.has_value())
				{
					debug(LOG_ERROR, "MSG_PLAYER_INFO from %u: Faction ID (%u) out of range (max %u)", playerQueue.index, (unsigned int)faction, (unsigned int)MAX_FACTION_ID);
					error = true;
					break;
				}

				// Don't let anyone except the host change these, otherwise it will end up inconsistent at some point, and the game gets really messed up.
				if (playerQueue.index == NetPlay.hostPlayer)
				{
					setPlayerColour(index, colour);
					NetPlay.players[index].position = position;
					NetPlay.players[index].team = team;
					NetPlay.players[index].ai = ai;
					NetPlay.players[index].difficulty = static_cast<AIDifficulty>(difficulty);
					NetPlay.players[index].faction = newFactionId.value();
					NetPlay.players[index].isSpectator = isSpectator;
				}

				debug(LOG_NET, "%s for player %u (%s)", n == 0 ? "Receiving MSG_PLAYER_INFO" : "                      and", (unsigned int)index, NetPlay.players[index].allocated ? "human" : "AI");

				if (wasAllocated && NetPlay.players[index].allocated && strncmp(oldName.c_str(), NetPlay.players[index].name, sizeof(NetPlay.players[index].name)) != 0)
				{
					printConsoleNameChange(oldName.c_str(), NetPlay.players[index].name);
				}
			}
			NETend();
			// If we're the game host make sure to send the updated
			// data to all other clients as well.
			if (NetPlay.isHost && !error)
			{
				NETBroadcastPlayerInfo(index); // ultimately triggers updateMultiplayGameData inside NETSendNPlayerInfoTo
				NETfixDuplicatePlayerNames();
			}
			else if (!error)
			{
				if (index == selectedPlayer)
				{
					handleAutoReadyRequest();
				}
				ActivityManager::instance().updateMultiplayGameData(game, ingame, NETGameIsLocked());
			}
			netPlayersUpdated = true;
			break;
		}
	case NET_PLAYER_JOINED:
		{
			uint8_t index;

			NETbeginDecode(playerQueue, NET_PLAYER_JOINED);
			NETuint8_t(&index);
			NETend();

			debug(LOG_NET, "Receiving NET_PLAYER_JOINED for player %u using socket %p",
			      (unsigned int)index, static_cast<void *>(bsocket));

			MultiPlayerJoin(index);
			netPlayersUpdated = true;
			break;
		}
	// This message type is when player is leaving 'nicely', and socket is still valid.
	case NET_PLAYER_LEAVING:
		{
			uint32_t index;

			NETbeginDecode(playerQueue, NET_PLAYER_LEAVING);
			NETuint32_t(&index);
			NETend();

			if (playerQueue.index != NetPlay.hostPlayer && index != playerQueue.index)
			{
				debug(LOG_ERROR, "Player %d left, but accidentally set player %d as leaving.", playerQueue.index, index);
				index = playerQueue.index;
			}

			if (connected_bsocket[index])
			{
				debug(LOG_NET, "Receiving NET_PLAYER_LEAVING for player %u on socket %p", (unsigned int)index, static_cast<void *>(connected_bsocket[index]));
			}
			else
			{
				// dropped from join screen most likely
				debug(LOG_NET, "Receiving NET_PLAYER_LEAVING for player %u (no socket?)", (unsigned int)index);
			}

			if (NetPlay.isHost)
			{
				debug(LOG_NET, "Broadcast leaving message to everyone else");
				NETbeginEncode(NETbroadcastQueue(), NET_PLAYER_LEAVING);
				NETuint32_t(&index);
				NETend();
			}

			debug(LOG_INFO, "Player %u has left the game.", index);
			NETplayerLeaving(index);		// need to close socket for the player that left.
			NETsetPlayerConnectionStatus(CONNECTIONSTATUS_PLAYER_LEAVING, index);
			break;
		}
	case NET_GAME_FLAGS:
		{
			debug(LOG_NET, "Receiving game flags");

			NETbeginDecode(playerQueue, NET_GAME_FLAGS);

			if (playerQueue.index != NetPlay.hostPlayer)
			{
				debug(LOG_ERROR, "NET_GAME_FLAGS sent by wrong player: %" PRIu32 "", playerQueue.index);
				NETend();
				break;
			}

			{
				static unsigned int max_flags = ARRAY_SIZE(NetGameFlags);
				// Retrieve the amount of game flags that we should receive
				uint8_t i, count;
				NETuint8_t(&count);

				// Make sure that we won't get buffer overflows by checking that we
				// have enough space to store the given amount of game flags.
				if (count > max_flags)
				{
					debug(LOG_NET, "NET_GAME_FLAGS: More game flags sent (%u) than our buffer can hold (%u)", (unsigned int)count, max_flags);
					count = max_flags;
				}

				// Retrieve all game flags
				for (i = 0; i < count; ++i)
				{
					NETint32_t(&NetGameFlags[i]);
				}
			}
			NETend();

			if (NetPlay.isHost)
			{
				NETsendGameFlags();
			}
			break;
		}
	case NET_DEBUG_SYNC:
		{
			recvDebugSync(playerQueue);
			break;
		}
	case NET_PLAYER_SWAP_INDEX_ACK:
		{
			ASSERT_HOST_ONLY(break);

			uint32_t oldPlayerIndex;
			uint32_t newPlayerIndex;

			NETbeginDecode(playerQueue, NET_PLAYER_SWAP_INDEX_ACK);
			NETuint32_t(&oldPlayerIndex);
			NETuint32_t(&newPlayerIndex);
			NETend();

			bool isSentByCorrectClient = responsibleFor(playerQueue.index, newPlayerIndex);
			if (!isSentByCorrectClient)
			{
				debug(LOG_ERROR, "NET_PLAYER_SWAP_INDEX_ACK sent by wrong player: %" PRIu32 "", playerQueue.index);
				break;
			}

			if (newPlayerIndex >= NET_waitingForIndexChangeAckSince.size() || !NET_waitingForIndexChangeAckSince[newPlayerIndex].has_value())
			{
				debug(LOG_ERROR, "NET_PLAYER_SWAP_INDEX_ACK sent despite us not waiting for it (for player: %" PRIu32 ")", newPlayerIndex);
				break;
			}

			NET_waitingForIndexChangeAckSince[newPlayerIndex] = nullopt;

			break;
		}

	default:
		return false;
	}

	NETpop(playerQueue);
	return true;
}

/*
*	Checks to see if a human player is still with us.
*	@note: resuscitation isn't possible with current code, so once we lose
*	the socket, then we have no way to connect with them again. Future
*	item to enhance.
*/
static void NETcheckPlayers()
{
	if (!NetPlay.isHost)
	{
		ASSERT(false, "Host only routine detected for client or not hosting yet!");
		return;
	}

	for (int i = 0; i < MAX_CONNECTED_PLAYERS ; i++)
	{
		if (NetPlay.players[i].allocated == 0)
		{
			continue;    // not allocated means that it most likely it is a AI player
		}
		if (NetPlay.players[i].heartbeat == 0 && NetPlay.players[i].heartattacktime == 0)	// looks like they are dead
		{
			NetPlay.players[i].heartattacktime = realTime;		// mark when this occurred
		}
		else
		{
			if (NetPlay.players[i].heartattacktime)
			{
				if (NetPlay.players[i].heartattacktime + (15 * GAME_TICKS_PER_SEC) <  realTime) // wait 15 secs
				{
					debug(LOG_NET, "Kicking due to client heart attack");
					NetPlay.players[i].kick = true;		// if still dead, then kick em.
				}
			}
		}
		if (NetPlay.players[i].kick)
		{
			debug(LOG_NET, "Kicking player %d", i);
			kickPlayer(i, "you are unwanted by the host.", ERROR_KICKED, false);
		}
	}
}

// ////////////////////////////////////////////////////////////////////////
// Receive a message over the current connection. We return true if there
// is a message for the higher level code to process, and false otherwise.
// We should not block here.
bool NETrecvNet(NETQUEUE *queue, uint8_t *type)
{
	const int status = upnp_status.load(); // hack fix for clang and c++11 - fixed in standard for c++14
	char buf[512] = {'\0'};
	switch (status)
	{
	case UPNP_ERROR_CONTROL_NOT_AVAILABLE:
	case UPNP_ERROR_DEVICE_NOT_FOUND:
		if (upnp_status == UPNP_ERROR_DEVICE_NOT_FOUND)
		{
			debug(LOG_NET, "UPnP device not found");
		}
		else if (upnp_status == UPNP_ERROR_CONTROL_NOT_AVAILABLE)
		{
			debug(LOG_NET, "controlURL not available, UPnP disabled");
		}
		// beware of writing a line too long, it screws up console line count. \n is safe for line split
		ssprintf(buf, _("No UPnP device found. Configure your router/firewall to open port %d!"), NETgetGameserverPort());
		addConsoleMessage(buf, DEFAULT_JUSTIFY, NOTIFY_MESSAGE);
		NetPlay.isUPNP_CONFIGURED = false;
		NetPlay.isUPNP_ERROR = true;
		upnp_status = 0;
		break;
	case UPNP_SUCCESS:
		NETaddRedirects();
		upnp_status = 0;
		break;
	default:
	case 0:
		ASSERT(upnp_status == 0, "bad value");
		break;
	}

	uint32_t current;

	if (!NetPlay.bComms)
	{
		return false;
	}

	if (NetPlay.isHost)
	{
		NETfixPlayerCount();
		NETallowJoining();
		lobbyConnectionHandler.run();
		NETcheckPlayers();		// make sure players are still alive & well
	}

	if (socket_set == nullptr || checkSockets(socket_set, NET_READ_TIMEOUT) <= 0)
	{
		goto checkMessages;
	}

	for (current = 0; current < MAX_CONNECTED_PLAYERS; ++current)
	{
		Socket **pSocket = NetPlay.isHost ? &connected_bsocket[current] : &bsocket;
		uint8_t buffer[NET_BUFFER_SIZE];
		size_t dataLen;

		if (!NetPlay.isHost && current != NetPlay.hostPlayer)
		{
			continue;  // Don't have a socket open to this player.
		}

		if (*pSocket == nullptr)
		{
			continue;
		}

		dataLen = NET_fillBuffer(pSocket, socket_set, buffer, sizeof(buffer));
		if (dataLen > 0)
		{
			// we received some data, add to buffer
			NETinsertRawData(NETnetQueue(current), buffer, dataLen);
		}
		else if (*pSocket == nullptr)
		{
			// If there is a error in NET_fillBuffer() then socket is already invalid.
			// This means that the player dropped / disconnected for whatever reason.
			debug(LOG_INFO, "Player, (player %u) seems to have dropped/disconnected.", (unsigned)current);

			if (NetPlay.isHost)
			{
				// Send message type specifically for dropped / disconnects
				NETplayerDropped(current);
				connected_bsocket[current] = nullptr;		// clear their socket
			}
			else
			{
				// lobby errors were set in NET_fillBuffer()
				return false;
			}
		}
	}

checkMessages:
	for (current = 0; current < MAX_CONNECTED_PLAYERS; ++current)
	{
		*queue = NETnetQueue(current);
		while (NETisMessageReady(*queue))
		{
			*type = NETgetMessage(*queue)->type;
			if (!NETprocessSystemMessage(*queue, type))
			{
				return true;  // We couldn't process the message, let the caller deal with it..
			}
		}
	}

	return false;
}

bool NETrecvGame(NETQUEUE *queue, uint8_t *type)
{
	for (unsigned current = 0; current < MAX_GAMEQUEUE_SLOTS; ++current)
	{
		*queue = NETgameQueue(current);
		if (queue->queue == nullptr)
		{
			continue;
		}
		while (!checkPlayerGameTime(current))  // Check for any messages that are scheduled to be read now.
		{
			if (!NETisMessageReady(*queue))
			{
				return false;  // Still waiting for messages from this player, and all players should process messages in the same order. Will have to freeze the game while waiting.
			}

			NETreplaySaveNetMessage(NETgetMessage(*queue), queue->index);
			*type = NETgetMessage(*queue)->type;

			if (*type == GAME_GAME_TIME)
			{
				recvPlayerGameTime(*queue);
				NETpop(*queue);
				continue;
			}

			return true;  // Have a message ready to read now.
		}
	}

	return false;  // No messages sceduled to be read yet. Game can continue.
}

// ////////////////////////////////////////////////////////////////////////
// File Transfer programs.
/** Send file. It returns % of file sent when 100 it's complete. Call until it returns 100.
*  @TODO: more error checking (?) different file types (?)
*          Maybe should close file handle, and seek each time?
*
*  @NOTE: MAX_FILE_TRANSFER_PACKET is set to 2k per packet since 7*2 = 14K which is pretty
*         much our limit.  Don't screw with that without having a bigger buffer!
*         NET_BUFFER_SIZE is at 16k.  (also remember text chat, plus all the other cruff)
*/
#define MAX_FILE_TRANSFER_PACKET 2048
int NETsendFile(WZFile &file, unsigned player)
{
	ASSERT_OR_RETURN(100, NetPlay.isHost, "Trying to send a file and we are not the host!");
	ASSERT_OR_RETURN(100, file.handle() != nullptr, "Null file handle");

	uint8_t inBuff[MAX_FILE_TRANSFER_PACKET];
	memset(inBuff, 0x0, sizeof(inBuff));

	// read some bytes.
	PHYSFS_sint64 readBytesResult = WZ_PHYSFS_readBytes(file.handle(), inBuff, MAX_FILE_TRANSFER_PACKET);
	if (readBytesResult < 0)
	{
		ASSERT(readBytesResult >= 0, "Error reading file.");
		file.closeFile();
		return 100;
	}
	uint32_t bytesToRead = static_cast<uint32_t>(readBytesResult);

	NETbeginEncode(NETnetQueue(player), NET_FILE_PAYLOAD);
	NETbin(file.hash.bytes, file.hash.Bytes);
	NETuint32_t(&file.size);  // total bytes in this file. (we don't support 64bit yet)
	NETuint32_t(&file.pos);  // start byte
	NETuint32_t(&bytesToRead);  // bytes in this packet
	NETbin(inBuff, bytesToRead);
	NETend();

	file.pos += bytesToRead;  // update position!
	if (file.pos == file.size)
	{
		file.closeFile(); // We are done sending to this client.
	}

	return static_cast<int>((uint64_t)file.pos * 100 / file.size);
}

bool validateReceivedFile(const WZFile& file)
{
	PHYSFS_file *fileHandle = PHYSFS_openRead(file.filename.c_str());
	ASSERT_OR_RETURN(false, fileHandle != nullptr, "Could not open downloaded file %s for reading: %s", file.filename.c_str(), WZ_PHYSFS_getLastError());

	PHYSFS_sint64 actualFileSize64 = PHYSFS_fileLength(fileHandle);
	if (actualFileSize64 < 0)
	{
		debug(LOG_ERROR, "Failed to determine file size of the downloaded file!");
		PHYSFS_close(fileHandle);
		return false;
	}
	if(actualFileSize64 > std::numeric_limits<int32_t>::max())
	{
		debug(LOG_ERROR, "Downloaded file is too large!");
		PHYSFS_close(fileHandle);
		return false;
	}

	uint32_t actualFileSize = static_cast<uint32_t>(actualFileSize64);
	if (actualFileSize != file.size)
	{
		debug(LOG_ERROR, "Downloaded map unexpected size! Got %" PRIu32", expected %" PRIu32"!", actualFileSize, file.size);
		PHYSFS_close(fileHandle);
		return false;
	}

	// verify actual downloaded file hash matches expected hash

	Sha256 actualFileHash;
	crypto_hash_sha256_state state;
	crypto_hash_sha256_init(&state);
	size_t bufferSize = std::min<size_t>(actualFileSize, 4 * 1024 * 1024);
	std::vector<unsigned char> fileChunkBuffer(bufferSize, '\0');
	PHYSFS_sint64 length_read = 0;
	do {
		length_read = WZ_PHYSFS_readBytes(fileHandle, fileChunkBuffer.data(), static_cast<PHYSFS_uint32>(bufferSize));
		if (length_read != bufferSize)
		{
			if (length_read < 0 || !PHYSFS_eof(fileHandle))
			{
				// did not read expected amount, but did not reach end of file - some other error reading the file occurred
				debug(LOG_ERROR, "Failed to read downloaded file: %s", WZ_PHYSFS_getLastError());
				PHYSFS_close(fileHandle);
				return false;
			}
		}
		crypto_hash_sha256_update(&state, fileChunkBuffer.data(), static_cast<unsigned long long>(length_read));
	} while (length_read == bufferSize);
	crypto_hash_sha256_final(&state, actualFileHash.bytes);
	fileChunkBuffer.clear();

	if (actualFileHash != file.hash)
	{
		debug(LOG_ERROR, "Downloaded file hash (%s) does not match requested file hash (%s)", actualFileHash.toString().c_str(), file.hash.toString().c_str());
		PHYSFS_close(fileHandle);
		return false;
	}

	PHYSFS_close(fileHandle);
	return true;
}

bool markAsDownloadedFile(const std::string &filename)
{
	// Files are written to the PhysFS writeDir
	const char * current_writeDir = PHYSFS_getWriteDir();
	ASSERT(current_writeDir != nullptr, "Failed to get PhysFS writeDir: %s", WZ_PHYSFS_getLastError());
	std::string fullFilePath = std::string(current_writeDir) + PHYSFS_getDirSeparator() + filename;

#if defined(WZ_OS_WIN)
	// On Windows:
	//	- Create the Alternate Data Stream required to set the Internet Zone identifier
	const wchar_t kWindowsZoneIdentifierADSSuffix[] = L":Zone.Identifier";

	// Convert fullFilePath to UTF-16 wchar_t
	int wstr_len = MultiByteToWideChar(CP_UTF8, 0, fullFilePath.c_str(), -1, NULL, 0);
	if (wstr_len <= 0)
	{
		debug(LOG_ERROR, "Could not convert string from UTF-8; MultiByteToWideChar failed with error %lu: %s\n", GetLastError(), fullFilePath.c_str());
		return false;
	}
	std::vector<wchar_t> wstr_filename(wstr_len, 0);
	if (MultiByteToWideChar(CP_UTF8, 0, fullFilePath.c_str(), -1, &wstr_filename[0], wstr_len) == 0)
	{
		debug(LOG_ERROR, "Could not convert string from UTF-8; MultiByteToWideChar[2] failed with error %lu: %s\n", GetLastError(), fullFilePath.c_str());
		return false;
	}
	std::wstring fullFilePathUTF16(wstr_filename.data());
	fullFilePathUTF16 += kWindowsZoneIdentifierADSSuffix;

	HANDLE hStream = CreateFileW(fullFilePathUTF16.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hStream == INVALID_HANDLE_VALUE)
	{
		// Failed to open stream
		debug(LOG_ERROR, "Could not open stream; failed with error %lu: %s\n", GetLastError(), fullFilePath.c_str());
		return false;
	}

	// Set it to "downloaded from the Internet Zone" (ZoneId 3)
	const char kWindowsZoneIdentifierADSDataInternetZone[] = "[ZoneTransfer]\r\nZoneId=3\r\n";
	DWORD dwNumberOfBytesWritten;
	if (WriteFile(hStream, kWindowsZoneIdentifierADSDataInternetZone, static_cast<DWORD>(strlen(kWindowsZoneIdentifierADSDataInternetZone)), &dwNumberOfBytesWritten, NULL) == 0)
	{
		debug(LOG_ERROR, "Failed to write to stream with error %lu: %s\n", GetLastError(), fullFilePath.c_str());
		CloseHandle(hStream);
		return false;
	}

	FlushFileBuffers(hStream);
	CloseHandle(hStream);

	return true;
#elif defined (WZ_OS_MAC)
	// On macOS:
	//	- Set the quarantine attribute on the file
	return cocoaSetFileQuarantineAttribute(fullFilePath.c_str());
#else
	// Not currently implemented
#endif
	return false;
}

// recv file. it returns % of the file so far recvd.
int NETrecvFile(NETQUEUE queue)
{
	Sha256 hash;
	hash.setZero();
	uint32_t size = 0;
	uint32_t pos = 0;
	uint32_t bytesToRead = 0;
	uint8_t buf[MAX_FILE_TRANSFER_PACKET];
	memset(buf, 0x0, sizeof(buf));

	//read incoming bytes.
	NETbeginDecode(queue, NET_FILE_PAYLOAD);
	NETbin(hash.bytes, hash.Bytes);
	NETuint32_t(&size);  // total bytes in this file. (we don't support 64bit yet)
	NETuint32_t(&pos);  // start byte
	NETuint32_t(&bytesToRead);  // bytes in this packet
	ASSERT_OR_RETURN(100, bytesToRead <= sizeof(buf), "Bad value.");
	NETbin(buf, bytesToRead);
	NETend();

	debug(LOG_NET, "New file position is %u", pos);

	auto file = std::find_if(DownloadingWzFiles.begin(), DownloadingWzFiles.end(), [&](WZFile const &file) { return file.hash == hash; });

	auto sendCancelFileDownload = [](Sha256 &hash) {
		NETbeginEncode(NETnetQueue(NetPlay.hostPlayer), NET_FILE_CANCELLED);
		NETbin(hash.bytes, hash.Bytes);
		NETend();
	};

	if (file == DownloadingWzFiles.end())
	{
		debug(LOG_WARNING, "Receiving file data we didn't request.");
		sendCancelFileDownload(hash);
		return 100;
	}

	auto terminateFileDownload = [sendCancelFileDownload](std::vector<WZFile>::iterator &file) {
		if (!file->closeFile())
		{
			debug(LOG_ERROR, "Could not close file handle after trying to terminate download: %s", WZ_PHYSFS_getLastError());
		}
		PHYSFS_delete(file->filename.c_str());
		sendCancelFileDownload(file->hash);
		DownloadingWzFiles.erase(file);
	};

	//sanity checks
	if (file->size != size)
	{
		if (file->size == 0)
		{
			// host does not send the file size until the first recvFile packet
			file->size = size;
		}
		else
		{
			// host sent a different file size for this file with this chunk (vs the first chunk)
			// this should not happen!
			debug(LOG_ERROR, "Host sent a different file size for this file; (original size: %u, new size: %u)", file->size, size);
			terminateFileDownload(file); // 'file' is now an invalidated iterator.
			return 100;
		}
	}

	if (size > MAX_NET_TRANSFERRABLE_FILE_SIZE)
	{
		// file size is too large
		debug(LOG_ERROR, "Downloaded filesize is too large; (size: %" PRIu32")", size);
		terminateFileDownload(file); // 'file' is now an invalidated iterator.
		return 100;
	}

	if (PHYSFS_tell(file->handle()) != static_cast<PHYSFS_sint64>(pos))
	{
		// actual position in file does not equal the expected position in the file (sent by the host)
		debug(LOG_ERROR, "Invalid file position in downloaded file; (desired: %" PRIu32")", pos);
		terminateFileDownload(file); // 'file' is now an invalidated iterator.
		return 100;
	}

	// Write packet to the file.
	WZ_PHYSFS_writeBytes(file->handle(), buf, bytesToRead);

	uint32_t newPos = pos + bytesToRead;
	file->pos = newPos;

	if (newPos >= size)  // last packet
	{
		if (!file->closeFile())
		{
			debug(LOG_ERROR, "Could not close file handle after trying to save map: %s", WZ_PHYSFS_getLastError());
		}

		if(!validateReceivedFile(*file))
		{
			// Delete the (invalid) downloaded file
			PHYSFS_delete(file->filename.c_str());
		}
		else
		{
			// Attach Quarantine / "downloaded" file attribute to file
			markAsDownloadedFile(file->filename.c_str());
		}

		DownloadingWzFiles.erase(file);
	}
	// 'file' may now be an invalidated iterator.

	//return the percentage count
	if (size)
	{
		return (newPos * 100) / size;
	}
	debug(LOG_ERROR, "Received 0 byte file from host?");
	return 100;		// file is nullbyte, so we are done.
}

unsigned NETgetDownloadProgress(unsigned player)
{
	std::vector<WZFile> const *files = player == selectedPlayer ?
		&DownloadingWzFiles :  // Check our own download progress.
		NetPlay.players[player].wzFiles.get();  // Check their download progress (currently only works if we are the host).

	ASSERT_OR_RETURN(100, files != nullptr, "Uninitialized wzFiles? (Player %u)", player);

	uint32_t progress = 100;
	for (WZFile const &file : *files)
	{
		progress = std::min<uint32_t>(progress, (uint32_t)((uint64_t)file.pos * 100 / (uint64_t)std::max<uint32_t>(file.size, 1)));
	}
	return static_cast<unsigned>(progress);
}

static ssize_t readLobbyResponse(Socket *sock, unsigned int timeout)
{
	uint32_t lobbyStatusCode;
	uint32_t MOTDLength;
	uint32_t buffer[2];
	ssize_t result, received = 0;

	// Get status and message length
	result = readAll(sock, &buffer, sizeof(buffer), timeout);
	if (result != sizeof(buffer))
	{
		goto error;
	}
	received += result;
	lobbyStatusCode = ntohl(buffer[0]);
	MOTDLength = ntohl(buffer[1]);

	// Get status message
	if (NetPlay.MOTD)
	{
		free(NetPlay.MOTD);
	}
	NetPlay.MOTD = (char *)malloc(MOTDLength + 1);
	result = readAll(sock, NetPlay.MOTD, MOTDLength, timeout);
	if (result != MOTDLength)
	{
		goto error;
	}
	received += result;
	// NUL terminate string
	NetPlay.MOTD[MOTDLength] = '\0';

	switch (lobbyStatusCode)
	{
	case 200:
		debug(LOG_NET, "Lobby success (%u): %s", (unsigned int)lobbyStatusCode, NetPlay.MOTD);
		NetPlay.HaveUpgrade = false;
		break;

	case 400:
		debug(LOG_NET, "**Upgrade available! Lobby success (%u): %s", (unsigned int)lobbyStatusCode, NetPlay.MOTD);
		NetPlay.HaveUpgrade = true;
		break;

	default:
		debug(LOG_ERROR, "Lobby error (%u): %s", (unsigned int)lobbyStatusCode, NetPlay.MOTD);
		// ensure if the lobby returns an error, we are prepared to display it (once)
		NetPlay.ShowedMOTD = false;
		// this is horrible but MOTD can have 0x0a and other junk in it
		std::string strmotd = std::string(NetPlay.MOTD);
		wz_command_interface_output("WZEVENT: lobbyerror (%u): %s\n", (unsigned int)lobbyStatusCode, base64Encode(std::vector<unsigned char>(strmotd.begin(), strmotd.end())).c_str());
		break;
	}

	return received;

error:
	if (result == SOCKET_ERROR)
	{
		if (NetPlay.MOTD)
		{
			free(NetPlay.MOTD);
		}
		if (asprintf(&NetPlay.MOTD, "Error while connecting to the lobby server: %s\nMake sure port %d can receive incoming connections.", strSockError(getSockErr()), gameserver_port) == -1)
		{
			NetPlay.MOTD = nullptr;
		}
		else
		{
			NetPlay.ShowedMOTD = false;
			debug(LOG_ERROR, "%s", NetPlay.MOTD);
		}
	}
	else
	{
		if (NetPlay.MOTD)
		{
			free(NetPlay.MOTD);
		}
		if (asprintf(&NetPlay.MOTD, "Disconnected from lobby server. Failed to register game.") == -1)
		{
			NetPlay.MOTD = nullptr;
		}
		else
		{
			NetPlay.ShowedMOTD = false;
			debug(LOG_ERROR, "%s", NetPlay.MOTD);
		}
	}

	std::string strmotd = (NetPlay.MOTD) ? std::string(NetPlay.MOTD) : std::string();
	wz_command_interface_output("WZEVENT: lobbysocketerror: %s\n", (!strmotd.empty()) ? base64Encode(std::vector<unsigned char>(strmotd.begin(), strmotd.end())).c_str() : "");

	return SOCKET_ERROR;
}

bool readGameStructsList(Socket *sock, unsigned int timeout, const std::function<bool (const GAMESTRUCT& game)>& handleEnumerateGameFunc)
{
	unsigned int gamecount = 0;
	uint32_t gamesavailable = 0;
	int result = 0;

	if ((result = readAll(sock, &gamesavailable, sizeof(gamesavailable), NET_TIMEOUT_DELAY)) == sizeof(gamesavailable))
	{
		gamesavailable = ntohl(gamesavailable);
	}
	else
	{
		if (result == SOCKET_ERROR)
		{
			debug(LOG_NET, "Server socket encountered error: %s", strSockError(getSockErr()));
		}
		else
		{
			debug(LOG_NET, "Server didn't respond (timeout)");
		}
		return false;
	}

	debug(LOG_NET, "receiving info on %u game(s)", (unsigned int)gamesavailable);

	while (gamecount < gamesavailable)
	{
		// Attempt to receive a game description structure
		GAMESTRUCT tmpGame;
		memset(&tmpGame, 0x00, sizeof(tmpGame));
		if (!NETrecvGAMESTRUCT(sock, &tmpGame))
		{
			debug(LOG_NET, "only %u game(s) received", (unsigned int)gamecount);
			return false;
		}

		if (tmpGame.desc.host[0] == '\0')
		{
			memset(tmpGame.desc.host, 0, sizeof(tmpGame.desc.host));
			strncpy(tmpGame.desc.host, getSocketTextAddress(sock), sizeof(tmpGame.desc.host) - 1);
		}

		uint32_t Vmgr = (tmpGame.future4 & 0xFFFF0000) >> 16;
		uint32_t Vmnr = (tmpGame.future4 & 0x0000FFFF);

		if (NETisGreaterVersion(Vmgr, Vmnr))
		{
			debug(LOG_NET, "Version update %d:%d", Vmgr, Vmnr);
			NetPlay.HaveUpgrade = true;
		}

		if (tmpGame.desc.dwSize != 0)
		{
			if (!handleEnumerateGameFunc(tmpGame))
			{
				// stop enumerating
				break;
			}
		}

		++gamecount;
	}

	return true;
}

bool LobbyServerConnectionHandler::connect()
{
	if (server_not_there)
	{
		return false;
	}
	if (lobby_disabled)
	{
		debug(LOG_ERROR, "Multiplayer lobby support unavailable. Please update your client.");
		wz_command_interface_output("WZEVENT: lobbyerror: Client support disabled / unavailable\n");
		gamestruct.gameId = 0;
		server_not_there = true;
		return true; // return true once, so that NETallowJoining processes the "first time connect" branch
	}
	if (currentState == LobbyConnectionState::Connecting_WaitingForResponse || currentState == LobbyConnectionState::Connected)
	{
		return false; // already connecting or connected
	}

	bool bProcessingConnectOrDisconnectThisCall = true;
	uint32_t gameId = 0;
	SocketAddress *const hosts = resolveHost(masterserver_name, masterserver_port);

	if (hosts == nullptr)
	{
		int sockErrInt = getSockErr();
		debug(LOG_ERROR, "Cannot resolve masterserver \"%s\": %s", masterserver_name, strSockError(sockErrInt));
		free(NetPlay.MOTD);
		if (asprintf(&NetPlay.MOTD, _("Could not resolve masterserver name (%s)!"), masterserver_name) == -1)
		{
			NetPlay.MOTD = nullptr;
		}
		wz_command_interface_output("WZEVENT: lobbyerror (%u): Cannot resolve lobby server: %s\n", 0, strSockError(sockErrInt));
		server_not_there = true;
		return bProcessingConnectOrDisconnectThisCall;
	}

	// Close an existing socket.
	if (rs_socket != nullptr)
	{
		socketClose(rs_socket);
		rs_socket = nullptr;
	}

	// try each address from resolveHost until we successfully connect.
	rs_socket = socketOpenAny(hosts, 1500);
	deleteSocketAddress(hosts);

	// No address succeeded.
	if (rs_socket == nullptr)
	{
		debug(LOG_ERROR, "Cannot connect to masterserver \"%s:%d\": %s", masterserver_name, masterserver_port, strSockError(getSockErr()));
		free(NetPlay.MOTD);
		if (asprintf(&NetPlay.MOTD, _("Error connecting to the lobby server: %s.\nMake sure port %d can receive incoming connections.\nIf you're using a router configure it to use UPnP\n or to forward the port to your system."),
					 strSockError(getSockErr()), masterserver_port) == -1)
		{
			NetPlay.MOTD = nullptr;
		}
		server_not_there = true;
		return bProcessingConnectOrDisconnectThisCall;
	}

	// Get a game ID
	if (writeAll(rs_socket, "gaId", sizeof("gaId")) == SOCKET_ERROR
		|| readAll(rs_socket, &gameId, sizeof(gameId), 10000) != sizeof(gameId))
	{
		free(NetPlay.MOTD);
		if (asprintf(&NetPlay.MOTD, "Failed to retrieve a game ID: %s", strSockError(getSockErr())) == -1)
		{
			NetPlay.MOTD = nullptr;
		}
		else
		{
			debug(LOG_ERROR, "%s", NetPlay.MOTD);
		}

		// The socket has been invalidated, so get rid of it. (using them now may cause SIGPIPE).
		disconnect();
		return bProcessingConnectOrDisconnectThisCall;
	}

	gamestruct.gameId = ntohl(gameId);
	debug(LOG_NET, "Using game ID: %u", (unsigned int)gamestruct.gameId);

	wz_command_interface_output("WZEVENT: lobbyid: %" PRIu32 "\n", gamestruct.gameId);

	// Register our game with the server
	if (writeAll(rs_socket, "addg", sizeof("addg")) == SOCKET_ERROR
		// and now send what the server wants
		|| !NETsendGAMESTRUCT(rs_socket, &gamestruct))
	{
		debug(LOG_ERROR, "Failed to register game with server: %s", strSockError(getSockErr()));
		disconnect();
		return bProcessingConnectOrDisconnectThisCall;
	}

	lastServerUpdate = realTime;
	queuedServerUpdate = false;

	lastConnectionTime = realTime;
	waitingForConnectionFinalize = allocSocketSet();
	SocketSet_AddSocket(waitingForConnectionFinalize, rs_socket);

	currentState = LobbyConnectionState::Connecting_WaitingForResponse;
	return bProcessingConnectOrDisconnectThisCall;
}

bool LobbyServerConnectionHandler::disconnect()
{
	if (currentState == LobbyConnectionState::Disconnected)
	{
		return false; // already disconnected
	}

	if (rs_socket != nullptr)
	{
		// we don't need this anymore, so clean up
		socketClose(rs_socket);
		rs_socket = nullptr;
		server_not_there = true;
	}

	queuedServerUpdate = false;

	ActivityManager::instance().hostGameLobbyServerDisconnect();

	currentState = LobbyConnectionState::Disconnected;
	return true;
}

void LobbyServerConnectionHandler::sendUpdate()
{
	if (lobby_disabled || server_not_there)
	{
		if (currentState != LobbyConnectionState::Disconnected)
		{
			disconnect();
		}
		return;
	}

	if (canSendServerUpdateNow())
	{
		sendUpdateNow();
	}
	else
	{
		// queue future update
		debug(LOG_NET, "Queueing server update");
		queuedServerUpdate = true;
	}
}

void LobbyServerConnectionHandler::sendUpdateNow()
{
	ASSERT_OR_RETURN(, rs_socket != nullptr, "Null socket");
	if (lobby_disabled)
	{
		if (currentState != LobbyConnectionState::Disconnected)
		{
			disconnect();
		}
		return;
	}

	if (!NETsendGAMESTRUCT(rs_socket, &gamestruct))
	{
		disconnect();
	}
	lastServerUpdate = realTime;
	queuedServerUpdate = false;
	// newer lobby server will return a lobby response / status after each update call
	if (rs_socket && readLobbyResponse(rs_socket, NET_TIMEOUT_DELAY) == SOCKET_ERROR)
	{
		disconnect();
	}
}

void LobbyServerConnectionHandler::sendKeepAlive()
{
	ASSERT_OR_RETURN(, rs_socket != nullptr, "Null socket");
	if (writeAll(rs_socket, "keep", sizeof("keep")) == SOCKET_ERROR)
	{
		// The socket has been invalidated, so get rid of it. (using them now may cause SIGPIPE).
		disconnect();
	}
	lastServerUpdate = realTime;
}

void LobbyServerConnectionHandler::run()
{
	switch (currentState)
	{
		case LobbyConnectionState::Disconnected:
			return;
			break;
		case LobbyConnectionState::Connecting_WaitingForResponse:
		{
			// check if response has been received
			ASSERT_OR_RETURN(, waitingForConnectionFinalize != nullptr, "Null socket set");
			ASSERT_OR_RETURN(, rs_socket != nullptr, "Null socket");
			bool exceededTimeout = (realTime - lastConnectionTime >= 10000);
			// We use readLobbyResponse to display error messages and handle state changes if there's no response
			// So if exceededTimeout, just call it with a low timeout
			int checkSocketRet = checkSockets(waitingForConnectionFinalize, NET_READ_TIMEOUT);
			if (checkSocketRet == SOCKET_ERROR)
			{
				debug(LOG_ERROR, "Lost connection to lobby server");
				disconnect();
				break;
			}
			if (exceededTimeout || (checkSocketRet > 0 && socketReadReady(rs_socket)))
			{
				if (readLobbyResponse(rs_socket, NET_TIMEOUT_DELAY) == SOCKET_ERROR)
				{
					disconnect();
					break;
				}
				deleteSocketSet(waitingForConnectionFinalize);
				waitingForConnectionFinalize = nullptr;
				currentState = LobbyConnectionState::Connected;
			}
			break;
		}
		case LobbyConnectionState::Connected:
		{
			// handle sending keep alive or queued updates
			if (!queuedServerUpdate)
			{
				if (allow_joining && shouldSendServerKeepAliveNow())
				{
					// ensure that the lobby server knows we're still alive by sending a no-op "keep-alive"
					sendKeepAlive();
				}
				break;
			}
			if (!canSendServerUpdateNow())
			{
				break;
			}
			queuedServerUpdate = false;
			sendUpdateNow();
			break;
		}
	}
}

bool NETregisterServer(int state)
{
	switch (state)
	{
		case WZ_SERVER_UPDATE:
			lobbyConnectionHandler.sendUpdate();
			break;
		case WZ_SERVER_CONNECT:
			return lobbyConnectionHandler.connect();
			break;
		case WZ_SERVER_DISCONNECT:
			return lobbyConnectionHandler.disconnect();
			break;
	}

	return false;
}

// ////////////////////////////////////////////////////////////////////////
//  Check player "slots" & update player count if needed.
void NETfixPlayerCount()
{
	int maxPlayers = game.maxPlayers;
	unsigned playercount = 0;
	for (int index = 0; index < game.maxPlayers; ++index)
	{
		if (NetPlay.players[index].ai == AI_CLOSED)
		{
			--maxPlayers;
		}
		else if (NetPlay.players[index].isSpectator)
		{
			--maxPlayers;
		}
		else if (NetPlay.players[index].ai != AI_OPEN || NetPlay.players[index].allocated)
		{
			++playercount;
		}
	}

	SpectatorInfo latestSpecInfo = SpectatorInfo::currentNetPlayState();

	if (allow_joining && NetPlay.isHost && (NetPlay.playercount != playercount || gamestruct.desc.dwMaxPlayers != maxPlayers || SpectatorInfo::fromUint32(gamestruct.desc.dwUserFlags[1]) != latestSpecInfo))
	{
		debug(LOG_NET, "Updating player count from %d/%d to %d/%d", (int)NetPlay.playercount, gamestruct.desc.dwMaxPlayers, playercount, maxPlayers);
		gamestruct.desc.dwCurrentPlayers = NetPlay.playercount = playercount;
		gamestruct.desc.dwMaxPlayers = maxPlayers;
		gamestruct.desc.dwUserFlags[1] = latestSpecInfo.toUint32();
		NETregisterServer(WZ_SERVER_UPDATE);
	}

}

static void NETaddSessionBanBadIP(const std::string& badIP)
{
	if (isLoopbackIP(badIP.c_str()))
	{
		return;
	}
	size_t *pNumBadIPAttempts = tmp_badIPs.tryGetPt(badIP);
	if (pNumBadIPAttempts != nullptr)
	{
		if (*pNumBadIPAttempts < std::numeric_limits<size_t>::max())
		{
			(*pNumBadIPAttempts)++;
		}

		if (*pNumBadIPAttempts == 100)
		{
			// add to permanent ban list
			addIPToBanList(badIP.c_str(), "BAD_USER");
		}
	}
	else
	{
		tmp_badIPs.insert(badIP, 1);
	}
}

static bool quickRejectConnection(const std::string& ip)
{
	auto it = tmp_pendingIPs.find(ip);
	if (it != tmp_pendingIPs.end())
	{
		if (it->second > 1)
		{
			return true;
		}
	}
	if (tmp_badIPs.tryGetPt(ip) != nullptr)
	{
		NETaddSessionBanBadIP(ip);
		return true;
	}

	return false;
}

static void NETcloseTempSocket(unsigned int i)
{
	std::string rIP = getSocketTextAddress(tmp_socket[i]);
	SocketSet_DelSocket(tmp_socket_set, tmp_socket[i]);
	socketClose(tmp_socket[i]);
	tmp_socket[i] = nullptr;
	tmp_connectState[i].reset();
	auto it = tmp_pendingIPs.find(rIP);
	if (it != tmp_pendingIPs.end())
	{
		if (it->second > 1)
		{
			it->second--;
		}
		else
		{
			tmp_pendingIPs.erase(it);
		}
	}
}

// ////////////////////////////////////////////////////////////////////////
// Host a game with a given name and player name. & 4 user game flags
static void NETallowJoining()
{
	unsigned int i;
	int32_t result;
	bool connectFailed = true;
	uint32_t major, minor;

	if (allow_joining == false)
	{
		return;
	}
	ASSERT(NetPlay.isHost, "Cannot receive joins if not host!");

	bool bFirstTimeConnect = NETregisterServer(WZ_SERVER_CONNECT);
	if (bFirstTimeConnect)
	{
		ActivityManager::instance().updateMultiplayGameData(game, ingame, NETGameIsLocked());
		ActivitySink::ListeningInterfaces listeningInterfaces;
		if (tcp_socket != nullptr)
		{
			listeningInterfaces.IPv4 = socketHasIPv4(tcp_socket);
			if (listeningInterfaces.IPv4)
			{
				listeningInterfaces.ipv4_port = NETgetGameserverPort();
			}
			listeningInterfaces.IPv6 = socketHasIPv6(tcp_socket);
			if (listeningInterfaces.IPv6)
			{
				listeningInterfaces.ipv6_port = NETgetGameserverPort();
			}
		}
		ActivityManager::instance().hostGame(gamestruct.name, NetPlay.players[0].name, NETgetMasterserverName(), NETgetMasterserverPort(), listeningInterfaces, gamestruct.gameId);
	}

	// This is here since we need to get the status, before we can show the info.
	// FIXME: find better location to stick this?
	if ((!NetPlay.ShowedMOTD) && (NetPlay.MOTD != nullptr))
	{
		ShowMOTD();
		free(NetPlay.MOTD);
		NetPlay.MOTD = nullptr;
		NetPlay.ShowedMOTD = true;
	}

	if (tmp_socket_set == nullptr)
	{
		// initialize server socket set
		// FIXME: why is this not done in NETinit()?? - Per
		tmp_socket_set = allocSocketSet();
		if (tmp_socket_set == nullptr)
		{
			debug(LOG_ERROR, "Cannot create socket set: %s", strSockError(getSockErr()));
			return;
		}
		// FIXME: I guess initialization of allowjoining is here now... - FlexCoral
		for (auto& tmpState : tmp_connectState)
		{
			tmpState.reset();
		}
		tmp_pendingIPs.clear();
		// NOTE: Do *NOT* call tmp_badIPs.clear() here - we want to preserve it until quit
	}

	// Find the first empty socket slot
	for (i = 0; i < MAX_TMP_SOCKETS; ++i)
	{
		if (tmp_socket[i] == nullptr)
		{
			break;
		}
	}
	if (i == MAX_TMP_SOCKETS)
	{
		debug(LOG_NET, "all temp sockets are currently used up!");
	}

	// See if there's an incoming connection (if we have space to handle it!)
	if (i < MAX_TMP_SOCKETS && tmp_socket[i] == nullptr // Make sure that we're not out of sockets
	    && (tmp_socket[i] = socketAccept(tcp_socket)) != nullptr)
	{
		NETinitQueue(NETnetTmpQueue(i));
		SocketSet_AddSocket(tmp_socket_set, tmp_socket[i]);

		std::string rIP = getSocketTextAddress(tmp_socket[i]);
		connectFailed = quickRejectConnection(rIP);
		tmp_pendingIPs[rIP]++;

		std::string rIPLogEntry = "Incoming connection from:";
		rIPLogEntry.append(rIP);
		NETlogEntry(rIPLogEntry.c_str(), SYNC_FLAG, i);

		tmp_connectState[i].ip = rIP;
		tmp_connectState[i].connectTime = std::chrono::steady_clock::now();
		tmp_connectState[i].connectState = TmpSocketInfo::TmpConnectState::PendingInitialConnect;
	}

	if (checkSockets(tmp_socket_set, NET_READ_TIMEOUT) > 0)
	{
		for (i = 0; i < MAX_TMP_SOCKETS; ++i)
		{
			if (tmp_socket[i] == nullptr)
			{
				continue;
			}

			if (!socketReadReady(tmp_socket[i]))
			{
				continue;
			}

			if (tmp_connectState[i].connectState == TmpSocketInfo::TmpConnectState::PendingInitialConnect)
			{
				char *p_buffer = tmp_connectState[i].buffer;

				ssize_t sizeRead = readNoInt(tmp_socket[i], p_buffer + tmp_connectState[i].usedBuffer, 8 - tmp_connectState[i].usedBuffer);
				if (sizeRead != SOCKET_ERROR)
				{
					tmp_connectState[i].usedBuffer += sizeRead;
				}

				// A 2.3.7 client sends a "list" command first, just drop the connection.
				if (tmp_connectState[i].usedBuffer >= 5 && (strcmp(tmp_connectState[i].buffer, "list") == 0))
				{
					debug(LOG_INFO, "An old client tried to connect, closing the socket.");
					NETlogEntry("Dropping old client.", SYNC_FLAG, i);
					NETlogEntry("Invalid (old)game version", SYNC_FLAG, i);
					NETaddSessionBanBadIP(tmp_connectState[i].ip);
					connectFailed = true;
				}

				if (tmp_connectState[i].usedBuffer >= 8)
				{
					// New clients send NETCODE_VERSION_MAJOR and NETCODE_VERSION_MINOR
					// Check these numbers with our own.

					memcpy(&major, p_buffer, sizeof(uint32_t));
					major = ntohl(major);
					p_buffer += sizeof(int32_t);
					memcpy(&minor, p_buffer, sizeof(uint32_t));
					minor = ntohl(minor);

					if (major == 0 && minor == 0)
					{
						// special case for lobby server "alive" check
						// expects a special response that includes the gameId
						const char ResponseStart[] = "WZLR";
						char buf[(sizeof(char) * 4) + sizeof(uint32_t) + sizeof(uint32_t)] = { 0 };
						char *pLobbyRespBuffer = buf;
						auto push32 = [&pLobbyRespBuffer](uint32_t value) {
							uint32_t swapped = htonl(value);
							memcpy(pLobbyRespBuffer, &swapped, sizeof(swapped));
							pLobbyRespBuffer += sizeof(swapped);
						};

						// Copy response prefix chars ("WZLR")
						memcpy(pLobbyRespBuffer, ResponseStart, sizeof(char) * strlen(ResponseStart));
						pLobbyRespBuffer += sizeof(char) * strlen(ResponseStart);

						// Copy version of response
						const uint32_t response_version = 1;
						push32(response_version);

						// Copy gameId (as 32bit large big endian number)
						push32(gamestruct.gameId);

						writeAll(tmp_socket[i], buf, sizeof(buf));
						connectFailed = true;
					}
					else if (NETisCorrectVersion(major, minor))
					{
						result = htonl(ERROR_NOERROR);
						memcpy(&tmp_connectState[i].buffer, &result, sizeof(result));
						writeAll(tmp_socket[i], &tmp_connectState[i].buffer, sizeof(result));
						socketBeginCompression(tmp_socket[i]);

						// Connection is successful.
						connectFailed = false;

						// Give client a challenge to solve before connecting
						tmp_connectState[i].connectChallenge.resize(NET_PING_TMP_PING_CHALLENGE_SIZE);
						genSecRandomBytes(tmp_connectState[i].connectChallenge.data(), tmp_connectState[i].connectChallenge.size());
						NETbeginEncode(NETnetTmpQueue(i), NET_PING);
						NETbytes(&(tmp_connectState[i].connectChallenge));
						NETend();
					}
					else
					{
						debug(LOG_ERROR, "Received an invalid version \"%" PRIu32 ".%" PRIu32 "\".", major, minor);
						result = htonl(ERROR_WRONGVERSION);
						memcpy(&tmp_connectState[i].buffer, &result, sizeof(result));
						writeAll(tmp_socket[i], &tmp_connectState[i].buffer, sizeof(result));
						NETlogEntry("Invalid game version", SYNC_FLAG, i);
						NETaddSessionBanBadIP(tmp_connectState[i].ip);
						connectFailed = true;
					}
					if ((!connectFailed) && (!NET_HasAnyOpenSlots()))
					{
						// early player count test, in case they happen to get in before updates.
						// Tell the player that we are completely full.
						uint8_t rejected = ERROR_FULL;
						NETbeginEncode(NETnetTmpQueue(i), NET_REJECTED);
						NETuint8_t(&rejected);
						NETend();
						NETflush();
						connectFailed = true;
					}
				}
				else
				{
					// Continue to wait (until timeout) for the full initial connect message
					continue;
				}

				// Remove a failed connection.
				if (connectFailed)
				{
					debug(LOG_NET, "freeing temp socket %p (%d)", static_cast<void *>(tmp_socket[i]), __LINE__);
					NETcloseTempSocket(i);
				}
				else
				{
					// on initial connect success...
					tmp_connectState[i].connectState = TmpSocketInfo::TmpConnectState::PendingJoinRequest;
					tmp_connectState[i].connectTime = std::chrono::steady_clock::now(); // reset connect time
				}
				continue;
			}
			else if (tmp_connectState[i].connectState == TmpSocketInfo::TmpConnectState::PendingJoinRequest)
			{
				uint8_t buffer[NET_BUFFER_SIZE];
				ssize_t size = readNoInt(tmp_socket[i], buffer, sizeof(buffer));
				uint8_t rejected = 0;

				if ((size == 0 && socketReadDisconnected(tmp_socket[i])) || size == SOCKET_ERROR)
				{
					// disconnect or programmer error
					if (size == 0)
					{
						debug(LOG_NET, "Client socket disconnected.");
					}
					else
					{
						debug(LOG_NET, "Client socket encountered error: %s", strSockError(getSockErr()));
					}
					NETlogEntry("Client socket disconnected (allowJoining)", SYNC_FLAG, i);
					debug(LOG_NET, "freeing temp socket %p (%d)", static_cast<void *>(tmp_socket[i]), __LINE__);
					NETcloseTempSocket(i);
					continue;
				}

				NETinsertRawData(NETnetTmpQueue(i), buffer, size);

				if (!NETisMessageReady(NETnetTmpQueue(i)))
				{
					// need to wait for a full and complete join message
					// sanity check
					if (NETincompleteMessageDataBuffered(NETnetTmpQueue(i)) > (NET_BUFFER_SIZE * 16))	// something definitely big enough to encompass the expected message(s) at this point
					{
						// client is sending data that doesn't appear to be a properly formatted message - cut it off
						rejected = ERROR_WRONGDATA;
						NETbeginEncode(NETnetTmpQueue(i), NET_REJECTED);
						NETuint8_t(&rejected);
						NETend();
						NETflush();
						NETpop(NETnetTmpQueue(i));

						NETaddSessionBanBadIP(tmp_connectState[i].ip);
						NETcloseTempSocket(i);
						sync_counter.cantjoin++;
					}
					continue;
				}

				if (NETgetMessage(NETnetTmpQueue(i))->type == NET_JOIN)
				{
					char name[64];
					char ModList[modlist_string_size] = { '\0' };
					char GamePassword[password_string_size] = { '\0' };
					uint8_t playerType = 0;
					EcKey::Key pkey;
					EcKey identity;
					EcKey::Sig challengeResponse;

					NETbeginDecode(NETnetTmpQueue(i), NET_JOIN);
					NETstring(name, sizeof(name));
					NETstring(ModList, sizeof(ModList));
					NETstring(GamePassword, sizeof(GamePassword));
					NETuint8_t(&playerType);
					NETbytes(&pkey);
					NETbytes(&challengeResponse);
					NETend();

					// verify signature that player is joining with, reject him if he can not do that
					if (!identity.fromBytes(pkey, EcKey::Public) || !identity.verify(challengeResponse, tmp_connectState[i].connectChallenge.data(), tmp_connectState[i].connectChallenge.size()))
					{
						debug(LOG_ERROR, "freeing temp socket %p, couldn't create player!", static_cast<void *>(tmp_socket[i]));

						rejected = ERROR_WRONGDATA;
						NETbeginEncode(NETnetTmpQueue(i), NET_REJECTED);
						NETuint8_t(&rejected);
						NETend();
						NETflush();
						NETpop(NETnetTmpQueue(i));

						NETaddSessionBanBadIP(tmp_connectState[i].ip);
						NETcloseTempSocket(i);
						sync_counter.cantjoin++;
						continue;
					}

					// save join info in the tmp_connectState
					sstrcpy(tmp_connectState[i].receivedJoinInfo.name, name);
					tmp_connectState[i].receivedJoinInfo.playerType = playerType;
					tmp_connectState[i].receivedJoinInfo.identity = identity;

					auto& joinRequestInfo = tmp_connectState[i].receivedJoinInfo;

					// connection checks
					auto connectPermissions = netPermissionsCheck_Connect(identity);

					if ((connectPermissions.has_value() && connectPermissions.value() == ConnectPermissions::Blocked)
						|| (!connectPermissions.has_value() && onBanList(tmp_connectState[i].ip.c_str())))
					{
						char buf[256] = {'\0'};
						ssprintf(buf, "** A player that you have kicked tried to rejoin the game, and was rejected. IP: %s", tmp_connectState[i].ip.c_str());
						debug(LOG_INFO, "%s", buf);
						NETlogEntry(buf, SYNC_FLAG, i);

						// Player has been kicked before, kick again.
						rejected = (uint8_t)ERROR_KICKED;
					}
					else if (joinRequestInfo.playerType != NET_JOIN_SPECTATOR && playerManagementRecord.hostMovedPlayerToSpectators(tmp_connectState[i].ip))
					{
						// The host previously relegated a player from this IP address to Spectators (this game), and it seems they are trying to rejoin as a Player - deny this
						char buf[256] = {'\0'};
						ssprintf(buf, "** A player that you moved to Spectators tried to rejoin the game (as a Player), and was rejected. IP: %s", tmp_connectState[i].ip.c_str());
						debug(LOG_INFO, "%s", buf);
						NETlogEntry(buf, SYNC_FLAG, i);

						// Player has been kicked before, kick again.
						rejected = (uint8_t)ERROR_KICKED;
					}
					else if (NetPlay.GamePassworded && strcmp(NetPlay.gamePassword, GamePassword) != 0)
					{
						// Wrong password. Reject.
						rejected = (uint8_t)ERROR_WRONGPASSWORD;
					}
					else if ((int)NetPlay.playercount > gamestruct.desc.dwMaxPlayers)
					{
						// Game full. Reject.
						rejected = (uint8_t)ERROR_FULL;
					}

					if (rejected)
					{
						char buf[256] = {'\0'};
						ssprintf(buf, "**Rejecting player(%s), reason (%u). ", tmp_connectState[i].ip.c_str(), (unsigned int) rejected);
						debug(LOG_INFO, "%s", buf);
						NETlogEntry(buf, SYNC_FLAG, i);
						NETbeginEncode(NETnetTmpQueue(i), NET_REJECTED);
						NETuint8_t(&rejected);
						NETend();
						NETflush();
						NETpop(NETnetTmpQueue(i));

						NETcloseTempSocket(i);
						sync_counter.cantjoin++;
						continue;
					}

					NETpop(NETnetTmpQueue(i));

					// on passing all built-in checks for connect...
					if (bAsyncJoinApprovalEnabled)
					{
						tmp_connectState[i].connectState = TmpSocketInfo::TmpConnectState::PendingAsyncApproval;

						// use the connectChallenge as a unique, random join ID
						tmp_connectState[i].uniqueJoinID = base64Encode(tmp_connectState[i].connectChallenge);

						std::string joinerPublicKeyB64 = base64Encode(joinRequestInfo.identity.toBytes(EcKey::Public));
						std::string joinerIdentityHash = joinRequestInfo.identity.publicHashString();
						std::string joinerName = joinRequestInfo.name;
						std::string joinerNameB64 = base64Encode(std::vector<unsigned char>(joinerName.begin(), joinerName.end()));
						wz_command_interface_output("WZEVENT: join approval needed: %s %s %s %s %s %s\n", tmp_connectState[i].uniqueJoinID.c_str(), tmp_connectState[i].ip.c_str(), joinerIdentityHash.c_str(), joinerPublicKeyB64.c_str(), joinerNameB64.c_str(), (joinRequestInfo.playerType == NET_JOIN_SPECTATOR) ? "spec" : "play");
					}
					else
					{
						// if async join approval is not enabled, go directly to pending success
						tmp_connectState[i].connectState = TmpSocketInfo::TmpConnectState::ProcessJoin;
					}
					tmp_connectState[i].connectTime = std::chrono::steady_clock::now(); // reset connect time
				}
				else
				{
					// unexpected message type at this time
					// reject the bad client
					rejected = ERROR_WRONGDATA;
					NETbeginEncode(NETnetTmpQueue(i), NET_REJECTED);
					NETuint8_t(&rejected);
					NETend();
					NETflush();
					NETpop(NETnetTmpQueue(i));

					NETaddSessionBanBadIP(tmp_connectState[i].ip);
					NETcloseTempSocket(i);
					sync_counter.cantjoin++;
				}
				continue;
			}
		}
	}

	auto currentSteadTime = std::chrono::steady_clock::now();

	for (i = 0; i < MAX_TMP_SOCKETS; ++i)
	{
		if (tmp_socket[i] == nullptr)
		{
			continue;
		}

		if (tmp_connectState[i].connectState == TmpSocketInfo::TmpConnectState::PendingAsyncApproval)
		{
			// check if async approval result has been set
			if (tmp_connectState[i].asyncJoinApprovalResult.has_value())
			{
				if (tmp_connectState[i].asyncJoinApprovalResult.value() == ERROR_NOERROR)
				{
					// async join approved - process the join
					tmp_connectState[i].connectState = TmpSocketInfo::TmpConnectState::ProcessJoin;
					tmp_connectState[i].connectTime = std::chrono::steady_clock::now(); // reset connect time
					// deliberately fall-through to the TmpSocketInfo::TmpConnectState::ProcessJoin condition further below
				}
				else
				{
					// async join rejected
					uint8_t rejected = static_cast<uint8_t>(tmp_connectState[i].asyncJoinApprovalResult.value());
					char buf[256] = {'\0'};
					ssprintf(buf, "**Rejecting player(%s), due to async approval rejection, reason (%u).", tmp_connectState[i].ip.c_str(), static_cast<unsigned int>(rejected));
					debug(LOG_INFO, "%s", buf);
					NETlogEntry(buf, SYNC_FLAG, i);
					NETbeginEncode(NETnetTmpQueue(i), NET_REJECTED);
					NETuint8_t(&rejected);
					NETend();
					NETflush();
					auto tmpQueue = NETnetTmpQueue(i);
					if (NETisMessageReady(tmpQueue))
					{
						NETpop(tmpQueue);
					}

					NETcloseTempSocket(i);
					sync_counter.cantjoin++;
					continue; // continue to next tmp_socket
				}
			}
			else
			{
				// if no async approval, do a timeout check
				std::chrono::milliseconds::rep timeout = 5000; // must currently be set relatively short because of the client join blocking delay
				if (std::chrono::duration_cast<std::chrono::milliseconds>(currentSteadTime - tmp_connectState[i].connectTime).count() > timeout)
				{
					debug(LOG_INFO, "Freeing temp socket %p due to async connection approval timeout (IP: %s)", static_cast<void *>(tmp_socket[i]), tmp_connectState[i].ip.c_str());

					uint8_t rejected = ERROR_HOSTDROPPED;
					NETbeginEncode(NETnetTmpQueue(i), NET_REJECTED);
					NETuint8_t(&rejected);
					NETend();
					NETflush();
					auto tmpQueue = NETnetTmpQueue(i);
					if (NETisMessageReady(tmpQueue))
					{
						NETpop(tmpQueue);
					}
					NETcloseTempSocket(i);
				}
				continue; // continue to next tmp_socket
			}
		}
		// note: *not* an "else if" because the condition above may transition the state to ProcessJoin!
		if (tmp_connectState[i].connectState == TmpSocketInfo::TmpConnectState::ProcessJoin)
		{
			optional<uint32_t> tmp = nullopt;
			uint8_t rejected = 0;

			auto joinRequestInfo = tmp_connectState[i].receivedJoinInfo; // keep a copy
			tmp_connectState[i].reset();

			if ((joinRequestInfo.playerType == NET_JOIN_SPECTATOR) || (int)NetPlay.playercount <= gamestruct.desc.dwMaxPlayers)
			{
				tmp = NET_CreatePlayer(joinRequestInfo.name, false, (joinRequestInfo.playerType == NET_JOIN_SPECTATOR));
			}

			if (!tmp.has_value() || tmp.value() > static_cast<uint32_t>(std::numeric_limits<uint8_t>::max()))
			{
				ASSERT(tmp.value_or(0) <= static_cast<uint32_t>(std::numeric_limits<uint8_t>::max()), "Currently limited to uint8_t");
				debug(LOG_ERROR, "freeing temp socket %p, couldn't create player!", static_cast<void *>(tmp_socket[i]));

				// Tell the player that we are full.
				rejected = ERROR_FULL;
				NETbeginEncode(NETnetTmpQueue(i), NET_REJECTED);
				NETuint8_t(&rejected);
				NETend();
				NETflush();

				NETcloseTempSocket(i);
				sync_counter.cantjoin++;
				continue; // continue to next tmp_socket
			}

			uint8_t index = static_cast<uint8_t>(tmp.value());

			debug(LOG_NET, "freeing temp socket %p (%d), creating permanent socket.", static_cast<void *>(tmp_socket[i]), __LINE__);
			SocketSet_DelSocket(tmp_socket_set, tmp_socket[i]);
			connected_bsocket[index] = tmp_socket[i];
			NET_waitingForIndexChangeAckSince[index] = nullopt;
			tmp_socket[i] = nullptr;
			SocketSet_AddSocket(socket_set, connected_bsocket[index]);
			NETmoveQueue(NETnetTmpQueue(i), NETnetQueue(index));

			// Get country code from client's ip address
			const char *countryCode = nullptr;
			if (gi)
			{
				countryCode = GeoIP_country_code_by_name(gi, NetPlay.players[index].IPtextAddress);
			}
			if (countryCode)
			{
				sstrcpy(NetPlay.players[index].countryCode, countryCode);
			}

			// Copy player's IP address.
			sstrcpy(NetPlay.players[index].IPtextAddress, getSocketTextAddress(connected_bsocket[index]));

			NETbeginEncode(NETnetQueue(index), NET_ACCEPTED);
			NETuint8_t(&index);
			NETuint32_t(&NetPlay.hostPlayer);
			NETend();

			// First send info about players to newcomer.
			NETSendAllPlayerInfoTo(index);
			// then send info about newcomer to all players.
			NETBroadcastPlayerInfo(index);

			char buf[250] = {'\0'};
			const char* pPlayerType = (NetPlay.players[index].isSpectator) ? "Spectator" : "Player";
			snprintf(buf, sizeof(buf), "%s[%" PRIu8 "] %s has joined, IP is: %s", pPlayerType, index, NetPlay.players[index].name, NetPlay.players[index].IPtextAddress);
			debug(LOG_INFO, "%s", buf);
			NETlogEntry(buf, SYNC_FLAG, index);

			std::string joinerPublicKeyB64 = base64Encode(joinRequestInfo.identity.toBytes(EcKey::Public));
			std::string joinerIdentityHash = joinRequestInfo.identity.publicHashString();
			wz_command_interface_output("WZEVENT: player join: %u %s %s %s\n", i, joinerPublicKeyB64.c_str(), joinerIdentityHash.c_str(), NetPlay.players[i].IPtextAddress);

			debug(LOG_NET, "%s, %s, with index of %u has joined using socket %p", pPlayerType, NetPlay.players[index].name, (unsigned int)index, static_cast<void *>(connected_bsocket[index]));

			// Increment player count
			gamestruct.desc.dwCurrentPlayers++;

			MultiPlayerJoin(index);

			ingame.VerifiedIdentity[index] = true;

			// Narrowcast to new player that everyone has joined.
			for (uint8_t j = 0; j < MAX_CONNECTED_PLAYERS; ++j)
			{
				if (index != j)  // We will broadcast the index == j case.
				{
					if (NetPlay.players[j].allocated)
					{
						NETbeginEncode(NETnetQueue(index), NET_PLAYER_JOINED);
						NETuint8_t(&j);
						NETend();
					}
				}
			}

			// Broadcast to everyone that a new player has joined
			NETbeginEncode(NETbroadcastQueue(), NET_PLAYER_JOINED);
			NETuint8_t(&index);
			NETend();

			for (uint8_t j = 0; j < MAX_CONNECTED_PLAYERS; ++j)
			{
				NETBroadcastPlayerInfo(j);
			}
			NETfixDuplicatePlayerNames();

			// Send the updated GAMESTRUCT to the masterserver
			NETregisterServer(WZ_SERVER_UPDATE);

			// reset flags for new players
			if (NetPlay.players[index].wzFiles)
			{
				NetPlay.players[index].wzFiles->clear();
			}
			else
			{
				ASSERT(false, "wzFiles is uninitialized?? (Player: %" PRIu8 ")", index);
			}
			continue; // continue to next tmp_socket
		}

		// in all other cases, do a timeout check
		std::chrono::milliseconds::rep timeout = (tmp_connectState[i].connectState == TmpSocketInfo::TmpConnectState::PendingInitialConnect) ? 2500 : 3000;

		if (std::chrono::duration_cast<std::chrono::milliseconds>(currentSteadTime - tmp_connectState[i].connectTime).count() > timeout)
		{
			debug(LOG_INFO, "Freeing temp socket %p due to initial connection timeout (IP: %s)", static_cast<void *>(tmp_socket[i]), tmp_connectState[i].ip.c_str());

			uint8_t rejected = 0;
			rejected = ERROR_WRONGDATA;
			NETbeginEncode(NETnetTmpQueue(i), NET_REJECTED);
			NETuint8_t(&rejected);
			NETend();
			NETflush();
			auto tmpQueue = NETnetTmpQueue(i);
			if (NETisMessageReady(tmpQueue))
			{
				NETpop(tmpQueue);
			}

			std::string rIP = getSocketTextAddress(tmp_socket[i]);
			NETaddSessionBanBadIP(rIP);

			NETcloseTempSocket(i);
			sync_counter.cantjoin++;
		}
	}
}

bool NEThostGame(const char *SessionName, const char *PlayerName, bool spectatorHost,
                 uint32_t gameType, uint32_t two, uint32_t three, uint32_t four,
                 UDWORD plyrs)	// # of players.
{
	debug(LOG_NET, "NEThostGame(%s, %s, %" PRIu32 ", %" PRIu32 ", %" PRIu32 ", %" PRIu32 ", %u)", SessionName, PlayerName,
		  gameType, two, three, four, plyrs);

	netPlayersUpdated = true;

	for (unsigned playerIndex = 0; playerIndex < MAX_CONNECTED_PLAYERS; ++playerIndex)
	{
		initPlayerNetworkProps(playerIndex);
	}
	if (!NetPlay.bComms)
	{
		ASSERT(!spectatorHost, "spectatorHost flag will be ignored");
		selectedPlayer			= 0;
		NetPlay.isHost			= true;
		NetPlay.hostPlayer 		= selectedPlayer;
		NetPlay.players[0].allocated	= true;
		NetPlay.players[0].difficulty	= AIDifficulty::HUMAN;
		NetPlay.playercount		= 1;
		debug(LOG_NET, "Hosting but no comms");
		// Now switch player color of the host to what they normally use for MP games
		if (war_getMPcolour() >= 0)
		{
			changeColour(NetPlay.hostPlayer, war_getMPcolour(), true);
		}
		return true;
	}

	// tcp_socket is the connection to the lobby server (or machine)
	if (!tcp_socket)
	{
		tcp_socket = socketListen(gameserver_port);
	}
	if (tcp_socket == nullptr)
	{
		debug(LOG_ERROR, "Cannot connect to master self: %s", strSockError(getSockErr()));
		return false;
	}
	debug(LOG_NET, "New tcp_socket = %p", static_cast<void *>(tcp_socket));
	// Host needs to create a socket set for MAX_PLAYERS
	if (!socket_set)
	{
		socket_set = allocSocketSet();
	}
	if (socket_set == nullptr)
	{
		debug(LOG_ERROR, "Cannot create socket set: %s", strSockError(getSockErr()));
		return false;
	}
	// allocate socket storage for all possible players
	for (unsigned i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
	{
		connected_bsocket[i] = nullptr;
		NETinitQueue(NETnetQueue(i));
	}

	NetPlay.isHost = true;
	NETlogEntry("Hosting game, resetting ban list.", SYNC_FLAG, 0);
	NETpermissionsInit();
	playerManagementRecord.clear();
	sstrcpy(gamestruct.name, SessionName);
	memset(&gamestruct.desc, 0, sizeof(gamestruct.desc));
	gamestruct.desc.dwSize = sizeof(gamestruct.desc);
	//gamestruct.desc.guidApplication = GAME_GUID;
	memset(gamestruct.desc.host, 0, sizeof(gamestruct.desc.host));
	gamestruct.desc.dwCurrentPlayers = (!spectatorHost) ? 1 : 0;
	gamestruct.desc.dwMaxPlayers = plyrs;
	gamestruct.desc.dwFlags = 0;
	gamestruct.desc.dwUserFlags[0] = gameType;
	gamestruct.desc.dwUserFlags[1] = two;
	gamestruct.desc.dwUserFlags[2] = three;
	gamestruct.desc.dwUserFlags[3] = four;
	memset(gamestruct.secondaryHosts, 0, sizeof(gamestruct.secondaryHosts));
	sstrcpy(gamestruct.extra, "Extra");						// extra string (future use)
	gamestruct.hostPort = gameserver_port;
	sstrcpy(gamestruct.mapname, game.map);					// map we are hosting
	sstrcpy(gamestruct.hostname, PlayerName);
	sstrcpy(gamestruct.versionstring, versionString);		// version (string)
	sstrcpy(gamestruct.modlist, getModList().c_str());      // List of mods
	gamestruct.GAMESTRUCT_VERSION = 4;						// version of this structure
	gamestruct.game_version_major = NETCODE_VERSION_MAJOR;	// Netcode Major version
	gamestruct.game_version_minor = NETCODE_VERSION_MINOR;	// NetCode Minor version
//	gamestruct.privateGame = 0;								// if true, it is a private game
	gamestruct.pureMap = game.isMapMod;								// If map-mod...
	gamestruct.Mods = 0;										// number of concatenated mods?
	gamestruct.gameId  = 0;
	gamestruct.limits = 0x0;									// used for limits
#if defined(WZ_OS_WIN)
	gamestruct.future3 = 0x77696e;								// for future use
#elif defined (WZ_OS_MAC)
	gamestruct.future3 = 0x6d6163;								// for future use
#else
	gamestruct.future3 = 0x6c696e;								// for future use
#endif
	gamestruct.future4 = NETCODE_VERSION_MAJOR << 16 | NETCODE_VERSION_MINOR;	// for future use

	if (spectatorHost)
	{
		// open one spectator slot for the host
		ASSERT(_NET_openNewSpectatorSlot_internal(false).has_value(), "Unable to open spectator slot for host??");
	}
	optional<uint32_t> newHostPlayerIdx = NET_CreatePlayer(PlayerName, (getHostLaunch() == HostLaunch::Autohost), spectatorHost);
	ASSERT_OR_RETURN(false, newHostPlayerIdx.has_value() && (newHostPlayerIdx.value() < MAX_PLAYERS || (spectatorHost && newHostPlayerIdx.value() < MAX_CONNECTED_PLAYERS)), "Failed to create player");
	selectedPlayer = newHostPlayerIdx.value();
	realSelectedPlayer = selectedPlayer;
	NetPlay.isHost	= true;
	NetPlay.isHostAlive = true;
	NetPlay.HaveUpgrade = false;
	NetPlay.hostPlayer	= selectedPlayer;

	MultiPlayerJoin(selectedPlayer);

	// Now switch player color of the host to what they normally use for SP games
	if (NetPlay.hostPlayer < MAX_PLAYERS && war_getMPcolour() >= 0)
	{
		changeColour(NetPlay.hostPlayer, war_getMPcolour(), true);
	}

	allow_joining = true;

	NETregisterServer(WZ_SERVER_DISCONNECT);

	debug(LOG_NET, "Hosting a server. We are player %d.", selectedPlayer);

	return true;
}

// ////////////////////////////////////////////////////////////////////////
// Stop the dplay interface from accepting more players.
bool NEThaltJoining()
{
	debug(LOG_NET, "temporarily locking game to prevent more players");

	allow_joining = false;
	// disconnect from the master server
	NETregisterServer(WZ_SERVER_DISCONNECT);
	return true;
}

// ////////////////////////////////////////////////////////////////////////
// find games on open connection
bool NETenumerateGames(const std::function<bool (const GAMESTRUCT& game)>& handleEnumerateGameFunc)
{
	SocketAddress *hosts;
	int result = 0;
	debug(LOG_NET, "Looking for games...");

	if (getLobbyError() == ERROR_INVALID || getLobbyError() == ERROR_KICKED || getLobbyError() == ERROR_HOSTDROPPED)
	{
		return false;
	}
	setLobbyError(ERROR_NOERROR);

	if (!NetPlay.bComms)
	{
		debug(LOG_ERROR, "Likely missing NETinit(true) - this won't return any results");
		return false;
	}
	if ((hosts = resolveHost(masterserver_name, masterserver_port)) == nullptr)
	{
		debug(LOG_ERROR, "Cannot resolve hostname \"%s\": %s", masterserver_name, strSockError(getSockErr()));
		setLobbyError(ERROR_CONNECTION);
		return false;
	}

	if (tcp_socket != nullptr)
	{
		debug(LOG_NET, "Deleting tcp_socket %p", static_cast<void *>(tcp_socket));
		if (socket_set)
		{
			SocketSet_DelSocket(socket_set, tcp_socket);
		}
		socketClose(tcp_socket);
		tcp_socket = nullptr;
	}

	tcp_socket = socketOpenAny(hosts, 15000);

	deleteSocketAddress(hosts);
	hosts = nullptr;

	if (tcp_socket == nullptr)
	{
		debug(LOG_ERROR, "Cannot connect to \"%s:%d\": %s", masterserver_name, masterserver_port, strSockError(getSockErr()));
		setLobbyError(ERROR_CONNECTION);
		return false;
	}
	debug(LOG_NET, "New tcp_socket = %p", static_cast<void *>(tcp_socket));
	// client machines only need 1 socket set
	socket_set = allocSocketSet();
	if (socket_set == nullptr)
	{
		debug(LOG_ERROR, "Cannot create socket set: %s", strSockError(getSockErr()));
		setLobbyError(ERROR_CONNECTION);
		return false;
	}
	debug(LOG_NET, "Created socket_set %p", static_cast<void *>(socket_set));

	SocketSet_AddSocket(socket_set, tcp_socket);

	debug(LOG_NET, "Sending list cmd");

	if (writeAll(tcp_socket, "list", sizeof("list")) == SOCKET_ERROR)
	{
		debug(LOG_NET, "Server socket encountered error: %s", strSockError(getSockErr()));
		SocketSet_DelSocket(socket_set, tcp_socket);		// mark it invalid
		socketClose(tcp_socket);
		tcp_socket = nullptr;

		// when we fail to receive a game count, bail out
		setLobbyError(ERROR_CONNECTION);
		return false;
	}

	// Retrieve the first batch of game structs
	// Earlier versions (and earlier lobby servers) restricted this to no more than 11
	std::vector<GAMESTRUCT> initialBatchOfGameStructs;

	if (!readGameStructsList(tcp_socket, NET_TIMEOUT_DELAY, [&initialBatchOfGameStructs](const GAMESTRUCT &lobbyGame) -> bool {
		initialBatchOfGameStructs.push_back(lobbyGame);
		return true; // continue enumerating
	}))
	{
		SocketSet_DelSocket(socket_set, tcp_socket);		// mark it invalid
		socketClose(tcp_socket);
		tcp_socket = nullptr;

		setLobbyError(ERROR_CONNECTION);
		return false;
	}

	// read the lobby response
	if (readLobbyResponse(tcp_socket, NET_TIMEOUT_DELAY) == SOCKET_ERROR)
	{
		socketClose(tcp_socket);
		tcp_socket = nullptr;
		addConsoleMessage(_("Failed to get a lobby response!"), DEFAULT_JUSTIFY, NOTIFY_MESSAGE);

		// treat as fatal error
		setLobbyError(ERROR_CONNECTION);
		return false;
	}

	// Backwards-compatible protocol enhancement, to raise game limit
	// Expects a uint32_t to determine whether to ignore the first batch of game structs
	// Earlier lobby servers will not provide anything, or may null-pad the lobby response / MOTD string
	// Hence as long as we don't treat "0" as signifying any change in behavior, this should be safe + backwards-compatible
	#define IGNORE_FIRST_BATCH 1
	uint32_t responseParameters = 0;
	if ((result = readAll(tcp_socket, &responseParameters, sizeof(responseParameters), NET_TIMEOUT_DELAY)) == sizeof(responseParameters))
	{
		responseParameters = ntohl(responseParameters);

		bool requestSecondBatch = true;
		bool ignoreFirstBatch = ((responseParameters & IGNORE_FIRST_BATCH) == IGNORE_FIRST_BATCH);
		if (!ignoreFirstBatch)
		{
			// pass the first batch to the handleEnumerateGameFunc
			for (const auto& lobbyGame : initialBatchOfGameStructs)
			{
				if (!handleEnumerateGameFunc(lobbyGame))
				{
					// stop enumerating
					requestSecondBatch = false;
					break;
				}
			}
		}

		if (requestSecondBatch)
		{
			if (!readGameStructsList(tcp_socket, NET_TIMEOUT_DELAY, handleEnumerateGameFunc))
			{
				// we failed to read a second list of game structs

				if (!ignoreFirstBatch)
				{
					// just log and treat the error as non-fatal
					debug(LOG_NET, "Second readGameStructsList call failed - ignoring");
				}
				else
				{
					// if ignoring the first batch, treat this as a fatal error
					debug(LOG_NET, "Second readGameStructsList call failed");

					SocketSet_DelSocket(socket_set, tcp_socket);		// mark it invalid
					socketClose(tcp_socket);
					tcp_socket = nullptr;

					// when we fail to receive a game count, bail out
					setLobbyError(ERROR_CONNECTION);
					return false;
				}
			}
		}
	}
	else
	{
		// to support earlier lobby servers that don't provide this additional second batch (and optional parameter), just process the first batch
		for (const auto& lobbyGame : initialBatchOfGameStructs)
		{
			if (!handleEnumerateGameFunc(lobbyGame))
			{
				// stop enumerating
				break;
			}
		}
	}

	SocketSet_DelSocket(socket_set, tcp_socket);		// mark it invalid (we are done with it)
	socketClose(tcp_socket);
	tcp_socket = nullptr;

	return true;
}

bool NETfindGames(std::vector<GAMESTRUCT>& results, size_t startingIndex, size_t resultsLimit, bool onlyMatchingLocalVersion /*= false*/)
{
	size_t gamecount = 0;
	results.clear();

	if (lobby_disabled)
	{
		return true;
	}

	bool success = NETenumerateGames([&results, &gamecount, startingIndex, resultsLimit, onlyMatchingLocalVersion](const GAMESTRUCT &lobbyGame) -> bool {
		if (gamecount++ < startingIndex)
		{
			// skip this item, continue
			return true;
		}
		if ((resultsLimit > 0) && (results.size() >= resultsLimit))
		{
			// stop processing games
			return false;
		}
		if (onlyMatchingLocalVersion && !NETisCorrectVersion(lobbyGame.game_version_major, lobbyGame.game_version_minor))
		{
			// skip this non-matching version, continue
			return true;
		}
		results.push_back(lobbyGame);
		return true;
	});

	return success;
}

bool NETfindGame(uint32_t gameId, GAMESTRUCT& output)
{
	bool foundMatch = false;
	memset(&output, 0x00, sizeof(output));
	NETenumerateGames([&foundMatch, &output, gameId](const GAMESTRUCT &lobbyGame) -> bool {
		if (lobbyGame.gameId != gameId)
		{
			// not a match - continue enumerating
			return true;
		}
		output = lobbyGame;
		foundMatch = true;
		return false; // stop searching
	});
	return foundMatch;
}

// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Functions used to setup and join games.
bool NETjoinGame(const char *host, uint32_t port, const char *playername, const EcKey& playerIdentity, bool asSpectator /*= false*/)
{
	SocketAddress *hosts = nullptr;
	unsigned int i;
	char buffer[sizeof(int32_t) * 2] = { 0 };
	char *p_buffer;
	uint32_t result;

	if (port == 0)
	{
		port = gameserver_port;
	}

	debug(LOG_NET, "resetting sockets.");
	NETclose();	// just to be sure :)

	debug(LOG_NET, "Trying to join [%s]:%d ...", host, port);

	netPlayersUpdated = true;

	hosts = resolveHost(host, port);
	if (hosts == nullptr)
	{
		debug(LOG_ERROR, "Cannot resolve hostname \"%s\": %s", host, strSockError(getSockErr()));
		return false;
	}

	if (tcp_socket != nullptr)
	{
		socketClose(tcp_socket);
	}

	tcp_socket = socketOpenAny(hosts, 15000);
	deleteSocketAddress(hosts);

	if (tcp_socket == nullptr)
	{
		debug(LOG_ERROR, "Cannot connect to [%s]:%d, %s", host, port, strSockError(getSockErr()));
		return false;
	}

	// client machines only need 1 socket set
	socket_set = allocSocketSet();
	if (socket_set == nullptr)
	{
		debug(LOG_ERROR, "Cannot create socket set: %s", strSockError(getSockErr()));
		return false;
	}
	debug(LOG_NET, "Created socket_set %p", static_cast<void *>(socket_set));

	// tcp_socket is used to talk to host machine
	SocketSet_AddSocket(socket_set, tcp_socket);

	// Send NETCODE_VERSION_MAJOR and NETCODE_VERSION_MINOR
	p_buffer = buffer;
	auto pushu32 = [&](uint32_t value) {
		uint32_t swapped = htonl(value);
		memcpy(p_buffer, &swapped, sizeof(swapped));
		p_buffer += sizeof(swapped);
	};
	pushu32(NETCODE_VERSION_MAJOR);
	pushu32(NETCODE_VERSION_MINOR);

	if (writeAll(tcp_socket, buffer, sizeof(buffer)) == SOCKET_ERROR
	    || readAll(tcp_socket, &result, sizeof(result), 1500) != sizeof(result))
	{
		debug(LOG_ERROR, "Couldn't send my version.");
		return false;
	}

	result = ntohl(result);
	if (result != ERROR_NOERROR)
	{
		debug(LOG_ERROR, "Received error %d", result);

		SocketSet_DelSocket(socket_set, tcp_socket);
		socketClose(tcp_socket);
		tcp_socket = nullptr;
		deleteSocketSet(socket_set);
		socket_set = nullptr;

		setLobbyError((LOBBY_ERROR_TYPES)result);
		return false;
	}

	// Allocate memory for a new socket
	NetPlay.hostPlayer = NET_HOST_ONLY;
	NETinitQueue(NETnetQueue(NET_HOST_ONLY));
	// NOTE: tcp_socket = bsocket now!
	bsocket = tcp_socket;
	tcp_socket = nullptr;
	socketBeginCompression(bsocket);

	uint8_t playerType = (!asSpectator) ? NET_JOIN_PLAYER : NET_JOIN_SPECTATOR;
	
	if (bsocket == nullptr)
	{
		NETclose();
		return false;  // Connection dropped while sending NET_JOIN.
	}
	socketFlush(bsocket, NetPlay.hostPlayer);  // Make sure the message was completely sent.

	i = wzGetTicks();
	// Loop until we've been accepted into the game
	for (;;)
	{
		NETQUEUE queue;
		uint8_t type;

		// FIXME: shouldn't there be some sort of rejection message?
		if ((unsigned)wzGetTicks() > i + 5000)
		{
			// timeout
			NETclose();
			return false;
		}

		if (bsocket == nullptr)
		{
			NETclose();
			return false;  // Connection dropped.
		}

		if (!NETrecvNet(&queue, &type))
		{
			continue;
		}

		if (type == NET_ACCEPTED)
		{
			// :)
			uint8_t index;

			NetPlay.hostPlayer = MAX_CONNECTED_PLAYERS + 1; // invalid host index

			NETbeginDecode(queue, NET_ACCEPTED);
			// Retrieve the player ID the game host arranged for us
			NETuint8_t(&index);
			NETuint32_t(&NetPlay.hostPlayer); // and the host player idx
			NETend();
			NETpop(queue);

			if (NetPlay.hostPlayer >= MAX_CONNECTED_PLAYERS)
			{
				debug(LOG_ERROR, "Bad host player number (%" PRIu32 ") received from host!", NetPlay.hostPlayer);
				NETclose();
				return false;
			}

			if (NetPlay.hostPlayer != NET_HOST_ONLY)
			{
				NETswapQueues(NETnetQueue(NET_HOST_ONLY), NETnetQueue(NetPlay.hostPlayer));
			}

			selectedPlayer = index;
			realSelectedPlayer = selectedPlayer;
			debug(LOG_NET, "NET_ACCEPTED received. Accepted into the game - I'm player %u using bsocket %p", (unsigned int)index, static_cast<void *>(bsocket));
			NetPlay.isHost = false;
			NetPlay.isHostAlive = true;

			if (index >= MAX_CONNECTED_PLAYERS)
			{
				debug(LOG_ERROR, "Bad player number (%u) received from host!", index);
				NETclose();
				return false;
			}

			NetPlay.players[index].allocated = true;
			setPlayerName(index, playername);
			NetPlay.players[index].heartbeat = true;

			// Copy host IP address.
			sstrcpy(NetPlay.players[NetPlay.hostPlayer].IPtextAddress, getSocketTextAddress(bsocket));

			// Get country code from host ip address
			const char *countryCode = nullptr;
			if (gi)
			{
				countryCode = GeoIP_country_code_by_name(gi, NetPlay.players[NetPlay.hostPlayer].IPtextAddress);
			}
			if (countryCode)
			{
				sstrcpy(NetPlay.players[NetPlay.hostPlayer].countryCode, countryCode);
			}

			return true;
		}
		else if (type == NET_REJECTED)
		{
			uint8_t rejection = 0;

			NETbeginDecode(queue, NET_REJECTED);
			NETuint8_t(&rejection);
			NETend();
			NETpop(queue);

			debug(LOG_NET, "NET_REJECTED received. Error code: %u", (unsigned int) rejection);

			setLobbyError((LOBBY_ERROR_TYPES)rejection);
			NETclose();
			return false;
		}
		else if (type == NET_PING)
		{
			std::vector<uint8_t> challenge(NET_PING_TMP_PING_CHALLENGE_SIZE, 0);
			NETbeginDecode(NETnetQueue(NET_HOST_ONLY), NET_PING);
			NETbytes(&challenge, NET_PING_TMP_PING_CHALLENGE_SIZE * 4);
			NETend();
			NETpop(queue);

			EcKey::Sig challengeResponse = playerIdentity.sign(challenge.data(), challenge.size());
			EcKey::Key identity = playerIdentity.toBytes(EcKey::Public);

			NETbeginEncode(NETnetQueue(NET_HOST_ONLY), NET_JOIN);
			NETstring(playername, 64);
			NETstring(getModList().c_str(), modlist_string_size);
			NETstring(NetPlay.gamePassword, sizeof(NetPlay.gamePassword));
			NETuint8_t(&playerType);
			NETbytes(&identity);
			NETbytes(&challengeResponse);
			NETend();
			NETflush();
		}
		else
		{
			debug(LOG_ERROR, "Unexpected %s.", messageTypeToString(type));
			NETpop(queue);
		}
	}
}

/*!
 * Set the masterserver name
 * \param hostname The hostname of the masterserver to connect to
 */
void NETsetMasterserverName(const char *hostname)
{
	sstrcpy(masterserver_name, hostname);
}

/**
 * @return The hostname of the masterserver we will connect to.
 */
const char *NETgetMasterserverName()
{
	return masterserver_name;
}

/*!
 * Set the masterserver port
 * \param port The port of the masterserver to connect to
 */
void NETsetMasterserverPort(unsigned int port)
{
	const unsigned int MAX_PORT = 65535;
	if (port > MAX_PORT || port == 0)
	{
		debug(LOG_ERROR, "Invalid port number: %u", port);
		return;
	}
	masterserver_port = port;
}

/**
 * @return The port of the masterserver we will connect to.
 */
unsigned int NETgetMasterserverPort()
{
	return masterserver_port;
}

/*!
 * Set the port we shall host games on
 * \param port The port to listen to
 */
void NETsetGameserverPort(unsigned int port)
{
	gameserver_port = port;
}

/**
 * @return The port we will host games on.
 */
unsigned int NETgetGameserverPort()
{
	return gameserver_port;
}

/*!
* Set the join preference for IPv6
* \param bTryIPv6First Whether to attempt IPv6 first when joining, before IPv4.
*/
void NETsetJoinPreferenceIPv6(bool bTryIPv6First)
{
	bJoinPrefTryIPv6First = bTryIPv6First;
}

/**
* @return Whether joining a game that advertises both IPv6 and IPv4 should attempt IPv6 first.
*/
bool NETgetJoinPreferenceIPv6()
{
	return bJoinPrefTryIPv6First;
}

void NETsetDefaultMPHostFreeChatPreference(bool enabled)
{
	bDefaultHostFreeChatEnabled = enabled;
}

bool NETgetDefaultMPHostFreeChatPreference()
{
	return bDefaultHostFreeChatEnabled;
}

void NETsetPlayerConnectionStatus(CONNECTION_STATUS status, unsigned player)
{
	unsigned n;
	const int timeouts[] = {GAME_TICKS_PER_SEC * 10, GAME_TICKS_PER_SEC * 10, GAME_TICKS_PER_SEC, GAME_TICKS_PER_SEC / 6};
	ASSERT(ARRAY_SIZE(timeouts) == CONNECTIONSTATUS_NORMAL, "Connection status timeout array too small.");

	if (player == NET_ALL_PLAYERS)
	{
		for (n = 0; n < MAX_CONNECTED_PLAYERS; ++n)
		{
			NETsetPlayerConnectionStatus(status, n);
		}
		return;
	}
	if (status == CONNECTIONSTATUS_NORMAL)
	{
		for (n = 0; n < CONNECTIONSTATUS_NORMAL; ++n)
		{
			NET_PlayerConnectionStatus[n][player] = 0;
		}
		return;
	}

	NET_PlayerConnectionStatus[status][player] = realTime + timeouts[status];
}

bool NETcheckPlayerConnectionStatus(CONNECTION_STATUS status, unsigned player)
{
	unsigned n;

	if (player == NET_ALL_PLAYERS)
	{
		for (n = 0; n < MAX_CONNECTED_PLAYERS; ++n)
		{
			if (NETcheckPlayerConnectionStatus(status, n))
			{
				return true;
			}
		}
		return false;
	}
	if (status == CONNECTIONSTATUS_NORMAL)
	{
		for (n = 0; n < CONNECTIONSTATUS_NORMAL; ++n)
		{
			if (NETcheckPlayerConnectionStatus((CONNECTION_STATUS)n, player))
			{
				return true;
			}
		}
		return false;
	}

	return realTime < NET_PlayerConnectionStatus[status][player];
}

const char *messageTypeToString(unsigned messageType_)
{
	MESSAGE_TYPES messageType = (MESSAGE_TYPES)messageType_;  // Cast to enum, so switch gives a warning if new message types are added without updating the switch.

	switch (messageType)
	{
	// Search:  ^\s*([\w_]+).*
	// Replace: case \1:                             return "\1";
	// Search:  (case ...............................) *(return "[\w_]+";)
	// Replace: \t\t\1\2

	// Net-related messages.
	case NET_MIN_TYPE:                  return "NET_MIN_TYPE";
	case NET_PING:                      return "NET_PING";
	case NET_PLAYER_STATS:              return "NET_PLAYER_STATS";
	case NET_TEXTMSG:                   return "NET_TEXTMSG";
	case NET_PLAYERRESPONDING:          return "NET_PLAYERRESPONDING";
	case NET_OPTIONS:                   return "NET_OPTIONS";
	case NET_KICK:                      return "NET_KICK";
	case NET_FIREUP:                    return "NET_FIREUP";
	case NET_COLOURREQUEST:             return "NET_COLOURREQUEST";
	case NET_FACTIONREQUEST:            return "NET_FACTIONREQUEST";
	case NET_AITEXTMSG:                 return "NET_AITEXTMSG";
	case NET_BEACONMSG:                 return "NET_BEACONMSG";
	case NET_TEAMREQUEST:               return "NET_TEAMREQUEST";
	case NET_JOIN:                      return "NET_JOIN";
	case NET_ACCEPTED:                  return "NET_ACCEPTED";
	case NET_PLAYER_INFO:               return "NET_PLAYER_INFO";
	case NET_PLAYER_JOINED:             return "NET_PLAYER_JOINED";
	case NET_PLAYER_LEAVING:            return "NET_PLAYER_LEAVING";
	case NET_PLAYER_DROPPED:            return "NET_PLAYER_DROPPED";
	case NET_GAME_FLAGS:                return "NET_GAME_FLAGS";
	case NET_READY_REQUEST:             return "NET_READY_REQUEST";
	case NET_REJECTED:                  return "NET_REJECTED";
	case NET_POSITIONREQUEST:           return "NET_POSITIONREQUEST";
	case NET_DATA_CHECK:                return "NET_DATA_CHECK";
	case NET_HOST_DROPPED:              return "NET_HOST_DROPPED";
	case NET_SEND_TO_PLAYER:            return "NET_SEND_TO_PLAYER";
	case NET_SHARE_GAME_QUEUE:          return "NET_SHARE_GAME_QUEUE";
	case NET_FILE_REQUESTED:            return "NET_FILE_REQUESTED";
	case NET_FILE_CANCELLED:            return "NET_FILE_CANCELLED";
	case NET_FILE_PAYLOAD:              return "NET_FILE_PAYLOAD";
	case NET_DEBUG_SYNC:                return "NET_DEBUG_SYNC";
	case NET_VOTE:                      return "NET_VOTE";
	case NET_VOTE_REQUEST:              return "NET_VOTE_REQUEST";
	case NET_SPECTEXTMSG:				return "NET_SPECTEXTMSG";
	case NET_PLAYERNAME_CHANGEREQUEST:  return "NET_PLAYERNAME_CHANGEREQUEST";
	case NET_PLAYER_SLOTTYPE_REQUEST:   return "NET_PLAYER_SLOTTYPE_REQUEST";
	case NET_PLAYER_SWAP_INDEX:         return "NET_PLAYER_SWAP_INDEX";
	case NET_PLAYER_SWAP_INDEX_ACK:     return "NET_PLAYER_SWAP_INDEX_ACK";
	case NET_DATA_CHECK2:               return "NET_DATA_CHECK2";
	case NET_SECURED_NET_MESSAGE:		return "NET_SECURED_NET_MESSAGE";
	case NET_TEAM_STRATEGY:				return "NET_TEAM_STRATEGY";
	case NET_QUICK_CHAT_MSG:			return "NET_QUICK_CHAT_MSG";
	case NET_HOST_CONFIG:				return "NET_HOST_CONFIG";
	case NET_MAX_TYPE:                  return "NET_MAX_TYPE";

	// Game-state-related messages, must be processed by all clients at the same game time.
	case GAME_MIN_TYPE:                 return "GAME_MIN_TYPE";
	case GAME_DROIDINFO:                return "GAME_DROIDINFO";
	case GAME_STRUCTUREINFO:            return "GAME_STRUCTUREINFO";
	case GAME_RESEARCHSTATUS:           return "GAME_RESEARCHSTATUS";
	case GAME_TEMPLATE:                 return "GAME_TEMPLATE";
	case GAME_TEMPLATEDEST:             return "GAME_TEMPLATEDEST";
	case GAME_ALLIANCE:                 return "GAME_ALLIANCE";
	case GAME_GIFT:                     return "GAME_GIFT";
	case GAME_LASSAT:                   return "GAME_LASSAT";
	case GAME_GAME_TIME:                return "GAME_GAME_TIME";
	case GAME_PLAYER_LEFT:              return "GAME_PLAYER_LEFT";
	case GAME_DROIDDISEMBARK:           return "GAME_DROIDDISEMBARK";
	case GAME_SYNC_REQUEST:             return "GAME_SYNC_REQUEST";

	// The following messages are used for debug mode.
	case GAME_DEBUG_MODE:               return "GAME_DEBUG_MODE";
	case GAME_DEBUG_ADD_DROID:          return "GAME_DEBUG_ADD_DROID";
	case GAME_DEBUG_ADD_STRUCTURE:      return "GAME_DEBUG_ADD_STRUCTURE";
	case GAME_DEBUG_ADD_FEATURE:        return "GAME_DEBUG_ADD_FEATURE";
	case GAME_DEBUG_REMOVE_DROID:       return "GAME_DEBUG_REMOVE_DROID";
	case GAME_DEBUG_REMOVE_STRUCTURE:   return "GAME_DEBUG_REMOVE_STRUCTURE";
	case GAME_DEBUG_REMOVE_FEATURE:     return "GAME_DEBUG_REMOVE_FEATURE";
	case GAME_DEBUG_FINISH_RESEARCH:    return "GAME_DEBUG_FINISH_RESEARCH";
	// End of redundant messages.
	case GAME_MAX_TYPE:                 return "GAME_MAX_TYPE";

	// The following messages are used for playing back replays.
	case REPLAY_ENDED:                  return "REPLAY_ENDED";
	// End of replay messages.
	}
	return "(UNUSED)";
}
