/*
 * Combat.h
 *
 * Interface to the combat routines.
 *
 */
#ifndef _combat_h
#define _combat_h

#include "lib/framework/frame.h"
#include "objects.h"

/* The range out of which the random number for the to hit should be taken */
#define HIT_DICE	100

/* set a variable to the role of a die between 0 and HIT_DICE */
#define HIT_ROLL(d)  (d) = rand() % HIT_DICE

// maximum difference in direction for a fixed turret to fire
#define FIXED_TURRET_DIR	1

// %age at which a unit is considered to be heavily damaged
#define HEAVY_DAMAGE_LEVEL	25

/* Initialise the combat system */
extern BOOL combInitialise(void);

/* Shutdown the combat system */
extern BOOL combShutdown(void);

/* Fire a weapon at something added int weapon_slot*/
extern void combFire(WEAPON *psWeap, BASE_OBJECT *psAttacker, BASE_OBJECT *psTarget, int weapon_slot);

/*checks through the target players list of structures and droids to see
if any support a counter battery sensor*/
extern void counterBatteryFire(BASE_OBJECT *psAttacker, BASE_OBJECT *psTarget);

#endif



