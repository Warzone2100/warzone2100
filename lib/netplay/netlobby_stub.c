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
BOOL NETsetRegistryEntries(char *name,char *guid,char *file,char *cline,char *path,char *cdir)						   
{
printf("NETsetRegistryEntries\n");
	return FALSE;
}


BOOL NETcheckRegistryEntries(char *name,char *guid)
{
printf("NETcheckRegistryEntries\n");
	return FALSE;
}

// ////////////////////////////////////////////////////////////////////////
// connect to a given lobby.
// returns FALSE if not lobbied!
BOOL NETconnectToLobby(LPNETPLAY lpNetPlay)
{
printf("NETconnectToLobby\n");
	return FALSE;
}
