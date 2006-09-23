/*
 * Action.c
 *
 * Functions for setting the action of a droid
 *
 */

#include "lib/framework/frame.h"
#include "lib/framework/trig.h"

#include "objects.h"
#include "action.h"
#include "map.h"
#include "combat.h"
#include "geometry.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_common/ivisdef.h"
#include "visibility.h"
#include "projectile.h"
#include "order.h"
#include "hci.h"
#include "transporter.h"
#include "console.h"
#include "research.h"
#include "drive.h"
#include "mission.h"
#include "audio_id.h"
#include "multiplay.h"
#include "formation.h"
#include "intdisplay.h"
#include "fpath.h"
#include "lib/script/script.h"
#include "scripttabs.h"

/* attack run distance */
#define	VTOL_ATTACK_LENGTH		1000
#define	VTOL_ATTACK_WIDTH		200
#define VTOL_ATTACK_TARDIST		400
#define VTOL_ATTACK_RETURNDIST	700

// turret rotation limits
#define VTOL_TURRET_RLIMIT		315
#define VTOL_TURRET_LLIMIT		45

// time to pause before a droid blows up
#define  ACTION_DESTRUCT_TIME	2000
//#define PITCH_UPPER_LIMIT	60
//#define PITCH_LOWER_LIMIT	-15


#define ACTION_TURRET_ROTATION_RATE 180
#define REPAIR_PITCH_LOWER		30
#define	REPAIR_PITCH_UPPER		-15

//how long to follow a damaged droid around before giving up if don't get near
#define KEEP_TRYING_REPAIR	10000
//how far away the repair droid can be from the damaged droid to function
#define REPAIR_RANGE		(TILE_UNITS * TILE_UNITS * 4)

//how many tiles to pull back
#define PULL_BACK_DIST		10

// data required for any action
typedef struct _droid_action_data
{
	DROID_ACTION	action;
	UDWORD			x,y;
	BASE_OBJECT		*psObj;
	BASE_STATS		*psStats;
} DROID_ACTION_DATA;

// Check if a droid has stoped moving
#define DROID_STOPPED(psDroid) \
	(psDroid->sMove.Status == MOVEINACTIVE || psDroid->sMove.Status == MOVEHOVER || \
	 psDroid->sMove.Status == MOVESHUFFLE)

// choose a landing position for a VTOL when it goes to rearm
BOOL actionVTOLLandingPos(DROID *psDroid, UDWORD *px, UDWORD *py);

/* Check if a target is at correct range to attack */
BOOL actionInAttackRange(DROID *psDroid, BASE_OBJECT *psObj)
{
	SDWORD			dx, dy, dz, radSq, rangeSq, longRange;
	SECONDARY_STATE state;
	WEAPON_STATS	*psStats;

	//if (psDroid->numWeaps == 0)
    if (psDroid->asWeaps[0].nStat == 0)
	{
		return FALSE;
	}

	psStats = asWeaponStats + psDroid->asWeaps[0].nStat;

	dx = (SDWORD)psDroid->x - (SDWORD)psObj->x;
	dy = (SDWORD)psDroid->y - (SDWORD)psObj->y;
	dz = (SDWORD)psDroid->z - (SDWORD)psObj->z;

	radSq = dx*dx + dy*dy;

	if ((psDroid->order == DORDER_ATTACKTARGET) &&
		secondaryGetState(psDroid, DSO_HALTTYPE, &state) &&
		(state == DSS_HALT_HOLD))
	{
		longRange = proj_GetLongRange(psStats, dz);
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
				longRange = proj_GetLongRange(psStats, dz);
				rangeSq = longRange * longRange;
			}
			break;
		case DSS_ARANGE_SHORT:
			rangeSq = psStats->shortRange * psStats->shortRange;
			break;
		case DSS_ARANGE_LONG:
			longRange = proj_GetLongRange(psStats, dz);
			rangeSq = longRange * longRange;
			break;
		default:
			ASSERT( FALSE, "actionInAttackRange: unknown attack range order" );
			longRange = proj_GetLongRange(psStats, dz);
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
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}

	return FALSE;
}


// check if a target is within weapon range
BOOL actionInRange(DROID *psDroid, BASE_OBJECT *psObj)
{
	SDWORD			dx, dy, dz, radSq, rangeSq, longRange;
	WEAPON_STATS	*psStats;

	//if (psDroid->numWeaps == 0)
    if (psDroid->asWeaps[0].nStat == 0)
	{
		return FALSE;
	}

	psStats = asWeaponStats + psDroid->asWeaps[0].nStat;

	dx = (SDWORD)psDroid->x - (SDWORD)psObj->x;
	dy = (SDWORD)psDroid->y - (SDWORD)psObj->y;
	dz = (SDWORD)psDroid->z - (SDWORD)psObj->z;

	radSq = dx*dx + dy*dy;

	longRange = proj_GetLongRange(psStats, dz);
	rangeSq = longRange * longRange;

	/* check max range */
	if ( radSq <= rangeSq )
	{
		/* check min range */
		rangeSq = psStats->minRange * psStats->minRange;
		if ( radSq >= rangeSq || !proj_Direct( psStats ) )
		{
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}

	return FALSE;
}



// check if a target is inside minimum weapon range
BOOL actionInsideMinRange(DROID *psDroid, BASE_OBJECT *psObj)
{
	SDWORD			dx, dy, dz, radSq, rangeSq, minRange;
	WEAPON_STATS	*psStats;

	//if (psDroid->numWeaps == 0)
    if (psDroid->asWeaps[0].nStat == 0)
	{
		return FALSE;
	}

	psStats = asWeaponStats + psDroid->asWeaps[0].nStat;

	dx = (SDWORD)psDroid->x - (SDWORD)psObj->x;
	dy = (SDWORD)psDroid->y - (SDWORD)psObj->y;
	dz = (SDWORD)psDroid->z - (SDWORD)psObj->z;

	radSq = dx*dx + dy*dy;

	minRange = (SDWORD)psStats->minRange;
	rangeSq = minRange * minRange;

	// check min range
	if ( radSq <= rangeSq )
	{
		return TRUE;
	}

	return FALSE;
}



/* Find a new location by a structure when building */
/*static BOOL actionNewBuildPos(DROID *psDroid, UDWORD *pX, UDWORD *pY)
{
	//STRUCTURE_STATS		*psStructStats;
	BASE_STATS				*psStats;

	// get the structure stats
	switch (psDroid->action)
	{
	case DACTION_MOVETOBUILD:
	case DACTION_FOUNDATION_WANDER:
		if ( psDroid->order == DORDER_BUILD || psDroid->order == DORDER_LINEBUILD )
		{
			//psStructStats = (STRUCTURE_STATS *)psDroid->psTarStats;
			psStats = psDroid->psTarStats;
		}
		else
		{
			ASSERT( psDroid->order == DORDER_HELPBUILD,
				"actionNewBuildPos: invalid order" );
			//psStructStats = ((STRUCTURE *)psDroid->psTarget)->pStructureType;
			psStats = (BASE_STATS *)((STRUCTURE *)psDroid->psTarget)->pStructureType;
		}
		break;
	case DACTION_BUILD:
	case DACTION_BUILDWANDER:
	case DACTION_MOVETODEMOLISH:
	//case DACTION_MOVETOREPAIR:
	case DACTION_MOVETORESTORE:
		//psStructStats = ((STRUCTURE *)psDroid->psTarget)->pStructureType;
		psStats = (BASE_STATS *)((STRUCTURE *)psDroid->psTarget)->pStructureType;
		break;
	case DACTION_MOVETOREPAIR:
	case DACTION_MOVETOREARM:
		psStats = (BASE_STATS *)((STRUCTURE *)psDroid->psActionTarget)->pStructureType;
		break;
	case DACTION_MOVETOCLEAR:
		psStats = (BASE_STATS *)((FEATURE *)psDroid->psTarget)->psStats;
		break;
	default:
		ASSERT( FALSE,
			"actionNewBuildPos: invalid action" );
		return FALSE;
		break;
	}
	//ASSERT( PTRVALID(psStructStats, sizeof(STRUCTURE_STATS)),
	//	"actionNewBuildPos: invalid structure stats pointer" );

	// find a new destination
	//if (getDroidDestination(psStructStats, psDroid->orderX, psDroid->orderY,pX,pY))
	if (psDroid->action == DACTION_MOVETOREARM OR psDroid->action == DACTION_MOVETOREPAIR)
	{
		//use action target
		if (getDroidDestination(psStats, psDroid->actionX, psDroid->actionY, pX, pY))
		{
			return TRUE;
		}
	}
	else
	{
		//use order target
		if (getDroidDestination(psStats, psDroid->orderX, psDroid->orderY, pX, pY))
		{
			return TRUE;
		}
	}

	return FALSE;
}*/


// Realign turret
void actionAlignTurret(BASE_OBJECT *psObj)
{
	UDWORD				rotation;
	UWORD				tRot, tPitch, nearest = 0;
	//get the maximum rotation this frame

	//rotation = (psDroid->turretRotRate * frameTime) / (4 * GAME_TICKS_PER_SEC);
    rotation = (ACTION_TURRET_ROTATION_RATE * frameTime) / (4 * GAME_TICKS_PER_SEC);
	if (rotation == 0)
	{
		rotation = 1;
	}

	switch (psObj->type)
	{
	case OBJ_DROID:
		tRot = ((DROID *)psObj)->turretRotation;
		tPitch = ((DROID *)psObj)->turretPitch;
		break;
	case OBJ_STRUCTURE:
		tRot = ((STRUCTURE *)psObj)->turretRotation;
		tPitch = ((STRUCTURE *)psObj)->turretPitch;

		// now find the nearest 90 degree angle
		nearest = (UWORD)(((tRot + 45) / 90) * 90);
		tRot = (UWORD)(((tRot + 360) - nearest) % 360);
		break;
	default:
		ASSERT( FALSE, "actionAlignTurret: invalid object type" );
		return;
		break;
	}


//	DBP1(("rotrate=%d framTime=%d rotation=%d\n",psDroid->turretRotRate,frameTime,rotation));

	//DBP1(("droid=%x turret=%x\n",psDroid,psDroid->turretRotation));
    DBP1(("unit=%x turret=%x\n",psDroid,ACTION_TURRET_ROTATION_RATE));


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
		((DROID *)psObj)->turretRotation = tRot;
		((DROID *)psObj)->turretPitch = tPitch;
		break;
	case OBJ_STRUCTURE:
		// now adjust back to the nearest 90 degree angle
		tRot = (UWORD)((tRot + nearest) % 360);

		((STRUCTURE *)psObj)->turretRotation = tRot;
		((STRUCTURE *)psObj)->turretPitch = tPitch;
		break;
	default:
		ASSERT( FALSE, "actionAlignTurret: invalid object type" );
		return;
		break;
	}
}

