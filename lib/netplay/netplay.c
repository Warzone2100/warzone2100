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
/**
 * @file netplay.c
 *
 * Basic netcode.
 */

#include "lib/framework/frame.h"
#include "lib/framework/string_ext.h"
#include "lib/gamelib/gtime.h"

#include <time.h>			// for stats
#include <SDL_thread.h>
#ifdef WZ_OS_MAC
#include <SDL_net/SDL_net.h>
#else
#include <SDL_net.h>
#endif
#include <physfs.h>
#include <string.h>

#include "netplay.h"
#include "netlog.h"

// WARNING !!! This is initialised via configuration.c !!!
char masterserver_name[255] = {'\0'};
unsigned int masterserver_port = 0, gameserver_port = 0;

#define MAX_CONNECTED_PLAYERS	8
#define MAX_TMP_SOCKETS		16

#define NET_READ_TIMEOUT	0
/*
*
*	NOTE /rant:  If the buffer size isn't big enough, it will invalidate the socket.
*	Which means that we need to allocate a buffer big enough to handle worst case
*	situations.
*	SDLNet_TCP_Recv() craps out because of this, and the forthcoming error message
*	is less than eloquent.
*	("SDL is not running on known window manager" is NOT a valid socket error msg!)
*	Perhaps SDLNet_GetError() being a alias for SDL_GetError() has something to do with this?
*
*	reference: MaxMsgSize in netplay.h  (currently set to 8192)
*	
*/
#define NET_BUFFER_SIZE	(MaxMsgSize)	// Would be 8K

// HACK(s) to allow us to call a src/multi*.c function
extern void recvMultiStats(void);								// from src/multistat.c
extern BOOL sendTextMessage(const char *pStr, BOOL all);		// from src/multiplay.c
extern BOOL MultiPlayerJoin(UDWORD dpid);						// from src/multijoin.c
extern BOOL MultiPlayerLeave(UDWORD dpid);						// from src/multijoin.c
extern void ShowMOTD(void);																// from src/multijoin.c
extern void kickPlayer(uint32_t player_id, const char *reason, LOBBY_ERROR_TYPES type);	// from src/multiinit.c
extern void setLobbyError (LOBBY_ERROR_TYPES error_type);							// from src/multiinit.c
extern LOBBY_ERROR_TYPES getLobbyError(void);										// from src/multiinit.c
// ////////////////////////////////////////////////////////////////////////
// Function prototypes
void NETplayerLeaving(UDWORD dpid);		// Cleanup sockets on player leaving (nicely)
void NETplayerDropped(UDWORD dpid);		// Broadcast NET_PLAYER_DROPPED & cleanup
static void NETallowJoining(void);
void sendVersionCheck(void);
void recvVersionCheck(void);
void VersionCheckTimeOut( uint32_t victimdpid );
void NETCheckVersion( uint32_t player );

void NETGameLocked( bool flag);
void NETresetGamePassword(void);
void sendPasswordCheck(void);
void recvPasswordCheck(void);
/*
 * Network globals, these are part of the new network API
 */
NETMSG NetMsg;

// ////////////////////////////////////////////////////////////////////////
// Types

typedef struct		// data regarding the last one second or so.
{
	UDWORD		bytesRecvd;
	UDWORD		bytesSent;	// number of bytes sent in about 1 sec.
	UDWORD		packetsSent;
	UDWORD		packetsRecvd;
} NETSTATS;

typedef struct
{
	uint16_t        size;
	void*           data;
	size_t          buffer_size;
} NET_PLAYER_DATA;

typedef struct
{
	uint32_t		id;
	BOOL			allocated;
	char			name[StringSize];
	uint32_t		flags;
	uint32_t		heartattacktime;		//time cardiac arrest started
	BOOL			heartbeat;				// if we are still alive or not
	BOOL			kick;					// if we should kick them
} NET_PLAYER;

typedef struct
{
	TCPsocket	socket;
	char*		buffer;
	unsigned int	buffer_start;
	unsigned int	bytes;
} NETBUFSOCKET;

#define PLAYER_HOST		1

// ////////////////////////////////////////////////////////////////////////
// Variables

NETPLAY	NetPlay;

static BOOL		allow_joining = false;
static	bool server_not_there = false;
static GAMESTRUCT	game;

/**
 * Socket used for these purposes:
 *  * Host a game, be a server.
 *  * Connect to the lobby server.
 *  * Join a server for a game.
 */
static TCPsocket	tcp_socket = NULL;		//socket used to talk to lobbyserver/ host machine

static NETBUFSOCKET*	bsocket = NULL;		//buffered socket (holds tcp_socket) (clients only?)
static NETBUFSOCKET*	connected_bsocket[MAX_CONNECTED_PLAYERS] = { NULL };
static SDLNet_SocketSet	socket_set = NULL;
static BOOL		is_server = false;

/**
 * Used for connections with clients.
 */
static TCPsocket	tmp_socket[MAX_TMP_SOCKETS] = { NULL };

static SDLNet_SocketSet	tmp_socket_set = NULL;
static char*		hostname;
static NETSTATS		nStats = { 0, 0, 0, 0 };
static NET_PLAYER	players[MAX_CONNECTED_PLAYERS];
static int32_t          NetGameFlags[4] = { 0, 0, 0, 0 };
char iptoconnect[PATH_MAX] = "\0"; // holds IP/hostname from command line

extern int NET_PlayerConnectionStatus;		// from src/display3d.c
extern LOBBY_ERROR_TYPES LobbyError;		// from src/multiint.c

//time when check sent. Note, uses dpid, & using 0xffffffff to signal nothing sent yet.
uint32_t VersionCheckTime[MAX_PLAYERS] = {-1, -1, -1, -1, -1, -1, -1, -1};
static BOOL playerVersionFlag[MAX_PLAYERS] = {false};	// we kick on false
static bool playerPasswordFlag[MAX_PLAYERS] = {false};		// we kick on false
// ////////////////////////////////////////////////////////////////////////////
#define VersionStringSize 80
/************************************************************************************
 **  NOTE (!)  Change the VersionString when net code changes!!
 **			   ie ("trunk", "2.1.3", ...)
 ************************************************************************************
**/
char VersionString[VersionStringSize] = "2.2"; 
static int NETCODE_VERSION_MAJOR = 2;	// unused for now
static int NETCODE_VERSION_MINOR = 2;	// unused for now
static int NUMBER_OF_MODS = 0;			// unused for now
static int NETCODE_HASH = 0;			// unused for now

void sendVersionCheck( void )
{
	uint32_t playerdpid = NetPlay.dpidPlayer;

	NETlogEntry("Sending version check", 0, 0);
	NETbeginEncode(NET_VERSION_CHECK, NET_ALL_PLAYERS);
		NETuint32_t(&playerdpid);
		NETstring( VersionString, sizeof(VersionString) );
		NETint32_t( &NETCODE_VERSION_MAJOR );
		NETint32_t( &NETCODE_VERSION_MINOR );
		NETint32_t( &NUMBER_OF_MODS );
		NETint32_t( &NETCODE_HASH );
	NETend();

	debug(LOG_NET, "sending dpid %u, version string [%s]", playerdpid, VersionString);
}
// Checks the version string, and if they are not the same, we auto-kick the player.
void recvVersionCheck()
{
	uint32_t victimdpid = 0;
	char playersVersion[VersionStringSize] = { '\0' };
	const char* msg;
	int32_t MajorVersion = 0;			// Not currently used
	int32_t MinorVersion = 0;			// Not currently used
	int32_t ModCount = 0;				// Not currently used
	int32_t Hash_Data = 0;				// Not currently used

	NETlogEntry("Receiving version check", 0, 0);

	NETbeginDecode(NET_VERSION_CHECK);
		NETuint32_t(&victimdpid);
		NETstring( playersVersion, sizeof(playersVersion) );
		NETint32_t( &MajorVersion );	// NETCODE_VERSION_MAJOR
		NETint32_t( &MinorVersion );	// NETCODE_VERSION_MINOR
		NETint32_t( &ModCount );		// NUMBER_OF_MODS
		NETint32_t( &Hash_Data );		// NETCODE_HASH
	NETend();

	if( strcmp(VersionString,playersVersion) != 0 )
	{
		debug(LOG_NET, "Received *wrong* version string [%s] from dpid %u. Expected [%s]", playersVersion, victimdpid, VersionString);

		sasprintf((char**)&msg, _("Player %u has the wrong game version.  Auto kicking."), victimdpid );
		sendTextMessage(msg, true);

		if(NetPlay.bHost)
		{
			kickPlayer( victimdpid, "you have the wrong version of the game, so Update it!", ERROR_WRONGVESION );

			// reset flags /time after we kick them
			VersionCheckTime[victimdpid] = -1;
			playerVersionFlag[victimdpid] = false;
		}
	}
	else
	{
		playerVersionFlag[victimdpid] = true;
		debug(LOG_NET, "Received correct version string [%s] from dpid %u", playersVersion, victimdpid);
	}
	// just in case someone has the bright idea of trying to fool us...

	if (NetPlay.GamePassworded && playerVersionFlag[victimdpid] && !playerPasswordFlag[victimdpid])
	{
		// really should ban them. :P
		debug(LOG_ERROR, "Received correct version string [%s] from dpid %u", playersVersion, victimdpid);
		debug(LOG_ERROR, "Did NOT receive passowrd from them --Autokicking ip %s", "123" );
		kickPlayer(victimdpid, "you  have attempted to thwart the security measuers.  Entry denied!", ERROR_CHEAT);
	}
}
void VersionCheckTimeOut( uint32_t victimdpid )
{
	const char* msg;

	debug(LOG_NET, "version string was never received from dpid %u. Auto kicking at %u", victimdpid, gameTime2 );

	sasprintf((char**)&msg, _("Player %u has the wrong game version.  Auto kicking."), victimdpid );
	sendTextMessage(msg, true);

	if(NetPlay.bHost)
	{
		kickPlayer( victimdpid, "You have the wrong version of the game, so Update it!", ERROR_WRONGVESION );

		// reset flags /time after we kick them
		VersionCheckTime[victimdpid] = -1;
		playerVersionFlag[victimdpid] = false;
	}
}
void NETCheckVersion( uint32_t player )
{
	// When flag is true, means we have received the check OK
	if( playerVersionFlag[player] == false )
	{
		if (player != HOST_DPID)
		{
			VersionCheckTimeOut( player);
		}
	}

}

