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
/*
 * Order.c
 *
 * Functions for setting the orders of a droid or group of droids
 *
 */
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/framework/math_ext.h"

#include "objects.h"
#include "order.h"
#include "action.h"
#include "map.h"
#include "geometry.h"
#include "projectile.h"
#include "effects.h"	// for waypoint display
#include "lib/gamelib/gtime.h"
#include "lib/netplay/netplay.h"
#include "intorder.h"
#include "orderdef.h"
#include "transporter.h"
#include "group.h"
#include "cmddroid.h"
#include "lib/script/script.h"
#include "scripttabs.h"
#include "scriptcb.h"

#include "multiplay.h"  //ajl

#include "mission.h"
#include "hci.h"
#include "visibility.h"
#include "display.h"
#include "ai.h"
#include "warcam.h"
#include "lib/sound/audio_id.h"
#include "lib/sound/audio.h"
#include "fpath.h"
#include "display3d.h"
#include "combat.h"
#include "console.h"

#include "random.h"

// how long to run for
#define RUN_TIME		8000
#define RUN_BURN_TIME	10000

// how far to move away from something that is being defended
#define DEFEND_MAXDIST		(TILE_UNITS * 3) //5)
#define DEFEND_BASEDIST		(TILE_UNITS * 3)
#define DEFEND_CMD_MAXDIST		(TILE_UNITS * 8)
#define DEFEND_CMD_BASEDIST		(TILE_UNITS * 5)

// how big an area for a repair droid to cover
#define REPAIR_MAXDIST		(TILE_UNITS * 5)
// how big an area for a constructor droid to cover
#define CONSTRUCT_MAXDIST		(TILE_UNITS * 8)

// how close to the target a droid has to be to be for DORDER_SCOUT to end
#define SCOUT_DIST			(TILE_UNITS * 8)

// how far a droid can wander when attacking during a DORDER_SCOUT
#define SCOUT_ATTACK_DIST	(TILE_UNITS * 5)

// retreat positions for the players
RUN_DATA	asRunData[MAX_PLAYERS];

// deal with a droid receiving a primary order
static BOOL secondaryGotPrimaryOrder(DROID *psDroid, DROID_ORDER order);

// check all the orders in the list for died objects
static void orderCheckList(DROID *psDroid);

// Clear all the orders from the list, up to listSize (without clearing pending (not yet synchronised) orders, that is).
static void orderClearDroidList(DROID *psDroid);

//Watermelon:add a timestamp to order circle
static UDWORD orderStarted;

// whether an order effect has been displayed
static BOOL bOrderEffectDisplayed = false;
// what the droid's action / order is currently
extern char DROIDDOING[512];
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////

//call this *AFTER* every mission so it gets reset
void initRunData(void)
{
    UBYTE   i;

    for (i = 0; i < MAX_PLAYERS; i++)
    {
        memset(&asRunData[i], 0, sizeof(RUN_DATA));
    }
}

//FIXME: unit doesn't shoot while returning to the guard position
// check whether a droid has to move back to the thing it is guarding
static void orderCheckGuardPosition(DROID *psDroid, SDWORD range)
{
	SDWORD		xdiff, ydiff;
	UDWORD		x,y;

	if (psDroid->psTarget != NULL)
	{
		// repair droids always follow behind - don't want them jumping into the line of fire
		if ((!(psDroid->droidType == DROID_REPAIR || psDroid->droidType == DROID_CYBORG_REPAIR))
		    && psDroid->psTarget->type == OBJ_DROID && orderStateLoc((DROID *)psDroid->psTarget, DORDER_MOVE, &x,&y))
		{
			// got a moving droid - check against where the unit is going
			psDroid->orderX = (UWORD)x;
			psDroid->orderY = (UWORD)y;
		}
		else
		{
			psDroid->orderX = psDroid->psTarget->pos.x;
			psDroid->orderY = psDroid->psTarget->pos.y;
		}
	}

	xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psDroid->orderX;
	ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psDroid->orderY;
	if (xdiff*xdiff + ydiff*ydiff > range*range)
	{
		if ((psDroid->sMove.Status != MOVEINACTIVE) &&
			((psDroid->action == DACTION_MOVE) ||
			 (psDroid->action == DACTION_MOVEFIRE)))
		{
			xdiff = (SDWORD)psDroid->sMove.DestinationX - (SDWORD)psDroid->orderX;
			ydiff = (SDWORD)psDroid->sMove.DestinationY - (SDWORD)psDroid->orderY;
			if (xdiff*xdiff + ydiff*ydiff > range*range)
			{
 				actionDroidLoc(psDroid, DACTION_MOVE, psDroid->orderX, psDroid->orderY);
			}
		}
		else
		{
			actionDroidLoc(psDroid, DACTION_MOVE, psDroid->orderX, psDroid->orderY);
		}
	}
}

/*For a given repair droid, check if there are any damaged droids within
a defined range*/
BASE_OBJECT * checkForRepairRange(DROID *psDroid,DROID *psTarget)
{
	DROID		*psCurr;

	ASSERT( psDroid->droidType == DROID_REPAIR || psDroid->droidType ==
        DROID_CYBORG_REPAIR, "checkForRepairRange:Invalid droid type" );

	if(psTarget != NULL
		&& psTarget->died)
	{
		psTarget = NULL;
	}

	// if guarding a unit - always check that first
	psCurr = (DROID*)orderStateObj(psDroid, DORDER_GUARD);
	if (psCurr != NULL
	 && psCurr->type == OBJ_DROID
	 && droidIsDamaged(psCurr))
	{
		return (BASE_OBJECT *)psCurr;
	}

	if ((psTarget != NULL) &&
		(psTarget->type == OBJ_DROID) &&
		(psTarget->player == psDroid->player))
	{
		psCurr = psTarget->psNext;
	}
	else
	{
		psCurr = apsDroidLists[psDroid->player];
	}
	for (; psCurr != NULL; psCurr = psCurr->psNext)
	{
		//check for damage
		if (droidIsDamaged(psCurr) && visibleObject((BASE_OBJECT *)psDroid, (BASE_OBJECT *)psCurr, false)
		    && droidSqDist(psDroid, (BASE_OBJECT *)psCurr) <
		       // Hold position? Repair range, else repair max dist
		       ((psDroid->order==DORDER_NONE && secondaryGetState(psDroid, DSO_HALTTYPE)==DSS_HALT_HOLD) ?
		        REPAIR_RANGE : REPAIR_MAXDIST*REPAIR_MAXDIST) )
		{
			return (BASE_OBJECT *)psCurr;
		}
	}
	return NULL;
}

/*For a given constructor droid, check if there are any damaged buildings within
a defined range*/
BASE_OBJECT * checkForDamagedStruct(DROID *psDroid, STRUCTURE *psTarget)
{
	STRUCTURE		*psCurr;

	ASSERT(psDroid->droidType == DROID_CONSTRUCT || psDroid->droidType == DROID_CYBORG_CONSTRUCT, "Invalid unit type");

	if(psTarget != NULL
		&& psTarget->died)
	{
		psTarget = NULL;
	}

	if ((psTarget != NULL) &&
		(psTarget->type == OBJ_STRUCTURE) &&
		(psTarget->player == psDroid->player))
	{
		psCurr = psTarget->psNext;
	}
	else
	{
		psCurr = apsStructLists[psDroid->player];
	}
	for (; psCurr != NULL; psCurr = psCurr->psNext)
	{
		//check for damage
		if (psCurr->status == SS_BUILT && structIsDamaged(psCurr) && !checkDroidsDemolishing(psCurr) 
		    && visibleObject((BASE_OBJECT *)psDroid, (BASE_OBJECT *)psCurr, false)
		    && droidSqDist(psDroid, (BASE_OBJECT *)psCurr) <
		       // Hold position? Repair range, else repair max dist
		       ((psDroid->order==DORDER_NONE && secondaryGetState(psDroid, DSO_HALTTYPE)==DSS_HALT_HOLD) ?
		        REPAIR_RANGE : REPAIR_MAXDIST*REPAIR_MAXDIST) )
		{
			return (BASE_OBJECT *)psCurr;
		}
	}
	return NULL;
}

