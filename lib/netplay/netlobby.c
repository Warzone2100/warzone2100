/*
 * netlobby.c
 *
 * provides dplay lobby features
 *
 * alex lee, nov97, pumpkin studios.
 */

#include <windowsx.h>
#include "frame.h"				// for dbprintf
#include "netplay.h"
//#include "netlobby.h"

// note registry entries should be set by the installation routine. 
// MPlayer requires registry entries too.

// ////////////////////////////////////////////////////////////////////////
// Prototypes.
BOOL NETconnectToLobby		(LPNETPLAY lpNetPlay);
BOOL NETsetRegistryEntries	(char *name,char *guid,char *file,char *cline,char *path,char *cdir);						   
BOOL NETcheckRegistryEntries(char *name,char *guid);

// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// allow lobbies to see this application, and hence remotely launch it.
// name  = name of app
// guid  = string containing app guid eg{45543-534-45346}
// file  = file name of app
// cline = command line params when launching.
// path  = path the app resides in
// cdir  = directory to set as current dir, once lauched.
BOOL NETsetRegistryEntries(char *name,char *guid,char *file,char *cline,char *path,char *cdir)						   
{
	char	keyname[256] = "SOFTWARE\\MICROSOFT\\DirectPlay\\Applications\\";
	HKEY	ghGameKey=NULL;							// duel registry key handle
	DWORD	ghGameDisp;								// key created or opened

	strcat(keyname,name);							// add the app name to the key

	RegCreateKeyEx(									// create/open key.
		HKEY_LOCAL_MACHINE,							// handle of an open key 
		keyname,									// address of subkey name 
		0,											// reserved 
	    NULL,										// address of class string 
		REG_OPTION_NON_VOLATILE,					// special options flag 
	    KEY_ALL_ACCESS,								// desired security access 
		NULL,										// address of key security structure 
		&ghGameKey,									// address of buffer for opened handle  
		&ghGameDisp									// address of disposition value buffer 
		);

	if(ghGameDisp == REG_CREATED_NEW_KEY)			// set the key values.
	{
		RegSetValueEx(ghGameKey, "Guid"				,0, REG_SZ, guid, strlen(guid)+1 );	//guid
		RegSetValueEx(ghGameKey, "File"				,0, REG_SZ, file, strlen(file)+1 );	//file
		RegSetValueEx(ghGameKey, "CommandLine"		,0, REG_SZ, cline,strlen(cline)+1);	//commandline
		RegSetValueEx(ghGameKey, "Path"				,0, REG_SZ, path, strlen(path)+1 );	//path
		RegSetValueEx(ghGameKey, "CurrentDirectory"	,0, REG_SZ, cdir, strlen(cdir)+1 );	//currentdir
	}

	RegCloseKey(ghGameKey);
	return TRUE;
}


