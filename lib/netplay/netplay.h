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

// ////////////////////////////////////////////////////////////////////////
// Include this file in your game to add multiplayer facilities.

// Constants
#define MaxNumberOfPlayers	8			// max number of players in a game.
#define MaxMsgSize		8000			// max size of a message in bytes.
#define	StringSize		64			// size of strings used.
#define ConnectionSize		255			// max size of a connection description.
#define MaxProtocols		12			// max number of returnable protocols.
#define MaxGames		12			// max number of concurrently playable games to allow.
//#define USE_DIRECTPLAY_PROTOCOL			// use DX6 protocol.

#define SESSION_JOINDISABLED	1

typedef struct {					//Available game storage... JUST FOR REFERENCE!
	SDWORD dwSize;
	SDWORD dwFlags;
	char host[16];	// host ip address
	SDWORD dwMaxPlayers;
	SDWORD dwCurrentPlayers;
	SDWORD dwUser1;
	SDWORD dwUser2;
	SDWORD dwUser3;
	SDWORD dwUser4;
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
	unsigned short	size;
	unsigned char	paddedBytes;		// numberofbytes appended for encryption
	unsigned char	type;
	unsigned char	destination;
	char 		body[MaxMsgSize];
} NETMSG, *LPNETMSG;

#define		ENCRYPTFLAG		100	// added to type to determine packet is encrypted.
#define		AUDIOMSG		255	// an audio packet (special message);
#define		FILEMSG			254	// a file packet

// ////////////////////////////////////////////////////////////////////////
// Player information. Update using NETplayerinfo
typedef struct {
	UDWORD dpid;
	char name[StringSize];
	BOOL bHost;				// a bool.
	BOOL bSpectator;
} PLAYER;

// ////////////////////////////////////////////////////////////////////////
// all the luvly Netplay info....
typedef struct {
	GAMESTRUCT	games[MaxGames];		// the collection of games
	PLAYER		players[MaxNumberOfPlayers];	// the array of players.
	UDWORD		playercount;			// number of players in game.

	UDWORD		dpidPlayer;			// ID of player created

	BOOL		bComms;				// actually do the comms?
	BOOL		bHost;				// TRUE if we are hosting the session
	BOOL		bLobbyLaunched;			// true if app launched by a lobby
	BOOL		bSpectator;			// true if just spectating

	BOOL		bEncryptAllPackets;		// set to true to encrypt all communications.
	UDWORD		cryptKey[4];			// 4*32 bit encryption key

	BOOL		bCaptureInUse;			// true if someone is speaking.
	BOOL		bAllowCaptureRecord;		// true if speech can be recorded.
	BOOL		bAllowCapturePlay;		// true if speech can be played.
} NETPLAY, *LPNETPLAY;

// ////////////////////////////////////////////////////////////////////////
// variables

extern NETPLAY				NetPlay;
extern LPNETPLAY			lpNetPlay;

// ////////////////////////////////////////////////////////////////////////
// functions available to you.
extern BOOL   NETinit(BOOL bFirstCall);				//init(guid can be NULL)
extern BOOL   NETfindProtocol(BOOL Lob);			//put connections in Protocols[] (Lobbies optional)
extern BOOL   NETselectProtocol(void * lpConnection);		//choose one.
extern BOOL   NETsend(NETMSG *msg, UDWORD player, BOOL guarantee);// send to player, possibly guaranteed
extern BOOL   NETbcast(NETMSG *msg,BOOL guarantee);		// broadcast to everyone, possibly guaranteed
extern BOOL   NETrecv(NETMSG *msg);				// recv a message if possible

extern UBYTE   NETsendFile(BOOL newFile, const char *fileName, UDWORD player);	// send file chunk.
extern UBYTE   NETrecvFile(NETMSG *pMsg);			// recv file chunk

extern BOOL NETclose	(void);					// close current game
extern BOOL NETshutdown(void);				// leave the game in play.

