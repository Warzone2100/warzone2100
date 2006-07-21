/*
 * netlobby.h
 * alex lee , pumpkin studios, nov 97
 */

// ////////////////////////////////////////////////////////////////////////
// Prototypes.

extern WZ_DEPRECATED BOOL NETcheckRegistryEntries	(char *name,char *guid);
extern WZ_DEPRECATED BOOL NETsetRegistryEntries	(char *name,char *guid,char *file,char *cline,char *path,char *cdir); 
extern WZ_DEPRECATED BOOL NETconnectToLobby		(LPNETPLAY lpNetPlay);
