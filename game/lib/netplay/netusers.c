/* 
 * netusers.c
 * functions regarding specific players
 */

#include "frame.h"
#include "netplay.h"

BOOL			NETuseNetwork			(BOOL val);
BOOL FAR PASCAL Playercounter			(DPID dpId,DWORD dwPlayerType,LPCDPNAME lpName,DWORD dwFlags,LPVOID lpContext);
UDWORD			NETplayerInfo			(LPGUID guidinstance);
BOOL			NETchangePlayerName		(UDWORD dpid, char *newName);
BOOL			NETgetLocalPlayerData	(DPID dpid,VOID *pData, DWORD *pSize);
BOOL			NETgetGlobalPlayerData	(DPID dpid,VOID *pData, DWORD *pSize);
BOOL			NETsetLocalPlayerData	(DPID dpid,VOID *pData, DWORD size);
BOOL			NETsetGlobalPlayerData	(DPID dpid,VOID *pData, DWORD size);

BOOL			NETspectate				(GUID guidSessionInstance);
BOOL			NETisSpectator			(DPID dpid);


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
	 HRESULT hr;

	if(!NetPlay.bComms)
	{
		memcpy(pData,pSingleUserData,userDataSize);
		return TRUE;
	}

	 hr =  IDirectPlayX_GetPlayerData(glpDP,dpid,pData,pSize,DPGET_LOCAL);
	 if(hr == DP_OK)
	 {
		 return TRUE;
	 }
	 else
	 {
		 DBPRINTF(("NETPLAY: failed to get Local Player Data\n"));
		 return FALSE;
	 }
 }

 // ////////////////////////////////////////////////////////////////////////
 BOOL NETgetGlobalPlayerData(DPID dpid,VOID *pData, DWORD *pSize)
 {
	 HRESULT hr;

  	 if(!NetPlay.bComms)
	 {	
		memcpy(pData,pSingleUserData,userDataSize);
 		return TRUE;
 	 }

	 hr =  IDirectPlayX_GetPlayerData(glpDP,dpid,pData,pSize,DPGET_REMOTE );

	 IDirectPlayX_SetPlayerData(glpDP,dpid,pData,*pSize,DPSET_LOCAL);	// update local copy 

	 if(hr == DP_OK)
	 {
		 return TRUE;
	 }
	 else
	 {
 		 DBPRINTF(("NETPLAY: failed to get Global Player Data\n"));
		 return FALSE;
	 }
 }
// ////////////////////////////////////////////////////////////////////////
 BOOL NETsetLocalPlayerData(DPID dpid,VOID *pData, DWORD size)
 {
	 HRESULT hr;

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
	 

	 hr =  IDirectPlayX_SetPlayerData(glpDP,dpid,pData,size,DPSET_LOCAL);
	 if(hr == DP_OK)
	 {
		 return TRUE;
	 }
	 else
	 {
		 DBPRINTF(("NETPLAY: failed to set Local Player Data\n"));
		 return FALSE;
	 }
 }

 // ////////////////////////////////////////////////////////////////////////
 BOOL NETsetGlobalPlayerData(DPID dpid,VOID *pData, DWORD size)
 {
	 HRESULT hr;
	 
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


	 hr =  IDirectPlayX_SetPlayerData(glpDP,dpid,pData,size,DPSET_GUARANTEED | DPSET_REMOTE);
	 if(hr == DP_OK)
	 {
		 return TRUE;
	 }
	 else
	 {
 		 DBPRINTF(("NETPLAY: failed to set Global Player Data\n"));
		 return FALSE;
	 }
 }

// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// functions to examine players in a given game.
BOOL FAR PASCAL Playercounter(DPID dpId,DWORD dwPlayerType,LPCDPNAME lpName,DWORD dwFlags,LPVOID lpContext)
{
	if(NetPlay.playercount == MaxNumberOfPlayers)
	{
		DBPRINTF(("NETPLAY: max players reached, ignoring others\n"));
		return FALSE;
	}

	// dont add spectators!
	if(dwFlags & DPENUMPLAYERS_SPECTATOR)
	{
		return TRUE;
	}

	//record name
	strcpy(NetPlay.players[NetPlay.playercount].name,(char*)(lpName->lpszShortName));

	// record dpid
	NetPlay.players[NetPlay.playercount].dpid= dpId;


	// record player type
	if(dwFlags & DPENUMPLAYERS_SERVERPLAYER)
	{
		NetPlay.players[NetPlay.playercount].bHost = TRUE;
	}
	else
	{
		NetPlay.players[NetPlay.playercount].bHost = FALSE;
	}
	 
	if(dwFlags & DPENUMPLAYERS_SPECTATOR)
	{
		NetPlay.players[NetPlay.playercount].bSpectator = TRUE;
	}
	else
	{
		NetPlay.players[NetPlay.playercount].bSpectator = FALSE;
	}

	NetPlay.playercount++;
	
	return (TRUE);
}

// ////////////////////////////////////////////////////////////////////////
// count players. call with null to enumerate the game already joined.
UDWORD NETplayerInfo(LPGUID guidinstance)
{
	
	NetPlay.playercount =0;													// reset player counter

	if(!NetPlay.bComms)
	{
		NetPlay.playercount				= 1;
		NetPlay.players[0].bHost		= TRUE;
		NetPlay.players[0].bSpectator	= FALSE;
		NetPlay.players[0].dpid			= 1;
		return 1;
	}

	ZeroMemory(NetPlay.players,(sizeof(PLAYER)*MaxNumberOfPlayers));	// reset player info

	if ( (NetPlay.bHost == TRUE) || (guidinstance == NULL) )
	{	
		IDirectPlayX_EnumPlayers(glpDP,	NULL,Playercounter,NULL,0); //DPENUMPLAYERS_REMOTE);   		
	}
	else
	{
		IDirectPlayX_EnumPlayers(glpDP,guidinstance,Playercounter,NULL,DPENUMPLAYERS_SESSION); 
	}

	return NetPlay.playercount;
}

// ////////////////////////////////////////////////////////////////////////
// rename the local player
//
// dont call this a lot, since it uses a guaranteed msg.
BOOL NETchangePlayerName(UDWORD dpid, char *newName)
{
	HRESULT hr;
	DPNAME	name;
	DPID	dp = (DPID)dpid;
	ZeroMemory(&name, sizeof(DPNAME));								// fill out name structure
	name.dwSize			= sizeof(DPNAME);
	name.lpszShortNameA = newName;
	name.lpszLongNameA	= NULL;

	if(!NetPlay.bComms)
	{
		strcpy(NetPlay.players[0].name,newName);
		return TRUE;
	}

	hr = IDirectPlayX_SetPlayerName(glpDP,dp,&name,DPSET_REMOTE );   		

	if (hr != DP_OK)
	{
		DBPRINTF(("NETPLAY: failed to change a players name\n"));
		return FALSE;
	}
	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Functions for spectators.

BOOL NETspectate(GUID guidSessionInstance)
{
	DPID				dpidPlayer;
	DPSESSIONDESC2		sessionDesc;
	HRESULT				hr;

	ZeroMemory(&sessionDesc, sizeof(DPSESSIONDESC2));					// join existing session
	sessionDesc.dwSize = sizeof(DPSESSIONDESC2);
    sessionDesc.guidInstance = guidSessionInstance;
	hr = IDirectPlayX_Open(glpDP,&sessionDesc, DPOPEN_JOIN);
	
	if FAILED(hr)
		goto FAILURE;

	hr = IDirectPlayX_CreatePlayer(glpDP,&dpidPlayer, NULL, NetPlay.hPlayerEvent, NULL, 0, DPPLAYER_SPECTATOR);
							
	if FAILED(hr)
		goto FAILURE;
	
	NetPlay.lpDirectPlay4A = glpDP;								// return connection info
	NetPlay.dpidPlayer = dpidPlayer;
	NetPlay.bHost = FALSE;
	NetPlay.bSpectator = TRUE;
	return (DP_OK);

FAILURE:
	IDirectPlayX_Close(glpDP);
	return (hr);
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

	return TRUE;	//not found, must be spectating

}