#define HEAVY_WEAPON_WEIGHT     50000

/* returns true if on target */

//BOOL actionTargetTurret(BASE_OBJECT *psAttacker, BASE_OBJECT *psTarget, UDWORD *pRotation,
//		UDWORD *pPitch, SDWORD rotRate, SDWORD pitchRate, BOOL bDirectFire, BOOL bInvert)
//BOOL actionTargetTurret(BASE_OBJECT *psAttacker, BASE_OBJECT *psTarget, UWORD *pRotation,
//		UWORD *pPitch, SWORD rotRate, SWORD pitchRate, BOOL bDirectFire, BOOL bInvert)
BOOL actionTargetTurret(BASE_OBJECT *psAttacker, BASE_OBJECT *psTarget, UWORD *pRotation,
		UWORD *pPitch, WEAPON_STATS *psWeapStats, BOOL bInvert)
{
	SWORD  tRotation, tPitch, rotRate, pitchRate;
	SDWORD  targetRotation,targetPitch;
	SDWORD  pitchError;
	SDWORD	rotationError, dx, dy, dz;
	BOOL	onTarget = FALSE;
	FRACT	fR;
	SDWORD	pitchLowerLimit, pitchUpperLimit;
	DROID	*psDroid = NULL;
//	iVector	muzzle;

    //these are constants now and can be set up at the start of the function
    rotRate = ACTION_TURRET_ROTATION_RATE;
    pitchRate = (SWORD)(ACTION_TURRET_ROTATION_RATE/2);

    //added for 22/07/99 upgrade - AB
    if (psWeapStats)
    {
        //extra heavy weapons on some structures need to rotate and pitch more slowly
        if (psWeapStats->weight > HEAVY_WEAPON_WEIGHT)
        {
            rotRate = (SWORD)(ACTION_TURRET_ROTATION_RATE/2 - (100 *
                (psWeapStats->weight - HEAVY_WEAPON_WEIGHT) / psWeapStats->weight));
            pitchRate = (SWORD) (rotRate / 2);
        }
    }

	tRotation = *pRotation;
	tPitch = *pPitch;

	//set the pitch limits based on the weapon stats of the attacker
	pitchLowerLimit = pitchUpperLimit = 0;
	if (psAttacker->type == OBJ_STRUCTURE)
	{
		pitchLowerLimit = asWeaponStats[((STRUCTURE *)psAttacker)->asWeaps[0].
			nStat].minElevation;
		pitchUpperLimit = asWeaponStats[((STRUCTURE *)psAttacker)->asWeaps[0].
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
			pitchLowerLimit = asWeaponStats[psDroid->asWeaps[0].
				nStat].minElevation;
			pitchUpperLimit = asWeaponStats[psDroid->asWeaps[0].
				nStat].maxElevation;
		}
		else if ( psDroid->droidType == DROID_REPAIR )
		{
			pitchLowerLimit = REPAIR_PITCH_LOWER;
			pitchUpperLimit = REPAIR_PITCH_UPPER;
		}
	}

	//get the maximum rotation this frame
	rotRate = (SWORD)(rotRate * frameTime / GAME_TICKS_PER_SEC);
	if (rotRate > 180)//crop to 180 degrees, no point in turning more than all the way round
	{
		rotRate = 180;
	}
	if (rotRate <= 0)
	{
		rotRate = 1;
	}
	pitchRate = (SWORD)(pitchRate * frameTime / GAME_TICKS_PER_SEC);
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
	targetRotation = calcDirection(psAttacker->x, psAttacker->y, psTarget->x, psTarget->y);

	//DBP1(("att: rotrate=%d framTime=%d rotation=%d target=%d   startrot=%d ",psAttacker->turretRotRate,frameTime,rotation,targetRotation,tRotation));
    DBP1(("att: rotrate=%d framTime=%d rotation=%d target=%d   startrot=%d ",
        ACTION_TURRET_ROTATION_RATE,frameTime,rotation,targetRotation,tRotation));


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
		if ((SDWORD)psAttacker->direction > targetRotation)
		{
			tRotation = (SWORD)(targetRotation + 360 - psAttacker->direction);
		}
		else
		{
			tRotation = (SWORD)(targetRotation - psAttacker->direction);
		}
			DBP1(("locked on target...\n"));
		onTarget = TRUE;
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
//	if ( asWeaponStats[psAttacker->asWeaps->nStat].direct == TRUE )
	if ( psWeapStats != NULL &&
		 ( proj_Direct( psWeapStats ) ||
		 ( (psAttacker->type == OBJ_DROID) &&
			!proj_Direct( psWeapStats ) &&
			actionInsideMinRange(psDroid, psDroid->psActionTarget) )) )
	{
// difference between muzzle position and droid origin is unlikely to affect aiming
// particularly as target origin is used
//		calcDroidMuzzleLocation( psAttacker, &muzzle);
		dx = psTarget->x - psAttacker->x;//muzzle.x;
		dy = psTarget->y - psAttacker->y;//muzzle.y;
//		dz = map_Height(psTarget->x, psTarget->y) - psAttacker->z;//muzzle.z;
		dz = psTarget->z - psAttacker->z;//muzzle.z;

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
			onTarget = FALSE;
		}
		else if (pitchError > pitchRate)
		{
			// add rotation
			tPitch = (SWORD)(tPitch + pitchRate);
			onTarget = FALSE;
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
			onTarget = FALSE;
		}
		else if (tPitch > pitchUpperLimit)
		{
			// add rotation
			tPitch = (SWORD)pitchUpperLimit;
			onTarget = FALSE;
		}

		if (tPitch < 0)
		{
			tPitch += 360;
		}
	}

	*pRotation = tRotation;
	*pPitch = tPitch;

	DBP1(("endrot=%d\n",tRotation));

	return onTarget;
}


// return whether a droid can see a target to fire on it
BOOL actionVisibleTarget(DROID *psDroid, BASE_OBJECT *psTarget)
{
	WEAPON_STATS	*psStats;

	//if (psDroid->numWeaps == 0)
    if ((psDroid->asWeaps[0].nStat == 0) ||
		vtolDroid(psDroid))
	{
		return visibleObject((BASE_OBJECT*)psDroid, psTarget);
	}

	psStats = asWeaponStats + psDroid->asWeaps[0].nStat;
	if (proj_Direct(psStats))
	{
		if (visibleObjWallBlock((BASE_OBJECT*)psDroid, psTarget))
		{
			return TRUE;
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
				return TRUE;
			}
		}
		else
		{
			if (visibleObject((BASE_OBJECT*)psDroid, psTarget))
			{
				return TRUE;
			}
		}
	}

	return FALSE;
}

