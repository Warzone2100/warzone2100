/*! \file configfile.h
 * \brief load and save favourites to the registry.
 */

extern BOOL openWarzoneKey(void);
extern BOOL closeWarzoneKey(void);
extern BOOL getWarzoneKeyNumeric(const char *pName, SDWORD *val);
extern BOOL setWarzoneKeyNumeric(const char *pName, SDWORD val);
extern BOOL getWarzoneKeyString(const char *pName, char *pString);
extern BOOL setWarzoneKeyString(const char *pName, const char *pString);

extern char RegFilePath[MAX_PATH];
