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
/**
 * @file action.c
 *
 * Functions for setting the action of a droid.
 *
 */
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h"
#include "lib/script/script.h"

#include "action.h"
#include "lib/sound/audio_id.h"
#include "lib/sound/audio.h"
#include "combat.h"
#include "formation.h"
#include "geometry.h"
#include "hci.h"
#include "intdisplay.h"
#include "mission.h"
#include "multiplay.h"
#include "projectile.h"
#include "research.h"
#include "scriptcb.h"
#include "scripttabs.h"
#include "transporter.h"
#include "visibility.h"

/* attack run distance */
#define	VTOL_ATTACK_LENGTH		1000
#define	VTOL_ATTACK_WIDTH		200
#define VTOL_ATTACK_TARDIST		400
#define VTOL_ATTACK_RETURNDIST	700

// turret rotation limits
#define VTOL_TURRET_RLIMIT		315
#define VTOL_TURRET_LLIMIT		45

/** Time to pause before a droid blows up. */
#define  ACTION_DESTRUCT_TIME	2000

/** Droids heavier than this rotate and pitch more slowly. */
#define HEAVY_WEAPON_WEIGHT     50000

#define ACTION_TURRET_ROTATION_RATE	45
#define REPAIR_PITCH_LOWER		30
#define	REPAIR_PITCH_UPPER		-15

/** How long to follow a damaged droid around before giving up if don't get near. */
#define KEEP_TRYING_REPAIR	10000

/** How far away the repair droid can be from the damaged droid to function. */
#define REPAIR_RANGE		(TILE_UNITS * TILE_UNITS * 4)

/* How many tiles to pull back. */
#define PULL_BACK_DIST		10

// data required for any action
typedef struct _droid_action_data
{
	DROID_ACTION	action;
	UDWORD			x,y;
	//Watermelon:multiple action target info
	BASE_OBJECT		*psObj;
	BASE_STATS		*psStats;
} DROID_ACTION_DATA;

// Check if a droid has stoped moving
#define DROID_STOPPED(psDroid) \
	(psDroid->sMove.Status == MOVEINACTIVE || psDroid->sMove.Status == MOVEHOVER || \
	 psDroid->sMove.Status == MOVESHUFFLE)

const char* getDroidActionName(DROID_ACTION action)
{
	static const char* name[] =
	{
		"DACTION_NONE",					// not doing anything
		"DACTION_MOVE",					// 1 moving to a location
		"DACTION_BUILD",					// building a structure
		"DACTION_BUILD_FOUNDATION",		// 3 building a foundation for a structure
		"DACTION_DEMOLISH",				// demolishing a structure
		"DACTION_REPAIR",					// 5 repairing a structure
		"DACTION_ATTACK",					// attacking something
		"DACTION_OBSERVE",				// 7 observing something
		"DACTION_FIRESUPPORT",			// attacking something visible by a sensor droid
		"DACTION_SULK",					// 9 refuse to do anything aggresive for a fixed time
		"DACTION_DESTRUCT",				// self destruct
		"DACTION_TRANSPORTOUT",			// 11 move transporter offworld
		"DACTION_TRANSPORTWAITTOFLYIN",	// wait for timer to move reinforcements in
		"DACTION_TRANSPORTIN",			// 13 move transporter onworld
		"DACTION_DROIDREPAIR",			// repairing a droid
		"DACTION_RESTORE",				// 15 restore resistance points of a structure
		"DACTION_CLEARWRECK",				// clearing building wreckage
		"DACTION_MOVEFIRE",				// 17
		"DACTION_MOVETOBUILD",			// moving to a new building location
		"DACTION_MOVETODEMOLISH",			// 19 moving to a new demolition location
		"DACTION_MOVETOREPAIR",			// moving to a new repair location
		"DACTION_BUILDWANDER",			// 21 moving around while building
		"DACTION_FOUNDATION_WANDER",		// moving around while building the foundation
		"DACTION_MOVETOATTACK",			// 23 moving to a target to attack
		"DACTION_ROTATETOATTACK",			// rotating to a target to attack
		"DACTION_MOVETOOBSERVE",			// 25 moving to be able to see a target
		"DACTION_WAITFORREPAIR",			// waiting to be repaired by a facility
		"DACTION_MOVETOREPAIRPOINT",		// 27 move to repair facility repair point
		"DACTION_WAITDURINGREPAIR",		// waiting to be repaired by a facility
		"DACTION_MOVETODROIDREPAIR",		// 29 moving to a new location next to droid to be repaired
		"DACTION_MOVETORESTORE",			// moving to a low resistance structure
		"DACTION_MOVETOCLEAR",			// 31 moving to a building wreck location
		"DACTION_MOVETOREARM",			// (32)moving to a rearming pad - VTOLS
		"DACTION_WAITFORREARM",			// (33)waiting for rearm - VTOLS
		"DACTION_MOVETOREARMPOINT",		// (34)move to rearm point - VTOLS - this actually moves them onto the pad
		"DACTION_WAITDURINGREARM",		// (35)waiting during rearm process- VTOLS
		"DACTION_VTOLATTACK",				// (36) a VTOL droid doing attack runs
		"DACTION_CLEARREARMPAD",			// (37) a VTOL droid being told to get off a rearm pad
		"DACTION_RETURNTOPOS",			// (38) used by scout/patrol order when returning to route
		"DACTION_FIRESUPPORT_RETREAT",	// (39) used by firesupport order when sensor retreats
		"ACTION UNKNOWN",
		"DACTION_CIRCLE"				// (41) circling while engaging
	};

	ASSERT(action < sizeof(name) / sizeof(name[0]), "DROID_ACTION out of range: %u", action);

	return name[action];
}

/* Check if a target is at correct range to attack */
BOOL actionInAttackRange(DROID *psDroid, BASE_OBJECT *psObj, int weapon_slot)
{
	SDWORD			dx, dy, dz, radSq, rangeSq, longRange;
	SECONDARY_STATE state;
	WEAPON_STATS	*psStats;

	CHECK_DROID(psDroid);
	if (psDroid->asWeaps[0].nStat == 0)
	{
		return false;
	}

	dx = (SDWORD)psDroid->pos.x - (SDWORD)psObj->pos.x;
	dy = (SDWORD)psDroid->pos.y - (SDWORD)psObj->pos.y;
	dz = (SDWORD)psDroid->pos.z - (SDWORD)psObj->pos.z;

	radSq = dx*dx + dy*dy;

	psStats = asWeaponStats + psDroid->asWeaps[weapon_slot].nStat;

	if (psDroid->order == DORDER_ATTACKTARGET
	    && secondaryGetState(psDroid, DSO_HALTTYPE, &state)
	    && state == DSS_HALT_HOLD)
	{
		longRange = proj_GetLongRange(psStats);
		rangeSq = longRange * longRange;
	}
	else
	{
		switch (psDroid->secondaryOrder & DSS_ARANGE_MASK)
		{
		case DSS_ARANGE_DEFAULT:
			//if (psStats->shortHit > psStats->longHit)
			if (weaponShortHit(psStats, psDroid->player) > weaponLongHit(psStats, psDroid->player))
			{
				rangeSq = psStats->shortRange * psStats->shortRange;
			}
			else
			{
				longRange = proj_GetLongRange(psStats);
				rangeSq = longRange * longRange;
			}
			break;
		case DSS_ARANGE_SHORT:
			rangeSq = psStats->shortRange * psStats->shortRange;
			break;
		case DSS_ARANGE_LONG:
			longRange = proj_GetLongRange(psStats);
			rangeSq = longRange * longRange;
			break;
		default:
			ASSERT(!"unknown attackrange order", "actionInAttackRange: unknown attack range order");
			longRange = proj_GetLongRange(psStats);
			rangeSq = longRange * longRange;
			break;
		}
	}

	/* check max range */
	if ( radSq <= rangeSq )
	{
		/* check min range */
		rangeSq = psStats->minRange * psStats->minRange;
		if ( radSq >= rangeSq || !proj_Direct( psStats ) )
		{
			return true;
		}
	}

	return false;
}


// check if a target is within weapon range
BOOL actionInRange(DROID *psDroid, BASE_OBJECT *psObj, int weapon_slot)
{
	SDWORD			dx, dy, dz, radSq, rangeSq, longRange;
	WEAPON_STATS	*psStats;

	CHECK_DROID(psDroid);

	if (psDroid->asWeaps[0].nStat == 0)
	{
		return false;
	}

	psStats = asWeaponStats + psDroid->asWeaps[weapon_slot].nStat;

	dx = (SDWORD)psDroid->pos.x - (SDWORD)psObj->pos.x;
	dy = (SDWORD)psDroid->pos.y - (SDWORD)psObj->pos.y;
	dz = (SDWORD)psDroid->pos.z - (SDWORD)psObj->pos.z;

	radSq = dx*dx + dy*dy;

	longRange = proj_GetLongRange(psStats);
	rangeSq = longRange * longRange;

	/* check max range */
	if ( radSq <= rangeSq )
	{
		/* check min range */
		rangeSq = psStats->minRange * psStats->minRange;
		if ( radSq >= rangeSq || !proj_Direct( psStats ) )
		{
			return true;
		}
	}

	return false;
}



// check if a target is inside minimum weapon range
BOOL actionInsideMinRange(DROID *psDroid, BASE_OBJECT *psObj, int weapon_slot)
{
	SDWORD			dx, dy, dz, radSq, rangeSq, minRange;
	WEAPON_STATS	*psStats;

	CHECK_DROID(psDroid);

	/* Watermelon:if I am a multi-turret droid */
	if (psDroid->asWeaps[0].nStat == 0)
	{
		return false;
	}

	psStats = asWeaponStats + psDroid->asWeaps[weapon_slot].nStat;

	dx = (SDWORD)psDroid->pos.x - (SDWORD)psObj->pos.x;
	dy = (SDWORD)psDroid->pos.y - (SDWORD)psObj->pos.y;
	dz = (SDWORD)psDroid->pos.z - (SDWORD)psObj->pos.z;

	radSq = dx*dx + dy*dy;

	minRange = (SDWORD)psStats->minRange;
	rangeSq = minRange * minRange;

	// check min range
	if ( radSq <= rangeSq )
	{
		return true;
	}

	return false;
}


// Realign turret
void actionAlignTurret(BASE_OBJECT *psObj, int weapon_slot)
{
	UDWORD				rotation;
	UWORD				nearest = 0;
	UWORD				tRot;
	UWORD				tPitch;

	//Watermelon:default turret rotation 0
	tRot = 0;

	//get the maximum rotation this frame
	rotation = timeAdjustedIncrement(ACTION_TURRET_ROTATION_RATE, true);
	if (rotation == 0)
	{
		rotation = 1;
	}

	switch (psObj->type)
	{
	case OBJ_DROID:
		tRot = ((DROID *)psObj)->turretRotation[weapon_slot];
		tPitch = ((DROID *)psObj)->turretPitch[weapon_slot];
		break;
	case OBJ_STRUCTURE:
		tRot = ((STRUCTURE *)psObj)->turretRotation[weapon_slot];
		tPitch = ((STRUCTURE *)psObj)->turretPitch[weapon_slot];

		// now find the nearest 90 degree angle
		nearest = (UWORD)(((tRot + 45) / 90) * 90);
		tRot = (UWORD)(((tRot + 360) - nearest) % 360);
		break;
	default:
		ASSERT(!"invalid object type", "actionAlignTurret: invalid object type");
		return;
		break;
	}

	if (rotation > 180)//crop to 180 degrees, no point in turning more than all the way round
	{
		rotation = 180;
	}
	if (tRot < 180)// +ve angle 0 - 179 degrees
	{
		if (tRot > rotation)
		{
			tRot = (UWORD)(tRot - rotation);
		}
		else
		{
			tRot = 0;
		}
	}
	else //angle greater than 180 rotate in opposite direction
	{
		if (tRot < (360 - rotation))
		{
			tRot = (UWORD)(tRot + rotation);
		}
		else
		{
			tRot = 0;
		}
	}
	tRot %= 360;

	// align the turret pitch
	if (tPitch < 180)// +ve angle 0 - 179 degrees
	{
		if (tPitch > rotation/2)
		{
			tPitch = (UWORD)(tPitch - rotation/2);
		}
		else
		{
			tPitch = 0;
		}
	}
	else // -ve angle rotate in opposite direction
	{
		if (tPitch < (360 -(SDWORD)rotation/2))
		{
			tPitch = (UWORD)(tPitch + rotation/2);
		}
		else
		{
			tPitch = 0;
		}
	}
	tPitch %= 360;

	switch (psObj->type)
	{
	case OBJ_DROID:
		((DROID *)psObj)->turretRotation[weapon_slot] = tRot;
		((DROID *)psObj)->turretPitch[weapon_slot] = tPitch;
		break;
	case OBJ_STRUCTURE:
		// now adjust back to the nearest 90 degree angle
		tRot = (UWORD)((tRot + nearest) % 360);

		((STRUCTURE *)psObj)->turretRotation[weapon_slot] = tRot;
		((STRUCTURE *)psObj)->turretPitch[weapon_slot] = tPitch;
		break;
	default:
		ASSERT(!"invalid object type", "actionAlignTurret: invalid object type");
		return;
		break;
	}
}

