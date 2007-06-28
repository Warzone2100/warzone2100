/*
 * clParse.c
 *
 * Parse command line arguments
 *
 */
#ifndef _MSC_VER	
#include <unistd.h>
#endif	// not for .net I should say..  --Qamly
#include "frame.h"
#include "widget.h"

#include "winmain.h"
#include "frontend.h"

#include "pieclip.h"
#include "warzoneconfig.h"
#include "configuration.h"

#include "clparse.h"
#include "piestate.h"
#include "loadsave.h"
#include "objects.h"
#include "advvis.h"
#include "multiplay.h"
#include "multiint.h"
#include "netplay.h"
#include "wrappers.h"
#include "cheat.h"

BOOL scanGameSpyFlags(LPSTR gflag,LPSTR value);

extern char	SaveGamePath[];
extern char default_data_path[MAX_PATH]; // from main.c (yes, I know, more globals - Per)

// whether to play the intro video
BOOL	clIntroVideo;

// let the end user into debug mode....
BOOL	bAllowDebugMode = FALSE;

// we need to find these early in the boot process
BOOL ParseCommandLineEarly(int argc, char** argv)
{
	char			*tokenType;
	char			*token;
	int i;

	/* loop through command line */
	for (i = 1; i < argc; ++i) {
		tokenType = argv[i];

		if (stricmp(tokenType, "-debugfile") == 0) {
			// find the file name
			token = argv[++i];
			if (token == NULL) {
				DBERROR( ("Missing filename?\n") );
				return FALSE;
			}
			debug_to_file(token);
		} else if (stricmp(tokenType, "-debug") == 0) {
			// find the part name
			token = argv[++i];
			if (token == NULL) {
				debug(LOG_ERROR, "Usage: -debug <flag>");
				return FALSE;
			}
			if (!debug_enable_switch(token)) {
				debug(LOG_ERROR, "Debug flag \"%s\" not found!", token);
				return FALSE;
			}
		} else if (stricmp(tokenType, "-datapath") == 0) {
			// find the quoted path name
			token = argv[++i];
			if (token == NULL)
			{
				DBERROR( ("Unrecognised datapath\n") );
				return FALSE;
			}
			strncpy(default_data_path, token, sizeof(default_data_path));
		}
	}
	return TRUE;
}

