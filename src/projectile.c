/***************************************************************************/
/*
 * Projectile functions
 *
 * Gareth Jones 11/7/97
 *
 */
/***************************************************************************/
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/trig.h"

#include "lib/gamelib/gtime.h"
#include "objects.h"
#include "move.h"
#include "action.h"
#include "combat.h"
#include "effects.h"
#include "map.h"
#include "audio_id.h"
#include "lib/sound/audio.h"
#include "lib/gamelib/hashtabl.h"
#include "anim_id.h"
#include "projectile.h"
#include "visibility.h"
#include "lib/script/script.h"
#include "scripttabs.h"
#include "scriptcb.h"
#include "group.h"
#include "cmddroid.h"
#include "feature.h"
#include "lib/ivis_common/piestate.h"
#include "loop.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/piematrix.h"

#include "scores.h"

#include "display3d.h"
#include "display.h"
#include "multiplay.h"
#include "multistat.h"

// Watermelon:I need this one for map grid iteration
#include "mapgrid.h"

/***************************************************************************/
/* max number of slots in hash table - prime numbers are best because hash
 * function used here is modulous of object pointer with table size -
 * prime number nearest 100 is 97.
 * Table of nearest primes in Binstock+Rex, "Practical Algorithms" p 91.
 */

#define	PROJ_HASH_TABLE_SIZE	97
#define PROJ_INIT				200
#define PROJ_EXT				10
#define	PROJ_MAX_PITCH			30
#define	ACC_GRAVITY				1000
#define	DIRECT_PROJ_SPEED		500
#define	CHECK_PROJ_ABOVE_GROUND	1
#define NOMINAL_DAMAGE	5

// Watermelon:they are from droid.c
/* The range for neighbouring objects */
#define PROJ_NAYBOR_RANGE		(TILE_UNITS*2)

// macro to see if an object is in NAYBOR_RANGE
// used by projGetNayb
#define IN_PROJ_NAYBOR_RANGE(psTempObj) \
	xdiff = dx - (SDWORD)psTempObj->x; \
	if (xdiff < 0) \
	{ \
		xdiff = -xdiff; \
	} \
	if (xdiff > PROJ_NAYBOR_RANGE) \
	{ \
		continue; \
	} \
\
	ydiff = dy - (SDWORD)psTempObj->y; \
	if (ydiff < 0) \
	{ \
		ydiff = -ydiff; \
	} \
	if (ydiff > PROJ_NAYBOR_RANGE) \
	{ \
		continue; \
	} \
\
	distSqr = xdiff*xdiff + ydiff*ydiff; \
	if (distSqr > PROJ_NAYBOR_RANGE*PROJ_NAYBOR_RANGE) \
	{ \
		continue; \
	} \

// Watermelon:neighbour global info ripped from droid.c
PROJ_NAYBOR_INFO	asProjNaybors[MAX_NAYBORS];
UDWORD				numProjNaybors=0;

static BASE_OBJECT	*CurrentProjNaybors = NULL;
static UDWORD	projnayborTime = 0;

/***************************************************************************/

static	HASHTABLE	*g_pProjObjTable;

// the last unit that did damage - used by script functions
BASE_OBJECT		*g_pProjLastAttacker;

extern BOOL	godMode;

/***************************************************************************/

static UDWORD	establishTargetRadius( BASE_OBJECT *psTarget );
static UDWORD	establishTargetHeight( BASE_OBJECT *psTarget );
static void	proj_InFlightDirectFunc( PROJ_OBJECT *psObj );
static void	proj_InFlightIndirectFunc( PROJ_OBJECT *psObj );
static void	proj_ImpactFunc( PROJ_OBJECT *psObj );
static void	proj_PostImpactFunc( PROJ_OBJECT *psObj );

//static void	proj_MachineGunInFlightFunc( PROJ_OBJECT *psObj );

static void	proj_checkBurnDamage( BASE_OBJECT *apsList, PROJ_OBJECT *psProj,
									FIRE_BOX *pFireBox );

static BOOL objectDamage(BASE_OBJECT *psObj, UDWORD damage, UDWORD weaponClass,UDWORD weaponSubClass,int angle);

/***************************************************************************/
BOOL gfxVisible(PROJ_OBJECT *psObj)
{
	BOOL bVisible = FALSE;

	// already know it is visible
	if (psObj->bVisible)
	{
		return TRUE;
	}

	// you fired it
	if(psObj->player == selectedPlayer)
	{
		return TRUE;
	}

	// always see in this mode
	if(godMode)
	{
		return TRUE;
	}

	// you can see the source
	if( (psObj->psSource!=NULL) && (psObj->psSource->visible[selectedPlayer]) )
	{
		bVisible = TRUE;
	}

	// you can see the destination
	if( (psObj->psDest!=NULL) && (psObj->psDest->visible[selectedPlayer]) )
	{
		bVisible = TRUE;
	}

	// someone elses structure firing at something you can't see
	if ( psObj->psSource != NULL &&
		psObj->psSource->type == OBJ_STRUCTURE &&
		psObj->psSource->player!=selectedPlayer &&
		( psObj->psDest == NULL || !psObj->psDest->visible[selectedPlayer] ) )
	{
		bVisible = FALSE;
	}

	// something you cannot see firing at a structure that isn't yours
	if ( psObj->psDest != NULL &&
		psObj->psDest->type == OBJ_STRUCTURE &&
		psObj->psDest->player != selectedPlayer &&
		( psObj->psSource == NULL || !psObj->psSource->visible[selectedPlayer] ) )
	{
		bVisible = FALSE;
	}

	return(bVisible);
}
/***************************************************************************/

BOOL
proj_InitSystem( void )
{
	/* allocate object hashtable */
	hashTable_Create( &g_pProjObjTable, PROJ_HASH_TABLE_SIZE,
						PROJ_INIT, PROJ_EXT, sizeof(PROJ_OBJECT) );
	return TRUE;
}

/***************************************************************************/

void
proj_FreeAllProjectiles( void )
{
	if (g_pProjObjTable)
	{
		hashTable_Clear( g_pProjObjTable );
	}
}

/***************************************************************************/

BOOL
proj_Shutdown( void )
{
	/* destroy hash table */
	hashTable_Destroy( g_pProjObjTable );
	g_pProjObjTable = NULL;

	return TRUE;
}

/***************************************************************************/

PROJ_OBJECT *
proj_GetFirst( void )
{
	return hashTable_GetFirst( g_pProjObjTable );
}

/***************************************************************************/

PROJ_OBJECT *
proj_GetNext( void )
{
	return hashTable_GetNext( g_pProjObjTable );
}

/***************************************************************************/

// update the kills after a target is destroyed
static void proj_UpdateKills(PROJ_OBJECT *psObj)
{
	DROID	        *psDroid ;//, *psSensor;
    BASE_OBJECT     *psSensor;//, *psTarget;

	if ((psObj->psSource == NULL) ||
		((psObj->psDest != NULL) && (psObj->psDest->type == OBJ_FEATURE)))
	{
		return;
	}


	if(bMultiPlayer)
	{
		sendDestroyExtra(psObj->psDest,psObj->psSource);
		updateMultiStatsKills(psObj->psDest,psObj->psSource->player);
	}


	if(psObj->psSource->type == OBJ_DROID)			/* update droid kills */
	{
		psDroid = (DROID*)psObj->psSource;
		psDroid->numKills++;
		cmdDroidUpdateKills(psDroid);
        //can't assume the sensor object is a droid - it might be a structure
		/*if (orderStateObj(psDroid, DORDER_FIRESUPPORT, (BASE_OBJECT **)&psSensor))
		{
			psSensor->numKills ++;
		}*/
		if (orderStateObj(psDroid, DORDER_FIRESUPPORT, &psSensor))
		{
            if (psSensor->type == OBJ_DROID)
            {
			    ((DROID *)psSensor)->numKills++;
            }
		}
	}
	else if (psObj->psSource->type == OBJ_STRUCTURE)
	{
		// see if there was a command droid designating this target
		psDroid = cmdDroidGetDesignator(psObj->psSource->player);
		if (psDroid != NULL)
		{
			if ((psDroid->action == DACTION_ATTACK) &&
				(psDroid->psActionTarget[0] == psObj->psDest))
			{
				psDroid->numKills ++;
			}
		}
	}
}

/***************************************************************************/

