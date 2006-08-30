/*
 * clParse.c
 *
 * Parse command line arguments
 *
 */
#ifndef _MSC_VER
#include <unistd.h>
#endif	// not for .net I should say..  --Qamly
#include "lib/framework/frame.h"
#include "lib/widget/widget.h"

#include "winmain.h"
#include "frontend.h"

#include "lib/ivis_common/pieclip.h"
#include "warzoneconfig.h"
#include "configuration.h"

#include "clparse.h"
#include "lib/ivis_common/piestate.h"
#include "loadsave.h"
#include "objects.h"
#include "advvis.h"
#include "multiplay.h"
#include "multiint.h"
#include "lib/netplay/netplay.h"
#include "wrappers.h"
#include "cheat.h"
#include "init.h"

extern BOOL NETsetupTCPIP(LPVOID *addr, char * machine);
extern BOOL scanGameSpyFlags(LPSTR gflag,LPSTR value);

extern char	SaveGamePath[];
extern char	datadir[MAX_PATH];
extern char * global_mods[MAX_MODS];
extern char * campaign_mods[MAX_MODS];
extern char * multiplay_mods[MAX_MODS];

void debug_callback_file( void**, const char * );
void debug_callback_file_init( void** );
void debug_callback_file_exit( void** );

// Functions for --shadow toggle
void setDrawShadows( BOOL val );
BOOL setWarzoneKeyNumeric( STRING *pName, DWORD val );

//! Whether to play the intro video
BOOL	clIntroVideo;

//! Let the end user into debug mode....
BOOL	bAllowDebugMode = FALSE;


//! Early parsing of the commandline
/**
 * First half of the command line parsing. Also see ParseCommandLine()
 * below. The parameters here are needed early in the boot process,
 * while the ones in ParseCommandLine can benefit from debugging being
 * set up first.
 * \param argc number of arguments given
 * \param argv string array of the arguments
 * \return Returns TRUE on success, FALSE on error */
BOOL ParseCommandLineEarly(int argc, char** argv)
{
	char			*tokenType;
	char			*token;
	int i;

	/* loop through command line */
	for (i = 1; i < argc; ++i) {
		tokenType = argv[i];

		if ( stricmp(tokenType, "--version") == 0 )
		{
#ifdef DEBUG
			fprintf( stdout, "Warzone 2100 - Version %s - Built %s - DEBUG\n", VERSION, __DATE__ );
#else
			fprintf( stdout, "Warzone 2100 - Version %s - Built %s\n", VERSION, __DATE__ );
#endif
			return FALSE;
		}
		else if ( stricmp(tokenType, "--help" ) == 0 )
		{
			// Show help
			fprintf( stdout,
				"Warzone 2100:\n"
				"   An OpenGL based 3D real time strategy game, scened in post-nuclear warfare\n"
				"Usage:\n"
				"   warzone2100 [OPTIONS]\n"
				"Options:\n"
				"   --cheat                    Run in cheat mode\n"
				"   --datadir DIR              Set default datadir to DIR\n"
				"   --debug FLAGS              Show debug for FLAGS\n"
				"   --debugfile FILE           Log debug output in FILE\n"
				"   --fullscreen               Play in fullscreen mode\n"
				"   --help                     Show this help and exit\n"
				"   --mod MOD                  Enable global mod MOD\n"
				"   --mod_ca MOD               Enable campaign only mod MOD\n"
				"   --mod_mp MOD               Enable multiplay only mod MOD\n"
				"   --savegame NAME            Load a saved game NAME\n"
				"   --window                   Play in windowed mode\n"
				"   --version                  Output version info and exit\n"
				"   --viewport WIDTHxHEIGHT    Set the dimensions of the viewport (screen or window)\n"
				"   --shadows YES/NO           Toggles the shadows\n" );
			return FALSE;
		}
		else if ( stricmp(tokenType, "--datadir") == 0 )
		{
			// find the quoted path name
			token = argv[++i];
			if (token == NULL)
			{
				debug( LOG_ERROR, "Unrecognised datadir\n" );
				return FALSE;
			}
			strncpy(datadir, token, sizeof(datadir));
		}
		else if ( stricmp(tokenType, "--debug") == 0 )
		{
			// find the part name
			token = argv[++i];
			if (token == NULL) {
				debug( LOG_ERROR, "Usage: --debug <flag>" );
				return FALSE;
			}
			if (!debug_enable_switch(token)) {
				debug(LOG_ERROR, "Debug flag \"%s\" not found!", token);
				return FALSE;
			}
		}
		else if ( stricmp(tokenType, "--debugfile") == 0 )
		{
			// find the file name
			token = argv[++i];
			if (token == NULL) {
				debug( LOG_ERROR, "Missing filename?\n" );
				abort();
				return FALSE;
			}
			debug_register_callback( debug_callback_file, debug_callback_file_init, debug_callback_file_exit, (void*)token );
		}
	}
	return TRUE;
}

/**************************************************************************

**************************************************************************/
//! second half of parsing the commandline
/**
 * Second half of command line parsing. See ParseCommandLineEarly() for
 * the first half. Note that render mode must come before resolution flag.
 * \param argc number of arguments given
 * \param argv string array of the arguments
 * \return Returns TRUE on success, FALSE on error */