//	Sets if the game is password protected or not
void NETGameLocked( bool flag)
{
	NetPlay.GamePassworded = flag;
	game.privateGame = flag;
	debug(LOG_NET, "Passworded game is %s", NetPlay.GamePassworded ? "TRUE" : "FALSE" );
}

//	Sets the game password
void NETsetGamePassword(const char *password)
{
	sstrcpy(NetPlay.gamePassword, password);
	debug(LOG_NET, "Password entered is: [%s]", NetPlay.gamePassword);
}

//	Resets the game password
void NETresetGamePassword(void)
{
	sstrcpy(NetPlay.gamePassword, "Enter Password Here");
	debug(LOG_NET, "password reset to 'Enter Password here'");
	NETGameLocked(false);
}

//	send our password to host
void sendPasswordCheck(void)
{
	uint32_t playerdpid = NetPlay.dpidPlayer;

	NETlogEntry("Sending password check", 0, 0);
	NETbeginEncode(NET_PASSWORD_CHECK, PLAYER_HOST);
		NETuint32_t(&playerdpid);
		NETstring( NetPlay.gamePassword, sizeof(NetPlay.gamePassword) );
	NETend();

	debug(LOG_NET, "sending dpid %u, password [%s]", playerdpid, NetPlay.gamePassword);
}

// Checks the password, and if they are not the same, we auto-kick the player.
void recvPasswordCheck(void)
{
	uint32_t victimdpid = 0;

	char passwordreceived[StringSize];
	const char* msg;

	NETlogEntry("Receiving password check", 0, 0);

	NETbeginDecode(NET_PASSWORD_CHECK);
		NETuint32_t(&victimdpid);
		NETstring( passwordreceived, sizeof(passwordreceived) );
	NETend();

	if (strcmp(NetPlay.gamePassword, passwordreceived) != 0)
	{
		debug(LOG_NET, "Received *wrong* password [%s] from dpid %u. Expected [%s]", passwordreceived, victimdpid, NetPlay.gamePassword);

		sasprintf((char**)&msg, _("Player %u has the wrong password.  Auto kicking."), victimdpid );
		sendTextMessage(msg, true);

		if (NetPlay.bHost)
		{
			kickPlayer(victimdpid, "you have the wrong password! Entry denied!", ERROR_WRONGPASSWORD);
			playerPasswordFlag[victimdpid] = false;
		}
	}
	else
	{
		playerPasswordFlag[victimdpid] = true;
		debug(LOG_NET, "Received correct password [%s] from dpid %u", passwordreceived, victimdpid);
	}
}

// *********** Socket with buffer that read NETMSGs ******************

static NETBUFSOCKET* NET_createBufferedSocket(void)
{
	NETBUFSOCKET* bs = (NETBUFSOCKET*)malloc(sizeof(*bs));

	bs->socket = NULL;
	bs->buffer = NULL;
	bs->buffer_start = 0;
	bs->bytes = 0;

	return bs;
}

static void NET_destroyBufferedSocket(NETBUFSOCKET* bs)
{
	free(bs->buffer);
	free(bs);
}

static void NET_initBufferedSocket(NETBUFSOCKET* bs, TCPsocket s)
{
	bs->socket = s;
	if (bs->buffer == NULL) {
		bs->buffer = (char*)malloc(NET_BUFFER_SIZE);
	}
	bs->buffer_start = 0;
	bs->bytes = 0;
}

static BOOL NET_fillBuffer(NETBUFSOCKET* bs, SDLNet_SocketSet socket_set)
{
	int size;
	char* bufstart = bs->buffer + bs->buffer_start + bs->bytes;
	const int bufsize = NET_BUFFER_SIZE - bs->buffer_start - bs->bytes;


	if (bs->buffer_start != 0)
	{
		return false;
	}

	if (SDLNet_SocketReady(bs->socket) <= 0)
	{
		return false;
	}

	size = SDLNet_TCP_Recv(bs->socket, bufstart, bufsize);

	if (size > 0)
	{
		bs->bytes += size;
		return true;
	}
	else
	{	// an error occured, or the remote host has closed the connection.
		if (socket_set != NULL)
		{
			SDLNet_TCP_DelSocket(socket_set, bs->socket);
		}
		ASSERT( bs->bytes < NET_BUFFER_SIZE, "Socket buffer is too small!");
		if( bs->bytes > NET_BUFFER_SIZE)
		{
			debug(LOG_ERROR, "Fatal connection error: buffer size of (%d) was too small, current byte count was %d", NET_BUFFER_SIZE, bs->bytes);
		}
		debug(LOG_WARNING, "SDLNet_TCP_Recv error: %s tcp_socket %p is now invalid", SDLNet_GetError(), bs->socket);
		SDLNet_TCP_Close(bs->socket);
		bs->socket = NULL;
	}

	return false;
}

// Check if we have a full message waiting for us. If not, return false and wait for more data.
// If there is a data remnant somewhere in the buffer except at its beginning, move it to the
// beginning.
static BOOL NET_recvMessage(NETBUFSOCKET* bs)
{
	NETMSG *pMsg = &NetMsg;
	unsigned int size;
	const NETMSG* message = (NETMSG*)(bs->buffer + bs->buffer_start);
	const unsigned int headersize =   sizeof(message->size)
					+ sizeof(message->type)
					+ sizeof(message->destination)
					+ sizeof(message->source);

	if (headersize > bs->bytes)
	{
		goto error;
	}

	size = SDL_SwapBE16(message->size) + headersize;

	if (size > bs->bytes)
	{
		goto error;
	}

	memcpy(pMsg, message, size);
	pMsg->size = SDL_SwapBE16(message->size);
	bs->buffer_start += size;
	bs->bytes -= size;

	return true;

error:
	if (bs->buffer_start != 0)
	{
		static char* tmp_buffer = NULL;
		char* buffer_start = bs->buffer + bs->buffer_start;
		char* tmp;

		// Create tmp buffer if necessary
		if (tmp_buffer == NULL)
		{
			tmp_buffer = (char*)malloc(NET_BUFFER_SIZE);
		}

		// Move remaining contents into tmp buffer
		memcpy(tmp_buffer, buffer_start, bs->bytes);

		// swap tmp buffer with buffer
		tmp = bs->buffer;
		bs->buffer = tmp_buffer;
		tmp_buffer = tmp;

		if (tmp_buffer)
		{
			free(tmp_buffer);
			tmp_buffer = NULL;
		}
		// Now data is in the beginning of the buffer
		bs->buffer_start = 0;
	}

	return false;
}