/* returns true if on target */
BOOL actionTargetTurret(BASE_OBJECT *psAttacker, BASE_OBJECT *psTarget, UWORD *pRotation,
		UWORD *pPitch, WEAPON_STATS *psWeapStats, BOOL bInvert, int weapon_slot)
{
	SWORD  tRotation, tPitch, rotRate, pitchRate;
	SDWORD  targetRotation,targetPitch;
	SDWORD  pitchError;
	SDWORD	rotationError, dx, dy, dz;
	BOOL	onTarget = false;
	float	fR;
	SDWORD	pitchLowerLimit, pitchUpperLimit;
	DROID	*psDroid = NULL;

	// these are constants now and can be set up at the start of the function
	rotRate = ACTION_TURRET_ROTATION_RATE * 4;
	pitchRate = ACTION_TURRET_ROTATION_RATE * 2;

	if (psWeapStats)
	{
		// extra heavy weapons on some structures need to rotate and pitch more slowly
		if (psWeapStats->weight > HEAVY_WEAPON_WEIGHT)
		{
			UDWORD excess = 100 * (psWeapStats->weight - HEAVY_WEAPON_WEIGHT) / psWeapStats->weight;

			rotRate = ACTION_TURRET_ROTATION_RATE * 2 - excess;
			pitchRate = (SWORD) (rotRate / 2);
		}
	}

	tRotation = *pRotation;
	tPitch = *pPitch;

	//set the pitch limits based on the weapon stats of the attacker
	pitchLowerLimit = pitchUpperLimit = 0;
	if (psAttacker->type == OBJ_STRUCTURE)
	{
		pitchLowerLimit = asWeaponStats[((STRUCTURE *)psAttacker)->asWeaps[weapon_slot].
			nStat].minElevation;
		pitchUpperLimit = asWeaponStats[((STRUCTURE *)psAttacker)->asWeaps[weapon_slot].
			nStat].maxElevation;
	}
	else if (psAttacker->type == OBJ_DROID)
	{
		psDroid = (DROID *) psAttacker;
		if ( (psDroid->droidType == DROID_WEAPON) ||
			 (psDroid->droidType == DROID_TRANSPORTER) ||
			 (psDroid->droidType == DROID_COMMAND) ||
			 (psDroid->droidType == DROID_CYBORG) ||
			 (psDroid->droidType == DROID_CYBORG_SUPER) )
		{
			pitchLowerLimit = asWeaponStats[psDroid->asWeaps[weapon_slot].
				nStat].minElevation;
			pitchUpperLimit = asWeaponStats[psDroid->asWeaps[weapon_slot].
				nStat].maxElevation;
		}
		else if ( psDroid->droidType == DROID_REPAIR )
		{
			pitchLowerLimit = REPAIR_PITCH_LOWER;
			pitchUpperLimit = REPAIR_PITCH_UPPER;
		}
	}

	//get the maximum rotation this frame
	rotRate = timeAdjustedIncrement(rotRate, true);
	if (rotRate > 180)//crop to 180 degrees, no point in turning more than all the way round
	{
		rotRate = 180;
	}
	if (rotRate <= 0)
	{
		rotRate = 1;
	}
	pitchRate = timeAdjustedIncrement(pitchRate, true);
	if (pitchRate > 180)//crop to 180 degrees, no point in turning more than all the way round
	{
		pitchRate = 180;
	}
	if (pitchRate <= 0)
	{
		pitchRate = 1;
	}

/*	if ( (psAttacker->type == OBJ_STRUCTURE) &&
		 (((STRUCTURE *)psAttacker)->pStructureType->type == REF_DEFENSE) &&
		 (asWeaponStats[((STRUCTURE *)psAttacker)->asWeaps[0].nStat].surfaceToAir == SHOOT_IN_AIR) )
	{
		rotRate = 180;
		pitchRate = 180;
	}*/

	//and point the turret at target
	targetRotation = calcDirection(psAttacker->pos.x, psAttacker->pos.y, psTarget->pos.x, psTarget->pos.y);

	rotationError = targetRotation - (tRotation + psAttacker->direction);

	//restrict rotationerror to =/- 180 degrees
	while (rotationError > 180)
	{
		rotationError -= 360;
	}

	while (rotationError < -180)
	{
		rotationError += 360;
	}

	if  (-rotationError > (SDWORD)rotRate)
	{
		// subtract rotation
		if (tRotation < rotRate)
		{
			tRotation = (SWORD)(tRotation + 360 - rotRate);
		}
		else
		{
			tRotation = (SWORD)(tRotation - rotRate);
		}
	}
	else if  (rotationError > (SDWORD)rotRate)
	{
		// add rotation
		tRotation = (SWORD)(tRotation + rotRate);
		tRotation %= 360;
	}
	else //roughly there so lock on and fire
	{
		if ( (SDWORD)psAttacker->direction > targetRotation )
		{
			tRotation = (SWORD)(targetRotation + 360 - psAttacker->direction);
		}
		else
		{
			tRotation = (SWORD)(targetRotation - psAttacker->direction);
		}
// 			debug( LOG_NEVER, "locked on target...\n");
		onTarget = true;
	}
	tRotation %= 360;

	if ((psAttacker->type == OBJ_DROID) &&
		vtolDroid((DROID *)psAttacker))
	{
		// limit the rotation for vtols
		if ((tRotation <= 180) && (tRotation > VTOL_TURRET_LLIMIT))
		{
			tRotation = VTOL_TURRET_LLIMIT;
		}
		else if ((tRotation > 180) && (tRotation < VTOL_TURRET_RLIMIT))
		{
			tRotation = VTOL_TURRET_RLIMIT;
		}
	}

	/* set muzzle pitch if direct fire */
//	if ( asWeaponStats[psAttacker->asWeaps->nStat].direct == true )
	if ( psWeapStats != NULL &&
		 ( proj_Direct( psWeapStats ) ||
		 ( (psAttacker->type == OBJ_DROID) &&
			!proj_Direct( psWeapStats ) &&
			actionInsideMinRange(psDroid, psTarget, weapon_slot) ) ) )
	{
// difference between muzzle position and droid origin is unlikely to affect aiming
// particularly as target origin is used
//		calcDroidMuzzleLocation( psAttacker, &muzzle);
		dx = psTarget->pos.x - psAttacker->pos.x;//muzzle.x;
		dy = psTarget->pos.y - psAttacker->pos.y;//muzzle.y;
//		dz = map_Height(psTarget->pos.x, psTarget->pos.y) - psAttacker->pos.z;//muzzle.z;
		dz = psTarget->pos.z - psAttacker->pos.z;//muzzle.z;

		/* get target distance */
		fR = trigIntSqrt( dx*dx + dy*dy );

		targetPitch = (SDWORD)( RAD_TO_DEG(atan2(dz, fR)));
		//tPitch = tPitch;
		if (tPitch > 180)
		{
			tPitch -=360;
		}

		/* invert calculations for bottom-mounted weapons (i.e. for vtols) */
		if ( bInvert )
		{
			tPitch = (SWORD)(-tPitch);
			targetPitch = -targetPitch;
		}

		pitchError = targetPitch - tPitch;

		if (pitchError < -pitchRate)
		{
			// move down
			tPitch = (SWORD)(tPitch - pitchRate);
			onTarget = false;
		}
		else if (pitchError > pitchRate)
		{
			// add rotation
			tPitch = (SWORD)(tPitch + pitchRate);
			onTarget = false;
		}
		else //roughly there so lock on and fire
		{
			tPitch = (SWORD)targetPitch;
		}

		/* re-invert result for bottom-mounted weapons (i.e. for vtols) */
		if ( bInvert )
		{
			tPitch = (SWORD)-tPitch;
		}

		if (tPitch < pitchLowerLimit)
		{
			// move down
			tPitch = (SWORD)pitchLowerLimit;
			onTarget = false;
		}
		else if (tPitch > pitchUpperLimit)
		{
			// add rotation
			tPitch = (SWORD)pitchUpperLimit;
			onTarget = false;
		}

		if (tPitch < 0)
		{
			tPitch += 360;
		}
	}

	*pRotation = tRotation;
	*pPitch = tPitch;

// 	debug( LOG_NEVER, "endrot=%d\n",tRotation);

	return onTarget;
}


// return whether a droid can see a target to fire on it
BOOL actionVisibleTarget(DROID *psDroid, BASE_OBJECT *psTarget, int weapon_slot)
{
	WEAPON_STATS	*psStats;

	CHECK_DROID(psDroid);
	ASSERT(psTarget != NULL, "actionVisibleTarget: Target is NULL");
	if (!psTarget)
	{
		return false;
	}

	if (psDroid->numWeaps == 0)
	{
		if ( visibleObject((BASE_OBJECT*)psDroid, psTarget) )
		{
			return true;
		}
	}

	if (vtolDroid(psDroid))
	{
		if ( visibleObject((BASE_OBJECT*)psDroid, psTarget) )
		{
			return true;
		}
		return false;
	}

	psStats = asWeaponStats + psDroid->asWeaps[weapon_slot].nStat;
	if (proj_Direct(psStats))
	{
		if (visibleObjWallBlock((BASE_OBJECT*)psDroid, psTarget))
		{
			return true;
		}
	}
	else
	{
		// indirect can only attack things they can see unless attacking
		// through a sensor droid - see DORDER_FIRESUPPORT
		if (orderState(psDroid, DORDER_FIRESUPPORT))
		{
			if (psTarget->visible[psDroid->player])
			{
				return true;
			}
		}
		else
		{
			if (visibleObject((BASE_OBJECT*)psDroid, psTarget))
			{
				return true;
			}
		}
	}

	return false;
}

static void actionAddVtolAttackRun( DROID *psDroid )
{
	double      fA;
	SDWORD		deltaX, deltaY, iA, iX, iY;
	BASE_OBJECT	*psTarget;
#if 0
	SDWORD		iVx, iVy;
#endif

	CHECK_DROID(psDroid);

	if ( psDroid->psActionTarget[0] != NULL )
	{
		psTarget = psDroid->psActionTarget[0];
	}
	else if ( psDroid->psTarget != NULL )
	{
		psTarget = psDroid->psTarget;
	}
	else
	{
		return;
	}

	/* get normal vector from droid to target */
	deltaX = psTarget->pos.x - psDroid->pos.x;
	deltaY = psTarget->pos.y - psDroid->pos.y;

	/* get magnitude of normal vector (Pythagorean theorem) */
	fA = trigIntSqrt( deltaX*deltaX + deltaY*deltaY );
	iA = fA;

#if 0
	/* get left perpendicular to normal vector:
	 * swap normal vector elements and negate y:
	 * scale to attack ellipse width
	 */
	iVx =  deltaY * VTOL_ATTACK_WIDTH / iA;
	iVy = -deltaX * VTOL_ATTACK_WIDTH / iA;

	/* add waypoint left perpendicular to target*/
	iX = psTarget->pos.x + iVx;
	iY = psTarget->pos.y + iVy;
#endif

	/* add waypoint behind target attack length away*/
	if (iA != 0)
	{
		iX = psTarget->pos.x + (deltaX * VTOL_ATTACK_LENGTH / iA);
		iY = psTarget->pos.y + (deltaY * VTOL_ATTACK_LENGTH / iA);
	}
	else
	{
		// We should only ever get here if both deltaX and deltaY
		// are zero (look at the above pythagoeran theorem).
		// This code is here to prevent a divide by zero error
		//
		// The next values are valid because if both deltas are zero
		// then iA will be zero as well resulting in:
		// (deltaXY * VTOL_ATTACK_LENGTH / iA) = (0 * VTOL_ATTACK_LENGTH / 0) = (0 / 0) = 0
		iX = psTarget->pos.x;
		iY = psTarget->pos.y;
	}

	if (iX <= 0
	 || iY<=0
	 || iX > world_coord(GetWidthOfMap())
	 || iY > world_coord(GetHeightOfMap()))
	{
		debug( LOG_NEVER, "*** actionAddVtolAttackRun: run off map! ***\n" );
	}
	else
	{
		moveDroidToDirect( psDroid, iX, iY );
	}
}

static void actionUpdateVtolAttack( DROID *psDroid )
{
	WEAPON_STATS	*psWeapStats[DROID_MAXWEAPS];
	UBYTE i;

	CHECK_DROID(psDroid);

	/* don't do attack runs whilst returning to base */
	if ( psDroid->order == DORDER_RTB )
	{
		return;
	}

	/* Watermelon:if I am a multi-turret droid */
	if (psDroid->numWeaps > 1)
	{
		for(i = 0;i < psDroid->numWeaps;i++)
		{
			if (psDroid->asWeaps[i].nStat != 0)
			{
				psWeapStats[i] = asWeaponStats + psDroid->asWeaps[i].nStat;
				ASSERT( psWeapStats != NULL,
				"actionUpdateVtolAttack: invalid weapon stats pointer" );
				break;
			}
		}
	}
	else
	{
		if (psDroid->asWeaps[0].nStat > 0)
		{
			psWeapStats[0] = asWeaponStats + psDroid->asWeaps[0].nStat;
			ASSERT( psWeapStats != NULL,
				"actionUpdateVtolAttack: invalid weapon stats pointer" );
		}
	}

	/* order back to base after fixed number of attack runs */
	if ( psWeapStats[0] != NULL )
	{
		if (vtolEmpty(psDroid))
		{
			moveToRearm(psDroid);
			return;
		}
	}

	/* circle around target if hovering and not cyborg */
	if (psDroid->sMove.Status == MOVEHOVER && !cyborgDroid(psDroid))
	{
		actionAddVtolAttackRun( psDroid );
	}
}

static void actionUpdateTransporter( DROID *psDroid )
{
	CHECK_DROID(psDroid);

	//check if transporter has arrived
	if (updateTransporter(psDroid))
	{
		// Got to destination
		psDroid->action = DACTION_NONE;
		return;
	}
}


