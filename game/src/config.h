/*
 * config.h
 * load and save favourites to the registry.
 */

extern BOOL loadConfig				(BOOL bResourceAvailable);
extern BOOL loadRenderMode			(VOID);
extern BOOL saveConfig				(VOID);
extern BOOL getWarzoneKeyNumeric	(STRING *pName,DWORD *val);
extern BOOL openWarzoneKey			(VOID);
extern BOOL closeWarzoneKey			(VOID);
extern BOOL setWarzoneKeyNumeric	(STRING *pName,DWORD val);

extern BOOL	bAllowSubtitles;
