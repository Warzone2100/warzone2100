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
/*
 * Combat.c
 *
 * Combat mechanics routines.
 *
 */

#include "lib/framework/frame.h"

#include "objects.h"
#include "combat.h"
#include "stats.h"
#include "visibility.h"
#include "lib/gamelib/gtime.h"
#include "map.h"
#include "move.h"
#include "messagedef.h"
#include "miscimd.h"
#include "projectile.h"
#include "lib/sound/audio.h"
#include "geometry.h"
#include "cmddroid.h"
#include "mapgrid.h"
#include "order.h"
#include "ai.h"
#include "action.h"

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

// Watermelon:real projectile
/* Fire a weapon at something */
void combFire(WEAPON *psWeap, BASE_OBJECT *psAttacker, BASE_OBJECT *psTarget, int weapon_slot)
{
	WEAPON_STATS	*psStats;
	UDWORD			xDiff, yDiff, distSquared;
	UDWORD			dice, damLevel;
	SDWORD			missDir, missDist, missX,missY;
	SDWORD			resultHitChance=0,baseHitChance=0,fireChance;
	UDWORD			firePause;
	SDWORD			targetDir,dirDiff;
	SDWORD			longRange;
	DROID			*psDroid = NULL;
	int				minOffset = 5;
	//Watermelon:predicted X,Y offset per sec
	SDWORD			predictX;
	SDWORD			predictY;
	SDWORD			dist;

	CHECK_OBJECT(psAttacker);
	CHECK_OBJECT(psTarget);

	ASSERT( psWeap != NULL,
		"combFire: Invalid weapon pointer" );

	/* Watermelon:dont shoot if the weapon_slot of a vtol is empty */
	if (psAttacker->type == OBJ_DROID &&
		vtolDroid(((DROID *)psAttacker)))
	{
		if (((DROID *)psAttacker)->sMove.iAttackRuns[weapon_slot] >= getNumAttackRuns(((DROID *)psAttacker), weapon_slot))
		{
			debug(LOG_NEVER, "VTOL slot %d is empty", weapon_slot);
			return;
		}
	}

	/* Get the stats for the weapon */
	psStats = asWeaponStats + psWeap->nStat;

	// check valid weapon/prop combination
	if (!validTarget(psAttacker, psTarget, weapon_slot))
	{
		return;
	}

	/*see if reload-able weapon and out of ammo*/
	if (psStats->reloadTime && !psWeap->ammo)
	{
		if (gameTime - psWeap->lastFired < weaponReloadTime(psStats, psAttacker->player))
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
		 actionInsideMinRange(psDroid, psTarget, weapon_slot)) )
	{
		if(!visibleObjWallBlock(psAttacker, psTarget))
		{
			// Can't see the target - can't hit it with direct fire
			debug(LOG_SENSOR, "combFire(%u[%s]->%u): Droid has no direct line of sight to target",
			      psAttacker->id, ((DROID *)psAttacker)->aName, psTarget->id);
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
			debug(LOG_SENSOR, "combFire(%u[%s]->%u): Structure has no direct line of sight to target",
			      psAttacker->id, ((STRUCTURE *)psAttacker)->pStructureType->pName, psTarget->id);
			return;
		}
	}
	else if ( proj_Direct(psStats) )
	{
		// VTOL or tall building
		if (!visibleObject(psAttacker, psTarget))
		{
			// Can't see the target - can't hit it with direct fire
			debug(LOG_SENSOR, "combFire(%u[%s]->%u): Tall object has no direct line of sight to target",
			      psAttacker->id, psStats->pName, psTarget->id);
			return;
		}
	}
	else
	{
		// Indirect fire
		if (!psTarget->visible[psAttacker->player])
		{
			// Can't get an indirect LOS - can't hit it with the weapon
			debug(LOG_SENSOR, "combFire(%u[%s]->%u): Object has no indirect sight of target",
			      psAttacker->id, psStats->pName, psTarget->id);
			return;
		}
	}

	// if the turret doesn't turn, check if the attacker is in alignment with the target
	if (psAttacker->type == OBJ_DROID && !psStats->rotate)
	{
		targetDir = calcDirection(psAttacker->pos.x, psAttacker->pos.y, psTarget->pos.x, psTarget->pos.y);
		dirDiff = labs(targetDir - (SDWORD)psAttacker->direction);
		if (dirDiff > FIXED_TURRET_DIR)
		{
			return;
		}
	}

	// base hit chance, based on weapon's chance to hit as defined in
	// weapons.txt and with applied weapon upgrades, without any accuracy modifiers
	baseHitChance = 0;

	/* Now see if the target is in range  - also check not too near */
	xDiff = abs(psAttacker->pos.x - psTarget->pos.x);
	yDiff = abs(psAttacker->pos.y - psTarget->pos.y);
	distSquared = xDiff*xDiff + yDiff*yDiff;
	dist = sqrtf(distSquared);
	longRange = proj_GetLongRange(psStats);

	if (distSquared <= (psStats->shortRange * psStats->shortRange) &&
		distSquared >= (psStats->minRange * psStats->minRange))
	{
		// get weapon chance to hit in the short range
		baseHitChance = weaponShortHit(psStats,psAttacker->player);
	}
	else if ((SDWORD)distSquared <= longRange * longRange &&
			 ( (distSquared >= psStats->minRange * psStats->minRange) ||
			   ((psAttacker->type == OBJ_DROID) &&
			   !proj_Direct(psStats) &&
			   actionInsideMinRange(psDroid, psTarget, weapon_slot)) ))
	{
		// get weapon chance to hit in the long range
		baseHitChance = weaponLongHit(psStats,psAttacker->player);
	}
	else
	{
		/* Out of range */
		debug(LOG_NEVER, "combFire(%u[%s]->%u): Out of range", psAttacker->id, psStats->pName, psTarget->id);
		return;
	}

	// if target was in range deal with weapon fire
	if(baseHitChance > 0)
	{
		/* note when the weapon fired */
		psWeap->lastFired = gameTime;

		/* reduce ammo if salvo */
		if (psStats->reloadTime)
		{
			psWeap->ammo--;
		}
	}

	// apply experience accuracy modifiers to the base
	//hit chance, not to the final hit chance
	resultHitChance = baseHitChance;

	// add the attacker's experience
	if (psAttacker->type == OBJ_DROID)
	{
		SDWORD	level = getDroidEffectiveLevel((DROID *) psAttacker);

		// increase total accuracy by EXP_ACCURACY_BONUS % for each experience level
		resultHitChance += EXP_ACCURACY_BONUS * level * baseHitChance / 100;
	}

	// subtract the defender's experience
	if (psTarget->type == OBJ_DROID)
	{
		SDWORD	level = getDroidEffectiveLevel((DROID *) psTarget);

		// decrease weapon accuracy by EXP_ACCURACY_BONUS % for each experience level
		resultHitChance -= EXP_ACCURACY_BONUS * level * baseHitChance / 100;

	}

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
			resultHitChance = FOM_PARTIAL_ACCURACY_PENALTY * resultHitChance / 100;
			break;
		case FOM_YES:
			// can fire while moving
			break;
		}
	}

	// visibility modifiers
	//if (psTarget->visible[psAttacker->player] < VIS_ATTACK_MOD_LEVEL)
	if (psTarget->visible[psAttacker->player] == 0)		//not sure if this can ever be > 0 here
	{
		resultHitChance = INVISIBLE_ACCURACY_PENALTY * resultHitChance / 100;
	}

	// cap resultHitChance to 0-100%, just in case
	resultHitChance = MAX(0, resultHitChance);
	resultHitChance = MIN(100, resultHitChance);

	HIT_ROLL(dice);

	// see if we were lucky to hit the target
	if (dice <= resultHitChance)
	{
		/* Kerrrbaaang !!!!! a hit */
		//Watermelon:Target prediction
		if(psTarget->type == OBJ_DROID)
		{
			predictX = (SDWORD)(trigSin( ((DROID *)psTarget)->sMove.moveDir ) * ((DROID *)psTarget)->sMove.speed * dist / psStats->flightSpeed );
			predictX += psTarget->pos.x;
			predictY = (SDWORD)(trigCos( ((DROID *)psTarget)->sMove.moveDir ) * ((DROID *)psTarget)->sMove.speed * dist / psStats->flightSpeed );
			predictY += psTarget->pos.y;

			// Make sure we don't pass any negative or out of bounds numbers to proj_SendProjectile
			predictX = MAX(predictX, 0);
			predictX = MIN(predictX, world_coord(mapWidth - 1));
			predictY = MAX(predictY, 0);
			predictY = MIN(predictY, world_coord(mapHeight - 1));
		}
		else
		{
			predictX = psTarget->pos.x;
			predictY = psTarget->pos.y;
		}
		debug(LOG_SENSOR, "combFire: Accurate prediction range (%d)", dice);
		if (!proj_SendProjectile(psWeap, psAttacker, psAttacker->player,
							predictX, predictY, psTarget->pos.z, psTarget, FALSE, FALSE, weapon_slot))
		{
			/* Out of memory - we can safely ignore this */
			debug(LOG_ERROR, "Out of memory");
			return;
		}
	}
	else
	{
		goto missed;
	}

	debug(LOG_SENSOR, "combFire: %u[%s]->%u: resultHitChance=%d, visibility=%hhu : ",
	      psAttacker->id, psStats->pName, psTarget->id, resultHitChance, psTarget->visible[psAttacker->player]);

	return;

