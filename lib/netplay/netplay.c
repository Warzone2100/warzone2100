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

#include "lib/framework/frame.h"

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
#define NET_BUFFER_SIZE		1024

// HACK to allow us to call a src/multistat.c function upon receiving a MSG_PLAYER_STATS msg
extern void recvMultiStats(void);

enum
{
	MSG_PLAYER_STATS = 12, // HACK-NOTE: This __must__ be the same to NET_PLAYER_STATS in src/multiplay.h
	MSG_JOIN = 90, // needs to start at 90
	MSG_ACCEPTED,
	MSG_PLAYER_INFO,
	MSG_PLAYER_JOINED,
	MSG_PLAYER_LEFT,
	MSG_GAME_FLAGS,
};

// ////////////////////////////////////////////////////////////////////////
// Function prototypes

static void NETallowJoining(void);
extern BOOL MultiPlayerJoin(UDWORD dpid);  /* from src/multijoin.c ! */
extern BOOL MultiPlayerLeave(UDWORD dpid); /* from src/multijoin.c ! */

/*
 * Network globals, these are part of the new network API
 */
NETMSG NetMsg;
static PACKETDIR NetDir;

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
	uint32_t        id;
	BOOL            allocated;
	char            name[StringSize];
	uint32_t        flags;
} NET_PLAYER;

typedef struct
{
	TCPsocket	socket;
	char*		buffer;
	unsigned int	buffer_start;
	unsigned int	bytes;
} NETBUFSOCKET;

/// This is the hardcoded dpid (player ID) value for the hosting player.
#define HOST_DPID 1

#define PLAYER_HOST		1

// ////////////////////////////////////////////////////////////////////////
// Variables

NETPLAY	NetPlay;

static BOOL		allow_joining = FALSE;
static GAMESTRUCT	game;
static TCPsocket	tcp_socket = NULL;
static NETBUFSOCKET*	bsocket = NULL;
static NETBUFSOCKET*	connected_bsocket[MAX_CONNECTED_PLAYERS] = { NULL };
static SDLNet_SocketSet	socket_set = NULL;
static BOOL		is_server = FALSE;
static TCPsocket	tmp_socket[MAX_TMP_SOCKETS] = { NULL };
static SDLNet_SocketSet	tmp_socket_set = NULL;
static char*		hostname;
static NETSTATS		nStats = { 0, 0, 0, 0 };
static NET_PLAYER	players[MAX_CONNECTED_PLAYERS];
static int32_t          NetGameFlags[4];

// *********** Socket with buffer that read NETMSGs ******************

static NETBUFSOCKET* NET_createBufferedSocket(void)
{
	NETBUFSOCKET* bs = (NETBUFSOCKET*)malloc(sizeof(NETBUFSOCKET));

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
		return FALSE;
	}

	if (SDLNet_SocketReady(bs->socket) <= 0)
	{
		return FALSE;
	}

	size = SDLNet_TCP_Recv(bs->socket, bufstart, bufsize);

	if (size > 0)
	{
		bs->bytes += size;
		return TRUE;
	} else {
		if (socket_set != NULL)
		{
			SDLNet_TCP_DelSocket(socket_set, bs->socket);
		}
		SDLNet_TCP_Close(bs->socket);
		bs->socket = NULL;
	}

	return FALSE;
}

// Check if we have a full message waiting for us. If not, return FALSE and wait for more data.
// If there is a data remnant somewhere in the buffer except at its beginning, move it to the
// beginning.
static BOOL NET_recvMessage(NETBUFSOCKET* bs, NETMSG* pMsg)
{
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

	size = message->size + headersize;

	if (size > bs->bytes)
	{
		goto error;
	}

	memcpy(pMsg, message, size);
	bs->buffer_start += size;
	bs->bytes -= size;

	return TRUE;

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

		// Now data is in the beginning of the buffer
		bs->buffer_start = 0;
	}

	return FALSE;
}

