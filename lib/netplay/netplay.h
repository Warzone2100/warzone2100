/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
 * Netplay.h
 *
 * Alex Lee sep97.
 */

#ifndef _netplay_h
#define _netplay_h

#include "nettypes.h"
#include <physfs.h>

// Lobby Connection errors

enum LOBBY_ERROR_TYPES
{
	ERROR_NOERROR,
	ERROR_CONNECTION,
	ERROR_FULL,
	ERROR_INVALID,
	ERROR_KICKED,
	ERROR_WRONGVERSION,
	ERROR_WRONGPASSWORD,
	ERROR_HOSTDROPPED,
	ERROR_WRONGDATA,
	ERROR_UNKNOWNFILEISSUE
};

enum CONNECTION_STATUS
{
	CONNECTIONSTATUS_PLAYER_DROPPED,
	CONNECTIONSTATUS_PLAYER_LEAVING,
	CONNECTIONSTATUS_DESYNC,
	CONNECTIONSTATUS_WAITING_FOR_PLAYER,

	CONNECTIONSTATUS_NORMAL
};

enum MESSAGE_TYPES
{
	// Net-related messages.
	NET_MIN_TYPE = 33,              ///< Minimum-1 valid NET_ type, *MUST* be first.
	NET_PING,                       ///< ping players.
	NET_PLAYER_STATS,               ///< player stats
	NET_TEXTMSG,                    ///< A simple text message between machines.
	NET_PLAYERRESPONDING,           ///< computer that sent this is now playing warzone!
	NET_OPTIONS,                    ///< welcome a player to a game.
	NET_KICK,                       ///< kick a player .
	NET_FIREUP,                     ///< campaign game has started, we can go too.. Shortcut message, not to be used in dmatch.
	NET_COLOURREQUEST,              ///< player requests a colour change.
	NET_AITEXTMSG,                  ///< chat between AIs
	NET_BEACONMSG,                  ///< place beacon
	NET_TEAMREQUEST,                ///< request team membership
	NET_JOIN,                       ///< join a game
	NET_ACCEPTED,                   ///< accepted into game
	NET_PLAYER_INFO,                ///< basic player info
	NET_PLAYER_JOINED,              ///< notice about player joining
	NET_PLAYER_LEAVING,             ///< A player is leaving, (nicely)
	NET_PLAYER_DROPPED,             ///< notice about player dropped / disconnected
	NET_GAME_FLAGS,                 ///< game flags
	NET_READY_REQUEST,              ///< player ready to start an mp game
	NET_REJECTED,                   ///< nope, you can't join
	NET_POSITIONREQUEST,            ///< position in GUI player list
	NET_DATA_CHECK,                 ///< Data integrity check
	NET_HOST_DROPPED,               ///< Host has dropped
	NET_SEND_TO_PLAYER,             ///< Non-host clients aren't directly connected to each other, so they talk via the host using these messages.
	NET_SHARE_GAME_QUEUE,           ///< Message contains a game message, which should be inserted into a queue.
	NET_FILE_REQUESTED,             ///< Player has requested a file (map/mod/?)
	NET_FILE_CANCELLED,             ///< Player cancelled a file request
	NET_FILE_PAYLOAD,               ///< sending file to the player that needs it
	NET_DEBUG_SYNC,                 ///< Synch error messages, so people don't have to use pastebin.
	NET_MAX_TYPE,                   ///< Maximum+1 valid NET_ type, *MUST* be last.

