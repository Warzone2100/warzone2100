/*
 * Levels.h
 *
 * Control the data loading for game levels
 *
 */
#ifndef _levels_h
#define _levels_h


// maximum number of WRF/WDG files

#define LEVEL_MAXFILES	9


// types of level datasets


enum _level_type
{
	LDS_COMPLETE,		// all data required for a stand alone level
	LDS_CAMPAIGN,		// the data set for a campaign (no map data)
	LDS_CAMSTART,		// mapdata for the start of a campaign
	LDS_CAMCHANGE,		// data for changing between levels
	LDS_EXPAND,			// extra data for expanding a campaign map
	LDS_BETWEEN,		// pause between missions
	LDS_MKEEP,			// off map mission (extra map data)
	LDS_MCLEAR,			// off map mission (extra map data)
	LDS_EXPAND_LIMBO,   // expand campaign map using droids held in apsLimboDroids
	LDS_MKEEP_LIMBO,    // off map saving any droids (selectedPlayer) at end into apsLimboDroids
	LDS_NONE,			//flags when not got a mission to go back to or when 
						//already on one - ****LEAVE AS LAST ONE****
};


typedef UDWORD LEVEL_TYPE;

// the WRF/WDG files needed for a particular level
// the WRF/WDG files needed for a particular level

typedef struct _level_dataset
{
	SWORD	type;					// type of map
	SWORD	players;				// number of players for the map
	SWORD	game;					// index of WRF/WDG that loads the scenario file
	char	*pName;					// title for the level
	int	dataDir;					// title for the level
	char	*apDataFiles[LEVEL_MAXFILES];		// the WRF/WDG files for the level
							// in load order
	struct _level_dataset *psBaseData;		// LEVEL_DATASET that must be loaded for this level to load
	struct _level_dataset *psChange;		// LEVEL_DATASET used when changing to this level from another

	struct _level_dataset *psNext;
} LEVEL_DATASET;


// the current level descriptions
extern LEVEL_DATASET	*psLevels;

// parse a level description data file
extern BOOL levParse(char *pBuffer, SDWORD size, int datadir);

// shutdown the level system
extern void levShutDown(void);

extern BOOL levInitialise(void);

// load up the base data set for a level (used by savegames)
extern BOOL levLoadBaseData(char *pName);

// load up the data for a level
extern BOOL levLoadData(char *pName, char *pSaveName, SDWORD saveType);

// find the level dataset
extern BOOL levFindDataSet(char *pName, LEVEL_DATASET **ppsDataSet);

// free the currently loaded dataset
extern BOOL levReleaseAll(void);

// free the data for the current mission
extern BOOL levReleaseMissionData(void);
 
//get the type of level currently being loaded of GTYPE type
extern SDWORD getLevelLoadType(void);

char *getLevelName( void );

#endif


