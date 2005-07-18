#ifndef _scores_h
#define _scores_h
// --------------------------------------------------------------------
typedef enum data_index
{
WD_UNITS_BUILT,
WD_UNITS_KILLED,
WD_UNITS_LOST,
WD_STR_BUILT,
WD_STR_KILLED,
WD_STR_LOST,
WD_ARTEFACTS_FOUND,
WD_MISSION_STARTED,
WD_SHOTS_ON_TARGET,
WD_SHOTS_OFF_TARGET,
WD_BARBARIANS_MOWED_DOWN
} DATA_INDEX;

// --------------------------------------------------------------------
/* The mission results data */
typedef	struct mission_data
{
UDWORD	unitsBuilt;		// How many units were built
UDWORD	unitsKilled;	// How many enemy units you blew up
UDWORD	unitsLost;		// How many units were lost
UDWORD	strBuilt;		// How many structures we built
UDWORD	strKilled;		// How many enemy structures you blew up
UDWORD	strLost;		// How many structures were lost
UDWORD	artefactsFound;	// How many artefacts were found
UDWORD	missionStarted;	// When was the mission started
UDWORD	shotsOnTarget;	// How many hits
UDWORD	shotsOffTarget;	// How many misses
UDWORD	babasMowedDown; // How many barbarians did we mow down?
} MISSION_DATA;

// Could use widgets, but hey.....
typedef	struct	_stat_bar
{
UDWORD	topX,topY;		// Obvious
UDWORD	width,height;	// Height down screen and width _unfilled_
UDWORD	percent;		// What percentage full is it?
UDWORD	stringID;		// String resource name to stick next to it.
UDWORD	queTime;		// How many game ticks before it's active?
BOOL	bQueued;		// Already fired off?
BOOL	bActive;		// Is this one active?
UDWORD	number;			// %d string for the associated text string.
UDWORD	colour;			// What colour is this bar then?
}STAT_BAR;

enum
{
STAT_UNIT_LOST,       
STAT_UNIT_KILLED,         
STAT_STR_LOST,            
STAT_STR_BLOWN_UP,        
STAT_UNITS_BUILT,         
STAT_UNITS_NOW,           
STAT_STR_BUILT,           
STAT_STR_NOW,             
STAT_ROOKIE,      
STAT_GREEN,       
STAT_TRAINED, 
STAT_REGULAR, 
STAT_VETERAN, 
STAT_CRACK,       
STAT_ELITE,       
STAT_SPECIAL, 
STAT_ACE
};     



extern BOOL	scoreInitSystem			( void );
extern void	scoreUpdateVar			( DATA_INDEX var );
extern void	scoreDataToConsole		( void );
extern void	scoreDataToScreen		( void );	
extern void constructTime			( STRING *psText, UDWORD hours, UDWORD minutes, UDWORD seconds );
extern void	getAsciiTime			( STRING *psText, UDWORD time );
extern BOOL	readScoreData			( UBYTE *pFileData, UDWORD fileSize );
extern BOOL	writeScoreData			( STRING *pFileName );

#endif

