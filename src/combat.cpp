/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
 * @file combat.c
 *
 * Combat mechanics routines.
 *
 */

#include "lib/framework/frame.h"
#include "lib/netplay/netplay.h"

#include "action.h"
#include "cluster.h"
#include "combat.h"
#include "difficulty.h"
#include "geometry.h"
#include "mapgrid.h"
#include "projectile.h"
#include "random.h"

// maximum random pause for firing
#define RANDOM_PAUSE	500

/* Fire a weapon at something */
bool combFire(WEAPON *psWeap, BASE_OBJECT *psAttacker, BASE_OBJECT *psTarget, int weapon_slot)
{
	WEAPON_STATS	*psStats;
	UDWORD			firePause;
	SDWORD			longRange;
	int				compIndex;

	CHECK_OBJECT(psAttacker);
	CHECK_OBJECT(psTarget);
	ASSERT(psWeap != NULL, "Invalid weapon pointer");

	/* Watermelon:dont shoot if the weapon_slot of a vtol is empty */
	if (psAttacker->type == OBJ_DROID && isVtolDroid(((DROID *)psAttacker)))
	{
		if (((DROID *)psAttacker)->sMove.iAttackRuns[weapon_slot] >= getNumAttackRuns(((DROID *)psAttacker), weapon_slot))
		{
			objTrace(psAttacker->id, "VTOL slot %d is empty", weapon_slot);
			return false;
		}
	}

	/* Get the stats for the weapon */
	compIndex = psWeap->nStat;
	ASSERT_OR_RETURN( false , compIndex < numWeaponStats, "Invalid range referenced for numWeaponStats, %d > %d", compIndex, numWeaponStats);
	psStats = asWeaponStats + compIndex;

	// check valid weapon/prop combination
	if (!validTarget(psAttacker, psTarget, weapon_slot))
	{
		return false;
	}

	unsigned fireTime = gameTime - deltaGameTime + 1;  // Can fire earliest at the start of the tick.

	/*see if reload-able weapon and out of ammo*/
	if (psStats->reloadTime && !psWeap->ammo)
	{
		fireTime = std::max(fireTime, psWeap->lastFired + weaponReloadTime(psStats, psAttacker->player));
		if (gameTime < fireTime)
		{
			return false;
		}
		//reset the ammo level
		psWeap->ammo = psStats->numRounds;
	}

	/* See when the weapon last fired to control it's rate of fire */
	firePause = weaponFirePause(psStats, psAttacker->player);
	firePause = std::max(firePause, 1u);  // Don't shoot infinitely many shots at once.
	fireTime = std::max(fireTime, psWeap->lastFired + firePause);

	if (gameTime < fireTime)
	{
		/* Too soon to fire again */
		return false;
	}

	if (psTarget->visible[psAttacker->player] != UBYTE_MAX)
	{
		// Can't see it - can't hit it
		objTrace(psAttacker->id, "combFire(%u[%s]->%u): Object has no indirect sight of target", psAttacker->id, psStats->pName, psTarget->id);
		return false;
	}

	/* Check we can see the target */
	if (psAttacker->type == OBJ_DROID && !isVtolDroid((DROID *)psAttacker)
	    && (proj_Direct(psStats) || actionInsideMinRange((DROID *)psAttacker, psTarget, psStats)))
	{
		if(!lineOfFire(psAttacker, psTarget, weapon_slot, true))
		{
			// Can't see the target - can't hit it with direct fire
			objTrace(psAttacker->id, "combFire(%u[%s]->%u): Droid has no direct line of sight to target",
			      psAttacker->id, ((DROID *)psAttacker)->aName, psTarget->id);
			return false;
		}
	}
	else if ((psAttacker->type == OBJ_STRUCTURE) &&
			 (((STRUCTURE *)psAttacker)->pStructureType->height == 1) &&
			 proj_Direct(psStats))
	{
		// a bunker can't shoot through walls
		if (!lineOfFire(psAttacker, psTarget, weapon_slot, true))
		{
			// Can't see the target - can't hit it with direct fire
			objTrace(psAttacker->id, "combFire(%u[%s]->%u): Structure has no direct line of sight to target",
			      psAttacker->id, ((STRUCTURE *)psAttacker)->pStructureType->pName, psTarget->id);
			return false;
		}
	}
	else if ( proj_Direct(psStats) )
	{
		// VTOL or tall building
		if (!lineOfFire(psAttacker, psTarget, weapon_slot, false))
		{
			// Can't see the target - can't hit it with direct fire
			objTrace(psAttacker->id, "combFire(%u[%s]->%u): Tall object has no direct line of sight to target",
			      psAttacker->id, psStats->pName, psTarget->id);
			return false;
		}
	}

	Vector3i deltaPos = psTarget->pos - psAttacker->pos;

	// if the turret doesn't turn, check if the attacker is in alignment with the target
	if (psAttacker->type == OBJ_DROID && !psStats->rotate)
	{
		uint16_t targetDir = iAtan2(removeZ(deltaPos));
		int dirDiff = abs(angleDelta(targetDir - psAttacker->rot.direction));
		if (dirDiff > FIXED_TURRET_DIR)
		{
			return false;
		}
	}

	/* Now see if the target is in range  - also check not too near */
	int dist = iHypot(removeZ(deltaPos));
	longRange = proj_GetLongRange(psStats);

	/* modification by CorvusCorax - calculate shooting angle */
	int min_angle = 0;
	// only calculate for indirect shots
	if (!proj_Direct(psStats) && dist > 0)
	{
		min_angle = arcOfFire(psAttacker,psTarget,weapon_slot,true);

		// prevent extremely steep shots
		min_angle = std::min(min_angle, DEG(PROJ_ULTIMATE_PITCH));

		// adjust maximum range of unit if forced to shoot very steep
		if (min_angle > DEG(PROJ_MAX_PITCH))
		{
			//do not allow increase of max range though
			if (iSin(2*min_angle) < iSin(2*DEG(PROJ_MAX_PITCH)))  // If PROJ_MAX_PITCH == 45, then always iSin(2*min_angle) <= iSin(2*DEG(PROJ_MAX_PITCH)), and the test is redundant.
			{
				longRange = longRange * iSin(2*min_angle) / iSin(2*DEG(PROJ_MAX_PITCH));
			}
		}
	}

	int baseHitChance = 0;
	if ((dist <= psStats->shortRange)  && (dist >= psStats->minRange))
	{
		// get weapon chance to hit in the short range
		baseHitChance = weaponShortHit(psStats,psAttacker->player);
	}
	else if ((dist <= longRange && dist >= psStats->minRange)
	         || (psAttacker->type == OBJ_DROID
	             && !proj_Direct(psStats)
	             && actionInsideMinRange((DROID *)psAttacker, psTarget, psStats)))
	{
		// get weapon chance to hit in the long range
		baseHitChance = weaponLongHit(psStats,psAttacker->player);

		// adapt for height adjusted artillery shots
		if (min_angle > DEG(PROJ_MAX_PITCH))
		{
			baseHitChance = baseHitChance * iCos(min_angle) / iCos(DEG(PROJ_MAX_PITCH));
		}
	}
	else
	{
		/* Out of range */
		objTrace(psAttacker->id, "combFire(%u[%s]->%u): Out of range", psAttacker->id, psStats->pName, psTarget->id);
		return false;
	}

	// apply experience accuracy modifiers to the base
	//hit chance, not to the final hit chance
	int resultHitChance = baseHitChance;

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
			return false;
			break;
		case FOM_PARTIAL:
			resultHitChance = FOM_PARTIAL_ACCURACY_PENALTY * resultHitChance / 100;
			break;
		case FOM_YES:
			// can fire while moving
			break;
		}
	}

	/* -------!!! From that point we are sure that we are firing !!!------- */

	// Add a random delay to the next shot.
	// TODO Add deltaFireTime to the time it takes to fire next. If just adding to psWeap->lastFired, it might put it in the future, causing assertions. And if not sometimes putting it in the future, the fire rate would be lower than advertised.
	//int fireJitter = firePause/100;  // ±1% variation in fire rate.
	//int deltaFireTime = gameRand(fireJitter*2 + 1) - fireJitter;

	/* note when the weapon fired */
	psWeap->lastFired = fireTime;

	/* reduce ammo if salvo */
	if (psStats->reloadTime)
	{
		psWeap->ammo--;
	}

	// increment the shots counter
	psWeap->shotsFired++;

	// predicted X,Y offset per sec
	Vector3i predict = psTarget->pos;

	//Watermelon:Target prediction
	if (isDroid(psTarget))
	{
		DROID *psDroid = castDroid(psTarget);

		int32_t flightTime;
		if (proj_Direct(psStats) || dist <= psStats->minRange)
		{
			flightTime = dist * GAME_TICKS_PER_SEC / psStats->flightSpeed;
		}
		else
		{
			int32_t vXY, vZ;  // Unused, we just want the flight time.
			flightTime = projCalcIndirectVelocities(dist, deltaPos.z, psStats->flightSpeed, &vXY, &vZ, min_angle);
		}

		if (psTarget->lastHitWeapon == WSC_EMP)
		{
			int empTime = EMP_DISABLE_TIME - (gameTime - psTarget->timeLastHit);
			CLIP(empTime, 0, EMP_DISABLE_TIME);
			if (empTime >= EMP_DISABLE_TIME * 9/10)
			{
				flightTime = 0;  /* Just hit.  Assume they'll get hit again */
			}
			else
			{
				flightTime = MAX(0, flightTime - empTime);
			}
		}

		predict += Vector3i(iSinCosR(psDroid->sMove.moveDir, psDroid->sMove.speed*flightTime / GAME_TICKS_PER_SEC), 0);
		if (!isFlying(psDroid))
		{
			predict.z = map_Height(removeZ(predict));  // Predict that the object will be on the ground.
		}
	}

	/* Fire off the bullet to the miss location. The miss is only visible if the player owns the target. (Why? - Per) */
	// What bVisible really does is to make the projectile audible even if it misses you. Since the target is NULL, proj_SendProjectile can't check if it was fired at you.
	bool bVisibleAnyway = psTarget->player == selectedPlayer;

	// see if we were lucky to hit the target
	bool isHit = gameRand(100) <= resultHitChance;
	if (isHit)
	{
		/* Kerrrbaaang !!!!! a hit */
		objTrace(psAttacker->id, "combFire: [%s]->%u: resultHitChance=%d, visibility=%d", psStats->pName, psTarget->id, resultHitChance, (int)psTarget->visible[psAttacker->player]);
		syncDebug("hit=(%d,%d,%d)", predict.x, predict.y, predict.z);
	}
	else /* Deal with a missed shot */
	{
		const int minOffset = 5;

		int missDist = 2 * (100 - resultHitChance) + minOffset;
		Vector3i miss = Vector3i(iSinCosR(gameRand(DEG(360)), missDist), 0);
		predict += miss;

		psTarget = NULL;  // Missed the target, so don't expect to hit it.

		objTrace(psAttacker->id, "combFire: Missed shot by (%4d,%4d)", miss.x, miss.y);
		syncDebug("miss=(%d,%d,%d)", predict.x, predict.y, predict.z);
	}

	// Make sure we don't pass any negative or out of bounds numbers to proj_SendProjectile
	CLIP(predict.x, 0, world_coord(mapWidth - 1));
	CLIP(predict.y, 0, world_coord(mapHeight - 1));

	proj_SendProjectileAngled(psWeap, psAttacker, psAttacker->player, predict, psTarget, bVisibleAnyway, weapon_slot, min_angle, fireTime);
	return true;
}

