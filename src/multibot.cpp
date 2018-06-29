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
/*
 * Multibot.c
 *
 * Alex Lee , 97/98 Pumpkin Studios, Bath
 * Multiplay stuff relevant to droids only.
 */
#include "lib/framework/frame.h"

#include "droid.h"						// for droid sending and ordering.
#include "droiddef.h"
#include "keymap.h"
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
#include "transporter.h"

#include <vector>
#include <algorithm>


enum SubType
{
	ObjOrder, LocOrder, SecondaryOrder
};

struct QueuedDroidInfo
{
	/// Sorts by order, then finally by droid id, to group multiple droids with the same order.
	bool operator <(QueuedDroidInfo const &z) const
	{
		int orComp = orderCompare(z);
		if (orComp != 0)
		{
			return orComp < 0;
		}
		return droidId < z.droidId;
	}
	/// Returns 0 if order is the same, non-zero otherwise.
	int orderCompare(QueuedDroidInfo const &z) const
	{
		if (player != z.player)
		{
			return player < z.player ? -1 : 1;
		}
		if (subType != z.subType)
		{
			return subType < z.subType ? -1 : 1;
		}
		switch (subType)
		{
		case ObjOrder:
		case LocOrder:
			if (order != z.order)
			{
				return order < z.order ? -1 : 1;
			}
			if (subType == ObjOrder)
			{
				if (destId != z.destId)
				{
					return destId < z.destId ? -1 : 1;
				}
				if (destType != z.destType)
				{
					return destType < z.destType ? -1 : 1;
				}
			}
			else
			{
				if (pos.x != z.pos.x)
				{
					return pos.x < z.pos.x ? -1 : 1;
				}
				if (pos.y != z.pos.y)
				{
					return pos.y < z.pos.y ? -1 : 1;
				}
			}
			if (order == DORDER_BUILD || order == DORDER_LINEBUILD)
			{
				if (structRef != z.structRef)
				{
					return structRef < z.structRef ? -1 : 1;
				}
				if (direction != z.direction)
				{
					return direction < z.direction ? -1 : 1;
				}
			}
			if (order == DORDER_LINEBUILD)
			{
				if (pos2.x != z.pos2.x)
				{
					return pos2.x < z.pos2.x ? -1 : 1;
				}
				if (pos2.y != z.pos2.y)
				{
					return pos2.y < z.pos2.y ? -1 : 1;
				}
			}
			if (order == DORDER_BUILDMODULE)
			{
				if (index != z.index)
				{
					return index < z.index ? -1 : 1;
				}
			}
			if (add != z.add)
			{
				return add < z.add ? -1 : 1;
			}
			break;
		case SecondaryOrder:
			if (secOrder != z.secOrder)
			{
				return secOrder < z.secOrder ? -1 : 1;
			}
			if (secState != z.secState)
			{
				return secState < z.secState ? -1 : 1;
			}
			break;
		}
		return 0;
	}

	uint8_t     player = 0;
	uint32_t    droidId = 0;
	SubType     subType = ObjOrder;
	// subType == ObjOrder || subType == LocOrder
	DROID_ORDER order = DORDER_NONE;
	uint32_t    destId = 0;     // if (subType == ObjOrder)
	OBJECT_TYPE destType = OBJ_DROID;   // if (subType == ObjOrder)
	Vector2i    pos = Vector2i(0, 0);            // if (subType == LocOrder)
	uint32_t    y = 0;          // if (subType == LocOrder)
	uint32_t    structRef = 0;  // if (order == DORDER_BUILD || order == DORDER_LINEBUILD)
	uint16_t    direction = 0;  // if (order == DORDER_BUILD || order == DORDER_LINEBUILD)
	uint32_t    index = 0;      // if (order == DORDER_BUILDMODULE)
	Vector2i    pos2 = Vector2i(0, 0);           // if (order == DORDER_LINEBUILD)
	bool        add = false;
	// subType == SecondaryOrder
	SECONDARY_ORDER secOrder = DSO_UNUSED;
	SECONDARY_STATE secState = DSS_NONE;
};