/* Update a droids order state */
void orderUpdateDroid(DROID *psDroid)
{
	BASE_OBJECT		*psObj;
	STRUCTURE		*psStruct, *psWall;
	REPAIR_FACILITY	*psRepairFac;
	SDWORD			xdiff,ydiff;
	BOOL			bAttack;
	UBYTE i;
	SDWORD			xoffset,yoffset;

	// clear the target if it has died
	if (psDroid->psTarget && psDroid->psTarget->died)
	{
		setDroidTarget(psDroid, NULL);
	}

	//clear its base struct if its died
	if (psDroid->psBaseStruct && psDroid->psBaseStruct->died)
	{
		setDroidBase(psDroid, NULL);
	}

	// check for died objects in the list
	orderCheckList(psDroid);

	if (isDead((BASE_OBJECT *)psDroid))
	{
		return;
	}

	switch (psDroid->order)
	{
	case DORDER_NONE:
	case DORDER_TEMP_HOLD:
		psObj = NULL;
		// see if there are any orders queued up
		if (orderDroidList(psDroid))
		{
			// started a new order, quit
			break;
		}
		// if you are in a command group, default to guarding the commander
		else if (hasCommander(psDroid) &&
				 (psDroid->psTarStats != (BASE_STATS *) structGetDemolishStat()))  // stop the constructor auto repairing when it is about to demolish
		{
			orderDroidObj(psDroid, DORDER_GUARD, (BASE_OBJECT *)psDroid->psGroup->psCommander, ModeImmediate);
		}
		else if (psDroid->droidType == DROID_TRANSPORTER && !bMultiPlayer)
		{
			//check transporter isn't sitting there waiting to be filled when nothing exists!
			if (psDroid->player == selectedPlayer && getDroidsToSafetyFlag()
			    && !missionDroidsRemaining(selectedPlayer))
			{
				// check that nothing is on the transporter (transporter counts as first in group)
				if (psDroid->psGroup && psDroid->psGroup->refCount < 2)
				{
					// the script can call startMission for this callback for offworld missions
					eventFireCallbackTrigger((TRIGGER_TYPE)CALL_START_NEXT_LEVEL);
				}
			}
		}

		//repair droids default to repairing droids within a given range
		else if ((psDroid->droidType == DROID_REPAIR || psDroid->droidType == DROID_CYBORG_REPAIR)
		         && !orderState(psDroid, DORDER_GUARD))
		{
			psObj = NULL;
			if (psDroid->action == DACTION_NONE)
			{
				psObj = checkForRepairRange(psDroid,NULL);
			}
			else if (psDroid->action == DACTION_SULK)
			{
				psObj = checkForRepairRange(psDroid,(DROID *)psDroid->psActionTarget[0]);
			}
			if (psObj)
			{
				actionDroidObj(psDroid, DACTION_DROIDREPAIR, (BASE_OBJECT *)psObj);
			}
		}

		//construct droids default to repairing structures within a given range
		else if ((psDroid->droidType == DROID_CONSTRUCT || psDroid->droidType == DROID_CYBORG_CONSTRUCT) &&
		         !orderState(psDroid, DORDER_GUARD) &&
		         (psDroid->psTarStats != (BASE_STATS *) structGetDemolishStat()))
		{
			psObj = NULL;
			if (psDroid->action == DACTION_NONE)
			{
				psObj = checkForDamagedStruct(psDroid,NULL);
			}
			else if (psDroid->action == DACTION_SULK)
			{
				psObj = checkForDamagedStruct(psDroid,(STRUCTURE *)psDroid->psActionTarget);
			}
			if (psObj)
			{
				actionDroidObj(psDroid, DACTION_REPAIR, psObj);
			}
		}

		// default to guarding if the correct secondary order is set
		else if (psDroid->psTarStats != (BASE_STATS *)structGetDemolishStat() && // stop the constructor auto repairing when it is about to demolish
		         secondaryGetState(psDroid, DSO_HALTTYPE) == DSS_HALT_GUARD &&
		         !isVtolDroid(psDroid))
		{
			orderDroidLoc(psDroid, DORDER_GUARD, psDroid->pos.x, psDroid->pos.y, ModeImmediate);
		}

		break;
	case DORDER_TRANSPORTRETURN:
		if (psDroid->action == DACTION_NONE)
		{
			missionMoveTransporterOffWorld( psDroid );

			/* clear order */
			psDroid->order = DORDER_NONE;
			setDroidTarget(psDroid, NULL);
			psDroid->psTarStats = NULL;
		}
		break;
	case DORDER_TRANSPORTOUT:
		if (psDroid->action == DACTION_NONE)
		{
			//if moving droids to safety and still got some droids left don't do callback
			if (psDroid->player == selectedPlayer && getDroidsToSafetyFlag() &&
				missionDroidsRemaining(selectedPlayer))
			{
				//move droids in Transporter into holding list
				moveDroidsToSafety(psDroid);
				//we need the transporter to just sit off world for a while...
				orderDroid(psDroid, DORDER_TRANSPORTIN, ModeImmediate);
				/* set action transporter waits for timer */
				actionDroid( psDroid, DACTION_TRANSPORTWAITTOFLYIN );

				missionSetReinforcementTime( gameTime );

				//don't do this until waited for the required time
				//fly Transporter back to get some more droids
				//orderDroidLoc( psDroid, DORDER_TRANSPORTIN,
				//    getLandingX(selectedPlayer), getLandingY(selectedPlayer));
			}
			else
			{
			    //the script can call startMission for this callback for offworld missions
			    eventFireCallbackTrigger((TRIGGER_TYPE)CALL_START_NEXT_LEVEL);

			    /* clear order */
			    psDroid->order = DORDER_NONE;
				setDroidTarget(psDroid, NULL);
			    psDroid->psTarStats = NULL;
            }
		}
		break;
	case DORDER_TRANSPORTIN:
		if ( (psDroid->action == DACTION_NONE) &&
			 (psDroid->sMove.Status == MOVEINACTIVE) )
		{
			/* clear order */
			psDroid->order = DORDER_NONE;
			setDroidTarget(psDroid, NULL);
			psDroid->psTarStats = NULL;

//FFS! You only wan't to do this if the droid being tracked IS the transporter! Not all the time!
// What about if your happily playing the game and tracking a droid, and re-enforcements come in!
// And suddenly BLAM!!!! It drops you out of camera mode for no apparent reason! TOTALY CONFUSING
// THE PLAYER!
//
// Just had to get that off my chest....end of rant.....
//
			if( psDroid == getTrackingDroid() ) { // Thats better...
				/* deselect transporter if have been tracking */
	    		if ( getWarCamStatus() )
		    	{
			    	camToggleStatus();
			    }
			}

			DeSelectDroid(psDroid);

            /*don't try the unload if moving droids to safety and still got some
            droids left  - wait until full and then launch again*/
            if (psDroid->player == selectedPlayer && getDroidsToSafetyFlag() &&
                missionDroidsRemaining(selectedPlayer))
            {
                resetTransporter();
            }
            else
            {
			    unloadTransporter( psDroid, psDroid->pos.x, psDroid->pos.y, false );
            }
		}
		break;
	case DORDER_MOVE:
	case DORDER_RETREAT:
	case DORDER_DESTRUCT:
		// Just wait for the action to finish then clear the order
		if (psDroid->action == DACTION_NONE || psDroid->action == DACTION_ATTACK)
		{
			psDroid->order = DORDER_NONE;
			setDroidTarget(psDroid, NULL);
			psDroid->psTarStats = NULL;
		}
		break;
	case DORDER_RECOVER:
		if (psDroid->psTarget == NULL)
		{
			psDroid->order = DORDER_NONE;
		}
		else if (psDroid->action == DACTION_NONE)
		{
			// stoped moving, but still havn't got the artifact
			actionDroidLoc(psDroid, DACTION_MOVE, psDroid->psTarget->pos.x,psDroid->psTarget->pos.y);
		}
		break;
	case DORDER_MOVE_ATTACKWALL:
	case DORDER_SCOUT_ATTACKWALL:
		//Watermelon:check against all weapons now
		for(i = 0;i <psDroid->numWeaps;i++)
		{
			if (psDroid->psTarget == NULL)
			{
				if (psDroid->order == DORDER_MOVE_ATTACKWALL)
				{
					psDroid->order = DORDER_MOVE;
				}
				else
				{
					psDroid->order = DORDER_SCOUT;
				}
				actionDroidLoc(psDroid, DACTION_MOVE, psDroid->orderX,psDroid->orderY);
			}
			else if ((((psDroid->action != DACTION_ATTACK) &&
					   (psDroid->action != DACTION_MOVETOATTACK) &&
					   (psDroid->action != DACTION_ROTATETOATTACK)) ||
					  (psDroid->psActionTarget[0] != psDroid->psTarget)) &&
					 actionInRange(psDroid, psDroid->psTarget, 0) )
			{
				actionDroidObj(psDroid, DACTION_ATTACK, psDroid->psTarget);
			}
			else if (psDroid->action == DACTION_NONE)
			{
				if (psDroid->order == DORDER_SCOUT_ATTACKWALL)
				{
					psDroid->order = DORDER_SCOUT;
				}
				actionDroidLoc(psDroid, DACTION_MOVE, psDroid->psTarget->pos.x, psDroid->psTarget->pos.y);
			}
		}
		break;
	case DORDER_SCOUT:
	case DORDER_PATROL:
		// if there is an enemy around, attack it
		if (psDroid->action == DACTION_MOVE || (psDroid->action == DACTION_NONE && isVtolDroid(psDroid)))
		{
			bool tooFarFromPath = false;
			if (isVtolDroid(psDroid) && psDroid->order == DORDER_PATROL)
			{
				// Don't stray too far from the patrol path - only attack if we're near it
				// A fun algorithm to detect if we're near the path
				SDWORD deltaX, deltaY;
				deltaX = (SDWORD)psDroid->orderX - (SDWORD)psDroid->orderX2;
				deltaY = (SDWORD)psDroid->orderY - (SDWORD)psDroid->orderY2;
				if (deltaX == 0 && deltaY == 0)
				{
					tooFarFromPath = false;
				}
				else if (abs(deltaX) >= abs(deltaY) &&
				    (SDWORD)MIN(psDroid->orderX, psDroid->orderX2)-SCOUT_DIST <= psDroid->pos.x &&
				    psDroid->pos.x <= (SDWORD)MAX(psDroid->orderX, psDroid->orderX2)+SCOUT_DIST)
				{
					tooFarFromPath = (abs(((SDWORD)psDroid->pos.x - (SDWORD)psDroid->orderX) * deltaY/deltaX +
					                      (SDWORD)psDroid->orderY - (SDWORD)psDroid->pos.y) > SCOUT_DIST);
				}
				else if (abs(deltaX) <= abs(deltaY) &&
				    (SDWORD)MIN(psDroid->orderY, psDroid->orderY2)-SCOUT_DIST <= psDroid->pos.y &&
				    psDroid->pos.y <= (SDWORD)MAX(psDroid->orderY, psDroid->orderY2)+SCOUT_DIST)
				{
					tooFarFromPath = (abs(((SDWORD)psDroid->pos.y - (SDWORD)psDroid->orderY) * deltaX/deltaY +
					                      (SDWORD)psDroid->orderX - (SDWORD)psDroid->pos.x) > SCOUT_DIST);
				}
				else
				{
					tooFarFromPath = true;
				}
			}
			if (!tooFarFromPath &&
			    (secondaryGetState(psDroid, DSO_ATTACK_LEVEL) == DSS_ALEV_ALWAYS) &&
			    (aiBestNearestTarget(psDroid, &psObj, 0, NULL) >= 0))
			{
				switch (psDroid->droidType)
				{
				case DROID_WEAPON:
				case DROID_CYBORG:
				case DROID_CYBORG_SUPER:
				case DROID_PERSON:
				case DROID_COMMAND:
					actionDroidObj(psDroid, DACTION_ATTACK, psObj);
					break;
				case DROID_SENSOR:
					actionDroidObj(psDroid, DACTION_OBSERVE, psObj);
					break;
				default:
					actionDroid(psDroid, DACTION_NONE);
					break;
				}
			}
		}
		if (psDroid->action == DACTION_NONE)
		{
			xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psDroid->orderX;
			ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psDroid->orderY;
			if (xdiff*xdiff + ydiff*ydiff < SCOUT_DIST*SCOUT_DIST)
			{
				if (psDroid->order == DORDER_PATROL)
				{
					UDWORD tempCoord;
					// see if we have anything queued up
					if (orderDroidList(psDroid))
					{
						// started a new order, quit
						break;
					}
					if (isVtolDroid(psDroid) && !vtolFull(psDroid))
					{
						moveToRearm(psDroid);
						break;
					}
					// head back to the other point
					tempCoord = psDroid->orderX;
					psDroid->orderX = psDroid->orderX2;
					psDroid->orderX2 = tempCoord;
					tempCoord = psDroid->orderY;
					psDroid->orderY = psDroid->orderY2;
					psDroid->orderY2 = tempCoord;
					actionDroidLoc(psDroid, DACTION_MOVE, psDroid->orderX,psDroid->orderY);
				}
				else
				{
					psDroid->order = DORDER_NONE;
				}
			}
			else
			{
				actionDroidLoc(psDroid, DACTION_MOVE, psDroid->orderX,psDroid->orderY);
			}
		}
		else if ((psDroid->action == DACTION_ATTACK) ||
				 (psDroid->action == DACTION_MOVETOATTACK) ||
				 (psDroid->action == DACTION_ROTATETOATTACK) ||
				 (psDroid->action == DACTION_OBSERVE) ||
				 (psDroid->action == DACTION_MOVETOOBSERVE))
		{
			// attacking something - see if the droid has gone too far
			xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psDroid->actionX;
			ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psDroid->actionY;
			if (xdiff*xdiff + ydiff*ydiff > SCOUT_ATTACK_DIST*SCOUT_ATTACK_DIST)
			{
				actionDroidLoc(psDroid, DACTION_RETURNTOPOS, psDroid->actionX,psDroid->actionY);
			}
		}
		break;
	case DORDER_CIRCLE:
		// if there is an enemy around, attack it
		if (psDroid->action == DACTION_MOVE &&
		    secondaryGetState(psDroid, DSO_ATTACK_LEVEL) == DSS_ALEV_ALWAYS &&
		    aiBestNearestTarget(psDroid, &psObj, 0, NULL) >= 0)
		{
			switch (psDroid->droidType)
			{
			case DROID_WEAPON:
			case DROID_CYBORG:
			case DROID_CYBORG_SUPER:
			case DROID_PERSON:
			case DROID_COMMAND:
				actionDroidObj(psDroid, DACTION_ATTACK, psObj);
				break;
			case DROID_SENSOR:
				actionDroidObj(psDroid, DACTION_OBSERVE, psObj);
				break;
			default:
				actionDroid(psDroid, DACTION_NONE);
				break;
			}
		}
		else if (psDroid->action == DACTION_NONE || psDroid->action == DACTION_MOVE)
		{
			if (psDroid->action == DACTION_MOVE)
			{
				if ( orderStarted && ((orderStarted + 500) > gameTime) )
				{
					break;
				}
				// see if we have anything queued up
				if (orderDroidList(psDroid))
				{
					// started a new order, quit
					break;
				}
				orderStarted = gameTime;
			}
			psDroid->action = DACTION_NONE;

			xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psDroid->orderX;
			ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psDroid->orderY;
			if (xdiff*xdiff + ydiff*ydiff <= 2000 * 2000)
			{
				if (psDroid->order == DORDER_CIRCLE)
				{
					//Watermelon:use orderX,orderY as local space origin and calculate droid direction in local space
					uint16_t angle = iAtan2(xdiff, ydiff);
					xoffset = iSinR(angle, 1500);
					yoffset = iCosR(angle, 1500);
					xdiff = psDroid->pos.x - (psDroid->orderX + xoffset);
					ydiff = psDroid->pos.y - (psDroid->orderY + yoffset);
					if (xdiff*xdiff + ydiff*ydiff < TILE_UNITS * TILE_UNITS)
					{
						//Watermelon:conter-clockwise 30 degree's per action
						angle -= DEG(30);
						xoffset = iSinR(angle, 1500);
						yoffset = iCosR(angle, 1500);
					}
					actionDroidLoc(psDroid, DACTION_MOVE, psDroid->orderX + xoffset, psDroid->orderY + yoffset);
				}
				else
				{
					psDroid->order = DORDER_NONE;
				}
			}
		}
		else if ((psDroid->action == DACTION_ATTACK) ||
				 (psDroid->action == DACTION_MOVETOATTACK) ||
				 (psDroid->action == DACTION_ROTATETOATTACK) ||
				 (psDroid->action == DACTION_OBSERVE) ||
				 (psDroid->action == DACTION_MOVETOOBSERVE))
		{
			// attacking something - see if the droid has gone too far
			xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psDroid->actionX;
			ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psDroid->actionY;
			//if (xdiff*xdiff + ydiff*ydiff > psDroid->sMove.iGuardRadius * psDroid->sMove.iGuardRadius)
			if (xdiff*xdiff + ydiff*ydiff > 2000 * 2000)
			{
				// head back to the target location
				actionDroidLoc(psDroid, DACTION_RETURNTOPOS, psDroid->orderX,psDroid->orderY);
			}
		}
		break;
	case DORDER_HELPBUILD:
	case DORDER_DEMOLISH:
	case DORDER_OBSERVE:
	case DORDER_REPAIR:
	case DORDER_DROIDREPAIR:
	case DORDER_RESTORE:
	case DORDER_CLEARWRECK:
		if (psDroid->action == DACTION_NONE ||
			psDroid->psTarget == NULL)
		{
			psDroid->order = DORDER_NONE;
			actionDroid(psDroid, DACTION_NONE);
			if (psDroid->player == selectedPlayer)
			{
				intRefreshScreen();
			}
		}
/*		if(psDroid->droidType == DROID_REPAIR
			&& psDroid->action == DACTION_SULK)
		{
			psObj = checkForRepairRange(psDroid,(DROID *)psDroid->psTarget);
			if (psObj)
			{
				orderDroidObj(psDroid, DORDER_DROIDREPAIR, psObj);
				break;
			}
		}*/
		break;
	case DORDER_REARM:
		if ((psDroid->psTarget == NULL) ||
			(psDroid->psActionTarget[0] == NULL) )
		{
			// arm pad destroyed find another
			psDroid->order = DORDER_NONE;
			moveToRearm(psDroid);
		}
		else if (psDroid->action == DACTION_NONE)
		{
			psDroid->order = DORDER_NONE;
		}
		break;
	case DORDER_ATTACK:
	case DORDER_ATTACKTARGET:
		if (psDroid->psTarget == NULL || psDroid->psTarget->died)
		{
			// if vtol then return to rearm pad as long as there are no other
			// orders queued up
			if (isVtolDroid(psDroid))
			{
				if (!orderDroidList(psDroid))
				{
					psDroid->order = DORDER_NONE;
					moveToRearm(psDroid);
					if (!vtolEmpty(psDroid))
					{
						// VTOL droid can do more work, let scripts handle it
						psScrVtolRetarget = psDroid;
						eventFireCallbackTrigger((TRIGGER_TYPE)CALL_VTOL_RETARGET);
						psScrVtolRetarget = NULL;
					}
				}
			}
			else
			{
				psDroid->order = DORDER_NONE;
				actionDroid(psDroid, DACTION_NONE);
			}
		}
		else if ( ((psDroid->action == DACTION_MOVE) ||
				   (psDroid->action == DACTION_MOVEFIRE)) &&
				   actionVisibleTarget(psDroid, psDroid->psTarget, 0) && !isVtolDroid(psDroid))
		{
			// moved near enough to attack change to attack action
			actionDroidObj(psDroid, DACTION_ATTACK, psDroid->psTarget);
		}
		else if ( (psDroid->action == DACTION_MOVETOATTACK) &&
				  !isVtolDroid(psDroid) &&
				  !actionVisibleTarget(psDroid, psDroid->psTarget, 0) )
		{
			// lost sight of the target while chasing it - change to a move action so
			// that the unit will fire on other things while moving
			actionDroidLoc(psDroid, DACTION_MOVE, psDroid->psTarget->pos.x, psDroid->psTarget->pos.y);
		}
		else if (!isVtolDroid(psDroid)
		      && psDroid->psTarget == psDroid->psActionTarget[0]
		      && actionInRange(psDroid, psDroid->psTarget, 0)
		      && (psWall = visGetBlockingWall((BASE_OBJECT *)psDroid, psDroid->psTarget))
		      && psWall->player != psDroid->player)
		{
			// there is a wall in the way - attack that
			actionDroidObj(psDroid, DACTION_ATTACK, (BASE_OBJECT *)psWall);
		}
		else if ((psDroid->action == DACTION_NONE) ||
				 (psDroid->action == DACTION_CLEARREARMPAD))
		{
			if ((psDroid->order == DORDER_ATTACKTARGET) &&
				secondaryGetState(psDroid, DSO_HALTTYPE) == DSS_HALT_HOLD &&
				!actionInRange(psDroid, psDroid->psTarget, 0) )
			{
				// on hold orders give up
				psDroid->order = DORDER_NONE;
				setDroidTarget(psDroid, NULL);
			}
			else if (!isVtolDroid(psDroid) ||
				allVtolsRearmed(psDroid))
			{
				actionDroidObj(psDroid, DACTION_ATTACK, psDroid->psTarget);
			}
		}
		break;
	case DORDER_BUILD:
		if (psDroid->action == DACTION_BUILD &&
			psDroid->psTarget == NULL)
		{
			psDroid->order = DORDER_NONE;
			actionDroid(psDroid, DACTION_NONE);
		}
		else if (psDroid->action == DACTION_NONE)
		{
			psDroid->order = DORDER_NONE;
		}
		break;
	case DORDER_EMBARK:
		{
			// only place it can be trapped - in multiPlayer can only put cyborgs onto a Cyborg Transporter
			DROID *temp = NULL;

			temp = (DROID*)psDroid->psTarget;
			if (!strcmp("Cyborg Transport", temp->aName) && !cyborgDroid(psDroid))
			{
				// NOTE: since we only have one type of transport (DROID_TRANSPORT), it isn't worth changing tons of code
				// to have two types available (DROID_TRANSPORT_SUPER), so we just check the name which can never be
				// renamed anyway, so we should be safe with this kludge.
				psDroid->order = DORDER_NONE;
				actionDroid(psDroid, DACTION_NONE);
				audio_PlayTrack( ID_SOUND_BUILD_FAIL );
				addConsoleMessage(_("We can't do that! We must be a Cyborg unit to use a Cyborg Transport!"), DEFAULT_JUSTIFY, selectedPlayer);
			}
			else
			{
				// Wait for the action to finish then assign to Transporter (if not already flying)
				if (psDroid->psTarget == NULL || transporterFlying((DROID *)psDroid->psTarget))
				{
					psDroid->order = DORDER_NONE;
					actionDroid(psDroid, DACTION_NONE);
				}
				else if (abs((SDWORD)psDroid->pos.x - (SDWORD)psDroid->psTarget->pos.x) < TILE_UNITS
					&& abs((SDWORD)psDroid->pos.y - (SDWORD)psDroid->psTarget->pos.y) < TILE_UNITS)
				{
					// save the target of current droid (the transporter)
					DROID * transporter = (DROID *)psDroid->psTarget;

					// Make sure that it really is a valid droid
					CHECK_DROID(transporter);

					// order the droid to stop so moveUpdateDroid does not process this unit
					orderDroid(psDroid, DORDER_STOP, ModeImmediate);
					setDroidTarget(psDroid, NULL);
					psDroid->psTarStats = NULL;
					secondarySetState(psDroid, DSO_RETURN_TO_LOC, DSS_NONE);

					/* We must add the droid to the transporter only *after*
					* processing changing its orders (see above).
					*/
					transporterAddDroid(transporter, psDroid);
				}
				else if (psDroid->action == DACTION_NONE)
				{
					actionDroidLoc(psDroid, DACTION_MOVE, psDroid->psTarget->pos.x,psDroid->psTarget->pos.y);
				}
			}
		}
		// Do we need to clear the secondary order "DSO_EMBARK" here? (PD)
		break;
    case DORDER_DISEMBARK:
        //only valid in multiPlayer mode
        if (bMultiPlayer)
        {
            //this order can only be given to Transporter droids
            if (psDroid->droidType == DROID_TRANSPORTER)
            {
                /*once the Transporter has reached its destination (and landed),
                get all the units to disembark*/
                if (psDroid->action != DACTION_MOVE && psDroid->action != DACTION_MOVEFIRE &&
				    psDroid->sMove.Status == MOVEINACTIVE && psDroid->sMove.iVertSpeed == 0)
                {
				unloadTransporter(psDroid, psDroid->pos.x, psDroid->pos.y, false);
                    //reset the transporter's order
                    psDroid->order = DORDER_NONE;
                }
            }
        }
        break;
	case DORDER_RTB:
		// Just wait for the action to finish then clear the order
		if (psDroid->action == DACTION_NONE)
		{
			psDroid->order = DORDER_NONE;
			secondarySetState(psDroid, DSO_RETURN_TO_LOC, DSS_NONE);
		}
		break;
	case DORDER_LEAVEMAP:
		if ((psDroid->pos.x < TILE_UNITS*2) ||
			(psDroid->pos.x > (mapWidth-2)*TILE_UNITS) ||
			(psDroid->pos.y < TILE_UNITS*2) ||
			(psDroid->pos.y > (mapHeight-2)*TILE_UNITS) ||
			(psDroid->action == DACTION_NONE))
		{
			psDroid->order = DORDER_NONE;
			psScrCBVtolOffMap = psDroid;
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_VTOL_OFF_MAP);
		}
		break;
	case DORDER_RTR:
	case DORDER_RTR_SPECIFIED:
		if (psDroid->psTarget == NULL)
		{
			// Our target got lost. Let's try again.
			psDroid->order = DORDER_NONE;
			orderDroid(psDroid, DORDER_RTR, ModeImmediate);
		}
		else if (psDroid->action == DACTION_NONE)
		{
			/* get repair facility pointer */
			psStruct = (STRUCTURE *) psDroid->psTarget;
			ASSERT( psStruct != NULL,
				"orderUpdateUnit: invalid structure pointer" );
			psRepairFac = (REPAIR_FACILITY *) psStruct->pFunctionality;
			ASSERT( psRepairFac != NULL,
				"orderUpdateUnit: invalid repair facility pointer" );

			if (objPosDiffSq(psDroid->pos, psDroid->psTarget->pos) < (TILE_UNITS * 8) * (TILE_UNITS * 8))
			{
				/* action droid to wait */
				actionDroid(psDroid, DACTION_WAITFORREPAIR);
			}
			else
			{
				// move the droid closer to the repair facility
				actionDroidLoc(psDroid, DACTION_MOVE, psDroid->psTarget->pos.x, psDroid->psTarget->pos.y);
			}
		}
		else if (psDroid->order == DORDER_RTR &&
				 (psDroid->action == DACTION_MOVE ||
				  psDroid->action == DACTION_MOVEFIRE) &&
				  ((psDroid->id + gameTime)/GAME_TICKS_PER_SEC != (psDroid->id + gameTime - deltaGameTime)/GAME_TICKS_PER_SEC))
		{
			// see if there is a repair facility nearer than the one currently selected
			orderDroid(psDroid, DORDER_RTR, ModeImmediate);
		}
		break;
	case DORDER_RUNBURN:
		if (psDroid->actionStarted + RUN_BURN_TIME < gameTime)
		{
			debug(LOG_DEATH, "orderUpdateDroid: Droid %d burned to death", psDroid->id); // why is this an order?
			destroyDroid( psDroid );
		}
		break;
	case DORDER_RUN:
		if (psDroid->action == DACTION_NONE)
		{
			// got there so stop running
			psDroid->order = DORDER_NONE;
		}
		if (psDroid->actionStarted + RUN_TIME < gameTime)
		{
			// been running long enough
			actionDroid(psDroid, DACTION_NONE);
			psDroid->order = DORDER_NONE;
		}
		break;
	case DORDER_LINEBUILD:
		if (psDroid->action == DACTION_NONE ||
			(psDroid->action == DACTION_BUILD && psDroid->psTarget == NULL))
		{
			// finished building the current structure
			if (map_coord(psDroid->orderX) == map_coord(psDroid->orderX2)
			 && map_coord(psDroid->orderY) == map_coord(psDroid->orderY2))
			{
				// finished all the structures - done
				psDroid->order = DORDER_NONE;
				setDroidTarget(psDroid, NULL);
				psDroid->psTarStats = NULL;
				break;
			}

			// update the position for another structure
			if (map_coord(psDroid->orderX) == map_coord(psDroid->orderX2))
			{
				// still got building to do - working vertically
				if (psDroid->orderY < psDroid->orderY2)
				{
					psDroid->orderY += TILE_UNITS;
				}
				else
				{
					psDroid->orderY -= TILE_UNITS;
				}
			}
			else if (map_coord(psDroid->orderY) == map_coord(psDroid->orderY2))
			{
				// still got building to do - working horizontally
				if (psDroid->orderX < psDroid->orderX2)
				{
					psDroid->orderX += TILE_UNITS;
				}
				else
				{
					psDroid->orderX -= TILE_UNITS;
				}
			}
			else
			{
				ASSERT( false, "orderUpdateUnit: LINEBUILD order on diagonal line" );
				break;
			}

			// build another structure
			setDroidTarget(psDroid, NULL);
			actionDroidLoc(psDroid, DACTION_BUILD, psDroid->orderX,psDroid->orderY);
			//intRefreshScreen();
		}
		break;
	case DORDER_FIRESUPPORT:
		if (psDroid->psTarget == NULL)
		{
			psDroid->order = DORDER_NONE;
			if (isVtolDroid(psDroid))
			{
				moveToRearm(psDroid);
			}
			else
			{
				actionDroid(psDroid, DACTION_NONE);
			}
		}
		//before targetting - check VTOL's are fully armed
		else if (vtolEmpty(psDroid))
		{
			moveToRearm(psDroid);
		}
		//indirect weapon droid attached to (standard)sensor droid
		else
		{
			BASE_OBJECT	*psFireTarget = NULL;

			if (psDroid->psTarget->type == OBJ_DROID)
			{
				DROID	*psSpotter = (DROID *)psDroid->psTarget;

				if (psSpotter->action == DACTION_OBSERVE
				    || (psSpotter->droidType == DROID_COMMAND && psSpotter->action == DACTION_ATTACK))
				{
					psFireTarget = psSpotter->psActionTarget[0];
				}
			}
			else if (psDroid->psTarget->type == OBJ_STRUCTURE)
			{
				STRUCTURE *psSpotter = (STRUCTURE *)psDroid->psTarget;

				psFireTarget = psSpotter->psTarget[0];
			}

			if (psFireTarget && !psFireTarget->died)
			{
				bAttack = false;
				if (isVtolDroid(psDroid))
				{
					if (!vtolEmpty(psDroid) &&
						((psDroid->action == DACTION_MOVETOREARM) ||
						 (psDroid->action == DACTION_WAITFORREARM)) &&
						(psDroid->sMove.Status != MOVEINACTIVE) )
					{
						// catch vtols that were attacking another target which was destroyed
						// get them to attack the new target rather than returning to rearm
						bAttack = true;
					}
					else if (allVtolsRearmed(psDroid))
					{
						bAttack = true;
					}
				}
				else
				{
					bAttack = true;
				}

				//if not currently attacking or target has changed
				if ( bAttack &&
					(!droidAttacking(psDroid) ||
					 psFireTarget != psDroid->psActionTarget[0]))
				{
					//get the droid to attack
					actionDroidObj(psDroid, DACTION_ATTACK, psFireTarget);
				}
			}
			else if (isVtolDroid(psDroid) &&
					 (psDroid->action != DACTION_NONE) &&
					 (psDroid->action != DACTION_FIRESUPPORT))
			{
				moveToRearm(psDroid);
			}
			else if ((psDroid->action != DACTION_FIRESUPPORT) &&
					 (psDroid->action != DACTION_FIRESUPPORT_RETREAT))
			{
				actionDroidObj(psDroid, DACTION_FIRESUPPORT, psDroid->psTarget);
			}
		}
		break;
	case DORDER_RECYCLE:
		if (psDroid->psTarget == NULL)
		{
			psDroid->order = DORDER_NONE;
			actionDroid(psDroid, DACTION_NONE);
		}
		else if (actionReachedBuildPos(psDroid, psDroid->psTarget->pos.x, psDroid->psTarget->pos.y,
						(BASE_STATS *)((STRUCTURE *)psDroid->psTarget)->pStructureType))
		{
			recycleDroid(psDroid);
		}
		else if (psDroid->action == DACTION_NONE)
		{
			actionDroidLoc(psDroid, DACTION_MOVE, psDroid->psTarget->pos.x,psDroid->psTarget->pos.y);
		}
		break;
	case DORDER_GUARD:
		if (orderDroidList(psDroid))
		{
			// started a queued order - quit
			break;
		}
		else if ((psDroid->action == DACTION_NONE) ||
				 (psDroid->action == DACTION_MOVE) ||
				 (psDroid->action == DACTION_MOVEFIRE))
		{
			// not doing anything, make sure the droid is close enough
			// to the thing it is defending
			if ((!(psDroid->droidType == DROID_REPAIR || psDroid->droidType == DROID_CYBORG_REPAIR))
			    && psDroid->psTarget != NULL && psDroid->psTarget->type == OBJ_DROID
			    && ((DROID *)psDroid->psTarget)->droidType == DROID_COMMAND)
			{
				// guarding a commander, allow more space
				orderCheckGuardPosition(psDroid, DEFEND_CMD_BASEDIST);
			}
			else
			{
				orderCheckGuardPosition(psDroid, DEFEND_BASEDIST);
			}
		}
		else if (psDroid->droidType == DROID_REPAIR || psDroid->droidType == DROID_CYBORG_REPAIR)
		{
			// repairing something, make sure the droid doesn't go too far
			orderCheckGuardPosition(psDroid, REPAIR_MAXDIST);
		}
		else if (psDroid->droidType == DROID_CONSTRUCT || psDroid->droidType == DROID_CYBORG_CONSTRUCT)
		{
			// repairing something, make sure the droid doesn't go too far
			orderCheckGuardPosition(psDroid, CONSTRUCT_MAXDIST);
		}
		else if (psDroid->droidType == DROID_TRANSPORTER)
		{
			// check transporter isn't sitting there waiting to be filled when nothing exists!
			if (psDroid->player == selectedPlayer && getDroidsToSafetyFlag() &&
			    !missionDroidsRemaining(selectedPlayer))
			{
				// check that nothing is on the transporter (transporter counts as first in group)
				if (psDroid->psGroup && psDroid->psGroup->refCount < 2)
				{
					// the script can call startMission for this callback for offworld missions
					eventFireCallbackTrigger((TRIGGER_TYPE)CALL_START_NEXT_LEVEL);
				}
			}
		}
		else
		{
			//let vtols return to rearm
			if (!vtolRearming(psDroid))
			{
				// attacking something, make sure the droid doesn't go too far
				if (psDroid->psTarget != NULL && psDroid->psTarget->type == OBJ_DROID &&
					((DROID *)psDroid->psTarget)->droidType == DROID_COMMAND)
				{
					// guarding a commander, allow more space
					orderCheckGuardPosition(psDroid, DEFEND_CMD_MAXDIST);
				}
				else
				{
					orderCheckGuardPosition(psDroid, DEFEND_MAXDIST);
				}
			}
		}

		// get combat units in a command group to attack the commanders target
		if ( hasCommander(psDroid) && (psDroid->numWeaps > 0) )
		{
			if ((psDroid->psGroup->psCommander->action == DACTION_ATTACK) &&
				(psDroid->psGroup->psCommander->psActionTarget[0] != NULL) &&
				(!psDroid->psGroup->psCommander->psActionTarget[0]->died))
			{
				psObj = psDroid->psGroup->psCommander->psActionTarget[0];
				if (psDroid->action == DACTION_ATTACK ||
					psDroid->action == DACTION_MOVETOATTACK)
				{
					if (psDroid->psActionTarget[0] != psObj)
					{
						actionDroidObj(psDroid, DACTION_ATTACK, psObj);
					}
				}
				else if (psDroid->action != DACTION_MOVE)
				{
					actionDroidObj(psDroid, DACTION_ATTACK, psObj);
				}
			}

			// make sure units in a command group are actually guarding the commander
			psObj = orderStateObj(psDroid, DORDER_GUARD);	// find out who is being guarded by the droid
			if (psObj == NULL
			 || psObj != (BASE_OBJECT *)psDroid->psGroup->psCommander)
			{
				orderDroidObj(psDroid, DORDER_GUARD, (BASE_OBJECT *)psDroid->psGroup->psCommander, ModeImmediate);
			}
		}

		//repair droids default to repairing droids within a given range
		psObj = NULL;
		if ((psDroid->droidType == DROID_REPAIR || psDroid->droidType == DROID_CYBORG_REPAIR))
		{
			if (psDroid->action == DACTION_NONE)
			{
				psObj = checkForRepairRange(psDroid,NULL);
			}
			else if (psDroid->action == DACTION_SULK)
			{
				psObj = checkForRepairRange(psDroid,(DROID *)psDroid->psActionTarget[0]);
			}
			if (psObj)
			{
				actionDroidObj(psDroid, DACTION_DROIDREPAIR, (BASE_OBJECT *)psObj);
			}
		}
		//construct droids default to repairing structures within a given range
		psObj = NULL;
		if ((psDroid->droidType == DROID_CONSTRUCT || psDroid->droidType == DROID_CYBORG_CONSTRUCT))
		{
			if (psDroid->action == DACTION_NONE)
			{
				psObj = checkForDamagedStruct(psDroid,NULL);
			}
			else if (psDroid->action == DACTION_SULK)
			{
				psObj = checkForDamagedStruct(psDroid,(STRUCTURE *)psDroid->psActionTarget);
			}
			if (psObj)
			{
				orderDroidObj(psDroid, DORDER_REPAIR, psObj, ModeImmediate);
			}
		}
		break;
	default:
		ASSERT( false, "orderUpdateUnit: unknown order" );
	}

	// catch any vtol that is rearming but has finished his order
	if (psDroid->order == DORDER_NONE && vtolRearming(psDroid)
	    && (psDroid->psActionTarget[0] == NULL || !psDroid->psActionTarget[0]->died))
	{
		psDroid->order = DORDER_REARM;
		setDroidTarget(psDroid, psDroid->psActionTarget[0]);
	}

	if (psDroid->selected)
	{
		// Tell us what the droid is doing.
		sprintf(DROIDDOING,"%.12s,id(%d) order(%d):%s action(%d):%s secondary:%x move:%s", droidGetName(psDroid), psDroid->id,
		        psDroid->order, getDroidOrderName(psDroid->order), psDroid->action, getDroidActionName(psDroid->action), (int)psDroid->secondaryOrder,
		        moveDescription(psDroid->sMove.Status));
	}
}


