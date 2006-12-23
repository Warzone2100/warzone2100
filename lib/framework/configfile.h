/*
 * config.h
 * load and save favourites to the registry.
 */

extern BOOL getWarzoneKeyNumeric	(STRING *pName,SDWORD *val);
extern BOOL openWarzoneKey			(VOID);
extern BOOL closeWarzoneKey			(VOID);
extern BOOL setWarzoneKeyNumeric	(STRING *pName,SDWORD val);
extern BOOL getWarzoneKeyString(STRING *pName, STRING *pString);
extern BOOL setWarzoneKeyString(STRING *pName, STRING *pString);
