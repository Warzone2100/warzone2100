/*
 * config.h
 * load and save favourites to the registry.
 */

extern BOOL loadConfig				(BOOL bResourceAvailable);
extern BOOL loadRenderMode			(VOID);
extern BOOL saveConfig				(VOID);
extern BOOL	loadLastIp				(STRING *lastIP);
extern BOOL	saveLastIp				(STRING *currentIP);