BOOL
proj_SendProjectile( WEAPON *psWeap, BASE_OBJECT *psAttacker, SDWORD player,
					 UDWORD tarX, UDWORD tarY, UDWORD tarZ, BASE_OBJECT *psTarget, BOOL bVisible, BOOL bPenetrate, int weapon_slot )
{
	PROJ_OBJECT		*psObj;
	SDWORD			tarHeight, srcHeight, iMinSq;
	SDWORD			altChange, dx, dy, dz, iVelSq, iVel;
	FRACT_D			fR, fA, fS, fT, fC;
	iVector			muzzle;
	SDWORD			iRadSq, iPitchLow, iPitchHigh, iTemp;
	UDWORD			heightVariance;
	WEAPON_STATS	*psWeapStats = &asWeaponStats[psWeap->nStat];

	ASSERT( PTRVALID(psWeapStats,sizeof(WEAPON_STATS)),
			"proj_SendProjectile: invalid weapon stats" );

	/* get unused projectile object from hashtable*/
	psObj = hashTable_GetElement( g_pProjObjTable );

	/* get muzzle offset */
	if (psAttacker == NULL)
	{
		// if there isn't an attacker just start at the target position
		// NB this is for the script function to fire the las sats
		muzzle.x = (SDWORD)tarX;
		muzzle.y = (SDWORD)tarY;
		muzzle.z = (SDWORD)tarZ;
	}
	else if (psAttacker->type == OBJ_DROID && weapon_slot >= 0)
	{
		calcDroidMuzzleLocation( (DROID *) psAttacker, &muzzle, weapon_slot);
		/*update attack runs for VTOL droid's each time a shot is fired*/
		updateVtolAttackRun((DROID *)psAttacker, weapon_slot);
	}
	else if (psAttacker->type == OBJ_STRUCTURE && weapon_slot >= 0)
	{
		calcStructureMuzzleLocation( (STRUCTURE *) psAttacker, &muzzle, weapon_slot);
	}
	else // incase anything wants a projectile
	{
		muzzle.x = psAttacker->x;
		muzzle.y = psAttacker->y;
		muzzle.z = psAttacker->z;
	}

	/* Initialise the structure */
	psObj->type		    = OBJ_BULLET;
	psObj->state		= PROJ_INFLIGHT;
	psObj->psWStats		= asWeaponStats + psWeap->nStat;
	psObj->x			= (UWORD)muzzle.x;
	psObj->y			= (UWORD)muzzle.y;
	psObj->z			= (UWORD)muzzle.z;
	psObj->startX		= muzzle.x;
	psObj->startY		= muzzle.y;
	psObj->tarX			= tarX;
	psObj->tarY			= tarY;
	psObj->targetRadius = establishTargetRadius(psTarget);	// New - needed to backtrack FX
	psObj->psDest		= psTarget;
	psObj->born			= gameTime;
	psObj->player		= (UBYTE)player;
	psObj->bVisible		= FALSE;
	psObj->airTarget	= (UBYTE)( ( psTarget != NULL &&
				psTarget->type == OBJ_DROID &&
				vtolDroid((DROID*)psTarget) ) ||
			( psTarget == NULL &&
				(SDWORD)tarZ > map_Height(tarX,tarY) ) );

	//Watermelon:use the source of the source of psObj :) (psAttacker is a projectile)
	if (bPenetrate)
	{
		psObj->psSource = ((PROJ_OBJECT *)psAttacker)->psSource;
		psObj->psDamaged = ((PROJ_OBJECT *)psAttacker)->psDest;
		((PROJ_OBJECT *)psAttacker)->state = PROJ_IMPACT;
	}
	else
	{
		psObj->psSource = psAttacker;
	}

	if (psTarget)
	{
		scoreUpdateVar(WD_SHOTS_ON_TARGET);
		heightVariance = 0;
		switch(psTarget->type)
		{
			case OBJ_DROID:
			case OBJ_FEATURE:
				if( ((DROID*)psTarget)->droidType == DROID_PERSON )
				{
					heightVariance = rand()%4;
				}
				else
				{
					heightVariance = rand()%8;
				}
				break;
			case OBJ_STRUCTURE:
				heightVariance = rand()%8;
				break;
			default:
				break;
		}
		tarHeight = psTarget->z + heightVariance;
	}
	else
	{
		tarHeight = (SDWORD)tarZ;
		scoreUpdateVar(WD_SHOTS_OFF_TARGET);
	}

	srcHeight			= muzzle.z;
	altChange			= tarHeight - srcHeight;

	psObj->srcHeight	= srcHeight;
	psObj->altChange	= altChange;

	dx = ((SDWORD)psObj->tarX) - muzzle.x;
	dy = ((SDWORD)psObj->tarY) - muzzle.y;
	dz = tarHeight - muzzle.z;

	/* roll never set */
	psObj->roll = 0;

	fR = (FRACT_D) atan2(dx, dy);
	if ( fR < 0.0 )
	{
		fR += (FRACT_D) (2*PI);
	}
	psObj->direction = (UWORD)( RAD_TO_DEG(fR) );


	/* get target distance */
	iRadSq = dx*dx + dy*dy + dz*dz;
	fR = trigIntSqrt( iRadSq );
	iMinSq = (SDWORD)(psWeapStats->minRange * psWeapStats->minRange);

	if ( proj_Direct(psObj->psWStats) ||
		 ( !proj_Direct(psWeapStats) && (iRadSq <= iMinSq) ) )
	{
		fR = (FRACT_D) atan2(dz, fR);
		if ( fR < 0.0 )
		{
			fR += (FRACT_D) (2*PI);
		}
		psObj->pitch = (SWORD)( RAD_TO_DEG(fR) );
		psObj->pInFlightFunc = proj_InFlightDirectFunc;
	}
	else
	{
		/* indirect */
		iVelSq = psObj->psWStats->flightSpeed * psObj->psWStats->flightSpeed;

 		fA = ACC_GRAVITY*MAKEFRACT_D(iRadSq) / (2*iVelSq);
		fC = 4 * fA * (MAKEFRACT_D(dz) + fA);
		fS = MAKEFRACT_D(iRadSq) - fC;

		/* target out of range - increase velocity to hit target */
		if ( fS < MAKEFRACT_D(0) )
		{
			/* set optimal pitch */
			psObj->pitch = PROJ_MAX_PITCH;

			fS = (FRACT_D)trigSin(PROJ_MAX_PITCH);
			fC = (FRACT_D)trigCos(PROJ_MAX_PITCH);
			fT = FRACTdiv_D( fS, fC );
			fS = ACC_GRAVITY*(MAKEFRACT_D(1)+FRACTmul_D(fT,fT));
			fS = FRACTdiv_D(fS,(2 * (FRACTmul_D(fR,fT) - MAKEFRACT_D(dz))));
			{
				FRACT_D Tmp;
				Tmp = FRACTmul_D(fR,fR);
				iVel = MAKEINT_D( trigIntSqrt(MAKEINT_D(FRACTmul_D(fS,Tmp))) );
			}
		}
		else
		{
			/* set velocity to stats value */
			iVel = psObj->psWStats->flightSpeed;

			/* get floating point square root */
			fS = trigIntSqrt( MAKEINT_D(fS) );

			fT = (FRACT_D) atan2(fR+fS, 2*fA);

			/* make sure angle positive */
			if ( fT < 0 )
			{
				fT += (FRACT_D) (2*PI);
			}
			iPitchLow = MAKEINT_D(RAD_TO_DEG(fT));

			fT = (FRACT_D) atan2(fR-fS, 2*fA);
			/* make sure angle positive */
			if ( fT < 0 )
			{
				fT += (FRACT_D) (2*PI);
			}
			iPitchHigh = MAKEINT_D(RAD_TO_DEG(fT));

			/* swap pitches if wrong way round */
			if ( iPitchLow > iPitchHigh )
			{
				iTemp = iPitchHigh;
				iPitchLow = iPitchHigh;
				iPitchHigh = iTemp;
			}

			/* chooselow pitch unless -ve */
			if ( iPitchLow > 0 )
			{
				psObj->pitch = (SWORD)iPitchLow;
			}
			else
			{
				psObj->pitch = (SWORD)iPitchHigh;
			}
		}

		/* if droid set muzzle pitch */
		//Watermelon:fix turret pitch for more turrets
		if (psAttacker != NULL && weapon_slot >= 0)
		{
			if (psAttacker->type == OBJ_DROID)
			{
				((DROID *) psAttacker)->turretPitch[weapon_slot] = psObj->pitch;
			}
			else if (psAttacker->type == OBJ_STRUCTURE)
			{
				((STRUCTURE *) psAttacker)->turretPitch[weapon_slot] = psObj->pitch;
			}
		}

		psObj->vXY = MAKEINT_D(iVel * trigCos(psObj->pitch));
		psObj->vZ  = MAKEINT_D(iVel * trigSin(psObj->pitch));

		/* set function pointer */
		psObj->pInFlightFunc = proj_InFlightIndirectFunc;
	}

	/* put object in hashtable */
	hashTable_InsertElement( g_pProjObjTable, psObj, (int) psObj, UNUSED_KEY );

	/* play firing audio */
	// only play if either object is visible, i know it's a bit of a hack, but it avoids the problem
	// of having to calculate real visibility values for each projectile.
	if ( bVisible || gfxVisible(psObj) )
	{
		// note that the projectile is visible
		psObj->bVisible = TRUE;

		if ( psObj->psWStats->iAudioFireID != NO_SOUND )
		{

            if ( psObj->psSource )
            {
				/* firing sound emitted from source */
    			audio_PlayObjDynamicTrack( (BASE_OBJECT *) psObj->psSource,
									psObj->psWStats->iAudioFireID, NULL );
				/* GJ HACK: move howitzer sound with shell */
				if ( psObj->psWStats->weaponSubClass == WSC_HOWITZERS )
				{
    				audio_PlayObjDynamicTrack( (BASE_OBJECT *) psObj,
									ID_SOUND_HOWITZ_FLIGHT, NULL );
				}
            }
			//don't play the sound for a LasSat in multiPlayer
            else if (!(bMultiPlayer AND psWeapStats->weaponSubClass == WSC_LAS_SAT))
			{
                    audio_PlayObjStaticTrack(psObj, psObj->psWStats->iAudioFireID);
            }
		}
	}

	if ((psAttacker != NULL) && !proj_Direct(psWeapStats))
	{
		//check for Counter Battery Sensor in range of target
		counterBatteryFire(psAttacker, psTarget);
	}

	return TRUE;
}

/***************************************************************************/