// calculate a position for units to pull back to if they
// need to increase the range between them and a target
static void actionCalcPullBackPoint(BASE_OBJECT *psObj, BASE_OBJECT *psTarget, SDWORD *px, SDWORD *py)
{
	SDWORD xdiff,ydiff, len;

	// get the vector from the target to the object
	xdiff = (SDWORD)psObj->pos.x - (SDWORD)psTarget->pos.x;
	ydiff = (SDWORD)psObj->pos.y - (SDWORD)psTarget->pos.y;
	len = (SDWORD)sqrtf(xdiff*xdiff + ydiff*ydiff);

	if (len == 0)
	{
		xdiff = TILE_UNITS;
		ydiff = TILE_UNITS;
	}
	else
	{
		xdiff = (xdiff * TILE_UNITS) / len;
		ydiff = (ydiff * TILE_UNITS) / len;
	}

	// create the position
	*px = (SDWORD)psObj->pos.x + xdiff * PULL_BACK_DIST;
	*py = (SDWORD)psObj->pos.y + ydiff * PULL_BACK_DIST;

	// make sure coordinates stay inside of the map
	clip_world_offmap(px, py);
}


// check whether a droid is in the neighboring tile to a build position
BOOL actionReachedBuildPos(DROID *psDroid, SDWORD x, SDWORD y, BASE_STATS *psStats)
{
	SDWORD		width, breadth, tx,ty, dx,dy;

	CHECK_DROID(psDroid);

	// do all calculations in half tile units so that
	// the droid moves to within half a tile of the target
	// NOT ANY MORE - JOHN
	dx = map_coord(psDroid->pos.x);
	dy = map_coord(psDroid->pos.y);

	if (StatIsStructure(psStats))
	{
		width = ((STRUCTURE_STATS *)psStats)->baseWidth;
		breadth = ((STRUCTURE_STATS *)psStats)->baseBreadth;
	}
	else
	{
		width = ((FEATURE_STATS *)psStats)->baseWidth;
		breadth = ((FEATURE_STATS *)psStats)->baseBreadth;
	}

	tx = map_coord(x);
	ty = map_coord(y);

	// move the x,y to the top left of the structure
	tx -= width/2;
	ty -= breadth/2;

	if ( (dx == (tx -1)) || (dx == (tx + width)) )
	{
		// droid could be at either the left or the right
		if ( (dy >= (ty -1)) && (dy <= (ty + breadth)) )
		{
			return true;
		}
	}
	else if ( (dy == (ty -1)) || (dy == (ty + breadth)) )
	{
		// droid could be at either the top or the bottom
		if ( (dx >= (tx -1)) && (dx <= (tx + width)) )
		{
			return true;
		}
	}

	return false;
}


// check if a droid is on the foundations of a new building
BOOL actionDroidOnBuildPos(DROID *psDroid, SDWORD x, SDWORD y, BASE_STATS *psStats)
{
	SDWORD	width, breadth, tx,ty, dx,dy;

	CHECK_DROID(psDroid);

	dx = map_coord(psDroid->pos.x);
	dy = map_coord(psDroid->pos.y);
	if (StatIsStructure(psStats))
	{
		width = ((STRUCTURE_STATS *)psStats)->baseWidth;
		breadth = ((STRUCTURE_STATS *)psStats)->baseBreadth;
	}
	else
	{
		width = ((FEATURE_STATS *)psStats)->baseWidth;
		breadth = ((FEATURE_STATS *)psStats)->baseBreadth;
	}

	tx = map_coord(x) - (width / 2);
	ty = map_coord(y) - (breadth / 2);

	if (dx >= tx
	 && dx < tx + width
	 && dy >= ty
	 && dy < ty + breadth)
	{
		return true;
	}

	return false;
}


// return the position of a players home base
// MAY return an invalid (0, 0) position on MP maps!
static void actionHomeBasePos(SDWORD player, SDWORD *px, SDWORD *py)
{
	STRUCTURE	*psStruct;

	ASSERT( player >= 0 && player < MAX_PLAYERS,
		"actionHomeBasePos: invalide player number" );

	for(psStruct = apsStructLists[player]; psStruct; psStruct=psStruct->psNext)
	{
		if (psStruct->pStructureType->type == REF_HQ)
		{
			*px = (SDWORD)psStruct->pos.x;
			*py = (SDWORD)psStruct->pos.y;
			return;
		}
	}

	*px = getLandingX(player);
	*py = getLandingY(player);
}


// tell the action system of a potential location for walls blocking routing
BOOL actionRouteBlockingPos(DROID *psDroid, SDWORD tx, SDWORD ty)
{
	SDWORD		i,j;
	MAPTILE		*psTile;
	STRUCTURE	*psWall;

	CHECK_DROID(psDroid);

	if (vtolDroid(psDroid) ||
		((psDroid->order != DORDER_MOVE) &&
		 (psDroid->order != DORDER_SCOUT)))
	{
		return false;
	}

	// see if there is a wall to attack around the location
	psWall = NULL;
	for(i= tx -1; i <= tx + 1; i++)
	{
		for(j= ty -1; j <= ty + 1; j++)
		{
			if (tileOnMap(i,j))
			{
				psTile = mapTile(i,j);
				if (TILE_HAS_WALL(psTile))
				{
					psWall = getTileStructure((UDWORD)i,(UDWORD)j);
					//Watermelon:fixes AI try to destroy ally's wall bug
					if (psWall->player != psDroid->player &&
						!aiCheckAlliances(psWall->player, psDroid->player))
					{
						goto done;
					}
					else
					{
						psWall = NULL;
					}
				}
			}
		}
	}

done:
	if (psWall != NULL)
	{
		if (psDroid->order == DORDER_MOVE)
		{
			psDroid->order = DORDER_MOVE_ATTACKWALL;
		}
		else if (psDroid->order == DORDER_SCOUT)
		{
			psDroid->order = DORDER_SCOUT_ATTACKWALL;
		}
		setDroidTarget(psDroid, (BASE_OBJECT *)psWall);
		return true;
	}

	return false;
}

#define	VTOL_ATTACK_AUDIO_DELAY		(3*GAME_TICKS_PER_SEC)