/* Give a command group an order */
static void orderCmdGroupBase(DROID_GROUP *psGroup, DROID_ORDER_DATA *psData)
{
	DROID	*psCurr, *psChosen;
	SDWORD	currdist, mindist;

	ASSERT( psGroup != NULL,
		"cmdUnitOrderGroupBase: invalid unit group" );

	if (psData->order == DORDER_RECOVER)
	{
		// picking up an artifact - only need to send one unit
		psChosen = NULL;
		mindist = SDWORD_MAX;
		for(psCurr = psGroup->psList; psCurr; psCurr=psCurr->psGrpNext)
		{
			currdist = objPosDiffSq(psCurr->pos, psData->psObj->pos);
			if (currdist < mindist)
			{
				psChosen = psCurr;
				mindist = currdist;
			}
		}
		if (psChosen != NULL)
		{
			orderDroidBase(psChosen, psData);
		}
	}
	else
	{
		for (psCurr = psGroup->psList; psCurr; psCurr=psCurr->psGrpNext)
		{
			if (!orderState(psCurr, DORDER_RTR))		// if you change this, youll need to change sendcmdgroup()
			{
				orderDroidBase(psCurr, psData);
			}
		}
	}
}


// check the position of units giving fire support to this unit and tell
// them to pull back if the sensor is going to move through them
WZ_DECL_UNUSED static void orderCheckFireSupportPos(DROID *psSensor, DROID_ORDER_DATA *psOrder)
{
	SDWORD		fsx,fsy, fsnum, sensorVX,sensorVY, fsVX,fsVY;
	uint16_t        sensorAngle, fsAngle, adiff;
	SDWORD		xdiff,ydiff;
	DROID		*psCurr;
	BASE_OBJECT	*psTarget;
	BOOL		bRetreat;

	// find the middle of and droids doing firesupport
	fsx = fsy = fsnum = 0;
	for(psCurr=apsDroidLists[psSensor->player]; psCurr; psCurr=psCurr->psNext)
	{
		if (!isVtolDroid(psCurr)
		 && (psTarget = orderStateObj(psCurr, DORDER_FIRESUPPORT))
		 && psTarget == (BASE_OBJECT *)psSensor
		 && secondaryGetState(psCurr, DSO_HALTTYPE) != DSS_HALT_HOLD)
		{
			// got a unit doing fire support
			fsnum += 1;
			fsx += (SDWORD)psCurr->pos.x;
			fsy += (SDWORD)psCurr->pos.y;
		}
	}

	bRetreat = false;
	if (fsnum != 0)
	{
		// there are some units to check the position of
		fsx /= fsnum;
		fsy /= fsnum;

		// don't do it if too near to the firesupport units
		xdiff = fsx - (SDWORD)psSensor->pos.x;
		ydiff = fsy - (SDWORD)psSensor->pos.y;
		if (xdiff*xdiff + ydiff*ydiff < (TILE_UNITS*5)*(TILE_UNITS*5))
		{
			goto done;
		}

		sensorVX = (SDWORD)psOrder->x - (SDWORD)psSensor->pos.x;
		sensorVY = (SDWORD)psOrder->y - (SDWORD)psSensor->pos.y;
		fsVX = fsx - (SDWORD)psSensor->pos.x;
		fsVY = fsy - (SDWORD)psSensor->pos.y;

		// now check if the move position is further away than the firesupport units
		if (sensorVX*sensorVX + sensorVY*sensorVY < fsVX*fsVX + fsVY*fsVY)
		{
			goto done;
		}

		// now get the angle between the firesupport units and the sensor move
		sensorAngle = iAtan2(sensorVY, sensorVX);
		fsAngle = iAtan2(fsVY, fsVX);
		adiff = angleDelta(fsAngle - sensorAngle);

		// if the angle between the firesupport units and the sensor move is bigger
		// than 45 degrees don't retreat
		if (adiff > DEG(45))
		{
			goto done;
		}

		bRetreat = true;
	}

done:
	// made a decision whether to retreat

	// now move the firesupport units
	for(psCurr=apsDroidLists[psSensor->player]; psCurr; psCurr=psCurr->psNext)
	{
		if (!isVtolDroid(psCurr)
		 && (psTarget = orderStateObj(psCurr, DORDER_FIRESUPPORT))
		 && psTarget == (BASE_OBJECT *)psSensor
		 && secondaryGetState(psCurr, DSO_HALTTYPE) != DSS_HALT_HOLD)
		{
			if (bRetreat)
			{
				actionDroidLoc(psCurr, DACTION_FIRESUPPORT_RETREAT, psOrder->x, psOrder->y);
			}
			else if (psCurr->action == DACTION_FIRESUPPORT_RETREAT)
			{
				actionDroid(psCurr, DACTION_NONE);
			}
		}
	}
}



#define	AUDIO_DELAY_FIRESUPPORT		(3*GAME_TICKS_PER_SEC)

static void orderPlayFireSupportAudio( BASE_OBJECT *psObj )
{
	DROID		*psDroid = NULL;
	STRUCTURE	*psStruct = NULL;
	SDWORD		iAudioID = NO_SOUND;



	/* play appropriate speech */
	switch ( psObj->type )
	{
		case OBJ_DROID:
			psDroid = (DROID *) psObj;
			ASSERT( psObj != NULL,
					"orderPlayFireSupportAudio: invalid droid pointer" );
			if ( psDroid->droidType == DROID_COMMAND )
			{
				iAudioID = ID_SOUND_ASSIGNED_TO_COMMANDER;
			}
			else if ( psDroid->droidType == DROID_SENSOR )
			{
				iAudioID = ID_SOUND_ASSIGNED_TO_SENSOR;
			}
			break;

		case OBJ_STRUCTURE:
			ASSERT( psObj != NULL,
					"orderPlayFireSupportAudio: invalid structure pointer" );
			psStruct = (STRUCTURE *) psObj;
            //check for non-CB first
			if ( structStandardSensor(psStruct) || structVTOLSensor(psStruct) )
			{
				iAudioID = ID_SOUND_ASSIGNED_TO_SENSOR;
			}
			else if ( structCBSensor(psStruct) || structVTOLCBSensor(psStruct) )
			{
				iAudioID = ID_SOUND_ASSIGNED_TO_COUNTER_RADAR;
			}
			break;
		default:
			break;
	}

	if ( iAudioID != NO_SOUND )
	{
		audio_QueueTrackMinDelay( iAudioID, AUDIO_DELAY_FIRESUPPORT );
	}
}


