/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/***************************************************************************/
/*
 * Projectile functions
 *
 */
/***************************************************************************/
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/trig.h"
#include "lib/framework/math-help.h"

#include "lib/gamelib/gtime.h"
#include "objects.h"
#include "move.h"
#include "action.h"
#include "combat.h"
#include "effects.h"
#include "map.h"
#include "lib/sound/audio_id.h"
#include "lib/sound/audio.h"
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
#include "mapgrid.h"

#define	PROJ_MAX_PITCH			30
#define	ACC_GRAVITY				1000
#define	DIRECT_PROJ_SPEED		500
#define NOMINAL_DAMAGE	5
#define VTOL_HITBOX_MODIFICATOR 100

/** Used for passing data to the checkBurnDamage function */
typedef struct
{
	SWORD   x1, y1;
	SWORD   x2, y2;
	SWORD   rad;
} FIRE_BOX;

// Watermelon:they are from droid.c
/* The range for neighbouring objects */
#define PROJ_NAYBOR_RANGE		(TILE_UNITS*4)

// Watermelon:neighbour global info ripped from droid.c
static PROJ_NAYBOR_INFO	asProjNaybors[MAX_NAYBORS];
static UDWORD		numProjNaybors = 0;

static BASE_OBJECT	*CurrentProjNaybors = NULL;
static UDWORD	projnayborTime = 0;

/* The list of projectiles in play */
static PROJECTILE *psProjectileList = NULL;

/* The next projectile to give out in the proj_First / proj_Next methods */
static PROJECTILE *psProjectileNext = NULL;

/***************************************************************************/

// the last unit that did damage - used by script functions
BASE_OBJECT		*g_pProjLastAttacker;

extern BOOL	godMode;

/***************************************************************************/

static UDWORD	establishTargetRadius( BASE_OBJECT *psTarget );
static UDWORD	establishTargetHeight( BASE_OBJECT *psTarget );
static void	proj_InFlightDirectFunc( PROJECTILE *psObj );
static void	proj_InFlightIndirectFunc( PROJECTILE *psObj );
static void	proj_ImpactFunc( PROJECTILE *psObj );
static void	proj_PostImpactFunc( PROJECTILE *psObj );
static void	proj_checkBurnDamage( BASE_OBJECT *apsList, PROJECTILE *psProj,
									FIRE_BOX *pFireBox );
static void	proj_Free(PROJECTILE *psObj);

static float objectDamage(BASE_OBJECT *psObj, UDWORD damage, UDWORD weaponClass,UDWORD weaponSubClass, HIT_SIDE impactSide);
static HIT_SIDE getHitSide (PROJECTILE *psObj, BASE_OBJECT *psTarget);

static void projGetNaybors(PROJECTILE *psObj);


/***************************************************************************/
BOOL gfxVisible(PROJECTILE *psObj)
{
	BOOL bVisible = false;

	// Already know it is visible
	if (psObj->bVisible)
	{
		return true;
	}

	// You fired it
	if (psObj->player == selectedPlayer)
	{
		return true;
	}

	// Always see in this mode
	if (godMode)
	{
		return true;
	}

	// You can see the source
	if (psObj->psSource != NULL
	 && !psObj->psSource->died
	 && psObj->psSource->visible[selectedPlayer])
	{
		bVisible = true;
	}

	// You can see the destination
	if (psObj->psDest != NULL
	 && !psObj->psDest->died
	 && psObj->psDest->visible[selectedPlayer])
	{
		bVisible = true;
	}

	// Someone elses structure firing at something you can't see
	if (psObj->psSource != NULL
	 && !psObj->psSource->died
	 && psObj->psSource->type == OBJ_STRUCTURE
	 && psObj->psSource->player != selectedPlayer
	 && (psObj->psDest == NULL
	  || psObj->psDest->died
	  || !psObj->psDest->visible[selectedPlayer]))
	{
		bVisible = false;
	}

	// Something you cannot see firing at a structure that isn't yours
	if (psObj->psDest != NULL
	 && !psObj->psDest->died
	 && psObj->psDest->type == OBJ_STRUCTURE
	 && psObj->psDest->player != selectedPlayer
	 && (psObj->psSource == NULL
	  || !psObj->psSource->visible[selectedPlayer]))
	{
		bVisible = false;
	}

	return bVisible;
}

/***************************************************************************/

BOOL
proj_InitSystem( void )
{
	psProjectileList = NULL;
	psProjectileNext = NULL;

	return true;
}

/***************************************************************************/

// Clean out all projectiles from the system, and properly decrement
// all reference counts.
void
proj_FreeAllProjectiles( void )
{
	PROJECTILE *psCurr = psProjectileList, *psPrev = NULL;

	while (psCurr)
	{
		psPrev = psCurr;
		psCurr = psCurr->psNext;
		proj_Free(psPrev);
	}

	psProjectileList = NULL;
	psProjectileNext = NULL;
}

/***************************************************************************/

BOOL
proj_Shutdown( void )
{
	proj_FreeAllProjectiles();

	return true;
}

/***************************************************************************/

// Free the memory held by a projectile, and decrement its reference counts,
// if any. Do not call directly on a projectile in a list, because then the
// list will be broken!
static void proj_Free(PROJECTILE *psObj)
{
	/* Decrement any reference counts the projectile may have increased */
	setProjectileDamaged(psObj, NULL);
	setProjectileSource(psObj, NULL);
	setProjectileDestination(psObj, NULL);

	free(psObj);
}

/***************************************************************************/

// Reset the first/next methods, and give out the first projectile in the list.
PROJECTILE *
proj_GetFirst( void )
{
	psProjectileNext = psProjectileList;
	return psProjectileList;
}

/***************************************************************************/

// Get the next projectile
PROJECTILE *
proj_GetNext( void )
{
	psProjectileNext = psProjectileNext->psNext;
	return psProjectileNext;
}

/***************************************************************************/

/*
 * Relates the quality of the attacker to the quality of the victim.
 * The value returned satisfies the following inequality: 0.5 <= ret <= 2.0
 */
static float QualityFactor(DROID *psAttacker, DROID *psVictim)
{
	float powerRatio = calcDroidPower(psVictim) / calcDroidPower(psAttacker);
	float pointsRatio = calcDroidPoints(psVictim) / calcDroidPoints(psAttacker);

	CLIP(powerRatio, 0.5, 2.0);
	CLIP(pointsRatio, 0.5, 2.0);

	return (powerRatio + pointsRatio) / 2;
}

// update the kills after a target is damaged/destroyed
static void proj_UpdateKills(PROJECTILE *psObj, float experienceInc)
{
	DROID	        *psDroid;
	BASE_OBJECT     *psSensor;

	CHECK_PROJECTILE(psObj);

	if ((psObj->psSource == NULL) ||
		((psObj->psDest != NULL) && (psObj->psDest->type == OBJ_FEATURE)))
	{
		return;
	}

	// If experienceInc is negative then the target was killed
	if (bMultiPlayer && experienceInc < 0.0f)
	{
		updateMultiStatsKills(psObj->psDest, psObj->psSource->player);
	}

	// Since we are no longer interested if it was killed or not, abs it
	experienceInc = fabs(experienceInc);

	if (psObj->psSource->type == OBJ_DROID)			/* update droid kills */
	{
		psDroid = (DROID *) psObj->psSource;

		// If it is 'droid-on-droid' then modify the experience by the Quality factor
		// Only do this in MP so to not un-balance the campaign
		if (psObj->psDest != NULL
		 && psObj->psDest->type == OBJ_DROID
		 && bMultiPlayer)
		{
			// Modify the experience gained by the 'quality factor' of the units
			experienceInc *= QualityFactor(psDroid, (DROID *) psObj->psDest);
		}

		psDroid->experience += experienceInc;
		cmdDroidUpdateKills(psDroid, experienceInc);

		psSensor = orderStateObj(psDroid, DORDER_FIRESUPPORT);
		if (psSensor
		 && psSensor->type == OBJ_DROID)
		{
		    ((DROID *) psSensor)->experience += experienceInc;
		}
	}
	else if (psObj->psSource->type == OBJ_STRUCTURE)
	{
		// See if there was a command droid designating this target
		psDroid = cmdDroidGetDesignator(psObj->psSource->player);

		if (psDroid != NULL
		 && psDroid->action == DACTION_ATTACK
		 && psDroid->psActionTarget[0] == psObj->psDest)
		{
			psDroid->experience += experienceInc;
		}
	}
}