// Update the action state for a droid
void actionUpdateDroid(DROID *psDroid)
{
	UDWORD				droidX,droidY;
	UDWORD				tlx,tly;
	STRUCTURE			*psStruct;
	STRUCTURE_STATS		*psStructStats;
	BASE_OBJECT			*psTarget;
	WEAPON_STATS		*psWeapStats;
	SDWORD				targetDir, dirDiff, pbx,pby;
	SDWORD				xdiff,ydiff, rangeSq;
	SECONDARY_STATE			state;
	PROPULSION_STATS	*psPropStats;
	BOOL				bChaseBloke, bInvert;
	FEATURE				*psNextWreck;
	BOOL				(*actionUpdateFunc)(DROID *psDroid) = NULL;
	SDWORD				moveAction;
	BOOL				bDoHelpBuild;
	MAPTILE				*psTile;
	UBYTE	 i = 0;
	//this is a bit field
	UBYTE	 num_weapons = 0;
	BASE_OBJECT			*psTargets[DROID_MAXWEAPS];
	UBYTE	j,iVisible = 1;
	BOOL	bHasTarget;

	CHECK_DROID(psDroid);

	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	ASSERT( psPropStats != NULL,
			"actionUpdateUnit: invalid propulsion stats pointer" );

	/* check whether turret inverted for actionTargetTurret */
	//if ( psDroid->droidType != DROID_CYBORG &&
	if ( !cyborgDroid(psDroid) &&
		psPropStats->propulsionType == LIFT   )
	{
		bInvert = true;
	}
	else
	{
		bInvert = false;
	}

	// clear the target if it has died
	// weapon droid
	if (psDroid->numWeaps > 0)
	{
		// clear targets
		for (i = 0;i < psDroid->numWeaps;i++)
		{
			if (psDroid->psActionTarget[i] && psDroid->psActionTarget[i]->died)
			{
				setDroidActionTarget(psDroid, NULL, i);
				if (i == 0)
				{
					if ( (psDroid->action != DACTION_MOVEFIRE) &&
						(psDroid->action != DACTION_TRANSPORTIN) &&
						(psDroid->action != DACTION_TRANSPORTOUT)   )
					{
						psDroid->action = DACTION_NONE;
						//if Vtol - return to rearm pad
						if (vtolDroid(psDroid))
						{
							moveToRearm(psDroid);
						}
					}
				}
			}
		}
	}
	//utility droid
	else
	{
		if (psDroid->psActionTarget[0] && psDroid->psActionTarget[0]->died)
		{
			setDroidActionTarget(psDroid, NULL, 0);
			if ( (psDroid->action != DACTION_MOVEFIRE) &&
				(psDroid->action != DACTION_TRANSPORTIN) &&
				(psDroid->action != DACTION_TRANSPORTOUT)   )
			{
				psDroid->action = DACTION_NONE;
				//if Vtol - return to rearm pad
				if (vtolDroid(psDroid))
				{
					moveToRearm(psDroid);
				}
			}
		}
	}

	//if the droid has been attacked by an EMP weapon, it is temporarily disabled
	if (psDroid->lastHitWeapon == WSC_EMP)
	{
		if (gameTime - psDroid->timeLastHit > EMP_DISABLE_TIME)
		{
			//the actionStarted time needs to be adjusted
			psDroid->actionStarted += (gameTime - psDroid->timeLastHit);
			//reset the lastHit parameters
			psDroid->timeLastHit = 0;
			psDroid->lastHitWeapon = UDWORD_MAX;
		}
		else
		{
			//get out without updating
			return;
		}
	}

	//if (psDroid->numWeaps == 0)
	/* Watermelon:if I am a multi-turret droid */
	if (psDroid->numWeaps > 1)
	{
		for(i = 0;i < psDroid->numWeaps;i++)
		{
			if (psDroid->asWeaps[i].nStat > 0)
			{
				num_weapons += (1 << (i+1));
			}
		}
	}
	else
	{
		num_weapons = 2;
	}

	psTarget = psDroid->psTarget;

	//Watermelon:safety check
	//if (psDroid->numWeaps > 0)
	if (psDroid->asWeaps[0].nStat > 0)
	{
		psWeapStats = asWeaponStats + psDroid->asWeaps[0].nStat;
	}
	else
	{
		psWeapStats = NULL;
	}

	switch (psDroid->action)
	{
	case DACTION_NONE:
	case DACTION_WAITFORREPAIR:
		// doing nothing
		//since not doing anything, see if need to self repair
		if (selfRepairEnabled(psDroid->player))
		{
			//wait for 1 second to give the repair facility a chance to do the repair work
			if (gameTime - psDroid->actionStarted > GAME_TICKS_PER_SEC)
			{
				droidSelfRepair(psDroid);
			}
		}
		break;
	case DACTION_WAITDURINGREPAIR:
		// don't want to be in a formation for this move
		if (psDroid->sMove.psFormation != NULL)
		{
			formationLeave(psDroid->sMove.psFormation, psDroid);
			psDroid->sMove.psFormation = NULL;
		}
		// move back to the repair facility if necessary
		if (DROID_STOPPED(psDroid) &&
			!actionReachedBuildPos(psDroid,
						(SDWORD)psDroid->psTarget->pos.x,(SDWORD)psDroid->psTarget->pos.y,
						(BASE_STATS *)((STRUCTURE*)psDroid->psTarget)->pStructureType ) )
		{
			moveDroidToNoFormation(psDroid, psDroid->psTarget->pos.x, psDroid->psTarget->pos.y);
		}
		break;
	case DACTION_TRANSPORTWAITTOFLYIN:
		//if we're moving droids to safety and currently waiting to fly back in, see if time is up
		if (psDroid->player == selectedPlayer && getDroidsToSafetyFlag())
		{
			if ((SDWORD)(mission.ETA - (gameTime - missionGetReinforcementTime())) <= 0)
			{
				if (!droidRemove(psDroid, mission.apsDroidLists))
				{
					ASSERT( false, "actionUpdate: Unable to remove transporter from mission list" );
				}
				addDroid(psDroid, apsDroidLists);
				//set the x/y up since they were set to INVALID_XY when moved offWorld
				missionGetTransporterExit(selectedPlayer, &droidX, &droidY);
				psDroid->pos.x = droidX;
				psDroid->pos.y = droidY;
				//fly Transporter back to get some more droids
				orderDroidLoc( psDroid, DORDER_TRANSPORTIN,
					getLandingX(selectedPlayer), getLandingY(selectedPlayer));
			}
			else
			{
				/*if we're currently moving units to safety and waiting to fly
				back in - check there is something to fly back for!*/
				if (!missionDroidsRemaining(selectedPlayer))
				{
					//the script can call startMission for this callback for offworld missions
					eventFireCallbackTrigger((TRIGGER_TYPE)CALL_START_NEXT_LEVEL);
				}
			}
		}
		break;

	case DACTION_MOVE:
		// moving to a location
		if (DROID_STOPPED(psDroid))
		{
			// Got to destination
			psDroid->action = DACTION_NONE;

			/* notify scripts we have reached the destination
			*  also triggers when patrolling and reached a waypoint
			*/
			psScrCBOrder = psDroid->order;
			psScrCBOrderDroid = psDroid;
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DROID_REACH_LOCATION);
			psScrCBOrderDroid = NULL;
			psScrCBOrder = DORDER_NONE;
		}

		//Watermelon:added multiple weapon check
		else if (psDroid->numWeaps > 0)
		{
			for(i = 0;i < psDroid->numWeaps;i++)
			{
				if ( (num_weapons & (1 << (i+1))) )
				{
					BASE_OBJECT *psTemp = NULL;

					//Watermelon:I moved psWeapStats flag update there
					psWeapStats = asWeaponStats + psDroid->asWeaps[i].nStat;
					if (!vtolDroid(psDroid) &&
						psDroid->asWeaps[i].nStat > 0 &&
						psWeapStats->rotate &&
						psWeapStats->fireOnMove != FOM_NO &&
						CAN_UPDATE_NAYBORS(psDroid) &&
						(aiBestNearestTarget(psDroid, &psTemp, i) >= 0))
					{
						if (secondaryGetState(psDroid, DSO_ATTACK_LEVEL, &state))
						{
							if (state == DSS_ALEV_ALWAYS)
							{
								psDroid->action = DACTION_MOVEFIRE;
							}
						}
						else
						{
							psDroid->action = DACTION_MOVEFIRE;
						}
					}
					setDroidActionTarget(psDroid, psTemp, 0);
				}
			}
		}
		break;
	case DACTION_RETURNTOPOS:
	case DACTION_FIRESUPPORT_RETREAT:
		if (DROID_STOPPED(psDroid))
		{
			// Got to destination
			psDroid->action = DACTION_NONE;
		}
		break;
	case DACTION_TRANSPORTIN:
	case DACTION_TRANSPORTOUT:
		actionUpdateTransporter( psDroid );
		break;
	case DACTION_MOVEFIRE:
		//check if vtol that its armed
		if (vtolEmpty(psDroid))
		{
			moveToRearm(psDroid);
		}

		bHasTarget = false;
		//loop through weapons and look for target for each weapon
		for (i = 0;i < psDroid->numWeaps;i++)
		{
			if (psDroid->psActionTarget[i] == NULL)
			{
				BASE_OBJECT *psTemp;

				if (aiBestNearestTarget(psDroid, &psTemp, i) >= 0)
				{
					bHasTarget = true;
					setDroidActionTarget(psDroid, psTemp, i);
				}
			}

			if (psDroid->psActionTarget[i] &&
				visibleObject((BASE_OBJECT*)psDroid, psDroid->psActionTarget[i]))
			{
				iVisible += (1 << (i+1));
			}
		}

		for (j = 0;j < psDroid->numWeaps;j++)
		{
			//Watermelon:vtResult uses psActionTarget[0] for now since it's the first target
			if (psDroid->psActionTarget[j] != NULL &&
				validTarget((BASE_OBJECT *)psDroid, psDroid->psActionTarget[j], j))
			{
				// firing on something while moving
				if (DROID_STOPPED(psDroid))
				{
					// Got to desitination
					psDroid->action = DACTION_NONE;
					break;
				}
				else if ((psDroid->psActionTarget[j] == NULL) ||
						!validTarget((BASE_OBJECT *)psDroid, psDroid->psActionTarget[j], j) ||
						(secondaryGetState(psDroid, DSO_ATTACK_LEVEL, &state) && (state != DSS_ALEV_ALWAYS)))
				{
					if (j == (psDroid->numWeaps - 1) && !bHasTarget)
					{
						// Target lost
						psDroid->action = DACTION_MOVE;
					}
					else
					{
						continue;
					}
					//if Vtol - return to rearm pad
					/*if (vtolDroid(psDroid))
					{
						moveToRearm(psDroid);
					}*/
				}
				//check the target hasn't become one the same player ID - eg Electronic Warfare
				else if	(electronicDroid(psDroid) &&
						(psDroid->player == psDroid->psActionTarget[j]->player))
				{
					setDroidActionTarget(psDroid, NULL, i);
					psDroid->action = DACTION_NONE;
				}
				else
				{
					if (iVisible == 1 && !bHasTarget && j == (psDroid->numWeaps - 1))
					{
						// lost the target
						psDroid->action = DACTION_MOVE;
						for (i = 0;i < psDroid->numWeaps;i++)
						{
							setDroidActionTarget(psDroid, NULL, i);
						}
					}

					if (iVisible & (1 << (j+1)))
					{
						bHasTarget = true;
						//Watermelon:I moved psWeapStats flag update there
						psWeapStats = asWeaponStats + psDroid->asWeaps[j].nStat;
						//Watermelon:to fix a AA-weapon attack ground unit exploit
						if ( (num_weapons & (1 << (j+1))) )
						{
							BASE_OBJECT* psActionTarget;

							if (psDroid->psActionTarget[j])
							{
								psActionTarget = psDroid->psActionTarget[j];
							}
							else
							{
								psActionTarget = psDroid->psActionTarget[0];
							}

							if (validTarget((BASE_OBJECT *)psDroid, psActionTarget, j))
							{
								if (actionTargetTurret((BASE_OBJECT*)psDroid, psActionTarget,
														&(psDroid->turretRotation[j]), &(psDroid->turretPitch[j]),
														&asWeaponStats[psDroid->asWeaps[j].nStat],
														bInvert,j))
								{
									// In range - fire !!!
									combFire(&psDroid->asWeaps[j], (BASE_OBJECT *)psDroid,
											psActionTarget, j);
								}
							}
							else
							{
								if ((iVisible & 2) && validTarget((BASE_OBJECT *)psDroid, psDroid->psActionTarget[0], j))
								{
									if (actionTargetTurret((BASE_OBJECT*)psDroid, psDroid->psActionTarget[0],
															&(psDroid->turretRotation[j]), &(psDroid->turretPitch[j]),
															&asWeaponStats[psDroid->asWeaps[j].nStat],
															bInvert,j))
									{
										// In range - fire !!!
										combFire(&psDroid->asWeaps[j], (BASE_OBJECT *)psDroid,
												psDroid->psActionTarget[0], j);
									}
								}
							}
						}
					}
				}
			}
		}

		//check its a VTOL unit since adding Transporter's into multiPlayer
		/* check vtol attack runs */
		if (vtolDroid(psDroid))
		{
			actionUpdateVtolAttack( psDroid );
		}

		break;

	case DACTION_ATTACK:
		ASSERT( psDroid->psActionTarget[0] != NULL,
			"actionUpdateUnit: target is NULL while attacking" );

		// don't wan't formations for this one
		if (psDroid->sMove.psFormation)
		{
			formationLeave(psDroid->sMove.psFormation, psDroid);
			psDroid->sMove.psFormation = NULL;
		}

		//check the target hasn't become one the same player ID - Electronic Warfare
		if ((electronicDroid(psDroid) && (psDroid->player == psDroid->psActionTarget[0]->player)))
		{
			for (i = 0;i < psDroid->numWeaps;i++)
			{
				setDroidActionTarget(psDroid, NULL, i);
			}
			psDroid->action = DACTION_NONE;
			break;
		}

		bHasTarget = false;
		for(i = 0;i < psDroid->numWeaps;i++)
		{
			BASE_OBJECT* psActionTarget;

			//Skip main turret target changes
			if (i > 0 &&
				psDroid->psActionTarget[i] == NULL &&
				aiChooseTarget((BASE_OBJECT*)psDroid, &psTargets[i], i, false))
			{
				setDroidActionTarget(psDroid, psTargets[i], i);
			}

			if (psDroid->psActionTarget[i])
			{
				psActionTarget = psDroid->psActionTarget[i];
			}
			else
			{
				psActionTarget = psDroid->psActionTarget[0];
			}

			if ( (num_weapons & (1 << (i+1))) &&
				actionVisibleTarget(psDroid, psActionTarget, i) &&
				actionInRange(psDroid, psActionTarget, i))
			{
				bHasTarget = true;
				psWeapStats = asWeaponStats + psDroid->asWeaps[i].nStat;
				if (validTarget((BASE_OBJECT *)psDroid, psActionTarget, i))
				{
					if (!psWeapStats->rotate)
					{
						// no rotating turret - need to check aligned with target
						targetDir = (SDWORD)calcDirection(
										psDroid->pos.x,psDroid->pos.y,
										psActionTarget->pos.x, psActionTarget->pos.y);
						dirDiff = labs(targetDir - (SDWORD)psDroid->direction);
					}
					else
					{
						dirDiff = 0;
					}
					if (dirDiff > FIXED_TURRET_DIR)
					{
						if (psDroid->sMove.Status != MOVESHUFFLE)
						{
							psDroid->action = DACTION_ROTATETOATTACK;
							moveTurnDroid(psDroid, psActionTarget->pos.x, psActionTarget->pos.y);
						}
					}
					else if (!psWeapStats->rotate ||
							actionTargetTurret((BASE_OBJECT*)psDroid, psActionTarget,
												&(psDroid->turretRotation[i]), &(psDroid->turretPitch[i]),
												&asWeaponStats[psDroid->asWeaps[i].nStat],
												bInvert,i))
					{
						/* In range - fire !!! */
						combFire(&psDroid->asWeaps[i], (BASE_OBJECT *)psDroid, psActionTarget, i);
					}
				}
			}
		}

		if (!bHasTarget)
		{
			if (((psDroid->order == DORDER_ATTACKTARGET
			   || psDroid->order == DORDER_FIRESUPPORT)
			  && secondaryGetState(psDroid, DSO_HALTTYPE, &state)
			  && (state == DSS_HALT_HOLD))
			 || (!vtolDroid(psDroid)
			  && (psTarget = orderStateObj(psDroid, DORDER_FIRESUPPORT))
			  && psTarget->type == OBJ_STRUCTURE))
			{
				// don't move if on hold or firesupport for a sensor tower
				psDroid->action = DACTION_NONE;				// holding, cancel the order.
			}
			else
			{
				psDroid->action = DACTION_MOVETOATTACK;	// out of range - chase it
			}
		}

		break;

	case DACTION_VTOLATTACK:
		//Watermelon:uses vtResult
		if (psDroid->psActionTarget[0] != NULL &&
			validTarget((BASE_OBJECT *)psDroid, psDroid->psActionTarget[0], 0))
		{
			//check if vtol that its armed
			if ( (vtolEmpty(psDroid)) ||
				(psDroid->psActionTarget[0] == NULL) ||
				//check the target hasn't become one the same player ID - Electronic Warfare
				(electronicDroid(psDroid) && (psDroid->player == psDroid->psActionTarget[0]->player)) ||
				!validTarget((BASE_OBJECT *)psDroid, psDroid->psActionTarget[0], 0) )
			{
				moveToRearm(psDroid);
				break;
			}

			for(i = 0;i <psDroid->numWeaps;i++)
			{
				if ( (num_weapons & (1 << (i+1))) &&
					validTarget((BASE_OBJECT *)psDroid, psDroid->psActionTarget[0], i) )
				{
					//Watermelon:I moved psWeapStats flag update there
					psWeapStats = asWeaponStats + psDroid->asWeaps[i].nStat;
					if (actionVisibleTarget(psDroid, psDroid->psActionTarget[0], i))
					{
						if ( actionInRange(psDroid, psDroid->psActionTarget[0], i) )
						{
							if ( psDroid->player == selectedPlayer )
							{
								audio_QueueTrackMinDelay( ID_SOUND_COMMENCING_ATTACK_RUN2,
															VTOL_ATTACK_AUDIO_DELAY );
							}

							if (actionTargetTurret((BASE_OBJECT*)psDroid, psDroid->psActionTarget[0],
													&(psDroid->turretRotation[i]), &(psDroid->turretPitch[i]),
													&asWeaponStats[psDroid->asWeaps[i].nStat],
													bInvert,i))
							{
									// In range - fire !!!
									combFire(&psDroid->asWeaps[i], (BASE_OBJECT *)psDroid,
									psDroid->psActionTarget[0], i);
							}
						}
						else
						{
							actionTargetTurret((BASE_OBJECT*)psDroid, psDroid->psActionTarget[0],
													&(psDroid->turretRotation[i]), &(psDroid->turretPitch[i]),
													&asWeaponStats[psDroid->asWeaps[i].nStat],
													bInvert,i);
						}
					}
				}
			}
		}
/*		else
		{
			// lost the target
			psDroid->action = DACTION_MOVETOATTACK;
			moveDroidTo(psDroid, psDroid->psActionTarget->pos.x, psDroid->psActionTarget->pos.y);
		}*/

		/* check vtol attack runs */
//		actionUpdateVtolAttack( psDroid );

		/* circle around target if hovering and not cyborg */
		if (DROID_STOPPED(psDroid))
		{
			actionAddVtolAttackRun( psDroid );
		}
		else
		{
			// if the vtol is close to the target, go around again
			xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psDroid->psActionTarget[0]->pos.x;
			ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psDroid->psActionTarget[0]->pos.y;
			rangeSq = xdiff*xdiff + ydiff*ydiff;
			if (rangeSq < (VTOL_ATTACK_TARDIST*VTOL_ATTACK_TARDIST))
			{
				// don't do another attack run if already moving away from the target
				xdiff = (SDWORD)psDroid->sMove.DestinationX - (SDWORD)psDroid->psActionTarget[0]->pos.x;
				ydiff = (SDWORD)psDroid->sMove.DestinationY - (SDWORD)psDroid->psActionTarget[0]->pos.y;
				if ((xdiff*xdiff + ydiff*ydiff) < (VTOL_ATTACK_TARDIST*VTOL_ATTACK_TARDIST))
				{
					actionAddVtolAttackRun( psDroid );
				}
			}
			// if the vtol is far enough away head for the target again
//			else if (rangeSq > (VTOL_ATTACK_RETURNDIST*VTOL_ATTACK_RETURNDIST))
			else if (rangeSq > (SDWORD)(psWeapStats->longRange*psWeapStats->longRange))
			{
				// don't do another attack run if already heading for the target
				xdiff = (SDWORD)psDroid->sMove.DestinationX - (SDWORD)psDroid->psActionTarget[0]->pos.x;
				ydiff = (SDWORD)psDroid->sMove.DestinationY - (SDWORD)psDroid->psActionTarget[0]->pos.y;
				if ((xdiff*xdiff + ydiff*ydiff) > (VTOL_ATTACK_TARDIST*VTOL_ATTACK_TARDIST))
				{
					moveDroidToDirect(psDroid, psDroid->psActionTarget[0]->pos.x,psDroid->psActionTarget[0]->pos.y);
				}
			}
		}
		break;

	case DACTION_MOVETOATTACK:

		// don't wan't formations for this one
		if (psDroid->sMove.psFormation)
		{
			formationLeave(psDroid->sMove.psFormation, psDroid);
			psDroid->sMove.psFormation = NULL;
		}

		// send vtols back to rearm
		if (vtolDroid(psDroid) &&
			vtolEmpty(psDroid))
		{
			moveToRearm(psDroid);
			break;
		}

		ASSERT(psDroid->psActionTarget[0] != NULL, "action update move to attack target is NULL");

		//check the target hasn't become one the same player ID - Electronic Warfare
		if ((electronicDroid(psDroid) && (psDroid->player == psDroid->psActionTarget[0]->player)) ||
			!validTarget((BASE_OBJECT *)psDroid, psDroid->psActionTarget[0], 0) )
		{
			for (i = 0;i < psDroid->numWeaps;i++)
			{
				setDroidActionTarget(psDroid, NULL, i);
			}
			psDroid->action = DACTION_NONE;
		}
		else
		{
			if (actionVisibleTarget(psDroid, psDroid->psActionTarget[0], 0))
			{
				for(i = 0;i < psDroid->numWeaps;i++)
				{
					if( (num_weapons & (1 << (i+1))) &&
						validTarget((BASE_OBJECT *)psDroid, psDroid->psActionTarget[0], i) &&
						actionVisibleTarget(psDroid, psDroid->psActionTarget[0], i) )
					{
						psWeapStats = asWeaponStats + psDroid->asWeaps[i].nStat;
						if (psWeapStats->rotate)
						{
							actionTargetTurret((BASE_OBJECT*)psDroid, psDroid->psActionTarget[0],
													&(psDroid->turretRotation[i]), &(psDroid->turretPitch[i]),
													&asWeaponStats[psDroid->asWeaps[i].nStat],
													bInvert,i);
						}

						bChaseBloke = false;
						if (!vtolDroid(psDroid) &&
							psDroid->psActionTarget[0]->type == OBJ_DROID &&
							((DROID *)psDroid->psActionTarget[0])->droidType == DROID_PERSON &&
							psWeapStats->fireOnMove != FOM_NO)
						{
							bChaseBloke = true;
						}


						if ( (actionInAttackRange(psDroid, psDroid->psActionTarget[0], i)) &&
							!bChaseBloke)
						{
							/* Stop the droid moving any closer */
		//				    ASSERT( psDroid->pos.x != 0 && psDroid->pos.y != 0,
		//						"moveUpdateUnit: Unit at (0,0)" );

							/* init vtol attack runs count if necessary */
							if ( psPropStats->propulsionType == LIFT )
							{
								psDroid->action = DACTION_VTOLATTACK;
								//actionAddVtolAttackRun( psDroid );
								//actionUpdateVtolAttack( psDroid );
							}
							else
							{
								moveStopDroid(psDroid);

								if (psWeapStats->rotate)
								{
									psDroid->action = DACTION_ATTACK;
								}
								else
								{
									psDroid->action = DACTION_ROTATETOATTACK;
									moveTurnDroid(psDroid, psDroid->psActionTarget[0]->pos.x,psDroid->psActionTarget[0]->pos.y);
								}
							}
						}
						else if ( actionInRange(psDroid, psDroid->psActionTarget[0], i) )
						{
							// fire while closing range
							combFire(&psDroid->asWeaps[i], (BASE_OBJECT *)psDroid, psDroid->psActionTarget[0], i);
						}
					}
				}
			}
			else
			{
				for(i = 0;i < psDroid->numWeaps;i++)
				{
					if ((psDroid->turretRotation[i] != 0) ||
						(psDroid->turretPitch[i] != 0))
					{
						actionAlignTurret((BASE_OBJECT *)psDroid, i);
					}
				}
			}

			if (DROID_STOPPED(psDroid) && psDroid->action != DACTION_ATTACK)
			{
				/* Stopped moving but haven't reached the target - possibly move again */

				if (psDroid->order == DORDER_ATTACKTARGET && secondaryGetState(psDroid, DSO_HALTTYPE, &state) && (state == DSS_HALT_HOLD))
				{
					psDroid->action = DACTION_NONE;			// on hold, give up.
				}
				else if ( actionInsideMinRange(psDroid, psDroid->psActionTarget[0], 0) )
				{
					//Watermelon:'hack' to make the droid to check the primary turrent instead of all
					psWeapStats = asWeaponStats + psDroid->asWeaps[0].nStat;
					if ( proj_Direct( psWeapStats ) )
					{
						// try and extend the range
						actionCalcPullBackPoint((BASE_OBJECT *)psDroid, psDroid->psActionTarget[0], &pbx,&pby);
						moveDroidTo(psDroid, (UDWORD)pbx, (UDWORD)pby);
					}
					else
					{
						if (psWeapStats->rotate)
						{
							psDroid->action = DACTION_ATTACK;
						}
						else
						{
							psDroid->action = DACTION_ROTATETOATTACK;
							moveTurnDroid( psDroid, psDroid->psActionTarget[0]->pos.x,
											psDroid->psActionTarget[0]->pos.y);
						}
					}
				}
				else
				{
					// try to close the range
					moveDroidTo(psDroid, psDroid->psActionTarget[0]->pos.x, psDroid->psActionTarget[0]->pos.y);
				}
			}
		}
		break;

	case DACTION_SULK:
		// unable to route to target ... don't do anything aggressive until time is up
		// we need to do something defensive at this point ???

		//hmmm, hope this doesn't cause any problems!
		if (gameTime > psDroid->actionStarted)
		{
// 			debug( LOG_NEVER, "sulk over.. %p\n", psDroid );
			psDroid->action = DACTION_NONE;			// Sulking is over lets get back to the action ... is this all I need to do to get it back into the action?
		}
		break;

	case DACTION_ROTATETOATTACK:
//		if (DROID_STOPPED(psDroid))
		if (psDroid->sMove.Status != MOVETURNTOTARGET)
		{
			psDroid->action = DACTION_ATTACK;
		}
		break;
	case DACTION_MOVETOBUILD:
		// The droid cannot be in a formation
		if (psDroid->sMove.psFormation != NULL)
		{
			formationLeave(psDroid->sMove.psFormation, psDroid);
			psDroid->sMove.psFormation = NULL;
		}

		// moving to a location to build a structure
		if (actionReachedBuildPos(psDroid,
						(SDWORD)psDroid->orderX,(SDWORD)psDroid->orderY, psDroid->psTarStats) &&
			!actionDroidOnBuildPos(psDroid,
						(SDWORD)psDroid->orderX,(SDWORD)psDroid->orderY, psDroid->psTarStats))
		{
			moveStopDroid(psDroid);
			bDoHelpBuild = false;

			// Got to destination - start building
			psStructStats = (STRUCTURE_STATS*)psDroid->psTarStats;
			if (psDroid->order == DORDER_BUILD && psDroid->psTarget == NULL)
			{
				// Starting a new structure
				// calculate the top left of the structure
				tlx = (SDWORD)psDroid->orderX - (SDWORD)(psStructStats->baseWidth * TILE_UNITS)/2;
				tly = (SDWORD)psDroid->orderY - (SDWORD)(psStructStats->baseBreadth * TILE_UNITS)/2;

				//need to check if something has already started building here?
				//unless its a module!
				if (IsStatExpansionModule(psStructStats))
				{
					debug( LOG_NEVER, "DACTION_MOVETOBUILD: setUpBuildModule\n");
					setUpBuildModule(psDroid);
				}
				else if (TILE_HAS_STRUCTURE(mapTile(map_coord(psDroid->orderX), map_coord(psDroid->orderY))))
				{
					// structure on the build location - see if it is the same type
					psStruct = getTileStructure(map_coord(psDroid->orderX), map_coord(psDroid->orderY));
					if (psStruct->pStructureType == (STRUCTURE_STATS *)psDroid->psTarStats)
					{
						// same type - do a help build
						setDroidTarget(psDroid, (BASE_OBJECT *)psStruct);
						bDoHelpBuild = true;
					}
					else if ((psStruct->pStructureType->type == REF_WALL ||
					psStruct->pStructureType->type == REF_WALLCORNER) &&
						((STRUCTURE_STATS *)psDroid->psTarStats)->type == REF_DEFENSE)
					{
							// building a gun tower over a wall - OK
							if (droidStartBuild(psDroid))
							{
								debug( LOG_NEVER, "DACTION_MOVETOBUILD: start foundation\n");
								psDroid->action = DACTION_BUILD;
							}
							else
							{
								psDroid->action = DACTION_NONE;
							}
					}
					else
					{
						psDroid->action = DACTION_NONE;
					}
				}
				else if (!validLocation((BASE_STATS*)psDroid->psTarStats,
				                        map_coord(tlx),
				                        map_coord(tly),
				                        psDroid->player,
				                        false))
				{
					debug( LOG_NEVER, "DACTION_MOVETOBUILD: !validLocation\n");
					psDroid->action = DACTION_NONE;
				}
				else
				{
					psDroid->action = DACTION_BUILD_FOUNDATION;
					psDroid->actionStarted = gameTime;
					psDroid->actionPoints = 0;
				}
			}
			else if(  (psDroid->order == DORDER_LINEBUILD||psDroid->order==DORDER_BUILD)
					&&(psStructStats->type == REF_WALL   || psStructStats->type == REF_WALLCORNER ||
					psStructStats->type == REF_DEFENSE))
			{
				// building a wall.
				psTile = mapTile(map_coord(psDroid->orderX), map_coord(psDroid->orderY));
				if (psDroid->psTarget == NULL
				 && (TILE_HAS_STRUCTURE(psTile)
				  || TILE_HAS_FEATURE(psTile)))
				{
					if (TILE_HAS_STRUCTURE(psTile))
					{
						// structure on the build location - see if it is the same type
						psStruct = getTileStructure(map_coord(psDroid->orderX), map_coord(psDroid->orderY));
						if (psStruct->pStructureType == (STRUCTURE_STATS *)psDroid->psTarStats)
						{
							// same type - do a help build
							setDroidTarget(psDroid, (BASE_OBJECT *)psStruct);
							bDoHelpBuild = true;
						}
						else if ((psStruct->pStructureType->type == REF_WALL || psStruct->pStructureType->type == REF_WALLCORNER) &&
								((STRUCTURE_STATS *)psDroid->psTarStats)->type == REF_DEFENSE)
						{
							// building a gun tower over a wall - OK
							if (droidStartBuild(psDroid))
							{
								debug( LOG_NEVER, "DACTION_MOVETOBUILD: start foundation\n");
								psDroid->action = DACTION_BUILD;
							}
							else
							{
								psDroid->action = DACTION_NONE;
							}
						}
						else
						{
							psDroid->action = DACTION_NONE;
						}
					}
					else
					{
						psDroid->action = DACTION_NONE;
					}
				}
				else if (droidStartBuild(psDroid))
				{
					psDroid->action = DACTION_BUILD;
					intBuildStarted(psDroid);
				}
				else
				{
					psDroid->action = DACTION_NONE;
				}
			}
			else
			{
				bDoHelpBuild = true;
			}

			if (bDoHelpBuild)
			{
				// continuing a partially built structure (order = helpBuild)
				if (droidStartBuild(psDroid))
				{
					debug( LOG_NEVER, "DACTION_MOVETOBUILD: starting help build\n");
					psDroid->action = DACTION_BUILD;
					intBuildStarted(psDroid);
				}
				else
				{
					debug( LOG_NEVER, "DACTION_MOVETOBUILD: giving up (3)\n");
					psDroid->action = DACTION_NONE;
				}
			}
		}
		else if (DROID_STOPPED(psDroid))
		{
			if (actionDroidOnBuildPos(psDroid,
						(SDWORD)psDroid->orderX,(SDWORD)psDroid->orderY, psDroid->psTarStats))
			{
				actionHomeBasePos(psDroid->player, &pbx,&pby);
				if (pbx == 0 || pby == 0)
				{
					debug(LOG_NEVER, "DACTION_MOVETOBUILD: No HQ, cannot move in that direction.");
					psDroid->action = DACTION_NONE;
					break;
				}
				moveDroidToNoFormation(psDroid, (UDWORD)pbx,(UDWORD)pby);
			}
			else
			{
				moveDroidToNoFormation(psDroid, psDroid->actionX,psDroid->actionY);
			}
		}
		break;
	case DACTION_BUILD:
		// The droid cannot be in a formation
		if (psDroid->sMove.psFormation != NULL)
		{
			formationLeave(psDroid->sMove.psFormation, psDroid);
			psDroid->sMove.psFormation = NULL;
		}

		if (DROID_STOPPED(psDroid) &&
			!actionReachedBuildPos(psDroid,
						(SDWORD)psDroid->orderX,(SDWORD)psDroid->orderY, psDroid->psTarStats))
		{
//			psDroid->action = DACTION_MOVETOBUILD;
			moveDroidToNoFormation(psDroid, psDroid->orderX, psDroid->orderY);
		}
		else if (!DROID_STOPPED(psDroid) &&
				psDroid->sMove.Status != MOVETURNTOTARGET &&
				psDroid->sMove.Status != MOVESHUFFLE &&
				actionReachedBuildPos(psDroid,
						(SDWORD)psDroid->orderX,(SDWORD)psDroid->orderY, psDroid->psTarStats))
		{
			moveStopDroid(psDroid);
		}

		if (droidUpdateBuild(psDroid))
		{
/*			if ( (psDroid->psTarget) && (psDroid->sMove.Status != MOVESHUFFLE) )
			{
				moveTurnDroid(psDroid,psDroid->psTarget->pos.x,psDroid->psTarget->pos.y);
			}*/
			// Watermelon:make construct droid to use turretRotation[0]
			actionTargetTurret((BASE_OBJECT*)psDroid, psDroid->psActionTarget[0],
									&(psDroid->turretRotation[0]), &(psDroid->turretPitch[0]),
									NULL,false,0);
		}
		break;
	case DACTION_MOVETODEMOLISH:
	case DACTION_MOVETOREPAIR:
	case DACTION_MOVETOCLEAR:
	case DACTION_MOVETORESTORE:

		// The droid cannot be in a formation
		if (psDroid->sMove.psFormation != NULL)
		{
			formationLeave(psDroid->sMove.psFormation, psDroid);
			psDroid->sMove.psFormation = NULL;
		}

		// see if the droid is at the edge of what it is moving to
		if (actionReachedBuildPos(psDroid,
						(SDWORD)psDroid->actionX,(SDWORD)psDroid->actionY, psDroid->psTarStats) &&
			!actionDroidOnBuildPos(psDroid,
						(SDWORD)psDroid->actionX,(SDWORD)psDroid->actionY, psDroid->psTarStats))
		{
			moveStopDroid(psDroid);

			// got to the edge - start doing whatever it was meant to do
			switch (psDroid->action)
			{
			case DACTION_MOVETODEMOLISH:
				psDroid->action = droidStartDemolishing(psDroid) ? DACTION_DEMOLISH : DACTION_NONE;
				break;
			case DACTION_MOVETOREPAIR:
				psDroid->action = droidStartRepair(psDroid) ? DACTION_REPAIR : DACTION_NONE;
				break;
			case DACTION_MOVETOCLEAR:
				psDroid->action = droidStartClearing(psDroid) ? DACTION_CLEARWRECK : DACTION_NONE;
				break;
			case DACTION_MOVETORESTORE:
				psDroid->action = droidStartRestore(psDroid) ? DACTION_RESTORE : DACTION_NONE;
				break;
			default:
				break;
			}
		}
		else if (DROID_STOPPED(psDroid))
		{
			if (actionDroidOnBuildPos(psDroid,
						(SDWORD)psDroid->actionX,(SDWORD)psDroid->actionY, psDroid->psTarStats))
			{
				actionHomeBasePos(psDroid->player, &pbx,&pby);
				if (pbx == 0 || pby == 0)
				{
					debug(LOG_NEVER, "No HQ - cannot move in that direction.");
					psDroid->action = DACTION_NONE;
					break;
				}
				moveDroidToNoFormation(psDroid, (UDWORD)pbx,(UDWORD)pby);
			}
			else
			{
				moveDroidToNoFormation(psDroid, psDroid->actionX,psDroid->actionY);
			}
		}
		break;

	case DACTION_DEMOLISH:
	case DACTION_REPAIR:
	case DACTION_CLEARWRECK:
	case DACTION_RESTORE:
		// set up for the specific action
		switch (psDroid->action)
		{
		case DACTION_DEMOLISH:
			moveAction = DACTION_MOVETODEMOLISH;
			actionUpdateFunc = droidUpdateDemolishing;
			break;
		case DACTION_REPAIR:
			moveAction = DACTION_MOVETOREPAIR;
			actionUpdateFunc = droidUpdateRepair;
			break;
		case DACTION_CLEARWRECK:
			moveAction = DACTION_MOVETOCLEAR;
			actionUpdateFunc = droidUpdateClearing;
			break;
		case DACTION_RESTORE:
			moveAction = DACTION_MOVETORESTORE;
			actionUpdateFunc = droidUpdateRestore;
			break;
		default:
			break;
		}

		// now do the action update
		if (DROID_STOPPED(psDroid) &&
			!actionReachedBuildPos(psDroid,
						(SDWORD)psDroid->actionX,(SDWORD)psDroid->actionY, psDroid->psTarStats))
		{
			moveDroidToNoFormation(psDroid, psDroid->actionX,psDroid->actionY);
		}
		else if (!DROID_STOPPED(psDroid) &&
				psDroid->sMove.Status != MOVETURNTOTARGET &&
				psDroid->sMove.Status != MOVESHUFFLE &&
				actionReachedBuildPos(psDroid,
						(SDWORD)psDroid->actionX,(SDWORD)psDroid->actionY, psDroid->psTarStats))
		{
			moveStopDroid(psDroid);
		}
		else if ( actionUpdateFunc(psDroid) )
		{
			//Watermelon:use 0 for non-combat(only 1 'weapon')
			actionTargetTurret((BASE_OBJECT*)psDroid, psDroid->psActionTarget[0],
									&(psDroid->turretRotation[0]), &(psDroid->turretPitch[0]),
									NULL,false,0);
		}
		else if (psDroid->action == DACTION_CLEARWRECK)
		{
			//see if there is any other wreckage in the area
			psDroid->action = DACTION_NONE;
			psNextWreck = checkForWreckage(psDroid);
			if (psNextWreck)
			{
				orderDroidObj(psDroid, DORDER_CLEARWRECK, (BASE_OBJECT *)psNextWreck);
			}
		}
		else
		{
			psDroid->action = DACTION_NONE;
		}
		break;

	case DACTION_MOVETOREARMPOINT:
		/* moving to rearm pad */
		if (DROID_STOPPED(psDroid))
		{
			psDroid->action = DACTION_WAITDURINGREARM;
		}
		break;
	case DACTION_MOVETOREPAIRPOINT:
		// don't want to be in a formation for this move
		if (psDroid->sMove.psFormation != NULL)
		{
			formationLeave(psDroid->sMove.psFormation, psDroid);
			psDroid->sMove.psFormation = NULL;
		}
		/* moving from front to rear of repair facility or rearm pad */
		if (actionReachedBuildPos(psDroid, psDroid->psActionTarget[0]->pos.x,psDroid->psActionTarget[0]->pos.y,
							(BASE_STATS *)((STRUCTURE *)psDroid->psActionTarget[0])->pStructureType))
		{
			moveStopDroid(psDroid);
			psDroid->action = DACTION_WAITDURINGREPAIR;
		}
		else if (DROID_STOPPED(psDroid))
		{
			moveDroidToNoFormation(psDroid, psDroid->psActionTarget[0]->pos.x,
						psDroid->psActionTarget[0]->pos.y);
		}
		break;
	case DACTION_BUILD_FOUNDATION:
		//building a structure's foundation - flattening the ground for now
		{
			psTile = mapTile(map_coord(psDroid->orderX), map_coord(psDroid->orderY));
			psStructStats = (STRUCTURE_STATS*)psDroid->psTarStats;
			tlx = (SDWORD)psDroid->orderX - (SDWORD)(psStructStats->baseWidth * TILE_UNITS)/2;
			tly = (SDWORD)psDroid->orderY - (SDWORD)(psStructStats->baseBreadth * TILE_UNITS)/2;
			if ((psDroid->psTarget == NULL) &&
				(TILE_HAS_STRUCTURE(psTile) ||
				TILE_HAS_FEATURE(psTile)))
			{
				if (TILE_HAS_STRUCTURE(psTile))
				{
					// structure on the build location - see if it is the same type
					psStruct = getTileStructure(map_coord(psDroid->orderX), map_coord(psDroid->orderY));
					if (psStruct->pStructureType == (STRUCTURE_STATS *)psDroid->psTarStats)
					{
						// same type - do a help build
						setDroidTarget(psDroid, (BASE_OBJECT *)psStruct);
					}
					else
					{
						psDroid->action = DACTION_NONE;
					}
				}
				else if (!validLocation((BASE_STATS*)psDroid->psTarStats,
				                        map_coord(tlx),
				                        map_coord(tly),
				                        psDroid->player,
				                        false))
				{
					psDroid->action = DACTION_NONE;
				}
			}

			//ready to start building the structure
			if ( psDroid->action != DACTION_NONE &&
				droidStartBuild(psDroid))
			{
				debug( LOG_NEVER, "DACTION_BUILD_FOUNDATION: start build\n");
				psDroid->action = DACTION_BUILD;
			}
			else
			{
				debug( LOG_NEVER, "DACTION_BUILD_FOUNDATION: giving up\n");
				psDroid->action = DACTION_NONE;
			}
		}
		break;
	case DACTION_OBSERVE:
		// align the turret
		actionTargetTurret((BASE_OBJECT*)psDroid, psDroid->psActionTarget[0],
						&(psDroid->turretRotation[0]), &(psDroid->turretPitch[0]),
						NULL,false,0);

		if (cbSensorDroid(psDroid))
		{
			// don't move to the target, just make sure it is visible
			// Anyone commenting this out will get a knee capping from John.
			// You have been warned!!
			psDroid->psActionTarget[0]->visible[psDroid->player] = UBYTE_MAX;
		}
		else
		{
			// make sure the target is within sensor range
			xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psDroid->psActionTarget[0]->pos.x;
			ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psDroid->psActionTarget[0]->pos.y;
			rangeSq = droidSensorRange(psDroid);
			rangeSq = rangeSq * rangeSq;
			if (!visibleObject((BASE_OBJECT *)psDroid, psDroid->psActionTarget[0]) ||
				xdiff*xdiff + ydiff*ydiff >= rangeSq)
			{
				psDroid->action = DACTION_MOVETOOBSERVE;
				moveDroidTo(psDroid, psDroid->psActionTarget[0]->pos.x, psDroid->psActionTarget[0]->pos.y);
			}
		}
		break;
	case DACTION_MOVETOOBSERVE:
		// align the turret
		actionTargetTurret((BASE_OBJECT*)psDroid, psDroid->psActionTarget[0],
						&(psDroid->turretRotation[0]), &(psDroid->turretPitch[0]),
						NULL,false,0);

		if (visibleObject((BASE_OBJECT *)psDroid, psDroid->psActionTarget[0]))
		{
			// make sure the target is within sensor range
			xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psDroid->psActionTarget[0]->pos.x;
			ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psDroid->psActionTarget[0]->pos.y;
			rangeSq = droidSensorRange(psDroid);
			rangeSq = rangeSq * rangeSq;
			if ((xdiff*xdiff + ydiff*ydiff < rangeSq) &&
				!DROID_STOPPED(psDroid))
			{
				psDroid->action = DACTION_OBSERVE;
				moveStopDroid(psDroid);
			}
		}
		if (DROID_STOPPED(psDroid) && psDroid->action == DACTION_MOVETOOBSERVE)
		{
			moveDroidTo(psDroid, psDroid->psActionTarget[0]->pos.x, psDroid->psActionTarget[0]->pos.y);
		}
		break;
	case DACTION_FIRESUPPORT:
		//can be either a droid or a structure now - AB 7/10/98
		ASSERT( (psDroid->psTarget->type == OBJ_DROID ||
			psDroid->psTarget->type == OBJ_STRUCTURE) &&
				(psDroid->psTarget->player == psDroid->player),
			"DACTION_FIRESUPPORT: incorrect target type" );
			//don't move VTOL's
			// also don't move closer to sensor towers
			if (!vtolDroid(psDroid) &&
				(psDroid->psTarget->type != OBJ_STRUCTURE))
			{
				//move droids to within short range of the sensor now!!!!
				xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psDroid->psTarget->pos.x;
				ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psDroid->psTarget->pos.y;
				rangeSq = asWeaponStats[psDroid->asWeaps[0].nStat].shortRange;
				rangeSq = rangeSq * rangeSq;
				if (xdiff*xdiff + ydiff*ydiff < rangeSq)
				{
					if (!DROID_STOPPED(psDroid))
					{
						moveStopDroid(psDroid);
					}
				}
				else
				{
					if (!DROID_STOPPED(psDroid))
					{
						xdiff = (SDWORD)psDroid->psTarget->pos.x - (SDWORD)psDroid->sMove.DestinationX;
						ydiff = (SDWORD)psDroid->psTarget->pos.y - (SDWORD)psDroid->sMove.DestinationY;
					}

					if (DROID_STOPPED(psDroid) ||
						(xdiff*xdiff + ydiff*ydiff > rangeSq))
					{
						if (secondaryGetState(psDroid, DSO_HALTTYPE, &state) && (
							state == DSS_HALT_HOLD))
						{
							// droid on hold, don't allow moves.
							psDroid->action = DACTION_NONE;
						}
						else
						{
							// move in range
							moveDroidTo(psDroid, psDroid->psTarget->pos.x,psDroid->psTarget->pos.y);
						}
					}
				}
			}
		//}
		break;
	case DACTION_DESTRUCT:
		if ((psDroid->actionStarted + ACTION_DESTRUCT_TIME) < gameTime)
		{
			if ( psDroid->droidType == DROID_PERSON )
			{
				droidBurn(psDroid);
			}
			else
			{
				debug(LOG_DEATH, "actionUpdateDroid: Droid %d destructed", (int)psDroid->id);
				destroyDroid(psDroid);
			}
		}
		break;
	case DACTION_MOVETODROIDREPAIR:
		// moving to repair a droid
		xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psDroid->psActionTarget[0]->pos.x;
		ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psDroid->psActionTarget[0]->pos.y;
		if ( xdiff*xdiff + ydiff*ydiff < REPAIR_RANGE )
		{
			// Got to destination - start repair
			//rotate turret to point at droid being repaired
			//Watermelon:use 0 for repair droid
			if (actionTargetTurret((BASE_OBJECT*)psDroid,
					psDroid->psActionTarget[0], &(psDroid->turretRotation[0]),
					&(psDroid->turretPitch[0]), NULL, false, 0))
			{
				if (droidStartDroidRepair(psDroid))
				{
					psDroid->action = DACTION_DROIDREPAIR;
				}
				else
				{
					psDroid->action = DACTION_NONE;
				}
			}
		}
		if (DROID_STOPPED(psDroid))
		{
			// Couldn't reach destination - try and find a new one
			psDroid->actionX = psDroid->psActionTarget[0]->pos.x;
			psDroid->actionY = psDroid->psActionTarget[0]->pos.y;
			moveDroidTo(psDroid, psDroid->actionX, psDroid->actionY);
		}
		break;
	case DACTION_DROIDREPAIR:
		//if not doing self-repair
		//Watermelon:use 0 for repair droid
		if (psDroid->psActionTarget[0] != (BASE_OBJECT *)psDroid)
		{
			actionTargetTurret((BASE_OBJECT*)psDroid,
					psDroid->psActionTarget[0], &(psDroid->turretRotation[0]),
					&(psDroid->turretPitch[0]), NULL, false, 0);
		}

		//check still next to the damaged droid
		xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psDroid->psActionTarget[0]->pos.x;
		ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psDroid->psActionTarget[0]->pos.y;
		if ( xdiff*xdiff + ydiff*ydiff > REPAIR_RANGE )
		{
			/*once started - don't allow the Repair droid to follow the
			damaged droid for too long*/
			/*if (psDroid->actionPoints)
			{
				if (gameTime - psDroid->actionStarted > KEEP_TRYING_REPAIR)
				{
					addConsoleMessage("Repair Droid has given up!",DEFAULT_JUSTIFY);
					psDroid->action = DACTION_NONE;
					break;
				}
			}*/
			// damaged droid has moved off - follow!
			psDroid->actionX = psDroid->psActionTarget[0]->pos.x;
			psDroid->actionY = psDroid->psActionTarget[0]->pos.y;
			psDroid->action = DACTION_MOVETODROIDREPAIR;
			moveDroidTo(psDroid, psDroid->actionX, psDroid->actionY);
		}
		else
		{
			if (!droidUpdateDroidRepair(psDroid))
			{
				psDroid->action = DACTION_NONE;
				//if the order is RTR then resubmit order so that the unit will go to repair facility point
				if (orderState(psDroid,DORDER_RTR))
				{
					orderDroid(psDroid, DORDER_RTR);
				}
			}
			else
			{
				// don't let the target for a repair shuffle
				if (((DROID *)psDroid->psActionTarget[0])->sMove.Status == MOVESHUFFLE)
				{
					moveStopDroid((DROID *)psDroid->psActionTarget[0]);
				}
			}
		}
		break;
	case DACTION_WAITFORREARM:
		// wait here for the rearm pad to ask the vtol to move
		if (psDroid->psActionTarget[0] == NULL)
		{
			// rearm pad destroyed - move to another
			moveToRearm(psDroid);
			break;
		}
		if (DROID_STOPPED(psDroid) && vtolHappy(psDroid))
		{
			// don't actually need to rearm so just sit next to the rearm pad
			psDroid->action = DACTION_NONE;
		}
		break;
	case DACTION_CLEARREARMPAD:
		if (DROID_STOPPED(psDroid))
		{
			psDroid->action = DACTION_NONE;
		}
		break;
	case DACTION_WAITDURINGREARM:
		// this gets cleared by the rearm pad
		break;
	case DACTION_MOVETOREARM:
		if (psDroid->psActionTarget[0] == NULL)
		{
			// base destroyed - find another
			moveToRearm(psDroid);
			break;
		}

		if (visibleObject((BASE_OBJECT *)psDroid, psDroid->psActionTarget[0]))
		{
			// got close to the rearm pad - now find a clear one
			debug( LOG_NEVER, "Unit %d: seen rearm pad\n", psDroid->id );
			psStruct = findNearestReArmPad(psDroid, (STRUCTURE *)psDroid->psActionTarget[0], true);
			if (psStruct != NULL)
			{
				// found a clear landing pad - go for it
				debug( LOG_NEVER, "Found clear rearm pad\n" );
				setDroidActionTarget(psDroid, (BASE_OBJECT *)psStruct, 0);
			}

			psDroid->action = DACTION_WAITFORREARM;
		}

		if (DROID_STOPPED(psDroid) ||
			(psDroid->action == DACTION_WAITFORREARM))
		{
			droidX = psDroid->psActionTarget[0]->pos.x;
			droidY = psDroid->psActionTarget[0]->pos.y;
			if (!actionVTOLLandingPos(psDroid, &droidX, &droidY))
			{
				// totally bunged up - give up
				debug( LOG_NEVER, "DACTION_MOVETOREARM: couldn't find a clear tile near rearm pad - RTB\n" );
				orderDroid(psDroid, DORDER_RTB);
				break;
			}
			moveDroidToDirect(psDroid, droidX,droidY);
		}
		break;
	default:
		ASSERT(!"unknown action", "actionUpdateUnit: unknown action");
		break;
	}

	if (psDroid->action != DACTION_MOVEFIRE &&
		psDroid->action != DACTION_ATTACK &&
		psDroid->action != DACTION_MOVETOATTACK &&
		psDroid->action != DACTION_MOVETODROIDREPAIR &&
		psDroid->action != DACTION_DROIDREPAIR &&
		psDroid->action != DACTION_BUILD &&
		psDroid->action != DACTION_OBSERVE &&
		psDroid->action != DACTION_MOVETOOBSERVE)
	{

		//Watermelon:use 0 for all non-combat droid types
		if (psDroid->numWeaps == 0)
		{
			if ((psDroid->turretRotation[0] != 0) ||
				(psDroid->turretPitch[0] != 0))
			{
				actionAlignTurret((BASE_OBJECT *)psDroid, 0);
			}
		}
		else
		{
			for (i = 0;i < psDroid->numWeaps;i++)
			{
				if ((psDroid->turretRotation[i] != 0) ||
					(psDroid->turretPitch[i] != 0))
				{
					actionAlignTurret((BASE_OBJECT *)psDroid, i);
				}
			}
		}
	}
	CHECK_DROID(psDroid);
}