/* The base order function */
void orderDroidBase(DROID *psDroid, DROID_ORDER_DATA *psOrder)
{
	UDWORD		iFactoryDistSq;
	STRUCTURE	*psStruct, *psRepairFac, *psFactory;
	const PROPULSION_STATS *psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	const Vector3i rPos(psOrder->x, psOrder->y, 0);

	syncDebugDroid(psDroid, '-');
	syncDebug("%d ordered %s", psDroid->id, getDroidOrderName(psOrder->order));

	if (psOrder->order != DORDER_TRANSPORTIN	// transporters special
	    && psOrder->psObj == NULL			// location-type order
	    && (validOrderForLoc(psOrder->order) || psOrder->order == DORDER_BUILD) 
	    && !fpathCheck(psDroid->pos, rPos, psPropStats->propulsionType))
	{
		if (!isHumanPlayer(psDroid->player))
		{
			debug(LOG_SCRIPT, "Invalid order %s given to player %d's %s for position (%d, %d) - ignoring", 
			      getDroidOrderName(psOrder->order), psDroid->player, droidGetName(psDroid), (int)psOrder->x, (int)psOrder->y);
		}
		objTrace(psDroid->id, "Invalid order %s for position (%d, %d) - ignoring", getDroidOrderName(psOrder->order), (int)psOrder->x, (int)psOrder->y);
		syncDebugDroid(psDroid, '?');
		return;
	}

	// deal with a droid receiving a primary order
	if (secondaryGotPrimaryOrder(psDroid, psOrder->order))
	{
		psOrder->order = DORDER_NONE;
	}

	// if this is a command droid - all it's units do the same thing
	if ((psDroid->droidType == DROID_COMMAND) &&
		(psDroid->psGroup != NULL) &&
		(psDroid->psGroup->type == GT_COMMAND) &&
		(psOrder->order != DORDER_GUARD) &&  //(psOrder->psObj == NULL)) &&
		(psOrder->order != DORDER_RTR) &&
		(psOrder->order != DORDER_RECYCLE) &&
		(psOrder->order != DORDER_MOVE))
	{
		if (psOrder->order == DORDER_ATTACK)
		{
			// change to attacktarget so that the group members
			// guard order does not get canceled
			psOrder->order = DORDER_ATTACKTARGET;
			orderCmdGroupBase(psDroid->psGroup, psOrder);
			psOrder->order = DORDER_ATTACK;
		}
		else
		{
			orderCmdGroupBase(psDroid->psGroup, psOrder);
		}

		// the commander doesn't have to pick up artifacts, one
		// of his units will do it for him (if there are any in his group).
		if ((psOrder->order == DORDER_RECOVER) &&
			(psDroid->psGroup->psList != NULL))
		{
			psOrder->order = DORDER_NONE;
		}
	}

	switch (psOrder->order)
	{
	case DORDER_NONE:
		// used when choose order cannot assign an order
		break;
	case DORDER_STOP:
		// get the droid to stop doing whatever it is doing
		actionDroid(psDroid, DACTION_NONE);
		psDroid->order = DORDER_NONE;
		setDroidTarget(psDroid, NULL);
		psDroid->psTarStats = NULL;
		psDroid->orderX = 0;
		psDroid->orderY = 0;
		psDroid->orderX2 = 0;
		psDroid->orderY2 = 0;
		break;
	case DORDER_TEMP_HOLD:
		// get the droid to stop doing whatever it is doing and temp hold
		actionDroid(psDroid, DACTION_NONE);
		psDroid->order = DORDER_TEMP_HOLD;
		setDroidTarget(psDroid, NULL);
		psDroid->psTarStats = NULL;
		psDroid->orderX = 0;
		psDroid->orderY = 0;
		psDroid->orderX2 = 0;
		psDroid->orderY2 = 0;
		break;
	case DORDER_MOVE:
	case DORDER_SCOUT:
		// can't move vtols to blocking tiles
		if (isVtolDroid(psDroid)
		    && fpathBlockingTile(map_coord(psOrder->x), map_coord(psOrder->y), getPropulsionStats(psDroid)->propulsionType))
		{
			break;
		}
		//in multiPlayer, cannot move Transporter to blocking tile either
		if (game.type == SKIRMISH
		    && psDroid->droidType == DROID_TRANSPORTER
		    && fpathBlockingTile(map_coord(psOrder->x), map_coord(psOrder->y), getPropulsionStats(psDroid)->propulsionType))
		{
			break;
		}
		// move a droid to a location
		psDroid->order = psOrder->order;
		psDroid->orderX = psOrder->x;
		psDroid->orderY = psOrder->y;
		actionDroidLoc(psDroid, DACTION_MOVE, psOrder->x,psOrder->y);
		break;
	case DORDER_PATROL:
		psDroid->order = psOrder->order;
		psDroid->orderX = psOrder->x;
		psDroid->orderY = psOrder->y;
		psDroid->orderX2 = psDroid->pos.x;
		psDroid->orderY2 = psDroid->pos.y;
		actionDroidLoc(psDroid, DACTION_MOVE, psOrder->x,psOrder->y);
		break;
	case DORDER_RECOVER:
		psDroid->order = DORDER_RECOVER;
		setDroidTarget(psDroid, psOrder->psObj);
		actionDroidLoc(psDroid, DACTION_MOVE, psOrder->psObj->pos.x,psOrder->psObj->pos.y);
		break;
	case DORDER_TRANSPORTOUT:
		// tell a (transporter) droid to leave home base for the offworld mission
		psDroid->order = DORDER_TRANSPORTOUT;
		psDroid->orderX = psOrder->x;
		psDroid->orderY = psOrder->y;
		actionDroidLoc(psDroid, DACTION_TRANSPORTOUT, psOrder->x,psOrder->y);
		break;
	case DORDER_TRANSPORTRETURN:
		// tell a (transporter) droid to return after unloading
		psDroid->order = DORDER_TRANSPORTRETURN;
		psDroid->orderX = psOrder->x;
		psDroid->orderY = psOrder->y;
		actionDroidLoc(psDroid, DACTION_TRANSPORTOUT, psOrder->x,psOrder->y);
		break;
	case DORDER_TRANSPORTIN:
		// tell a (transporter) droid to fly onworld
		psDroid->order = DORDER_TRANSPORTIN;
		psDroid->orderX = psOrder->x;
		psDroid->orderY = psOrder->y;
		actionDroidLoc(psDroid, DACTION_TRANSPORTIN, psOrder->x,psOrder->y);
		break;
	case DORDER_ATTACK:
	case DORDER_ATTACKTARGET:
		if (psDroid->numWeaps == 0
		    || psDroid->asWeaps[0].nStat == 0
		    || psDroid->droidType == DROID_TRANSPORTER)
		{
			break;
		}
		else if (psDroid->order == DORDER_GUARD && psOrder->order == DORDER_ATTACKTARGET)
		{
			// attacking something while guarding, don't change the order
			actionDroidObj(psDroid, DACTION_ATTACK, (BASE_OBJECT *)psOrder->psObj);
		}
		else if (!psOrder->psObj->died)
		{
			//cannot attack a Transporter with EW in multiPlayer
			if (game.type == SKIRMISH && electronicDroid(psDroid)
			    && psOrder->psObj->type == OBJ_DROID && ((DROID *)psOrder->psObj)->droidType == DROID_TRANSPORTER)
			{
				break;
			}
			setDroidTarget(psDroid, psOrder->psObj);
			psDroid->order = psOrder->order;

			if (isVtolDroid(psDroid)
			    || actionInsideMinRange(psDroid, psOrder->psObj, NULL)
			    || (psOrder->order == DORDER_ATTACKTARGET
			        && secondaryGetState(psDroid, DSO_HALTTYPE) == DSS_HALT_HOLD))
			{
				actionDroidObj(psDroid, DACTION_ATTACK, psOrder->psObj);
			}
			else
			{
				actionDroidLoc(psDroid, DACTION_MOVE, psOrder->psObj->pos.x, psOrder->psObj->pos.y);
			}
		}
		break;
	case DORDER_BUILD:
		// build a new structure
		if (!(psDroid->droidType == DROID_CONSTRUCT || psDroid->droidType == DROID_CYBORG_CONSTRUCT))
		{
			break;
		}
		ASSERT( psOrder->psStats != NULL,
			"orderUnitBase: invalid structure stats pointer" );
		psDroid->order = DORDER_BUILD;
		psDroid->orderX = psOrder->x;
		psDroid->orderY = psOrder->y;
		psDroid->orderDirection = psOrder->direction;
		setDroidTarget(psDroid, NULL);
		psDroid->psTarStats = psOrder->psStats;
		ASSERT((!psDroid->psTarStats || ((STRUCTURE_STATS *)psDroid->psTarStats)->type != REF_DEMOLISH), "Cannot build demolition");
		actionDroidLoc(psDroid, DACTION_BUILD, psOrder->x,psOrder->y);
		objTrace(psDroid->id, "Starting new construction effort of %s", psOrder->psStats ? psOrder->psStats->pName : "NULL POINTER");
		break;
	case DORDER_BUILDMODULE:
		//build a module onto the structure
		if (!(psDroid->droidType == DROID_CONSTRUCT || psDroid->droidType == DROID_CYBORG_CONSTRUCT))
		{
			break;
		}
		psDroid->order = DORDER_BUILD;
		psDroid->orderX = psOrder->psObj->pos.x;
		psDroid->orderY = psOrder->psObj->pos.y;
		psDroid->orderDirection = 0;
		setDroidTarget(psDroid, NULL);
		psDroid->psTarStats = (BASE_STATS *)getModuleStat((STRUCTURE *)psOrder->psObj);
		ASSERT(psDroid->psTarStats != NULL, "orderUnitBase: should have found a module stats");
		ASSERT((!psDroid->psTarStats || ((STRUCTURE_STATS *)psDroid->psTarStats)->type != REF_DEMOLISH), "Cannot build demolition");
		actionDroidLoc(psDroid, DACTION_BUILD, psOrder->psObj->pos.x,psOrder->psObj->pos.y);
		objTrace(psDroid->id, "Starting new upgrade of %s", psOrder->psStats ? psOrder->psStats->pName : "NULL POINTER");
		break;
	case DORDER_LINEBUILD:
		// build a line of structures
		ASSERT_OR_RETURN(, psDroid->droidType == DROID_CONSTRUCT || psDroid->droidType == DROID_CYBORG_CONSTRUCT, "Not a constructor droid");
		ASSERT(psOrder->psStats != NULL, "Invalid structure stats pointer");

		psDroid->order = DORDER_LINEBUILD;
		psDroid->orderX = psOrder->x;
		psDroid->orderY = psOrder->y;
		psDroid->orderX2 = psOrder->x2;
		psDroid->orderY2 = psOrder->y2;
		psDroid->orderDirection = psOrder->direction;
		setDroidTarget(psDroid, NULL);
		psDroid->psTarStats = psOrder->psStats;
		ASSERT((!psDroid->psTarStats || ((STRUCTURE_STATS *)psDroid->psTarStats)->type != REF_DEMOLISH), "Cannot build demolition");
		actionDroidLoc(psDroid, DACTION_BUILD, psOrder->x,psOrder->y);
		objTrace(psDroid->id, "Starting new line construction of %s", psOrder->psStats ? psOrder->psStats->pName : "NULL POINTER");
		break;
	case DORDER_HELPBUILD:
		// help to build a structure that is starting to be built
		ASSERT_OR_RETURN(, psDroid->droidType == DROID_CONSTRUCT || psDroid->droidType == DROID_CYBORG_CONSTRUCT, "Not a constructor droid");
		ASSERT_OR_RETURN(, psOrder->psObj != NULL, "Help to build a NULL pointer?");
		psDroid->order = DORDER_HELPBUILD;
		psDroid->orderX = psOrder->psObj->pos.x;
		psDroid->orderY = psOrder->psObj->pos.y;
		setDroidTarget(psDroid, psOrder->psObj);
		psDroid->psTarStats = (BASE_STATS *)((STRUCTURE *)psOrder->psObj)->pStructureType;
		ASSERT((!psDroid->psTarStats || ((STRUCTURE_STATS *)psDroid->psTarStats)->type != REF_DEMOLISH), "Cannot build demolition");
		actionDroidLoc(psDroid, DACTION_BUILD, psDroid->orderX, psDroid->orderY);
		objTrace(psDroid->id, "Helping construction of %s", psDroid->psTarStats ? psDroid->psTarStats->pName : "NULL POINTER");
		break;
	case DORDER_DEMOLISH:
		if (!(psDroid->droidType == DROID_CONSTRUCT || psDroid->droidType == DROID_CYBORG_CONSTRUCT))
		{
			break;
		}
		psDroid->order = DORDER_DEMOLISH;
		psDroid->orderX = psOrder->psObj->pos.x;
		psDroid->orderY = psOrder->psObj->pos.y;
		psDroid->psTarStats = NULL;
		setDroidTarget(psDroid, psOrder->psObj);
		actionDroidObj(psDroid, DACTION_DEMOLISH, (BASE_OBJECT *)psOrder->psObj);
		break;
	case DORDER_REPAIR:
		if (!(psDroid->droidType == DROID_CONSTRUCT || psDroid->droidType == DROID_CYBORG_CONSTRUCT))
		{
			break;
		}
		psDroid->order = DORDER_REPAIR;
		psDroid->orderX = psOrder->psObj->pos.x;
		psDroid->orderY = psOrder->psObj->pos.y;
		setDroidTarget(psDroid, psOrder->psObj);
		actionDroidObj(psDroid, DACTION_REPAIR, (BASE_OBJECT *)psOrder->psObj);
		break;
	case DORDER_DROIDREPAIR:
		if (!(psDroid->droidType == DROID_REPAIR || psDroid->droidType == DROID_CYBORG_REPAIR))
		{
			break;
		}
		psDroid->order = DORDER_DROIDREPAIR;
		setDroidTarget(psDroid, psOrder->psObj);
		actionDroidObj(psDroid, DACTION_DROIDREPAIR, (BASE_OBJECT *)psOrder->psObj);
		break;
	case DORDER_OBSERVE:
		// keep an object within sensor view
		psDroid->order = DORDER_OBSERVE;
		setDroidTarget(psDroid, psOrder->psObj);
		actionDroidObj(psDroid, DACTION_OBSERVE, (BASE_OBJECT *)psOrder->psObj);
		break;
	case DORDER_FIRESUPPORT:
		if (psDroid->asWeaps[0].nStat == 0)
		{
			break;
		}
		psDroid->order = DORDER_FIRESUPPORT;
		setDroidTarget(psDroid, psOrder->psObj);
		// let the order update deal with vtol droids
		if (!isVtolDroid(psDroid))
		{
			actionDroidObj(psDroid, DACTION_FIRESUPPORT, (BASE_OBJECT *)psOrder->psObj);
		}

		if ( psDroid->player == selectedPlayer )
		{
			orderPlayFireSupportAudio( psOrder->psObj );
		}
		break;
	case DORDER_RETREAT:
	case DORDER_RUNBURN:
	case DORDER_RUN:
		psDroid->order = psOrder->order;
		if ((psOrder->order == DORDER_RUN) &&
			((psOrder->x != 0) || (psOrder->y != 0)))
		{
			psDroid->orderX = psOrder->x;
			psDroid->orderY = psOrder->y;
		}
		else
		{
			psDroid->orderX = (UWORD)asRunData[psDroid->player].sPos.x;
			psDroid->orderY = (UWORD)asRunData[psDroid->player].sPos.y;
		}
		if (psDroid->orderX == 0 || psDroid->orderY)
		{
			// We have still not managed to find a valid place to run.
			objTrace(psDroid->id, "Wants to run, but has no designated retreat point - standing still.");
			break;
		}
		actionDroidLoc(psDroid, DACTION_MOVE, psDroid->orderX,psDroid->orderY);
		break;
	case DORDER_DESTRUCT:
		psDroid->order = DORDER_DESTRUCT;
		actionDroid(psDroid, DACTION_DESTRUCT);
		break;
	case DORDER_RTB:
		// send vtols back to their return pos
		if (isVtolDroid(psDroid) && !bMultiPlayer && psDroid->player != selectedPlayer)
		{
			int iDX = asVTOLReturnPos[psDroid->player].x;
			int iDY = asVTOLReturnPos[psDroid->player].y;

			if (iDX && iDY)
			{
				psDroid->order = DORDER_LEAVEMAP;
				actionDroidLoc(psDroid, DACTION_MOVE, iDX, iDY);
				break;
			}
		}

		for(psStruct=apsStructLists[psDroid->player]; psStruct; psStruct = psStruct->psNext)
		{
			if (psStruct->pStructureType->type == REF_HQ)
			{
				UDWORD	droidX = psStruct->pos.x;
				UDWORD	droidY = psStruct->pos.y;

				psDroid->order = DORDER_RTB;
				// Find a place to land for vtols. And Transporters in a multiPlay game.
				if (isVtolDroid(psDroid) || (game.type == SKIRMISH && psDroid->droidType == DROID_TRANSPORTER))
				{
					actionVTOLLandingPos(psDroid, &droidX,&droidY);
				}
				actionDroidLoc(psDroid, DACTION_MOVE, droidX,droidY);
				break;
			}
		}
		// no HQ go to the landing zone
		if (psDroid->order != DORDER_RTB)
		{
			// see if the LZ has been set up
			int iDX = getLandingX(psDroid->player);
			int iDY = getLandingY(psDroid->player);

			if (iDX && iDY)
			{
			    psDroid->order = DORDER_RTB;
			    //actionDroidLoc(psDroid, DACTION_MOVE, getLandingX(psDroid->player),
			    //				getLandingY(psDroid->player));
			    actionDroidLoc(psDroid, DACTION_MOVE, iDX,iDY);
			}
			else
			{
				// haven't got an LZ set up so don't do anything
				actionDroid(psDroid, DACTION_NONE);
				psDroid->order = DORDER_NONE;
				setDroidTarget(psDroid, NULL);
				psDroid->psTarStats = NULL;
			}
		}
		break;
	case DORDER_RTR:
	case DORDER_RTR_SPECIFIED:
		if (isVtolDroid(psDroid))
		{
			moveToRearm(psDroid);
			break;
		}
		if (psOrder->psObj == NULL)
		{
			int iRepairFacDistSq = 0;

			psRepairFac = NULL;
			for(psStruct=apsStructLists[psDroid->player]; psStruct; psStruct = psStruct->psNext)
			{
				if ((psStruct->pStructureType->type == REF_REPAIR_FACILITY) ||
					((psStruct->pStructureType->type == REF_HQ) && (psRepairFac == NULL)))
				{
					int iStructDistSq = droidSqDist(psDroid, (BASE_OBJECT *)psStruct);

					if (iStructDistSq <= 0)
					{
						continue;	// cannot reach position
					}

					/* Choose current structure if first repair facility found or nearer than previously chosen facility, or is built */
					if (psRepairFac == NULL || psRepairFac->pStructureType->type == REF_HQ || iRepairFacDistSq > iStructDistSq
					    || (psRepairFac->status != SS_BUILT && psStruct->status == SS_BUILT))
					{
						psRepairFac = psStruct;
						iRepairFacDistSq = iStructDistSq;
					}
				}
			}
		}
		else
		{
			psRepairFac = (STRUCTURE *)psOrder->psObj;
		}

		// droids doing a DORDER_RTR periodically give themselves a DORDER_RTR so that
		// they always go to the nearest repair facility
		// this stops the unit doing anything more if the same repair facility gets chosen
		if (psDroid->order == DORDER_RTR && psDroid->psTarget == (BASE_OBJECT *)psRepairFac)
		{
			objTrace(psDroid->id, "DONE FOR NOW");
			break;
		}

		/* give repair order if repair facility found */
		if (psRepairFac != NULL)
		{
			if (psRepairFac->pStructureType->type == REF_REPAIR_FACILITY)
			{
				/* move to front of structure */
				psDroid->order  = psOrder->order;
				psDroid->orderX = psRepairFac->pos.x;
				psDroid->orderY = psRepairFac->pos.y;
				setDroidTarget(psDroid, (BASE_OBJECT *) psRepairFac);
				/* If in multiPlayer, and the Transporter has been sent to be
				 * repaired, need to find a suitable location to drop down. */
				if (game.type == SKIRMISH && psDroid->droidType == DROID_TRANSPORTER)
				{
					UDWORD droidX, droidY;
					droidX = psDroid->orderX;
					droidY = psDroid->orderY;

					objTrace(psDroid->id, "Repair transport");
					actionVTOLLandingPos(psDroid, &droidX,&droidY);
					actionDroidLoc(psDroid, DACTION_MOVE, droidX,droidY);
				}
				else
				{
					objTrace(psDroid->id, "Go to repair facility at (%d, %d) using (%d, %d)!", (int)psRepairFac->pos.x, (int)psRepairFac->pos.y, (int)psDroid->orderX, (int)psDroid->orderY);
					actionDroidObjLoc(psDroid, DACTION_MOVE, (BASE_OBJECT *)psRepairFac, psDroid->orderX, psDroid->orderY);
				}
			}
			else
			{
				orderDroid(psDroid, DORDER_RTB, ModeImmediate);
			}
		}
		else
		{
			// no repair facility or HQ go to the landing zone
			if (!bMultiPlayer && selectedPlayer == 0)
			{
				orderDroid(psDroid, DORDER_RTB, ModeImmediate);
			}
		}
		break;
	case DORDER_EMBARK:
		// move the droid to the transporter location
		psDroid->order = DORDER_EMBARK;
		psDroid->orderX = psOrder->psObj->pos.x;
		psDroid->orderY = psOrder->psObj->pos.y;
		setDroidTarget(psDroid, psOrder->psObj);
		actionDroidLoc(psDroid, DACTION_MOVE, psOrder->psObj->pos.x, psOrder->psObj->pos.y);
		break;
	case DORDER_DISEMBARK:
        //only valid in multiPlayer mode
        if (bMultiPlayer)
        {
            //this order can only be given to Transporter droids
            if (psDroid->droidType == DROID_TRANSPORTER)
            {
                psDroid->order = DORDER_DISEMBARK;
		        psDroid->orderX = psOrder->x;
		        psDroid->orderY = psOrder->y;
                //move the Transporter to the requested location
		        actionDroidLoc(psDroid, DACTION_MOVE, psOrder->x,psOrder->y);
                //close the Transporter interface - if up
                if (widgGetFromID(psWScreen,IDTRANS_FORM) != NULL)
                {
                    intRemoveTrans();
                }
            }
        }
        break;
	case DORDER_RECYCLE:
		psFactory = NULL;
		iFactoryDistSq = 0;
		for(psStruct=apsStructLists[psDroid->player]; psStruct; psStruct = psStruct->psNext)
		{
			// Look for nearest factory or repair facility
			if (psStruct->pStructureType->type == REF_FACTORY || psStruct->pStructureType->type == REF_CYBORG_FACTORY
			    || psStruct->pStructureType->type == REF_VTOL_FACTORY || psStruct->pStructureType->type == REF_REPAIR_FACILITY)
			{
				/* get droid->facility distance squared */
				int iStructDistSq = droidSqDist(psDroid, (BASE_OBJECT *)psStruct);

				/* Choose current structure if first facility found or nearer than previously chosen facility */
				if (psStruct->status == SS_BUILT && iStructDistSq > 0 && (psFactory == NULL || iFactoryDistSq > iStructDistSq))
				{
					psFactory = psStruct;
					iFactoryDistSq = iStructDistSq;
				}
			}
		}

		/* give recycle order if facility found */
		if ( psFactory != NULL )
		{
			/* move to front of structure */
			psDroid->order  = DORDER_RECYCLE;
			psDroid->orderX = psFactory->pos.x;
			psDroid->orderY = psFactory->pos.y + world_coord(getStructureBreadth(psFactory)) / 2 + TILE_UNITS / 2;
			setDroidTarget(psDroid, (BASE_OBJECT *) psFactory);
			actionDroidObjLoc( psDroid, DACTION_MOVE, (BASE_OBJECT *) psFactory,
								psDroid->orderX, psDroid->orderY);
		}
		break;
	case DORDER_GUARD:
		psDroid->order = DORDER_GUARD;
		setDroidTarget(psDroid, psOrder->psObj);
		if (psOrder->psObj != NULL)
		{
			psDroid->orderX = psOrder->psObj->pos.x;
			psDroid->orderY = psOrder->psObj->pos.y;
		}
		else
		{
			psDroid->orderX = psOrder->x;
			psDroid->orderY = psOrder->y;
		}
		actionDroid( psDroid, DACTION_NONE );
		break;
	case DORDER_RESTORE:
		if (!electronicDroid(psDroid))
		{
			break;
		}
		if (psOrder->psObj->type != OBJ_STRUCTURE)
		{
			ASSERT( false, "orderDroidBase: invalid object type for Restore order" );
			break;
		}
		psDroid->order = DORDER_RESTORE;
		psDroid->orderX = psOrder->psObj->pos.x;
		psDroid->orderY = psOrder->psObj->pos.y;
		setDroidTarget(psDroid, psOrder->psObj);
		actionDroidObj(psDroid, DACTION_RESTORE, (BASE_OBJECT *)psOrder->psObj);
		break;
	case DORDER_CLEARWRECK:
		if (!(psDroid->droidType == DROID_CONSTRUCT || psDroid->droidType == DROID_CYBORG_CONSTRUCT))
		{
			break;
		}
		psDroid->order = DORDER_CLEARWRECK;
		psDroid->orderX = psOrder->psObj->pos.x;
		psDroid->orderY = psOrder->psObj->pos.y;
		setDroidTarget(psDroid, psOrder->psObj);
		actionDroidObj(psDroid, DACTION_CLEARWRECK, (BASE_OBJECT *)psOrder->psObj);
		break;
	case DORDER_REARM:
		// didn't get executed before
		if (!isVtolDroid(psDroid))
		{
			break;
		}
		psDroid->order = DORDER_REARM;
		setDroidTarget(psDroid, psOrder->psObj);
		actionDroidObj(psDroid,DACTION_MOVETOREARM, (BASE_OBJECT *)psOrder->psObj);
		assignVTOLPad(psDroid, (STRUCTURE *)psOrder->psObj);
		break;
	case DORDER_CIRCLE:
		if (!isVtolDroid(psDroid))
		{
			break;
		}
		psDroid->order = psOrder->order;
		psDroid->orderX = psOrder->x;
		psDroid->orderY = psOrder->y;
		actionDroidLoc(psDroid, DACTION_MOVE, psOrder->x,psOrder->y);
		break;
	default:
		ASSERT( false, "orderUnitBase: unknown order" );
		break;
	}

	syncDebugDroid(psDroid, '+');
}