void
proj_InFlightDirectFunc( PROJ_OBJECT *psObj )
{
	WEAPON_STATS	*psStats;
	SDWORD			timeSoFar;
	SDWORD			dx, dy, dz, iX, iY, dist, xdiff, ydiff;
	//Watermelon: make zdiff always positive
	UDWORD			zdiff;
	SDWORD			rad;
	iVector			pos;
	//Watermelon:int i
	UDWORD			i;
	//Watermelon:2 temp BASE_OBJECT pointer
	BASE_OBJECT		*psTempObj;
	BASE_OBJECT		*psNewTarget;
	//Watermelon:Missile or not
	BOOL			bMissile = FALSE;
	//Watermelon:extended 'lifespan' of a projectile
	//no more disappeared projectiles.
	SDWORD			extendRad;
	//Watermelon:given explosive weapons some 'hit collision' bonus
	//esp the AAGun,or it will never hit anything with the new hit system
	UDWORD			wpRadius = 1;
	//Watermelon:Penetrate or not
	BOOL			bPenetrate;
	WEAPON			asWeap;

	ASSERT( PTRVALID(psObj, sizeof(PROJ_OBJECT)),
		"proj_InFlightDirectFunc: invalid projectile pointer" );

	psStats = psObj->psWStats;
	ASSERT( PTRVALID(psStats, sizeof(WEAPON_STATS)),
		"proj_InFlightDirectFunc: Invalid weapon stats pointer" );

	bPenetrate = psStats->penetrate;
	timeSoFar = gameTime - psObj->born;

    //we want a delay between Las-Sats firing and actually hitting in multiPlayer
		//magic number but that's how long the audio countdown message lasts!
    if ( bMultiPlayer && psStats->weaponSubClass == WSC_LAS_SAT && timeSoFar < 8 * GAME_TICKS_PER_SEC )
	{
		return;
    }

	/* If it's homing and it has a target (not a miss)... */
	if ( psStats->movementModel == MM_HOMINGDIRECT && psObj->psDest )
	{
		dx = (SDWORD)psObj->psDest->x - (SDWORD)psObj->startX;
		dy = (SDWORD)psObj->psDest->y - (SDWORD)psObj->startY;
		dz = (SDWORD)psObj->psDest->z - (SDWORD)psObj->srcHeight;
	}
	else
	{
		dx = (SDWORD)psObj->tarX - (SDWORD)psObj->startX;
		dy = (SDWORD)psObj->tarY - (SDWORD)psObj->startY;
		dz = (SDWORD)(psObj->altChange);
	}

	// ffs
	rad = (SDWORD)iSQRT( dx*dx + dy*dy );
	//Watermelon:extended life span
	extendRad = (SDWORD)(rad * 1.5f);

	if (rad == 0) // Prevent div by zero
	{
		rad = 1;
	}

	dist = timeSoFar * psStats->flightSpeed / GAME_TICKS_PER_SEC;

	iX = psObj->startX + (dist * dx / rad);
	iY = psObj->startY + (dist * dy / rad);

	/* impact if about to go off map else update coordinates */
	if ( worldOnMap( iX, iY ) == FALSE )
	{
		psObj->state = PROJ_IMPACT;
		debug( LOG_NEVER, "**** projectile off map - removed ****\n" );
		return;
	}
	else
	{
		psObj->x = (UWORD)iX;
		psObj->y = (UWORD)iY;
	}
	psObj->z = (UWORD)(psObj->srcHeight) + (dist * dz / rad);

	if ( gfxVisible(psObj) )
	{
		if ( psStats->weaponSubClass == WSC_FLAME )
		{
			effectGiveAuxVar(PERCENT(dist,rad));
			pos.x = psObj->x;
			pos.y = psObj->z-8;
			pos.z = psObj->y;
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_FLAMETHROWER,FALSE,NULL,0);
		}
		else if ( psStats->weaponSubClass == WSC_COMMAND ||
			psStats->weaponSubClass == WSC_ELECTRONIC ||
			psStats->weaponSubClass == WSC_EMP )
		{
			effectGiveAuxVar((PERCENT(dist,rad)/2));
			pos.x = psObj->x;
			pos.y = psObj->z-8;
			pos.z = psObj->y;
  			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_LASER,FALSE,NULL,0);
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_FLARE,FALSE,NULL,0);
		}
		else if ( psStats->weaponSubClass == WSC_ROCKET ||
			psStats->weaponSubClass == WSC_MISSILE ||
			psStats->weaponSubClass == WSC_SLOWROCKET ||
			psStats->weaponSubClass == WSC_SLOWMISSILE )
		{
			pos.x = psObj->x;
			pos.y = psObj->z+8;
			pos.z = psObj->y;
			addEffect(&pos,EFFECT_SMOKE,SMOKE_TYPE_TRAIL,FALSE,NULL,0);
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_FLARE,FALSE,NULL,0);
		}
	}

	//Watermelon:weapon radius,or the 'real' projectile will never hit a moving target with the changes...
	if ( psStats->weaponSubClass == WSC_ROCKET ||
		psStats->weaponSubClass == WSC_MISSILE ||
		psStats->weaponSubClass == WSC_SLOWROCKET ||
		psStats->weaponSubClass == WSC_SLOWMISSILE )
	{
		wpRadius = 2; // Increase hitradius
		bMissile = TRUE;
	}
	else if ( psStats->weaponSubClass == WSC_MGUN ||
			psStats->weaponSubClass == WSC_COMMAND )
	{
		wpRadius = 2; // Increase hitradius
		//Watermelon:extended life span
		extendRad = (SDWORD)(rad * 1.2f);
	}
	else if ( psStats->weaponSubClass == WSC_CANNON ||
			psStats->weaponSubClass == WSC_BOMB ||
			psStats->weaponSubClass == WSC_ELECTRONIC ||
			psStats->weaponSubClass == WSC_EMP ||
			psStats->weaponSubClass == WSC_FLAME ||
			psStats->weaponSubClass == WSC_ENERGY ||
			psStats->weaponSubClass == WSC_GAUSS )
	{
		wpRadius = 2; // Increase hitradius
		//Watermelon:extended life span
		extendRad = (SDWORD)(rad * 1.5f);
	}
	else if ( psStats->weaponSubClass == WSC_AAGUN )
	{
		wpRadius = 16;
		extendRad = rad;
	}

	for (i = 0;i < numProjNaybors;i++)
	{
		if ( asProjNaybors[i].psObj->player != psObj->player &&
			( asProjNaybors[i].psObj->type == OBJ_DROID ||
			asProjNaybors[i].psObj->type == OBJ_STRUCTURE ||
			asProjNaybors[i].psObj->type == OBJ_BULLET ||
			asProjNaybors[i].psObj->type == OBJ_FEATURE ) &&
			!aiCheckAlliances(asProjNaybors[i].psObj->player,psObj->player) )
		{
			psTempObj = asProjNaybors[i].psObj;
			if ( psTempObj == psObj->psDamaged )
			{
				continue;
			}

			//Watermelon;so a projectile wont collide with another projectile unless it's a counter-missile weapon
			if ( psTempObj->type == OBJ_BULLET && !( bMissile || ((PROJ_OBJECT *)psTempObj)->psWStats->weaponSubClass == WSC_COUNTER ) )
			{
				continue;
			}

			//Watermelon:ignore oil resource and pickup
			if ( psTempObj->type == OBJ_FEATURE &&
				((FEATURE *)psTempObj)->psStats->damageable == 0 )
			{
				continue;
			}

			if ( psTempObj->type == OBJ_STRUCTURE || psTempObj->type == OBJ_FEATURE )
			{
				//Watermelon:AA weapon shouldnt hit buildings
				if ( psObj->psWStats->surfaceToAir == SHOOT_IN_AIR )
				{
					continue;
				}

				xdiff = (SDWORD)psObj->x - (SDWORD)psTempObj->x;
				ydiff = (SDWORD)psObj->y - (SDWORD)psTempObj->y;
				zdiff = abs((UDWORD)psObj->z - (UDWORD)psTempObj->z);

				if ( zdiff < establishTargetHeight(psTempObj) && 
					(xdiff*xdiff + ydiff*ydiff) < ( (SDWORD)establishTargetRadius(psTempObj) * (SDWORD)establishTargetRadius(psTempObj) ) )
				{
					psNewTarget = psTempObj;
					psObj->psDest = psNewTarget;
					psObj->state = PROJ_IMPACT;
					return;
				}
			}
			else
			{
				if ( psObj->psWStats->surfaceToAir == SHOOT_IN_AIR &&
					psTempObj->type == OBJ_DROID && !vtolDroid((DROID *)psTempObj) )
				{
					continue;
				}

				xdiff = (SDWORD)psObj->x - (SDWORD)psTempObj->x;
				ydiff = (SDWORD)psObj->y - (SDWORD)psTempObj->y;
				zdiff = abs((UDWORD)psObj->z - (UDWORD)psTempObj->z);
		
				if ( zdiff < establishTargetHeight(psTempObj) &&
					(xdiff*xdiff + ydiff*ydiff) < (SDWORD)( wpRadius * establishTargetRadius(psTempObj) * establishTargetRadius(psTempObj) ) )
				{
					psNewTarget = psTempObj;
					psObj->psDest = psNewTarget;

					if (bPenetrate)
					{
						asWeap.nStat = psObj->psWStats - asWeaponStats;
						//Watermelon:just assume we damaged the chosen target
						psObj->psDamaged = psNewTarget;

						proj_SendProjectile( &asWeap, (BASE_OBJECT*)psObj, psObj->player, (psObj->startX + extendRad * dx / rad), (psObj->startY + extendRad * dy / rad), psObj->z, NULL, TRUE, bPenetrate, -1 );
					}
					else
					{
						psObj->state = PROJ_IMPACT;
					}
					return;
				}
			}
		}
	}

	/* See if effect has finished */
	if ( psStats->movementModel == MM_HOMINGDIRECT && psObj->psDest )
	{
		xdiff = (SDWORD)psObj->x - (SDWORD)psObj->psDest->x;
		ydiff = (SDWORD)psObj->y - (SDWORD)psObj->psDest->y;
		zdiff = abs((UDWORD)psObj->z - (UDWORD)psObj->psDest->z);

		if (zdiff < establishTargetHeight(psObj->psDest) &&
			(xdiff*xdiff + ydiff*ydiff) < ((SDWORD)psObj->targetRadius * (SDWORD)psObj->targetRadius) )
		{
		  	psObj->state = PROJ_IMPACT;
		}
	}
	else if ( dist > extendRad - (SDWORD)psObj->targetRadius )
	{
		psObj->state = PROJ_IMPACT;

		/* It's damage time */
		if (psObj->psDest)
		{
			xdiff = (SDWORD)psObj->x - (SDWORD)psObj->psDest->x;
			ydiff = (SDWORD)psObj->y - (SDWORD)psObj->psDest->y;
			zdiff = abs((UDWORD)psObj->z - (UDWORD)psObj->psDest->z);

			//Watermelon:'real' hit check even if the projectile is about to 'timeout'
			if ( zdiff < establishTargetHeight(psObj->psDest) &&
				(xdiff*xdiff + ydiff*ydiff) < (SDWORD)( wpRadius * establishTargetRadius(psObj->psDest) * establishTargetRadius(psObj->psDest) ) )
			{
			}
			else
			{
				//Watermelon:missed.you can now 'dodge' projectile by micro,so cyborgs should be more useful now
				psObj->psDest = NULL;
			}
		}
	}

#if CHECK_PROJ_ABOVE_GROUND
	/* check not trying to travel through terrain - if so count as a miss */
	if ( mapObjIsAboveGround( (BASE_OBJECT *) psObj ) == FALSE )
	{
		psObj->state = PROJ_IMPACT;
		/* miss registered if NULL target */
		psObj->psDest = NULL;
		return;
	}
#endif

	/* add smoke trail to indirect weapons firing directly */
	if( !proj_Direct( psStats ) && gfxVisible(psObj))
	{
		pos.x = psObj->x;
		pos.y = psObj->z+8;
		pos.z = psObj->y;
		addEffect(&pos,EFFECT_SMOKE,SMOKE_TYPE_TRAIL,FALSE,NULL,0);
	}
}

/***************************************************************************/