extern UDWORD  NETgetBytesSent(void);				// return bytes sent/recv.  call regularly for good results
extern UDWORD  NETgetPacketsSent(void);				// return packets sent/recv.  call regularly for good results
extern UDWORD	NETgetBytesRecvd(void);				// return bytes sent/recv.  call regularly for good results
extern UDWORD	NETgetPacketsRecvd(void);			// return packets sent/recv.  call regularly for good results
extern UDWORD	NETgetRecentBytesSent(void);			// more immediate functions.
extern UDWORD	NETgetRecentPacketsSent(void);
extern UDWORD	NETgetRecentBytesRecvd(void);
extern UDWORD	NETgetRecentPacketsRecvd(void);

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
extern BOOL	NETuseNetwork(BOOL val);	// TURN on/off networking.
extern UDWORD	NETplayerInfo(void);		// count players in this game.
extern BOOL	NETchangePlayerName(UDWORD dpid, char *newName);// change a players name.
extern BOOL	NETgetLocalPlayerData(UDWORD dpid, void *pData, SDWORD *pSize);
extern BOOL	NETgetGlobalPlayerData(UDWORD dpid, void *pData, SDWORD *pSize);
extern BOOL	NETsetLocalPlayerData(UDWORD dpid, void *pData, SDWORD size);
extern BOOL	NETsetGlobalPlayerData(UDWORD dpid, void *pData, SDWORD size);

#include "netlog.h"

// encryption
extern BOOL	NETsetKey(UDWORD c1,UDWORD c2,UDWORD c3, UDWORD c4);
extern NETMSG*	NETmanglePacket(NETMSG *msg);
extern void	NETunmanglePacket(NETMSG *msg);
extern BOOL	NETmangleData(UDWORD *input, UDWORD *result, UDWORD dataSize);
extern BOOL	NETunmangleData(UDWORD *input, UDWORD *result, UDWORD dataSize);
extern UBYTE	NEThashVal(UDWORD value);
extern UDWORD	NEThashBuffer(char *pData, UDWORD size);

extern WZ_DECL_DEPRECATED BOOL NETcheckRegistryEntries	(char *name,char *guid);
extern WZ_DECL_DEPRECATED BOOL NETsetRegistryEntries	(char *name,char *guid,char *file,char *cline,char *path,char *cdir);
extern WZ_DECL_DEPRECATED BOOL NETconnectToLobby		(LPNETPLAY lpNetPlay);
extern void NETsetMasterserverName(const char* hostname);
extern void NETsetMasterserverPort(unsigned int port);
extern void NETsetGameserverPort(unsigned int port);

// Some shortcuts to help you along!
/* FIXME: This is _not_ portable! Bad, Pumpkin, bad! - Per */
#define NetAdd(m,pos,thing) \
	memcpy(&(m.body[pos]),&(thing),sizeof(thing))

#define NetAddUint8(m,pos,thing) \
	*((Uint8*)(&((m).body[(pos)]))) = (thing)

#define NetAddUint16(m,pos,thing) \
	*((Uint16*)(&((m).body[(pos)]))) = (thing)

#define NetAddUint32(m,pos,thing) \
	*((Uint32*)(&((m).body[(pos)]))) = (thing)

#define NetAdd2(m,pos,thing) \
	memcpy( &((*m).body[pos]), &(thing), sizeof(thing))

#define NetAddSt(m,pos,stri) \
	strcpy(&(m.body[pos]),stri)



#define NetGet(m,pos,thing) \
	memcpy(&(thing),&(m->body[pos]),sizeof(thing))

#define NetGetUint8(m,pos,thing) \
	(thing) = *((Uint8*)(&((m)->body[(pos)])))

#define NetGetUint16(m,pos,thing) \
	(thing) = *((Uint16*)(&((m)->body[(pos)])))

#define NetGetUint32(m,pos,thing) \
	(thing) = *((Uint32*)(&((m)->body[(pos)])))

#endif