// note that render mode must come before resolution flag.
BOOL ParseCommandLine(int argc, char** argv)
{
//	char			seps[] = " ,\t\n";
	char			*tokenType;
	char			*token;
//	char			seps2[] ="\"";
	char			cl[255];
	char			cl2[255];
	unsigned char		*pXor;
	unsigned int		w, h;
	int i;

	// for cheating
	sprintf(cl,"%s","VR^\\WZ^KVQXL\\^SSFH^XXZM");
	pXor = xorString(cl);
	sprintf(cl2,"%s%s","-",cl);

	/* loop through command line */
	for( i = 1; i < argc; ++i) {
		tokenType = argv[i];

		if ( stricmp( tokenType, "-window" ) == 0 )
		{
			war_setFullscreen(FALSE);
		}
		else if ( stricmp( tokenType, "-fullscreen" ) == 0 )
		{
			war_setFullscreen(TRUE);
		}
		else if ( stricmp( tokenType, "-intro" ) == 0 )
		{
			SetGameMode(GS_VIDEO_MODE);
		}
		else if ( stricmp( tokenType, "-title" ) == 0 )
		{
			SetGameMode(GS_TITLE_SCREEN);
		}
		else if ( stricmp( tokenType, "-game" ) == 0 )
		{
			// find the game name
			token = argv[++i];
			if (token == NULL)
			{
				DBERROR( ("Unrecognised -game name\n") );
				return FALSE;
			}
			strncpy(pLevelName, token, 254);
			SetGameMode(GS_NORMAL);
		}
		else if ( stricmp( tokenType, "-savegame" ) == 0 )
		{
			// find the game name
			token = argv[++i];
			if (token == NULL)
			{
				DBERROR( ("Unrecognised -savegame name\n") );
				return FALSE;
			}
			strcpy(saveGameName, SaveGamePath);
			strncat(saveGameName, token, 240);
			SetGameMode(GS_SAVEGAMELOAD);
		}
		else if( stricmp( tokenType,cl2) == 0)
		{
			bAllowDebugMode =TRUE;
		}
		else if( sscanf(tokenType,"-%ix%i", &w, &h) == 2)
		{
			pie_SetVideoBuffer(w, h);
		}
		else if( stricmp( tokenType,"-noTranslucent") == 0)
		{
			war_SetTranslucent(FALSE);
		}
		else if( stricmp( tokenType,"-noAdditive") == 0)
		{
			war_SetAdditive(FALSE);
		}
		else if( stricmp( tokenType,"-noFog") == 0)
		{
			pie_SetFogCap(FOG_CAP_NO);
		}
		else if( stricmp( tokenType,"-greyFog") == 0)
		{
			pie_SetFogCap(FOG_CAP_GREY);
		}
		else if (stricmp(tokenType, "-CDA") == 0) {
			war_SetPlayAudioCDs(TRUE);
		}
		else if (stricmp(tokenType, "-noCDA") == 0) {
			war_SetPlayAudioCDs(FALSE);
		}
		else if( stricmp( tokenType,"-2meg") == 0)
		{
			pie_SetTexCap(TEX_CAP_2M);
		}
		else if( stricmp( tokenType,"-seqSmall") == 0)
		{
			war_SetSeqMode(SEQ_SMALL);
		}
		else if( stricmp( tokenType,"-seqSkip") == 0)
		{
			war_SetSeqMode(SEQ_SKIP);
		}
		else if( stricmp( tokenType,"-disableLobby") == 0)
		{
			bDisableLobby = TRUE;
		}
/*		else if ( stricmp( tokenType,"-routeLimit") == 0)
		{
			// find the actual maximum routing limit
			token = strtok( NULL, seps );
			if (token == NULL)
			{
				DBERROR( ("Unrecognised -routeLimit value\n") );
				return FALSE;
			}

			if (!openWarzoneKey())
			{
				DBERROR(("Couldn't open registry for -routeLimit"));
				return FALSE;
			}
			fpathSetMaxRoute(atoi(token));
			setWarzoneKeyNumeric("maxRoute",(DWORD)(fpathGetMaxRoute()));
			closeWarzoneKey();
		}*/

	// gamespy flags
		else if (   stricmp(tokenType, "+host") == 0		// host a multiplayer.
			 || stricmp(tokenType, "+connect") == 0
			 || stricmp(tokenType, "+name") == 0
			 || stricmp(tokenType, "+ip") == 0
			 || stricmp(tokenType, "+maxplayers") == 0
			 || stricmp(tokenType, "+hostname") == 0)
		{
			token = argv[++i];
			scanGameSpyFlags(tokenType, token);
		}	
	// end of gamespy

		else
		{
			// ignore (gamespy requirement)
		//	DBERROR( ("Unrecognised command-line token %s\n", tokenType) );
		//	return FALSE;
		}
	}
	return TRUE;
}


// ////////////////////////////////////////////////////////
// gamespy flags
BOOL scanGameSpyFlags(LPSTR gflag,LPSTR value)
{
	static UBYTE count = 0;
//	UDWORD val;
	LPVOID finalconnection;	 

#if 0
	// check for gamespy flag...

	// if game spy not enabled....
	if(!openWarzoneKey())
		return FALSE;
	if(!getWarzoneKeyNumeric("allowGameSpy",&val))
	{
		return TRUE;
	}
	closeWarzoneKey();
#endif

	count++;
	if(count == 1)
	{
		lobbyInitialise();
		bDisableLobby	 = TRUE;				// dont reset everything on boot!
		gameSpy.bGameSpy = TRUE;
	}

	if(	 stricmp( gflag,"+host") == 0)			// host a multiplayer.
	{
		NetPlay.bHost = 1;
		game.bytesPerSec			= INETBYTESPERSEC;	
		game.packetsPerSec			= INETPACKETS;
		NETsetupTCPIP(&finalconnection,"");
		NETselectProtocol(finalconnection);
	}
	else if( stricmp( gflag,"+connect") == 0)	// join a multiplayer.
	{
		NetPlay.bHost = 0;
		game.bytesPerSec			= INETBYTESPERSEC;	
		game.packetsPerSec			= INETPACKETS;
		NETsetupTCPIP(&finalconnection,value);
		NETselectProtocol(finalconnection);
		// gflag is add to con to.
	}
	else if( stricmp( gflag,"+name") == 0)		// player name.
	{
		strcpy((char *)sPlayer,value);		
	}
	else if( stricmp( gflag,"+hostname") == 0)	// game name.
	{	
		strcpy(game.name,value);
	}

/*not used from here on..
+ip
+maxplayers
+game
+team
+skin
+playerid
+password
tokenType = strtok( NULL, seps );
*/
	return TRUE;
}