static std::vector<QueuedDroidInfo> queuedOrders;


// ////////////////////////////////////////////////////////////////////////////
// Local Prototypes

static BASE_OBJECT *processDroidTarget(OBJECT_TYPE desttype, uint32_t destid);
static BASE_OBJECT TargetMissing_(OBJ_NUM_TYPES, 0, 0);         // This memory is never referenced.
static BASE_OBJECT *const TargetMissing = &TargetMissing_;  // Error return value for processDroidTarget.

// ////////////////////////////////////////////////////////////////////////////
// Command Droids.

// sod em.


// ////////////////////////////////////////////////////////////////////////////
// Secondary Orders.

// Send
bool sendDroidSecondary(const DROID *psDroid, SECONDARY_ORDER sec, SECONDARY_STATE state)
{
	if (!bMultiMessages)
	{
		return true;
	}

	QueuedDroidInfo info;

	info.player = psDroid->player;
	info.droidId = psDroid->id;
	info.subType = SecondaryOrder;
	info.secOrder = sec;
	info.secState = state;

	// Send later, grouped by order, so multiple droids with the same order can be encoded to much less data.
	queuedOrders.push_back(info);

	return true;
}

/** Broadcast that droid is being unloaded from a transporter
 *
 *  \sa recvDroidDisEmbark()
 */
bool sendDroidDisembark(DROID const *psTransporter, DROID const *psDroid)
{
	if (!bMultiMessages)
	{
		return true;
	}

	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_DROIDDISEMBARK);
	{
		uint32_t player = psTransporter->player;
		uint32_t droidId = psDroid->id;
		uint32_t transportId = psTransporter->id;

		NETuint32_t(&player);
		NETuint32_t(&droidId);
		NETuint32_t(&transportId);
	}
	return NETend();
}

/** Receive info about a droid that is being unloaded from a transporter
 */