void
proj_InFlightIndirectFunc( PROJ_OBJECT *psObj )
{
	WEAPON_STATS	*psStats;
	SDWORD			iTime, iRad, iDist, dx, dy, dz, iX, iY;
	iVector			pos;
	FRACT			fVVert;
	BOOL			bOver = FALSE;
	//Watermelon:psTempObj,psNewTarget,i,xdiff,ydiff,zdiff
	BASE_OBJECT		*psTempObj;
	BASE_OBJECT		*psNewTarget;
	UDWORD			i;
	SDWORD			xdiff,ydiff,extendRad;
	UDWORD			zdiff;
	UDWORD			wpRadius = 3;
	BOOL			bPenetrate;
	WEAPON			asWeap;

	ASSERT( PTRVALID(psObj, sizeof(PROJ_OBJECT)),
		"proj_InFlightIndirectFunc: invalid projectile pointer" );

	psStats = psObj->psWStats;
	bPenetrate = psStats->penetrate;
	ASSERT( PTRVALID(psStats, sizeof(WEAPON_STATS)),
		"proj_InFlightIndirectFunc: Invalid weapon stats pointer" );

	iTime = gameTime - psObj->born;

	dx = (SDWORD)psObj->tarX-(SDWORD)psObj->startX;
	dy = (SDWORD)psObj->tarY-(SDWORD)psObj->startY;

	// ffs
	iRad = (SDWORD)iSQRT( dx*dx + dy*dy );

	iDist = iTime * psObj->vXY / GAME_TICKS_PER_SEC;

	iX = psObj->startX + (iDist * dx / iRad);
	iY = psObj->startY + (iDist * dy / iRad);

	/* impact if about to go off map else update coordinates */
	if ( !worldOnMap( iX, iY ) )
	{
	  	psObj->state = PROJ_IMPACT;
		debug( LOG_NEVER, "**** projectile off map - removed ****\n" );
		return;
	}
	else
	{
		psObj->x = (UWORD)iX;
		psObj->y = (UWORD)iY;
	}

	dz = (psObj->vZ - (iTime*ACC_GRAVITY/GAME_TICKS_PER_SEC/2)) *
				iTime / GAME_TICKS_PER_SEC;
	psObj->z = (UWORD)(psObj->srcHeight + dz);

	fVVert = MAKEFRACT(psObj->vZ - (iTime*ACC_GRAVITY/GAME_TICKS_PER_SEC));
	psObj->pitch = (SWORD)( RAD_TO_DEG(atan2(fVVert, psObj->vXY)) );

	//Watermelon:extended life span for artillery projectile
	extendRad = (SDWORD)(iRad * 1.2f);

	if( gfxVisible(psObj) )
	{
		switch(psStats->weaponSubClass)
		{
			case WSC_FLAME:
				effectGiveAuxVar(PERCENT(iDist,iRad));
				pos.x = psObj->x;
				pos.y = psObj->z-8;
				pos.z = psObj->y;
				addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_FLAMETHROWER,FALSE,NULL,0);
				break;
			case WSC_COMMAND:
			case WSC_ELECTRONIC:
			case WSC_EMP:
				effectGiveAuxVar((PERCENT(iDist,iRad)/2));
				pos.x = psObj->x;
				pos.y = psObj->z-8;
				pos.z = psObj->y;
				addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_LASER,FALSE,NULL,0);
				addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_FLARE,FALSE,NULL,0);
				break;
			case WSC_ROCKET:
			case WSC_MISSILE:
			case WSC_SLOWROCKET:
			case WSC_SLOWMISSILE:
				pos.x = psObj->x;
				pos.y = psObj->z+8;
				pos.z = psObj->y;
				addEffect(&pos,EFFECT_SMOKE,SMOKE_TYPE_TRAIL,FALSE,NULL,0);
				addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_FLARE,FALSE,NULL,0);
				break;
		}
	}

	//Watermelon:test test
	for (i = 0;i < numProjNaybors;i++)
	{
		if (asProjNaybors[i].psObj->player != psObj->player &&
			(asProjNaybors[i].psObj->type == OBJ_DROID ||
			asProjNaybors[i].psObj->type == OBJ_STRUCTURE ||
			asProjNaybors[i].psObj->type == OBJ_BULLET ||
			asProjNaybors[i].psObj->type == OBJ_FEATURE) &&
			!aiCheckAlliances(asProjNaybors[i].psObj->player,psObj->player))
		{
			psTempObj = asProjNaybors[i].psObj;
			//Watermelon;dont collide with any other projectiles
			if ( psTempObj->type == OBJ_BULLET )
			{
				continue;
			}

			//Watermelon:ignore oil resource and pickup
			if ( psTempObj->type == OBJ_FEATURE &&
				((FEATURE *)psTempObj)->psStats->damageable == 0 )
			{
				continue;
			}

			if ( psTempObj->type == OBJ_STRUCTURE || psTempObj->type == OBJ_FEATURE )
			{
				xdiff = (SDWORD)psObj->x - (SDWORD)psTempObj->x;
				ydiff = (SDWORD)psObj->y - (SDWORD)psTempObj->y;
				zdiff = abs((UDWORD)psObj->z - (UDWORD)psTempObj->z);

				if ( zdiff < establishTargetHeight(psTempObj) &&
					(xdiff*xdiff + ydiff*ydiff) < ( (SDWORD)establishTargetRadius(psTempObj) * (SDWORD)establishTargetRadius(psTempObj) ) )
				{
					psNewTarget = psTempObj;
					psObj->psDest = psNewTarget;
		  			psObj->state = PROJ_IMPACT;
					return;
				}
			}
			else
			{
				xdiff = (SDWORD)psObj->x - (SDWORD)psTempObj->x;
				ydiff = (SDWORD)psObj->y - (SDWORD)psTempObj->y;
				zdiff = abs((UDWORD)psObj->z - (UDWORD)psTempObj->z);

				if ( zdiff < establishTargetHeight(psTempObj) &&
					(UDWORD)(xdiff*xdiff + ydiff*ydiff) < ( wpRadius * (SDWORD)establishTargetRadius(psTempObj) * (SDWORD)establishTargetRadius(psTempObj) ) )
				{
					psNewTarget = psTempObj;
					psObj->psDest = psNewTarget;

					if (bPenetrate)
					{
						asWeap.nStat = psObj->psWStats - asWeaponStats;
						//Watermelon:just assume we damaged the chosen target
						psObj->psDamaged = psNewTarget;

						proj_SendProjectile( &asWeap, (BASE_OBJECT*)psObj, psObj->player, (psObj->startX + extendRad * dx / iRad), (psObj->startY + extendRad * dy / iRad), psObj->z, NULL, TRUE, bPenetrate, -1 );
					}
					else
					{
						psObj->state = PROJ_IMPACT;
					}
					return;
				}
			}
		}
	}

	/* See if effect has finished */
	if ( iDist > (extendRad - (SDWORD)psObj->targetRadius) )
	{
		psObj->state = PROJ_IMPACT;

		pos.x = psObj->x;
		pos.z = psObj->y;
		pos.y = map_Height(pos.x,pos.z) + 8;

		/* It's damage time */
		//Watermelon:'real' check
		if ( psObj->psDest )
		{
			psObj->z = (UWORD)(pos.y + 8); // bring up the impact explosion

			xdiff = (SDWORD)psObj->x - (SDWORD)psObj->psDest->x;
			ydiff = (SDWORD)psObj->y - (SDWORD)psObj->psDest->y;
			zdiff = abs((UDWORD)psObj->z - (UDWORD)psObj->psDest->z);

			//Watermelon:dont apply the 'hitbox' bonus if the target is a building
			if ( psObj->psDest->type == OBJ_STRUCTURE )
			{
				wpRadius = 1;
			}

			if ( zdiff < establishTargetHeight(psObj->psDest) &&
				(xdiff*xdiff + ydiff*ydiff) < (SDWORD)( wpRadius * establishTargetRadius(psObj->psDest) * establishTargetRadius(psObj->psDest) ) )
			{
				psObj->state = PROJ_IMPACT;
			}
			else
			{
				psObj->psDest = NULL;
			}
		}
		bOver = TRUE;
	}

#if CHECK_PROJ_ABOVE_GROUND
	/* check not trying to travel through terrain - if so count as a miss */
	if ( mapObjIsAboveGround( (BASE_OBJECT *) psObj ) == FALSE )
	{
		psObj->state = PROJ_IMPACT;
		/* miss registered if NULL target */
		psObj->psDest = NULL;
		bOver = TRUE;
	}
#endif

	/* Add smoke particle at projectile location (in world coords) */
	/* Add a trail graphic */
	/* If it's indirect and not a flamethrower - add a smoke trail! */
	if ( psStats->weaponSubClass != WSC_FLAME &&
		psStats->weaponSubClass != WSC_ENERGY &&
		psStats->weaponSubClass != WSC_COMMAND &&
		psStats->weaponSubClass != WSC_ELECTRONIC &&
		psStats->weaponSubClass != WSC_EMP &&
		!bOver )
	{
		if(gfxVisible(psObj))
		{
			pos.x = psObj->x;
			pos.y = psObj->z+4;
			pos.z = psObj->y;
			addEffect(&pos,EFFECT_SMOKE,SMOKE_TYPE_TRAIL,FALSE,NULL,0);
		}
	}
}

/***************************************************************************/

