/*
 * AI.c
 *
 * AI update functions for the different object types.
 *
 */

/* Droid attack printf's */
#include "lib/framework/frame.h"
#include "objects.h"
#include "map.h"
#include "findpath.h"
#include "visibility.h"
#include "lib/gamelib/gtime.h"
#include "combat.h"
#include "hci.h"
#include "player.h"
#include "power.h"
#include "geometry.h"
#include "order.h"
#include "action.h"
#include "mapgrid.h"
#include "drive.h"
#include "projectile.h"
#include "cmddroid.h"
#include "group.h"

#include "multiplay.h"

// alliances
UBYTE	alliances[MAX_PLAYERS][MAX_PLAYERS];

/* alliance code for ai. return true if an alliance has formed. */
BOOL aiCheckAlliances(UDWORD s1,UDWORD s2)
{
    //features have their player number set to (MAX_PLAYERS + 1)
    if ( s1 == (MAX_PLAYERS + 1) || s2 == (MAX_PLAYERS + 1))
    {
        return FALSE;
    }
	if ((s1 == s2) ||
		(alliances[s1][s2] == ALLIANCE_FORMED))
	{
		return TRUE;
	}

	return FALSE;
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

	if (!playerInitialise())
	{
		return FALSE;
	}

	return TRUE;
}

/* Shutdown the AI system */
BOOL aiShutdown(void)
{
	playerShutDown();

	return TRUE;
}

// Find the nearest target to a droid
BOOL aiNearestTarget(DROID *psDroid, BASE_OBJECT **ppsObj)
{
	UDWORD			i;
	BASE_OBJECT		*psTarget, *psObj;
	BOOL			electronic = FALSE;

	if ((psDroid->id % 8) == (frameGetFrameNumber() % 8))
	{
		droidGetNaybors(psDroid);
	}
	else
	{
		return FALSE;
	}

    //don't bother looking if empty vtol droid
    if (vtolEmpty(psDroid))
    {
        return FALSE;
    }

	//electronic warfare can only be used against structures at present - not any more! AB 6/11/98
	electronic = electronicDroid(psDroid);
	psTarget = NULL;
	for (i=0; i< numNaybors; i++)
	{
		if (asDroidNaybors[i].psObj->player != psDroid->player &&
			(asDroidNaybors[i].psObj->type == OBJ_DROID ||
			 asDroidNaybors[i].psObj->type == OBJ_STRUCTURE) &&
			 asDroidNaybors[i].psObj->visible[psDroid->player] &&
			 !aiCheckAlliances(asDroidNaybors[i].psObj->player,psDroid->player))
		{
			psObj = asDroidNaybors[i].psObj;
			if (!validTarget((BASE_OBJECT *)psDroid, psObj))
			{
				continue;
			}
			else if (psObj->type == OBJ_DROID)
			{
                //in multiPlayer - don't attack Transporters with EW
                if (bMultiPlayer)
                {
                    //if not electronic then valid target
                    if (!electronic OR (electronic AND ((DROID *)psObj)->
                        droidType != DROID_TRANSPORTER))
                    {
                        //only a valid target if NOT a transporter
                        psTarget = psObj;
					    break;
                    }
                }
                else
				{
					psTarget = psObj;
					break;
                }
			}
			else if (psObj->type == OBJ_STRUCTURE)
			{
				if (electronic)
				{
					/*don't want to target structures with resistance of zero if
                    using electronic warfare*/
//					if (((STRUCTURE *)psObj)->pStructureType->resistance != 0)// AND
						//((STRUCTURE *)psObj)->resistance >= (SDWORD)(((STRUCTURE *)
						//psObj)->pStructureType->resistance))
						//((STRUCTURE *)psObj)->resistance >= (SDWORD)
						//structureResistance(((STRUCTURE *)psObj)->pStructureType,
						//psObj->player))
                    if (validStructResistance((STRUCTURE *)psObj))
					{
						psTarget = psObj;
						break;
					}
				}
				//else if (((STRUCTURE *)psObj)->numWeaps > 0)
                else if (((STRUCTURE *)psObj)->asWeaps[0].nStat > 0)
				{
					// structure with weapons - go for this
					psTarget = psObj;
					break;
				}
				else if ( (  ((STRUCTURE *)psObj)->pStructureType->type != REF_WALL
						   &&((STRUCTURE *)psObj)->pStructureType->type != REF_WALLCORNER
						  )
						 || driveModeActive()
						 || (bMultiPlayer && game.type == SKIRMISH && !isHumanPlayer(psDroid->player))
						)
				{
					psTarget = psObj;
				}
			}
		}
	}

	if (psTarget)
	{
		*ppsObj = psTarget;
		return TRUE;
	}

	return FALSE;
}


