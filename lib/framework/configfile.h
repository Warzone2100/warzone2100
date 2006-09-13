/*
 * config.h
 * load and save favourites to the registry.
 */

extern BOOL getWarzoneKeyNumeric	(const STRING *pName,DWORD *val);
extern BOOL openWarzoneKey			(void);
extern BOOL closeWarzoneKey			(void);
extern BOOL setWarzoneKeyNumeric	(const STRING *pName,DWORD val);
extern BOOL getWarzoneKeyString(const STRING *pName, STRING *pString);
extern BOOL setWarzoneKeyString(const STRING *pName, const STRING *pString);
