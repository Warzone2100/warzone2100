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
 * Netplay.c
 *
 * Alex Lee sep97-> june98
 */

// ////////////////////////////////////////////////////////////////////////
// includes

#include "lib/framework/frame.h"

#include <time.h>			// for stats
#include <SDL/SDL_thread.h>
#ifdef WZ_OS_MAC
#include <SDL_net/SDL_net.h>
#else
#include <SDL/SDL_net.h>
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

#define MSG_JOIN		90
#define MSG_ACCEPTED		91
#define MSG_PLAYER_INFO		92
#define MSG_PLAYER_DATA		93
#define MSG_PLAYER_JOINED	94
#define MSG_PLAYER_LEFT		95
#define MSG_GAME_FLAGS		96

//#define NET_DEBUG

static BOOL allow_joining = FALSE;

static void NETallowJoining(void);
BOOL MultiPlayerJoin(UDWORD dpid);
BOOL MultiPlayerLeave(UDWORD dpid);

// ////////////////////////////////////////////////////////////////////////
// Variables

NETPLAY	NetPlay;
static GAMESTRUCT game;

void NETsetMessageSize(NETMSG* pMsg, unsigned int size) {
	unsigned int tmp = 8 - (size % 8);

	if (tmp == 8) {
		pMsg->size = size;
		pMsg->paddedBytes = 0;
	} else {
		pMsg->size = size + tmp;
		pMsg->paddedBytes = tmp;
	}
}

// *********** Socket with buffer that read NETMSGs ******************

typedef struct {
	TCPsocket	socket;
	char*		buffer;
	unsigned int	buffer_start;
	unsigned int	bytes;
} NETBUFSOCKET;

static NETBUFSOCKET* NET_createBufferedSocket(void) {
	NETBUFSOCKET* bs = (NETBUFSOCKET*)malloc(sizeof(NETBUFSOCKET));

	bs->socket = NULL;
	bs->buffer = NULL;
	bs->buffer_start = 0;
	bs->bytes = 0;

	return bs;
}

void NET_destroyBufferedSocket(NETBUFSOCKET* bs) {
	free(bs->buffer);
	free(bs);
}

void NET_initBufferedSocket(NETBUFSOCKET* bs, TCPsocket s) {
	bs->socket = s;
	if (bs->buffer == NULL) {
		bs->buffer = (char*)malloc(NET_BUFFER_SIZE);
	}
	bs->buffer_start = 0;
	bs->bytes = 0;
}

BOOL NET_fillBuffer(NETBUFSOCKET* bs, SDLNet_SocketSet socket_set)
{
	int size;
	char* bufstart = bs->buffer + bs->buffer_start + bs->bytes;
	const int bufsize = NET_BUFFER_SIZE - bs->buffer_start - bs->bytes;


	if (bs->buffer_start != 0) {
		return FALSE;
	}

	if (SDLNet_SocketReady(bs->socket) <= 0) {
		return FALSE;
	}



	size = SDLNet_TCP_Recv(bs->socket, bufstart, bufsize);

	if (size > 0) {
		bs->bytes += size;
		return TRUE;
	} else {
		if (socket_set != NULL) {
			SDLNet_TCP_DelSocket(socket_set, bs->socket);
		}
		SDLNet_TCP_Close(bs->socket);
		bs->socket = NULL;
	}

	return FALSE;
}

