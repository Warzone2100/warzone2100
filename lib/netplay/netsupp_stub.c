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
#include "frame.h"
#include "netplay.h"
#include "netsupp.h"

#include <time.h>
#include <stdio.h>

// ////////////////////////////////////////////////////////////////////////
// Open the DPLAY interface
HRESULT CreateDirectPlayInterface( LPDIRECTPLAY4A *lplpDirectPlay4A )
{
	return 0;
}


// ////////////////////////////////////////////////////////////////////////
// Shutdown the DPLAY interface
HRESULT DestroyDirectPlayInterface(HWND hWnd, LPDIRECTPLAY4A lpDirectPlay4A)
{
	return 0;
}


// ////////////////////////////////////////////////////////////////////////
// Create a new DPLAY game
HRESULT HostSession(LPDIRECTPLAY4A lpDirectPlay4A, LPSTR lpszSessionName, LPSTR lpszPlayerName,LPNETPLAY lpNetPlay,
					DWORD one, DWORD two, DWORD three, DWORD four, UDWORD mplayers)
{
	return 0;
}

// ////////////////////////////////////////////////////////////////////////
// Enter a DPLAY game 
HRESULT JoinSession(LPDIRECTPLAY4A lpDirectPlay4A,
					LPGUID lpguidSessionInstance, LPSTR lpszPlayerName,
					LPNETPLAY	lpNetPlay)
{
	return 0;
}


// ////////////////////////////////////////////////////////////////////////
// Logging for degug only
// ////////////////////////////////////////////////////////////////////////

BOOL NETstartLogging()
{
	return FALSE;
}

BOOL NETstopLogging()
{	
	return FALSE;
}


BOOL NETlogEntry(CHAR *str,UDWORD a,UDWORD b)
{
	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////
