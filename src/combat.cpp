/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
#include "lib/framework/fixedpoint.h"
#include "lib/netplay/netplay.h"

#include "action.h"
#include "combat.h"
#include "difficulty.h"
#include "geometry.h"
#include "mapgrid.h"
#include "projectile.h"
#include "random.h"
#include "qtscript.h"
#include "order.h"
#include "objmem.h"

/* Fire a weapon at something */
bool combFire(WEAPON *psWeap, BASE_OBJECT *psAttacker, BASE_OBJECT *psTarget, int weapon_slot)
{
	WEAPON_STATS	*psStats;
	UDWORD			firePause;
	SDWORD			longRange;
	SDWORD			shortRange;
	int				compIndex;

	CHECK_OBJECT(psAttacker);
	CHECK_OBJECT(psTarget);
	ASSERT(psWeap != nullptr, "Invalid weapon pointer");

	/* Don't shoot if the weapon_slot of a vtol is empty */
	if (psAttacker->type == OBJ_DROID && isVtolDroid(((DROID *)psAttacker))
	    && psWeap->usedAmmo >= getNumAttackRuns(((DROID *)psAttacker), weapon_slot))
	{
		objTrace(psAttacker->id, "VTOL slot %d is empty", weapon_slot);
		return false;
	}

	/* Get the stats for the weapon */
	compIndex = psWeap->nStat;
	ASSERT_OR_RETURN(false , compIndex < numWeaponStats, "Invalid range referenced for numWeaponStats, %d > %d", compIndex, numWeaponStats);
	psStats = asWeaponStats + compIndex;

	// check valid weapon/prop combination
	if (!validTarget(psAttacker, psTarget, weapon_slot))
	{
		return false;
	}

	unsigned fireTime = gameTime - deltaGameTime + 1;  // Can fire earliest at the start of the tick.

	// See if reloadable weapon.
	if (psStats->upgrade[psAttacker->player].reloadTime)
	{
		unsigned reloadTime = psWeap->lastFired + weaponReloadTime(psStats, psAttacker->player);
		if (psWeap->ammo == 0)  // Out of ammo?
		{
			fireTime = std::max(fireTime, reloadTime);  // Have to wait for weapon to reload before firing.
			if (gameTime < fireTime)
			{
				return false;
			}
		}

		if (reloadTime <= fireTime)
		{
			//reset the ammo level
			psWeap->ammo = psStats->upgrade[psAttacker->player].numRounds;
		}
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
		objTrace(psAttacker->id, "combFire(%u[%s]->%u): Object has no indirect sight of target", psAttacker->id, getName(psStats), psTarget->id);
		return false;
	}

	/* Check we can hit the target */
	bool tall = (psAttacker->type == OBJ_DROID && isVtolDroid((DROID *)psAttacker))
	            || (psAttacker->type == OBJ_STRUCTURE && ((STRUCTURE *)psAttacker)->pStructureType->height > 1);
	if (proj_Direct(psStats) && !lineOfFire(psAttacker, psTarget, weapon_slot, tall))
	{
		// Can't see the target - can't hit it with direct fire
		objTrace(psAttacker->id, "combFire(%u[%s]->%u): No direct line of sight to target",
		         psAttacker->id, objInfo(psAttacker), psTarget->id);
		return false;
	}

	Vector3i deltaPos = psTarget->pos - psAttacker->pos;

	// if the turret doesn't turn, check if the attacker is in alignment with the target
	if (psAttacker->type == OBJ_DROID && !psStats->rotate)
	{
		uint16_t targetDir = iAtan2(deltaPos.xy());
		int dirDiff = abs(angleDelta(targetDir - psAttacker->rot.direction));
		if (dirDiff > FIXED_TURRET_DIR)
		{
			return false;
		}
	}

	/* Now see if the target is in range  - also check not too near */
	int dist = iHypot(deltaPos.xy());
	longRange = proj_GetLongRange(psStats, psAttacker->player);
	shortRange = proj_GetShortRange(psStats, psAttacker->player);

	int min_angle = 0;
	// Calculate angle for indirect shots
	if (!proj_Direct(psStats) && dist > 0)
	{
		min_angle = arcOfFire(psAttacker, psTarget, weapon_slot, true);

		// prevent extremely steep shots
		min_angle = std::min(min_angle, DEG(PROJ_ULTIMATE_PITCH));

		// adjust maximum range of unit if forced to shoot very steep
		if (min_angle > DEG(PROJ_MAX_PITCH))
		{
			//do not allow increase of max range though
			if (iSin(2 * min_angle) < iSin(2 * DEG(PROJ_MAX_PITCH))) // If PROJ_MAX_PITCH == 45, then always iSin(2*min_angle) <= iSin(2*DEG(PROJ_MAX_PITCH)), and the test is redundant.
			{
				longRange = longRange * iSin(2 * min_angle) / iSin(2 * DEG(PROJ_MAX_PITCH));
			}
		}
	}

	int baseHitChance = 0;
	const int min_range = proj_GetMinRange(psStats, psAttacker->player);
	if (dist <= shortRange && dist >= min_range)
	{
		// get weapon chance to hit in the short range
		baseHitChance = weaponShortHit(psStats, psAttacker->player);
	}
	else if (dist <= longRange && dist >= min_range)
	{
		// get weapon chance to hit in the long range
		baseHitChance = weaponLongHit(psStats, psAttacker->player);
	}
	else
	{
		/* Out of range */
		objTrace(psAttacker->id, "combFire(%u[%s]->%u): Out of range", psAttacker->id, getName(psStats), psTarget->id);
		return false;
	}

	// adapt for height adjusted artillery shots
	if (min_angle > DEG(PROJ_MAX_PITCH))
	{
		baseHitChance = baseHitChance * iCos(min_angle) / iCos(DEG(PROJ_MAX_PITCH));
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

	if (psAttacker->type == OBJ_DROID && ((DROID *)psAttacker)->sMove.Status != MOVEINACTIVE
	    && !psStats->fireOnMove)
	{
		return false;	// Can't fire while moving
	}

	/* -------!!! From that point we are sure that we are firing !!!------- */

	/* note when the weapon fired */
	psWeap->lastFired = fireTime;

	/* reduce ammo if salvo */
	if (psStats->upgrade[psAttacker->player].reloadTime)
	{
		psWeap->ammo--;
	}

	// increment the shots counter
	psWeap->shotsFired++;

	// predicted X,Y offset per sec
	Vector3i predict = psTarget->pos;

	// Target prediction
	if (isDroid(psTarget) && castDroid(psTarget)->sMove.bumpTime == 0)
	{
		DROID *psDroid = castDroid(psTarget);

		int32_t flightTime;
		if (proj_Direct(psStats) || dist <= proj_GetMinRange(psStats, psAttacker->player))
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
			if (empTime >= EMP_DISABLE_TIME * 9 / 10)
			{
				flightTime = 0;  /* Just hit.  Assume they'll get hit again */
			}
			else
			{
				flightTime = MAX(0, flightTime - empTime);
			}
		}

		predict += Vector3i(iSinCosR(psDroid->sMove.moveDir, psDroid->sMove.speed * flightTime / GAME_TICKS_PER_SEC), 0);
		if (!isFlying(psDroid))
		{
			predict.z = map_Height(predict.xy());  // Predict that the object will be on the ground.
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
		objTrace(psAttacker->id, "combFire: [%s]->%u: resultHitChance=%d, visibility=%d", getName(psStats), psTarget->id, resultHitChance, (int)psTarget->visible[psAttacker->player]);
		syncDebug("hit=(%d,%d,%d)", predict.x, predict.y, predict.z);
	}
	else /* Deal with a missed shot */
	{
		// Get the shape of the target to avoid "missing" inside of it
		const ObjectShape targetShape = establishTargetShape(psTarget);

		// Worst possible shot based on distance and weapon accuracy
		Vector3i deltaPosPredict = psAttacker->pos - predict;
		int worstShot;
		if (resultHitChance > 0)
		{
			worstShot = (iHypot(deltaPosPredict.xy()) * 100 / resultHitChance) / 5;
 		}
		else
		{
			worstShot = iHypot(deltaPosPredict.xy()) * 2;
		}

		// Use a random seed to determine how far the miss will land from the target
		// That (num/100)^3 allow the misses to fall much more frequently close to the target
		int num = gameRand(100) + 1;
		int minOffset = 2 * targetShape.radius();

		int missDist = minOffset + (worstShot * num * num * num) / (100 * 100 * 100);

		// Determine the angle of the miss in the 270 degrees in "front" of the target.
		// The 90 degrees behind would most probably cause an unwanted hit when the projectile will be drawn through the hitbox.
		Vector3i miss = Vector3i(iSinCosR(gameRand(DEG(270)) - DEG(135) + iAtan2(deltaPosPredict.xy()), missDist), 0);
		predict += miss;

		psTarget = nullptr;  // Missed the target, so don't expect to hit it.

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
	/*if a null target is passed in ignore - this will be the case when a 'miss'
	projectile is sent - we may have to cater for these at some point*/
	// also ignore cases where you attack your own player
	// Also ignore cases where there are already 1000 missiles heading towards the attacker.
	if (psTarget == nullptr
	    || (psAttacker != nullptr && psAttacker->player == psTarget->player)
	    || (psAttacker != nullptr && aiObjectIsProbablyDoomed(psAttacker, false)))
	{
		return;
	}

	CHECK_OBJECT(psTarget);

	for (BASE_OBJECT *psViewer = apsSensorList[0]; psViewer; psViewer = psViewer->psNextFunc)
	{
		if (aiCheckAlliances(psTarget->player, psViewer->player))
		{
			if ((psViewer->type == OBJ_STRUCTURE && !structCBSensor((STRUCTURE *)psViewer))
			    || (psViewer->type == OBJ_DROID && !cbSensorDroid((DROID *)psViewer)))
			{
				continue;
			}
			const int sensorRange = objSensorRange(psViewer);

			// Check sensor distance from target
			const int xDiff = psViewer->pos.x - psTarget->pos.x;
			const int yDiff = psViewer->pos.y - psTarget->pos.y;

			if (xDiff * xDiff + yDiff * yDiff < sensorRange * sensorRange)
			{
				// Inform viewer of target
				if (psViewer->type == OBJ_DROID)
				{
					orderDroidObj((DROID *)psViewer, DORDER_OBSERVE, psAttacker, ModeImmediate);
				}
				else if (psViewer->type == OBJ_STRUCTURE)
				{
					setStructureTarget((STRUCTURE *)psViewer, psAttacker, 0, ORIGIN_CB_SENSOR);
				}
			}
		}
	}
}

int objArmour(BASE_OBJECT *psObj, WEAPON_CLASS weaponClass)
{
	int armour = 0;
	if (psObj->type == OBJ_DROID)
	{
		armour = bodyArmour(asBodyStats + ((DROID *)psObj)->asBits[COMP_BODY], psObj->player, weaponClass);
	}
	else if (psObj->type == OBJ_STRUCTURE && weaponClass == WC_KINETIC && ((STRUCTURE *)psObj)->status != SS_BEING_BUILT)
	{
		armour = ((STRUCTURE *)psObj)->pStructureType->upgrade[psObj->player].armour;
	}
	else if (psObj->type == OBJ_STRUCTURE && weaponClass == WC_HEAT && ((STRUCTURE *)psObj)->status != SS_BEING_BUILT)
	{
		armour = ((STRUCTURE *)psObj)->pStructureType->upgrade[psObj->player].thermal;
	}
	else if (psObj->type == OBJ_FEATURE && weaponClass == WC_KINETIC)
	{
		armour = ((FEATURE *)psObj)->psStats->armourValue;
	}
	return armour;
}

/* Deals damage to an object
 * \param psObj object to deal damage to
 * \param damage amount of damage to deal
 * \param weaponClass the class of the weapon that deals the damage
 * \param weaponSubClass the subclass of the weapon that deals the damage
 * \return < 0 when the dealt damage destroys the object, > 0 when the object survives
 */
int32_t objDamage(BASE_OBJECT *psObj, unsigned damage, unsigned originalhp, WEAPON_CLASS weaponClass, WEAPON_SUBCLASS weaponSubClass, bool isDamagePerSecond, int minDamage)
{
	int level = 1;
	int armour = objArmour(psObj, weaponClass);
	const int lastHit = psObj->timeLastHit;

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

	if (psObj->type == OBJ_STRUCTURE || psObj->type == OBJ_DROID)
	{
		// Force sending messages, even if messages were turned off, since a non-synchronised script will execute here. (Aaargh!)
		bool bMultiMessagesBackup = bMultiMessages;
		bMultiMessages = bMultiPlayer;

		triggerEventAttacked(psObj, g_pProjLastAttacker, lastHit);

		bMultiMessages = bMultiMessagesBackup;
	}

	if (psObj->type == OBJ_DROID)
	{
		DROID *psDroid = (DROID *)psObj;

		// Retrieve highest, applicable, experience level
		level = getDroidEffectiveLevel(psDroid);
	}

	// Reduce damage taken by EXP_REDUCE_DAMAGE % for each experience level
	int actualDamage = (damage * (100 - EXP_REDUCE_DAMAGE * level)) / 100;

	// Apply at least the minimum damage amount
	actualDamage = MAX(actualDamage - armour, actualDamage * minDamage / 100);

	// And at least MIN_WEAPON_DAMAGE points
	actualDamage = MAX(actualDamage, MIN_WEAPON_DAMAGE);

	debug(LOG_ATTACK, "objDamage(%d): body: %d, armour: %d, basic damage: %d, actual damage: %d", psObj->id, psObj->body, armour, damage, actualDamage);

	if (isDamagePerSecond)
	{
		int deltaDamageRate = actualDamage - psObj->periodicalDamage;
		if (deltaDamageRate <= 0)
		{
			return 0;  // Did this much damage already, this tick, so don't do more.
		}
		actualDamage = gameTimeAdjustedAverage(deltaDamageRate);
		psObj->periodicalDamage += deltaDamageRate;
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
	int actualDamage, armour, level = 1;

	if (psTarget == nullptr)
	{
		return 0;    // Hard to destroy the ground. The armour on the mud is very strong and blocks all damage.
	}

	damage = calcDamage(weaponDamage(psStats, player), psStats->weaponEffect, psTarget);

	// EMP cannons do no damage, if we are one return now
	if (psStats->weaponSubClass == WSC_EMP)
	{
		return 0;
	}

	// apply game difficulty setting
	damage = modifyForDifficultyLevel(damage, psTarget->player != selectedPlayer);
	armour = objArmour(psTarget, psStats->weaponClass);

	if (psTarget->type == OBJ_DROID)
	{
		DROID *psDroid = (DROID *)psTarget;

		// Retrieve highest, applicable, experience level
		level = getDroidEffectiveLevel(psDroid);
	}
	//debug(LOG_ATTACK, "objGuessFutureDamage(%d): body %d armour %d damage: %d", psObj->id, psObj->body, armour, damage);

	// Reduce damage taken by EXP_REDUCE_DAMAGE % for each experience level
	actualDamage = (damage * (100 - EXP_REDUCE_DAMAGE * level)) / 100;

	// You always do at least a third of the experience modified damage
	actualDamage = MAX(actualDamage - armour, actualDamage * psStats->upgrade[player].minimumDamage / 100);

	// And at least MIN_WEAPON_DAMAGE points
	actualDamage = MAX(actualDamage, MIN_WEAPON_DAMAGE);

	//objTrace(psObj->id, "objGuessFutureDamage: Would penetrate %d", actualDamage);

	return actualDamage;
}