/*checks through the target players list of structures and droids to see
if any support a counter battery sensor*/
void counterBatteryFire(BASE_OBJECT *psAttacker, BASE_OBJECT *psTarget)
{
	BASE_OBJECT		*psViewer;

	/*if a null target is passed in ignore - this will be the case when a 'miss'
	projectile is sent - we may have to cater for these at some point*/
	// also ignore cases where you attack your own player
	// Also ignore cases where there are already 1000 missiles heading towards the attacker.
	if ((psTarget == NULL) ||
		((psAttacker != NULL) && (psAttacker->player == psTarget->player)) ||
		aiObjectIsProbablyDoomed(psAttacker))
	{
		return;
	}

	CHECK_OBJECT(psTarget);

	gridStartIterate(psTarget->pos.x, psTarget->pos.y, PREVIOUS_DEFAULT_GRID_SEARCH_RADIUS);
	for (psViewer = gridIterate(); psViewer != NULL; psViewer = gridIterate())
	{
		STRUCTURE	*psStruct;
		DROID		*psDroid;
		SDWORD		sensorRange = 0;

		if (psViewer->player != psTarget->player)
		{
			//ignore non target players' objects
			continue;
		}
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
			if (cbSensorDroid(psDroid))
			{
				sensorRange = asSensorStats[psDroid->asBits[COMP_SENSOR].
					nStat].range;
			}
		}
		//check sensor distance from target
		if (sensorRange)
		{
			SDWORD	xDiff = (SDWORD)psViewer->pos.x - (SDWORD)psTarget->pos.x;
			SDWORD	yDiff = (SDWORD)psViewer->pos.y - (SDWORD)psTarget->pos.y;

			if (xDiff*xDiff + yDiff*yDiff < sensorRange * sensorRange)
			{
				//inform viewer of target
				if (psViewer->type == OBJ_DROID)
				{
					orderDroidObj((DROID *)psViewer, DORDER_OBSERVE, psAttacker, ModeImmediate);
				}
				else if (psViewer->type == OBJ_STRUCTURE)
				{
					((STRUCTURE *)psViewer)->psTarget[0] = psAttacker;
				}
			}
		}
	}
}

