/*
 * NETsupp.h
 *
 * Alex Lee, pumpkin studios, nov97
 * The meaty network support functions
 * as well as some logging stuff.
 * Net Encryption also lives here.
 */

// ////////////////////////////////////////////////////////////////////////
// Includes
//#include "frame.h"
#include "netplay.h"
#include "netsupp.h"

#include <time.h>
#include <stdio.h>
static FILE			*pFileHandle;

// ////////////////////////////////////////////////////////////////////////
// Prototypes

HRESULT JoinSession		(LPDIRECTPLAY4A lpDirectPlay4A,LPGUID lpguidSessionInstance, LPSTR lpszPlayerName,LPNETPLAY lpNetPlay);					
HRESULT HostSession		(LPDIRECTPLAY4A lpDirectPlay4A,LPSTR lpszSessionName, LPSTR lpszPlayerName,LPNETPLAY lpNetPlay,
						 DWORD one, DWORD two, DWORD three, DWORD four, UDWORD plyrs);
HRESULT DestroyDirectPlayInterface(HWND hWnd, LPDIRECTPLAY4A lpDirectPlay4A);
HRESULT CreateDirectPlayInterface( LPDIRECTPLAY4A *lplpDirectPlay4A);

BOOL	NETstartLogging	(VOID);
BOOL	NETstopLogging	(VOID);
BOOL	NETlogEntry		(CHAR *str,UDWORD a,UDWORD b);

// ////////////////////////////////////////////////////////////////////////
// Open the DPLAY interface
HRESULT CreateDirectPlayInterface( LPDIRECTPLAY4A *lplpDirectPlay4A )
{
	HRESULT				hr;
	LPDIRECTPLAY4A		lpDirectPlay4A = NULL;
	
	hr = CoCreateInstance(	&CLSID_DirectPlay, NULL, CLSCTX_INPROC_SERVER, // Create an IDirectPlay4 interface
							&IID_IDirectPlay4A, (LPVOID*)&lpDirectPlay4A);

	*lplpDirectPlay4A = lpDirectPlay4A;									// return interface created
	return (hr);
}


// ////////////////////////////////////////////////////////////////////////
// Shutdown the DPLAY interface
HRESULT DestroyDirectPlayInterface(HWND hWnd, LPDIRECTPLAY4A lpDirectPlay4A)
{
	HRESULT		hr = DP_OK;
	if (lpDirectPlay4A)
	{
		hr = IDirectPlayX_Release(glpDP);
	}
	return (hr);
}


// ////////////////////////////////////////////////////////////////////////
// Create a new DPLAY game
HRESULT HostSession(LPDIRECTPLAY4A lpDirectPlay4A, LPSTR lpszSessionName, LPSTR lpszPlayerName,LPNETPLAY lpNetPlay,
					DWORD one, DWORD two, DWORD three, DWORD four, UDWORD mplayers)
{
	DPID				dpidPlayer;
	DPNAME				dpName;
	DPSESSIONDESC2		sessionDesc;
	HRESULT				hr;

	ZeroMemory(&sessionDesc, sizeof(DPSESSIONDESC2));					// host a new session
	sessionDesc.dwSize = sizeof(DPSESSIONDESC2);
   
    sessionDesc.guidApplication = GAME_GUID;
    sessionDesc.dwMaxPlayers = mplayers;
	sessionDesc.lpszSessionNameA = lpszSessionName;
#ifdef USE_DIRECTPLAY_PROTOCOL
	sessionDesc.dwFlags = DPSESSION_MIGRATEHOST | DPSESSION_KEEPALIVE | DPSESSION_DIRECTPLAYPROTOCOL ;
#else
	sessionDesc.dwFlags = DPSESSION_MIGRATEHOST | DPSESSION_KEEPALIVE;
#endif
	sessionDesc.dwUser1 = one;											// set the user flags
	sessionDesc.dwUser2 = two;
	sessionDesc.dwUser3 = three;
	sessionDesc.dwUser4 = four;

	hr = IDirectPlayX_Open(glpDP,&sessionDesc, DPOPEN_CREATE);
	if FAILED(hr)
	{
		goto OPEN_FAILURE;
	}
	ZeroMemory(&dpName, sizeof(DPNAME));								// fill out name structure
	dpName.dwSize = sizeof(DPNAME);
	dpName.lpszShortNameA = lpszPlayerName;
	dpName.lpszLongNameA = NULL;

	hr = IDirectPlayX_CreatePlayer(glpDP,&dpidPlayer, &dpName, lpNetPlay->hPlayerEvent, NULL, 0, 0);		
							
	if FAILED(hr)
	{
		goto CREATEPLAYER_FAILURE;
	}
	lpNetPlay->lpDirectPlay4A	= lpDirectPlay4A;							// return connection info
	lpNetPlay->dpidPlayer		= dpidPlayer;
	lpNetPlay->bHost			= TRUE;
	lpNetPlay->bSpectator		= FALSE;
	return (DP_OK);

CREATEPLAYER_FAILURE:
OPEN_FAILURE:
	IDirectPlayX_Close(glpDP);
	return (hr);
}

