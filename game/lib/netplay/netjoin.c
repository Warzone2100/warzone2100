/*
 * Net join.
 * join related stuff
 */ 

#include "frame.h"
#include "netplay.h"
#include "netsupp.h"


BOOL			NEThaltJoining		(VOID);
BOOL FAR PASCAL NETfindGameCallback	(LPCDPSESSIONDESC2 lpSessionDesc,LPDWORD lpdwTimeOut,DWORD dwFlags,LPVOID lpContext);
BOOL			NETfindGame			(BOOL async);
BOOL			NETjoinGame			(GUID guidSessionInstance, LPSTR playername);
BOOL			NEThostGame			(LPSTR SessionName, LPSTR PlayerName,DWORD one,DWORD two,DWORD three,DWORD four,UDWORD plyrs);
HRESULT			NETclose			(VOID);
DWORD			NETgetGameFlags		(UDWORD flag);
DWORD			NETgetGameFlagsUnjoined(UDWORD gameid, UDWORD flag);
BOOL			NETsetGameFlags		(UDWORD flag,DWORD value);		

static UDWORD		gamecount	= 0;


// Description Info. Used to remove ingame mallocs in netplay..
LPVOID		lpPermDescription=NULL;
UDWORD		descriptionSize=0;

VOID freePermMalloc(void)
{
	FREE(lpPermDescription);
	descriptionSize = 0;
	lpPermDescription = NULL;

	DBPRINTF(("NETPLAY: permalloc freed \n"));
}

VOID permMalloc(UDWORD size)
{
	if(descriptionSize < size)	// sort the buffer out.
	{
		DBPRINTF(("NETPLAY: permalloc changed from %d bytes to %d bytes \n",descriptionSize,size));
		if(descriptionSize !=0)	// get rid of old one.
		{
			freePermMalloc();
		}	

		memSetBlockHeap(NULL);
		lpPermDescription	= MALLOC(size);
		descriptionSize		= size;
	}
}

// ////////////////////////////////////////////////////////////////////////
// Stop the dplay interface from accepting more players.
BOOL NEThaltJoining(VOID)
{
	LPDPSESSIONDESC2	sessionDesc;							// template to find. 
	LPDPLCONNECTION		lobDesc;
	DWORD				size=1;
	HRESULT				hr;
	LPVOID				mempointer;

	if (NetPlay.bLobbyLaunched)														//Lobby version.
	{
		hr = IDirectPlayLobby_GetConnectionSettings(glpDPL, 0, NULL, &size);// get size
		mempointer = MALLOC(size);											// alloc space
		hr = IDirectPlayLobby_GetConnectionSettings(glpDPL, 0, mempointer, &size);
		lobDesc = (LPDPLCONNECTION)mempointer;

		lobDesc->lpSessionDesc->dwFlags = lobDesc->lpSessionDesc->dwFlags | DPSESSION_JOINDISABLED;	// set the flags.

		hr = IDirectPlayLobby_SetConnectionSettings(glpDPL, 0, 0, lobDesc);	//write it back
	}
	else																	// ordinary version
	{
		hr=IDirectPlayX_GetSessionDesc(glpDP, NULL, &size );				// get size
		mempointer = MALLOC(size);											// alloc
		hr=IDirectPlayX_GetSessionDesc(glpDP, mempointer, &size );			// get desc.
		sessionDesc = (LPDPSESSIONDESC2)mempointer;

		sessionDesc->dwFlags = sessionDesc->dwFlags | DPSESSION_JOINDISABLED;// set the flags.

		hr= IDirectPlayX_SetSessionDesc(glpDP,sessionDesc,0);				// write it back
	}

	if(PTRVALID(mempointer,size))	
	{
		FREE(mempointer);
	}
	return TRUE;
}



// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// routines to find games currently running on the chosen protocol

BOOL FAR PASCAL NETfindGameCallback(LPCDPSESSIONDESC2 lpSessionDesc,LPDWORD lpdwTimeOut,DWORD dwFlags,LPVOID lpContext)
{					

	if (dwFlags == DPESC_TIMEDOUT )
	{
		return (FALSE);
	}
	if(gamecount >= MaxGames)
	{
		DBPRINTF(("NETPLAY:Maximum number of available games exceeded. terminating search\n"));
		return (FALSE);
	}
	strcpy(NetPlay.games[gamecount].name, (char*)( lpSessionDesc->lpszSessionName));
	memcpy(&NetPlay.games[gamecount].desc,lpSessionDesc,sizeof(DPSESSIONDESC2));
	gamecount=gamecount+1;

	return (TRUE);
}

// ////////////////////////////////////////////////////////////////////////
// find games on open connection, option to do this asynchronously 
// since it can sometimes take a while.

