/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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

// Lobby Connection errors

typedef enum
{
	ERROR_NOERROR,
	ERROR_CONNECTION,
	ERROR_FULL,
	ERROR_CHEAT,
	ERROR_KICKED,
	ERROR_WRONGVERSION,
	ERROR_WRONGPASSWORD				// NOTE WRONG_PASSWORD results in conflict
} LOBBY_ERROR_TYPES;


typedef enum
{
	NET_DROID,				//0 a new droid
	NET_DROIDINFO,			//1 update a droid order.
	NET_DROIDDEST,			//2 issue a droid destruction
	NET_DROIDMOVE,			//3 move a droid, don't change anything else though..
	NET_GROUPORDER,			//4 order a group of droids.
	NET_TEMPLATE,			//5 a new template
	NET_TEMPLATEDEST,		//6 remove template
	NET_FEATUREDEST,		//7 destroy a game feature.
	NET_PING,				//8 ping players.
	NET_CHECK_DROID,		//9 check & update bot position and damage.
	NET_CHECK_STRUCT,		//10 check & update struct damage.
	NET_CHECK_POWER,		//11 power levels for a player.
	NET_PLAYER_STATS,		//12 player stats: HACK-NOTE: lib/netplay/netplay.c depends on this being 12
	NET_BUILD,				//13 build a new structure
	NET_STRUCTDEST,			//14 specify a strucutre to destroy
	NET_BUILDFINISHED,		//15 a building is complete.
	NET_RESEARCH,			//16 Research has been completed.
	NET_TEXTMSG,			//17 A simple text message between machines.
	NET_UNUSED_18,			//18 
	NET_UNUSED_19,			//19
	NET_PLAYERCOMPLETE,		//20 All Setup information about player x has been sent
	NET_UNUSED_21,			//
	NET_STRUCT,				//22 a complete structure
	NET_UNUSED_23,			//
	NET_FEATURES,			//24 information regarding features.
	NET_PLAYERRESPONDING,	//25 computer that sent this is now playing warzone!
	NET_OPTIONS,			//26 welcome a player to a game.
	NET_KICK,				//27 kick a player .
	NET_SECONDARY,			//28 set a droids secondary order
	NET_FIREUP,				//29 campaign game has started, we can go too.. Shortcut message, not to be used in dmatch.
	NET_ALLIANCE,			//30 alliance data.
	NET_GIFT,				//31 a luvly gift between players.
	NET_DEMOLISH,			//32 a demolish is complete.
	NET_COLOURREQUEST,		//33 player requests a colour change.
	NET_ARTIFACTS,			//34 artifacts randomly placed.
	NET_DMATCHWIN,			//35 winner of a deathmatch. NOTUSED
	NET_SCORESUBMIT,		//36 submission of scores to host.
	NET_DESTROYXTRA,		//37 destroy droid with destroyer intact.
	NET_VTOL,				//38 vtol rearmed
	NET_UNUSED_39,			//
	NET_WHITEBOARD,			//40 whiteboard.
	NET_SECONDARY_ALL,		//41 complete secondary order.
	NET_DROIDEMBARK,		//42 droid embarked on a Transporter
	NET_DROIDDISEMBARK,		//43 droid disembarked from a Transporter
	NET_RESEARCHSTATUS,		//44 research state.
	NET_LASSAT,				//45 lassat firing.
	NET_REQUESTMAP,			//46 dont have map, please send it.
	NET_AITEXTMSG,			//47 chat between AIs
	NET_TEAMS_ON,			//48 locked teams mode
	NET_BEACONMSG,			//49 place beacon
	NET_SET_TEAMS,			//50 set locked teams
	NET_TEAMREQUEST,		//51 request team membership
	NET_JOIN,				//52 join a game
	NET_ACCEPTED,			//53 accepted into game
	NET_PLAYER_INFO,		//54 basic player info
	NET_PLAYER_JOINED,		//55 notice about player joining
	NET_PLAYER_LEAVING,		//56 A player is leaving, (nicely)
	NET_PLAYER_DROPPED,		//57 notice about player dropped / disconnected
	NET_GAME_FLAGS,			//58 game flags
	NET_READY_REQUEST,		//59 player ready to start an mp game
							//60 to prevent conflict
	NET_VERSION_CHECK = 61,	//61 version check
	NET_REQUEST_VERSION,	//62 Host requests version check
	NET_REQUEST_PASSWORD,	//63 Host requests password
	NET_PASSWORD_CHECK,		//64 password check
	NET_POSITIONREQUEST,	//65 position in GUI player list
	NUM_GAME_PACKETS		//   *MUST* be last.
} MESSAGE_TYPES;

// Constants
// @NOTE / FIXME: We need a way to detect what should happen if the msg buffer exceeds this.
#define MaxMsgSize		8192		// max size of a message in bytes.
#define	StringSize		64			// size of strings used.
#define MaxGames		12			// max number of concurrently playable games to allow.
#define extra_string_size	239		// extra 255 char for future use
#define modlist_string_size	255		// For a concatenated list of mods

#define SESSION_JOINDISABLED	1

typedef struct {					//Available game storage... JUST FOR REFERENCE!
	int32_t dwSize;
	int32_t dwFlags;
	char host[40];	// host's ip address (can fit a full IPv4 and IPv6 address + terminating NUL)
	int32_t dwMaxPlayers;
	int32_t dwCurrentPlayers;
	int32_t dwUserFlags[4];
} SESSIONDESC;

/**
 * @note when changing this structure, NETsendGAMESTRUCT, NETrecvGAMESTRUCT and
 *       the lobby server should be changed accordingly.
 */
