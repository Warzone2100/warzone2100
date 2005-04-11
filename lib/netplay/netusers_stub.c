/* 
 * netusers.c
 * functions regarding specific players
 */

#include "frame.h"
#include "netplay.h"

VOID			*pSingleUserData;		// single player mode. a local copy...
DWORD			userDataSize=0;

// call to disable/enable ALL comms. Absolute arse of a thing, be very careful!
BOOL NETuseNetwork(BOOL val)
{
	if(val)
	{
		NetPlay.bComms = TRUE;
	}
	else
	{
		NetPlay.bComms = FALSE;
	}
	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////
BOOL NETgetLocalPlayerData(DPID dpid,VOID *pData, DWORD *pSize)
{
	if(!NetPlay.bComms)
	{
		memcpy(pData,pSingleUserData,userDataSize);
		return TRUE;
	}

	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////
BOOL NETgetGlobalPlayerData(DPID dpid,VOID *pData, DWORD *pSize)
{
	if(!NetPlay.bComms)
	{	
		memcpy(pData,pSingleUserData,userDataSize);
		return TRUE;
	}

	return FALSE;
}
// ////////////////////////////////////////////////////////////////////////
BOOL NETsetLocalPlayerData(DPID dpid,VOID *pData, DWORD size)
{
	if(!NetPlay.bComms)
	{	
		if(userDataSize ==0)
		{
			pSingleUserData = MALLOC(size);
			userDataSize = size;
		}
		memcpy(pSingleUserData,pData,userDataSize);
		return TRUE;
	}

	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////
BOOL NETsetGlobalPlayerData(DPID dpid,VOID *pData, DWORD size)
{
	 
	if(!NetPlay.bComms)
	{	
		if(userDataSize ==0)
		{
			pSingleUserData = MALLOC(size);
			userDataSize = size;
		}
		memcpy(pSingleUserData,pData,userDataSize);
		return TRUE;
	}


	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////
// count players. call with null to enumerate the game already joined.
UDWORD NETplayerInfo(LPGUID guidinstance)
{
	NetPlay.playercount =0;		// reset player counter

	if(!NetPlay.bComms)
	{
		NetPlay.playercount		= 1;
		NetPlay.players[0].bHost	= TRUE;
		NetPlay.players[0].bSpectator	= FALSE;
		NetPlay.players[0].dpid		= 1;
		return 1;
	}

	memcpy(NetPlay.players, 0, (sizeof(PLAYER)*MaxNumberOfPlayers));	// reset player info
	return 0;
}

// ////////////////////////////////////////////////////////////////////////
// rename the local player
//
// dont call this a lot, since it uses a guaranteed msg.
BOOL NETchangePlayerName(UDWORD dpid, char *newName)
{
	if(!NetPlay.bComms)
	{
		strcpy(NetPlay.players[0].name,newName);
		return TRUE;
	}

	return FALSE;
}


// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Functions for spectators.

BOOL NETspectate(GUID guidSessionInstance)
{
	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////
BOOL NETisSpectator(DPID dpid)
{
	UBYTE i;

	if(dpid = NetPlay.dpidPlayer == dpid)	// checking ourselves
	{
		return NetPlay.bSpectator;
	}

	// could enumerate the spectators and check if he's there!
	// bugger it, just check that dpid isn't a player instead!
	for(i=0;i<MaxNumberOfPlayers;i++)
	{
		if(NetPlay.players[i].dpid == dpid)
		{
			return FALSE;
		}
	}

	return TRUE;
}