void
proj_ImpactFunc( PROJ_OBJECT *psObj )
{
	WEAPON_STATS	*psStats;
	SDWORD			i, iAudioImpactID;
	DROID			*psCurrD, *psNextD;
	STRUCTURE		*psCurrS, *psNextS;
	FEATURE			*psCurrF, *psNextF;
	UDWORD			dice;
	SDWORD			tarX0,tarY0, tarX1,tarY1;
	SDWORD			radSquared, xDiff,yDiff;
	BOOL			bKilled;//,bMultiTemp;
	iVector			position,scatter;
	UDWORD			damage;	//optimisation - were all being calculated twice on PC
	//Watermelon: tarZ0,tarZ1,zDiff for AA AOE weapons;
	SDWORD			tarZ0,tarZ1,zDiff;
	int				impact_angle;


	ASSERT( PTRVALID(psObj, sizeof(PROJ_OBJECT)),
		"proj_ImpactFunc: invalid projectile pointer" );

	psStats = psObj->psWStats;
	ASSERT( PTRVALID(psStats, sizeof(WEAPON_STATS)),
		"proj_ImpactFunc: Invalid weapon stats pointer" );

	/* play impact audio */
	if(gfxVisible(psObj))
	{
		if ( psStats->iAudioImpactID == NO_SOUND)
		{
			/* play richochet if MG */
			if ( psObj->psDest != NULL && psObj->psWStats->weaponSubClass == WSC_MGUN
					&& ONEINTHREE )
			{
				iAudioImpactID = ID_SOUND_RICOCHET_1 + (rand()%3);
				audio_PlayStaticTrack( psObj->psDest->x, psObj->psDest->y, iAudioImpactID );
			}
		}
		else
		{
			if ( psObj->psDest == NULL )
			{
				audio_PlayStaticTrack( psObj->tarX, psObj->tarY,
							psStats->iAudioImpactID );
			}
			else
			{
				audio_PlayStaticTrack( psObj->psDest->x, psObj->psDest->y,
							psStats->iAudioImpactID );
			}
		}
	}
	// note the attacker if any
	g_pProjLastAttacker = psObj->psSource;

	if(gfxVisible(psObj))
	{
		/* Set stuff burning if it's a flame droid... */
//		if(psStats->weaponSubClass == WSC_FLAME)
//		{
			/* Shouldn't need to do this check but the stats aren't all at a value yet... */ // FIXME
			if ( psStats->incenRadius && psStats->incenTime )
			{
				position.x = psObj->tarX;
				position.z = psObj->tarY;
				position.y = map_Height(position.x, position.z);
				effectGiveAuxVar(psStats->incenRadius);
				effectGiveAuxVarSec(psStats->incenTime);
				addEffect(&position,EFFECT_FIRE,FIRE_TYPE_LOCALISED,FALSE,NULL,0);
			}
//		}
		// may want to add both a fire effect and the las sat effect
		if (psStats->weaponSubClass == WSC_LAS_SAT)
		{
			position.x = psObj->tarX;
			position.z = psObj->tarY;
			position.y = map_Height(position.x, position.z);
			addEffect(&position,EFFECT_SAT_LASER,SAT_LASER_STANDARD,FALSE,NULL,0);
			if(clipXY(psObj->tarX,psObj->tarY))
			{
				shakeStart();
			}
		}
	}
	/* Nothings been killed */
	bKilled = FALSE;
	if ( psObj->psDest != NULL )
	{
		ASSERT( PTRVALID(psObj->psDest, sizeof(BASE_OBJECT)),
			"proj_ImpactFunc: Invalid destination object pointer" );
	}

	if ( psObj->psDest == NULL )
	{
		/* The bullet missed or the target was destroyed in flight */
		/* So show the MISS effect */
	 	position.x = psObj->x;
		position.z = psObj->y;
		position.y = psObj->z;//map_Height(psObj->x, psObj->y) + 24;//jps
		scatter.x = psStats->radius; scatter.y = 0;	scatter.z = psStats->radius;

		if ( gfxVisible(psObj) )
		{
			// you got to have at least 1 in the numExplosions field or you'll see nowt..:-)
			if (psObj->airTarget)
			{
				if ((psStats->surfaceToAir & SHOOT_IN_AIR) && !(psStats->surfaceToAir & SHOOT_ON_GROUND))
				{
					if (psStats->facePlayer)
					{
						addMultiEffect(&position,&scatter,EFFECT_EXPLOSION,EXPLOSION_TYPE_SPECIFIED,TRUE,psStats->pTargetMissGraphic,psStats->numExplosions,psStats->lightWorld,psStats->effectSize);
					}
					else
					{
						addMultiEffect(&position,&scatter,EFFECT_EXPLOSION,EXPLOSION_TYPE_NOT_FACING,TRUE,psStats->pTargetMissGraphic,psStats->numExplosions,psStats->lightWorld,psStats->effectSize);
					}
					/* Add some smoke to flak */
					addMultiEffect(&position,&scatter,EFFECT_SMOKE,SMOKE_TYPE_DRIFTING,FALSE,NULL,3,0,0);
				}
			}
			else if( TERRAIN_TYPE(mapTile(psObj->x/TILE_UNITS,psObj->y/TILE_UNITS)) == TER_WATER )
			{
				if (psStats->facePlayer)
				{
					addMultiEffect(&position,&scatter,EFFECT_EXPLOSION,EXPLOSION_TYPE_SPECIFIED,TRUE,psStats->pWaterHitGraphic,psStats->numExplosions,psStats->lightWorld,psStats->effectSize);
				}
				else
				{
					addMultiEffect(&position,&scatter,EFFECT_EXPLOSION,EXPLOSION_TYPE_NOT_FACING,TRUE,psStats->pWaterHitGraphic,psStats->numExplosions,psStats->lightWorld,psStats->effectSize);
				}
			}
			else
			{
				if (psStats->facePlayer)
				{
					addMultiEffect(&position,&scatter,EFFECT_EXPLOSION,EXPLOSION_TYPE_SPECIFIED,TRUE,psStats->pTargetMissGraphic,psStats->numExplosions,psStats->lightWorld,psStats->effectSize);
				}
				else
				{
					addMultiEffect(&position,&scatter,EFFECT_EXPLOSION,EXPLOSION_TYPE_NOT_FACING,TRUE,psStats->pTargetMissGraphic,psStats->numExplosions,psStats->lightWorld,psStats->effectSize);
				}
			}
		}
	}
	else
	{
		if (psObj->psDest->type == OBJ_FEATURE &&
			((FEATURE *)psObj->psDest)->psStats->damageable == 0)
		{
			debug( LOG_NEVER, "proj_ImpactFunc: trying to damage non-damageable target,projectile removed\n");
			if ( hashTable_RemoveElement( g_pProjObjTable, psObj, (int) psObj, UNUSED_KEY ) == FALSE )
			{
				debug( LOG_NEVER, "proj_ImpactFunc: couldn't remove projectile from table\n" );
			}
			return;
		}
		position.x = psObj->x;
		position.z = psObj->y;
		position.y = psObj->z;//map_Height(psObj->x, psObj->y) + 24;
		scatter.x = psStats->radius; scatter.y = 0;	scatter.z = psStats->radius;

		if(gfxVisible(psObj))
		{
			/* Watermelon:gives AA gun explosive field and smoke effect even if it hits the target */
			if (psObj->airTarget)
			{
				if ((psStats->surfaceToAir & SHOOT_IN_AIR) &&
					!(psStats->surfaceToAir & SHOOT_ON_GROUND) &&
					psStats->weaponSubClass == WSC_AAGUN)
				{
					if(psStats->facePlayer)
					{
						addMultiEffect(&position,&scatter,EFFECT_EXPLOSION,EXPLOSION_TYPE_SPECIFIED,TRUE,psStats->pTargetMissGraphic,psStats->numExplosions,psStats->lightWorld,psStats->effectSize);
					}
					else
					{
						addMultiEffect(&position,&scatter,EFFECT_EXPLOSION,EXPLOSION_TYPE_NOT_FACING,TRUE,psStats->pTargetMissGraphic,psStats->numExplosions,psStats->lightWorld,psStats->effectSize);
					}
					/* Add some smoke to flak */
					addMultiEffect(&position,&scatter,EFFECT_SMOKE,SMOKE_TYPE_DRIFTING,FALSE,NULL,3,0,0);
				}
			}
			else
			{
				if(psStats->facePlayer)
				{
					addMultiEffect(&position,&scatter,EFFECT_EXPLOSION,EXPLOSION_TYPE_SPECIFIED,TRUE,psStats->pTargetHitGraphic,psStats->numExplosions,psStats->lightWorld,psStats->effectSize);
				}
				else
				{
					addMultiEffect(&position,&scatter,EFFECT_EXPLOSION,EXPLOSION_TYPE_NOT_FACING,TRUE,psStats->pTargetHitGraphic,psStats->numExplosions,psStats->lightWorld,psStats->effectSize);
				}
			}
		}

		/* Do damage to the target */
		if (proj_Direct(psStats))
		{
			/*Check for Electronic Warfare damage where we know the subclass
            of the weapon*/
			if (psStats->weaponSubClass == WSC_ELECTRONIC)// AND psObj->psDest->
				//type == OBJ_STRUCTURE)
			{
				if (psObj->psSource)
				{
					//if (electronicDamage((STRUCTURE *)psObj->psDest, calcDamage(
                    if (electronicDamage(psObj->psDest, calcDamage(weaponDamage(
                        psStats,psObj->player), psStats->weaponEffect,
                        psObj->psDest), psObj->player))
					{
						if (psObj->psSource->type == OBJ_DROID)
						{
							//the structure has lost all resistance so quit
							((DROID *)psObj->psSource)->order = DORDER_NONE;
							actionDroid((DROID *)(psObj->psSource), DACTION_NONE);
						}
                        else if (psObj->psSource->type == OBJ_STRUCTURE)
                        {
                            //the droid has lost all resistance - init the structures target
                            ((STRUCTURE *)psObj->psSource)->psTarget[0] = NULL;
                        }
					}
				}
			}
			else
			{
				/* Just assume a direct fire weapon hits the target */
				damage = calcDamage(weaponDamage(psStats,psObj->player), psStats->weaponEffect, psObj->psDest);
				if(bMultiPlayer)
				{
					if(psObj->psSource && myResponsibility(psObj->psSource->player)  )
					{
						updateMultiStatsDamage(psObj->psSource->player,psObj->psDest->player,damage);
					}
				}

				debug(LOG_NEVER, "Damage to object %d, player %d\n",
						psObj->psDest->id, psObj->psDest->player);
				/*the damage depends on the weapon effect and the target propulsion type or structure strength*/

				if (psObj->psDest->type == OBJ_DROID)
				{
					if (psObj->altChange > 300)
					{
						impact_angle = HIT_ANGLE_TOP;
					}
					else if (psObj->z < (psObj->psDest->z - 50))
					{
						impact_angle = HIT_ANGLE_BOTTOM;
					}
					else
					{
						xDiff = psObj->startX - psObj->psDest->x;
						yDiff = psObj->startY - psObj->psDest->y;
						impact_angle = abs(psObj->psDest->direction - (180 * atan2f((float)xDiff, (float)yDiff) /PI));
						if (impact_angle >= 360)
						{
							impact_angle -= 360;
						}
					}
				}
				else
				{
					impact_angle = 0;
				}
	  			bKilled = objectDamage(psObj->psDest,damage , psStats->weaponClass,psStats->weaponSubClass, impact_angle);

				if(bKilled)
				{
					proj_UpdateKills(psObj);
				}
				else
				{
					psObj->psDamaged = psObj->psDest;
				}
			}
		}
		else
		{
			/* See if the target is still close to the impact point */
			/* Should do a better test than this but as we don't have */
			/* a world size for objects we'll just see if it is within */
			/* a tile of the bullet */
			/***********************************************************/
			/*  MIGHT WANT TO CHANGE THIS                              */
			/***********************************************************/
			if ( (psObj->psDest->type == OBJ_STRUCTURE) ||
				 (psObj->psDest->type == OBJ_FEATURE) ||
				(((SDWORD)psObj->psDest->x >= (SDWORD)psObj->x - TILE_UNITS) &&
				((SDWORD)psObj->psDest->x <= (SDWORD)psObj->x + TILE_UNITS) &&
				((SDWORD)psObj->psDest->y >= (SDWORD)psObj->y - TILE_UNITS) &&
				((SDWORD)psObj->psDest->y <= (SDWORD)psObj->y + TILE_UNITS)))
			{

				damage = calcDamage(weaponDamage(
							psStats, psObj->player),
							psStats->weaponEffect, psObj->psDest);
				if(bMultiPlayer)
				{
					if(psObj->psSource && myResponsibility(psObj->psSource->player))
					{
						//updateMultiStatsDamage(psObj->psSource->player,psObj->psDest->player,psStats->damage);
						updateMultiStatsDamage(psObj->psSource->player,
							//psObj->psDest->player,	calcDamage(psStats->damage,
							psObj->psDest->player, damage	);
					}
				}
				debug(LOG_NEVER, "Damage to object %d, player %d\n",
						psObj->psDest->id, psObj->psDest->player);
				//Watermelon:just assume it as from TOP for indirect artillery
				if (psObj->psDest->type == OBJ_DROID)
				{
					impact_angle = HIT_ANGLE_TOP;
				}
				else
				{
					impact_angle = HIT_SIDE_FRONT;
				}
				bKilled = objectDamage(psObj->psDest, damage, psStats->weaponClass,psStats->weaponSubClass, impact_angle);

				if(bKilled)
				{
					proj_UpdateKills(psObj);
				}
				else
				{
					psObj->psDamaged = psObj->psDest;
				}
			}
		}
	}

	if ((psStats->radius == 0) && (psStats->incenTime == 0) )
	{
		position.x = psObj->x;
		position.z = psObj->y;
		position.y = psObj->z;
		scatter.x = psStats->radius; scatter.y = 0;	scatter.z = psStats->radius;

		if(gfxVisible(psObj))
		{
			if(TERRAIN_TYPE(mapTile(psObj->x/TILE_UNITS,psObj->y/TILE_UNITS)) == TER_WATER)
			{
				if(psStats->facePlayer)
				{
					addMultiEffect(&position,&scatter,EFFECT_EXPLOSION,EXPLOSION_TYPE_SPECIFIED,TRUE,psStats->pWaterHitGraphic,psStats->numExplosions,psStats->lightWorld,psStats->effectSize);
				}
				else
				{
					addMultiEffect(&position,&scatter,EFFECT_EXPLOSION,EXPLOSION_TYPE_NOT_FACING,TRUE,psStats->pWaterHitGraphic,psStats->numExplosions,psStats->lightWorld,psStats->effectSize);
				}
			}
			else
			{
				if(psStats->facePlayer)
				{
					addMultiEffect(&position,&scatter,EFFECT_EXPLOSION,EXPLOSION_TYPE_SPECIFIED,TRUE,psStats->pTargetHitGraphic,psStats->numExplosions,psStats->lightWorld,psStats->effectSize);
				}
				else
				{
					addMultiEffect(&position,&scatter,EFFECT_EXPLOSION,EXPLOSION_TYPE_NOT_FACING,TRUE,psStats->pTargetHitGraphic,psStats->numExplosions,psStats->lightWorld,psStats->effectSize);
				}
			}
		}

		/* This was just a simple bullet - release it and return */
		if ( hashTable_RemoveElement( g_pProjObjTable, psObj, (int) psObj, UNUSED_KEY ) == FALSE )
		{
			debug( LOG_NEVER, "proj_ImpactFunc: couldn't remove projectile from table\n" );
		}
		return;
	}

	if (psStats->radius != 0)
	{
		/* An area effect bullet */
		psObj->state = PROJ_POSTIMPACT;

		/* Note when it exploded for the explosion effect */
		psObj->born = gameTime;

	/* Work out the bounding box for the blast radius */
		tarX0 = (SDWORD)psObj->x - (SDWORD)psStats->radius;
		tarY0 = (SDWORD)psObj->y - (SDWORD)psStats->radius;
		tarX1 = (SDWORD)psObj->x + (SDWORD)psStats->radius;
		tarY1 = (SDWORD)psObj->y + (SDWORD)psStats->radius;
		/* Watermelon:height bounding box  for airborne units*/
		tarZ0 = (SDWORD)psObj->z - (SDWORD)psStats->radius;
		tarZ1 = (SDWORD)psObj->z + (SDWORD)psStats->radius;

		/* Store the radius squared */
		radSquared = psStats->radius * psStats->radius * psStats->radius;

		/* Watermelon:air suppression */
		if (psObj->airTarget)
		{
			for (i=0; i<MAX_PLAYERS; i++)
			{
				for (psCurrD = apsDroidLists[i]; psCurrD; psCurrD = psNextD)
				{
					/* have to store the next pointer as psCurrD could be destroyed */
					psNextD = psCurrD->psNext;

					/* Watermelon:skip no vtol droids and landed votl's */
					if (!vtolDroid(psCurrD) ||
						(vtolDroid(psCurrD) && psCurrD->sMove.Status == MOVEINACTIVE))
					{
						continue;
					}

					/* see if psCurrD is hit (don't hit main target twice) */
					if (((BASE_OBJECT *)psCurrD != psObj->psDest) &&
						((SDWORD)psCurrD->x >= tarX0) &&
						((SDWORD)psCurrD->x <= tarX1) &&
						((SDWORD)psCurrD->y >= tarY0) &&
						((SDWORD)psCurrD->y <= tarY1) &&
						((SDWORD)psCurrD->z >= tarZ0) &&
						((SDWORD)psCurrD->z <= tarZ1))
					{
						/* Within the bounding box, now check the radius */
						xDiff = psCurrD->x - psObj->x;
						yDiff = psCurrD->y - psObj->y;
						zDiff = psCurrD->z - psObj->z;
						if ((xDiff*xDiff + yDiff*yDiff + zDiff*zDiff) <= radSquared)
						{
							HIT_ROLL(dice);
							if (dice < weaponRadiusHit(psStats, psObj->player))
							{
								debug(LOG_NEVER, "Damage to object %d, player %d\n",
										psCurrD->id, psCurrD->player);

								damage = calcDamage(
											weaponRadDamage(psStats, psObj->player),
											psStats->weaponEffect, (BASE_OBJECT *)psCurrD);
								if(bMultiPlayer)
								{
									if(psObj->psSource && myResponsibility(psObj->psSource->player))
									{
										updateMultiStatsDamage(psObj->psSource->player,
											psCurrD->player, damage);
									}
									turnOffMultiMsg(TRUE);
								}

								//Watermelon:uses a slightly different check for angle,
								// since fragment of a project is from the explosion spot not from the projectile start position
								if (psObj->altChange > 300)
								{
									impact_angle = HIT_ANGLE_TOP;
								}
								else if (psObj->z < (psCurrD->z - 50))
								{
									impact_angle = HIT_ANGLE_BOTTOM;
								}
								else
								{
									xDiff = psObj->x - psCurrD->x;
									yDiff = psObj->y - psCurrD->y;
									impact_angle = abs(psCurrD->direction - (180 * atan2f((float)xDiff, (float)yDiff) /PI));
									if (impact_angle >= 360)
									{
										impact_angle -= 360;
									}
								}
								bKilled = droidDamage(psCurrD, damage, psStats->weaponClass,psStats->weaponSubClass, impact_angle);

								turnOffMultiMsg(FALSE);	// multiplay msgs back on.

								if(bKilled)
								{
									proj_UpdateKills(psObj);
								}
							}
						}
					}
				}
			}
		}
		else
		{
			/* Do damage to everything in range */
			for (i=0; i<MAX_PLAYERS; i++)
			{
				for (psCurrD = apsDroidLists[i]; psCurrD; psCurrD = psNextD)
				{
					/* have to store the next pointer as psCurrD could be destroyed */
					psNextD = psCurrD->psNext;

					if (vtolDroid(psCurrD) &&
						(psCurrD->sMove.Status != MOVEINACTIVE))
					{
						// skip VTOLs in the air
						continue;
					}

					/* see if psCurrD is hit (don't hit main target twice) */
					if (((BASE_OBJECT *)psCurrD != psObj->psDest) &&
						((SDWORD)psCurrD->x >= tarX0) &&
						((SDWORD)psCurrD->x <= tarX1) &&
						((SDWORD)psCurrD->y >= tarY0) &&
						((SDWORD)psCurrD->y <= tarY1))
					{
						/* Within the bounding box, now check the radius */
						xDiff = psCurrD->x - psObj->x;
						yDiff = psCurrD->y - psObj->y;
						if ((xDiff*xDiff + yDiff*yDiff) <= radSquared)
						{
							HIT_ROLL(dice);
							if (dice < weaponRadiusHit(psStats, psObj->player))
							{
								debug(LOG_NEVER, "Damage to object %d, player %d\n",
										psCurrD->id, psCurrD->player);

								damage = calcDamage(
											weaponRadDamage(psStats, psObj->player),
											psStats->weaponEffect, (BASE_OBJECT *)psCurrD);
								if(bMultiPlayer)
								{
									if(psObj->psSource && myResponsibility(psObj->psSource->player))
									{
										updateMultiStatsDamage(psObj->psSource->player,
											psCurrD->player, damage);
									}
									turnOffMultiMsg(TRUE);
								}

								//Watermelon:uses a slightly different check for angle,
								// since fragment of a project is from the explosion spot not from the projectile start position
								if (psObj->altChange > 300)
								{
									impact_angle = HIT_ANGLE_TOP;
								}
								else if (psObj->z < (psCurrD->z - 50))
								{
									impact_angle = HIT_ANGLE_BOTTOM;
								}
								else
								{
									xDiff = psObj->x - psCurrD->x;
									yDiff = psObj->y - psCurrD->y;
									impact_angle = abs(psCurrD->direction - (180 * atan2f((float)xDiff, (float)yDiff) /PI));
									if (impact_angle >= 360)
									{
										impact_angle -= 360;
									}
								}
								bKilled = droidDamage(psCurrD, damage, psStats->weaponClass,psStats->weaponSubClass, impact_angle);

								turnOffMultiMsg(FALSE);	// multiplay msgs back on.

								if(bKilled)
								{
									proj_UpdateKills(psObj);
								}
							}
						}
					}
				}
				for (psCurrS = apsStructLists[i]; psCurrS; psCurrS = psNextS)
				{
					/* have to store the next pointer as psCurrD could be destroyed */
					psNextS = psCurrS->psNext;

					/* see if psCurrS is hit (don't hit main target twice) */
					if (((BASE_OBJECT *)psCurrS != psObj->psDest) &&
						((SDWORD)psCurrS->x >= tarX0) &&
						((SDWORD)psCurrS->x <= tarX1) &&
						((SDWORD)psCurrS->y >= tarY0) &&
						((SDWORD)psCurrS->y <= tarY1))
					{
						/* Within the bounding box, now check the radius */
						xDiff = psCurrS->x - psObj->x;
						yDiff = psCurrS->y - psObj->y;
						if ((xDiff*xDiff + yDiff*yDiff) <= radSquared)
						{
							HIT_ROLL(dice);
							if (dice < weaponRadiusHit(psStats, psObj->player))
							{
							damage = calcDamage(weaponRadDamage(
						   		psStats, psObj->player),
											psStats->weaponEffect, (BASE_OBJECT *)psCurrS);

								if(bMultiPlayer)
								{
									if(psObj->psSource && myResponsibility(psObj->psSource->player))
									{
										updateMultiStatsDamage(psObj->psSource->player,	psCurrS->player,damage);
									}
								}

								bKilled = structureDamage(psCurrS, damage,
									psStats->weaponClass, psStats->weaponSubClass);

								if(bKilled)
								{
									proj_UpdateKills(psObj);
								}
							}
						}
					}
					// Missed by old method, but maybe in landed within the building's footprint(baseplate)
					else if(ptInStructure(psCurrS,psObj->x,psObj->y) AND (BASE_OBJECT*)psCurrS!=psObj->psDest)
					{
						damage = NOMINAL_DAMAGE;

				  		if(bMultiPlayer)
						{
				   			if(psObj->psSource && myResponsibility(psObj->psSource->player))
							{
				   				updateMultiStatsDamage(psObj->psSource->player,	psCurrS->player,damage);
							}
						}

				  		bKilled = structureDamage(psCurrS, damage,
							psStats->weaponClass, psStats->weaponSubClass);

				   		if(bKilled)
						{
				   			proj_UpdateKills(psObj);
						}
					}
				}
			}
		}

		for (psCurrF = apsFeatureLists[0]; psCurrF; psCurrF = psNextF)
		{
			/* have to store the next pointer as psCurrD could be destroyed */
			psNextF = psCurrF->psNext;

			//ignore features that are not damageable
			if(!psCurrF->psStats->damageable)
			{
				continue;
			}
			/* see if psCurrS is hit (don't hit main target twice) */
			if (((BASE_OBJECT *)psCurrF != psObj->psDest) &&
				((SDWORD)psCurrF->x >= tarX0) &&
				((SDWORD)psCurrF->x <= tarX1) &&
				((SDWORD)psCurrF->y >= tarY0) &&
				((SDWORD)psCurrF->y <= tarY1))
			{
				/* Within the bounding box, now check the radius */
				xDiff = psCurrF->x - psObj->x;
				yDiff = psCurrF->y - psObj->y;
				if ((xDiff*xDiff + yDiff*yDiff) <= radSquared)
				{
					HIT_ROLL(dice);
					if (dice < weaponRadiusHit(psStats, psObj->player))
					{
						debug(LOG_NEVER, "Damage to object %d, player %d\n",
								psCurrF->id, psCurrF->player);

						bKilled = featureDamage(psCurrF, calcDamage(weaponRadDamage(
							psStats, psObj->player), psStats->weaponEffect,
							(BASE_OBJECT *)psCurrF), psStats->weaponSubClass);
						if(bKilled)
						{
							proj_UpdateKills(psObj);
						}
					}
				}
			}
		}
	}

	if (psStats->incenTime != 0)
	{
		/* Incendiary round */
		/* Incendiary damage gets done in the bullet update routine */
		/* Just note when the incendiary started burning            */
		psObj->state = PROJ_POSTIMPACT;
		psObj->born = gameTime;
	}
	/* Something was blown up */
}

