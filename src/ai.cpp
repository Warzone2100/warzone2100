/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
 * @file ai.c
 *
 * AI update functions for the different object types.
 *
 */

#include "lib/framework/frame.h"

#include "action.h"
#include "cmddroid.h"
#include "combat.h"
#include "drive.h"
#include "mapgrid.h"
#include "map.h"
#include "projectile.h"

/* Weights used for target selection code,
 * target distance is used as 'common currency'
 */
#define	WEIGHT_DIST_TILE			13						//In points used in weaponmodifier.txt and structuremodifier.txt
#define	WEIGHT_DIST_TILE_DROID		WEIGHT_DIST_TILE		//How much weight a distance of 1 tile (128 world units) has when looking for the best nearest target
#define	WEIGHT_DIST_TILE_STRUCT		WEIGHT_DIST_TILE
#define	WEIGHT_HEALTH_DROID			(WEIGHT_DIST_TILE * 10)	//How much weight unit damage has (100% of damage is equaly weighted as 10 tiles distance)
//~100% damage should be ~8 tiles (max sensor range)
#define	WEIGHT_HEALTH_STRUCT		(WEIGHT_DIST_TILE * 7)

#define	WEIGHT_NOT_VISIBLE_F		10						//We really don't like objects we can't see

#define	WEIGHT_SERVICE_DROIDS		(WEIGHT_DIST_TILE_DROID * 5)		//We don't want them to be repairing droids or structures while we are after them
#define	WEIGHT_WEAPON_DROIDS		(WEIGHT_DIST_TILE_DROID * 4)		//We prefer to go after anything that has a gun and can hurt us
#define	WEIGHT_COMMAND_DROIDS		(WEIGHT_DIST_TILE_DROID * 6)		//Commanders get a higher priority
#define	WEIGHT_MILITARY_STRUCT		WEIGHT_DIST_TILE_STRUCT				//Droid/cyborg factories, repair facility; shouldn't have too much weight
#define	WEIGHT_WEAPON_STRUCT		WEIGHT_WEAPON_DROIDS				//Same as weapon droids (?)
#define	WEIGHT_DERRICK_STRUCT		(WEIGHT_MILITARY_STRUCT + WEIGHT_DIST_TILE_STRUCT * 4)	//Even if it's 4 tiles further away than defenses we still choose it

#define	WEIGHT_STRUCT_NOTBUILT_F	8						//Humans won't fool us anymore!

#define OLD_TARGET_THRESHOLD		(WEIGHT_DIST_TILE * 4)	//it only makes sense to switch target if new one is 4+ tiles closer

#define	EMP_DISABLED_PENALTY_F		10								//EMP shouldn't attack emped targets again
#define	EMP_STRUCT_PENALTY_F		(EMP_DISABLED_PENALTY_F * 2)	//EMP don't attack structures, should be bigger than EMP_DISABLED_PENALTY_F

#define TOO_CLOSE_PENALTY_F             20

#define TARGET_DOOMED_PENALTY_F		10	// Targets that have a lot of damage incoming are less attractive
#define TARGET_DOOMED_SLOW_RELOAD_T	21	// Weapon ROF threshold for above penalty. per minute.

//Some weights for the units attached to a commander
#define	WEIGHT_CMD_RANK				(WEIGHT_DIST_TILE * 4)			//A single rank is as important as 4 tiles distance
#define	WEIGHT_CMD_SAME_TARGET		WEIGHT_DIST_TILE				//Don't want this to be too high, since a commander can have many units assigned

uint8_t alliances[MAX_PLAYER_SLOTS][MAX_PLAYER_SLOTS];

/// A bitfield of vision sharing in alliances, for quick manipulation of vision information
PlayerMask alliancebits[MAX_PLAYER_SLOTS];

/// A bitfield for the satellite uplink
PlayerMask satuplinkbits;

static int aiDroidRange(DROID *psDroid, int weapon_slot)
{
	int32_t longRange;
	if (psDroid->droidType == DROID_SENSOR)
	{
		longRange = objSensorRange(psDroid);
	}
	else if (psDroid->numWeaps == 0 || psDroid->asWeaps[0].nStat == 0)
	{
		// Can't attack without a weapon
		return 0;
	}
	else
	{
		WEAPON_STATS *psWStats = psDroid->asWeaps[weapon_slot].nStat + asWeaponStats;
		longRange = proj_GetLongRange(psWStats, psDroid->player);
	}

	return longRange;
}

// see if a structure has the range to fire on a target
static bool aiStructHasRange(STRUCTURE *psStruct, BASE_OBJECT *psTarget, int weapon_slot)
{
	if (psStruct->numWeaps == 0 || psStruct->asWeaps[0].nStat == 0)
	{
		// Can't attack without a weapon
		return false;
	}

	WEAPON_STATS *psWStats = psStruct->asWeaps[weapon_slot].nStat + asWeaponStats;

	int longRange = proj_GetLongRange(psWStats, psStruct->player);
	return objPosDiffSq(psStruct, psTarget) < longRange * longRange && lineOfFire(psStruct, psTarget, weapon_slot, true);
}

static bool aiDroidHasRange(DROID *psDroid, BASE_OBJECT *psTarget, int weapon_slot)
{
	int32_t longRange = aiDroidRange(psDroid, weapon_slot);

	return objPosDiffSq(psDroid, psTarget) < longRange * longRange;
}

static bool aiObjHasRange(BASE_OBJECT *psObj, BASE_OBJECT *psTarget, int weapon_slot)
{
	if (psObj->type == OBJ_DROID)
	{
		return aiDroidHasRange((DROID *)psObj, psTarget, weapon_slot);
	}
	else if (psObj->type == OBJ_STRUCTURE)
	{
		return aiStructHasRange((STRUCTURE *)psObj, psTarget, weapon_slot);
	}
	return false;
}