BOOL ParseCommandLine(int argc, char** argv)
{
	char *tokenType=NULL, *token=NULL;
	unsigned int width=0, height=0;
	int i = 0, j = 0;

	/* loop through command line */
	for( i = 1; i < argc; ++i) {
		tokenType = argv[i];

		if ( stricmp(tokenType, "--cheat") == 0 )
		{
			fprintf(stdout, "  ** CHEAT MODE ACTIVATED! **\n");
			bAllowDebugMode = TRUE;
		}
		else if ( stricmp( tokenType, "--fullscreen" ) == 0 )
		{
			war_setFullscreen(TRUE);
		}
		else if ( stricmp( tokenType, "--game" ) == 0 )
		{
			// find the game name
			token = argv[++i];
			if (token == NULL)
			{
				debug( LOG_ERROR, "Unrecognised game name\n" );
				abort();
				return FALSE;
			}
			strncpy(pLevelName, token, 254);
			SetGameMode(GS_NORMAL);
		}
		else if ( stricmp(tokenType, "--mod") == 0 )
		{
			// find the file name
			token = argv[++i];
			if (token == NULL) {
				debug( LOG_ERROR, "Missing mod name?\n" );
				return FALSE;
			}

			for( j = 0; global_mods[j] != NULL && j < 100; j++ );
			if( global_mods[j] != NULL )
				debug( LOG_ERROR, "Too many mods registered! Aborting!" );
			global_mods[j] = token;
		}
		else if ( stricmp(tokenType, "--mod_ca") == 0 )
		{
			// find the file name
			token = argv[++i];
			if (token == NULL)
			{
				debug( LOG_ERROR, "Missing mod name?\n" );
				return FALSE;
			}

			for( j = 0; campaign_mods[j] != NULL && j < 100; j++ );
			if( campaign_mods[j] != NULL )
				debug( LOG_ERROR, "Too many mods registered! Aborting!" );
			campaign_mods[j] = token;
		}
		else if ( stricmp(tokenType, "--mod_mp") == 0 )
		{
			// find the file name
			token = argv[++i];
			if (token == NULL)
			{
				debug( LOG_ERROR, "Missing mod name?\n" );
				return FALSE;
			}

			for( j = 0; multiplay_mods[j] != NULL && j < 100; j++ );
			if( multiplay_mods[j] != NULL )
				debug( LOG_ERROR, "Too many mods registered! Aborting!" );
			multiplay_mods[j] = token;
		}
		else if ( stricmp( tokenType, "--savegame" ) == 0 )
		{
			// find the game name
			token = argv[++i];
			if (token == NULL)
			{
				debug( LOG_ERROR, "Unrecognised savegame name\n" );
				abort();
				return FALSE;
			}
			strcpy(saveGameName, SaveGamePath);
			strcat(saveGameName, "/");
			strncat(saveGameName, token, 240);
			SetGameMode(GS_SAVEGAMELOAD);
		}
		else if ( stricmp( tokenType,"--shadows") == 0 )
		{
			token = argv[++i];
			if ( stricmp( token, "yes" ) == 0 )
			{
				setDrawShadows( TRUE );
				setWarzoneKeyNumeric( "shadows", TRUE );
			}
			else if ( stricmp( token, "no" ) == 0 )
			{
				setDrawShadows( FALSE );
				setWarzoneKeyNumeric( "shadows", FALSE );
			}
			else
			{
				debug( LOG_ERROR, "Shadow toggle must be \"yes\" or \"no\"! Aborting!" );
				return FALSE;
			}
		}
		else if ( stricmp(tokenType, "--viewport") == 0 )
		{
			token = argv[++i];
			if ( !sscanf( token, "%ix%i", &width, &height ) == 2 )
			{
				debug( LOG_ERROR, "Invalid viewport\n" );
				abort();
				return FALSE;
			}
			pie_SetVideoBuffer( width, height );
		}
		else if ( stricmp( tokenType, "--window" ) == 0 )
		{
			war_setFullscreen(FALSE);
		}
		else if ( stricmp( tokenType, "--intro" ) == 0 )
		{
			SetGameMode(GS_VIDEO_MODE);
		}
		else if ( stricmp( tokenType, "--title" ) == 0 )
		{
			SetGameMode(GS_TITLE_SCREEN);
		}
		else if ( stricmp( tokenType,"--noTranslucent") == 0 )
		{
			war_SetTranslucent(FALSE);
		}
		else if ( stricmp( tokenType,"--noAdditive") == 0)
		{
			war_SetAdditive(FALSE);
		}
		else if ( stricmp( tokenType,"--noFog") == 0 )
		{
			pie_SetFogCap(FOG_CAP_NO);
		}
		else if ( stricmp( tokenType,"--greyFog") == 0 )
		{
			pie_SetFogCap(FOG_CAP_GREY);
		}
		else if ( stricmp(tokenType, "--CDA") == 0 )
		{
			war_SetPlayAudioCDs(TRUE);
		}
		else if ( stricmp(tokenType, "--noCDA") == 0 )
		{
			war_SetPlayAudioCDs(FALSE);
		}
		else if ( stricmp( tokenType,"--seqSmall") == 0 )
		{
			war_SetSeqMode(SEQ_SMALL);
		}
		else if ( stricmp( tokenType,"--seqSkip") == 0 )
		{
			war_SetSeqMode(SEQ_SKIP);
		}
		else if ( stricmp( tokenType,"--disableLobby") == 0 )
		{
			bDisableLobby = TRUE;
		}

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
//! gamespy flags
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

