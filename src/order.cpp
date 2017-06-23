/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
 *
 * @file
 * Functions for setting the orders of a droid or group of droids.
 *
 */

#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/input.h"
#include "lib/framework/math_ext.h"
#include "lib/ivis_opengl/ivisdef.h"

#include "objects.h"
#include "order.h"
#include "action.h"
#include "map.h"
#include "projectile.h"
#include "effects.h"	// for waypoint display
#include "lib/gamelib/gtime.h"
#include "lib/netplay/netplay.h"
#include "intorder.h"
#include "orderdef.h"
#include "transporter.h"
#include "qtscript.h"
#include "group.h"
#include "cmddroid.h"
#include "lib/script/script.h"
#include "scriptcb.h"
#include "move.h"
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
#include "console.h"
#include "mapgrid.h"

#include "random.h"

/** How long a droid runs after it fails do respond due to low moral. */
#define RUN_TIME		8000

/** How long a droid runs burning after it fails do respond due to low moral. */
#define RUN_BURN_TIME	10000

/** The distance a droid has in guard mode. */
#define DEFEND_MAXDIST		(TILE_UNITS * 3)

/** The distance a droid has in guard mode.
 * @todo seems to be used as equivalent to GUARD_MAXDIST.
 */
#define DEFEND_BASEDIST		(TILE_UNITS * 3)

/** The distance a droid has in guard mode. Equivalent to GUARD_MAXDIST, but used for droids being on a command group. */
#define DEFEND_CMD_MAXDIST		(TILE_UNITS * 8)

/** The distance a droid has in guard mode. Equivalent to GUARD_BASEDIST, but used for droids being on a command group. */
#define DEFEND_CMD_BASEDIST		(TILE_UNITS * 5)

/** The maximum distance a constructor droid has in guard mode. */
#define CONSTRUCT_MAXDIST		(TILE_UNITS * 8)

/** The maximum distance allowed to a droid to move out of the path on a patrol/scout. */
#define SCOUT_DIST			(TILE_UNITS * 8)

/** The maximum distance allowed to a droid to move out of the path if already attacking a target on a patrol/scout. */
#define SCOUT_ATTACK_DIST	(TILE_UNITS * 5)

/** This instance is a global instance of RUN_DATA. It has the information of each player about retreat position and minimum moral/health for a unit to retreat to. */
RUN_DATA	asRunData[MAX_PLAYERS];

static bool secondaryGotPrimaryOrder(DROID *psDroid, DROID_ORDER order);

static void orderClearDroidList(DROID *psDroid);

/** Whether an order effect has been displayed
 * @todo better documentation required.
 */
static bool bOrderEffectDisplayed = false;

/** What the droid's action/order it is currently. This is used to debug purposes, jointly with showSAMPLES(). */
extern char DROIDDOING[512];
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////

/** This function initializes asRunData, allocking it. It should be called AFTER every mission so that asRunData gets reset. */
void initRunData()
{
	UBYTE   i;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		memset(&asRunData[i], 0, sizeof(RUN_DATA));
	}
}


/** This function checks if the droid is off range. If yes, it uses actionDroid() to make the droid to move to its target if its target is on range, or to move to its order position if not.
 * @todo droid doesn't shoot while returning to the guard position.
 */
static void orderCheckGuardPosition(DROID *psDroid, SDWORD range)
{
	SDWORD		xdiff, ydiff;
	UDWORD		x, y;

	if (psDroid->order.psObj != nullptr)
	{
		// repair droids always follow behind - don't want them jumping into the line of fire
		if ((!(psDroid->droidType == DROID_REPAIR || psDroid->droidType == DROID_CYBORG_REPAIR))
		    && psDroid->order.psObj->type == OBJ_DROID && orderStateLoc((DROID *)psDroid->order.psObj, DORDER_MOVE, &x, &y))
		{
			// got a moving droid - check against where the unit is going
			psDroid->order.pos = Vector2i(x, y);
		}
		else
		{
			psDroid->order.pos = psDroid->order.psObj->pos.xy;
		}
	}

	xdiff = psDroid->pos.x - psDroid->order.pos.x;
	ydiff = psDroid->pos.y - psDroid->order.pos.y;
	if (xdiff * xdiff + ydiff * ydiff > range * range)
	{
		if ((psDroid->sMove.Status != MOVEINACTIVE) &&
		    ((psDroid->action == DACTION_MOVE) ||
		     (psDroid->action == DACTION_MOVEFIRE)))
		{
			xdiff = psDroid->sMove.destination.x - psDroid->order.pos.x;
			ydiff = psDroid->sMove.destination.y - psDroid->order.pos.y;
			if (xdiff * xdiff + ydiff * ydiff > range * range)
			{
				actionDroid(psDroid, DACTION_MOVE, psDroid->order.pos.x, psDroid->order.pos.y);
			}
		}
		else
		{
			actionDroid(psDroid, DACTION_MOVE, psDroid->order.pos.x, psDroid->order.pos.y);
		}
	}
}


/** This function checks if there are any damaged droids within a defined range.
 * It returns the damaged droid if there is any, or nullptr if none was found.
 */
DROID *checkForRepairRange(DROID *psDroid)
{
	DROID *psFailedTarget = nullptr;
	if (psDroid->action == DACTION_SULK)
	{
		psFailedTarget = (DROID *)psDroid->psActionTarget[0];
	}

	ASSERT(psDroid->droidType == DROID_REPAIR || psDroid->droidType == DROID_CYBORG_REPAIR, "Invalid droid type");

	unsigned radius = psDroid->order.type == DORDER_HOLD ? REPAIR_RANGE : REPAIR_MAXDIST;

	unsigned bestDistanceSq = radius * radius;
	DROID *best = nullptr;

	for (BASE_OBJECT *object : gridStartIterate(psDroid->pos.x, psDroid->pos.y, radius))
	{
		unsigned distanceSq = droidSqDist(psDroid, object);  // droidSqDist returns -1 if unreachable, (unsigned)-1 is a big number.
		if (object == orderStateObj(psDroid, DORDER_GUARD))
		{
			distanceSq = 0;  // If guarding a unit — always do that first.
		}

		DROID *droid = castDroid(object);
		if (droid != nullptr &&  // Must be a droid.
		    droid != psFailedTarget &&   // Must not have just failed to reach it.
		    distanceSq <= bestDistanceSq &&  // Must be as close as possible.
		    aiCheckAlliances(psDroid->player, droid->player) &&  // Must be a friendly droid.
		    droidIsDamaged(droid) &&  // Must need repairing.
		    visibleObject(psDroid, droid, false))  // Must be able to sense it.
		{
			bestDistanceSq = distanceSq;
			best = droid;
		}
	}

	return best;
}


/** This function checks if there are any structures to repair or help build in a given radius near the droid defined by REPAIR_RANGE if it is on hold, and REPAIR_MAXDIST if not on hold.
 * It returns a damaged or incomplete structure if any was found or nullptr if none was found.
 */
static std::pair<STRUCTURE *, DROID_ACTION> checkForDamagedStruct(DROID *psDroid)
{
	STRUCTURE *psFailedTarget = nullptr;
	if (psDroid->action == DACTION_SULK)
	{
		psFailedTarget = (STRUCTURE *)psDroid->psActionTarget[0];
	}

	unsigned radius = psDroid->order.type == DORDER_HOLD ? REPAIR_RANGE : REPAIR_MAXDIST;

	unsigned bestDistanceSq = radius * radius;
	std::pair<STRUCTURE *, DROID_ACTION> best = {nullptr, DACTION_NONE};

	for (BASE_OBJECT *object : gridStartIterate(psDroid->pos.x, psDroid->pos.y, radius))
	{
		unsigned distanceSq = droidSqDist(psDroid, object);  // droidSqDist returns -1 if unreachable, (unsigned)-1 is a big number.

		STRUCTURE *structure = castStructure(object);
		if (structure == nullptr ||  // Must be a structure.
		    structure == psFailedTarget ||  // Must not have just failed to reach it.
		    distanceSq > bestDistanceSq ||  // Must be as close as possible.
		    !visibleObject(psDroid, structure, false) ||  // Must be able to sense it.
		    !aiCheckAlliances(psDroid->player, structure->player) ||  // Must be a friendly structure.
		    checkDroidsDemolishing(structure))  // Must not be trying to get rid of it.
		{
			continue;
		}

		// Check for structures to repair.
		if (structure->status == SS_BUILT && structIsDamaged(structure))
		{
			bestDistanceSq = distanceSq;
			best = {structure, DACTION_REPAIR};
		}
		// Check for structures to help build.
		else if (structure->status == SS_BEING_BUILT)
		{
			bestDistanceSq = distanceSq;
			best = {structure, DACTION_BUILD};
		}
	}

	return best;
}

static bool isRepairlikeAction(DROID_ACTION action)
{
	switch (action)
	{
		case DACTION_BUILD:
		case DACTION_BUILDWANDER:
		case DACTION_DEMOLISH:
		case DACTION_DROIDREPAIR:
		case DACTION_MOVETOBUILD:
		case DACTION_MOVETODEMOLISH:
		case DACTION_MOVETODROIDREPAIR:
		case DACTION_MOVETOREPAIR:
		case DACTION_MOVETORESTORE:
		case DACTION_REPAIR:
		case DACTION_RESTORE:
			return true;
		default:
			return false;
	}
}

static bool tryDoRepairlikeAction(DROID *psDroid)
{
	if (isRepairlikeAction(psDroid->action))
	{
		return true;  // Already doing something.
	}

	switch (psDroid->droidType)
	{
		case DROID_REPAIR:
		case DROID_CYBORG_REPAIR:
			//repair droids default to repairing droids within a given range
			if (DROID *repairTarget = checkForRepairRange(psDroid))
			{
				actionDroid(psDroid, DACTION_DROIDREPAIR, repairTarget);
			}
			break;
		case DROID_CONSTRUCT:
		case DROID_CYBORG_CONSTRUCT:
		{
			//construct droids default to repairing and helping structures within a given range
			auto damaged = checkForDamagedStruct(psDroid);
			if (damaged.second == DACTION_REPAIR)
			{
				actionDroid(psDroid, damaged.second, damaged.first);
			}
			else if (damaged.second == DACTION_BUILD)
			{
				psDroid->order.psStats = damaged.first->pStructureType;
				psDroid->order.direction = damaged.first->rot.direction;
				actionDroid(psDroid, damaged.second, damaged.first->pos.x, damaged.first->pos.y);
			}
			break;
		}
		default:
			return false;
	}
	return true;
}

/** This function updates all the orders status, according with psdroid's current order and state.
 * @todo This is as quite complex function. Suggestion to try to refactor it.
 */