/***************************************************************************/

void
proj_PostImpactFunc( PROJ_OBJECT *psObj )
{
	WEAPON_STATS	*psStats;
	SDWORD			i, age;
	FIRE_BOX		flame;

	ASSERT( PTRVALID(psObj, sizeof(PROJ_OBJECT)),
		"proj_PostImpactFunc: invalid projectile pointer" );

	psStats = psObj->psWStats;
	ASSERT( PTRVALID(psStats, sizeof(WEAPON_STATS)),
		"proj_PostImpactFunc: Invalid weapon stats pointer" );

	age = (SDWORD)gameTime - (SDWORD)psObj->born;

	/* Time to finish postimpact effect? */
	if (age > (SDWORD)psStats->radiusLife && age > (SDWORD)psStats->incenTime)
	{
		if ( hashTable_RemoveElement( g_pProjObjTable, psObj,
									(int) psObj, UNUSED_KEY ) == FALSE )
		{
			debug( LOG_NEVER, "proj_PostImpactFunc: couldn't remove projectile from table\n" );
		}
		return;
	}

	/* Burning effect */
	if (psStats->incenTime > 0)
	{
		/* See if anything is in the fire and burn it */

		/* Calculate the fire's bounding box */
		flame.x1 = (SWORD)(psObj->x - psStats->incenRadius);
		flame.y1 = (SWORD)(psObj->y - psStats->incenRadius);
		flame.x2 = (SWORD)(psObj->x + psStats->incenRadius);
		flame.y2 = (SWORD)(psObj->y + psStats->incenRadius);
		flame.rad = (SWORD)(psStats->incenRadius*psStats->incenRadius);

		for (i=0; i<MAX_PLAYERS; i++)
		{
			/* Don't damage your own droids - unrealistic, but better */
			if(i!=psObj->player)
			{
				proj_checkBurnDamage((BASE_OBJECT*)apsDroidLists[i], psObj, &flame);
				proj_checkBurnDamage((BASE_OBJECT*)apsStructLists[i], psObj, &flame);
			}
		}
	}
}

