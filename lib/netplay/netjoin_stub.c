/*
 * Net join.
 * join related stuff
 */ 

#include "frame.h"
#include "netplay.h"
#include "netsupp.h"


// ////////////////////////////////////////////////////////////////////////
// Stop the dplay interface from accepting more players.
BOOL NEThaltJoining(VOID)
{
	return TRUE;
}



// ////////////////////////////////////////////////////////////////////////
// find games on open connection, option to do this asynchronously 
// since it can sometimes take a while.

BOOL NETfindGame(BOOL async)										/// may (not) want to use async here...
{
	return FALSE;
}




// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// Functions used to setup and join games.

BOOL NETjoinGame(GUID guidSessionInstance, LPSTR playername)
{
	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////
// Host a game with a given name and player name. & 4 user game flags
BOOL NEThostGame(LPSTR SessionName, LPSTR PlayerName,	DWORD one,		// flags.
														DWORD two,
														DWORD three,
														DWORD four,
														UDWORD plyrs)	// # of players.
{
	if(!NetPlay.bComms)
	{
		NetPlay.dpidPlayer		= 1;
		NetPlay.bHost			= TRUE;
		return TRUE;
	}

	return FALSE;
}



// ////////////////////////////////////////////////////////////////////////
//close the open game..
HRESULT NETclose(VOID)
{
	return 0;
}


// ////////////////////////////////////////////////////////////////////////
// return one of the four user flags in the current sessiondescription.
DWORD NETgetGameFlags(UDWORD flag)
{		
	return 0;
}

DWORD NETgetGameFlagsUnjoined(UDWORD gameid, UDWORD flag)
{
	return 0;
}

// ////////////////////////////////////////////////////////////////////////
// Set a game flag
BOOL NETsetGameFlags(UDWORD flag,DWORD value)
{		
	if(!NetPlay.bComms)
	{
		return TRUE;
	}

	return FALSE;
}