// ////////////////////////////////////////////////////////////////////////
// Enter a DPLAY game 
HRESULT JoinSession(LPDIRECTPLAY4A lpDirectPlay4A,
					LPGUID lpguidSessionInstance, LPSTR lpszPlayerName,
					LPNETPLAY	lpNetPlay)
{
	DPID				dpidPlayer;
	DPNAME				dpName;
	DPSESSIONDESC2		sessionDesc;
	HRESULT				hr;

	ZeroMemory(&sessionDesc, sizeof(DPSESSIONDESC2));					// join existing session
	sessionDesc.dwSize = sizeof(DPSESSIONDESC2);
    sessionDesc.guidInstance = *lpguidSessionInstance;

	hr = IDirectPlayX_Open(glpDP,&sessionDesc, DPOPEN_JOIN);
	
	if FAILED(hr)
		goto OPEN_FAILURE;

	ZeroMemory(&dpName, sizeof(DPNAME));								// fill out name structure
	dpName.dwSize = sizeof(DPNAME);
	dpName.lpszShortNameA = lpszPlayerName;
	dpName.lpszLongNameA = NULL;

	hr = IDirectPlayX_CreatePlayer(glpDP,&dpidPlayer, &dpName, lpNetPlay->hPlayerEvent, NULL, 0, 0);			
							
	if FAILED(hr)
		goto CREATEPLAYER_FAILURE;
	
	lpNetPlay->lpDirectPlay4A = lpDirectPlay4A;							// return connection info
	lpNetPlay->dpidPlayer = dpidPlayer;
	lpNetPlay->bHost = FALSE;
	lpNetPlay->bSpectator = FALSE;

	return (DP_OK);

CREATEPLAYER_FAILURE:
OPEN_FAILURE:
	IDirectPlayX_Close(glpDP);
	return (hr);
}


// ////////////////////////////////////////////////////////////////////////
// Logging for degug only
// ////////////////////////////////////////////////////////////////////////

BOOL NETstartLogging()
{
#ifdef LOG
	time_t aclock;
	struct tm *newtime;
	
	time( &aclock );                 /* Get time in seconds */
	 newtime = localtime( &aclock );  /* Convert time to struct */

	pFileHandle = fopen("c:\\netplay.log", "w");								// open the file
	if (!pFileHandle)
	{
		return FALSE;
	}
	fprintf(pFileHandle,"NETPLAY log: %s\n",asctime( newtime ) );
#endif
	return TRUE;
}

BOOL NETstopLogging()
{	
#ifdef LOG
	if (fclose(pFileHandle) != 0)
	{
		return FALSE;
	}
#endif
	return TRUE;
}


BOOL NETlogEntry(CHAR *str,UDWORD a,UDWORD b)
{
#ifdef LOG
	static UDWORD lastframe = 0;
	UDWORD frame= frameGetFrameNumber();
	time_t aclock;
	struct tm *newtime;

#ifndef MASSIVELOGS
	if(a ==9 || a==10)
	{
		return TRUE;
	}
#endif
		
	time( &aclock );                 /* Get time in seconds */
	newtime = localtime( &aclock );  /* Convert time to struct */

	// check to see if a new frame.
	if(frame != lastframe)
	{
		lastframe = frame;
		fprintf(pFileHandle,"-----------------------------------------------------------\n");
	}	

	switch(a)		// replace common msgs with txt descriptions
	{
	case 1:
		fprintf(pFileHandle,"%s \t: NET_DROIDINFO  \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 2:
		fprintf(pFileHandle,"%s \t: NET_DROIDDEST  \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 3:
		fprintf(pFileHandle,"%s \t: NET_DROIDMOVE  \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 4:
		fprintf(pFileHandle,"%s \t: NET_GROUPORDER  \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 8:
		fprintf(pFileHandle,"%s \t: NET_PING \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 9:
		fprintf(pFileHandle,"%s \t: NET_CHECK_DROID \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 10:
		fprintf(pFileHandle,"%s \t: NET_CHECK_STRUCT \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 11:
		fprintf(pFileHandle,"%s \t: NET_CHECK_POWER \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 13:
		fprintf(pFileHandle,"%s \t: NET_BUILD \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 15:
		fprintf(pFileHandle,"%s \t: NET_BUILDFINISHED \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 17:
		fprintf(pFileHandle,"%s \t: NET_TXTMSG \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 18:
		fprintf(pFileHandle,"%s \t: NET_LEAVING \t:%d\t\t%s",str,b,asctime( newtime ));
		fprintf(pFileHandle,"************************************************************\n");
		fprintf(pFileHandle,"************************************************************\n");
		break;
	case 19:
		fprintf(pFileHandle,"%s \t: NET_REQUESTDROID \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 23:
		fprintf(pFileHandle,"%s \t: NET_WHOLEDROID \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 22:
		fprintf(pFileHandle,"%s \t: NET_STRUCT (Whole) \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 25:
		fprintf(pFileHandle,"%s \t: NET_PLAYERRESPONDING \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 26:
		fprintf(pFileHandle,"%s \t: NET_OPTIONS \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 27:
		fprintf(pFileHandle,"%s \t: NET_WAYPOINT \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 28:
		fprintf(pFileHandle,"%s \t: NET_SECONDARY \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 29:
		fprintf(pFileHandle,"%s \t: NET_FIREUP \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 34:
		fprintf(pFileHandle,"%s \t: NET_ARTIFACTS \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 36:
		fprintf(pFileHandle,"%s \t: NET_SCORESUBMIT \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 37:
		fprintf(pFileHandle,"%s \t: NET_DESTROYXTRA \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 38:
		fprintf(pFileHandle,"%s \t: NET_VTOL \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 39:
		fprintf(pFileHandle,"%s \t: NET_VTOLREARM \t:%d\t\t%s",str,b,asctime( newtime ));
		break;

	default:
		fprintf(pFileHandle,"%s \t:%d \t\t\t:%d\t\t%s",str,a,b,asctime( newtime ));
		break;
	}
	fflush(pFileHandle);
#endif
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