BOOL NETcheckRegistryEntries(char *name,char *guid)
{
	char	basekey[256] = "SOFTWARE\\MICROSOFT\\DirectPlay\\Applications\\";
	HKEY	key;
	DWORD	type,resultsize;
	char	result[256];
	char	path[256];
	HANDLE	pFileHandle;

	strcat(basekey,name);		// form base path

	// check app exists in registry
	if(	RegOpenKeyEx( HKEY_LOCAL_MACHINE, basekey,0,KEY_ALL_ACCESS, &key ) != ERROR_SUCCESS)
	{
		DBERROR(("NETPLAY: DirectPlay Registry Key Does Not Exist. No Lobby Support"));
		RegCloseKey(key);
		return FALSE;
	}

	// check guid exists
	resultsize = 256;
	if(RegQueryValueEx(key,"Guid",NULL,&type,(char*)&result,&resultsize) !=  ERROR_SUCCESS)
	{
		DBERROR(("NETPLAY: DirectPlay Registry Key Does Not Have Guid Entry. No Lobby Support"));
		RegCloseKey(key);
		return FALSE;
	}

	// check guid matches.
	if( strcmp(guid,result) != 0)
	{
		DBERROR(("NETPLAY: DirectPlay guid does not match game guid. No Lobby Support")); 
		RegCloseKey(key);
		return FALSE;
	}

	// check File exists on disk
	resultsize = 256;
	if(RegQueryValueEx(key,"Path",	NULL,&type,(char*)&result,&resultsize) !=  ERROR_SUCCESS)
	{
		DBERROR(("NETPLAY: DirectPlay Registry Key Does Not Have An Path Entry. No Lobby Support"));
		RegCloseKey(key);
		return FALSE;
	}
	strcpy(path,result);
	resultsize = 256;
	if(RegQueryValueEx(key,"File",	NULL,&type,(char*)&result,&resultsize) !=  ERROR_SUCCESS)
	{
		DBERROR(("NETPLAY: DirectPlay Registry Key Does Not Have An File Entry. No Lobby Support"));
		RegCloseKey(key);
		return FALSE;
	}
	strcat(path,result);

	pFileHandle = fopen(path, "rb");
	if (pFileHandle == NULL)										// doesn't exist..
	{
		DBERROR(("NETPLAY: DirectPlay Registry FileName Does Not Match Your Game Installation. No Lobby Support"));
		RegCloseKey(key);
		return FALSE;
	}
	else
	{
		fclose(pFileHandle);
	}	

	// success..
	RegCloseKey(key);
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
// connect to a given lobby.
// returns FALSE if not lobbied!
BOOL NETconnectToLobby(LPNETPLAY lpNetPlay)
{
	LPDIRECTPLAY2A		lpDirectPlay2A = NULL;
	LPDIRECTPLAY4A		lpDirectPlay4A = NULL;
	LPDIRECTPLAYLOBBYA	lpDPlob = NULL;
	LPDPLCONNECTION		lpConnSettings = NULL;
	DPID				dpidPlayer;
	DWORD				dwSize;
	HRESULT				hr;

	hr = DirectPlayLobbyCreate(NULL, &lpDPlob, NULL, NULL, 0);						//create lobby interface
	if FAILED(hr)
	{
		DBPRINTF(("NETPLAY:no lobby device\n"));
		goto FAILURE;
	}															
	hr = IDirectPlayLobby_GetConnectionSettings(lpDPlob,0, NULL, &dwSize);			// get settings size

	if(hr == DPERR_NOTLOBBIED)								// not lobbied!!!
	{
		NetPlay.bLobbyLaunched = FALSE;
		if (lpDPlob)
		{
			IDirectPlayLobby_Release(lpDPlob);				// free stuff
		}
		return FALSE;
	}

	NetPlay.bLobbyLaunched = TRUE;					// we were lobby launched. 
	
	if (DPERR_BUFFERTOOSMALL != hr)
	{
		DBPRINTF(("NETPLAY:buf to small\n"));
		goto FAILURE;
	}

	lpConnSettings = (LPDPLCONNECTION) GlobalAllocPtr(GHND, dwSize);				// allocate memory for connection settings
	if (NULL == lpConnSettings)
	{
		hr = DPERR_OUTOFMEMORY;
		DBPRINTF(("NETPLAY:out of mem\n"));
		goto FAILURE;
	}

	hr = IDirectPlayLobby_GetConnectionSettings(lpDPlob,0, lpConnSettings, &dwSize);// get the connection settings
	if FAILED(hr)
	{
	
		DBPRINTF(("NETPLAY:didnt get settings\n"));
		goto FAILURE;
	}

#ifdef USE_DIRECTPLAY_PROTOCOL
   	lpConnSettings->lpSessionDesc->dwFlags	= DPSESSION_MIGRATEHOST | DPSESSION_KEEPALIVE | DPSESSION_DIRECTPLAYPROTOCOL ;
#else
  	lpConnSettings->lpSessionDesc->dwFlags	= DPSESSION_MIGRATEHOST | DPSESSION_KEEPALIVE;
#endif

    lpConnSettings->lpSessionDesc->dwMaxPlayers = MaxNumberOfPlayers;
  
    hr = IDirectPlayLobby_SetConnectionSettings(lpDPlob,0, 0, lpConnSettings);		// store the updated  settings
	if FAILED(hr)
	{
		DBPRINTF(("NETPLAY:didnt set settings\n"));
		goto FAILURE;
	}
	hr = IDirectPlayLobby_Connect(lpDPlob,0, &lpDirectPlay2A, NULL);				// connect. returns IDirectPlay2A interface
	if FAILED(hr)
	{
		DBPRINTF(("NETPLAY:didnt connect\n"));
		goto FAILURE;
	}
	hr = IDirectPlay2_QueryInterface(lpDirectPlay2A, &IID_IDirectPlay4A, (LPVOID *) &lpDirectPlay4A);// Obtain IDirectPlay4A interface
	if FAILED(hr)
	{	DBPRINTF(("NETPLAY:didnt query\n"));
		goto FAILURE;
	}
	hr = IDirectPlayX_CreatePlayer(lpDirectPlay4A,&dpidPlayer,						// create a player
							lpConnSettings->lpPlayerName, 
							lpNetPlay->hPlayerEvent, NULL, 0, 0);
	if FAILED(hr)
	{
		DBPRINTF(("NETPLAY:didnt create player\n"));
		goto FAILURE;
	}
	lpNetPlay->lpDirectPlay4A = lpDirectPlay4A;										// setup connection info
	glpDP  = lpDirectPlay4A;														// Store DPLAY interface
	glpDPL = lpDPlob;																// store the lobby.

	lpNetPlay->dpidPlayer = dpidPlayer;
	if (lpConnSettings->dwFlags & DPLCONNECTION_CREATESESSION)
	{
		lpNetPlay->bHost = TRUE;
	}
	else
	{
		lpNetPlay->bHost = FALSE;
	}

	lpDirectPlay4A = NULL;			// set to NULL here so won't release below
	lpDPlob		   = NULL;
	goto SUCCESS;

FAILURE:
	DBPRINTF(("NETPLAY:lobby connect failed\n"));
	if (lpDirectPlay2A)		IDirectPlay2_Release(lpDirectPlay2A);
	if (lpDirectPlay4A)		IDirectPlayX_Release(lpDirectPlay4A);
	if (lpDPlob)			IDirectPlayLobby_Release(lpDPlob);
	if (lpConnSettings)		GlobalFreePtr(lpConnSettings);
	return FALSE;
SUCCESS:
	if (lpDirectPlay2A)		IDirectPlay2_Release(lpDirectPlay2A);
	if (lpDirectPlay4A)		IDirectPlayX_Release(lpDirectPlay4A);
	if (lpDPlob)			IDirectPlayLobby_Release(lpDPlob);
	if (lpConnSettings)		GlobalFreePtr(lpConnSettings);

	return (TRUE);
}