/* Overall action function that is called by the specific action functions */
static void actionDroidBase(DROID *psDroid, DROID_ACTION_DATA *psAction)
{
	SECONDARY_STATE			state;
	SDWORD			pbx,pby;
	WEAPON_STATS	*psWeapStats;
	UDWORD			droidX,droidY;
	BASE_OBJECT		*psTarget;
	//Watermelon:added MinRangeResult;
	UBYTE	i;

	CHECK_DROID(psDroid);

	switch (psAction->action)
	{
	case DACTION_NONE:
		// Clear up what ever the droid was doing before if necessary
			if (!DROID_STOPPED(psDroid))
			{
				moveStopDroid(psDroid);
			}
			psDroid->action = DACTION_NONE;
			psDroid->actionX = 0;
			psDroid->actionY = 0;
			psDroid->actionStarted = 0;
			psDroid->actionPoints = 0;
			psDroid->powerAccrued = 0;
			if (psDroid->numWeaps > 0)
			{
				for (i = 0;i < psDroid->numWeaps;i++)
				{
					setDroidActionTarget(psDroid, NULL, i);
				}
			}
			else
			{
				setDroidActionTarget(psDroid, NULL, 0);
			}
		break;

	case DACTION_TRANSPORTWAITTOFLYIN:
		psDroid->action = DACTION_TRANSPORTWAITTOFLYIN;
		break;

	case DACTION_ATTACK:
		// can't attack without a weapon
		// or yourself
		if ((psDroid->asWeaps[0].nStat == 0) ||
			(psDroid->droidType == DROID_TRANSPORTER) ||
			(psAction->psObj == (BASE_OBJECT *)psDroid))
		{
			return;
		}

		//check electronic droids only attack structures - not so anymore!
		if (electronicDroid(psDroid))
		{
			//check for low or zero resistance - just zero resistance!
			if (psAction->psObj->type == OBJ_STRUCTURE
			    && !validStructResistance((STRUCTURE *)psAction->psObj))
			{
				//structure is low resistance already so don't attack
				psDroid->action = DACTION_NONE;
				break;
			}

			//in multiPlayer cannot electronically attack a tranporter
			if (bMultiPlayer
			    && psAction->psObj->type == OBJ_DROID
			    && ((DROID *)psAction->psObj)->droidType == DROID_TRANSPORTER)
			{
				psDroid->action = DACTION_NONE;
				break;
			}
		}

		// note the droid's current pos so that scout & patrol orders know how far the
		// droid has gone during an attack
		// slightly strange place to store this I know, but I didn't want to add any more to the droid
		psDroid->actionX = psDroid->pos.x;
		psDroid->actionY = psDroid->pos.y;
		setDroidActionTarget(psDroid, psAction->psObj, 0);

		if (((psDroid->order == DORDER_ATTACKTARGET
		   || psDroid->order == DORDER_FIRESUPPORT)
		  && secondaryGetState(psDroid, DSO_HALTTYPE, &state)
		  && (state == DSS_HALT_HOLD))
		   || (!vtolDroid(psDroid)
		    && (psTarget = orderStateObj(psDroid, DORDER_FIRESUPPORT))
		    && psTarget->type == OBJ_STRUCTURE))
		{
			psDroid->action = DACTION_ATTACK;		// holding, try attack straightaway
		}
		else if (actionInsideMinRange(psDroid, psAction->psObj, 0))
		{
			psWeapStats = &asWeaponStats[psDroid->asWeaps[0].nStat];
			if ( !proj_Direct( psWeapStats ) )
			{
				if (psWeapStats->rotate)
				{
					psDroid->action = DACTION_ATTACK;
				}
				else
				{
					psDroid->action = DACTION_ROTATETOATTACK;
					moveTurnDroid(psDroid, psDroid->psActionTarget[0]->pos.x, psDroid->psActionTarget[0]->pos.y);
				}
			}
			else
			{
				/* direct fire - try and extend the range */
				psDroid->action = DACTION_MOVETOATTACK;
				actionCalcPullBackPoint((BASE_OBJECT *)psDroid, psAction->psObj, &pbx,&pby);

				turnOffMultiMsg(true);
				moveDroidTo(psDroid, (UDWORD)pbx, (UDWORD)pby);
				turnOffMultiMsg(false);
			}
		}
		else
		{
			psDroid->action = DACTION_MOVETOATTACK;
			turnOffMultiMsg(true);
			moveDroidTo(psDroid, psAction->psObj->pos.x, psAction->psObj->pos.y);
			turnOffMultiMsg(false);
		}
		break;

	case DACTION_MOVETOREARM:
		psDroid->action = DACTION_MOVETOREARM;
		psDroid->actionX = psAction->psObj->pos.x;
		psDroid->actionY = psAction->psObj->pos.y;
		psDroid->actionStarted = gameTime;
		setDroidActionTarget(psDroid, psAction->psObj, 0);
		droidX = psDroid->psActionTarget[0]->pos.x;
		droidY = psDroid->psActionTarget[0]->pos.y;
		if (!actionVTOLLandingPos(psDroid, &droidX, &droidY))
		{
			// totally bunged up - give up
			orderDroid(psDroid, DORDER_RTB);
			break;
		}
		moveDroidToDirect(psDroid, droidX, droidY);
		break;
	case DACTION_CLEARREARMPAD:
		debug( LOG_NEVER, "Unit %d clearing rearm pad\n", psDroid->id);
		psDroid->action = DACTION_CLEARREARMPAD;
		setDroidActionTarget(psDroid, psAction->psObj, 0);
		droidX = psDroid->psActionTarget[0]->pos.x;
		droidY = psDroid->psActionTarget[0]->pos.y;
		if (!actionVTOLLandingPos(psDroid, &droidX, &droidY))
		{
			// totally bunged up - give up
			orderDroid(psDroid, DORDER_RTB);
			break;
		}
		moveDroidToDirect(psDroid, droidX, droidY);
		break;
	case DACTION_MOVE:
	case DACTION_TRANSPORTIN:
	case DACTION_TRANSPORTOUT:
	case DACTION_RETURNTOPOS:
	case DACTION_FIRESUPPORT_RETREAT:
		psDroid->action = psAction->action;
		psDroid->actionX = psAction->x;
		psDroid->actionY = psAction->y;
		psDroid->actionStarted = gameTime;
		setDroidActionTarget(psDroid, psAction->psObj, 0);
		moveDroidTo(psDroid, psAction->x, psAction->y);
		break;

	case DACTION_BUILD:
		ASSERT( psDroid->order == DORDER_BUILD || psDroid->order == DORDER_HELPBUILD ||
				psDroid->order == DORDER_LINEBUILD,
			"actionUnitBase: cannot start build action without a build order" );
		ASSERT(psAction->x > 0 && psAction->y > 0, "actionUnitBase: Bad build order position");
		psDroid->action = DACTION_MOVETOBUILD;
		psDroid->actionX = psAction->x;
		psDroid->actionY = psAction->y;
		if (actionDroidOnBuildPos(psDroid, psDroid->orderX, psDroid->orderY, psDroid->psTarStats))
		{
			actionHomeBasePos(psDroid->player, &pbx,&pby);
			if (pbx == 0 || pby == 0)
			{
				debug(LOG_NEVER, "DACTION_BUILD: No HQ, cannot move in that direction.");
				psDroid->action = DACTION_NONE;
				break;
			}
			moveDroidToNoFormation(psDroid, (UDWORD)pbx,(UDWORD)pby);
		}
		else
		{
			moveDroidToNoFormation(psDroid, psDroid->actionX,psDroid->actionY);
		}
		break;
	case DACTION_DEMOLISH:
		ASSERT( psDroid->order == DORDER_DEMOLISH,
			"actionUnitBase: cannot start demolish action without a demolish order" );
		psDroid->action = DACTION_MOVETODEMOLISH;
		psDroid->actionX = psAction->x;
		psDroid->actionY = psAction->y;
		ASSERT( (psDroid->psTarget != NULL) && (psDroid->psTarget->type == OBJ_STRUCTURE),
			"actionUnitBase: invalid target for demolish order" );
		psDroid->psTarStats = (BASE_STATS *)((STRUCTURE *)psDroid->psTarget)->pStructureType;
		setDroidActionTarget(psDroid, psAction->psObj, 0);
		moveDroidTo(psDroid, psAction->x, psAction->y);
		break;
	case DACTION_REPAIR:
		//ASSERT( psDroid->order == DORDER_REPAIR,
		//	"actionDroidBase: cannot start repair action without a repair order" );
		psDroid->action = DACTION_MOVETOREPAIR;
		psDroid->actionX = psAction->x;
		psDroid->actionY = psAction->y;
		//this needs setting so that automatic repair works
		setDroidActionTarget(psDroid, psAction->psObj, 0);
		ASSERT( (psDroid->psActionTarget[0] != NULL) && (psDroid->psActionTarget[0]->type == OBJ_STRUCTURE),
			"actionUnitBase: invalid target for demolish order" );
		psDroid->psTarStats = (BASE_STATS *)((STRUCTURE *)psDroid->psActionTarget[0])->pStructureType;
		moveDroidTo(psDroid, psAction->x, psAction->y);
		break;
	case DACTION_OBSERVE:
		setDroidActionTarget(psDroid, psAction->psObj, 0);
		psDroid->actionX = psDroid->pos.x;
		psDroid->actionY = psDroid->pos.y;
		if (//(secondaryGetState(psDroid, DSO_HALTTYPE, &state) && (state == DSS_HALT_HOLD)) ||
			cbSensorDroid(psDroid))
		{
			psDroid->action = DACTION_OBSERVE;
		}
		else
		{
			psDroid->action = DACTION_MOVETOOBSERVE;
			moveDroidTo(psDroid, psDroid->psActionTarget[0]->pos.x, psDroid->psActionTarget[0]->pos.y);
		}
		break;
	case DACTION_FIRESUPPORT:
		psDroid->action = DACTION_FIRESUPPORT;
		if(!vtolDroid(psDroid) &&
			!(secondaryGetState(psDroid, DSO_HALTTYPE, &state) && (state == DSS_HALT_HOLD)) &&	// check hold
			(psDroid->psTarget->type != OBJ_STRUCTURE))
		{
			moveDroidTo(psDroid, psDroid->psTarget->pos.x, psDroid->psTarget->pos.y);		// movetotarget.
		}

		break;
	case DACTION_SULK:
// 		debug( LOG_NEVER, "Go with sulk ... %p\n", psDroid );
		psDroid->action = DACTION_SULK;
		// hmmm, hope this doesn't cause any problems!
		psDroid->actionStarted = gameTime+MIN_SULK_TIME+(rand()%(
			MAX_SULK_TIME-MIN_SULK_TIME));
		break;
	case DACTION_DESTRUCT:
		psDroid->action = DACTION_DESTRUCT;
		psDroid->actionStarted = gameTime;
		break;
	case DACTION_WAITFORREPAIR:
		psDroid->action = DACTION_WAITFORREPAIR;
		// set the time so we can tell whether the start the self repair or not
		psDroid->actionStarted = gameTime;
		break;
	case DACTION_MOVETOREPAIRPOINT:
		psDroid->action = psAction->action;
		psDroid->actionX = psAction->x;
		psDroid->actionY = psAction->y;
		psDroid->actionStarted = gameTime;
		setDroidActionTarget(psDroid, psAction->psObj, 0);
		moveDroidToNoFormation( psDroid, psAction->x, psAction->y );
		break;
	case DACTION_WAITDURINGREPAIR:
		psDroid->action = DACTION_WAITDURINGREPAIR;
		break;
	case DACTION_MOVETOREARMPOINT:
		debug( LOG_NEVER, "Unit %d moving to rearm point\n", psDroid->id);
		psDroid->action = psAction->action;
		psDroid->actionX = psAction->x;
		psDroid->actionY = psAction->y;
		psDroid->actionStarted = gameTime;
		setDroidActionTarget(psDroid, psAction->psObj, 0);
		moveDroidToDirect( psDroid, psAction->x, psAction->y );

		// make sure there arn't any other VTOLs on the rearm pad
		ensureRearmPadClear((STRUCTURE *)psAction->psObj, psDroid);
		break;
	case DACTION_DROIDREPAIR:
//		ASSERT( psDroid->order == DORDER_DROIDREPAIR,
//			"actionDroidBase: cannot start droid repair action without a repair order" );
		psDroid->action = DACTION_MOVETODROIDREPAIR;
		psDroid->actionX = psAction->x;
		psDroid->actionY = psAction->y;
		setDroidActionTarget(psDroid, psAction->psObj, 0);
		//initialise the action points
		psDroid->actionPoints  = 0;
		moveDroidTo(psDroid, psAction->x, psAction->y);
		break;
	case DACTION_RESTORE:
		ASSERT( psDroid->order == DORDER_RESTORE,
			"actionUnitBase: cannot start restore action without a restore order" );
		psDroid->action = DACTION_MOVETORESTORE;
		psDroid->actionX = psAction->x;
		psDroid->actionY = psAction->y;
		ASSERT( (psDroid->psTarget != NULL) && (psDroid->psTarget->type == OBJ_STRUCTURE),
			"actionUnitBase: invalid target for restore order" );
		psDroid->psTarStats = (BASE_STATS *)((STRUCTURE *)psDroid->psTarget)->pStructureType;
		setDroidActionTarget(psDroid, psAction->psObj, 0);
		moveDroidTo(psDroid, psAction->x, psAction->y);
		break;
	case DACTION_CLEARWRECK:
		ASSERT( psDroid->order == DORDER_CLEARWRECK,
			"actionUnitBase: cannot start clear action without a clear order" );
		psDroid->action = DACTION_MOVETOCLEAR;
		psDroid->actionX = psAction->x;
		psDroid->actionY = psAction->y;
		ASSERT( (psDroid->psTarget != NULL) && (psDroid->psTarget->type == OBJ_FEATURE),
			"actionUnitBase: invalid target for demolish order" );
		psDroid->psTarStats = (BASE_STATS *)((FEATURE *)psDroid->psTarget)->psStats;
		setDroidActionTarget(psDroid, psDroid->psTarget, 0);
		moveDroidTo(psDroid, psAction->x, psAction->y);
		break;

	default:
		ASSERT(!"unknown action", "actionUnitBase: unknown action");
		break;
	}
	CHECK_DROID(psDroid);
}