static void actionAddVtolAttackRun( DROID *psDroid )
{
	FRACT_D		fA;
	SDWORD		iVNx, iVNy, iA, iX, iY;
	BASE_OBJECT	*psTarget;
#if 0
	SDWORD		iVx, iVy;
#endif

	if ( psDroid->psActionTarget != NULL )
	{
		psTarget = psDroid->psActionTarget;
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
	iVNx = psTarget->x - psDroid->x;
	iVNy = psTarget->y - psDroid->y;

	/* get magnitude of normal vector */
	fA = trigIntSqrt( iVNx*iVNx + iVNy*iVNy );
	iA = MAKEINT(fA);

#if 0
	/* get left perpendicular to normal vector:
	 * swap normal vector elements and negate y:
	 * scale to attack ellipse width
	 */
	iVx =  iVNy * VTOL_ATTACK_WIDTH / iA;
	iVy = -iVNx * VTOL_ATTACK_WIDTH / iA;

	/* add waypoint left perpendicular to target*/
	iX = psTarget->x + iVx;
	iY = psTarget->y + iVy;
//	orderAddWayPoint( psDroid, iX, iY );
#endif

	/* add waypoint behind target attack length away*/
	iX = psTarget->x + (iVNx * VTOL_ATTACK_LENGTH / iA);
	iY = psTarget->y + (iVNy * VTOL_ATTACK_LENGTH / iA);
//	orderAddWayPoint( psDroid, iX, iY );

	if ( iX<=0 || iY<=0 ||
		 iX>(SDWORD)(GetWidthOfMap()<<TILE_SHIFT) ||
		 iY>(SDWORD)(GetHeightOfMap()<<TILE_SHIFT)   )
	{
		debug( LOG_NEVER, "*** actionAddVtolAttackRun: run off map! ***\n" );
	}
	else
	{
		moveDroidToDirect( psDroid, iX, iY );
	}

	/* update attack run count - done in projectile.c now every time a bullet is fired*/
	//psDroid->sMove.iAttackRuns++;
}

static void actionUpdateVtolAttack( DROID *psDroid )
{
	WEAPON_STATS	*psWeapStats = NULL;

	/* don't do attack runs whilst returning to base */
	if ( psDroid->order == DORDER_RTB )
	{
		return;
	}

	//if (psDroid->numWeaps > 0)
    if (psDroid->asWeaps[0].nStat > 0)
	{
		psWeapStats = asWeaponStats + psDroid->asWeaps[0].nStat;
		ASSERT( PTRVALID(psWeapStats, sizeof(WEAPON_STATS)),
				"actionUpdateVtolAttack: invalid weapon stats pointer" );
	}

	/* order back to base after fixed number of attack runs */
	if ( psWeapStats != NULL )
	{
		//if ( psDroid->sMove.iAttackRuns >= psWeapStats->vtolAttackRuns )
        if (vtolEmpty(psDroid))
		{
			moveToRearm(psDroid);
			return;
		}
	}

	/* circle around target if hovering and not cyborg */
	if ( psDroid->sMove.Status == MOVEHOVER &&
		 //psDroid->droidType != DROID_CYBORG    )
         !cyborgDroid(psDroid))
	{
		actionAddVtolAttackRun( psDroid );
	}
}

static void actionUpdateTransporter( DROID *psDroid )
{
	//check if transporter has arrived
	if (updateTransporter(psDroid))
	{
		// Got to destination
		psDroid->action = DACTION_NONE;
		return;
	}

    //check the target hasn't become one the same player ID - Electronic Warfare
    if (psDroid->psActionTarget != NULL AND
        psDroid->player == psDroid->psActionTarget->player)
    {
        psDroid->psActionTarget = NULL;
    }

    	/* check for weapon */
	//if ( psDroid->numWeaps > 0 )
    if (psDroid->asWeaps[0].nStat > 0)
    {
	    if (psDroid->psActionTarget == NULL)
	    {
		    aiNearestTarget(psDroid, &psDroid->psActionTarget);
	    }

	    if ( psDroid->psActionTarget != NULL )
	    {
		    if ( visibleObject((BASE_OBJECT*)psDroid, psDroid->psActionTarget) )
		    {
			    /*if (actionTargetTurret((BASE_OBJECT*)psDroid, psDroid->psActionTarget,
						&(psDroid->turretRotation), &(psDroid->turretPitch),
						psDroid->turretRotRate, (SWORD)(psDroid->turretRotRate/2),
						//asWeaponStats[psDroid->asWeaps->nStat].direct))
						proj_Direct(&asWeaponStats[psDroid->asWeaps->nStat]),
						TRUE))*/
			    if (actionTargetTurret((BASE_OBJECT*)psDroid, psDroid->psActionTarget,
						&(psDroid->turretRotation), &(psDroid->turretPitch),
						&asWeaponStats[psDroid->asWeaps->nStat],
						TRUE))
			    {
				    // In range - fire !!!
				    combFire(psDroid->asWeaps, (BASE_OBJECT *)psDroid,
						     psDroid->psActionTarget);
			    }
		    }
		    else
		    {
			    // lost the target
			    psDroid->psActionTarget = NULL;
		    }
        }
	}
}


// calculate a position for units to pull back to if they
// need to increase the range between them and a target
static void actionCalcPullBackPoint(BASE_OBJECT *psObj, BASE_OBJECT *psTarget, SDWORD *px, SDWORD *py)
{
	SDWORD xdiff,ydiff, len;

	// get the vector from the target to the object
	xdiff = (SDWORD)psObj->x - (SDWORD)psTarget->x;
	ydiff = (SDWORD)psObj->y - (SDWORD)psTarget->y;
	len = (SDWORD)iSQRT(xdiff*xdiff + ydiff*ydiff);

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
	*px = (SDWORD)psObj->x + xdiff * PULL_BACK_DIST;
	*py = (SDWORD)psObj->y + ydiff * PULL_BACK_DIST;
}


// check whether a droid is in the neighboring tile to a build position
BOOL actionReachedBuildPos(DROID *psDroid, SDWORD x, SDWORD y, BASE_STATS *psStats)
{
	SDWORD		width, breadth, tx,ty, dx,dy;

	// do all calculations in half tile units so that
	// the droid moves to within half a tile of the target
	// NOT ANY MORE - JOHN
	dx = (SDWORD)(psDroid->x >> (TILE_SHIFT));
	dy = (SDWORD)(psDroid->y >> (TILE_SHIFT));

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

	tx = x >> (TILE_SHIFT);
	ty = y >> (TILE_SHIFT);

	// move the x,y to the top left of the structure
	tx -= width/2;
	ty -= breadth/2;

	if ( (dx == (tx -1)) || (dx == (tx + width)) )
	{
		// droid could be at either the left or the right
		if ( (dy >= (ty -1)) && (dy <= (ty + breadth)) )
		{
			return TRUE;
		}
	}
	else if ( (dy == (ty -1)) || (dy == (ty + breadth)) )
	{
		// droid could be at either the top or the bottom
		if ( (dx >= (tx -1)) && (dx <= (tx + width)) )
		{
			return TRUE;
		}
	}

	return FALSE;
}


// check if a droid is on the foundations of a new building
BOOL actionDroidOnBuildPos(DROID *psDroid, SDWORD x, SDWORD y, BASE_STATS *psStats)
{
	SDWORD	width, breadth, tx,ty, dx,dy;

	dx = (SDWORD)(psDroid->x >> (TILE_SHIFT));
	dy = (SDWORD)(psDroid->y >> (TILE_SHIFT));
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

	tx = (x >> (TILE_SHIFT)) - (width/2);
	ty = (y >> (TILE_SHIFT)) - (breadth/2);

	if ( (dx >= tx) && (dx < tx+width) &&
		 (dy >= ty) && (dy < ty+breadth) )
	{
		return TRUE;
	}

	return FALSE;
}


// return the position of a players home base
void actionHomeBasePos(SDWORD player, SDWORD *px, SDWORD *py)
{
	STRUCTURE	*psStruct;

	ASSERT( player >= 0 && player < MAX_PLAYERS,
		"actionHomeBasePos: invalide player number" );

	for(psStruct = apsStructLists[player]; psStruct; psStruct=psStruct->psNext)
	{
		if (psStruct->pStructureType->type == REF_HQ)
		{
			*px = (SDWORD)psStruct->x;
			*py = (SDWORD)psStruct->y;
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

	if (vtolDroid(psDroid) ||
		((psDroid->order != DORDER_MOVE) &&
		 (psDroid->order != DORDER_SCOUT)))
	{
		return FALSE;
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
				if (psTile->tileInfoBits & BITS_WALL)
				{
					psWall = getTileStructure((UDWORD)i,(UDWORD)j);
					if (psWall->player != psDroid->player)
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
		psDroid->psTarget = (BASE_OBJECT *)psWall;
		return TRUE;
	}

	return FALSE;
}

#define	VTOL_ATTACK_AUDIO_DELAY		(3*GAME_TICKS_PER_SEC)

// Update the action state for a droid
void actionUpdateDroid(DROID *psDroid)
{
//	UDWORD				structX,structY;
	UDWORD				droidX,droidY;
	UDWORD				tlx,tly;
	STRUCTURE			*psStruct;
	STRUCTURE_STATS		*psStructStats;
	BASE_OBJECT			*psTarget = psDroid->psTarget;//, *psObj;
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

	psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	ASSERT( PTRVALID(psPropStats, sizeof(PROPULSION_STATS)),
			"actionUpdateUnit: invalid propulsion stats pointer" );

	ASSERT( psDroid->turretRotation < 360, "turretRotation out of range" );
	ASSERT( psDroid->direction < 360, "unit direction out of range" );

	/* check whether turret inverted for actionTargetTurret */
	//if ( psDroid->droidType != DROID_CYBORG &&
    if ( !cyborgDroid(psDroid) &&
		 psPropStats->propulsionType == LIFT   )
	{
		bInvert = TRUE;
	}
	else
	{
		bInvert = FALSE;
	}

	// clear the target if it has died
	if (psDroid->psActionTarget && psDroid->psActionTarget->died)
	{
		psDroid->psActionTarget = NULL;
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
			formationLeave(psDroid->sMove.psFormation, (BASE_OBJECT *)psDroid);
			psDroid->sMove.psFormation = NULL;
		}
		// move back to the repair facility if necessary
		if (DROID_STOPPED(psDroid) &&
			!actionReachedBuildPos(psDroid,
						(SDWORD)psDroid->psTarget->x,(SDWORD)psDroid->psTarget->y,
						(BASE_STATS *)((STRUCTURE*)psDroid->psTarget)->pStructureType ) )
		{
			moveDroidToNoFormation(psDroid, psDroid->psTarget->x, psDroid->psTarget->y);
		}
		break;
	case DACTION_TRANSPORTWAITTOFLYIN:
        //if we're moving droids to safety and currently waiting to fly back in, see if time is up
        if (psDroid->player == selectedPlayer AND getDroidsToSafetyFlag())
        {
			if ((SDWORD)(mission.ETA - (gameTime - missionGetReinforcementTime())) <= 0)
			{
                if (!droidRemove(psDroid, mission.apsDroidLists))
                {
                    ASSERT( FALSE, "actionUpdate: Unable to remove transporter from mission list" );
                }
                addDroid(psDroid, apsDroidLists);
                //set the x/y up since they were set to INVALID_XY when moved offWorld
                missionGetTransporterExit(selectedPlayer, (UWORD*)&droidX, (UWORD*)&droidY);
                psDroid->x = (UWORD)droidX;
                psDroid->y = (UWORD)droidY;
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
			//if vtol and offworld and empty - 'magic' it back home!
/*			alternatively - lets not - John.
			if (vtolEmpty(psDroid) AND missionIsOffworld())
			{
				//check has reached LZ
				xdiff = (SDWORD)psDroid->x - (SDWORD)psDroid->actionX;
				ydiff = (SDWORD)psDroid->y - (SDWORD)psDroid->actionY;
				if (xdiff*xdiff + ydiff*ydiff <= TILE_UNITS*TILE_UNITS)
				{
					//magic! - take droid out of list and add to one back at home base
					if (droidRemove(psDroid, apsDroidLists))
                    {
                        //only add to mission lists if successfully removed from current droid lists
					    addDroid(psDroid, mission.apsDroidLists);
    					//make sure fully armed etc by time back at home
	    				mendVtol(psDroid);
		    			addConsoleMessage("VTOL MAGIC!",DEFAULT_JUSTIFY);
                    }
                    //else the droid should be dead!
				}
				else
				{
					//re-order the droid back to the LZ
					orderDroidLoc(psDroid, DORDER_MOVE, getLandingX(psDroid->player),
						getLandingY(psDroid->player));
				}
			}*/
		}

/*		else if(secondaryGetState(psDroid, DSO_HALTTYPE, &state) && (state == DSS_HALT_HOLD))
		{
			psDroid->action = DACTION_NONE;		// hold is set, stop moving.
			moveStopDroid(psDroid);
		}*/

/*		else if ((psDroid->order == DORDER_SCOUT) &&
				 aiNearestTarget(psDroid, &psObj))
		{
			if (psDroid->numWeaps > 0)
			{
				orderDroidObj(psDroid, DORDER_ATTACK, psObj);
			}
			else
			{
				orderDroid(psDroid, DORDER_STOP);
			}
		}*/

		//else if (psDroid->numWeaps > 0 &&
        else if (!vtolDroid(psDroid) &&
				 psDroid->asWeaps[0].nStat > 0 &&
				 psWeapStats->rotate &&
				 psWeapStats->fireOnMove != FOM_NO &&
				 aiNearestTarget(psDroid, &psDroid->psActionTarget))
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
		break;
	case DACTION_RETURNTOPOS:
	case DACTION_FIRESUPPORT_RETREAT:
		if (DROID_STOPPED(psDroid))
		{
			// Got to destination
			psDroid->action = DACTION_NONE;
		}
/*		else if(secondaryGetState(psDroid, DSO_HALTTYPE, &state) && (state == DSS_HALT_HOLD))
		{
			psDroid->action = DACTION_NONE;		// hold is set, stop moving.
			moveStopDroid(psDroid);
		}*/
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
		// firing on something while moving
		if (DROID_STOPPED(psDroid))
		{
			// Got to desitination
			psDroid->action = DACTION_NONE;
		}
		else if ((psDroid->psActionTarget == NULL) ||
				 !validTarget((BASE_OBJECT *)psDroid, psDroid->psActionTarget) ||
				 (secondaryGetState(psDroid, DSO_ATTACK_LEVEL, &state) && (state != DSS_ALEV_ALWAYS)))
		{
			// Target lost
			psDroid->action = DACTION_MOVE;
            //if Vtol - return to rearm pad
            /*if (vtolDroid(psDroid))
            {
			    moveToRearm(psDroid);
            }*/
		}
        //check the target hasn't become one the same player ID - Electronic Warfare
        else if	(electronicDroid(psDroid) &&
				 (psDroid->player == psDroid->psActionTarget->player))
        {
            psDroid->psActionTarget = NULL;
            psDroid->action = DACTION_NONE;
        }
		else
		{
			if (visibleObject((BASE_OBJECT*)psDroid, psDroid->psActionTarget))
			{
				/*if (actionTargetTurret((BASE_OBJECT*)psDroid, psDroid->psActionTarget,
										&(psDroid->turretRotation), &(psDroid->turretPitch),
										psDroid->turretRotRate, (SWORD)(psDroid->turretRotRate/2),
										//asWeaponStats[psDroid->asWeaps->nStat].direct))
										proj_Direct(&asWeaponStats[psDroid->asWeaps->nStat]),
										bInvert))*/
				if (actionTargetTurret((BASE_OBJECT*)psDroid, psDroid->psActionTarget,
										&(psDroid->turretRotation), &(psDroid->turretPitch),
										&asWeaponStats[psDroid->asWeaps->nStat],
										bInvert))
				{
					// In range - fire !!!
					combFire(psDroid->asWeaps, (BASE_OBJECT *)psDroid,
							 psDroid->psActionTarget);
				}
			}
			else
			{
				// lost the target
				psDroid->action = DACTION_MOVE;
				psDroid->psActionTarget = NULL;
			}
		}

        //check its a VTOL unit since adding Transporter's into multiPlayer
		/* check vtol attack runs */
		//if ( psPropStats->propulsionType == LIFT )
        if (vtolDroid(psDroid))
		{
			actionUpdateVtolAttack( psDroid );
		}

		break;

	case DACTION_ATTACK:
		ASSERT( psDroid->psActionTarget != NULL,
			"actionUpdateUnit: target is NULL while attacking" );

		// don't wan't formations for this one
		if (psDroid->sMove.psFormation)
		{
			formationLeave(psDroid->sMove.psFormation, (BASE_OBJECT *)psDroid);
			psDroid->sMove.psFormation = NULL;
		}

        //check the target hasn't become one the same player ID - Electronic Warfare
        if ((electronicDroid(psDroid) && (psDroid->player == psDroid->psActionTarget->player)) ||
			!validTarget((BASE_OBJECT *)psDroid, psDroid->psActionTarget))// ||
//			(secondaryGetState(psDroid, DSO_ATTACK_LEVEL, &state) && (state != DSS_ALEV_ALWAYS)))
        {
            psDroid->psActionTarget = NULL;
            psDroid->action = DACTION_NONE;
        }
        else if (actionVisibleTarget(psDroid, psDroid->psActionTarget) &&
			actionInAttackRange(psDroid, psDroid->psActionTarget))
		{
			if (!psWeapStats->rotate)
			{
				// no rotating turret - need to check aligned with target
				targetDir = (SDWORD)calcDirection(
								psDroid->x,psDroid->y,
								psDroid->psActionTarget->x,psDroid->psActionTarget->y);
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
					moveTurnDroid(psDroid, psDroid->psActionTarget->x,psDroid->psActionTarget->y);
				}
			}
			else if (!psWeapStats->rotate ||
					/*actionTargetTurret((BASE_OBJECT*)psDroid, psDroid->psActionTarget,
										&(psDroid->turretRotation), &(psDroid->turretPitch),
										psDroid->turretRotRate, (SWORD)(psDroid->turretRotRate/2),
										//asWeaponStats[psDroid->asWeaps->nStat].direct))
										proj_Direct(&asWeaponStats[psDroid->asWeaps->nStat]),
										bInvert))*/
					actionTargetTurret((BASE_OBJECT*)psDroid, psDroid->psActionTarget,
										&(psDroid->turretRotation), &(psDroid->turretPitch),
										&asWeaponStats[psDroid->asWeaps->nStat],
										bInvert))
			{
				/* In range - fire !!! */
				combFire(psDroid->asWeaps, (BASE_OBJECT *)psDroid, psDroid->psActionTarget);
			}
		}
		else
		{
			if ( ( (psDroid->order == DORDER_ATTACKTARGET || psDroid->order == DORDER_FIRESUPPORT) &&
				   secondaryGetState(psDroid, DSO_HALTTYPE, &state) && (state == DSS_HALT_HOLD) ) ||
				 ( !vtolDroid(psDroid) &&
				   orderStateObj(psDroid, DORDER_FIRESUPPORT, &psTarget) && (psTarget->type == OBJ_STRUCTURE) ) )
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
		//check if vtol that its armed
		if ( (vtolEmpty(psDroid)) ||
			 (psDroid->psActionTarget == NULL) ||
	        //check the target hasn't become one the same player ID - Electronic Warfare
			 (electronicDroid(psDroid) && (psDroid->player == psDroid->psActionTarget->player)) ||
			 !validTarget((BASE_OBJECT *)psDroid, psDroid->psActionTarget) )// ||
//			 (secondaryGetState(psDroid, DSO_ATTACK_LEVEL, &state) && (state != DSS_ALEV_ALWAYS)))
		{
			moveToRearm(psDroid);
			break;
		}

        if (actionVisibleTarget(psDroid, psDroid->psActionTarget))
		{
			if (actionInRange(psDroid, psDroid->psActionTarget))
			{
				if ( psDroid->player == selectedPlayer )
				{
					audio_QueueTrackMinDelay( ID_SOUND_COMMENCING_ATTACK_RUN2,
												VTOL_ATTACK_AUDIO_DELAY );
				}

				if (actionTargetTurret((BASE_OBJECT*)psDroid, psDroid->psActionTarget,
										&(psDroid->turretRotation), &(psDroid->turretPitch),
										&asWeaponStats[psDroid->asWeaps->nStat],
										bInvert))
				{
					// In range - fire !!!
					combFire(psDroid->asWeaps, (BASE_OBJECT *)psDroid,
							 psDroid->psActionTarget);
				}
			}
			else
			{
				actionTargetTurret((BASE_OBJECT*)psDroid, psDroid->psActionTarget,
										&(psDroid->turretRotation), &(psDroid->turretPitch),
										&asWeaponStats[psDroid->asWeaps->nStat],
										bInvert);
			}
		}
/*		else
		{
			// lost the target
			psDroid->action = DACTION_MOVETOATTACK;
			moveDroidTo(psDroid, psDroid->psActionTarget->x, psDroid->psActionTarget->y);
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
			xdiff = (SDWORD)psDroid->x - (SDWORD)psDroid->psActionTarget->x;
			ydiff = (SDWORD)psDroid->y - (SDWORD)psDroid->psActionTarget->y;
			rangeSq = xdiff*xdiff + ydiff*ydiff;
			if (rangeSq < (VTOL_ATTACK_TARDIST*VTOL_ATTACK_TARDIST))
			{
				// don't do another attack run if already moving away from the target
				xdiff = (SDWORD)psDroid->sMove.DestinationX - (SDWORD)psDroid->psActionTarget->x;
				ydiff = (SDWORD)psDroid->sMove.DestinationY - (SDWORD)psDroid->psActionTarget->y;
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
				xdiff = (SDWORD)psDroid->sMove.DestinationX - (SDWORD)psDroid->psActionTarget->x;
				ydiff = (SDWORD)psDroid->sMove.DestinationY - (SDWORD)psDroid->psActionTarget->y;
				if ((xdiff*xdiff + ydiff*ydiff) > (VTOL_ATTACK_TARDIST*VTOL_ATTACK_TARDIST))
				{
					moveDroidToDirect(psDroid, psDroid->psActionTarget->x,psDroid->psActionTarget->y);
				}
			}
		}
		break;

	case DACTION_MOVETOATTACK:
		DBP1(("MOVETOATTACK - %x\n",psDroid));

		// don't wan't formations for this one
		if (psDroid->sMove.psFormation)
		{
			formationLeave(psDroid->sMove.psFormation, (BASE_OBJECT *)psDroid);
			psDroid->sMove.psFormation = NULL;
		}

		// send vtols back to rearm
		if (vtolDroid(psDroid) &&
			vtolEmpty(psDroid))
		{
			moveToRearm(psDroid);
			break;
		}

        //check the target hasn't become one the same player ID - Electronic Warfare
        if ((electronicDroid(psDroid) && (psDroid->player == psDroid->psActionTarget->player)) ||
			!validTarget((BASE_OBJECT *)psDroid, psDroid->psActionTarget) )// ||
//			(secondaryGetState(psDroid, DSO_ATTACK_LEVEL, &state) && (state != DSS_ALEV_ALWAYS)))
        {
            psDroid->psActionTarget = NULL;
            psDroid->action = DACTION_NONE;
        }
        else
        {
            if (actionVisibleTarget(psDroid, psDroid->psActionTarget))
		    {
			    if (psWeapStats->rotate)
			    {
				    /*actionTargetTurret((BASE_OBJECT*)psDroid, psDroid->psActionTarget,
										    &(psDroid->turretRotation), &(psDroid->turretPitch),
										    psDroid->turretRotRate, (SWORD)(psDroid->turretRotRate/2),
										    //asWeaponStats[psDroid->asWeaps->nStat].direct);
										    proj_Direct(&asWeaponStats[psDroid->asWeaps->nStat]),
										    bInvert);*/
				    actionTargetTurret((BASE_OBJECT*)psDroid, psDroid->psActionTarget,
										    &(psDroid->turretRotation), &(psDroid->turretPitch),
										    &asWeaponStats[psDroid->asWeaps->nStat],
										    bInvert);
			    }

    			bChaseBloke = FALSE;
	    		if (!vtolDroid(psDroid) &&
					psDroid->psActionTarget->type == OBJ_DROID &&
		    		((DROID *)psDroid->psActionTarget)->droidType == DROID_PERSON &&
			    	psWeapStats->fireOnMove != FOM_NO)
			    {
				    bChaseBloke = TRUE;
			    }


    			if (actionInAttackRange(psDroid, psDroid->psActionTarget) &&
	    			!bChaseBloke)
		    	{
			    	/* Stop the droid moving any closer */
//				    ASSERT( psDroid->x != 0 && psDroid->y != 0,
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
    						moveTurnDroid(psDroid, psDroid->psActionTarget->x,psDroid->psActionTarget->y);
	    				}
		    		}
			    }
			    else if (actionInRange(psDroid, psDroid->psActionTarget))
			    {
				    // fire while closing range
				    combFire(psDroid->asWeaps, (BASE_OBJECT *)psDroid, psDroid->psActionTarget);
			    }
		    }
		    else
		    {
    			if ((psDroid->turretRotation!=0) ||
					(psDroid->turretPitch!=0))
				{
	    			actionAlignTurret((BASE_OBJECT *)psDroid);
				}
		    }
		    if (DROID_STOPPED(psDroid) && psDroid->action != DACTION_ATTACK)
    		{
	    		/* Stopped moving but haven't reached the target - possibly move again */

	    		if ((psDroid->order == DORDER_ATTACKTARGET) && secondaryGetState(psDroid, DSO_HALTTYPE, &state) && (state == DSS_HALT_HOLD))
		    	{
			    	psDroid->action = DACTION_NONE;			// on hold, give up.
			    }
			    else if (actionInsideMinRange(psDroid, psDroid->psActionTarget))
				{
					if ( proj_Direct( psWeapStats ) )
					{
						// try and extend the range
						actionCalcPullBackPoint((BASE_OBJECT *)psDroid, psDroid->psActionTarget, &pbx,&pby);
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
    						moveTurnDroid( psDroid, psDroid->psActionTarget->x,
											psDroid->psActionTarget->y);
						}
					}
				}
				else
			    {
					// try to close the range
				    moveDroidTo(psDroid, psDroid->psActionTarget->x, psDroid->psActionTarget->y);
			    }
		    }
        }
		break;

	case DACTION_SULK:
		// unable to route to target ... don't do anything aggressive until time is up
		// we need to do something defensive at this point ???
		//if (gameTime>psDroid->actionHeight)				// actionHeight is used here for the ending time for this action

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
			formationLeave(psDroid->sMove.psFormation, (BASE_OBJECT *)psDroid);
			psDroid->sMove.psFormation = NULL;
		}

		// moving to a location to build a structure
		if (actionReachedBuildPos(psDroid,
						(SDWORD)psDroid->orderX,(SDWORD)psDroid->orderY, psDroid->psTarStats) &&
			!actionDroidOnBuildPos(psDroid,
						(SDWORD)psDroid->orderX,(SDWORD)psDroid->orderY, psDroid->psTarStats))
		{
			moveStopDroid(psDroid);
/*			xdiff = (SDWORD)psDroid->x - (SDWORD)psDroid->actionX;
			ydiff = (SDWORD)psDroid->y - (SDWORD)psDroid->actionY;
			if ( xdiff*xdiff + ydiff*ydiff >= TILE_UNITS*TILE_UNITS )
			{
				DBP2(("DACTION_MOVETOBUILD: new location\n"));
				// Couldn't reach destination - try and find a new one
				if (actionNewBuildPos(psDroid, &droidX,&droidY))
				{
					psDroid->actionX = droidX;
					psDroid->actionY = droidY;
					moveDroidTo(psDroid, droidX,droidY);
				}
				else
				{
					DBP2(("DACTION_MOVETOBUILD: giving up\n"));
					psDroid->action = DACTION_NONE;
				}
			}
			else*/
			{
				bDoHelpBuild = FALSE;

				// Got to destination - start building
				psStructStats = (STRUCTURE_STATS*)psDroid->psTarStats;
				if (psDroid->order == DORDER_BUILD &&
					(psDroid->psTarget == NULL))
//					&&  (
//					psStructStats->type != REF_WALL ||
//					psStructStats->type != REF_WALLCORNER))
					//psDroid->order == DORDER_LINEBUILD)
				{
					// Starting a new structure
					// calculate the top left of the structure
					tlx = (SDWORD)psDroid->orderX - (SDWORD)(psStructStats->baseWidth * TILE_UNITS)/2;
					tly = (SDWORD)psDroid->orderY - (SDWORD)(psStructStats->baseBreadth * TILE_UNITS)/2;
//					tlx = tlx >> TILE_UNITS;
//					tly = tly >> TILE_UNITS;

					//need to check if something has already started building here?
					//unless its a module!
					if (IsStatExpansionModule(psStructStats))
					{
						DBP2(("DACTION_MOVETOBUILD: setUpBuildModule\n"));
						setUpBuildModule(psDroid);
					}
					else if ( TILE_HAS_STRUCTURE(mapTile(psDroid->orderX >> TILE_SHIFT, psDroid->orderY >> TILE_SHIFT)) )
					{
						// structure on the build location - see if it is the same type
						psStruct = getTileStructure(psDroid->orderX >> TILE_SHIFT, psDroid->orderY >> TILE_SHIFT);
						if (psStruct->pStructureType == (STRUCTURE_STATS *)psDroid->psTarStats)
						{
							// same type - do a help build
							psDroid->psTarget = (BASE_OBJECT *)psStruct;
							bDoHelpBuild = TRUE;
						}
						else if ((psStruct->pStructureType->type == REF_WALL ||
                            psStruct->pStructureType->type == REF_WALLCORNER) &&
							((STRUCTURE_STATS *)psDroid->psTarStats)->type == REF_DEFENSE)
						{
							// building a gun tower over a wall - OK
							if (droidStartBuild(psDroid))
							{
								DBP2(("DACTION_MOVETOBUILD: start foundation\n"));
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
//						psDroid->orderX >> TILE_SHIFT, psDroid->orderY >> TILE_SHIFT))
						tlx >> TILE_SHIFT, tly >> TILE_SHIFT, psDroid->player, FALSE))
					{
						DBP2(("DACTION_MOVETOBUILD: !validLocation\n"));
						psDroid->action = DACTION_NONE;
					}
					else if (droidStartFoundation(psDroid))
					{
						DBP2(("DACTION_MOVETOBUILD: start foundation\n"));
						psDroid->action = DACTION_BUILD_FOUNDATION;
					}
					else
					{
						DBP2(("DACTION_MOVETOBUILD: giving up (2)\n"));
						psDroid->action = DACTION_NONE;
					}
				}
				else if(  (psDroid->order == DORDER_LINEBUILD||psDroid->order==DORDER_BUILD)
						&&(psStructStats->type == REF_WALL   || psStructStats->type == REF_WALLCORNER ||
						   psStructStats->type == REF_DEFENSE))
				{
					// building a wall.
					psTile = mapTile(psDroid->orderX>>TILE_SHIFT, psDroid->orderY>>TILE_SHIFT);
					if ((psDroid->psTarget == NULL) &&
					    (TILE_HAS_STRUCTURE(psTile) ||
					     TILE_HAS_FEATURE(psTile)))
					{
						if (TILE_HAS_STRUCTURE(psTile))
						{
							// structure on the build location - see if it is the same type
							psStruct = getTileStructure(psDroid->orderX >> TILE_SHIFT, psDroid->orderY >> TILE_SHIFT);
							if (psStruct->pStructureType == (STRUCTURE_STATS *)psDroid->psTarStats)
							{
								// same type - do a help build
								psDroid->psTarget = (BASE_OBJECT *)psStruct;
								bDoHelpBuild = TRUE;
							}
							else if ((psStruct->pStructureType->type == REF_WALL || psStruct->pStructureType->type == REF_WALLCORNER) &&
									 ((STRUCTURE_STATS *)psDroid->psTarStats)->type == REF_DEFENSE)
							{
								// building a gun tower over a wall - OK
								if (droidStartBuild(psDroid))
								{
									DBP2(("DACTION_MOVETOBUILD: start foundation\n"));
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
					bDoHelpBuild = TRUE;
				}

				if (bDoHelpBuild)
				{
					// continuing a partially built structure (order = helpBuild)
					if (droidStartBuild(psDroid))
					{
						DBP2(("DACTION_MOVETOBUILD: starting help build\n"));
						psDroid->action = DACTION_BUILD;
						intBuildStarted(psDroid);
					}
					else
					{
						DBP2(("DACTION_MOVETOBUILD: giving up (3)\n"));
						psDroid->action = DACTION_NONE;
					}
				}
			}
		}
		else if (DROID_STOPPED(psDroid))
		{
			if (actionDroidOnBuildPos(psDroid,
						(SDWORD)psDroid->orderX,(SDWORD)psDroid->orderY, psDroid->psTarStats))
			{
				actionHomeBasePos(psDroid->player, &pbx,&pby);
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
			formationLeave(psDroid->sMove.psFormation, (BASE_OBJECT *)psDroid);
			psDroid->sMove.psFormation = NULL;
		}

		if (DROID_STOPPED(psDroid) &&
			!actionReachedBuildPos(psDroid,
						(SDWORD)psDroid->orderX,(SDWORD)psDroid->orderY, psDroid->psTarStats))
		{
//			psDroid->action = DACTION_MOVETOBUILD;
			moveDroidToNoFormation(psDroid, psDroid->actionX,psDroid->actionY);
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
		   		moveTurnDroid(psDroid,psDroid->psTarget->x,psDroid->psTarget->y);
			}*/
			actionTargetTurret((BASE_OBJECT*)psDroid, psDroid->psActionTarget,
									&(psDroid->turretRotation), &(psDroid->turretPitch),
									NULL,FALSE);
		}
		else
		{
			DBP2(("DACTION_BUILD: done\n"));
			psDroid->action = DACTION_NONE;
		}
		break;
	case DACTION_MOVETODEMOLISH:
	case DACTION_MOVETOREPAIR:
	case DACTION_MOVETOCLEAR:
	case DACTION_MOVETORESTORE:
    {
		// The droid cannot be in a formation
		if (psDroid->sMove.psFormation != NULL)
		{
			formationLeave(psDroid->sMove.psFormation, (BASE_OBJECT *)psDroid);
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
				moveDroidToNoFormation(psDroid, (UDWORD)pbx,(UDWORD)pby);
			}
			else
			{
				moveDroidToNoFormation(psDroid, psDroid->actionX,psDroid->actionY);
			}
		}
		break;
    }
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
			actionTargetTurret((BASE_OBJECT*)psDroid, psDroid->psActionTarget,
									&(psDroid->turretRotation), &(psDroid->turretPitch),
									NULL,FALSE);
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
			formationLeave(psDroid->sMove.psFormation, (BASE_OBJECT *)psDroid);
			psDroid->sMove.psFormation = NULL;
		}
		/* moving from front to rear of repair facility or rearm pad */
/*		xdiff = (SDWORD)psDroid->x - ((SDWORD)psDroid->psActionTarget->x + TILE_UNITS);
		ydiff = (SDWORD)psDroid->y - (SDWORD)psDroid->psActionTarget->y;
		if (xdiff*xdiff + ydiff*ydiff < (TILE_UNITS/2)*(TILE_UNITS/2))*/
		if (actionReachedBuildPos(psDroid, psDroid->psActionTarget->x,psDroid->psActionTarget->y,
							(BASE_STATS *)((STRUCTURE *)psDroid->psActionTarget)->pStructureType))
		{
			moveStopDroid(psDroid);
			psDroid->action = DACTION_WAITDURINGREPAIR;
		}
		else if (DROID_STOPPED(psDroid))
		{
			moveDroidToNoFormation(psDroid, psDroid->psActionTarget->x,
						psDroid->psActionTarget->y);
		}
		break;
	case DACTION_BUILD_FOUNDATION:
		//building a structure's foundation - flattening the ground for now
		if (droidUpdateFoundation(psDroid))
		{
			//the droid moves around the site in UpdateFoundation now

			//randomly move the droid around the construction site
			/*if ((rand() % 50) < 1)
			{
				DBP2(("DACTION_BUILD_FOUNDATION: start wander\n"));
				psStructStats = (STRUCTURE_STATS *)psDroid->psTarStats;
				if (getDroidDestination(psStructStats, psDroid->orderX, psDroid->orderY,
					&droidX, &droidY))
				{
					psDroid->action = DACTION_FOUNDATION_WANDER;
					psDroid->actionX = droidX;
					psDroid->actionY = droidY;
					moveDroidTo(psDroid, droidX,droidY);
				}
			}*/
		}
		else
		{
			psTile = mapTile(psDroid->orderX>>TILE_SHIFT, psDroid->orderY>>TILE_SHIFT);
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
					psStruct = getTileStructure(psDroid->orderX >> TILE_SHIFT, psDroid->orderY >> TILE_SHIFT);
					if (psStruct->pStructureType == (STRUCTURE_STATS *)psDroid->psTarStats)
					{
						// same type - do a help build
						psDroid->psTarget = (BASE_OBJECT *)psStruct;
					}
					else
					{
						psDroid->action = DACTION_NONE;
					}
				}
				else if (!validLocation((BASE_STATS*)psDroid->psTarStats,
					tlx >> TILE_SHIFT, tly >> TILE_SHIFT, psDroid->player, FALSE))
				{
					psDroid->action = DACTION_NONE;
				}
			}

			//ready to start building the structure
			if ( psDroid->action != DACTION_NONE &&
				 droidStartBuild(psDroid))
			{
				DBP2(("DACTION_BUILD_FOUNDATION: start build\n"));
				psDroid->action = DACTION_BUILD;


				//add one to current quantity for this player - done in droidStartBuild()
				//asStructLimits[psDroid->player][(STRUCTURE_STATS *)psDroid->
					//psTarStats - asStructureStats].currentQuantity++;
			}
			else
			{
				DBP2(("DACTION_BUILD_FOUNDATION: giving up\n"));
				psDroid->action = DACTION_NONE;
			}
		}
		break;
	case DACTION_OBSERVE:
		// align the turret
		actionTargetTurret((BASE_OBJECT*)psDroid, psDroid->psActionTarget,
						&(psDroid->turretRotation), &(psDroid->turretPitch),
						NULL,FALSE);

		if (cbSensorDroid(psDroid))
		{
			// don't move to the target, just make sure it is visible
			// Anyone commenting this out will get a knee capping from John.
			// You have been warned!!
			psDroid->psActionTarget->visible[psDroid->player] = UBYTE_MAX;
		}
		else
		{
			// make sure the target is within sensor range
			xdiff = (SDWORD)psDroid->x - (SDWORD)psDroid->psActionTarget->x;
			ydiff = (SDWORD)psDroid->y - (SDWORD)psDroid->psActionTarget->y;
			//if change this back - change in MOVETOOBSERVE as well
			//rangeSq = 2 * (SDWORD)psDroid->sensorRange / 3;
			rangeSq = (SDWORD)psDroid->sensorRange;
			rangeSq = rangeSq * rangeSq;
			if (!visibleObject((BASE_OBJECT *)psDroid, psDroid->psActionTarget) ||
				xdiff*xdiff + ydiff*ydiff >= rangeSq)
			{
/*				if (secondaryGetState(psDroid, DSO_HALTTYPE, &state) && (state == DSS_HALT_HOLD))
				{
					psDroid->action = DACTION_NONE;						// holding, don't move.
				}
				else*/
				{
					psDroid->action = DACTION_MOVETOOBSERVE;
					moveDroidTo(psDroid, psDroid->psActionTarget->x,psDroid->psActionTarget->y);
				}
			}
		}
		break;
	case DACTION_MOVETOOBSERVE:
		// align the turret
		actionTargetTurret((BASE_OBJECT*)psDroid, psDroid->psActionTarget,
						&(psDroid->turretRotation), &(psDroid->turretPitch),
						NULL,FALSE);

		if (visibleObject((BASE_OBJECT *)psDroid, psDroid->psActionTarget))
		{
			// make sure the target is within sensor range
			xdiff = (SDWORD)psDroid->x - (SDWORD)psDroid->psActionTarget->x;
			ydiff = (SDWORD)psDroid->y - (SDWORD)psDroid->psActionTarget->y;
            //if change this back - change in OBSERVE as well
			//rangeSq = 2 * (SDWORD)psDroid->sensorRange / 3;
            rangeSq = (SDWORD)psDroid->sensorRange;
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
/*			if (secondaryGetState(psDroid, DSO_HALTTYPE, &state) && (state == DSS_HALT_HOLD))
			{
				psDroid->action = DACTION_NONE;				// on hold, don't go any further.
			}
			else*/
			{
				moveDroidTo(psDroid, psDroid->psActionTarget->x,psDroid->psActionTarget->y);
			}
		}
		break;
	case DACTION_FIRESUPPORT:
		//can be either a droid or a structure now - AB 7/10/98
		ASSERT( (psDroid->psTarget->type == OBJ_DROID OR
			psDroid->psTarget->type == OBJ_STRUCTURE) &&
				(psDroid->psTarget->player == psDroid->player),
			"DACTION_FIRESUPPORT: incorrect target type" );
/*		if (orderState(((DROID *)psDroid->psTarget), DORDER_OBSERVE))
		{
			// move to attack
			psDroid->action = DACTION_MOVETOFSUPP_ATTACK;
			psDroid->psActionTarget = psDroid->psTarget->psTarget;
			moveDroidTo(psDroid, psDroid->psActionTarget->x, psDroid->psActionTarget->y);
		}
		else
		{*/
		//Move droids attached to structures and droids now...AB 13/10/98
		//move (indirect weapon)droids attached to a sensor
		//if (psDroid->psTarget->type == OBJ_DROID)
		{
            //don't move VTOL's
			// also don't move closer to sensor towers
            if (!vtolDroid(psDroid) &&
				(psDroid->psTarget->type != OBJ_STRUCTURE))
            {
			    //move droids to within short range of the sensor now!!!!
			    xdiff = (SDWORD)psDroid->x - (SDWORD)psDroid->psTarget->x;
			    ydiff = (SDWORD)psDroid->y - (SDWORD)psDroid->psTarget->y;
			    // make sure the weapon droid is within 2/3 weapon range of the sensor
			    //rangeSq = 2 * proj_GetLongRange(asWeaponStats + psDroid->asWeaps[0].nStat, 0) / 3;
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
						xdiff = (SDWORD)psDroid->psTarget->x - (SDWORD)psDroid->sMove.DestinationX;
						ydiff = (SDWORD)psDroid->psTarget->y - (SDWORD)psDroid->sMove.DestinationY;
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
							moveDroidTo(psDroid, psDroid->psTarget->x,psDroid->psTarget->y);
						}
					}
			    }
            }
		}
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
				destroyDroid(psDroid);
			}
		}
		break;
	case DACTION_MOVETODROIDREPAIR:
		// moving to repair a droid
		xdiff = (SDWORD)psDroid->x - (SDWORD)psDroid->psActionTarget->x;
		ydiff = (SDWORD)psDroid->y - (SDWORD)psDroid->psActionTarget->y;
		if ( xdiff*xdiff + ydiff*ydiff < REPAIR_RANGE )
		{
			// Got to destination - start repair
			//rotate turret to point at droid being repaired
			/*if (actionTargetTurret((BASE_OBJECT*)psDroid,
					psDroid->psActionTarget, &(psDroid->turretRotation),
					&(psDroid->turretPitch), psDroid->turretRotRate,
					(SWORD)(psDroid->turretRotRate/2), FALSE, FALSE))*/
			if (actionTargetTurret((BASE_OBJECT*)psDroid,
					psDroid->psActionTarget, &(psDroid->turretRotation),
					&(psDroid->turretPitch), NULL, FALSE))
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
			/*droidX = psDroid->psTarget->x >> TILE_SHIFT;
			droidY = psDroid->psTarget->y >> TILE_SHIFT;
			if (!pickATile(&droidX, &droidY, LOOK_NEXT_TO_DROID))
			{
				//couldn't get next to the droid, so stop trying
				psDroid->action = DACTION_NONE;
			}
			psDroid->actionX = droidX << TILE_SHIFT;
			psDroid->actionY = droidY << TILE_SHIFT;*/
			psDroid->actionX = psDroid->psActionTarget->x;
			psDroid->actionY = psDroid->psActionTarget->y;
			moveDroidTo(psDroid, psDroid->actionX, psDroid->actionY);
		}
		break;
	case DACTION_DROIDREPAIR:
		/*actionTargetTurret((BASE_OBJECT*)psDroid,
					psDroid->psActionTarget, &(psDroid->turretRotation),
					&(psDroid->turretPitch), psDroid->turretRotRate,
					(SWORD)(psDroid->turretRotRate/2), FALSE, FALSE);*/

        //if not doing self-repair
        if (psDroid->psActionTarget != (BASE_OBJECT *)psDroid)
        {
		    actionTargetTurret((BASE_OBJECT*)psDroid,
					psDroid->psActionTarget, &(psDroid->turretRotation),
					&(psDroid->turretPitch), NULL, FALSE);
        }

		//check still next to the damaged droid
		xdiff = (SDWORD)psDroid->x - (SDWORD)psDroid->psActionTarget->x;
		ydiff = (SDWORD)psDroid->y - (SDWORD)psDroid->psActionTarget->y;
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
			psDroid->actionX = psDroid->psActionTarget->x;
			psDroid->actionY = psDroid->psActionTarget->y;
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
				if (((DROID *)psDroid->psActionTarget)->sMove.Status == MOVESHUFFLE)
				{
					moveStopDroid((DROID *)psDroid->psActionTarget);
				}
			}
		}
		break;
	case DACTION_WAITFORREARM:
		// wait here for the rearm pad to ask the vtol to move
		if (psDroid->psActionTarget == NULL)
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
		if (psDroid->psActionTarget == NULL)
		{
			// base destroyed - find another
			moveToRearm(psDroid);
			break;
		}

		if (visibleObject((BASE_OBJECT *)psDroid, psDroid->psActionTarget))
		{
			// got close to the rearm pad - now find a clear one
			DBP4(("Unit %d: seen rearm pad\n", psDroid->id));
			psStruct = findNearestReArmPad(psDroid, (STRUCTURE *)psDroid->psActionTarget, TRUE);
			if (psStruct != NULL)
			{
				// found a clear landing pad - go for it
				DBP4(("Found clear rearm pad\n"));
				psDroid->psActionTarget = (BASE_OBJECT *)psStruct;
			}

			psDroid->action = DACTION_WAITFORREARM;
		}

		if (DROID_STOPPED(psDroid) ||
			(psDroid->action == DACTION_WAITFORREARM))
		{
			droidX = psDroid->psActionTarget->x;
			droidY = psDroid->psActionTarget->y;
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
		ASSERT( FALSE, "actionUpdateUnit: unknown action" );
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

		if ((psDroid->turretRotation!=0) ||
			(psDroid->turretPitch !=0))
		{
			actionAlignTurret((BASE_OBJECT *)psDroid);
		}
	}
}


