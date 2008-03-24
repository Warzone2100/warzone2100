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
 * Netplay.h
 *
 * Alex Lee sep97.
 */

#ifndef _netplay_h
#define _netplay_h

#include "nettypes.h"

typedef enum
{
	NET_DROID,			//0 a new droid
	NET_DROIDINFO,			//1 update a droid order.
	NET_DROIDDEST,			//2 issue a droid destruction
	NET_DROIDMOVE,			//3 move a droid, don't change anything else though..
	NET_GROUPORDER,			//4 order a group of droids.
	NET_TEMPLATE,			//5 a new template
	NET_TEMPLATEDEST,		//6 remove template
	NET_FEATUREDEST,		//7 destroy a game feature.
	NET_PING,			//8 ping players.
	NET_CHECK_DROID,		//9 check & update bot position and damage.
	NET_CHECK_STRUCT,		//10 check & update struct damage.
	NET_CHECK_POWER,		//11 power levels for a player.
	NET_PLAYER_STATS,		//12 player stats: HACK-NOTE: lib/netplay/netplay.c depends on this being 12
	NET_BUILD,			//13 build a new structure
	NET_STRUCTDEST,			//14 specify a strucutre to destroy
	NET_BUILDFINISHED,		//15 a building is complete.
	NET_RESEARCH,			//16 Research has been completed.
	NET_TEXTMSG,			//17 A simple text message between machines.
	NET_LEAVING,			//18 A player is leaving, (nicely)
	NET_UNUSED_19,
	NET_PLAYERCOMPLETE,		//20 All Setup information about player x has been sent
	NET_UNUSED_21,
	NET_STRUCT,			//22 a complete structure
	NET_UNUSED_23,
	NET_FEATURES,			//24 information regarding features.
	NET_PLAYERRESPONDING,		//25 computer that sent this is now playing warzone!
	NET_OPTIONS,			//26 welcome a player to a game.
	NET_KICK,			//27 kick a player .
	NET_SECONDARY,			//28 set a droids secondary order
	NET_FIREUP,			//29 campaign game has started, we can go too.. Shortcut message, not to be used in dmatch.
	NET_ALLIANCE,			//30 alliance data.
	NET_GIFT,			//31 a luvly gift between players.
	NET_DEMOLISH,			//32 a demolish is complete.
	NET_COLOURREQUEST,		//33 player requests a colour change.
	NET_ARTIFACTS,			//34 artifacts randomly placed.
	NET_DMATCHWIN,			//35 winner of a deathmatch. NOTUSED
	NET_SCORESUBMIT,		//36 submission of scores to host.
	NET_DESTROYXTRA,		//37 destroy droid with destroyer intact.
	NET_VTOL,			//38 vtol rearmed
	NET_UNUSED_39,
	NET_WHITEBOARD,			//40 whiteboard.
	NET_SECONDARY_ALL,		//41 complete secondary order.
	NET_DROIDEMBARK,		//42 droid embarked on a Transporter
	NET_DROIDDISEMBARK,		//43 droid disembarked from a Transporter
	NET_RESEARCHSTATUS,		//44 research state.
	NET_LASSAT,			//45 lassat firing.
	NET_REQUESTMAP,			//46 dont have map, please send it.
	NET_AITEXTMSG,			//47 chat between AIs
	NET_TEAMS_ON,			//48 locked teams mode
	NET_BEACONMSG,			//49 place beacon
	NET_SET_TEAMS,			//50 set locked teams
	NET_TEAMREQUEST,		//51 request team membership
	NET_JOIN,			//52 join a game
	NET_ACCEPTED,			//53 accepted into game
	NET_PLAYER_INFO,		//54 basic player info
	NET_PLAYER_JOINED,		//55 notice about player joining
	NET_PLAYER_LEFT,		//56 notice about player leaving
	NET_GAME_FLAGS,			//57 game flags
	NUM_GAME_PACKETS
} MESSAGE_TYPES;

// Constants
#define MaxMsgSize		8000			// max size of a message in bytes.
#define	StringSize		64			// size of strings used.
#define MaxGames		12			// max number of concurrently playable games to allow.

#define SESSION_JOINDISABLED	1

typedef struct {					//Available game storage... JUST FOR REFERENCE!
	int32_t dwSize;
	int32_t dwFlags;
	char host[16];	// host ip address
	int32_t dwMaxPlayers;
	int32_t dwCurrentPlayers;
	int32_t dwUser1;
	int32_t dwUser2;
	int32_t dwUser3;
	int32_t dwUser4;
} SESSIONDESC;