/* Deals damage to an object
 * \param psObj object to deal damage to
 * \param damage amount of damage to deal
 * \param weaponClass the class of the weapon that deals the damage
 * \param weaponSubClass the subclass of the weapon that deals the damage
 * \return < 0 when the dealt damage destroys the object, > 0 when the object survives
 */
int32_t objDamage(BASE_OBJECT *psObj, unsigned damage, unsigned originalhp, WEAPON_CLASS weaponClass, WEAPON_SUBCLASS weaponSubClass, bool isDamagePerSecond)
{
	int	actualDamage, armour, level = 1;

	// If the previous hit was by an EMP cannon and this one is not:
	// don't reset the weapon class and hit time
	// (Giel: I guess we need this to determine when the EMP-"shock" is over)
	if (psObj->lastHitWeapon != WSC_EMP || weaponSubClass == WSC_EMP)
	{
		psObj->timeLastHit = gameTime;
		psObj->lastHitWeapon = weaponSubClass;
	}

	// EMP cannons do no damage, if we are one return now
	if (weaponSubClass == WSC_EMP)
	{
		return 0;
	}


	// apply game difficulty setting
	damage = modifyForDifficultyLevel(damage, psObj->player != selectedPlayer);

	armour = psObj->armour[weaponClass];

	debug(LOG_ATTACK, "objDamage(%d): body %d armour %d damage: %d", psObj->id, psObj->body, armour, damage);

	if (psObj->type == OBJ_STRUCTURE || psObj->type == OBJ_DROID)
	{
		// Force sending messages, even if messages were turned off, since a non-synchronised script will execute here. (Aaargh!)
		bool bMultiMessagesBackup = bMultiMessages;
		bMultiMessages = bMultiPlayer;

		clustObjectAttacked((BASE_OBJECT *)psObj);

		bMultiMessages = bMultiMessagesBackup;
	}


	if (psObj->type == OBJ_DROID)
	{
		DROID *psDroid = (DROID *)psObj;

		// Retrieve highest, applicable, experience level
		level = getDroidEffectiveLevel(psDroid);
	}

	// Reduce damage taken by EXP_REDUCE_DAMAGE % for each experience level
	actualDamage = (damage * (100 - EXP_REDUCE_DAMAGE * level)) / 100;

	// You always do at least a third of the experience modified damage
	actualDamage = MAX(actualDamage - armour, actualDamage / 3);

	// And at least MIN_WEAPON_DAMAGE points
	actualDamage = MAX(actualDamage, MIN_WEAPON_DAMAGE);

	if (isDamagePerSecond)
	{
		int deltaDamageRate = actualDamage - psObj->burnDamage;
		if (deltaDamageRate <= 0)
		{
			return 0;  // Did this much damage already, this tick, so don't do more.
		}
		actualDamage = gameTimeAdjustedAverage(deltaDamageRate);
		psObj->burnDamage += deltaDamageRate;
	}

	objTrace(psObj->id, "objDamage: Penetrated %d", actualDamage);
	syncDebug("damage%u dam%u,o%u,wc%d.%d,ar%d,lev%d,aDam%d,isDps%d", psObj->id, damage, originalhp, weaponClass, weaponSubClass, armour, level, actualDamage, isDamagePerSecond);

	// for some odd reason, we have 0 hitpoints.
	if (!originalhp)
	{
		ASSERT(originalhp, "original hitpoints are 0 ?");
		return -65536;  // it is dead
	}

	// If the shell did sufficient damage to destroy the object, deal with it and return
	if (actualDamage >= psObj->body)
	{
		return -(int64_t)65536 * psObj->body / originalhp;
	}

	// Subtract the dealt damage from the droid's remaining body points
	psObj->body -= actualDamage;

	syncDebugObject(psObj, 'D');

	return (int64_t)65536 * actualDamage / originalhp;
}