/* Give a droid an order */
void orderDroid(DROID *psDroid, DROID_ORDER order, QUEUE_MODE mode)
{
	DROID_ORDER_DATA	sOrder;

	ASSERT( psDroid != NULL,
		"orderUnit: Invalid unit pointer" );
	ASSERT( order == DORDER_NONE ||
			order == DORDER_RETREAT ||
			order == DORDER_DESTRUCT ||
			order == DORDER_RTR ||
			order == DORDER_RTB ||
			order == DORDER_RECYCLE ||
			order == DORDER_RUN ||
			order == DORDER_RUNBURN ||
			order == DORDER_TRANSPORTIN ||
			order == DORDER_STOP ||		// Added this PD.
			order == DORDER_TEMP_HOLD,
		"orderUnit: Invalid order" );

	memset(&sOrder,0,sizeof(DROID_ORDER_DATA));
	sOrder.order = order;

	if (mode == ModeQueue && bMultiPlayer)
	{
		sendDroidInfo(psDroid,  order,  0,  0, NULL, NULL, 0, 0, 0,  false);
	}
	else
	{
		orderClearDroidList(psDroid);
		orderDroidBase(psDroid, &sOrder);
	}
}

/* Check the order state of a droid */
BOOL orderState(DROID *psDroid, DROID_ORDER order)
{
	if (order == DORDER_RTR)
	{
		return psDroid->order == DORDER_RTR || psDroid->order == DORDER_RTR_SPECIFIED;
	}

	return psDroid->order == order;
}

BOOL validOrderForLoc(DROID_ORDER order)
{
	return (order == DORDER_NONE ||	order == DORDER_MOVE ||	order == DORDER_GUARD ||
		order == DORDER_SCOUT || order == DORDER_RUN || order == DORDER_PATROL ||
		order == DORDER_TRANSPORTOUT || order == DORDER_TRANSPORTIN  ||
		order == DORDER_TRANSPORTRETURN || order == DORDER_DISEMBARK ||
		order == DORDER_CIRCLE);
}

/* Give a droid an order with a location target */
void orderDroidLoc(DROID *psDroid, DROID_ORDER order, UDWORD x, UDWORD y, QUEUE_MODE mode)
{
	DROID_ORDER_DATA	sOrder;

	ASSERT_OR_RETURN(, psDroid != NULL, "Invalid unit pointer");
	ASSERT_OR_RETURN(, validOrderForLoc(order), "Invalid order for location");

	if (mode == ModeQueue && bMultiPlayer) //ajl
	{
		sendDroidInfo(psDroid,  order,  x,  y, NULL, NULL, 0, 0, 0,  false);
		return;  // Wait to receive our order before changing the droid.
	}

	orderClearDroidList(psDroid);

	memset(&sOrder,0,sizeof(DROID_ORDER_DATA));
	sOrder.order = order;
	sOrder.x = (UWORD)x;
	sOrder.y = (UWORD)y;
	orderDroidBase(psDroid, &sOrder);
}


/* Get the state of a droid order with it's location */
BOOL orderStateLoc(DROID *psDroid, DROID_ORDER order, UDWORD *pX, UDWORD *pY)
{
	if (order != psDroid->order)
	{
		return false;
	}

	// check the order is one with a location
	switch (psDroid->order)
	{
	default:
		// not a location order - return false
		break;
	case DORDER_MOVE:
	case DORDER_RETREAT:
		*pX = psDroid->orderX;
		*pY = psDroid->orderY;
		return true;
		break;
	}

	return false;
}

BOOL validOrderForObj(DROID_ORDER order)
{
	return (order == DORDER_NONE || order == DORDER_HELPBUILD || order == DORDER_DEMOLISH ||
		order == DORDER_REPAIR || order == DORDER_ATTACK || order == DORDER_FIRESUPPORT ||
		order == DORDER_OBSERVE || order == DORDER_ATTACKTARGET || order == DORDER_RTR ||
		order == DORDER_RTR_SPECIFIED || order == DORDER_EMBARK || order == DORDER_GUARD ||
		order == DORDER_DROIDREPAIR || order == DORDER_RESTORE || order == DORDER_BUILDMODULE ||
		order == DORDER_REARM || order == DORDER_CLEARWRECK || order == DORDER_RECOVER);
}

/* Give a droid an order with an object target */
void orderDroidObj(DROID *psDroid, DROID_ORDER order, BASE_OBJECT *psObj, QUEUE_MODE mode)
{
	DROID_ORDER_DATA	sOrder;

	ASSERT(psDroid != NULL, "Invalid unit pointer");
	ASSERT(validOrderForObj(order), "Invalid order for object");
	ASSERT(!isBlueprint(psObj), "Target is a blueprint");

	if (mode == ModeQueue && bMultiPlayer) //ajl
	{
		sendDroidInfo(psDroid, order, 0, 0, psObj, NULL, 0, 0, 0,  false);
		return;  // Wait for the order to be received before changing the droid.
	}

	orderClearDroidList(psDroid);

	memset(&sOrder,0,sizeof(DROID_ORDER_DATA));
	sOrder.order = order;
	sOrder.psObj = psObj;
	orderDroidBase(psDroid, &sOrder);
}


/* Get the state of a droid order with an object */
BASE_OBJECT* orderStateObj(DROID *psDroid, DROID_ORDER order)
{
	BOOL	match = false;

	switch (order)
	{
	case DORDER_BUILD:
	case DORDER_LINEBUILD:
	case DORDER_HELPBUILD:
		if (psDroid->order == DORDER_BUILD ||
			psDroid->order == DORDER_HELPBUILD ||
			psDroid->order == DORDER_LINEBUILD)
		{
			match = true;
		}
		break;
	case DORDER_ATTACK:
	case DORDER_FIRESUPPORT:
	case DORDER_OBSERVE:
	case DORDER_DEMOLISH:
	case DORDER_DROIDREPAIR:
	case DORDER_REARM:
	case DORDER_GUARD:
		if (psDroid->order == order)
		{
			match = true;
		}
		break;
	case DORDER_RTR:
		if (psDroid->order == DORDER_RTR ||
			psDroid->order == DORDER_RTR_SPECIFIED)
		{
			match = true;
		}
	default:
		break;
	}

	if (!match)
	{
		return NULL;
	}

	// check the order is one with an object
	switch (psDroid->order)
	{
	default:
		// not an object order - return false
		return NULL;
		break;
	case DORDER_BUILD:
	case DORDER_LINEBUILD:
		if (psDroid->action == DACTION_BUILD ||
			psDroid->action == DACTION_BUILDWANDER)
		{
			return psDroid->psTarget;
		}
		break;
	case DORDER_HELPBUILD:
		if (psDroid->action == DACTION_BUILD ||
			psDroid->action == DACTION_BUILDWANDER ||
			psDroid->action == DACTION_MOVETOBUILD)
		{
			return psDroid->psTarget;
		}
		break;
	//case DORDER_HELPBUILD:
	case DORDER_ATTACK:
	case DORDER_FIRESUPPORT:
	case DORDER_OBSERVE:
	case DORDER_DEMOLISH:
	case DORDER_RTR:
	case DORDER_RTR_SPECIFIED:
	case DORDER_DROIDREPAIR:
	case DORDER_REARM:
	case DORDER_GUARD:
		return psDroid->psTarget;
		break;
	}

	return NULL;
}


/* Give a droid an order with a location and a stat */
void orderDroidStatsLocDir(DROID *psDroid, DROID_ORDER order, BASE_STATS *psStats, UDWORD x, UDWORD y, uint16_t direction, QUEUE_MODE mode)
{
	DROID_ORDER_DATA	sOrder;

	ASSERT(psDroid != NULL, "Invalid unit pointer");
	ASSERT(order == DORDER_BUILD, "Invalid order for location");

	if (mode == ModeQueue && bMultiPlayer)
	{
		sendDroidInfo(psDroid, order, x, y, NULL, psStats, 0, 0, direction,  false);
		return;  // Wait for our order before changing the droid.
	}

	orderClearDroidList(psDroid);

	memset(&sOrder,0,sizeof(DROID_ORDER_DATA));
	sOrder.order = order;
	sOrder.x = (UWORD)x;
	sOrder.y = (UWORD)y;
	sOrder.direction = direction;
	sOrder.psStats = psStats;
	orderDroidBase(psDroid, &sOrder);
}

/* add an order with a location and a stat to the droids order list*/
void orderDroidStatsLocDirAdd(DROID *psDroid, DROID_ORDER order, BASE_STATS *psStats, UDWORD x, UDWORD y, uint16_t direction, bool add)
{
	ASSERT(psDroid != NULL, "Invalid unit pointer");

	// can only queue build orders with this function
	if (order != DORDER_BUILD)
	{
		return;
	}

	sendDroidInfo(psDroid, order, x, y, NULL, psStats, 0, 0, direction,  add);
}


/* Give a droid an order with a location and a stat */
void orderDroidStatsTwoLocDir(DROID *psDroid, DROID_ORDER order, BASE_STATS *psStats, UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2, uint16_t direction, QUEUE_MODE mode)
{
	DROID_ORDER_DATA	sOrder;

	ASSERT(psDroid != NULL,	"Invalid unit pointer");
	ASSERT(order == DORDER_LINEBUILD, "Invalid order for location");
	ASSERT(x1 == x2 || y1 == y2, "Invalid locations for LINEBUILD");

	if (mode == ModeQueue && bMultiPlayer)
	{
		sendDroidInfo(psDroid, order, x1, y1, NULL, psStats, x2, y2, direction,  false);
		return;  // Wait for our order before changing the droid.
	}

	orderClearDroidList(psDroid);

	memset(&sOrder,0,sizeof(DROID_ORDER_DATA));
	sOrder.order = order;
	sOrder.x = (UWORD)x1;
	sOrder.y = (UWORD)y1;
	sOrder.x2 = (UWORD)x2;
	sOrder.y2 = (UWORD)y2;
	sOrder.direction = direction;
	sOrder.psStats = psStats;
	orderDroidBase(psDroid, &sOrder);
}

/* Add an order with a location and a stat */
void orderDroidStatsTwoLocDirAdd(DROID *psDroid, DROID_ORDER order, BASE_STATS *psStats, UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2, uint16_t direction)
{
	ASSERT(psDroid != NULL, "Invalid unit pointer");
	ASSERT(order == DORDER_LINEBUILD, "Invalid order for location");
	ASSERT(x1 == x2 || y1 == y2, "Invalid locations for LINEBUILD");

	sendDroidInfo(psDroid, order, x1, y1, NULL, psStats, x2, y2, direction,  true);
}


/* Get the state of a droid order with a location and a stat */
BOOL orderStateStatsLoc(DROID *psDroid, DROID_ORDER order, BASE_STATS **ppsStats, UDWORD *pX, UDWORD *pY)
{
	BOOL	match = false;

	switch (order)
	{
	case DORDER_BUILD:
	case DORDER_LINEBUILD:
		if (psDroid->order == DORDER_BUILD ||
			psDroid->order == DORDER_LINEBUILD)
		{
			match = true;
		}
		break;
	default:
		break;
	}
	if (!match)
	{
		return false;
	}

	// check the order is one with stats and a location
	switch (psDroid->order)
	{
	default:
		// not a stats/location order - return false
		return false;
		break;
	case DORDER_BUILD:
	case DORDER_LINEBUILD:
		if (psDroid->action == DACTION_MOVETOBUILD ||
			psDroid->action == DACTION_BUILD_FOUNDATION ||
			psDroid->action == DACTION_FOUNDATION_WANDER)
		{
			*ppsStats = psDroid->psTarStats;
			*pX = psDroid->orderX;
			*pY = psDroid->orderY;
			return true;
		}
		break;
	}

	return false;
}

static OrderListEntry orderDataToOrderList(DROID_ORDER_DATA const *psOrder)
{
	OrderListEntry list;
	bool useStats = psOrder->order == DORDER_BUILD || psOrder->order == DORDER_LINEBUILD;

	list.order         = psOrder->order;
	list.psOrderTarget = useStats? (void *)psOrder->psStats : (void *)psOrder->psObj;
	list.x             = psOrder->x;
	list.y             = psOrder->y;
	list.x2            = psOrder->x2;
	list.y2            = psOrder->y2;
	list.direction     = psOrder->direction;

	return list;
}

void orderDroidAddPending(DROID *psDroid, DROID_ORDER_DATA *psOrder)
{
	ASSERT(psDroid != NULL, "Invalid unit pointer");

	psDroid->asOrderList.push_back(orderDataToOrderList(psOrder));

	//don't display the arrow-effects with build orders since unnecessary
	if (!bOrderEffectDisplayed && (psOrder->order != DORDER_BUILD || psOrder->order != DORDER_LINEBUILD || psOrder->order != DORDER_BUILDMODULE || psOrder->order != DORDER_HELPBUILD))
	{
		Vector3i position;
		position.x = psOrder->x;
		position.z = psOrder->y;
		position.y = map_Height(position.x, position.z) + 32;
		if (psOrder->psObj != NULL && psOrder->psObj->sDisplay.imd != NULL)
		{
			position.y += psOrder->psObj->sDisplay.imd->max.y;
		}
		addEffect(&position, EFFECT_WAYPOINT, WAYPOINT_TYPE, false, NULL, 0);
		bOrderEffectDisplayed = true;
	}
}

// add an order to a droids order list
void orderDroidAdd(DROID *psDroid, DROID_ORDER_DATA *psOrder)
{
	ASSERT(psDroid != NULL, "Invalid unit pointer");

	if (psDroid->listSize >= psDroid->asOrderList.size())
	{
		// Make more room to store the order.
		psDroid->asOrderList.push_back(OrderListEntry());
	}

	psDroid->asOrderList[psDroid->listSize] = orderDataToOrderList(psOrder);
	psDroid->listSize += 1;

	// if not doing anything - do it immediately
	if (psDroid->listSize <= 1 &&
	    (psDroid->order == DORDER_NONE ||
	     psDroid->order == DORDER_GUARD ||
	     psDroid->order == DORDER_PATROL ||
	     psDroid->order == DORDER_CIRCLE ||
	     psDroid->order == DORDER_TEMP_HOLD))
	{
		orderDroidList(psDroid);
	}
}


// do the next order from a droids order list
BOOL orderDroidList(DROID *psDroid)
{
	DROID_ORDER_DATA	sOrder;

	if (psDroid->listSize > 0)
	{
		// there are some orders to give
		memset(&sOrder, 0, sizeof(DROID_ORDER_DATA));
		sOrder.order = psDroid->asOrderList[0].order;
		switch (psDroid->asOrderList[0].order)
		{
		case DORDER_MOVE:
		case DORDER_SCOUT:
		case DORDER_DISEMBARK:
			sOrder.psObj = NULL;
			break;
		case DORDER_ATTACK:
		case DORDER_REPAIR:
		case DORDER_OBSERVE:
		case DORDER_DROIDREPAIR:
		case DORDER_FIRESUPPORT:
		case DORDER_CLEARWRECK:
		case DORDER_DEMOLISH:
		case DORDER_HELPBUILD:
		case DORDER_BUILDMODULE:
			sOrder.psObj = (BASE_OBJECT *)psDroid->asOrderList[0].psOrderTarget;
			break;
		case DORDER_BUILD:
		case DORDER_LINEBUILD:
			sOrder.psObj = NULL;
			sOrder.psStats = (BASE_STATS *)psDroid->asOrderList[0].psOrderTarget;
			break;
		default:
			ASSERT( false, "orderDroidList: Invalid order" );
			return false;
		}
		sOrder.x	 = psDroid->asOrderList[0].x;
		sOrder.y	 = psDroid->asOrderList[0].y;
		sOrder.x2	 = psDroid->asOrderList[0].x2;
		sOrder.y2	 = psDroid->asOrderList[0].y2;
		sOrder.direction = psDroid->asOrderList[0].direction;
		orderDroidListEraseRange(psDroid, 0, 1);

		//qba'g fraq OHVYQ beqref va zhygvcynlre
		//  && !(fBeqre.beqre == QBEQRE_OHVYQ || fBeqre.beqre == QBEQRE_YVAROHVYQ)
		// Previous comment and code was so mind-bogglingly insane, that it makes more sense after rot13.
		orderDroidBase(psDroid, &sOrder);
		syncDebugDroid(psDroid, 'o');

		return true;
	}

	return false;
}

