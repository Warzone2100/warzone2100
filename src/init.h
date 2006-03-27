/*
 * Init.h
 *
 * Interface to the initialisation routines.
 *
 */
#ifndef _init_h
#define _init_h

// the number of patches
#define MAX_NUM_PATCHES 1

// the size of the file loading buffer
#define FILE_LOAD_BUFF_SIZE		(1024*1024)

extern BOOL InitialiseGlobals(void);
extern BOOL systemInitialise(void);
extern BOOL systemShutdown(void);
extern BOOL frontendInitialise(char *ResourceFile);
extern BOOL frontendShutdown(void);
extern BOOL stageOneInitialise(void);
extern BOOL stageOneShutDown(void);
extern BOOL stageTwoInitialise(void);
extern BOOL stageTwoShutDown(void);

extern BOOL stageThreeInitialise(void);

extern BOOL stageThreeShutDown(void);
extern BOOL gameReset(void);
extern BOOL newMapInitialise(void);
// Reset the game between campaigns
extern BOOL campaignReset(void);
// Reset the game when loading a save game
extern BOOL saveGameReset(void);

BOOL buildMapList();
BOOL loadLevels(int patchlevel);

// the block heap for the game data
extern BLOCK_HEAP	*psGameHeap;

// the block heap for the campaign map
extern BLOCK_HEAP	*psMapHeap;

// the block heap for the pre WRF data
extern BLOCK_HEAP	*psMissionHeap;

#endif