/***************************************************************************/

BOOL proj_SendProjectile(WEAPON *psWeap, BASE_OBJECT *psAttacker, int player, Vector3i target, BASE_OBJECT *psTarget, BOOL bVisible, BOOL bPenetrate, int weapon_slot)
{
	PROJECTILE		*psObj = malloc(sizeof(PROJECTILE));
	SDWORD			tarHeight, srcHeight, iMinSq;
	SDWORD			altChange, dx, dy, dz, iVelSq, iVel;
	double          fR, fA, fS, fT, fC;
	Vector3i muzzle;
	SDWORD			iRadSq, iPitchLow, iPitchHigh, iTemp;
	UDWORD			heightVariance;
	WEAPON_STATS	*psWeapStats = &asWeaponStats[psWeap->nStat];

	ASSERT( psWeapStats != NULL,
			"proj_SendProjectile: invalid weapon stats" );

	/* get muzzle offset */
	if (psAttacker == NULL)
	{
		// if there isn't an attacker just start at the target position
		// NB this is for the script function to fire the las sats
		muzzle = target;
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
		muzzle.x = psAttacker->pos.x;
		muzzle.y = psAttacker->pos.y;
		muzzle.z = psAttacker->pos.z;
	}

	/* Initialise the structure */
	psObj->type		    = OBJ_PROJECTILE;
	psObj->state		= PROJ_INFLIGHT;
	psObj->psWStats		= asWeaponStats + psWeap->nStat;
	Vector3uw_Set(&psObj->pos, muzzle.x, muzzle.y, muzzle.z);
	psObj->startX		= muzzle.x;
	psObj->startY		= muzzle.y;
	psObj->tarX			= target.x;
	psObj->tarY			= target.y;
	psObj->targetRadius = (psTarget ? establishTargetRadius(psTarget) : 0); // needed to backtrack FX
	psObj->born			= gameTime;
	psObj->player		= (UBYTE)player;
	psObj->bVisible		= false;
	psObj->airTarget	= false;
	psObj->psDamaged	= NULL; // must initialize these to NULL first!
	psObj->psSource		= NULL;
	psObj->psDest		= NULL;
	psObj->died		= 0;
	setProjectileDestination(psObj, psTarget);

	ASSERT(!psTarget || !psTarget->died, "Aiming at dead target!");

	/* If target is a VTOL or higher than ground, it is an air target. */
	if ((psTarget != NULL && psTarget->type == OBJ_DROID && vtolDroid((DROID*)psTarget))
	    || (psTarget == NULL && target.z > map_Height(target.x, target.y)))
	{
		psObj->airTarget = true;
	}

	//Watermelon:use the source of the source of psObj :) (psAttacker is a projectile)
	if (bPenetrate && psAttacker)
	{
		// psAttacker is a projectile if bPenetrate
		PROJECTILE *psProj = (PROJECTILE*)psAttacker;

		ASSERT(psProj->type == OBJ_PROJECTILE, "Penetrating but not projectile?");

		if (psProj->psSource && !psProj->psSource->died)
		{
			setProjectileSource(psObj, psProj->psSource);
		}

		if (psProj->psDest && !psProj->psDest->died)
		{
			setProjectileDamaged(psObj, psProj->psDest);
		}
		psProj->state = PROJ_IMPACT;
	}
	else
	{
		setProjectileSource(psObj, psAttacker);
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

			case OBJ_PROJECTILE:
				ASSERT(!"invalid object type: bullet", "proj_SendProjectile: invalid object type: OBJ_PROJECTILE");
				break;

			case OBJ_TARGET:
				ASSERT(!"invalid object type: target", "proj_SendProjectile: invalid object type: OBJ_TARGET");
				break;

			default:
				ASSERT(!"unknown object type", "proj_SendProjectile: unknown object type");
				break;
		}
		tarHeight = psTarget->pos.z + heightVariance;
	}
	else
	{
		tarHeight = target.z;
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

	fR = (double) atan2(dx, dy);
	if ( fR < 0.0 )
	{
		fR += (double) (2 * M_PI);
	}
	psObj->direction = RAD_TO_DEG(fR);


	/* get target distance */
	iRadSq = dx*dx + dy*dy + dz*dz;
	fR = trigIntSqrt( iRadSq );
	iMinSq = (SDWORD)(psWeapStats->minRange * psWeapStats->minRange);

	if ( proj_Direct(psObj->psWStats) ||
		 ( !proj_Direct(psWeapStats) && (iRadSq <= iMinSq) ) )
	{
		fR = (double) atan2(dz, fR);
		if ( fR < 0.0 )
		{
			fR += (double) (2 * M_PI);
		}
		psObj->pitch = (SWORD)( RAD_TO_DEG(fR) );
		psObj->pInFlightFunc = proj_InFlightDirectFunc;
	}
	else
	{
		/* indirect */
		iVelSq = psObj->psWStats->flightSpeed * psObj->psWStats->flightSpeed;

 		fA = ACC_GRAVITY * (double)iRadSq / (2 * iVelSq);
		fC = 4 * fA * ((double)dz + fA);
		fS = (double)iRadSq - fC;

		/* target out of range - increase velocity to hit target */
		if ( fS < 0. )
		{
			/* set optimal pitch */
			psObj->pitch = PROJ_MAX_PITCH;

			fS = (double)trigSin(PROJ_MAX_PITCH);
			fC = (double)trigCos(PROJ_MAX_PITCH);
			fT = fS / fC;
			fS = ACC_GRAVITY * (1. + fT * fT);
			fS = fS / (2 * (fR * fT - (double)dz));
			{
				double Tmp = fR * fR;
				iVel = trigIntSqrt(fS * Tmp);
			}
		}
		else
		{
			/* set velocity to stats value */
			iVel = psObj->psWStats->flightSpeed;

			/* get floating point square root */
			fS = trigIntSqrt(fS);

			fT = (double) atan2(fR + fS, 2 * fA);

			/* make sure angle positive */
			if ( fT < 0 )
			{
				fT += (double) (2 * M_PI);
			}
			iPitchLow = RAD_TO_DEG(fT);

			fT = (double) atan2(fR-fS, 2*fA);
			/* make sure angle positive */
			if ( fT < 0 )
			{
				fT += (double) (2 * M_PI);
			}
			iPitchHigh = RAD_TO_DEG(fT);

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

		psObj->vXY = iVel * trigCos(psObj->pitch);
		psObj->vZ  = iVel * trigSin(psObj->pitch);

		/* set function pointer */
		psObj->pInFlightFunc = proj_InFlightIndirectFunc;
	}

	/* put the projectile object first in the global list */
	psObj->psNext = psProjectileList;
	psProjectileList = psObj;

	/* play firing audio */
	// only play if either object is visible, i know it's a bit of a hack, but it avoids the problem
	// of having to calculate real visibility values for each projectile.
	if ( bVisible || gfxVisible(psObj) )
	{
		// note that the projectile is visible
		psObj->bVisible = true;

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
            else if (!(bMultiPlayer && psWeapStats->weaponSubClass == WSC_LAS_SAT))
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

	CHECK_PROJECTILE(psObj);

	return true;
}

/***************************************************************************/

static void proj_InFlightDirectFunc( PROJECTILE *psProj )
{
	/* we want a delay between Las-Sats firing and actually hitting in multiPlayer
	magic number but that's how long the audio countdown message lasts! */
	const unsigned int LAS_SAT_DELAY = 8;
	unsigned int timeSoFar;
	unsigned int distancePercent; /* How far we are 0..100 */
	float distanceRatio; /* How far we are, 1.0==at target */
	float distanceExtensionFactor; /* Extended lifespan */
	Vector3i move;
	unsigned int i;
	// Projectile is missile:
	bool bMissile = false;
	WEAPON_STATS *psStats;

	CHECK_PROJECTILE(psProj);

	psStats = psProj->psWStats;
	ASSERT( psStats != NULL,
		"proj_InFlightDirectFunc: Invalid weapon stats pointer" );

	timeSoFar = gameTime - psProj->born;

	/* we want a delay between Las-Sats firing and actually hitting in multiPlayer
	magic number but that's how long the audio countdown message lasts! */
    if ( bMultiPlayer && psStats->weaponSubClass == WSC_LAS_SAT &&
		timeSoFar < LAS_SAT_DELAY * GAME_TICKS_PER_SEC )
	{
		return;
    }

	/* Do movement */
	{
		unsigned int targetDistance, currentDistance;

		/* Calculate movement vector: */
		if ( psStats->movementModel == MM_HOMINGDIRECT && psProj->psDest )
		{
			/* If it's homing and it has a target (not a miss)... */
			move.x = psProj->psDest->pos.x - psProj->startX;
			move.y = psProj->psDest->pos.y - psProj->startY;
			move.z = psProj->psDest->pos.z - psProj->srcHeight;
		}
		else
		{
			move.x = psProj->tarX - psProj->startX;
			move.y = psProj->tarY - psProj->startY;
			move.z = psProj->altChange;
		}

		targetDistance = sqrtf( move.x*move.x + move.y*move.y );
		currentDistance = timeSoFar * psStats->flightSpeed / GAME_TICKS_PER_SEC;

		// Prevent div by zero:
		if (targetDistance == 0)
			targetDistance = 1;

		distanceRatio = (float)currentDistance / targetDistance;
		distancePercent = PERCENT(currentDistance, targetDistance);

		{
			/* Calculate next position */
			Vector3uw nextPos = {
				psProj->startX + (distanceRatio * move.x),
				psProj->startY + (distanceRatio * move.y),
				psProj->srcHeight + (distanceRatio * move.z)
			};

			/* impact if about to go off map else update coordinates */
			if ( !worldOnMap( nextPos.x, nextPos.y ) )
			{
				psProj->state = PROJ_IMPACT;
				setProjectileDestination(psProj, NULL);
				debug( LOG_NEVER, "**** projectile off map - removed ****\n" );
				return;
			}

			/* Update position */
			psProj->pos = nextPos;
		}
	}

	// Calculate extended lifespan where appropriate
	switch (psStats->weaponSubClass)
	{
		case WSC_MGUN:
		case WSC_COMMAND:
			distanceExtensionFactor = 1.2f;
			break;
		case WSC_CANNON:
		case WSC_BOMB:
		case WSC_ELECTRONIC:
		case WSC_EMP:
		case WSC_FLAME:
		case WSC_ENERGY:
		case WSC_GAUSS:
			distanceExtensionFactor = 1.5f;
			break;
		case WSC_AAGUN: // No extended distance
			distanceExtensionFactor = 1.0f;
			break;
		case WSC_ROCKET:
		case WSC_MISSILE:
		case WSC_SLOWROCKET:
		case WSC_SLOWMISSILE:
			bMissile = true; // Take the same extended targetDistance as artillery
		case WSC_COUNTER:
		case WSC_MORTARS:
		case WSC_HOWITZERS:
		case WSC_LAS_SAT:
			distanceExtensionFactor = 1.5f;
			break;
		default:
			// NUM_WEAPON_SUBCLASS and INVALID_SUBCLASS
			break;
	}

	for (i = 0; i < numProjNaybors; i++)
	{
		BASE_OBJECT *psTempObj = asProjNaybors[i].psObj;

		CHECK_OBJECT(psTempObj);

		if ( psTempObj == psProj->psDamaged )
		{
			// Dont damage one target twice
			continue;
		}

		if (psTempObj->died)
		{
			// Do not damage dead objects further
			continue;
		}

		if ( psTempObj->type == OBJ_PROJECTILE &&
			!( bMissile || ((PROJECTILE *)psTempObj)->psWStats->weaponSubClass == WSC_COUNTER ) )
		{
			// A projectile should not collide with another projectile unless it's a counter-missile weapon
			continue;
		}

		if ( psTempObj->type == OBJ_FEATURE &&
			!((FEATURE *)psTempObj)->psStats->damageable )
		{
			// Ignore oil resources, artifacts and other pickups
			continue;
		}

		if ( psTempObj->player == psProj->player ||
			aiCheckAlliances(psTempObj->player, psProj->player) )
		{
			// No friendly fire
			continue;
		}

		if (psStats->surfaceToAir == SHOOT_IN_AIR &&
			( psTempObj->type == OBJ_STRUCTURE ||
				psTempObj->type == OBJ_FEATURE ||
				(psTempObj->type == OBJ_DROID && !vtolDroid((DROID *)psTempObj))
			))
		{
			// AA weapons should not hit buildings and non-vtol droids
			continue;
		}

		{
			// FIXME HACK Needed since we got those ugly Vector3uw floating around in BASE_OBJECT...
			Vector3i
				posProj = {psProj->pos.x, psProj->pos.y, psProj->pos.z},
				posTemp = {psTempObj->pos.x, psTempObj->pos.y, psTempObj->pos.z};

			Vector3i diff = Vector3i_Sub(posProj, posTemp);
			diff.z = abs(diff.z);

			unsigned int targetHeight = establishTargetHeight(psTempObj);
			unsigned int targetRadius = establishTargetRadius(psTempObj);

			/* We hit! */
			if ( diff.z < targetHeight &&
				(diff.x*diff.x + diff.y*diff.y) < targetRadius * targetRadius )
			{
				setProjectileDestination(psProj, psTempObj);

				/* In case we have a penetrating weapon: */
				if ( psTempObj->type == OBJ_DROID && psStats->penetrate )
				{
					WEAPON asWeap = {psStats - asWeaponStats, 0, 0, 0, 0};
					// Determine position to fire a missile at
					// (must be at least 0 because we don't use signed integers
					//  this shouldn't be larger than the height and width of the map either)
					Vector3i newDest = {
						psProj->startX + move.x * distanceExtensionFactor,
						psProj->startY + move.y * distanceExtensionFactor,
						psProj->srcHeight + move.z * distanceExtensionFactor
					};

					newDest.x = clip(newDest.x, 0, world_coord(mapWidth - 1));
					newDest.y = clip(newDest.y, 0, world_coord(mapHeight - 1));

					//Watermelon:just assume we damaged the chosen target
					setProjectileDamaged(psProj, psTempObj);

					proj_SendProjectile( &asWeap, (BASE_OBJECT*)psProj, psProj->player, newDest, NULL, true, psStats->penetrate, -1 );
				}
				else
				{
					/* Buildings cannot be penetrated... */
					psProj->state = PROJ_IMPACT;
				}
				return;
			}
		}
	}

	if (distanceRatio > distanceExtensionFactor || /* We've traveled our maximum range */
		!mapObjIsAboveGround( (BASE_OBJECT *) psProj ) ) /* trying to travel through terrain */
	{
		/* Miss due to range or height */
		psProj->state = PROJ_IMPACT;
		setProjectileDestination(psProj, NULL); /* miss registered if NULL target */
		return;
	}

	/* Paint effects if visible */
	if ( gfxVisible(psProj) )
	{
		/* add smoke trail to indirect weapons firing directly */
		if( !proj_Direct(psStats) && gfxVisible(psProj))
		{
			Vector3i pos = { psProj->pos.x, psProj->pos.z+8, psProj->pos.y };
			addEffect(&pos,EFFECT_SMOKE,SMOKE_TYPE_TRAIL,false,NULL,0);
		}

		switch (psStats->weaponSubClass)
		{
			case WSC_FLAME:
			{
				Vector3i pos = { psProj->pos.x, psProj->pos.z-8, psProj->pos.y };
				effectGiveAuxVar(distancePercent);
				addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_FLAMETHROWER,false,NULL,0);
			} break;
			case WSC_COMMAND:
			case WSC_ELECTRONIC:
			case WSC_EMP:
			{
				Vector3i pos = { psProj->pos.x, psProj->pos.z-8, psProj->pos.y };
				effectGiveAuxVar(distancePercent/2);
				addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_LASER,false,NULL,0);
			} break;
			case WSC_ROCKET:
			case WSC_MISSILE:
			case WSC_SLOWROCKET:
			case WSC_SLOWMISSILE:
			{
				Vector3i pos = { psProj->pos.x, psProj->pos.z+8, psProj->pos.y };
				addEffect(&pos,EFFECT_SMOKE,SMOKE_TYPE_TRAIL,false,NULL,0);
			} break;
			default:
				/* No effect */
				break;
		}
	}
}