/* Overall action function that is called by the specific action functions */
static void actionDroidBase(DROID *psDroid, DROID_ACTION_DATA *psAction)
{
	SECONDARY_STATE			state;
	SDWORD			pbx,pby;
	WEAPON_STATS	*psWeapStats;
	UDWORD			droidX,droidY;
	BASE_OBJECT		*psTarget;

	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
		"actionUnitBase: Invalid Unit pointer" );
	ASSERT( psDroid->type == OBJ_DROID,
		"actionUnitBase: Unit pointer does not reference a unit" );

	switch (psAction->action)
	{
	case DACTION_NONE:
		// Clear up what ever the droid was doing before if necessary
//		if(!driveModeActive() || !psDroid->selected) {
			if (!DROID_STOPPED(psDroid))
			{
				moveStopDroid(psDroid);
			}
			psDroid->action = DACTION_NONE;
			psDroid->actionX = 0;
			psDroid->actionY = 0;
			psDroid->actionStarted = 0;
			psDroid->actionPoints = 0;
			//psDroid->actionHeight = 0;
            psDroid->powerAccrued = 0;
			psDroid->psActionTarget = NULL;
//		} //else {
//			if(psDroid->player == 0)
//			{
//				DBPRINTF(("DACTION_NONE %p\n",psDroid);
//			}
//		}
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
			if (psAction->psObj->type == OBJ_STRUCTURE AND (
//				(((STRUCTURE *)psAction->psObj)->pStructureType->resistance == 0)))
                /* OR (((STRUCTURE *)psAction->psObj)->resistance <
				(SDWORD)structureResistance(((STRUCTURE *)psAction->psObj)->
				pStructureType, psAction->psObj->player))))*/
				//psObj)->resistance < (SDWORD)((STRUCTURE *)psAction->
				//psObj)->pStructureType->resistance)))
                !validStructResistance((STRUCTURE *)psAction->psObj)))
			{
				//structure is low resistance already so don't attack
				psDroid->action = DACTION_NONE;
				break;
			}
            //in multiPlayer cannot electronically attack a tranporter
            if (bMultiPlayer AND psAction->psObj->type == OBJ_DROID AND
                ((DROID *)psAction->psObj)->droidType == DROID_TRANSPORTER)
            {
                psDroid->action = DACTION_NONE;
				break;
            }
		}