/* Give a droid an action */
void actionDroid(DROID *psDroid, DROID_ACTION action)
{
	DROID_ACTION_DATA	sAction;

	memset(&sAction, 0, sizeof(DROID_ACTION_DATA));
	sAction.action = action;
	actionDroidBase(psDroid, &sAction);
}

/* Give a droid an action with a location target */
void actionDroidLoc(DROID *psDroid, DROID_ACTION action, UDWORD x, UDWORD y)
{
	DROID_ACTION_DATA	sAction;

	memset(&sAction, 0, sizeof(DROID_ACTION_DATA));
	sAction.action = action;
	sAction.x = x;
	sAction.y = y;
	actionDroidBase(psDroid, &sAction);
}

/* Give a droid an action with an object target */
void actionDroidObj(DROID *psDroid, DROID_ACTION action, BASE_OBJECT *psObj)
{
	DROID_ACTION_DATA	sAction;

	memset(&sAction, 0, sizeof(DROID_ACTION_DATA));
	sAction.action = action;
	sAction.psObj = psObj;
	sAction.x = psObj->pos.x;
	sAction.y = psObj->pos.y;
	actionDroidBase(psDroid, &sAction);
}

/* Give a droid an action with an object target and a location */
void actionDroidObjLoc(DROID *psDroid, DROID_ACTION action,
					   BASE_OBJECT *psObj, UDWORD x, UDWORD y)
{
	DROID_ACTION_DATA	sAction;

	memset(&sAction, 0, sizeof(DROID_ACTION_DATA));
	sAction.action = action;
	sAction.psObj = psObj;
	sAction.x = x;
	sAction.y = y;
	actionDroidBase(psDroid, &sAction);
}