	// Game-state-related messages, must be processed by all clients at the same game time.
	GAME_MIN_TYPE = 111,            ///< Minimum-1 valid GAME_ type, *MUST* be first.
	GAME_DROIDINFO,                 ///< update a droid order.
	GAME_STRUCTUREINFO,             ///< Structure state.
	GAME_RESEARCHSTATUS,            ///< research state.
	GAME_TEMPLATE,                  ///< a new template
	GAME_TEMPLATEDEST,              ///< remove template
	GAME_ALLIANCE,                  ///< alliance data.
	GAME_GIFT,                      ///< a luvly gift between players.
	GAME_LASSAT,                    ///< lassat firing.
	GAME_GAME_TIME,                 ///< Game time. Used for synchronising, so that all messages are executed at the same gameTime on all clients.
	GAME_PLAYER_LEFT,               ///< Player has left or dropped.
	GAME_DROIDDISEMBARK,            ///< droid disembarked from a Transporter
	// The following messages are used for debug mode.
	GAME_DEBUG_MODE,                ///< Request enable/disable debug mode.
	GAME_DEBUG_ADD_DROID,           ///< Add droid.
	GAME_DEBUG_ADD_STRUCTURE,       ///< Add structure.
	GAME_DEBUG_ADD_FEATURE,         ///< Add feature.
	GAME_DEBUG_REMOVE_DROID,        ///< Remove droid.
	GAME_DEBUG_REMOVE_STRUCTURE,    ///< Remove structure.
	GAME_DEBUG_REMOVE_FEATURE,      ///< Remove feature.
	GAME_DEBUG_FINISH_RESEARCH,     ///< Research has been completed.
	// End of debug messages.
	GAME_MAX_TYPE                   ///< Maximum+1 valid GAME_ type, *MUST* be last.
};
//#define SYNC_FLAG (NUM_GAME_PACKETS * NUM_GAME_PACKETS)	//special flag used for logging.
#define SYNC_FLAG 0x10000000	//special flag used for logging. (Not sure what this is. Was added in trunk, NUM_GAME_PACKETS not in newnet.)

// Constants
// @NOTE / FIXME: We need a way to detect what should happen if the msg buffer exceeds this.
#define MaxMsgSize		16384		// max size of a message in bytes.
#define	StringSize		64			// size of strings used.
#define MaxGames		11			// max number of concurrently playable games to allow.
#define extra_string_size	159		// extra 199 char for future use
#define map_string_size		40
#define	hostname_string_size	40
#define modlist_string_size	255		// For a concatenated list of mods
#define password_string_size 64		// longer passwords slow down the join code

#define SESSION_JOINDISABLED	1

#define MAX_CONNECTED_PLAYERS   MAX_PLAYERS
#define MAX_TMP_SOCKETS         16

struct SESSIONDESC  //Available game storage... JUST FOR REFERENCE!
{
	int32_t dwSize;
	int32_t dwFlags;
	char host[40];	// host's ip address (can fit a full IPv4 and IPv6 address + terminating NUL)
	int32_t dwMaxPlayers;
	int32_t dwCurrentPlayers;
	int32_t dwUserFlags[4];
};

/**
 * @note when changing this structure, NETsendGAMESTRUCT, NETrecvGAMESTRUCT and
 *       the lobby server should be changed accordingly.
 */
struct GAMESTRUCT
{
	/* Version of this structure and thus the binary lobby protocol.
	 * @NOTE: <em>MUST</em> be the first item of this struct.
	 */
	uint32_t	GAMESTRUCT_VERSION;

	char		name[StringSize];
	SESSIONDESC	desc;
	// END of old GAMESTRUCT format
	// NOTE: do NOT save the following items in game.c--it will break savegames.
	char		secondaryHosts[2][40];
	char		extra[extra_string_size];		// extra string (future use)
	char		mapname[map_string_size];		// map server is hosting
	char		hostname[hostname_string_size];	// ...
	char		versionstring[StringSize];		// 
	char		modlist[modlist_string_size];	// ???
	uint32_t	game_version_major;				// 
	uint32_t	game_version_minor;				// 
	uint32_t	privateGame;					// if true, it is a private game
	uint32_t	pureGame;						// NO mods allowed if true
	uint32_t	Mods;							// number of concatenated mods?
	// Game ID, used on the lobby server to link games with multiple address families to eachother
	uint32_t	gameId;
	uint32_t	limits;							// holds limits bitmask (NO_VTOL|NO_TANKS|NO_BORGS)
	uint32_t	future3;						// for future use
	uint32_t	future4;						// for future use
};

// ////////////////////////////////////////////////////////////////////////
// Message information. ie. the packets sent between machines.

#define NET_ALL_PLAYERS 255
#define NET_HOST_ONLY 0
// the following structure is going to be used to track if we sync or not
struct SYNC_COUNTER
{
	uint16_t	kicks;
	uint16_t	joins;
	uint16_t	left;
	uint16_t	drops;
	uint16_t	cantjoin;
	uint16_t	banned;
	uint16_t	rejected;
};

