/*
 * Combat.c
 *
 * Combat mechanics routines.
 *
 */

#include <math.h>

/* Turn on the damage printf's from combExplodeBullet */
//#define DEBUG_GROUP1
/* Turn on LOS printf's */
//#define DEBUG_GROUP2
/* Turn on Missed printf's */
//#define DEBUG_GROUP3
#include "lib/framework/frame.h"

#include "objects.h"
#include "combat.h"
#include "stats.h"
#include "visibility.h"
#include "lib/gamelib/gtime.h"
#include "map.h"
#include "move.h"
#include "findpath.h"
#include "messagedef.h"
#include "miscimd.h"
#include "projectile.h"
#include "lib/sound/audio.h"
#include "audio_id.h"
#include "geometry.h"
#include "cmddroid.h"
#include "mapgrid.h"
#include "order.h"
#include "ai.h"
#include "action.h"

#define	EXPLOSION_AUDIO	0

/* Number of tiles that missed bullets scatter from target */
//#define DIRECT_SCATTERDIST 3
//#define INDIRECT_SCATTERDIST 2

/* minimum miss distance */
#define MIN_MISSDIST	(TILE_UNITS/6)

/* The number of tiles of clear space needed for indirect fire */
#define INDIRECT_LOSDIST 2

// maximum random pause for firing
#define RANDOM_PAUSE	500

// visibility level below which the to hit chances are reduced
#define VIS_ATTACK_MOD_LEVEL	150

/* direction array for missed bullets */
typedef struct _bul_dir
{
	SDWORD x,y;
} BUL_DIR;
#define BUL_MAXSCATTERDIR 8
static BUL_DIR aScatterDir[BUL_MAXSCATTERDIR] =
{
	{ 0,-1 },
	{ 1,-1 },
	{ 1,0 },
	{ 1,1 },
	{ 0,1 },
	{ -1,1 },
	{ -1,0 },
	{ -1,-1 },
};

/* Initialise the combat system */
BOOL combInitialise(void)
{
	return TRUE;
}


/* Shutdown the combat system */
BOOL combShutdown(void)
{
	return TRUE;
}