void orderDroidListEraseRange(DROID *psDroid, unsigned indexBegin, unsigned indexEnd)
{
	// Erase elements
	indexEnd = MIN(indexEnd, psDroid->asOrderList.size());  // Do nothing if trying to pop an empty list.
	psDroid->asOrderList.erase(psDroid->asOrderList.begin() + indexBegin, psDroid->asOrderList.begin() + indexEnd);

	// Update indices into list.
	psDroid->listSize         -= MIN(indexEnd, psDroid->listSize)         - MIN(indexBegin, psDroid->listSize);
	psDroid->listPendingBegin -= MIN(indexEnd, psDroid->listPendingBegin) - MIN(indexBegin, psDroid->listPendingBegin);
}

// clear all the synchronised orders from the list
void orderClearDroidList(DROID *psDroid)
{
	syncDebug("droid%d list cleared", psDroid->id);
	orderDroidListEraseRange(psDroid, 0, psDroid->listSize);
}

void orderClearTargetFromDroidList(DROID *psDroid, BASE_OBJECT *psTarget)
{
	for (unsigned i = 0; i < psDroid->asOrderList.size(); ++i)
	{
		if (psDroid->asOrderList[i].psOrderTarget == psTarget)
		{
			if (i < psDroid->listSize)
			{
				syncDebug("droid%d list erase%d", psDroid->id, psTarget->id);
			}
			orderDroidListEraseRange(psDroid, i, i + 1);
			--i;  // If this underflows, the ++i will overflow it back.
		}
	}
}

// check all the orders in the list for died objects
void orderCheckList(DROID *psDroid)
{
	for (unsigned i = 0; i < psDroid->asOrderList.size(); ++i)
	{
		DROID_ORDER order = psDroid->asOrderList[i].order;
		//if order requires an object
		if (order == DORDER_ATTACK ||
		    order == DORDER_REPAIR ||
		    order == DORDER_OBSERVE ||
		    order == DORDER_DROIDREPAIR ||
		    order == DORDER_FIRESUPPORT ||
		    order == DORDER_CLEARWRECK ||
		    order == DORDER_DEMOLISH ||
		    order == DORDER_HELPBUILD ||
		    order == DORDER_BUILDMODULE)
		{
			BASE_OBJECT *psTarget = (BASE_OBJECT *)psDroid->asOrderList[i].psOrderTarget;
			if (psTarget != NULL && psTarget->died)
			{
				if (i < psDroid->listSize)
				{
					syncDebug("droid%d list erase dead droid%d", psDroid->id, psTarget->id);
				}
				orderDroidListEraseRange(psDroid, i, i + 1);
				--i;  // If this underflows, the ++i will overflow it back.
			}
		}
	}
}


// add a location order to a droids order list
static bool orderDroidLocAdd(DROID *psDroid, DROID_ORDER order, UDWORD x, UDWORD y, bool add = true)
{
	// can only queue move, scout, and disembark orders
	if (order != DORDER_MOVE && order != DORDER_SCOUT && order != DORDER_DISEMBARK)
	{
		return false;
	}

	sendDroidInfo(psDroid,  order,  x, y, NULL, NULL, 0, 0, 0,  add);

	return true;
}


// add an object order to a droids order list
static BOOL orderDroidObjAdd(DROID *psDroid, DROID_ORDER order, BASE_OBJECT *psObj[DROID_MAXWEAPS])
{
	ASSERT(!isBlueprint(psObj[0]), "Target is a blueprint");

	// check can queue the order
	if (order != DORDER_ATTACK &&
		order != DORDER_REPAIR &&
		order != DORDER_OBSERVE &&
		order != DORDER_DROIDREPAIR &&
		order != DORDER_FIRESUPPORT &&
		order != DORDER_CLEARWRECK &&
        order != DORDER_DEMOLISH &&
        order != DORDER_HELPBUILD &&
        order != DORDER_BUILDMODULE)
	{
		return false;
	}

	sendDroidInfo(psDroid,  order,  psObj[0]->pos.x, psObj[0]->pos.y, psObj[0], NULL, 0, 0, 0,  true);

	return true;
}

/* Choose an order for a droid from a location */
DROID_ORDER chooseOrderLoc(DROID *psDroid, UDWORD x,UDWORD y, BOOL altOrder)
{
	DROID_ORDER		order = DORDER_NONE;
	PROPULSION_TYPE		propulsion = getPropulsionStats(psDroid)->propulsionType;

	if (psDroid->droidType == DROID_TRANSPORTER && game.type == CAMPAIGN)
	{
		// transports can't be controlled in campaign
		return DORDER_NONE;
	}

	// default to move; however, we can only end up on a tile
	// where can stay, ie VTOLs must be able to land as well
	if (isVtolDroid(psDroid))
	{
		propulsion = PROPULSION_TYPE_WHEELED;
	}
	if (!fpathBlockingTile(map_coord(x), map_coord(y), propulsion))
	{
		order = DORDER_MOVE;
	}

	// scout if alt was pressed
	if (altOrder)
	{
		order = DORDER_SCOUT;
		if (isVtolDroid(psDroid))
		{
			// Patrol if in a VTOL
			order = DORDER_PATROL;
		}
	}

	// and now we want Transporters to fly! - in multiPlayer!!
	if (psDroid->droidType == DROID_TRANSPORTER && game.type == SKIRMISH)
	{
		/* in MultiPlayer - if ALT-key is pressed then need to get the Transporter
		 * to fly to location and all units disembark */
		if (altOrder)
		{
			order = DORDER_DISEMBARK;
		}
	}
	else if (secondaryGetState(psDroid, DSO_CIRCLE) == DSS_CIRCLE_SET)
	{
		order = DORDER_CIRCLE;
		secondarySetState(psDroid, DSO_CIRCLE, DSS_NONE);
	}
	else if (secondaryGetState(psDroid, DSO_PATROL) == DSS_PATROL_SET)
	{
		order = DORDER_PATROL;
		secondarySetState(psDroid, DSO_PATROL, DSS_NONE);
	}

	return order;
}


/* Give selected droids an order from a location target or
   move selected Delivery Point to new location

   If add is true then the order is queued in the droid
*/
void orderSelectedLoc(uint32_t player, uint32_t x, uint32_t y, bool add)
{
	DROID			*psCurr;
	DROID_ORDER		order;

	//if were in build select mode ignore all other clicking
	if (intBuildSelectMode())
	{
		return;
	}

	// remove any units from their command group
	for(psCurr = apsDroidLists[player]; psCurr; psCurr=psCurr->psNext)
	{
		if (psCurr->selected && hasCommander(psCurr))
		{
			grpLeave(psCurr->psGroup, psCurr);
		}
	}

	// note that an order list graphic needs to be displayed
	bOrderEffectDisplayed = false;

	for(psCurr = apsDroidLists[player]; psCurr; psCurr=psCurr->psNext)
	{
		if (psCurr->selected)
		{
			// can't use bMultiPlayer since multimsg could be off.
			if (psCurr->droidType == DROID_TRANSPORTER && game.type == CAMPAIGN)
			{
				// Transport in campaign cannot be controlled by players
				DeSelectDroid(psCurr);
				continue;
			}

			order = chooseOrderLoc(psCurr, x, y, (keyDown(KEY_LALT) || keyDown(KEY_RALT)));
			// see if the order can be added to the list
			if (order != DORDER_NONE && !(add && orderDroidLocAdd(psCurr, order, x, y)))
			{
				// if not just do it straight off
				orderDroidLoc(psCurr, order, x, y, ModeQueue);
			}
		}
	}
}


/* Choose an order for a droid from an object */
DROID_ORDER chooseOrderObj(DROID *psDroid, BASE_OBJECT *psObj, BOOL altOrder)
{
	DROID_ORDER		order;
	STRUCTURE		*psStruct;
	FEATURE			*psFeature;

	if (psDroid->droidType == DROID_TRANSPORTER)
	{
        //in multiPlayer, need to be able to get Transporter repaired
        if (bMultiPlayer)
        {
            if (aiCheckAlliances(psObj->player, psDroid->player) &&
			  psObj->type == OBJ_STRUCTURE)
            {
		        psStruct = (STRUCTURE *) psObj;
		        ASSERT( psObj != NULL,
				        "chooseOrderObj: invalid structure pointer" );
			    if ( psStruct->pStructureType->type == REF_REPAIR_FACILITY &&
				     psStruct->status == SS_BUILT)
			    {
				    return DORDER_RTR_SPECIFIED;
			    }
            }
		}
		return DORDER_NONE;
	}

	if (altOrder && (psObj->type == OBJ_DROID || psObj->type == OBJ_STRUCTURE) && psDroid->player == psObj->player)
	{
		if ((psDroid->droidType == DROID_WEAPON) || cyborgDroid(psDroid) ||
			(psDroid->droidType == DROID_COMMAND))
		{
			return DORDER_ATTACK;
		}
		else if (psDroid->droidType == DROID_SENSOR)
		{
			return DORDER_OBSERVE;
		}
		else if ((psDroid->droidType == DROID_REPAIR ||
		         psDroid->droidType == DROID_CYBORG_REPAIR) && psObj->type == OBJ_DROID)
		{
			return DORDER_REPAIR;
		}
	}
	//check for transporters first
	if (psObj->type == OBJ_DROID && ((DROID *)psObj)->droidType == DROID_TRANSPORTER && psObj->player == psDroid->player)
	{
		order = DORDER_EMBARK;
	}
	// go to recover an artifact/oil drum - don't allow VTOL's to get this order
	else if (psObj->type == OBJ_FEATURE &&
		(((FEATURE *)psObj)->psStats->subType == FEAT_GEN_ARTE ||
		 ((FEATURE *)psObj)->psStats->subType == FEAT_OIL_DRUM) )
	{
		if (isVtolDroid(psDroid))
		{
			order = DORDER_NONE;
		}
		else
		{
			order = DORDER_RECOVER;
		}
	}
	// else default to attack if the droid has a weapon
	else if (psDroid->numWeaps > 0
			&& psObj->player != psDroid->player
			&& !aiCheckAlliances(psObj->player , psDroid->player) )
	{
		// check valid weapon/prop combination
		if (!validTarget((BASE_OBJECT *)psDroid, psObj, 0))
		{
			order = DORDER_NONE;
		}
		else
		{
			order = DORDER_ATTACK;
		}
	}
	else if (psDroid->droidType == DROID_SENSOR
			&& psObj->player != psDroid->player
			&& !aiCheckAlliances(psObj->player , psDroid->player) )
	{
		//check for standard sensor or VTOL intercept sensor
		if (asSensorStats[psDroid->asBits[COMP_SENSOR].nStat].type == STANDARD_SENSOR
		    || asSensorStats[psDroid->asBits[COMP_SENSOR].nStat].type == VTOL_INTERCEPT_SENSOR
		    || asSensorStats[psDroid->asBits[COMP_SENSOR].nStat].type == SUPER_SENSOR)
		{
 			// a sensor droid observing an object
			order = DORDER_OBSERVE;
		}
		else
		{
			order = DORDER_NONE;
		}
	}
	else if (droidSensorDroidWeapon(psObj, psDroid))
	{
		// got an indirect weapon droid or vtol doing fire support
		order = DORDER_FIRESUPPORT;
		setSensorAssigned();
	}
	else if ( psObj->player == psDroid->player &&
			  psObj->type == OBJ_DROID &&
			  ((DROID *)psObj)->droidType == DROID_COMMAND &&
			  psDroid->droidType != DROID_COMMAND &&
			  psDroid->droidType != DROID_CONSTRUCT &&
              psDroid->droidType != DROID_CYBORG_CONSTRUCT)
	{
		// get a droid to join a command droids group
		cmdDroidAddDroid((DROID *) psObj, psDroid);
		DeSelectDroid(psDroid);
		order = DORDER_NONE;
	}
	//repair droid
	else if (aiCheckAlliances(psObj->player, psDroid->player) &&
		psObj->type == OBJ_DROID &&
        (psDroid->droidType == DROID_REPAIR ||
        psDroid->droidType == DROID_CYBORG_REPAIR) &&
		droidIsDamaged((DROID *)psObj))
	{
		order = DORDER_DROIDREPAIR;
	}
	// guarding constructor droids
	else if (aiCheckAlliances(psObj->player, psDroid->player) &&
			 psObj->type == OBJ_DROID &&
			 (((DROID *)psObj)->droidType == DROID_CONSTRUCT ||
             ((DROID *)psObj)->droidType == DROID_CYBORG_CONSTRUCT ||
             ((DROID *)psObj)->droidType == DROID_SENSOR ||
			 (((DROID *)psObj)->droidType == DROID_COMMAND && psObj->player != psDroid->player)) &&
			 (psDroid->droidType == DROID_WEAPON ||
			  psDroid->droidType == DROID_CYBORG ||
			  psDroid->droidType == DROID_CYBORG_SUPER) &&
			 proj_Direct(asWeaponStats + psDroid->asWeaps[0].nStat))
	{
		order = DORDER_GUARD;
		assignSensorTarget(psObj);
		psDroid->selected = false;
	}
	else if ( aiCheckAlliances(psObj->player, psDroid->player) &&
			  psObj->type == OBJ_STRUCTURE )
	{
		psStruct = (STRUCTURE *) psObj;
		ASSERT(psObj != NULL, "Invalid structure pointer");

		/* check whether construction droid */
		order = DORDER_NONE;
		if (psDroid->droidType == DROID_CONSTRUCT ||
		    psDroid->droidType == DROID_CYBORG_CONSTRUCT)
		{
			//Re-written to allow demolish order to be added to the queuing system
			if (intDemolishSelectMode() && psObj->player == psDroid->player)
			{
				//check to see if anything is currently trying to build the structure
				//can't build and demolish at the same time!
				if (psStruct->status != SS_BUILT &&
					checkDroidsBuilding(psStruct))
				{
					order = DORDER_NONE;
				}
				else
				{
					order = DORDER_DEMOLISH;
				}
			}
			//check for non complete structures
			else if (psStruct->status != SS_BUILT)
			{
				//if something else is demolishing, then help demolish
				if (checkDroidsDemolishing(psStruct))
				{
					order = DORDER_DEMOLISH;
				}
				//else help build
				else
				{
					order = DORDER_HELPBUILD;
				}
			}
			else if ( psStruct->body < structureBody(psStruct))
			{
				order = DORDER_REPAIR;
			}
			//check if can build a module
			else if (buildModule(psStruct))
			{
				order = DORDER_BUILDMODULE;
			}
			else
			{
				order = DORDER_NONE;
			}
		}

		if (order == DORDER_NONE)
		{
			/* check repair facility and in need of repair */
			if ( psStruct->pStructureType->type == REF_REPAIR_FACILITY &&
				 psStruct->status == SS_BUILT)
			{
				order = DORDER_RTR_SPECIFIED;
			}
			else if (electronicDroid(psDroid) &&
				//psStruct->resistance < (SDWORD)(psStruct->pStructureType->resistance))
				psStruct->resistance < (SDWORD)structureResistance(psStruct->
					pStructureType, psStruct->player))
			{
				order = DORDER_RESTORE;
			}
			//check for counter battery assignment
			else if (structSensorDroidWeapon(psStruct, psDroid))
			{
				order = DORDER_FIRESUPPORT;
				//inform display system
				setSensorAssigned();
				//deselect droid
				DeSelectDroid(psDroid);
			}
			//REARM VTOLS
			else if (isVtolDroid(psDroid))
			{
				//default to no order
				order = DORDER_NONE;
				//check if rearm pad
				if (psStruct->pStructureType->type == REF_REARM_PAD)
				{
					//don't bother checking cos we want it to go there if directed
					order = DORDER_REARM;
				}
			}
			// Some droids shouldn't be guarding
			else if ((psDroid->droidType == DROID_WEAPON ||
			          psDroid->droidType == DROID_CYBORG ||
			          psDroid->droidType == DROID_CYBORG_SUPER)
			         && proj_Direct(asWeaponStats + psDroid->asWeaps[0].nStat))
			{
				order = DORDER_GUARD;
			}
		}
	}
	//check for constructor droid clearing up wrecked buildings
	else if ((psDroid->droidType == DROID_CONSTRUCT || psDroid->droidType == DROID_CYBORG_CONSTRUCT)
	         && psObj->type == OBJ_FEATURE)
	{
		psFeature = (FEATURE *) psObj;
		ASSERT(psObj != NULL, "Invalid feature pointer");
		if (psFeature->psStats->subType == FEAT_BUILD_WRECK)
		{
			order = DORDER_CLEARWRECK;
		}
		else
		{
			order = DORDER_NONE;
		}
	}
	else
	{
		order = DORDER_NONE;
	}

	return order;
}

static void orderPlayOrderObjAudio( UDWORD player, BASE_OBJECT *psObj )
{
	DROID	*psDroid;

	/* loop over selected droids */
	for( psDroid = apsDroidLists[player]; psDroid; psDroid=psDroid->psNext )
	{
		if ( psDroid->selected )
		{
			/* currently only looks for VTOL */
			if ( isVtolDroid( psDroid ) )
			{
				switch ( psDroid->order )
				{
					case DORDER_ATTACK:
						audio_QueueTrack( ID_SOUND_ON_OUR_WAY2 );
						break;
					default:
						break;
				}
			}

			/* only play audio once */
			break;
		}
	}
}

/* Give selected droids an order from an object target
 * If add is true the order is queued with the droid
 */
