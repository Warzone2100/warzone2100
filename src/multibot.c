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
 * Multibot.c
 *
 * Alex Lee , 97/98 Pumpkin Studios, Bath
 * Multiplay stuff relevant to droids only.
 */
#include "lib/framework/frame.h"

#include "droid.h"						// for droid sending and ordering.
#include "droiddef.h"
#include "basedef.h"						// for sending WHOLE droids.
#include "stats.h"
#include "move.h"						// for ordering droids
#include "objmem.h"
#include "power.h"						// for powercalculated
#include "order.h"
#include "geometry.h"					// for formations.
#include "map.h"
#include "group.h"
#include "formation.h"
#include "lib/netplay/netplay.h"					// the netplay library.
#include "multiplay.h"					// warzone net stuff.
#include "multijoin.h"
#include "cmddroid.h"					// command droids
#include "action.h"
#include "console.h"
#include "mapgrid.h"

#define ANYPLAYER	99
#define UNKNOWN		99

// ////////////////////////////////////////////////////////////////////////////
// External Stuff.
extern DROID_ORDER chooseOrderLoc(DROID *psDroid, UDWORD x,UDWORD y);
extern DROID_ORDER chooseOrderObj(DROID *psDroid, BASE_OBJECT *psObj);

// ////////////////////////////////////////////////////////////////////////////
// Local Prototypes

static BOOL sendRequestDroid(UDWORD droidId);
static void ProcessDroidOrder(DROID *psDroid, DROID_ORDER order, UDWORD x, UDWORD y, OBJECT_TYPE desttype, UDWORD destid);

// ////////////////////////////////////////////////////////////////////////////
// Command Droids.

// sod em.


// ////////////////////////////////////////////////////////////////////////////
// vtol bits.
// happy vtol = vtol ready to go back to attack.
BOOL sendHappyVtol(const DROID* psDroid)
{
	if (!myResponsibility(psDroid->player))
	{
		return FALSE;
	}

	NETbeginEncode(NET_VTOL, NET_ALL_PLAYERS);
	{
		uint8_t player = psDroid->player;
		uint32_t droid = psDroid->id;

		NETuint8_t(&player);
		NETuint32_t(&droid);
	}
	return NETend();
}

