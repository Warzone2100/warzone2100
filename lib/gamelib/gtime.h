/*! \file gtime.h
 * \brief Interface to the game clock.
 *
 */
#ifndef _gtime_h
#define _gtime_h

/* The number of ticks per second for the game clock */
#ifdef WIN32
#define GAME_TICKS_PER_SEC		1000
#else
#define GAME_TICKS_PER_SEC		1000


#endif

// The maximum time for one frame (stops the clock running away when debugging)
// changed to /6 by ajl. if this needs to go back to ticks/10 then tell me. 
#define GTIME_MAXFRAME	(GAME_TICKS_PER_SEC/6)		

/* The current time in the game world */
extern UDWORD gameTime;

/* The time for the last frame */
extern UDWORD frameTime;

/* The current time in the game world */
extern UDWORD gameTime2;	// Never stops.

/* The time for the last frame */
extern UDWORD frameTime2;	// Never stops.

/* Initialise the game clock */
extern BOOL gameTimeInit(void);

/* Call this each loop to update the game timer */
extern void gameTimeUpdate(void);

/* Returns TRUE if gameTime is stopped. */
extern BOOL gameTimeIsStopped(void);

/* Call this to stop the game timer */
extern void gameTimeStop(void);

/* Call this to restart the game timer after a call to gameTimeStop */
extern void gameTimeStart(void);

/*Call this to reset the game timer*/
extern void gameTimeReset(UDWORD time);


// reset the game time modifiers
void gameTimeResetMod(void);
// set the time modifier
void gameTimeSetMod(FRACT mod);
// get the current time modifier
void gameTimeGetMod(FRACT *pMod);


// get the current time modifier
void gameTimeGetModifier(UDWORD *pMod, UDWORD *pFactor);

/* Useful for periodical stuff */
/* Will return a number that climbs over tickFrequency game ticks and ends up in the required range. */
/*	
	For instance getTimeValueRange(4096,256) will return a number that cycles through
	the values 0..256 every 4.096 seconds...
	Ensure that the first is an integer multiple of the second 
*/
extern UDWORD	getTimeValueRange(UDWORD tickFrequency, UDWORD requiredRange);
extern	UDWORD	getStaticTimeValueRange(UDWORD tickFrequency, UDWORD requiredRange);

extern void	getTimeComponents(UDWORD time, UDWORD *hours, UDWORD *minutes, UDWORD *seconds);

#endif