void orderSelectedObjAdd(UDWORD player, BASE_OBJECT *psObj, BOOL add)
{
	DROID		*psCurr, *psDemolish;
	DROID_ORDER	order;

	// remove any units from their command group
	for(psCurr = apsDroidLists[player]; psCurr; psCurr=psCurr->psNext)
	{
		if (psCurr->selected && hasCommander(psCurr))
		{
			grpLeave(psCurr->psGroup, psCurr);
		}
	}

	// note that an order list graphic needs to be displayed
	bOrderEffectDisplayed = false;

	psDemolish = NULL;
	for (psCurr = apsDroidLists[player]; psCurr; psCurr=psCurr->psNext)
	{
		if (psCurr->selected)
		{
			if (isBlueprint(psObj))
			{
				if (isConstructionDroid(psCurr))
				{
					// Help build the planned structure.
					orderDroidStatsLocDirAdd(psCurr, DORDER_BUILD, castStructure(psObj)->pStructureType, psObj->pos.x, psObj->pos.y, castStructure(psObj)->rot.direction, add);
				}
				else
				{
					// Help watch the structure being built.
					orderDroidLocAdd(psCurr, DORDER_MOVE, psObj->pos.x, psObj->pos.y, add);
				}
				continue;
			}

			order = chooseOrderObj(psCurr, psObj, (keyDown(KEY_LALT) || keyDown(KEY_RALT)));
			if (order == DORDER_DEMOLISH && player == selectedPlayer)
			{
				psDemolish = psCurr;
			}
			// see if the order can be added to the list
			if (!(add && orderDroidObjAdd(psCurr, order, &psObj)))
			{
				// if not just do it straight off
				orderDroidObj(psCurr, order, psObj, ModeQueue);
			}
		}
	}

	orderPlayOrderObjAudio( player, psObj );
}

void orderSelectedObj(UDWORD player, BASE_OBJECT *psObj)
{
	orderSelectedObjAdd(player, psObj, false);
}


/* order all selected droids with a location and a stat */
void orderSelectedStatsLocDir(UDWORD player, DROID_ORDER order, BASE_STATS *psStats, UDWORD x, UDWORD y, uint16_t direction, BOOL add)
{
	DROID		*psCurr;

//turn off the build queue availability until desired release date!
#ifdef DISABLE_BUILD_QUEUE
	add = false;
#endif

	for(psCurr = apsDroidLists[player]; psCurr; psCurr=psCurr->psNext)
	{
		if (psCurr->selected)
		{
			if (add)
			{
				orderDroidStatsLocDirAdd(psCurr, order, psStats, x, y, direction);
			}
			else
			{
				orderDroidStatsLocDir(psCurr, order, psStats, x, y, direction, ModeQueue);
			}
		}
	}
}


/* order all selected droids with two a locations and a stat */
void orderSelectedStatsTwoLocDir(UDWORD player, DROID_ORDER order, BASE_STATS *psStats, UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2, uint16_t direction, BOOL add)
{
	DROID		*psCurr;

//turn off the build queue availability until desired release date!
#ifdef DISABLE_BUILD_QUEUE
	add = false;
#endif

	for(psCurr = apsDroidLists[player]; psCurr; psCurr=psCurr->psNext)
	{
		if (psCurr->selected)
		{
			if (add)
			{
				orderDroidStatsTwoLocDirAdd(psCurr, order, psStats, x1, y1, x2, y2, direction);
			}
			else
			{
				orderDroidStatsTwoLocDir(psCurr, order, psStats, x1, y1, x2, y2, direction, ModeQueue);
			}
		}
	}
}


// See if the player has access to a transporter in this map.
//
DROID *FindATransporter(unsigned player)
{
	DROID *psDroid;

	for (psDroid = apsDroidLists[player]; psDroid != NULL; psDroid = psDroid->psNext)
	{
		if( psDroid->droidType == DROID_TRANSPORTER ) {
			return psDroid;
		}
	}

	return NULL;
}


// See if the player has access to a factory in this map.
//
static STRUCTURE *FindAFactory(UDWORD player, UDWORD factoryType)
{
	STRUCTURE *psStruct;

	ASSERT( player < MAX_PLAYERS,
		"FindAFactory: invalid player number" );

	for(psStruct = apsStructLists[player]; psStruct != NULL; psStruct = psStruct->psNext)
	{
		if(psStruct->pStructureType->type == factoryType)
		{
			return psStruct;
		}
	}

	return NULL;
}


// See if the player has access to a repair facility in this map.
//
static STRUCTURE *FindARepairFacility(unsigned player)
{
	STRUCTURE *psStruct;

	for (psStruct = apsStructLists[player]; psStruct != NULL; psStruct = psStruct->psNext)
	{
		if(psStruct->pStructureType->type == REF_REPAIR_FACILITY) {
			return psStruct;
		}
	}

	return NULL;
}


// see if a droid supports a secondary order
BOOL secondarySupported(DROID *psDroid, SECONDARY_ORDER sec)
{
	BOOL supported;

	supported = true;	// Default to supported.

	switch (sec)
	{
	case DSO_ASSIGN_PRODUCTION:
	case DSO_ASSIGN_CYBORG_PRODUCTION:
	case DSO_ASSIGN_VTOL_PRODUCTION:
	case DSO_CLEAR_PRODUCTION:		// remove production from a command droid
	case DSO_FIRE_DESIGNATOR:
		if (psDroid->droidType != DROID_COMMAND)
		{
			supported = false;
		}
		if ((sec == DSO_ASSIGN_PRODUCTION && FindAFactory(psDroid->player, REF_FACTORY) == NULL) ||
			(sec == DSO_ASSIGN_CYBORG_PRODUCTION && FindAFactory(psDroid->player, REF_CYBORG_FACTORY) == NULL) ||
			(sec == DSO_ASSIGN_VTOL_PRODUCTION && FindAFactory(psDroid->player, REF_VTOL_FACTORY) == NULL))
		{
			supported = false;
		}
		// don't allow factories to be assigned to commanders during a Limbo Expand mission
		if ((sec == DSO_ASSIGN_PRODUCTION || sec == DSO_ASSIGN_CYBORG_PRODUCTION || sec == DSO_ASSIGN_VTOL_PRODUCTION)
		    && missionLimboExpand())
		{
			supported = false;
		}
		break;

	case DSO_ATTACK_RANGE:
	case DSO_ATTACK_LEVEL:
		if (psDroid->droidType == DROID_REPAIR || psDroid->droidType == DROID_CYBORG_REPAIR)
		{
			supported = false;
		}
		if (psDroid->droidType == DROID_CONSTRUCT || psDroid->droidType == DROID_CYBORG_CONSTRUCT)
		{
			supported = false;
		}
		break;

	case DSO_CIRCLE:
		if (!isVtolDroid(psDroid))
		{
			supported = false;
		}
		break;

	case DSO_REPAIR_LEVEL:
	case DSO_PATROL:
	case DSO_HALTTYPE:
	case DSO_RETURN_TO_LOC:
		break;

	case DSO_RECYCLE:			// Only if player has got a factory.
		if ((FindAFactory(psDroid->player, REF_FACTORY) == NULL) &&
			(FindAFactory(psDroid->player, REF_CYBORG_FACTORY) == NULL) &&
			(FindAFactory(psDroid->player, REF_VTOL_FACTORY) == NULL) &&
			(FindARepairFacility(psDroid->player) == NULL))
		{
			supported = false;
		}
		break;

	default:
		supported = false;
		break;
	}

	return supported;
}


// get the state of a secondary order, return false if unsupported
SECONDARY_STATE secondaryGetState(DROID *psDroid, SECONDARY_ORDER sec)
{
	uint32_t state;

	state = psDroid->secondaryOrder;

	switch (sec)
	{
	case DSO_ATTACK_RANGE:
		return (SECONDARY_STATE)(state & DSS_ARANGE_MASK);
		break;
	case DSO_REPAIR_LEVEL:
		return (SECONDARY_STATE)(state & DSS_REPLEV_MASK);
		break;
	case DSO_ATTACK_LEVEL:
		return (SECONDARY_STATE)(state & DSS_ALEV_MASK);
		break;
	case DSO_ASSIGN_PRODUCTION:
	case DSO_ASSIGN_CYBORG_PRODUCTION:
	case DSO_ASSIGN_VTOL_PRODUCTION:
		return (SECONDARY_STATE)(state & DSS_ASSPROD_MASK);
		break;
	case DSO_RECYCLE:
		return (SECONDARY_STATE)(state & DSS_RECYCLE_MASK);
		break;
	case DSO_PATROL:
		return (SECONDARY_STATE)(state & DSS_PATROL_MASK);
		break;
	case DSO_CIRCLE:
		return (SECONDARY_STATE)(state & DSS_CIRCLE_MASK);
		break;
	case DSO_HALTTYPE:
		if (psDroid->order == DORDER_TEMP_HOLD)
		{
			return DSS_HALT_HOLD;
		}
		return (SECONDARY_STATE)(state & DSS_HALT_MASK);
		break;
	case DSO_RETURN_TO_LOC:
		return (SECONDARY_STATE)(state & DSS_RTL_MASK);
		break;
	case DSO_FIRE_DESIGNATOR:
		if (cmdDroidGetDesignator(psDroid->player) == psDroid)
		{
			return DSS_FIREDES_SET;
		}
		break;
	default:
		break;
	}

	return DSS_NONE;
}

#ifdef DEBUG
static char *secondaryPrintFactories(UDWORD state)
{
	SDWORD		i;
	static		char aBuff[255];

	memset(aBuff, 0, sizeof(aBuff));
	for(i=0; i<5; i++)
	{
		if (state & (1 << (i + DSS_ASSPROD_SHIFT)))
		{
			aBuff[i] = (char)('0' + i);
		}
		else
		{
			aBuff[i] = ' ';
		}
		if (state & (1 << (i + DSS_ASSPROD_CYBORG_SHIFT)))
		{
			aBuff[i*2 + 5] = 'c';
			aBuff[i*2 + 6] = (char)('0' + i);
		}
		else
		{
			aBuff[i*2 + 5] = ' ';
			aBuff[i*2 + 6] = ' ';
		}
	}

	return aBuff;
}
#else
#define secondaryPrintFactories(x)
#endif


// check the damage level of a droid against it's secondary state
void secondaryCheckDamageLevel(DROID *psDroid)
{
	SECONDARY_STATE	State;
	unsigned int repairLevel;

	State = secondaryGetState(psDroid, DSO_REPAIR_LEVEL);
	if (State == DSS_REPLEV_LOW)
	{
		repairLevel = REPAIRLEV_HIGH;			//repair often
	}
	else if(State == DSS_REPLEV_HIGH)
	{
		repairLevel = REPAIRLEV_LOW;	 		// don't repair often.
	}
	else
	{
		repairLevel = 0;						//never repair
	}

	//don't bother checking if 'do or die'
	if( repairLevel && PERCENT(psDroid->body,psDroid->originalBody) <= repairLevel)
	{
		if (psDroid->selected)
		{
			DROID* psTempDroid;
			for (psTempDroid = apsDroidLists[selectedPlayer]; psTempDroid; psTempDroid = psTempDroid->psNext)
			{
				if (psTempDroid!=psDroid && psTempDroid->selected)
				{
					DeSelectDroid(psDroid);
					break;
				}
			}
		}
		if (!isVtolDroid(psDroid))
		{
			psDroid->group = UBYTE_MAX;
		}

		/* set return to repair if not on hold */
		if ( psDroid->order != DORDER_RTR &&
			 psDroid->order != DORDER_RTB &&
			 !vtolRearming(psDroid))
		{
			if (isVtolDroid(psDroid))
			{
				moveToRearm(psDroid);
			}
			else
			{
				orderDroid(psDroid, DORDER_RTR, ModeImmediate);
			}
		}
	}
}


// set the state of a secondary order, return false if failed.
BOOL secondarySetState(DROID *psDroid, SECONDARY_ORDER sec, SECONDARY_STATE State)
{
	UDWORD		CurrState, factType, prodType;
	STRUCTURE	*psStruct;
	SDWORD		factoryInc;
	BOOL		retVal;
	DROID		*psTransport, *psCurr, *psNext;
	DROID_ORDER     order;



	if(bMultiMessages)
	{
		sendDroidSecondary(psDroid,sec,State);
		return true;  // Wait for our order before changing the droid.
	}


	// set the state for any droids in the command group
	if ((sec != DSO_RECYCLE) &&
		psDroid->droidType == DROID_COMMAND &&
		psDroid->psGroup != NULL &&
		psDroid->psGroup->type == GT_COMMAND)
	{
		grpSetSecondary(psDroid->psGroup, sec, State);
	}

	CurrState = psDroid->secondaryOrder;

	retVal = true;
	switch (sec) {
		case DSO_ATTACK_RANGE:
			CurrState = (CurrState & ~DSS_ARANGE_MASK) | State;
			break;

		case DSO_REPAIR_LEVEL:
			CurrState = (CurrState & ~DSS_REPLEV_MASK) | State;
			psDroid->secondaryOrder = CurrState;
			secondaryCheckDamageLevel(psDroid);
			break;

		case DSO_ATTACK_LEVEL:
			CurrState = (CurrState & ~DSS_ALEV_MASK) | State;
			if (State == DSS_ALEV_NEVER)
			{
				if (orderState(psDroid, DORDER_ATTACK))
				{
					// just kill these orders
					orderDroid(psDroid, DORDER_STOP, ModeImmediate);
					if (isVtolDroid(psDroid))
					{
						moveToRearm(psDroid);
					}
				}
				else if ( orderState(psDroid, DORDER_GUARD) &&
						  droidAttacking(psDroid))
				{
					// send the unit back to the guard position
					actionDroid(psDroid, DACTION_NONE);
				}
				else if ( orderState(psDroid, DORDER_PATROL) )
				{
					// send the unit back to the patrol
					actionDroidLoc(psDroid, DACTION_RETURNTOPOS, psDroid->actionX, psDroid->actionY);
				}
			}
			break;


		case DSO_ASSIGN_PRODUCTION:
		case DSO_ASSIGN_CYBORG_PRODUCTION:
		case DSO_ASSIGN_VTOL_PRODUCTION:
#ifdef DEBUG
			debug( LOG_NEVER, "order factories %s\n", secondaryPrintFactories(State));
#endif
			if ( sec == DSO_ASSIGN_PRODUCTION)
			{
				prodType = REF_FACTORY;
			}
			else if ( sec == DSO_ASSIGN_CYBORG_PRODUCTION)
			{
				prodType = REF_CYBORG_FACTORY;
			}
			else
			{
				prodType = REF_VTOL_FACTORY;
			}

			if (psDroid->droidType == DROID_COMMAND)
			{
				// look for the factories
				for (psStruct = apsStructLists[psDroid->player]; psStruct;
					 psStruct=psStruct->psNext)
				{
					factType = psStruct->pStructureType->type;
					if ( factType == REF_FACTORY ||
						 factType == REF_VTOL_FACTORY ||
						 factType == REF_CYBORG_FACTORY )
					{
						factoryInc = ((FACTORY*)psStruct->pFunctionality)->psAssemblyPoint->factoryInc;
						if (factType == REF_FACTORY)
						{
							factoryInc += DSS_ASSPROD_SHIFT;
						}
						else if ( factType == REF_CYBORG_FACTORY )
						{
							factoryInc += DSS_ASSPROD_CYBORG_SHIFT;
						}
						else
						{
							factoryInc += DSS_ASSPROD_VTOL_SHIFT;
						}
						if ( !( CurrState & ( 1 << factoryInc) ) &&
							  ( State & ( 1 << factoryInc) ) )
						{
							assignFactoryCommandDroid(psStruct, psDroid);// assign this factory to the command droid
						}
						else if ( ( prodType == factType ) &&
								  ( CurrState & ( 1 << factoryInc) ) &&
								  !( State & ( 1 << factoryInc) ) )
						{
							// remove this factory from the command droid
							assignFactoryCommandDroid(psStruct, NULL);
						}
					}
				}
				if (prodType == REF_FACTORY)
				{
					CurrState &= ~DSS_ASSPROD_FACT_MASK;
				}
				else if (prodType == REF_CYBORG_FACTORY)
				{
					CurrState &= ~DSS_ASSPROD_CYB_MASK;
				}
				else
				{
					CurrState &= ~DSS_ASSPROD_VTOL_MASK;
				}
				CurrState |= (State & DSS_ASSPROD_MASK);
#ifdef DEBUG
				debug( LOG_NEVER, "final factories %s\n", secondaryPrintFactories(CurrState));
#endif
			}
			break;

		case DSO_CLEAR_PRODUCTION:
			if (psDroid->droidType == DROID_COMMAND)
			{
				// simply clear the flag - all the factory stuff is done in assignFactoryCommandDroid
				CurrState &= ~ (State & DSS_ASSPROD_MASK);
			}
			break;


		case DSO_RECYCLE:
			if(State & DSS_RECYCLE_MASK)
			{
				if (!orderState(psDroid, DORDER_RECYCLE))
				{
					orderDroid(psDroid, DORDER_RECYCLE, ModeImmediate);
				}
				CurrState &= ~(DSS_RTL_MASK|DSS_RECYCLE_MASK|DSS_HALT_MASK);
				CurrState |= DSS_RECYCLE_SET|DSS_HALT_GUARD;
				psDroid->group = UBYTE_MAX;
				if (psDroid->psGroup != NULL)
				{
					if (psDroid->droidType == DROID_COMMAND)
					{
						// remove all the units from the commanders group
						for (psCurr = psDroid->psGroup->psList; psCurr; psCurr=psNext)
						{
							psNext = psCurr->psGrpNext;
							grpLeave(psCurr->psGroup, psCurr);
							orderDroid(psCurr, DORDER_STOP, ModeImmediate);
						}
					}
					else if (psDroid->psGroup->type == GT_COMMAND)
					{
						grpLeave(psDroid->psGroup, psDroid);
					}
				}
			}
			else
			{
				if (orderState(psDroid, DORDER_RECYCLE))
				{
					orderDroid(psDroid, DORDER_STOP, ModeImmediate);
				}
				CurrState &= ~DSS_RECYCLE_MASK;
			}
			break;
		case DSO_CIRCLE:
			if (State & DSS_CIRCLE_SET)
			{
				CurrState |= DSS_CIRCLE_SET;
			}
			else
			{
				CurrState &= ~DSS_CIRCLE_MASK;
			}
			break;
		case DSO_PATROL:
			if (State & DSS_PATROL_SET)
			{
				CurrState |= DSS_PATROL_SET;
			}
			else
			{
				CurrState &= ~DSS_PATROL_MASK;
			}
			break;
		case DSO_HALTTYPE:
			switch (State & DSS_HALT_MASK)
			{
			case DSS_HALT_PURSUE:
				CurrState &= ~ DSS_HALT_MASK;
				CurrState |= DSS_HALT_PURSUE;
				if (orderState(psDroid, DORDER_GUARD))
				{
					orderDroid(psDroid, DORDER_STOP, ModeImmediate);
				}
				break;
			case DSS_HALT_GUARD:
				CurrState &= ~ DSS_HALT_MASK;
				CurrState |= DSS_HALT_GUARD;
				orderDroidLoc(psDroid, DORDER_GUARD, psDroid->pos.x, psDroid->pos.y, ModeImmediate);
				break;
			case DSS_HALT_HOLD:
				CurrState &= ~ DSS_HALT_MASK;
				CurrState |= DSS_HALT_HOLD;
				if (!orderState(psDroid, DORDER_FIRESUPPORT))
				{
					orderDroid(psDroid, DORDER_STOP, ModeImmediate);
				}
				break;
			}
			break;
		case DSO_RETURN_TO_LOC:
			if ((State & DSS_RTL_MASK) == 0)
			{
				if (orderState(psDroid, DORDER_RTR) ||
					orderState(psDroid, DORDER_RTB) ||
					orderState(psDroid, DORDER_EMBARK))
				{
					orderDroid(psDroid, DORDER_STOP, ModeImmediate);
				}
				CurrState &= ~DSS_RTL_MASK;
			}
			else
			{
				order = DORDER_NONE;
				CurrState &= ~DSS_RTL_MASK;
				if ((CurrState & DSS_HALT_MASK) == DSS_HALT_HOLD)
				{
					CurrState &= ~DSS_HALT_MASK;
					CurrState |= DSS_HALT_GUARD;
				}
				switch (State & DSS_RTL_MASK)
				{
				case DSS_RTL_REPAIR:
					order = DORDER_RTR;
					CurrState |= DSS_RTL_REPAIR;
					// can't clear the selection here cos it breaks the secondary order screen
					break;
				case DSS_RTL_BASE:
					order = DORDER_RTB;
					CurrState |= DSS_RTL_BASE;
					break;
				case DSS_RTL_TRANSPORT:
					psTransport = FindATransporter(psDroid->player);
					if (psTransport != NULL)
					{
						// in multiPlayer can only put cyborgs onto a Transporter
						if (bMultiPlayer && !cyborgDroid(psDroid))
						{
							retVal = false;
						}
						else
						{
							order = DORDER_EMBARK;
							CurrState |= DSS_RTL_TRANSPORT;
							if (!orderState(psDroid, DORDER_EMBARK))
							{
								orderDroidObj(psDroid, DORDER_EMBARK, (BASE_OBJECT *)psTransport, ModeImmediate);
							}
						}
					}
					else
					{
						retVal = false;
					}
					break;
				default:
					order = DORDER_NONE;
					break;
				}
				if (!orderState(psDroid, order))
				{
					orderDroid(psDroid, order, ModeImmediate);
				}
			}
			break;

		case DSO_FIRE_DESIGNATOR:
			// don't actually set any secondary flags - the cmdDroid array is
			// always used to determine which commander is the designator
			if (State & DSS_FIREDES_SET)
			{
				cmdDroidSetDesignator(psDroid);
			}
			else if (cmdDroidGetDesignator(psDroid->player) == psDroid)
			{
				cmdDroidClearDesignator(psDroid->player);
			}
			break;

		default:
			break;
	}

	psDroid->secondaryOrder = CurrState;

	return retVal;
}


