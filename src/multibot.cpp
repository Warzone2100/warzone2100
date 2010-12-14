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
 * Multibot.c
 *
 * Alex Lee , 97/98 Pumpkin Studios, Bath
 * Multiplay stuff relevant to droids only.
 */
#include "lib/framework/frame.h"

#include "droid.h"						// for droid sending and ordering.
#include "droiddef.h"
#include "stats.h"
#include "move.h"						// for ordering droids
#include "objmem.h"
#include "power.h"						// for powercalculated
#include "order.h"
#include "map.h"
#include "group.h"
#include "lib/netplay/netplay.h"					// the netplay library.
#include "multiplay.h"					// warzone net stuff.
#include "multijoin.h"
#include "cmddroid.h"					// command droids
#include "action.h"
#include "console.h"
#include "mapgrid.h"
#include "multirecv.h"

#include <vector>
#include <algorithm>

#define ANYPLAYER	99

struct QueuedDroidInfo
{
	/// Sorts by order, then finally by droid id, to group multiple droids with the same order.
	bool operator <(QueuedDroidInfo const &z) const
	{
		int orComp = orderCompare(z);
		if (orComp != 0) return orComp < 0;
		return droidId < z.droidId;
	}
	/// Returns 0 if order is the same, non-zero otherwise.
	int orderCompare(QueuedDroidInfo const &z) const
	{
		if (player != z.player)       return player < z.player ? -1 : 1;
		if (order != z.order)         return order < z.order ? -1 : 1;
		if (subType != z.subType)     return subType < z.subType ? -1 : 1;
		if (subType)
		{
			if (destId != z.destId)       return destId < z.destId ? -1 : 1;
			if (destType != z.destType)   return destType < z.destType ? -1 : 1;
		}
		else
		{
			if (x != z.x)                 return x < z.x ? -1 : 1;
			if (y != z.y)                 return y < z.y ? -1 : 1;
		}
		if (order == DORDER_BUILD || order == DORDER_LINEBUILD)
		{
			if (structRef != z.structRef) return structRef < z.structRef ? -1 : 1;
			if (direction != z.direction) return direction < z.direction ? -1 : 1;
		}
		if (order == DORDER_LINEBUILD)
		{
			if (x2 != z.x2)               return x2 < z.x2 ? -1 : 1;
			if (y2 != z.y2)               return y2 < z.y2 ? -1 : 1;
		}
		if (add != z.add)             return add < z.add ? -1 : 1;
		return 0;
	}

	uint8_t     player;
	uint32_t    droidId;
	DROID_ORDER order;
	BOOL        subType;
	uint32_t    destId;     // if (subType)
	OBJECT_TYPE destType;   // if (subType)
	uint32_t    x;          // if (!subType)
	uint32_t    y;          // if (!subType)
	uint32_t    structRef;  // if (order == DORDER_BUILD || order == DORDER_LINEBUILD)
	uint16_t    direction;  // if (order == DORDER_BUILD || order == DORDER_LINEBUILD)
	uint32_t    x2;         // if (order == DORDER_LINEBUILD)
	uint32_t    y2;         // if (order == DORDER_LINEBUILD)

	BOOL        add;
};

static std::vector<QueuedDroidInfo> queuedOrders;


// ////////////////////////////////////////////////////////////////////////////
// Local Prototypes

static BASE_OBJECT *processDroidTarget(OBJECT_TYPE desttype, uint32_t destid);
static BASE_OBJECT TargetMissing_;  // This memory is never referenced.
static BASE_OBJECT *const TargetMissing = &TargetMissing_;  // Error return value for processDroidTarget.

// ////////////////////////////////////////////////////////////////////////////
// Command Droids.

// sod em.


// ////////////////////////////////////////////////////////////////////////////
// Secondary Orders.

