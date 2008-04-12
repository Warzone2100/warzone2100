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
#include "geometry.h"
#include "map.h"
#include "mapgrid.h"
#include "multiplay.h"
#include "projectile.h"
#include "visibility.h"

/* Calculates attack priority for a certain target */
static SDWORD targetAttackWeight(BASE_OBJECT *psTarget, BASE_OBJECT *psAttacker, SDWORD weapon_slot);

// alliances
UBYTE	alliances[MAX_PLAYERS][MAX_PLAYERS];

/* alliance code for ai. return true if an alliance has formed. */
BOOL aiCheckAlliances(UDWORD s1,UDWORD s2)
{
    //features have their player number set to (MAX_PLAYERS + 1)
    if ( s1 == (MAX_PLAYERS + 1) || s2 == (MAX_PLAYERS + 1))
    {
        return false;
    }
	if ((s1 == s2) ||
		(alliances[s1][s2] == ALLIANCE_FORMED))
	{
		return true;
	}

	return false;
}

/* Initialise the AI system */
BOOL aiInitialise(void)
{
	SDWORD		i,j;

	for(i=0; i<MAX_PLAYERS; i++)
	{
		for(j=0; j<MAX_PLAYERS; j++)
		{
			alliances[i][j] = ALLIANCE_BROKEN;
		}
	}

	return true;
}

/* Shutdown the AI system */
BOOL aiShutdown(void)
{
	return true;
}