void orderUpdateDroid(DROID *psDroid)
{
	BASE_OBJECT		*psObj;
	STRUCTURE		*psStruct, *psWall;
	SDWORD			xdiff, ydiff;
	bool			bAttack;
	SDWORD			xoffset, yoffset;

	// clear the target if it has died
	if (psDroid->order.psObj && psDroid->order.psObj->died)
	{
		setDroidTarget(psDroid, nullptr);
		objTrace(psDroid->id, "Target dead");
	}

	//clear its base struct if its died
	if (psDroid->psBaseStruct && psDroid->psBaseStruct->died)
	{
		setDroidBase(psDroid, nullptr);
		objTrace(psDroid->id, "Base struct dead");
	}

	// check for died objects in the list
	orderCheckList(psDroid);

	if (isDead(psDroid))
	{
		return;
	}

	switch (psDroid->order.type)
	{
	case DORDER_NONE:
	case DORDER_HOLD:
		psObj = nullptr;
		// see if there are any orders queued up
		if (orderDroidList(psDroid))
		{
			// started a new order, quit
			break;
		}
		// if you are in a command group, default to guarding the commander
		else if (hasCommander(psDroid) &&
		         psDroid->order.psStats !=  structGetDemolishStat())  // stop the constructor auto repairing when it is about to demolish
		{
			orderDroidObj(psDroid, DORDER_GUARD, psDroid->psGroup->psCommander, ModeImmediate);
		}
		else if (isTransporter(psDroid) && !bMultiPlayer)
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
					triggerEvent(TRIGGER_TRANSPORTER_EXIT, psDroid);
				}
			}
		}
		else if (tryDoRepairlikeAction(psDroid))
		{
		}
		// default to guarding
		else if (psDroid->order.psStats != structGetDemolishStat() && !isVtolDroid(psDroid))
		{
			orderDroidLoc(psDroid, DORDER_GUARD, psDroid->pos.x, psDroid->pos.y, ModeImmediate);
		}
		break;
	case DORDER_TRANSPORTRETURN:
		if (psDroid->action == DACTION_NONE)
		{
			missionMoveTransporterOffWorld(psDroid);

			/* clear order */
			psDroid->order = DroidOrder(DORDER_NONE);
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
				actionDroid(psDroid, DACTION_TRANSPORTWAITTOFLYIN);

				missionSetReinforcementTime(gameTime);

				//don't do this until waited for the required time
				//fly Transporter back to get some more droids
				//orderDroidLoc( psDroid, DORDER_TRANSPORTIN,
				//    getLandingX(selectedPlayer), getLandingY(selectedPlayer));
			}
			else
			{
				//the script can call startMission for this callback for offworld missions
				eventFireCallbackTrigger((TRIGGER_TYPE)CALL_START_NEXT_LEVEL);
				triggerEvent(TRIGGER_TRANSPORTER_EXIT, psDroid);

				/* clear order */
				psDroid->order = DroidOrder(DORDER_NONE);
			}
		}
		break;
	case DORDER_TRANSPORTIN:
		if ((psDroid->action == DACTION_NONE) &&
		    (psDroid->sMove.Status == MOVEINACTIVE))
		{
			/* clear order */
			psDroid->order = DroidOrder(DORDER_NONE);

//FFS! You only wan't to do this if the droid being tracked IS the transporter! Not all the time!
// What about if your happily playing the game and tracking a droid, and re-enforcements come in!
// And suddenly BLAM!!!! It drops you out of camera mode for no apparent reason! TOTALY CONFUSING
// THE PLAYER!
//
// Just had to get that off my chest....end of rant.....
//
			if (psDroid == getTrackingDroid())    // Thats better...
			{
				/* deselect transporter if have been tracking */
				if (getWarCamStatus())
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
				unloadTransporter(psDroid, psDroid->pos.x, psDroid->pos.y, false);
			}
		}
		break;
	case DORDER_MOVE:
	case DORDER_RETREAT:
		// Just wait for the action to finish then clear the order
		if (psDroid->action == DACTION_NONE || psDroid->action == DACTION_ATTACK)
		{
			psDroid->order = DroidOrder(DORDER_NONE);
		}
		break;
	case DORDER_RECOVER:
		if (psDroid->order.psObj == nullptr)
		{
			psDroid->order = DroidOrder(DORDER_NONE);
		}
		else if (psDroid->action == DACTION_NONE)
		{
			// stoped moving, but still havn't got the artifact
			actionDroid(psDroid, DACTION_MOVE, psDroid->order.psObj->pos.x, psDroid->order.psObj->pos.y);
		}
		break;
	case DORDER_SCOUT:
	case DORDER_PATROL:
		// if there is an enemy around, attack it
		if (psDroid->action == DACTION_MOVE || psDroid->action == DACTION_MOVEFIRE || (psDroid->action == DACTION_NONE && isVtolDroid(psDroid)))
		{
			bool tooFarFromPath = false;
			if (isVtolDroid(psDroid) && psDroid->order.type == DORDER_PATROL)
			{
				// Don't stray too far from the patrol path - only attack if we're near it
				// A fun algorithm to detect if we're near the path
				Vector2i delta = psDroid->order.pos - psDroid->order.pos2;
				if (delta == Vector2i(0, 0))
				{
					tooFarFromPath = false;
				}
				else if (abs(delta.x) >= abs(delta.y) &&
				         MIN(psDroid->order.pos.x, psDroid->order.pos2.x) - SCOUT_DIST <= psDroid->pos.x &&
				         psDroid->pos.x <= MAX(psDroid->order.pos.x, psDroid->order.pos2.x) + SCOUT_DIST)
				{
					tooFarFromPath = (abs((psDroid->pos.x - psDroid->order.pos.x) * delta.y / delta.x +
					                      psDroid->order.pos.y - psDroid->pos.y) > SCOUT_DIST);
				}
				else if (abs(delta.x) <= abs(delta.y) &&
				         MIN(psDroid->order.pos.y, psDroid->order.pos2.y) - SCOUT_DIST <= psDroid->pos.y &&
				         psDroid->pos.y <= MAX(psDroid->order.pos.y, psDroid->order.pos2.y) + SCOUT_DIST)
				{
					tooFarFromPath = (abs((psDroid->pos.y - psDroid->order.pos.y) * delta.x / delta.y +
					                      psDroid->order.pos.x - psDroid->pos.x) > SCOUT_DIST);
				}
				else
				{
					tooFarFromPath = true;
				}
			}
			if (!tooFarFromPath)
			{
				// true if in condition to set actionDroid to attack/observe
				bool attack = secondaryGetState(psDroid, DSO_ATTACK_LEVEL) == DSS_ALEV_ALWAYS &&
				              aiBestNearestTarget(psDroid, &psObj, 0, SCOUT_ATTACK_DIST) >= 0;
				switch (psDroid->droidType)
				{
				case DROID_CONSTRUCT:
				case DROID_CYBORG_CONSTRUCT:
				case DROID_REPAIR:
				case DROID_CYBORG_REPAIR:
					tryDoRepairlikeAction(psDroid);
					break;
				case DROID_WEAPON:
				case DROID_CYBORG:
				case DROID_CYBORG_SUPER:
				case DROID_PERSON:
				case DROID_COMMAND:
					if (attack)
					{
						actionDroid(psDroid, DACTION_ATTACK, psObj);
					}
					break;
				case DROID_SENSOR:
					if (attack)
					{
						actionDroid(psDroid, DACTION_OBSERVE, psObj);
					}
					break;
				default:
					actionDroid(psDroid, DACTION_NONE);
					break;
				}
			}
		}
		if (psDroid->action == DACTION_NONE)
		{
			xdiff = psDroid->pos.x - psDroid->order.pos.x;
			ydiff = psDroid->pos.y - psDroid->order.pos.y;
			if (xdiff * xdiff + ydiff * ydiff < SCOUT_DIST * SCOUT_DIST)
			{
				if (psDroid->order.type == DORDER_PATROL)
				{
					// see if we have anything queued up
					if (orderDroidList(psDroid))
					{
						// started a new order, quit
						break;
					}
					if (isVtolDroid(psDroid) && !vtolFull(psDroid) && (psDroid->secondaryOrder & DSS_ALEV_MASK) != DSS_ALEV_NEVER)
					{
						moveToRearm(psDroid);
						break;
					}
					// head back to the other point
					std::swap(psDroid->order.pos, psDroid->order.pos2);
					actionDroid(psDroid, DACTION_MOVE, psDroid->order.pos.x, psDroid->order.pos.y);
				}
				else
				{
					psDroid->order = DroidOrder(DORDER_NONE);
				}
			}
			else
			{
				actionDroid(psDroid, DACTION_MOVE, psDroid->order.pos.x, psDroid->order.pos.y);
			}
		}
		else if ((psDroid->action == DACTION_ATTACK) ||
		         (psDroid->action == DACTION_MOVETOATTACK) ||
		         (psDroid->action == DACTION_ROTATETOATTACK) ||
		         (psDroid->action == DACTION_OBSERVE) ||
		         (psDroid->action == DACTION_MOVETOOBSERVE))
		{
			// attacking something - see if the droid has gone too far, go up to twice the distance we want to go, so that we don't repeatedly turn back when the target is almost in range.
			if (objPosDiffSq(psDroid->pos, Vector3i(psDroid->actionPos, 0)) > (SCOUT_ATTACK_DIST * 2 * SCOUT_ATTACK_DIST * 2))
			{
				actionDroid(psDroid, DACTION_RETURNTOPOS, psDroid->actionPos.x, psDroid->actionPos.y);
			}
		}
		if (psDroid->order.type == DORDER_PATROL && isVtolDroid(psDroid) && vtolEmpty(psDroid) && (psDroid->secondaryOrder & DSS_ALEV_MASK) != DSS_ALEV_NEVER)
		{
			moveToRearm(psDroid);  // Completely empty (and we're not set to hold fire), don't bother patrolling.
			break;
		}
		break;
	case DORDER_CIRCLE:
		// if there is an enemy around, attack it
		if (psDroid->action == DACTION_MOVE &&
		    secondaryGetState(psDroid, DSO_ATTACK_LEVEL) == DSS_ALEV_ALWAYS &&
		    aiBestNearestTarget(psDroid, &psObj, 0, SCOUT_ATTACK_DIST) >= 0)
		{
			switch (psDroid->droidType)
			{
			case DROID_WEAPON:
			case DROID_CYBORG:
			case DROID_CYBORG_SUPER:
			case DROID_PERSON:
			case DROID_COMMAND:
				actionDroid(psDroid, DACTION_ATTACK, psObj);
				break;
			case DROID_SENSOR:
				actionDroid(psDroid, DACTION_OBSERVE, psObj);
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
				// see if we have anything queued up
				if (orderDroidList(psDroid))
				{
					// started a new order, quit
					break;
				}
			}

			Vector2i edgeDiff = psDroid->pos.xy - psDroid->actionPos;
			if (psDroid->action != DACTION_MOVE || edgeDiff * edgeDiff <= TILE_UNITS * 4 * TILE_UNITS * 4)
			{
				//Watermelon:use orderX,orderY as local space origin and calculate droid direction in local space
				Vector2i diff = psDroid->pos.xy - psDroid->order.pos;
				uint16_t angle = iAtan2(diff) - DEG(30);
				do
				{
					xoffset = iSinR(angle, 1500);
					yoffset = iCosR(angle, 1500);
					angle -= DEG(10);
				}
				while (!worldOnMap(psDroid->order.pos.x + xoffset, psDroid->order.pos.y + yoffset));    // Don't try to fly off map.
				actionDroid(psDroid, DACTION_MOVE, psDroid->order.pos.x + xoffset, psDroid->order.pos.y + yoffset);
			}

			if (isVtolDroid(psDroid) && vtolEmpty(psDroid) && (psDroid->secondaryOrder & DSS_ALEV_MASK) != DSS_ALEV_NEVER)
			{
				moveToRearm(psDroid);  // Completely empty (and we're not set to hold fire), don't bother circling.
				break;
			}
		}
		else if ((psDroid->action == DACTION_ATTACK) ||
		         (psDroid->action == DACTION_MOVETOATTACK) ||
		         (psDroid->action == DACTION_ROTATETOATTACK) ||
		         (psDroid->action == DACTION_OBSERVE) ||
		         (psDroid->action == DACTION_MOVETOOBSERVE))
		{
			// attacking something - see if the droid has gone too far
			xdiff = psDroid->pos.x - psDroid->actionPos.x;
			ydiff = psDroid->pos.y - psDroid->actionPos.y;
			//if (xdiff*xdiff + ydiff*ydiff > psDroid->sMove.iGuardRadius * psDroid->sMove.iGuardRadius)
			if (xdiff * xdiff + ydiff * ydiff > 2000 * 2000)
			{
				// head back to the target location
				actionDroid(psDroid, DACTION_RETURNTOPOS, psDroid->order.pos.x, psDroid->order.pos.y);
			}
		}
		break;
	case DORDER_HELPBUILD:
	case DORDER_DEMOLISH:
	case DORDER_OBSERVE:
	case DORDER_REPAIR:
	case DORDER_DROIDREPAIR:
	case DORDER_RESTORE:
		if (psDroid->action == DACTION_NONE ||
		    psDroid->order.psObj == nullptr)
		{
			psDroid->order = DroidOrder(DORDER_NONE);
			actionDroid(psDroid, DACTION_NONE);
			if (psDroid->player == selectedPlayer)
			{
				intRefreshScreen();
			}
		}
		break;
	case DORDER_REARM:
		if ((psDroid->order.psObj == nullptr) ||
		    (psDroid->psActionTarget[0] == nullptr))
		{
			// arm pad destroyed find another
			psDroid->order = DroidOrder(DORDER_NONE);
			moveToRearm(psDroid);
		}
		else if (psDroid->action == DACTION_NONE)
		{
			psDroid->order = DroidOrder(DORDER_NONE);
		}
		break;
	case DORDER_ATTACK:
	case DORDER_ATTACKTARGET:
		if (psDroid->order.psObj == nullptr || psDroid->order.psObj->died)
		{
			// if vtol then return to rearm pad as long as there are no other
			// orders queued up
			if (isVtolDroid(psDroid))
			{
				if (!orderDroidList(psDroid))
				{
					psDroid->order = DroidOrder(DORDER_NONE);
					moveToRearm(psDroid);
					if (!vtolEmpty(psDroid))
					{
						// VTOL droid can do more work, let scripts handle it
						psScrVtolRetarget = psDroid;
						eventFireCallbackTrigger((TRIGGER_TYPE)CALL_VTOL_RETARGET);
						psScrVtolRetarget = nullptr;
					}
				}
			}
			else
			{
				psDroid->order = DroidOrder(DORDER_NONE);
				actionDroid(psDroid, DACTION_NONE);
			}
		}
		else if (((psDroid->action == DACTION_MOVE) ||
		          (psDroid->action == DACTION_MOVEFIRE)) &&
		         actionVisibleTarget(psDroid, psDroid->order.psObj, 0) && !isVtolDroid(psDroid))
		{
			// moved near enough to attack change to attack action
			actionDroid(psDroid, DACTION_ATTACK, psDroid->order.psObj);
		}
		else if ((psDroid->action == DACTION_MOVETOATTACK) &&
		         !isVtolDroid(psDroid) &&
		         !actionVisibleTarget(psDroid, psDroid->order.psObj, 0))
		{
			// lost sight of the target while chasing it - change to a move action so
			// that the unit will fire on other things while moving
			actionDroid(psDroid, DACTION_MOVE, psDroid->order.psObj->pos.x, psDroid->order.psObj->pos.y);
		}
		else if (!isVtolDroid(psDroid)
		         && psDroid->order.psObj == psDroid->psActionTarget[0]
		         && actionInRange(psDroid, psDroid->order.psObj, 0)
		         && (psWall = visGetBlockingWall(psDroid, psDroid->order.psObj))
		         && psWall->player != psDroid->player)
		{
			// there is a wall in the way - attack that
			actionDroid(psDroid, DACTION_ATTACK, psWall);
		}
		else if ((psDroid->action == DACTION_NONE) ||
		         (psDroid->action == DACTION_CLEARREARMPAD))
		{
			if (!isVtolDroid(psDroid) || allVtolsRearmed(psDroid))
			{
				actionDroid(psDroid, DACTION_ATTACK, psDroid->order.psObj);
			}
		}
		break;
	case DORDER_BUILD:
		if (psDroid->action == DACTION_BUILD &&
		    psDroid->order.psObj == nullptr)
		{
			psDroid->order = DroidOrder(DORDER_NONE);
			actionDroid(psDroid, DACTION_NONE);
			objTrace(psDroid->id, "Clearing build order since build target is gone");
		}
		else if (psDroid->action == DACTION_NONE)
		{
			psDroid->order = DroidOrder(DORDER_NONE);
			objTrace(psDroid->id, "Clearing build order since build action is reset");
		}
		break;
	case DORDER_EMBARK:
		{
			// only place it can be trapped - in multiPlayer can only put cyborgs onto a Cyborg Transporter
			DROID *temp = (DROID *)psDroid->order.psObj;	// NOTE: It is possible to have a NULL here 

			// FIXME: since we now have 2 transporter types, we should fix this in the scripts for campaign
			if (temp && temp->droidType == DROID_TRANSPORTER && !cyborgDroid(psDroid) && game.type != CAMPAIGN && bMultiPlayer)
			{
				psDroid->order = DroidOrder(DORDER_NONE);
				actionDroid(psDroid, DACTION_NONE);
				if (psDroid->player == selectedPlayer)
				{
					audio_PlayTrack(ID_SOUND_BUILD_FAIL);
					addConsoleMessage(_("We can't do that! We must be a Cyborg unit to use a Cyborg Transport!"), DEFAULT_JUSTIFY, selectedPlayer);
				}
			}
			else
			{
				// Wait for the action to finish then assign to Transporter (if not already flying)
				if (psDroid->order.psObj == nullptr || transporterFlying((DROID *)psDroid->order.psObj))
				{
					psDroid->order = DroidOrder(DORDER_NONE);
					actionDroid(psDroid, DACTION_NONE);
				}
				else if (abs((SDWORD)psDroid->pos.x - (SDWORD)psDroid->order.psObj->pos.x) < TILE_UNITS
				         && abs((SDWORD)psDroid->pos.y - (SDWORD)psDroid->order.psObj->pos.y) < TILE_UNITS)
				{
					// save the target of current droid (the transporter)
					DROID *transporter = (DROID *)psDroid->order.psObj;

					// Make sure that it really is a valid droid
					CHECK_DROID(transporter);

					// order the droid to stop so moveUpdateDroid does not process this unit
					orderDroid(psDroid, DORDER_STOP, ModeImmediate);
					setDroidTarget(psDroid, nullptr);
					psDroid->order.psObj = nullptr;
					secondarySetState(psDroid, DSO_RETURN_TO_LOC, DSS_NONE);

					/* We must add the droid to the transporter only *after*
					* processing changing its orders (see above).
					*/
					transporterAddDroid(transporter, psDroid);
				}
				else if (psDroid->action == DACTION_NONE)
				{
					actionDroid(psDroid, DACTION_MOVE, psDroid->order.psObj->pos.x, psDroid->order.psObj->pos.y);
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
			if (isTransporter(psDroid))
			{
				/*once the Transporter has reached its destination (and landed),
				get all the units to disembark*/
				if (psDroid->action != DACTION_MOVE && psDroid->action != DACTION_MOVEFIRE &&
				    psDroid->sMove.Status == MOVEINACTIVE && psDroid->sMove.iVertSpeed == 0)
				{
					unloadTransporter(psDroid, psDroid->pos.x, psDroid->pos.y, false);
					//reset the transporter's order
					psDroid->order = DroidOrder(DORDER_NONE);
				}
			}
		}
		break;
	case DORDER_RTB:
		// Just wait for the action to finish then clear the order
		if (psDroid->action == DACTION_NONE)
		{
			psDroid->order = DroidOrder(DORDER_NONE);
			secondarySetState(psDroid, DSO_RETURN_TO_LOC, DSS_NONE);
		}
		break;
	case DORDER_LEAVEMAP:
		if ((psDroid->pos.x < TILE_UNITS * 2) ||
		    (psDroid->pos.x > (mapWidth - 2)*TILE_UNITS) ||
		    (psDroid->pos.y < TILE_UNITS * 2) ||
		    (psDroid->pos.y > (mapHeight - 2)*TILE_UNITS) ||
		    (psDroid->action == DACTION_NONE))
		{
			psDroid->order = DroidOrder(DORDER_NONE);
			psScrCBVtolOffMap = psDroid;
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_VTOL_OFF_MAP);
		}
		break;
	case DORDER_RTR:
	case DORDER_RTR_SPECIFIED:
		if (psDroid->order.psObj == nullptr)
		{
			// Our target got lost. Let's try again.
			psDroid->order = DroidOrder(DORDER_NONE);
			orderDroid(psDroid, DORDER_RTR, ModeImmediate);
		}
		else if (psDroid->action == DACTION_NONE)
		{
			/* get repair facility pointer */
			psStruct = (STRUCTURE *)psDroid->order.psObj;
			ASSERT(psStruct != nullptr,
			       "orderUpdateUnit: invalid structure pointer");

			if (objPosDiffSq(psDroid->pos, psDroid->order.psObj->pos) < (TILE_UNITS * 8) * (TILE_UNITS * 8))
			{
				/* action droid to wait */
				actionDroid(psDroid, DACTION_WAITFORREPAIR);
			}
			else
			{
				// move the droid closer to the repair facility
				actionDroid(psDroid, DACTION_MOVE, psDroid->order.psObj->pos.x, psDroid->order.psObj->pos.y);
			}
		}
		else if (psDroid->order.type == DORDER_RTR &&
		         (psDroid->action == DACTION_MOVE ||
		          psDroid->action == DACTION_MOVEFIRE) &&
		         ((psDroid->id + gameTime) / GAME_TICKS_PER_SEC != (psDroid->id + gameTime - deltaGameTime) / GAME_TICKS_PER_SEC))
		{
			// see if there is a repair facility nearer than the one currently selected
			orderDroid(psDroid, DORDER_RTR, ModeImmediate);
		}
		break;
	case DORDER_RUN:
		if (psDroid->action == DACTION_NONE)
		{
			// got there so stop running
			psDroid->order = DroidOrder(DORDER_NONE);
		}
		if (psDroid->actionStarted + RUN_TIME <= gameTime)
		{
			// been running long enough
			actionDroid(psDroid, DACTION_NONE);
			psDroid->order = DroidOrder(DORDER_NONE);
		}
		break;
	case DORDER_LINEBUILD:
		if (psDroid->action == DACTION_NONE ||
		    (psDroid->action == DACTION_BUILD && psDroid->order.psObj == nullptr))
		{
			// finished building the current structure
			if (map_coord(psDroid->order.pos) == map_coord(psDroid->order.pos2))
			{
				// finished all the structures - done
				psDroid->order = DroidOrder(DORDER_NONE);
				break;
			}

			// update the position for another structure
			if (map_coord(psDroid->order.pos.x) == map_coord(psDroid->order.pos2.x))
			{
				// still got building to do - working vertically
				if (psDroid->order.pos.y < psDroid->order.pos2.y)
				{
					psDroid->order.pos.y += TILE_UNITS;
				}
				else
				{
					psDroid->order.pos.y -= TILE_UNITS;
				}
			}
			else if (map_coord(psDroid->order.pos.y) == map_coord(psDroid->order.pos2.y))
			{
				// still got building to do - working horizontally
				if (psDroid->order.pos.x < psDroid->order.pos2.x)
				{
					psDroid->order.pos.x += TILE_UNITS;
				}
				else
				{
					psDroid->order.pos.x -= TILE_UNITS;
				}
			}
			else
			{
				ASSERT(false, "orderUpdateUnit: LINEBUILD order on diagonal line");
				break;
			}

			// build another structure
			setDroidTarget(psDroid, nullptr);
			actionDroid(psDroid, DACTION_BUILD, psDroid->order.pos.x, psDroid->order.pos.y);
			//intRefreshScreen();
		}
		break;
	case DORDER_FIRESUPPORT:
		if (psDroid->order.psObj == nullptr)
		{
			psDroid->order = DroidOrder(DORDER_NONE);
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
			BASE_OBJECT	*psFireTarget = nullptr;

			if (psDroid->order.psObj->type == OBJ_DROID)
			{
				DROID	*psSpotter = (DROID *)psDroid->order.psObj;

				if (psSpotter->action == DACTION_OBSERVE
				    || (psSpotter->droidType == DROID_COMMAND && psSpotter->action == DACTION_ATTACK))
				{
					psFireTarget = psSpotter->psActionTarget[0];
				}
			}
			else if (psDroid->order.psObj->type == OBJ_STRUCTURE)
			{
				STRUCTURE *psSpotter = (STRUCTURE *)psDroid->order.psObj;

				psFireTarget = psSpotter->psTarget[0];
			}

			if (psFireTarget && !psFireTarget->died && checkAnyWeaponsTarget(psDroid, psFireTarget))
			{
				bAttack = false;
				if (isVtolDroid(psDroid))
				{
					if (!vtolEmpty(psDroid) &&
					    ((psDroid->action == DACTION_MOVETOREARM) ||
					     (psDroid->action == DACTION_WAITFORREARM)) &&
					    (psDroid->sMove.Status != MOVEINACTIVE))
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
				if (bAttack &&
				    (!droidAttacking(psDroid) ||
				     psFireTarget != psDroid->psActionTarget[0]))
				{
					//get the droid to attack
					actionDroid(psDroid, DACTION_ATTACK, psFireTarget);
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
				actionDroid(psDroid, DACTION_FIRESUPPORT, psDroid->order.psObj);
			}
		}
		break;
	case DORDER_RECYCLE:
		if (psDroid->order.psObj == nullptr)
		{
			psDroid->order = DroidOrder(DORDER_NONE);
			actionDroid(psDroid, DACTION_NONE);
		}
		else if (actionReachedBuildPos(psDroid, psDroid->order.psObj->pos.x, psDroid->order.psObj->pos.y, ((STRUCTURE *)psDroid->order.psObj)->rot.direction, ((STRUCTURE *)psDroid->order.psObj)->pStructureType))
		{
			recycleDroid(psDroid);
		}
		else if (psDroid->action == DACTION_NONE)
		{
			actionDroid(psDroid, DACTION_MOVE, psDroid->order.psObj->pos.x, psDroid->order.psObj->pos.y);
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
			    && psDroid->order.psObj != nullptr && psDroid->order.psObj->type == OBJ_DROID
			    && ((DROID *)psDroid->order.psObj)->droidType == DROID_COMMAND)
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
		else if (isTransporter(psDroid))
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
					triggerEvent(TRIGGER_TRANSPORTER_EXIT, psDroid);
				}
			}
		}
		else
		{
			//let vtols return to rearm
			if (!vtolRearming(psDroid))
			{
				// attacking something, make sure the droid doesn't go too far
				if (psDroid->order.psObj != nullptr && psDroid->order.psObj->type == OBJ_DROID &&
				    ((DROID *)psDroid->order.psObj)->droidType == DROID_COMMAND)
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
		if (hasCommander(psDroid) && (psDroid->numWeaps > 0))
		{
			if ((psDroid->psGroup->psCommander->action == DACTION_ATTACK) &&
			    (psDroid->psGroup->psCommander->psActionTarget[0] != nullptr) &&
			    (!psDroid->psGroup->psCommander->psActionTarget[0]->died))
			{
				psObj = psDroid->psGroup->psCommander->psActionTarget[0];
				if (psDroid->action == DACTION_ATTACK ||
				    psDroid->action == DACTION_MOVETOATTACK)
				{
					if (psDroid->psActionTarget[0] != psObj)
					{
						actionDroid(psDroid, DACTION_ATTACK, psObj);
					}
				}
				else if (psDroid->action != DACTION_MOVE)
				{
					actionDroid(psDroid, DACTION_ATTACK, psObj);
				}
			}

			// make sure units in a command group are actually guarding the commander
			psObj = orderStateObj(psDroid, DORDER_GUARD);	// find out who is being guarded by the droid
			if (psObj == nullptr
			    || psObj != psDroid->psGroup->psCommander)
			{
				orderDroidObj(psDroid, DORDER_GUARD, psDroid->psGroup->psCommander, ModeImmediate);
			}
		}

		tryDoRepairlikeAction(psDroid);
		break;
	default:
		ASSERT(false, "orderUpdateUnit: unknown order");
	}

	// catch any vtol that is rearming but has finished his order
	if (psDroid->order.type == DORDER_NONE && vtolRearming(psDroid)
	    && (psDroid->psActionTarget[0] == nullptr || !psDroid->psActionTarget[0]->died))
	{
		psDroid->order = DroidOrder(DORDER_REARM, psDroid->psActionTarget[0]);
	}

	if (psDroid->selected)
	{
		// Tell us what the droid is doing.
		sprintf(DROIDDOING, "%.12s,id(%d) order(%d):%s action(%d):%s secondary:%x move:%s", droidGetName(psDroid), psDroid->id,
		        psDroid->order.type, getDroidOrderName(psDroid->order.type), psDroid->action, getDroidActionName(psDroid->action), psDroid->secondaryOrder,
		        moveDescription(psDroid->sMove.Status));
	}
}