BOOL NETfindGame(BOOL async)										/// may (not) want to use async here...
{
	HRESULT				hr;
	DPSESSIONDESC2		sessionDesc;								// template to find. 
	GUID				guidSessionInstance;
	DWORD				size;
//	LPVOID				mempointer=0;
	LPDPSESSIONDESC2	lpSessionDesc;								// template to find. 


	guidSessionInstance			= GUID_NULL;						// get guid of currently selected session
	ZeroMemory(&sessionDesc, sizeof(DPSESSIONDESC2));				// add sessions to session list
	sessionDesc.dwSize			= sizeof(DPSESSIONDESC2);
    sessionDesc.guidApplication = GAME_GUID;
	size						= sizeof(DPSESSIONDESC2);

	gamecount=0;
	ZeroMemory(NetPlay.games,(MaxGames*sizeof(DPSESSIONDESC2)));

	if (async == TRUE)
	{
		hr = IDirectPlayX_EnumSessions(glpDP,&sessionDesc, 0, NETfindGameCallback,
									  NULL, DPENUMSESSIONS_ALL | DPENUMSESSIONS_ASYNC | DPENUMSESSIONS_RETURNSTATUS );
	}
	else
	{
		hr = IDirectPlayX_EnumSessions(glpDP,&sessionDesc, 0, NETfindGameCallback,
									  NULL, DPENUMSESSIONS_ALL | DPENUMSESSIONS_RETURNSTATUS );
	}

	if( hr == DPERR_GENERIC)									
	{

		hr=IDirectPlayX_GetSessionDesc(glpDP, NULL, &size );
		if(hr == DPERR_BUFFERTOOSMALL)							// we are already connected. add this game.
		{
			permMalloc(size);
			hr=IDirectPlayX_GetSessionDesc(glpDP, lpPermDescription, &size );	// get desc.
		
			if(!FAILED(hr))
			{
				lpSessionDesc = (LPDPSESSIONDESC2)lpPermDescription;

				strcpy(NetPlay.games[0].name, (char*)(lpSessionDesc->lpszSessionName));	
				memcpy(&NetPlay.games[0].desc,lpSessionDesc,sizeof(DPSESSIONDESC2));
				gamecount=1;
			}
		}

	}
	else if ( hr == DPERR_CONNECTING)								// still connecting, so thats ok.
	{
		return (TRUE);
	}
	else if ( hr != DP_OK )											// failed.
	{
		return (FALSE);
	}

	return (TRUE);
}




// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Functions used to setup and join games.

BOOL NETjoinGame(GUID guidSessionInstance, LPSTR playername)
{
	HRESULT hr;
			
	hr = JoinSession(glpDP, &guidSessionInstance, playername, &NetPlay);		// join this session
	if FAILED(hr)
	{
		DBPRINTF(("NETPLAY:Failed to Join Game\n"));
		return (FALSE);
	}
	
	return (TRUE);
}

// ////////////////////////////////////////////////////////////////////////
// Host a game with a given name and player name. & 4 user game flags
BOOL NEThostGame(LPSTR SessionName, LPSTR PlayerName,	DWORD one,		// flags.
														DWORD two,
														DWORD three,
														DWORD four,
														UDWORD plyrs)	// # of players.
{
	HRESULT	hr;

	if(!NetPlay.bComms)
	{
		NetPlay.dpidPlayer		= 1;
		NetPlay.bHost			= TRUE;
		return TRUE;
	}

	hr = HostSession(glpDP,SessionName,PlayerName,&NetPlay,one,two,three,four,plyrs);
	if FAILED(hr)
	{
		DBERROR(("failed to host game"));
		return (FALSE);
	}
	return (TRUE);
}



// ////////////////////////////////////////////////////////////////////////
//close the open game..
HRESULT NETclose(VOID)
{
	IDirectPlayX_DestroyPlayer(glpDP,NetPlay.dpidPlayer);
	NetPlay.dpidPlayer = 0;
	if(glpDP)
	{
		IDirectPlayX_Close(glpDP);
	}
	return (DP_OK);
}


