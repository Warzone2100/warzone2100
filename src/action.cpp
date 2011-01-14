/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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

#include "lib/framework/frame.h"
#include "lib/script/script.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "lib/netplay/netplay.h"

#include "action.h"
#include "combat.h"
#include "geometry.h"
#include "intdisplay.h"
#include "mission.h"
#include "projectile.h"
#include "random.h"
#include "research.h"
#include "scriptcb.h"
#include "scripttabs.h"
#include "transporter.h"

/* attack run distance */
#define	VTOL_ATTACK_LENGTH		1000
#define	VTOL_ATTACK_WIDTH		200
#define VTOL_ATTACK_TARDIST		400

// turret rotation limit
#define VTOL_TURRET_LIMIT               DEG(45)
#define VTOL_TURRET_LIMIT_BOMB          DEG(60)

#define	VTOL_ATTACK_AUDIO_DELAY		(3*GAME_TICKS_PER_SEC)

/** Time to pause before a droid blows up. */
#define  ACTION_DESTRUCT_TIME	2000

/** Droids heavier than this rotate and pitch more slowly. */
#define HEAVY_WEAPON_WEIGHT     50000

#define ACTION_TURRET_ROTATION_RATE	45
#define REPAIR_PITCH_LOWER		30
#define	REPAIR_PITCH_UPPER		-15

/* How many tiles to pull back. */
#define PULL_BACK_DIST		10

// data required for any action
typedef struct _droid_action_data
{
	DROID_ACTION	action;
	UDWORD			x,y;
	//multiple action target info
	BASE_OBJECT		*psObj;
	BASE_STATS		*psStats;
} DROID_ACTION_DATA;

// Check if a droid has stopped moving
#define DROID_STOPPED(psDroid) \
	(psDroid->sMove.Status == MOVEINACTIVE || psDroid->sMove.Status == MOVEHOVER || \
	 psDroid->sMove.Status == MOVESHUFFLE)

/** Radius for search when looking for VTOL landing position */
static const int vtolLandingRadius = 23;

/**
 * @typedef tileMatchFunction
 *
 * @brief pointer to a 'tile search function', used by spiralSearch()
 *
 * @param x,y  are the coordinates that should be inspected.
 *
 * @param data a pointer to state data, allows the search function to retain
 *             state in between calls and can be used as a means of returning
 *             its result to the caller of spiralSearch().
 *
 * @return true when the search has finished, false when the search should
 *         continue.
 */
typedef bool (*tileMatchFunction)(int x, int y, void* matchState);

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
		"DACTION_SULK",					// 9 refuse to do anything aggressive for a fixed time
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

	ASSERT_OR_RETURN(NULL, action < sizeof(name) / sizeof(name[0]), "DROID_ACTION out of range: %u", action);

	return name[action];
}

/* Check if a target is at correct range to attack */
static BOOL actionInAttackRange(DROID *psDroid, BASE_OBJECT *psObj, int weapon_slot)
{
	SDWORD			dx, dy, radSq, rangeSq, longRange;
	WEAPON_STATS	*psStats;
	int compIndex;

	CHECK_DROID(psDroid);
	if (psDroid->asWeaps[0].nStat == 0)
	{
		return false;
	}

	dx = (SDWORD)psDroid->pos.x - (SDWORD)psObj->pos.x;
	dy = (SDWORD)psDroid->pos.y - (SDWORD)psObj->pos.y;

	radSq = dx*dx + dy*dy;

	compIndex = psDroid->asWeaps[weapon_slot].nStat;
	ASSERT_OR_RETURN( false, compIndex < numWeaponStats, "Invalid range referenced for numWeaponStats, %d > %d", compIndex, numWeaponStats);
	psStats = asWeaponStats + compIndex;

	if (psDroid->order == DORDER_ATTACKTARGET
		&& secondaryGetState(psDroid, DSO_HALTTYPE) == DSS_HALT_HOLD)
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
			ASSERT(!"unknown attackrange order", "unknown attack range order");
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
	SDWORD			dx, dy, radSq, rangeSq, longRange;
	WEAPON_STATS	*psStats;
	int compIndex;

	CHECK_DROID(psDroid);

	if (psDroid->asWeaps[0].nStat == 0)
	{
		return false;
	}

	compIndex = psDroid->asWeaps[weapon_slot].nStat;
	ASSERT_OR_RETURN( false, compIndex < numWeaponStats, "Invalid range referenced for numWeaponStats, %d > %d", compIndex, numWeaponStats);
	psStats = asWeaponStats + compIndex;

	dx = (SDWORD)psDroid->pos.x - (SDWORD)psObj->pos.x;
	dy = (SDWORD)psDroid->pos.y - (SDWORD)psObj->pos.y;

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
BOOL actionInsideMinRange(DROID *psDroid, BASE_OBJECT *psObj, WEAPON_STATS *psStats)
{
	SDWORD	dx, dy, radSq, rangeSq, minRange;

	CHECK_DROID(psDroid);
	CHECK_OBJECT(psObj);

	if (!psStats)
	{
		psStats = getWeaponStats(psDroid, 0);
	}

	/* if I am a multi-turret droid */
	if (psDroid->asWeaps[0].nStat == 0)
	{
		return false;
	}

	dx = (SDWORD)psDroid->pos.x - (SDWORD)psObj->pos.x;
	dy = (SDWORD)psDroid->pos.y - (SDWORD)psObj->pos.y;

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
	int32_t         rotation;
	uint16_t        nearest = 0;
	uint16_t        tRot;
	uint16_t        tPitch;

	//default turret rotation 0
	tRot = 0;

	//get the maximum rotation this frame
	rotation = gameTimeAdjustedIncrement(DEG(ACTION_TURRET_ROTATION_RATE));

	switch (psObj->type)
	{
	case OBJ_DROID:
		tRot = ((DROID *)psObj)->asWeaps[weapon_slot].rot.direction;
		tPitch = ((DROID *)psObj)->asWeaps[weapon_slot].rot.pitch;
		break;
	case OBJ_STRUCTURE:
		tRot = ((STRUCTURE *)psObj)->asWeaps[weapon_slot].rot.direction;
		tPitch = ((STRUCTURE *)psObj)->asWeaps[weapon_slot].rot.pitch;

		// now find the nearest 90 degree angle
		nearest = (uint16_t)((tRot + DEG(45)) / DEG(90) * DEG(90));  // Cast wrapping indended.
		break;
	default:
		ASSERT(!"invalid object type", "invalid object type");
		return;
	}

	tRot += clip(angleDelta(nearest - tRot), -rotation, rotation);  // Addition wrapping intended.

	// align the turret pitch
	tPitch += clip(angleDelta(0 - tPitch), -rotation/2, rotation/2);  // Addition wrapping intended.

	switch (psObj->type)
	{
	case OBJ_DROID:
		((DROID *)psObj)->asWeaps[weapon_slot].rot.direction = tRot;
		((DROID *)psObj)->asWeaps[weapon_slot].rot.pitch = tPitch;
		break;
	case OBJ_STRUCTURE:
		((STRUCTURE *)psObj)->asWeaps[weapon_slot].rot.direction = tRot;
		((STRUCTURE *)psObj)->asWeaps[weapon_slot].rot.pitch = tPitch;
		break;
	default:
		ASSERT(!"invalid object type", "invalid object type");
		return;
	}
}

/* returns true if on target */
BOOL actionTargetTurret(BASE_OBJECT *psAttacker, BASE_OBJECT *psTarget, WEAPON *psWeapon)
{
	WEAPON_STATS *psWeapStats = asWeaponStats + psWeapon->nStat;
	uint16_t tRotation, tPitch;
	int32_t  rotRate, pitchRate;
	uint16_t targetRotation, targetPitch;
	int32_t  pitchError;
	int32_t  rotationError, dx, dy, dz;
	int32_t  rotationTolerance = 0;
	bool     onTarget;
	int32_t  dxy;
	int32_t  pitchLowerLimit, pitchUpperLimit;
	bool     bRepair;

	if (!psTarget)
	{
		return false;
	}

	bRepair = psAttacker->type == OBJ_DROID && ((DROID *)psAttacker)->droidType == DROID_REPAIR;

	// these are constants now and can be set up at the start of the function
	rotRate = DEG(ACTION_TURRET_ROTATION_RATE) * 4;
	pitchRate = DEG(ACTION_TURRET_ROTATION_RATE) * 2;

	// extra heavy weapons on some structures need to rotate and pitch more slowly
	if (psWeapStats->weight > HEAVY_WEAPON_WEIGHT && !bRepair)
	{
		UDWORD excess = DEG(100) * (psWeapStats->weight - HEAVY_WEAPON_WEIGHT) / psWeapStats->weight;

		rotRate = DEG(ACTION_TURRET_ROTATION_RATE) * 2 - excess;
		pitchRate = rotRate / 2;
	}

	tRotation = psWeapon->rot.direction;
	tPitch = psWeapon->rot.pitch;

	//set the pitch limits based on the weapon stats of the attacker
	pitchLowerLimit = pitchUpperLimit = 0;
	if (psAttacker->type == OBJ_STRUCTURE)
	{
		pitchLowerLimit = DEG(psWeapStats->minElevation);
		pitchUpperLimit = DEG(psWeapStats->maxElevation);
	}
	else if (psAttacker->type == OBJ_DROID)
	{
		DROID *psDroid = (DROID *)psAttacker;

		if (psDroid->droidType == DROID_WEAPON || psDroid->droidType == DROID_TRANSPORTER
			|| psDroid->droidType == DROID_COMMAND || psDroid->droidType == DROID_CYBORG
			|| psDroid->droidType == DROID_CYBORG_SUPER)
		{
			pitchLowerLimit = DEG(psWeapStats->minElevation);
			pitchUpperLimit = DEG(psWeapStats->maxElevation);
		}
		else if ( psDroid->droidType == DROID_REPAIR )
		{
			pitchLowerLimit = DEG(REPAIR_PITCH_LOWER);
			pitchUpperLimit = DEG(REPAIR_PITCH_UPPER);
		}
	}

	//get the maximum rotation this frame
	rotRate = gameTimeAdjustedIncrement(rotRate);
	rotRate = MAX(rotRate, DEG(1));
	pitchRate = gameTimeAdjustedIncrement(pitchRate);
	pitchRate = MAX(pitchRate, DEG(1));

	//and point the turret at target
	targetRotation = calcDirection(psAttacker->pos.x, psAttacker->pos.y, psTarget->pos.x, psTarget->pos.y);

	//restrict rotationerror to =/- 180 degrees
	rotationError = angleDelta(targetRotation - (tRotation + psAttacker->rot.direction));

	tRotation += clip(rotationError, -rotRate, rotRate);  // Addition wrapping intentional.
	if (psAttacker->type == OBJ_DROID && isVtolDroid((DROID *)psAttacker))
	{
		// limit the rotation for vtols
		int32_t limit = VTOL_TURRET_LIMIT;
		if (psWeapStats->weaponSubClass == WSC_BOMB || psWeapStats->weaponSubClass == WSC_EMP)
		{
			limit = 0;  // Don't turn bombs.
			rotationTolerance = VTOL_TURRET_LIMIT_BOMB;
		}
		tRotation = (uint16_t)clip(angleDelta(tRotation), -limit, limit);  // Cast wrapping intentional.
	}
	onTarget = abs(angleDelta(targetRotation - (tRotation + psAttacker->rot.direction))) <= rotationTolerance;

	/* set muzzle pitch if direct fire */
	if (!bRepair && (proj_Direct(psWeapStats) || ((psAttacker->type == OBJ_DROID)
													  && !proj_Direct(psWeapStats)
												  && actionInsideMinRange((DROID *)psAttacker, psTarget, psWeapStats))))
	{
		dx = psTarget->pos.x - psAttacker->pos.x;
		dy = psTarget->pos.y - psAttacker->pos.y;
		dz = psTarget->pos.z - psAttacker->pos.z;

		/* get target distance */
		dxy = iHypot(dx, dy);

		targetPitch = iAtan2(dz, dxy);

		/* invert calculations for bottom-mounted weapons (i.e. for vtols) */
		//if (bInvert) { why do anything here? }

		pitchError = angleDelta(targetPitch - tPitch);

		tPitch += clip(pitchError, -pitchRate, pitchRate);  // Addition wrapping intended.
		tPitch = (uint16_t)clip(angleDelta(tPitch), pitchLowerLimit, pitchUpperLimit);  // Cast wrapping intended.
		onTarget = onTarget && targetPitch == tPitch;

		/* re-invert result for bottom-mounted weapons (i.e. for vtols) */
		//if (bInvert) { why do anything here? }

	}

	psWeapon->rot.direction = tRotation;
	psWeapon->rot.pitch = tPitch;

	return onTarget;
}


// return whether a droid can see a target to fire on it
BOOL actionVisibleTarget(DROID *psDroid, BASE_OBJECT *psTarget, int weapon_slot)
{
	WEAPON_STATS	*psStats;
	int compIndex;

	CHECK_DROID(psDroid);
	ASSERT_OR_RETURN(false, psTarget != NULL, "Target is NULL");

	if (psDroid->numWeaps == 0)
	{
		if ( visibleObject(psDroid, psTarget, false) )
		{
			return true;
		}
	}

	if (isVtolDroid(psDroid))
	{
		if ( visibleObject(psDroid, psTarget, false) )
		{
			return true;
		}
		return false;
	}
	compIndex = psDroid->asWeaps[weapon_slot].nStat;
	ASSERT_OR_RETURN( false, compIndex < numWeaponStats, "Invalid range referenced for numWeaponStats, %d > %d", compIndex, numWeaponStats);
	psStats = asWeaponStats + compIndex;

	if (proj_Direct(psStats))
	{
		if (visibleObject(psDroid, psTarget, true))
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
			if (visibleObject(psDroid, psTarget, false))
			{
				return true;
			}
		}
	}

	return false;
}