/*send the vtol droid back to the nearest rearming pad - if one otherwise
return to base*/
// IF YOU CHANGE THE ORDER/ACTION RESULTS TELL ALEXL!!!! && recvvtolrearm
void moveToRearm(DROID *psDroid)
{
	STRUCTURE	*psStruct;
	UBYTE		chosen=0;

	CHECK_DROID(psDroid);

	if (!vtolDroid(psDroid))
	{
		return;
	}

    //if droid is already returning - ignore
	if (vtolRearming(psDroid))
    {
        return;
    }

	//get the droid to fly back to a ReArming Pad
	// don't worry about finding a clear one for the minute
	psStruct = findNearestReArmPad(psDroid, psDroid->psBaseStruct, false);
	if (psStruct)
	{
		// note a base rearm pad if the vtol doesn't have one
		if (psDroid->psBaseStruct == NULL)
		{
			setDroidBase(psDroid, psStruct);
		}

		//return to re-arming pad
		if (psDroid->order == DORDER_NONE)
		{
			// no order set - use the rearm order to ensure the unit goes back
			// to the landing pad
			orderDroidObj(psDroid, DORDER_REARM, (BASE_OBJECT *)psStruct);
			chosen=1;
		}
		else
		{
			actionDroidObj(psDroid, DACTION_MOVETOREARM, (BASE_OBJECT *)psStruct);
			chosen=2;
		}
	}
	else
	{
		//return to base un-armed
		orderDroid( psDroid, DORDER_RTB );
		chosen =3;
	}
}