missed:
	/* Deal with a missed shot */

	missDist = 2 * (100 - resultHitChance);
	missDir = rand() % BUL_MAXSCATTERDIR;
	missX = aScatterDir[missDir].x * missDist + psTarget->pos.x + minOffset;
	missY = aScatterDir[missDir].y * missDist + psTarget->pos.y + minOffset;

	debug(LOG_NEVER, "combFire: Missed shot (%d) ended up at (%4d,%4d)", dice, missX, missY);

	/* Fire off the bullet to the miss location. The miss is only visible if the player owns
	 * the target. (Why? - Per) */
	proj_SendProjectile(psWeap, psAttacker, psAttacker->player, missX,missY, psTarget->pos.z, NULL,
	                    psTarget->player == selectedPlayer, FALSE, weapon_slot);

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

	CHECK_OBJECT(psTarget);

	gridStartIterate((SDWORD)psTarget->pos.x, (SDWORD)psTarget->pos.y);
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
			if (structCBSensor(psStruct) || structVTOLCBSensor(psStruct))
			{
				sensorRange = psStruct->pStructureType->pSensor->range;
			}
		}
		else if (psViewer->type == OBJ_DROID)
		{
			psDroid = (DROID *)psViewer;
			//must be a CB sensor
			/*if (asSensorStats[psDroid->asBits[COMP_SENSOR].nStat].type ==
				INDIRECT_CB_SENSOR || asSensorStats[psDroid->asBits[COMP_SENSOR].
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
			xDiff = (SDWORD)psViewer->pos.x - (SDWORD)psTarget->pos.x;
			yDiff = (SDWORD)psViewer->pos.y - (SDWORD)psTarget->pos.y;
			if (xDiff*xDiff + yDiff*yDiff < sensorRange * sensorRange)
			{
				//inform viewer of target
				if (psViewer->type == OBJ_DROID)
				{
					orderDroidObj((DROID *)psViewer, DORDER_OBSERVE, psAttacker);
				}
				else if (psViewer->type == OBJ_STRUCTURE)
				{
					((STRUCTURE *)psViewer)->psTarget[0] = psAttacker;
				}
			}
		}
	}
}