// Send
BOOL sendDroidSecondary(const DROID* psDroid, SECONDARY_ORDER sec, SECONDARY_STATE state)
{
	if (!bMultiMessages)
		return true;

	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_SECONDARY);
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
BOOL recvDroidSecondary(NETQUEUE queue)
{
	DROID*          psDroid;
	SECONDARY_ORDER sec = DSO_ATTACK_RANGE;
	SECONDARY_STATE state = DSS_NONE;

	NETbeginDecode(queue, GAME_SECONDARY);
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
			return false;
		}
	}
	NETend();

	// Set the droids secondary order
	turnOffMultiMsg(true);
	secondarySetState(psDroid, sec, state);
	turnOffMultiMsg(false);

	return true;
}

/** Broadcast droid and transporter loading information
 *
 *  \sa recvDroidEmbark(),sendDroidDisEmbark(),recvDroidDisEmbark()
 */
BOOL sendDroidEmbark(const DROID* psDroid, const DROID* psTransporter)
{
	if (!bMultiMessages)
		return true;

	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_DROIDEMBARK);
	{
		uint8_t player = psDroid->player;
		uint32_t droidID = psDroid->id;
		uint32_t transporterID = psTransporter->id;

		NETuint8_t(&player);
		NETuint32_t(&droidID);
		NETuint32_t(&transporterID);
	}
	return NETend();
}

/** Receive droid and transporter loading information
 *
 *  \sa sendDroidEmbark(),sendDroidDisEmbark(),recvDroidDisEmbark()
 */
BOOL recvDroidEmbark(NETQUEUE queue)
{
	DROID* psDroid;
	DROID* psTransporterDroid;
	BOOL bDroidRemoved;

	NETbeginDecode(queue, GAME_DROIDEMBARK);
	{
		uint8_t player;
		uint32_t droidID;
		uint32_t transporterID;

		NETuint8_t(&player);
		NETuint32_t(&droidID);
		NETuint32_t(&transporterID);

		// we have to find the droid on our (local) list first.
		if (!IdToDroid(droidID, player, &psDroid))
		{
			NETend();
			// Possible it already died? (sync error?)
			debug(LOG_WARNING, "player's %d droid %d wasn't found?", player,droidID);
			return false;
		}
		if (!IdToDroid(transporterID, player, &psTransporterDroid))
		{
			NETend();
			// Possible it already died? (sync error?)
			debug(LOG_WARNING, "player's %d transport droid %d wasn't found?", player,transporterID);
			return false;
		}

		if (psDroid == NULL)
		{
			// how can this happen?
			return true;
		}

		// Take it out of the world without destroying it (just removes it from the droid list)
		bDroidRemoved = droidRemove(psDroid, apsDroidLists);

		// Init the order for when disembark
		psDroid->order = DORDER_NONE;
		setDroidTarget(psDroid, NULL);
		psDroid->psTarStats = NULL;

		if (bDroidRemoved)
		{
			// and now we need to add it to their transporter group!
			grpJoin(psTransporterDroid->psGroup, psDroid);
		}
		else
		{
			// possible sync error?
			debug(LOG_WARNING, "Eh? Where did unit %d go? Couldn't load droid onto transporter.", droidID);
		}
	}
	NETend();
	return true;
}

/** Broadcast that droid is being unloaded from a transporter
 *
 *  \sa sendDroidEmbark(),recvDroidEmbark(),recvDroidDisEmbark()
 */
BOOL sendDroidDisEmbark(const DROID* psDroid, const DROID* psTransporter)
{
	if (!bMultiMessages)
		return true;

	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_DROIDDISEMBARK);
	{
		uint8_t player = psDroid->player;
		uint32_t droidID = psDroid->id;
		uint32_t transporterID = psTransporter->id;
		Position pos = droidGetPrecisePosition(psDroid);

		NETuint8_t(&player);
		NETuint32_t(&droidID);
		NETuint32_t(&transporterID);
		NETPosition(&pos);
	}
	return NETend();
}

/** Receive info about a droid that is being unloaded from a transporter
 *
 *  \sa sendDroidEmbark(),recvDroidEmbark(),sendDroidDisEmbark()
 */