/* Fire a weapon at something */
void combFire(WEAPON *psWeap, BASE_OBJECT *psAttacker, BASE_OBJECT *psTarget)
{
	WEAPON_STATS	*psStats;
	UDWORD			xDiff, yDiff, distSquared;
	UDWORD			dice, damLevel;
	SDWORD			missDir, missDist, missX,missY;
	SDWORD			hitMod, hitInc, fireChance;
	UDWORD			firePause;
	SDWORD			targetDir,dirDiff;
	SDWORD			longRange;
	DROID			*psDroid = NULL;
	SDWORD			level, cmdLevel;
	BOOL			bMissVisible;

	ASSERT((PTRVALID(psWeap, sizeof(WEAPON)),
		"combFire: Invalid weapon pointer"));
	ASSERT((PTRVALID(psAttacker, sizeof(BASE_OBJECT)),
		"combFire: Invalid attacker pointer"));
	ASSERT((PTRVALID(psTarget, sizeof(BASE_OBJECT)),
		"combFire: Invalid target pointer"));

	/* Get the stats for the weapon */
	psStats = asWeaponStats + psWeap->nStat;

    //check valid weapon/prop combination
    if (!validTarget(psAttacker, psTarget))
    {
        return;
    }

	/*see if reload-able weapon and out of ammo*/
	if (psStats->reloadTime AND !psWeap->ammo)
	{
		if (gameTime - psWeap->lastFired < psStats->reloadTime)
		{
			return;
		}
		//reset the ammo level
		psWeap->ammo = psStats->numRounds;
    }

	/* See when the weapon last fired to control it's rate of fire */
	firePause = weaponFirePause(psStats, psAttacker->player);

	// increase the pause if heavily damaged
	switch (psAttacker->type)
	{
	case OBJ_DROID:
		psDroid = (DROID *)psAttacker;
		damLevel = PERCENT(psDroid->body, psDroid->originalBody);

		break;
	case OBJ_STRUCTURE:
		damLevel = PERCENT(((STRUCTURE *)psAttacker)->body,
						//((STRUCTURE *)psAttacker)->baseBodyPoints);
			structureBody((STRUCTURE *)psAttacker));
		break;
	default:
		damLevel = 100;
		break;
	}

	if (damLevel < HEAVY_DAMAGE_LEVEL)
	{
		firePause += firePause;
	}


	if (gameTime - psWeap->lastFired <= firePause)
	{
		/* Too soon to fire again */
		return;
	}

	// add a random delay to the fire
	fireChance = gameTime - (psWeap->lastFired + firePause);
	if (rand() % RANDOM_PAUSE > fireChance)
	{
		return;
	}

	/* Check we can see the target */
	if ( (psAttacker->type == OBJ_DROID) &&
		 !vtolDroid((DROID *)psAttacker) &&
		 (proj_Direct(psStats) ||
		  actionInsideMinRange(psDroid, psDroid->psActionTarget)) )
	{
		if (!visibleObjWallBlock(psAttacker, psTarget))
		{
			// Can't see the target - can't hit it with direct fire
			DBP3(("directLOS failed\n"));
			return;
		}
	}
	else if ((psAttacker->type == OBJ_STRUCTURE) &&
			 (((STRUCTURE *)psAttacker)->pStructureType->height == 1) &&
			 proj_Direct(psStats))
	{
		// a bunker can't shoot through walls
		if (!visibleObjWallBlock(psAttacker, psTarget))
		{
			// Can't see the target - can't hit it with direct fire
			DBP3(("directLOS failed\n"));
			return;
		}
	}
	else if ( proj_Direct(psStats) )
	{
		if (!visibleObject(psAttacker, psTarget))
		{
			// Can't see the target - can't hit it with direct fire
			DBP3(("directLOS failed\n"));
			return;
		}
	}
	else
	{
		if (!psTarget->visible[psAttacker->player])
		{
			// Can't get an indirect LOS - can't hit it with the weapon
			DBP3(("indirectLOS failed\n"));
			return;
		}
	}

/*	if ( proj_Direct(psStats) ||
		 ((psAttacker->type == OBJ_DROID) &&
		  !proj_Direct(psStats) &&
		   actionInsideMinRange(psDroid, psDroid->psActionTarget)) )
	{
		switch (psAttacker->type)
		{
		case OBJ_DROID:
			if (!visibleObjWallBlock(psAttacker, psTarget))
			{
				// Can't see the target - can't hit it with direct fire
				DBP3(("directLOS failed\n"));
				return;
			}
			break;
		default:
			if (!visibleObject(psAttacker, psTarget))
			{
				// Can't see the target - can't hit it with direct fire
				DBP3(("directLOS failed\n"));
				return;
			}
			break;
		}
	}
	else
	{
		if (!psTarget->visible[psAttacker->player])
		{
			// Can't get an indirect LOS - can't hit it with the weapon
			DBP3(("indirectLOS failed\n"));
			return;
		}
	}*/

	// if the turret doesn't turn, check the attacker is in alignment with the
	// target
	if (psAttacker->type == OBJ_DROID &&
		!psStats->rotate)
	{
		targetDir = (SDWORD)calcDirection(psAttacker->x,psAttacker->y,
										  psTarget->x,psTarget->y);
		dirDiff = labs(targetDir - (SDWORD)psAttacker->direction);
		if (dirDiff > FIXED_TURRET_DIR)
		{
			return;
		}
	}

	// base modifier 100% of original
	hitMod = 100;
	// base hit increment of zero
	hitInc = 0;

	// apply upgrades - do these when know if its longHit or shortHit
	//hitMod = hitMod * (asWeaponUpgrade[psAttacker->player]
	//							[psStats->weaponSubClass].shortHit + 100) / 100;

	// add the attackers experience modifier
	if (psAttacker->type == OBJ_DROID)
	{
//		hitMod = hitMod + (hitMod * 5) * getDroidLevel((DROID *)psAttacker) / 100;
//		hitMod = hitMod + hitMod * cmdDroidHitMod((DROID *)psAttacker) / 100;
		level = getDroidLevel((DROID *)psAttacker);
		cmdLevel = cmdGetCommanderLevel((DROID *)psAttacker);
		if (level > cmdLevel)
		{
			hitInc += 5 * level;
		}
		else
		{
			hitInc += 5 * cmdLevel;
		}
	}

	// subtract the defenders experience modifier
/*	if (psTarget->type == OBJ_DROID)
	{
//		hitMod = hitMod - hitMod * 2 * getDroidLevel((DROID *)psTarget) / 100;
//		hitMod = hitMod - hitMod * cmdDroidEvasionMod((DROID *)psTarget) / 100;
		level = getDroidLevel((DROID *)psTarget);
		cmdLevel = cmdGetCommanderLevel((DROID *)psTarget);
		if (level > cmdLevel)
		{
			hitInc -= 5 * level;
		}
		else
		{
			hitInc -= 5 * cmdLevel;
		}
	}*/

	// fire while moving modifiers
	if (psAttacker->type == OBJ_DROID &&
		((DROID *)psAttacker)->sMove.Status != MOVEINACTIVE)
	{
		switch (psStats->fireOnMove)
		{
		case FOM_NO:
			// Can't fire while moving
			return;
			break;
		case FOM_PARTIAL:
			hitMod = 50*hitMod/100;
			break;
		case FOM_YES:
			// can fire while moving
			break;
		}
	}

	// visibility modifiers
	if (psTarget->visible[psAttacker->player] < VIS_ATTACK_MOD_LEVEL)
	{
		hitMod = 50*hitMod/100;
	}

	DBP3(("%s Hit mod %d : ", psStats->pName, hitMod));

	/* Now see if the target is in range  - also check not too near*/
	xDiff = abs(psAttacker->x - psTarget->x);
	yDiff = abs(psAttacker->y - psTarget->y);
	distSquared = xDiff*xDiff + yDiff*yDiff;
	longRange = proj_GetLongRange(psStats, (SDWORD)psAttacker->z-(SDWORD)psTarget->z);
	if (distSquared <= (psStats->shortRange * psStats->shortRange) AND
		distSquared >= (psStats->minRange * psStats->minRange))
	{
		/* note when the weapon fired */
		psWeap->lastFired = gameTime;
		/*reduce ammo if salvo*/
		if (psStats->reloadTime)
		{
			psWeap->ammo--;
		}

		/* Can do a short range shot - see if it hits */
		/************************************************/
		/* NEED TO TAKE ACCOUNT OF ECM, BODY SHAPE ETC. */
		/************************************************/
		HIT_ROLL(dice);
		//if (dice <= psStats->shortHit * hitMod /100)
		if (dice <= (weaponShortHit(psStats,psAttacker->player) * hitMod /100) + hitInc)
		{
			/* Kerrrbaaang !!!!! a hit */
			DBP3(("Shot hit (%d)\n", dice));
			if (!proj_SendProjectile(psWeap, psAttacker, psAttacker->player,
								psTarget->x, psTarget->y, psTarget->z, psTarget, FALSE))
			{
				/* Out of memory - we can safely ignore this */
				DBP3(("Out of memory"));
				return;
			}
		}
		else
		{
			goto missed;
		}
	}
	else if ((SDWORD)distSquared <= longRange * longRange &&
			 ( (distSquared >= psStats->minRange * psStats->minRange) ||
			   ((psAttacker->type == OBJ_DROID) &&
				!proj_Direct(psStats) &&
			     actionInsideMinRange(psDroid, psDroid->psActionTarget)) ) )
	{
		/* note when the weapon fired */
		psWeap->lastFired = gameTime;
		/*reduce ammo if salvo*/
		if (psStats->reloadTime)
		{
			psWeap->ammo--;
		}

		/* Can do a long range shot - see if it hits */
		/************************************************/
		/* NEED TO TAKE ACCOUNT OF ECM, BODY SHAPE ETC. */
		/************************************************/
		HIT_ROLL(dice);
		//if (dice <= psStats->longHit * hitMod /100)
		if (dice <= (weaponLongHit(psStats,psAttacker->player) * hitMod /100) + hitInc)
		{
			/* Kerrrbaaang !!!!! a hit */
			DBP3(("Shot hit (%d)\n", dice));
			if (!proj_SendProjectile(psWeap, psAttacker, psAttacker->player,
								psTarget->x, psTarget->y, psTarget->z, psTarget, FALSE))
			{
				/* Out of memory - we can safely ignore this */
				DBP3(("Out of memory"));
				return;
			}
		}
		else
		{
			goto missed;
		}
	}
	else
	{
		/* Out of range */
		DBP3(("Out of range\n"));
		return;
	}

	return;

missed:
	/* Deal with a missed shot */
	DBP3(("Missed shot (%d)\n", dice));

	// Approximate the distance between the attacker and target
	xDiff = ABSDIF(psAttacker->x,psTarget->x);
	yDiff = ABSDIF(psAttacker->y,psTarget->y);
	missDist = (xDiff > yDiff ? xDiff + (yDiff >> 1) : yDiff + (xDiff >> 1));

	// Calculate where the shot will end up
	missDist = missDist >> 2;
	if (missDist < MIN_MISSDIST)
	{
		missDist = MIN_MISSDIST;
	}
	missDir = rand() % BUL_MAXSCATTERDIR;
	missX = aScatterDir[missDir].x * (rand() % missDist) + psTarget->x;
	missY = aScatterDir[missDir].y * (rand() % missDist) + psTarget->y;
	DBP3(("Miss Loc: w(%4d,%4d), t(%3d,%3d)\n",
		missX, missY, missX>>TILE_SHIFT, missY>>TILE_SHIFT));

	// decide if a miss is visible
	bMissVisible = FALSE;
	if (psTarget->player == selectedPlayer)
	{
		bMissVisible = TRUE;
	}

	/* Fire off the bullet to the miss location */
	if (!proj_SendProjectile( psWeap, psAttacker, psAttacker->player, missX,missY, psTarget->z, NULL, bMissVisible) )
	{
		/* Out of memory */
		DBP3(("Out of memory"));
		return;
	}

	return;
}

