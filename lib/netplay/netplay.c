/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
#include "lib/framework/crc.h"
#include "lib/framework/file.h"
#include "lib/gamelib/gtime.h"
#include "src/component.h"		// FIXME: we need to handle this better
#include "src/modding.h"		// FIXME: we need to handle this better
#include <time.h>			// for stats
#include <SDL_timer.h>
#include <SDL_thread.h>
#include <physfs.h>
#include <string.h>

#include "netplay.h"
#include "netlog.h"
#include "netsocket.h"

#include "miniupnpc/miniwget.h"
#include "miniupnpc/miniupnpc.h"
#include "miniupnpc/upnpcommands.h"
#include "lib/exceptionhandler/dumpinfo.h"

#include "src/multistat.h"
#include "src/multijoin.h"
#include "src/multiint.h"
#include "src/multiplay.h"
#include "src/warzoneconfig.h"

#ifdef WZ_OS_LINUX
#include <execinfo.h>  // Nonfatal runtime backtraces.
#endif //WZ_OS_LINUX

// For /(hton|ntoh)[sh]/.
#if   defined(WZ_OS_UNIX)
# include <arpa/inet.h>
#endif
#ifdef WZ_OS_WIN
# include <winsock2.h>
#endif

// WARNING !!! This is initialised via configuration.c !!!
char masterserver_name[255] = {'\0'};
static unsigned int masterserver_port = 0, gameserver_port = 0;

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

// ////////////////////////////////////////////////////////////////////////
// Function prototypes
static void NETplayerLeaving(UDWORD player);		// Cleanup sockets on player leaving (nicely)
static void NETplayerDropped(UDWORD player);		// Broadcast NET_PLAYER_DROPPED & cleanup
static void NETregisterServer(int state);
static void NETallowJoining(void);
static void recvDebugSync(NETQUEUE queue);
static bool onBanList(const char *ip);
static void addToBanList(const char *ip, const char *name);

/*
 * Network globals, these are part of the new network API
 */
SYNC_COUNTER sync_counter;		// keeps track on how well we are in sync
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

// ////////////////////////////////////////////////////////////////////////
// Variables

NETPLAY	NetPlay;
PLAYER_IP	*IPlist = NULL;
static BOOL		allow_joining = false;
static	bool server_not_there = false;
static GAMESTRUCT	gamestruct;

// update flags
bool netPlayersUpdated;
int mapDownloadProgress;

/**
 * Socket used for these purposes:
 *  * Host a game, be a server.
 *  * Connect to the lobby server.
 *  * Join a server for a game.
 */
static Socket* tcp_socket = NULL;		//socket used to talk to lobbyserver/ host machine

static Socket *bsocket = NULL;                  //buffered socket (holds tcp_socket) (clients only?)
static Socket *connected_bsocket[MAX_CONNECTED_PLAYERS] = { NULL };
static SocketSet* socket_set = NULL;

// UPnP
static int upnp = false;
static bool upnp_done = false;
SDL_Thread *upnpdiscover;

static struct UPNPUrls urls;
static struct IGDdatas data;

// local ip address
static char lanaddr[16];
static char clientAddress[40] = { '\0' };
/**
 * Used for connections with clients.
 */
static Socket* tmp_socket[MAX_TMP_SOCKETS] = { NULL };

static SocketSet* tmp_socket_set = NULL;
static char*		hostname;
static NETSTATS		nStats = { 0, 0, 0, 0 };
static int32_t          NetGameFlags[4] = { 0, 0, 0, 0 };
char iptoconnect[PATH_MAX] = "\0"; // holds IP/hostname from command line

unsigned NET_PlayerConnectionStatus[CONNECTIONSTATUS_NORMAL][MAX_PLAYERS];

// ////////////////////////////////////////////////////////////////////////////
#define VersionStringSize 80
/************************************************************************************
 **  NOTE (!)  Change the VersionString when net code changes!!
 **            ie ("trunk", "2.1.3", "3.0", ...)
 ************************************************************************************
**/
char VersionString[VersionStringSize] = "master, netcode 4.1009";
static int NETCODE_VERSION_MAJOR = 4;
static int NETCODE_VERSION_MINOR = 1009;

bool NETisCorrectVersion(uint32_t game_version_major, uint32_t game_version_minor)
{
	return (NETCODE_VERSION_MAJOR == game_version_major && NETCODE_VERSION_MINOR == game_version_minor);
}
bool NETgameIsCorrectVersion(GAMESTRUCT* check_game)
{
	return (NETCODE_VERSION_MAJOR == check_game->game_version_major && NETCODE_VERSION_MINOR == check_game->game_version_minor);
}

//	Sets if the game is password protected or not
void NETGameLocked( bool flag)
{
	NetPlay.GamePassworded = flag;
	gamestruct.privateGame = flag;
	NETlogEntry("Password is", SYNC_FLAG, NetPlay.GamePassworded);
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
	sstrcpy(NetPlay.gamePassword, _("Enter password here"));
	debug(LOG_NET, "password reset to 'Enter password here'");
	NETGameLocked(false);
}

// *********** Socket with buffer that read NETMSGs ******************

static size_t NET_fillBuffer(Socket **pSocket, SocketSet* socket_set, uint8_t *bufstart, int bufsize)
{
	Socket *socket = *pSocket;
	ssize_t size;

	if (!socketReadReady(socket))
	{
		return 0;
	}

	size = readNoInt(socket, bufstart, bufsize);

	if ((size != 0 || !socketReadDisconnected(socket)) && size != SOCKET_ERROR)
	{
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
			debug(LOG_NET, "%s tcp_socket %p is now invalid", strSockError(getSockErr()), socket);
		}

		// an error occured, or the remote host has closed the connection.
		if (socket_set != NULL)
		{
			SocketSet_DelSocket(socket_set, socket);
		}

		ASSERT(size <= bufsize, "Socket buffer is too small!");

		if (size > bufsize)
		{
			debug(LOG_ERROR, "Fatal connection error: buffer size of (%d) was too small, current byte count was %zd", bufsize, size);
			NETlogEntry("Fatal connection error: buffer size was too small!", SYNC_FLAG, selectedPlayer);
		}
		if (tcp_socket == socket)
		{
			debug(LOG_NET, "Host connection was lost!");
			NETlogEntry("Host connection was lost!", SYNC_FLAG, selectedPlayer);
			tcp_socket = NULL;
			//Game is pretty much over --should just end everything when HOST dies.
			NetPlay.isHostAlive = false;
			setLobbyError(ERROR_HOSTDROPPED);
			NETclose();
			return 0;
		}
		socketClose(socket);
		*pSocket = NULL;
	}

	return 0;
}

void NET_InitPlayer(int i, bool initPosition)
{
	NetPlay.players[i].allocated = false;
	NetPlay.players[i].heartattacktime = 0;
	NetPlay.players[i].heartbeat = true;		// we always start with a hearbeat
	NetPlay.players[i].kick = false;
	if (!NetPlay.isHost)
	{	// only clear name outside of games.
		NetPlay.players[i].name[0] = '\0';
	}
	if (initPosition)
	{
		NetPlay.players[i].colour = i;
		setPlayerColour(i, i);  // PlayerColour[] in component.c must match this! Why is this in more than one place??!
		NetPlay.players[i].position = i;
		NetPlay.players[i].team = i;
	}
	NetPlay.players[i].ready = false;
	NetPlay.players[i].needFile = false;
	NetPlay.players[i].wzFile.isCancelled = false;
	NetPlay.players[i].wzFile.isSending = false;
}