/* Initialise the AI system */
bool aiInitialise(void)
{
	SDWORD		i, j;

	for (i = 0; i < MAX_PLAYER_SLOTS; i++)
	{
		alliancebits[i] = 0;
		for (j = 0; j < MAX_PLAYER_SLOTS; j++)
		{
			bool valid = (i == j && i < MAX_PLAYERS);

			alliances[i][j] = valid ? ALLIANCE_FORMED : ALLIANCE_BROKEN;
			alliancebits[i] |= valid << j;
		}
	}
	satuplinkbits = 0;

	return true;
}

/* Shutdown the AI system */
bool aiShutdown(void)
{
	return true;
}

/** Search the global list of sensors for a possible target for psObj. */
static BASE_OBJECT *aiSearchSensorTargets(BASE_OBJECT *psObj, int weapon_slot, WEAPON_STATS *psWStats, UWORD *targetOrigin)
{
	int		longRange = proj_GetLongRange(psWStats, psObj->player);
	int		tarDist = longRange * longRange;
	bool		foundCB = false;
	int		minDist = psWStats->upgrade[psObj->player].minRange * psWStats->upgrade[psObj->player].minRange;
	BASE_OBJECT	*psTarget = NULL;

	if (targetOrigin)
	{
		*targetOrigin = ORIGIN_UNKNOWN;
	}

	for (BASE_OBJECT *psSensor = apsSensorList[0]; psSensor; psSensor = psSensor->psNextFunc)
	{
		BASE_OBJECT	*psTemp = NULL;
		bool		isCB = false;
		bool		isRD = false;

		if (!aiCheckAlliances(psSensor->player, psObj->player))
		{
			continue;
		}
		else if (psSensor->type == OBJ_DROID)
		{
			DROID		*psDroid = (DROID *)psSensor;

			ASSERT_OR_RETURN(NULL, psDroid->droidType == DROID_SENSOR, "A non-sensor droid in a sensor list is non-sense");
			// Skip non-observing droids.
			if (psDroid->action != DACTION_OBSERVE)
			{
				continue;
			}
			psTemp = psDroid->psActionTarget[0];
			isCB = cbSensorDroid(psDroid);
			isRD = objRadarDetector((BASE_OBJECT *)psDroid);
		}
		else if (psSensor->type == OBJ_STRUCTURE)
		{
			STRUCTURE	*psCStruct = (STRUCTURE *)psSensor;

			// skip incomplete structures
			if (psCStruct->status != SS_BUILT)
			{
				continue;
			}
			psTemp = psCStruct->psTarget[0];
			isCB = structCBSensor(psCStruct);
			isRD = objRadarDetector((BASE_OBJECT *)psCStruct);
		}
		if (!psTemp || psTemp->died || aiObjectIsProbablyDoomed(psTemp, false) || !validTarget(psObj, psTemp, 0) || aiCheckAlliances(psTemp->player, psObj->player))
		{
			continue;
		}
		int distSq = objPosDiffSq(psTemp->pos, psObj->pos);
		// Need to be in range, prefer closer targets or CB targets
		if ((isCB > foundCB || (isCB == foundCB && distSq < tarDist)) && distSq > minDist)
		{
			if (aiObjHasRange(psObj, psTemp, weapon_slot) && visibleObject(psSensor, psTemp, false))
			{
				tarDist = distSq;
				psTarget = psTemp;
				if (targetOrigin)
				{
					*targetOrigin = ORIGIN_SENSOR;
				}

				if (isCB)
				{
					if (targetOrigin)
					{
						*targetOrigin = ORIGIN_CB_SENSOR;
					}
					foundCB = true;  // got CB target, drop everything and shoot!
				}
				else if (isRD)
				{
					if (targetOrigin)
					{
						*targetOrigin = ORIGIN_RADAR_DETECTOR;
					}
				}
			}
		}
	}
	return psTarget;
}