typedef struct
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
	char		versionstring[StringSize];		// 
	char		modlist[modlist_string_size];	// ???
	uint32_t	game_version_major;				// 
	uint32_t	game_version_minor;				// 
	uint32_t	privateGame;					// if true, it is a private game
	uint32_t	pureGame;						// NO mods allowed if true
	uint32_t	Mods;							// number of concatenated mods?
	// Game ID, used on the lobby server to link games with multiple address families to eachother
	uint32_t	gameId;
	uint32_t	future2;						// for future use
	uint32_t	future3;						// for future use
	uint32_t	future4;						// for future use
} GAMESTRUCT;

// ////////////////////////////////////////////////////////////////////////
// Message information. ie. the packets sent between machines.

#define NET_ALL_PLAYERS 255
#define NET_HOST_ONLY 0

typedef struct {
	uint16_t	size;				// used size of body
	uint8_t		type;				// type of packet
	uint8_t		destination;		// player to send to, or NET_ALL_PLAYERS
	uint8_t		source;				// player it is sent from
	char 		body[MaxMsgSize];	// msg buffer
	BOOL		status;				// If the packet compiled or not (this is _not_ sent!)
} NETMSG;

#define		FILEMSG			254		// a file packet

// ////////////////////////////////////////////////////////////////////////
// Player information. Filled when players join, never re-ordered. selectedPlayer global points to 
// currently controlled player. This array is indexed by GUI slots in pregame.
typedef struct
{
	char		name[StringSize];	///< Player name
	int32_t		position;		///< Map starting position
	int32_t		colour;			///< Which colour slot this player is using
	BOOL		allocated;		///< Active?
	uint32_t	heartattacktime;	///< Time cardiac arrest started
	BOOL		heartbeat;		///< If we are still alive or not
	BOOL		kick;			///< If we should kick them
	int32_t		connection;		///< Index into connection list
	int32_t		team;			///< Which team we are on
	BOOL		ready;			///< player ready to start?
	uint32_t	versionCheckTime;	///< Time when check sent. Uses 0xffffffff for nothing sent yet
	BOOL		playerVersionFlag;	///< We kick on false
} PLAYER;

// ////////////////////////////////////////////////////////////////////////
// all the luvly Netplay info....
typedef struct {
	GAMESTRUCT	games[MaxGames];	///< The collection of games
	PLAYER		players[MAX_PLAYERS];	///< The array of players.
	uint32_t	playercount;		///< Number of players in game.
	uint32_t	hostPlayer;		///< Index of host in player array
	uint32_t	bComms;			///< Actually do the comms?
	BOOL		isHost;			///< True if we are hosting the game
	int32_t		maxPlayers;		///< Maximum number of players in this game
	char gamePassword[StringSize];		//
	bool GamePassworded;				// if we have a password or not.
	bool ShowedMOTD;					// only want to show this once
	char MOTDbuffer[255];				// buffer for MOTD
	char* MOTD;
} NETPLAY;

// ////////////////////////////////////////////////////////////////////////
// variables

extern NETPLAY				NetPlay;
extern NETMSG NetMsg;

// ////////////////////////////////////////////////////////////////////////
// functions available to you.
extern int   NETinit(BOOL bFirstCall);				// init
extern BOOL   NETsend(NETMSG *msg, UDWORD player);	// send to player
extern BOOL   NETbcast(NETMSG *msg);				// broadcast to everyone
extern BOOL   NETrecv(uint8_t *type);				// recv a message if possible

extern UBYTE   NETsendFile(BOOL newFile, char *fileName, UDWORD player);	// send file chunk.
extern UBYTE   NETrecvFile(void);			// recv file chunk

extern int NETclose(void);					// close current game
extern int NETshutdown(void);					// leave the game in play.

extern UDWORD	NETgetBytesSent(void);				// return bytes sent/recv.  call regularly for good results
extern UDWORD	NETgetPacketsSent(void);			// return packets sent/recv.  call regularly for good results
extern UDWORD	NETgetBytesRecvd(void);				// return bytes sent/recv.  call regularly for good results
extern UDWORD	NETgetPacketsRecvd(void);			// return packets sent/recv.  call regularly for good results
extern UDWORD	NETgetRecentBytesSent(void);		// more immediate functions.
extern UDWORD	NETgetRecentPacketsSent(void);
extern UDWORD	NETgetRecentBytesRecvd(void);

// from netjoin.c
extern SDWORD	NETgetGameFlags(UDWORD flag);			// return one of the four flags(dword) about the game.
extern int32_t	NETgetGameFlagsUnjoined(unsigned int gameid, unsigned int flag);	// return one of the four flags(dword) about the game.
extern BOOL	NETsetGameFlags(UDWORD flag, SDWORD value);	// set game flag(1-4) to value.
extern BOOL	NEThaltJoining(void);				// stop new players joining this game
extern BOOL	NETfindGame(void);		// find games being played(uses GAME_GUID);
extern BOOL	NETjoinGame(UDWORD gameNumber, const char* playername);			// join game given with playername
extern BOOL	NEThostGame(const char* SessionName, const char* PlayerName,// host a game
			    SDWORD one, SDWORD two, SDWORD three, SDWORD four, UDWORD plyrs);
extern BOOL	NETchangePlayerName(UDWORD player, char *newName);// change a players name.

#include "netlog.h"

extern void NETsetMasterserverName(const char* hostname);
extern const char* NETgetMasterserverName(void);
extern void NETsetMasterserverPort(unsigned int port);
extern unsigned int NETgetMasterserverPort(void);
extern void NETsetGameserverPort(unsigned int port);
extern unsigned int NETgetGameserverPort(void);

extern BOOL NETsetupTCPIP(const char *machine);
extern void NETsetGamePassword(const char *password);
extern void NETBroadcastPlayerInfo(uint32_t index);
extern void NETCheckVersion(uint32_t player);
extern void NET_InitPlayers(void);

#endif