/* Guesses how damage a shot might do.
 * \param psObj object that might be hit
 * \param damage amount of damage to deal
 * \param weaponClass the class of the weapon that deals the damage
 * \param weaponSubClass the subclass of the weapon that deals the damage
 * \return guess at amount of damage
 */
unsigned int objGuessFutureDamage(WEAPON_STATS *psStats, unsigned int player, BASE_OBJECT *psTarget)
{
	unsigned int damage;
	int	actualDamage, armour = 0, level = 1;

	if (psTarget == NULL)
		return 0;  // Hard to destroy the ground. The armour on the mud is very strong and blocks all damage.

	damage = calcDamage(weaponDamage(psStats, player), psStats->weaponEffect, psTarget);

	// EMP cannons do no damage, if we are one return now
	if (psStats->weaponSubClass == WSC_EMP)
	{
		return 0;
	}


	// apply game difficulty setting
	damage = modifyForDifficultyLevel(damage, psTarget->player != selectedPlayer);

	armour = MAX(armour, psTarget->armour[psStats->weaponClass]);

	//debug(LOG_ATTACK, "objGuessFutureDamage(%d): body %d armour %d damage: %d", psObj->id, psObj->body, armour, damage);

	if (psTarget->type == OBJ_DROID)
	{
		DROID *psDroid = (DROID *)psTarget;

		// Retrieve highest, applicable, experience level
		level = getDroidEffectiveLevel(psDroid);
	}

	// Reduce damage taken by EXP_REDUCE_DAMAGE % for each experience level
	actualDamage = (damage * (100 - EXP_REDUCE_DAMAGE * level)) / 100;

	// You always do at least a third of the experience modified damage
	actualDamage = MAX(actualDamage - armour, actualDamage / 3);

	// And at least MIN_WEAPON_DAMAGE points
	actualDamage = MAX(actualDamage, MIN_WEAPON_DAMAGE);

	//objTrace(psObj->id, "objGuessFutureDamage: Would penetrate %d", actualDamage);

	return actualDamage;
}