/* Calculates attack priority for a certain target */
static SDWORD targetAttackWeight(BASE_OBJECT *psTarget, BASE_OBJECT *psAttacker, SDWORD weapon_slot)
{
	SDWORD			targetTypeBonus = 0, damageRatio = 0, attackWeight = 0, noTarget = -1;
	UDWORD			weaponSlot;
	DROID			*targetDroid = NULL, *psAttackerDroid = NULL, *psGroupDroid, *psDroid;
	STRUCTURE		*targetStructure = NULL;
	WEAPON_EFFECT	weaponEffect;
	WEAPON_STATS	*attackerWeapon;
	bool			bEmpWeap = false, bCmdAttached = false, bTargetingCmd = false, bDirect = false;

	if (psTarget == NULL || psAttacker == NULL || psTarget->died)
	{
		return noTarget;
	}
	ASSERT(psTarget != psAttacker, "targetAttackWeight: Wanted to evaluate the worth of attacking ourselves...");

	targetTypeBonus = 0;			//Sensors/ecm droids, non-military structures get lower priority

	/* Get attacker weapon effect */
	if (psAttacker->type == OBJ_DROID)
	{
		psAttackerDroid = (DROID *)psAttacker;

		attackerWeapon = (WEAPON_STATS *)(asWeaponStats + psAttackerDroid->asWeaps[weapon_slot].nStat);

		//check if this droid is assigned to a commander
		bCmdAttached = hasCommander(psAttackerDroid);

		//find out if current target is targeting our commander
		if (bCmdAttached)
		{
			if (psTarget->type == OBJ_DROID)
			{
				psDroid = (DROID *)psTarget;

				//go through all enemy weapon slots
				for (weaponSlot = 0; !bTargetingCmd &&
				     weaponSlot < ((DROID *)psTarget)->numWeaps; weaponSlot++)
				{
					//see if this weapon is targeting our commander
					if (psDroid->psActionTarget[weaponSlot] == (BASE_OBJECT *)psAttackerDroid->psGroup->psCommander)
					{
						bTargetingCmd = true;
					}
				}
			}
			else
			{
				if (psTarget->type == OBJ_STRUCTURE)
				{
					//go through all enemy weapons
					for (weaponSlot = 0; !bTargetingCmd && weaponSlot < ((STRUCTURE *)psTarget)->numWeaps; weaponSlot++)
					{
						if (((STRUCTURE *)psTarget)->psTarget[weaponSlot] ==
						    (BASE_OBJECT *)psAttackerDroid->psGroup->psCommander)
						{
							bTargetingCmd = true;
						}
					}
				}
			}
		}
	}
	else if (psAttacker->type == OBJ_STRUCTURE)
	{
		attackerWeapon = ((WEAPON_STATS *)(asWeaponStats + ((STRUCTURE *)psAttacker)->asWeaps[weapon_slot].nStat));
	}
	else	/* feature */
	{
		ASSERT(!"invalid attacker object type", "targetAttackWeight: Invalid attacker object type");
		return noTarget;
	}

	bDirect = proj_Direct(attackerWeapon);
	if (psAttacker->type == OBJ_DROID && psAttackerDroid->droidType == DROID_SENSOR)
	{
		// Sensors are considered a direct weapon,
		// but for computing expected damage it makes more sense to use indirect damage
		bDirect = false;
	}

	//Get weapon effect
	weaponEffect = attackerWeapon->weaponEffect;

	//See if attacker is using an EMP weapon
	bEmpWeap = (attackerWeapon->weaponSubClass == WSC_EMP);

	int dist = iHypot(removeZ(psAttacker->pos - psTarget->pos));
	bool tooClose = dist <= attackerWeapon->upgrade[psAttacker->player].minRange;
	if (tooClose)
	{
		dist = objSensorRange(psAttacker);  // If object is too close to fire at, consider it to be at maximum range.
	}

	/* Calculate attack weight */
	if (psTarget->type == OBJ_DROID)
	{
		targetDroid = (DROID *)psTarget;

		if (targetDroid->died)
		{
			debug(LOG_NEVER, "Target droid is dead, skipping invalid droid.\n");
			return noTarget;
		}

		/* Calculate damage this target suffered */
		if (targetDroid->originalBody == 0) // FIXME Somewhere we get 0HP droids from
		{
			damageRatio = 0;
			debug(LOG_ERROR, "targetAttackWeight: 0HP droid detected!");
			debug(LOG_ERROR, "  Type: %i Name: \"%s\" Owner: %i \"%s\")",
			      targetDroid->droidType, targetDroid->aName, targetDroid->player, getPlayerName(targetDroid->player));
		}
		else
		{
			damageRatio = 100 - 100 * targetDroid->body / targetDroid->originalBody;
		}
		assert(targetDroid->originalBody != 0); // Assert later so we get the info from above

		/* See if this type of a droid should be prioritized */
		switch (targetDroid->droidType)
		{
		case DROID_SENSOR:
		case DROID_ECM:
		case DROID_PERSON:
		case DROID_TRANSPORTER:
		case DROID_SUPERTRANSPORTER:
		case DROID_DEFAULT:
		case DROID_ANY:
			break;

		case DROID_CYBORG:
		case DROID_WEAPON:
		case DROID_CYBORG_SUPER:
			targetTypeBonus = WEIGHT_WEAPON_DROIDS;
			break;

		case DROID_COMMAND:
			targetTypeBonus = WEIGHT_COMMAND_DROIDS;
			break;

		case DROID_CONSTRUCT:
		case DROID_REPAIR:
		case DROID_CYBORG_CONSTRUCT:
		case DROID_CYBORG_REPAIR:
			targetTypeBonus = WEIGHT_SERVICE_DROIDS;
			break;
		}

		/* Now calculate the overall weight */
		attackWeight = asWeaponModifier[weaponEffect][(asPropulsionStats + targetDroid->asBits[COMP_PROPULSION])->propulsionType] // Our weapon's effect against target
		               + asWeaponModifierBody[weaponEffect][(asBodyStats + targetDroid->asBits[COMP_BODY])->size]
		               + WEIGHT_DIST_TILE_DROID * objSensorRange(psAttacker) / TILE_UNITS
		               - WEIGHT_DIST_TILE_DROID * dist / TILE_UNITS // farther droids are less attractive
		               + WEIGHT_HEALTH_DROID * damageRatio / 100 // we prefer damaged droids
		               + targetTypeBonus; // some droid types have higher priority

		/* If attacking with EMP try to avoid targets that were already "EMPed" */
		if (bEmpWeap &&
		    (targetDroid->lastHitWeapon == WSC_EMP) &&
		    ((gameTime - targetDroid->timeLastHit) < EMP_DISABLE_TIME))		//target still disabled
		{
			attackWeight /= EMP_DISABLED_PENALTY_F;
		}
	}
	else if (psTarget->type == OBJ_STRUCTURE)
	{
		targetStructure = (STRUCTURE *)psTarget;

		/* Calculate damage this target suffered */
		damageRatio = 100 - 100 * targetStructure->body / structureBody(targetStructure);

		/* See if this type of a structure should be prioritized */
		switch (targetStructure->pStructureType->type)
		{
		case REF_DEFENSE:
			targetTypeBonus = WEIGHT_WEAPON_STRUCT;
			break;

		case REF_RESOURCE_EXTRACTOR:
			targetTypeBonus = WEIGHT_DERRICK_STRUCT;
			break;

		case REF_FACTORY:
		case REF_CYBORG_FACTORY:
		case REF_REPAIR_FACILITY:
			targetTypeBonus = WEIGHT_MILITARY_STRUCT;
			break;
		default:
			break;
		}

		/* Now calculate the overall weight */
		attackWeight = asStructStrengthModifier[weaponEffect][targetStructure->pStructureType->strength] // Our weapon's effect against target
		               + WEIGHT_DIST_TILE_STRUCT * objSensorRange(psAttacker) / TILE_UNITS
		               - WEIGHT_DIST_TILE_STRUCT * dist / TILE_UNITS // farther structs are less attractive
		               + WEIGHT_HEALTH_STRUCT * damageRatio / 100 // we prefer damaged structures
		               + targetTypeBonus; // some structure types have higher priority

		/* Go for unfinished structures only if nothing else found (same for non-visible structures) */
		if (targetStructure->status != SS_BUILT)		//a decoy?
		{
			attackWeight /= WEIGHT_STRUCT_NOTBUILT_F;
		}

		/* EMP should only attack structures if no enemy droids are around */
		if (bEmpWeap)
		{
			attackWeight /= EMP_STRUCT_PENALTY_F;
		}
	}
	else	//a feature
	{
		return 1;
	}

	/* We prefer objects we can see and can attack immediately */
	if (!visibleObject((BASE_OBJECT *)psAttacker, psTarget, true))
	{
		attackWeight /= WEIGHT_NOT_VISIBLE_F;
	}

	if (tooClose)
	{
		attackWeight /= TOO_CLOSE_PENALTY_F;
	}

	/* Penalty for units that are already considered doomed (but the missile might miss!) */
	if (aiObjectIsProbablyDoomed(psTarget, bDirect))
	{
		/* indirect firing units have slow reload times, so give the target a chance to die,
		 * and give a different unit a chance to get in range, too. */
		if (weaponROF(attackerWeapon, psAttacker->player) < TARGET_DOOMED_SLOW_RELOAD_T)
		{
			debug(LOG_NEVER, "Not killing unit - doomed. My ROF: %i (%s)", weaponROF(attackerWeapon, psAttacker->player), getName(attackerWeapon));
			return noTarget;
		}
		attackWeight /= TARGET_DOOMED_PENALTY_F;
	}

	/* Commander-related criterias */
	if (bCmdAttached)	//attached to a commander and don't have a target assigned by some order
	{
		ASSERT(psAttackerDroid->psGroup->psCommander != NULL, "Commander is NULL");

		//if commander is being targeted by our target, try to defend the commander
		if (bTargetingCmd)
		{
			attackWeight += WEIGHT_CMD_RANK * (1 + getDroidLevel(psAttackerDroid->psGroup->psCommander));
		}

		//fire support - go through all droids assigned to the commander
		for (psGroupDroid = psAttackerDroid->psGroup->psList; psGroupDroid; psGroupDroid = psGroupDroid->psGrpNext)
		{
			for (weaponSlot = 0; weaponSlot < psGroupDroid->numWeaps; weaponSlot++)
			{
				//see if this droid is currently targeting current target
				if (psGroupDroid->order.psObj == psTarget ||
				    psGroupDroid->psActionTarget[weaponSlot] == psTarget)
				{
					//we prefer targets that are already targeted and hence will be destroyed faster
					attackWeight += WEIGHT_CMD_SAME_TARGET;
				}
			}
		}
	}

	return std::max<int>(1, attackWeight);
}