bool recvDroidDisEmbark(NETQUEUE queue)
{
	DROID *psFoundDroid = nullptr, *psTransporterDroid = nullptr;
	DROID *psCheckDroid = nullptr;

	NETbeginDecode(queue, GAME_DROIDDISEMBARK);
	{
		uint32_t player;
		uint32_t droidID;
		uint32_t transporterID;

		NETuint32_t(&player);
		NETuint32_t(&droidID);
		NETuint32_t(&transporterID);

		NETend();

		// find the transporter first
		psTransporterDroid = IdToDroid(transporterID, player);
		if (!psTransporterDroid)
		{
			// Possible it already died? (sync error?)
			debug(LOG_WARNING, "player's %d transport droid %d wasn't found?", player, transporterID);
			return false;
		}
		if (!canGiveOrdersFor(queue.index, psTransporterDroid->player))
		{
			return false;
		}
		// we need to find the droid *in* the transporter
		psCheckDroid = psTransporterDroid ->psGroup->psList;
		while (psCheckDroid)
		{
			// is this the one we want?
			if (psCheckDroid->id == droidID)
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

		transporterRemoveDroid(psTransporterDroid, psFoundDroid, ModeImmediate);
	}
	return true;
}


// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Droids

// ////////////////////////////////////////////////////////////////////////////
// Send a new Droid to the other players
bool SendDroid(DROID_TEMPLATE *pTemplate, uint32_t x, uint32_t y, uint8_t player, uint32_t id, const INITIAL_DROID_ORDERS *initialOrdersP)
{
	if (!bMultiMessages)
	{
		return true;
	}

	ASSERT_OR_RETURN(false, x != 0 && y != 0, "SendDroid: Invalid droid coordinates");
	ASSERT_OR_RETURN(false, player < MAX_PLAYERS, "invalid player %u", player);

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
	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_DEBUG_ADD_DROID);
	{
		Position pos(x, y, 0);
		bool haveInitialOrders = initialOrdersP != nullptr;
		int32_t droidType = pTemplate->droidType;

		NETuint8_t(&player);
		NETuint32_t(&id);
		NETPosition(&pos);
		WzString name = pTemplate->name;
		NETwzstring(name);
		NETint32_t(&droidType);
		NETuint8_t(&pTemplate->asParts[COMP_BODY]);
		NETuint8_t(&pTemplate->asParts[COMP_BRAIN]);
		NETuint8_t(&pTemplate->asParts[COMP_PROPULSION]);
		NETuint8_t(&pTemplate->asParts[COMP_REPAIRUNIT]);
		NETuint8_t(&pTemplate->asParts[COMP_ECM]);
		NETuint8_t(&pTemplate->asParts[COMP_SENSOR]);
		NETuint8_t(&pTemplate->asParts[COMP_CONSTRUCT]);
		NETint8_t(&pTemplate->numWeaps);
		for (int i = 0; i < pTemplate->numWeaps; i++)
		{
			NETuint32_t(&pTemplate->asWeaps[i]);
		}
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
	debug(LOG_LIFE, "===> sending Droid from %u id of %u ", player, id);
	return NETend();
}

// ////////////////////////////////////////////////////////////////////////////
// receive droid creation information from other players
bool recvDroid(NETQUEUE queue)
{
	DROID_TEMPLATE t, *pT = &t;
	DROID *psDroid = nullptr;
	uint8_t player = 0;
	uint32_t id = 0;
	Position pos(0, 0, 0);
	bool haveInitialOrders = false;
	INITIAL_DROID_ORDERS initialOrders = { 0, 0, 0, 0 };

	NETbeginDecode(queue, GAME_DEBUG_ADD_DROID);
	{
		int32_t droidType;

		NETuint8_t(&player);
		NETuint32_t(&id);
		NETPosition(&pos);
		WzString name;
		NETwzstring(name);
		pT->name = name;
		pT->id = pT->name;
		NETint32_t(&droidType);
		NETuint8_t(&pT->asParts[COMP_BODY]);
		NETuint8_t(&pT->asParts[COMP_BRAIN]);
		NETuint8_t(&pT->asParts[COMP_PROPULSION]);
		NETuint8_t(&pT->asParts[COMP_REPAIRUNIT]);
		NETuint8_t(&pT->asParts[COMP_ECM]);
		NETuint8_t(&pT->asParts[COMP_SENSOR]);
		NETuint8_t(&pT->asParts[COMP_CONSTRUCT]);
		NETint8_t(&pT->numWeaps);
		ASSERT_OR_RETURN(false, pT->numWeaps >= 0 && pT->numWeaps <= ARRAY_SIZE(pT->asWeaps), "Bad numWeaps %d", pT->numWeaps);
		for (int i = 0; i < pT->numWeaps; i++)
		{
			NETuint32_t(&pT->asWeaps[i]);
		}
		NETbool(&haveInitialOrders);
		if (haveInitialOrders)
		{
			NETuint32_t(&initialOrders.secondaryOrder);
			NETint32_t(&initialOrders.moveToX);
			NETint32_t(&initialOrders.moveToY);
			NETuint32_t(&initialOrders.factoryId);  // For making scripts happy.
		}
		pT->droidType = (DROID_TYPE)droidType;
	}
	NETend();

	if (!getDebugMappingStatus() && bMultiPlayer)
	{
		debug(LOG_WARNING, "Failed to add droid for player %u.", NetPlay.players[queue.index].position);
		return false;
	}

	ASSERT_OR_RETURN(false, player < MAX_PLAYERS, "invalid player %u", player);

	debug(LOG_LIFE, "<=== getting Droid from %u id of %u ", player, id);
	if ((pos.x == 0 && pos.y == 0) || pos.x > world_coord(mapWidth) || pos.y > world_coord(mapHeight))
	{
		debug(LOG_ERROR, "Received bad droid position (%d, %d) from %d about p%d (%s)", (int)pos.x, (int)pos.y,
		      queue.index, player, isHumanPlayer(player) ? "Human" : "AI");
		return false;
	}

	// Create that droid on this machine.
	psDroid = reallyBuildDroid(pT, pos, player, false);

	// If we were able to build the droid set it up
	if (psDroid)
	{
		psDroid->id = id;
		addDroid(psDroid, apsDroidLists);

		if (haveInitialOrders)
		{
			psDroid->secondaryOrder = initialOrders.secondaryOrder;
			psDroid->secondaryOrderPending = psDroid->secondaryOrder;
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
	NETenum(&info->subType);
	switch (info->subType)
	{
	case ObjOrder:
	case LocOrder:
		NETenum(&info->order);
		if (info->subType == ObjOrder)
		{
			NETuint32_t(&info->destId);
			NETenum(&info->destType);
		}
		else
		{
			NETauto(&info->pos);
		}
		if (info->order == DORDER_BUILD || info->order == DORDER_LINEBUILD)
		{
			NETuint32_t(&info->structRef);
			NETuint16_t(&info->direction);
		}
		if (info->order == DORDER_LINEBUILD)
		{
			NETauto(&info->pos2);
		}
		if (info->order == DORDER_BUILDMODULE)
		{
			NETauto(&info->index);
		}
		NETbool(&info->add);
		break;
	case SecondaryOrder:
		NETenum(&info->secOrder);
		NETenum(&info->secState);
		break;
	}
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

DROID_ORDER_DATA infoToOrderData(QueuedDroidInfo const &info, STRUCTURE_STATS const *psStats)
{
	DROID_ORDER_DATA sOrder;
	sOrder.type = info.order;
	sOrder.pos = info.pos;
	sOrder.pos2 = info.pos2;
	sOrder.direction = info.direction;
	sOrder.index = info.index;
	sOrder.psObj = processDroidTarget(info.destType, info.destId);
	sOrder.psStats = const_cast<STRUCTURE_STATS *>(psStats);

	return sOrder;
}

// ////////////////////////////////////////////////////////////////////////////
// Droid update information
void sendDroidInfo(DROID *psDroid, DroidOrder const &order, bool add)
{
	if (!myResponsibility(psDroid->player))
	{
		return;
	}

	QueuedDroidInfo info;

	info.player = psDroid->player;
	info.droidId = psDroid->id;
	info.subType = order.psObj != nullptr ? ObjOrder : LocOrder;
	info.order = order.type;
	if (info.subType == ObjOrder)
	{
		info.destId = order.psObj->id;
		info.destType = order.psObj->type;
	}
	else
	{
		info.pos = order.pos;
	}
	if (order.type == DORDER_BUILD || order.type == DORDER_LINEBUILD)
	{
		info.structRef = order.psStats->ref;
		info.direction = order.direction;
		if (!isConstructionDroid(psDroid))
		{
			return;  // No point ordering things to build if they can't build anything.
		}
	}
	if (order.type == DORDER_LINEBUILD)
	{
		info.pos2 = order.pos2;
	}
	if (order.type == DORDER_BUILDMODULE)
	{
		info.index = order.index;
	}

	info.add = add;

	// Send later, grouped by order, so multiple droids with the same order can be encoded to much less data.
	queuedOrders.push_back(info);

	// Update pending orders, so the UI knows it happened.
	DROID_ORDER_DATA sOrder = infoToOrderData(info, order.psStats);
	if (!add)
	{
		psDroid->listPendingBegin = psDroid->asOrderList.size();
	}
	orderDroidAddPending(psDroid, &sOrder);
}

// ////////////////////////////////////////////////////////////////////////////
// receive droid information form other players.
bool recvDroidInfo(NETQUEUE queue)
{
	NETbeginDecode(queue, GAME_DROIDINFO);
	{
		QueuedDroidInfo info;
		NETQueuedDroidInfo(&info);

		STRUCTURE_STATS *psStats = nullptr;
		if (info.subType == LocOrder && (info.order == DORDER_BUILD || info.order == DORDER_LINEBUILD))
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

		switch (info.subType)
		{
		case ObjOrder:       syncDebug("Order=%s,%d(%d)", getDroidOrderName(info.order), info.destId, info.destType); break;
		case LocOrder:       syncDebug("Order=%s,(%d,%d)", getDroidOrderName(info.order), info.pos.x, info.pos.y); break;
		case SecondaryOrder: syncDebug("SecondaryOrder=%d,%08X", (int)info.secOrder, (int)info.secState); break;
		}

		DROID_ORDER_DATA sOrder = infoToOrderData(info, psStats);

		uint32_t num = 0;
		NETuint32_t(&num);

		for (unsigned n = 0; n < num; ++n)
		{
			// Get the next droid ID which is being given this order.
			uint32_t deltaDroidId = 0;
			NETuint32_t(&deltaDroidId);
			info.droidId += deltaDroidId;

			DROID *psDroid = IdToDroid(info.droidId, info.player);
			if (!psDroid)
			{
				debug(LOG_NEVER, "Packet from %d refers to non-existent droid %u, [%s : p%d]",
				      queue.index, info.droidId, isHumanPlayer(info.player) ? "Human" : "AI", info.player);
				syncDebug("Droid %d missing", info.droidId);
				continue;  // Can't find the droid, so skip this droid.
			}
			if (!canGiveOrdersFor(queue.index, psDroid->player))
			{
				debug(LOG_WARNING, "Droid order (by %d) for wrong player (%d).", queue.index, psDroid->player);
				syncDebug("Wrong player.");
				continue;
			}

			CHECK_DROID(psDroid);

			syncDebugDroid(psDroid, '<');

			switch (info.subType)
			{
			case ObjOrder:
			case LocOrder:
				/*
				* If the current order not is a command order and we are not a
				* commander yet are in the commander group remove us from it.
				*/
				if (hasCommander(psDroid))
				{
					psDroid->psGroup->remove(psDroid);
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
				break;
			case SecondaryOrder:
				// Set the droids secondary order
				turnOffMultiMsg(true);
				secondarySetState(psDroid, info.secOrder, info.secState);
				turnOffMultiMsg(false);
				break;
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
		return nullptr;
	}
	// Target is an object
	else
	{
		BASE_OBJECT *psObj = nullptr;

		switch (desttype)
		{
		case OBJ_DROID:
			psObj = IdToDroid(destid, ANYPLAYER);
			break;
		case OBJ_STRUCTURE:
			psObj = IdToStruct(destid, ANYPLAYER);
			break;
		case OBJ_FEATURE:
			psObj = IdToFeature(destid, ANYPLAYER);
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
bool SendDestroyDroid(const DROID *psDroid)
{
	if (!bMultiMessages)
	{
		return true;
	}

	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_DEBUG_REMOVE_DROID);
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
bool recvDestroyDroid(NETQUEUE queue)
{
	DROID *psDroid;

	NETbeginDecode(queue, GAME_DEBUG_REMOVE_DROID);
	{
		uint32_t id;

		// Retrieve the droid
		NETuint32_t(&id);
		psDroid = IdToDroid(id, ANYPLAYER);
		if (!psDroid)
		{
			debug(LOG_DEATH, "droid %d on request from player %d can't be found? Must be dead already?",
			      id, queue.index);
			return false;
		}
	}
	NETend();

	if (!getDebugMappingStatus() && bMultiPlayer)
	{
		debug(LOG_WARNING, "Failed to remove droid for player %u.", NetPlay.players[queue.index].position);
		return false;
	}

	// If the droid has not died on our machine yet, destroy it
	if (!psDroid->died)
	{
		turnOffMultiMsg(true);
		debug(LOG_DEATH, "Killing droid %d on request from player %d - huh?", psDroid->id, queue.index);
		destroyDroid(psDroid, gameTime - deltaGameTime + 1);  // deltaGameTime is actually 0 here, since we're between updates. However, the value of gameTime - deltaGameTime + 1 will not change when we start the next tick.
		turnOffMultiMsg(false);
	}
	else
	{
		debug(LOG_DEATH, "droid %d is confirmed dead by player %d.", psDroid->id, queue.index);
	}

	return true;
}