//		psDroid->actionX = psAction->psObj->x;
//		psDroid->actionY = psAction->psObj->y;
		// note the droid's current pos so that scout & patrol orders know how far the
		// droid has gone during an attack
		// slightly strange place to store this I know, but I didn't want to add any more to the droid
		psDroid->actionX = psDroid->x;
		psDroid->actionY = psDroid->y;

		psDroid->psActionTarget = psAction->psObj;
		if ( ( (psDroid->order == DORDER_ATTACKTARGET || psDroid->order == DORDER_FIRESUPPORT) &&
			   secondaryGetState(psDroid, DSO_HALTTYPE, &state) && (state == DSS_HALT_HOLD)) ||
			 ( !vtolDroid(psDroid) &&
			   orderStateObj(psDroid, DORDER_FIRESUPPORT, &psTarget) && (psTarget->type == OBJ_STRUCTURE) ) )
		{
			psDroid->action = DACTION_ATTACK;		// holding, try attack straightaway
			psDroid->psActionTarget = psAction->psObj;
		}
		else if (actionInsideMinRange(psDroid, psAction->psObj))
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
    				moveTurnDroid( psDroid, psDroid->psActionTarget->x,
											psDroid->psActionTarget->y);
				}
			}
			else
			{
				/* direct fire - try and extend the range */
				psDroid->action = DACTION_MOVETOATTACK;
				actionCalcPullBackPoint((BASE_OBJECT *)psDroid, psAction->psObj, &pbx,&pby);

				turnOffMultiMsg(TRUE);
				moveDroidTo(psDroid, (UDWORD)pbx, (UDWORD)pby);
				turnOffMultiMsg(FALSE);
			}
		}
		else
		{
			psDroid->action = DACTION_MOVETOATTACK;

			turnOffMultiMsg(TRUE);
			moveDroidTo(psDroid, psAction->psObj->x, psAction->psObj->y);
			turnOffMultiMsg(FALSE);

		}
		break;

	case DACTION_MOVETOREARM:
		psDroid->action = DACTION_MOVETOREARM;
		psDroid->actionX = psAction->psObj->x;
		psDroid->actionY = psAction->psObj->y;
		psDroid->actionStarted = gameTime;
		psDroid->psActionTarget = psAction->psObj;
		droidX = psDroid->psActionTarget->x;
		droidY = psDroid->psActionTarget->y;
		if (!actionVTOLLandingPos(psDroid, &droidX, &droidY))
		{
			// totally bunged up - give up
			orderDroid(psDroid, DORDER_RTB);
			break;
		}
		moveDroidToDirect(psDroid, droidX, droidY);
		break;
	case DACTION_CLEARREARMPAD:
		DBP4(("Unit %d clearing rearm pad\n", psDroid->id));
		psDroid->action = DACTION_CLEARREARMPAD;
		psDroid->psActionTarget = psAction->psObj;
		droidX = psDroid->psActionTarget->x;
		droidY = psDroid->psActionTarget->y;
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
		psDroid->psActionTarget = psAction->psObj;
		moveDroidTo(psDroid, psAction->x, psAction->y);
		break;

	case DACTION_BUILD:
		ASSERT( psDroid->order == DORDER_BUILD || psDroid->order == DORDER_HELPBUILD ||
				psDroid->order == DORDER_LINEBUILD,
			"actionUnitBase: cannot start build action without a build order" );
		psDroid->action = DACTION_MOVETOBUILD;
		psDroid->actionX = psAction->x;
		psDroid->actionY = psAction->y;
		if (actionDroidOnBuildPos(psDroid,
					(SDWORD)psDroid->orderX,(SDWORD)psDroid->orderY, psDroid->psTarStats))
		{
			actionHomeBasePos(psDroid->player, &pbx,&pby);
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
		psDroid->psActionTarget = psAction->psObj;
		moveDroidTo(psDroid, psAction->x, psAction->y);
		break;
	case DACTION_REPAIR:
		//ASSERT( psDroid->order == DORDER_REPAIR,
		//	"actionDroidBase: cannot start repair action without a repair order" );
		psDroid->action = DACTION_MOVETOREPAIR;
		psDroid->actionX = psAction->x;
		psDroid->actionY = psAction->y;
		//this needs setting so that automatic repair works
		psDroid->psActionTarget = psAction->psObj;
		ASSERT( (psDroid->psActionTarget != NULL) && (psDroid->psActionTarget->type == OBJ_STRUCTURE),
			"actionUnitBase: invalid target for demolish order" );
		psDroid->psTarStats = (BASE_STATS *)((STRUCTURE *)psDroid->psActionTarget)->pStructureType;
		moveDroidTo(psDroid, psAction->x, psAction->y);
		break;
	case DACTION_OBSERVE:
		psDroid->psActionTarget = psAction->psObj;
		psDroid->actionX = psDroid->x;
		psDroid->actionY = psDroid->y;
		if (//(secondaryGetState(psDroid, DSO_HALTTYPE, &state) && (state == DSS_HALT_HOLD)) ||
			cbSensorDroid(psDroid))
		{
			psDroid->action = DACTION_OBSERVE;
		}
		else
		{
			psDroid->action = DACTION_MOVETOOBSERVE;
			moveDroidTo(psDroid, psDroid->psActionTarget->x, psDroid->psActionTarget->y);
		}
		break;
	case DACTION_FIRESUPPORT:
		psDroid->action = DACTION_FIRESUPPORT;
		if(!vtolDroid(psDroid) &&
			!(secondaryGetState(psDroid, DSO_HALTTYPE, &state) && (state == DSS_HALT_HOLD)) &&	// check hold
			(psDroid->psTarget->type != OBJ_STRUCTURE))
		{
			moveDroidTo(psDroid, psDroid->psTarget->x, psDroid->psTarget->y);		// movetotarget.
		}

		break;
	case DACTION_SULK:
// 		debug( LOG_NEVER, "Go with sulk ... %p\n", psDroid );
		psDroid->action = DACTION_SULK;
        //hmmm, hope this doesn't cause any problems!
		//psDroid->actionStarted = gameTime;			// what is action started used for ? Certainly not used here!
		//psDroid->actionHeight = (UWORD)(gameTime+MIN_SULK_TIME+(rand()%(
		//	MAX_SULK_TIME-MIN_SULK_TIME)));	// actionHeight is used here for the ending time for this action
        psDroid->actionStarted = gameTime+MIN_SULK_TIME+(rand()%(
			MAX_SULK_TIME-MIN_SULK_TIME));
		break;
	case DACTION_DESTRUCT:
		psDroid->action = DACTION_DESTRUCT;
		psDroid->actionStarted = gameTime;
		break;
	case DACTION_WAITFORREPAIR:
		psDroid->action = DACTION_WAITFORREPAIR;
        //set the time so we can tell whether the start the self repair or not
        psDroid->actionStarted = gameTime;
		break;
	case DACTION_MOVETOREPAIRPOINT:
		psDroid->action = psAction->action;
		psDroid->actionX = psAction->x;
		psDroid->actionY = psAction->y;
		psDroid->actionStarted = gameTime;
		psDroid->psActionTarget = psAction->psObj;
		moveDroidToNoFormation( psDroid, psAction->x, psAction->y );
		break;
	case DACTION_WAITDURINGREPAIR:
		psDroid->action = DACTION_WAITDURINGREPAIR;
		break;
	case DACTION_MOVETOREARMPOINT:
		DBP4(("Unit %d moving to rearm point\n", psDroid->id));
		psDroid->action = psAction->action;
		psDroid->actionX = psAction->x;
		psDroid->actionY = psAction->y;
		psDroid->actionStarted = gameTime;
		psDroid->psActionTarget = psAction->psObj;
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
		psDroid->psActionTarget = psAction->psObj;
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
		psDroid->psActionTarget = psAction->psObj;
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
		psDroid->psActionTarget = psDroid->psTarget;
		moveDroidTo(psDroid, psAction->x, psAction->y);
		break;
	default:
		ASSERT( FALSE, "actionUnitBase: unknown action" );
		break;
	}
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
	sAction.x = psObj->x;
	sAction.y = psObj->y;
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

	// IF YOU CHANGE THE ORDER/ACTION RESULTS TELL ALEXL!!!! && recvvtolrearm

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
	psStruct = findNearestReArmPad(psDroid, psDroid->psBaseStruct, FALSE);
	if (psStruct)
	{
		// note a base rearm pad if the vtol doesn't have one
		if (psDroid->psBaseStruct == NULL)
		{
			psDroid->psBaseStruct = psStruct;
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
	if(bMultiPlayer)
	{
		sendVtolRearm(psDroid,psStruct,chosen);
	}
}


// whether a tile is suitable for a vtol to land on
static BOOL vtolLandingTile(SDWORD x, SDWORD y)
{
	MAPTILE		*psTile;

	if (x < 0 || x >= (SDWORD)mapWidth ||
		y < 0 || y >= (SDWORD)mapHeight)
	{
		return FALSE;
	}

	psTile = mapTile(x,y);

	if ((psTile->tileInfoBits & BITS_FPATHBLOCK) ||
		(TILE_OCCUPIED(psTile)) ||
		(TERRAIN_TYPE(psTile) == TER_CLIFFFACE) ||
		(TERRAIN_TYPE(psTile) == TER_WATER))
	{
		return FALSE;
	}
	return TRUE;
}


// choose a landing position for a VTOL when it goes to rearm
BOOL actionVTOLLandingPos(DROID *psDroid, UDWORD *px, UDWORD *py)
{
	SDWORD	i,j;
	SDWORD	startX,endX,startY,endY, tx,ty;
	UDWORD	passes;
	DROID	*psCurr;
	BOOL	result;

//	ASSERT( (psDroid->psActionTarget != NULL),
//		"actionVTOLLandingPos: no rearm pad set for the VTOL" );

	/* Initial box dimensions and set iteration count to zero */
//	startX = endX = (SDWORD)psDroid->psActionTarget->x >> TILE_SHIFT;
//	startY = endY = (SDWORD)psDroid->psActionTarget->y >> TILE_SHIFT;
	startX = endX = (SDWORD)*px >> TILE_SHIFT;
	startY = endY = (SDWORD)*py >> TILE_SHIFT;
	passes = 0;

	// set blocking flags for all the other droids
	for(psCurr=apsDroidLists[psDroid->player]; psCurr; psCurr = psCurr->psNext)
	{
		if (DROID_STOPPED(psCurr))
		{
			tx = psCurr->x >> TILE_SHIFT;
			ty = psCurr->y >> TILE_SHIFT;
		}
		else
		{
			tx = psCurr->sMove.DestinationX >> TILE_SHIFT;
			ty = psCurr->sMove.DestinationY >> TILE_SHIFT;
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
	result = FALSE;
	while(passes<20)
	{
		/* Process whole box */
		for(i = startX; i <= endX; i++)
		{
			for(j = startY; j<= endY; j++)
			{
				/* Test only perimeter as internal tested previous iteration */
				if(i==startX OR i==endX OR j==startY OR j==endY)
				{
					/* Good enough? */
					if(vtolLandingTile(i,j))
					{
						/* Set exit conditions and get out NOW */
						DBP4(("Unit %d landing pos (%d,%d)\n",psDroid->id, i,j));
						*px = (i << TILE_SHIFT) + TILE_UNITS/2;
						*py = (j << TILE_SHIFT) + TILE_UNITS/2;
						result = TRUE;
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
			tx = psCurr->x >> TILE_SHIFT;
			ty = psCurr->y >> TILE_SHIFT;
		}
		else
		{
			tx = psCurr->sMove.DestinationX >> TILE_SHIFT;
			ty = psCurr->sMove.DestinationY >> TILE_SHIFT;
		}
        if (tileOnMap(tx, ty))
        {
		    mapTile(tx,ty)->tileInfoBits &= ~BITS_FPATHBLOCK;
        }
	}

	return result;

}