BOOL NET_recvMessage(NETBUFSOCKET* bs, NETMSG* pMsg)
{
	unsigned int size;
	const NETMSG* message = (NETMSG*)(bs->buffer + bs->buffer_start);
	const unsigned int headersize =   sizeof(message->size)
					+ sizeof(message->type)
					+ sizeof(message->paddedBytes)
					+ sizeof(message->destination);

	if (headersize > bs->bytes) {
		goto error;
	}

	size = message->size + headersize;

	if (size > bs->bytes) {
		goto error;
	}

	debug( LOG_NET, "NETrecvMessage: received message of type %i and size %i.\n", message->type, message->size );

	memcpy(pMsg, message, size);
	bs->buffer_start += size;
	bs->bytes -= size;

	return TRUE;

error:
	if (bs->buffer_start != 0) {
		static char* tmp_buffer = NULL;
		char* buffer_start = bs->buffer + bs->buffer_start;
		char* tmp;

		//printf("Moving data in buffer\n");

		// Create tmp buffer if necessary
		if (tmp_buffer == NULL) {
			//printf("Creating tmp buffer\n");
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

static TCPsocket	tcp_socket = NULL;
static NETBUFSOCKET*	bsocket = NULL;
static NETBUFSOCKET*	connected_bsocket[MAX_CONNECTED_PLAYERS] = { NULL };
static SDLNet_SocketSet	socket_set = NULL;
static BOOL		is_server = FALSE;

static TCPsocket tmp_socket[MAX_TMP_SOCKETS] = { 0 };
static SDLNet_SocketSet tmp_socket_set = NULL;

static NETMSG		message;

static char*		hostname;

// ////////////////////////////////////////////////////////////////////////
typedef struct {			// data regarding the last one second or so.
	UDWORD		bytesRecvd;
	UDWORD		bytesSent;	// number of bytes sent in about 1 sec.
	UDWORD		packetsSent;
	UDWORD		packetsRecvd;
} NETSTATS;

static NETSTATS	nStats = { 0, 0, 0, 0 };

#define PLAYER_HOST		1
#define PLAYER_SPECTATOR	2

// ////////////////////////////////////////////////////////////////////////
// Players stuff

typedef struct {
	BOOL		allocated;
	unsigned int	id;
	char		name[StringSize];
	unsigned int	flags;
} NET_PLAYER;

static NET_PLAYER	players[MAX_CONNECTED_PLAYERS];

static void NET_InitPlayers(void) {
	unsigned int i;

	for (i = 0; i < MAX_CONNECTED_PLAYERS; ++i) {
		players[i].allocated = FALSE;
		players[i].id = i;
	}
}

void NETBroadcastPlayerInfo(int dpid) {
	message.type = MSG_PLAYER_INFO;
	NETsetMessageSize(&message, sizeof(NET_PLAYER));
	memcpy(message.body, &players[dpid], sizeof(NET_PLAYER));
	NETbcast(&message, TRUE);
}

unsigned int NET_CreatePlayer(const char* name, unsigned int flags) {
	unsigned int i;

	for (i = 1; i < MAX_CONNECTED_PLAYERS; ++i) {
		if (players[i].allocated == FALSE) {
			players[i].allocated = TRUE;
			strcpy(players[i].name, name);
			players[i].flags = flags;
			NETBroadcastPlayerInfo(i);
			return i;
		}
	}

	return 0;
}

void NET_DestroyPlayer(unsigned int id) {
	players[id].allocated = FALSE;
}

// ////////////////////////////////////////////////////////////////////////
// count players. call with null to enumerate the game already joined.
UDWORD NETplayerInfo(void)
{
	unsigned int i;

	NetPlay.playercount = 0;		// reset player counter

	if(!NetPlay.bComms)
	{
		NetPlay.playercount		= 1;
		NetPlay.players[0].bHost	= TRUE;
		NetPlay.players[0].bSpectator	= FALSE;
		NetPlay.players[0].dpid		= 1;
		return 1;
	}

	memset(NetPlay.players, 0, (sizeof(PLAYER)*MaxNumberOfPlayers));	// reset player info

	for (i = 0; i < MAX_CONNECTED_PLAYERS; ++i) {
		if (players[i].allocated == TRUE) {
			NetPlay.players[NetPlay.playercount].dpid = i;
			strcpy(NetPlay.players[NetPlay.playercount].name, players[i].name);
			if (players[i].flags & PLAYER_HOST) {
				NetPlay.players[NetPlay.playercount].bHost = TRUE;
			} else {
				NetPlay.players[NetPlay.playercount].bHost = FALSE;
			}
			if (players[i].flags & PLAYER_SPECTATOR) {
				NetPlay.players[NetPlay.playercount].bSpectator = TRUE;
			} else {
				NetPlay.players[NetPlay.playercount].bSpectator = FALSE;
			}
			NetPlay.playercount++;
		}
	}

	return NetPlay.playercount;
}

// ////////////////////////////////////////////////////////////////////////
// rename the local player
// dont call this a lot, since it uses a guaranteed msg.
BOOL NETchangePlayerName(UDWORD dpid, char *newName)
{
	if(!NetPlay.bComms)
	{
		strcpy(NetPlay.players[0].name, newName);
		return TRUE;
	}

	strcpy(players[dpid].name, newName);

	NETBroadcastPlayerInfo(dpid);

	return TRUE;
}

typedef struct {
	unsigned int	size;
	void*		data;
	unsigned int	buffer_size;
} NET_PLAYER_DATA;

static NET_PLAYER_DATA	local_player_data[MAX_CONNECTED_PLAYERS] = { { 0, NULL, 0 } };
static NET_PLAYER_DATA	global_player_data[MAX_CONNECTED_PLAYERS] = { { 0, NULL, 0 } };

static void resize_local_player_data(unsigned int i, unsigned int size) {
	if (local_player_data[i].buffer_size < size) {
		if (local_player_data[i].data != NULL) {
			free(local_player_data[i].data);
		}
		local_player_data[i].data = malloc(size);
		local_player_data[i].buffer_size = size;
	}
}

static void resize_global_player_data(unsigned int i, unsigned int size) {
	if (global_player_data[i].buffer_size < size) {
		if (global_player_data[i].data != NULL) {
			free(global_player_data[i].data);
		}
		global_player_data[i].data = malloc(size);
		global_player_data[i].buffer_size = size;
	}
}

// ////////////////////////////////////////////////////////////////////////
BOOL NETgetLocalPlayerData(UDWORD dpid, void *pData, SDWORD *pSize)
{
	memcpy(pData, local_player_data[dpid].data, local_player_data[dpid].size);
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
BOOL NETgetGlobalPlayerData(UDWORD dpid, void *pData, SDWORD *pSize)
{
	if(!NetPlay.bComms)
	{
		memcpy(pData, local_player_data[dpid].data, local_player_data[dpid].size);
		return TRUE;
	}

	memcpy(pData, global_player_data[dpid].data, global_player_data[dpid].size);

	return TRUE;
}
// ////////////////////////////////////////////////////////////////////////
BOOL NETsetLocalPlayerData(UDWORD dpid,void *pData, SDWORD size)
{
	local_player_data[dpid].size = size;
	resize_local_player_data(dpid, size);
	memcpy(local_player_data[dpid].data, pData, size);
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
BOOL NETsetGlobalPlayerData(UDWORD dpid, void *pData, SDWORD size)
{
	if(!NetPlay.bComms)
	{
		local_player_data[dpid].size = size;
		resize_local_player_data(dpid, size);
		memcpy(local_player_data[dpid].data, pData, size);
		return TRUE;
	}

	global_player_data[dpid].size = size;
	resize_global_player_data(dpid, size);
	memcpy(global_player_data[dpid].data, pData, size);

	{ // broadcast player data
		unsigned int* p_dpid = (unsigned int*)(message.body);

		message.type = MSG_PLAYER_DATA;
		NETsetMessageSize(&message, sizeof(unsigned int)+size);
		*p_dpid = dpid;
		memcpy(message.body+sizeof(unsigned int), pData, size);
	}

	NETBroadcastPlayerInfo(dpid);

	return TRUE;
}





// ///////////////////////////////////////////////////////////////////////
// Game flags stuff...

SDWORD NetGameFlags[4];

// ////////////////////////////////////////////////////////////////////////
// return one of the four user flags in the current sessiondescription.
SDWORD NETgetGameFlags(UDWORD flag)
{
	if (flag < 1 || flag > 4) {
		return 0;
	} else {
		return NetGameFlags[flag-1];
	}
}

// ////////////////////////////////////////////////////////////////////////
// Set a game flag
BOOL NETsetGameFlags(UDWORD flag, SDWORD value)
{
	if(!NetPlay.bComms) {
		return TRUE;
	}

	if (flag < 1 || flag > 4) {
		return NetGameFlags[flag-1] = value;
	}

	message.type = MSG_GAME_FLAGS;
	NETsetMessageSize(&message, sizeof(NetGameFlags));
	memcpy(message.body, NetGameFlags, sizeof(NetGameFlags));
	NETbcast(&message, TRUE);

	return TRUE;
}







// ////////////////////////////////////////////////////////////////////////
// setup stuff
BOOL NETinit(BOOL bFirstCall)
{
	UDWORD i;
	debug( LOG_NET, "NETinit" );

	if(bFirstCall)
	{
		debug( LOG_NEVER, "NETPLAY: Init called, MORNIN'\n" );

		NetPlay.bLobbyLaunched		= FALSE;				// clean up
		NetPlay.dpidPlayer		= 0;
		NetPlay.bHost			= 0;
		NetPlay.bComms			= TRUE;

		NetPlay.bEncryptAllPackets	= FALSE;
		NETsetKey(0x2fe8f810, 0xb72a5, 0x114d0, 0x2a7);	// j-random key to get us started

		NetPlay.bAllowCaptureRecord	= FALSE;
		NetPlay.bAllowCapturePlay	= FALSE;
		NetPlay.bCaptureInUse		= FALSE;


		for(i=0;i<MaxNumberOfPlayers;i++)
		{
			memset(&NetPlay.players[i], 0, sizeof(PLAYER));
			memset(&NetPlay.games[i], 0, sizeof(GAMESTRUCT));
		}
		//GAME_GUID = g;

		NETuseNetwork(TRUE);
		NETstartLogging();
	}

	if (SDLNet_Init() == -1) {
		debug( LOG_ERROR, "SDLNet_Init: %s\n", SDLNet_GetError() );
		return FALSE;
	}

	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////
// SHUTDOWN THE CONNECTION.
BOOL NETshutdown(void)
{
	unsigned int i;
	debug( LOG_NET, "NETshutdown" );

	for( i = 0; i < MAX_CONNECTED_PLAYERS; i++ )
	{
		if( local_player_data[i].data != NULL )
		{
			free( local_player_data[i].data );
		}
	}

	NETstopLogging();
	SDLNet_Quit();
	return 0;
}

// ////////////////////////////////////////////////////////////////////////
//close the open game..
BOOL NETclose(void)
{
	unsigned int i;

	debug( LOG_NET, "NETclose\n" );

	NEThaltJoining();
	is_server=FALSE;

	if(bsocket) {
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

	if(tmp_socket_set) {
		SDLNet_FreeSocketSet(tmp_socket_set);
		tmp_socket_set=NULL;
	}

	for(i=0;i<MAX_TMP_SOCKETS;i++) {
		if(tmp_socket[i]) {
			SDLNet_TCP_Close(tmp_socket[i]);
			tmp_socket[i]=NULL;
		}
	}

	if(socket_set) {
		SDLNet_FreeSocketSet(socket_set);
		socket_set=NULL;
	}

	if(tcp_socket) {
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

UDWORD NETgetRecentPacketsRecvd(void)
{
	return nStats.packetsRecvd;
}


// ////////////////////////////////////////////////////////////////////////
// Send a message to a player, option to guarantee message
BOOL NETsend(NETMSG *msg, UDWORD player, BOOL guarantee)
{
	int size;

	debug( LOG_NET, "NETsend\n" );

	if(!NetPlay.bComms)
	{
		return TRUE;
	}

	if (player >= MAX_CONNECTED_PLAYERS) return FALSE;
	//printf("Sending message %i to %i\n", msg->type, msg->destination);
	msg->destination = player;

	if (   NetPlay.bEncryptAllPackets
	    && (msg->type!=AUDIOMSG)
	    && (msg->type!=FILEMSG)) {
		NETmanglePacket(msg);
	}

//printf("NETsend %i\n", msg->type);

	size = msg->size + sizeof(msg->size) + sizeof(msg->type) + sizeof(msg->paddedBytes) + sizeof(msg->destination);

	if (is_server) {
		if (   player < MAX_CONNECTED_PLAYERS
		    && connected_bsocket[player] != NULL
		    && connected_bsocket[player]->socket != NULL
		    && SDLNet_TCP_Send(connected_bsocket[player]->socket,
				       msg, size) == size) {
			nStats.bytesSent   += size;
			nStats.packetsSent += 1;
			return TRUE;
		}
	} else {
		if (   tcp_socket
		    && SDLNet_TCP_Send(tcp_socket, msg, size) == size) {
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

	debug( LOG_NET, "NETbcast\n");

	msg->destination = NET_ALL_PLAYERS;

	if (   NetPlay.bEncryptAllPackets
	    && (msg->type!=AUDIOMSG)
	    && (msg->type!=FILEMSG)) {
		NETmanglePacket(msg);
	}

	size = msg->size + sizeof(msg->size) + sizeof(msg->type) + sizeof(msg->paddedBytes) + sizeof(msg->destination);

//printf("NETbcast %i\n", msg->type);

	if (is_server) {
		unsigned int i;

		for (i = 0; i < MAX_CONNECTED_PLAYERS; ++i) {
			if (   connected_bsocket[i] != NULL
			    && connected_bsocket[i]->socket != NULL) {
				SDLNet_TCP_Send(connected_bsocket[i]->socket, msg, size);
			}
		}

	} else {
		if (   tcp_socket == NULL
		    || SDLNet_TCP_Send(tcp_socket, msg, size) < size) {
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
	debug( LOG_NET, "NETprocessSystemMessage\n" );

	switch (pMsg->type) {
		case MSG_PLAYER_INFO: {
			NET_PLAYER* pi = (NET_PLAYER*)(pMsg->body);
			int dpid = pi->id;

			//printf("Receiving info for player %i\n", dpid);

			memcpy(&players[dpid], pi, sizeof(NET_PLAYER));
			NETplayerInfo();

			if (is_server) {
				NETBroadcastPlayerInfo(dpid);
			}

			//printf("Just received and processed MSG_PLAYER_INFO\n");
		}	break;

		case MSG_PLAYER_DATA: {
			unsigned int* pdpid = (unsigned int*)(pMsg->body);
			int dpid = *pdpid;
			unsigned int size = pMsg->size - sizeof(unsigned int);

			//printf("Receiving DATA for player %i\n", dpid);

			global_player_data[dpid].size = size;
			resize_global_player_data(dpid, size);
			memcpy(global_player_data[dpid].data, pMsg->body+sizeof(unsigned int), size);

			if (is_server) {
				NETbcast(pMsg, TRUE);
			}

			//printf("Just received and processed MSG_PLAYER_DATA\n");
		}	break;

		case MSG_PLAYER_JOINED: {
			unsigned int* pdpid = (unsigned int*)(pMsg->body);
			int dpid = *pdpid;

			MultiPlayerJoin(dpid);
		}	break;

		case MSG_PLAYER_LEFT: {
			unsigned int* pdpid = (unsigned int*)(pMsg->body);
			int dpid = *pdpid;

			NET_DestroyPlayer(dpid);
			MultiPlayerLeave(dpid);
		}	break;

		case MSG_GAME_FLAGS: {
			//printf("Receiving game flags\n");

			memcpy(NetGameFlags, pMsg->body, sizeof(NetGameFlags));

			if (is_server) {
				NETbcast(pMsg, TRUE);
			}

			//printf("Just received and processed MSG_GAME_FLAGS\n");
		}	break;
		default:
			return FALSE;
	}

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
// receive a message over the current connection

BOOL NETrecv(NETMSG * pMsg)
{
	static unsigned int current = 0;
	BOOL received;
	int size;

	if (!NetPlay.bComms)
	{
		return FALSE;
	}

	if (is_server) {
		NETallowJoining();
	}

	do {
receive_message:
		received = FALSE;
		//printf("Receiving something\n");

		if (is_server) {
			if (connected_bsocket[current] == NULL) return FALSE;

			received = NET_recvMessage(connected_bsocket[current], pMsg);

			if (received == FALSE) {
				unsigned int i = current + 1;

				if (   socket_set == NULL
				    || SDLNet_CheckSockets(socket_set, NET_READ_TIMEOUT) <= 0) {
					return FALSE;
				}
				for (;;) {
					if (connected_bsocket[i]->socket == NULL) {
					} else if (NET_fillBuffer(connected_bsocket[i], socket_set)) {
						received = NET_recvMessage(connected_bsocket[i], pMsg);
						current = i;
						break;
					} else if (connected_bsocket[i]->socket == NULL) {
						unsigned int* message_dpid = (unsigned int*)(message.body);

						game.desc.dwCurrentPlayers--;

						message.type = MSG_PLAYER_LEFT;
						NETsetMessageSize(&message, 4);
						*message_dpid = i;
						NETbcast(&message, TRUE);

						NET_DestroyPlayer(i);
						MultiPlayerLeave(i);
					}
					if (++i == MAX_CONNECTED_PLAYERS) {
						i = 0;
					}
					if (i == current+1) {
						return FALSE;
					}
				}
			}
		} else {
			if (bsocket == NULL) {
				return FALSE;
			} else {
				received = NET_recvMessage(bsocket, pMsg);

				if (received == FALSE) {
					if (   socket_set != NULL
					    && SDLNet_CheckSockets(socket_set, NET_READ_TIMEOUT) > 0
					    && NET_fillBuffer(bsocket, socket_set)) {
						received = NET_recvMessage(bsocket, pMsg);
					}
				}
			}
		}

		if (received == FALSE) {
			return FALSE;
		} else {
			size =	  pMsg->size + sizeof(pMsg->size) + sizeof(pMsg->type)
				+ sizeof(pMsg->paddedBytes) + sizeof(pMsg->destination);
			if (is_server == FALSE) {
			} else if (pMsg->destination == NET_ALL_PLAYERS) {
				unsigned int j;

				for (j = 0; j < MAX_CONNECTED_PLAYERS; ++j) {
					if (   j != current
					    && connected_bsocket[j] != NULL
					    && connected_bsocket[j]->socket != NULL) {
						SDLNet_TCP_Send(connected_bsocket[j]->socket,
								pMsg, size);
					}
				}
			} else if (pMsg->destination != NetPlay.dpidPlayer) {
				if (   pMsg->destination < MAX_CONNECTED_PLAYERS
				    && connected_bsocket[pMsg->destination] != NULL
				    && connected_bsocket[pMsg->destination]->socket != NULL) {
					//printf("Reflecting message to UDWORD %i\n", pMsg->destination);
					SDLNet_TCP_Send(connected_bsocket[pMsg->destination]->socket,
							pMsg, size);
				} else {
					//printf("Cannot reflect message %i to UDWORD %i\n", pMsg->type, pMsg->destination);
				}

				goto receive_message;
			}

			if (   (pMsg->type>=ENCRYPTFLAG)
			    && (pMsg->type!=AUDIOMSG)
			    && (pMsg->type!=FILEMSG)) {
				NETunmanglePacket(pMsg);
			}

			//printf("Received message : type = %i, size = %i, size = %i.\n", pMsg->type, pMsg->size, size);

			nStats.bytesRecvd   += size;
			nStats.packetsRecvd += 1;
		}

	} while (NETprocessSystemMessage(pMsg) == TRUE);

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Protocol functions

BOOL NETsetupTCPIP(void ** addr, char * machine)
{
	debug( LOG_NET, "NETsetupTCPIP\n" );

	if (   hostname != NULL
	    && hostname != masterserver_name) {
		free(hostname);
	}
	if (   machine != NULL
	    && machine[0] != '\0') {
		hostname = strdup(machine);
	} else {
		hostname = masterserver_name;
	}

	return TRUE;
}

// select a current protocol. Call with a connection returned by findprotocol.
BOOL NETselectProtocol(void * lpConnection)
{
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
// call with true to enumerate available protocols.
BOOL NETfindProtocol(BOOL Lob)
{
	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////
// File Transfer programs.
// uses guaranteed messages to send files between clients.

// send file. it returns % of file sent. when 100 it's complete. call until it returns 100.

UBYTE NETsendFile(BOOL newFile, const char *fileName, UDWORD player)
{
	static PHYSFS_sint64	fileSize,currPos;
	static PHYSFS_file	*pFileHandle;
	PHYSFS_sint64		bytesRead;
	UBYTE		inBuff[2048];
	NETMSG		msg;

	if(newFile)		//force it true for now -Q
	{
		// open the file.
		pFileHandle = PHYSFS_openRead(fileName);			// check file exists
		if (pFileHandle == NULL)
		{
			debug( LOG_WZ, "NETsendFile: Failed\n" );
			return 0;															// failed
		}
		// get the file's size.
		fileSize = 0;
		currPos =0;
		do
		{
			bytesRead = PHYSFS_read( pFileHandle, &inBuff, 1, sizeof(inBuff) );
			fileSize += bytesRead;
		} while(bytesRead != 0);

		PHYSFS_seek(pFileHandle, 0 );
	}
	// read some bytes.
	bytesRead = PHYSFS_read(pFileHandle, &inBuff, sizeof(inBuff), 1 );

	// form a message
	NetAdd(msg,0,fileSize);		// total bytes in this file.
	NetAdd(msg,4,bytesRead);	// bytes in this packet
	NetAdd(msg,8,currPos);		// start byte
	msg.body[12]=strlen(fileName);
	msg.size	= 13;
	NetAddSt(msg,msg.size,fileName);
	msg.size	+= strlen(fileName);
	memcpy(&(msg.body[msg.size]),&inBuff,bytesRead);
	msg.size	+= bytesRead;
	msg.type	=  FILEMSG;
	msg.paddedBytes = 0;
	if(player==0)
	{
		NETbcast(&msg,TRUE);		// send it.
	}
	else
	{
		NETsend(&msg,player,TRUE);
	}

	currPos += bytesRead;		// update position!
	if(currPos == fileSize)
	{
		PHYSFS_close(pFileHandle);
	}

	return (currPos*100)/fileSize;
}


// recv file. it returns % of the file so far recvd.
UBYTE NETrecvFile(NETMSG *pMsg)
{
	UDWORD			pos, fileSize, currPos, bytesRead;
	char			fileName[256];
	unsigned int		len;
	static PHYSFS_file	*pFileHandle;

	//read incoming bytes.
	NetGet(pMsg,0,fileSize);
	NetGet(pMsg,4,bytesRead);
	NetGet(pMsg,8,currPos);

	// read filename
	len = (unsigned int)(pMsg->body[12]);
	memcpy(fileName,&(pMsg->body[13]),len);
	fileName[len] = '\0';	// terminate string.
	pos = 13+len;
	//printf("NETrecvFile: Creating new file %s\n", fileName);

	if(currPos == 0)	// first packet!
	{
		//printf("NETrecvFile: Creating new file %s\n", fileName);
		pFileHandle = PHYSFS_openWrite(fileName);	// create a new file.
	}

	//write packet to the file.
	PHYSFS_write( pFileHandle, &(pMsg->body[pos]), bytesRead, 1 );

	if(currPos+bytesRead == fileSize)	// last packet
	{
		PHYSFS_close(pFileHandle);
	}

	//return the percent count.
	return ((currPos+bytesRead)*100) / fileSize;
}

void NETregisterServer(int state) {
	static TCPsocket rs_socket = NULL;
	static int registered = 0;
	static int server_not_there = 0;
	IPaddress ip;

	if (server_not_there) {
		return;
	}

	if (state != registered) {
		switch(state) {
			case 1: {
				if(SDLNet_ResolveHost(&ip, masterserver_name, masterserver_port) == -1) {
					debug( LOG_ERROR, "NETregisterServer: Cannot resolve masterserver \"%s\": %s\n", masterserver_name, SDLNet_GetError() );
					server_not_there = 1;
					return;
				}

				if(!rs_socket) rs_socket = SDLNet_TCP_Open(&ip);
				if(rs_socket == NULL) {
					debug( LOG_ERROR, "NETregisterServer: Cannot connect to masterserver \"%s:%d\": %s\n", masterserver_name, masterserver_port, SDLNet_GetError() );
					server_not_there = 1;
					return;
				}

				SDLNet_TCP_Send(rs_socket, "addg", 5);
				SDLNet_TCP_Send(rs_socket, &game, sizeof(GAMESTRUCT));
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

static void NETallowJoining(void) {
	unsigned int i;
	const UDWORD numgames = SDL_SwapBE32(1);	// always 1 on normal server
	char buffer[5];

	if (allow_joining == FALSE) return;

	NETregisterServer(1);

	if (tmp_socket_set == NULL) {
		tmp_socket_set = SDLNet_AllocSocketSet(MAX_TMP_SOCKETS+1);
		if (tmp_socket_set == NULL) {
			debug( LOG_ERROR, "NETallowJoining: Cannot create socket set: %s\n", SDLNet_GetError() );
			return;
		}
		SDLNet_TCP_AddSocket(tmp_socket_set, tcp_socket);
	}

	if (SDLNet_CheckSockets(tmp_socket_set, NET_READ_TIMEOUT) > 0) {
		if (SDLNet_SocketReady(tcp_socket)) {
			for (i = 0; i < MAX_TMP_SOCKETS; ++i) {
				if (tmp_socket[i] == NULL) {
					break;
				}
			}
			tmp_socket[i] = SDLNet_TCP_Accept(tcp_socket);
			SDLNet_TCP_AddSocket(tmp_socket_set, tmp_socket[i]);
			if (SDLNet_CheckSockets(tmp_socket_set, 1000) > 0
			    && SDLNet_SocketReady(tmp_socket)
			    && SDLNet_TCP_Recv(tmp_socket[i], buffer, 5)) {
				if(strcmp(buffer, "list")==0) {
					SDLNet_TCP_Send(tmp_socket[i], &numgames, sizeof(UDWORD));
					SDLNet_TCP_Send(tmp_socket[i], &game, sizeof(GAMESTRUCT));
				} else if(strcmp(buffer, "join")==0) {
					SDLNet_TCP_Send(tmp_socket[i], &game, sizeof(GAMESTRUCT));
				}

			} else {
				return;
			}
		}
		for(i = 0; i < MAX_TMP_SOCKETS; ++i) {
			if (   tmp_socket[i] != NULL
			    && SDLNet_SocketReady(tmp_socket[i]) > 0) {
				int size = SDLNet_TCP_Recv(tmp_socket[i], &message, sizeof(NETMSG));

				if (size <= 0) {
					// socket probably disconnected.
					SDLNet_TCP_DelSocket(tmp_socket_set, tmp_socket[i]);
					SDLNet_TCP_Close(tmp_socket[i]);
					tmp_socket[i] = NULL;
				} else if (message.type == MSG_JOIN) {
					int j;

					char* name = message.body;
					unsigned int dpid = NET_CreatePlayer(name, 0);
					unsigned int* message_dpid = (unsigned int*)(message.body);

					SDLNet_TCP_DelSocket(tmp_socket_set, tmp_socket[i]);
					NET_initBufferedSocket(connected_bsocket[dpid], tmp_socket[i]);
					SDLNet_TCP_AddSocket(socket_set, connected_bsocket[dpid]->socket);
					tmp_socket[i] = NULL;

					//printf("Creating player %i\n", dpid);
					game.desc.dwCurrentPlayers++;

					message.type = MSG_ACCEPTED;
					NETsetMessageSize(&message, 4);
					*message_dpid = dpid;
					NETsend(&message, dpid, TRUE);

					MultiPlayerJoin(dpid);
					message.type = MSG_PLAYER_JOINED;
					NETsetMessageSize(&message, 4);

					// Send info about players to newcomer.
					for (j = 0; j < MAX_CONNECTED_PLAYERS; ++j) {
						if (   players[j].allocated
						    && dpid != players[j].id) {
							*message_dpid = players[j].id;
							NETsend(&message, dpid, TRUE);
						}
					}

					// Send info about newcomer to all players.
					*message_dpid = dpid;
					NETbcast(&message, TRUE);

					for (j = 0; j < MAX_CONNECTED_PLAYERS; ++j) {
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

	debug( LOG_NET, "NEThostGame\n" );

	if(!NetPlay.bComms)
	{
		NetPlay.dpidPlayer		= 1;
		NetPlay.bHost			= TRUE;

		return TRUE;
	}

	if(SDLNet_ResolveHost(&ip, NULL, gameserver_port) == -1) {
		debug( LOG_ERROR, "NEThostGame: Cannot resolve master self: %s\n", SDLNet_GetError() );
		return FALSE;
	}

	if(!tcp_socket) tcp_socket = SDLNet_TCP_Open(&ip);
	if(tcp_socket == NULL) {
		printf("NEThostGame: Cannot connect to master self: %s\n", SDLNet_GetError());
		return FALSE;
	}

	if(!socket_set) socket_set = SDLNet_AllocSocketSet(MAX_CONNECTED_PLAYERS);
	if (socket_set == NULL) {
		debug( LOG_ERROR, "NEThostGame: Cannot create socket set: %s\n", SDLNet_GetError() );
		return FALSE;
	}
	for (i = 0; i < MAX_CONNECTED_PLAYERS; ++i) {
		connected_bsocket[i] = NET_createBufferedSocket();
	}

	is_server = TRUE;

	strcpy(game.name, SessionName);
	memset(&game.desc, 0, sizeof(SESSIONDESC));
	game.desc.dwSize = sizeof(SESSIONDESC);
	//game.desc.guidApplication = GAME_GUID;
	strcpy(game.desc.host, "");
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
	NetPlay.bSpectator	= FALSE;

	MultiPlayerJoin(NetPlay.dpidPlayer);

	allow_joining = TRUE;

	NETregisterServer(0);

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
// Stop the dplay interface from accepting more players.
BOOL NEThaltJoining(void)
{
	debug( LOG_NET, "NEThaltJoining\n" );

	allow_joining = FALSE;
	// disconnect from the master server
	NETregisterServer(0);
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
// find games on open connection
BOOL NETfindGame()
{
	static UDWORD gamecount = 0, gamesavailable;
	IPaddress ip;
	char buffer[sizeof(GAMESTRUCT)*2];
	GAMESTRUCT* tmpgame = (GAMESTRUCT*)buffer;
	unsigned int port = (hostname == masterserver_name) ? masterserver_port : gameserver_port;

	debug( LOG_NET, "NETfindGame\n" );

	gamecount = 0;
	NetPlay.games[0].desc.dwSize = 0;
	NetPlay.games[0].desc.dwCurrentPlayers = 0;
	NetPlay.games[0].desc.dwMaxPlayers = 0;

	if(!NetPlay.bComms)
	{
		NetPlay.dpidPlayer		= 1;
		NetPlay.bHost			= TRUE;
		return TRUE;
	}

	if (SDLNet_ResolveHost(&ip, hostname, port) == -1) {
		debug( LOG_ERROR, "NETfindGame: Cannot resolve hostname \"%s\": %s\n", hostname, SDLNet_GetError() );
		return FALSE;
	}

	if (tcp_socket != NULL) {
		SDLNet_TCP_Close(tcp_socket);
	}

	tcp_socket = SDLNet_TCP_Open(&ip);
	if (tcp_socket == NULL) {
		debug( LOG_ERROR, "NETfindGame: Cannot connect to \"%s:%d\": %s\n", hostname, port, SDLNet_GetError() );
		return FALSE;
	}

	socket_set = SDLNet_AllocSocketSet(1);
	if (socket_set == NULL) {
		debug( LOG_ERROR, "NETfindGame: Cannot create socket set: %s\n", SDLNet_GetError() );
		return FALSE;
	}
	SDLNet_TCP_AddSocket(socket_set, tcp_socket);

	SDLNet_TCP_Send(tcp_socket, "list", 5);

	if (   SDLNet_CheckSockets(socket_set, 1000) > 0
	    && SDLNet_SocketReady(tcp_socket)
	    && SDLNet_TCP_Recv(tcp_socket, &gamesavailable, sizeof(gamesavailable))) {
		gamesavailable = SDL_SwapBE32(gamesavailable);
	}

	debug( LOG_NET, "receiving info of %u game(s)\n", gamesavailable );

	do {
		if (   SDLNet_CheckSockets(socket_set, 1000) > 0
		    && SDLNet_SocketReady(tcp_socket)
		    && SDLNet_TCP_Recv(tcp_socket, buffer, sizeof(GAMESTRUCT)) == sizeof(GAMESTRUCT)
		    && tmpgame->desc.dwSize == sizeof(SESSIONDESC)) {
			strcpy(NetPlay.games[gamecount].name, tmpgame->name);
			NetPlay.games[gamecount].desc.dwSize = tmpgame->desc.dwSize;
			NetPlay.games[gamecount].desc.dwCurrentPlayers = tmpgame->desc.dwCurrentPlayers;
			NetPlay.games[gamecount].desc.dwMaxPlayers = tmpgame->desc.dwMaxPlayers;
			if (tmpgame->desc.host[0] == '\0') {
				unsigned char* address = (unsigned char*)(&(ip.host));

				sprintf(NetPlay.games[gamecount].desc.host,
					"%i.%i.%i.%i",
 					(int)(address[0]),
 					(int)(address[1]),
 					(int)(address[2]),
 					(int)(address[3]));
			} else {
				strcpy(NetPlay.games[gamecount].desc.host, tmpgame->desc.host);
			}
//printf("Received info for host %s\n", NetPlay.games[gamecount].desc.host);
			gamecount++;
		}
	} while (gamecount<gamesavailable);

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Functions used to setup and join games.
BOOL NETjoinGame(UDWORD gameNumber, const char* playername)
{
	char* name;
	IPaddress ip;
	char buffer[sizeof(GAMESTRUCT)*2];
	GAMESTRUCT* tmpgame = (GAMESTRUCT*)buffer;

	debug( LOG_NET, "NETjoinGame gameNumber=%d\n", gameNumber );

	NETclose();	// just to be sure :)

	if (hostname != masterserver_name) {
		free(hostname);
	}
	hostname = strdup(NetPlay.games[gameNumber].desc.host);

	if(SDLNet_ResolveHost(&ip, hostname, gameserver_port) == -1) {
		debug( LOG_ERROR, "NETjoinGame: Cannot resolve hostname \"%s\": %s\n", hostname, SDLNet_GetError() );
		return FALSE;
	}

	if (tcp_socket != NULL) {
		SDLNet_TCP_Close(tcp_socket);
	}

	tcp_socket = SDLNet_TCP_Open(&ip);
 	if (tcp_socket == NULL) {
		debug( LOG_ERROR, "NETjoinGame: Cannot connect to \"%s:%d\": %s\n", hostname, gameserver_port, SDLNet_GetError() );
		return FALSE;
	}

	socket_set = SDLNet_AllocSocketSet(1);
	if (socket_set == NULL) {
		debug( LOG_ERROR, "NETjoinGame: Cannot create socket set: %s\n", SDLNet_GetError() );
 		return FALSE;
 	}
	SDLNet_TCP_AddSocket(socket_set, tcp_socket);


	SDLNet_TCP_Send(tcp_socket, "join", 5);

	if (   SDLNet_CheckSockets(socket_set, 1000) > 0
	    && SDLNet_SocketReady(tcp_socket)
	    && SDLNet_TCP_Recv(tcp_socket, buffer, sizeof(GAMESTRUCT)*2) == sizeof(GAMESTRUCT)
	    && tmpgame->desc.dwSize == sizeof(SESSIONDESC)) {
		strcpy(NetPlay.games[gameNumber].name, tmpgame->name);
		NetPlay.games[gameNumber].desc.dwSize = tmpgame->desc.dwSize;
		NetPlay.games[gameNumber].desc.dwCurrentPlayers = tmpgame->desc.dwCurrentPlayers;
		NetPlay.games[gameNumber].desc.dwMaxPlayers = tmpgame->desc.dwMaxPlayers;
		strcpy(NetPlay.games[gameNumber].desc.host, tmpgame->desc.host);
		if (tmpgame->desc.host[0] == '\0') {
			unsigned char* address = (unsigned char*)(&(ip.host));

			sprintf(NetPlay.games[gameNumber].desc.host,
				"%i.%i.%i.%i",
				(int)(address[0]),
				(int)(address[1]),
				(int)(address[2]),
				(int)(address[3]));
		} else {
			strcpy(NetPlay.games[gameNumber].desc.host, tmpgame->desc.host);
		}
//printf("JoinGame: Received info for host %s\n", NetPlay.games[gameNumber].desc.host);
	}

	bsocket = NET_createBufferedSocket();
	NET_initBufferedSocket(bsocket, tcp_socket);

	message.type = MSG_JOIN;
	NETsetMessageSize(&message, 64);
	name = message.body;
	strcpy(name, playername);
	NETsend(&message, 1, TRUE);

	for (;;) {
		NETrecv(&message);

		if (message.type == MSG_ACCEPTED) {
			unsigned int* message_dpid = (unsigned int*)(message.body);

			NetPlay.dpidPlayer = *message_dpid;
//printf("I'm player %i\n", NetPlay.dpidPlayer);
			NetPlay.bHost = FALSE;
			NetPlay.bSpectator = FALSE;

			players[NetPlay.dpidPlayer].allocated = TRUE;
			players[NetPlay.dpidPlayer].id = NetPlay.dpidPlayer;
			strcpy(players[NetPlay.dpidPlayer].name, playername);
			players[NetPlay.dpidPlayer].flags = 0;

			return TRUE;
		}
	}

	return FALSE;
}


/*!
 * Set the masterserver name
 * \param hostname The hostname of the masterserver to connect to
 */
void NETsetMasterserverName(const char* hostname)
{
	size_t name_size = strlen(hostname);
	if ( name_size > 255 )
		name_size = 255;
	strncpy(masterserver_name, hostname, name_size);
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