struct WZFile
{
	PHYSFS_file	*pFileHandle;		// handle
	PHYSFS_sint32 fileSize_32;		// size
	int32_t		currPos;			// current position
	bool	isSending;				// sending to this player
	bool	isCancelled;			// player cancelled
	int32_t	filetype;				// future use (1=map 2=mod 3=...)
};

struct wzFileStatus
{
	int32_t player;					// the client we sent data to
	int32_t done;					// how far done we are (100= finished)
	int32_t byteCount;				// current byte count
};

enum wzFileEnum
{
	WZ_FILE_OK,
	ALREADY_HAVE_FILE,
	STUCK_IN_FILE_LOOP
};

enum
{
	DIFFICULTY_EASY, DIFFICULTY_MEDIUM, DIFFICULTY_HARD, DIFFICULTY_INSANE
};

// ////////////////////////////////////////////////////////////////////////
// Player information. Filled when players join, never re-ordered. selectedPlayer global points to 
// currently controlled player.
struct PLAYER
{
	char		name[StringSize];	///< Player name
	int32_t		position;		///< Map starting position
	int32_t		colour;			///< Which colour slot this player is using
	bool		allocated;		///< Allocated as a human player
	uint32_t	heartattacktime;	///< Time cardiac arrest started
	bool		heartbeat;		///< If we are still alive or not
	bool		kick;			///< If we should kick them
	int32_t		connection;		///< Index into connection list
	int32_t		team;			///< Which team we are on
	bool		ready;			///< player ready to start?
	int8_t		ai;			///< index into sorted list of AIs, zero is always default AI
	int8_t		difficulty;		///< difficulty level of AI
	bool		needFile;			///< if We need a file sent to us
	WZFile		wzFile;				///< for each player, we keep track of map progress
	char		IPtextAddress[40];	///< IP of this player
};

// ////////////////////////////////////////////////////////////////////////
// all the luvly Netplay info....
struct NETPLAY
{
	GAMESTRUCT	games[MaxGames];	///< The collection of games
	PLAYER		players[MAX_PLAYERS];	///< The array of players.
	uint32_t	playercount;		///< Number of players in game.
	uint32_t	hostPlayer;		///< Index of host in player array
	uint32_t	bComms;			///< Actually do the comms?
	bool		isHost;			///< True if we are hosting the game
	bool		isUPNP;					// if we want the UPnP detection routines to run
	bool		isHostAlive;	/// if the host is still alive
	PHYSFS_file	*pMapFileHandle;
	char gamePassword[password_string_size];		//
	bool GamePassworded;				// if we have a password or not.
	bool ShowedMOTD;					// only want to show this once
	char MOTDbuffer[255];				// buffer for MOTD
	char* MOTD;
};

struct PLAYER_IP
{
	char	pname[40];
	char	IPAddress[40];
};
#define MAX_BANS 255
// ////////////////////////////////////////////////////////////////////////
// variables

extern NETPLAY				NetPlay;
extern SYNC_COUNTER sync_counter;
extern PLAYER_IP	*IPlist;
// update flags
extern bool netPlayersUpdated;
extern int mapDownloadProgress;
extern char iptoconnect[PATH_MAX]; // holds IP/hostname from command line

// ////////////////////////////////////////////////////////////////////////
// functions available to you.
extern int   NETinit(bool bFirstCall);				// init
bool NETsend(NETQUEUE queue, NetMessage const *message);                 ///< send to player, or broadcast if player == NET_ALL_PLAYERS.
extern bool NETrecvNet(NETQUEUE *queue, uint8_t *type);                  ///< recv a message from the net queues if possible.
extern bool NETrecvGame(NETQUEUE *queue, uint8_t *type);                 ///< recv a message from the game queues which is sceduled to execute by time, if possible.
void NETflush(void);                                                     ///< Flushes any data stuck in compression buffers.

extern UBYTE   NETsendFile(char *fileName, UDWORD player);	// send file chunk.
extern UBYTE   NETrecvFile(NETQUEUE queue);                     // recv file chunk

extern int NETclose(void);					// close current game
extern int NETshutdown(void);					// leave the game in play.

extern void NETaddRedirects(void);
extern void NETremRedirects(void);
extern void NETdiscoverUPnPDevices(void);