/***************************************************************************/

static void
proj_Update( PROJ_OBJECT *psObj )
{
	ASSERT( PTRVALID(psObj, sizeof(PROJ_OBJECT)),
		"proj_Update: Invalid bullet pointer" );

	/* See if any of the stored objects have died
	 * since the projectile was created
	 */
	if (psObj->psSource && psObj->psSource->died)
	{
		psObj->psSource = NULL;
	}
	if (psObj->psDest && psObj->psDest->died)
	{
		psObj->psDest = NULL;
	}

	//Watermelon:get naybors
	//if(psObj->psWStats->weaponSubClass == WSC_ROCKET OR psObj->psWStats->weaponSubClass == WSC_MISSILE OR
		//psObj->psWStats->weaponSubClass == WSC_SLOWROCKET OR psObj->psWStats->weaponSubClass == WSC_SLOWMISSILE)
	//{
		projGetNaybors((PROJ_OBJECT *)psObj);
	//}

	switch (psObj->state)
	{
		case PROJ_INFLIGHT:
			(psObj->pInFlightFunc) ( psObj );
			break;

		case PROJ_IMPACT:
			proj_ImpactFunc( psObj );
			break;

		case PROJ_POSTIMPACT:
			proj_PostImpactFunc( psObj );
			break;
	}
}

/***************************************************************************/

void
proj_UpdateAll( void )
{
	PROJ_OBJECT	*psObj;

	psObj = (PROJ_OBJECT *) hashTable_GetFirst( g_pProjObjTable );

	while ( psObj != NULL )
	{
		proj_Update( psObj );

		psObj = (PROJ_OBJECT *) hashTable_GetNext( g_pProjObjTable );
	}
}

/***************************************************************************/

void
proj_checkBurnDamage( BASE_OBJECT *apsList, PROJ_OBJECT *psProj,
						FIRE_BOX *pFireBox )
{
	BASE_OBJECT		*psCurr, *psNext;
	SDWORD			xDiff,yDiff;
	WEAPON_STATS	*psStats;
	UDWORD			damageSoFar;
	SDWORD			damageToDo;
	BOOL			bKilled;
//	BOOL			bMultiTemp;

	// note the attacker if any
	g_pProjLastAttacker = psProj->psSource;

	psStats = psProj->psWStats;
	for (psCurr = apsList; psCurr; psCurr = psNext)
	{
		/* have to store the next pointer as psCurr could be destroyed */
		psNext = psCurr->psNext;

		if ((psCurr->type == OBJ_DROID) &&
			vtolDroid((DROID*)psCurr) &&
			((DROID *)psCurr)->sMove.Status != MOVEINACTIVE)
		{
			// can't set flying vtols on fire
			continue;
		}

		/* see if psCurr is hit (don't hit main target twice) */
		if (((SDWORD)psCurr->x >= pFireBox->x1) &&
			((SDWORD)psCurr->x <= pFireBox->x2) &&
			((SDWORD)psCurr->y >= pFireBox->y1) &&
			((SDWORD)psCurr->y <= pFireBox->y2))
		{
			/* Within the bounding box, now check the radius */
			xDiff = psCurr->x - psProj->x;
			yDiff = psCurr->y - psProj->y;
			if ((xDiff*xDiff + yDiff*yDiff) <= pFireBox->rad)
			{
				/* The object is in the fire */
				psCurr->inFire |= IN_FIRE;

				if ( (psCurr->burnStart == 0) ||
					 (psCurr->inFire & BURNING) )
				{
					/* This is the first turn the object is in the fire */
					psCurr->burnStart = gameTime;
					psCurr->burnDamage = 0;
				}
				else
				{
					/* Calculate how much damage should have
					   been done up till now */
					damageSoFar = (gameTime - psCurr->burnStart)
								  //* psStats->incenDamage
								  * weaponIncenDamage(psStats,psProj->player)
								  / GAME_TICKS_PER_SEC;
					damageToDo = (SDWORD)damageSoFar
								 - (SDWORD)psCurr->burnDamage;
					if (damageToDo > 0)
					{
						debug(LOG_NEVER, "Burn damage of %d to object %d, player %d\n",
								damageToDo, psCurr->id, psCurr->player);

//						if(bMultiPlayer)
//						{
//							bMultiPlayer = FALSE;
//							bMultiTemp = TRUE;
//						}
//						else
//						{
//							bMultiTemp = FALSE;
//						}

						//bKilled = psCurr->damage(psCurr, damageToDo, psStats->weaponClass);

						//Watermelon:just assume the burn damage is from FRONT
	  					bKilled = objectDamage(psCurr, damageToDo, psStats->weaponClass,psStats->weaponSubClass, 0);

						psCurr->burnDamage += damageToDo;

//						if(bMultiTemp)
//						{
//							bMultiPlayer = TRUE;
//						}

						if(bKilled)
						{
							proj_UpdateKills(psProj);
						}
					}
					/* The damage could be negative if the object
					   is being burn't by another fire
					   with a higher burn damage */
				}
			}
		}
	}
}

/***************************************************************************/

// return whether a weapon is direct or indirect
BOOL proj_Direct(WEAPON_STATS *psStats)
{
	switch (psStats->movementModel)
	{
	case MM_DIRECT:
	case MM_HOMINGDIRECT:
	case MM_ERRATICDIRECT:
	case MM_SWEEP:
		return TRUE;
		break;
	case MM_INDIRECT:
	case MM_HOMINGINDIRECT:
		return FALSE;
		break;
	default:
		ASSERT( FALSE,"proj_Direct: unknown movement model" );
		break;
	}

	return TRUE;
}

/***************************************************************************/

// return the maximum range for a weapon
SDWORD proj_GetLongRange(WEAPON_STATS *psStats, SDWORD dz)
{
//	dz;
	return (SDWORD)psStats->longRange;
}

#ifndef WIN32
#define max(a,b) ((a)>(b)) ? (a) :(b)
#endif

/***************************************************************************/
//Watemelon:added case for OBJ_BULLET
UDWORD	establishTargetRadius( BASE_OBJECT *psTarget )
{
UDWORD		radius;
STRUCTURE	*psStructure;
FEATURE		*psFeat;
//Watermelon:droid pointer

	radius = 0;

	if(psTarget == NULL)	// Can this happen?
	{
		return( 0 );
	}
	else
	{
		switch(psTarget->type)
		{
		case OBJ_DROID:
			switch(((DROID *)psTarget)->droidType)
			{
				case DROID_WEAPON:
				case DROID_SENSOR:
				case DROID_ECM:
				case DROID_CONSTRUCT:
				case DROID_COMMAND:
				case DROID_REPAIR:
				case DROID_PERSON:
				case DROID_CYBORG:
				case DROID_CYBORG_CONSTRUCT:
				case DROID_CYBORG_REPAIR:
				case DROID_CYBORG_SUPER:
					//Watermelon:'hitbox' size is now based on imd size
					radius = abs(psTarget->sDisplay.imd->radius);
					break;
				case DROID_DEFAULT:
				case DROID_TRANSPORTER:
				default:
					radius = TILE_UNITS/4;	// how will we arrive at this?
			}
			break;
		case OBJ_STRUCTURE:
			psStructure = (STRUCTURE*)psTarget;
			radius = ((max(psStructure->pStructureType->baseBreadth,psStructure->pStructureType->baseWidth)) * TILE_UNITS)/2;
			break;
		case OBJ_FEATURE:
//			radius = TILE_UNITS/4;	// how will we arrive at this?
			psFeat = (FEATURE *)psTarget;
			radius = ((max(psFeat->psStats->baseBreadth,psFeat->psStats->baseWidth)) * TILE_UNITS)/2;
			break;
		case OBJ_BULLET:
			//Watermelon 1/2 radius of a droid?
			radius = TILE_UNITS/8;
		default:
			break;
		}
	}

	return(radius);
}
/***************************************************************************/