BOOL recvDroidDisEmbark(NETQUEUE queue)
{
	DROID *psFoundDroid = NULL, *psTransporterDroid = NULL;
	DROID *psCheckDroid = NULL;

	NETbeginDecode(queue, GAME_DROIDDISEMBARK);
	{
		uint8_t player;
		uint32_t droidID;
		uint32_t transporterID;
		Position pos;

		NETuint8_t(&player);
		NETuint32_t(&droidID);
		NETuint32_t(&transporterID);
		NETPosition(&pos);

		NETend();

		// find the transporter first
		if (!IdToDroid(transporterID, player, &psTransporterDroid))
		{
			// Possible it already died? (sync error?)
			debug(LOG_WARNING, "player's %d transport droid %d wasn't found?", player, transporterID);
			return false;
		}
		// we need to find the droid *in* the transporter
		psCheckDroid = psTransporterDroid ->psGroup->psList;
		while (psCheckDroid)
		{
			// is this the one we want?
			if( psCheckDroid->id == droidID)
			{
				psFoundDroid = psCheckDroid;
				break;
			}
			// not found, so check next one in *group*
			psCheckDroid = psCheckDroid->psGrpNext;
		}
		// don't continue if we couldn't find it.
		if (!psFoundDroid)
		{
			// I don't think this could ever be possible...but
			debug(LOG_ERROR, "Couldn't find droid %d to disembark from player %d's transporter?", droidID, player);
			return false;
		}

		// remove it from the transporter
		grpLeave(psFoundDroid->psGroup, psFoundDroid);

		// and add it back to the bloody droid list
		addDroid(psFoundDroid, apsDroidLists);

		// Add it back into the world at the x/y
		droidSetPrecisePosition(psFoundDroid, pos);

		if (!droidOnMap(psFoundDroid))
		{
			debug(LOG_ERROR, "droid %d disembarked was NOT on map?", psFoundDroid->id);
			return false;
		}

		updateDroidOrientation(psFoundDroid);

		// Initialise the movement data
		initDroidMovement(psFoundDroid);
	}
	return true;
}


// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Droids

// ////////////////////////////////////////////////////////////////////////////
// Send a new Droid to the other players
BOOL SendDroid(const DROID_TEMPLATE* pTemplate, uint32_t x, uint32_t y, uint8_t player, uint32_t id, const INITIAL_DROID_ORDERS *initialOrdersP)
{
	if (!bMultiMessages)
		return true;

	ASSERT(x != 0 && y != 0, "SendDroid: Invalid droid coordinates");
	ASSERT( player < MAX_PLAYERS, "invalid player %u", player);

	// Dont send other droids during campaign setup
	if (ingame.localJoiningInProgress)
	{
		return true;
	}

	// Only send the droid if we are responsible
	if (!myResponsibility(player))
	{
		// Don't build if we are not responsible
		return false;
	}

	debug(LOG_SYNC, "Droid sent with id of %u", id);
	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_DROID);
	{
		Position pos = { x, y, 0 };
		uint32_t templateID = pTemplate->multiPlayerID;
		BOOL haveInitialOrders = initialOrdersP != NULL;

		NETuint8_t(&player);
		NETuint32_t(&id);
		NETPosition(&pos);
		NETuint32_t(&templateID);
		NETbool(&haveInitialOrders);
		if (haveInitialOrders)
		{
			INITIAL_DROID_ORDERS initialOrders = *initialOrdersP;
			NETuint32_t(&initialOrders.secondaryOrder);
			NETint32_t(&initialOrders.moveToX);
			NETint32_t(&initialOrders.moveToY);
			NETuint32_t(&initialOrders.factoryId);  // For making scripts happy.
		}
	}
	debug(LOG_LIFE, "===> sending Droid from %u id of %u ",player,id);
	return NETend();
}