/*checks through the target players list of structures and droids to see
if any support a counter battery sensor*/
void counterBatteryFire(BASE_OBJECT *psAttacker, BASE_OBJECT *psTarget)
{
	STRUCTURE		*psStruct;
	DROID			*psDroid;
	BASE_OBJECT		*psViewer;
	SDWORD			sensorRange;
	SDWORD			xDiff, yDiff;

	/*if a null target is passed in ignore - this will be the case when a 'miss'
	projectile is sent - we may have to cater for these at some point*/
	// also ignore cases where you attack your own player
	if ((psTarget == NULL) ||
		((psAttacker != NULL) && (psAttacker->player == psTarget->player)))
	{
		return;
	}


	gridStartIterate((SDWORD)psTarget->x, (SDWORD)psTarget->y);
	for (psViewer = gridIterate(); psViewer != NULL; psViewer = gridIterate())
	{
		if (psViewer->player != psTarget->player)
		{
			//ignore non target players' objects
			continue;
		}
		sensorRange = 0;
		if (psViewer->type == OBJ_STRUCTURE)
		{
			psStruct = (STRUCTURE *)psViewer;
			//check if have a sensor of correct type
			if (structCBSensor(psStruct) OR structVTOLCBSensor(psStruct))
			{
				sensorRange = psStruct->pStructureType->pSensor->range;
			}
		}
		else if (psViewer->type == OBJ_DROID)
		{
			psDroid = (DROID *)psViewer;
			//must be a CB sensor
			/*if (asSensorStats[psDroid->asBits[COMP_SENSOR].nStat].type ==
				INDIRECT_CB_SENSOR OR asSensorStats[psDroid->asBits[COMP_SENSOR].
				nStat].type == VTOL_CB_SENSOR)*/
            if (cbSensorDroid(psDroid))
			{
				sensorRange = asSensorStats[psDroid->asBits[COMP_SENSOR].
					nStat].range;
			}
		}
		//check sensor distance from target
		if (sensorRange)
		{
			xDiff = (SDWORD)psViewer->x - (SDWORD)psTarget->x;
			yDiff = (SDWORD)psViewer->y - (SDWORD)psTarget->y;
			if (xDiff*xDiff + yDiff*yDiff < sensorRange * sensorRange)
			{
				//inform viewer of target
				if (psViewer->type == OBJ_DROID)
				{
					orderDroidObj((DROID *)psViewer, DORDER_OBSERVE, psAttacker);
				}
				else if (psViewer->type == OBJ_STRUCTURE)
				{
					((STRUCTURE *)psViewer)->psTarget = psAttacker;
				}
			}
		}
	}
}