// Find the best nearest target for a droid.
// If extraRange is higher than zero, then this is the range it accepts for movement to target.
// Returns integer representing target priority, -1 if failed
int aiBestNearestTarget(DROID *psDroid, BASE_OBJECT **ppsObj, int weapon_slot, int extraRange)
{
	SDWORD				bestMod = 0, newMod, failure = -1;
	BASE_OBJECT                     *psTarget = NULL, *bestTarget = NULL, *tempTarget;
	bool				electronic = false;
	STRUCTURE			*targetStructure;
	WEAPON_EFFECT			weaponEffect;
	UWORD				tmpOrigin = ORIGIN_UNKNOWN;

	//don't bother looking if empty vtol droid
	if (vtolEmpty(psDroid))
	{
		return failure;
	}

	/* Return if have no weapons */
	// The ai orders a non-combat droid to patrol = crash without it...
	if ((psDroid->asWeaps[0].nStat == 0 || psDroid->numWeaps == 0) && psDroid->droidType != DROID_SENSOR)
	{
		return failure;
	}
	// Check if we have a CB target to begin with
	if (!proj_Direct(asWeaponStats + psDroid->asWeaps[weapon_slot].nStat))
	{
		WEAPON_STATS *psWStats = psDroid->asWeaps[weapon_slot].nStat + asWeaponStats;

		bestTarget = aiSearchSensorTargets((BASE_OBJECT *)psDroid, weapon_slot, psWStats, &tmpOrigin);
		bestMod = targetAttackWeight(bestTarget, (BASE_OBJECT *)psDroid, weapon_slot);
	}

	weaponEffect = (asWeaponStats + psDroid->asWeaps[weapon_slot].nStat)->weaponEffect;

	electronic = electronicDroid(psDroid);

	// Range was previously 9*TILE_UNITS. Increasing this doesn't seem to help much, though. Not sure why.
	int droidRange = std::min(aiDroidRange(psDroid, weapon_slot) + extraRange, objSensorRange(psDroid) + 6 * TILE_UNITS);

	static GridList gridList;  // static to avoid allocations.
	gridList = gridStartIterate(psDroid->pos.x, psDroid->pos.y, droidRange);
	for (GridIterator gi = gridList.begin(); gi != gridList.end(); ++gi)
	{
		BASE_OBJECT *friendlyObj = NULL;
		BASE_OBJECT *targetInQuestion = *gi;

		/* This is a friendly unit, check if we can reuse its target */
		if (aiCheckAlliances(targetInQuestion->player, psDroid->player))
		{
			friendlyObj = targetInQuestion;
			targetInQuestion = NULL;

			/* Can we see what it is doing? */
			if (friendlyObj->visible[psDroid->player] == UBYTE_MAX)
			{
				if (friendlyObj->type == OBJ_DROID)
				{
					DROID	*friendlyDroid = (DROID *)friendlyObj;

					/* See if friendly droid has a target */
					tempTarget = friendlyDroid->psActionTarget[0];
					if (tempTarget && !tempTarget->died)
					{
						//make sure a weapon droid is targeting it
						if (friendlyDroid->numWeaps > 0)
						{
							// make sure this target wasn't assigned explicitly to this droid
							if (friendlyDroid->order.type != DORDER_ATTACK)
							{
								targetInQuestion = tempTarget;  //consider this target
							}
						}
					}
				}
				else if (friendlyObj->type == OBJ_STRUCTURE)
				{
					tempTarget = ((STRUCTURE *)friendlyObj)->psTarget[0];
					if (tempTarget && !tempTarget->died)
					{
						targetInQuestion = tempTarget;
					}
				}
			}
		}

		if (targetInQuestion != NULL
		    && targetInQuestion != psDroid  // in case friendly unit had me as target
		    && (targetInQuestion->type == OBJ_DROID || targetInQuestion->type == OBJ_STRUCTURE || targetInQuestion->type == OBJ_FEATURE)
		    && targetInQuestion->visible[psDroid->player] == UBYTE_MAX
		    && !aiCheckAlliances(targetInQuestion->player, psDroid->player)
		    && validTarget(psDroid, targetInQuestion, weapon_slot)
		    && objPosDiffSq(psDroid, targetInQuestion) < droidRange * droidRange)
		{
			if (targetInQuestion->type == OBJ_DROID)
			{
				// in multiPlayer - don't attack Transporters with EW
				if (bMultiPlayer)
				{
					// if not electronic then valid target
					if (!electronic
					    || (electronic
					        && !isTransporter((DROID *)targetInQuestion)))
					{
						//only a valid target if NOT a transporter
						psTarget = targetInQuestion;
					}
				}
				else
				{
					psTarget = targetInQuestion;
				}
			}
			else if (targetInQuestion->type == OBJ_STRUCTURE)
			{
				STRUCTURE *psStruct = (STRUCTURE *)targetInQuestion;

				if (electronic)
				{
					/* don't want to target structures with resistance of zero if using electronic warfare */
					if (validStructResistance((STRUCTURE *)targetInQuestion))
					{
						psTarget = targetInQuestion;
					}
				}
				else if (psStruct->asWeaps[0].nStat > 0)
				{
					// structure with weapons - go for this
					psTarget = targetInQuestion;
				}
				else if ((psStruct->pStructureType->type != REF_WALL && psStruct->pStructureType->type != REF_WALLCORNER)
				         || driveModeActive() || (bMultiPlayer && !isHumanPlayer(psDroid->player)))
				{
					psTarget = targetInQuestion;
				}
			}
			else if (targetInQuestion->type == OBJ_FEATURE
			         && psDroid->lastFrustratedTime > 0
			         && gameTime - psDroid->lastFrustratedTime < FRUSTRATED_TIME
			         && ((FEATURE *)targetInQuestion)->psStats->damageable
			         && psDroid->player != scavengerPlayer())  // hack to avoid scavs blowing up their nice feature walls
			{
				psTarget = targetInQuestion;
				objTrace(psDroid->id, "considering shooting at %s in frustration", objInfo(targetInQuestion));
			}

			/* Check if our weapon is most effective against this object */
			if (psTarget != NULL && psTarget == targetInQuestion)		//was assigned?
			{
				newMod = targetAttackWeight(psTarget, (BASE_OBJECT *)psDroid, weapon_slot);

				/* Remember this one if it's our best target so far */
				if (newMod >= 0 && (newMod > bestMod || bestTarget == NULL))
				{
					bestMod = newMod;
					tmpOrigin = ORIGIN_ALLY;
					bestTarget = psTarget;
				}
			}
		}
	}

	if (bestTarget)
	{
		ASSERT(!bestTarget->died, "AI gave us a target that is already dead.");
		targetStructure = visGetBlockingWall((BASE_OBJECT *)psDroid, bestTarget);

		/* See if target is blocked by a wall; only affects direct weapons */
		if (proj_Direct(asWeaponStats + psDroid->asWeaps[weapon_slot].nStat)
		    && targetStructure)
		{
			//are we any good against walls?
			if (asStructStrengthModifier[weaponEffect][targetStructure->pStructureType->strength] >= 100)		//can attack atleast with default strength
			{
				bestTarget = (BASE_OBJECT *)targetStructure;			//attack wall
			}
		}

		*ppsObj = bestTarget;
		return bestMod;
	}

	return failure;
}