// whether a tile is suitable for a vtol to land on
static BOOL vtolLandingTile(SDWORD x, SDWORD y)
{
	MAPTILE		*psTile;

	if (x < 0 || x >= (SDWORD)mapWidth ||
		y < 0 || y >= (SDWORD)mapHeight)
	{
		return false;
	}

	psTile = mapTile(x,y);

	if ((psTile->tileInfoBits & BITS_FPATHBLOCK) ||
		(TILE_OCCUPIED(psTile)) ||
		(terrainType(psTile) == TER_CLIFFFACE) ||
		(terrainType(psTile) == TER_WATER))
	{
		return false;
	}
	return true;
}


// choose a landing position for a VTOL when it goes to rearm
BOOL actionVTOLLandingPos(DROID *psDroid, UDWORD *px, UDWORD *py)
{
	SDWORD	i,j;
	SDWORD	startX,endX,startY,endY, tx,ty;
	UDWORD	passes;
	DROID	*psCurr;
	BOOL	result;

	CHECK_DROID(psDroid);

	/* Initial box dimensions and set iteration count to zero */
	startX = endX = map_coord(*px);
	startY = endY = map_coord(*py);
	passes = 0;

	// set blocking flags for all the other droids
	for(psCurr=apsDroidLists[psDroid->player]; psCurr; psCurr = psCurr->psNext)
	{
		if (DROID_STOPPED(psCurr))
		{
			tx = map_coord(psCurr->pos.x);
			ty = map_coord(psCurr->pos.y);
		}
		else
		{
			tx = map_coord(psCurr->sMove.DestinationX);
			ty = map_coord(psCurr->sMove.DestinationY);
		}
		if (psCurr != psDroid)
		{
			if (tileOnMap(tx, ty))
			{
				mapTile(tx,ty)->tileInfoBits |= BITS_FPATHBLOCK;
			}
		}
	}

	/* Keep going until we get a tile or we exceed distance */
	result = false;
	while(passes<20)
	{
		/* Process whole box */
		for(i = startX; i <= endX; i++)
		{
			for(j = startY; j<= endY; j++)
			{
				/* Test only perimeter as internal tested previous iteration */
				if(i==startX || i==endX || j==startY || j==endY)
				{
					/* Good enough? */
					if(vtolLandingTile(i,j))
					{
						/* Set exit conditions and get out NOW */
						debug( LOG_NEVER, "Unit %d landing pos (%d,%d)\n",psDroid->id, i,j);
						*px = world_coord(i) + TILE_UNITS / 2;
						*py = world_coord(j) + TILE_UNITS / 2;
						result = true;
						goto exit;
					}
				}
			}
		}
		/* Expand the box out in all directions - off map handled by tileAcceptable */
		startX--; startY--;	endX++;	endY++;	passes++;
	}

exit:

	// clear blocking flags for all the other droids
	for(psCurr=apsDroidLists[psDroid->player]; psCurr; psCurr = psCurr->psNext)
	{
		if (DROID_STOPPED(psCurr))
		{
			tx = map_coord(psCurr->pos.x);
			ty = map_coord(psCurr->pos.y);
		}
		else
		{
			tx = map_coord(psCurr->sMove.DestinationX);
			ty = map_coord(psCurr->sMove.DestinationY);
		}
        if (tileOnMap(tx, ty))
        {
		    mapTile(tx,ty)->tileInfoBits &= ~BITS_FPATHBLOCK;
        }
	}

	return result;

}
