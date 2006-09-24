/***************************************************************************/
/*
 * loadsave.h
 */
/***************************************************************************/

#ifndef _loadsave_h
#define _loadsave_h

/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/

typedef enum _loadsave_mode
{
LOAD_FRONTEND,
LOAD_MISSIONEND,
SAVE_MISSIONEND,
LOAD_INGAME,
SAVE_INGAME,
LOAD_FORCE,
SAVE_FORCE
}LOADSAVE_MODE;

/***************************************************************************/
/*
 *	Global Variables
 */
/***************************************************************************/

extern BOOL		bLoadSaveUp;							// true when interface is up and should be run.
//the name of the save game to load from the front end
extern STRING saveGameName[256];
extern STRING	sRequestResult[255];
extern BOOL		bRequestLoad;

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/

extern void		drawBlueBox		(UDWORD x,UDWORD y, UDWORD w, UDWORD h);

extern BOOL		addLoadSave		(LOADSAVE_MODE mode, const char *defaultdir, const char *extension,CHAR *title);
extern BOOL		closeLoadSave	(void);
extern BOOL		runLoadSave		(BOOL bResetMissionWidgets);
extern BOOL		displayLoadSave	(void);

extern void		removeWildcards	(char *pStr);

// return whether the save screen was displayed in the mission results screen
BOOL saveInMissionRes(void);

// return whether the save screen was displayed in the middle of a mission
BOOL saveMidMission(void);


extern void deleteSaveGame(char* saveGameName);

#endif //_loadsave_h
