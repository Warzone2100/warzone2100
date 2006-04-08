/*
 * netlobby.c
 *
 * provides dplay lobby features
 *
 * alex lee, nov97, pumpkin studios.
 */

#include "frame.h"				// for dbprintf
#include "netplay.h"
//#include "netlobby.h"

// ////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////
// allow lobbies to see this application, and hence remotely launch it.
// name  = name of app
// guid  = string containing app guid eg{45543-534-45346}
// file  = file name of app
// cline = command line params when launching.
// path  = path the app resides in
// cdir  = directory to set as current dir, once lauched.
WZ_DEPRECATED BOOL NETsetRegistryEntries(char *name,char *guid,char *file,char *cline,char *path,char *cdir) // FIXME Remove if unused
{
	debug( LOG_NET, "NETsetRegistryEntries\n" );
	return FALSE;
}


WZ_DEPRECATED BOOL NETcheckRegistryEntries(char *name,char *guid) // FIXME Remove if unused
{
	debug( LOG_NET, "NETcheckRegistryEntries\n" );
	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////
// connect to a given lobby.
// returns FALSE if not lobbied!
WZ_DEPRECATED BOOL NETconnectToLobby(LPNETPLAY lpNetPlay) // FIXME Remove if unused
{
	debug( LOG_NET, "NETconnectToLobby\n" );
	return FALSE;
}