// ////////////////////////////////////////////////////////////////////////////
// receive droid creation information from other players
BOOL recvDroid(NETQUEUE queue)
{
	DROID_TEMPLATE* pT;
	DROID* psDroid;
	uint8_t player;
	uint32_t id;
	Position pos;
	uint32_t templateID;
	BOOL haveInitialOrders;
	INITIAL_DROID_ORDERS initialOrders;

	NETbeginDecode(queue, GAME_DROID);
	{
		NETuint8_t(&player);
		NETuint32_t(&id);
		NETPosition(&pos);
		NETuint32_t(&templateID);
		NETbool(&haveInitialOrders);
		if (haveInitialOrders)
		{
			NETuint32_t(&initialOrders.secondaryOrder);
			NETint32_t(&initialOrders.moveToX);
			NETint32_t(&initialOrders.moveToY);
			NETuint32_t(&initialOrders.factoryId);  // For making scripts happy.
		}

		pT = IdToTemplate(templateID, player);
	}
	NETend();

	ASSERT( player < MAX_PLAYERS, "invalid player %u", player);

	debug(LOG_LIFE, "<=== getting Droid from %u id of %u ",player,id);
	if ((pos.x == 0 && pos.y == 0) || pos.x > world_coord(mapWidth) || pos.y > world_coord(mapHeight))
	{
		debug(LOG_ERROR, "Received bad droid position (%d, %d) from %d about p%d (%s)", (int)pos.x, (int)pos.y,
				queue.index, player, isHumanPlayer(player) ? "Human" : "AI");
		return false;
	}

	// If we can not find the template ask for the entire droid instead
	if (!pT)
	{
		debug(LOG_ERROR, "Packet from %d refers to non-existent template %u, [%s : p%d]",
				queue.index, templateID, isHumanPlayer(player) ? "Human" : "AI", player);
		return false;
	}

	// Create that droid on this machine.
	psDroid = reallyBuildDroid(pT, pos.x, pos.y, player, false);

	// If we were able to build the droid set it up
	if (psDroid)
	{
		psDroid->id = id;
		addDroid(psDroid, apsDroidLists);

		if (haveInitialOrders)
		{
			psDroid->secondaryOrder = initialOrders.secondaryOrder;
			orderDroidLoc(psDroid, DORDER_MOVE, initialOrders.moveToX, initialOrders.moveToY, ModeImmediate);
			cbNewDroid(IdToStruct(initialOrders.factoryId, ANYPLAYER), psDroid);
		}

		syncDebugDroid(psDroid, '+');
	}
	else
	{
		debug(LOG_ERROR, "Packet from %d cannot create droid for p%d (%s)!", queue.index,
			player, isHumanPlayer(player) ? "Human" : "AI");
#ifdef DEBUG
		CONPRINTF(ConsoleString, (ConsoleString, "MULTIPLAYER: Couldn't build a remote droid, relying on checking to resync"));
#endif
		return false;
	}

	return true;
}


/// Does not read/write info->droidId!
static void NETQueuedDroidInfo(QueuedDroidInfo *info)
{
	NETuint8_t(&info->player);
	NETenum(&info->order);
	NETbool(&info->subType);
	if (info->subType)
	{
		NETuint32_t(&info->destId);
		NETenum(&info->destType);
	}
	else
	{
		NETuint32_t(&info->x);
		NETuint32_t(&info->y);
	}
	if (info->order == DORDER_BUILD || info->order == DORDER_LINEBUILD)
	{
		NETuint32_t(&info->structRef);
		NETuint16_t(&info->direction);
	}
	if (info->order == DORDER_LINEBUILD)
	{
		NETuint32_t(&info->x2);
		NETuint32_t(&info->y2);
	}
	NETbool(&info->add);
}

