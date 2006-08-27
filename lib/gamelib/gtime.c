/*
 * GTime.c
 *
 * Provide a game clock that runs only when the game runs.
 *
 */

#include <time.h>

//#define DEBUG_GROUP1
#include "lib/framework/frame.h"
#include "gtime.h"


#define TIME_FIX

//#define RATE_LIMIT

#define GTIME_MINFRAME	(GAME_TICKS_PER_SEC/80)

/* The current time in the game world */
UDWORD gameTime;

/* The time for the last frame */
UDWORD frameTime;

/* The current time in the game world ( never stops )*/
UDWORD gameTime2;

/* The time for the last frame (never stops)*/
UDWORD frameTime2;


// the current clock modifier
static FRACT	modifier;


// the amount of game time before the last time clock speed was set
static UDWORD	timeOffset;
static UDWORD	timeOffset2;

// the tick count the last time the clock speed was set
static UDWORD	baseTime;
static UDWORD	baseTime2;

/* When the game paused so that gameTime can be adjusted when the game restarts */
static SDWORD	pauseStart;

/* Count how many times gameTimeStop has been called without a game time start */
static UDWORD	stopCount;

/* Initialise the game clock */
BOOL gameTimeInit(void)
{
	//gameTime = 0;
    /*start the timer off at 2 so that when the scripts strip the map of objects
    for multiPlayer they will be processed as if they died*/
    gameTime = 2;
	timeOffset = 0;
	baseTime = SDL_GetTicks();

	gameTime2 = 0;
	timeOffset2 = 0;
	baseTime2 = baseTime;


	modifier = FRACTCONST(1,1);


	stopCount = 0;

	return TRUE;
}

UDWORD	getTimeValueRange(UDWORD tickFrequency, UDWORD requiredRange)
{
UDWORD	div1,div2;

	div1 = gameTime2%tickFrequency;
	div2 = tickFrequency/requiredRange;
	return(div1/div2);
}

UDWORD	getStaticTimeValueRange(UDWORD tickFrequency, UDWORD requiredRange)
{
UDWORD	div1,div2;

	div1 = gameTime%tickFrequency;
	div2 = tickFrequency/requiredRange;
	return(div1/div2);
}


/* Call this each loop to update the game timer */
void gameTimeUpdate(void)
{
	UDWORD				currTime, fpMod;
#ifdef TIME_FIX
	uint64_t	newTime;
	uint64_t	extraTime;
#else
	unsigned long long	newTime;
#endif

	currTime = SDL_GetTicks();

	if (currTime < baseTime)
	{
#ifdef RATE_LIMIT
		// Limit the frame time
#endif

		// ooops the clock has wrapped round -
		// someone actually managed to keep windows running for 50 days !!!!
		// ........
	}

	//don't update the game time if gameTimeStop has been called
	if (stopCount == 0)
	{

		// Calculate the new game time
		newTime = currTime - baseTime;

		// convert the modifier to fixed point cos we loose accuracy
		fpMod = MAKEINT(FRACTmul(modifier, FRACTCONST(1000,1)));

		newTime = newTime * fpMod / 1000;

		newTime += timeOffset;

		// Calculate the time for this frame
		frameTime = (UDWORD)(newTime - gameTime);

		// Limit the frame time
		if (frameTime > GTIME_MAXFRAME)
		{
#ifdef TIME_FIX
			extraTime = frameTime - GTIME_MAXFRAME;
			extraTime = extraTime * 1000 / fpMod;//adjust the addition to base time
			baseTime += (UDWORD)extraTime;
#else
			baseTime += frameTime - GTIME_MAXFRAME;
#endif

			newTime = gameTime + GTIME_MAXFRAME;
			frameTime = GTIME_MAXFRAME;
		}

		// Store the game time
		gameTime = (UDWORD)newTime;

	}

	// now update gameTime2 which does not pause
	newTime = currTime - baseTime2;

	newTime += timeOffset;

	// Calculate the time for this frame
	frameTime2 = (UDWORD)newTime - gameTime2;

	// Limit the frame time
	if (frameTime2 > GTIME_MAXFRAME)
	{
		baseTime2 += frameTime2 - GTIME_MAXFRAME;
		newTime = gameTime2 + GTIME_MAXFRAME;
		frameTime2 = GTIME_MAXFRAME;
	}

	// Store the game time
	gameTime2 = (UDWORD)newTime;
}


// reset the game time modifiers
void gameTimeResetMod(void)
{
	timeOffset = gameTime;
	timeOffset2 = gameTime2;

	baseTime = SDL_GetTicks();
	baseTime2 = SDL_GetTicks();

	modifier = FRACTCONST(1,1);
}

// set the time modifier
void gameTimeSetMod(FRACT mod)
{
	gameTimeResetMod();
	modifier = mod;
}

// get the current time modifier
void gameTimeGetMod(FRACT *pMod)
{
	*pMod = modifier;
}


BOOL gameTimeIsStopped(void)
{
	return (stopCount != 0);
}


/* Call this to stop the game timer */
void gameTimeStop(void)
{
	if (stopCount == 0)
	{
		pauseStart = SDL_GetTicks();
		DBP1(("Clock paused at %d\n", pauseStart));
	}
	stopCount += 1;
}

/* Call this to restart the game timer after a call to gameTimeStop */
void gameTimeStart(void)
{
	if (stopCount == 1)
	{
		// shift the base time to now
		timeOffset = gameTime;
		baseTime = SDL_GetTicks();
	}

	if (stopCount > 0)
	{
		stopCount --;
	}
}

/*Call this to reset the game timer*/
void gameTimeReset(UDWORD time)
{
	// reset the game timers
	gameTime = time;
	timeOffset = time;

	gameTime2 = time;
	timeOffset2 = time;

	baseTime = SDL_GetTicks();//used from save game only so GetTickCount is as valid as anything
	baseTime2 = SDL_GetTicks();


	modifier = FRACTCONST(1,1);

}

void	getTimeComponents(UDWORD time, UDWORD *hours, UDWORD *minutes, UDWORD *seconds)
{
UDWORD	h,m,s;
UDWORD	tph,tpm;

	/* Ticks in a minute */
	tpm = GAME_TICKS_PER_SEC*60;

	/* Ticks in an hour */
	tph = tpm*60;

	h = time/tph;

	m = (time - (h*tph))/tpm;

	s = (time - ((h*tph) + (m*tpm)))/GAME_TICKS_PER_SEC;

	*hours = h;
	*minutes = m;
	*seconds = s;
}