// ////////////////////////////////////////////////////////////////////////
// return one of the four user flags in the current sessiondescription.
DWORD NETgetGameFlags(UDWORD flag)
{		
	LPDPSESSIONDESC2	sessionDesc;							// template to find. 
	DWORD				size=1;
	HRESULT				hr;
//	LPVOID				mempointer;
	DWORD				result;
	LPDPLCONNECTION		lobDesc;

	if(NetPlay.bLobbyLaunched)
	{
		hr = IDirectPlayLobby_GetConnectionSettings(glpDPL, 0, NULL, &size);		// get the size 
	
		permMalloc(size);

		hr = IDirectPlayLobby_GetConnectionSettings(glpDPL, 0, lpPermDescription, &size);// get it.

		if (hr != DP_OK)
		{
			DBPRINTF(("NETPLAY:  couldn't get lobby game flags.\n"));
			return 0;
		}

		lobDesc = (LPDPLCONNECTION)lpPermDescription;

		switch(flag)
		{
		case 1:
			result = lobDesc->lpSessionDesc->dwUser1;
			break;
		case 2:
			result = lobDesc->lpSessionDesc->dwUser2;	
			break;
		case 3:
			result =lobDesc->lpSessionDesc->dwUser3;
			break;
		case 4:	
			result = lobDesc->lpSessionDesc->dwUser4;
			break;
		default:
			DBERROR(("Invalid flag for getgameflags in netplay lib"));
			break;
		}
	}
	else
	{
		hr=IDirectPlayX_GetSessionDesc(glpDP, NULL, &size );
		permMalloc(size);
		hr=IDirectPlayX_GetSessionDesc(glpDP, lpPermDescription, &size );
		
		if (hr != DP_OK)
		{
			DBPRINTF(("NETPLAY: couldn't get game flags\n"));
			return 0;
		}

		sessionDesc = (LPDPSESSIONDESC2)lpPermDescription;

		switch(flag)
		{
		case 1:
			result = sessionDesc->dwUser1;
			break;
		case 2:
			result = sessionDesc->dwUser2;	
			break;
		case 3:
			result =sessionDesc->dwUser3;
			break;
		case 4:	
			result = sessionDesc->dwUser4;
			break;
		default:
			DBERROR(("Invalid flag for getgameflags in netplay lib"));
			break;
		}
	}
	return result;
}

DWORD NETgetGameFlagsUnjoined(UDWORD gameid, UDWORD flag)
{
	switch(flag)
	{
	case 1:
		return NetPlay.games[gameid].desc.dwUser1;
		break;
	case 2:
		return NetPlay.games[gameid].desc.dwUser2;	
		break;
	case 3:
	    return NetPlay.games[gameid].desc.dwUser3;
		break;
	case 4:	
		return NetPlay.games[gameid].desc.dwUser4;
		break;
	default:
		DBERROR(("Invalid flag for getgameflagsunjoined in netplay lib"));
		return 0;
		break;
	}
}

// ////////////////////////////////////////////////////////////////////////
// Set a game flag
BOOL NETsetGameFlags(UDWORD flag,DWORD value)
{		
	LPDPSESSIONDESC2	sessionDesc;
	DWORD				size=1;
	HRESULT				hr;
//	LPVOID				mempointer=NULL;
	LPDPLCONNECTION		lobDesc;


	if(!NetPlay.bComms)
	{
		return TRUE;
	}

	if (NetPlay.bLobbyLaunched)															// LOBBY VERSION
	{
		hr = IDirectPlayLobby_GetConnectionSettings(glpDPL, 0, NULL, &size);		// get the size 

		permMalloc(size);
		
		hr = IDirectPlayLobby_GetConnectionSettings(glpDPL, 0, lpPermDescription, &size);	// get it.
		if(hr != DP_OK)
		{
			DBPRINTF(("NETPLAY: couldn't set lobby game flags \n"));
			return FALSE;
		}

		lobDesc = (LPDPLCONNECTION)lpPermDescription;		
	
		switch(flag)																//mod it
		{	
		case 1:
			lobDesc->lpSessionDesc->dwUser1 = value;
			break;
		case 2:
			lobDesc->lpSessionDesc->dwUser2 =value;
			break;
		case 3:
			lobDesc->lpSessionDesc->dwUser3 = value;
			break;
		case 4:	
			lobDesc->lpSessionDesc->dwUser4 = value;
			break;
		default:
			DBERROR(("Invalid flag for setgameflags in netplay lib"));
			break;
		}
		hr = IDirectPlayLobby_SetConnectionSettings(glpDPL, 0, 0, lobDesc);			//write it back
		if(hr != DP_OK)
		{
			DBPRINTF(("NETPLAY: couldn't set lobby game flags 2\n"));
		}
	}
	else																	// NON LOBBY VERSION
	{
		hr=IDirectPlayX_GetSessionDesc(glpDP, NULL, &size );				//get size
		permMalloc(size);
		hr=IDirectPlayX_GetSessionDesc(glpDP, lpPermDescription, &size );	//get it

		if (hr != DP_OK)
		{
			DBPRINTF(("NETPLAY: couldn't set game flags \n"));
			return FALSE;
		}

		sessionDesc = (LPDPSESSIONDESC2)lpPermDescription;

		switch(flag)														//mod it
		{
		case 1:
			sessionDesc->dwUser1 = value;
			break;
		case 2:
			sessionDesc->dwUser2 =value;
			break;
		case 3:
			sessionDesc->dwUser3 = value;
			break;
		case 4:	
			sessionDesc->dwUser4 = value;
			break;
		default:
			DBERROR(("Invalid flag for setgameflags in netplay lib"));
			break;
		}

		hr= IDirectPlayX_SetSessionDesc(glpDP,sessionDesc,0);				// write it back

		if(hr != DP_OK)
		{
			DBPRINTF(("NETPLAY: couldn't set lobby game flags \n"));
			return FALSE;
		}
	}
	return TRUE;
}