void NET_InitPlayers()
{
	unsigned int i;

	for (i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
	{
		NET_InitPlayer(i, true);
		NETinitQueue(NETnetQueue(i));
	}
	NETinitQueue(NETbroadcastQueue());

	NetPlay.hostPlayer = NET_HOST_ONLY;	// right now, host starts always at index zero
	NetPlay.playercount = 0;
	NetPlay.pMapFileHandle = NULL;
	debug(LOG_NET, "Players initialized");
}

static void NETSendNPlayerInfoTo(uint32_t *index, uint32_t indexLen, unsigned to)
{
	int n;

	NETbeginEncode(NETnetQueue(to), NET_PLAYER_INFO);
		NETuint32_t(&indexLen);
		for (n = 0; n < indexLen; ++n)
		{
			debug(LOG_NET, "sending player's (%u) info to all players", index[n]);
			NETlogEntry(" sending player's info to all players", SYNC_FLAG, index[n]);
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
		}
		NETuint32_t(&NetPlay.hostPlayer);
	NETend();
}

static void NETSendPlayerInfoTo(uint32_t index, unsigned to)
{
	NETSendNPlayerInfoTo(&index, 1, to);
}

static void NETSendAllPlayerInfoTo(unsigned to)
{
	static uint32_t indices[MAX_PLAYERS] = {0, 1, 2, 3, 4, 5, 6, 7};
	ASSERT_OR_RETURN( , NetPlay.isHost == true, "Invalid call for non-host");

	NETSendNPlayerInfoTo(indices, ARRAY_SIZE(indices), to);
}

void NETBroadcastTwoPlayerInfo(uint32_t index1, uint32_t index2)
{
	uint32_t indices[2] = {index1, index2};
	NETSendNPlayerInfoTo(indices, 2, NET_ALL_PLAYERS);
}

void NETBroadcastPlayerInfo(uint32_t index)
{
	NETSendPlayerInfoTo(index, NET_ALL_PLAYERS);
}

static signed int NET_CreatePlayer(const char* name)
{
	signed int index;

	for (index = 0; index < MAX_CONNECTED_PLAYERS; index++)
	{
		if (NetPlay.players[index].allocated == false)
		{
			char buf[250] = {'\0'};

			ssprintf(buf, "A new player has been created. Player, %s, is set to slot %u", name, index);
			debug(LOG_NET,"%s", buf);
			NETlogEntry(buf, SYNC_FLAG, index);
			NET_InitPlayer(index, false);	// re-init everything
			NetPlay.players[index].allocated = true;
			sstrcpy(NetPlay.players[index].name, name);
			NetPlay.playercount++;
			sync_counter.joins++;
			return index;
		}
	}

	debug(LOG_ERROR, "Could not find place for player %s", name);
	NETlogEntry("Could not find a place for player!", SYNC_FLAG, index);
	return -1;
}

static void NET_DestroyPlayer(unsigned int index)
{
	debug(LOG_NET, "Freeing slot %u for a new player", index);
	NETlogEntry("Freeing slot for a new player.", SYNC_FLAG, index);
	if (NetPlay.players[index].allocated)
	{
		NetPlay.players[index].allocated = false;
		NetPlay.playercount--;
		gamestruct.desc.dwCurrentPlayers = NetPlay.playercount;
		if (allow_joining && NetPlay.isHost)
		{
			// Update player count in the lobby by disconnecting
			// and reconnecting
			NETregisterServer(0);
			NETregisterServer(1);
		}
	}
	NET_InitPlayer(index, false);  // reinitialize
}

/**
 * @note Connection dropped. Handle it gracefully.
 * \param index
 */
static void NETplayerClientDisconnect(uint32_t index)
{
	if(connected_bsocket[index])
	{
		debug(LOG_NET, "Player (%u) has left unexpectedly, closing socket %p", index, connected_bsocket[index]);

		NETplayerLeaving(index);

		NETlogEntry("Player has left unexpectedly.", SYNC_FLAG, index);
		// Announce to the world. This was really icky, because we may have been calling the send
		// function recursively. We really ought to have had a send queue, and now we finally do...
		NETbeginEncode(NETbroadcastQueue(), NET_PLAYER_DROPPED);
			NETuint32_t(&index);
		NETend();
	}
	else
	{
		debug(LOG_ERROR, "Player (%u) has left unexpectedly - but socket already closed?", index);
	}
}

/**
 * @note When a player leaves nicely (ie, we got a NET_PLAYER_LEAVING
 *       message), we clean up the socket that we used.
 * \param index
 */
static void NETplayerLeaving(UDWORD index)
{
	if(connected_bsocket[index])
	{
		debug(LOG_NET, "Player (%u) has left, closing socket %p", index, connected_bsocket[index]);
		NETlogEntry("Player has left nicely.", SYNC_FLAG, index);

		// Although we can get a error result from DelSocket, it don't really matter here.
		SocketSet_DelSocket(socket_set, connected_bsocket[index]);
		socketClose(connected_bsocket[index]);
		connected_bsocket[index] = NULL;
	}
	else
	{
		debug(LOG_NET, "Player (%u) has left nicely, socket already closed?", index);
	}
	sync_counter.left++;
	MultiPlayerLeave(index);		// more cleanup
	NET_DestroyPlayer(index);		// sets index player's array to false
}

/**
 * @note When a player's connection is broken we broadcast the NET_PLAYER_DROPPED
 *       message.
 * \param index
 */
static void NETplayerDropped(UDWORD index)
{
	uint32_t id = index;

	// Send message type specifically for dropped / disconnects
	NETbeginEncode(NETbroadcastQueue(), NET_PLAYER_DROPPED);
		NETuint32_t(&id);
	NETend();
	debug(LOG_INFO, "sending NET_PLAYER_DROPPED for player %d", id);
	sync_counter.drops++;
	NET_DestroyPlayer(id);		// just clears array
	MultiPlayerLeave(id);			// more cleanup

	NETsetPlayerConnectionStatus(CONNECTIONSTATUS_PLAYER_DROPPED, id);
}

/**
 * @note Cleanup for when a player is kicked.
 * \param index
 */
void NETplayerKicked(UDWORD index)
{
	// kicking a player counts as "leaving nicely", since "nicely" in this case
	// simply means "there wasn't a connection error."
	debug(LOG_INFO, "Player %u was kicked.", index);
	sync_counter.kicks++;
	NETlogEntry("Player was kicked.", SYNC_FLAG, index);
	addToBanList(NetPlay.players[index].IPtextAddress, NetPlay.players[index].name);
	NETplayerLeaving(index);		// need to close socket for the player that left.
	NETsetPlayerConnectionStatus(CONNECTIONSTATUS_PLAYER_LEAVING, index);
}

// ////////////////////////////////////////////////////////////////////////
// rename the local player
BOOL NETchangePlayerName(UDWORD index, char *newName)
{
	if(!NetPlay.bComms)
	{
		sstrcpy(NetPlay.players[0].name, newName);
		return true;
	}
	debug(LOG_NET, "Requesting a change of player name for pid=%u to %s", index, newName);
	NETlogEntry("Player wants a name change.", SYNC_FLAG, index);
	sstrcpy(NetPlay.players[index].name, newName);

	NETBroadcastPlayerInfo(index);

	return true;
}

void NETfixDuplicatePlayerNames(void)
{
	char name[StringSize];
	unsigned i, j, pass;
	for (i = 1; i != MAX_PLAYERS; ++i)
	{
		sstrcpy(name, NetPlay.players[i].name);
		if (name[0] == '\0')
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
		return NetGameFlags[flag-1];
	}
}

static void NETsendGameFlags(void)
{
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
 * @return true on success, false when a socket error has occurred
 *
 * @see GAMESTRUCT,NETrecvGAMESTRUCT
 */
static bool NETsendGAMESTRUCT(Socket* sock, const GAMESTRUCT* ourgamestruct)
{
	// A buffer that's guaranteed to have the correct size (i.e. it
	// circumvents struct padding, which could pose a problem).  Initialise
	// to zero so that we can be sure we're not sending any (undefined)
	// memory content across the network.
	char buf[sizeof(ourgamestruct->GAMESTRUCT_VERSION) + sizeof(ourgamestruct->name) + sizeof(ourgamestruct->desc.host) + (sizeof(int32_t) * 8) +
		sizeof(ourgamestruct->secondaryHosts) + sizeof(ourgamestruct->extra) + sizeof(ourgamestruct->mapname) + sizeof(ourgamestruct->hostname) + sizeof(ourgamestruct->versionstring) +
		sizeof(ourgamestruct->modlist) + (sizeof(uint32_t) * 9) ] = { 0 };
	char *buffer = buf;
	unsigned int i;
	ssize_t result;

	// Now dump the data into the buffer
	// Copy 32bit large big endian numbers
	*(uint32_t*)buffer = htonl(ourgamestruct->GAMESTRUCT_VERSION);
	buffer += sizeof(uint32_t);

	// Copy a string
	strlcpy(buffer, ourgamestruct->name, sizeof(ourgamestruct->name));
	buffer += sizeof(ourgamestruct->name);

	// Copy 32bit large big endian numbers
	*(int32_t*)buffer = htonl(ourgamestruct->desc.dwSize);
	buffer += sizeof(int32_t);
	*(int32_t*)buffer = htonl(ourgamestruct->desc.dwFlags);
	buffer += sizeof(int32_t);

	// Copy yet another string
	strlcpy(buffer, ourgamestruct->desc.host, sizeof(ourgamestruct->desc.host));
	buffer += sizeof(ourgamestruct->desc.host);

	// Copy 32bit large big endian numbers
	*(int32_t*)buffer = htonl(ourgamestruct->desc.dwMaxPlayers);
	buffer += sizeof(int32_t);
	*(int32_t*)buffer = htonl(ourgamestruct->desc.dwCurrentPlayers);
	buffer += sizeof(int32_t);
	for (i = 0; i < ARRAY_SIZE(ourgamestruct->desc.dwUserFlags); ++i)
	{
		*(int32_t*)buffer = htonl(ourgamestruct->desc.dwUserFlags[i]);
		buffer += sizeof(int32_t);
	}

	// Copy a string
	for (i = 0; i <ARRAY_SIZE(ourgamestruct->secondaryHosts); ++i)
	{
		strlcpy(buffer, ourgamestruct->secondaryHosts[i], sizeof(ourgamestruct->secondaryHosts[i]));
		buffer += sizeof(ourgamestruct->secondaryHosts[i]);
	}

	// Copy a string
	strlcpy(buffer, ourgamestruct->extra, sizeof(ourgamestruct->extra));
	buffer += sizeof(ourgamestruct->extra);

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
	*(uint32_t*)buffer = htonl(ourgamestruct->game_version_major);
	buffer += sizeof(uint32_t);

	// Copy 32bit large big endian numbers
	*(uint32_t*)buffer = htonl(ourgamestruct->game_version_minor);
	buffer += sizeof(uint32_t);

	// Copy 32bit large big endian numbers
	*(uint32_t*)buffer = htonl(ourgamestruct->privateGame);
	buffer += sizeof(uint32_t);

	// Copy 32bit large big endian numbers
	*(uint32_t*)buffer = htonl(ourgamestruct->pureGame);
	buffer += sizeof(uint32_t);

	// Copy 32bit large big endian numbers
	*(uint32_t*)buffer = htonl(ourgamestruct->Mods);
	buffer += sizeof(uint32_t);

	// Copy 32bit large big endian numbers
	*(uint32_t*)buffer = htonl(ourgamestruct->gameId);
	buffer += sizeof(uint32_t);

	// Copy 32bit large big endian numbers
	*(uint32_t*)buffer = htonl(ourgamestruct->future2);
	buffer += sizeof(uint32_t);

	// Copy 32bit large big endian numbers
	*(uint32_t*)buffer = htonl(ourgamestruct->future3);
	buffer += sizeof(uint32_t);

	// Copy 32bit large big endian numbers
	*(uint32_t*)buffer = htonl(ourgamestruct->future4);
	buffer += sizeof(uint32_t);


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

	debug(LOG_NET, "sending GAMESTRUCT");

	return true;
}

/**
 * @note \c game is being retrieved from the master server (if browsing the
 *       lobby). The implementation of NETrecvGAMESTRUCT should assume the data
 *       to be packed in network byte order (big-endian).
 *
 * @see GAMESTRUCT,NETsendGAMESTRUCT
 */
static bool NETrecvGAMESTRUCT(GAMESTRUCT* ourgamestruct)
{
	// A buffer that's guaranteed to have the correct size (i.e. it
	// circumvents struct padding, which could pose a problem).
	char buf[sizeof(ourgamestruct->GAMESTRUCT_VERSION) + sizeof(ourgamestruct->name) + sizeof(ourgamestruct->desc.host) + (sizeof(int32_t) * 8) +
		sizeof(ourgamestruct->secondaryHosts) + sizeof(ourgamestruct->extra) + sizeof(ourgamestruct->mapname) + sizeof(ourgamestruct->hostname) + sizeof(ourgamestruct->versionstring) +
		sizeof(ourgamestruct->modlist) + (sizeof(uint32_t) * 9) ] = { 0 };
	char* buffer = buf;
	unsigned int i;
	ssize_t result = 0;

	// Read a GAMESTRUCT from the connection
	if (tcp_socket == NULL
	 || socket_set == NULL
	 || checkSockets(socket_set, NET_TIMEOUT_DELAY) <= 0
	 || !socketReadReady(tcp_socket)
	 || (result = readNoInt(tcp_socket, buf, sizeof(buf))) != sizeof(buf))
	{
		unsigned int time = SDL_GetTicks();
		if (result == SOCKET_ERROR)
		{
			debug(LOG_ERROR, "Server socket (%p) ecountered error: %s", tcp_socket, strSockError(getSockErr()));
			SocketSet_DelSocket(socket_set, tcp_socket);		// mark it invalid
			socketClose(tcp_socket);
			tcp_socket = NULL;
			return false;
		}
		i = result;
		while (i < sizeof(buf) && SDL_GetTicks() < time + 2500)
		{
			result = readNoInt(tcp_socket, buf+i, sizeof(buf)-i);
			if (result == SOCKET_ERROR
			 || (result == 0 && socketReadDisconnected(tcp_socket)))
			{
				debug(LOG_ERROR, "Server socket (%p) ecountered error: %s", tcp_socket, strSockError(getSockErr()));
				debug(LOG_ERROR, "GAMESTRUCT recv failed; received %u bytes out of %d", i, (int)sizeof(buf));
				SocketSet_DelSocket(socket_set, tcp_socket);		// mark it invalid
				socketClose(tcp_socket);
				tcp_socket = NULL;
				return false;
			}
			i += result;
		}
		if (i != sizeof(buf))
		{
			debug(LOG_ERROR, "GAMESTRUCT recv size mismatch; received %u bytes; expecting %d", i, (int)sizeof(buf));
			return false;
		}
	}

	// Now dump the data into the game struct
	// Copy 32bit large big endian numbers
	ourgamestruct->GAMESTRUCT_VERSION = ntohl(*(uint32_t*)buffer);
	buffer += sizeof(uint32_t);
	// Copy a string
	sstrcpy(ourgamestruct->name, buffer);
	buffer += sizeof(ourgamestruct->name);

	// Copy 32bit large big endian numbers
	ourgamestruct->desc.dwSize = ntohl(*(int32_t*)buffer);
	buffer += sizeof(int32_t);
	ourgamestruct->desc.dwFlags = ntohl(*(int32_t*)buffer);
	buffer += sizeof(int32_t);

	// Copy yet another string
	sstrcpy(ourgamestruct->desc.host, buffer);
	buffer += sizeof(ourgamestruct->desc.host);

	// Copy 32bit large big endian numbers
	ourgamestruct->desc.dwMaxPlayers = ntohl(*(int32_t*)buffer);
	buffer += sizeof(int32_t);
	ourgamestruct->desc.dwCurrentPlayers = ntohl(*(int32_t*)buffer);
	buffer += sizeof(int32_t);
	for (i = 0; i < ARRAY_SIZE(ourgamestruct->desc.dwUserFlags); ++i)
	{
		ourgamestruct->desc.dwUserFlags[i] = ntohl(*(int32_t*)buffer);
		buffer += sizeof(int32_t);
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
	ourgamestruct->game_version_major = ntohl(*(uint32_t*)buffer);
	buffer += sizeof(uint32_t);
	ourgamestruct->game_version_minor = ntohl(*(uint32_t*)buffer);
	buffer += sizeof(uint32_t);
	ourgamestruct->privateGame = ntohl(*(uint32_t*)buffer);
	buffer += sizeof(uint32_t);
	ourgamestruct->pureGame = ntohl(*(uint32_t*)buffer);
	buffer += sizeof(uint32_t);
	ourgamestruct->Mods = ntohl(*(uint32_t*)buffer);
	buffer += sizeof(uint32_t);
	ourgamestruct->gameId = ntohl(*(uint32_t*)buffer);
	buffer += sizeof(uint32_t);	
	ourgamestruct->future2 = ntohl(*(uint32_t*)buffer);
	buffer += sizeof(uint32_t);	
	ourgamestruct->future3 = ntohl(*(uint32_t*)buffer);
	buffer += sizeof(uint32_t);	
	ourgamestruct->future4 = ntohl(*(uint32_t*)buffer);
	buffer += sizeof(uint32_t);	
	
	// cat the modstring (if there is one) to the version string to display it for the end-user
	if (ourgamestruct->modlist[0] != '\0')
	{
		ssprintf(ourgamestruct->versionstring, "%s, Mods:%s", ourgamestruct->versionstring, ourgamestruct->modlist);
	}
	debug(LOG_NET, "received GAMESTRUCT");

	return true;
}



static int upnp_init(void *asdf)
{
	struct UPNPDev *devlist;
	struct UPNPDev *dev;
	char *descXML;
	int descXMLsize = 0;
	char buf[255];

	memset(&urls, 0, sizeof(struct UPNPUrls));
	memset(&data, 0, sizeof(struct IGDdatas));

	if (NetPlay.isUPNP)
	{
		debug(LOG_NET, "Searching for UPnP devices for automatic port forwarding...");
		devlist = upnpDiscover(2000, NULL, NULL, 0);
		debug(LOG_NET, "UPnP device search finished.");
		if (devlist)
		{
			dev = devlist;
			while (dev)
			{
				if (strstr(dev->st, "InternetGatewayDevice"))
					break;
				dev = dev->pNext;
			}
			if (!dev)
			{
				dev = devlist; /* defaulting to first device */
			}

			debug(LOG_NET, "UPnP device found: %s %s\n", dev->descURL, dev->st);

			descXML = miniwget_getaddr(dev->descURL, &descXMLsize, lanaddr, sizeof(lanaddr));
			debug(LOG_NET, "LAN address: %s", lanaddr);
			if (descXML)
			{
				parserootdesc (descXML, descXMLsize, &data);
				free (descXML); descXML = 0;
				GetUPNPUrls (&urls, &data, dev->descURL);
			}
			ssprintf(buf, "UPnP device found: %s %s LAN address %s", dev->descURL, dev->st, lanaddr);
			addDumpInfo(buf);
			freeUPNPDevlist(devlist);

			if (!urls.controlURL || urls.controlURL[0] == '\0')
			{
				ssprintf(buf, "controlURL not available, UPnP disabled");
				addDumpInfo(buf);
				return false;
			}
			return true;
		}
		ssprintf(buf, "UPnP device not found.");
		addDumpInfo(buf);
		debug(LOG_NET, "No UPnP devices found.");
		return false;
	}
	else
	{
		ssprintf(buf, "UPnP detection routine disabled by user.");
		addDumpInfo(buf);
		debug(LOG_NET, "UPnP detection routine disabled by user.");
		return false;
	}
}

static bool upnp_add_redirect(int port)
{
	char externalIP[16];
	char port_str[16];
	int r;

	debug(LOG_NET, "upnp_add_redir(%d)\n", port);
	UPNP_GetExternalIPAddress(urls.controlURL, data.servicetype, externalIP);
	sprintf(port_str, "%d", port);
	r = UPNP_AddPortMapping(urls.controlURL, data.servicetype,
			port_str, port_str, lanaddr, "Warzone 2100", "TCP", 0);
	if (r != UPNPCOMMAND_SUCCESS)
	{
		debug(LOG_NET, "AddPortMapping(%s, %s, %s) failed\n", port_str, port_str, lanaddr);
		return false;
	}
	return true;

}

static void upnp_rem_redirect(int port)
{
	char port_str[16];
	debug(LOG_NET, "upnp_rem_redir(%d)", port);
	sprintf(port_str, "%d", port);
	UPNP_DeletePortMapping(urls.controlURL, data.servicetype, port_str, "TCP", 0);
}

void NETaddRedirects(void)
{
	debug(LOG_NET, "%s\n", __FUNCTION__);
	if (!upnp_done)
	{
		SDL_WaitThread(upnpdiscover, &upnp);
		upnp_done = true;
	}
	if (upnp) {
		upnp_add_redirect(gameserver_port);
	}
}

void NETremRedirects(void)
{
	debug(LOG_NET, "%s\n", __FUNCTION__);
	if (upnp)
	{
		upnp_rem_redirect(gameserver_port);
	}
}

void NETdiscoverUPnPDevices(void)
{
	upnpdiscover = SDL_CreateThread(&upnp_init, NULL);
}

// ////////////////////////////////////////////////////////////////////////
// setup stuff
int NETinit(BOOL bFirstCall)
{
	UDWORD i;

	debug(LOG_NET, "NETinit");
	NETlogEntry("NETinit!", SYNC_FLAG, selectedPlayer);
	NET_InitPlayers();

	SOCKETinit();

	if(bFirstCall)
	{
		debug(LOG_NET, "NETPLAY: Init called, MORNIN'");

		for(i = 0; i < MAX_PLAYERS; i++)
		{
			memset(&NetPlay.games[i], 0, sizeof(NetPlay.games[i]));
		}
		// NOTE NetPlay.isUPNP is already set in configuration.c!
		NetPlay.bComms = true;
		NetPlay.GamePassworded = false;
		NetPlay.ShowedMOTD = false;
		NetPlay.isHostAlive = false;
		NetPlay.gamePassword[0] = '\0';
		NetPlay.MOTD = strdup("");
		sstrcpy(NetPlay.gamePassword,_("Enter password here"));
		NETstartLogging();
	}

	NetPlay.ShowedMOTD = false;
	NetPlay.GamePassworded = false;
	memset(&sync_counter, 0x0, sizeof(sync_counter));	//clear counters

	return 0;
}


// ////////////////////////////////////////////////////////////////////////
// SHUTDOWN THE CONNECTION.
int NETshutdown(void)
{
	debug( LOG_NET, "NETshutdown" );
	NETlogEntry("NETshutdown", SYNC_FLAG, selectedPlayer);
	NETstopLogging();
	if (IPlist)
		free(IPlist);
	IPlist = NULL;

	SOCKETshutdown();

	if (NetPlay.bComms && NetPlay.isUPNP)
	{
		NETremRedirects();
	}
	return 0;
}

// ////////////////////////////////////////////////////////////////////////
//close the open game..
int NETclose(void)
{
	unsigned int i;

	// reset flag 
	NetPlay.ShowedMOTD = false;
	NEThaltJoining();

	debug(LOG_NET, "Terminating sockets.");

	NetPlay.isHost = false;
	server_not_there = false;
	allow_joining = false;

	if(bsocket)
	{	// need SocketSet_DelSocket() as well, socket_set or tmp_socket_set?
		debug(LOG_NET, "Closing bsocket %p socket %p (tcp_socket=%p)", bsocket, bsocket, tcp_socket);
		//socketClose(bsocket);
		bsocket=NULL;
	}

	for(i = 0; i < MAX_CONNECTED_PLAYERS; i++)
	{
		if (connected_bsocket[i])
		{
			debug(LOG_NET, "Closing connected_bsocket[%u], %p", i, connected_bsocket[i]);
			socketClose(connected_bsocket[i]);
			connected_bsocket[i]=NULL;
		}
		NET_DestroyPlayer(i);
	}

	if (tmp_socket_set)
	{
		debug(LOG_NET, "Freeing tmp_socket_set %p", tmp_socket_set);
		deleteSocketSet(tmp_socket_set);
		tmp_socket_set=NULL;
	}

	for (i = 0; i < MAX_TMP_SOCKETS; i++)
	{
		if (tmp_socket[i])
		{
			// FIXME: need SocketSet_DelSocket() as well, socket_set or tmp_socket_set?
			debug(LOG_NET, "Closing tmp_socket[%d] %p", i, tmp_socket[i]);
			socketClose(tmp_socket[i]);
			tmp_socket[i]=NULL;
		}
	}

	if (socket_set)
	{
		// checking to make sure tcp_socket is still valid
		if (tcp_socket)
		{
			SocketSet_DelSocket(socket_set, tcp_socket);
		}
		debug(LOG_NET, "Freeing socket_set %p", socket_set);
		deleteSocketSet(socket_set);
		socket_set=NULL;
	}
	if (tcp_socket)
	{
		debug(LOG_NET, "Closing tcp_socket %p", tcp_socket);
		socketClose(tcp_socket);
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
BOOL NETsend(uint8_t player, NETMESSAGE message)
{
	ssize_t result = 0;

	if(!NetPlay.bComms)
	{
		return true;
	}

	if (player >= MAX_CONNECTED_PLAYERS && player != NET_ALL_PLAYERS) return false;

	if (NetPlay.isHost)
	{
		int firstPlayer = player == NET_ALL_PLAYERS ? 0                         : player;
		int lastPlayer  = player == NET_ALL_PLAYERS ? MAX_CONNECTED_PLAYERS - 1 : player;
		for (player = firstPlayer; player <= lastPlayer; ++player)
		{
			// We are the host, send directly to player.
			if (connected_bsocket[player] != NULL)
			{
				uint8_t *rawData = NETmessageRawData(message);
				size_t rawLen    = NETmessageRawSize(message);
				result = writeAll(connected_bsocket[player], rawData, rawLen);
				NETmessageDestroyRawData(rawData);  // Done with the data.

				if (result == rawLen)
				{
					nStats.bytesSent   += rawLen;
					nStats.packetsSent += 1;
				}
				else if (result == SOCKET_ERROR)
				{
					// Write error, most likely client disconnect.
					debug(LOG_ERROR, "Failed to send message: %s", strSockError(getSockErr()));
					NETlogEntry("client disconnect?", SYNC_FLAG, player);
					NETplayerClientDisconnect(player);
				}
			}
		}
		return true;
	}
	else if (player == NetPlay.hostPlayer)
	{
		// We are a client, send directly to player, who happens to be the host.
		if (tcp_socket)
		{
			uint8_t *rawData = NETmessageRawData(message);
			size_t rawLen    = NETmessageRawSize(message);
			result = writeAll(tcp_socket, rawData, rawLen);
			NETmessageDestroyRawData(rawData);  // Done with the data.

			if (result == rawLen)
			{
				nStats.bytesSent   += rawLen;
				nStats.packetsSent += 1;
			}
			else if (result == SOCKET_ERROR)
			{
				// Write error, most likely host disconnect.
				debug(LOG_ERROR, "Failed to send message: %s", strSockError(getSockErr()));
				debug(LOG_ERROR, "Host connection was broken, socket %p.", tcp_socket);
				NETlogEntry("write error--client disconnect.", SYNC_FLAG, player);
				SocketSet_DelSocket(socket_set, tcp_socket);            // mark it invalid
				socketClose(tcp_socket);
				tcp_socket = NULL;
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
		NETbeginEncode(NETnetQueue(NET_HOST_ONLY), NET_SEND_TO_PLAYER);
			NETuint8_t(&sender);
			NETuint8_t(&player);
			NETNETMESSAGE(&message);
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

	if (NetPlay.isHost)
	{
		int player;
		for (player = 0; player < MAX_CONNECTED_PLAYERS; ++player)
		{
			// We are the host, send directly to player.
			if (connected_bsocket[player] != NULL)
			{
				socketFlush(connected_bsocket[player]);
			}
		}
	}
	else
	{
		if (bsocket != NULL)
		{
			socketFlush(bsocket);
		}
	}
}

///////////////////////////////////////////////////////////////////////////
// Check if a message is a system message
static BOOL NETprocessSystemMessage(NETQUEUE playerQueue, uint8_t type)
{
	switch (type)
	{
		case NET_SEND_TO_PLAYER:
		{
			uint8_t sender;
			uint8_t receiver;
			NETMESSAGE message = NULL;
			NETbeginDecode(playerQueue, NET_SEND_TO_PLAYER);
				NETuint8_t(&sender);
				NETuint8_t(&receiver);
				NETNETMESSAGE(&message);  // Must destroy message later.
			if (!NETend())
			{
				debug(LOG_ERROR, "Incomplete NET_SEND_TO_PLAYER.");
				NETdestroyNETMESSAGE(message);
				break;
			}
			if ((receiver == selectedPlayer || receiver == NET_ALL_PLAYERS) && playerQueue.index == NetPlay.hostPlayer)
			{
				// Message was sent to us via the host.
				if (sender != selectedPlayer)  // TODO Tell host not to send us our own broadcast messages.
				{
					NETinsertMessageFromNet(NETnetQueue(sender), message);
				}
			}
			else if (NetPlay.isHost && sender == playerQueue.index)
			{
				// We are the host, and player is asking us to send the message to receiver.
				NETbeginEncode(NETnetQueue(receiver), NET_SEND_TO_PLAYER);
					NETuint8_t(&sender);
					NETuint8_t(&receiver);
					NETNETMESSAGE(&message);
				NETend();

				if (receiver == NET_ALL_PLAYERS)
				{
					NETinsertMessageFromNet(NETnetQueue(sender), message);  // Message is also for the host.
					// Not sure if flushing here can make a difference, maybe it can:
					//NETflush();  // Send the message to everyone as fast as possible.
				}
			}
			else
			{
				debug(LOG_ERROR, "Player %d sent us a NET_SEND_TO_PLAYER addressed to %d from %d. We are %d.", playerQueue.index, receiver, sender, selectedPlayer);
			}

			NETdestroyNETMESSAGE(message);
			break;
		}
		case NET_SHARE_GAME_QUEUE:
		{
			uint8_t player = 0;
			uint32_t num = 0, n;
			NETMESSAGE message = NULL;

			// Encoded in NETprocessSystemMessage in nettypes.cpp.
			NETbeginDecode(playerQueue, NET_SHARE_GAME_QUEUE);
				NETuint8_t(&player);
				NETuint32_t(&num);
				for (n = 0; n < num; ++n)
				{
					NETNETMESSAGE(&message);

					// TODO Check that playerQueue is actually responsible for this game queue.
					NETinsertMessageFromNet(NETgameQueue(player), message);

					NETdestroyNETMESSAGE(message);
					message = NULL;
				}
			if (!NETend() || player > MAX_PLAYERS)
			{
				debug(LOG_ERROR, "Bad NET_SHARE_GAME_QUEUE message.");
				break;
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
			uint32_t index = MAX_PLAYERS;
			int32_t colour = 0;
			int32_t position = 0;
			int32_t team = 0;
			uint32_t hostPlayer = 0;
			bool error = false;

			NETbeginDecode(playerQueue, NET_PLAYER_INFO);
				NETuint32_t(&indexLen);
				if (indexLen > MAX_PLAYERS || (playerQueue.index != NET_HOST_ONLY && indexLen > 1))
				{
					debug(LOG_ERROR, "MSG_PLAYER_INFO: Bad number of players updated");
					NETend();
					break;
				}

				for (n = 0; n < indexLen; ++n)
				{
					bool wasAllocated = false;
					char oldName[sizeof(NetPlay.players[index].name)];

					// Retrieve the player's ID
					NETuint32_t(&index);

					// Bail out if the given ID number is out of range
					if (index >= MAX_CONNECTED_PLAYERS || (playerQueue.index != NetPlay.hostPlayer && (playerQueue.index != index || !NetPlay.players[index].allocated)))
					{
						debug(LOG_ERROR, "MSG_PLAYER_INFO from %u: Player ID (%u) out of range (max %u)", playerQueue.index, index, (unsigned int)MAX_CONNECTED_PLAYERS);
						NETend();
						error = true;
						break;
					}

					// Retrieve the rest of the data
					wasAllocated = NetPlay.players[index].allocated;
					NETbool(&NetPlay.players[index].allocated);
					NETbool(&NetPlay.players[index].heartbeat);
					NETbool(&NetPlay.players[index].kick);
					strncpy(oldName, NetPlay.players[index].name, sizeof(NetPlay.players[index].name));
					NETstring(NetPlay.players[index].name, sizeof(NetPlay.players[index].name));
					NETuint32_t(&NetPlay.players[index].heartattacktime);
					NETint32_t(&colour);
					NETint32_t(&position);
					NETint32_t(&team);
					NETbool(&NetPlay.players[index].ready);

					// Don't let anyone except the host change these, otherwise it will end up inconsistent at some point, and the game gets really messed up.
					if (playerQueue.index == NetPlay.hostPlayer)
					{
						NetPlay.players[index].colour = colour;
						NetPlay.players[index].position = position;
						NetPlay.players[index].team = team;
						//NetPlay.hostPlayer = hostPlayer;  // Huh?
					}

					debug(LOG_NET, "%s for player %u (%s)", n == 0? "Receiving MSG_PLAYER_INFO" : "                      and", (unsigned int)index, NetPlay.players[index].allocated ? "human" : "AI");
					// update the color to the local array
					setPlayerColour(index, NetPlay.players[index].colour);

					if (wasAllocated && NetPlay.players[index].allocated && strncmp(oldName, NetPlay.players[index].name, sizeof(NetPlay.players[index].name)) != 0)
					{
						printConsoleNameChange(oldName, NetPlay.players[index].name);
					}
				}
				NETuint32_t(&hostPlayer);
			NETend();
			// If we're the game host make sure to send the updated
			// data to all other clients as well.
			if (NetPlay.isHost && !error)
			{
				NETBroadcastPlayerInfo(index);
				NETfixDuplicatePlayerNames();
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
				(unsigned int)index, tcp_socket);

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

			if(connected_bsocket[index])
			{
				debug(LOG_NET, "Receiving NET_PLAYER_LEAVING for player %u on socket %p", (unsigned int)index, connected_bsocket[index]);
			}
			else
			{	// dropped from join screen most likely
				debug(LOG_NET, "Receiving NET_PLAYER_LEAVING for player %u (no socket?)", (unsigned int)index);
			}

			if (NetPlay.isHost)
			{
				debug(LOG_NET, "Broadcast leaving message to everyone else");
				NETbeginEncode(NETbroadcastQueue(), NET_PLAYER_LEAVING);
				{
					BOOL host = NetPlay.isHost;
					uint32_t id = index;

					NETuint32_t(&id);
					NETbool(&host);
				}
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
static void NETcheckPlayers(void)
{
	int i;

	for (i = 0; i< MAX_PLAYERS ; i++)
	{
		if (NetPlay.players[i].allocated == 0) continue;		// not allocated means that it most like it is a AI player
		if (NetPlay.players[i].heartbeat == 0 && NetPlay.players[i].heartattacktime == 0)	// looks like they are dead
		{
			NetPlay.players[i].heartattacktime = gameTime2;		// mark when this occured
		}
		else
		{
			if (NetPlay.players[i].heartattacktime)
			{
				if (NetPlay.players[i].heartattacktime + (15 * GAME_TICKS_PER_SEC) <  gameTime2) // wait 15 secs
				{
					debug(LOG_NET, "Kicking due to client heart attack");
					NetPlay.players[i].kick = true;		// if still dead, then kick em.
				}
			}
		}
		if (NetPlay.players[i].kick)
		{
			debug(LOG_NET, "Kicking player %d", i);
			NETplayerDropped(i);
		}
	}
}

// ////////////////////////////////////////////////////////////////////////
// Receive a message over the current connection. We return true if there
// is a message for the higher level code to process, and false otherwise.
// We should not block here.
BOOL NETrecvNet(NETQUEUE *queue, uint8_t *type)
{
	uint32_t current;

	if (!NetPlay.bComms)
	{
		return false;
	}

	if (NetPlay.isHost)
	{
		NETallowJoining();
	}

	NETcheckPlayers();		// make sure players are still alive & well

	if (socket_set == NULL || checkSockets(socket_set, NET_READ_TIMEOUT) <= 0)
	{
		goto checkMessages;
	}

	for (current = 0; current < MAX_CONNECTED_PLAYERS; ++current)
	{
		Socket **pSocket = NetPlay.isHost ? &connected_bsocket[current] : &bsocket;
		uint8_t buffer[NET_BUFFER_SIZE];
		size_t dataLen;

		if (!NetPlay.isHost && current != NET_HOST_ONLY)
		{
			continue;  // Don't have a socket open to this player.
		}

		if (*pSocket == NULL)
		{
			continue;
		}

		dataLen = NET_fillBuffer(pSocket, socket_set, buffer, sizeof(buffer));
		if (dataLen > 0)
		{
			// we received some data, add to buffer
			NETinsertRawData(NETnetQueue(current), buffer, dataLen);
		}
		else if (*pSocket == NULL)
		{
			// If there is a error in NET_fillBuffer() then socket is already invalid.
			// This means that the player dropped / disconnected for whatever reason. 
			debug(LOG_INFO, "Player, (player %u) seems to have dropped/disconnected.", (unsigned)current);

			if (NetPlay.isHost)
			{
				// Send message type specifically for dropped / disconnects
				NETplayerDropped(current);
				NetPlay.players[current].kick = true;           // they are going to get kicked.
			}
		}
	}

checkMessages:
	for (current = 0; current < MAX_CONNECTED_PLAYERS; ++current)
	{
		*queue = NETnetQueue(current);
		while (NETisMessageReady(*queue))
		{
			*type = NETmessageType(NETgetMessage(*queue));
			if (!NETprocessSystemMessage(*queue, *type))
			{
				return true;  // We couldn't process the message, let the caller deal with it..
			}
		}
	}

	//NETlogPacket(pMsg, true);

	return false;
}

BOOL NETrecvGame(NETQUEUE *queue, uint8_t *type)
{
	uint32_t current;
	for (current = 0; current < MAX_PLAYERS; ++current)
	{
		*queue = NETgameQueue(current);
		while (!checkPlayerGameTime(current) && NETisMessageReady(*queue))  // Check for any messages that are scheduled to be read now.
		{
			*type = NETmessageType(NETgetMessage(*queue));

			if (*type == GAME_GAME_TIME)
			{
				recvPlayerGameTime(*queue);
				NETpop(*queue);
				continue;
			}

			if (!NETprocessSystemMessage(*queue, *type))
			{
				return true;  // We couldn't process the message, let the caller deal with it..
			}
			else
			{
				debug(LOG_ERROR, "There was a system message in a game queue...");
			}
		}

		if (!checkPlayerGameTime(current))
		{
			break;  // Still waiting for messages from this player, and all players should process messages in the same order.
		}
	}

	return false;
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
*  @TODO: more error checking (?) different file types (?)
*          Maybe should close file handle, and seek each time?
*     
*  @NOTE: MAX_FILE_TRANSFER_PACKET is set to 2k per packet since 7*2 = 14K which is pretty
*         much our limit.  Don't screw with that without having a bigger buffer!
*         NET_BUFFER_SIZE is at 16k.  (also remember text chat, plus all the other cruff)
*/
#define MAX_FILE_TRANSFER_PACKET 2048
UBYTE NETsendFile(char *fileName, UDWORD player)
{
	int32_t         bytesRead = 0;
	uint8_t		sendto = 0;
	uint8_t         inBuff[MAX_FILE_TRANSFER_PACKET];

	// We are not the host, so we don't care. (in fact, this would be a error)
	if (!NetPlay.isHost)
	{
		debug(LOG_ERROR, "trying to send a file and we are not the host!");
		return true;
	}

	memset(inBuff, 0x0, sizeof(inBuff));

	// read some bytes.
	bytesRead = PHYSFS_read(NetPlay.players[player].wzFile.pFileHandle, inBuff,1, MAX_FILE_TRANSFER_PACKET);
	sendto = (uint8_t) player;

	NETbeginEncode(NETnetQueue(sendto), NET_FILE_PAYLOAD);
		NETint32_t(&NetPlay.players[player].wzFile.fileSize_32);		// total bytes in this file. (we don't support 64bit yet)
		NETint32_t(&bytesRead);											// bytes in this packet
		NETint32_t(&NetPlay.players[player].wzFile.currPos);			// start byte
		NETstring(fileName, 256);										//256 = max filename size
		NETbin(inBuff, bytesRead);
	NETend();

	NetPlay.players[player].wzFile.currPos += bytesRead;		// update position!
	if(NetPlay.players[player].wzFile.currPos == NetPlay.players[player].wzFile.fileSize_32)
	{
		PHYSFS_close(NetPlay.players[player].wzFile.pFileHandle);
		NetPlay.players[player].wzFile.isSending = false;	// we are done sending to this client.
		NetPlay.players[player].needFile = false;
	}

	return (NetPlay.players[player].wzFile.currPos * 100) / NetPlay.players[player].wzFile.fileSize_32;
}

/* @TODO more error checking (?) different file types (?) */
// recv file. it returns % of the file so far recvd.
UBYTE NETrecvFile(NETQUEUE queue)
{
	int32_t		fileSize = 0, currPos = 0, bytesRead = 0;
	char		fileName[256];
	uint8_t         outBuff[MAX_FILE_TRANSFER_PACKET];
	static bool isLoop = false;

	memset(fileName, 0x0, sizeof(fileName));
	memset(outBuff, 0x0, sizeof(outBuff));

	//read incoming bytes.
	NETbeginDecode(queue, NET_FILE_PAYLOAD);
	NETint32_t(&fileSize);		// total bytes in this file.
	NETint32_t(&bytesRead);		// bytes in this packet
	NETint32_t(&currPos);		// start byte
	NETstring(fileName, 256);	// read filename (only valid on 1st packet)
	debug(LOG_NET, "Creating new file %s, position is %d", fileName, currPos);

	if (currPos == 0)	// first packet!
	{
		if (PHYSFS_exists(fileName))
		{
			PHYSFS_file *fin;
			PHYSFS_sint64 fsize;
			fin = PHYSFS_openRead(fileName);
			if (!fin)
			{
				// the file exists, but we can't open it, and I have no clue how to fix this...
				debug(LOG_FATAL, "PHYSFS_openRead(\"%s\") failed with error: %s\n", fileName, PHYSFS_getLastError());

				debug(LOG_NET, "We are leaving 'nicely' after a fatal error");
				NETbeginEncode(NETnetQueue(NET_HOST_ONLY), NET_PLAYER_LEAVING);
				{
					BOOL host = NetPlay.isHost;
					uint32_t id = selectedPlayer;

					NETuint32_t(&id);
					NETbool(&host);
				}
				NETend();

				abort();
			}
			else
			{
				fsize = PHYSFS_fileLength(fin);
			}
			if ((int32_t) fsize == fileSize)
			{
				uint32_t reason = ALREADY_HAVE_FILE;
				debug(LOG_NET, "We already have the file %s! ", fileName);
				PHYSFS_close(fin);
				NETend();

				NETbeginEncode(NETnetQueue(NET_HOST_ONLY), NET_FILE_CANCELLED);
					NETuint32_t(&selectedPlayer);
					NETuint32_t(&reason);
				NETend();
				if (!isLoop)
				{
					isLoop = true;
				}
				else
				{
					uint32_t reason = STUCK_IN_FILE_LOOP;
	
					NETend();
					// we should never get here, it means, that the game can't detect the level, but we have the file.
					// so we kick this player out.
					NETbeginEncode(NETnetQueue(NET_HOST_ONLY), NET_FILE_CANCELLED);
						NETuint32_t(&selectedPlayer);
						NETuint32_t(&reason);
					NETend();
					PHYSFS_close(NetPlay.pMapFileHandle);
					NetPlay.pMapFileHandle = NULL;
					debug(LOG_FATAL, "Something is really wrong with the file's (%s) data, game can't detect it?", fileName);
					return 100;
				}
			}
			PHYSFS_close(fin);

			debug(LOG_NET, "We already have the file %s, but different size %d vs %d.  Redownloading", fileName, (int32_t) fsize, fileSize);

		}
		NetPlay.pMapFileHandle = PHYSFS_openWrite(fileName);	// create a new file.
	}

	if (!NetPlay.pMapFileHandle) // file can't be opened
	{
		debug(LOG_FATAL, "Fatal error while creating file: %s", PHYSFS_getLastError());
		debug(LOG_FATAL, "Either we do not have write permission, or the Host sent us a invalid file (%s)!", fileName);
		abort();
	}

	NETbin(outBuff, bytesRead);
	NETend();

	//write packet to the file.
	PHYSFS_write(NetPlay.pMapFileHandle, outBuff, bytesRead, 1);

	if (currPos+bytesRead == fileSize)	// last packet
	{
		PHYSFS_close(NetPlay.pMapFileHandle);
		NetPlay.pMapFileHandle = NULL;
	}

	//return the percentage count
	return ((currPos + bytesRead) * 100) / fileSize;
}

static ssize_t readLobbyResponse(Socket* sock, unsigned int timeout)
{
	uint32_t lobbyStatusCode;
	uint32_t MOTDLength;
	uint32_t buffer[2];
	ssize_t result, received = 0;

	// Get status and message length
	result = readAll(sock, &buffer, sizeof(buffer), timeout);
	if (result != sizeof(buffer))
		goto error;
	received += result;
	lobbyStatusCode = ntohl(buffer[0]);
	MOTDLength = ntohl(buffer[1]);

	// Get status message
	free(NetPlay.MOTD);
	NetPlay.MOTD = malloc(MOTDLength + 1);
	result = readAll(sock, NetPlay.MOTD, MOTDLength, timeout);
	if (result != MOTDLength)
		goto error;
	received += result;
	// NUL terminate string
	NetPlay.MOTD[MOTDLength] = '\0';

	if (lobbyStatusCode / 100 != 2) // Check whether status code is 2xx (success)
	{
		debug(LOG_ERROR, "Lobby error (%u): %s", (unsigned int)lobbyStatusCode, NetPlay.MOTD);
		return SOCKET_ERROR;
	}

	debug(LOG_NET, "Lobby success (%u): %s", (unsigned int)lobbyStatusCode, NetPlay.MOTD);
	return received;

error:
	if (result == SOCKET_ERROR)
	{
		free(NetPlay.MOTD);
		if (asprintf(&NetPlay.MOTD, "Error while connecting to the lobby server: %s\nMake sure port %d can receive incoming connections.", strSockError(getSockErr()), gameserver_port) == -1)
			NetPlay.MOTD = NULL;
		debug(LOG_ERROR, "%s", NetPlay.MOTD);
	}
	else
	{
		free(NetPlay.MOTD);
		if (asprintf(&NetPlay.MOTD, "Disconnected from lobby server. Failed to register game.") == -1)
			NetPlay.MOTD = NULL;
		debug(LOG_ERROR, "%s", NetPlay.MOTD);
	}

	return SOCKET_ERROR;
}

static void NETregisterServer(int state)
{
	static Socket* rs_socket[2] = {NULL, NULL};
	static int registered = 0;
	unsigned int i;

	if (server_not_there)
	{
		return;
	}

	if (state != registered)
	{
		switch(state)
		{
			case 1:
			{
				bool succesful = false;
				uint32_t gameId = 0;
				SocketAddress *const hosts = resolveHost(masterserver_name, masterserver_port);

				if (hosts == NULL)
				{
					debug(LOG_ERROR, "Cannot resolve masterserver \"%s\": %s", masterserver_name, strSockError(getSockErr()));
					free(NetPlay.MOTD);
					if (asprintf(&NetPlay.MOTD, _("Could not resolve masterserver name (%s)!"), masterserver_name) == -1)
						NetPlay.MOTD = NULL;
					server_not_there = true;
					return;
				}

				socketArrayClose(rs_socket, ARRAY_SIZE(rs_socket));  // Make sure there aren't any leftover sockets.
				socketArrayOpen(rs_socket, ARRAY_SIZE(rs_socket), hosts, 15000);
				deleteSocketAddress(hosts);

				if (rs_socket[0] == NULL)
				{
					debug(LOG_ERROR, "Cannot connect to masterserver \"%s:%d\": %s", masterserver_name, masterserver_port, strSockError(getSockErr()));
					free(NetPlay.MOTD);
					if (asprintf(&NetPlay.MOTD, _("Could not communicate with lobby server! Is TCP port %u open for outgoing traffic?"), masterserver_port) == -1)
						NetPlay.MOTD = NULL;
					server_not_there = true;
					return;
				}

				// Get a game ID
				if (writeAll(rs_socket[0], "gaId", sizeof("gaId")) == SOCKET_ERROR
				 || readAll(rs_socket[0], &gameId, sizeof(gameId), 10000) != sizeof(gameId))
				{
					free(NetPlay.MOTD);
					if (asprintf(&NetPlay.MOTD, "Failed to retrieve a game ID: %s", strSockError(getSockErr())) == -1)
						NetPlay.MOTD = NULL;
					debug(LOG_ERROR, "%s", NetPlay.MOTD);

					// The sockets have been invalidated, so get rid of it. (using them now may cause SIGPIPE).
					socketArrayClose(rs_socket, ARRAY_SIZE(rs_socket));
					server_not_there = true;
					return;
				}

				gamestruct.gameId = ntohl(gameId);
				debug(LOG_NET, "Using game ID: %u", (unsigned int)gamestruct.gameId);

				// Register our game with the server for all available address families
				for (i = 0; i < ARRAY_SIZE(rs_socket); ++i)
				{
					if (rs_socket[i] == NULL)
						continue;

					if (writeAll(rs_socket[i], "addg", sizeof("addg")) == SOCKET_ERROR
					    // and now send what the server wants
					 || !NETsendGAMESTRUCT(rs_socket[i], &gamestruct))
					{
						debug(LOG_ERROR, "Failed to register game with server: %s", strSockError(getSockErr()));
						socketClose(rs_socket[i]);
						rs_socket[i] = NULL;
					}
				}

				// Get the return codes
				for (i = 0; i < ARRAY_SIZE(rs_socket); ++i)
				{
					if (rs_socket[i] == NULL)
						continue;

					if (readLobbyResponse(rs_socket[i], NET_TIMEOUT_DELAY) == SOCKET_ERROR)
					{
						socketClose(rs_socket[i]);
						rs_socket[i] = NULL;
						continue;
					}

					succesful = true;
				}

				if (!succesful)
				{
					server_not_there = true;
					return;
				}
			}
			break;

			case 0:
				// we don't need this anymore, so clean up
				socketArrayClose(rs_socket, ARRAY_SIZE(rs_socket));
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
	UDWORD numgames = htonl(1);	// always 1 on normal server
	char buffer[5];
	ssize_t recv_result = 0;

	if (allow_joining == false) return;
	ASSERT(NetPlay.isHost, "Cannot receive joins if not host!");

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
		// initialize server socket set
		// FIXME: why is this not done in NETinit()?? - Per
		tmp_socket_set = allocSocketSet();
		if (tmp_socket_set == NULL)
		{
			debug(LOG_ERROR, "Cannot create socket set: %s", strSockError(getSockErr()));
			return;
		}
	}

	// Find the first empty socket slot
	for (i = 0; i < MAX_TMP_SOCKETS; ++i)
	{
		if (tmp_socket[i] == NULL)
		{
			break;
		}
	}
	if (i == MAX_TMP_SOCKETS)
	{
		// this should *never* happen, it would mean we are going to reuse a socket already in use.
		debug(LOG_ERROR, "all temp sockets are used up!");
		return;
	}

	// See if there's an incoming connection
	if (tmp_socket[i] == NULL // Make sure that we're not out of sockets
	 && (tmp_socket[i] = socketAccept(tcp_socket)) != NULL)
	{
		NETinitQueue(NETnetTmpQueue(i));
		SocketSet_AddSocket(tmp_socket_set, tmp_socket[i]);
		if (checkSockets(tmp_socket_set, NET_TIMEOUT_DELAY) > 0
		    && socketReadReady(tmp_socket[i])
		    && (recv_result = readNoInt(tmp_socket[i], buffer, 5))
		    && recv_result != SOCKET_ERROR)
		{
			if(strcmp(buffer, "list")==0)
			{
				debug(LOG_NET, "cmd: list.  Sending game list");
				if (writeAll(tmp_socket[i], &numgames, sizeof(numgames)) == SOCKET_ERROR)
				{
					// Write error, most likely client disconnect.
					debug(LOG_ERROR, "Failed to send message: %s", strSockError(getSockErr()));
					debug(LOG_ERROR, "Couldn't get list from server. Make sure required ports are open. (TCP 9998-9999)");
				}
				else
				{
					// get the correct player count after kicks / leaves
					gamestruct.desc.dwCurrentPlayers = NetPlay.playercount;
					debug(LOG_NET, "Sending update to server to reflect new player count %d", NetPlay.playercount);
					NETsendGAMESTRUCT(tmp_socket[i], &gamestruct);
				}

				debug(LOG_NET, "freeing temp socket %p (%d)", tmp_socket[i], __LINE__);
				SocketSet_DelSocket(tmp_socket_set, tmp_socket[i]);
				socketClose(tmp_socket[i]);
				tmp_socket[i] = NULL;
			}
			else if (strcmp(buffer, "join") == 0)
			{
				debug(LOG_NET, "cmd: join.  Sending GAMESTRUCT");
				if (!NETsendGAMESTRUCT(tmp_socket[i], &gamestruct))
				{
					debug(LOG_ERROR, "Failed to respond (with GAMESTRUCT) to 'join' command, socket (%p) error: %s", tmp_socket[i], strSockError(getSockErr()));
					SocketSet_DelSocket(tmp_socket_set, tmp_socket[i]);
					socketClose(tmp_socket[i]);
					tmp_socket[i] = NULL;
				}
				socketBeginCompression(tmp_socket[i]);
			}
			else
			{
				debug(LOG_NET, "freeing temp socket %p (%d)", tmp_socket[i], __LINE__);
				SocketSet_DelSocket(tmp_socket_set, tmp_socket[i]);
				socketClose(tmp_socket[i]);
				tmp_socket[i] = NULL;
			}
		}
		else
		{
			debug(LOG_NET, "freeing temp socket %p (%d)", tmp_socket[i], __LINE__);
			SocketSet_DelSocket(tmp_socket_set, tmp_socket[i]);
			socketClose(tmp_socket[i]);
			tmp_socket[i] = NULL;
		}
	}

	if (checkSockets(tmp_socket_set, NET_READ_TIMEOUT) > 0)
	{
		for(i = 0; i < MAX_TMP_SOCKETS; ++i)
		{
			if (   tmp_socket[i] != NULL
			    && socketReadReady(tmp_socket[i]))
			{
				uint8_t buffer[NET_BUFFER_SIZE];
				ssize_t size = readNoInt(tmp_socket[i], buffer, sizeof(buffer));

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
					debug(LOG_NET, "freeing temp socket %p (%d)", tmp_socket[i], __LINE__);
					SocketSet_DelSocket(tmp_socket_set, tmp_socket[i]);
					socketClose(tmp_socket[i]);
					tmp_socket[i] = NULL;
					continue;
				}

				NETinsertRawData(NETnetTmpQueue(i), buffer, size);

				if (NETisMessageReady(NETnetTmpQueue(i)) && NETmessageType(NETgetMessage(NETnetTmpQueue(i))) == NET_JOIN)
				{
					uint8_t j;
					uint8_t index;
					uint8_t rejected = 0;
					int tmp;

					char name[64];
					int32_t MajorVersion = 0;
					int32_t MinorVersion = 0;
					char ModList[modlist_string_size] = { '\0' };
					char GamePassword[password_string_size] = { '\0' };

					if (onBanList(clientAddress))
					{
						char buf[256] = {'\0'};

						ssprintf(buf, "** A player that you have kicked tried to rejoin the game, and was rejected. IP:%s", clientAddress );
						debug(LOG_INFO, "%s", buf);
						NETlogEntry(buf, SYNC_FLAG, i);
						SocketSet_DelSocket(tmp_socket_set, tmp_socket[i]);
						socketClose(tmp_socket[i]);
						tmp_socket[i] = NULL;
						sync_counter.rejected++;
						return;
					}

					NETbeginDecode(NETnetTmpQueue(i), NET_JOIN);
						NETstring(name, sizeof(name));
						NETint32_t(&MajorVersion);	// NETCODE_VERSION_MAJOR
						NETint32_t(&MinorVersion);	// NETCODE_VERSION_MINOR
						NETstring(ModList, sizeof(ModList));
						NETstring(GamePassword, sizeof(GamePassword));
					NETend();
					NETpop(NETnetTmpQueue(i));

					tmp = NET_CreatePlayer(name);

					if (tmp == -1)
					{
						// FIXME: No room. Dropping the player without warning since protocol doesn't seem to support rejection at this point
						debug(LOG_ERROR, "freeing temp socket %p, couldn't create player!", tmp_socket[i]);
						SocketSet_DelSocket(tmp_socket_set, tmp_socket[i]);
						socketClose(tmp_socket[i]);
						tmp_socket[i] = NULL;
						sync_counter.cantjoin++;
						return;
					}

					index = tmp;

					debug(LOG_NET, "freeing temp socket %p (%d), creating permanent socket.", tmp_socket[i], __LINE__);
					SocketSet_DelSocket(tmp_socket_set, tmp_socket[i]);
					connected_bsocket[index] = tmp_socket[i];
					tmp_socket[i] = NULL;
					SocketSet_AddSocket(socket_set, connected_bsocket[index]);
					NETmoveQueue(NETnetTmpQueue(i), NETnetQueue(index));

					if (!NETisCorrectVersion(MajorVersion, MinorVersion))
					{
						// Wrong version. Reject.
						rejected = (uint8_t)ERROR_WRONGVERSION;
					}
					else if (NetPlay.GamePassworded && strcmp(NetPlay.gamePassword, GamePassword) != 0)
					{
						// Wrong password. Reject.
						rejected = (uint8_t)ERROR_WRONGPASSWORD;
					}
					else if (NetPlay.playercount > gamestruct.desc.dwMaxPlayers)
					{
						// Game full. Reject.
						rejected = (uint8_t)ERROR_FULL;
					}
					else if (strcmp(getModList(), ModList) != 0)
					{
						// Incompatible mods. Reject.
						rejected = (uint8_t)ERROR_WRONGDATA;
					}

					if (rejected)
					{
						debug(LOG_INFO, "We were rejected, reason (%u)", (unsigned int) rejected);
						//NETlogEntry(buf, SYNC_FLAG, index);  // buf undeclared in newnet branch.
						NETbeginEncode(NETnetQueue(index), NET_REJECTED);
							NETuint8_t(&rejected);
						NETend();
						NETflush();

						allow_joining = false; // no need to inform master server
						NET_DestroyPlayer(index);
						allow_joining = true;

						SocketSet_DelSocket(socket_set, connected_bsocket[index]);
						socketClose(connected_bsocket[index]);
						connected_bsocket[index] = NULL;
						return;
					}

					sstrcpy(NetPlay.players[index].IPtextAddress, clientAddress);
					{
						char buf[250] = {'\0'};
						snprintf(buf, sizeof(buf), "Player %d has joined, IP is:%s", index, clientAddress);
						NETlogEntry(buf, SYNC_FLAG, index);
					}

					NETbeginEncode(NETnetQueue(index), NET_ACCEPTED);
					NETuint8_t(&index);
					NETend();

					// First send info about players to newcomer.
					NETSendAllPlayerInfoTo(index);
					// then send info about newcomer to all players.
					NETBroadcastPlayerInfo(index);

					debug(LOG_NET, "Player, %s, with index of %u has joined using socket %p", name, (unsigned int)index, connected_bsocket[index]);

					// Increment player count
					gamestruct.desc.dwCurrentPlayers++;

					MultiPlayerJoin(index);

					// Narrowcast to new player that everyone has joined.
					for (j = 0; j < MAX_CONNECTED_PLAYERS; ++j)
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

					for (j = 0; j < MAX_CONNECTED_PLAYERS; ++j)
					{
						NETBroadcastPlayerInfo(j);
					}
					NETfixDuplicatePlayerNames();

					// Make sure the master server gets updated by disconnecting from it
					// NETallowJoining will reconnect
					NETregisterServer(0);
					// reset flags for new players
					NetPlay.players[index].wzFile.isCancelled = false;
					NetPlay.players[index].wzFile.isSending = false;
					NetPlay.players[index].needFile = false;
				}
			}
		}
	}
}

BOOL NEThostGame(const char* SessionName, const char* PlayerName,
		 SDWORD one, SDWORD two, SDWORD three, SDWORD four,
		 UDWORD plyrs)	// # of players.
{
	unsigned int i;

	debug(LOG_NET, "NEThostGame(%s, %s, %d, %d, %d, %d, %u)", SessionName, PlayerName,
	      one, two, three, four, plyrs);

	mapDownloadProgress = 100;
	netPlayersUpdated = true;

	if (NetPlay.bComms && NetPlay.isUPNP)
	{
		NETaddRedirects();
	}
	NET_InitPlayers();
	NetPlay.maxPlayers = MAX_PLAYERS;
	if(!NetPlay.bComms)
	{
		selectedPlayer			= 0;
		NetPlay.isHost			= true;
		NetPlay.players[0].allocated	= true;
		NetPlay.players[0].connection	= -1;
		NetPlay.playercount		= 1;
		debug(LOG_NET, "Hosting but no comms");
		// Now switch player color of the host to what they normally use for SP games
		if ( getPlayerColour(NET_HOST_ONLY) != war_GetSPcolor())
		{
			changeColour(NET_HOST_ONLY, war_GetSPcolor());
		}
		return true;
	}

	// tcp_socket is the connection to the lobby server (or machine)
	if (!tcp_socket)
		tcp_socket = socketListen(gameserver_port);
	if(tcp_socket == NULL)
	{
		debug(LOG_ERROR, "Cannot connect to master self: %s", strSockError(getSockErr()));
		return false;
	}
	debug(LOG_NET, "New tcp_socket = %p", tcp_socket);
	// Host needs to create a socket set for MAX_PLAYERS
	if(!socket_set) socket_set = allocSocketSet();
	if (socket_set == NULL)
	{
		debug(LOG_ERROR, "Cannot create socket set: %s", strSockError(getSockErr()));
		return false;
	}
	// allocate socket storage for all possible players
	for (i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
	{
		connected_bsocket[i] = NULL;
		NETinitQueue(NETnetQueue(i));
	}

	NetPlay.isHost = true;
	NETlogEntry("Hosting game, resetting ban list.", SYNC_FLAG, 0);
	if (IPlist)
	{ 
		free(IPlist);
		IPlist = NULL;
	}
	sstrcpy(gamestruct.name, SessionName);
	memset(&gamestruct.desc, 0, sizeof(gamestruct.desc));
	gamestruct.desc.dwSize = sizeof(gamestruct.desc);
	//gamestruct.desc.guidApplication = GAME_GUID;
	memset(gamestruct.desc.host, 0, sizeof(gamestruct.desc.host));
	gamestruct.desc.dwCurrentPlayers = 1;
	gamestruct.desc.dwMaxPlayers = plyrs;
	gamestruct.desc.dwFlags = 0;
	gamestruct.desc.dwUserFlags[0] = one;
	gamestruct.desc.dwUserFlags[1] = two;
	gamestruct.desc.dwUserFlags[2] = three;
	gamestruct.desc.dwUserFlags[3] = four;
	memset(gamestruct.secondaryHosts, 0, sizeof(gamestruct.secondaryHosts));
	sstrcpy(gamestruct.extra, "Extra");						// extra string (future use)
	sstrcpy(gamestruct.mapname, game.map);					// map we are hosting
	sstrcpy(gamestruct.hostname, PlayerName);
	sstrcpy(gamestruct.versionstring, VersionString);		// version (string)
	sstrcpy(gamestruct.modlist, getModList());				// List of mods
	gamestruct.GAMESTRUCT_VERSION = 3;						// version of this structure
	gamestruct.game_version_major = NETCODE_VERSION_MAJOR;	// Netcode Major version
	gamestruct.game_version_minor = NETCODE_VERSION_MINOR;	// NetCode Minor version
//	gamestruct.privateGame = 0;								// if true, it is a private game
	gamestruct.pureGame = 0;									// NO mods allowed if true
	gamestruct.Mods = 0;										// number of concatenated mods?
	gamestruct.gameId  = 0;
	gamestruct.future2 = 0xBAD02;								// for future use
	gamestruct.future3 = 0xBAD03;								// for future use
	gamestruct.future4 = 0xBAD04;								// for future use

	selectedPlayer= NET_CreatePlayer(PlayerName);
	realSelectedPlayer = selectedPlayer;
	NetPlay.isHost	= true;
	NetPlay.isHostAlive = true;
	NetPlay.hostPlayer	= NET_HOST_ONLY;
	ASSERT(selectedPlayer == NET_HOST_ONLY, "For now, host must start at player index zero, was %d", (int)selectedPlayer);

	MultiPlayerJoin(selectedPlayer);

	// Now switch player color of the host to what they normally use for SP games
	if ( getPlayerColour(NET_HOST_ONLY) != war_GetSPcolor())
	{
		changeColour(NET_HOST_ONLY, war_GetSPcolor());
	}

	allow_joining = true;

	NETregisterServer(0);

	debug(LOG_NET, "Hosting a server. We are player %d.", selectedPlayer);

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
	SocketAddress* hosts;
	unsigned int gamecount = 0;
	uint32_t gamesavailable;
	unsigned int port = (hostname == masterserver_name) ? masterserver_port : gameserver_port;
	int result = 0;
	debug(LOG_NET, "Looking for games...");
	
	if (getLobbyError() == ERROR_CHEAT || getLobbyError() == ERROR_KICKED)
	{
		return false;
	}
	setLobbyError(ERROR_NOERROR);

	NetPlay.games[0].desc.dwSize = 0;
	NetPlay.games[0].desc.dwCurrentPlayers = 0;
	NetPlay.games[0].desc.dwMaxPlayers = 0;

	if(!NetPlay.bComms)
	{
		selectedPlayer	= NET_HOST_ONLY;		// Host is always 0
		NetPlay.isHost		= true;
		NetPlay.hostPlayer	= NET_HOST_ONLY;
		return true;
	}
	// We first check to see if we were given a IP/hostname from the command line
	if (strlen(iptoconnect) )
	{
		hosts = resolveHost(iptoconnect, port);
		if (hosts == NULL)
		{
			debug(LOG_ERROR, "Error connecting to client via hostname provided (%s)",iptoconnect);
			debug(LOG_ERROR, "Cannot resolve hostname :%s",strSockError(getSockErr()));
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
	else if ((hosts = resolveHost(hostname, port)) == NULL)
	{
		debug(LOG_ERROR, "Cannot resolve hostname \"%s\": %s", hostname, strSockError(getSockErr()));
		setLobbyError(ERROR_CONNECTION);
		return false;
	}

	if (tcp_socket != NULL)
	{
		debug(LOG_NET, "Deleting tcp_socket %p", tcp_socket);
		if (socket_set)
		{
			SocketSet_DelSocket(socket_set, tcp_socket);
		}
		socketClose(tcp_socket);
		tcp_socket = NULL;
	}

	tcp_socket = socketOpenAny(hosts, 15000);
	
	deleteSocketAddress(hosts);
	hosts = NULL;

	if (tcp_socket == NULL)
	{
		debug(LOG_ERROR, "Cannot connect to \"%s:%d\": %s", hostname, port, strSockError(getSockErr()));
		setLobbyError(ERROR_CONNECTION);
		return false;
	}
	debug(LOG_NET, "New tcp_socket = %p", tcp_socket);
	// client machines only need 1 socket set
	socket_set = allocSocketSet();
	if (socket_set == NULL)
	{
		debug(LOG_ERROR, "Cannot create socket set: %s", strSockError(getSockErr()));
		setLobbyError(ERROR_CONNECTION);
		return false;
	}
	debug(LOG_NET, "Created socket_set %p", socket_set);

	SocketSet_AddSocket(socket_set, tcp_socket);

	debug(LOG_NET, "Sending list cmd");

	if (writeAll(tcp_socket, "list", sizeof("list")) != SOCKET_ERROR
	 && checkSockets(socket_set, NET_TIMEOUT_DELAY) > 0
	 && socketReadReady(tcp_socket)
	 && (result = readNoInt(tcp_socket, &gamesavailable, sizeof(gamesavailable))))
	{
		gamesavailable = MIN(ntohl(gamesavailable), ARRAY_SIZE(NetPlay.games));
	}
	else
	{
		if (result == SOCKET_ERROR)
		{
			debug(LOG_NET, "Server socket ecountered error: %s", strSockError(getSockErr()));
		}
		else
		{
			debug(LOG_NET, "Server didn't respond (timeout)");
		}
		SocketSet_DelSocket(socket_set, tcp_socket);		// mark it invalid
		socketClose(tcp_socket);
		tcp_socket = NULL;

		// when we fail to receive a game count, bail out
		setLobbyError(ERROR_CONNECTION);
		return false;
	}

	debug(LOG_NET, "receiving info on %u game(s)", (unsigned int)gamesavailable);

	// Clear old games from list.
	memset(NetPlay.games, 0x00, sizeof(NetPlay.games));

	while (gamecount < gamesavailable)
	{
		// Attempt to receive a game description structure
		if (!NETrecvGAMESTRUCT(&NetPlay.games[gamecount]))
		{
			debug(LOG_NET, "only %u game(s) received", (unsigned int)gamecount);
			// If we fail, success depends on the amount of games that we've read already
			return gamecount;
		}

		if (NetPlay.games[gamecount].desc.host[0] == '\0')
		{
			strncpy(NetPlay.games[gamecount].desc.host, getSocketTextAddress(tcp_socket), sizeof(NetPlay.games[gamecount].desc.host));
		}

		++gamecount;
	}

	return true;
}

// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Functions used to setup and join games.
BOOL NETjoinGame(UDWORD gameNumber, const char* playername)
{
	SocketAddress *hosts = NULL;
	unsigned int i;

	debug(LOG_NET, "resetting sockets.");
	NETclose();	// just to be sure :)

	debug(LOG_NET, "Trying to join gameNumber (%u)...", gameNumber);

	mapDownloadProgress = 100;
	netPlayersUpdated = true;

	if (hostname == masterserver_name)
	{
		hostname = NULL;
	}

	// Loop through all of the hosts, using the first one we can connect to.
	for (i = 0; i < ARRAY_SIZE(NetPlay.games[gameNumber].secondaryHosts) + 1; ++i)
	{
		free(hostname);
		if (i > 0)
		{
			hostname = strdup(NetPlay.games[gameNumber].secondaryHosts[i - 1]);
		}
		else
		{
			hostname = strdup(NetPlay.games[gameNumber].desc.host);
		}

		hosts = resolveHost(hostname, gameserver_port);
		if (hosts == NULL)
		{
			debug(LOG_ERROR, "Cannot resolve hostname \"%s\": %s", hostname, strSockError(getSockErr()));
			continue;
		}

		if (tcp_socket != NULL)
		{
			socketClose(tcp_socket);
		}

		tcp_socket = socketOpenAny(hosts, 15000);
		deleteSocketAddress(hosts);
		if (tcp_socket != NULL)
		{
			break;
		}
	}

	if (tcp_socket == NULL)
	{
		return false;
	}

	// client machines only need 1 socket set
	socket_set = allocSocketSet();
	if (socket_set == NULL)
	{
		debug(LOG_ERROR, "Cannot create socket set: %s", strSockError(getSockErr()));
 		return false;
 	}
	debug(LOG_NET, "Created socket_set %p", socket_set);

	// tcp_socket is used to talk to host machine
	SocketSet_AddSocket(socket_set, tcp_socket);

	if (writeAll(tcp_socket, "join", sizeof("join")) == SOCKET_ERROR)
	{
		debug(LOG_ERROR, "Failed to send 'join' command: %s", strSockError(getSockErr()));
		SocketSet_DelSocket(socket_set, tcp_socket);
		socketClose(tcp_socket);
		tcp_socket = NULL;
		deleteSocketSet(socket_set);
		socket_set = NULL;
		return false;
	}

	if (NETrecvGAMESTRUCT(&NetPlay.games[gameNumber])
	 && NetPlay.games[gameNumber].desc.host[0] == '\0')
	{
		strncpy(NetPlay.games[gameNumber].desc.host, getSocketTextAddress(tcp_socket), sizeof(NetPlay.games[gameNumber].desc.host));
	}
	if (NetPlay.games[gameNumber].desc.dwCurrentPlayers >= NetPlay.games[gameNumber].desc.dwMaxPlayers)
	{
		// Shouldn't join; game is full
		SocketSet_DelSocket(socket_set, tcp_socket);
		socketClose(tcp_socket);
		tcp_socket = NULL;
		deleteSocketSet(socket_set);
		socket_set = NULL;
		setLobbyError(ERROR_FULL);
		return false;
	}
	// Allocate memory for a new socket
	NETinitQueue(NETnetQueue(NET_HOST_ONLY));
	// NOTE: tcp_socket = bsocket now!
	bsocket = tcp_socket;
	socketBeginCompression(bsocket);

	// Send a join message to the host
	NETbeginEncode(NETnetQueue(NET_HOST_ONLY), NET_JOIN);
		// Casting constness away, because NETstring is const-incorrect
		// when sending/encoding a packet.
		NETstring((char*)playername, 64);
		NETint32_t(&NETCODE_VERSION_MAJOR);
		NETint32_t(&NETCODE_VERSION_MINOR);
		NETstring(getModList(), modlist_string_size);
		NETstring(NetPlay.gamePassword, sizeof(NetPlay.gamePassword));
	NETend();
	socketFlush(bsocket);  // Make sure the message was completely sent.

	i = SDL_GetTicks();
	// Loop until we've been accepted into the game
	for (;;)
	{
		NETQUEUE queue;
		uint8_t type;

		// FIXME: shouldn't there be some sort of rejection message?
		if (SDL_GetTicks() > i + 5000)
		{
			// timeout
			return false;
		}

		if (!NETrecvNet(&queue, &type))
		{
			continue;
		}

		if (type == NET_ACCEPTED)
		{
			// :)
			uint8_t index;

			NETbeginDecode(queue, NET_ACCEPTED);
				// Retrieve the player ID the game host arranged for us
				NETuint8_t(&index);
			NETend();
			NETpop(queue);

			selectedPlayer = index;
			realSelectedPlayer = selectedPlayer;
			debug(LOG_NET, "NET_ACCEPTED received. Accepted into the game - I'm player %u using bsocket %p, tcp_socket=%p", (unsigned int)index, bsocket, tcp_socket);
			NetPlay.isHost = false;
			NetPlay.isHostAlive = true;

			if (index >= MAX_CONNECTED_PLAYERS)
			{
				debug(LOG_ERROR, "Bad player number (%u) received from host!", index);
				return false;
			}

			NetPlay.players[index].allocated = true;
			sstrcpy(NetPlay.players[index].name, playername);
			NetPlay.players[index].heartbeat = true;

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
		}

		NETpop(queue);
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


void NETsetPlayerConnectionStatus(CONNECTION_STATUS status, unsigned player)
{
	unsigned n;
	const int timeouts[] = {GAME_TICKS_PER_SEC*10, GAME_TICKS_PER_SEC*10, GAME_TICKS_PER_SEC, GAME_TICKS_PER_SEC/6};
	ASSERT(ARRAY_SIZE(timeouts) == CONNECTIONSTATUS_NORMAL, "Connection status timeout array too small.");

	if (status == CONNECTIONSTATUS_NORMAL)
	{
		for (n = 0; n < CONNECTIONSTATUS_NORMAL; ++n)
		{
			NET_PlayerConnectionStatus[n][player] = 0;
		}
		return;
	}
	if (player == NET_ALL_PLAYERS)
	{
		for (n = 0; n < MAX_PLAYERS; ++n)
		{
			NETsetPlayerConnectionStatus(status, n);
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
		for (n = 0; n < MAX_PLAYERS; ++n)
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

#define MAX_LEN_LOG_LINE 512  // From debug.c - no use printing something longer.
#define MAX_SYNC_MESSAGES 20000
#define MAX_SYNC_HISTORY 12

static unsigned syncDebugNext = 0;
static uint32_t syncDebugNum[MAX_SYNC_HISTORY];
static uint32_t syncDebugGameTime[MAX_SYNC_HISTORY + 1];
static char const *syncDebugFunctions[MAX_SYNC_HISTORY][MAX_SYNC_MESSAGES];
static char *syncDebugStrings[MAX_SYNC_HISTORY][MAX_SYNC_MESSAGES];
static uint32_t syncDebugCrcs[MAX_SYNC_HISTORY + 1];

void _syncDebug(const char *function, const char *str, ...)
{
	va_list ap;
	char outputBuffer[MAX_LEN_LOG_LINE];

	va_start(ap, str);
	vssprintf(outputBuffer, str, ap);
	va_end(ap);

	if (syncDebugNum[syncDebugNext] < MAX_SYNC_MESSAGES)
	{
		syncDebugFunctions[syncDebugNext][syncDebugNum[syncDebugNext]] = function;  // Function names are link-time constants, no need to duplicate.
		syncDebugStrings[syncDebugNext][syncDebugNum[syncDebugNext]] = strdup(outputBuffer);
		syncDebugCrcs[syncDebugNext] = crcSum(syncDebugCrcs[syncDebugNext], function,     strlen(function)     + 1);
		syncDebugCrcs[syncDebugNext] = crcSum(syncDebugCrcs[syncDebugNext], outputBuffer, strlen(outputBuffer) + 1);
		++syncDebugNum[syncDebugNext];
	}
}

void _syncDebugBacktrace(const char *function)
{
	uint32_t backupCrc = syncDebugCrcs[syncDebugNext];  // Ignore CRC changes from _syncDebug(), since identical backtraces can be printed differently.

#ifdef WZ_OS_LINUX
	void *btv[20];
	unsigned num = backtrace(btv, sizeof(btv)/sizeof(*btv));
	char **btc = backtrace_symbols(btv, num);
	unsigned i;
	for (i = 1; i + 2 < num; ++i)  // =1: Don't print "src/warzone2100(syncDebugBacktrace+0x16) [0x6312d1]". +2: Don't print last two lines of backtrace such as "/lib/libc.so.6(__libc_start_main+0xe6) [0x7f91e040ea26]", since the address varies (even with the same binary).
	{
		_syncDebug("BT", "%s", btc[i]);
	}
	free(btc);
#else
	_syncDebug("BT", "Sorry, syncDebugBacktrace() not implemented on your system. Called from %s.", function);
#endif

	// Use CRC of something platform-independent, to avoid false positive desynchs.
	syncDebugCrcs[syncDebugNext] = crcSum(backupCrc, function, strlen(function) + 1);
}

static void clearSyncDebugNext(void)
{
	unsigned i;

	for (i = 0; i != syncDebugNum[syncDebugNext]; ++i)
	{
		free(syncDebugStrings[syncDebugNext][i]);
		syncDebugFunctions[syncDebugNext][i] = NULL;  // Function names are link-time constants, and therefore shouldn't and can't be freed.
		syncDebugStrings[syncDebugNext][i] = NULL;
	}
	syncDebugNum[syncDebugNext] = 0;
	syncDebugGameTime[syncDebugNext] = 0;
	syncDebugCrcs[syncDebugNext] = 0x00000000;
}

void resetSyncDebug()
{
	for (syncDebugNext = 0; syncDebugNext < MAX_SYNC_HISTORY; ++syncDebugNext)
	{
		clearSyncDebugNext();
	}

	syncDebugGameTime[MAX_SYNC_HISTORY] = 0;
	syncDebugCrcs[MAX_SYNC_HISTORY] = 0x00000000;

	syncDebugNext = 0;
}

uint32_t nextDebugSync(void)
{
	uint32_t ret = ~syncDebugCrcs[syncDebugNext];  // Invert bits, since everyone else seems to do that with CRCs...

	// Save gameTime, so we know which CRC to compare with, later.
	syncDebugGameTime[syncDebugNext] = gameTime;

	// Go to next position, and free it ready for use.
	syncDebugNext = (syncDebugNext + 1)%MAX_SYNC_HISTORY;
	clearSyncDebugNext();

	return ret;
}

static void dumpDebugSync(uint8_t *buf, size_t bufLen, uint32_t time, unsigned player)
{
	char fname[100];
	PHYSFS_file *fp;

	ssprintf(fname, "logs/desync%u_p%u.txt", time, player);
	fp = openSaveFile(fname);
	PHYSFS_write(fp, buf, bufLen, 1);
	PHYSFS_close(fp);

	debug(LOG_ERROR, "Dumped player %u's sync error at gameTime %u to file: %s%s", player, time, PHYSFS_getRealDir(fname), fname);
}

static void sendDebugSync(uint8_t *buf, uint32_t bufLen, uint32_t time)
{
	// Save our own, before sending, so that if we have 2 clients running on the same computer, to guarantee that it is done saving before the other client saves on top.
	dumpDebugSync(buf, bufLen, time, selectedPlayer);

	NETbeginEncode(NETbroadcastQueue(), NET_DEBUG_SYNC);
		NETuint32_t(&time);
		NETuint32_t(&bufLen);
		NETbin(buf, bufLen);
	NETend();
}

static uint8_t debugSyncTmpBuf[2000000];
static void recvDebugSync(NETQUEUE queue)
{
	uint32_t time = 0;
	uint32_t bufLen = 0;

	NETbeginDecode(queue, NET_DEBUG_SYNC);
		NETuint32_t(&time);
		NETuint32_t(&bufLen);
		bufLen = MIN(bufLen, ARRAY_SIZE(debugSyncTmpBuf));
		NETbin(debugSyncTmpBuf, bufLen);
	NETend();

	dumpDebugSync(debugSyncTmpBuf, bufLen, time, queue.index);
}

bool checkDebugSync(uint32_t checkGameTime, uint32_t checkCrc)
{
	unsigned index;
	unsigned i;
	static uint32_t numDumps = 0;
	size_t bufSize = 0;

	if (checkGameTime == syncDebugGameTime[syncDebugNext])  // Can't happen - and syncDebugGameTime[] == 0, until just before sending the CRC, anyway.
	{
		debug(LOG_ERROR, "Huh? We aren't done yet...");
		return true;
	}

	for (index = 0; index < MAX_SYNC_HISTORY + 1; ++index)
	{
		if (syncDebugGameTime[index] == checkGameTime)
		{
			if (~syncDebugCrcs[index] == checkCrc)  // Invert bits, since everyone else seems to do that with CRCs...
			{
				return true;                    // Check passed. (So far... There might still be more players to compare CRCs with.)
			}

			break;                                  // Check failed!
		}
	}

	if (index >= MAX_SYNC_HISTORY)
	{
		return false;                                   // Couldn't check. May have dumped already, or MAX_SYNC_HISTORY isn't big enough compared to the maximum latency.
	}

	// Dump our version, and also erase it, so we only dump it at most once.
	debug(LOG_ERROR, "Inconsistent sync debug at gameTime %u. My version has %u lines, CRC = 0x%08X.", syncDebugGameTime[index], syncDebugNum[index], ~syncDebugCrcs[index] & 0xFFFFFFFF);
	bufSize += snprintf((char *)debugSyncTmpBuf + bufSize, ARRAY_SIZE(debugSyncTmpBuf) - bufSize, "===== BEGIN gameTime=%u, %u lines, CRC 0x%08X =====\n", syncDebugGameTime[index], syncDebugNum[index], ~syncDebugCrcs[index] & 0xFFFFFFFF);
	bufSize = MIN(bufSize, ARRAY_SIZE(debugSyncTmpBuf));  // snprintf will not overflow debugSyncTmpBuf, but returns as much as it would have printed if possible.
	for (i = 0; i < syncDebugNum[index]; ++i)
	{
		bufSize += snprintf((char *)debugSyncTmpBuf + bufSize, ARRAY_SIZE(debugSyncTmpBuf) - bufSize, "[%s] %s\n", syncDebugFunctions[index][i], syncDebugStrings[index][i]);
		bufSize = MIN(bufSize, ARRAY_SIZE(debugSyncTmpBuf));  // snprintf will not overflow debugSyncTmpBuf, but returns as much as it would have printed if possible.
		free(syncDebugStrings[index][i]);
	}
	bufSize += snprintf((char *)debugSyncTmpBuf + bufSize, ARRAY_SIZE(debugSyncTmpBuf) - bufSize, "===== END gameTime=%u, %u lines, CRC 0x%08X =====\n", syncDebugGameTime[index], syncDebugNum[index], ~syncDebugCrcs[index] & 0xFFFFFFFF);
	bufSize = MIN(bufSize, ARRAY_SIZE(debugSyncTmpBuf));  // snprintf will not overflow debugSyncTmpBuf, but returns as much as it would have printed if possible.
	if (numDumps < 5)
	{
		++numDumps;
		sendDebugSync(debugSyncTmpBuf, bufSize, syncDebugGameTime[index]);
	}

	// Backup correct CRC for checking against remaining players, even though we erased the logs (which were dumped already).
	syncDebugGameTime[MAX_SYNC_HISTORY] = syncDebugGameTime[index];
	syncDebugCrcs[MAX_SYNC_HISTORY]     = syncDebugCrcs[index];

	// Finish erasing our version.
	syncDebugNum[index] = 0;
	syncDebugGameTime[index] = 0;
	syncDebugCrcs[index] = 0x00000000;

	return false;  // Ouch.
}

const char *messageTypeToString(unsigned messageType_)
{
	MESSAGE_TYPES messageType = (MESSAGE_TYPES)messageType_;  // Cast to enum, so switch gives a warning if new message types are added without updating the switch.

	switch (messageType)
	{
		// Search:  \s*([\w_]+).*
		// Replace: case \1:                             return "\1";
		// Search:  (case ...............................) *(return "[\w_]+";)
		// Replace: \t\t\1\2

		case NET_MIN_TYPE:                  return "NET_MIN_TYPE";
		case NET_PING:                      return "NET_PING";
		case NET_PLAYER_STATS:              return "NET_PLAYER_STATS";
		case NET_TEXTMSG:                   return "NET_TEXTMSG";
		case NET_PLAYERRESPONDING:          return "NET_PLAYERRESPONDING";
		case NET_OPTIONS:                   return "NET_OPTIONS";
		case NET_KICK:                      return "NET_KICK";
		case NET_FIREUP:                    return "NET_FIREUP";
		case NET_COLOURREQUEST:             return "NET_COLOURREQUEST";
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
		case NET_MAX_TYPE:                  return "NET_MAX_TYPE";

		case GAME_MIN_TYPE:                 return "GAME_MIN_TYPE";
		case GAME_DROID:                    return "GAME_DROID";
		case GAME_DROIDINFO:                return "GAME_DROIDINFO";
		case GAME_TEMPLATE:                 return "GAME_TEMPLATE";
		case GAME_TEMPLATEDEST:             return "GAME_TEMPLATEDEST";
		case GAME_FEATUREDEST:              return "GAME_FEATUREDEST";
		case GAME_RESEARCH:                 return "GAME_RESEARCH";
		case GAME_FEATURES:                 return "GAME_FEATURES";
		case GAME_SECONDARY:                return "GAME_SECONDARY";
		case GAME_ALLIANCE:                 return "GAME_ALLIANCE";
		case GAME_GIFT:                     return "GAME_GIFT";
		case GAME_ARTIFACTS:                return "GAME_ARTIFACTS";
		case GAME_RESEARCHSTATUS:           return "GAME_RESEARCHSTATUS";
		case GAME_STRUCTUREINFO:            return "GAME_STRUCTUREINFO";
		case GAME_LASSAT:                   return "GAME_LASSAT";
		case GAME_GAME_TIME:                return "GAME_GAME_TIME";
		case GAME_PLAYER_LEFT:              return "GAME_PLAYER_LEFT";
		case GAME_DROIDDEST:                return "GAME_DROIDDEST";
		case GAME_CHECK_DROID:              return "GAME_CHECK_DROID";
		case GAME_CHECK_STRUCT:             return "GAME_CHECK_STRUCT";
		case GAME_CHECK_POWER:              return "GAME_CHECK_POWER";
		case GAME_STRUCTDEST:               return "GAME_STRUCTDEST";
		case GAME_BUILDFINISHED:            return "GAME_BUILDFINISHED";
		case GAME_DEMOLISH:                 return "GAME_DEMOLISH";
		case GAME_DROIDEMBARK:              return "GAME_DROIDEMBARK";
		case GAME_DROIDDISEMBARK:           return "GAME_DROIDDISEMBARK";
		case GAME_MAX_TYPE:                 return "GAME_MAX_TYPE";
	}
	return "(INVALID MESSAGE TYPE)";
}

/**
 * Check if ip is on the banned list.
 * \param ip IP address converted to text
 */
static bool onBanList(const char *ip)
{
	int i;

	if (!IPlist) return false;		//if no bans are added, then don't check.
	for(i = 0; i < MAX_BANS ; i++)
	{
		if (strcmp(ip, IPlist[i].IPAddress) == 0)
		{
			return true;
		}
	}
	return false;
}

/**
 * Create the banned list.
 * \param ip IP address in text format
 * \param name Name of the player we are banning
 */
static void addToBanList(const char *ip, const char *name)
{
	static int numBans = 0;

	if (!IPlist)
	{
		IPlist = malloc(sizeof(PLAYER_IP) * MAX_BANS + 1);
		if (!IPlist)
		{
			debug(LOG_FATAL, "Out of memory!");
			abort();
		}
		numBans = 0;
	}
	memset(IPlist, 0x0, sizeof(PLAYER_IP) * MAX_BANS);
	sstrcpy(IPlist[numBans].IPAddress, ip);
	sstrcpy(IPlist[numBans].pname, name);
	numBans++;
	sync_counter.banned++;
	if (numBans > MAX_BANS)
	{
		debug(LOG_INFO, "We have exceeded %d bans, resetting to 0", MAX_BANS);
		numBans = 0;
	}
}