static void actionAddVtolAttackRun( DROID *psDroid )
{
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
	iA = iHypot(deltaX, deltaY);

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
		debug( LOG_NEVER, "*** actionAddVtolAttackRun: run off map! ***" );
	}
	else
	{
		moveDroidToDirect( psDroid, iX, iY );
	}
}

static void actionUpdateVtolAttack( DROID *psDroid )
{
	WEAPON_STATS	*psWeapStats[DROID_MAXWEAPS] = { NULL, NULL, NULL };
	UBYTE i;

	CHECK_DROID(psDroid);

	/* don't do attack runs whilst returning to base */
	if ( psDroid->order == DORDER_RTB )
	{
		return;
	}

	/* if I am a multi-turret droid */
	if (psDroid->numWeaps > 1)
	{
		for(i = 0;i < psDroid->numWeaps;i++)
		{
			if (psDroid->asWeaps[i].nStat != 0)
			{
				psWeapStats[i] = asWeaponStats + psDroid->asWeaps[i].nStat;
				ASSERT(psWeapStats != NULL, "invalid weapon stats pointer");
				break;
			}
		}
	}
	else
	{
		if (psDroid->asWeaps[0].nStat > 0)
		{
			psWeapStats[0] = asWeaponStats + psDroid->asWeaps[0].nStat;
			ASSERT(psWeapStats != NULL, "invalid weapon stats pointer");
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
	len = iHypot(xdiff, ydiff);

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

	ASSERT_OR_RETURN(false, psStats != NULL && psDroid != NULL, "Bad stat or droid");
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
static BOOL actionDroidOnBuildPos(DROID *psDroid, SDWORD x, SDWORD y, BASE_STATS *psStats)
{
	SDWORD	width, breadth, tx,ty, dx,dy;

	CHECK_DROID(psDroid);
	ASSERT_OR_RETURN(false, psStats != NULL, "Bad stat");

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

	ASSERT_OR_RETURN(, player >= 0 && player < MAX_PLAYERS, "Invalid player number %d", (int)player);

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

// Update the action state for a droid
void actionUpdateDroid(DROID *psDroid)
{
	BASE_OBJECT			*psTarget;
	PROPULSION_STATS	*psPropStats;
	BOOL				(*actionUpdateFunc)(DROID *psDroid) = NULL;
	signed int i;
	//this is a bit field
	bool nonNullWeapon[DROID_MAXWEAPS] = { false };
	BASE_OBJECT			*psTargets[DROID_MAXWEAPS];
	bool bHasTarget;

	CHECK_DROID(psDroid);

	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	ASSERT_OR_RETURN(, psPropStats != NULL, "Invalid propulsion stats pointer");

	// clear the target if it has died
	for (i = 0; i < DROID_MAXWEAPS; i++)
	{
		if (psDroid->psActionTarget[i] && psDroid->psActionTarget[i]->died)
		{
			setDroidActionTarget(psDroid, NULL, i);
			if (i == 0)
			{
				if (psDroid->action != DACTION_MOVEFIRE &&
				    psDroid->action != DACTION_TRANSPORTIN &&
				    psDroid->action != DACTION_TRANSPORTOUT)
				{
					psDroid->action = DACTION_NONE;
					// if VTOL - return to rearm pad if not patrolling
					if (isVtolDroid(psDroid))
					{
						if (psDroid->order == DORDER_PATROL)
						{
							// Back to the patrol.
							actionDroid(psDroid, DACTION_MOVE, psDroid->orderX,psDroid->orderY);
						}
						else
						{
							moveToRearm(psDroid);
						}
					}
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
			psDroid->lastHitWeapon = WSC_NUM_WEAPON_SUBCLASSES;
		}
		else
		{
			//get out without updating
			return;
		}
	}

	for (i = 0; i < psDroid->numWeaps; ++i)
	{
		if (psDroid->asWeaps[i].nStat > 0)
		{
			nonNullWeapon[i] = true;
		}
	}

	// HACK: Apparently we can't deal with a droid that only has NULL weapons ?
	// FIXME: Find out whether this is really necessary
	if (psDroid->numWeaps <= 1)
		nonNullWeapon[0] = true;

	psTarget = psDroid->psTarget;

	switch (psDroid->action)
	{
	case DACTION_NONE:
	case DACTION_WAITFORREPAIR:
		// doing nothing
		// see if there's anything to shoot.
		if (psDroid->numWeaps > 0 && !isVtolDroid(psDroid)
		    && (psDroid->order == DORDER_NONE || psDroid->order == DORDER_TEMP_HOLD || psDroid->order == DORDER_RTR))
		{
			for (i = 0;i < psDroid->numWeaps;i++)
			{
				if (nonNullWeapon[i])
				{
					BASE_OBJECT *psTemp = NULL;

					WEAPON_STATS* const psWeapStats = &asWeaponStats[psDroid->asWeaps[i].nStat];
					if (psDroid->asWeaps[i].nStat > 0
					 && psWeapStats->rotate
					 && aiBestNearestTarget(psDroid, &psTemp, i, NULL) >= 0)
					{
						if (secondaryGetState(psDroid, DSO_ATTACK_LEVEL) == DSS_ALEV_ALWAYS)
						{
							psDroid->action = DACTION_ATTACK;
							setDroidActionTarget(psDroid, psTemp, 0);
						}
					}
				}
			}
		}
		// Still not doing anything? See if we need to self repair.
		if ((psDroid->action == DACTION_NONE || psDroid->action == DACTION_WAITFORREPAIR)
		    && selfRepairEnabled(psDroid->player))
		{
			droidSelfRepair(psDroid);
		}
		break;
	case DACTION_WAITDURINGREPAIR:
		// Check that repair facility still exists
		if (!psDroid->psTarget)
		{
			psDroid->action = DACTION_NONE;
			break;
		}
		// move back to the repair facility if necessary
		if (DROID_STOPPED(psDroid) &&
			!actionReachedBuildPos(psDroid,
						(SDWORD)psDroid->psTarget->pos.x,(SDWORD)psDroid->psTarget->pos.y,
						((STRUCTURE*)psDroid->psTarget)->pStructureType ) )
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
				UDWORD droidX, droidY;

				if (!droidRemove(psDroid, mission.apsDroidLists))
				{
					ASSERT_OR_RETURN( , false, "Unable to remove transporter from mission list" );
				}
				addDroid(psDroid, apsDroidLists);
				//set the x/y up since they were set to INVALID_XY when moved offWorld
				missionGetTransporterExit(selectedPlayer, &droidX, &droidY);
				psDroid->pos.x = droidX;
				psDroid->pos.y = droidY;
				//fly Transporter back to get some more droids
				orderDroidLoc( psDroid, DORDER_TRANSPORTIN,
					getLandingX(selectedPlayer), getLandingY(selectedPlayer), ModeImmediate);
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
			actionMove(psDroid, nonNullWeapon);
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
			actionMoveFire(psDroid, nonNullWeapon, &bHasTarget);
			break;

	case DACTION_ATTACK:
		ASSERT_OR_RETURN( , psDroid->psActionTarget[0] != NULL, "target is NULL while attacking");

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
		for (i = 0;i < psDroid->numWeaps;i++)
		{
			BASE_OBJECT* psActionTarget;

			if (i > 0)
			{
				// If we're ordered to shoot something, and we can, shoot it
				if ((psDroid->order == DORDER_ATTACK || psDroid->order == DORDER_ATTACKTARGET) &&
				    psDroid->psActionTarget[i] != psDroid->psActionTarget[0] &&
				    validTarget(psDroid, psDroid->psActionTarget[0], i) &&
				    actionInRange(psDroid, psDroid->psActionTarget[0], i))
				{
					setDroidActionTarget(psDroid, psDroid->psActionTarget[0], i);
				}
				// If we still don't have a target, try to find one
				else
				{
					if (psDroid->psActionTarget[i] == NULL &&
					aiChooseTarget(psDroid, &psTargets[i], i, false, NULL))  // Can probably just use psTarget instead of psTargets[i], and delete the psTargets variable.
					{
						setDroidActionTarget(psDroid, psTargets[i], i);
					}
				}
			}

			if (psDroid->psActionTarget[i])
			{
				psActionTarget = psDroid->psActionTarget[i];
			}
			else
			{
				psActionTarget = psDroid->psActionTarget[0];
			}

			if (nonNullWeapon[i]
			 && actionVisibleTarget(psDroid, psActionTarget, i)
			 && actionInRange(psDroid, psActionTarget, i)
			 && (psDroid->order == DORDER_ATTACK
			  || psDroid->order == DORDER_ATTACKTARGET
			  || !aiObjectIsProbablyDoomed(psActionTarget)))
			{
				WEAPON_STATS* const psWeapStats = &asWeaponStats[psDroid->asWeaps[i].nStat];
				bHasTarget = true;
				if (validTarget(psDroid, psActionTarget, i))
				{
					int dirDiff = 0;

					if (!psWeapStats->rotate)
					{
						// no rotating turret - need to check aligned with target
						const uint16_t targetDir = calcDirection(psDroid->pos.x, psDroid->pos.y, psActionTarget->pos.x, psActionTarget->pos.y);
						dirDiff = abs(angleDelta(targetDir - psDroid->rot.direction));
					}

					if (dirDiff > FIXED_TURRET_DIR)
					{
						if (i > 0)
						{
							if (psDroid->psActionTarget[i] != psDroid->psActionTarget[0])
							{
								// Nope, can't shoot this, try something else next time
								setDroidActionTarget(psDroid, NULL, i);
							}
						}
						else if (psDroid->sMove.Status != MOVESHUFFLE)
						{
							psDroid->action = DACTION_ROTATETOATTACK;
							moveTurnDroid(psDroid, psActionTarget->pos.x, psActionTarget->pos.y);
						}
					}
					else if (!psWeapStats->rotate ||
							actionTargetTurret(psDroid, psActionTarget, &psDroid->asWeaps[i]))
					{
						/* In range - fire !!! */
						combFire(&psDroid->asWeaps[i], psDroid, psActionTarget, i);
					}
				}
				else if (i > 0)
				{
					// Nope, can't shoot this, try something else next time
					setDroidActionTarget(psDroid, NULL, i);
				}
			}
			else if (i > 0)
			{
				// Nope, can't shoot this, try something else next time
				setDroidActionTarget(psDroid, NULL, i);
			}
		}

		if (!bHasTarget)
		{
			if (((psDroid->order == DORDER_ATTACKTARGET
			   || psDroid->order == DORDER_FIRESUPPORT)
			  && secondaryGetState(psDroid, DSO_HALTTYPE) == DSS_HALT_HOLD)
			 || (!isVtolDroid(psDroid)
			  && (psTarget = orderStateObj(psDroid, DORDER_FIRESUPPORT))
			  && psTarget->type == OBJ_STRUCTURE)
			  || (psDroid->order == DORDER_NONE) || (psDroid->order == DORDER_TEMP_HOLD)
			  || (psDroid->order == DORDER_RTR))
			{
				// don't move if on hold or firesupport for a sensor tower
				// also don't move if we're holding position or waiting for repair
				psDroid->action = DACTION_NONE;				// holding, cancel the order.
			}
			else
			{
				psDroid->action = DACTION_MOVETOATTACK;	// out of range - chase it
			}
		}

		break;

	case DACTION_VTOLATTACK:
	{
		WEAPON_STATS* psWeapStats = NULL;

		//uses vtResult
		if (psDroid->psActionTarget[0] != NULL &&
			validTarget(psDroid, psDroid->psActionTarget[0], 0))
		{
			//check if vtol that its armed
			if ( (vtolEmpty(psDroid)) ||
				(psDroid->psActionTarget[0] == NULL) ||
				//check the target hasn't become one the same player ID - Electronic Warfare
				(electronicDroid(psDroid) && (psDroid->player == psDroid->psActionTarget[0]->player)) ||
				!validTarget(psDroid, psDroid->psActionTarget[0], 0) )
			{
				moveToRearm(psDroid);
				break;
			}

			for(i = 0;i <psDroid->numWeaps;i++)
			{
				if (nonNullWeapon[i]
				 && validTarget(psDroid, psDroid->psActionTarget[0], i))
				{
					//I moved psWeapStats flag update there
					psWeapStats = &asWeaponStats[psDroid->asWeaps[i].nStat];
					if (actionVisibleTarget(psDroid, psDroid->psActionTarget[0], i))
					{
						if ( actionInRange(psDroid, psDroid->psActionTarget[0], i) )
						{
							if ( psDroid->player == selectedPlayer )
							{
								audio_QueueTrackMinDelay( ID_SOUND_COMMENCING_ATTACK_RUN2,
															VTOL_ATTACK_AUDIO_DELAY );
							}

							if (actionTargetTurret(psDroid, psDroid->psActionTarget[0], &psDroid->asWeaps[i]))
							{
									// In range - fire !!!
									combFire(&psDroid->asWeaps[i], psDroid,
									psDroid->psActionTarget[0], i);
							}
						}
						else
						{
							actionTargetTurret(psDroid, psDroid->psActionTarget[0], &psDroid->asWeaps[i]);
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
			const Vector2i diff = removeZ(psDroid->pos - psDroid->psActionTarget[0]->pos);
			const int rangeSq = diff*diff;
			if (rangeSq < VTOL_ATTACK_TARDIST*VTOL_ATTACK_TARDIST)
			{
				// don't do another attack run if already moving away from the target
				const Vector2i diff = psDroid->sMove.destination - removeZ(psDroid->psActionTarget[0]->pos);
				if (diff*diff < VTOL_ATTACK_TARDIST*VTOL_ATTACK_TARDIST)
				{
					actionAddVtolAttackRun( psDroid );
				}
			}
			// in case psWeapStats is still NULL
			else if (psWeapStats)
			{	// if the vtol is far enough away head for the target again
				if (rangeSq > (SDWORD)(psWeapStats->longRange * psWeapStats->longRange))
				{
					// don't do another attack run if already heading for the target
					const Vector2i diff = psDroid->sMove.destination - removeZ(psDroid->psActionTarget[0]->pos);
					if (diff*diff > VTOL_ATTACK_TARDIST*VTOL_ATTACK_TARDIST)
					{
						moveDroidToDirect(psDroid, psDroid->psActionTarget[0]->pos.x,psDroid->psActionTarget[0]->pos.y);
					}
				}
			}
		}
		break;
	}
	case DACTION_MOVETOATTACK:
		// send vtols back to rearm
		if (isVtolDroid(psDroid) &&
			vtolEmpty(psDroid))
		{
			moveToRearm(psDroid);
			break;
		}

		ASSERT_OR_RETURN( , psDroid->psActionTarget[0] != NULL, "action update move to attack target is NULL");

		//check the target hasn't become one the same player ID - Electronic Warfare
		if ((electronicDroid(psDroid) && (psDroid->player == psDroid->psActionTarget[0]->player)) ||
			!validTarget(psDroid, psDroid->psActionTarget[0], 0) )
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
					if (nonNullWeapon[i]
					 && validTarget(psDroid, psDroid->psActionTarget[0], i)
					 && actionVisibleTarget(psDroid, psDroid->psActionTarget[0], i))
					{
						bool chaseBloke = false;
						WEAPON_STATS* const psWeapStats = &asWeaponStats[psDroid->asWeaps[i].nStat];

						if (psWeapStats->rotate)
						{
							actionTargetTurret(psDroid, psDroid->psActionTarget[0], &psDroid->asWeaps[i]);
						}

						if (!isVtolDroid(psDroid) &&
							psDroid->psActionTarget[0]->type == OBJ_DROID &&
							((DROID *)psDroid->psActionTarget[0])->droidType == DROID_PERSON &&
							psWeapStats->fireOnMove != FOM_NO)
						{
							chaseBloke = true;
						}


						if (actionInAttackRange(psDroid, psDroid->psActionTarget[0], i)
						 && !chaseBloke)
						{
							/* Stop the droid moving any closer */
		//				    ASSERT( psDroid->pos.x != 0 && psDroid->pos.y != 0,
		//						"moveUpdateUnit: Unit at (0,0)" );

							/* init vtol attack runs count if necessary */
							if ( psPropStats->propulsionType == PROPULSION_TYPE_LIFT )
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
							combFire(&psDroid->asWeaps[i], psDroid, psDroid->psActionTarget[0], i);
						}
					}
				}
			}
			else
			{
				for(i = 0;i < psDroid->numWeaps;i++)
				{
					if ((psDroid->asWeaps[i].rot.direction != 0) ||
						(psDroid->asWeaps[i].rot.pitch != 0))
					{
						actionAlignTurret(psDroid, i);
					}
				}
			}

			if (DROID_STOPPED(psDroid) && psDroid->action != DACTION_ATTACK)
			{
				/* Stopped moving but haven't reached the target - possibly move again */

				//'hack' to make the droid to check the primary turrent instead of all
				WEAPON_STATS* const psWeapStats = &asWeaponStats[psDroid->asWeaps[0].nStat];

				if (psDroid->order == DORDER_ATTACKTARGET
				 && secondaryGetState(psDroid, DSO_HALTTYPE) == DSS_HALT_HOLD)
				{
					psDroid->action = DACTION_NONE;			// on hold, give up.
				}
				else if (actionInsideMinRange(psDroid, psDroid->psActionTarget[0], psWeapStats))
				{
					if ( proj_Direct( psWeapStats ) )
					{
						SDWORD pbx, pby;

						// try and extend the range
						actionCalcPullBackPoint(psDroid, psDroid->psActionTarget[0], &pbx,&pby);
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
// 			debug( LOG_NEVER, "sulk over.. %p", psDroid );
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
		if (!psDroid->psTarStats)
		{
			psDroid->action = DACTION_NONE;
			break;
		}
		// moving to a location to build a structure
		if (actionReachedBuildPos(psDroid,
						(SDWORD)psDroid->orderX,(SDWORD)psDroid->orderY, psDroid->psTarStats) &&
			!actionDroidOnBuildPos(psDroid,
						(SDWORD)psDroid->orderX,(SDWORD)psDroid->orderY, psDroid->psTarStats))
		{
			bool helpBuild = false;
			// Got to destination - start building
			STRUCTURE_STATS* const psStructStats = (STRUCTURE_STATS*)psDroid->psTarStats;
			uint16_t dir = psDroid->orderDirection;
			moveStopDroid(psDroid);
			objTrace(psDroid->id, "Halted in our tracks - at construction site");
			if (psDroid->order == DORDER_BUILD && psDroid->psTarget == NULL)
			{
				// Starting a new structure
				// calculate the top left of the structure
				const UDWORD tlx = psDroid->orderX - getStructureStatsWidth(psStructStats, dir) * TILE_UNITS/2;
				const UDWORD tly = psDroid->orderY - getStructureStatsBreadth(psStructStats, dir) * TILE_UNITS/2;

				//need to check if something has already started building here?
				//unless its a module!
				if (IsStatExpansionModule(psStructStats))
				{
					syncDebug("Reached build target: module");
					debug( LOG_NEVER, "DACTION_MOVETOBUILD: setUpBuildModule");
					setUpBuildModule(psDroid);
				}
				else if (TileHasStructure(mapTile(map_coord(psDroid->orderX), map_coord(psDroid->orderY))))
				{
					// structure on the build location - see if it is the same type
					STRUCTURE* const psStruct = getTileStructure(map_coord(psDroid->orderX), map_coord(psDroid->orderY));
					if (psStruct->pStructureType == (STRUCTURE_STATS *)psDroid->psTarStats)
					{
						// same type - do a help build
						syncDebug("Reached build target: do-help");
						setDroidTarget(psDroid, psStruct);
						helpBuild = true;
					}
					else if ((psStruct->pStructureType->type == REF_WALL ||
					psStruct->pStructureType->type == REF_WALLCORNER) &&
						((STRUCTURE_STATS *)psDroid->psTarStats)->type == REF_DEFENSE)
					{
							// building a gun tower over a wall - OK
							if (droidStartBuild(psDroid))
							{
								syncDebug("Reached build target: tower");
								debug( LOG_NEVER, "DACTION_MOVETOBUILD: start foundation");
								psDroid->action = DACTION_BUILD;
							}
							else
							{
								syncDebug("Reached build target: wall-in-way");
								psDroid->action = DACTION_NONE;
							}
					}
					else
					{
						syncDebug("Reached build target: already-structure");
						objTrace(psDroid->id, "DACTION_MOVETOBUILD: tile has structure already");
						cancelBuild(psDroid);
					}
				}
				else if (!validLocation(psDroid->psTarStats,
										map_coord(tlx),
										map_coord(tly), dir,
										psDroid->player,
										false))
				{
					syncDebug("Reached build target: invalid");
					objTrace(psDroid->id, "DACTION_MOVETOBUILD: !validLocation");
					cancelBuild(psDroid);
				}
				else
				{
					syncDebug("Reached build target: build");
					psDroid->action = DACTION_BUILD_FOUNDATION;
					psDroid->actionStarted = gameTime;
					psDroid->actionPoints = 0;
				}
			}
			else if ((psDroid->order == DORDER_LINEBUILD || psDroid->order==DORDER_BUILD)
			         && (psStructStats->type == REF_WALL || psStructStats->type == REF_WALLCORNER || psStructStats->type == REF_GATE ||
			             psStructStats->type == REF_DEFENSE || psStructStats->type == REF_REARM_PAD))
			{
				// building a wall.
				MAPTILE* const psTile = mapTile(map_coord(psDroid->orderX), map_coord(psDroid->orderY));
				syncDebug("Reached build target: wall");
				if (psDroid->psTarget == NULL
				 && (TileHasStructure(psTile)
				  || TileHasFeature(psTile)))
				{
					if (TileHasStructure(psTile))
					{
						// structure on the build location - see if it is the same type
						STRUCTURE* const psStruct = getTileStructure(map_coord(psDroid->orderX), map_coord(psDroid->orderY));
						if (psStruct->pStructureType == (STRUCTURE_STATS *)psDroid->psTarStats)
						{
							// same type - do a help build
							setDroidTarget(psDroid, psStruct);
							helpBuild = true;
						}
						else if ((psStruct->pStructureType->type == REF_WALL || psStruct->pStructureType->type == REF_WALLCORNER) &&
								((STRUCTURE_STATS *)psDroid->psTarStats)->type == REF_DEFENSE)
						{
							// building a gun tower over a wall - OK
							if (droidStartBuild(psDroid))
							{
								debug( LOG_NEVER, "DACTION_MOVETOBUILD: start foundation");
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
				syncDebug("Reached build target: planned-help");
				helpBuild = true;
			}

			if (helpBuild)
			{
				// continuing a partially built structure (order = helpBuild)
				if (droidStartBuild(psDroid))
				{
					debug( LOG_NEVER, "DACTION_MOVETOBUILD: starting help build");
					psDroid->action = DACTION_BUILD;
					intBuildStarted(psDroid);
				}
				else
				{
					debug( LOG_NEVER, "DACTION_MOVETOBUILD: giving up (3)");
					psDroid->action = DACTION_NONE;
				}
			}
		}
		else if (DROID_STOPPED(psDroid))
		{
			if (actionDroidOnBuildPos(psDroid,
						(SDWORD)psDroid->orderX,(SDWORD)psDroid->orderY, psDroid->psTarStats))
			{
				SDWORD pbx = 0, pby = 0;

				actionHomeBasePos(psDroid->player, &pbx, &pby);
				if (pbx == 0 || pby == 0)
				{
					objTrace(psDroid->id, "DACTION_MOVETOBUILD: No HQ, cannot move in that direction");
					psDroid->action = DACTION_NONE;
					break;
				}
				objTrace(psDroid->id, "DACTION_MOVETOBUILD: Starting to drive inside construction site");
				moveDroidToNoFormation(psDroid, (UDWORD)pbx,(UDWORD)pby);
			}
			else
			{
				objTrace(psDroid->id, "DACTION_MOVETOBUILD: Starting to drive toward construction site - move status was %d", (int)psDroid->sMove.Status);
				moveDroidToNoFormation(psDroid, psDroid->actionX,psDroid->actionY);
			}
		}
		break;
	case DACTION_BUILD:
		if (!psDroid->psTarStats)
		{
			psDroid->action = DACTION_NONE;
			break;
		}
		if (DROID_STOPPED(psDroid) &&
			!actionReachedBuildPos(psDroid,
						(SDWORD)psDroid->orderX,(SDWORD)psDroid->orderY, psDroid->psTarStats))
		{
			objTrace(psDroid->id, "DACTION_BUILD: Starting to drive toward construction site");
			moveDroidToNoFormation(psDroid, psDroid->orderX, psDroid->orderY);
		}
		else if (!DROID_STOPPED(psDroid) &&
				psDroid->sMove.Status != MOVETURNTOTARGET &&
				psDroid->sMove.Status != MOVESHUFFLE &&
				actionReachedBuildPos(psDroid,
						(SDWORD)psDroid->orderX,(SDWORD)psDroid->orderY, psDroid->psTarStats))
		{
			objTrace(psDroid->id, "DACTION_BUILD: Stopped - at construction site");
			moveStopDroid(psDroid);
		}
		if (psDroid->action == DACTION_SULK)
		{
			objTrace(psDroid->id, "Failed to go to objective, aborting build action");
			psDroid->action = DACTION_NONE;
			break;
		}
		if (droidUpdateBuild(psDroid))
		{
			actionTargetTurret(psDroid, psDroid->psActionTarget[0], &psDroid->asWeaps[0]);
		}
		break;
	case DACTION_MOVETODEMOLISH:
	case DACTION_MOVETOREPAIR:
	case DACTION_MOVETOCLEAR:
	case DACTION_MOVETORESTORE:
		if (!psDroid->psTarStats)
		{
			psDroid->action = DACTION_NONE;
			break;
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
				SDWORD pbx = 0, pby = 0;

				actionHomeBasePos(psDroid->player, &pbx, &pby);
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
		if (!psDroid->psTarStats)
		{
			psDroid->action = DACTION_NONE;
			break;
		}
		// set up for the specific action
		switch (psDroid->action)
		{
		case DACTION_DEMOLISH:
			// DACTION_MOVETODEMOLISH;
			actionUpdateFunc = droidUpdateDemolishing;
			break;
		case DACTION_REPAIR:
			// DACTION_MOVETOREPAIR;
			actionUpdateFunc = droidUpdateRepair;
			break;
		case DACTION_CLEARWRECK:
			// DACTION_MOVETOCLEAR;
			actionUpdateFunc = droidUpdateClearing;
			break;
		case DACTION_RESTORE:
			// DACTION_MOVETORESTORE;
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
			if (secondaryGetState(psDroid, DSO_HALTTYPE) != DSS_HALT_HOLD ||
			    (psDroid->order != DORDER_NONE && psDroid->order != DORDER_TEMP_HOLD))
			{
				objTrace(psDroid->id, "Secondary order: Go to construction site");
				moveDroidToNoFormation(psDroid, psDroid->actionX,psDroid->actionY);
			}
			else
			{
				psDroid->action = DACTION_NONE;
			}
		}
		else if (!DROID_STOPPED(psDroid) &&
				psDroid->sMove.Status != MOVETURNTOTARGET &&
				psDroid->sMove.Status != MOVESHUFFLE &&
				actionReachedBuildPos(psDroid,
						(SDWORD)psDroid->actionX,(SDWORD)psDroid->actionY, psDroid->psTarStats))
		{
			objTrace(psDroid->id, "Stopped - reached build position");
			moveStopDroid(psDroid);
		}
		else if ( actionUpdateFunc(psDroid) )
		{
			//use 0 for non-combat(only 1 'weapon')
			actionTargetTurret(psDroid, psDroid->psActionTarget[0], &psDroid->asWeaps[0]);
		}
		else if (psDroid->action == DACTION_CLEARWRECK)
		{
			//see if there is any other wreckage in the area
			FEATURE* const psNextWreck = checkForWreckage(psDroid);
			psDroid->action = DACTION_NONE;
			if (psNextWreck)
			{
				orderDroidObj(psDroid, DORDER_CLEARWRECK, psNextWreck, ModeImmediate);
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
		/* moving from front to rear of repair facility or rearm pad */
		if (actionReachedBuildPos(psDroid, psDroid->psActionTarget[0]->pos.x,psDroid->psActionTarget[0]->pos.y,
							((STRUCTURE *)psDroid->psActionTarget[0])->pStructureType))
		{
			objTrace(psDroid->id, "Arrived at repair point - waiting for our turn");
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
		if (!psDroid->psTarStats)
		{
			psDroid->action = DACTION_NONE;
			break;
		}
		//building a structure's foundation - flattening the ground for now
		{
			MAPTILE* const psTile = mapTile(map_coord(psDroid->orderX), map_coord(psDroid->orderY));
			STRUCTURE_STATS* const psStructStats = (STRUCTURE_STATS*)psDroid->psTarStats;
			uint16_t dir = psDroid->orderDirection;
			const UDWORD tlx = psDroid->orderX - getStructureStatsWidth(psStructStats, dir) * TILE_UNITS/2;
			const UDWORD tly = psDroid->orderY - getStructureStatsBreadth(psStructStats, dir) * TILE_UNITS/2;
			if ((psDroid->psTarget == NULL) &&
				(TileHasStructure(psTile) ||
				TileHasFeature(psTile)))
			{
				if (TileHasStructure(psTile))
				{
					// structure on the build location - see if it is the same type
					STRUCTURE* const psStruct = getTileStructure(map_coord(psDroid->orderX), map_coord(psDroid->orderY));
					if (psStruct->pStructureType == (STRUCTURE_STATS *)psDroid->psTarStats)
					{
						// same type - do a help build
						setDroidTarget(psDroid, psStruct);
					}
					else
					{
						psDroid->action = DACTION_NONE;
					}
				}
				else if (!validLocation(psDroid->psTarStats,
				                        map_coord(tlx),
				                        map_coord(tly), dir,
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
				debug( LOG_NEVER, "DACTION_BUILD_FOUNDATION: start build");
				psDroid->action = DACTION_BUILD;
			}
			else
			{
				debug( LOG_NEVER, "DACTION_BUILD_FOUNDATION: giving up");
				psDroid->action = DACTION_NONE;
			}
		}
		break;
	case DACTION_OBSERVE:
		// align the turret
		actionTargetTurret(psDroid, psDroid->psActionTarget[0], &psDroid->asWeaps[0]);

		// WSS shouldn't get a free pass to hit anything on map
		if ((cbSensorDroid(psDroid) && asSensorStats[psDroid->asBits[COMP_SENSOR].nStat].type != SUPER_SENSOR)
		    || objRadarDetector(psDroid))
		{
			// don't move to the target, just make sure it is visible
			// Anyone commenting this out will get a knee capping from John.
			// You have been warned!!
			psDroid->psActionTarget[0]->visible[psDroid->player] = UBYTE_MAX;
		}
		else
		{
			// make sure the target is within sensor range
			const int xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psDroid->psActionTarget[0]->pos.x;
			const int ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psDroid->psActionTarget[0]->pos.y;
			int rangeSq = droidSensorRange(psDroid);
			rangeSq = rangeSq * rangeSq;
			if (!visibleObject(psDroid, psDroid->psActionTarget[0], false)
			 || xdiff * xdiff + ydiff * ydiff >= rangeSq)
			{
				if (secondaryGetState(psDroid, DSO_HALTTYPE) == DSS_HALT_HOLD &&
					(psDroid->order == DORDER_NONE || psDroid->order == DORDER_TEMP_HOLD))
				{
					psDroid->action = DACTION_NONE;
				}
				else
				{
					psDroid->action = DACTION_MOVETOOBSERVE;
					moveDroidTo(psDroid, psDroid->psActionTarget[0]->pos.x, psDroid->psActionTarget[0]->pos.y);
				}
			}
		}
		break;
	case DACTION_MOVETOOBSERVE:
		// align the turret
		actionTargetTurret(psDroid, psDroid->psActionTarget[0], &psDroid->asWeaps[0]);

		if (visibleObject(psDroid, psDroid->psActionTarget[0], false))
		{
			// make sure the target is within sensor range
			const int xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psDroid->psActionTarget[0]->pos.x;
			const int ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psDroid->psActionTarget[0]->pos.y;
			int rangeSq = droidSensorRange(psDroid);
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
		if (!psDroid->psTarget)
		{
			psDroid->action = DACTION_NONE;
			return;
		}
		//can be either a droid or a structure now - AB 7/10/98
		ASSERT_OR_RETURN(, (psDroid->psTarget->type == OBJ_DROID || psDroid->psTarget->type == OBJ_STRUCTURE)
				   && (psDroid->psTarget->player == psDroid->player), "DACTION_FIRESUPPORT: incorrect target type" );

			//don't move VTOL's
			// also don't move closer to sensor towers
			if (!isVtolDroid(psDroid) &&
				(psDroid->psTarget->type != OBJ_STRUCTURE))
			{
				//move droids to within short range of the sensor now!!!!
				Vector2i diff = removeZ(psDroid->pos - psDroid->psTarget->pos);
				int rangeSq = asWeaponStats[psDroid->asWeaps[0].nStat].shortRange;
				rangeSq = rangeSq * rangeSq;
				if (diff*diff < rangeSq)
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
						diff = removeZ(psDroid->psTarget->pos) - psDroid->sMove.destination;
					}

					if (DROID_STOPPED(psDroid) || diff*diff > rangeSq)
					{
						if (secondaryGetState(psDroid, DSO_HALTTYPE) == DSS_HALT_HOLD)
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
				debug(LOG_DEATH, "Droid %d destructed", (int)psDroid->id);
				destroyDroid(psDroid);
			}
		}
		break;
	case DACTION_MOVETODROIDREPAIR:
	{
		// moving to repair a droid
		const int xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psDroid->psActionTarget[0]->pos.x;
		const int ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psDroid->psActionTarget[0]->pos.y;
		if ( xdiff*xdiff + ydiff*ydiff < REPAIR_RANGE )
		{
			// Got to destination - start repair
			//rotate turret to point at droid being repaired
			//use 0 for repair droid
			actionTargetTurret(psDroid, psDroid->psActionTarget[0], &psDroid->asWeaps[0]);
			if (droidStartDroidRepair(psDroid))
			{
				psDroid->action = DACTION_DROIDREPAIR;
			}
			else
			{
				psDroid->action = DACTION_NONE;
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
	}
	case DACTION_DROIDREPAIR:
	{
		int xdiff, ydiff;

		// If not doing self-repair (psActionTarget[0] is repair target)
		if (psDroid->psActionTarget[0] != psDroid)
		{
			actionTargetTurret(psDroid, psDroid->psActionTarget[0], &psDroid->asWeaps[0]);
		}
		// Just self-repairing.
		// See if there's anything to shoot.
		else if (psDroid->numWeaps > 0 && !isVtolDroid(psDroid)
		          && (psDroid->order == DORDER_NONE || psDroid->order == DORDER_TEMP_HOLD || psDroid->order == DORDER_RTR))
		{
			for (i = 0;i < psDroid->numWeaps;i++)
			{
				if (nonNullWeapon[i])
				{
					BASE_OBJECT *psTemp = NULL;

					WEAPON_STATS* const psWeapStats = &asWeaponStats[psDroid->asWeaps[i].nStat];
					if (psDroid->asWeaps[i].nStat > 0 && psWeapStats->rotate
					    && secondaryGetState(psDroid, DSO_ATTACK_LEVEL) == DSS_ALEV_ALWAYS
					    && aiBestNearestTarget(psDroid, &psTemp, i, NULL) >= 0 && psTemp)
					{
						psDroid->action = DACTION_ATTACK;
						setDroidActionTarget(psDroid, psTemp, 0);
						break;
					}
				}
			}
		}
		if (psDroid->action != DACTION_DROIDREPAIR)
		{
			break;	// action has changed
		}

		//check still next to the damaged droid
		xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psDroid->psActionTarget[0]->pos.x;
		ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psDroid->psActionTarget[0]->pos.y;
		if (xdiff * xdiff + ydiff * ydiff > REPAIR_RANGE)
		{
			if (secondaryGetState(psDroid, DSO_HALTTYPE) != DSS_HALT_HOLD || psDroid->order == DORDER_REPAIR)
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
				// damaged droid has moved off - follow if we're not holding position!
				psDroid->actionX = psDroid->psActionTarget[0]->pos.x;
				psDroid->actionY = psDroid->psActionTarget[0]->pos.y;
				psDroid->action = DACTION_MOVETODROIDREPAIR;
				moveDroidTo(psDroid, psDroid->actionX, psDroid->actionY);
			}
			else
			{
				psDroid->action = DACTION_NONE;
			}
		}
		else
		{
			if (!droidUpdateDroidRepair(psDroid))
			{
				psDroid->action = DACTION_NONE;
				//if the order is RTR then resubmit order so that the unit will go to repair facility point
				if (orderState(psDroid,DORDER_RTR))
				{
					orderDroid(psDroid, DORDER_RTR, ModeImmediate);
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
	}
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

		if (visibleObject(psDroid, psDroid->psActionTarget[0], false))
		{
			STRUCTURE* const psStruct = findNearestReArmPad(psDroid, (STRUCTURE *)psDroid->psActionTarget[0], true);
			// got close to the rearm pad - now find a clear one
			objTrace(psDroid->id, "Seen rearm pad - searching for available one");

			if (psStruct != NULL)
			{
				// found a clear landing pad - go for it
				objTrace(psDroid->id, "Found clear rearm pad");
				setDroidActionTarget(psDroid, psStruct, 0);
			}

			psDroid->action = DACTION_WAITFORREARM;
		}

		if (DROID_STOPPED(psDroid) ||
			(psDroid->action == DACTION_WAITFORREARM))
		{
			UDWORD droidX = psDroid->psActionTarget[0]->pos.x;
			UDWORD droidY = psDroid->psActionTarget[0]->pos.y;
			if (!actionVTOLLandingPos(psDroid, &droidX, &droidY))
			{
				// totally bunged up - give up
				objTrace(psDroid->id, "Couldn't find a clear tile near rearm pad - returning to base");
				orderDroid(psDroid, DORDER_RTB, ModeImmediate);
				break;
			}
			moveDroidToDirect(psDroid, droidX,droidY);
		}
		break;
	default:
		ASSERT(!"unknown action", "unknown action");
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

		//use 0 for all non-combat droid types
		if (psDroid->numWeaps == 0)
		{
			if (psDroid->asWeaps[0].rot.direction != 0 || psDroid->asWeaps[0].rot.pitch != 0)
			{
				actionAlignTurret(psDroid, 0);
			}
		}
		else
		{
			for (i = 0;i < psDroid->numWeaps;i++)
			{
				if (psDroid->asWeaps[i].rot.direction != 0 || psDroid->asWeaps[i].rot.pitch != 0)
				{
					actionAlignTurret(psDroid, i);
				}
			}
		}
	}
	CHECK_DROID(psDroid);
}

/* Overall action function that is called by the specific action functions */
static void actionDroidBase(DROID *psDroid, DROID_ACTION_DATA *psAction)
{
	SDWORD			pbx,pby;
	WEAPON_STATS		*psWeapStats = getWeaponStats(psDroid, 0);
	UDWORD			droidX,droidY;
	BASE_OBJECT		*psTarget;
	//added MinRangeResult;
	UBYTE	i;

	CHECK_DROID(psDroid);

	psDroid->actionStarted = gameTime;
	syncDebugDroid(psDroid, '-');
	syncDebug("%d does %s", psDroid->id, getDroidActionName(psAction->action));

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
			(psAction->psObj == psDroid))
		{
			break;
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
		   || psDroid->order == DORDER_NONE
		   || psDroid->order == DORDER_TEMP_HOLD
		   || psDroid->order == DORDER_FIRESUPPORT)
		  && secondaryGetState(psDroid, DSO_HALTTYPE) == DSS_HALT_HOLD)
		   || (!isVtolDroid(psDroid)
		    && (psTarget = orderStateObj(psDroid, DORDER_FIRESUPPORT))
		    && psTarget->type == OBJ_STRUCTURE))
		{
			psDroid->action = DACTION_ATTACK;		// holding, try attack straightaway
		}
		else if (actionInsideMinRange(psDroid, psAction->psObj, psWeapStats))
		{
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
				actionCalcPullBackPoint(psDroid, psAction->psObj, &pbx,&pby);

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
			orderDroid(psDroid, DORDER_RTB, ModeImmediate);
			break;
		}
		moveDroidToDirect(psDroid, droidX, droidY);
		break;
	case DACTION_CLEARREARMPAD:
		debug( LOG_NEVER, "Unit %d clearing rearm pad", psDroid->id);
		psDroid->action = DACTION_CLEARREARMPAD;
		setDroidActionTarget(psDroid, psAction->psObj, 0);
		droidX = psDroid->psActionTarget[0]->pos.x;
		droidY = psDroid->psActionTarget[0]->pos.y;
		if (!actionVTOLLandingPos(psDroid, &droidX, &droidY))
		{
			// totally bunged up - give up
			orderDroid(psDroid, DORDER_RTB, ModeImmediate);
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
		if (!psDroid->psTarStats)
		{
			psDroid->action = DACTION_NONE;
			break;
		}
		ASSERT(psDroid->order == DORDER_BUILD || psDroid->order == DORDER_HELPBUILD ||
				psDroid->order == DORDER_LINEBUILD,
			"cannot start build action without a build order");
		ASSERT_OR_RETURN( , psAction->x > 0 && psAction->y > 0, "Bad build order position");
		psDroid->action = DACTION_MOVETOBUILD;
		psDroid->actionX = psAction->x;
		psDroid->actionY = psAction->y;
		if (actionDroidOnBuildPos(psDroid, psDroid->orderX, psDroid->orderY, psDroid->psTarStats))
		{
			actionHomeBasePos(psDroid->player, &pbx,&pby);
			if (pbx == 0 || pby == 0)
			{
				objTrace(psDroid->id, "Setting DACTION_BUILD: No HQ, cannot move in that direction");
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
		ASSERT(psDroid->order == DORDER_DEMOLISH,
			"cannot start demolish action without a demolish order");
		psDroid->action = DACTION_MOVETODEMOLISH;
		psDroid->actionX = psAction->x;
		psDroid->actionY = psAction->y;
		ASSERT((psDroid->psTarget != NULL) && (psDroid->psTarget->type == OBJ_STRUCTURE),
			"invalid target for demolish order" );
		psDroid->psTarStats = ((STRUCTURE *)psDroid->psTarget)->pStructureType;
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
		ASSERT((psDroid->psActionTarget[0] != NULL) && (psDroid->psActionTarget[0]->type == OBJ_STRUCTURE),
			"invalid target for demolish order" );
		psDroid->psTarStats = ((STRUCTURE *)psDroid->psActionTarget[0])->pStructureType;
		if (secondaryGetState(psDroid, DSO_HALTTYPE) == DSS_HALT_HOLD &&
		    (psDroid->order == DORDER_NONE || psDroid->order == DORDER_TEMP_HOLD))
		{
			psDroid->action = DACTION_REPAIR;
		}
		else
		{
			psDroid->action = DACTION_MOVETOREPAIR;
			moveDroidTo(psDroid, psAction->x, psAction->y);
		}
		break;
	case DACTION_OBSERVE:
		setDroidActionTarget(psDroid, psAction->psObj, 0);
		psDroid->actionX = psDroid->pos.x;
		psDroid->actionY = psDroid->pos.y;
		if ((secondaryGetState(psDroid, DSO_HALTTYPE) == DSS_HALT_HOLD &&
		    (psDroid->order == DORDER_NONE || psDroid->order == DORDER_TEMP_HOLD)) ||
			cbSensorDroid(psDroid) || objRadarDetector(psDroid))
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
		if(!isVtolDroid(psDroid) &&
			(secondaryGetState(psDroid, DSO_HALTTYPE) != DSS_HALT_HOLD) &&	// check hold
			(psDroid->psTarget->type != OBJ_STRUCTURE))
		{
			moveDroidTo(psDroid, psDroid->psTarget->pos.x, psDroid->psTarget->pos.y);		// movetotarget.
		}

		break;
	case DACTION_SULK:
// 		debug( LOG_NEVER, "Go with sulk ... %p", psDroid );
		psDroid->action = DACTION_SULK;
		// hmmm, hope this doesn't cause any problems!
		psDroid->actionStarted = gameTime + MIN_SULK_TIME + (gameRand(MAX_SULK_TIME - MIN_SULK_TIME));
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
		debug( LOG_NEVER, "Unit %d moving to rearm point", psDroid->id);
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
		psDroid->actionStarted = gameTime;
		if (secondaryGetState(psDroid, DSO_HALTTYPE) == DSS_HALT_HOLD &&
		    (psDroid->order == DORDER_NONE || psDroid->order == DORDER_TEMP_HOLD))
		{
			psDroid->action = DACTION_DROIDREPAIR;
		}
		else
		{
			psDroid->action = DACTION_MOVETODROIDREPAIR;
			moveDroidTo(psDroid, psAction->x, psAction->y);
		}
		break;
	case DACTION_RESTORE:
		ASSERT( psDroid->order == DORDER_RESTORE,
			"cannot start restore action without a restore order" );
		psDroid->action = DACTION_MOVETORESTORE;
		psDroid->actionX = psAction->x;
		psDroid->actionY = psAction->y;
		ASSERT( (psDroid->psTarget != NULL) && (psDroid->psTarget->type == OBJ_STRUCTURE),
			"invalid target for restore order" );
		psDroid->psTarStats = ((STRUCTURE *)psDroid->psTarget)->pStructureType;
		setDroidActionTarget(psDroid, psAction->psObj, 0);
		moveDroidTo(psDroid, psAction->x, psAction->y);
		break;
	case DACTION_CLEARWRECK:
		ASSERT( psDroid->order == DORDER_CLEARWRECK,
			"cannot start clear action without a clear order" );
		psDroid->action = DACTION_MOVETOCLEAR;
		psDroid->actionX = psAction->x;
		psDroid->actionY = psAction->y;
		ASSERT( (psDroid->psTarget != NULL) && (psDroid->psTarget->type == OBJ_FEATURE),
			"invalid target for demolish order" );
		psDroid->psTarStats = (BASE_STATS *)((FEATURE *)psDroid->psTarget)->psStats;
		setDroidActionTarget(psDroid, psDroid->psTarget, 0);
		moveDroidTo(psDroid, psAction->x, psAction->y);
		break;

	default:
		ASSERT(!"unknown action", "actionUnitBase: unknown action");
		break;
	}
	syncDebugDroid(psDroid, '+');
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
void actionDroid(DROID *psDroid, DROID_ACTION action, UDWORD x, UDWORD y)
{
	DROID_ACTION_DATA	sAction;

	memset(&sAction, 0, sizeof(DROID_ACTION_DATA));
	sAction.action = action;
	sAction.x = x;
	sAction.y = y;
	actionDroidBase(psDroid, &sAction);
}

/* Give a droid an action with an object target */
void actionDroid(DROID *psDroid, DROID_ACTION action, BASE_OBJECT *psObj)
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
void actionDroid(DROID *psDroid, DROID_ACTION action,
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

	CHECK_DROID(psDroid);

	if (!isVtolDroid(psDroid))
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
			orderDroidObj(psDroid, DORDER_REARM, psStruct, ModeImmediate);
		}
		else
		{
			actionDroid(psDroid, DACTION_MOVETOREARM, psStruct);
		}
	}
	else
	{
		//return to base un-armed
		orderDroid(psDroid, DORDER_RTB, ModeImmediate);
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
		(TileIsOccupied(psTile)) ||
		(terrainType(psTile) == TER_CLIFFFACE) ||
		(terrainType(psTile) == TER_WATER))
	{
		return false;
	}
	return true;
}

/**
 * Performs a space-filling spiral-like search from startX,startY up to (and
 * including) radius. For each tile, the search function is called; if it
 * returns 'true', the search will finish immediately.
 *
 * @param startX,startY starting x and y coordinates
 *
 * @param max_radius radius to examine. Search will finish when @c max_radius is exceeded.
 *
 * @param match searchFunction to use; described in typedef
 * \param matchState state for the search function
 * \return true if finished because the searchFunction requested termination,
 *         false if the radius limit was reached
 */
static bool spiralSearch(int startX, int startY, int max_radius, tileMatchFunction match, void* matchState)
{
	int radius;          // radius counter

	// test center tile
	if (match(startX, startY, matchState))
	{
		return true;
	}

	// test for each radius, from 1 to max_radius (inclusive)
	for (radius = 1; radius <= max_radius; ++radius)
	{
		// choose tiles that are between radius and radius+1 away from center
		// distances are squared
		const int min_distance = radius * radius;
		const int max_distance = min_distance + 2 * radius;

		// X offset from startX
		int dx;

		// dx starts with 1, to visiting tiles on same row or col as start twice
		for (dx = 1; dx <= max_radius; dx++)
		{
			// Y offset from startY
			int dy;

			for (dy = 0; dy <= max_radius; dy++)
			{
				// Current distance, squared
				const int distance = dx * dx + dy * dy;

				// Ignore tiles outside of the current circle
				if (distance < min_distance || distance > max_distance)
				{
					continue;
				}

				// call search function for each of the 4 quadrants of the circle
				if (match(startX + dx, startY + dy, matchState)
				 || match(startX - dx, startY - dy, matchState)
				 || match(startX + dy, startY - dx, matchState)
				 || match(startX - dy, startY + dx, matchState))
				{
					return true;
				}
			}
		}
	}

	return false;
}

/**
 * an internal tileMatchFunction that checks if x and y are coordinates of a
 * valid landing place.
 *
 * @param matchState a pointer to a Vector2i where these coordintates should be stored
 *
 * @return true if coordinates are a valid landing tile, false if not.
 */
static bool vtolLandingTileSearchFunction(int x, int y, void* matchState)
{
	Vector2i* const xyCoords = (Vector2i*)matchState;

	if (vtolLandingTile(x, y))
	{
		xyCoords->x = x;
		xyCoords->y = y;
		return true;
	}

	return false;
}

// choose a landing position for a VTOL when it goes to rearm
bool actionVTOLLandingPos(const DROID* psDroid, UDWORD* px, UDWORD* py)
{
	int startX, startY;
	DROID* psCurr;
	bool   foundTile;
	Vector2i xyCoords;

	CHECK_DROID(psDroid);

	/* Initial box dimensions and set iteration count to zero */
	startX = map_coord(*px);
	startY = map_coord(*py);

	// set blocking flags for all the other droids
	for(psCurr=apsDroidLists[psDroid->player]; psCurr; psCurr = psCurr->psNext)
	{
		Vector2i t;
		if (DROID_STOPPED(psCurr))
		{
			t = map_coord(removeZ(psCurr->pos));
		}
		else
		{
			t = map_coord(psCurr->sMove.destination);
		}
		if (psCurr != psDroid)
		{
			if (tileOnMap(t))
			{
				mapTile(t)->tileInfoBits |= BITS_FPATHBLOCK;
			}
		}
	}

	// search for landing tile; will stop when found or radius exceeded
	foundTile = spiralSearch(startX, startY, vtolLandingRadius,
	    vtolLandingTileSearchFunction, &xyCoords);
	if (foundTile)
	{
		debug( LOG_NEVER, "Unit %d landing pos (%d,%d)",
		       psDroid->id, xyCoords.x, xyCoords.y);
		*px = world_coord(xyCoords.x) + TILE_UNITS / 2;
		*py = world_coord(xyCoords.y) + TILE_UNITS / 2;
	}

	// clear blocking flags for all the other droids
	for(psCurr=apsDroidLists[psDroid->player]; psCurr; psCurr = psCurr->psNext)
	{
		Vector2i t;
		if (DROID_STOPPED(psCurr))
		{
			t = map_coord(removeZ(psCurr->pos));
		}
		else
		{
			t = map_coord(psCurr->sMove.destination);
		}
		if (tileOnMap(t))
		{
			mapTile(t)->tileInfoBits &= ~BITS_FPATHBLOCK;
		}
	}

	return foundTile;
}

void actionMove(DROID *psDroid, bool *nonNullWeapon)
{
	int i;
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
	
	//added multiple weapon check
	else if (psDroid->numWeaps > 0)
	{
		for(i = 0;i < psDroid->numWeaps;i++)
		{
			if (nonNullWeapon[i])
			{
				BASE_OBJECT *psTemp = NULL;
				
				//I moved psWeapStats flag update there
				WEAPON_STATS* const psWeapStats = &asWeaponStats[psDroid->asWeaps[i].nStat];
				if (!isVtolDroid(psDroid)
						&& psDroid->asWeaps[i].nStat > 0
						&& psWeapStats->rotate
						&& psWeapStats->fireOnMove != FOM_NO
						&& aiBestNearestTarget(psDroid, &psTemp, i, NULL) >= 0)
				{
					if (secondaryGetState(psDroid, DSO_ATTACK_LEVEL) == DSS_ALEV_ALWAYS)
					{
						psDroid->action = DACTION_MOVEFIRE;
						setDroidActionTarget(psDroid, psTemp, 0);
					}
				}
			}
		}
	}
}

void actionMoveFire(DROID *psDroid, bool *nonNullWeapon, bool *bHasTarget)
{
	signed int i;
	unsigned int j;
	bool hasVisibleTarget = false;
	bool targetVisibile[DROID_MAXWEAPS] = { false };
	
	//check if vtol that its armed
	if (vtolEmpty(psDroid))
	{
		moveToRearm(psDroid);
	}
	
	*bHasTarget = false;
	//loop through weapons and look for target for each weapon
	for (i = 0; i < psDroid->numWeaps; ++i)
	{
		if (psDroid->psActionTarget[i] != NULL && aiObjectIsProbablyDoomed(psDroid->psActionTarget[i]))
		{
			setDroidActionTarget(psDroid, NULL, i);  // Target not worth shooting at anymore.
		}
		
		if (psDroid->psActionTarget[i] == NULL)
		{
			BASE_OBJECT *psTemp;
			
			if (aiBestNearestTarget(psDroid, &psTemp, i, NULL) >= 0)
			{
				*bHasTarget = true;
				setDroidActionTarget(psDroid, psTemp, i);
			}
		}
		
		if (psDroid->psActionTarget[i]
				&& visibleObject(psDroid, psDroid->psActionTarget[i], false))
		{
			hasVisibleTarget = true;
			targetVisibile[i] = true;
		}
	}
	
	for (j = 0;j < psDroid->numWeaps;j++)
	{
		//vtResult uses psActionTarget[0] for now since it's the first target
		if (psDroid->psActionTarget[j] != NULL &&
				validTarget(psDroid, psDroid->psActionTarget[j], j))
		{
			// firing on something while moving
			if (DROID_STOPPED(psDroid))
			{
				// Got to desitination
				psDroid->action = DACTION_NONE;
				break;
			}
			else if (psDroid->psActionTarget[j] == NULL
							 || !validTarget(psDroid, psDroid->psActionTarget[j], j)
							 || (secondaryGetState(psDroid, DSO_ATTACK_LEVEL) != DSS_ALEV_ALWAYS))
			{
				if (j == (psDroid->numWeaps - 1) && !(*bHasTarget))
				{
					// Target lost
					psDroid->action = DACTION_MOVE;
				}
				else
				{
					continue;
				}
				//if Vtol - return to rearm pad
				/*if (isVtolDroid(psDroid))
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
				if (!hasVisibleTarget
						&& !(*bHasTarget)
						&& j == (psDroid->numWeaps - 1))
				{
					// lost the target
					psDroid->action = DACTION_MOVE;
					for (i = 0; i < psDroid->numWeaps;i++)
					{
						setDroidActionTarget(psDroid, NULL, i);
					}
				}
				
				if (targetVisibile[j])
				{
					*bHasTarget = true;
					//to fix a AA-weapon attack ground unit exploit
					if (nonNullWeapon[j])
					{
						BASE_OBJECT* psActionTarget = psDroid->psActionTarget[j];
						
						if (!psActionTarget)
						{
							if (targetVisibile[0])
							{
								psActionTarget = psDroid->psActionTarget[0];
							}
						}
						
						if (psActionTarget && validTarget(psDroid, psActionTarget, j)
								&& actionTargetTurret(psDroid, psActionTarget, &psDroid->asWeaps[j]))
						{
							// In range - fire !!!
							combFire(&psDroid->asWeaps[j], psDroid, psActionTarget, j);
						}
					}
				}
			}
		}
	}
	
	/* Extra bit of paranoia, in case none of the weapons have a target */
	*bHasTarget = false;
	for (i = 0; i < psDroid->numWeaps; i++)
	{
		if (psDroid->psActionTarget[i] != NULL)
		{
			*bHasTarget = true;
			break;
		}
	}
	if (!(*bHasTarget))
	{
		psDroid->action = DACTION_MOVE;
	}
	
	//check its a VTOL unit since adding Transporter's into multiPlayer
	/* check vtol attack runs */
	if (isVtolDroid(psDroid))
	{
		actionUpdateVtolAttack( psDroid );
	}
	
}	