/***************************************************************************/

static void proj_InFlightIndirectFunc( PROJECTILE *psObj )
{
	WEAPON_STATS	*psStats;
	SDWORD			iTime, iRad, iDist, dx, dy, dz, iX, iY;
	Vector3i pos;
	float			fVVert;
	BOOL			bOver = false;
	BASE_OBJECT		*psNewTarget;
	UDWORD			i;
	SDWORD			xdiff,ydiff,extendRad;
	UDWORD			zdiff;
	BOOL			bPenetrate;
	WEAPON			asWeap;

	CHECK_PROJECTILE(psObj);

	psStats = psObj->psWStats;
	bPenetrate = psStats->penetrate;
	ASSERT( psStats != NULL,
		"proj_InFlightIndirectFunc: Invalid weapon stats pointer" );

	iTime = gameTime - psObj->born;

	dx = (SDWORD)psObj->tarX-(SDWORD)psObj->startX;
	dy = (SDWORD)psObj->tarY-(SDWORD)psObj->startY;

	// ffs
	iRad = (SDWORD)sqrtf( dx*dx + dy*dy );

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
		psObj->pos.x = (UWORD)iX;
		psObj->pos.y = (UWORD)iY;
	}

	dz = (psObj->vZ - (iTime*ACC_GRAVITY/GAME_TICKS_PER_SEC/2)) *
				iTime / GAME_TICKS_PER_SEC;
	psObj->pos.z = (UWORD)(psObj->srcHeight + dz);

	fVVert = psObj->vZ - (iTime * ACC_GRAVITY / GAME_TICKS_PER_SEC);
	psObj->pitch = (SWORD)( RAD_TO_DEG(atan2(fVVert, psObj->vXY)) );

	//Watermelon:extended life span for artillery projectile
	extendRad = (SDWORD)(iRad * 1.2f);

	if( gfxVisible(psObj) )
	{
		switch(psStats->weaponSubClass)
		{
			case WSC_MGUN:
			case WSC_CANNON:
			case WSC_MORTARS:
			case WSC_ENERGY:
			case WSC_GAUSS:
			case WSC_HOWITZERS:
			case WSC_AAGUN:
			case WSC_LAS_SAT:
			case WSC_BOMB:
			case WSC_COUNTER:
			case NUM_WEAPON_SUBCLASS:
			case INVALID_SUBCLASS:
				break;
			case WSC_FLAME:
				effectGiveAuxVar(PERCENT(iDist,iRad));
				pos.x = psObj->pos.x;
				pos.y = psObj->pos.z-8;
				pos.z = psObj->pos.y;
				addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_FLAMETHROWER,false,NULL,0);
				break;
			case WSC_COMMAND:
			case WSC_ELECTRONIC:
			case WSC_EMP:
				effectGiveAuxVar((PERCENT(iDist,iRad)/2));
				pos.x = psObj->pos.x;
				pos.y = psObj->pos.z-8;
				pos.z = psObj->pos.y;
				addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_LASER,false,NULL,0);
				break;
			case WSC_ROCKET:
			case WSC_MISSILE:
			case WSC_SLOWROCKET:
			case WSC_SLOWMISSILE:
				pos.x = psObj->pos.x;
				pos.y = psObj->pos.z+8;
				pos.z = psObj->pos.y;
				addEffect(&pos,EFFECT_SMOKE,SMOKE_TYPE_TRAIL,false,NULL,0);
				break;
		}
	}

	//Watermelon:test test
	for (i = 0;i < numProjNaybors;i++)
	{
		BASE_OBJECT *psTempObj = asProjNaybors[i].psObj;

		CHECK_OBJECT(psTempObj);

		//Watermelon;dont collide with any other projectiles
		if ( psTempObj->type == OBJ_PROJECTILE )
		{
			continue;
		}

		//Watermelon:ignore oil resource and pickup
		if ( psTempObj->type == OBJ_FEATURE &&
			((FEATURE *)psTempObj)->psStats->damageable == 0 )
		{
			continue;
		}

		if (psTempObj->died)
		{
			continue;
		}

		if (psTempObj->player != psObj->player &&
			(psTempObj->type == OBJ_DROID ||
			psTempObj->type == OBJ_STRUCTURE ||
			psTempObj->type == OBJ_PROJECTILE ||
			psTempObj->type == OBJ_FEATURE) &&
			!aiCheckAlliances(psTempObj->player,psObj->player))
		{
			if ( psTempObj->type == OBJ_STRUCTURE || psTempObj->type == OBJ_FEATURE )
			{
				xdiff = (SDWORD)psObj->pos.x - (SDWORD)psTempObj->pos.x;
				ydiff = (SDWORD)psObj->pos.y - (SDWORD)psTempObj->pos.y;
				zdiff = abs((UDWORD)psObj->pos.z - (UDWORD)psTempObj->pos.z);

				if ( zdiff < establishTargetHeight(psTempObj) &&
					(xdiff*xdiff + ydiff*ydiff) < ( (SDWORD)establishTargetRadius(psTempObj) * (SDWORD)establishTargetRadius(psTempObj) ) )
				{
					psNewTarget = psTempObj;
					setProjectileDestination(psObj, psNewTarget);
		  			psObj->state = PROJ_IMPACT;
					return;
				}
			}
			else
			{
				xdiff = (SDWORD)psObj->pos.x - (SDWORD)psTempObj->pos.x;
				ydiff = (SDWORD)psObj->pos.y - (SDWORD)psTempObj->pos.y;
				zdiff = abs((UDWORD)psObj->pos.z - (UDWORD)psTempObj->pos.z);

				if ( zdiff < establishTargetHeight(psTempObj) &&
					(UDWORD)(xdiff*xdiff + ydiff*ydiff) < ( (SDWORD)establishTargetRadius(psTempObj) * (SDWORD)establishTargetRadius(psTempObj) ) )
				{
					psNewTarget = psTempObj;
					setProjectileDestination(psObj, psNewTarget);

					if (bPenetrate)
					{
						// Determine position to fire a missile at
						// (must be at least 0 because we don't use signed integers
						//  this shouldn't be larger than the height and width of the map either)
						Vector3i target = {
							psObj->startX + extendRad * dx / iRad,
							psObj->startY + extendRad * dy / iRad,
							psObj->pos.z
						};
						target.x = clip(target.x, 0, world_coord(mapWidth - 1));
						target.y = clip(target.y, 0, world_coord(mapHeight - 1));

						asWeap.nStat = psObj->psWStats - asWeaponStats;
						//Watermelon:just assume we damaged the chosen target
						setProjectileDamaged(psObj, psNewTarget);

						proj_SendProjectile( &asWeap, (BASE_OBJECT*)psObj, psObj->player, target, NULL, true, bPenetrate, -1 );
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

		pos.x = psObj->pos.x;
		pos.z = psObj->pos.y;
		pos.y = map_Height(pos.x,pos.z) + 8;

		/* It's damage time */
		//Watermelon:'real' check
		if ( psObj->psDest )
		{
			psObj->pos.z = (UWORD)(pos.y + 8); // bring up the impact explosion

			xdiff = (SDWORD)psObj->pos.x - (SDWORD)psObj->psDest->pos.x;
			ydiff = (SDWORD)psObj->pos.y - (SDWORD)psObj->psDest->pos.y;
			zdiff = abs((UDWORD)psObj->pos.z - (UDWORD)psObj->psDest->pos.z);

			if ( zdiff < establishTargetHeight(psObj->psDest) &&
				(xdiff*xdiff + ydiff*ydiff) < (SDWORD)( establishTargetRadius(psObj->psDest) * establishTargetRadius(psObj->psDest) ) )
			{
				psObj->state = PROJ_IMPACT;
			}
			else
			{
				setProjectileDestination(psObj, NULL);
			}
		}
		bOver = true;
	}

#if CHECK_PROJ_ABOVE_GROUND
	/* check not trying to travel through terrain - if so count as a miss */
	if ( mapObjIsAboveGround( (BASE_OBJECT *) psObj ) == false )
	{
		psObj->state = PROJ_IMPACT;
		/* miss registered if NULL target */
		setProjectileDestination(psObj, NULL);
		bOver = true;
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
			pos.x = psObj->pos.x;
			pos.y = psObj->pos.z+4;
			pos.z = psObj->pos.y;
			addEffect(&pos,EFFECT_SMOKE,SMOKE_TYPE_TRAIL,false,NULL,0);
		}
	}
}

/***************************************************************************/

static void proj_ImpactFunc( PROJECTILE *psObj )
{
	WEAPON_STATS	*psStats;
	SDWORD			i, iAudioImpactID;
	DROID			*psCurrD, *psNextD;
	STRUCTURE		*psCurrS, *psNextS;
	FEATURE			*psCurrF, *psNextF;
	UDWORD			dice;
	SDWORD			tarX0,tarY0, tarX1,tarY1;
	SDWORD			radCubed, xDiff,yDiff;
	float			relativeDamage;
	Vector3i position,scatter;
	UDWORD			damage;	//optimisation - were all being calculated twice on PC
	//Watermelon: tarZ0,tarZ1,zDiff for AA AOE weapons;
	SDWORD			tarZ0,tarZ1,zDiff;
	// EvilGuru: Data about the effect to be shown
	EFFECT_TYPE     facing;
	iIMDShape       *imd;
	HIT_SIDE	impactSide = HIT_SIDE_FRONT;

	CHECK_PROJECTILE(psObj);

	psStats = psObj->psWStats;
	ASSERT( psStats != NULL,
		"proj_ImpactFunc: Invalid weapon stats pointer" );

	// Get if we are facing or not
	facing = (psStats->facePlayer) ? EXPLOSION_TYPE_SPECIFIED : EXPLOSION_TYPE_NOT_FACING;

	// note the attacker if any
	g_pProjLastAttacker = psObj->psSource;

	/* play impact audio */
	if (gfxVisible(psObj))
	{
		if (psStats->iAudioImpactID == NO_SOUND)
		{
			/* play richochet if MG */
			if (psObj->psDest != NULL && psObj->psWStats->weaponSubClass == WSC_MGUN
			 && ONEINTHREE)
			{
				iAudioImpactID = ID_SOUND_RICOCHET_1 + (rand() % 3);
				audio_PlayStaticTrack(psObj->psDest->pos.x, psObj->psDest->pos.y, iAudioImpactID);
			}
		}
		else
		{
			if (psObj->psDest == NULL)
			{
				audio_PlayStaticTrack(psObj->tarX, psObj->tarY, psStats->iAudioImpactID);
			}
			else
			{
				audio_PlayStaticTrack(psObj->psDest->pos.x, psObj->psDest->pos.y, psStats->iAudioImpactID);
			}
		}

		/* Shouldn't need to do this check but the stats aren't all at a value yet... */ // FIXME
		if (psStats->incenRadius && psStats->incenTime)
		{
			position.x = psObj->tarX;
			position.z = psObj->tarY;
			position.y = map_Height(position.x, position.z);
			effectGiveAuxVar(psStats->incenRadius);
			effectGiveAuxVarSec(psStats->incenTime);
			addEffect(&position, EFFECT_FIRE, FIRE_TYPE_LOCALISED, false, NULL, 0);
		}

		// may want to add both a fire effect and the las sat effect
		if (psStats->weaponSubClass == WSC_LAS_SAT)
		{
			position.x = psObj->tarX;
			position.z = psObj->tarY;
			position.y = map_Height(position.x, position.z);
			addEffect(&position, EFFECT_SAT_LASER, SAT_LASER_STANDARD, false, NULL, 0);
			if (clipXY(psObj->tarX, psObj->tarY))
			{
				shakeStart();
			}
		}
	}

	// Set the effects position and radius
	position.x = psObj->pos.x;
	position.z = psObj->pos.y;
	position.y = psObj->pos.z;//map_Height(psObj->pos.x, psObj->pos.y) + 24;
	scatter.x = psStats->radius;
	scatter.y = 0;
	scatter.z = psStats->radius;

	// If the projectile missed its target (or the target died)
	if (psObj->psDest == NULL)
	{
		if (gfxVisible(psObj))
		{
			// The graphic to show depends on if we hit water or not
			if (terrainType(mapTile(map_coord(psObj->pos.x), map_coord(psObj->pos.y))) == TER_WATER)
			{
				imd = psStats->pWaterHitGraphic;
			}
			// We did not hit water, the regular miss graphic will do the trick
			else
			{
				imd = psStats->pTargetMissGraphic;
			}

			addMultiEffect(&position, &scatter, EFFECT_EXPLOSION, facing, true, imd, psStats->numExplosions, psStats->lightWorld, psStats->effectSize);

			// If the target was a VTOL hit in the air add smoke
			if (psObj->airTarget
			 && (psStats->surfaceToAir & SHOOT_IN_AIR)
			 && !(psStats->surfaceToAir & SHOOT_ON_GROUND))
			{
				addMultiEffect(&position, &scatter, EFFECT_SMOKE, SMOKE_TYPE_DRIFTING, false, NULL, 3, 0, 0);
			}
		}
	}
	// The projectile hit its intended target
	else
	{
		CHECK_OBJECT(psObj->psDest);

		if (psObj->psDest->type == OBJ_FEATURE
		 && ((FEATURE *)psObj->psDest)->psStats->damageable == 0)
		{
			debug(LOG_NEVER, "proj_ImpactFunc: trying to damage non-damageable target,projectile removed");
			psObj->died = gameTime;
			return;
		}

		if (gfxVisible(psObj))
		{
			// If we hit a VTOL with an AA gun use the miss graphic and add some smoke
			if (psObj->airTarget
			 && (psStats->surfaceToAir & SHOOT_IN_AIR)
			 && !(psStats->surfaceToAir & SHOOT_ON_GROUND)
			 && psStats->weaponSubClass == WSC_AAGUN)
			{
				imd = psStats->pTargetMissGraphic;
				addMultiEffect(&position, &scatter, EFFECT_SMOKE, SMOKE_TYPE_DRIFTING, false, NULL, 3, 0, 0);
			}
			// Otherwise we just hit it plain and simple
			else
			{
				imd = psStats->pTargetHitGraphic;
			}

			addMultiEffect(&position, &scatter, EFFECT_EXPLOSION, facing, true, imd, psStats->numExplosions, psStats->lightWorld, psStats->effectSize);
		}

		// Check for electronic warfare damage where we know the subclass and source
		if (proj_Direct(psStats)
		 && psStats->weaponSubClass == WSC_ELECTRONIC
		 && psObj->psSource)
		{
			// If we did enough `damage' to capture the target
			if (electronicDamage(psObj->psDest,
			                     calcDamage(weaponDamage(psStats, psObj->player), psStats->weaponEffect, psObj->psDest),
			                     psObj->player))
			{
				switch (psObj->psSource->type)
				{
					case OBJ_DROID:
						((DROID *) psObj->psSource)->order = DORDER_NONE;
						actionDroid((DROID *) (psObj->psSource), DACTION_NONE);
						break;

					case OBJ_STRUCTURE:
						((STRUCTURE *) psObj->psSource)->psTarget[0] = NULL;
						break;

					// This is only here to prevent the compiler from producing
					// warnings for unhandled enumeration values
					default:
						break;
				}
			}
		}
		// Else it is just a regular weapon (direct or indirect)
		else
		{
			// Calculate the damage the weapon does to its target
			damage = calcDamage(weaponDamage(psStats, psObj->player), psStats->weaponEffect, psObj->psDest);

			// If we are in a multi-player game and the attacker is our responsibility
			if (bMultiPlayer && psObj->psSource && myResponsibility(psObj->psSource->player))
			{
				updateMultiStatsDamage(psObj->psSource->player, psObj->psDest->player, damage);
			}

			debug(LOG_NEVER, "Damage to object %d, player %d\n",
			      psObj->psDest->id, psObj->psDest->player);

			// If the target is a droid work out the side of it we hit
			if (psObj->psDest->type == OBJ_DROID)
			{
				// For indirect weapons (e.g. artillery) just assume the side as HIT_SIDE_TOP
				impactSide = proj_Direct(psStats) ? getHitSide(psObj, psObj->psDest) : HIT_SIDE_TOP;
			}

			// Damage the object
			relativeDamage = objectDamage(psObj->psDest,damage , psStats->weaponClass,psStats->weaponSubClass, impactSide);

			proj_UpdateKills(psObj, relativeDamage);

			if (relativeDamage >= 0)	// So long as the target wasn't killed
			{
				setProjectileDamaged(psObj, psObj->psDest);
			}
		}
	}

	// If the projectile does no splash damage and does not set fire to things
	if ((psStats->radius == 0) && (psStats->incenTime == 0) )
	{
		psObj->died = gameTime;
		return;
	}

	if (psStats->radius != 0)
	{
		/* An area effect bullet */
		psObj->state = PROJ_POSTIMPACT;

		/* Note when it exploded for the explosion effect */
		psObj->born = gameTime;

		/* Work out the bounding box for the blast radius */
		tarX0 = (SDWORD)psObj->pos.x - (SDWORD)psStats->radius;
		tarY0 = (SDWORD)psObj->pos.y - (SDWORD)psStats->radius;
		tarX1 = (SDWORD)psObj->pos.x + (SDWORD)psStats->radius;
		tarY1 = (SDWORD)psObj->pos.y + (SDWORD)psStats->radius;
		/* Watermelon:height bounding box  for airborne units*/
		tarZ0 = (SDWORD)psObj->pos.z - (SDWORD)psStats->radius;
		tarZ1 = (SDWORD)psObj->pos.z + (SDWORD)psStats->radius;

		/* Store the radius cubed */
		radCubed = psStats->radius * psStats->radius * psStats->radius;

		for (i = 0; i < MAX_PLAYERS; i++)
		{
			for (psCurrD = apsDroidLists[i]; psCurrD; psCurrD = psNextD)
			{
				/* have to store the next pointer as psCurrD could be destroyed */
				psNextD = psCurrD->psNext;

				/* see if psCurrD is hit (don't hit main target twice) */
				if (((BASE_OBJECT *)psCurrD != psObj->psDest) &&
					((SDWORD)psCurrD->pos.x >= tarX0) &&
					((SDWORD)psCurrD->pos.x <= tarX1) &&
					((SDWORD)psCurrD->pos.y >= tarY0) &&
					((SDWORD)psCurrD->pos.y <= tarY1) &&
					((SDWORD)psCurrD->pos.z >= tarZ0) &&
					((SDWORD)psCurrD->pos.z <= tarZ1))
				{
					/* Within the bounding box, now check the radius */
					xDiff = psCurrD->pos.x - psObj->pos.x;
					yDiff = psCurrD->pos.y - psObj->pos.y;
					zDiff = psCurrD->pos.z - psObj->pos.z;
					if ((xDiff*xDiff + yDiff*yDiff + zDiff*zDiff) <= radCubed)
					{
						HIT_ROLL(dice);
						if (dice < weaponRadiusHit(psStats, psObj->player))
						{
							debug(LOG_NEVER, "Damage to object %d, player %d\n",
									psCurrD->id, psCurrD->player);

							damage = calcDamage(
										weaponRadDamage(psStats, psObj->player),
										psStats->weaponEffect, (BASE_OBJECT *)psCurrD);
							if (bMultiPlayer)
							{
								if (psObj->psSource && myResponsibility(psObj->psSource->player))
								{
									updateMultiStatsDamage(psObj->psSource->player, psCurrD->player, damage);
								}
								turnOffMultiMsg(true);
							}

							//Watermelon:uses a slightly different check for angle,
							// since fragment of a project is from the explosion spot not from the projectile start position
							impactSide = getHitSide(psObj, (BASE_OBJECT *)psCurrD);

							relativeDamage = droidDamage(psCurrD, damage, psStats->weaponClass, psStats->weaponSubClass, impactSide);

							turnOffMultiMsg(false);	// multiplay msgs back on.

							proj_UpdateKills(psObj, relativeDamage);
						}
					}
				}
			}
			if (!psObj->airTarget)
			{
				for (psCurrS = apsStructLists[i]; psCurrS; psCurrS = psNextS)
				{
					/* have to store the next pointer as psCurrD could be destroyed */
					psNextS = psCurrS->psNext;

					/* see if psCurrS is hit (don't hit main target twice) */
					if (((BASE_OBJECT *)psCurrS != psObj->psDest) &&
						((SDWORD)psCurrS->pos.x >= tarX0) &&
						((SDWORD)psCurrS->pos.x <= tarX1) &&
						((SDWORD)psCurrS->pos.y >= tarY0) &&
						((SDWORD)psCurrS->pos.y <= tarY1))
					{
						/* Within the bounding box, now check the radius */
						xDiff = psCurrS->pos.x - psObj->pos.x;
						yDiff = psCurrS->pos.y - psObj->pos.y;
						if ((xDiff*xDiff + yDiff*yDiff) <= radCubed)
						{
							HIT_ROLL(dice);
							if (dice < weaponRadiusHit(psStats, psObj->player))
							{
								damage = calcDamage(weaponRadDamage(psStats, psObj->player),
								                    psStats->weaponEffect,
								                    (BASE_OBJECT *)psCurrS);

								if (bMultiPlayer)
								{
									if (psObj->psSource && myResponsibility(psObj->psSource->player))
									{
										updateMultiStatsDamage(psObj->psSource->player,	psCurrS->player,damage);
									}
								}

								//Watermelon:uses a slightly different check for angle,
								// since fragment of a project is from the explosion spot not from the projectile start position
								impactSide = getHitSide(psObj, (BASE_OBJECT *)psCurrS);

								relativeDamage = structureDamage(psCurrS,
								                                damage,
								                                psStats->weaponClass,
								                                psStats->weaponSubClass, impactSide);

								proj_UpdateKills(psObj, relativeDamage);
							}
						}
					}
					// Missed by old method, but maybe in landed within the building's footprint(baseplate)
					else if(ptInStructure(psCurrS,psObj->pos.x, psObj->pos.y) && (BASE_OBJECT*)psCurrS != psObj->psDest)
					{
						damage = NOMINAL_DAMAGE;

						if(bMultiPlayer)
						{
							if(psObj->psSource && myResponsibility(psObj->psSource->player))
							{
								updateMultiStatsDamage(psObj->psSource->player,	psCurrS->player,damage);
							}
						}

						relativeDamage = structureDamage(psCurrS,
						                                damage,
						                                psStats->weaponClass,
						                                psStats->weaponSubClass, impactSide);

						proj_UpdateKills(psObj, relativeDamage);
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
				((SDWORD)psCurrF->pos.x >= tarX0) &&
				((SDWORD)psCurrF->pos.x <= tarX1) &&
				((SDWORD)psCurrF->pos.y >= tarY0) &&
				((SDWORD)psCurrF->pos.y <= tarY1))
			{
				/* Within the bounding box, now check the radius */
				xDiff = psCurrF->pos.x - psObj->pos.x;
				yDiff = psCurrF->pos.y - psObj->pos.y;
				if ((xDiff*xDiff + yDiff*yDiff) <= radCubed)
				{
					HIT_ROLL(dice);
					if (dice < weaponRadiusHit(psStats, psObj->player))
					{
						debug(LOG_NEVER, "Damage to object %d, player %d\n",
								psCurrF->id, psCurrF->player);

						// Watermelon:uses a slightly different check for angle,
						// since fragment of a project is from the explosion spot not from the projectile start position
						impactSide = getHitSide(psObj, (BASE_OBJECT *)psCurrF);

						relativeDamage = featureDamage(psCurrF,
						                              calcDamage(weaponRadDamage(psStats, psObj->player),
						                                         psStats->weaponEffect,
						                                         (BASE_OBJECT *)psCurrF),
						                              psStats->weaponClass,
						                              psStats->weaponSubClass, impactSide);

						proj_UpdateKills(psObj, relativeDamage);
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

static void proj_PostImpactFunc( PROJECTILE *psObj )
{
	WEAPON_STATS	*psStats;
	SDWORD			i, age;
	FIRE_BOX		flame;

	CHECK_PROJECTILE(psObj);

	psStats = psObj->psWStats;
	ASSERT( psStats != NULL,
		"proj_PostImpactFunc: Invalid weapon stats pointer" );

	age = (SDWORD)gameTime - (SDWORD)psObj->born;

	/* Time to finish postimpact effect? */
	if (age > (SDWORD)psStats->radiusLife && age > (SDWORD)psStats->incenTime)
	{
		psObj->died = gameTime;
		return;
	}

	/* Burning effect */
	if (psStats->incenTime > 0)
	{
		/* See if anything is in the fire and burn it */

		/* Calculate the fire's bounding box */
		flame.x1 = (SWORD)(psObj->pos.x - psStats->incenRadius);
		flame.y1 = (SWORD)(psObj->pos.y - psStats->incenRadius);
		flame.x2 = (SWORD)(psObj->pos.x + psStats->incenRadius);
		flame.y2 = (SWORD)(psObj->pos.y + psStats->incenRadius);
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

static void proj_Update(PROJECTILE *psObj)
{
	CHECK_PROJECTILE(psObj);

	/* See if any of the stored objects have died
	 * since the projectile was created
	 */
	if (psObj->psSource && psObj->psSource->died)
	{
		setProjectileSource(psObj, NULL);
	}
	if (psObj->psDest && psObj->psDest->died)
	{
		setProjectileDestination(psObj, NULL);
	}
	if (psObj->psDamaged && psObj->psDamaged->died)
	{
		setProjectileDamaged(psObj, NULL);
	}

	// This extra check fixes a crash in cam2, mission1
	if (worldOnMap(psObj->pos.x, psObj->pos.y) == false)
	{
		psObj->died = true;
		return;
	}

	projGetNaybors((PROJECTILE *)psObj);

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

// iterate through all projectiles and update their status
void proj_UpdateAll()
{
	PROJECTILE	*psObj, *psPrev;

	for (psObj = psProjectileList; psObj != NULL; psObj = psObj->psNext)
	{
		proj_Update( psObj );
	}

	// Now delete any dead projectiles
	psObj = psProjectileList;

	// is the first node dead?
	while (psObj && psObj == psProjectileList && psObj->died)
	{
		psProjectileList = psObj->psNext;
		proj_Free(psObj);
		psObj = psProjectileList;
	}

	// first object is now either NULL or not dead, so we have time to set this below
	psPrev = NULL;

	// are any in the list dead?
	while (psObj)
	{
		if (psObj->died)
		{
			psPrev->psNext = psObj->psNext;
			proj_Free(psObj);
			psObj = psPrev->psNext;
		} else {
			psPrev = psObj;
			psObj = psObj->psNext;
		}
	}
}

/***************************************************************************/

static void proj_checkBurnDamage( BASE_OBJECT *apsList, PROJECTILE *psProj, FIRE_BOX *pFireBox )
{
	BASE_OBJECT		*psCurr, *psNext;
	SDWORD			xDiff,yDiff;
	WEAPON_STATS	*psStats;
	UDWORD			damageSoFar;
	SDWORD			damageToDo;
	float			relativeDamage;

	CHECK_PROJECTILE(psProj);

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
		if (((SDWORD)psCurr->pos.x >= pFireBox->x1) &&
			((SDWORD)psCurr->pos.x <= pFireBox->x2) &&
			((SDWORD)psCurr->pos.y >= pFireBox->y1) &&
			((SDWORD)psCurr->pos.y <= pFireBox->y2))
		{
			/* Within the bounding box, now check the radius */
			xDiff = psCurr->pos.x - psProj->pos.x;
			yDiff = psCurr->pos.y - psProj->pos.y;
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

						//Watermelon:just assume the burn damage is from FRONT
	  					relativeDamage = objectDamage(psCurr, damageToDo, psStats->weaponClass,psStats->weaponSubClass, 0);

						psCurr->burnDamage += damageToDo;

						proj_UpdateKills(psProj, relativeDamage);
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
bool proj_Direct(const WEAPON_STATS* psStats)
{
	ASSERT(psStats != NULL, "proj_Direct: called with NULL weapon!");
	if (!psStats)
	{
		return true; // arbitrary value in no-debug case
	}
	ASSERT(psStats->movementModel < NUM_MOVEMENT_MODEL, "proj_Direct: invalid weapon stat");

	switch (psStats->movementModel)
	{
	case MM_DIRECT:
	case MM_HOMINGDIRECT:
	case MM_ERRATICDIRECT:
	case MM_SWEEP:
		return true;
		break;
	case MM_INDIRECT:
	case MM_HOMINGINDIRECT:
		return false;
		break;
	case NUM_MOVEMENT_MODEL:
	case INVALID_MOVEMENT:
		break; // error checking in assert above; this is for no-debug case
	}

	return true; // just to satisfy compiler
}

/***************************************************************************/

// return the maximum range for a weapon
SDWORD proj_GetLongRange(const WEAPON_STATS* psStats)
{
	return psStats->longRange;
}


/***************************************************************************/
static UDWORD	establishTargetRadius(BASE_OBJECT *psTarget)
{
	UDWORD		radius;
	STRUCTURE	*psStructure;
	FEATURE		*psFeat;

	CHECK_OBJECT(psTarget);
	radius = 0;

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
					radius = abs(psTarget->sDisplay.imd->radius) * 2;
					break;
				case DROID_DEFAULT:
				case DROID_TRANSPORTER:
				default:
					radius = TILE_UNITS/4;	// how will we arrive at this?
			}
			break;
		case OBJ_STRUCTURE:
			psStructure = (STRUCTURE*)psTarget;
			radius = (MAX(psStructure->pStructureType->baseBreadth, psStructure->pStructureType->baseWidth) * TILE_UNITS) / 2;
			break;
		case OBJ_FEATURE:
//			radius = TILE_UNITS/4;	// how will we arrive at this?
			psFeat = (FEATURE *)psTarget;
			radius = (MAX(psFeat->psStats->baseBreadth,psFeat->psStats->baseWidth) * TILE_UNITS) / 2;
			break;
		case OBJ_PROJECTILE:
			//Watermelon 1/2 radius of a droid?
			radius = TILE_UNITS/8;
		default:
			break;
	}

	return(radius);
}
/***************************************************************************/

/*the damage depends on the weapon effect and the target propulsion type or
structure strength*/
UDWORD	calcDamage(UDWORD baseDamage, WEAPON_EFFECT weaponEffect, BASE_OBJECT *psTarget)
{
	UDWORD	damage;

	if (psTarget->type == OBJ_STRUCTURE)
	{
		damage = baseDamage * asStructStrengthModifier[weaponEffect][((
			STRUCTURE *)psTarget)->pStructureType->strength] / 100;
	}
	else if (psTarget->type == OBJ_DROID)
	{
		damage = baseDamage * asWeaponModifier[weaponEffect][(
   			asPropulsionStats + ((DROID *)psTarget)->asBits[COMP_PROPULSION].
			nStat)->propulsionType] / 100;
	}
	// Default value
	else
	{
		damage = baseDamage;
	}

    // A little fail safe!
    if (damage == 0 && baseDamage != 0)
    {
        damage = 1;
    }

	return damage;
}

/*
 * A quick explanation about hown this function works:
 *  - It returns an integer between 0 and 100 (see note for exceptions);
 *  - this represents the amount of damage inflicted on the droid by the weapon
 *    in relation to its original health.
 *  - e.g. If 100 points of (*actual*) damage were done to a unit who started
 *    off (when first produced) with 400 points then .25 would be returned.
 *  - If the actual damage done to a unit is greater than its remaining points
 *    then the actual damage is clipped: so if we did 200 actual points of
 *    damage to a cyborg with 150 points left the actual damage would be taken
 *    as 150.
 *  - Should sufficient damage be done to destroy/kill a unit then the value is
 *    multiplied by -1, resulting in a negative number.
 */
static float objectDamage(BASE_OBJECT *psObj, UDWORD damage, UDWORD weaponClass,UDWORD weaponSubClass, HIT_SIDE impactSide)
{
	switch (psObj->type)
	{
		case OBJ_DROID:
			return droidDamage((DROID *)psObj, damage, weaponClass,weaponSubClass, impactSide);
			break;

		case OBJ_STRUCTURE:
			return structureDamage((STRUCTURE *)psObj, damage, weaponClass, weaponSubClass, impactSide);
			break;

		case OBJ_FEATURE:
			return featureDamage((FEATURE *)psObj, damage, weaponClass, weaponSubClass, impactSide);
			break;

		case OBJ_PROJECTILE:
			ASSERT(!"invalid object type: bullet", "objectDamage: invalid object type: OBJ_PROJECTILE");
			break;

		case OBJ_TARGET:
			ASSERT(!"invalid object type: target", "objectDamage: invalid object type: OBJ_TARGET");
			break;

		default:
			ASSERT(!"unknown object type", "objectDamage: unknown object type");
	}

	return 0;
}

/**
 * This function will calculate which side of the droid psTarget the projectile
 * psObj hit. Although it is possible to extract the target from psObj it is
 * only the `direct' target of the projectile. Since impact sides also apply for
 * any splash damage a projectile might do the exact target is needed.
 */
static HIT_SIDE getHitSide(PROJECTILE *psObj, BASE_OBJECT *psTarget)
{
	int deltaX, deltaY;
	int impactAngle;

	// If we hit the top of the droid
	if (psObj->altChange > 300)
	{
		return HIT_SIDE_TOP;
	}
	// If the height difference between us and the target is > 50
	else if (psObj->pos.z < (psTarget->pos.z - 50))
	{
		return HIT_SIDE_BOTTOM;
	}
	// We hit an actual `side'
	else
	{
		deltaX = psObj->startX - psTarget->pos.x;
		deltaY = psObj->startY - psTarget->pos.y;

		/*
		 * Work out the impact angle. It is easiest to understand if you
		 * model the target droid as a circle, divided up into 360 pieces.
		 */
		impactAngle = abs(psTarget->direction - (180 * atan2f(deltaX, deltaY) / M_PI));

		impactAngle = wrap(impactAngle, 360);

		// Use the impact angle to work out the side hit
		// Right
		if (impactAngle > 45 && impactAngle < 135)
			return HIT_SIDE_RIGHT;
		// Rear
		else if (impactAngle >= 135 && impactAngle <= 225)
			return HIT_SIDE_REAR;
		// Left
		else if (impactAngle > 225 && impactAngle < 315)
			return HIT_SIDE_LEFT;
		// Front - default
		else //if (impactAngle <= 45 || impactAngle >= 315)
			return HIT_SIDE_FRONT;
	}
}

/* Returns true if an object has just been hit by an electronic warfare weapon*/
static BOOL	justBeenHitByEW(BASE_OBJECT *psObj)
{
DROID		*psDroid;
FEATURE		*psFeature;
STRUCTURE	*psStructure;

	if(gamePaused())
	{
		return(false);	// Don't shake when paused...!
	}

	switch(psObj->type)
	{
		case OBJ_DROID:
			psDroid = (DROID*)psObj;
			if ((gameTime - psDroid->timeLastHit) < ELEC_DAMAGE_DURATION
			 && psDroid->lastHitWeapon == WSC_ELECTRONIC)
			{
				return(true);
			}
			break;

		case OBJ_FEATURE:
			psFeature = (FEATURE*)psObj;
			if ((gameTime - psFeature->timeLastHit) < ELEC_DAMAGE_DURATION)
			{
				return(true);
			}
			break;

		case OBJ_STRUCTURE:
			psStructure = (STRUCTURE*)psObj;
			if ((gameTime - psStructure->timeLastHit) < ELEC_DAMAGE_DURATION
			 && psStructure->lastHitWeapon == WSC_ELECTRONIC)
			{
				return true;
			}
			break;

		case OBJ_PROJECTILE:
			ASSERT(!"invalid object type: bullet", "justBeenHitByEW: invalid object type: OBJ_PROJECTILE");
			abort();
			break;

		case OBJ_TARGET:
			ASSERT(!"invalid object type: target", "justBeenHitByEW: invalid object type: OBJ_TARGET");
			abort();
			break;

		default:
			ASSERT(!"unknown object type", "justBeenHitByEW: unknown object type");
			abort();
	}

	return false;
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
static void projGetNaybors(PROJECTILE *psObj)
{
	SDWORD		xdiff, ydiff;
	UDWORD		dx,dy, distSqr;
	//Watermelon:renamed to psTempObj from psObj
	BASE_OBJECT	*psTempObj;

	CHECK_PROJECTILE(psObj);

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
	dx = ((BASE_OBJECT *)psObj)->pos.x;
	dy = ((BASE_OBJECT *)psObj)->pos.y;

	gridStartIterate((SDWORD)dx, (SDWORD)dy);
	for (psTempObj = gridIterate(); psTempObj != NULL; psTempObj = gridIterate())
	{
		if (psTempObj != (BASE_OBJECT *)psObj && !psTempObj->died)
		{
			// see if an object is in NAYBOR_RANGE
			xdiff = dx - (SDWORD)psTempObj->pos.x;
			if (xdiff < 0)
			{
				xdiff = -xdiff;
			}
			if (xdiff > PROJ_NAYBOR_RANGE)
			{
				continue;
			}

			ydiff = dy - (SDWORD)psTempObj->pos.y;
			if (ydiff < 0)
			{
				ydiff = -ydiff;
			}
			if (ydiff > PROJ_NAYBOR_RANGE)
			{
				continue;
			}

			distSqr = xdiff*xdiff + ydiff*ydiff;
			if (distSqr > PROJ_NAYBOR_RANGE*PROJ_NAYBOR_RANGE)
			{
				continue;
			}

			addProjNaybor(psTempObj, distSqr);
		}
	}
}

static UDWORD	establishTargetHeight(BASE_OBJECT *psTarget)
{
	UDWORD		height;
	UDWORD		utilityHeight = 0, yMax = 0, yMin = 0; // Temporaries for addition of utility's height to total height
	DROID		*psDroid;
	STRUCTURE_STATS		*psStructureStats;

	if (psTarget == NULL)
	{
		return 0;
	}
	CHECK_OBJECT(psTarget);

	switch(psTarget->type)
	{
		case OBJ_DROID:
			psDroid = (DROID*)psTarget;
			height = asBodyStats[psDroid->asBits[COMP_BODY].nStat].pIMD->max.y - asBodyStats[psDroid->asBits[COMP_BODY].nStat].pIMD->min.y;

			// Don't do this for Barbarian Propulsions as they don't possess a turret (and thus have pIMD == NULL)
			if (!strcmp(asPropulsionStats[psDroid->asBits[COMP_PROPULSION].nStat].pName, "BaBaProp") )
			{
				return height;
			}

			// Commanders don't have pIMD either
			if (psDroid->droidType == DROID_COMMAND)
				return height;

			// VTOL's don't have pIMD either it seems...
			if (vtolDroid(psDroid))
			{
				return (height + VTOL_HITBOX_MODIFICATOR);
			}

			switch(psDroid->droidType)
			{
				case DROID_WEAPON:
					if ( psDroid->numWeaps > 0 )
					{
						yMax = (asWeaponStats[psDroid->asWeaps[0].nStat]).pIMD->max.y;
						yMin = (asWeaponStats[psDroid->asWeaps[0].nStat]).pIMD->min.y;
					}
					break;

				case DROID_SENSOR:
					yMax = (asSensorStats[psDroid->asBits[COMP_SENSOR].nStat]).pIMD->max.y;
					yMin = (asSensorStats[psDroid->asBits[COMP_SENSOR].nStat]).pIMD->min.y;
					break;

				case DROID_ECM:
					yMax = (asECMStats[psDroid->asBits[COMP_ECM].nStat]).pIMD->max.y;
					yMin = (asECMStats[psDroid->asBits[COMP_ECM].nStat]).pIMD->min.y;
					break;

				case DROID_CONSTRUCT:
					yMax = (asConstructStats[psDroid->asBits[COMP_CONSTRUCT].nStat]).pIMD->max.y;
					yMin = (asConstructStats[psDroid->asBits[COMP_CONSTRUCT].nStat]).pIMD->min.y;
					break;

				case DROID_REPAIR:
					yMax = (asRepairStats[psDroid->asBits[COMP_REPAIRUNIT].nStat]).pIMD->max.y;
					yMin = (asRepairStats[psDroid->asBits[COMP_REPAIRUNIT].nStat]).pIMD->min.y;
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

			utilityHeight = (yMax + yMin)/2;
			height += utilityHeight;

			return height;

		case OBJ_STRUCTURE:
			psStructureStats = ((STRUCTURE *)psTarget)->pStructureType;
			return (psStructureStats->pIMD->max.y + psStructureStats->pIMD->min.y) / 2;
		case OBJ_FEATURE:
			// Just use imd ymax+ymin
			return (psTarget->sDisplay.imd->max.y + psTarget->sDisplay.imd->min.y) / 2;
		case OBJ_PROJECTILE:
			// 16 for bullet
			return 16;
		default:
			return 0;
	}
}