/** This function sends all members of the psGroup the order psData using orderDroidBase().
 * If the order data is to recover an artifact, the order is only given to the closest droid to the artifact.
 */
static void orderCmdGroupBase(DROID_GROUP *psGroup, DROID_ORDER_DATA *psData)
{
	DROID	*psCurr, *psChosen;
	SDWORD	currdist, mindist;

	ASSERT_OR_RETURN(, psGroup != nullptr, "Invalid unit group");

	syncDebug("Commander group order");

	if (psData->type == DORDER_RECOVER)
	{
		// picking up an artifact - only need to send one unit
		psChosen = nullptr;
		mindist = SDWORD_MAX;
		for (psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
		{
			currdist = objPosDiffSq(psCurr->pos, psData->psObj->pos);
			if (currdist < mindist)
			{
				psChosen = psCurr;
				mindist = currdist;
			}
			syncDebug("command %d,%d", psCurr->id, currdist);
		}
		if (psChosen != nullptr)
		{
			orderDroidBase(psChosen, psData);
		}
	}
	else
	{
		for (psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
		{
			syncDebug("command %d", psCurr->id);
			if (!orderState(psCurr, DORDER_RTR))		// if you change this, youll need to change sendcmdgroup()
			{
				orderDroidBase(psCurr, psData);
			}
		}
	}
}


/** The minimum delay to be used on orderPlayFireSupportAudio() for fire support sound.*/
#define	AUDIO_DELAY_FIRESUPPORT		(3*GAME_TICKS_PER_SEC)


/** This function chooses the sound to play after the object is assigned to fire support a specific unit. Uses audio_QueueTrackMinDelay() to play the sound.
 * @todo this function is about playing audio. I'm not sure this should be in here.
 */
static void orderPlayFireSupportAudio(BASE_OBJECT *psObj)
{
	SDWORD	iAudioID = NO_SOUND;
	DROID *psDroid;
	STRUCTURE *psStruct;

	ASSERT_OR_RETURN(, psObj != nullptr, "Invalid pointer");
	/* play appropriate speech */
	switch (psObj->type)
	{
	case OBJ_DROID:
		psDroid = (DROID *)psObj;
		if (psDroid->droidType == DROID_COMMAND)
		{
			iAudioID = ID_SOUND_ASSIGNED_TO_COMMANDER;
		}
		else if (psDroid->droidType == DROID_SENSOR)
		{
			iAudioID = ID_SOUND_ASSIGNED_TO_SENSOR;
		}
		break;

	case OBJ_STRUCTURE:
		psStruct = (STRUCTURE *)psObj;
		//check for non-CB first
		if (structStandardSensor(psStruct) || structVTOLSensor(psStruct))
		{
			iAudioID = ID_SOUND_ASSIGNED_TO_SENSOR;
		}
		else if (structCBSensor(psStruct) || structVTOLCBSensor(psStruct))
		{
			iAudioID = ID_SOUND_ASSIGNED_TO_COUNTER_RADAR;
		}
		break;
	default:
		break;
	}

	if (iAudioID != NO_SOUND)
	{
		audio_QueueTrackMinDelay(iAudioID, AUDIO_DELAY_FIRESUPPORT);
	}
}


/** This function actually tells the droid to perform the psOrder.
 * This function is called everytime to send a direct order to a droid.
 */
void orderDroidBase(DROID *psDroid, DROID_ORDER_DATA *psOrder)
{
	UDWORD		iFactoryDistSq;
	STRUCTURE	*psStruct, *psRepairFac, *psFactory;
	const PROPULSION_STATS *psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION];
	const Vector3i rPos(psOrder->pos, 0);

	syncDebugDroid(psDroid, '-');
	syncDebug("%d ordered %s", psDroid->id, getDroidOrderName(psOrder->type));

	if (psOrder->type != DORDER_TRANSPORTIN         // transporters special
	    && psOrder->psObj == nullptr			// location-type order
	    && (validOrderForLoc(psOrder->type) || psOrder->type == DORDER_BUILD)
	    && !fpathCheck(psDroid->pos, rPos, psPropStats->propulsionType))
	{
		if (!isHumanPlayer(psDroid->player))
		{
			debug(LOG_SCRIPT, "Invalid order %s given to player %d's %s for position (%d, %d) - ignoring",
			      getDroidOrderName(psOrder->type), psDroid->player, droidGetName(psDroid), psOrder->pos.x, psOrder->pos.y);
		}
		objTrace(psDroid->id, "Invalid order %s for position (%d, %d) - ignoring", getDroidOrderName(psOrder->type), psOrder->pos.x, psOrder->pos.y);
		syncDebugDroid(psDroid, '?');
		return;
	}

	// deal with a droid receiving a primary order
	if (secondaryGotPrimaryOrder(psDroid, psOrder->type))
	{
		psOrder->type = DORDER_NONE;
	}

	// if this is a command droid - all it's units do the same thing
	if ((psDroid->droidType == DROID_COMMAND) &&
	    (psDroid->psGroup != nullptr) &&
	    (psDroid->psGroup->type == GT_COMMAND) &&
	    (psOrder->type != DORDER_GUARD) &&  //(psOrder->psObj == NULL)) &&
	    (psOrder->type != DORDER_RTR) &&
	    (psOrder->type != DORDER_RECYCLE) &&
	    (psOrder->type != DORDER_MOVE))
	{
		if (psOrder->type == DORDER_ATTACK)
		{
			// change to attacktarget so that the group members
			// guard order does not get canceled
			psOrder->type = DORDER_ATTACKTARGET;
			orderCmdGroupBase(psDroid->psGroup, psOrder);
			psOrder->type = DORDER_ATTACK;
		}
		else
		{
			orderCmdGroupBase(psDroid->psGroup, psOrder);
		}

		// the commander doesn't have to pick up artifacts, one
		// of his units will do it for him (if there are any in his group).
		if ((psOrder->type == DORDER_RECOVER) &&
		    (psDroid->psGroup->psList != nullptr))
		{
			psOrder->type = DORDER_NONE;
		}
	}

	switch (psOrder->type)
	{
	case DORDER_NONE:
		// used when choose order cannot assign an order
		break;
	case DORDER_STOP:
		// get the droid to stop doing whatever it is doing
		actionDroid(psDroid, DACTION_NONE);
		psDroid->order = DroidOrder(DORDER_NONE);
		break;
	case DORDER_HOLD:
		// get the droid to stop doing whatever it is doing and temp hold
		actionDroid(psDroid, DACTION_NONE);
		psDroid->order = *psOrder;
		break;
	case DORDER_MOVE:
	case DORDER_SCOUT:
		// can't move vtols to blocking tiles
		if (isVtolDroid(psDroid)
		    && fpathBlockingTile(map_coord(psOrder->pos), getPropulsionStats(psDroid)->propulsionType))
		{
			break;
		}
		//in multiPlayer, cannot move Transporter to blocking tile either
		if (game.type == SKIRMISH
		    && isTransporter(psDroid)
		    && fpathBlockingTile(map_coord(psOrder->pos), getPropulsionStats(psDroid)->propulsionType))
		{
			break;
		}
		// move a droid to a location
		psDroid->order = *psOrder;
		actionDroid(psDroid, DACTION_MOVE, psOrder->pos.x, psOrder->pos.y);
		break;
	case DORDER_PATROL:
		psDroid->order = *psOrder;
		psDroid->order.pos2 = psDroid->pos.xy;
		actionDroid(psDroid, DACTION_MOVE, psOrder->pos.x, psOrder->pos.y);
		break;
	case DORDER_RECOVER:
		psDroid->order = *psOrder;
		actionDroid(psDroid, DACTION_MOVE, psOrder->psObj->pos.x, psOrder->psObj->pos.y);
		break;
	case DORDER_TRANSPORTOUT:
		// tell a (transporter) droid to leave home base for the offworld mission
		psDroid->order = *psOrder;
		actionDroid(psDroid, DACTION_TRANSPORTOUT, psOrder->pos.x, psOrder->pos.y);
		break;
	case DORDER_TRANSPORTRETURN:
		// tell a (transporter) droid to return after unloading
		psDroid->order = *psOrder;
		actionDroid(psDroid, DACTION_TRANSPORTOUT, psOrder->pos.x, psOrder->pos.y);
		break;
	case DORDER_TRANSPORTIN:
		// tell a (transporter) droid to fly onworld
		psDroid->order = *psOrder;
		actionDroid(psDroid, DACTION_TRANSPORTIN, psOrder->pos.x, psOrder->pos.y);
		break;
	case DORDER_ATTACK:
	case DORDER_ATTACKTARGET:
		if (psDroid->numWeaps == 0
		    || psDroid->asWeaps[0].nStat == 0
		    || isTransporter(psDroid))
		{
			break;
		}
		else if (psDroid->order.type == DORDER_GUARD && psOrder->type == DORDER_ATTACKTARGET)
		{
			// attacking something while guarding, don't change the order
			actionDroid(psDroid, DACTION_ATTACK, psOrder->psObj);
		}
		else if (psOrder->psObj && !psOrder->psObj->died)
		{
			//cannot attack a Transporter with EW in multiPlayer
			// FIXME: Why not ?
			if (game.type == SKIRMISH && electronicDroid(psDroid)
			    && psOrder->psObj->type == OBJ_DROID && isTransporter((DROID *)psOrder->psObj))
			{
				break;
			}
			psDroid->order = *psOrder;

			if (isVtolDroid(psDroid) || actionInRange(psDroid, psOrder->psObj, 0))
			{
				actionDroid(psDroid, DACTION_ATTACK, psOrder->psObj);
			}
			else
			{
				actionDroid(psDroid, DACTION_MOVE, psOrder->psObj->pos.x, psOrder->psObj->pos.y);
			}
		}
		break;
	case DORDER_BUILD:
	case DORDER_LINEBUILD:
		// build a new structure or line of structures
		ASSERT_OR_RETURN(, isConstructionDroid(psDroid), "%s cannot construct things!", objInfo(psDroid));
		ASSERT_OR_RETURN(, psOrder->psStats != nullptr, "invalid structure stats pointer");
		psDroid->order = *psOrder;
		ASSERT_OR_RETURN(, !psDroid->order.psStats || psDroid->order.psStats->type != REF_DEMOLISH, "Cannot build demolition");
		actionDroid(psDroid, DACTION_BUILD, psOrder->pos.x, psOrder->pos.y);
		objTrace(psDroid->id, "Starting new construction effort of %s", psOrder->psStats ? getName(psOrder->psStats) : "NULL");
		break;
	case DORDER_BUILDMODULE:
		//build a module onto the structure
		if (!isConstructionDroid(psDroid) || psOrder->index < nextModuleToBuild((STRUCTURE *)psOrder->psObj, -1))
		{
			break;
		}
		psDroid->order = DroidOrder(DORDER_BUILD, getModuleStat((STRUCTURE *)psOrder->psObj), psOrder->psObj->pos.xy, 0);
		ASSERT_OR_RETURN(, psDroid->order.psStats != nullptr, "should have found a module stats");
		ASSERT_OR_RETURN(, !psDroid->order.psStats || psDroid->order.psStats->type != REF_DEMOLISH, "Cannot build demolition");
		actionDroid(psDroid, DACTION_BUILD, psOrder->psObj->pos.x, psOrder->psObj->pos.y);
		objTrace(psDroid->id, "Starting new upgrade of %s", psOrder->psStats ? getName(psOrder->psStats) : "NULL");
		break;
	case DORDER_HELPBUILD:
		// help to build a structure that is starting to be built
		ASSERT_OR_RETURN(, isConstructionDroid(psDroid), "Not a constructor droid");
		ASSERT_OR_RETURN(, psOrder->psObj != nullptr, "Help to build a NULL pointer?");
		psDroid->order = *psOrder;
		psDroid->order.pos = psOrder->psObj->pos.xy;
		psDroid->order.psStats = ((STRUCTURE *)psOrder->psObj)->pStructureType;
		ASSERT_OR_RETURN(,!psDroid->order.psStats || psDroid->order.psStats->type != REF_DEMOLISH, "Cannot build demolition");
		actionDroid(psDroid, DACTION_BUILD, psDroid->order.pos.x, psDroid->order.pos.y);
		objTrace(psDroid->id, "Helping construction of %s", psOrder->psStats ? getName(psDroid->order.psStats) : "NULL");
		break;
	case DORDER_DEMOLISH:
		if (!(psDroid->droidType == DROID_CONSTRUCT || psDroid->droidType == DROID_CYBORG_CONSTRUCT))
		{
			break;
		}
		psDroid->order = *psOrder;
		psDroid->order.pos = psOrder->psObj->pos.xy;
		actionDroid(psDroid, DACTION_DEMOLISH, psOrder->psObj);
		break;
	case DORDER_REPAIR:
		if (!(psDroid->droidType == DROID_CONSTRUCT || psDroid->droidType == DROID_CYBORG_CONSTRUCT))
		{
			break;
		}
		psDroid->order = *psOrder;
		psDroid->order.pos = psOrder->psObj->pos.xy;
		actionDroid(psDroid, DACTION_REPAIR, psOrder->psObj);
		break;
	case DORDER_DROIDREPAIR:
		if (!(psDroid->droidType == DROID_REPAIR || psDroid->droidType == DROID_CYBORG_REPAIR))
		{
			break;
		}
		psDroid->order = *psOrder;
		actionDroid(psDroid, DACTION_DROIDREPAIR, psOrder->psObj);
		break;
	case DORDER_OBSERVE:
		// keep an object within sensor view
		psDroid->order = *psOrder;
		actionDroid(psDroid, DACTION_OBSERVE, psOrder->psObj);
		break;
	case DORDER_FIRESUPPORT:
		if (isTransporter(psDroid))
		{
			debug(LOG_ERROR, "Sorry, transports cannot be assigned to commanders.");
			psDroid->order = DroidOrder(DORDER_NONE);
			break;
		}
		if (psDroid->asWeaps[0].nStat == 0)
		{
			break;
		}
		psDroid->order = *psOrder;
		// let the order update deal with vtol droids
		if (!isVtolDroid(psDroid))
		{
			actionDroid(psDroid, DACTION_FIRESUPPORT, psOrder->psObj);
		}

		if (psDroid->player == selectedPlayer)
		{
			orderPlayFireSupportAudio(psOrder->psObj);
		}
		break;
	case DORDER_COMMANDERSUPPORT:
		if (isTransporter(psDroid))
		{
			debug(LOG_ERROR, "Sorry, transports cannot be assigned to commanders.");
			psDroid->order = DroidOrder(DORDER_NONE);
			break;
		}
		ASSERT_OR_RETURN(, psOrder->psObj != nullptr, "Can't command a NULL");
		if (psDroid->player == selectedPlayer)
		{
			orderPlayFireSupportAudio(psOrder->psObj);
		}
		cmdDroidAddDroid((DROID *)psOrder->psObj, psDroid);
		break;
	case DORDER_RETREAT:
	case DORDER_RUN:
		psDroid->order = *psOrder;
		if (psOrder->type != DORDER_RUN || psOrder->pos == Vector2i(0, 0))
		{
			psDroid->order.pos = asRunData[psDroid->player].sPos;
		}
		if (psDroid->order.pos == Vector2i(0, 0))
		{
			// We have still not managed to find a valid place to run.
			objTrace(psDroid->id, "Wants to run, but has no designated retreat point - standing still.");
			break;
		}
		actionDroid(psDroid, DACTION_MOVE, psDroid->order.pos.x, psDroid->order.pos.y);
		break;
	case DORDER_RTB:
		// send vtols back to their return pos
		if (isVtolDroid(psDroid) && !bMultiPlayer && psDroid->player != selectedPlayer)
		{
			int iDX = asVTOLReturnPos[psDroid->player].x;
			int iDY = asVTOLReturnPos[psDroid->player].y;

			if (iDX && iDY)
			{
				psDroid->order = DroidOrder(DORDER_LEAVEMAP);
				actionDroid(psDroid, DACTION_MOVE, iDX, iDY);
				break;
			}
		}

		for (psStruct = apsStructLists[psDroid->player]; psStruct; psStruct = psStruct->psNext)
		{
			if (psStruct->pStructureType->type == REF_HQ)
			{
				Vector2i pos = psStruct->pos.xy;

				psDroid->order = *psOrder;
				// Find a place to land for vtols. And Transporters in a multiPlay game.
				if (isVtolDroid(psDroid) || (game.type == SKIRMISH && isTransporter(psDroid)))
				{
					actionVTOLLandingPos(psDroid, &pos);
				}
				actionDroid(psDroid, DACTION_MOVE, pos.x, pos.y);
				break;
			}
		}
		// no HQ go to the landing zone
		if (psDroid->order.type != DORDER_RTB)
		{
			// see if the LZ has been set up
			int iDX = getLandingX(psDroid->player);
			int iDY = getLandingY(psDroid->player);

			if (iDX && iDY)
			{
				psDroid->order = *psOrder;
				actionDroid(psDroid, DACTION_MOVE, iDX, iDY);
			}
			else
			{
				// haven't got an LZ set up so don't do anything
				actionDroid(psDroid, DACTION_NONE);
				psDroid->order = DroidOrder(DORDER_NONE);
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
		if (psOrder->psObj == nullptr)
		{
			int iRepairFacDistSq = 0;

			psRepairFac = nullptr;
			for (psStruct = apsStructLists[psDroid->player]; psStruct; psStruct = psStruct->psNext)
			{
				if ((psStruct->pStructureType->type == REF_REPAIR_FACILITY) ||
				    ((psStruct->pStructureType->type == REF_HQ) && (psRepairFac == nullptr)))
				{
					int iStructDistSq = droidSqDist(psDroid, psStruct);

					if (iStructDistSq <= 0)
					{
						continue;	// cannot reach position
					}

					/* Choose current structure if first repair facility found or nearer than previously chosen facility, or is built */
					if (psRepairFac == nullptr || psRepairFac->pStructureType->type == REF_HQ || iRepairFacDistSq > iStructDistSq
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
		if (psDroid->order.type == DORDER_RTR && psDroid->order.psObj == psRepairFac)
		{
			objTrace(psDroid->id, "DONE FOR NOW");
			break;
		}

		/* give repair order if repair facility found */
		if (psRepairFac != nullptr)
		{
			/* move to front of structure */
			psDroid->order = DroidOrder(psOrder->type, psRepairFac);
			psDroid->order.pos = psRepairFac->pos.xy;
			/* If in multiPlayer, and the Transporter has been sent to be
				* repaired, need to find a suitable location to drop down. */
			if (game.type == SKIRMISH && isTransporter(psDroid))
			{
				Vector2i pos = psDroid->order.pos;

				objTrace(psDroid->id, "Repair transport");
				actionVTOLLandingPos(psDroid, &pos);
				actionDroid(psDroid, DACTION_MOVE, pos.x, pos.y);
			}
			else
			{
				objTrace(psDroid->id, "Go to repair facility at (%d, %d) using (%d, %d)!", psRepairFac->pos.x, psRepairFac->pos.y, psDroid->order.pos.x, psDroid->order.pos.y);
				actionDroid(psDroid, DACTION_MOVE, psRepairFac, psDroid->order.pos.x, psDroid->order.pos.y);
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
		{
			DROID *embarkee = castDroid(psOrder->psObj);
			if (isTransporter(psDroid)  // require a transporter for embarking.
			    || embarkee == nullptr || !isTransporter(embarkee))  // nor can a transporter load another transporter
			{
				debug(LOG_ERROR, "Sorry, can only load things that aren't transporters into things that are.");
				psDroid->order = DroidOrder(DORDER_NONE);
				break;
			}
			// move the droid to the transporter location
			psDroid->order = *psOrder;
			psDroid->order.pos = psOrder->psObj->pos.xy;
			actionDroid(psDroid, DACTION_MOVE, psOrder->psObj->pos.x, psOrder->psObj->pos.y);
			break;
		}
	case DORDER_DISEMBARK:
		//only valid in multiPlayer mode
		if (bMultiPlayer)
		{
			//this order can only be given to Transporter droids
			if (isTransporter(psDroid))
			{
				psDroid->order = *psOrder;
				//move the Transporter to the requested location
				actionDroid(psDroid, DACTION_MOVE, psOrder->pos.x, psOrder->pos.y);
				//close the Transporter interface - if up
				if (widgGetFromID(psWScreen, IDTRANS_FORM) != nullptr)
				{
					intRemoveTrans();
				}
			}
		}
		break;
	case DORDER_RECYCLE:
		psFactory = nullptr;
		iFactoryDistSq = 0;
		for (psStruct = apsStructLists[psDroid->player]; psStruct; psStruct = psStruct->psNext)
		{
			// Look for nearest factory or repair facility
			if (psStruct->pStructureType->type == REF_FACTORY || psStruct->pStructureType->type == REF_CYBORG_FACTORY
			    || psStruct->pStructureType->type == REF_VTOL_FACTORY || psStruct->pStructureType->type == REF_REPAIR_FACILITY)
			{
				/* get droid->facility distance squared */
				int iStructDistSq = droidSqDist(psDroid, psStruct);

				/* Choose current structure if first facility found or nearer than previously chosen facility */
				if (psStruct->status == SS_BUILT && iStructDistSq > 0 && (psFactory == nullptr || iFactoryDistSq > iStructDistSq))
				{
					psFactory = psStruct;
					iFactoryDistSq = iStructDistSq;
				}
			}
		}

		/* give recycle order if facility found */
		if (psFactory != nullptr)
		{
			/* move to front of structure */
			psDroid->order = DroidOrder(psOrder->type, psFactory);
			psDroid->order.pos = psFactory->pos.xy;
			setDroidTarget(psDroid,  psFactory);
			actionDroid(psDroid, DACTION_MOVE, psFactory, psDroid->order.pos.x, psDroid->order.pos.y);
		}
		break;
	case DORDER_GUARD:
		psDroid->order = *psOrder;
		if (psOrder->psObj != nullptr)
		{
			psDroid->order.pos = psOrder->psObj->pos.xy;
		}
		actionDroid(psDroid, DACTION_NONE);
		break;
	case DORDER_RESTORE:
		if (!electronicDroid(psDroid))
		{
			break;
		}
		if (psOrder->psObj->type != OBJ_STRUCTURE)
		{
			ASSERT(false, "orderDroidBase: invalid object type for Restore order");
			break;
		}
		psDroid->order = *psOrder;
		psDroid->order.pos = psOrder->psObj->pos.xy;
		actionDroid(psDroid, DACTION_RESTORE, psOrder->psObj);
		break;
	case DORDER_REARM:
		// didn't get executed before
		if (!isVtolDroid(psDroid))
		{
			break;
		}
		psDroid->order = *psOrder;
		actionDroid(psDroid, DACTION_MOVETOREARM, psOrder->psObj);
		assignVTOLPad(psDroid, (STRUCTURE *)psOrder->psObj);
		break;
	case DORDER_CIRCLE:
		if (!isVtolDroid(psDroid))
		{
			break;
		}
		psDroid->order = *psOrder;
		actionDroid(psDroid, DACTION_MOVE, psOrder->pos.x, psOrder->pos.y);
		break;
	default:
		ASSERT(false, "orderUnitBase: unknown order");
		break;
	}

	syncDebugDroid(psDroid, '+');
}


/** This function sends the droid an order. It uses sendDroidInfo() if mode == ModeQueue and orderDroidBase() if not. */
void orderDroid(DROID *psDroid, DROID_ORDER order, QUEUE_MODE mode)
{
	ASSERT(psDroid != nullptr,
	       "orderUnit: Invalid unit pointer");
	ASSERT(order == DORDER_NONE ||
	       order == DORDER_RETREAT ||
	       order == DORDER_RTR ||
	       order == DORDER_RTB ||
	       order == DORDER_RECYCLE ||
	       order == DORDER_RUN ||
	       order == DORDER_TRANSPORTIN ||
	       order == DORDER_STOP ||		// Added this PD.
	       order == DORDER_HOLD,
	       "orderUnit: Invalid order");

	DROID_ORDER_DATA sOrder(order);
	if (mode == ModeQueue && bMultiPlayer)
	{
		sendDroidInfo(psDroid, sOrder, false);
	}
	else
	{
		orderClearDroidList(psDroid);
		orderDroidBase(psDroid, &sOrder);
	}
}


/** This function compares the current droid's order to the order.
 * Returns true if they are the same, false else.
 */
bool orderState(DROID *psDroid, DROID_ORDER order)
{
	if (order == DORDER_RTR)
	{
		return psDroid->order.type == DORDER_RTR || psDroid->order.type == DORDER_RTR_SPECIFIED;
	}

	return psDroid->order.type == order;
}


/** This function returns true if the order is an acceptable order to give for a given location on the map.*/
bool validOrderForLoc(DROID_ORDER order)
{
	return (order == DORDER_NONE ||	order == DORDER_MOVE ||	order == DORDER_GUARD ||
	        order == DORDER_SCOUT || order == DORDER_RUN || order == DORDER_PATROL ||
	        order == DORDER_TRANSPORTOUT || order == DORDER_TRANSPORTIN  ||
	        order == DORDER_TRANSPORTRETURN || order == DORDER_DISEMBARK ||
	        order == DORDER_CIRCLE);
}


/** This function sends the droid an order with a location.
 * If the mode is ModeQueue, the order is added to the droid's order list using sendDroidInfo(), else, a DROID_ORDER_DATA is alloc, the old order list is erased, and the order is sent using orderDroidBase().
 */
void orderDroidLoc(DROID *psDroid, DROID_ORDER order, UDWORD x, UDWORD y, QUEUE_MODE mode)
{
	ASSERT_OR_RETURN(, psDroid != nullptr, "Invalid unit pointer");
	ASSERT_OR_RETURN(, validOrderForLoc(order), "Invalid order for location");

	DROID_ORDER_DATA sOrder(order, Vector2i(x, y));
	if (mode == ModeQueue)
	{
		sendDroidInfo(psDroid, sOrder, false);
		return;  // Wait to receive our order before changing the droid.
	}

	orderClearDroidList(psDroid);
	orderDroidBase(psDroid, &sOrder);
}


/** This function attributes the order's location to (pX,pY) if the order is the same as the droid.
 * Returns true if it was attributed and false if not.
 */
bool orderStateLoc(DROID *psDroid, DROID_ORDER order, UDWORD *pX, UDWORD *pY)
{
	if (order != psDroid->order.type)
	{
		return false;
	}

	// check the order is one with a location
	switch (psDroid->order.type)
	{
	default:
		// not a location order - return false
		break;
	case DORDER_MOVE:
	case DORDER_RETREAT:
		*pX = psDroid->order.pos.x;
		*pY = psDroid->order.pos.y;
		return true;
		break;
	}

	return false;
}


/** This function returns true if the order is a valid order to give to an object and false if it's not.*/
bool validOrderForObj(DROID_ORDER order)
{
	return (order == DORDER_NONE || order == DORDER_HELPBUILD || order == DORDER_DEMOLISH ||
	        order == DORDER_REPAIR || order == DORDER_ATTACK || order == DORDER_FIRESUPPORT || order == DORDER_COMMANDERSUPPORT ||
	        order == DORDER_OBSERVE || order == DORDER_ATTACKTARGET || order == DORDER_RTR ||
	        order == DORDER_RTR_SPECIFIED || order == DORDER_EMBARK || order == DORDER_GUARD ||
	        order == DORDER_DROIDREPAIR || order == DORDER_RESTORE || order == DORDER_BUILDMODULE ||
	        order == DORDER_REARM || order == DORDER_RECOVER);
}


/** This function sends an order with an object to the droid.
 * If the mode is ModeQueue, the order is added to the droid's order list using sendDroidInfo(), else, a DROID_ORDER_DATA is alloc, the old order list is erased, and the order is sent using orderDroidBase().
 */
void orderDroidObj(DROID *psDroid, DROID_ORDER order, BASE_OBJECT *psObj, QUEUE_MODE mode)
{
	ASSERT(psDroid != nullptr, "Invalid unit pointer");
	ASSERT(validOrderForObj(order), "Invalid order for object");
	ASSERT_OR_RETURN(, !isBlueprint(psObj), "Target %s is a blueprint", objInfo(psObj));
	ASSERT_OR_RETURN(, !psObj->died, "Target dead");

	DroidOrder sOrder(order, psObj);
	if (mode == ModeQueue) //ajl
	{
		sendDroidInfo(psDroid, sOrder, false);
		return;  // Wait for the order to be received before changing the droid.
	}

	orderClearDroidList(psDroid);
	orderDroidBase(psDroid, &sOrder);
}


/** This function returns the order's target if it has one, and NULL if the order is not a target order.
 * @todo the first switch can be removed and substituted by orderState() function.
 * @todo the use of this function is somewhat superfluous on some cases. Investigate.
 */
BASE_OBJECT *orderStateObj(DROID *psDroid, DROID_ORDER order)
{
	bool	match = false;

	switch (order)
	{
	case DORDER_BUILD:
	case DORDER_LINEBUILD:
	case DORDER_HELPBUILD:
		if (psDroid->order.type == DORDER_BUILD ||
		    psDroid->order.type == DORDER_HELPBUILD ||
		    psDroid->order.type == DORDER_LINEBUILD)
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
		if (psDroid->order.type == order)
		{
			match = true;
		}
		break;
	case DORDER_RTR:
		if (psDroid->order.type == DORDER_RTR ||
		    psDroid->order.type == DORDER_RTR_SPECIFIED)
		{
			match = true;
		}
	default:
		break;
	}

	if (!match)
	{
		return nullptr;
	}

	// check the order is one with an object
	switch (psDroid->order.type)
	{
	default:
		// not an object order - return false
		return nullptr;
		break;
	case DORDER_BUILD:
	case DORDER_LINEBUILD:
		if (psDroid->action == DACTION_BUILD ||
		    psDroid->action == DACTION_BUILDWANDER)
		{
			return psDroid->order.psObj;
		}
		break;
	case DORDER_HELPBUILD:
		if (psDroid->action == DACTION_BUILD ||
		    psDroid->action == DACTION_BUILDWANDER ||
		    psDroid->action == DACTION_MOVETOBUILD)
		{
			return psDroid->order.psObj;
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
		return psDroid->order.psObj;
		break;
	}

	return nullptr;
}


/** This function sends the droid an order with a location and stats.
 * If the mode is ModeQueue, the order is added to the droid's order list using sendDroidInfo(), else, a DROID_ORDER_DATA is alloc, the old order list is erased, and the order is sent using orderDroidBase().
 */
void orderDroidStatsLocDir(DROID *psDroid, DROID_ORDER order, STRUCTURE_STATS *psStats, UDWORD x, UDWORD y, uint16_t direction, QUEUE_MODE mode)
{
	ASSERT(psDroid != nullptr, "Invalid unit pointer");
	ASSERT(order == DORDER_BUILD, "Invalid order for location");

	DroidOrder sOrder(order, psStats, Vector2i(x, y), direction);
	if (mode == ModeQueue && bMultiPlayer)
	{
		sendDroidInfo(psDroid, sOrder, false);
		return;  // Wait for our order before changing the droid.
	}

	orderClearDroidList(psDroid);
	orderDroidBase(psDroid, &sOrder);
}


/** This function adds that order to the droid's list using sendDroidInfo().
 * @todo seems closely related with orderDroidStatsLocDir(). See if this one can be incorporated on it.
 */
void orderDroidStatsLocDirAdd(DROID *psDroid, DROID_ORDER order, STRUCTURE_STATS *psStats, UDWORD x, UDWORD y, uint16_t direction, bool add)
{
	ASSERT(psDroid != nullptr, "Invalid unit pointer");

	// can only queue build orders with this function
	if (order != DORDER_BUILD)
	{
		return;
	}

	sendDroidInfo(psDroid, DroidOrder(order, psStats, Vector2i(x, y), direction), add);
}


/** Equivalent to orderDroidStatsLocDir(), but uses two locations.*/
void orderDroidStatsTwoLocDir(DROID *psDroid, DROID_ORDER order, STRUCTURE_STATS *psStats, UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2, uint16_t direction, QUEUE_MODE mode)
{
	ASSERT(psDroid != nullptr,	"Invalid unit pointer");
	ASSERT(order == DORDER_LINEBUILD, "Invalid order for location");
	ASSERT(x1 == x2 || y1 == y2, "Invalid locations for LINEBUILD");

	DroidOrder sOrder(order, psStats, Vector2i(x1, y1), Vector2i(x2, y2), direction);
	if (mode == ModeQueue && bMultiPlayer)
	{
		sendDroidInfo(psDroid, sOrder, false);
		return;  // Wait for our order before changing the droid.
	}

	orderClearDroidList(psDroid);
	orderDroidBase(psDroid, &sOrder);
}


/** Equivalent to orderDroidStatsLocDirAdd(), but uses two locations.
 * @todo seems closely related with orderDroidStatsTwoLocDir(). See if this can be incorporated on it.
 */
void orderDroidStatsTwoLocDirAdd(DROID *psDroid, DROID_ORDER order, STRUCTURE_STATS *psStats, UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2, uint16_t direction)
{
	ASSERT(psDroid != nullptr, "Invalid unit pointer");
	ASSERT(order == DORDER_LINEBUILD, "Invalid order for location");
	ASSERT(x1 == x2 || y1 == y2, "Invalid locations for LINEBUILD");

	sendDroidInfo(psDroid, DroidOrder(order, psStats, Vector2i(x1, y1), Vector2i(x2, y2), direction), true);
}


/** This function returns false if droid's order and order don't match or the order is not a location order. Else ppsStats = psDroid->psTarStats, (pX,pY) = psDroid.(orderX,orderY) and it returns true.
 * @todo seems closely related to orderStateLoc()
 */
bool orderStateStatsLoc(DROID *psDroid, DROID_ORDER order, BASE_STATS **ppsStats, UDWORD *pX, UDWORD *pY)
{
	bool	match = false;

	switch (order)
	{
	case DORDER_BUILD:
	case DORDER_LINEBUILD:
		if (psDroid->order.type == DORDER_BUILD ||
		    psDroid->order.type == DORDER_LINEBUILD)
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
	switch (psDroid->order.type)
	{
	default:
		// not a stats/location order - return false
		return false;
		break;
	case DORDER_BUILD:
	case DORDER_LINEBUILD:
		if (psDroid->action == DACTION_MOVETOBUILD)
		{
			*ppsStats = psDroid->order.psStats;
			*pX = psDroid->order.pos.x;
			*pY = psDroid->order.pos.y;
			return true;
		}
		break;
	}

	return false;
}


/** @todo needs documentation.*/
void orderDroidAddPending(DROID *psDroid, DROID_ORDER_DATA *psOrder)
{
	ASSERT_OR_RETURN(, psDroid != nullptr, "Invalid unit pointer");

	psDroid->asOrderList.push_back(*psOrder);

	// Only display one arrow, bOrderEffectDisplayed must be set to false once per arrow.
	if (!bOrderEffectDisplayed)
	{
		Vector3i position;
		if (psOrder->psObj == nullptr)
		{
			position.x = psOrder->pos.x;
			position.z = psOrder->pos.y;
		}
		else
		{
			position = psOrder->psObj->pos.xzy;
		}
		position.y = map_Height(position.x, position.z) + 32;
		if (psOrder->psObj != nullptr && psOrder->psObj->sDisplay.imd != nullptr)
		{
			position.y += psOrder->psObj->sDisplay.imd->max.y;
		}
		addEffect(&position, EFFECT_WAYPOINT, WAYPOINT_TYPE, false, nullptr, 0);
		bOrderEffectDisplayed = true;
	}
}


/** Add an order to a droid's order list
 * @todo needs better documentation.
 */
void orderDroidAdd(DROID *psDroid, DROID_ORDER_DATA *psOrder)
{
	ASSERT_OR_RETURN(, psDroid != nullptr, "Invalid unit pointer");

	if (psDroid->listSize >= psDroid->asOrderList.size())
	{
		// Make more room to store the order.
		psDroid->asOrderList.resize(psDroid->asOrderList.size() + 1);
	}

	psDroid->asOrderList[psDroid->listSize] = *psOrder;
	psDroid->listSize += 1;

	// if not doing anything - do it immediately
	if (psDroid->listSize <= 1 &&
	    (psDroid->order.type == DORDER_NONE ||
	     psDroid->order.type == DORDER_GUARD ||
	     psDroid->order.type == DORDER_PATROL ||
	     psDroid->order.type == DORDER_CIRCLE ||
	     psDroid->order.type == DORDER_HOLD))
	{
		orderDroidList(psDroid);
	}
}


/** This function goes to the droid's order list and sets a new order to it from its order list.*/
bool orderDroidList(DROID *psDroid)
{
	if (psDroid->listSize > 0)
	{
		// there are some orders to give
		DROID_ORDER_DATA sOrder = psDroid->asOrderList[0];
		orderDroidListEraseRange(psDroid, 0, 1);

		switch (sOrder.type)
		{
		case DORDER_MOVE:
		case DORDER_SCOUT:
		case DORDER_DISEMBARK:
			ASSERT(sOrder.psObj == nullptr && sOrder.psStats == nullptr, "Extra %s parameters.", getDroidOrderName(sOrder.type));
			sOrder.psObj = nullptr;
			sOrder.psStats = nullptr;
			break;
		case DORDER_ATTACK:
		case DORDER_REPAIR:
		case DORDER_OBSERVE:
		case DORDER_DROIDREPAIR:
		case DORDER_FIRESUPPORT:
		case DORDER_DEMOLISH:
		case DORDER_HELPBUILD:
		case DORDER_BUILDMODULE:
		case DORDER_RECOVER:
			ASSERT(sOrder.psStats == nullptr, "Extra %s parameters.", getDroidOrderName(sOrder.type));
			sOrder.psStats = nullptr;
			break;
		case DORDER_BUILD:
		case DORDER_LINEBUILD:
			ASSERT(sOrder.psObj == nullptr, "Extra %s parameters.", getDroidOrderName(sOrder.type));
			sOrder.psObj = nullptr;
			break;
		default:
			ASSERT(false, "orderDroidList: Invalid order");
			return false;
		}

		orderDroidBase(psDroid, &sOrder);
		syncDebugDroid(psDroid, 'o');

		return true;
	}

	return false;
}


/** This function goes to the droid's order list and erases its elements from indexBegin to indexEnd.*/
void orderDroidListEraseRange(DROID *psDroid, unsigned indexBegin, unsigned indexEnd)
{
	// Erase elements
	indexEnd = MIN(indexEnd, psDroid->asOrderList.size());  // Do nothing if trying to pop an empty list.
	psDroid->asOrderList.erase(psDroid->asOrderList.begin() + indexBegin, psDroid->asOrderList.begin() + indexEnd);

	// Update indices into list.
	psDroid->listSize         -= MIN(indexEnd, psDroid->listSize)         - MIN(indexBegin, psDroid->listSize);
	psDroid->listPendingBegin -= MIN(indexEnd, psDroid->listPendingBegin) - MIN(indexBegin, psDroid->listPendingBegin);
}


/** This function clears all the synchronised orders from the list, calling orderDroidListEraseRange() from 0 to psDroid->listSize.*/
void orderClearDroidList(DROID *psDroid)
{
	syncDebug("droid%d list cleared", psDroid->id);
	orderDroidListEraseRange(psDroid, 0, psDroid->listSize);
}


/** This function clears all the orders from droid's order list that don't have target as psTarget.*/
void orderClearTargetFromDroidList(DROID *psDroid, BASE_OBJECT *psTarget)
{
	for (unsigned i = 0; i < psDroid->asOrderList.size(); ++i)
	{
		if (psDroid->asOrderList[i].psObj == psTarget)
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


/** This function checks its order list: if a given order needs a target and the target has died, the order is removed from the list.*/
void orderCheckList(DROID *psDroid)
{
	for (unsigned i = 0; i < psDroid->asOrderList.size(); ++i)
	{
		BASE_OBJECT *psTarget = psDroid->asOrderList[i].psObj;
		if (psTarget != nullptr && psTarget->died)
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


/** This function sends the droid an order with a location using sendDroidInfo().
 * @todo it is very close to what orderDroidLoc() function does. Suggestion to refract them.
 */
static bool orderDroidLocAdd(DROID *psDroid, DROID_ORDER order, UDWORD x, UDWORD y, bool add = true)
{
	// can only queue move, scout, and disembark orders
	if (order != DORDER_MOVE && order != DORDER_SCOUT && order != DORDER_DISEMBARK)
	{
		return false;
	}

	sendDroidInfo(psDroid, DroidOrder(order, Vector2i(x, y)), add);

	return true;
}


/** This function sends the droid an order with a location using sendDroidInfo().
 * @todo it is very close to what orderDroidObj() function does. Suggestion to refract them.
 */
static bool orderDroidObjAdd(DROID *psDroid, DroidOrder const &order, bool add)
{
	ASSERT(!isBlueprint(order.psObj), "Target %s for queue is a blueprint", objInfo(order.psObj));

	// check can queue the order
	switch (order.type)
	{
	case DORDER_ATTACK:
	case DORDER_REPAIR:
	case DORDER_OBSERVE:
	case DORDER_DROIDREPAIR:
	case DORDER_FIRESUPPORT:
	case DORDER_DEMOLISH:
	case DORDER_HELPBUILD:
	case DORDER_BUILDMODULE:
		break;
	default:
		return false;
	}

	sendDroidInfo(psDroid, order, add);

	return true;
}


/** This function returns an order which is assigned according to the location and droid. Uses altOrder flag to choose between a direct order or an altOrder.*/
DROID_ORDER chooseOrderLoc(DROID *psDroid, UDWORD x, UDWORD y, bool altOrder)
{
	DROID_ORDER		order = DORDER_NONE;
	PROPULSION_TYPE		propulsion = getPropulsionStats(psDroid)->propulsionType;

	if (isTransporter(psDroid) && game.type == CAMPAIGN)
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
	if (isTransporter(psDroid) && game.type == SKIRMISH)
	{
		/* in MultiPlayer - if ALT-key is pressed then need to get the Transporter
		 * to fly to location and all units disembark */
		if (altOrder)
		{
			order = DORDER_DISEMBARK;
		}
	}
	else if (secondaryGetState(psDroid, DSO_CIRCLE, ModeQueue) == DSS_CIRCLE_SET)  // ModeQueue here means to check whether we pressed the circle button, whether or not it synched yet. The reason for this weirdness is that a circle order makes no sense as a secondary state in the first place (the circle button _should_ have been only in the UI, not in the game state..!), so anything dealing with circle orders will necessarily be weird.
	{
		order = DORDER_CIRCLE;
		secondarySetState(psDroid, DSO_CIRCLE, DSS_NONE);
	}
	else if (secondaryGetState(psDroid, DSO_PATROL, ModeQueue) == DSS_PATROL_SET)  // ModeQueue here means to check whether we pressed the patrol button, whether or not it synched yet. The reason for this weirdness is that a patrol order makes no sense as a secondary state in the first place (the patrol button _should_ have been only in the UI, not in the game state..!), so anything dealing with patrol orders will necessarily be weird.
	{
		order = DORDER_PATROL;
		secondarySetState(psDroid, DSO_PATROL, DSS_NONE);
	}

	return order;
}


/** This function sends the selected droids an order to given a location. If a delivery point is selected, it is moved to a new location.
 * If add is true then the order is queued.
 * This function should only be called from UI.
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

	// note that an order list graphic needs to be displayed
	bOrderEffectDisplayed = false;

	for (psCurr = apsDroidLists[player]; psCurr; psCurr = psCurr->psNext)
	{
		if (psCurr->selected)
		{
			// can't use bMultiPlayer since multimsg could be off.
			// FIXME: Fix this for DROID_SUPERTRANSPORTER when we fix campaign scripts
			if (psCurr->droidType == DROID_TRANSPORTER && game.type == CAMPAIGN)
			{
				// Transport in campaign cannot be controlled by players
				DeSelectDroid(psCurr);
				continue;
			}

			order = chooseOrderLoc(psCurr, x, y, specialOrderKeyDown());
			// see if the order can be added to the list
			if (order != DORDER_NONE && !(add && orderDroidLocAdd(psCurr, order, x, y)))
			{
				// if not just do it straight off
				orderDroidLoc(psCurr, order, x, y, ModeQueue);
			}
		}
	}
}


static int highestQueuedModule(DroidOrder const &order, STRUCTURE const *structure, int prevHighestQueuedModule)
{
	int thisQueuedModule = -1;
	switch (order.type)
	{
	default:
		break;
	case DORDER_BUILDMODULE:
		if (order.psObj == structure)  // Order must be for this structure.
		{
			thisQueuedModule = order.index;  // Order says which module to build.
		}
		break;
	case DORDER_BUILD:
	case DORDER_HELPBUILD:
		{
			// Current order is weird, the DORDER_BUILDMODULE mutates into a DORDER_BUILD, and we use the order.pos instead of order.psObj.
			// Also, might be DORDER_BUILD if selecting the module from the menu before clicking on the structure.
			STRUCTURE *orderStructure = castStructure(worldTile(order.pos)->psObject);
			if (orderStructure == structure && (order.psStats == orderStructure->pStructureType || order.psStats == getModuleStat(orderStructure)))  // Order must be for this structure.
			{
				thisQueuedModule = nextModuleToBuild(structure, prevHighestQueuedModule);
			}
			break;
		}
	}
	return std::max(prevHighestQueuedModule, thisQueuedModule);
}

static int highestQueuedModule(DROID const *droid, STRUCTURE const *structure)
{
	int module = highestQueuedModule(droid->order, structure, -1);
	for (unsigned n = droid->listPendingBegin; n < droid->asOrderList.size(); ++n)
	{
		module = highestQueuedModule(droid->asOrderList[n], structure, module);
	}
	return module;
}

/** This function returns an order according to the droid, object (target) and altOrder.*/
DroidOrder chooseOrderObj(DROID *psDroid, BASE_OBJECT *psObj, bool altOrder)
{
	DroidOrder order(DORDER_NONE);
	STRUCTURE		*psStruct;

	if (isTransporter(psDroid))
	{
		//in multiPlayer, need to be able to get Transporter repaired
		if (bMultiPlayer)
		{
			if (aiCheckAlliances(psObj->player, psDroid->player) &&
			    psObj->type == OBJ_STRUCTURE)
			{
				psStruct = (STRUCTURE *) psObj;
				ASSERT_OR_RETURN(DroidOrder(DORDER_NONE), psObj != nullptr, "Invalid structure pointer");
				if (psStruct->pStructureType->type == REF_REPAIR_FACILITY &&
				    psStruct->status == SS_BUILT)
				{
					return DroidOrder(DORDER_RTR_SPECIFIED, psObj);
				}
			}
		}
		return DroidOrder(DORDER_NONE);
	}

	if (altOrder && (psObj->type == OBJ_DROID || psObj->type == OBJ_STRUCTURE) && psDroid->player == psObj->player)
	{
		if (psDroid->droidType == DROID_SENSOR)
		{
			return DroidOrder(DORDER_OBSERVE, psObj);
		}
		else if ((psDroid->droidType == DROID_REPAIR ||
		          psDroid->droidType == DROID_CYBORG_REPAIR) && psObj->type == OBJ_DROID)
		{
			return DroidOrder(DORDER_DROIDREPAIR, psObj);
		}
		else if ((psDroid->droidType == DROID_WEAPON) || cyborgDroid(psDroid) ||
		         (psDroid->droidType == DROID_COMMAND))
		{
			return DroidOrder(DORDER_ATTACK, psObj);
		}
	}
	//check for transporters first
	if (psObj->type == OBJ_DROID && isTransporter((DROID *)psObj) && psObj->player == psDroid->player)
	{
		order = DroidOrder(DORDER_EMBARK, psObj);
	}
	// go to recover an artifact/oil drum - don't allow VTOL's to get this order
	else if (psObj->type == OBJ_FEATURE &&
	         (((FEATURE *)psObj)->psStats->subType == FEAT_GEN_ARTE ||
	          ((FEATURE *)psObj)->psStats->subType == FEAT_OIL_DRUM))
	{
		if (!isVtolDroid(psDroid))
		{
			order = DroidOrder(DORDER_RECOVER, psObj);
		}
	}
	// else default to attack if the droid has a weapon
	else if (psDroid->numWeaps > 0
	         && psObj->player != psDroid->player
	         && !aiCheckAlliances(psObj->player, psDroid->player))
	{
		// check valid weapon/prop combination
		if (validTarget(psDroid, psObj, 0))
		{
			order = DroidOrder(DORDER_ATTACK, psObj);
		}
	}
	else if (psDroid->droidType == DROID_SENSOR
	         && psObj->player != psDroid->player
	         && !aiCheckAlliances(psObj->player , psDroid->player))
	{
		//check for standard sensor or VTOL intercept sensor
		if (asSensorStats[psDroid->asBits[COMP_SENSOR]].type == STANDARD_SENSOR
		    || asSensorStats[psDroid->asBits[COMP_SENSOR]].type == VTOL_INTERCEPT_SENSOR
		    || asSensorStats[psDroid->asBits[COMP_SENSOR]].type == SUPER_SENSOR)
		{
			// a sensor droid observing an object
			order = DroidOrder(DORDER_OBSERVE, psObj);
		}
	}
	else if (droidSensorDroidWeapon(psObj, psDroid))
	{
		// got an indirect weapon droid or vtol doing fire support
		order = DroidOrder(DORDER_FIRESUPPORT, psObj);
		setSensorAssigned();
	}
	else if (psObj->player == psDroid->player &&
	         psObj->type == OBJ_DROID &&
	         ((DROID *)psObj)->droidType == DROID_COMMAND &&
	         psDroid->droidType != DROID_COMMAND &&
	         psDroid->droidType != DROID_CONSTRUCT &&
	         psDroid->droidType != DROID_CYBORG_CONSTRUCT)
	{
		// get a droid to join a command droids group
		DeSelectDroid(psDroid);
		order = DroidOrder(DORDER_COMMANDERSUPPORT, psObj);
	}
	//repair droid
	else if (aiCheckAlliances(psObj->player, psDroid->player) &&
	         psObj->type == OBJ_DROID &&
	         (psDroid->droidType == DROID_REPAIR ||
	          psDroid->droidType == DROID_CYBORG_REPAIR) &&
	         droidIsDamaged((DROID *)psObj))
	{
		order = DroidOrder(DORDER_DROIDREPAIR, psObj);
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
		order = DroidOrder(DORDER_GUARD, psObj);
		assignSensorTarget(psObj);
		psDroid->selected = false;
	}
	else if (aiCheckAlliances(psObj->player, psDroid->player) &&
	         psObj->type == OBJ_STRUCTURE)
	{
		psStruct = (STRUCTURE *) psObj;
		ASSERT_OR_RETURN(DroidOrder(DORDER_NONE), psObj != nullptr, "Invalid structure pointer");

		/* check whether construction droid */
		if (psDroid->droidType == DROID_CONSTRUCT ||
		    psDroid->droidType == DROID_CYBORG_CONSTRUCT)
		{
			int moduleIndex = nextModuleToBuild(psStruct, ctrlShiftDown() ? highestQueuedModule(psDroid, psStruct) : -1);

			//Re-written to allow demolish order to be added to the queuing system
			if (intDemolishSelectMode() && psObj->player == psDroid->player)
			{
				//check to see if anything is currently trying to build the structure
				//can't build and demolish at the same time!
				if (psStruct->status == SS_BUILT || !checkDroidsBuilding(psStruct))
				{
					order = DroidOrder(DORDER_DEMOLISH, psObj);
				}
			}
			//check for non complete structures
			else if (psStruct->status != SS_BUILT)
			{
				//if something else is demolishing, then help demolish
				if (checkDroidsDemolishing(psStruct))
				{
					order = DroidOrder(DORDER_DEMOLISH, psObj);
				}
				//else help build
				else
				{
					order = DroidOrder(DORDER_HELPBUILD, psObj);
					if (moduleIndex > 0)
					{
						order = DroidOrder(DORDER_BUILDMODULE, psObj, moduleIndex);  // Try scheduling a module, instead.
					}
				}
			}
			else if (psStruct->body < structureBody(psStruct))
			{
				order = DroidOrder(DORDER_REPAIR, psObj);
			}
			//check if can build a module
			else if (moduleIndex > 0)
			{
				order = DroidOrder(DORDER_BUILDMODULE, psObj, moduleIndex);
			}
		}

		if (order.type == DORDER_NONE)
		{
			/* check repair facility and in need of repair */
			if (psStruct->pStructureType->type == REF_REPAIR_FACILITY &&
			    psStruct->status == SS_BUILT)
			{
				order = DroidOrder(DORDER_RTR_SPECIFIED, psObj);
			}
			else if (electronicDroid(psDroid) &&
			         //psStruct->resistance < (SDWORD)(psStruct->pStructureType->resistance))
			         psStruct->resistance < (SDWORD)structureResistance(psStruct->
			                 pStructureType, psStruct->player))
			{
				order = DroidOrder(DORDER_RESTORE, psObj);
			}
			//check for counter battery assignment
			else if (structSensorDroidWeapon(psStruct, psDroid))
			{
				order = DroidOrder(DORDER_FIRESUPPORT, psObj);
				//inform display system
				setSensorAssigned();
				//deselect droid
				DeSelectDroid(psDroid);
			}
			//REARM VTOLS
			else if (isVtolDroid(psDroid))
			{
				//default to no order
				//check if rearm pad
				if (psStruct->pStructureType->type == REF_REARM_PAD)
				{
					//don't bother checking cos we want it to go there if directed
					order = DroidOrder(DORDER_REARM, psObj);
				}
			}
			// Some droids shouldn't be guarding
			else if ((psDroid->droidType == DROID_WEAPON ||
			          psDroid->droidType == DROID_CYBORG ||
			          psDroid->droidType == DROID_CYBORG_SUPER)
			         && proj_Direct(asWeaponStats + psDroid->asWeaps[0].nStat))
			{
				order = DroidOrder(DORDER_GUARD, psObj);
			}
		}
	}

	return order;
}


/** This function runs through all the player's droids and if the droid is vtol and is selected and is attacking, uses audio_QueueTrack() to play a sound.
 * @todo this function has variable psObj unused. Consider removing it from argument.
 * @todo this function runs through all the player's droids, but only uses the selected ones. Consider an efficiency improvement in here.
 * @todo current scope of this function is quite small. Consider refactoring it.
 */
static void orderPlayOrderObjAudio(UDWORD player, BASE_OBJECT *psObj)
{
	DROID	*psDroid;

	/* loop over selected droids */
	for (psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->selected)
		{
			/* currently only looks for VTOL */
			if (isVtolDroid(psDroid))
			{
				switch (psDroid->order.type)
				{
				case DORDER_ATTACK:
					audio_QueueTrack(ID_SOUND_ON_OUR_WAY2);
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


/** This function sends orders to all the selected droids according to the object.
 * If add is true, the orders are queued.
 * @todo this function runs through all the player's droids, but only uses the selected ones. Consider an efficiency improvement in here.
 */
void orderSelectedObjAdd(UDWORD player, BASE_OBJECT *psObj, bool add)
{
	DROID		*psCurr;
	// note that an order list graphic needs to be displayed
	bOrderEffectDisplayed = false;

	for (psCurr = apsDroidLists[player]; psCurr; psCurr = psCurr->psNext)
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

			DroidOrder order = chooseOrderObj(psCurr, psObj, specialOrderKeyDown());
			// see if the order can be added to the list
			if (order.type != DORDER_NONE && !orderDroidObjAdd(psCurr, order, add))
			{
				// if not just do it straight off
				orderDroidObj(psCurr, order.type, order.psObj, ModeQueue);
			}
		}
	}

	orderPlayOrderObjAudio(player, psObj);
}


/** This function just calls orderSelectedObjAdd with add = false.*/
void orderSelectedObj(UDWORD player, BASE_OBJECT *psObj)
{
	orderSelectedObjAdd(player, psObj, false);
}


/** Given a player, this function send an order with localization and status to selected droids.
 * If add is true, the orders are queued.
 * @todo this function runs through all the player's droids, but only uses the selected ones and the ones that are construction droids. Consider an efficiency improvement.
 */
void orderSelectedStatsLocDir(UDWORD player, DROID_ORDER order, STRUCTURE_STATS *psStats, UDWORD x, UDWORD y, uint16_t direction, bool add)
{
	DROID		*psCurr;

	for (psCurr = apsDroidLists[player]; psCurr; psCurr = psCurr->psNext)
	{
		if (psCurr->selected && isConstructionDroid(psCurr))
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


/** Same as orderSelectedStatsLocDir() but with two locations.
 * @todo this function runs through all the player's droids, but only uses the selected ones. Consider an efficiency improvement.
 */
void orderSelectedStatsTwoLocDir(UDWORD player, DROID_ORDER order, STRUCTURE_STATS *psStats, UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2, uint16_t direction, bool add)
{
	DROID		*psCurr;

	for (psCurr = apsDroidLists[player]; psCurr; psCurr = psCurr->psNext)
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


/** This function runs though all player's droids to check if any of then is a transporter. Returns the transporter droid if any was found, and NULL else.*/
DROID *FindATransporter(DROID const *embarkee)
{
	bool isCyborg = cyborgDroid(embarkee) || !bMultiPlayer;  // In campaign, any unit can go on a regular transporter, so consider any unit to be a cyborg.

	DROID *bestDroid = nullptr;
	unsigned bestDist = ~0u;

	for (DROID *psDroid = apsDroidLists[embarkee->player]; psDroid != nullptr; psDroid = psDroid->psNext)
	{
		if ((isCyborg && psDroid->droidType == DROID_TRANSPORTER) || psDroid->droidType == DROID_SUPERTRANSPORTER)
		{
			unsigned dist = iHypot((psDroid->pos - embarkee->pos).xy);
			if (!checkTransporterSpace(psDroid, embarkee, false))
			{
				dist += 0x8000000;  // Should prefer transports that aren't full.
			}
			if (dist < bestDist)
			{
				bestDroid = psDroid;
				bestDist = dist;
			}
		}
	}

	return bestDroid;
}


/** Given a factory type, this function runs though all player's structures to check if any is of factory type. Returns the structure if any was found, and NULL else.*/
static STRUCTURE *FindAFactory(UDWORD player, UDWORD factoryType)
{
	STRUCTURE *psStruct;

	ASSERT_OR_RETURN(nullptr, player < MAX_PLAYERS, "Invalid player number");

	for (psStruct = apsStructLists[player]; psStruct != nullptr; psStruct = psStruct->psNext)
	{
		if (psStruct->pStructureType->type == factoryType)
		{
			return psStruct;
		}
	}

	return nullptr;
}


/** This function runs though all player's structures to check if any of then is a repair facility. Returns the structure if any was found, and NULL else.*/
static STRUCTURE *FindARepairFacility(unsigned player)
{
	STRUCTURE *psStruct;

	for (psStruct = apsStructLists[player]; psStruct != nullptr; psStruct = psStruct->psNext)
	{
		if (psStruct->pStructureType->type == REF_REPAIR_FACILITY)
		{
			return psStruct;
		}
	}

	return nullptr;
}


/** This function returns true if the droid supports the secondary order, and false if not.*/
bool secondarySupported(DROID *psDroid, SECONDARY_ORDER sec)
{
	bool supported;

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
		if ((sec == DSO_ASSIGN_PRODUCTION && FindAFactory(psDroid->player, REF_FACTORY) == nullptr) ||
		    (sec == DSO_ASSIGN_CYBORG_PRODUCTION && FindAFactory(psDroid->player, REF_CYBORG_FACTORY) == nullptr) ||
		    (sec == DSO_ASSIGN_VTOL_PRODUCTION && FindAFactory(psDroid->player, REF_VTOL_FACTORY) == nullptr))
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
	case DSO_RETURN_TO_LOC:
		break;

	case DSO_RECYCLE:			// Only if player has got a factory.
		if ((FindAFactory(psDroid->player, REF_FACTORY) == nullptr) &&
		    (FindAFactory(psDroid->player, REF_CYBORG_FACTORY) == nullptr) &&
		    (FindAFactory(psDroid->player, REF_VTOL_FACTORY) == nullptr) &&
		    (FindARepairFacility(psDroid->player) == nullptr))
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


/** This function returns the droid order's secondary state of the secondary order.*/
SECONDARY_STATE secondaryGetState(DROID *psDroid, SECONDARY_ORDER sec, QUEUE_MODE mode)
{
	uint32_t state;

	state = psDroid->secondaryOrder;
	if (mode == ModeQueue)
	{
		state = psDroid->secondaryOrderPending;  // The UI wants to know the state, so return what the state will be after orders are synchronised.
	}

	switch (sec)
	{
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
	for (i = 0; i < 5; i++)
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
			aBuff[i * 2 + 5] = 'c';
			aBuff[i * 2 + 6] = (char)('0' + i);
		}
		else
		{
			aBuff[i * 2 + 5] = ' ';
			aBuff[i * 2 + 6] = ' ';
		}
	}

	return aBuff;
}
#else
#define secondaryPrintFactories(x)
#endif


/** This function returns true if the droid needs repair according to the repair state, and in case there are some other droids selected, deselect those that are going to repair.
 * @todo there is some problem related with the values of REPAIRLEV_HIGH and REPAIRLEV_LOW that needs to be fixed.
 */
static bool secondaryCheckDamageLevelDeselect(DROID *psDroid, SECONDARY_STATE repairState)
{
	unsigned repairLevel;
	switch (repairState)
	{
	case DSS_REPLEV_LOW:   repairLevel = REPAIRLEV_HIGH; break;  // LOW → HIGH, seems DSS_REPLEV_LOW and DSS_REPLEV_HIGH are badly named?
	case DSS_REPLEV_HIGH:  repairLevel = REPAIRLEV_LOW;  break;
	default:
	case DSS_REPLEV_NEVER: repairLevel = 0;              break;
	}

	// psDroid->body / psDroid->originalBody < repairLevel / 100, without integer truncation
	if (psDroid->body * 100 <= repairLevel * psDroid->originalBody)
	{
		// Only deselect the droid if there is another droid selected.
		if (psDroid->selected)
		{
			DROID *psTempDroid;
			for (psTempDroid = apsDroidLists[selectedPlayer]; psTempDroid; psTempDroid = psTempDroid->psNext)
			{
				if (psTempDroid != psDroid && psTempDroid->selected)
				{
					DeSelectDroid(psDroid);
					break;
				}
			}
		}
		return true;
	}
	return false;
}


/** This function checks the droid damage level against its secondary state. If the damage level is too high, then it sends an order to the droid to return to repair.*/
void secondaryCheckDamageLevel(DROID *psDroid)
{
	if (secondaryCheckDamageLevelDeselect(psDroid, secondaryGetState(psDroid, DSO_REPAIR_LEVEL)))
	{
		if (!isVtolDroid(psDroid))
		{
			psDroid->group = UBYTE_MAX;
		}

		/* set return to repair if not on hold */
		if (psDroid->order.type != DORDER_RTR &&
		    psDroid->order.type != DORDER_RTB &&
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


/** This function assigns a state to a droid. It returns true if it assigned and false if it failed to assign.*/
bool secondarySetState(DROID *psDroid, SECONDARY_ORDER sec, SECONDARY_STATE State, QUEUE_MODE mode)
{
	UDWORD		CurrState, factType, prodType;
	STRUCTURE	*psStruct;
	SDWORD		factoryInc;
	bool		retVal;
	DROID		*psTransport, *psCurr, *psNext;
	DROID_ORDER     order;

	CurrState = psDroid->secondaryOrder;
	if (bMultiMessages && mode == ModeQueue)
	{
		CurrState = psDroid->secondaryOrderPending;
	}

	// Figure out what the new secondary state will be (once the order is synchronised.
	// Why does this have to be so ridiculously complicated?
	uint32_t secondaryMask = 0;
	uint32_t secondarySet = 0;
	switch (sec)
	{
	case DSO_REPAIR_LEVEL:
		secondaryMask = DSS_REPLEV_MASK;
		secondarySet = State;
		break;
	case DSO_ATTACK_LEVEL:
		secondaryMask = DSS_ALEV_MASK;
		secondarySet = State;
		break;
	case DSO_ASSIGN_PRODUCTION:
		if (psDroid->droidType == DROID_COMMAND)
		{
			secondaryMask = DSS_ASSPROD_FACT_MASK;
			secondarySet = State & DSS_ASSPROD_MASK;
		}
		break;
	case DSO_ASSIGN_CYBORG_PRODUCTION:
		if (psDroid->droidType == DROID_COMMAND)
		{
			secondaryMask = DSS_ASSPROD_CYB_MASK;
			secondarySet = State & DSS_ASSPROD_MASK;
		}
		break;
	case DSO_ASSIGN_VTOL_PRODUCTION:
		if (psDroid->droidType == DROID_COMMAND)
		{
			secondaryMask = DSS_ASSPROD_VTOL_MASK;
			secondarySet = State & DSS_ASSPROD_MASK;
		}
		break;
	case DSO_CLEAR_PRODUCTION:
		if (psDroid->droidType == DROID_COMMAND)
		{
			secondaryMask = State & DSS_ASSPROD_MASK;
		} break;
	case DSO_RECYCLE:
		if (State & DSS_RECYCLE_MASK)
		{
			secondaryMask = DSS_RTL_MASK | DSS_RECYCLE_MASK;
			secondarySet = DSS_RECYCLE_SET;
		}
		else
		{
			secondaryMask = DSS_RECYCLE_MASK;
		}
		break;
	case DSO_CIRCLE:  // This doesn't even make any sense whatsoever as a secondary order...
		secondaryMask = DSS_CIRCLE_MASK;
		secondarySet = (State & DSS_CIRCLE_SET) ? DSS_CIRCLE_SET : 0;
		break;
	case DSO_PATROL:  // This doesn't even make any sense whatsoever as a secondary order...
		secondaryMask = DSS_PATROL_MASK;
		secondarySet = (State & DSS_PATROL_SET) ? DSS_PATROL_SET : 0;
		break;
	case DSO_RETURN_TO_LOC:
		secondaryMask = DSS_RTL_MASK;
		switch (State & DSS_RTL_MASK)
		{
		case DSS_RTL_REPAIR:
		case DSS_RTL_BASE:
			secondarySet = State;
			break;
		case DSS_RTL_TRANSPORT:
			psTransport = FindATransporter(psDroid);
			if (psTransport != nullptr)
			{
				secondarySet = State;
			}
			break;
		}
		break;
	case DSO_UNUSED:
	case DSO_UNUSED2:
	case DSO_FIRE_DESIGNATOR:
		// Do nothing.
		break;
	}
	uint32_t newSecondaryState = (CurrState & ~secondaryMask) | secondarySet;

	if (bMultiMessages && mode == ModeQueue)
	{
		if (sec == DSO_REPAIR_LEVEL)
		{
			secondaryCheckDamageLevelDeselect(psDroid, State);  // Deselect droid immediately, if applicable, so it isn't ordered around by mistake.
		}

		sendDroidSecondary(psDroid, sec, State);
		psDroid->secondaryOrderPending = newSecondaryState;
		++psDroid->secondaryOrderPendingCount;
		return true;  // Wait for our order before changing the droid.
	}


	// set the state for any droids in the command group
	if ((sec != DSO_RECYCLE) &&
	    psDroid->droidType == DROID_COMMAND &&
	    psDroid->psGroup != nullptr &&
	    psDroid->psGroup->type == GT_COMMAND)
	{
		psDroid->psGroup->setSecondary(sec, State);
	}

	retVal = true;
	switch (sec)
	{
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
			else if (orderState(psDroid, DORDER_GUARD) &&
			         droidAttacking(psDroid))
			{
				// send the unit back to the guard position
				actionDroid(psDroid, DACTION_NONE);
			}
			else if (orderState(psDroid, DORDER_PATROL))
			{
				// send the unit back to the patrol
				actionDroid(psDroid, DACTION_RETURNTOPOS, psDroid->actionPos.x, psDroid->actionPos.y);
			}
		}
		break;


	case DSO_ASSIGN_PRODUCTION:
	case DSO_ASSIGN_CYBORG_PRODUCTION:
	case DSO_ASSIGN_VTOL_PRODUCTION:
#ifdef DEBUG
		debug(LOG_NEVER, "order factories %s\n", secondaryPrintFactories(State));
#endif
		if (sec == DSO_ASSIGN_PRODUCTION)
		{
			prodType = REF_FACTORY;
		}
		else if (sec == DSO_ASSIGN_CYBORG_PRODUCTION)
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
			     psStruct = psStruct->psNext)
			{
				factType = psStruct->pStructureType->type;
				if (factType == REF_FACTORY ||
				    factType == REF_VTOL_FACTORY ||
				    factType == REF_CYBORG_FACTORY)
				{
					factoryInc = ((FACTORY *)psStruct->pFunctionality)->psAssemblyPoint->factoryInc;
					if (factType == REF_FACTORY)
					{
						factoryInc += DSS_ASSPROD_SHIFT;
					}
					else if (factType == REF_CYBORG_FACTORY)
					{
						factoryInc += DSS_ASSPROD_CYBORG_SHIFT;
					}
					else
					{
						factoryInc += DSS_ASSPROD_VTOL_SHIFT;
					}
					if (!(CurrState & (1 << factoryInc)) &&
					    (State & (1 << factoryInc)))
					{
						assignFactoryCommandDroid(psStruct, psDroid);// assign this factory to the command droid
					}
					else if ((prodType == factType) &&
					         (CurrState & (1 << factoryInc)) &&
					         !(State & (1 << factoryInc)))
					{
						// remove this factory from the command droid
						assignFactoryCommandDroid(psStruct, nullptr);
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
			debug(LOG_NEVER, "final factories %s\n", secondaryPrintFactories(CurrState));
#endif
		}
		break;

	case DSO_CLEAR_PRODUCTION:
		if (psDroid->droidType == DROID_COMMAND)
		{
			// simply clear the flag - all the factory stuff is done in assignFactoryCommandDroid
			CurrState &= ~(State & DSS_ASSPROD_MASK);
		}
		break;


	case DSO_RECYCLE:
		if (State & DSS_RECYCLE_MASK)
		{
			if (!orderState(psDroid, DORDER_RECYCLE))
			{
				orderDroid(psDroid, DORDER_RECYCLE, ModeImmediate);
			}
			CurrState &= ~(DSS_RTL_MASK | DSS_RECYCLE_MASK);
			CurrState |= DSS_RECYCLE_SET;
			psDroid->group = UBYTE_MAX;
			if (psDroid->psGroup != nullptr)
			{
				if (psDroid->droidType == DROID_COMMAND)
				{
					// remove all the units from the commanders group
					for (psCurr = psDroid->psGroup->psList; psCurr; psCurr = psNext)
					{
						psNext = psCurr->psGrpNext;
						psCurr->psGroup->remove(psCurr);
						orderDroid(psCurr, DORDER_STOP, ModeImmediate);
					}
				}
				else if (psDroid->psGroup->type == GT_COMMAND)
				{
					psDroid->psGroup->remove(psDroid);
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
				psTransport = FindATransporter(psDroid);
				if (psTransport != nullptr)
				{
					order = DORDER_EMBARK;
					CurrState |= DSS_RTL_TRANSPORT;
					if (!orderState(psDroid, DORDER_EMBARK))
					{
						orderDroidObj(psDroid, DORDER_EMBARK, psTransport, ModeImmediate);
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

	if (CurrState != newSecondaryState)
	{
		debug(LOG_WARNING, "Guessed the new secondary state incorrectly, expected 0x%08X, got 0x%08X, was 0x%08X, sec = %d, state = 0x%08X.", newSecondaryState, CurrState, psDroid->secondaryOrder, sec, State);
	}
	psDroid->secondaryOrder = CurrState;
	psDroid->secondaryOrderPendingCount = std::max(psDroid->secondaryOrderPendingCount - 1, 0);
	if (psDroid->secondaryOrderPendingCount == 0)
	{
		psDroid->secondaryOrderPending = psDroid->secondaryOrder;  // If no orders are pending, make sure UI uses the actual state.
	}

	return retVal;
}


/** This function returns false and resets the droid's secondary order.
 * @todo this function seems strange (always return false?) and is only used once on the entire code. Consider refactoring it to something more tangible.
 */
bool secondaryGotPrimaryOrder(DROID *psDroid, DROID_ORDER order)
{
	UDWORD	oldState;

	if (isTransporter(psDroid))
	{
		return false;
	}

	if (order != DORDER_NONE &&
	    order != DORDER_STOP &&
	    order != DORDER_GUARD)
	{
		//reset 2ndary order
		oldState = psDroid->secondaryOrder;
		psDroid->secondaryOrder &= ~(DSS_RTL_MASK | DSS_RECYCLE_MASK | DSS_PATROL_MASK);
		psDroid->secondaryOrderPending &= ~(DSS_RTL_MASK | DSS_RECYCLE_MASK | DSS_PATROL_MASK);

		if ((oldState != psDroid->secondaryOrder) &&
		    (psDroid->player == selectedPlayer))
		{
			intRefreshScreen();
		}
	}

	return false;
}


/** This function assigns all droids of the group to the state.
 * @todo this function runs through all the player's droids. Consider something more efficient to select a group.
 * @todo SECONDARY_STATE argument is called "state", which is not current style. Suggestion to change it to "pState".
 */
static void secondarySetGroupState(UDWORD player, UDWORD group, SECONDARY_ORDER sec, SECONDARY_STATE state)
{
	DROID	*psCurr;

	for (psCurr = apsDroidLists[player]; psCurr; psCurr = psCurr->psNext)
	{
		if (psCurr->group == group &&
		    secondaryGetState(psCurr, sec) != state)
		{
			secondarySetState(psCurr, sec, state);
		}
	}
}


/** This function returns the average secondary state of a numerical group of a player.
 * @todo this function runs through all the player's droids. Consider something more efficient to select a group.
 * @todo this function uses a "local" define. Consider removing it, refactoring this function.
 */
static SECONDARY_STATE secondaryGetAverageGroupState(UDWORD player, UDWORD group, UDWORD mask)
{
#define MAX_STATES		5
	struct
	{
		UDWORD state, num;
	} aStateCount[MAX_STATES];
	SDWORD	i, numStates, max;
	DROID	*psCurr;

	// count the number of units for each state
	numStates = 0;
	memset(aStateCount, 0, sizeof(aStateCount));
	for (psCurr = apsDroidLists[player]; psCurr; psCurr = psCurr->psNext)
	{
		if (psCurr->group == group)
		{
			for (i = 0; i < numStates; i++)
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
	for (i = 0; i < numStates; i++)
	{
		if (aStateCount[i].num > aStateCount[max].num)
		{
			max = i;
		}
	}

	return (SECONDARY_STATE)aStateCount[max].state;
}


/** This function sets all the group's members to have the same secondary state as the average secondary state of the group.
 * @todo this function runs through all the player's droids. Consider something more efficient to select a group.
 * @todo this function uses a "local" define. Consider removing it, refactoring this function.
 */
void secondarySetAverageGroupState(UDWORD player, UDWORD group)
{
	// lookup table for orders and masks
#define MAX_ORDERS	2
	struct
	{
		SECONDARY_ORDER order;
		UDWORD mask;
	} aOrders[MAX_ORDERS] =
	{
		{ DSO_REPAIR_LEVEL, DSS_REPLEV_MASK },
		{ DSO_ATTACK_LEVEL, DSS_ALEV_MASK },
	};
	SDWORD	i, state;

	for (i = 0; i < MAX_ORDERS; i++)
	{
		state = secondaryGetAverageGroupState(player, group, aOrders[i].mask);
		secondarySetGroupState(player, group, aOrders[i].order, (SECONDARY_STATE)state);
	}
}


/** This function runs through all player's droids and check if their moral is below the minimum moral to flee. If it is, then it uses the orderDroid() to send the order DORDER_RUN to the droid.*/
void orderMoralCheck(UDWORD player)
{
	DROID	*psCurr;
	SDWORD	units, numVehicles, leadership, personLShip, check;

	// count the number of vehicles and units on the side
	units = 0;
	numVehicles = 0;
	for (psCurr = apsDroidLists[player]; psCurr; psCurr = psCurr->psNext)
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
	for (psCurr = apsDroidLists[player]; psCurr; psCurr = psCurr->psNext)
	{
		if (orderState(psCurr, DORDER_RUN) ||
		    orderState(psCurr, DORDER_RETREAT) ||
		    orderState(psCurr, DORDER_RTB) ||
		    orderState(psCurr, DORDER_RTR))
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


/** This function runs through all group's droids and check if their moral is below the minimum moral to flee. If it is, then the order DORDER_RUN is set to that droid.
 * @todo this fuction is near a duplicate of orderMoralCheck(). A better approach to erase this one should be tried.
 */
void orderGroupMoralCheck(DROID_GROUP *psGroup)
{
	DROID		*psCurr;
	SDWORD		units, numVehicles, leadership, personLShip, check;
	RUN_DATA	*psRunData;

	// count the number of vehicles and units on the side
	units = 0;
	numVehicles = 0;
	for (psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
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
	for (psCurr = psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
	{
		if (orderState(psCurr, DORDER_RUN) ||
		    orderState(psCurr, DORDER_RETREAT) ||
		    orderState(psCurr, DORDER_RTB) ||
		    orderState(psCurr, DORDER_RTR))
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


/** This function checks if the droid's health level is below a minimum health to flee.
 * If it is, then it uses the orderDroid() to send the order DORDER_RUN to the droid.
 */
void orderHealthCheck(DROID *psDroid)
{
	DROID		*psCurr;
	SBYTE       healthLevel = 0;
	UDWORD      retreatX = 0, retreatY = 0;

	if (isTransporter(psDroid))
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
		      orderState(psDroid, DORDER_RETREAT) ||
		      orderState(psDroid, DORDER_RTB) ||
		      orderState(psDroid, DORDER_RTR)))
		{
			syncDebug("Droid running.");
			orderDroidLoc(psDroid, DORDER_RUN, retreatX, retreatY, ModeImmediate);
		}

		// order each unit in the same group to run
		if (psDroid->psGroup)
		{
			for (psCurr = psDroid->psGroup->psList; psCurr; psCurr = psCurr->psGrpNext)
			{
				if (orderState(psCurr, DORDER_RUN) || orderState(psCurr, DORDER_RETREAT)
				    || orderState(psCurr, DORDER_RTB) || orderState(psCurr, DORDER_RTR))
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


/** This function changes the structure's secondary state to be the function input's state.
 * Returns true if the function changed the structure's state, and false if it did not.
 * @todo SECONDARY_STATE argument is called "State", which is not current style. Suggestion to change it to "pState".
 */
bool setFactoryState(STRUCTURE *psStruct, SECONDARY_ORDER sec, SECONDARY_STATE State)
{
	UDWORD		CurrState;
	bool		retVal;
	FACTORY     *psFactory;


	if (!StructIsFactory(psStruct))
	{
		ASSERT(false, "setFactoryState: structure is not a factory");
		return false;
	}

	psFactory = (FACTORY *)psStruct->pFunctionality;

	CurrState = psFactory->secondaryOrder;

	retVal = true;
	switch (sec)
	{
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
	default:
		break;
	}

	psFactory->secondaryOrder = CurrState;

	return retVal;
}


/** This function sets the structure's secondary state to be pState.
 *  return true except on an ASSERT (which is not a good design.)
 *  or, an invalid factory.
 */
bool getFactoryState(STRUCTURE *psStruct, SECONDARY_ORDER sec, SECONDARY_STATE *pState)
{
	UDWORD	state;

	ASSERT_OR_RETURN(false, StructIsFactory(psStruct), "Structure is not a factory");
	if ((FACTORY *)psStruct->pFunctionality)
	{
		state = ((FACTORY *)psStruct->pFunctionality)->secondaryOrder;

		switch (sec)
		{
		case DSO_REPAIR_LEVEL:
			*pState = (SECONDARY_STATE)(state & DSS_REPLEV_MASK);
			break;
		case DSO_ATTACK_LEVEL:
			*pState = (SECONDARY_STATE)(state & DSS_ALEV_MASK);
			break;
		case DSO_PATROL:
			*pState = (SECONDARY_STATE)(state & DSS_PATROL_MASK);
			break;
		default:
			*pState = (SECONDARY_STATE)0;
			break;
		}

		return true;
	}
	return false;
}


/** lasSat structure can select a target
 * @todo improve documentation: it is not clear what this function performs by the current documentation.
 */
void orderStructureObj(UDWORD player, BASE_OBJECT *psObj)
{
	STRUCTURE   *psStruct;

	for (psStruct = apsStructLists[player]; psStruct; psStruct = psStruct->psNext)
	{
		if (lasSatStructSelected(psStruct))
		{
			// send the weapon fire
			sendLasSat(player, psStruct, psObj);

			break;
		}
	}
}


/** This function maps the order enum to its name, returning its enum name as a (const char*)
 * Formally, this function is equivalent to a stl map: for a given key (enum), returns a mapped value (char*).
 */
const char *getDroidOrderName(DROID_ORDER order)
{
	switch (order)
	{
	case DORDER_NONE:                     return "DORDER_NONE";
	case DORDER_STOP:                     return "DORDER_STOP";
	case DORDER_MOVE:                     return "DORDER_MOVE";
	case DORDER_ATTACK:                   return "DORDER_ATTACK";
	case DORDER_BUILD:                    return "DORDER_BUILD";
	case DORDER_HELPBUILD:                return "DORDER_HELPBUILD";
	case DORDER_LINEBUILD:                return "DORDER_LINEBUILD";
	case DORDER_DEMOLISH:                 return "DORDER_DEMOLISH";
	case DORDER_REPAIR:                   return "DORDER_REPAIR";
	case DORDER_OBSERVE:                  return "DORDER_OBSERVE";
	case DORDER_FIRESUPPORT:              return "DORDER_FIRESUPPORT";
	case DORDER_RETREAT:                  return "DORDER_RETREAT";
	case DORDER_UNUSED_2:                 return "DORDER_UNUSED_2";
	case DORDER_RTB:                      return "DORDER_RTB";
	case DORDER_RTR:                      return "DORDER_RTR";
	case DORDER_RUN:                      return "DORDER_RUN";
	case DORDER_EMBARK:                   return "DORDER_EMBARK";
	case DORDER_DISEMBARK:                return "DORDER_DISEMBARK";
	case DORDER_ATTACKTARGET:             return "DORDER_ATTACKTARGET";
	case DORDER_COMMANDERSUPPORT:         return "DORDER_COMMANDERSUPPORT";
	case DORDER_BUILDMODULE:              return "DORDER_BUILDMODULE";
	case DORDER_RECYCLE:                  return "DORDER_RECYCLE";
	case DORDER_TRANSPORTOUT:             return "DORDER_TRANSPORTOUT";
	case DORDER_TRANSPORTIN:              return "DORDER_TRANSPORTIN";
	case DORDER_TRANSPORTRETURN:          return "DORDER_TRANSPORTRETURN";
	case DORDER_GUARD:                    return "DORDER_GUARD";
	case DORDER_DROIDREPAIR:              return "DORDER_DROIDREPAIR";
	case DORDER_RESTORE:                  return "DORDER_RESTORE";
	case DORDER_SCOUT:                    return "DORDER_SCOUT";
	case DORDER_UNUSED_3:                 return "DORDER_UNUSED_3";
	case DORDER_UNUSED:                   return "DORDER_UNUSED";
	case DORDER_PATROL:                   return "DORDER_PATROL";
	case DORDER_REARM:                    return "DORDER_REARM";
	case DORDER_RECOVER:                  return "DORDER_RECOVER";
	case DORDER_LEAVEMAP:                 return "DORDER_LEAVEMAP";
	case DORDER_RTR_SPECIFIED:            return "DORDER_RTR_SPECIFIED";
	case DORDER_CIRCLE:                   return "DORDER_CIRCLE";
	case DORDER_HOLD:                     return "DORDER_HOLD";
	};

	ASSERT(false, "DROID_ORDER out of range: %u", order);

	return "DORDER_#INVALID#";
}