// Find the best nearest target for a droid
// Returns integer representing target priority, -1 if failed
SDWORD aiBestNearestTarget(DROID *psDroid, BASE_OBJECT **ppsObj, int weapon_slot)
{
	UDWORD				i;
	SDWORD				bestMod,newMod,failure=-1;
	BASE_OBJECT			*psTarget,*friendlyObj,*bestTarget,*targetInQuestion,*tempTarget;
	BOOL				electronic = false;
	STRUCTURE			*targetStructure;
	WEAPON_EFFECT		weaponEffect;

	//don't bother looking if empty vtol droid
	if (vtolEmpty(psDroid))
	{
		return failure;
	}

	/* Return if have no weapons */
	// Watermelon:added a protection against no weapon droid 'numWeaps'
	// The ai orders a non-combat droid to patrol = crash without it...
	if(psDroid->asWeaps[0].nStat == 0 || psDroid->numWeaps == 0)
	{
		return failure;
	}

	droidGetNaybors(psDroid);

	//weaponMod = asWeaponModifier[weaponEffect][(asPropulsionStats + ((DROID*)psObj)->asBits[COMP_PROPULSION].nStat)->propulsionType];
	weaponEffect = ((WEAPON_STATS *)(asWeaponStats + psDroid->asWeaps[weapon_slot].nStat))->weaponEffect;

	//electronic warfare can only be used against structures at present - not any more! AB 6/11/98
	electronic = electronicDroid(psDroid);
	psTarget = NULL;
	bestTarget = NULL;
	bestMod = 0;
	for (i=0; i< numNaybors; i++)
	{
		friendlyObj = NULL;
		targetInQuestion = asDroidNaybors[i].psObj;

		/* This is a friendly unit, check if we can reuse its target */
		if(targetInQuestion->player == psDroid->player ||
			aiCheckAlliances(targetInQuestion->player,psDroid->player))
		{
			friendlyObj = targetInQuestion;
			targetInQuestion = NULL;

			/* Can we see what it is doing? */
			if(friendlyObj->visible[psDroid->player])
			{
				if(friendlyObj->type == OBJ_DROID)
				{
					DROID	*friendlyDroid = (DROID *)friendlyObj;

					/* See if friendly droid has a target */
					tempTarget = friendlyDroid->psActionTarget[0];
					if(tempTarget && !tempTarget->died)
					{
						//make sure a weapon droid is targeting it
						if(friendlyDroid->numWeaps > 0)
						{
							// make sure this target wasn't assigned explicitly to this droid
							if(friendlyDroid->order != DORDER_ATTACK)
							{
								// make sure target is near enough
								if (dirtySqrt(psDroid->pos.x, psDroid->pos.y, tempTarget->pos.x, tempTarget->pos.y)
								    < droidSensorRange(psDroid))
								{
									targetInQuestion = tempTarget;		//consider this target
								}
							}
						}
					}
				}
				else if(friendlyObj->type == OBJ_STRUCTURE)
				{
					tempTarget = ((STRUCTURE*)friendlyObj)->psTarget[0];
					if (tempTarget && !tempTarget->died)
					{
						targetInQuestion = tempTarget;
					}
				}
			}
		}

		if (targetInQuestion != NULL &&
			targetInQuestion != (BASE_OBJECT *)psDroid &&		//in case friendly unit had me as target
			(targetInQuestion->type == OBJ_DROID ||
			 targetInQuestion->type == OBJ_STRUCTURE) &&
			 targetInQuestion->visible[psDroid->player] &&
			 targetInQuestion->player != psDroid->player &&
			 !aiCheckAlliances(targetInQuestion->player,psDroid->player))
		{

			if ( !validTarget((BASE_OBJECT *)psDroid, targetInQuestion, weapon_slot) )
			{
				continue;
			}
			else if (targetInQuestion->type == OBJ_DROID)
			{
				// in multiPlayer - don't attack Transporters with EW
				if (bMultiPlayer)
				{
					// if not electronic then valid target
					if (!electronic
					    || (electronic
					        && ((DROID *)targetInQuestion)->droidType != DROID_TRANSPORTER))
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
				if (electronic)
				{
					/* don't want to target structures with resistance of zero if using electronic warfare */
					if (validStructResistance((STRUCTURE *)targetInQuestion))
					{
						psTarget = targetInQuestion;
					}
				}
				else if (((STRUCTURE *)targetInQuestion)->asWeaps[weapon_slot].nStat > 0)
				{
					// structure with weapons - go for this
					psTarget = targetInQuestion;
				}
				else if ( (  ((STRUCTURE *)targetInQuestion)->pStructureType->type != REF_WALL
						   &&((STRUCTURE *)targetInQuestion)->pStructureType->type != REF_WALLCORNER
						  )
						 || driveModeActive()
						 || (bMultiPlayer && game.type == SKIRMISH && !isHumanPlayer(psDroid->player))
						)
				{
					psTarget = targetInQuestion;
				}
			}

			/* Check if our weapon is most effective against this object */
			if(psTarget != NULL && psTarget == targetInQuestion)		//was assigned?
			{
				newMod = targetAttackWeight(psTarget, (BASE_OBJECT *)psDroid, weapon_slot);

				/* Remember this one if it's our best target so far */
				if( newMod >= 0 && (newMod > bestMod || bestTarget == NULL))
				{
					bestMod = newMod;
					bestTarget = psTarget;
				}

			}
		}

	}

	if (bestTarget)
	{
		ASSERT(!bestTarget->died, "aiBestNearestTarget: AI gave us a target that is already dead.");
		/* See if target is blocked by a wall; only affects direct weapons */
		if (proj_Direct(asWeaponStats + psDroid->asWeaps[weapon_slot].nStat)
		 && (targetStructure = visGetBlockingWall((BASE_OBJECT *)psDroid, bestTarget)))
		{
			//are we any good against walls?
			if(asStructStrengthModifier[weaponEffect][targetStructure->pStructureType->strength] >= 100)		//can attack atleast with default strength
			{
				bestTarget = (BASE_OBJECT *)targetStructure;			//attack wall
			}
		}

		*ppsObj = bestTarget;
		return bestMod;
	}

	return failure;
}

/* Calculates attack priority for a certain target */
static SDWORD targetAttackWeight(BASE_OBJECT *psTarget, BASE_OBJECT *psAttacker, SDWORD weapon_slot)
{
	SDWORD			targetTypeBonus=0, damageRatio=0, attackWeight=0, noTarget=-1;
	UDWORD			weaponSlot;
	DROID			*targetDroid=NULL,*psAttackerDroid=NULL,*psGroupDroid,*psDroid;
	STRUCTURE		*targetStructure=NULL;
	WEAPON_EFFECT	weaponEffect;
	WEAPON_STATS	*attackerWeapon;
	BOOL			bEmpWeap=false,bCmdAttached=false,bTargetingCmd=false;

	if (psTarget == NULL || psAttacker == NULL)
	{
		return noTarget;
	}
	ASSERT(psTarget != psAttacker, "targetAttackWeight: Wanted to evaluate the worth of attacking ourselves...");

	targetTypeBonus = 0;			//Sensors/ecm droids, non-military structures get lower priority

	/* Get attacker weapon effect */
	if(psAttacker->type == OBJ_DROID)
	{
		psAttackerDroid = (DROID *)psAttacker;

		attackerWeapon = (WEAPON_STATS *)(asWeaponStats +
			psAttackerDroid->asWeaps[weapon_slot].nStat);


		//check if this droid is assigned to a commander
		bCmdAttached = (psAttackerDroid->droidType != DROID_COMMAND &&
						psAttackerDroid->psGroup != NULL &&
						psAttackerDroid->psGroup->type == GT_COMMAND);

		//find out if current target is targeting our commander
		if(bCmdAttached)
		{
			if(psTarget->type == OBJ_DROID)
			{
				psDroid = (DROID *)psTarget;

				//go through all enemy weapon slots
				for(weaponSlot = 0; !bTargetingCmd &&
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
				if(psTarget->type == OBJ_STRUCTURE)
				{
					//go through all enemy weapons
					for(weaponSlot = 0; !bTargetingCmd &&
						weaponSlot < ((STRUCTURE *)psTarget)->numWeaps; weaponSlot++)
					{
						if( ((STRUCTURE *)psTarget)->psTarget[weaponSlot] ==
								(BASE_OBJECT *)psAttackerDroid->psGroup->psCommander)
						{
							bTargetingCmd = true;
						}
					}
				}
			}
		}
	}
	else if(psAttacker->type == OBJ_STRUCTURE)
	{
		attackerWeapon = ((WEAPON_STATS *)(asWeaponStats +
			((STRUCTURE *)psAttacker)->asWeaps[weapon_slot].nStat));
	}
	else	/* feature */
	{
		ASSERT(!"invalid attacker object type", "targetAttackWeight: Invalid attacker object type");
		return noTarget;
	}

	//Get weapon effect
	weaponEffect = attackerWeapon->weaponEffect;

	//See if attacker is using an EMP weapon
	bEmpWeap = (attackerWeapon->weaponSubClass == WSC_EMP);

	/* Calculate attack weight */
	if(psTarget->type == OBJ_DROID)
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
			damageRatio = 1 - targetDroid->body / targetDroid->originalBody;
		}
		assert(targetDroid->originalBody != 0); // Assert later so we get the info from above

		/* See if this type of a droid should be prioritized */
		switch (targetDroid->droidType)
		{
			case DROID_SENSOR:
			case DROID_ECM:
			case DROID_PERSON:
			case DROID_TRANSPORTER:
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
		attackWeight = asWeaponModifier[weaponEffect][(asPropulsionStats + targetDroid->asBits[COMP_PROPULSION].nStat)->propulsionType] // Our weapon's effect against target
				- WEIGHT_DIST_TILE_DROID * map_coord(dirtySqrt(psAttacker->pos.x, psAttacker->pos.y, targetDroid->pos.x, targetDroid->pos.y)) // farer droids are less attractive
				+ WEIGHT_HEALTH_DROID * damageRatio // we prefer damaged droids
				+ targetTypeBonus; // some droid types have higher priority

		/* If attacking with EMP try to avoid targets that were already "EMPed" */
		if(bEmpWeap &&
			(targetDroid->lastHitWeapon == WSC_EMP) &&
			((gameTime - targetDroid->timeLastHit) < EMP_DISABLE_TIME))		//target still disabled
		{
			attackWeight /= EMP_DISABLED_PENALTY_F;
		}
	}
	else if(psTarget->type == OBJ_STRUCTURE)
	{
		targetStructure = (STRUCTURE *)psTarget;

		/* Calculate damage this target suffered */
		damageRatio = 1 - targetStructure->body / structureBody(targetStructure);

		/* See if this type of a structure should be prioritized */
		switch(targetStructure->pStructureType->type)
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
				- WEIGHT_DIST_TILE_STRUCT * map_coord(dirtySqrt(psAttacker->pos.x, psAttacker->pos.y, targetStructure->pos.x, targetStructure->pos.y)) // farer structs are less attractive
				+ WEIGHT_HEALTH_STRUCT * damageRatio // we prefer damaged structures
				+ targetTypeBonus; // some structure types have higher priority

		/* Go for unfinished structures only if nothing else found (same for non-visible structures) */
		if(targetStructure->status != SS_BUILT)		//a decoy?
		{
			attackWeight /= WEIGHT_STRUCT_NOTBUILT_F;
		}

		/* EMP should only attack structures if no enemy droids are around */
		if(bEmpWeap)
		{
			attackWeight /= EMP_STRUCT_PENALTY_F;
		}

	}
	else	//a feature
	{
		return noTarget;
	}

	/* We prefer objects we can see and can attack immediately */
	if(!visibleObjWallBlock((BASE_OBJECT *)psAttacker, psTarget))
	{
		attackWeight /= WEIGHT_NOT_VISIBLE_F;
	}

	/* Commander-related criterias */
	if(bCmdAttached)	//attached to a commander and don't have a target assigned by some order
	{
		ASSERT(psAttackerDroid->psGroup->psCommander != NULL, "Commander is NULL");

		//if commander is being targeted by our target, try to defend the commander
		if(bTargetingCmd)
		{
			attackWeight += WEIGHT_CMD_RANK * ( 1 + getDroidLevel(psAttackerDroid->psGroup->psCommander));
		}

		//fire support - go through all droids assigned to the commander
		for(psGroupDroid = psAttackerDroid->psGroup->psList;
			psGroupDroid; psGroupDroid = psGroupDroid->psGrpNext)
		{
			for(weaponSlot = 0; weaponSlot < psGroupDroid->numWeaps; weaponSlot++)
			{
				//see if this droid is currently targeting current target
				if(psGroupDroid->psTarget == psTarget ||
				   psGroupDroid->psActionTarget[weaponSlot] == psTarget)
				{
					//we prefer targets that are already targeted and hence will be destroyed faster
					attackWeight += WEIGHT_CMD_SAME_TARGET;
				}
			}
		}
	}

	return attackWeight;
}


// see if a structure has the range to fire on a target
static BOOL aiStructHasRange(STRUCTURE *psStruct, BASE_OBJECT *psTarget, int weapon_slot)
{
	WEAPON_STATS		*psWStats;
	SDWORD				xdiff,ydiff, longRange;

	if (psStruct->numWeaps == 0 || psStruct->asWeaps[0].nStat == 0)
	{
		// Can't attack without a weapon
		return false;
	}

	psWStats = psStruct->asWeaps[weapon_slot].nStat + asWeaponStats;

	xdiff = (SDWORD)psStruct->pos.x - (SDWORD)psTarget->pos.x;
	ydiff = (SDWORD)psStruct->pos.y - (SDWORD)psTarget->pos.y;
	longRange = proj_GetLongRange(psWStats);
	if (xdiff*xdiff + ydiff*ydiff < longRange*longRange)
	{
		// in range
		return true;

	}

	return false;
}


// see if an object is a wall
static BOOL aiObjIsWall(BASE_OBJECT *psObj)
{
	if (psObj->type != OBJ_STRUCTURE)
	{
		return false;
	}

	if ( ((STRUCTURE *)psObj)->pStructureType->type != REF_WALL &&
		 ((STRUCTURE *)psObj)->pStructureType->type != REF_WALLCORNER )
	{
		return false;
	}

	return true;
}


/* See if there is a target in range */
BOOL aiChooseTarget(BASE_OBJECT *psObj, BASE_OBJECT **ppsTarget, int weapon_slot, BOOL bUpdateTarget)
{
	UDWORD	radSquared;
	BASE_OBJECT		*psTarget = NULL;
	SDWORD			xdiff,ydiff, distSq, tarDist, minDist;//, longRange;
	BOOL			bCBTower;
	STRUCTURE		*psCStruct;
	DROID			*psCommander;
	BOOL			bCommanderBlock;
	UDWORD			sensorRange = objSensorRange(psObj);
	SECONDARY_STATE	state;
	SDWORD			curTargetWeight=-1,newTargetWeight;

	/* Get the sensor range */
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
			// Can't attack without a weapon
			return false;
		}
		radSquared = sensorRange * sensorRange;
		break;
	case OBJ_STRUCTURE:
		if (((STRUCTURE *)psObj)->numWeaps == 0 || ((STRUCTURE *)psObj)->asWeaps[0].nStat == 0)
		{
			// Can't attack without a weapon
			return false;
		}

		// increase the sensor range for AA sites
		// AA sites are defensive structures that can only shoot in the air
		if ( (((STRUCTURE *)psObj)->pStructureType->type == REF_DEFENSE) &&
			 (asWeaponStats[((STRUCTURE *)psObj)->asWeaps[weapon_slot].nStat].surfaceToAir == SHOOT_IN_AIR) )
		{
			sensorRange = 3 * sensorRange / 2;
		}

		radSquared = sensorRange*sensorRange;
		break;
	default:
		radSquared = 0;
		break;
	}

	/* See if there is a something in range */
	if (psObj->type == OBJ_DROID)
	{
		BASE_OBJECT *psCurrTarget = ((DROID *)psObj)->psActionTarget[0];

		/* find a new target */
		newTargetWeight = aiBestNearestTarget((DROID *)psObj, &psTarget, weapon_slot);

		/* Calculate weight of the current target if updating; but take care not to target
		 * ourselves... */
		if (bUpdateTarget && psCurrTarget != psObj)
		{
			curTargetWeight = targetAttackWeight(psCurrTarget, psObj, weapon_slot);
		}

		if (newTargetWeight >= 0 &&		//found a new target
			(!bUpdateTarget ||			//choosing a new target, don't care if current one is better
			(curTargetWeight <= 0) ||	//attacker had no valid target, use new one
			(newTargetWeight > (curTargetWeight + OLD_TARGET_THRESHOLD))	//updating and new target is better
			))
		{
			/*check its a valid target*/
			if (validTarget(psObj, psTarget, weapon_slot))
			{
    			/* See if in sensor range */
			    xdiff = psTarget->pos.x - psObj->pos.x;
			    ydiff = psTarget->pos.y - psObj->pos.y;
			    if ((xdiff*xdiff + ydiff*ydiff < (SDWORD)radSquared) ||			//target is within our sensor range
					(secondaryGetState((DROID *)psObj, DSO_HALTTYPE, &state) &&	//in case we got this target from a friendly unit see if can pursue it
					(state != DSS_HALT_HOLD)))									//make sure it's guard or pursue
				{
					ASSERT(!isDead(psTarget), "aiChooseTarget: Droid found a dead target!");
				    *ppsTarget = psTarget;
				    return true;
				}
			}
		}
	}
	else if (psObj->type == OBJ_STRUCTURE)
	{
		WEAPON_STATS	*psWStats;

		ASSERT( ((STRUCTURE *)psObj)->asWeaps[weapon_slot].nStat > 0,
			"aiChooseTarget: no weapons on structure" );

		psWStats = ((STRUCTURE *)psObj)->asWeaps[weapon_slot].nStat + asWeaponStats;

		// see if there is a target from the command droids
		psTarget = NULL;
		psCommander = cmdDroidGetDesignator(psObj->player);
		bCommanderBlock = false;
		if (!proj_Direct(psWStats) && (psCommander != NULL) &&
			aiStructHasRange((STRUCTURE *)psObj, (BASE_OBJECT *)psCommander, weapon_slot))
		{
			// there is a commander that can fire designate for this structure
			// set bCommanderBlock so that the structure does not fire until the commander
			// has a target - (slow firing weapons will not be ready to fire otherwise).
			bCommanderBlock = true;
			if (psCommander->action == DACTION_ATTACK
			    && psCommander->psActionTarget[0] != NULL
			    && !psCommander->psActionTarget[0]->died)
			{
				// the commander has a target to fire on
				if (aiStructHasRange((STRUCTURE *)psObj, psCommander->psActionTarget[0], weapon_slot))
				{
					// target in range - fire on it
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
		tarDist = SDWORD_MAX;
		minDist = psWStats->minRange * psWStats->minRange;
		bCBTower = false;
		if (psTarget == NULL &&
			!bCommanderBlock &&
			!proj_Direct(psWStats))
		{
			for(psCStruct=apsStructLists[psObj->player]; psCStruct; psCStruct=psCStruct->psNext)
			{
				// skip incomplete structures
				if (psCStruct->status != SS_BUILT)
				{
					continue;
				}

				if (!bCBTower
				    && structStandardSensor(psCStruct)
				    && psCStruct->psTarget[0] != NULL
				    && !psCStruct->psTarget[0]->died)
				{
					/*check its a valid target*/
					//Watermelon:Greater than 1 for now
					if ( validTarget(psObj, psCStruct->psTarget[0], 0) &&
						aiStructHasRange((STRUCTURE *)psObj, psCStruct->psTarget[0], weapon_slot))
					{
					    xdiff = (SDWORD)psCStruct->psTarget[0]->pos.x - (SDWORD)psObj->pos.x;
					    ydiff = (SDWORD)psCStruct->psTarget[0]->pos.y - (SDWORD)psObj->pos.y;
					    distSq = xdiff*xdiff + ydiff*ydiff;
					    if ((distSq < tarDist) &&
						    (distSq > minDist))
					    {
						    tarDist = distSq;
						    psTarget = psCStruct->psTarget[0];
					    }
					}
				}
				else if (structCBSensor(psCStruct)
				         && psCStruct->psTarget[0] != NULL
				         && !psCStruct->psTarget[0]->died)
				{
					/*check its a valid target*/
					if ( validTarget(psObj, psCStruct->psTarget[0], 0) &&
						aiStructHasRange((STRUCTURE *)psObj, psCStruct->psTarget[0], weapon_slot))
					{
						xdiff = (SDWORD)psCStruct->psTarget[0]->pos.x - (SDWORD)psObj->pos.x;
						ydiff = (SDWORD)psCStruct->psTarget[0]->pos.y - (SDWORD)psObj->pos.y;
						distSq = xdiff*xdiff + ydiff*ydiff;
					    if ((!bCBTower || (distSq < tarDist)) &&
						    (distSq > minDist))
					    {
						    tarDist = distSq;
						    psTarget = psCStruct->psTarget[0];
						    bCBTower = true;
					    }
					}
				}
			}
		}

		if ((psTarget == NULL) &&
			!bCommanderBlock)
		{
			BASE_OBJECT *psCurr;

			gridStartIterate((SDWORD)psObj->pos.x, (SDWORD)psObj->pos.y);
			psCurr = gridIterate();
			while (psCurr != NULL)
			{
				//don't target features
				if (psCurr->type != OBJ_FEATURE && !psCurr->died)
				{
					if (psObj->player != psCurr->player &&
						!aiCheckAlliances(psCurr->player,psObj->player))
					{
						/*check its a valid target*/
						//Watermelon:Greater than 1 for now
						if ( validTarget(psObj, psCurr, weapon_slot) &&
							!aiObjIsWall(psCurr))
						{
						    // See if in sensor range and visible
						    xdiff = psCurr->pos.x - psObj->pos.x;
						    ydiff = psCurr->pos.y - psObj->pos.y;
						    distSq = xdiff*xdiff + ydiff*ydiff;
						    if (distSq < (SDWORD)radSquared &&
							    psCurr->visible[psObj->player] &&
							    distSq < tarDist)
						    {
							    psTarget = psCurr;
							    tarDist = distSq;
						    }
						}
					}
				}
				psCurr = gridIterate();
			}
		}

		if (psTarget)
		{
			ASSERT(!psTarget->died, "aiChooseTarget: Structure found a dead target!");
			*ppsTarget = psTarget;
			return true;
		}
	}

	return false;
}


/* See if there is a target in range for Sensor objects*/
BOOL aiChooseSensorTarget(BASE_OBJECT *psObj, BASE_OBJECT **ppsTarget)
{
	SDWORD	sensorRange = objSensorRange(psObj);
	UDWORD	radSquared;
	BASE_OBJECT		*psCurr,*psTemp = NULL;
	BASE_OBJECT		*psTarget = NULL;
	SDWORD	xdiff,ydiff, distSq, tarDist;

	/* Get the sensor range */
	switch (psObj->type)
	{
	case OBJ_DROID:
		if (asSensorStats[((DROID *)psObj)->asBits[COMP_SENSOR].nStat].
			location != LOC_TURRET)
		{
			// to be used for Turret Sensors only
			return false;
		}
		radSquared = sensorRange * sensorRange;
		break;
	case OBJ_STRUCTURE:
		if (!(structStandardSensor((STRUCTURE *)psObj) ||
			structVTOLSensor((STRUCTURE *)psObj)))
		{
			// to be used for Standard and VTOL intercept Turret Sensors only
			return false;
		}
		radSquared = sensorRange * sensorRange;
		break;
	default:
		radSquared = 0;
		break;
	}

	/* See if there is a something in range */
	if (psObj->type == OBJ_DROID && CAN_UPDATE_NAYBORS( (DROID *)psObj ))
	{
		if (aiBestNearestTarget((DROID *)psObj, &psTarget, 0) >= 0)
		{
			/* See if in sensor range */
			xdiff = psTarget->pos.x - psObj->pos.x;
			ydiff = psTarget->pos.y - psObj->pos.y;
			if (xdiff*xdiff + ydiff*ydiff < (SDWORD)radSquared)
			{
				*ppsTarget = psTarget;
				return true;
			}
		}
	}
	else
	{
		tarDist = SDWORD_MAX;
		gridStartIterate((SDWORD)psObj->pos.x, (SDWORD)psObj->pos.y);
		psCurr = gridIterate();
		while (psCurr != NULL)
		{
			    //don't target features
			    if (psCurr->type != OBJ_FEATURE && !psCurr->died)
			    {
				    if (psObj->player != psCurr->player &&
					    !aiCheckAlliances(psCurr->player,psObj->player) &&
					    !aiObjIsWall(psCurr))
				    {
					    // See if in sensor range and visible
					    xdiff = psCurr->pos.x - psObj->pos.x;
					    ydiff = psCurr->pos.y - psObj->pos.y;
					    distSq = xdiff*xdiff + ydiff*ydiff;
					    if (distSq < (SDWORD)radSquared &&
						    psCurr->visible[psObj->player] &&
						    distSq < tarDist)
					    {
						    psTemp = psCurr;
						    tarDist = distSq;
					    }
				    }
			    }
			    psCurr = gridIterate();
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

/* Do the AI for a droid */
void aiUpdateDroid(DROID *psDroid)
{
	BASE_OBJECT	*psTarget;
	SECONDARY_STATE	state;
	BOOL		lookForTarget,updateTarget;

	ASSERT( psDroid != NULL,
		"updateUnitAI: invalid Unit pointer" );

	lookForTarget = true;
	updateTarget = true;

	// don't look for a target if sulking
	if (psDroid->action == DACTION_SULK)
	{
		lookForTarget = false;
		updateTarget = false;
	}
	// don't look for a target if doing something else
	if (!orderState(psDroid, DORDER_NONE) &&
		!orderState(psDroid, DORDER_GUARD))
	{
		lookForTarget = false;
	}

	/* Only try to update target if already have some target */
	if (psDroid->action != DACTION_ATTACK &&
		psDroid->action != DACTION_MOVEFIRE &&
		psDroid->action != DACTION_MOVETOATTACK &&
		psDroid->action != DACTION_ROTATETOATTACK)
	{
		updateTarget = false;
	}

	/* Don't update target if we are sent to attack and reached
		attack destination (attacking our target) */
	if (orderState(psDroid, DORDER_ATTACK) && psDroid->psActionTarget[0] == psDroid->psTarget)
	{
		updateTarget = false;
	}

	// don't look for a target if there are any queued orders
	if (psDroid->listSize > 0)
	{
		lookForTarget = false;
		updateTarget = false;
	}

	// horrible check to stop droids looking for a target if
	// they would switch to the guard order in the order update loop
	if ((psDroid->order == DORDER_NONE) &&
		(psDroid->player == selectedPlayer) &&
		!vtolDroid(psDroid) &&
		secondaryGetState(psDroid, DSO_HALTTYPE, &state) &&
		(state == DSS_HALT_GUARD))
	{
		lookForTarget = false;
		updateTarget = false;
	}

	// don't allow units to start attacking if they will switch to guarding the commander
	if (psDroid->droidType != DROID_COMMAND && psDroid->psGroup != NULL &&
		 psDroid->psGroup->type == GT_COMMAND)
	{
		lookForTarget = false;
		updateTarget = false;
	}

	if(bMultiPlayer && vtolDroid(psDroid) && isHumanPlayer(psDroid->player))
	{
		lookForTarget = false;
		updateTarget = false;
	}

	// do not choose another target if doing anything while guarding
	if (orderState(psDroid, DORDER_GUARD) &&
		(psDroid->action != DACTION_NONE))
	{
		lookForTarget = false;
	}

	// do not look for a target if droid is currently under direct control.
	if(driveModeActive() && (psDroid == driveGetDriven())) {
		lookForTarget = false;
		updateTarget = false;
	}

	// only computer sensor droids in the single player game aquire targets
	if ((psDroid->droidType == DROID_SENSOR && psDroid->player == selectedPlayer)
	    && !bMultiPlayer)
	{
		lookForTarget = false;
		updateTarget = false;
	}

	// do not attack if the attack level is wrong
	if (secondaryGetState(psDroid, DSO_ATTACK_LEVEL, &state))
	{
		if (state != DSS_ALEV_ALWAYS)
		{
			lookForTarget = false;
		}
	}

	/* Don't rebuild 'Naybor' list too often */
	if(!CAN_UPDATE_NAYBORS(psDroid))
	{
		lookForTarget = false;
	}

	/* For commanders and non-assigned non-commanders:
	 look for a better target once in a while */
	if(!lookForTarget && updateTarget)
	{
		if((psDroid->numWeaps > 0) && ((psDroid->droidType == DROID_COMMAND) ||
			!(psDroid->psGroup && (psDroid->psGroup->type == GT_COMMAND))))	//not assigned to commander
		{
			if((psDroid->id % TARGET_UPD_SKIP_FRAMES) ==
				(frameGetFrameNumber() % TARGET_UPD_SKIP_FRAMES))
			{
				int i;

				(void)updateAttackTarget((BASE_OBJECT*)psDroid, 0); // this function always has to be called on weapon-slot 0 (even if ->numWeaps == 0)

				//updates all targets
				for (i = 1; i < psDroid->numWeaps; ++i)
				{
					(void)updateAttackTarget((BASE_OBJECT*)psDroid, i);
				}
			}
		}
	}

	/* Null target - see if there is an enemy to attack */

	if (lookForTarget && !updateTarget)
	{
		turnOffMultiMsg(true);
		if (psDroid->droidType == DROID_SENSOR)
		{
			//Watermelon:only 1 target for sensor droid
			if ( aiChooseTarget((BASE_OBJECT *)psDroid, &psTarget, 0, true) )
			{
				orderDroidObj(psDroid, DORDER_OBSERVE, psTarget);
			}
		}
		else
		{
			if (aiChooseTarget((BASE_OBJECT *)psDroid, &psTarget, 0, true))
			{
				orderDroidObj(psDroid, DORDER_ATTACKTARGET, psTarget);
			}
		}
		turnOffMultiMsg(false);
	}
}

/*set of rules which determine whether the weapon associated with the object
can fire on the propulsion type of the target*/
//Watermelon:added weapon_slot
BOOL validTarget(BASE_OBJECT *psObject, BASE_OBJECT *psTarget, int weapon_slot)
{
	BOOL	bTargetInAir, bValidTarget = false;
	UBYTE	surfaceToAir;

	if (!psTarget)
	{
		return false;
	}

	//need to check propulsion type of target
	switch (psTarget->type)
	{
	case OBJ_DROID:
        if (asPropulsionTypes[asPropulsionStats[((DROID *)psTarget)->asBits[
            COMP_PROPULSION].nStat].propulsionType].travel == AIR)
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
    	// Can't attack without a weapon
		//Watermelon:re-enabled if (((DROID *)psObject)->numWeaps != 0) to prevent crash
		if ( ((DROID *)psObject)->numWeaps != 0 &&
			((DROID *)psObject)->asWeaps[weapon_slot].nStat != 0 )
		{
			surfaceToAir = asWeaponStats[((DROID *)psObject)->asWeaps[weapon_slot].nStat].surfaceToAir;
			if ( ((surfaceToAir & SHOOT_IN_AIR) && bTargetInAir) || ((surfaceToAir & SHOOT_ON_GROUND) && !bTargetInAir) )
			{
				return true;
			}
		}
		else
		{
			return false;
		}
		/*
        if (((DROID *)psObject)->asWeaps[0].nStat != 0 && ((DROID *)psObject)->numWeaps > 0)
		{
            surfaceToAir = asWeaponStats[((DROID *)psObject)->asWeaps[0].nStat].surfaceToAir;
        }
		else
		{
			 surfaceToAir = 0;
		}
		*/
		break;
	case OBJ_STRUCTURE:
		// Can't attack without a weapon
		//Watermelon:re-enabled if (((DROID *)psObject)->numWeaps != 0) to prevent crash
		if ( ((STRUCTURE *)psObject)->numWeaps != 0 &&
			((STRUCTURE *)psObject)->asWeaps[weapon_slot].nStat != 0 )
		{
			surfaceToAir = asWeaponStats[((STRUCTURE *)psObject)->asWeaps[weapon_slot].nStat].surfaceToAir;
		}
		else
		{
			surfaceToAir = 0;
		}

		if ( ((surfaceToAir & SHOOT_IN_AIR) && bTargetInAir) || ((surfaceToAir & SHOOT_ON_GROUND) && !bTargetInAir) )
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

/* Make droid/structure look for a better target */
BOOL updateAttackTarget(BASE_OBJECT * psAttacker, SDWORD weapon_slot)
{
	BASE_OBJECT		*psBetterTarget=NULL;

	if(aiChooseTarget(psAttacker, &psBetterTarget, weapon_slot, true))	//update target
	{
		if(psAttacker->type == OBJ_DROID)
		{
			DROID *psDroid = (DROID *)psAttacker;

			if( (orderState(psDroid, DORDER_NONE) ||
				orderState(psDroid, DORDER_GUARD) ||
				orderState(psDroid, DORDER_ATTACKTARGET)) &&
				weapon_slot == 0)	//Watermelon:only primary slot(0) updates affect order
			{
				orderDroidObj((DROID *)psAttacker, DORDER_ATTACKTARGET, psBetterTarget);
			}
			else	//can't override current order
			{
				setDroidActionTarget(psDroid, psBetterTarget, weapon_slot);
			}
		}
		else if (psAttacker->type == OBJ_STRUCTURE)
		{
			STRUCTURE *psBuilding = (STRUCTURE *)psAttacker;

			setStructureTarget(psBuilding, psBetterTarget, weapon_slot);
		}

		return true;
	}

	return false;
}