// Actually send the droid info.
void sendQueuedDroidInfo()
{
	// Sort queued orders, to group the same order to multiple droids.
	std::sort(queuedOrders.begin(), queuedOrders.end());

	std::vector<QueuedDroidInfo>::iterator eqBegin, eqEnd;
	for (eqBegin = queuedOrders.begin(); eqBegin != queuedOrders.end(); eqBegin = eqEnd)
	{
		// Find end of range of orders which differ only by the droid ID.
		for (eqEnd = eqBegin + 1; eqEnd != queuedOrders.end() && eqEnd->orderCompare(*eqBegin) == 0; ++eqEnd)
		{}

		NETbeginEncode(NETgameQueue(selectedPlayer), GAME_DROIDINFO);
			NETQueuedDroidInfo(&*eqBegin);

			uint32_t num = eqEnd - eqBegin;
			NETuint32_t(&num);

			uint32_t prevDroidId = 0;
			for (unsigned n = 0; n < num; ++n)
			{
				uint32_t droidId = (eqBegin + n)->droidId;

				// Encode deltas between droid IDs, since the deltas are smaller than the actual droid IDs, and will encode to less bytes on average.
				uint32_t deltaDroidId = droidId - prevDroidId;
				NETuint32_t(&deltaDroidId);

				prevDroidId = droidId;
			}
		NETend();
	}

	// Sent the orders. Don't send them again.
	queuedOrders.clear();
}

DROID_ORDER_DATA infoToOrderData(QueuedDroidInfo const &info, BASE_STATS const *psStats)
{
	DROID_ORDER_DATA sOrder;

	memset(&sOrder, 0x00, sizeof(sOrder));
	sOrder.order = info.order;
	sOrder.x = info.x;
	sOrder.y = info.y;
	sOrder.x2 = info.x2;
	sOrder.y2 = info.y2;
	sOrder.direction = info.direction;
	sOrder.psObj = processDroidTarget(info.destType, info.destId);
	sOrder.psStats = const_cast<BASE_STATS *>(psStats);

	return sOrder;
}

