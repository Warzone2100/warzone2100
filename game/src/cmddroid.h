/*
 * CmdDroid.h
 *
 * Typedef's for command droids
 *
 */
#ifndef _cmddroid_h
#define _cmddroid_h

#include "cmddroiddef.h"

// The number of available command droids for each player
//extern SWORD	numCommandDroids[MAX_PLAYERS];

// The command droids for each player
//extern COMMAND_DROID	asCommandDroids[MAX_PLAYERS][MAX_CMDDROIDS];

// Initialise the command droids
extern BOOL cmdDroidInit(void);

// ShutDown the command droids
extern void cmdDroidShutDown(void);

// Make new command droids available
extern void cmdDroidAvailable(BRAIN_STATS *psBrainStats, SDWORD player);

// create a command droid
//extern BOOL buildCommandDroid(SDWORD player, SDWORD droid, DROID *psDroid);

// destroy a command droid
//extern void destroyCommandDroid(SDWORD player, SDWORD droid);

// update the command droids
extern void cmdDroidUpdate(void);

// add a droid to a command group
extern void cmdDroidAddDroid(DROID *psCommander, DROID *psDroid);

// return the current target designator for a player
extern DROID *cmdDroidGetDesignator(UDWORD player);

// set the current target designator for a player
extern void cmdDroidSetDesignator(DROID *psDroid);

// set the current target designator for a player
extern void cmdDroidClearDesignator(UDWORD player);

// get the index of the command droid
extern SDWORD cmdDroidGetIndex(DROID *psCommander);

// get the experience level of a command droid
extern SDWORD cmdDroidGetLevel(DROID *psCommander);

// get the maximum group size for a command droid
extern SDWORD cmdDroidMaxGroup(DROID *psCommander);

// update the kills of a command droid if psKiller is in a command group
extern void cmdDroidUpdateKills(DROID *psKiller);

// get the to hit modifier for a droid due to command droids
extern SDWORD cmdDroidHitMod(DROID *psDroid);

// get the evasion modifier for a droid due to command droids
extern SDWORD cmdDroidEvasionMod(DROID *psDroid);

// get the level of a droids commander, if any
extern SDWORD cmdGetCommanderLevel(DROID *psDroid);

// Select all droids assigned to the passed in command droids
extern void	cmdSelectSubDroids(DROID *psDroid);

// note that commander experience should be increased
extern void cmdDroidMultiExpBoost(BOOL bDoit);


#endif