// see if a structure has the range to fire on a target
BOOL aiStructHasRange(STRUCTURE *psStruct, BASE_OBJECT *psTarget)
{
	WEAPON_STATS		*psWStats;
	SDWORD				xdiff,ydiff, longRange;

    if (psStruct->asWeaps[0].nStat == 0)
	{
		// Can't attack without a weapon
		return FALSE;
	}

	psWStats = psStruct->asWeaps[0].nStat + asWeaponStats;

	xdiff = (SDWORD)psStruct->x - (SDWORD)psTarget->x;
	ydiff = (SDWORD)psStruct->y - (SDWORD)psTarget->y;
	longRange = proj_GetLongRange(psWStats,0);
	if (xdiff*xdiff + ydiff*ydiff < longRange*longRange)
	{
		// in range
		return TRUE;

	}

	return FALSE;
}


// see if an object is a wall
BOOL aiObjIsWall(BASE_OBJECT *psObj)
{
	if (psObj->type != OBJ_STRUCTURE)
	{
		return FALSE;
	}

	if ( ((STRUCTURE *)psObj)->pStructureType->type != REF_WALL &&
		 ((STRUCTURE *)psObj)->pStructureType->type != REF_WALLCORNER )
	{
		return FALSE;
	}

	return TRUE;
}


