/*
 * Player.h
 *
 * Update code for computer player AI
 *
 */

#ifndef _player_h
#define _player_h

/* Initialise the player AI system */
extern BOOL playerInitialise(void);

/* Reset the player AI after a load game */
extern void playerReset(void);

/* Shutdown the player AI system */
extern void playerShutDown(void);

/* Update the AI for a player */
extern void playerUpdate(UDWORD player);

/* deal with a new droid being built by a factory */
extern void playerNewDroid(DROID *psDroid);

/* Get a player to start manufacturing droids */
//extern void playerStartManufacture(UDWORD player);

/*sets the point new droids go to - x/y in world coords*/
//extern void setAssemblyPoint(UDWORD x, UDWORD y, UDWORD player);

/* sends players droids to attack a specified x/y */
extern void attackLocation(UDWORD x, UDWORD y, UDWORD player);

#endif