// deal with a droid receiving a primary order
BOOL secondaryGotPrimaryOrder(DROID *psDroid, DROID_ORDER order)
{
	UDWORD	oldState;

	if (psDroid->droidType == DROID_TRANSPORTER)
	{
		return false;
	}

	if (order != DORDER_NONE &&
		order != DORDER_STOP &&
		order != DORDER_DESTRUCT &&
		order != DORDER_GUARD)
	{
		//reset 2ndary order
		oldState = psDroid->secondaryOrder;
		psDroid->secondaryOrder &= ~ (DSS_RTL_MASK|DSS_RECYCLE_MASK|DSS_PATROL_MASK);


		if((oldState != psDroid->secondaryOrder) &&
		   (psDroid->player == selectedPlayer))
		{
			intRefreshScreen();
		}
	}

	return false;
}


// set the state of a numeric group
static void secondarySetGroupState(UDWORD player, UDWORD group, SECONDARY_ORDER sec, SECONDARY_STATE state)
{
	DROID	*psCurr;

	for(psCurr = apsDroidLists[player]; psCurr; psCurr=psCurr->psNext)
	{
		if (psCurr->group == group &&
			secondaryGetState(psCurr, sec) != state)
		{
			secondarySetState(psCurr, sec, state);
		}
	}
}

// get the average secondary state of a numeric group
static SECONDARY_STATE secondaryGetAverageGroupState(UDWORD player, UDWORD group, UDWORD mask)
{
#define MAX_STATES		5
	struct { UDWORD state, num; } aStateCount[MAX_STATES];
	SDWORD	i, numStates, max;
	DROID	*psCurr;

	// count the number of units for each state
	numStates = 0;
	memset(aStateCount, 0, sizeof(aStateCount));
	for(psCurr=apsDroidLists[player]; psCurr; psCurr=psCurr->psNext)
	{
		if (psCurr->group == group)
		{
			for (i=0; i<numStates; i++)
			{
				if (aStateCount[i].state == (psCurr->secondaryOrder & mask))
				{
					aStateCount[i].num += 1;
					break;
				}
			}

			if (i == numStates)
			{
				aStateCount[numStates].state = psCurr->secondaryOrder & mask;
				aStateCount[numStates].num = 1;
				numStates += 1;
			}
		}
	}

	max = 0;
	for (i=0; i<numStates; i++)
	{
		if (aStateCount[i].num > aStateCount[max].num)
		{
			max = i;
		}
	}

	return (SECONDARY_STATE)aStateCount[max].state;
}


// make all the members of a numeric group have the same secondary states
void secondarySetAverageGroupState(UDWORD player, UDWORD group)
{
	// lookup table for orders and masks
	#define MAX_ORDERS	4
	struct { SECONDARY_ORDER order; UDWORD mask; } aOrders[MAX_ORDERS] =
	{
		{ DSO_ATTACK_RANGE, DSS_ARANGE_MASK },
		{ DSO_REPAIR_LEVEL, DSS_REPLEV_MASK },
		{ DSO_ATTACK_LEVEL, DSS_ALEV_MASK },
		{ DSO_HALTTYPE, DSS_HALT_MASK }
	};
	SDWORD	i, state;

	for(i=0; i<MAX_ORDERS; i++)
	{
		state = secondaryGetAverageGroupState(player, group, aOrders[i].mask);
		secondarySetGroupState(player, group, aOrders[i].order, (SECONDARY_STATE)state);
	}
}


// do a moral check for a player
void orderMoralCheck(UDWORD player)
{
	DROID	*psCurr;
	SDWORD	units, numVehicles, leadership, personLShip, check;

	// count the number of vehicles and units on the side
	units=0;
	numVehicles = 0;
	for(psCurr=apsDroidLists[player]; psCurr; psCurr=psCurr->psNext)
	{
		units += 1;
		if (psCurr->droidType != DROID_PERSON)
		{
			numVehicles += 1;
		}
	}

	if (units > asRunData[player].forceLevel)
	{
		// too many units, don't run
		return;
	}

	// calculate the overall leadership
	leadership = asRunData[player].leadership + 10;
	personLShip = asRunData[player].leadership + numVehicles * 3;

	// do the moral check for each droid
	for(psCurr = apsDroidLists[player]; psCurr; psCurr=psCurr->psNext)
	{
		if (orderState(psCurr, DORDER_RUN) ||
			orderState(psCurr, DORDER_RUNBURN) ||
			orderState(psCurr, DORDER_RETREAT) ||
			orderState(psCurr, DORDER_RTB) ||
			orderState(psCurr, DORDER_RTR) ||
			orderState(psCurr, DORDER_DESTRUCT))
		{
			// already running - ignore
			continue;
		}

		check = gameRand(100);
		syncDebug("Run check = %d", check);
		if (psCurr->droidType == DROID_PERSON)
		{
			if (check > personLShip)
			{
				syncDebug("Person running.");
				orderDroid(psCurr, DORDER_RUN, ModeImmediate);
			}
		}
		else
		{
			if (check > leadership)
			{
				syncDebug("Droid running.");
				orderDroid(psCurr, DORDER_RUN, ModeImmediate);
			}
		}
	}
}

// do a moral check for a group
// Near-duplicate of orderMoralCheck().
void orderGroupMoralCheck(DROID_GROUP *psGroup)
{
	DROID		*psCurr;
	SDWORD		units, numVehicles, leadership, personLShip, check;
	RUN_DATA	*psRunData;

	// count the number of vehicles and units on the side
	units=0;
	numVehicles = 0;
	for(psCurr=psGroup->psList; psCurr; psCurr=psCurr->psGrpNext)
	{
		units += 1;
		if (psCurr->droidType != DROID_PERSON)
		{
			numVehicles += 1;
		}
	}

	psRunData = &psGroup->sRunData;
	if (units > psRunData->forceLevel)
	{
		// too many units, don't run
		return;
	}

	// calculate the overall leadership
	leadership = psRunData->leadership + 10;
	personLShip = psRunData->leadership + numVehicles * 3;

	// do the moral check for each droid
	for(psCurr = psGroup->psList; psCurr; psCurr=psCurr->psGrpNext)
	{
		if (orderState(psCurr, DORDER_RUN) ||
			orderState(psCurr, DORDER_RUNBURN) ||
			orderState(psCurr, DORDER_RETREAT) ||
			orderState(psCurr, DORDER_RTB) ||
			orderState(psCurr, DORDER_RTR) ||
			orderState(psCurr, DORDER_DESTRUCT))
		{
			// already running - ignore
			continue;
		}

		check = gameRand(100);
		syncDebug("Run check = %d", check);
		if (psCurr->droidType == DROID_PERSON)
		{
			if (check > personLShip)
			{
				syncDebug("Person running.");
				orderDroidLoc(psCurr, DORDER_RUN, psRunData->sPos.x, psRunData->sPos.y, ModeImmediate);
			}
		}
		else
		{
			if (check > leadership)
			{
				syncDebug("Droid running.");
				orderDroidLoc(psCurr, DORDER_RUN, psRunData->sPos.x, psRunData->sPos.y, ModeImmediate);
			}
		}
	}
}

// do a health check for a droid
void orderHealthCheck(DROID *psDroid)
{
	DROID		*psCurr;
	SBYTE       healthLevel = 0;
	UDWORD      retreatX = 0, retreatY = 0;

	if (psDroid->droidType == DROID_TRANSPORTER)
	{
		return;
	}

	// get the health value to compare with
	if (psDroid->psGroup)
	{
		healthLevel = psDroid->psGroup->sRunData.healthLevel;
		retreatX = psDroid->psGroup->sRunData.sPos.x;
		retreatY = psDroid->psGroup->sRunData.sPos.y;
	}

	// if health has not been set for the group - use players'
	if (!healthLevel)
	{
		healthLevel = asRunData[psDroid->player].healthLevel;
	}

	// if not got a health level set then ignore
	if (!healthLevel)
	{
		return;
	}

	// if pos has not been set for the group - use players'
	if (retreatX == 0 && retreatY == 0)
	{
		retreatX = asRunData[psDroid->player].sPos.x;
		retreatY = asRunData[psDroid->player].sPos.y;
	}

	if (PERCENT(psDroid->body, psDroid->originalBody) < healthLevel)
	{
		// order this droid to turn and run - // if already running - ignore
		if (!(orderState(psDroid, DORDER_RUN) ||
			orderState(psDroid, DORDER_RUNBURN) ||
			orderState(psDroid, DORDER_RETREAT) ||
			orderState(psDroid, DORDER_RTB) ||
			orderState(psDroid, DORDER_RTR) ||
			orderState(psDroid, DORDER_DESTRUCT)))
		{
			syncDebug("Droid running.");
			orderDroidLoc(psDroid, DORDER_RUN, retreatX, retreatY, ModeImmediate);
		}

		// order each unit in the same group to run
		if (psDroid->psGroup)
		{
			for(psCurr = psDroid->psGroup->psList; psCurr; psCurr=psCurr->psGrpNext)
			{
				if (orderState(psCurr, DORDER_RUN) || orderState(psCurr, DORDER_RUNBURN) || orderState(psCurr, DORDER_RETREAT)
				    || orderState(psCurr, DORDER_RTB) || orderState(psCurr, DORDER_RTR) || orderState(psCurr, DORDER_DESTRUCT))
				{
					// already running - ignore
					continue;
				}
				syncDebug("Group running.");
				orderDroidLoc(psCurr, DORDER_RUN, retreatX, retreatY, ModeImmediate);
			}
		}
	}
}

// set the state of a secondary order for a Factory, return false if failed.
BOOL setFactoryState(STRUCTURE *psStruct, SECONDARY_ORDER sec, SECONDARY_STATE State)
{
	UDWORD		CurrState;
	BOOL		retVal;
	FACTORY     *psFactory;


	if (!StructIsFactory(psStruct))
	{
		ASSERT( false, "setFactoryState: structure is not a factory" );
		return false;
	}

	psFactory = (FACTORY *)psStruct->pFunctionality;

	CurrState = psFactory->secondaryOrder;

	retVal = true;
	switch (sec) {
		case DSO_ATTACK_RANGE:
			CurrState = (CurrState & ~DSS_ARANGE_MASK) | State;
			break;

		case DSO_REPAIR_LEVEL:
			CurrState = (CurrState & ~DSS_REPLEV_MASK) | State;
			break;

		case DSO_ATTACK_LEVEL:
			CurrState = (CurrState & ~DSS_ALEV_MASK) | State;
			break;

		case DSO_PATROL:
			if (State & DSS_PATROL_SET)
			{
				CurrState |= DSS_PATROL_SET;
			}
			else
			{
				CurrState &= ~DSS_PATROL_MASK;
			}
			break;
		case DSO_HALTTYPE:
			switch (State & DSS_HALT_MASK)
			{
			case DSS_HALT_PURSUE:
				CurrState &= ~ DSS_HALT_MASK;
				CurrState |= DSS_HALT_PURSUE;
				break;
			case DSS_HALT_GUARD:
				CurrState &= ~ DSS_HALT_MASK;
				CurrState |= DSS_HALT_GUARD;
				break;
			case DSS_HALT_HOLD:
				CurrState &= ~ DSS_HALT_MASK;
				CurrState |= DSS_HALT_HOLD;
				break;
			}
			break;
		default:
			break;
	}

	psFactory->secondaryOrder = CurrState;

	return retVal;
}

// get the state of a secondary order for a Factory, return false if unsupported
BOOL getFactoryState(STRUCTURE *psStruct, SECONDARY_ORDER sec, SECONDARY_STATE *pState)
{
	UDWORD	state;

	ASSERT_OR_RETURN(false, StructIsFactory(psStruct), "Structure is not a factory");

	state = ((FACTORY *)psStruct->pFunctionality)->secondaryOrder;

	switch (sec)
	{
	case DSO_ATTACK_RANGE:
		*pState = (SECONDARY_STATE)(state & DSS_ARANGE_MASK);
		break;
	case DSO_REPAIR_LEVEL:
		*pState = (SECONDARY_STATE)(state & DSS_REPLEV_MASK);
		break;
	case DSO_ATTACK_LEVEL:
		*pState = (SECONDARY_STATE)(state & DSS_ALEV_MASK);
		break;
	case DSO_PATROL:
		*pState = (SECONDARY_STATE)(state & DSS_PATROL_MASK);
		break;
	case DSO_HALTTYPE:
		*pState = (SECONDARY_STATE)(state & DSS_HALT_MASK);
		break;
	default:
		*pState = (SECONDARY_STATE)0;
		break;
	}

	return true;
}

//lasSat structure can select a target
void orderStructureObj(UDWORD player, BASE_OBJECT *psObj)
{
	STRUCTURE   *psStruct;

	for (psStruct = apsStructLists[player]; psStruct; psStruct = psStruct->psNext)
	{
		if (lasSatStructSelected(psStruct))
		{
			// Lassats have just one weapon
			unsigned int firePause = weaponFirePause(&asWeaponStats[psStruct->asWeaps[0].nStat], (UBYTE)player);
			unsigned int damLevel = PERCENT(psStruct->body, structureBody(psStruct));

			if (damLevel < HEAVY_DAMAGE_LEVEL)
			{
				firePause += firePause;
			}

			if (isHumanPlayer(player)
				&& (gameTime - psStruct->asWeaps[0].lastFired <= firePause) )
			{
				/* Too soon to fire again */
				break;
			}

			// send the weapon fire
			if(bMultiMessages)
			{
				sendLasSat(player,psStruct,psObj);
			}
			else
			{
				//ok to fire - so fire away
				proj_SendProjectile(&psStruct->asWeaps[0], NULL, player, psObj->pos, psObj, true, 0);
				//set up last fires time
				psStruct->asWeaps[0].lastFired =  gameTime;

				//play 5 second countdown message
				audio_QueueTrackPos(ID_SOUND_LAS_SAT_COUNTDOWN, psObj->pos.x, psObj->pos.y, psObj->pos.z);
			}

			break;
		}
	}
}

const char* getDroidOrderName(DROID_ORDER order)
{
	static const char* name[] =
	{
		"DORDER_NONE",				// no order set
		"DORDER_STOP",				// stop the current order
		"DORDER_MOVE",				// 2 - move to a location
		"DORDER_ATTACK",				// attack an enemy
		"DORDER_BUILD",				// 4 - build a structure
		"DORDER_HELPBUILD",			// help to build a structure
		"DORDER_LINEBUILD",			// 6 - build a number of structures in a row (walls + bridges)
		"DORDER_DEMOLISH",			// demolish a structure
		"DORDER_REPAIR",				// 8 - repair a structure
		"DORDER_OBSERVE",				// keep a target in sensor view
		"DORDER_FIRESUPPORT",			// 10 - attack whatever the linked sensor droid attacks
		"DORDER_RETREAT",				// return to the players retreat position
		"DORDER_DESTRUCT",			// 12 - self destruct
		"DORDER_RTB",					// return to base
		"DORDER_RTR",					// 14 - return to repair at any repair facility
		"DORDER_RUN",					// run away after moral failure
		"DORDER_EMBARK",				// 16 - board a transporter
		"DORDER_DISEMBARK",			// get off a transporter
		"DORDER_ATTACKTARGET",		// 18 - a suggestion to attack something
									// i.e. the target was chosen because the droid could see it
		"DORDER_COMMAND",				// a command droid issuing orders to it's group
		"DORDER_BUILDMODULE",			// 20 - build a module (power, research or factory)
		"DORDER_RECYCLE",				// return to factory to be recycled
		"DORDER_TRANSPORTOUT",		// 22 - offworld transporter order
		"DORDER_TRANSPORTIN",			// onworld transporter order
		"DORDER_TRANSPORTRETURN",		// 24 - transporter return after unloading
		"DORDER_GUARD",				// guard a structure
		"DORDER_DROIDREPAIR",			// 26 - repair a droid
		"DORDER_RESTORE",				// restore resistance points for a structure
		"DORDER_SCOUT",				// 28 - same as move, but stop if an enemy is seen
		"DORDER_RUNBURN",				// run away on fire
		"DORDER_CLEARWRECK",			// 30 - constructor droid to clear up building wreckage
		"DORDER_PATROL",				// move between two way points
		"DORDER_REARM",				// 32 - order a vtol to rearming pad
		"DORDER_MOVE_ATTACKWALL",		// move to a location taking out a blocking wall on the way
		"DORDER_SCOUT_ATTACKWALL",	// 34 - scout to a location taking out a blocking wall on the way
		"DORDER_RECOVER",				// pick up an artifact
		"DORDER_LEAVEMAP",			// 36 - vtol flying off the map
		"DORDER_RTR_SPECIFIED",		// return to repair at a specified repair center
		"DORDER_UNDEFINED",
		"DORDER_UNDEFINED2",
		"DORDER_CIRCLE",				// circles target location and engage
		"DORDER_TEMP_HOLD"				// do nothing until given next order
	};

	ASSERT(order < sizeof(name) / sizeof(name[0]), "DROID_ORDER out of range: %u", order);

	return name[order];
}