// Are there a lot of bullets heading towards the droid?
static bool aiDroidIsProbablyDoomed(DROID *psDroid, bool isDirect)
{
	if (isDirect)
	{
		return psDroid->expectedDamageDirect > psDroid->body
		      && psDroid->expectedDamageDirect - psDroid->body > psDroid->body / 5; // Doomed if projectiles will damage 120% of remaining body points.
	}
	else
	{
		return psDroid->expectedDamageIndirect > psDroid->body
		      && psDroid->expectedDamageIndirect - psDroid->body > psDroid->body / 5; // Doomed if projectiles will damage 120% of remaining body points.
	}
}

// Are there a lot of bullets heading towards the structure?
static bool aiStructureIsProbablyDoomed(STRUCTURE *psStructure)
{
	return psStructure->expectedDamage > psStructure->body
	       && psStructure->expectedDamage - psStructure->body > psStructure->body / 15; // Doomed if projectiles will damage 106.6666666667% of remaining body points.
}

// Are there a lot of bullets heading towards the object?
bool aiObjectIsProbablyDoomed(BASE_OBJECT *psObject, bool isDirect)
{
	if (psObject->died)
	{
		return true;    // Was definitely doomed.
	}

	switch (psObject->type)
	{
	case OBJ_DROID:
		return aiDroidIsProbablyDoomed((DROID *)psObject, isDirect);
	case OBJ_STRUCTURE:
		return aiStructureIsProbablyDoomed((STRUCTURE *)psObject);
	default:
		return false;
	}
}

// Update the expected damage of the object.
void aiObjectAddExpectedDamage(BASE_OBJECT *psObject, SDWORD damage, bool isDirect)
{
	if (psObject == NULL)
	{
		return;    // Hard to destroy the ground.
	}

	switch (psObject->type)
	{
	case OBJ_DROID:
		if (isDirect)
		{
			((DROID *)psObject)->expectedDamageDirect += damage;
			ASSERT((SDWORD)((DROID *)psObject)->expectedDamageDirect >= 0, "aiObjectAddExpectedDamage: Negative amount of projectiles heading towards droid.");
		}
		else
		{
			((DROID *)psObject)->expectedDamageIndirect += damage;
			ASSERT((SDWORD)((DROID *)psObject)->expectedDamageIndirect >= 0, "aiObjectAddExpectedDamage: Negative amount of projectiles heading towards droid.");
		}
		break;
	case OBJ_STRUCTURE:
		((STRUCTURE *)psObject)->expectedDamage += damage;
		ASSERT((SDWORD)((STRUCTURE *)psObject)->expectedDamage >= 0, "aiObjectAddExpectedDamage: Negative amount of projectiles heading towards droid.");
		break;
	default:
		break;
	}
}