/* See if there is a target in range */
BOOL aiChooseTarget(BASE_OBJECT *psObj,
					BASE_OBJECT **ppsTarget)
{
	UDWORD	radSquared;
//	UDWORD	player;
	BASE_OBJECT		*psCurr, *psTarget;
	SDWORD			xdiff,ydiff, distSq, tarDist, minDist;//, longRange;
	WEAPON_STATS	*psWStats;
	BOOL			bCBTower;
	STRUCTURE		*psCStruct;
	DROID			*psCommander;
	BOOL			bCommanderBlock;
	UDWORD			sensorRange;

	/* Get the sensor range */
	switch (psObj->type)
	{
	case OBJ_DROID:
		//if (((DROID *)psObj)->numWeaps == 0)
        if (((DROID *)psObj)->asWeaps[0].nStat == 0 &&
			((DROID *)psObj)->droidType != DROID_SENSOR)
		{
			// Can't attack without a weapon
			return FALSE;
		}
		radSquared = ((DROID *)psObj)->sensorRange *
					 ((DROID *)psObj)->sensorRange;
		break;
	case OBJ_STRUCTURE:
		//if (((STRUCTURE *)psObj)->numWeaps == 0)
        if (((STRUCTURE *)psObj)->asWeaps[0].nStat == 0)
		{
			// Can't attack without a weapon
			return FALSE;
		}
		sensorRange = ((STRUCTURE *)psObj)->sensorRange;

		// increase the sensor range for AA sites
		// AA sites are defensive structures that can only shoot in the air
		if ( (((STRUCTURE *)psObj)->pStructureType->type == REF_DEFENSE) &&
			 (asWeaponStats[((STRUCTURE *)psObj)->asWeaps[0].nStat].surfaceToAir == SHOOT_IN_AIR) )
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
		if (aiNearestTarget((DROID *)psObj, &psTarget))
		{
            /*check its a valid target*/
            if (validTarget(psObj, psTarget))
            {
    			/* See if in sensor range */
			    xdiff = psTarget->x - psObj->x;
			    ydiff = psTarget->y - psObj->y;
			    if (xdiff*xdiff + ydiff*ydiff < (SDWORD)radSquared)
			    {
				    *ppsTarget = psTarget;
				    return TRUE;
			    }
		    }
        }
	}
	else if (psObj->type == OBJ_STRUCTURE)
	{
		//ASSERT( ((STRUCTURE *)psObj)->numWeaps > 0,
        ASSERT( ((STRUCTURE *)psObj)->asWeaps[0].nStat > 0,
			"aiChooseTarget: no weapons on structure" );
		psWStats = ((STRUCTURE *)psObj)->asWeaps[0].nStat + asWeaponStats;


		// see if there is a target from the command droids
		psTarget = NULL;
		psCommander = cmdDroidGetDesignator(psObj->player);
		bCommanderBlock = FALSE;
		if (!proj_Direct(psWStats) && (psCommander != NULL) &&
			aiStructHasRange((STRUCTURE *)psObj, (BASE_OBJECT *)psCommander))
		{
			// there is a commander that can fire designate for this structure
			// set bCommanderBlock so that the structure does not fire until the commander
			// has a target - (slow firing weapons will not be ready to fire otherwise).
			bCommanderBlock = TRUE;
			if (psCommander->action == DACTION_ATTACK &&
				psCommander->psActionTarget != NULL)
			{
				// the commander has a target to fire on
				if (aiStructHasRange((STRUCTURE *)psObj, psCommander->psActionTarget))
				{
					// target in range - fire on it
					psTarget = psCommander->psActionTarget;
				}
				else
				{
					// target out of range - release the commander block
					bCommanderBlock = FALSE;
				}
			}
		}

		// indirect fire structures use sensor towers first
		tarDist = SDWORD_MAX;
		minDist = psWStats->minRange * psWStats->minRange;
		bCBTower = FALSE;
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

				if (!bCBTower &&
					structStandardSensor(psCStruct) &&
					psCStruct->psTarget != NULL)
				{
                    /*check its a valid target*/
                    if (validTarget(psObj, psCStruct->psTarget) &&
						aiStructHasRange((STRUCTURE *)psObj, psCStruct->psTarget))
                    {
					    xdiff = (SDWORD)psCStruct->psTarget->x - (SDWORD)psObj->x;
					    ydiff = (SDWORD)psCStruct->psTarget->y - (SDWORD)psObj->y;
					    distSq = xdiff*xdiff + ydiff*ydiff;
					    if ((distSq < tarDist) &&
						    (distSq > minDist))
					    {
						    tarDist = distSq;
						    psTarget = psCStruct->psTarget;
					    }
                    }
				}
				else if (structCBSensor(psCStruct) &&
						 psCStruct->psTarget != NULL)
				{
                    /*check its a valid target*/
                    if (validTarget(psObj, psCStruct->psTarget) &&
						aiStructHasRange((STRUCTURE *)psObj, psCStruct->psTarget))
                    {
    					xdiff = (SDWORD)psCStruct->psTarget->x - (SDWORD)psObj->x;
	    				ydiff = (SDWORD)psCStruct->psTarget->y - (SDWORD)psObj->y;
		    			distSq = xdiff*xdiff + ydiff*ydiff;
					    if ((!bCBTower || (distSq < tarDist)) &&
						    (distSq > minDist))
					    {
						    tarDist = distSq;
						    psTarget = psCStruct->psTarget;
						    bCBTower = TRUE;
					    }
                    }
				}
			}
		}

		if ((psTarget == NULL) &&
			!bCommanderBlock)
		{
			gridStartIterate((SDWORD)psObj->x, (SDWORD)psObj->y);
			psCurr = gridIterate();
			while (psCurr != NULL)
			{
				//don't target features
				if (psCurr->type != OBJ_FEATURE)
				{
					if (psObj->player != psCurr->player &&
						!aiCheckAlliances(psCurr->player,psObj->player))
					{
                        /*check its a valid target*/
                        if (validTarget(psObj, psCurr) &&
							!aiObjIsWall(psCurr))
                        {
						    // See if in sensor range and visible
						    xdiff = psCurr->x - psObj->x;
						    ydiff = psCurr->y - psObj->y;
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

/*		for(player = 0; player < MAX_PLAYERS; player++)
		{
			if (player != psObj->player && !aiCheckAlliances(player,psObj->player) )
			{
				for(psCurr = (BASE_OBJECT *)apsDroidLists[player]; psCurr;
					psCurr = psCurr->psNext)
				{
					// See if in sensor range and visible
					xdiff = psCurr->x - psObj->x;
					ydiff = psCurr->y - psObj->y;
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

		if (psTarget)
		{
			*ppsTarget = psTarget;
			return TRUE;
		}

		for(player = 0; player < MAX_PLAYERS; player++)
		{
			if (player != psObj->player  && !aiCheckAlliances(player,psObj->player) )
			{
				for(psCurr = (BASE_OBJECT *)apsStructLists[player]; psCurr;
					psCurr = psCurr->psNext)
				{
					// See if in sensor range and visible
					xdiff = psCurr->x - psObj->x;
					ydiff = psCurr->y - psObj->y;
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
		}*/

		if (psTarget)
		{
			*ppsTarget = psTarget;
			return TRUE;
		}
	}

	return FALSE;
}


/* See if there is a target in range for Sensor objects*/
BOOL aiChooseSensorTarget(BASE_OBJECT *psObj, BASE_OBJECT **ppsTarget)
{
	UDWORD	radSquared;
//	UDWORD	player;
	BASE_OBJECT		*psCurr, *psTarget;
	SDWORD	xdiff,ydiff, distSq, tarDist;
//    BOOL    bSuperSensor = FALSE;

	/* Get the sensor range */
	switch (psObj->type)
	{
	case OBJ_DROID:
		if (asSensorStats[((DROID *)psObj)->asBits[COMP_SENSOR].nStat].
			location != LOC_TURRET)
		{
			// to be used for Turret Sensors only
			return FALSE;
		}
		radSquared = ((DROID *)psObj)->sensorRange *
					 ((DROID *)psObj)->sensorRange;
		break;
	case OBJ_STRUCTURE:
		if (!(structStandardSensor((STRUCTURE *)psObj) OR
			structVTOLSensor((STRUCTURE *)psObj)))
		{
			// to be used for Standard and VTOL intercept Turret Sensors only
			return FALSE;
		}
		radSquared = ((STRUCTURE *)psObj)->sensorRange *
					 ((STRUCTURE *)psObj)->sensorRange;
        //AB 3/9/99 - the SUPER_SENSOR now uses the stat defined range - same as all other sensors
        //if we've got a SUPER sensor attached, then the whole of the map can be seen
        /*if (((STRUCTURE *)psObj)->pStructureType->pSensor->type == SUPER_SENSOR)
        {
            bSuperSensor = TRUE;
        }*/
		break;
	default:
		radSquared = 0;
		break;
	}

	/* See if there is a something in range */
	if (psObj->type == OBJ_DROID)
	{
		if (aiNearestTarget((DROID *)psObj, &psTarget))
		{
			/* See if in sensor range */
			xdiff = psTarget->x - psObj->x;
			ydiff = psTarget->y - psObj->y;
			if (xdiff*xdiff + ydiff*ydiff < (SDWORD)radSquared)
			{
				*ppsTarget = psTarget;
				return TRUE;
			}
		}
	}
	else
	{
        //AB 3/9/99 - the SUPER_SENSOR now uses the stat defined range - same as all other sensors
        //new for the SUPER sensor which can see EVERYWHERE!
        /*if (bSuperSensor)
        {
            DROID       *psDroid;
            STRUCTURE   *psStruct;
            UBYTE       player;

            tarDist = SDWORD_MAX;
            psTarget = NULL;
            // just go through the list of droids/structures for the oppositions
            // and get the nearest target. This might be REAL slow...
            for (player = 0; player < MAX_PLAYERS; player++)
            {
                //ignore the Sensor Structure's objects
                if ((player == psObj->player) OR (aiCheckAlliances(player,psObj->player)))
                {
                    continue;
                }
                for (psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
                {
                    //everything is visible!
					xdiff = psDroid->x - psObj->x;
					ydiff = psDroid->y - psObj->y;
					distSq = xdiff*xdiff + ydiff*ydiff;
                    //store if nearer
                    if (distSq < tarDist)
                    {
    					psTarget = (BASE_OBJECT *)psDroid;
	    				tarDist = distSq;
                    }
                }
                for (psStruct = apsStructLists[player]; psStruct; psStruct = psStruct->psNext)
                {
                    //everything is visible!
					xdiff = psStruct->x - psObj->x;
					ydiff = psStruct->y - psObj->y;
					distSq = xdiff*xdiff + ydiff*ydiff;
                    //store if nearer
                    if (distSq < tarDist)
                    {
    					psTarget = (BASE_OBJECT *)psStruct;
	    				tarDist = distSq;
                    }
                }
            }
        }
        else*/
        {
		    tarDist = SDWORD_MAX;
		    psTarget = NULL;
		    gridStartIterate((SDWORD)psObj->x, (SDWORD)psObj->y);
		    psCurr = gridIterate();
		    while (psCurr != NULL)
		    {
			    //don't target features
			    if (psCurr->type != OBJ_FEATURE)
			    {
				    if (psObj->player != psCurr->player &&
					    !aiCheckAlliances(psCurr->player,psObj->player) &&
					    !aiObjIsWall(psCurr))
				    {
					    // See if in sensor range and visible
					    xdiff = psCurr->x - psObj->x;
					    ydiff = psCurr->y - psObj->y;
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
			    psCurr = gridIterate();
		    }
        }

		if (psTarget)
		{
			*ppsTarget = psTarget;
			return TRUE;
		}
	}

	return FALSE;
}

/* Do the AI for a droid */
void aiUpdateDroid(DROID *psDroid)
{
	BASE_OBJECT	*psTarget;
	SECONDARY_STATE		state;
	BOOL		lookForTarget;
//	BOOL		bTemp;

	ASSERT( PTRVALID(psDroid, sizeof(DROID)),
		"updateUnitAI: invalid Unit pointer" );

	lookForTarget = TRUE;
	// don't look for a target if sulking
	if (psDroid->action == DACTION_SULK)
	{
		lookForTarget = FALSE;
	}
	// don't look for a target if doing something else
	if (!orderState(psDroid, DORDER_NONE) &&
		!orderState(psDroid, DORDER_GUARD))
	{
		lookForTarget = FALSE;
	}
	// don't look for a target if there are any queued orders
	if (psDroid->listSize > 0)
	{
		lookForTarget = FALSE;
	}
	// horrible check to stop droids looking for a target if
	// they would switch to the guard order in the order update loop
	if ((psDroid->order == DORDER_NONE) &&
		(psDroid->player == selectedPlayer) &&
		!vtolDroid(psDroid) &&
		secondaryGetState(psDroid, DSO_HALTTYPE, &state) &&
		(state == DSS_HALT_GUARD))
	{
		lookForTarget = FALSE;
	}

	// don't allow units to start attacking if they will switch to guarding the commander
	if (psDroid->droidType != DROID_COMMAND && psDroid->psGroup != NULL &&
		 psDroid->psGroup->type == GT_COMMAND)
	{
		lookForTarget = FALSE;
	}

	if(bMultiPlayer && vtolDroid(psDroid) && isHumanPlayer(psDroid->player))
	{
		lookForTarget = FALSE;
	}


	// do not choose another target if doing anything while guarding
	if (orderState(psDroid, DORDER_GUARD) &&
		(psDroid->action != DACTION_NONE))
	{
		lookForTarget = FALSE;
	}
    //do not look for a target if droid is an empty vtol
/*    if (vtolEmpty(psDroid))
    {
        lookForTarget = FALSE;
    }*/

	// do not look for a target if droid is currently under direct control.
	if(driveModeActive() && (psDroid == driveGetDriven())) {
		lookForTarget = FALSE;
	}

	// only computer senosr droids in the single player game aquire targets
	if ((psDroid->droidType == DROID_SENSOR && psDroid->player == selectedPlayer)
		&& !bMultiPlayer
		)
	{
		lookForTarget = FALSE;
	}

	// do not attack if the attack level is wrong
	if (secondaryGetState(psDroid, DSO_ATTACK_LEVEL, &state))
	{
		if (state != DSS_ALEV_ALWAYS)
		{
			lookForTarget = FALSE;
		}
	}

	/* Null target - see if there is an enemy to attack */
	if (lookForTarget &&
		aiChooseTarget((BASE_OBJECT *)psDroid, &psTarget))
	{
//			my_error("",0,"","Droid(%s) attacking : %d\n",
//					psDroid->pName, psTarget->id );


		turnOffMultiMsg(TRUE);
		if (psDroid->droidType == DROID_SENSOR)
		{
			orderDroidObj(psDroid, DORDER_OBSERVE, psTarget);
		}
		else
		{
			orderDroidObj(psDroid, DORDER_ATTACKTARGET, psTarget);
		}
			DBP1(("Unit(%s) attacking : %d\n",
					psDroid->pName, psTarget->id));
		turnOffMultiMsg(FALSE);

	}
}

/*set of rules which determine whether the weapon associated with the object
can fire on the propulsion type of the target*/
BOOL validTarget(BASE_OBJECT *psObject, BASE_OBJECT *psTarget)
{
	BOOL			bTargetInAir, bValidTarget;
    UBYTE           surfaceToAir;

    bValidTarget = FALSE;

    //need to check propulsion type of target
	switch (psTarget->type)
	{
	case OBJ_DROID:
        if (asPropulsionTypes[asPropulsionStats[((DROID *)psTarget)->asBits[
            COMP_PROPULSION].nStat].propulsionType].travel == AIR)
        {
			if (((DROID *)psTarget)->sMove.Status != MOVEINACTIVE)
			{
	            bTargetInAir = TRUE;
			}
			else
			{
	            bTargetInAir = FALSE;
			}
        }
        else
        {
            bTargetInAir = FALSE;
        }
        break;
    case OBJ_STRUCTURE:
	default:
        //lets hope so!
        bTargetInAir = FALSE;
        break;
    }

    //need what can shoot at
	switch (psObject->type)
	{
	case OBJ_DROID:
    	// Can't attack without a weapon
		//if (((DROID *)psObject)->numWeaps != 0)
        if (((DROID *)psObject)->asWeaps[0].nStat != 0)
		{
            surfaceToAir = asWeaponStats[((DROID *)psObject)->asWeaps[0].nStat].surfaceToAir;
        }
		else
		{
			 surfaceToAir = 0;
		}
		break;
	case OBJ_STRUCTURE:
        // Can't attack without a weapon
		//if (((STRUCTURE *)psObject)->numWeaps != 0)
        if (((STRUCTURE *)psObject)->asWeaps[0].nStat != 0)
		{
            surfaceToAir = asWeaponStats[((STRUCTURE *)psObject)->asWeaps[0].nStat].surfaceToAir;
        }
		else
		{
			surfaceToAir = 0;
		}
		break;
	default:
        surfaceToAir = 0;
		break;
	}

    //if target is in the air and you can shoot in the air - OK
    if (bTargetInAir AND (surfaceToAir & SHOOT_IN_AIR))
    {
        bValidTarget = TRUE;
    }

    //if target is on the ground and can shoot at it - OK
    if (!bTargetInAir AND (surfaceToAir & SHOOT_ON_GROUND))
    {
        bValidTarget = TRUE;
    }

    return bValidTarget;
}