static void NET_InitPlayers(void)
{
	unsigned int i;

	for (i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
	{
		players[i].allocated = FALSE;
		players[i].id = i;
	}
}

static void NETBroadcastPlayerInfo(int dpid)
{
	NET_PLAYER* player = (NET_PLAYER*)NetMsg.body;

	NetMsg.type = MSG_PLAYER_INFO;
	NetMsg.size = sizeof(NET_PLAYER);
	memcpy(NetMsg.body, &players[dpid], sizeof(NET_PLAYER));

	player->id    = SDL_SwapBE32(player->id);
	player->flags = SDL_SwapBE32(player->flags);

	NETbcast(&NetMsg, TRUE);
}

static unsigned int NET_CreatePlayer(const char* name, unsigned int flags)
{
	unsigned int i;

	for (i = 1; i < MAX_CONNECTED_PLAYERS; ++i)
	{
		if (players[i].allocated == FALSE)
		{
			players[i].allocated = TRUE;
			strlcpy(players[i].name, name, sizeof(players[i].name));
			players[i].flags = flags;
			NETBroadcastPlayerInfo(i);
			return i;
		}
	}

	return 0;
}

static void NET_DestroyPlayer(unsigned int id)
{
	players[id].allocated = FALSE;
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
		NetPlay.players[0].bHost	= TRUE;
		NetPlay.players[0].dpid		= HOST_DPID;
		return 1;
	}

	memset(NetPlay.players, 0, sizeof(PLAYER) * MAX_PLAYERS);	// reset player info

	for (i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
	{
		if (players[i].allocated == TRUE)
		{
			NetPlay.players[NetPlay.playercount].dpid = i;
			strlcpy(NetPlay.players[NetPlay.playercount].name, players[i].name, sizeof(NetPlay.players[NetPlay.playercount].name));

			if (players[i].flags & PLAYER_HOST)
			{
				NetPlay.players[NetPlay.playercount].bHost = TRUE;
			}
			else
			{
				NetPlay.players[NetPlay.playercount].bHost = FALSE;
			}

			NetPlay.playercount++;
		}
	}

	return NetPlay.playercount;
}

// ////////////////////////////////////////////////////////////////////////
// rename the local player
BOOL NETchangePlayerName(UDWORD dpid, char *newName)
{
	if(!NetPlay.bComms)
	{
		strlcpy(NetPlay.players[0].name, newName, sizeof(NetPlay.players[0].name));
		return TRUE;
	}
	debug(LOG_NET, "Requesting a change of player name for pid=%d to %s", dpid, newName);

	strlcpy(players[dpid].name, newName, sizeof(players[dpid].name));

	NETBroadcastPlayerInfo(dpid);

	return TRUE;
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
	NETbeginEncode(MSG_GAME_FLAGS, NET_ALL_PLAYERS);
	{
		// Send the amount of game flags we're about to send
		uint8_t i, count = sizeof(NetGameFlags) / sizeof(*NetGameFlags);
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
		return TRUE;
	}

	if (flag < 1 || flag > 4)
	{
		return NetGameFlags[flag-1] = value;
	}

	NETsendGameFlags();

	return TRUE;
}

static void NETsendGAMESTRUCT(TCPsocket socket, const GAMESTRUCT* game)
{
	// A buffer that's guaranteed to have the correct size (i.e. it
	// circumvents struct padding, which could pose a problem).
	char buf[sizeof(game->name) + sizeof(game->desc.host) + sizeof(int32_t) * 8];
	char* buffer = buf;;

	// Now dump the data into the buffer
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
	*(int32_t*)buffer = SDL_SwapBE32(game->desc.dwUser1);
	buffer += sizeof(int32_t);
	*(int32_t*)buffer = SDL_SwapBE32(game->desc.dwUser2);
	buffer += sizeof(int32_t);
	*(int32_t*)buffer = SDL_SwapBE32(game->desc.dwUser3);
	buffer += sizeof(int32_t);
	*(int32_t*)buffer = SDL_SwapBE32(game->desc.dwUser4);
	buffer += sizeof(int32_t);

	// Send over the GAMESTRUCT
	SDLNet_TCP_Send(socket, buf, sizeof(buf));
}

static bool NETrecvGAMESTRUCT(GAMESTRUCT* game)
{
	// A buffer that's guaranteed to have the correct size (i.e. it
	// circumvents struct padding, which could pose a problem).
	char buf[sizeof(game->name) + sizeof(game->desc.host) + sizeof(int32_t) * 8];
	char* buffer = buf;

	// Read a GAMESTRUCT from the connection
	if (tcp_socket == NULL
	 || socket_set == NULL
	 || SDLNet_CheckSockets(socket_set, 1000) <= 0
	 || !SDLNet_SocketReady(tcp_socket)
	 || SDLNet_TCP_Recv(tcp_socket, buf, sizeof(buf)) != sizeof(buf))
	{
		return false;
	}

	// Now dump the data into the game struct
	// Copy a string
	strlcpy(game->name, buffer, sizeof(game->name));
	buffer += sizeof(game->name);

	// Copy 32bit large big endian numbers
	game->desc.dwSize = SDL_SwapBE32(*(int32_t*)buffer);
	buffer += sizeof(int32_t);
	game->desc.dwFlags = SDL_SwapBE32(*(int32_t*)buffer);
	buffer += sizeof(int32_t);

	// Copy yet another string
	strlcpy(game->desc.host, buffer, sizeof(game->desc.host));
	buffer += sizeof(game->desc.host);

	// Copy 32bit large big endian numbers
	game->desc.dwMaxPlayers = SDL_SwapBE32(*(int32_t*)buffer);
	buffer += sizeof(int32_t);
	game->desc.dwCurrentPlayers = SDL_SwapBE32(*(int32_t*)buffer);
	buffer += sizeof(int32_t);
	game->desc.dwUser1 = SDL_SwapBE32(*(int32_t*)buffer);
	buffer += sizeof(int32_t);
	game->desc.dwUser2 = SDL_SwapBE32(*(int32_t*)buffer);
	buffer += sizeof(int32_t);
	game->desc.dwUser3 = SDL_SwapBE32(*(int32_t*)buffer);
	buffer += sizeof(int32_t);
	game->desc.dwUser4 = SDL_SwapBE32(*(int32_t*)buffer);
	buffer += sizeof(int32_t);

	return true;
}

// ////////////////////////////////////////////////////////////////////////
// setup stuff
BOOL NETinit(BOOL bFirstCall)
{
	UDWORD i;
	debug( LOG_NET, "NETinit" );

	if(bFirstCall)
	{
		debug(LOG_NET, "NETPLAY: Init called, MORNIN'");

		NetPlay.dpidPlayer		= 0;
		NetPlay.bHost			= 0;
		NetPlay.bComms			= TRUE;

		for(i = 0; i < MAX_PLAYERS; i++)
		{
			memset(&NetPlay.players[i], 0, sizeof(PLAYER));
			memset(&NetPlay.games[i], 0, sizeof(GAMESTRUCT));
		}
		NetPlay.bComms = TRUE;
		NETstartLogging();
	}

	if (SDLNet_Init() == -1)
	{
		debug(LOG_ERROR, "SDLNet_Init: %s", SDLNet_GetError());
		return FALSE;
	}

	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////
// SHUTDOWN THE CONNECTION.
BOOL NETshutdown(void)
{
	debug( LOG_NET, "NETshutdown" );

	NETstopLogging();
	SDLNet_Quit();
	return 0;
}

// ////////////////////////////////////////////////////////////////////////
//close the open game..
BOOL NETclose(void)
{
	unsigned int i;

	debug(LOG_NET, "NETclose");

	NEThaltJoining();
	is_server=FALSE;

	if(bsocket)
	{
		NET_destroyBufferedSocket(bsocket);
		bsocket=NULL;
	}

	for(i=0;i<MAX_CONNECTED_PLAYERS;i++) {
		if(connected_bsocket[i]) {
			NET_destroyBufferedSocket(connected_bsocket[i]);
			connected_bsocket[i]=NULL;
		}
		NET_DestroyPlayer(i);
	}

	if(tmp_socket_set)
	{
		SDLNet_FreeSocketSet(tmp_socket_set);
		tmp_socket_set=NULL;
	}

	for (i = 0; i < MAX_TMP_SOCKETS; i++)
	{
		if (tmp_socket[i])
		{
			SDLNet_TCP_Close(tmp_socket[i]);
			tmp_socket[i]=NULL;
		}
	}

	if (socket_set)
	{
		SDLNet_FreeSocketSet(socket_set);
		socket_set=NULL;
	}

	if (tcp_socket)
	{
		SDLNet_TCP_Close(tcp_socket);
		tcp_socket=NULL;
	}
	return FALSE;
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
BOOL NETsend(NETMSG *msg, UDWORD player, BOOL guarantee)
{
	int size;

	if(!NetPlay.bComms)
	{
		return TRUE;
	}

	if (player >= MAX_CONNECTED_PLAYERS) return FALSE;
	msg->destination = player;
	msg->source = selectedPlayer;

	size = msg->size + sizeof(msg->size) + sizeof(msg->type) + sizeof(msg->destination) + sizeof(msg->source);

	NETlogPacket(msg, FALSE);

	if (is_server)
	{
		if (   player < MAX_CONNECTED_PLAYERS
		    && connected_bsocket[player] != NULL
		    && connected_bsocket[player]->socket != NULL
		    && SDLNet_TCP_Send(connected_bsocket[player]->socket,
				       msg, size) == size)
		{
			nStats.bytesSent   += size;
			nStats.packetsSent += 1;
			return TRUE;
		}
	} else {
		if (   tcp_socket
		    && SDLNet_TCP_Send(tcp_socket, msg, size) == size)
		{
			return TRUE;
		}
	}

	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////
// broadcast a message to all players.
BOOL NETbcast(NETMSG *msg, BOOL guarantee)
{
	int size;

	if(!NetPlay.bComms)
	{
		return TRUE;
	}

	msg->destination = NET_ALL_PLAYERS;
	msg->source = selectedPlayer;

	size = msg->size + sizeof(msg->size) + sizeof(msg->type) + sizeof(msg->destination) + sizeof(msg->source);

	NETlogPacket(msg, FALSE);

	if (is_server)
	{
		unsigned int i;

		for (i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
		{
			if (   connected_bsocket[i] != NULL
			    && connected_bsocket[i]->socket != NULL)
			{
				SDLNet_TCP_Send(connected_bsocket[i]->socket, msg, size);
			}
		}

	} else {
		if (   tcp_socket == NULL
		    || SDLNet_TCP_Send(tcp_socket, msg, size) < size)
		{
			return FALSE;
		}
	}

	nStats.bytesSent   += size;
	nStats.packetsSent += 1;

	return TRUE;
}

///////////////////////////////////////////////////////////////////////////
// Check if a message is a system message
BOOL NETprocessSystemMessage(NETMSG * pMsg)
{
	switch (pMsg->type)
	{
		case MSG_PLAYER_STATS:
		{
			recvMultiStats();
			break;
		}
		case MSG_PLAYER_INFO: {
			NET_PLAYER* pi = (NET_PLAYER*)(pMsg->body);
			unsigned int dpid;

			pi->id    = SDL_SwapBE32(pi->id);
			pi->flags = SDL_SwapBE32(pi->flags);

			dpid = pi->id;

			debug(LOG_NET, "NETprocessSystemMessage: Receiving MSG_PLAYER_INFO for player %d from player %d. We are %d.",
			      dpid, pMsg->source, NetPlay.dpidPlayer);

			// Bail out if the given ID number is out of range
			if (dpid >= MAX_CONNECTED_PLAYERS)
			{
				debug(LOG_ERROR, "NETprocessSystemMessage: MSG_PLAYER_INFO: Player ID (%u) out of range (max %u)",
				      dpid, MAX_CONNECTED_PLAYERS);
				NETend();
				break;
			}

			memcpy(&players[dpid], pi, sizeof(NET_PLAYER));
			NETplayerInfo();

			if (is_server)
			{
				NETBroadcastPlayerInfo(dpid);
			}
			break;
		}
		case MSG_PLAYER_JOINED:
		{
			uint8_t dpid;
			NETbeginDecode();
				NETuint8_t(&dpid);
			NETend();

			debug(LOG_NET, "NETprocessSystemMessage: Receiving MSG_PLAYER_JOINED for player %u", (unsigned int)dpid);

			MultiPlayerJoin(dpid);
			break;
		}
		case MSG_PLAYER_LEFT:
		{
			uint32_t dpid;
			NETbeginDecode();
				NETuint32_t(&dpid);
			NETend();

			debug(LOG_NET, "NETprocessSystemMessage: Receiving MSG_PLAYER_LEFT for player %u", (unsigned int)dpid);

			NET_DestroyPlayer(dpid);
			MultiPlayerLeave(dpid);
			break;
		}
		case MSG_GAME_FLAGS:
		{
			debug(LOG_NET, "NETprocessSystemMessage: Receiving game flags");

			NETbeginDecode();
			{
				static unsigned int max_flags = sizeof(NetGameFlags) / sizeof(*NetGameFlags);
				// Retrieve the amount of game flags that we should receive
				uint8_t i, count;
				NETuint8_t(&count);

				// Make sure that we won't get buffer overflows by checking that we
				// have enough space to store the given amount of game flags.
				if (count > max_flags)
				{
					debug(LOG_NET, "NETprocessSystemMessage: MSG_GAME_FLAGS: More game flags sent (%u) than our buffer can hold (%u)", (unsigned int)count, max_flags);
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
		default:
			return FALSE;
	}

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
// Receive a message over the current connection. We return TRUE if there
// is a message for the higher level code to process, and FALSE otherwise.
// We should not block here.
BOOL NETrecv(NETMSG * pMsg)
{
	static unsigned int current = 0;
	BOOL received;
	int size;

	if (!NetPlay.bComms)
	{
		return FALSE;
	}

	if (is_server)
	{
		NETallowJoining();
	}

	do {
receive_message:
		received = FALSE;

		if (is_server)
		{
			if (connected_bsocket[current] == NULL)
			{
				return FALSE;
			}

			received = NET_recvMessage(connected_bsocket[current], pMsg);

			if (received == FALSE)
			{
				uint32_t i = current + 1;

				if (socket_set == NULL
				    || SDLNet_CheckSockets(socket_set, NET_READ_TIMEOUT) <= 0)
				{
					return FALSE;
				}
				for (;;)
				{
					if (connected_bsocket[i]->socket == NULL)
					{
						// do nothing
					}
					else if (NET_fillBuffer(connected_bsocket[i], socket_set))
					{
						// we received some data, add to buffer
						received = NET_recvMessage(connected_bsocket[i], pMsg);
						current = i;
						break;
					}
					else if (connected_bsocket[i]->socket == NULL)
					{
						// check if we dropped any players in the check above

						// Decrement player count
						--game.desc.dwCurrentPlayers;

						debug(LOG_NET, "NETrecv: dpid to send set to %u", (unsigned int)i);
						NETbeginEncode(MSG_PLAYER_LEFT, NET_ALL_PLAYERS);
							NETuint32_t(&i);
						NETend();

						NET_DestroyPlayer(i);
						MultiPlayerLeave(i);
					}

					if (++i == MAX_CONNECTED_PLAYERS)
					{
						i = 0;
					}

					if (i == current+1)
					{
						return FALSE;
					}
				}
			}
		} else {
			// we are a client
			if (bsocket == NULL)
			{
				return FALSE;
			} else {
				received = NET_recvMessage(bsocket, pMsg);

				if (received == FALSE)
				{
					if (   socket_set != NULL
					    && SDLNet_CheckSockets(socket_set, NET_READ_TIMEOUT) > 0
					    && NET_fillBuffer(bsocket, socket_set))
					{
						received = NET_recvMessage(bsocket, pMsg);
					}
				}
			}
		}

		if (received == FALSE)
		{
			return FALSE;
		}
		else
		{
			size =	  pMsg->size + sizeof(pMsg->size) + sizeof(pMsg->type)
				+ sizeof(pMsg->destination) + sizeof(pMsg->source);
			if (is_server == FALSE)
			{
				// do nothing
			}
			else if (pMsg->destination == NET_ALL_PLAYERS)
			{
				unsigned int j;

				// we are the host, and have received a broadcast packet; distribute it
				for (j = 0; j < MAX_CONNECTED_PLAYERS; ++j)
				{
					if (   j != current
					    && connected_bsocket[j] != NULL
					    && connected_bsocket[j]->socket != NULL)
					{
						SDLNet_TCP_Send(connected_bsocket[j]->socket,
								pMsg, size);
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
					debug(LOG_NET, "Reflecting message type %hhu to UDWORD %hhu", pMsg->type, pMsg->destination);
					SDLNet_TCP_Send(connected_bsocket[pMsg->destination]->socket,
							pMsg, size);
				} else {
					debug(LOG_NET, "Cannot reflect message type %hhu to %hhu", pMsg->type, pMsg->destination);
				}

				goto receive_message;
			}

			nStats.bytesRecvd   += size;
			nStats.packetsRecvd += 1;
		}

	} while (NETprocessSystemMessage(pMsg) == TRUE);

	NETlogPacket(pMsg, TRUE);

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Protocol functions

BOOL NETsetupTCPIP(void ** addr, const char * machine)
{
	debug(LOG_NET, "NETsetupTCPIP(,%s)", machine ? machine : "NULL");

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

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
// File Transfer programs.
// send file. it returns % of file sent. when 100 it's complete. call until it returns 100.
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
		if (pFileHandle == NULL)
		{
			debug(LOG_ERROR, "NETsendFile: Failed");
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
	bytesRead = PHYSFS_read(pFileHandle, inBuff,1, MAX_FILE_TRANSFER_PACKET);

	if (player == 0)
	{	// FIXME: why would you send (map) file to everyone ??
		// even if they already have it? multiplay.c 1529 & 1550 are both
		// NETsendFile(TRUE,mapStr,0); & NETsendFile(FALSE,game.map,0);
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
	NETbeginDecode();
	NETint32_t(&fileSize);		// total bytes in this file.
	NETint32_t(&bytesRead);		// bytes in this packet
	NETint32_t(&currPos);		// start byte

	// read filename
	NETstring(fileName, 256);	// Ugh. 256 = max array size
	debug(LOG_NET, "NETrecvFile: Creating new file %s", fileName);

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

void NETregisterServer(int state)
{
	static TCPsocket rs_socket = NULL;
	static int registered = 0;
	static int server_not_there = 0;
	IPaddress ip;

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
					debug(LOG_ERROR, "NETregisterServer: Cannot resolve masterserver \"%s\": %s", masterserver_name, SDLNet_GetError());
					server_not_there = 1;
					return;
				}

				if(!rs_socket) rs_socket = SDLNet_TCP_Open(&ip);
				if(rs_socket == NULL)
				{
					debug(LOG_ERROR, "NETregisterServer: Cannot connect to masterserver \"%s:%d\": %s", masterserver_name, masterserver_port, SDLNet_GetError());
					server_not_there = 1;
					return;
				}

				SDLNet_TCP_Send(rs_socket, "addg", 5);
				NETsendGAMESTRUCT(rs_socket, &game);
			}
			break;

			case 0:
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

	if (allow_joining == FALSE) return;

	NETregisterServer(1);

	if (tmp_socket_set == NULL)
	{
		// initialize server socket set
		// FIXME: why is this not done in NETinit()?? - Per
		tmp_socket_set = SDLNet_AllocSocketSet(MAX_TMP_SOCKETS+1);
		if (tmp_socket_set == NULL)
		{
			debug(LOG_ERROR, "NETallowJoining: Cannot create socket set: %s", SDLNet_GetError());
			return;
		}
		SDLNet_TCP_AddSocket(tmp_socket_set, tcp_socket);
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
			SDLNet_TCP_AddSocket(tmp_socket_set, tmp_socket[i]);
			if (SDLNet_CheckSockets(tmp_socket_set, 1000) > 0
			    && SDLNet_SocketReady(tmp_socket[0])
			    && SDLNet_TCP_Recv(tmp_socket[i], buffer, 5))
			{
				if(strcmp(buffer, "list")==0)
				{
					SDLNet_TCP_Send(tmp_socket[i], &numgames, sizeof(UDWORD));
					NETsendGAMESTRUCT(tmp_socket[i], &game);
				}
				else if (strcmp(buffer, "join") == 0)
				{
					NETsendGAMESTRUCT(tmp_socket[i], &game);
				}

			} else {
				return;
			}
		}
		for(i = 0; i < MAX_TMP_SOCKETS; ++i)
		{
			if (   tmp_socket[i] != NULL
			    && SDLNet_SocketReady(tmp_socket[i]) > 0)
			{
				int size = SDLNet_TCP_Recv(tmp_socket[i], &NetMsg, sizeof(NETMSG));

				if (size <= 0)
				{
					// socket probably disconnected.
					SDLNet_TCP_DelSocket(tmp_socket_set, tmp_socket[i]);
					SDLNet_TCP_Close(tmp_socket[i]);
					tmp_socket[i] = NULL;
				}
				else if (NetMsg.type == MSG_JOIN)
				{
					char name[64];
					int j;
					uint8_t dpid;

					NETbeginDecode();
						NETstring(name, sizeof(name));
					NETend();

					dpid = NET_CreatePlayer(name, 0);

					debug(LOG_NET, "NETallowJoining, MSG_JOIN: dpid set to %u", (unsigned int)dpid);
					SDLNet_TCP_DelSocket(tmp_socket_set, tmp_socket[i]);
					NET_initBufferedSocket(connected_bsocket[dpid], tmp_socket[i]);
					SDLNet_TCP_AddSocket(socket_set, connected_bsocket[dpid]->socket);
					tmp_socket[i] = NULL;

					// Increment player count
					game.desc.dwCurrentPlayers++;

					NETbeginEncode(MSG_ACCEPTED, dpid);
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
							NETbeginEncode(MSG_PLAYER_JOINED, dpid);
								NETuint8_t(&id);
							NETend();
						}
					}

					// Send info about newcomer to all players.
					NETbeginEncode(MSG_PLAYER_JOINED, NET_ALL_PLAYERS);
						NETuint8_t(&dpid);
					NETend();

					for (j = 0; j < MAX_CONNECTED_PLAYERS; ++j)
					{
						NETBroadcastPlayerInfo(j);
					}

					// Make sure the master server gets updated by disconnecting from it
					// NETallowJoining will reconnect
					NETregisterServer(0);
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
		NetPlay.bHost			= TRUE;

		return TRUE;
	}

	if(SDLNet_ResolveHost(&ip, NULL, gameserver_port) == -1)
	{
		debug(LOG_ERROR, "NEThostGame: Cannot resolve master self: %s", SDLNet_GetError());
		return FALSE;
	}

	if(!tcp_socket) tcp_socket = SDLNet_TCP_Open(&ip);
	if(tcp_socket == NULL)
	{
		printf("NEThostGame: Cannot connect to master self: %s", SDLNet_GetError());
		return FALSE;
	}

	if(!socket_set) socket_set = SDLNet_AllocSocketSet(MAX_CONNECTED_PLAYERS);
	if (socket_set == NULL)
	{
		debug(LOG_ERROR, "NEThostGame: Cannot create socket set: %s", SDLNet_GetError());
		return FALSE;
	}
	for (i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
	{
		connected_bsocket[i] = NET_createBufferedSocket();
	}

	is_server = TRUE;

	strlcpy(game.name, SessionName, sizeof(game.name));
	memset(&game.desc, 0, sizeof(SESSIONDESC));
	game.desc.dwSize = sizeof(SESSIONDESC);
	//game.desc.guidApplication = GAME_GUID;
	game.desc.host[0] = '\0';
	game.desc.dwCurrentPlayers = 1;
	game.desc.dwMaxPlayers = plyrs;
	game.desc.dwFlags = 0;
	game.desc.dwUser1 = one;
	game.desc.dwUser2 = two;
	game.desc.dwUser3 = three;
	game.desc.dwUser4 = four;

	NET_InitPlayers();
	NetPlay.dpidPlayer	= NET_CreatePlayer(PlayerName, PLAYER_HOST);
	NetPlay.bHost		= TRUE;

	MultiPlayerJoin(NetPlay.dpidPlayer);

	allow_joining = TRUE;

	NETregisterServer(0);

	debug(LOG_NET, "Hosting a server. We are player %d.", NetPlay.dpidPlayer);

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
// Stop the dplay interface from accepting more players.
BOOL NEThaltJoining(void)
{
	debug(LOG_NET, "NEThaltJoining");

	allow_joining = FALSE;
	// disconnect from the master server
	NETregisterServer(0);
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
// find games on open connection
BOOL NETfindGame(void)
{
	unsigned int gamecount = 0;
	uint32_t gamesavailable;
	IPaddress ip;
	unsigned int port = (hostname == masterserver_name) ? masterserver_port : gameserver_port;

	debug(LOG_NET, "NETfindGame");

	NetPlay.games[0].desc.dwSize = 0;
	NetPlay.games[0].desc.dwCurrentPlayers = 0;
	NetPlay.games[0].desc.dwMaxPlayers = 0;

	if(!NetPlay.bComms)
	{
		NetPlay.dpidPlayer		= HOST_DPID;
		NetPlay.bHost			= TRUE;
		return TRUE;
	}

	if (SDLNet_ResolveHost(&ip, hostname, port) == -1)
	{
		debug(LOG_ERROR, "NETfindGame: Cannot resolve hostname \"%s\": %s", hostname, SDLNet_GetError());
		return FALSE;
	}

	if (tcp_socket != NULL)
	{
		SDLNet_TCP_Close(tcp_socket);
	}

	tcp_socket = SDLNet_TCP_Open(&ip);
	if (tcp_socket == NULL)
	{
		debug(LOG_ERROR, "NETfindGame: Cannot connect to \"%s:%d\": %s", hostname, port, SDLNet_GetError());
		return FALSE;
	}

	socket_set = SDLNet_AllocSocketSet(1);
	if (socket_set == NULL)
	{
		debug(LOG_ERROR, "NETfindGame: Cannot create socket set: %s", SDLNet_GetError());
		return FALSE;
	}
	SDLNet_TCP_AddSocket(socket_set, tcp_socket);

	SDLNet_TCP_Send(tcp_socket, "list", 5);

	if (SDLNet_CheckSockets(socket_set, 1000) > 0
	 && SDLNet_SocketReady(tcp_socket)
	 && SDLNet_TCP_Recv(tcp_socket, &gamesavailable, sizeof(gamesavailable)))
	{
		gamesavailable = SDL_SwapBE32(gamesavailable);
	}
	else
	{
		// when we fail to receive a game count, bail out
		return FALSE;
	}

	debug(LOG_NET, "receiving info of %u game(s)", (unsigned int)gamesavailable);

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

			// Guarantee to nul-terminate
			NetPlay.games[gamecount].desc.host[sizeof(NetPlay.games[gamecount].desc.host) - 1] = '\0';
		}

		++gamecount;
	} while (gamecount < gamesavailable);

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Functions used to setup and join games.
BOOL NETjoinGame(UDWORD gameNumber, const char* playername)
{
	IPaddress ip;

	debug(LOG_NET, "NETjoinGame gameNumber=%d", gameNumber);

	NETclose();	// just to be sure :)

	if (hostname != masterserver_name)
	{
		free(hostname);
	}
	hostname = strdup(NetPlay.games[gameNumber].desc.host);

	if(SDLNet_ResolveHost(&ip, hostname, gameserver_port) == -1)
	{
		debug(LOG_ERROR, "NETjoinGame: Cannot resolve hostname \"%s\": %s", hostname, SDLNet_GetError());
		return FALSE;
	}

	if (tcp_socket != NULL)
	{
		SDLNet_TCP_Close(tcp_socket);
	}

	tcp_socket = SDLNet_TCP_Open(&ip);
 	if (tcp_socket == NULL)
	{
		debug(LOG_ERROR, "NETjoinGame: Cannot connect to \"%s:%d\": %s", hostname, gameserver_port, SDLNet_GetError());
		return FALSE;
	}

	socket_set = SDLNet_AllocSocketSet(1);
	if (socket_set == NULL)
	{
		debug(LOG_ERROR, "NETjoinGame: Cannot create socket set: %s", SDLNet_GetError());
 		return FALSE;
 	}
	SDLNet_TCP_AddSocket(socket_set, tcp_socket);

	SDLNet_TCP_Send(tcp_socket, "join", 5);

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

	bsocket = NET_createBufferedSocket();
	NET_initBufferedSocket(bsocket, tcp_socket);

	// Send a join message
	NETbeginEncode(MSG_JOIN, HOST_DPID);
		// Casting constness away, because NETstring is const-incorrect
		// when sending/encoding a packet.
		NETstring((char*)playername, 64);
	NETend();

	// Loop until we've been accepted into the game
	for (;;)
	{
		NETrecv(&NetMsg);

		if (NetMsg.type == MSG_ACCEPTED)
		{
			uint8_t dpid;
			NETbeginDecode();
				// Retrieve the player ID the game host arranged for us
				NETuint8_t(&dpid);
			NETend();

			NetPlay.dpidPlayer = dpid;
			debug(LOG_NET, "NETjoinGame: MSG_ACCEPTED received. Accepted into the game - I'm player %u",
			      (unsigned int)NetPlay.dpidPlayer);
			NetPlay.bHost = FALSE;

			if (NetPlay.dpidPlayer >= MAX_CONNECTED_PLAYERS)
			{
				debug(LOG_ERROR, "Bad player number (%u) received from host!", NetPlay.dpidPlayer);
				return FALSE;
			}

			players[NetPlay.dpidPlayer].allocated = TRUE;
			players[NetPlay.dpidPlayer].id = NetPlay.dpidPlayer;
			strlcpy(players[NetPlay.dpidPlayer].name, playername, sizeof(players[NetPlay.dpidPlayer].name));
			players[NetPlay.dpidPlayer].flags = 0;

			return TRUE;
		}
	}

	return FALSE;
}

void NETsetPacketDir(PACKETDIR dir)
{
    NetDir = dir;
}

PACKETDIR NETgetPacketDir()
{
    return NetDir;
}

/*!
 * Set the masterserver name
 * \param hostname The hostname of the masterserver to connect to
 */
void NETsetMasterserverName(const char* hostname)
{
	strlcpy(masterserver_name, hostname, sizeof(masterserver_name));
}


/*!
 * Set the masterserver port
 * \param port The port of the masterserver to connect to
 */
void NETsetMasterserverPort(unsigned int port)
{
	masterserver_port = port;
}


/*!
 * Set the port we shall host games on
 * \param port The port to listen to
 */
void NETsetGameserverPort(unsigned int port)
{
	gameserver_port = port;
}