// see if an object is a wall
static bool aiObjIsWall(BASE_OBJECT *psObj)
{
	if (psObj->type != OBJ_STRUCTURE)
	{
		return false;
	}

	if (((STRUCTURE *)psObj)->pStructureType->type != REF_WALL &&
	    ((STRUCTURE *)psObj)->pStructureType->type != REF_WALLCORNER)
	{
		return false;
	}

	return true;
}


/* See if there is a target in range */
bool aiChooseTarget(BASE_OBJECT *psObj, BASE_OBJECT **ppsTarget, int weapon_slot, bool bUpdateTarget, UWORD *targetOrigin)
{
	BASE_OBJECT		*psTarget = NULL;
	DROID			*psCommander;
	SDWORD			curTargetWeight = -1;
	UWORD 			tmpOrigin = ORIGIN_UNKNOWN;

	if (targetOrigin)
	{
		*targetOrigin = ORIGIN_UNKNOWN;
	}

	switch (psObj->type)
	{
	case OBJ_DROID:
		if (((DROID *)psObj)->asWeaps[weapon_slot].nStat == 0)
		{
			return false;
		}
		if (((DROID *)psObj)->asWeaps[0].nStat == 0 &&
		    ((DROID *)psObj)->droidType != DROID_SENSOR)
		{
			return false;	// Can't target without a weapon or sensor
		}
		break;
	case OBJ_STRUCTURE:
		if (((STRUCTURE *)psObj)->numWeaps == 0 || ((STRUCTURE *)psObj)->asWeaps[0].nStat == 0)
		{
			// Can't attack without a weapon
			return false;
		}
		break;
	default:
		break;
	}

	/* See if there is a something in range */
	if (psObj->type == OBJ_DROID)
	{
		BASE_OBJECT *psCurrTarget = ((DROID *)psObj)->psActionTarget[0];

		/* find a new target */
		int newTargetWeight = aiBestNearestTarget((DROID *)psObj, &psTarget, weapon_slot);

		/* Calculate weight of the current target if updating; but take care not to target
		 * ourselves... */
		if (bUpdateTarget && psCurrTarget != psObj)
		{
			curTargetWeight = targetAttackWeight(psCurrTarget, psObj, weapon_slot);
		}

		if (newTargetWeight >= 0		// found a new target
		    && (!bUpdateTarget			// choosing a new target, don't care if current one is better
		        || curTargetWeight <= 0		// attacker had no valid target, use new one
		        || newTargetWeight > curTargetWeight + OLD_TARGET_THRESHOLD)	// updating and new target is better
		    && validTarget(psObj, psTarget, weapon_slot)
		    && aiDroidHasRange((DROID *)psObj, psTarget, weapon_slot))
		{
			ASSERT(!isDead(psTarget), "Droid found a dead target!");
			*ppsTarget = psTarget;
			return true;
		}
	}
	else if (psObj->type == OBJ_STRUCTURE)
	{
		WEAPON_STATS	*psWStats = NULL;
		bool	bCommanderBlock = false;

		ASSERT(((STRUCTURE *)psObj)->asWeaps[weapon_slot].nStat > 0, "no weapons on structure");

		psWStats = ((STRUCTURE *)psObj)->asWeaps[weapon_slot].nStat + asWeaponStats;
		int longRange = proj_GetLongRange(psWStats, psObj->player);

		// see if there is a target from the command droids
		psTarget = NULL;
		psCommander = cmdDroidGetDesignator(psObj->player);
		if (!proj_Direct(psWStats) && (psCommander != NULL) &&
		    aiStructHasRange((STRUCTURE *)psObj, (BASE_OBJECT *)psCommander, weapon_slot))
		{
			// there is a commander that can fire designate for this structure
			// set bCommanderBlock so that the structure does not fire until the commander
			// has a target - (slow firing weapons will not be ready to fire otherwise).
			bCommanderBlock = true;

			// I do believe this will never happen, check for yourself :-)
			debug(LOG_NEVER, "Commander %d is good enough for fire designation", psCommander->id);

			if (psCommander->action == DACTION_ATTACK
			    && psCommander->psActionTarget[0] != NULL
			    && !psCommander->psActionTarget[0]->died)
			{
				// the commander has a target to fire on
				if (aiStructHasRange((STRUCTURE *)psObj, psCommander->psActionTarget[0], weapon_slot))
				{
					// target in range - fire on it
					tmpOrigin = ORIGIN_COMMANDER;
					psTarget = psCommander->psActionTarget[0];
				}
				else
				{
					// target out of range - release the commander block
					bCommanderBlock = false;
				}
			}
		}

		// indirect fire structures use sensor towers first
		if (psTarget == NULL && !bCommanderBlock && !proj_Direct(psWStats))
		{
			psTarget = aiSearchSensorTargets(psObj, weapon_slot, psWStats, &tmpOrigin);
		}

		if (psTarget == NULL && !bCommanderBlock)
		{
			int targetValue = -1;
			int tarDist = INT32_MAX;
			int srange = longRange;

			if (!proj_Direct(psWStats) && srange > objSensorRange(psObj))
			{
				// search radius of indirect weapons limited by their sight, unless they use
				// external sensors to provide fire designation
				srange = objSensorRange(psObj);
			}

			static GridList gridList;  // static to avoid allocations.
			gridList = gridStartIterate(psObj->pos.x, psObj->pos.y, srange);
			for (GridIterator gi = gridList.begin(); gi != gridList.end(); ++gi)
			{
				BASE_OBJECT *psCurr = *gi;
				/* Check that it is a valid target */
				if (psCurr->type != OBJ_FEATURE && !psCurr->died
				    && !aiCheckAlliances(psCurr->player, psObj->player)
				    && validTarget(psObj, psCurr, weapon_slot) && psCurr->visible[psObj->player] == UBYTE_MAX
				    && aiStructHasRange((STRUCTURE *)psObj, psCurr, weapon_slot))
				{
					int newTargetValue = targetAttackWeight(psCurr, psObj, weapon_slot);
					// See if in sensor range and visible
					int distSq = objPosDiffSq(psCurr->pos, psObj->pos);
					if (newTargetValue < targetValue || (newTargetValue == targetValue && distSq >= tarDist))
					{
						continue;
					}

					tmpOrigin = ORIGIN_VISUAL;
					psTarget = psCurr;
					tarDist = distSq;
					targetValue = newTargetValue;
				}
			}
		}

		if (psTarget)
		{
			ASSERT(!psTarget->died, "aiChooseTarget: Structure found a dead target!");
			if (targetOrigin)
			{
				*targetOrigin = tmpOrigin;
			}
			*ppsTarget = psTarget;
			return true;
		}
	}

	return false;
}