// ////////////////////////////////////////////////////////////////////////////
// Droid update information
bool sendDroidInfo(DROID *psDroid, DROID_ORDER order, uint32_t x, uint32_t y, const BASE_OBJECT *psObj, const BASE_STATS *psStats, uint32_t x2, uint32_t y2, uint16_t direction, bool add)
{
	if (!myResponsibility(psDroid->player))
	{
		return true;
	}

	QueuedDroidInfo info;
	memset(&info, 0x00, sizeof(info));  // Suppress uninitialised warnings. (The uninitialised values in the queue would be ignored when reading the queue.)

	info.player = psDroid->player;
	info.droidId = psDroid->id;
	info.order = order;
	info.subType = psObj != NULL;
	if (info.subType)
	{
		info.destId = psObj->id;
		info.destType = psObj->type;
	}
	else
	{
		info.x = x;
		info.y = y;
	}
	if (order == DORDER_BUILD || order == DORDER_LINEBUILD)
	{
		info.structRef = psStats->ref;
		info.direction = direction;
	}
	if (order == DORDER_LINEBUILD)
	{
		info.x2 = x2;
		info.y2 = y2;
	}

	info.add = add;

	// Send later, grouped by order, so multiple droids with the same order can be encoded to much less data.
	queuedOrders.push_back(info);

	// Update pending orders, so the UI knows it happened.
	DROID_ORDER_DATA sOrder = infoToOrderData(info, psStats);
	if (!add)
	{
		psDroid->listPendingBegin = psDroid->listPendingEnd;
	}
	orderDroidAddPending(psDroid, &sOrder);

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// receive droid information form other players.
BOOL recvDroidInfo(NETQUEUE queue)
{
	NETbeginDecode(queue, GAME_DROIDINFO);
	{
		QueuedDroidInfo info;
		memset(&info, 0x00, sizeof(info));
		NETQueuedDroidInfo(&info);

		STRUCTURE_STATS *psStats = NULL;
		if (info.order == DORDER_BUILD || info.order == DORDER_LINEBUILD)
		{
			// Find structure target
			for (unsigned typeIndex = 0; typeIndex < numStructureStats; typeIndex++)
			{
				if (asStructureStats[typeIndex].ref == info.structRef)
				{
					psStats = asStructureStats + typeIndex;
					break;
				}
			}
		}

		if (info.subType)
		{
			syncDebug("Order=%s,%d(%d)", getDroidOrderName(info.order), info.destId, info.destType);
		}
		else
		{
			syncDebug("Order=%s,(%d,%d)", getDroidOrderName(info.order), info.x, info.y);
		}

		DROID_ORDER_DATA sOrder = infoToOrderData(info, (BASE_STATS *)psStats);

		uint32_t num = 0;
		NETuint32_t(&num);

		for (unsigned n = 0; n < num; ++n)
		{
			// Get the next droid ID which is being given this order.
			uint32_t deltaDroidId = 0;
			NETuint32_t(&deltaDroidId);
			info.droidId += deltaDroidId;

			DROID *psDroid = NULL;
			if (!IdToDroid(info.droidId, ANYPLAYER, &psDroid))
			{
				debug(LOG_NEVER, "Packet from %d refers to non-existent droid %u, [%s : p%d]",
				      queue.index, info.droidId, isHumanPlayer(info.player) ? "Human" : "AI", info.player);
				syncDebug("Droid %d missing", info.droidId);
				continue;  // Can't find the droid, so skip this droid.
			}

			CHECK_DROID(psDroid);

			syncDebugDroid(psDroid, '<');

			/*
			* If the current order not is a command order and we are not a
			* commander yet are in the commander group remove us from it.
			*/
			if (hasCommander(psDroid))
			{
				grpLeave(psDroid->psGroup, psDroid);
			}

			if (sOrder.psObj != TargetMissing)  // Only do order if the target didn't die.
			{
				if (!info.add)
				{
					orderDroidListEraseRange(psDroid, 0, psDroid->listSize + 1);  // Clear all non-pending orders, plus the first pending order (which is probably the order we just received).
					orderDroidBase(psDroid, &sOrder);  // Execute the order immediately (even if in the middle of another order.
				}
				else
				{
					orderDroidAdd(psDroid, &sOrder);   // Add the order to the (non-pending) list. Will probably overwrite the corresponding pending order, assuming all pending orders were written to the list.
				}
			}

			syncDebugDroid(psDroid, '>');

			CHECK_DROID(psDroid);
		}
	}
	NETend();

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// process droid order
static BASE_OBJECT *processDroidTarget(OBJECT_TYPE desttype, uint32_t destid)
{
	// Target is a location
	if (destid == 0 && desttype == 0)
	{
		return NULL;
	}
	// Target is an object
	else
	{
		BASE_OBJECT *psObj = NULL;
		DROID		*pD;

		switch (desttype)
		{
			case OBJ_DROID:
				if (IdToDroid(destid, ANYPLAYER, &pD))
				{
					psObj = (BASE_OBJECT*)pD;
				}
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
			syncDebug("Target missing");
			return TargetMissing;  // Can't return NULL, since then the order would still be attempted.
		}

		return psObj;
	}
}


// ////////////////////////////////////////////////////////////////////////////
// Inform other players that a droid has been destroyed
BOOL SendDestroyDroid(const DROID* psDroid)
{
	if (!bMultiMessages)
	{
		return true;
	}

	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_DROIDDEST);
	{
		uint32_t id = psDroid->id;

		// Send the droid's ID
		debug(LOG_DEATH, "Requested all players to destroy droid %u", (unsigned int)id);
		NETuint32_t(&id);
	}
	return NETend();
}

// ////////////////////////////////////////////////////////////////////////////
// Accept a droid which was destroyed on another machine
BOOL recvDestroyDroid(NETQUEUE queue)
{
	DROID* psDroid;

	NETbeginDecode(queue, GAME_DROIDDEST);
	{
		uint32_t id;

		// Retrieve the droid
		NETuint32_t(&id);
		if (!IdToDroid(id, ANYPLAYER, &psDroid))
		{
			debug(LOG_DEATH, "droid %d on request from player %d can't be found? Must be dead already?",
					id, queue.index );
			return false;
		}
	}
	NETend();

	// If the droid has not died on our machine yet, destroy it
	if(!psDroid->died)
	{
		turnOffMultiMsg(true);
		debug(LOG_DEATH, "Killing droid %d on request from player %d - huh?", psDroid->id, queue.index);
		destroyDroid(psDroid);
		turnOffMultiMsg(false);
	}
	else
	{
		debug(LOG_DEATH, "droid %d is confirmed dead by player %d.", psDroid->id, queue.index);
	}

	return true;
}
