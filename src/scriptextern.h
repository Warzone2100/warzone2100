/*
 * ScriptExtern.h
 *
 * All game variable access functions for the scripts
 *
 */
#ifndef _scriptextern_h
#define _scriptextern_h

// current game level
extern SDWORD	scrGameLevel;
// whether the tutorial is active
extern BOOL		bInTutorial;
// whether any additional special case victory/failure conditions have been met
extern BOOL		bExtraVictoryFlag;
extern BOOL		bExtraFailFlag;
extern BOOL		bTrackTransporter;


// ID numbers for external variables
enum _externids
{
	EXTID_MAPWIDTH,
	EXTID_MAPHEIGHT,
	EXTID_GAMEINIT,
	EXTID_SELECTEDPLAYER,
	EXTID_GAMELEVEL,
	EXTID_GAMETIME,
	EXTID_TUTORIAL,
	EXTID_MULTIGAMETYPE,
	EXTID_MULTIGAMEHUMANMAX,
	EXTID_MULTIGAMEBASETYPE,
	EXTID_CURSOR,
	EXTID_INTMODE, 
	EXTID_TARGETTYPE,				// IHATESCRIPTSANDEVERYTHINGTHEYSTANDFOR(ESPECIALLYONSUNDAYS)
	EXTID_EXTRAVICTORYFLAG,
	EXTID_EXTRAFAILFLAG,
	EXTID_TRACKTRANSPORTER,
	EXTID_MULTIGAMEALLIANCESTYPE,
};

// reset the script externals for a new level
extern void scrExternReset(void);

// General function to get some basic game values
extern BOOL scrGenExternGet(UDWORD index);

// General function to set some basic game values
extern BOOL scrGenExternSet(UDWORD index);

#endif