/* See if there is a target in range for Sensor objects*/
bool aiChooseSensorTarget(BASE_OBJECT *psObj, BASE_OBJECT **ppsTarget)
{
	int		sensorRange = objSensorRange(psObj);
	unsigned int	radSquared = sensorRange * sensorRange;
	bool		radarDetector = objRadarDetector(psObj);

	if (!objActiveRadar(psObj) && !radarDetector)
	{
		ASSERT(false, "Only to be used for sensor turrets!");
		return false;
	}

	/* See if there is something in range */
	if (psObj->type == OBJ_DROID)
	{
		BASE_OBJECT	*psTarget = NULL;

		if (aiBestNearestTarget((DROID *)psObj, &psTarget, 0) >= 0)
		{
			/* See if in sensor range */
			const int xdiff = psTarget->pos.x - psObj->pos.x;
			const int ydiff = psTarget->pos.y - psObj->pos.y;
			const unsigned int distSq = xdiff * xdiff + ydiff * ydiff;

			// I do believe this will never happen, check for yourself :-)
			debug(LOG_NEVER, "Sensor droid(%d) found possible target(%d)!!!", psObj->id, psTarget->id);

			if (distSq < radSquared)
			{
				*ppsTarget = psTarget;
				return true;
			}
		}
	}
	else	// structure
	{
		BASE_OBJECT    *psTemp = NULL;
		int		tarDist = SDWORD_MAX;

		static GridList gridList;  // static to avoid allocations.
		gridList = gridStartIterate(psObj->pos.x, psObj->pos.y, objSensorRange(psObj));
		for (GridIterator gi = gridList.begin(); gi != gridList.end(); ++gi)
		{
			BASE_OBJECT *psCurr = *gi;
			// Don't target features or doomed/dead objects
			if (psCurr->type != OBJ_FEATURE && !psCurr->died && !aiObjectIsProbablyDoomed(psCurr, false))
			{
				if (!aiCheckAlliances(psCurr->player, psObj->player) && !aiObjIsWall(psCurr))
				{
					// See if in sensor range and visible
					const int xdiff = psCurr->pos.x - psObj->pos.x;
					const int ydiff = psCurr->pos.y - psObj->pos.y;
					const unsigned int distSq = xdiff * xdiff + ydiff * ydiff;

					if (distSq < radSquared && psCurr->visible[psObj->player] == UBYTE_MAX && distSq < tarDist)
					{
						psTemp = psCurr;
						tarDist = distSq;
					}
				}
			}
		}

		if (psTemp)
		{
			ASSERT(!psTemp->died, "aiChooseSensorTarget gave us a dead target");
			*ppsTarget = psTemp;
			return true;
		}
	}

	return false;
}

/* Make droid/structure look for a better target */
static bool updateAttackTarget(BASE_OBJECT *psAttacker, SDWORD weapon_slot)
{
	BASE_OBJECT		*psBetterTarget = NULL;
	UWORD			tmpOrigin = ORIGIN_UNKNOWN;

	if (aiChooseTarget(psAttacker, &psBetterTarget, weapon_slot, true, &tmpOrigin))	//update target
	{
		if (psAttacker->type == OBJ_DROID)
		{
			DROID *psDroid = (DROID *)psAttacker;

			if ((orderState(psDroid, DORDER_NONE) ||
			     orderState(psDroid, DORDER_GUARD) ||
			     orderState(psDroid, DORDER_ATTACKTARGET)) &&
			    weapon_slot == 0)
			{
				actionDroid((DROID *)psAttacker, DACTION_ATTACK, psBetterTarget);
			}
			else	//can't override current order
			{
				setDroidActionTarget(psDroid, psBetterTarget, weapon_slot);
			}
		}
		else if (psAttacker->type == OBJ_STRUCTURE)
		{
			STRUCTURE *psBuilding = (STRUCTURE *)psAttacker;

			setStructureTarget(psBuilding, psBetterTarget, weapon_slot, tmpOrigin);
		}

		return true;
	}

	return false;
}

