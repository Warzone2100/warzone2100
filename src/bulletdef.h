/***************************************************************************/
/*
 * BulletDef.h
 *
 * Structure Definitions for the bullet object.
 *
 */
/***************************************************************************/

#ifndef _bulletdef_h
#define _bulletdef_h

/***************************************************************************/

typedef enum PROJ_STATE
{
	PROJ_INFLIGHT,
	PROJ_IMPACT,
	PROJ_POSTIMPACT
}
PROJ_STATE;

/***************************************************************************/

struct PROJ_OBJECT;

typedef void (* PROJECTILE_FUNC) ( struct PROJ_OBJECT *psObj );

typedef struct PROJECTILE
{
	struct PROJECTILE	*psNext;
}
PROJECTILE;

typedef struct PROJ_OBJECT
{
	/* Use only simple object elements */
	SIMPLE_ELEMENTS( struct PROJ_OBJECT );

	UBYTE			state;			/* current projectile state */
	UBYTE			airTarget;		/* whether the projectile was fired at an airborn target */
	
	UBYTE			player;			/* needed because damage and radDamage vary 
									from base stat per player because of upgrades*/
	UBYTE			bVisible;		// whether the selected player should see the projectile

	WEAPON_STATS	*psWStats;		/* firing weapon stats */
	
	BASE_OBJECT		*psSource;		/* what fired the projectile */
	BASE_OBJECT		*psDest;		/* projectile target */
	BASE_OBJECT		*psDamaged;		/* Watermelon:the target it already damaged,dont damage the same target twice */

	UDWORD			startX,startY;	/* Where projectile started */
	UDWORD			tarX,tarY;		/* The target coordinates */
	SDWORD			vXY, vZ;		/* axis velocities */
	UDWORD			srcHeight;		/* Height of origin */
	SDWORD			altChange;		/* Change in altitude */
	UDWORD			born;
	UDWORD			targetRadius;	// needed to backtrack the projectiles.
	
	PROJECTILE_FUNC	pInFlightFunc;
}
PROJ_OBJECT;

/***************************************************************************/

#endif