/*the damage depends on the weapon effect and the target propulsion type or
structure strength*/
UDWORD	calcDamage(UDWORD baseDamage, WEAPON_EFFECT weaponEffect, BASE_OBJECT *psTarget)
{
	UDWORD	damage;

	//default value
	damage = baseDamage;

	if (psTarget->type == OBJ_STRUCTURE)
	{
		damage = baseDamage * asStructStrengthModifier[weaponEffect][((
			STRUCTURE *)psTarget)->pStructureType->strength] / 100;

        //a little fail safe!
        if (damage == 0 AND baseDamage != 0)
        {
            damage = 1;
        }

#if(0)
	{
		UDWORD Mod;
		UDWORD PropType=  (( STRUCTURE *)psTarget)->pStructureType->strength;
		UDWORD damage1;

		Mod=asStructStrengthModifier[weaponEffect][PropType];

		damage1 = baseDamage * Mod / 100;


//	my_error("",0,"","STRUCT damage1=%d damage=%d baseDamage=%d mod=%d (weaponEffect=%d proptype=%d) \n",damage1,damage,baseDamage,Mod,weaponEffect,PropType);
	}
#endif


	}
	else if (psTarget->type == OBJ_DROID)
	{

		damage = baseDamage * asWeaponModifier[weaponEffect][(
   			asPropulsionStats + ((DROID *)psTarget)->asBits[COMP_PROPULSION].
			nStat)->propulsionType] / 100;

        //a little fail safe!
        if (damage == 0 AND baseDamage != 0)
        {
            damage = 1;
        }

#if(0)
	{
		UDWORD Mod;
		UDWORD PropType=	  (asPropulsionStats + ((DROID *)psTarget)->asBits[COMP_PROPULSION].nStat)->propulsionType;
		UDWORD damage1;

		Mod=asWeaponModifier[weaponEffect][PropType];

		damage1 = baseDamage * Mod / 100;


		debug( LOG_NEVER, "damage1=%d damage=%d baseDamage=%d mod=%d (weaponEffect=%d proptype=%d) \n", damage1, damage, baseDamage, Mod, weaponEffect, PropType );
	}
#endif



	}



	return damage;
}


BOOL objectDamage(BASE_OBJECT *psObj, UDWORD damage, UDWORD weaponClass,UDWORD weaponSubClass, int angle)
{
	switch (psObj->type)
	{
		case OBJ_DROID:
			return droidDamage((DROID *)psObj, damage, weaponClass,weaponSubClass, angle);
			break;
		case OBJ_STRUCTURE:
			return structureDamage((STRUCTURE *)psObj, damage, weaponClass, weaponSubClass);
			break;
		case OBJ_FEATURE:
			return featureDamage((FEATURE *)psObj, damage, weaponSubClass);
			break;
		default:
			ASSERT( FALSE, "objectDamage - unknown object type" );
	}
	return FALSE;
}



#define HIT_THRESHOLD	(GAME_TICKS_PER_SEC/6)	// got to be over 5 frames per sec.
/* Returns true if an object has just been hit by an electronic warfare weapon*/
BOOL	justBeenHitByEW( BASE_OBJECT *psObj )
{
DROID		*psDroid;
FEATURE		*psFeature;
STRUCTURE	*psStructure;

	if(gamePaused())
	{
		return(FALSE);	// Don't shake when paused...!
	}

	switch(psObj->type)
	{
	case OBJ_DROID:
		psDroid = (DROID*)psObj;
		if( (gameTime - psDroid->timeLastHit) < HIT_THRESHOLD AND
            psDroid->lastHitWeapon == WSC_ELECTRONIC)
			return(TRUE);
	case OBJ_FEATURE:
		psFeature = (FEATURE*)psObj;
		if( (gameTime - psFeature->timeLastHit) < HIT_THRESHOLD )
			return(TRUE);
		break;
	case OBJ_STRUCTURE:
		psStructure = (STRUCTURE*)psObj;
		if( (gameTime - psStructure->timeLastHit) < HIT_THRESHOLD AND
            psStructure->lastHitWeapon == WSC_ELECTRONIC)
			return(TRUE);
		break;
	default:
		debug( LOG_ERROR, "Weird object type in justBeenHitByEW" );
		abort();
		break;
	}

	return(FALSE);
}

void	objectShimmy(BASE_OBJECT *psObj)
{
	if(justBeenHitByEW(psObj))
	{
  		iV_MatrixRotateX(SKY_SHIMMY);
 		iV_MatrixRotateY(SKY_SHIMMY);
 		iV_MatrixRotateZ(SKY_SHIMMY);
		if(psObj->type == OBJ_DROID)
		{
			iV_TRANSLATE(1-rand()%3,0,1-rand()%3);
		}
	}
}

// Watermelon:addProjNaybor ripped from droid.c
/* Add a new object to the projectile naybor list */
static void addProjNaybor(BASE_OBJECT *psObj, UDWORD distSqr)
{
	UDWORD	pos;

	if (numProjNaybors >= MAX_NAYBORS)
	{
//		DBPRINTF(("Naybor list maxed out for id %d\n", psObj->id));
		return;
	}
	else if (numProjNaybors == 0)
	{
		// No objects in the list
		asProjNaybors[0].psObj = psObj;
		asProjNaybors[0].distSqr = distSqr;
		numProjNaybors++;
	}
	else if (distSqr >= asProjNaybors[numProjNaybors-1].distSqr)
	{
		// Simple case - this is the most distant object
		asProjNaybors[numProjNaybors].psObj = psObj;
		asProjNaybors[numProjNaybors].distSqr = distSqr;
		numProjNaybors++;
	}
	else
	{
		// Move all the objects further away up the list
		pos = numProjNaybors;
		while (pos > 0 && asProjNaybors[pos - 1].distSqr > distSqr)
		{
			memcpy(asProjNaybors + pos, asProjNaybors + (pos - 1), sizeof(PROJ_NAYBOR_INFO));
			pos --;
		}

		// Insert the object at the correct position
		asProjNaybors[pos].psObj = psObj;
		asProjNaybors[pos].distSqr = distSqr;
		numProjNaybors++;
	}

	ASSERT( numProjNaybors <= MAX_NAYBORS,
		"addNaybor: numNaybors > MAX_NAYBORS" );
}

//Watermelon: projGetNaybors ripped from droid.c
/* Find all the objects close to the projectile */
void projGetNaybors(PROJ_OBJECT *psObj)
{
//	DROID		*psCurrD;
//	STRUCTURE	*psCurrS;
//	FEATURE		*psCurrF;
	SDWORD		xdiff, ydiff;
//	UDWORD		player;
	UDWORD		dx,dy, distSqr;
	//Watermelon:renamed to psTempObj from psObj
	BASE_OBJECT	*psTempObj;

// Ensure only called max of once per droid per game cycle.
	if(CurrentProjNaybors == (BASE_OBJECT *)psObj && projnayborTime == gameTime) {
		return;
	}
	CurrentProjNaybors = (BASE_OBJECT *)psObj;
	projnayborTime = gameTime;



	// reset the naybor array
	numProjNaybors = 0;
#ifdef DEBUG
	memset(asProjNaybors, 0xcd, sizeof(asProjNaybors));
#endif

	// search for naybor objects
	dx = ((BASE_OBJECT *)psObj)->x;
	dy = ((BASE_OBJECT *)psObj)->y;

	gridStartIterate((SDWORD)dx, (SDWORD)dy);
	for (psTempObj = gridIterate(); psTempObj != NULL; psTempObj = gridIterate())
	{
		if (psTempObj != (BASE_OBJECT *)psObj)
		{
			IN_PROJ_NAYBOR_RANGE(psTempObj);

			addProjNaybor(psTempObj, distSqr);
		}
	}
}

UDWORD	establishTargetHeight( BASE_OBJECT *psTarget )
{
	UDWORD		height;
	DROID		*psDroid;

	if(psTarget == NULL)	// Can this happen?
	{
		return( 0 );
	}
	else
	{
		//Get absolute ymax+ymin
		height = abs(psTarget->sDisplay.imd->ymax) + abs(psTarget->sDisplay.imd->ymin);

		switch(psTarget->type)
		{
		case OBJ_DROID:
			psDroid = (DROID*)psTarget;
			// Don't do this for Barbarian Propulsions as they don't possess a turret (and thus have pIMD == NULL)
			if (!strcmp(asPropulsionStats[psDroid->asBits[COMP_PROPULSION].nStat].pName, "BaBaProp") )
			{
				return (height);
			}
			switch(psDroid->droidType)
			{
				case DROID_WEAPON:
					if ( psDroid->numWeaps > 0 )
					{
						height += abs((asWeaponStats[psDroid->asWeaps[0].nStat]).pIMD->ymax);
						height += abs((asWeaponStats[psDroid->asWeaps[0].nStat]).pIMD->ymin);
					}
					break;
				case DROID_SENSOR:
					height += abs((asSensorStats[psDroid->asBits[COMP_SENSOR].nStat]).pIMD->ymax);
					height += abs((asSensorStats[psDroid->asBits[COMP_SENSOR].nStat]).pIMD->ymin);
					break;
				case DROID_ECM:
					height += abs((asECMStats[psDroid->asBits[COMP_ECM].nStat]).pIMD->ymax);
					height += abs((asECMStats[psDroid->asBits[COMP_ECM].nStat]).pIMD->ymin);
					break;
				case DROID_CONSTRUCT:
					height += abs((asConstructStats[psDroid->asBits[COMP_CONSTRUCT].nStat]).pIMD->ymax);
					height += abs((asConstructStats[psDroid->asBits[COMP_CONSTRUCT].nStat]).pIMD->ymin);
					break;
				case DROID_COMMAND:
					height += abs((asBrainStats[psDroid->asBits[COMP_BRAIN].nStat]).pIMD->ymax);
					height += abs((asBrainStats[psDroid->asBits[COMP_BRAIN].nStat]).pIMD->ymin);
					break;
				case DROID_REPAIR:
					height += abs((asRepairStats[psDroid->asBits[COMP_REPAIRUNIT].nStat]).pIMD->ymax);
					height += abs((asRepairStats[psDroid->asBits[COMP_REPAIRUNIT].nStat]).pIMD->ymin);
					break;
				case DROID_PERSON:
					//TODO:add person 'state'checks here(stand, knee, crouch, prone etc)
				case DROID_CYBORG:
				case DROID_CYBORG_CONSTRUCT:
				case DROID_CYBORG_REPAIR:
				case DROID_CYBORG_SUPER:
				case DROID_DEFAULT:
				case DROID_TRANSPORTER:
				default:
					break;
			}
			break;
		case OBJ_STRUCTURE:
		case OBJ_FEATURE:
			//Just use imd ymax+ymin,since HeightScale is not available atm
			break;
		case OBJ_BULLET:
			//16 for bullet
			height = 16;
			break;
		default:
			break;
		}
	}

	return (height);
}