// Games Storage Structures
typedef struct {
	char		name[StringSize];
	SESSIONDESC	desc;
} GAMESTRUCT;

// ////////////////////////////////////////////////////////////////////////
// Message information. ie. the packets sent between machines.

#define NET_ALL_PLAYERS 255

typedef struct {
	uint16_t	size;			// used size of body
	uint8_t		type;			// type of packet
	uint8_t		destination;		// player to send to, or NET_ALL_PLAYERS
	uint8_t		source;			// player it is sent from
	char 		body[MaxMsgSize];
	BOOL		status;			// If the packet compiled or not (this is _not_ sent!)
} NETMSG;

#define		FILEMSG			254	// a file packet

// ////////////////////////////////////////////////////////////////////////
// Player information. Update using NETplayerinfo
typedef struct {
	uint32_t dpid;
	char name[StringSize];

	// These are actually boolean values so uint8_t would suffice just as well.
	// The problem is however that these where previously declared as BOOL,
	// which is typedef'd as int, which on most platforms is equal to uint32_t.
	uint32_t bHost;
} PLAYER;

// ////////////////////////////////////////////////////////////////////////
// all the luvly Netplay info....
typedef struct {
	GAMESTRUCT	games[MaxGames];		// the collection of games
	PLAYER		players[MAX_PLAYERS];	// the array of players.
	uint32_t        playercount;			// number of players in game.
	uint32_t        dpidPlayer;			// ID of player created

	// booleans
	uint32_t        bComms;				// actually do the comms?
	uint32_t        bHost;				// true if we are hosting the session
} NETPLAY;

// ////////////////////////////////////////////////////////////////////////
// variables

extern NETPLAY				NetPlay;

extern NETMSG NetMsg;

// ////////////////////////////////////////////////////////////////////////
// functions available to you.
extern BOOL   NETinit(BOOL bFirstCall);				//init(guid can be NULL)
extern BOOL   NETsend(NETMSG *msg, UDWORD player);		// send to player
extern BOOL   NETbcast(NETMSG *msg);				// broadcast to everyone
extern BOOL   NETrecv(uint8_t *type);				// recv a message if possible

extern UBYTE   NETsendFile(BOOL newFile, char *fileName, UDWORD player);	// send file chunk.
extern UBYTE   NETrecvFile(void);			// recv file chunk

extern BOOL NETclose	(void);					// close current game
extern BOOL NETshutdown(void);				// leave the game in play.

extern UDWORD  NETgetBytesSent(void);				// return bytes sent/recv.  call regularly for good results
extern UDWORD  NETgetPacketsSent(void);				// return packets sent/recv.  call regularly for good results
extern UDWORD	NETgetBytesRecvd(void);				// return bytes sent/recv.  call regularly for good results
extern UDWORD	NETgetPacketsRecvd(void);			// return packets sent/recv.  call regularly for good results
extern UDWORD	NETgetRecentBytesSent(void);			// more immediate functions.
extern UDWORD	NETgetRecentPacketsSent(void);
extern UDWORD	NETgetRecentBytesRecvd(void);

// from netjoin.c
extern SDWORD	NETgetGameFlags(UDWORD flag);			// return one of the four flags(dword) about the game.
extern SDWORD	NETgetGameFlagsUnjoined(UDWORD gameid, UDWORD flag);	// return one of the four flags(dword) about the game.
extern BOOL	NETsetGameFlags(UDWORD flag, SDWORD value);	// set game flag(1-4) to value.
extern BOOL	NEThaltJoining(void);				// stop new players joining this game
extern BOOL	NETfindGame(void);		// find games being played(uses GAME_GUID);
extern BOOL	NETjoinGame(UDWORD gameNumber, const char* playername);			// join game given with playername
extern BOOL	NEThostGame(const char* SessionName, const char* PlayerName,// host a game
			    SDWORD one, SDWORD two, SDWORD three, SDWORD four, UDWORD plyrs);

//from netusers.c
extern UDWORD	NETplayerInfo(void);		// count players in this game.
extern BOOL	NETchangePlayerName(UDWORD dpid, char *newName);// change a players name.

#include "netlog.h"

extern void NETsetMasterserverName(const char* hostname);
extern void NETsetMasterserverPort(unsigned int port);
extern void NETsetGameserverPort(unsigned int port);

extern BOOL NETsetupTCPIP(const char *machine);

#endif