extern UDWORD	NETgetBytesSent(void);				// return bytes sent/recv.  call regularly for good results
extern UDWORD	NETgetPacketsSent(void);			// return packets sent/recv.  call regularly for good results
extern UDWORD	NETgetBytesRecvd(void);				// return bytes sent/recv.  call regularly for good results
extern UDWORD	NETgetPacketsRecvd(void);			// return packets sent/recv.  call regularly for good results
extern UDWORD	NETgetRecentBytesSent(void);		// more immediate functions.
extern UDWORD	NETgetRecentPacketsSent(void);
extern UDWORD	NETgetRecentBytesRecvd(void);

extern void NETplayerKicked(UDWORD index);			// Cleanup after player has been kicked

// from netjoin.c
extern SDWORD	NETgetGameFlags(UDWORD flag);			// return one of the four flags(dword) about the game.
extern int32_t	NETgetGameFlagsUnjoined(unsigned int gameid, unsigned int flag);	// return one of the four flags(dword) about the game.
extern bool	NETsetGameFlags(UDWORD flag, SDWORD value);	// set game flag(1-4) to value.
extern bool	NEThaltJoining(void);				// stop new players joining this game
extern bool	NETfindGame(void);		// find games being played(uses GAME_GUID);
extern bool	NETjoinGame(const char* host, uint32_t port, const char* playername); // join game given with playername
extern bool	NEThostGame(const char* SessionName, const char* PlayerName,// host a game
			    SDWORD one, SDWORD two, SDWORD three, SDWORD four, UDWORD plyrs);
extern bool	NETchangePlayerName(UDWORD player, char *newName);// change a players name.
void            NETfixDuplicatePlayerNames(void);  // Change a player's name automatically, if there are duplicates.

#include "netlog.h"

extern void NETsetMasterserverName(const char* hostname);
extern const char* NETgetMasterserverName(void);
extern void NETsetMasterserverPort(unsigned int port);
extern unsigned int NETgetMasterserverPort(void);
extern void NETsetGameserverPort(unsigned int port);
extern unsigned int NETgetGameserverPort(void);

extern bool NETsetupTCPIP(const char *machine);
extern void NETsetGamePassword(const char *password);
extern void NETBroadcastPlayerInfo(uint32_t index);
void NETBroadcastTwoPlayerInfo(uint32_t index1, uint32_t index2);
extern bool NETisCorrectVersion(uint32_t game_version_major, uint32_t game_version_minor);
void NET_InitPlayer(int i, bool initPosition);
extern void NET_InitPlayers(void);

void NETGameLocked(bool flag);
void NETresetGamePassword(void);

void NETsetPlayerConnectionStatus(CONNECTION_STATUS status, unsigned player);    ///< Cumulative, except that CONNECTIONSTATUS_NORMAL resets.
bool NETcheckPlayerConnectionStatus(CONNECTION_STATUS status, unsigned player);  ///< True iff connection status icon hasn't expired for this player. CONNECTIONSTATUS_NORMAL means any status, NET_ALL_PLAYERS means all players.

const char *messageTypeToString(unsigned messageType);

/// Sync debugging. Only prints anything, if different players would print different things.
#define syncDebug(...) do { _syncDebug(__FUNCTION__, __VA_ARGS__); } while(0)
void _syncDebug(const char *function, const char *str, ...)
	WZ_DECL_FORMAT(printf, 2, 3);
/// Faster than syncDebug. Make sure that str is a format string that takes ints only.
void _syncDebugIntList(const char *function, const char *str, int *ints, size_t numInts);
#define syncDebugBacktrace() do { _syncDebugBacktrace(__FUNCTION__); } while(0)
void _syncDebugBacktrace(const char *function);                  ///< Adds a backtrace to syncDebug, if the platform supports it. Can be a bit slow, don't call way too often, unless desperate.

void resetSyncDebug(void);                                       ///< Resets the syncDebug, so syncDebug from a previous game doesn't cause a spurious desynch dump.
uint32_t nextDebugSync(void);                                    ///< Returns a CRC corresponding to all syncDebug() calls since the last nextDebugSync() or resetSyncDebug() call.
bool checkDebugSync(uint32_t checkGameTime, uint32_t checkCrc);  ///< Dumps all syncDebug() calls from that gameTime, if the CRC doesn't match.

#endif
