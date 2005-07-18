/* WinMain.h */

#ifndef _winmain_h
#define _winmain_h

#define GS_TITLE_SCREEN		1
#define GS_MISSION_SCREEN	2
#define GS_NORMAL			3
#define GS_VIDEO_MODE		4
#define	GS_SAVEGAMELOAD		5
#define FILE_PATH "C:\\Deliverance\\data"

//flag to indicate when initialisation is complete
extern  BOOL	gameInitialised;
extern  BOOL	frontendInitialised;
extern	BOOL	reInit;
extern	BOOL	bDisableLobby;

extern	UDWORD GetGameMode(void);
extern	void SetGameMode(UDWORD status);

#endif