/* Do the AI for a droid */
void aiUpdateDroid(DROID *psDroid)
{
	BASE_OBJECT	*psTarget;
	bool		lookForTarget, updateTarget;

	ASSERT(psDroid != NULL, "Invalid droid pointer");
	if (!psDroid || isDead((BASE_OBJECT *)psDroid))
	{
		return;
	}

	lookForTarget = false;
	updateTarget = false;

	// look for a target if doing nothing
	if (orderState(psDroid, DORDER_NONE) ||
	    orderState(psDroid, DORDER_GUARD) ||
	    orderState(psDroid, DORDER_HOLD))
	{
		lookForTarget = true;
	}
	// but do not choose another target if doing anything while guarding
	// exception for sensors, to allow re-targetting when target is doomed
	if (orderState(psDroid, DORDER_GUARD) && psDroid->action != DACTION_NONE && psDroid->droidType != DROID_SENSOR)
	{
		lookForTarget = false;
	}
	// don't look for a target if sulking
	if (psDroid->action == DACTION_SULK)
	{
		lookForTarget = false;
	}

	/* Only try to update target if already have some target */
	if (psDroid->action == DACTION_ATTACK ||
	    psDroid->action == DACTION_MOVEFIRE ||
	    psDroid->action == DACTION_MOVETOATTACK ||
	    psDroid->action == DACTION_ROTATETOATTACK)
	{
		updateTarget = true;
	}
	if ((orderState(psDroid, DORDER_OBSERVE) || orderState(psDroid, DORDER_ATTACKTARGET)) &&
	    psDroid->order.psObj && psDroid->order.psObj->died)
	{
		lookForTarget = true;
		updateTarget = false;
	}

	/* Don't update target if we are sent to attack and reached attack destination (attacking our target) */
	if (orderState(psDroid, DORDER_ATTACK) && psDroid->psActionTarget[0] == psDroid->order.psObj)
	{
		updateTarget = false;
	}

	// don't look for a target if there are any queued orders
	if (psDroid->listSize > 0)
	{
		lookForTarget = false;
		updateTarget = false;
	}

	// don't allow units to start attacking if they will switch to guarding the commander
	if (hasCommander(psDroid))
	{
		lookForTarget = false;
		updateTarget = false;
	}

	if (bMultiPlayer && isVtolDroid(psDroid) && isHumanPlayer(psDroid->player))
	{
		lookForTarget = false;
		updateTarget = false;
	}

	// do not look for a target if droid is currently under direct control.
	if (driveModeActive() && (psDroid == driveGetDriven()))
	{
		lookForTarget = false;
		updateTarget = false;
	}

	// CB and VTOL CB droids can't autotarget.
	if (psDroid->droidType == DROID_SENSOR && !standardSensorDroid(psDroid))
	{
		lookForTarget = false;
		updateTarget = false;
	}

	// do not attack if the attack level is wrong
	if (secondaryGetState(psDroid, DSO_ATTACK_LEVEL) != DSS_ALEV_ALWAYS)
	{
		lookForTarget = false;
	}

	/* For commanders and non-assigned non-commanders: look for a better target once in a while */
	if (!lookForTarget && updateTarget && psDroid->numWeaps > 0 && !hasCommander(psDroid)
	    && (psDroid->id + gameTime) / TARGET_UPD_SKIP_FRAMES != (psDroid->id + gameTime - deltaGameTime) / TARGET_UPD_SKIP_FRAMES)
	{
		for (int i = 0; i < psDroid->numWeaps; ++i)
		{
			updateAttackTarget((BASE_OBJECT *)psDroid, i);
		}
	}

	/* Null target - see if there is an enemy to attack */

	if (lookForTarget && !updateTarget)
	{
		if (psDroid->droidType == DROID_SENSOR)
		{
			if (aiChooseSensorTarget((BASE_OBJECT *)psDroid, &psTarget))
			{
				actionDroid(psDroid, DACTION_OBSERVE, psTarget);
			}
		}
		else
		{
			if (aiChooseTarget((BASE_OBJECT *)psDroid, &psTarget, 0, true, NULL))
			{
				actionDroid(psDroid, DACTION_ATTACK, psTarget);
			}
		}
	}
}

/* Check if any of our weapons can hit the target... */
bool checkAnyWeaponsTarget(BASE_OBJECT *psObject, BASE_OBJECT *psTarget)
{
	DROID *psDroid = (DROID *) psObject;
	for (int i = 0; i < psDroid->numWeaps; i++)
	{
		if (validTarget(psObject, psTarget, i))
		{
			return true;
		}
	}
	return false;
}

/* Set of rules which determine whether the weapon associated with the object can fire on the propulsion type of the target. */
bool validTarget(BASE_OBJECT *psObject, BASE_OBJECT *psTarget, int weapon_slot)
{
	bool	bTargetInAir = false, bValidTarget = false;
	UBYTE	surfaceToAir = 0;

	if (!psTarget)
	{
		return false;
	}

	// Need to check propulsion type of target
	switch (psTarget->type)
	{
	case OBJ_DROID:
		if (asPropulsionTypes[asPropulsionStats[((DROID *)psTarget)->asBits[COMP_PROPULSION]].propulsionType].travel == AIR)
		{
			if (((DROID *)psTarget)->sMove.Status != MOVEINACTIVE)
			{
				bTargetInAir = true;
			}
			else
			{
				bTargetInAir = false;
			}
		}
		else
		{
			bTargetInAir = false;
		}
		break;
	case OBJ_STRUCTURE:
	default:
		//lets hope so!
		bTargetInAir = false;
		break;
	}

	//need what can shoot at
	switch (psObject->type)
	{
	case OBJ_DROID:
		if (((DROID *)psObject)->droidType == DROID_SENSOR)
		{
			return !bTargetInAir;  // Sensor droids should not target anything in the air.
		}

		// Can't attack without a weapon
		if (((DROID *)psObject)->numWeaps != 0 && ((DROID *)psObject)->asWeaps[weapon_slot].nStat != 0)
		{
			surfaceToAir = asWeaponStats[((DROID *)psObject)->asWeaps[weapon_slot].nStat].surfaceToAir;
			if (((surfaceToAir & SHOOT_IN_AIR) && bTargetInAir) || ((surfaceToAir & SHOOT_ON_GROUND) && !bTargetInAir))
			{
				return true;
			}
		}
		else
		{
			return false;
		}
		break;
	case OBJ_STRUCTURE:
		// Can't attack without a weapon
		if (((STRUCTURE *)psObject)->numWeaps != 0 && ((STRUCTURE *)psObject)->asWeaps[weapon_slot].nStat != 0)
		{
			surfaceToAir = asWeaponStats[((STRUCTURE *)psObject)->asWeaps[weapon_slot].nStat].surfaceToAir;
		}
		else
		{
			surfaceToAir = 0;
		}

		if (((surfaceToAir & SHOOT_IN_AIR) && bTargetInAir) || ((surfaceToAir & SHOOT_ON_GROUND) && !bTargetInAir))
		{
			return true;
		}
		break;
	default:
		surfaceToAir = 0;
		break;
	}

	//if target is in the air and you can shoot in the air - OK
	if (bTargetInAir && (surfaceToAir & SHOOT_IN_AIR))
	{
		bValidTarget = true;
	}

	//if target is on the ground and can shoot at it - OK
	if (!bTargetInAir && (surfaceToAir & SHOOT_ON_GROUND))
	{
		bValidTarget = true;
	}

	return bValidTarget;
}