BOOL recvHappyVtol()
{
	DROID* pD;
	unsigned int i;

	NETbeginDecode();
	{
		uint8_t player;
		uint32_t droid;

		NETuint8_t(&player);
		NETuint32_t(&droid);

		if (!IdToDroid(droid, player, &pD))
		{
			NETend();
			return FALSE;
		}
	}
	NETend();

	// Rearming also repairs VTOLs
	pD->body = pD->originalBody;

	for (i = 0; i < pD->numWeaps; i++)
	{
		pD->sMove.iAttackRuns[i] = 0; // finish it for next time round.
		pD->asWeaps[i].ammo = asWeaponStats[pD->asWeaps[i].nStat].numRounds;
		pD->asWeaps[i].lastFired = 0;
	}

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// Secondary Orders.

// Send
BOOL sendDroidSecondary(const DROID* psDroid, SECONDARY_ORDER sec, SECONDARY_STATE state)
{
	NETbeginEncode(NET_SECONDARY, NET_ALL_PLAYERS);
	{
		uint8_t player = psDroid->player;
		uint32_t droid = psDroid->id;

		NETuint8_t(&player);
		NETuint32_t(&droid);
		NETenum(&sec);
		NETenum(&state);
	}
	return NETend();
}

// recv
BOOL recvDroidSecondary()
{
	DROID*          psDroid;
	SECONDARY_ORDER sec;
	SECONDARY_STATE	state;

	NETbeginDecode();
	{
		uint8_t player;
		uint32_t droid;

		NETuint8_t(&player);
		NETuint32_t(&droid);
		NETenum(&sec);
		NETenum(&state);

		// If we can not find the droid should we not ask for it?
		if (!IdToDroid(droid, player, &psDroid))
		{
			NETend();
			return FALSE;
		}
	}
	NETend();

	// Set the droids secondary order
	turnOffMultiMsg(TRUE);
	secondarySetState(psDroid, sec, state);
	turnOffMultiMsg(FALSE);

	return TRUE;
}

BOOL sendDroidSecondaryAll(const DROID* psDroid)
{
	NETbeginEncode(NET_SECONDARY_ALL, NET_ALL_PLAYERS);
	{
		uint8_t player = psDroid->player;
		uint32_t droid = psDroid->id;
		uint32_t secOrder = psDroid->secondaryOrder;

		NETuint8_t(&player);
		NETuint32_t(&droid);
		NETuint32_t(&secOrder);
	}
	return NETend();
}

BOOL recvDroidSecondaryAll()
{
	DROID* psDroid;

	NETbeginDecode();
	{
		uint8_t player;
		uint32_t droid, secOrder;

		NETuint8_t(&player);
		NETuint32_t(&droid);
		NETuint32_t(&secOrder);

		if (!IdToDroid(droid, player, &psDroid))
		{
			NETend();
			return FALSE;
		}

		if (psDroid != NULL)
		{
			psDroid->secondaryOrder = secOrder;
		}
	}
	NETend();

	return TRUE;
}

BOOL sendDroidEmbark(const DROID* psDroid)
{
	NETbeginEncode(NET_DROIDEMBARK, NET_ALL_PLAYERS);
	{
		uint8_t player = psDroid->player;
		uint32_t droid = psDroid->id;

		NETuint8_t(&player);
		NETuint32_t(&droid);
	}
	return NETend();
}

BOOL recvDroidEmbark()
{
	DROID* psDroid;

	NETbeginDecode();
	{
		uint8_t player;
		uint32_t droid;

		NETuint8_t(&player);
		NETuint32_t(&droid);

		if (!IdToDroid(droid, player, &psDroid))
		{
			NETend();
			return FALSE;
		}
	}
	NETend();

	if (psDroid == NULL)
	{
		return TRUE;
	}

	// Take it out of the world without destroying it
	droidRemove(psDroid, apsDroidLists);

	// Init the order for when disembark
	psDroid->order = DORDER_NONE;
	setDroidTarget(psDroid, NULL);
	psDroid->psTarStats = NULL;

	return TRUE;
}

BOOL sendDroidDisEmbark(const DROID* psDroid)
{
	NETbeginEncode(NET_DROIDDISEMBARK, NET_ALL_PLAYERS);
	{
		uint8_t player = psDroid->player;
		uint32_t droid = psDroid->id;
		Vector3uw pos = psDroid->pos;

		NETuint8_t(&player);
		NETuint32_t(&droid);
		NETVector3uw(&pos);
	}
	return NETend();
}

BOOL recvDroidDisEmbark()
{
	DROID* psDroid;

	NETbeginDecode();
	{
		uint8_t player;
		uint32_t droid;
		Vector3uw pos;

		NETuint8_t(&player);
		NETuint32_t(&droid);
		NETVector3uw(&pos);

		NETend();

		if (!IdToDroid(droid, player, &psDroid))
		{
			return FALSE;
		}

		if (psDroid == NULL)
		{
			return TRUE;
		}

		// Add it back into the world at the x/y
		psDroid->pos = pos;
	}

	if (!worldOnMap(psDroid->pos.x, psDroid->pos.y))
	{
		debug(LOG_ERROR, "recvDroidDisEmbark: droid not disembarked on map");
		return FALSE;
	}

	updateDroidOrientation(psDroid);

	// Initialise the movement data
	initDroidMovement(psDroid);

	// Reset droid orders
	orderDroid(psDroid, DORDER_STOP);
	gridAddObject((BASE_OBJECT *)psDroid);
	psDroid->cluster = 0;

	addDroid(psDroid, apsDroidLists);

	return TRUE;
}


// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Droids

// posibly Send an updated droid movement order.
BOOL SendDroidMove(const DROID* psDroid, uint32_t x, uint32_t y, BOOL formation)
{
	// Don't allow a move to happen at all if it is not our responsibility
	if (!myResponsibility(psDroid->player))
	{
		return FALSE; // Do not allow move
	}

	// If the unit has no actions or orders, allow it to happen but do not send
	if (psDroid->action == DACTION_NONE || psDroid->order == DORDER_MOVE)
	{
		return TRUE;
	}

	NETbeginEncode(NET_DROIDMOVE, NET_ALL_PLAYERS);
	{
		uint8_t player = psDroid->player;
		uint32_t droid = psDroid->id;

		NETuint8_t(&player);
		NETuint32_t(&droid);
		NETuint32_t(&x);
		NETuint32_t(&y);
		NETbool(&formation);
	}
	return NETend();
}

// recv and updated droid position
BOOL recvDroidMove()
{
	DROID* psDroid;
	uint32_t x, y;
	BOOL formation;

	NETbeginDecode();
	{
		uint8_t player;
		uint32_t droid;

		NETuint8_t(&player);
		NETuint32_t(&droid);
		NETuint32_t(&x);
		NETuint32_t(&y);
		NETbool(&formation);

		NETend();

		/*
		 * If we could not find the droid, request it. We can safely return here
		 * as when the droid is sent it will contain the updated movement position.
		 */
		if (!IdToDroid(droid, player, &psDroid))
		{
			sendRequestDroid(droid);
			return TRUE;
		}
	}

	turnOffMultiMsg(TRUE);
	if (formation)
	{
		moveDroidTo(psDroid, x, y); // Do the move
	}
	else
	{
		moveDroidToNoFormation(psDroid, x, y); // Move, no form...
	}
	turnOffMultiMsg(FALSE);

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// Send a new Droid to the other players
BOOL SendDroid(const DROID_TEMPLATE* pTemplate, uint32_t x, uint32_t y, uint8_t player, uint32_t id)
{
	// Dont send other droids during campaign setup
	if (ingame.localJoiningInProgress)
	{
		return TRUE;
	}

	// Only send the droid if we are responsible
	if (!myResponsibility(player))
	{
		// Don't build if we are not responsible
		return FALSE;
	}

	NETbeginEncode(NET_DROID, NET_ALL_PLAYERS);
	{
		Vector3uw pos = { x, y, 0 };
		uint32_t templateID = pTemplate->multiPlayerID;

		NETuint8_t(&player);
		NETuint32_t(&id);
		NETVector3uw(&pos);
		NETuint32_t(&templateID);
		NETbool(&powerCalculated);
	}
	return NETend();
}

// ////////////////////////////////////////////////////////////////////////////
// receive droid creation information from other players
BOOL recvDroid()
{
	DROID_TEMPLATE* pT;
	DROID* psDroid;
    uint8_t player;
    uint32_t id;
    Vector3uw pos;
    BOOL power;

	NETbeginDecode();
	{
		uint32_t templateID;

		NETuint8_t(&player);
		NETuint32_t(&id);
		NETVector3uw(&pos);
		NETuint32_t(&templateID);
		NETbool(&power);

		pT = IdToTemplate(templateID, player);
	}
	NETend();

	// If we can not find the template ask for the entire droid instead
	if (!pT)
	{
		debug(LOG_NET, "Couldn't find template to build recvd droid");
		sendRequestDroid(id);
		return FALSE;
	}

	// If the power to build the droid has been calculated
	if (power
	// Use the power required to build the droid
	 && !usePower(player, pT->powerPoints))
	{
		debug(LOG_NET, "Not enough power to build recvd droid, player = %hhu", player);
		// Build anyway..
	}

	// Create that droid on this machine.
	turnOffMultiMsg(TRUE);
	psDroid = buildDroid(pT, pos.x, pos.y, player, FALSE);
	turnOffMultiMsg(FALSE);

	// If we were able to build the droid set it up
	if (psDroid)
	{
		psDroid->id = id;
		addDroid(psDroid, apsDroidLists);
	}
	else
	{
		DBCONPRINTF(ConsoleString, (ConsoleString, "MULTIPLAYER: Couldn't build a remote droid, relying on checking to resync"));
		return FALSE;
	}

	return TRUE;
}


/*!
 * Type of the target of the movement
 */
typedef enum {
	NET_ORDER_SUBTYPE_POSITION,
	NET_ORDER_SUBTYPE_OBJECT,
	NET_ORDER_SUBTYPE_SPECIAL // x and y are 0, no idea what that means
} NET_ORDER_SUBTYPE;


// ////////////////////////////////////////////////////////////////////////////
/*!
 * Droid Group/selection orders.
 * Minimises comms by sending orders for whole groups, rather than each droid
 */
BOOL SendGroupOrderSelected(uint8_t player, uint32_t x, uint32_t y, const BASE_OBJECT* psObj)
{
	NETbeginEncode(NET_GROUPORDER, NET_ALL_PLAYERS);
	{
		DROID_ORDER order = UNKNOWN;
		BOOL subType = (psObj) ? TRUE : FALSE, cmdOrder = FALSE;
		DROID* psDroid;
		uint8_t droidCount;

		NETenum(&order);
		NETbool(&cmdOrder);
		NETbool(&subType);

		// If they are being ordered to `goto' an object
		if (subType)
		{
			uint32_t id = psObj->id;
			uint32_t type = psObj->type;

			NETuint32_t(&id);
			NETenum(&type);
		}
		// Else if the droids are being ordered to `goto' a specific position
		else
		{
			NETuint32_t(&x);
			NETuint32_t(&y);
		}

		// Work out the number of droids to send
		for (psDroid = apsDroidLists[player], droidCount = 0; psDroid; psDroid = psDroid->psNext)
		{
			if (psDroid->selected)
				++droidCount;
		}

		// If there are less than 2 droids don't bother (to allow individual orders)
		if (droidCount < 2)
		{
			return FALSE;
		}

		// Add the number of droids to the message
		NETuint8_t(&droidCount);

		// Add the droids to the message
		for (psDroid = apsDroidLists[player]; psDroid && droidCount; psDroid = psDroid->psNext)
		{
			if (psDroid->selected)
			{
				NETuint32_t(&psDroid->id);
				--droidCount;
			}
		}
	}
	return NETend();
}

BOOL SendGroupOrderGroup(const DROID_GROUP* psGroup, DROID_ORDER order, uint32_t x, uint32_t y, const BASE_OBJECT* psObj)
{
	NETbeginEncode(NET_GROUPORDER, NET_ALL_PLAYERS);
	{
		BOOL subType = (psObj) ? TRUE : FALSE, cmdOrder = FALSE;
		DROID* psDroid;
		uint8_t droidCount;

		NETenum(&order);
		NETbool(&cmdOrder);
		NETbool(&subType);

		// If they are being ordered to `goto' an object
		if (subType)
		{
			uint32_t id = psObj->id;
			uint32_t type = psObj->type;

			NETuint32_t(&id);
			NETenum(&type);
		}
		// Else if the droids are being ordered to `goto' a specific position
		else
		{
			NETuint32_t(&x);
			NETuint32_t(&y);
		}

		// Work out the number of droids to send
		for (psDroid = psGroup->psList, droidCount = 0; psDroid; psDroid = psDroid->psGrpNext)
		{
			++droidCount;
		}

		// Add the number of droids to the message
		NETuint8_t(&droidCount);

		// Add the droids to the message
		for (psDroid = psGroup->psList; psDroid; psDroid = psDroid->psGrpNext)
		{
			NETuint32_t(&psDroid->id);
		}
	}
	return NETend();
}

// ////////////////////////////////////////////////////////////////////////////
// receive a group order.
BOOL recvGroupOrder()
{
	NETbeginDecode();
	{
		DROID_ORDER order;
		BOOL subType, cmdOrder;

		uint32_t destId, x, y;
		OBJECT_TYPE destType;

		uint8_t droidCount;

		NETenum(&order);
		NETbool(&cmdOrder);
		NETbool(&subType);

		// If they are being ordered to `goto' an object
		if (subType)
		{
			NETuint32_t(&destId);
			NETenum(&destType);
		}
		// Else if the droids are being ordered to `goto' a specific position
		else
		{
			NETuint32_t(&x);
			NETuint32_t(&y);
		}

		// Get the droid count
		NETuint8_t(&droidCount);

		// Retrieve the droids from the message
		for (; droidCount; --droidCount)
		{
			uint32_t id;
			DROID* psDroid;

			// Retrieve the id number for the current droid
			if (!NETuint32_t(&id))
			{
				// If somehow we fail assume the message got truncated prematurely
				debug(LOG_NET, "recvGroupOrder: error retrieving droid ID number; while there are (supposed to be) still %hhu droids left", droidCount);

				return FALSE;
			}

			if (!IdToDroid(id, ANYPLAYER, &psDroid))
			{
				// If the droid's not found, request it
				sendRequestDroid(id);
				return FALSE;
			}

			/*
			 * If the current order not is a command order and we are not a
			 * commander yet are in the commander group remove us from it.
			 */
			if (!cmdOrder && psDroid->droidType != DROID_COMMAND
			 && psDroid->psGroup != NULL && psDroid->psGroup->type == GT_COMMAND)
			{
				grpLeave(psDroid->psGroup, psDroid);
			}

			// Process the droid's order
			if (subType)
			{
				/* If they are being ordered to `goto' an object then we don't
				 * have any X and Y coordinate.
				 */
				ProcessDroidOrder(psDroid, order, 0, 0, destType, destId);
			}
			else
			{
				/* Otherwise if the droids are being ordered to `goto' a
				 * specific position. Then we don't have any destination info
				 */
				ProcessDroidOrder(psDroid, order, x, y, 0, 0);
			}
		}
	}
	NETend();

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// Droid update information
BOOL SendDroidInfo(const DROID* psDroid, DROID_ORDER order, uint32_t x, uint32_t y, const BASE_OBJECT* psObj)
{
	if (!myResponsibility(psDroid->player))
	{
		return TRUE;
	}

	NETbeginEncode(NET_DROIDINFO, NET_ALL_PLAYERS);
	{
		uint32_t droidId = psDroid->id;
		BOOL subType = (psObj) ? TRUE : FALSE;

		// Send the droid's ID
		NETuint32_t(&droidId);

		// Send the droid's order
		NETenum(&order);
		NETbool(&subType);

		if (subType)
		{
			uint32_t destId = psObj->id;
			uint32_t destType = psObj->type;

			NETuint32_t(&destId);
			NETenum(&destType);
		}
		else
		{
			NETuint32_t(&x);
			NETuint32_t(&y);
		}
	}
	return NETend();
}

// ////////////////////////////////////////////////////////////////////////////
// receive droid information form other players.
BOOL recvDroidInfo()
{
	NETbeginDecode();
	{
		uint32_t    droidId;
		DROID*      psDroid;
		DROID_ORDER order;
		BOOL        subType;

		// Get the droid
		NETuint32_t(&droidId);

		if (!IdToDroid(droidId, ANYPLAYER, &psDroid))
		{
			sendRequestDroid(droidId);
			return FALSE;
		}

		// Get the droid's order
		NETenum(&order);
		NETbool(&subType);

		if (subType)
		{
			uint32_t destId, destType;

			NETuint32_t(&destId);
			NETenum(&destType);

			ProcessDroidOrder(psDroid, order, 0, 0, destType, destId);
		}
		else
		{
			uint32_t x, y;

			NETuint32_t(&x);
			NETuint32_t(&y);

			ProcessDroidOrder(psDroid, order, x, y, 0, 0);
		}
	}
	NETend();

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// process droid order
static void ProcessDroidOrder(DROID *psDroid, DROID_ORDER order, uint32_t x, uint32_t y, OBJECT_TYPE desttype, uint32_t destid)
{
	// Target is a location
	if (destid == 0 && desttype == 0)
	{
		// Don't bother if it is close
		if (abs(psDroid->pos.x - x) < (TILE_UNITS/2)
		 && abs(psDroid->pos.y - y) < (TILE_UNITS/2))
		{
			return;
		}

		// If no specific order was passed work one out based on the location
		if (order == UNKNOWN)
		{
			order = chooseOrderLoc(psDroid, x, y);
		}

		turnOffMultiMsg(TRUE);
		orderDroidLoc(psDroid, order, x, y);
		turnOffMultiMsg(FALSE);
	}
	// Target is an object
	else
	{
		BASE_OBJECT *psObj = NULL;
		DROID		*pD;

		switch (desttype)
		{
			case OBJ_DROID:
				IdToDroid(destid,ANYPLAYER,&pD);
				psObj = (BASE_OBJECT*)pD;
				break;
			case OBJ_STRUCTURE:
				psObj = (BASE_OBJECT*)IdToStruct(destid,ANYPLAYER);
				break;
			case OBJ_FEATURE:
				psObj = (BASE_OBJECT*)IdToFeature(destid,ANYPLAYER);
				break;

			// We should not get this!
			case OBJ_PROJECTILE:
				debug(LOG_ERROR, "ProcessDroidOrder: order specified destination as a bullet. what am i to do??");
				break;
			default:
				debug(LOG_ERROR, "ProcessDroidOrder: unknown object type");
				break;
		}

		// If we did not find anything, return
		if (!psObj)													// failed to find it;
		{
			return;
		}

		// If we didn't sepcify an order, then pick one
		if (order == UNKNOWN)
		{
			order = chooseOrderObj(psDroid, psObj);
		}

		turnOffMultiMsg(TRUE);
		orderDroidObj(psDroid, order, psObj);
		turnOffMultiMsg(FALSE);
	}
}


// ////////////////////////////////////////////////////////////////////////////
// Inform other players that a droid has been destroyed
BOOL SendDestroyDroid(const DROID* psDroid)
{
	NETbeginEncode(NET_DROIDDEST, NET_ALL_PLAYERS);
	{
		uint32_t id = psDroid->id;

		// Send the droid's ID
		NETuint32_t(&id);
	}
	return NETend();
}

// ////////////////////////////////////////////////////////////////////////////
// Accept a droid which was destroyed on another machine
BOOL recvDestroyDroid()
{
	DROID* psDroid;

	NETbeginDecode();
	{
		uint32_t id;

		// Retrieve the droid
		NETuint32_t(&id);
		if (!IdToDroid(id, ANYPLAYER, &psDroid))
		{
			return FALSE;
		}
	}
	NETend();

	// If the droid has not died on our machine yet, destroy it
	if(!psDroid->died)
	{
		turnOffMultiMsg(TRUE);
		destroyDroid(psDroid);
		turnOffMultiMsg(FALSE);
	}

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// stuff for sending the WHOLE of a droid!
BOOL sendWholeDroid(DROID *pD, UDWORD dest)
{
	NETMSG	m;
	UDWORD	sizecount = 0;
	UDWORD	noTarget = 0;
	SDWORD	asParts[DROID_MAXCOMP];
	UDWORD	asWeaps[DROID_MAXWEAPS];
	unsigned int i;
	BOOL bNoTarget = TRUE;
	uint16_t direction = pD->direction * 32; // preserve precision

	if (pD->numWeaps == 0)
	{
		if (pD->asWeaps[0].nStat > 0)									// build some bits for the template.
		{
			   asWeaps[0] = pD->asWeaps[0].nStat;
		}
		else
		{
			asWeaps[0] = 0;
		}
	}

	asParts[COMP_BODY]		=	pD->asBits[COMP_BODY].nStat;		//allocate the components
	asParts[COMP_BRAIN]		=	pD->asBits[COMP_BRAIN].nStat;
	asParts[COMP_PROPULSION]=	pD->asBits[COMP_PROPULSION].nStat;
	asParts[COMP_SENSOR]	=	pD->asBits[COMP_SENSOR].nStat;
	asParts[COMP_ECM]		=	pD->asBits[COMP_ECM].nStat;
	asParts[COMP_REPAIRUNIT]=	pD->asBits[COMP_REPAIRUNIT].nStat;
	asParts[COMP_CONSTRUCT]	=	pD->asBits[COMP_CONSTRUCT].nStat;
	asParts[COMP_WEAPON]	=	pD->asBits[COMP_WEAPON].nStat;
	NetAdd(m,sizecount,asParts);					sizecount+=sizeof(asParts);
	NetAdd(m,sizecount,pD->numWeaps);				sizecount+=sizeof(pD->numWeaps);
	NetAdd(m,sizecount,pD->asWeaps);				sizecount+=sizeof(pD->asWeaps);			// to build a template.

	NetAdd(m,sizecount,pD->pos.x);						sizecount+=sizeof(pD->pos.x);
	NetAdd(m,sizecount,pD->pos.y);						sizecount+=sizeof(pD->pos.y);
	NetAdd(m,sizecount,pD->pos.z);						sizecount+=sizeof(pD->pos.z);
	NetAdd(m,sizecount,pD->player);					sizecount+=sizeof(pD->player);

	NetAddSt(m,sizecount,pD->aName);				sizecount+=strlen(pD->aName)+1;

	// that's enough to build a template, now the specific stuff!
	NetAdd(m,sizecount,pD->id);						sizecount+=sizeof(pD->id);

	NetAdd(m,sizecount,pD->NameVersion);			sizecount+=sizeof(pD->NameVersion);
	NetAdd(m,sizecount,pD->droidType);				sizecount+=sizeof(pD->droidType);

	NetAdd(m,sizecount,direction);				sizecount+=sizeof(direction);
	NetAdd(m,sizecount,pD->pitch);					sizecount+=sizeof(pD->pitch);
	NetAdd(m,sizecount,pD->roll);					sizecount+=sizeof(pD->roll);
	NetAdd(m,sizecount,pD->visible);				sizecount+=sizeof(pD->visible);
	NetAdd(m,sizecount,pD->inFire);					sizecount+=sizeof(pD->inFire);
	NetAdd(m,sizecount,pD->burnDamage);				sizecount+=sizeof(pD->burnDamage);

	NetAdd(m,sizecount,pD->body);					sizecount+=sizeof(pD->body);
	NetAdd(m,sizecount,pD->secondaryOrder);			sizecount+=sizeof(pD->secondaryOrder);
	NetAdd(m,sizecount,pD->order);					sizecount+=sizeof(pD->order);
	NetAdd(m,sizecount,pD->orderX);					sizecount+=sizeof(pD->orderX);
	NetAdd(m,sizecount,pD->orderY);					sizecount+=sizeof(pD->orderY);
	NetAdd(m,sizecount,pD->orderX2);				sizecount+=sizeof(pD->orderX2);
	NetAdd(m,sizecount,pD->orderY2);				sizecount+=sizeof(pD->orderY2);

	for (i = 0; i < pD->numWeaps; i++)
	{
		if (pD->psActionTarget[i])
		{
			NetAdd(m,sizecount,pD->psActionTarget[i]->id);		sizecount+=sizeof(pD->psActionTarget[i]->id);
			bNoTarget = FALSE;
		}
	}

	if (bNoTarget)
	{
		NetAdd(m,sizecount,noTarget);				sizecount+=sizeof(noTarget);
	}

	if (pD->psTarStats)
	{
		NetAdd(m,sizecount,pD->psTarStats->ref);	sizecount+=sizeof(pD->psTarStats->ref);
	}
	else
	{
		NetAdd(m,sizecount,noTarget);				sizecount+=sizeof(noTarget);
	}


	m.type = NET_WHOLEDROID;
	m.size = (UWORD)sizecount;
	return NETsend(&m,dest,FALSE);
}
// ////////////////////////////////////////////////////////////////////////////
// receive a whole droid!!!!
BOOL receiveWholeDroid(NETMSG *m)
{
	UDWORD			sizecount=0;
	DROID_TEMPLATE	dt;
	DROID			*pD,*existingdroid;
	UWORD x,y,z;
	UDWORD id;
	UBYTE player;
	UBYTE i;
	uint16_t direction;

	// get the stuff
	NetGet(m,sizecount,dt.asParts);				sizecount+=sizeof(dt.asParts);		// build a template
	NetGet(m,sizecount,dt.asWeaps);				sizecount+=sizeof(dt.asWeaps);
	NetGet(m,sizecount,dt.numWeaps);			sizecount+=sizeof(dt.numWeaps);		// numWeaps
	NetGet(m,sizecount,x);						sizecount+=sizeof(x);				// edit it.
	NetGet(m,sizecount,y);						sizecount+=sizeof(y);
	NetGet(m,sizecount,z);						sizecount+=sizeof(z);
	NetGet(m,sizecount,player);					sizecount+=sizeof(player);

	dt.pName = (char*)&dt.aName;
	strlcpy(dt.aName, &(m->body[sizecount]), sizeof(dt.aName));
	sizecount+=strlen(dt.pName)+1;		// name is pointed at directly into the buffer.

	if(dt.asWeaps[0] == 0)
	{
		dt.numWeaps =0;
	}

	dt.powerPoints = calcTemplatePower(&dt);

	NetGet(m,sizecount,id);						sizecount+=sizeof(id);

	if(IdToDroid(id,ANYPLAYER,&existingdroid))// if a droid of id already exists then go no further.
	{
		return FALSE;
	}

	// could do usepower , but we usually do this in an emergency, so leave it!
	turnOffMultiMsg(TRUE);
	pD = buildDroid(&dt,x,y,player, FALSE);			// make a droid
	turnOffMultiMsg(FALSE);

	if(!pD)										// failed to build it, give up.
	{
		return FALSE;
	}

	STATIC_ASSERT(sizeof(id) == sizeof(pD->id));

	// now the instance specific stuff.
	pD->id = id;
	pD->pos.x = x;									//correct builddroid to use exact pos, not tile center
	pD->pos.y = y;
	pD->pos.z = z;

	NetGet(m,sizecount,pD->NameVersion);		sizecount+=sizeof(pD->NameVersion);
	NetGet(m,sizecount,pD->droidType);			sizecount+=sizeof(pD->droidType);

	NetGet(m,sizecount, direction);			sizecount+=sizeof(direction);
	pD->direction = (float)direction / 32;
	NetGet(m,sizecount,pD->pitch);				sizecount+=sizeof(pD->pitch);
	NetGet(m,sizecount,pD->roll);				sizecount+=sizeof(pD->roll);

	NetGet(m,sizecount,pD->visible);			sizecount+=sizeof(pD->visible);
	NetGet(m,sizecount,pD->inFire);				sizecount+=sizeof(pD->inFire);
	NetGet(m,sizecount,pD->burnDamage);			sizecount+=sizeof(pD->burnDamage);

	NetGet(m,sizecount,pD->body);				sizecount+=sizeof(pD->body);

	NetGet(m,sizecount,pD->secondaryOrder);		sizecount+=sizeof(pD->secondaryOrder);

	for (i = 0;i < dt.numWeaps;i++)
	{
		NetGet(m, sizecount, id);			sizecount += sizeof(id);
		pD->psActionTarget[i] = IdToPointer(id, ANYPLAYER);
	}
	pD->psTarStats = NULL;

	addDroid(pD, apsDroidLists);

	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Functions for cases where a machine receives a netmessage about a certain
// droid. The droid is unknown, so the machine uses tese functions in order to
// find out about it.
BOOL sendRequestDroid(uint32_t droidId)
{
	if (ingame.localJoiningInProgress)		// Don't worry if still joining.
	{
		return FALSE;
	}

	debug(LOG_NEVER, "multibot: unknown droid %u, requesting info", droidId);

	NETbeginEncode(NET_REQUESTDROID, NET_ALL_PLAYERS);
	{
		int32_t dpid = player2dpid[selectedPlayer];

		NETuint32_t(&droidId);
		NETint32_t(&dpid);
	}
	return NETend();
}

// ////////////////////////////////////////////////////////////////////////////
BOOL recvRequestDroid()
{
	DROID*      psDroid;
	uint32_t    droidId;
	int32_t     dpid;

	NETbeginDecode();
	{
		NETuint32_t(&droidId);
		NETint32_t(&dpid);
	}
	NETend();

	// Get the droid
	if (!(IdToDroid(droidId, ANYPLAYER, &psDroid)))
	{
		// Can't find it, so ignore
		return TRUE;
	}

	// If we are responsible, send it
	if (myResponsibility(psDroid->player))
	{
		sendWholeDroid(psDroid, dpid);
	}

	return TRUE;
}
