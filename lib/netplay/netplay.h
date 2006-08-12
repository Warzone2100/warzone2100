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
	DWORD dwSize;
	DWORD dwFlags;
	char host[16];	// host ip address
//	GUID  guidInstance;
//	GUID  guidApplication;
	DWORD dwMaxPlayers;
	DWORD dwCurrentPlayers;
//    union  {
//        LPWSTR lpszSessionName;
//        LPSTR  lpszSessionNameA;};
//    union  {
//        LPWSTR lpszPassword;
//        LPSTR  lpszPasswordA;  };
//    DWORD dwReserved1;
//    DWORD dwReserved2;
	DWORD dwUser1;
	DWORD dwUser2;
	DWORD dwUser3;
	DWORD dwUser4;
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
	DPID dpid;
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

	DPID		dpidPlayer;			// ID of player created

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
extern BOOL   NETselectProtocol(LPVOID lpConnection);		//choose one. 
extern BOOL   NETsend(NETMSG *msg, DPID player, BOOL guarantee);// send to player, possibly guaranteed
extern BOOL   NETbcast(NETMSG *msg,BOOL guarantee);		// broadcast to everyone, possibly guaranteed
extern BOOL   NETrecv(NETMSG *msg);				// recv a message if possible

extern UCHAR   NETsendFile(BOOL newFile, CHAR *fileName, DPID player);	// send file chunk.
extern UCHAR   NETrecvFile(NETMSG *pMsg);			// recv file chunk

extern HRESULT NETclose	(VOID);					// close current game
extern HRESULT NETshutdown(VOID);				// leave the game in play.

extern UDWORD  NETgetBytesSent(VOID);				// return bytes sent/recv.  call regularly for good results
extern UDWORD  NETgetPacketsSent(VOID);				// return packets sent/recv.  call regularly for good results
extern UDWORD	NETgetBytesRecvd(VOID);				// return bytes sent/recv.  call regularly for good results
extern UDWORD	NETgetPacketsRecvd(VOID);			// return packets sent/recv.  call regularly for good results
extern UDWORD	NETgetRecentBytesSent(VOID);			// more immediate functions.
extern UDWORD	NETgetRecentPacketsSent(VOID);
extern UDWORD	NETgetRecentBytesRecvd(VOID);
extern UDWORD	NETgetRecentPacketsRecvd(VOID);

// from netjoin.c
extern DWORD	NETgetGameFlags(UDWORD flag);			// return one of the four flags(dword) about the game.
extern DWORD	NETgetGameFlagsUnjoined(UDWORD gameid,UDWORD flag);	// return one of the four flags(dword) about the game.
extern BOOL	NETsetGameFlags(UDWORD flag,DWORD value);	// set game flag(1-4) to value.		
extern BOOL	NEThaltJoining(VOID);				// stop new players joining this game
extern BOOL	NETfindGame(BOOL asynchronously);		// find games being played(uses GAME_GUID);
extern BOOL	NETjoinGame(UDWORD gameNumber, LPSTR playername);			// join game given with playername
extern BOOL	NEThostGame(LPSTR SessionName, LPSTR PlayerName,// host a game 
			    DWORD one, DWORD two, DWORD three, DWORD four, UDWORD plyrs);

//from netusers.c
extern BOOL	NETuseNetwork(BOOL val);	// TURN on/off networking.
extern UDWORD	NETplayerInfo();		// count players in this game.
extern BOOL	NETchangePlayerName(UDWORD dpid, char *newName);// change a players name.
extern BOOL	NETgetLocalPlayerData(DPID dpid, VOID *pData, DWORD *pSize);
extern BOOL	NETgetGlobalPlayerData(DPID dpid, VOID *pData, DWORD *pSize);
extern BOOL	NETsetLocalPlayerData(DPID dpid, VOID *pData, DWORD size);
extern BOOL	NETsetGlobalPlayerData(DPID dpid, VOID *pData, DWORD size);

extern WZ_DEPRECATED BOOL	NETspectate();			// create a spectator
extern WZ_DEPRECATED BOOL	NETisSpectator(DPID dpid);	// check for spectator status.

#include "netlog.h"

// from net audio.
extern WZ_DEPRECATED BOOL	NETprocessAudioCapture(VOID);			//capture
extern WZ_DEPRECATED BOOL	NETstopAudioCapture(VOID);
extern WZ_DEPRECATED BOOL	NETstartAudioCapture(VOID);
extern WZ_DEPRECATED BOOL	NETshutdownAudioCapture(VOID);
extern WZ_DEPRECATED BOOL	NETinitAudioCapture(VOID);

extern WZ_DEPRECATED BOOL	NETinitPlaybackBuffer(void *pSoundBuffer);	// playback
extern WZ_DEPRECATED VOID	NETplayIncomingAudio(NETMSG *pMsg);
extern WZ_DEPRECATED BOOL	NETqueueIncomingAudio(void *pSoundData, DWORD soundBytes,BOOL bStream);
extern WZ_DEPRECATED BOOL	NETshutdownAudioPlayback(VOID);

// encryption
extern BOOL	NETsetKey(UDWORD c1,UDWORD c2,UDWORD c3, UDWORD c4);
extern NETMSG*	NETmanglePacket(NETMSG *msg);
extern VOID	NETunmanglePacket(NETMSG *msg);
extern BOOL	NETmangleData(long *input, long *result, UDWORD dataSize);
extern BOOL	NETunmangleData(long *input, long *result, UDWORD dataSize);
extern UDWORD	NEThashFile(char *pFileName);
extern UCHAR	NEThashVal(UDWORD value);
extern UDWORD	NEThashBuffer(char *pData, UDWORD size);

extern WZ_DEPRECATED BOOL NETcheckRegistryEntries	(char *name,char *guid);
extern WZ_DEPRECATED BOOL NETsetRegistryEntries	(char *name,char *guid,char *file,char *cline,char *path,char *cdir); 
extern WZ_DEPRECATED BOOL NETconnectToLobby		(LPNETPLAY lpNetPlay);
//#include "netlobby.h"	// more functions to provide lobby facilities.

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

