/*
 * netlobby.h
 * alex lee , pumpkin studios, nov 97
 */

// ////////////////////////////////////////////////////////////////////////
// Prototypes.

extern BOOL NETcheckRegistryEntries	(char *name,char *guid);
extern BOOL NETsetRegistryEntries	(char *name,char *guid,char *file,char *cline,char *path,char *cdir); 
extern BOOL NETconnectToLobby		(LPNETPLAY lpNetPlay);
