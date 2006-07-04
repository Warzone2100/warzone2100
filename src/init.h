/*
 * Init.h
 *
 * Interface to the initialisation routines.
 *
 */
#ifndef _init_h
#define _init_h

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

#define MOD_NONE 0
#define MOD_CAMPAIGN 1
#define MOD_MULTIPLAY 2

typedef struct _SearchPath
{
	char path[MAX_PATH];
	unsigned int priority;
	struct _SearchPath * higherPriority, * lowerPriority;
} SearchPath;

typedef enum { mod_none=0, mod_campaign=1, mod_multiplay=2 } searchPathMode;

void registerSearchPath( const char path[], unsigned int priority );
BOOL rebuildSearchPath( searchPathMode mode, BOOL force );

BOOL buildMapList();

// the block heap for the game data
extern BLOCK_HEAP	*psGameHeap;

// the block heap for the campaign map
extern BLOCK_HEAP	*psMapHeap;

// the block heap for the pre WRF data
extern BLOCK_HEAP	*psMissionHeap;

#endif

