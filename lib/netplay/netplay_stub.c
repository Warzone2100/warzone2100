/*
 * Netplay.c
 *
 * Alex Lee sep97-> june98
 */

// ////////////////////////////////////////////////////////////////////////
// includes
#include "frame.h"
#include "netplay.h"
#include "netsupp.h"
#include <time.h>			//for stats

// ////////////////////////////////////////////////////////////////////////
// Variables 

NETPLAY                         NetPlay;

// ////////////////////////////////////////////////////////////////////////
typedef struct{					// data regarding the last one second or so.
	UDWORD		bytesRecvd;
	UDWORD		bytesSent;		// number of bytes sent in about 1 sec.
	UDWORD		packetsSent;			
	UDWORD		packetsRecvd;
}NETSTATS;

// ////////////////////////////////////////////////////////////////////////
// setup stuff
BOOL NETinit(GUID g,BOOL bFirstCall)
{
	UDWORD			i;

//	NEThashFile("warzonedebug.exe");

	if(bFirstCall)
	{
		DBPRINTF(("NETPLAY: Init called, MORNIN' \n "));
		
		NetPlay.bLobbyLaunched		= FALSE;				// clean up 
		NetPlay.lpDirectPlay4A		= NULL;				
		NetPlay.hPlayerEvent		= NULL;				
		NetPlay.dpidPlayer		= 0;				
		NetPlay.bHost			= 0;
		NetPlay.bComms			= FALSE; // was TRUE

		NetPlay.bEncryptAllPackets	= FALSE;
		NETsetKey(0x2fe8f810, 0xb72a5, 0x114d0, 0x2a7);	// j-random key to get us started

		NetPlay.bAllowCaptureRecord	= FALSE;
		NetPlay.bAllowCapturePlay	= FALSE;
		NetPlay.bCaptureInUse		= FALSE;


		for(i=0;i<MaxNumberOfPlayers;i++)
		{
			memset(&NetPlay.protocols[i], 0, sizeof(PROTO));
			memset(&NetPlay.players[i], 0, sizeof(PLAYER));
			memset(&NetPlay.games[i], 0, sizeof(GAMESTRUCT));
		}
		//GAME_GUID = g;	

		NETuseNetwork(FALSE);
		NETstartLogging();
	}
	
	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////
// SHUTDOWN THE CONNECTION.
HRESULT NETshutdown(VOID)
{
	return 0;
}

// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Send and Recv functions

// ////////////////////////////////////////////////////////////////////////
// return bytes of data sent recently.
UDWORD NETgetBytesSent(VOID)
{
	return 0;
}

UDWORD NETgetRecentBytesSent(VOID)
{
	return 0;
}


UDWORD NETgetBytesRecvd(VOID)
{
	return 0;
}

UDWORD NETgetRecentBytesRecvd(VOID)
{
	return 0;
}


//return number of packets sent last sec.
UDWORD NETgetPacketsSent(VOID)
{
	return 0;
}


UDWORD NETgetRecentPacketsSent(VOID)
{
	return 0;
}


UDWORD NETgetPacketsRecvd(VOID)
{
	return 0;
}

UDWORD NETgetRecentPacketsRecvd(VOID)
{
	return 0;
}


// ////////////////////////////////////////////////////////////////////////
// Send a message to a player, option to guarantee message
BOOL NETsend(NETMSG *msg, DPID player, BOOL guarantee)
{	
	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////
// broadcast a message to all players.
BOOL NETbcast(NETMSG *msg,BOOL guarantee)
{	
	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////
// receive a message over the current connection
BOOL NETrecv(NETMSG * pMsg)
{
	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Protocol functions

// select a current protocol. Call with a connection returned by findprotocol.
BOOL NETselectProtocol(LPVOID lpConnection)
{
	return FALSE;
}


// ////////////////////////////////////////////////////////////////////////
// call with true to enumerate available protocols.
BOOL NETfindProtocol(BOOL Lob)
{
	return FALSE;
}


// ////////////////////////////////////////////////////////////////////////
// File Transfer programs.
// uses guaranteed messages to send files between clients.

// send file. it returns % of file sent. when 100 it's complete. call until it returns 100.

UBYTE NETsendFile(BOOL newFile, CHAR *fileName, DPID player)
{
	return 0;
}


// recv file. it returns % of the file so far recvd.
UBYTE NETrecvFile(NETMSG *pMsg)
{
	return 0;
}
