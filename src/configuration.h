/*
 * config.h
 * load and save favourites to the registry.
 */

extern BOOL loadConfig				(BOOL bResourceAvailable);
extern BOOL loadRenderMode			(void);
extern BOOL saveConfig				(void);
extern void closeConfig( void );