static void NET_InitPlayers(void)
{
	unsigned int i;

	for (i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
	{
		players[i].allocated = false;
		players[i].id = i;
		players[i].heartattacktime = 0;
		players[i].heartbeat = true;		// we always start with a hearbeat
		players[i].kick = false;
	}
}

static void NETBroadcastPlayerInfo(uint32_t dpid)
{
	NETbeginEncode(NET_PLAYER_INFO, NET_ALL_PLAYERS);
		NETuint32_t(&players[dpid].id);
		NETbool(&players[dpid].allocated);
		NETbool(&players[dpid].heartbeat);
		NETbool(&players[dpid].kick);
		NETstring(players[dpid].name, sizeof(players[dpid].name));
		NETuint32_t(&players[dpid].flags);
		NETuint32_t(&players[dpid].heartattacktime);
	NETend();
}

static unsigned int NET_CreatePlayer(const char* name, unsigned int flags)
{
	unsigned int i;

	for (i = 1; i < MAX_CONNECTED_PLAYERS; ++i)
	{
		if (players[i].allocated == false)
		{
			debug(LOG_NET, "A new player has been created. Player, %s, is set to slot %u", name, i);
			players[i].allocated = true;
			sstrcpy(players[i].name, name);
			players[i].flags = flags;
			NETBroadcastPlayerInfo(i);
			return i;
		}
	}

	return 0;
}

static void NET_DestroyPlayer(unsigned int id)
{
	debug(LOG_NET, "Freeing slot %u for a new player", id);
	players[id].allocated = false;
}

// ////////////////////////////////////////////////////////////////////////
// count players.
UDWORD NETplayerInfo(void)
{
	unsigned int i;

	NetPlay.playercount = 0;		// reset player counter

	if(!NetPlay.bComms)
	{
		NetPlay.playercount		= 1;
		NetPlay.players[0].bHost	= true;
		NetPlay.players[0].dpid		= HOST_DPID;
		return 1;
	}

	memset(NetPlay.players, 0, sizeof(NetPlay.players));	// reset player info

	for (i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
	{
		if (players[i].allocated == true)
		{
			NetPlay.players[NetPlay.playercount].dpid = i;
			sstrcpy(NetPlay.players[NetPlay.playercount].name, players[i].name);

			if (players[i].flags & PLAYER_HOST)
			{
				NetPlay.players[NetPlay.playercount].bHost = true;
			}
			else
			{
				NetPlay.players[NetPlay.playercount].bHost = false;
			}

			NetPlay.playercount++;
		}
	}

	return NetPlay.playercount;
}
/**
 * @note When a player leaves nicely (ie, we got a NET_PLAYER_LEAVING
 *       message), we clean up the socket that we used.
 * \param dpid
 */
void NETplayerLeaving(UDWORD dpid)
{
	if(connected_bsocket[dpid])
	{
		debug(LOG_NET, "Player (%u) has left nicely, closing socket %p",
			dpid, connected_bsocket[dpid]->socket);

		// Although we can get a error result from DelSocket, it don't really matter here.
		SDLNet_TCP_DelSocket(socket_set, connected_bsocket[dpid]->socket);
		SDLNet_TCP_Close(connected_bsocket[dpid]->socket);
		connected_bsocket[dpid]->socket = NULL;
	}
	else
	{
		debug(LOG_NET, "Player (%u) has left nicely", dpid);
	}
}
/**
 * @note When a player's connection is broken we broadcast the NET_PLAYER_DROPPED
 *       message.
 * \param dpid
 */
void NETplayerDropped(UDWORD dpid)
{
	uint32_t id = dpid;

	// Send message type speciffically for dropped / disconnects
	NETbeginEncode(NET_PLAYER_DROPPED, NET_ALL_PLAYERS);
		NETuint32_t(&id);
	NETend();
	
	NET_DestroyPlayer(id);		// just clears array
	MultiPlayerLeave(id);			// more cleanup
	NET_PlayerConnectionStatus = 2;	//DROPPED_CONNECTION
}
// ////////////////////////////////////////////////////////////////////////
// rename the local player
BOOL NETchangePlayerName(UDWORD dpid, char *newName)
{
	if(!NetPlay.bComms)
	{
		sstrcpy(NetPlay.players[0].name, newName);
		return true;
	}
	debug(LOG_NET, "Requesting a change of player name for pid=%u to %s", dpid, newName);

	sstrcpy(players[dpid].name, newName);

	NETBroadcastPlayerInfo(dpid);

	return true;
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
		return NetGameFlags[flag-1];
	}
}

static void NETsendGameFlags(void)
{
	NETbeginEncode(NET_GAME_FLAGS, NET_ALL_PLAYERS);
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
BOOL NETsetGameFlags(UDWORD flag, SDWORD value)
{
	if(!NetPlay.bComms)
	{
		return true;
	}

	if (flag > 0 && flag < 5)
	{
		return (NetGameFlags[flag-1] = value);
	}

	NETsendGameFlags();

	return true;
}

/**
 * @note \c game is being sent to the master server (if hosting)
 *       The implementation of NETsendGAMESTRUCT <em>must</em> guarantee to
 *       pack it in network byte order (big-endian).
 *
 * @see GAMESTRUCT,NETrecvGAMESTRUCT
 */
static void NETsendGAMESTRUCT(TCPsocket socket, const GAMESTRUCT* game)
{
	// A buffer that's guaranteed to have the correct size (i.e. it
	// circumvents struct padding, which could pose a problem).  Initialise
	// to zero so that we can be sure we're not sending any (undefined)
	// memory content across the network.
	char buf[sizeof(game->GAMESTRUCT_VERSION) + sizeof(game->name) + sizeof(game->desc.host) + (sizeof(int32_t) * 8) +
		sizeof(game->misc) + sizeof(game->extra) + sizeof(game->versionstring) +
		sizeof(game->modlist) + (sizeof(uint32_t) * 9) ] = { 0 };
	char *buffer = buf;
	unsigned int i;
	int result;

	// Now dump the data into the buffer
	// Copy 32bit large big endian numbers
	*(uint32_t*)buffer = SDL_SwapBE32(game->GAMESTRUCT_VERSION);
	buffer += sizeof(uint32_t);

	// Copy a string
	strlcpy(buffer, game->name, sizeof(game->name));
	buffer += sizeof(game->name);

	// Copy 32bit large big endian numbers
	*(int32_t*)buffer = SDL_SwapBE32(game->desc.dwSize);
	buffer += sizeof(int32_t);
	*(int32_t*)buffer = SDL_SwapBE32(game->desc.dwFlags);
	buffer += sizeof(int32_t);

	// Copy yet another string
	strlcpy(buffer, game->desc.host, sizeof(game->desc.host));
	buffer += sizeof(game->desc.host);

	// Copy 32bit large big endian numbers
	*(int32_t*)buffer = SDL_SwapBE32(game->desc.dwMaxPlayers);
	buffer += sizeof(int32_t);
	*(int32_t*)buffer = SDL_SwapBE32(game->desc.dwCurrentPlayers);
	buffer += sizeof(int32_t);
	for (i = 0; i < ARRAY_SIZE(game->desc.dwUserFlags); ++i)
	{
		*(int32_t*)buffer = SDL_SwapBE32(game->desc.dwUserFlags[i]);
		buffer += sizeof(int32_t);
	}

	// Copy a string
	strlcpy(buffer, game->misc, sizeof(game->misc));
	buffer += sizeof(game->misc);

	// Copy a string
	strlcpy(buffer, game->extra, sizeof(game->extra));
	buffer += sizeof(game->extra);

	// Copy a string
	strlcpy(buffer, game->versionstring, sizeof(game->versionstring));
	buffer += sizeof(game->versionstring);

	// Copy a string
	strlcpy(buffer, game->modlist, sizeof(game->modlist));
	buffer += sizeof(game->modlist);

	// Copy 32bit large big endian numbers
	*(uint32_t*)buffer = SDL_SwapBE32(game->game_version_major);
	buffer += sizeof(uint32_t);

	// Copy 32bit large big endian numbers
	*(uint32_t*)buffer = SDL_SwapBE32(game->game_version_minor);
	buffer += sizeof(uint32_t);

	// Copy 32bit large big endian numbers
	*(uint32_t*)buffer = SDL_SwapBE32(game->privateGame);
	buffer += sizeof(uint32_t);

	// Copy 32bit large big endian numbers
	*(uint32_t*)buffer = SDL_SwapBE32(game->pureGame);
	buffer += sizeof(uint32_t);

	// Copy 32bit large big endian numbers
	*(uint32_t*)buffer = SDL_SwapBE32(game->Mods);
	buffer += sizeof(uint32_t);

	// Copy 32bit large big endian numbers
	*(uint32_t*)buffer = SDL_SwapBE32(game->future1);
	buffer += sizeof(uint32_t);

	// Copy 32bit large big endian numbers
	*(uint32_t*)buffer = SDL_SwapBE32(game->future2);
	buffer += sizeof(uint32_t);

	// Copy 32bit large big endian numbers
	*(uint32_t*)buffer = SDL_SwapBE32(game->future3);
	buffer += sizeof(uint32_t);

	// Copy 32bit large big endian numbers
	*(uint32_t*)buffer = SDL_SwapBE32(game->future4);
	buffer += sizeof(uint32_t);


	// Send over the GAMESTRUCT
	result = SDLNet_TCP_Send(socket, buf, sizeof(buf));
	if (result != sizeof(buf))
	{
		// If packet could not be sent, we should inform user of the error.
		debug(LOG_ERROR, "Failed to send GAMESTRUCT. Reason: %s", SDLNet_GetError());
		debug(LOG_ERROR, "Please make sure TCP ports %u & %u are open!", masterserver_port, gameserver_port);
	}
}

/**
 * @note \c game is being retrieved from the master server (if browsing the
 *       lobby). The implementation of NETrecvGAMESTRUCT should assume the data
 *       to be packed in network byte order (big-endian).
 *
 * @see GAMESTRUCT,NETsendGAMESTRUCT
 */
static bool NETrecvGAMESTRUCT(GAMESTRUCT* game)
{
	// A buffer that's guaranteed to have the correct size (i.e. it
	// circumvents struct padding, which could pose a problem).
	char buf[sizeof(game->GAMESTRUCT_VERSION) + sizeof(game->name) + sizeof(game->desc.host) + (sizeof(int32_t) * 8) +
		sizeof(game->misc) + sizeof(game->extra) + sizeof(game->versionstring) +
		sizeof(game->modlist) + (sizeof(uint32_t) * 9) ] = { 0 };
	char* buffer = buf;
	unsigned int i;
	int result = 0;

	// Read a GAMESTRUCT from the connection
	if (tcp_socket == NULL
	 || socket_set == NULL
	 || SDLNet_CheckSockets(socket_set, 1000) <= 0
	 || !SDLNet_SocketReady(tcp_socket)
	 || (result = SDLNet_TCP_Recv(tcp_socket, buf, sizeof(buf))) != sizeof(buf))
	{
		if (result < 0)
		{
			debug(LOG_WARNING, "SDLNet_TCP_Recv error: %s tcp_socket %p is now invalid", SDLNet_GetError(), tcp_socket);
			tcp_socket = NULL;  //SDLnet docs say to 'disconnect' here. Dunno how. :S
		}
		return false;
	}

	// Now dump the data into the game struct
	// Copy 32bit large big endian numbers
	game->GAMESTRUCT_VERSION = SDL_SwapBE32(*(uint32_t*)buffer);
	buffer += sizeof(uint32_t);
	// Copy a string
	sstrcpy(game->name, buffer);
	buffer += sizeof(game->name);

	// Copy 32bit large big endian numbers
	game->desc.dwSize = SDL_SwapBE32(*(int32_t*)buffer);
	buffer += sizeof(int32_t);
	game->desc.dwFlags = SDL_SwapBE32(*(int32_t*)buffer);
	buffer += sizeof(int32_t);

	// Copy yet another string
	sstrcpy(game->desc.host, buffer);
	buffer += sizeof(game->desc.host);

	// Copy 32bit large big endian numbers
	game->desc.dwMaxPlayers = SDL_SwapBE32(*(int32_t*)buffer);
	buffer += sizeof(int32_t);
	game->desc.dwCurrentPlayers = SDL_SwapBE32(*(int32_t*)buffer);
	buffer += sizeof(int32_t);
	for (i = 0; i < ARRAY_SIZE(game->desc.dwUserFlags); ++i)
	{
		game->desc.dwUserFlags[i] = SDL_SwapBE32(*(int32_t*)buffer);
		buffer += sizeof(int32_t);
	}

	// Copy a string
	sstrcpy(game->misc, buffer);
	buffer += sizeof(game->misc);

	// Copy a string
	sstrcpy(game->extra, buffer);
	buffer += sizeof(game->extra);

	// Copy a string
	sstrcpy(game->versionstring, buffer);
	buffer += sizeof(game->versionstring);

	// Copy a string
	sstrcpy(game->modlist, buffer);
	buffer += sizeof(game->modlist);

	// Copy 32bit large big endian numbers
	game->game_version_major = SDL_SwapBE32(*(uint32_t*)buffer);
	buffer += sizeof(uint32_t);
	game->game_version_minor = SDL_SwapBE32(*(uint32_t*)buffer);
	buffer += sizeof(uint32_t);
	game->privateGame = SDL_SwapBE32(*(uint32_t*)buffer);
	buffer += sizeof(uint32_t);
	game->pureGame = SDL_SwapBE32(*(uint32_t*)buffer);
	buffer += sizeof(uint32_t);
	game->Mods = SDL_SwapBE32(*(uint32_t*)buffer);
	buffer += sizeof(uint32_t);
	game->future1 = SDL_SwapBE32(*(uint32_t*)buffer);
	buffer += sizeof(uint32_t);	
	game->future2 = SDL_SwapBE32(*(uint32_t*)buffer);
	buffer += sizeof(uint32_t);	
	game->future3 = SDL_SwapBE32(*(uint32_t*)buffer);
	buffer += sizeof(uint32_t);	
	game->future4 = SDL_SwapBE32(*(uint32_t*)buffer);
	buffer += sizeof(uint32_t);	
	
	return true;
}

// ////////////////////////////////////////////////////////////////////////
// setup stuff
int NETinit(BOOL bFirstCall)
{
	UDWORD i;

	debug(LOG_NET, "NETinit");
	if(bFirstCall)
	{
		debug(LOG_NET, "NETPLAY: Init called, MORNIN'");

		NetPlay.dpidPlayer		= 0;
		NetPlay.bHost			= 0;
		for(i = 0; i < MAX_PLAYERS; i++)
		{
			memset(&NetPlay.players[i], 0, sizeof(NetPlay.players[i]));
			memset(&NetPlay.games[i], 0, sizeof(NetPlay.games[i]));
		}
		NetPlay.bComms = true;
		NetPlay.GamePassworded = false;
		NetPlay.ShowedMOTD = false;
		NetPlay.gamePassword[0] = '\0';
		NetPlay.MOTDbuffer[0] = '\0';
		sstrcpy(NetPlay.gamePassword,"Enter Password First");
		NETstartLogging();
	}

	if (SDLNet_Init() == -1)
	{
		debug(LOG_ERROR, "SDLNet_Init reported: %s", SDLNet_GetError());
		return -1;
	}

	NetPlay.ShowedMOTD = false;
	NetPlay.GamePassworded = false;

	return 0;
}


// ////////////////////////////////////////////////////////////////////////
// SHUTDOWN THE CONNECTION.
int NETshutdown(void)
{
	debug( LOG_NET, "NETshutdown" );

	NETstopLogging();
	SDLNet_Quit();
	return 0;
}

// ////////////////////////////////////////////////////////////////////////
//close the open game..
int NETclose(void)
{
	unsigned int i;
	int result;

	// reset flag 
	NetPlay.ShowedMOTD = false;
	NEThaltJoining();

	debug(LOG_NET, "Terminating sockets.");

	is_server=false;
	server_not_there = false;

	if(bsocket)
	{	// need SDLNet_TCP_DelSocket() as well, socket_set or tmp_socket_set?
		debug(LOG_NET, "Closing bsocket %p socket %p (tcp_socket=%p)", bsocket, bsocket->socket, tcp_socket);
		//SDLNet_TCP_Close(bsocket->socket);
		NET_destroyBufferedSocket(bsocket);
		bsocket=NULL;
	}

	for(i = 0; i < MAX_CONNECTED_PLAYERS; i++)
	{
		if (connected_bsocket[i])
		{
			if(connected_bsocket[i]->socket)
			{
				debug(LOG_NET, "Closing connected_bsocket[%u], %p", i, connected_bsocket[i]->socket);
				SDLNet_TCP_Close(connected_bsocket[i]->socket);
			}
			NET_destroyBufferedSocket(connected_bsocket[i]);
			connected_bsocket[i]=NULL;
		}
		NET_DestroyPlayer(i);
	}

	if (tmp_socket_set)
	{
		debug(LOG_NET, "Freeing tmp_socket_set %p", tmp_socket_set);
		SDLNet_FreeSocketSet(tmp_socket_set);
		tmp_socket_set=NULL;
	}

	for (i = 0; i < MAX_TMP_SOCKETS; i++)
	{
		if (tmp_socket[i])
		{
			// FIXME: need SDLNet_TCP_DelSocket() as well, socket_set or tmp_socket_set?
			debug(LOG_NET, "Closing tmp_socket[%d] %p", i, tmp_socket[i]);
			SDLNet_TCP_Close(tmp_socket[i]);
			tmp_socket[i]=NULL;
		}
	}

	if (socket_set)
	{
		// checking to make sure tcp_socket is still valid
		result = SDLNet_TCP_DelSocket(socket_set, tcp_socket);
		if (result == -1)
		{
			// non fatal error
			debug(LOG_NET,"SDLNet_DelSocket error: %s", SDLNet_GetError());
			tcp_socket = NULL;
		}
		debug(LOG_NET, "Freeing socket_set %p", socket_set);
		SDLNet_FreeSocketSet(socket_set);
		socket_set=NULL;
	}
	if (tcp_socket)
	{
		debug(LOG_NET, "Closing tcp_socket %p", tcp_socket);
		SDLNet_TCP_Close(tcp_socket);
		tcp_socket=NULL;
	}

	return 0;
}


// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Send and Recv functions

// ////////////////////////////////////////////////////////////////////////
// return bytes of data sent recently.
UDWORD NETgetBytesSent(void)
{
	static UDWORD	lastsec=0;
	static UDWORD	timy=0;

	if(  (UDWORD)clock() > (timy+CLOCKS_PER_SEC) )
	{
		timy = clock();
		lastsec = nStats.bytesSent;
		nStats.bytesSent = 0;
	}

	return lastsec;
}

UDWORD NETgetRecentBytesSent(void)
{
	return nStats.bytesSent;
}


UDWORD NETgetBytesRecvd(void)
{
	static UDWORD	lastsec=0;
	static UDWORD	timy=0;
	if(  (UDWORD)clock() > (timy+CLOCKS_PER_SEC) )
	{
		timy = clock();
		lastsec = nStats.bytesRecvd;
		nStats.bytesRecvd = 0;
	}
	return lastsec;
}

UDWORD NETgetRecentBytesRecvd(void)
{
	return nStats.bytesRecvd;
}


//return number of packets sent last sec.
UDWORD NETgetPacketsSent(void)
{
	static UDWORD	lastsec=0;
	static UDWORD	timy=0;

	if(  (UDWORD)clock() > (timy+CLOCKS_PER_SEC) )
	{
		timy = clock();
		lastsec = nStats.packetsSent;
		nStats.packetsSent = 0;
	}

	return lastsec;
}


UDWORD NETgetRecentPacketsSent(void)
{
	return nStats.packetsSent;
}


UDWORD NETgetPacketsRecvd(void)
{
	static UDWORD	lastsec=0;
	static UDWORD	timy=0;
	if(  (UDWORD)clock() > (timy+CLOCKS_PER_SEC) )
	{
		timy = clock();
		lastsec = nStats.packetsRecvd;
		nStats.packetsRecvd = 0;
	}
	return lastsec;
}


// ////////////////////////////////////////////////////////////////////////
// Send a message to a player, option to guarantee message
BOOL NETsend(NETMSG *msg, UDWORD player)
{
	int size;
	int result = 0;

	if(!NetPlay.bComms)
	{
		return true;
	}

	if (player >= MAX_CONNECTED_PLAYERS) return false;
	msg->destination = player;
	msg->source = selectedPlayer;

	size = msg->size + sizeof(msg->size) + sizeof(msg->type) + sizeof(msg->destination) + sizeof(msg->source);

	NETlogPacket(msg, false);
	msg->size = SDL_SwapBE16(msg->size);

	if (is_server)
	{	// FIXME: We are NOT checking SDLNet_CheckSockets/SDLNet_SocketReady 
		// SDLNet_TCP_Send *can* block!
		if (   player < MAX_CONNECTED_PLAYERS
		    && connected_bsocket[player] != NULL
		    && connected_bsocket[player]->socket != NULL
		    && (result = SDLNet_TCP_Send(connected_bsocket[player]->socket,
				       msg, size) == size))
		{
			nStats.bytesSent   += size;
			nStats.packetsSent += 1;
			return true;
		}
		else
		{
			if (result < size)
			{
				// TCP_Send error, inform user of problem.  This really should never happen normally.
				// Possible client disconnect, and in either case, socket is most likely invalid now.
				debug(LOG_WARNING, "SDLNet_TCP_Send returned: %d error: %s line %d", result, SDLNet_GetError(), __LINE__);
				connected_bsocket[player]->socket = NULL ; // It is invalid,  Unknown how to handle.
			}
		}
	}
	else
	{
		// FIXME: We are NOT checking SDLNet_CheckSockets/SDLNet_SocketReady 
		// SDLNet_TCP_Send *can* block!
			if (tcp_socket && (result = SDLNet_TCP_Send(tcp_socket, msg, size) == size))
			{
				return true;
			}
			else
			{
				if (result < size)
				{
					// TCP_Send error, inform user of problem.  This really should never happen normally.
					// Possible client disconnect, and in either case, socket is most likely invalid now.
					debug(LOG_WARNING, "SDLNet_TCP_Send returned: %d error: %s line %d", result, SDLNet_GetError(), __LINE__);
					tcp_socket = NULL; // unknow how to handle when invalid.
				}
			}

	}

	return false;
}

// ////////////////////////////////////////////////////////////////////////
// broadcast a message to all players.
BOOL NETbcast(NETMSG *msg)
{
	int size;
	int result = 0;

	if(!NetPlay.bComms)
	{
		return true;
	}

	msg->destination = NET_ALL_PLAYERS;
	msg->source = selectedPlayer;

	size = msg->size + sizeof(msg->size) + sizeof(msg->type) + sizeof(msg->destination) + sizeof(msg->source);

	NETlogPacket(msg, false);
	msg->size = SDL_SwapBE16(msg->size);

	if (is_server)
	{
		unsigned int i;

		for (i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
		{
			if (   connected_bsocket[i] == NULL
			    || connected_bsocket[i]->socket == NULL)
			{
				continue;
			}
			else
			{	
				// FIXME: We are NOT checking SDLNet_CheckSockets/SDLNet_SocketReady 
				// SDLNet_TCP_Send *can* block!
				result = SDLNet_TCP_Send(connected_bsocket[i]->socket, msg, size);
				if (result < size)
				{
					// TCP_Send error, inform user of problem.  This really should never happen normally.
					// Possible client disconnect, and in either case, socket is most likely invalid now.
					debug(LOG_WARNING, "(server) SDLNet_TCP_Send returned %d < %d, socket %p invalid: %s",
						  result, size, connected_bsocket[i]->socket, SDLNet_GetError());
					players[i].heartbeat = false;	//mark them dead
					debug(LOG_WARNING, "Player (dpid %u) connection was broken.", i);
					connected_bsocket[i]->socket = NULL; // Unsure how to handle invalid sockets.
				}
			}
		}
	}
	else
	{
		if (tcp_socket == NULL)
		{
			return false;
		}
		// FIXME: We are NOT checking SDLNet_CheckSockets/SDLNet_SocketReady 
		// SDLNet_TCP_Send *can* block!
		result = SDLNet_TCP_Send(tcp_socket, msg, size);
		if (result < size)
		{
			// I believe host has dropped...here
			debug(LOG_WARNING, "(client) SDLNet_TCP_Send returned %d < %d, tcp_socket %p is now invalid: %s", 
				  result, size, tcp_socket, SDLNet_GetError());
			debug(LOG_WARNING, "Host connection was broken?");
			tcp_socket = NULL; // unsure how to handle invalid sockets.
			players[HOST_DPID].heartbeat = false;	//mark them dead
			//Game is pretty much over --should just end everything when HOST dies.
			return false;
		}
	}

	nStats.bytesSent   += size;
	nStats.packetsSent += 1;

	return true;
}

///////////////////////////////////////////////////////////////////////////
// Check if a message is a system message
static BOOL NETprocessSystemMessage(void)
{
	NETMSG *pMsg = &NetMsg;

	switch (pMsg->type)
	{
		case NET_PLAYER_STATS:
		{
			recvMultiStats();
			break;
		}
		case NET_PLAYER_INFO:
		{
			uint32_t dpid;
			NETbeginDecode(NET_PLAYER_INFO);
				// Retrieve the player's ID
				NETuint32_t(&dpid);

				debug(LOG_NET, "Receiving MSG_PLAYER_INFO for player %u", (unsigned int)dpid);

				// Bail out if the given ID number is out of range
				if (dpid >= MAX_CONNECTED_PLAYERS)
				{
					debug(LOG_WARNING, "MSG_PLAYER_INFO: Player ID (%u) out of range (max %u)", dpid, (unsigned int)MAX_CONNECTED_PLAYERS);
					NETend();
					break;
				}

				// Copy the ID into the correct player's slot
				players[dpid].id = dpid;

				// Retrieve the rest of the data
				NETbool(&players[dpid].allocated);
				NETbool(&players[dpid].heartbeat);
				NETbool(&players[dpid].kick);
				NETstring(players[dpid].name, sizeof(players[dpid].name));
				NETuint32_t(&players[dpid].flags);
				NETuint32_t(&players[dpid].heartattacktime);
			NETend();

			NETplayerInfo();

			// If we're the game host make sure to send the updated
			// data to all other clients as well.
			if (is_server)
			{
				NETBroadcastPlayerInfo(dpid);
			}
			break;
		}
		case NET_PLAYER_JOINED:
		{
			uint8_t dpid;
			NETbeginDecode(NET_PLAYER_JOINED);
				NETuint8_t(&dpid);
			NETend();

			debug(LOG_NET, "Receiving NET_PLAYER_JOINED for player %u using socket %p",
				(unsigned int)dpid, tcp_socket);

			MultiPlayerJoin(dpid);
			break;
		}
		// This message type is when player is leaving 'nicely', and socket is still valid.
		case NET_PLAYER_LEAVING:
		{
			uint32_t dpid;

			NETbeginDecode(NET_PLAYER_LEAVING);
				NETuint32_t(&dpid);
			NETend();

			if(connected_bsocket[dpid])
			{
				debug(LOG_NET, "Receiving NET_PLAYER_LEAVING for player %u on socket %p",
							(unsigned int)dpid, connected_bsocket[dpid]->socket);
			}
			else
			{	// dropped from join screen most likely
				debug(LOG_NET, "Receiving NET_PLAYER_LEAVING for player %u ", (unsigned int)dpid);
			}

			NET_DestroyPlayer(dpid);			// sets dpid player's array to false
			MultiPlayerLeave(dpid);				// more cleanup
			NETplayerLeaving(dpid);				// need to close socket for the player that left.
			NET_PlayerConnectionStatus = 1;		//LEAVING_NICELY
			break;
		}
		case NET_GAME_FLAGS:
		{
			debug(LOG_NET, "Receiving game flags");

			NETbeginDecode(NET_GAME_FLAGS);
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

 			if (is_server)
 			{
				NETsendGameFlags();
			}
			break;
		}
		case NET_PASSWORD_CHECK:
		{
			recvPasswordCheck();
			break;
		}
		case NET_REQUEST_PASSWORD:
		{
			sendPasswordCheck();
			break;
		}
		case NET_VERSION_CHECK:
		{
			recvVersionCheck();
			break;
		}
		case NET_REQUEST_VERSION:
		{
			sendVersionCheck();
			break;
		}

		default:
			return false;
	}

	return true;
}
/*
*	Checks to see if a human player is still with us.
*	@note: resuscitation isn't possible with current code, so once we lose
*	the socket, then we have no way to connect with them again. Future 
*	item to enhance.
*/
static void NETcheckPlayers(void)
{
	int i;

	for(i=0; i< MAX_PLAYERS ; i++)
	{
		if(players[i].allocated == 0) continue;		// not allocated means that it most like it is a AI player
		if( players[i].heartbeat == 0 && players[i].heartattacktime == 0)	// looks like they are dead
		{
			players[i].heartattacktime = gameTime2;		// mark when this occured
		}
		else
		{
			if(players[i].heartattacktime)
			{
				if( players[i].heartattacktime + (15 * GAME_TICKS_PER_SEC) <  gameTime2) // wait 15 secs
				{
					players[i].kick = true;		// if still dead, then kick em.
				}
			}
		}
		if( players[i].kick)
		{
			NETplayerDropped(i);
		}
	}
}
// ////////////////////////////////////////////////////////////////////////
// Receive a message over the current connection. We return true if there
// is a message for the higher level code to process, and false otherwise.
// We should not block here.
BOOL NETrecv(uint8_t *type)
{
	NETMSG *pMsg = &NetMsg;
	static unsigned int current = 0;
	BOOL received;
	int size;

	if (!NetPlay.bComms)
	{
		return false;
	}

	if (is_server)
	{
		NETallowJoining();
	}

	NETcheckPlayers();		// make sure players are still alive & well

	do {
receive_message:
		received = false;

		if (is_server)
		{
			if (connected_bsocket[current] == NULL)
			{
				return false;
			}

			received = NET_recvMessage(connected_bsocket[current]);

			if (received == false)
			{
				uint32_t i = (current + 1) % 8;

				if (socket_set == NULL
				    || SDLNet_CheckSockets(socket_set, NET_READ_TIMEOUT) <= 0)
				{
					return false;
				}
				for (;;)
				{
					ASSERT(i < MAX_CONNECTED_PLAYERS, "Bad player number %u (current was %u)", i, current);
					if (connected_bsocket[i] == NULL || connected_bsocket[i]->socket == NULL)
					{
						// do nothing
					}
					else if (NET_fillBuffer(connected_bsocket[i], socket_set))
					{
						// we received some data, add to buffer
						received = NET_recvMessage(connected_bsocket[i]);
						current = i;
						break;
					}
					else if (connected_bsocket[i]->socket == NULL)
					{
						// If there is a error in NET_fillBuffer() then socket is already invalid.
						// This means that the player dropped / disconnected for whatever reason. 
						debug(LOG_WARNING, "Player, (dpid %u) seems to have dropped/disconnected.", i);

						// Send message type speciffically for dropped / disconnects
						NETbeginEncode(NET_PLAYER_DROPPED, NET_ALL_PLAYERS);
							NETuint32_t(&i);
						NETend();
	
						NET_DestroyPlayer(i);			// just clears array
						MultiPlayerLeave(i);			// more cleanup
						NET_PlayerConnectionStatus = 2;	//DROPPED_CONNECTION
						players[i].kick = true;			//they are going to get kicked.
					}

					if (++i == MAX_CONNECTED_PLAYERS)
					{
						i = 0;
					}

					if (i == current+1)
					{
						return false;
					}
				}
			}
		}
		else
		{
			// we are a client
			if (bsocket == NULL)
			{
				return false;
			}
			else
			{
				received = NET_recvMessage(bsocket);

				if (received == false)
				{
					if (   socket_set != NULL
					    && SDLNet_CheckSockets(socket_set, NET_READ_TIMEOUT) > 0
					    && NET_fillBuffer(bsocket, socket_set))
					{
						received = NET_recvMessage(bsocket);
					}
				}
			}
		}

		if (received == false)
		{
			return false;
		}
		else
		{
			size =	  pMsg->size + sizeof(pMsg->size) + sizeof(pMsg->type)
				+ sizeof(pMsg->destination) + sizeof(pMsg->source);
			if (is_server == false)
			{
				// do nothing
			}
			else if (pMsg->destination == NET_ALL_PLAYERS)
			{
				unsigned int j;

				pMsg->size = SDL_SwapBE16(pMsg->size);

				// we are the host, and have received a broadcast packet; distribute it
				for (j = 0; j < MAX_CONNECTED_PLAYERS; ++j)
				{
					if (   j != current
					    && connected_bsocket[j] != NULL
					    && connected_bsocket[j]->socket != NULL)
					{
						SDLNet_TCP_Send(connected_bsocket[j]->socket, pMsg, size);
					}
				}
			}
			else if (pMsg->destination != NetPlay.dpidPlayer)
			{
				// message was not meant for us; send it further
				if (   pMsg->destination < MAX_CONNECTED_PLAYERS
				    && connected_bsocket[pMsg->destination] != NULL
				    && connected_bsocket[pMsg->destination]->socket != NULL)
				{
					int result = 0;

					debug(LOG_NET, "Reflecting message type %hhu to %hhu", pMsg->type, pMsg->destination);
					pMsg->size = SDL_SwapBE16(pMsg->size);

					result = SDLNet_TCP_Send(connected_bsocket[pMsg->destination]->socket, pMsg, size);
					if (result < size)
					{
						debug(LOG_WARNING,"SDLNet_TCP_Send(line %d) failed, because of %s", __LINE__, SDLNet_GetError());
						connected_bsocket[pMsg->destination]->socket = NULL; // socket becomes invalid on error
					}
				}
				else
				{
					debug(LOG_NET, "Cannot reflect message type %hhu to %hhu", pMsg->type, pMsg->destination);
				}

				goto receive_message;
			}

			nStats.bytesRecvd   += size;
			nStats.packetsRecvd += 1;
		}

	} while (NETprocessSystemMessage() == true);

	NETlogPacket(pMsg, true);

	*type = pMsg->type;
	return true;
}

// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Protocol functions

BOOL NETsetupTCPIP(const char *machine)
{
	debug(LOG_NET, "NETsetupTCPIP(%s)", machine ? machine : "NULL");

	if (   hostname != NULL
	    && hostname != masterserver_name)
	{
		free(hostname);
	}
	if (   machine != NULL
	    && machine[0] != '\0')
	{
		hostname = strdup(machine);
	} else {
		hostname = masterserver_name;
	}

	return true;
}

// ////////////////////////////////////////////////////////////////////////
// File Transfer programs.
/** Send file. It returns % of file sent when 100 it's complete. Call until it returns 100. 
 * @TODO Needs to be rewritten. See issue #215. */
#define MAX_FILE_TRANSFER_PACKET 256
UBYTE NETsendFile(BOOL newFile, char *fileName, UDWORD player)
{
	static int32_t  	fileSize,currPos;
	static PHYSFS_file	*pFileHandle;
	int32_t  		bytesRead;
	char			inBuff[MAX_FILE_TRANSFER_PACKET];
	uint8_t			sendto = 0;

	memset(inBuff, 0x0, sizeof(inBuff));
	if (newFile)
	{
		// open the file.
		pFileHandle = PHYSFS_openRead(fileName);			// check file exists
		debug(LOG_WZ, "Reading...[directory: %s] %s", PHYSFS_getRealDir(fileName), fileName);
		if (pFileHandle == NULL)
		{
			debug(LOG_ERROR, "Failed to open %s for reading: %s", fileName, PHYSFS_getLastError());
			return 0; // failed
		}
		// get the file's size.
		fileSize = 0;
		currPos = 0;
		do
		{
			bytesRead = PHYSFS_read(pFileHandle, inBuff, 1, MAX_FILE_TRANSFER_PACKET);
			fileSize += bytesRead;
		} while(bytesRead != 0);

		PHYSFS_seek(pFileHandle, 0);
	}
	// read some bytes.
	if (!pFileHandle)
	{
		debug(LOG_ERROR, "No filehandle");
		return 0; // failed
	}
	bytesRead = PHYSFS_read(pFileHandle, inBuff,1, MAX_FILE_TRANSFER_PACKET);

	if (player == 0)
	{	// FIXME: why would you send (map) file to everyone ??
		// even if they already have it? multiplay.c 1529 & 1550 are both
		// NETsendFile(true,mapStr,0); & NETsendFile(false,game.map,0);
		// so we ALWAYS send it, it seems?
		NETbeginEncode(FILEMSG, NET_ALL_PLAYERS);	// send it.
	}
	else
	{
		sendto = (uint8_t) player;
		NETbeginEncode(FILEMSG,sendto);
	}

	// form a message
	NETint32_t(&fileSize);		// total bytes in this file.
	NETint32_t(&bytesRead);	// bytes in this packet
	NETint32_t(&currPos);		// start byte

	NETstring(fileName, 256);	//256 = max filename size
	NETbin(inBuff, bytesRead);
	NETend();

	currPos += bytesRead;		// update position!
	if(currPos == fileSize)
	{
		PHYSFS_close(pFileHandle);
	}

	return (currPos * 100) / fileSize;
}


// recv file. it returns % of the file so far recvd.
UBYTE NETrecvFile(void)
{
	int32_t		fileSize, currPos, bytesRead;
	char		fileName[256];
	char		outBuff[MAX_FILE_TRANSFER_PACKET];
	static PHYSFS_file	*pFileHandle;

	memset(fileName, 0x0, sizeof(fileName));
	memset(outBuff, 0x0, sizeof(outBuff));

	//read incoming bytes.
	NETbeginDecode(FILEMSG);
	NETint32_t(&fileSize);		// total bytes in this file.
	NETint32_t(&bytesRead);		// bytes in this packet
	NETint32_t(&currPos);		// start byte

	// read filename
	NETstring(fileName, 256);	// Ugh. 256 = max array size
	debug(LOG_NET, "Creating new file %s", fileName);

	if (currPos == 0)	// first packet!
	{
		pFileHandle = PHYSFS_openWrite(fileName);	// create a new file.
	}

	NETbin(outBuff, bytesRead);
	NETend();

	//write packet to the file.
	PHYSFS_write(pFileHandle, outBuff, bytesRead, 1);

	if (currPos+bytesRead == fileSize)	// last packet
	{
		PHYSFS_close(pFileHandle);
	}

	//return the percentage count
	return ((currPos + bytesRead) * 100) / fileSize;
}

static void NETregisterServer(int state)
{
	static TCPsocket rs_socket = NULL;
	static int registered = 0;
	static SDLNet_SocketSet masterset;
	IPaddress ip;
	int result = 0;

	if (server_not_there)
	{
		return;
	}

	if (state != registered)
	{
		switch(state)
		{
			case 1: {
				if(SDLNet_ResolveHost(&ip, masterserver_name, masterserver_port) == -1)
				{
					debug(LOG_ERROR, "Cannot resolve masterserver \"%s\": %s", masterserver_name, SDLNet_GetError());
					ssprintf(NetPlay.MOTDbuffer, _("Could not resolve masterserver name (%s)!"), masterserver_name);
					server_not_there = true;
					return;
				}

				if(!rs_socket) rs_socket = SDLNet_TCP_Open(&ip);
				if(rs_socket == NULL)
				{
					debug(LOG_ERROR, "Cannot connect to masterserver \"%s:%d\": %s", masterserver_name, masterserver_port, SDLNet_GetError());
					ssprintf(NetPlay.MOTDbuffer, _("Could not communicate with lobby server! Is TCP port %u open?"), masterserver_port);
					server_not_there = true;
					return;
				}
				// master server socket set to be friendly
				if (!masterset)
				{
					masterset = SDLNet_AllocSocketSet(1);
					if (!masterset)
					{
						debug(LOG_ERROR, "Couldn't create socket set for master server because: %s", SDLNet_GetError());
					}
					result = SDLNet_TCP_AddSocket(masterset, rs_socket);
					if (result != -1)
					{
						debug(LOG_NET,"TCP_AddSocket using socket set %p, socket %p", masterset, rs_socket);
					}
				}
				// get the MOTD from the server
				SDLNet_TCP_Send(rs_socket, (void*)"motd", sizeof("motd"));
				result = SDLNet_CheckSockets(masterset, 1000);
				if (result ==-1)
				{
					debug(LOG_NET, "Warning, MOTD CheckSockets Failed. Error %s", SDLNet_GetError());
				}
				if (SDLNet_SocketReady(rs_socket))	// true on activity
				{
					result = SDLNet_TCP_Recv(rs_socket, NetPlay.MOTDbuffer, sizeof(NetPlay.MOTDbuffer));
					if (result <= 0)
					{
						debug(LOG_NET, "Warning, MOTD TCP_Recv failed. result = %d, error %s", result,  SDLNet_GetError());
						sstrcpy(NetPlay.MOTDbuffer, "MOTD communication error with lobby server, TCP_Recv failed!");
					}
					debug(LOG_NET, "MOTD is [%s]", NetPlay.MOTDbuffer);
				}
				else
				{
					debug(LOG_NET, "Warning, MOTD Socket Not ready.  Is the motd.txt file empty?  Error %s", SDLNet_GetError());
					sstrcpy(NetPlay.MOTDbuffer, "MOTD communication error with lobby server, socket not ready?");
				}

				SDLNet_TCP_Send(rs_socket, (void*)"addg", sizeof("addg"));
				// and now send what the server wants
				NETsendGAMESTRUCT(rs_socket, &game);
			}
			break;

			case 0:
				// we don't need this anymore, so clean up
				result = SDLNet_TCP_DelSocket(masterset,rs_socket);
				if (result == -1)
				{
					debug(LOG_ERROR, "Couldn't delete socket set because: %s", SDLNet_GetError());
				}
				SDLNet_FreeSocketSet(masterset);
				masterset = NULL;
				SDLNet_TCP_Close(rs_socket);
				rs_socket=NULL;
			break;
		}
		registered=state;
	}
}


// ////////////////////////////////////////////////////////////////////////
// Host a game with a given name and player name. & 4 user game flags

static void NETallowJoining(void)
{
	unsigned int i;
	UDWORD numgames = SDL_SwapBE32(1);	// always 1 on normal server
	char buffer[5];
	int recv_result = 9999;

	if (allow_joining == false) return;

	NETregisterServer(1);

	// This is here since we need to get the status, before we can show the info.
	// FIXME: find better location to stick this?
	if (!NetPlay.ShowedMOTD)
	{
		ShowMOTD();
		NetPlay.ShowedMOTD = true;
	}

	if (tmp_socket_set == NULL)
	{
		int result = 0;

		// initialize server socket set
		// FIXME: why is this not done in NETinit()?? - Per
		tmp_socket_set = SDLNet_AllocSocketSet(MAX_TMP_SOCKETS+1);
		if (tmp_socket_set == NULL)
		{
			debug(LOG_ERROR, "Cannot create socket set: %s", SDLNet_GetError());
			return;
		}
		debug(LOG_NET, "Created tmp_socket_set %p", tmp_socket_set);

		result = SDLNet_TCP_AddSocket(tmp_socket_set, tcp_socket);
		if( result != -1)
		{
			debug(LOG_NET,"TCP_AddSocket using socket set %p, socket %p", tmp_socket_set, tcp_socket);
		}
		else
		{
			// Should never fail, or we will have major problems.
			debug(LOG_ERROR,"SDLNet_AddSocket: %s\n", SDLNet_GetError());
		}
	}

	if (SDLNet_CheckSockets(tmp_socket_set, NET_READ_TIMEOUT) > 0)
	{
		if (SDLNet_SocketReady(tcp_socket))
		{
			for (i = 0; i < MAX_TMP_SOCKETS; ++i)
			{
				if (tmp_socket[i] == NULL)
				{
					break;
				}
			}
			tmp_socket[i] = SDLNet_TCP_Accept(tcp_socket);
			debug(LOG_NET, "tmp_socket[%d]=%p Accepted", i, tmp_socket[i]);
			SDLNet_TCP_AddSocket(tmp_socket_set, tmp_socket[i]);
			if (SDLNet_CheckSockets(tmp_socket_set, 1000) > 0
			    && SDLNet_SocketReady(tmp_socket[i])
			    && (recv_result = SDLNet_TCP_Recv(tmp_socket[i], buffer, 5)))
			{
				if(strcmp(buffer, "list")==0)
				{
					int result = 0;
					debug(LOG_NET, "cmd: list.  Sending game list");
					result = SDLNet_TCP_Send(tmp_socket[i], &numgames, sizeof(numgames));
					if( result < sizeof(numgames) )
					{
						// TCP_Send error, inform user of problem.  This really should never happen normally.
						// Possible client disconnect, and in either case, socket is most likely invalid now.
						// How to handle error?
						debug(LOG_WARNING, "SDLNet_TCP_Send returned: %d error: %s line %d", result, SDLNet_GetError(),__LINE__);
						debug(LOG_WARNING, "Couldn't get list from server. Make sure required ports are open. (TCP 9998-9999)");
					}
					else
					{
						// get the correct player count after kicks / leaves
						game.desc.dwCurrentPlayers = NETplayerInfo();
						NETsendGAMESTRUCT(tmp_socket[i], &game);
					}

					SDLNet_TCP_DelSocket(tmp_socket_set, tmp_socket[i]);
					SDLNet_TCP_Close(tmp_socket[i]);
					tmp_socket[i] = NULL;
				}
				else if (strcmp(buffer, "join") == 0)
				{
					debug(LOG_NET, "cmd: join.  Sending GAMESTRUCT");
					NETsendGAMESTRUCT(tmp_socket[i], &game);
				}
				else
				{
					SDLNet_TCP_DelSocket(tmp_socket_set, tmp_socket[i]);
					SDLNet_TCP_Close(tmp_socket[i]);
					tmp_socket[i] = NULL;
				}
			}
			else
			{
				SDLNet_TCP_DelSocket(tmp_socket_set, tmp_socket[i]);
				SDLNet_TCP_Close(tmp_socket[i]);
				tmp_socket[i] = NULL;
			}
		}
		for(i = 0; i < MAX_TMP_SOCKETS; ++i)
		{
			if (   tmp_socket[i] != NULL
			    && SDLNet_SocketReady(tmp_socket[i]) > 0)
			{
				int size = SDLNet_TCP_Recv(tmp_socket[i], &NetMsg, sizeof(NetMsg));

				if (size <= 0)
				{
					// socket probably disconnected.
					debug(LOG_NET, "tmp socket %p probably disconnected", tmp_socket[i]);
					SDLNet_TCP_DelSocket(tmp_socket_set, tmp_socket[i]);
					SDLNet_TCP_Close(tmp_socket[i]);
					tmp_socket[i] = NULL;
				}
				else if (NetMsg.type == NET_JOIN)
				{
					char name[64];
					uint32_t j;
					uint8_t dpid;

					NETbeginDecode(NET_JOIN);
						NETstring(name, sizeof(name));
					NETend();

					dpid = NET_CreatePlayer(name, 0);

					SDLNet_TCP_DelSocket(tmp_socket_set, tmp_socket[i]);
					NET_initBufferedSocket(connected_bsocket[dpid], tmp_socket[i]);
					SDLNet_TCP_AddSocket(socket_set, connected_bsocket[dpid]->socket);
					tmp_socket[i] = NULL;

					debug(LOG_NET, "Player, %s, with dpid of %u has joined using socket %p", name,
									(unsigned int)dpid, connected_bsocket[dpid]->socket);

					// Increment player count
					game.desc.dwCurrentPlayers++;

					NETbeginEncode(NET_ACCEPTED, dpid);
						NETuint8_t(&dpid);
					NETend();

					MultiPlayerJoin(dpid);

					// Send info about players to newcomer.
					for (j = 0; j < MAX_CONNECTED_PLAYERS; ++j)
					{
						if (players[j].allocated
						 && dpid != players[j].id)
						{
							uint8_t id = players[j].id;
							NETbeginEncode(NET_PLAYER_JOINED, dpid);
								NETuint8_t(&id);
							NETend();
						}
					}

					// Send info about newcomer to all players.
					NETbeginEncode(NET_PLAYER_JOINED, NET_ALL_PLAYERS);
						NETuint8_t(&dpid);
					NETend();

					for (j = 0; j < MAX_CONNECTED_PLAYERS; ++j)
					{
						NETBroadcastPlayerInfo(j);
					}

					// Make sure the master server gets updated by disconnecting from it
					// NETallowJoining will reconnect
					NETregisterServer(0);
					// if this is a password locked game then ask for password.
					if (NetPlay.GamePassworded)
					{
						debug(LOG_NET, "Requesting password from %u", dpid);
						NETbeginEncode(NET_REQUEST_PASSWORD, dpid);
						NETend();
					}
					// and now, request version from new person
					NETbeginEncode(NET_REQUEST_VERSION, dpid);
					NETend();
					VersionCheckTime[dpid] = gameTime2;		//time we sent the msg
					// add 7 sec delay factor for lag/slow modems?
					VersionCheckTime[dpid] += (GAME_TICKS_PER_SEC * 7);
					debug(LOG_NET, "Requesting Version check @(+7) %u from %u", VersionCheckTime[dpid], dpid);
				}
			}
		}
	}
}

BOOL NEThostGame(const char* SessionName, const char* PlayerName,
		 SDWORD one, SDWORD two, SDWORD three, SDWORD four,
		 UDWORD plyrs)	// # of players.
{
	IPaddress ip;
	unsigned int i;

	debug(LOG_NET, "NEThostGame(%s, %s, %d, %d, %d, %d, %u)", SessionName, PlayerName,
	      one, two, three, four, plyrs);

	if(!NetPlay.bComms)
	{
		NetPlay.dpidPlayer		= HOST_DPID;
		NetPlay.bHost			= true;

		return true;
	}

	if(SDLNet_ResolveHost(&ip, NULL, gameserver_port) == -1)
	{
		debug(LOG_ERROR, "Cannot resolve master self: %s", SDLNet_GetError());
		return false;
	}
	// tcp_socket is the connection to the lobby server (or machine)
	if(!tcp_socket) tcp_socket = SDLNet_TCP_Open(&ip);
	if(tcp_socket == NULL)
	{
		debug(LOG_ERROR, "Cannot connect to master self: %s", SDLNet_GetError());
		return false;
	}
	debug(LOG_NET, "New tcp_socket = %p", tcp_socket);
	// Host needs to create a socket set for MAX_PLAYERS
	if(!socket_set) socket_set = SDLNet_AllocSocketSet(MAX_CONNECTED_PLAYERS);
	if (socket_set == NULL)
	{
		debug(LOG_ERROR, "Cannot create socket set: %s", SDLNet_GetError());
		return false;
	}
	SDLNet_TCP_AddSocket(socket_set, tcp_socket);
	// allocate socket storage for all possible players
	for (i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
	{
		connected_bsocket[i] = NET_createBufferedSocket();
	}

	is_server = true;

	sstrcpy(game.name, SessionName);
	memset(&game.desc, 0, sizeof(game.desc));
	game.desc.dwSize = sizeof(game.desc);
	//game.desc.guidApplication = GAME_GUID;
	game.desc.host[0] = '\0';
	game.desc.dwCurrentPlayers = 1;
	game.desc.dwMaxPlayers = plyrs;
	game.desc.dwFlags = 0;
	game.desc.dwUserFlags[0] = one;
	game.desc.dwUserFlags[1] = two;
	game.desc.dwUserFlags[2] = three;
	game.desc.dwUserFlags[3] = four;
	sstrcpy(game.misc, "Misc data" );					// misc string  (future use)
	sstrcpy(game.extra, "Extra");						// extra string (future use)
	sstrcpy(game.versionstring, VersionString);			// version (string)
	sstrcpy(game.modlist, "Mod list");					// List of mods
	game.GAMESTRUCT_VERSION = 2;						// version of this structure
	game.game_version_major = NETCODE_VERSION_MAJOR;	// Netcode Major version
	game.game_version_minor = NETCODE_VERSION_MINOR;	// NetCode Minor version
//	game.privateGame = 0;								// if true, it is a private game
	game.pureGame = 0;									// NO mods allowed if true
	game.Mods = 0;										// number of concatenated mods?
	game.future1 = 0xBAD01;								// for future use
	game.future2 = 0xBAD02;								// for future use
	game.future3 = 0xBAD03;								// for future use
	game.future4 = 0xBAD04;								// for future use

	NET_InitPlayers();
	NetPlay.dpidPlayer	= NET_CreatePlayer(PlayerName, PLAYER_HOST);
	NetPlay.bHost		= true;

	MultiPlayerJoin(NetPlay.dpidPlayer);

	allow_joining = true;

	NETregisterServer(0);

	debug(LOG_NET, "Hosting a server. We are player %d.", NetPlay.dpidPlayer);

	return true;
}

// ////////////////////////////////////////////////////////////////////////
// Stop the dplay interface from accepting more players.
BOOL NEThaltJoining(void)
{
	debug(LOG_NET, "temporarily locking game to prevent more players");

	allow_joining = false;
	// disconnect from the master server
	NETregisterServer(0);
	return true;
}

// ////////////////////////////////////////////////////////////////////////
// find games on open connection
BOOL NETfindGame(void)
{
	unsigned int gamecount = 0;
	uint32_t gamesavailable;
	IPaddress ip;
	unsigned int port = (hostname == masterserver_name) ? masterserver_port : gameserver_port;
	int result = 0;
	debug(LOG_NET, "Looking for games...");
	
	if (getLobbyError() > ERROR_CONNECTION)
	{
		return false;
	}
	setLobbyError(ERROR_NOERROR);

	NetPlay.games[0].desc.dwSize = 0;
	NetPlay.games[0].desc.dwCurrentPlayers = 0;
	NetPlay.games[0].desc.dwMaxPlayers = 0;

	if(!NetPlay.bComms)
	{
		NetPlay.dpidPlayer		= HOST_DPID;
		NetPlay.bHost			= true;
		return true;
	}
	// We first check to see if we were given a IP/hostname from the command line
	if (strlen(iptoconnect) )
	{
		if (SDLNet_ResolveHost(&ip, iptoconnect, port) == -1)
		{
			debug(LOG_ERROR, "Error connecting to client via hostname provided (%s)",iptoconnect);
			debug(LOG_ERROR, "Cannot resolve hostname :%s",SDLNet_GetError());
			setLobbyError(ERROR_CONNECTION);
			return false;
		}
		else
		{
			// We got a valid ip now
			hostname = strdup(iptoconnect);		//copy it
			memset(iptoconnect,0x0,sizeof(iptoconnect));	//reset it (so we don't loop back to this routine)
		}
	}
	else if (SDLNet_ResolveHost(&ip, hostname, port) == -1)
	{
		debug(LOG_ERROR, "Cannot resolve hostname \"%s\": %s", hostname, SDLNet_GetError());
		setLobbyError(ERROR_CONNECTION);
		return false;
	}

	if (tcp_socket != NULL)
	{
		debug(LOG_NET, "Deleting tcp_socket %p", tcp_socket);
		SDLNet_TCP_DelSocket(socket_set,tcp_socket);
		SDLNet_TCP_Close(tcp_socket);
		tcp_socket = NULL;
	}

	tcp_socket = SDLNet_TCP_Open(&ip);
	if (tcp_socket == NULL)
	{
		debug(LOG_ERROR, "Cannot connect to \"%s:%d\": %s", hostname, port, SDLNet_GetError());
		setLobbyError(ERROR_CONNECTION);
		return false;
	}
	debug(LOG_NET, "New tcp_socket = %p", tcp_socket);
	// client machines only need 1 socket set
	socket_set = SDLNet_AllocSocketSet(1);
	if (socket_set == NULL)
	{
		debug(LOG_ERROR, "Cannot create socket set: %s", SDLNet_GetError());
		setLobbyError(ERROR_CONNECTION);
		return false;
	}
	debug(LOG_NET, "Created socket_set %p", socket_set);

	SDLNet_TCP_AddSocket(socket_set, tcp_socket);

	debug(LOG_NET, "Sending list cmd");
	SDLNet_TCP_Send(tcp_socket, (void*)"list", sizeof("list"));

	if (SDLNet_CheckSockets(socket_set, 1000) > 0
	 && SDLNet_SocketReady(tcp_socket)
	 && (result = SDLNet_TCP_Recv(tcp_socket, &gamesavailable, sizeof(gamesavailable))))
	{
		gamesavailable = SDL_SwapBE32(gamesavailable);
	}
	else
	{
		if (result < 0)
		{
			debug(LOG_NET, "SDLNet_TCP_Recv returned %d, error: %s - tcp_socket %p is now invalid.",
			      result, SDLNet_GetError(), tcp_socket);
			tcp_socket = NULL; // unsure how to handle invalid sockets?
		}
		// when we fail to receive a game count, bail out
		setLobbyError(ERROR_CONNECTION);
		return false;
	}

	debug(LOG_NET, "receiving info on %u game(s)", (unsigned int)gamesavailable);

	do
	{
		// Attempt to receive a game description structure
		if (!NETrecvGAMESTRUCT(&NetPlay.games[gamecount]))
			// If we fail, success depends on the amount of games that we've read already
			return gamecount;

		if (NetPlay.games[gamecount].desc.host[0] == '\0')
		{
			unsigned char* address = (unsigned char*)(&(ip.host));

			snprintf(NetPlay.games[gamecount].desc.host, sizeof(NetPlay.games[gamecount].desc.host),
			"%i.%i.%i.%i",
				(int)(address[0]),
				(int)(address[1]),
				(int)(address[2]),
				(int)(address[3]));
		}

		++gamecount;
	} while (gamecount < gamesavailable);

	return true;
}

// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Functions used to setup and join games.
BOOL NETjoinGame(UDWORD gameNumber, const char* playername)
{
	IPaddress ip;

	debug(LOG_NET, "resetting sockets.");
	NETclose();	// just to be sure :)

	debug(LOG_NET, "Trying to join gameNumber (%u)...", gameNumber);

	if (hostname != masterserver_name)
	{
		free(hostname);
	}
	hostname = strdup(NetPlay.games[gameNumber].desc.host);

	if(SDLNet_ResolveHost(&ip, hostname, gameserver_port) == -1)
	{
		debug(LOG_ERROR, "Cannot resolve hostname \"%s\": %s", hostname, SDLNet_GetError());
		return false;
	}

	if (tcp_socket != NULL)
	{
		SDLNet_TCP_Close(tcp_socket);
	}

	tcp_socket = SDLNet_TCP_Open(&ip);
 	if (tcp_socket == NULL)
	{
		debug(LOG_ERROR, "Cannot connect to \"%s:%d\": %s", hostname, gameserver_port, SDLNet_GetError());
		return false;
	}
	// client machines only need 1 socket set
	socket_set = SDLNet_AllocSocketSet(1);
	if (socket_set == NULL)
	{
		debug(LOG_ERROR, "Cannot create socket set: %s", SDLNet_GetError());
 		return false;
 	}
	debug(LOG_NET, "Created socket_set %p", socket_set);

	// tcp_socket is used to talk to host machine
	SDLNet_TCP_AddSocket(socket_set, tcp_socket);

	SDLNet_TCP_Send(tcp_socket, (void*)"join", sizeof("join"));

	if (NETrecvGAMESTRUCT(&NetPlay.games[gameNumber])
	 && NetPlay.games[gameNumber].desc.host[0] == '\0')
	{
		unsigned char* address = (unsigned char*)(&(ip.host));

		snprintf(NetPlay.games[gameNumber].desc.host, sizeof(NetPlay.games[gameNumber].desc.host),
			"%i.%i.%i.%i",
			(int)(address[0]),
			(int)(address[1]),
			(int)(address[2]),
			(int)(address[3]));
	}
	// Allocate memory for a new socket
	bsocket = NET_createBufferedSocket();
	// NOTE: tcp_socket = bsocket->socket now!
	NET_initBufferedSocket(bsocket, tcp_socket);

	// Send a join message
	NETbeginEncode(NET_JOIN, HOST_DPID);
		// Casting constness away, because NETstring is const-incorrect
		// when sending/encoding a packet.
		NETstring((char*)playername, 64);
	NETend();

	// Loop until we've been accepted into the game
	for (;;)
	{
		uint8_t type = NUM_GAME_PACKETS;

		NETrecv(&type);

		if (type == NET_ACCEPTED)
		{
			uint8_t dpid;
			NETbeginDecode(NET_ACCEPTED);
				// Retrieve the player ID the game host arranged for us
				NETuint8_t(&dpid);
			NETend();

			NetPlay.dpidPlayer = dpid;
			debug(LOG_NET, "NET_ACCEPTED received. Accepted into the game - I'm player %u using bsocket %p, tcp_socket=%p",
				(unsigned int)NetPlay.dpidPlayer, bsocket->socket, tcp_socket);
			NetPlay.bHost = false;

			if (NetPlay.dpidPlayer >= MAX_CONNECTED_PLAYERS)
			{
				debug(LOG_ERROR, "Bad player number (%u) received from host!", NetPlay.dpidPlayer);
				return false;
			}

			players[NetPlay.dpidPlayer].allocated = true;
			players[NetPlay.dpidPlayer].id = NetPlay.dpidPlayer;
			sstrcpy(players[NetPlay.dpidPlayer].name, playername);
			players[NetPlay.dpidPlayer].flags = 0;
			players[NetPlay.dpidPlayer].heartbeat = true;

			return true;
		}
	}
}

/*!
 * Set the masterserver name
 * \param hostname The hostname of the masterserver to connect to
 */
void NETsetMasterserverName(const char* hostname)
{
	sstrcpy(masterserver_name, hostname);
}

/**
 * @return The hostname of the masterserver we will connect to.
 */
const char* NETgetMasterserverName()
{
	return masterserver_name;
}

/*!
 * Set the masterserver port
 * \param port The port of the masterserver to connect to
 */
void NETsetMasterserverPort(unsigned int port)
{
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
