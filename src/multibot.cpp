// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
#include "transporter.h"
#include "game_world.h"
#include "multibot_serde.h"
#include "ordersource.h"
#include "ordersource_wire.h"

#include <vector>
#include <algorithm>


static std::vector<QueuedDroidInfo> queuedOrders;

static constexpr size_t DroidInfoMaxVarintBytes = 5;
static constexpr size_t DroidInfoMaxPayloadBytes = MaxMsgSize - NetMessage::HEADER_LENGTH;
static constexpr uint32_t MaxDroidInfoDroids = static_cast<uint32_t>((DroidInfoMaxPayloadBytes - DroidInfoMaxVarintBytes) / DroidInfoMaxVarintBytes);

static void applyOrderSource(QueuedDroidInfo &info, const OrderSource &source)
{
	info.provenance = orderProvenanceFromSource(source);
}


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
bool sendDroidSecondary(const DROID *psDroid, SECONDARY_ORDER sec, SECONDARY_STATE state, const OrderSource &source)
{
	if (!bMultiMessages)
	{
		return true;
	}

	if (!orderSourcePermitsPlayerAction(psDroid->player, source))
	{
		orderProvenanceRecord(psDroid->player, source.origin(), true);
		debug(LOG_NET, "Invalid secondary order for player %u from %s", psDroid->player, source.toDescription().c_str());
		return false;
	}
	orderProvenanceRecord(psDroid->player, source.origin(), false);

	QueuedDroidInfo info;

	info.player = psDroid->player;
	info.droidId = psDroid->id;
	info.subType = SecondaryOrder;
	info.secOrder = sec;
	info.secState = state;
	applyOrderSource(info, source);

	// Send later, grouped by order, so multiple droids with the same order can be encoded to much less data.
	queuedOrders.push_back(info);

	return true;
}

/** Broadcast that droid is being unloaded from a transporter
 *
 *  \sa recvDroidDisEmbark()
 */
bool sendDroidDisembark(const DROID *psTransporter, DROID const *psDroid, const OrderSource &source)
{
	if (!bMultiMessages)
	{
		return true;
	}

	if (!orderSourcePermitsPlayerAction(psDroid->player, source))
	{
		orderProvenanceRecord(psDroid->player, source.origin(), true);
		debug(LOG_NET, "Invalid disembark for player %u from %s", psDroid->player, source.toDescription().c_str());
		return false;
	}
	orderProvenanceRecord(psDroid->player, source.origin(), false);

	auto w = NETbeginEncode(NETgameQueue(realSelectedPlayer), GAME_DROIDDISEMBARK);
	uint32_t player = psTransporter->player;
	uint32_t droidId = psDroid->id;
	uint32_t transportId = psTransporter->id;

	NETuint32_t(w, player);
	NETuint32_t(w, droidId);
	NETuint32_t(w, transportId);

	return NETend(w);
}

/** Receive info about a droid that is being unloaded from a transporter
 */
bool recvDroidDisEmbark(NETQUEUE queue)
{
	DROID *psFoundDroid = nullptr, *psTransporterDroid = nullptr;

	auto r = NETbeginDecode(queue, GAME_DROIDDISEMBARK);
	{
		uint32_t player;
		uint32_t droidID;
		uint32_t transporterID;

		NETuint32_t(r, player);
		NETuint32_t(r, droidID);
		NETuint32_t(r, transporterID);

		bool validMessage = NETend(r);

		// find the transporter first
		psTransporterDroid = validMessage ? IdToDroid(gameWorld.objects, transporterID, player) : nullptr;
		if (validMessage && !psTransporterDroid)
		{
			// Possible it already died
			syncDebug("Transporter %u of player %u not found.", transporterID, player);
			return false;
		}
		if (!validMessage || !canGiveOrdersFor(queue.index, psTransporterDroid->player) || !psTransporterDroid->isTransporter() || droidID == transporterID)
		{
			if (recordInvalidMessage(queue.index, GAME_DROIDDISEMBARK))
			{
				debug(LOG_INFO, "Ignoring invalid GAME_DROIDDISEMBARK from %d (droid %u, transporter %u) - further ones will not be logged", (int)queue.index, droidID, transporterID);
			}
			return false;
		}
		// we need to find the droid *in* the transporter
		if (!transporterFlying(psTransporterDroid) && psTransporterDroid->psGroup)
		{
			const auto& groupList = psTransporterDroid->psGroup->psList;
			auto it = std::find_if(groupList.begin(), groupList.end(),
				[droidID](DROID* d)
				{
					return d->id == droidID;
				});
			if (it != groupList.end() && *it != psTransporterDroid)
			{
				psFoundDroid = *it;
			}
		}
		// don't continue if we couldn't find it.
		if (!psFoundDroid)
		{
			// Happens if the droid was already unloaded, or the transporter took off, before this was processed
			syncDebug("Droid %u not found in transporter %u.", droidID, transporterID);
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
bool SendDroid(const DROID_TEMPLATE *pTemplate, uint32_t x, uint32_t y, uint8_t player, uint32_t id, const INITIAL_DROID_ORDERS *initialOrdersP)
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
	auto w = NETbeginEncode(NETgameQueue(realSelectedPlayer), GAME_DEBUG_ADD_DROID);
	{
		Position pos(x, y, 0);
		bool haveInitialOrders = initialOrdersP != nullptr;
		int32_t droidType = pTemplate->droidType;

		NETuint8_t(w, player);
		NETuint32_t(w, id);
		NETPosition(w, pos);
		NETwzstring(w, pTemplate->name);
		NETint32_t(w, droidType);
		NETuint8_t(w, pTemplate->asParts[COMP_BODY]);
		NETuint8_t(w, pTemplate->asParts[COMP_BRAIN]);
		NETuint8_t(w, pTemplate->asParts[COMP_PROPULSION]);
		NETuint8_t(w, pTemplate->asParts[COMP_REPAIRUNIT]);
		NETuint8_t(w, pTemplate->asParts[COMP_ECM]);
		NETuint8_t(w, pTemplate->asParts[COMP_SENSOR]);
		NETuint8_t(w, pTemplate->asParts[COMP_CONSTRUCT]);
		NETint8_t(w, pTemplate->numWeaps);
		for (int i = 0; i < pTemplate->numWeaps; i++)
		{
			NETuint32_t(w, pTemplate->asWeaps[i]);
		}
		NETbool(w, haveInitialOrders);
		if (haveInitialOrders)
		{
			INITIAL_DROID_ORDERS initialOrders = *initialOrdersP;
			NETuint32_t(w, initialOrders.secondaryOrder);
			NETint32_t(w, initialOrders.moveToX);
			NETint32_t(w, initialOrders.moveToY);
			NETuint32_t(w, initialOrders.factoryId);  // For making scripts happy.
		}
	}
	debug(LOG_LIFE, "===> sending Droid from %u id of %u ", player, id);
	return NETend(w);
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
	bool validTemplate = false;
	INITIAL_DROID_ORDERS initialOrders = { 0, 0, 0, 0 };

	auto r = NETbeginDecode(queue, GAME_DEBUG_ADD_DROID);
	{
		int32_t droidType = 0;

		NETuint8_t(r, player);
		NETuint32_t(r, id);
		NETPosition(r, pos);
		WzString name;
		NETwzstring(r, name);
		pT->name = name;
		pT->id = pT->name;
		NETint32_t(r, droidType);
		NETuint8_t(r, pT->asParts[COMP_BODY]);
		NETuint8_t(r, pT->asParts[COMP_BRAIN]);
		NETuint8_t(r, pT->asParts[COMP_PROPULSION]);
		NETuint8_t(r, pT->asParts[COMP_REPAIRUNIT]);
		NETuint8_t(r, pT->asParts[COMP_ECM]);
		NETuint8_t(r, pT->asParts[COMP_SENSOR]);
		NETuint8_t(r, pT->asParts[COMP_CONSTRUCT]);
		NETint8_t(r, pT->numWeaps);
		validTemplate = pT->numWeaps >= 0 && pT->numWeaps <= ARRAY_SIZE(pT->asWeaps) && droidType >= 0 && droidType < DROID_ANY;
		if (validTemplate)
		{
			for (int i = 0; i < pT->numWeaps; i++)
			{
				NETuint32_t(r, pT->asWeaps[i]);
				validTemplate = validTemplate && pT->asWeaps[i] < compStatCount(COMP_WEAPON);
			}
			NETbool(r, haveInitialOrders);
			if (haveInitialOrders)
			{
				NETuint32_t(r, initialOrders.secondaryOrder);
				NETint32_t(r, initialOrders.moveToX);
				NETint32_t(r, initialOrders.moveToY);
				NETuint32_t(r, initialOrders.factoryId);  // For making scripts happy.
			}
			for (unsigned comp = 0; comp < DROID_MAXCOMP; ++comp)
			{
				validTemplate = validTemplate && pT->asParts[comp] < compStatCount(comp);
			}
		}
		pT->droidType = (DROID_TYPE)droidType;
	}
	bool validMessage = NETend(r);

	const DebugInputManager& dbgInputManager = gInputManager.debugManager();
	bool debugAllowed = dbgInputManager.debugMappingsAllowed() || !bMultiPlayer;
	bool validPos = worldOnMap(gameWorld.map, pos.x, pos.y) && !(pos.x == 0 && pos.y == 0);
	if (!validMessage || !validTemplate || !validPos || player >= MAX_PLAYERS || !debugAllowed)
	{
		if (recordInvalidMessage(queue.index, GAME_DEBUG_ADD_DROID))
		{
			debug(LOG_INFO, "Ignoring invalid GAME_DEBUG_ADD_DROID from %d (valid: %d, template: %d, pos: (%d, %d), player: %d, debug allowed: %d) - further invalid ones will not be logged",
			      (int)queue.index, (int)validMessage, (int)validTemplate, (int)pos.x, (int)pos.y, (int)player, (int)debugAllowed);
		}
		return false;
	}

	debug(LOG_LIFE, "<=== getting Droid from %u id of %u ", player, id);

	// Create that droid on this machine.
	const auto rot = Rotation();
	psDroid = reallyBuildDroid(gameWorld, pT, pos, player, false, rot, id);

	// If we were able to build the droid set it up
	if (psDroid)
	{
		limitCommanderExpForProduction(psDroid);

		addDroid(psDroid, gameWorld.objects.droids);

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
		CONPRINTF("MULTIPLAYER: Couldn't build a remote droid, relying on checking to resync");
#endif
		return false;
	}

	return true;
}


// Actually send the droid info.
void sendQueuedDroidInfo()
{
	// Given an order type, we bring all other orders of the same type together
	// WHILE KEEPING their relative order!! This is important because
	// sending all MOVE and then all HOLD_POSITION is clearly not the same as
	// sending all HOLD_POSITION and then MOVE
	// yes, we need an ordered map here, not std::sort
	static std::map<DROID_ORDER, std::vector<QueuedDroidInfo> > orderedMap; // static to avoid allocations
	orderedMap.clear();
	for (auto &info: queuedOrders)
	{
		orderedMap[info.order].push_back(info);
	}
	std::vector<QueuedDroidInfo>::const_iterator eqBegin, eqEnd;
	for (auto &pair: orderedMap)
	{
		const auto& qOrders = pair.second;
		for (eqBegin = qOrders.begin(); eqBegin != qOrders.end(); eqBegin = eqEnd)
		{
			// Find end of range of orders which differ only by the droid ID.
			for (eqEnd = eqBegin + 1; eqEnd != qOrders.end() && eqEnd->orderCompare(*eqBegin) == 0; ++eqEnd)
			{}

			// A range that would not fit in one message is split into several messages.
			for (auto chunkBegin = eqBegin; chunkBegin != eqEnd;)
			{
				auto w = NETbeginEncode(NETgameQueue(realSelectedPlayer), GAME_DROIDINFO);
				NETQueuedDroidInfo(w, *eqBegin);

				// The count and each droid ID delta encode to at most 5 bytes.
				const size_t headerBytes = w.msgBuilder.payloadSize();
				ASSERT(headerBytes + 2 * DroidInfoMaxVarintBytes <= DroidInfoMaxPayloadBytes, "GAME_DROIDINFO header too large: %zu", headerBytes);
				const size_t maxIds = (DroidInfoMaxPayloadBytes - headerBytes - DroidInfoMaxVarintBytes) / DroidInfoMaxVarintBytes;
				const uint32_t num = static_cast<uint32_t>(std::min<size_t>(eqEnd - chunkBegin, std::max<size_t>(maxIds, 1)));
				NETuint32_t(w, num);

				uint32_t prevDroidId = 0;
				for (unsigned n = 0; n < num; ++n)
				{
					uint32_t droidId = (chunkBegin + n)->droidId;

					// Encode deltas between droid IDs, since the deltas are smaller than the actual droid IDs, and will encode to less bytes on average.
					uint32_t deltaDroidId = droidId - prevDroidId;
					NETuint32_t(w, deltaDroidId);

					prevDroidId = droidId;
				}
				NETend(w);
				chunkBegin += num;
			}
		}
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
void sendDroidInfo(DROID *psDroid, DroidOrder const &order, bool add, const OrderSource &source)
{
	if (!myResponsibility(psDroid->player))
	{
		return;
	}
	if (NETisReplay())
	{
		return;
	}

	// Check before orderDroidAddPending() below, so an invalid order does not
	// leave a phantom waypoint drawn on the issuing client's own screen.
	if (!orderSourcePermitsPlayerAction(psDroid->player, source))
	{
		orderProvenanceRecord(psDroid->player, source.origin(), true);
		debug(LOG_NET, "Invalid droid order for player %u from %s", psDroid->player, source.toDescription().c_str());
		return;
	}
	orderProvenanceRecord(psDroid->player, source.origin(), false);

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
		if (!psDroid->isConstructionDroid())
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

	applyOrderSource(info, source);

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

static constexpr int MaxQueuedDroidOrders = 512;

static bool validOrderForNetLoc(DROID_ORDER order)
{
	switch (order)
	{
	case DORDER_STOP:
	case DORDER_HOLD:
	case DORDER_RTB:
	case DORDER_RTR:
	case DORDER_RECYCLE:
	case DORDER_BUILD:
	case DORDER_LINEBUILD:
		return true;
	default:
		return validOrderForLoc(order);
	}
}

// Must match orderDroidList()
static bool validOrderForNetQueue(DROID_ORDER order)
{
	switch (order)
	{
	case DORDER_MOVE:
	case DORDER_SCOUT:
	case DORDER_DISEMBARK:
	case DORDER_ATTACK:
	case DORDER_REPAIR:
	case DORDER_OBSERVE:
	case DORDER_DROIDREPAIR:
	case DORDER_FIRESUPPORT:
	case DORDER_DEMOLISH:
	case DORDER_HELPBUILD:
	case DORDER_BUILDMODULE:
	case DORDER_RECOVER:
	case DORDER_BUILD:
	case DORDER_LINEBUILD:
		return true;
	default:
		return false;
	}
}

static uint32_t secondaryStateMask(SECONDARY_ORDER sec)
{
	switch (sec)
	{
	case DSO_ATTACK_RANGE: return DSS_ARANGE_MASK;
	case DSO_REPAIR_LEVEL: return DSS_REPLEV_MASK;
	case DSO_ATTACK_LEVEL: return DSS_ALEV_MASK;
	case DSO_ASSIGN_PRODUCTION:
	case DSO_ASSIGN_CYBORG_PRODUCTION:
	case DSO_ASSIGN_VTOL_PRODUCTION:
	case DSO_CLEAR_PRODUCTION: return DSS_ASSPROD_MASK;
	case DSO_RECYCLE: return DSS_RECYCLE_MASK;
	case DSO_PATROL: return DSS_PATROL_MASK;
	case DSO_HALTTYPE: return DSS_HALT_MASK;
	case DSO_RETURN_TO_LOC: return DSS_RTL_MASK;
	case DSO_FIRE_DESIGNATOR: return DSS_FIREDES_MASK;
	case DSO_CIRCLE: return DSS_CIRCLE_MASK;
	case DSO_ACCEPT_RETREP: return DSS_ACCREP_MASK;
	case DSO_UNUSED: break;
	}
	return 0;
}

static bool validDroidInfoOrderType(QueuedDroidInfo const &info)
{
	switch (info.subType)
	{
	case ObjOrder:
		if (!validOrderForObj(info.order) || (info.destType != OBJ_DROID && info.destType != OBJ_STRUCTURE && info.destType != OBJ_FEATURE))
		{
			return false;
		}
		break;
	case LocOrder:
		if (!validOrderForNetLoc(info.order))
		{
			return false;
		}
		break;
	case SecondaryOrder:
	{
		uint32_t mask = secondaryStateMask(info.secOrder);
		return mask != 0 && (static_cast<uint32_t>(info.secState) & ~mask) == 0;
	}
	default:
		return false;
	}
	if (info.add && !validOrderForNetQueue(info.order))
	{
		return false;
	}
	return true;
}

static bool validDroidInfoOrderData(QueuedDroidInfo const &info, DROID_ORDER_DATA const &sOrder)
{
	if (info.subType == SecondaryOrder)
	{
		return true;
	}
	if (info.subType == ObjOrder && sOrder.psObj == nullptr)
	{
		return false;
	}
	if ((info.order == DORDER_BUILD || info.order == DORDER_LINEBUILD) && (sOrder.psStats == nullptr || sOrder.psStats->type == REF_DEMOLISH))
	{
		return false;
	}
	if (sOrder.psObj != TargetMissing && !validTargetForOrder(info.order, sOrder.psObj))
	{
		return false;
	}
	if ((info.order == DORDER_BUILD || info.order == DORDER_LINEBUILD) && !worldOnMap(gameWorld.map, sOrder.pos))
	{
		return false;
	}
	if (info.order == DORDER_LINEBUILD && !worldOnMap(gameWorld.map, sOrder.pos2))
	{
		return false;
	}
	if (sOrder.psObj != TargetMissing && sOrder.psObj != nullptr)
	{
		switch (info.order)
		{
		case DORDER_FIRESUPPORT:
			return sOrder.psObj->type == OBJ_DROID || sOrder.psObj->type == OBJ_STRUCTURE;
		case DORDER_EMBARK:
			return castDroid(sOrder.psObj)->isTransporter();
		case DORDER_COMMANDERSUPPORT:
			return castDroid(sOrder.psObj)->droidType == DROID_COMMAND;
		case DORDER_BUILDMODULE:
			return getModuleStat(static_cast<const STRUCTURE *>(sOrder.psObj)) != nullptr;
		default:
			break;
		}
	}
	return true;
}

// Ownership and alliances can change while an order is in flight, so these aren't logged
static bool droidInfoOrderTargetApplies(QueuedDroidInfo const &info, DROID_ORDER_DATA const &sOrder)
{
	if (sOrder.psObj == TargetMissing || sOrder.psObj == nullptr)
	{
		return true;
	}
	switch (info.order)
	{
	case DORDER_FIRESUPPORT:
		return aiCheckAlliances(sOrder.psObj->player, info.player);
	case DORDER_EMBARK:
	case DORDER_COMMANDERSUPPORT:
		return sOrder.psObj->player == info.player;
	case DORDER_ATTACK:
	case DORDER_ATTACKTARGET:
		return !bMultiPlayer
		       || (sOrder.psObj->type != OBJ_DROID && sOrder.psObj->type != OBJ_STRUCTURE)
		       || sOrder.psObj->player == info.player
		       || !aiCheckAlliances(sOrder.psObj->player, info.player);
	default:
		return true;
	}
}

// Whether this order can be given to this droid
static bool validDroidInfoOrderForDroid(QueuedDroidInfo const &info, DROID_ORDER_DATA const &sOrder, DROID const *psDroid)
{
	if (info.subType == SecondaryOrder)
	{
		if (psDroid->isTransporter())
		{
			const uint32_t secState = static_cast<uint32_t>(info.secState);
			if ((info.secOrder == DSO_RETURN_TO_LOC && (secState & DSS_RTL_MASK) == DSS_RTL_TRANSPORT)
			    || (info.secOrder == DSO_RECYCLE && (secState & DSS_RECYCLE_MASK) != 0))
			{
				return false;
			}
		}
		return true;
	}
	if ((info.order == DORDER_BUILD || info.order == DORDER_LINEBUILD || info.order == DORDER_HELPBUILD) && !psDroid->isConstructionDroid())
	{
		return false;
	}
	if ((info.order == DORDER_EMBARK || info.order == DORDER_COMMANDERSUPPORT || info.order == DORDER_FIRESUPPORT) && psDroid->isTransporter())
	{
		return false;
	}
	if ((info.order == DORDER_TRANSPORTOUT || info.order == DORDER_TRANSPORTIN || info.order == DORDER_TRANSPORTRETURN) && !psDroid->isTransporter())
	{
		return false;
	}
	if ((info.order == DORDER_TRANSPORTOUT || info.order == DORDER_TRANSPORTIN || info.order == DORDER_TRANSPORTRETURN) && bMultiPlayer)
	{
		return false;
	}
	if (info.order == DORDER_COMMANDERSUPPORT && psDroid->droidType == DROID_COMMAND)
	{
		return false;
	}
	if (sOrder.psStats != nullptr)
	{
		const UBYTE availability = apStructTypeLists[psDroid->player][sOrder.psStats - asStructureStats];
		return availability == AVAILABLE || availability == REDUNDANT;
	}
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// receive droid information form other players.
bool recvDroidInfo(NETQUEUE queue)
{
	auto r = NETbeginDecode(queue, GAME_DROIDINFO);
	{
		QueuedDroidInfo info;
		NETQueuedDroidInfo(r, info);

		orderProvenanceRecordReported(info.player, static_cast<OrderOrigin>(info.provenance.origin));

		uint32_t num = 0;
		if (!NETcount(r, num, MaxDroidInfoDroids) || !validDroidInfoOrderType(info))
		{
			if (recordInvalidMessage(queue.index, GAME_DROIDINFO))
			{
				debug(LOG_INFO, "Ignoring invalid GAME_DROIDINFO from %d (subType: %d, order: %d, add: %d) - further ones will not be logged", (int)queue.index, (int)info.subType, (int)info.order, (int)info.add);
			}
			syncDebug("Invalid order type.");
			NETend(r);
			return false;
		}

		if (!canGiveOrdersFor(queue.index, info.player))
		{
			if (recordInvalidMessage(queue.index, GAME_DROIDINFO))
			{
				debug(LOG_INFO, "Ignoring GAME_DROIDINFO from %d for player %d - further invalid ones will not be logged", (int)queue.index, (int)info.player);
			}
			syncDebug("Wrong player.");
			NETend(r);
			return false;
		}

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

		if (!validDroidInfoOrderData(info, sOrder))
		{
			if (recordInvalidMessage(queue.index, GAME_DROIDINFO))
			{
				debug(LOG_INFO, "Ignoring invalid GAME_DROIDINFO from %d (order: %s, target: %s, stats: %s) - further ones will not be logged", (int)queue.index, getDroidOrderName(info.order),
				      (sOrder.psObj != nullptr && sOrder.psObj != TargetMissing) ? objInfo(sOrder.psObj) : "none", (psStats != nullptr) ? getStatsName(psStats) : "none");
			}
			syncDebug("Invalid order data.");
			NETend(r);
			return false;
		}
		if (!droidInfoOrderTargetApplies(info, sOrder))
		{
			syncDebug("Order target no longer applies.");
			NETend(r);
			return false;
		}

		for (uint32_t n = 0; n < num; ++n)
		{
			// Get the next droid ID which is being given this order.
			uint32_t deltaDroidId = 0;
			NETuint32_t(r, deltaDroidId);
			if (!r.valid())
			{
				if (recordInvalidMessage(queue.index, GAME_DROIDINFO))
				{
					debug(LOG_INFO, "Truncated GAME_DROIDINFO from %d (expected %" PRIu32 " droids, got %" PRIu32 ") - further invalid ones will not be logged", (int)queue.index, num, n);
				}
				syncDebug("Truncated.");
				break;
			}
			info.droidId += deltaDroidId;

			DROID *psDroid = IdToDroid(gameWorld.objects, info.droidId, info.player);
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
			if (!validDroidInfoOrderForDroid(info, sOrder, psDroid))
			{
				if (recordInvalidMessage(queue.index, GAME_DROIDINFO))
				{
					debug(LOG_INFO, "Ignoring GAME_DROIDINFO from %d (order: %s, stats: %s) for %s - further invalid ones will not be logged", (int)queue.index, getDroidOrderName(info.order),
					      (sOrder.psStats != nullptr) ? getStatsName(sOrder.psStats) : "none", objInfo(psDroid));
				}
				syncDebug("Invalid order for droid.");
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
				if (hasCommander(psDroid)
					&& info.order != DORDER_RTR
					&& info.order != DORDER_RTR_SPECIFIED)
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
					else if (psDroid->listSize < MaxQueuedDroidOrders)
					{
						orderDroidAdd(psDroid, &sOrder);   // Add the order to the (non-pending) list. Will probably overwrite the corresponding pending order, assuming all pending orders were written to the list.
					}
					else
					{
						syncDebug("Order list full.");
					}
				}
				break;
			case SecondaryOrder:
				// Set the droids secondary order
				turnOffMultiMsg(true);
				secondarySetState(psDroid, gameWorld.objects, info.secOrder, info.secState);
				turnOffMultiMsg(false);
				break;
			}

			syncDebugDroid(psDroid, '>');

			CHECK_DROID(psDroid);
		}
	}
	return NETend(r);
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
			psObj = IdToDroid(gameWorld.objects, destid, ANYPLAYER);
			break;
		case OBJ_STRUCTURE:
			psObj = IdToStruct(destid, ANYPLAYER);
			break;
		case OBJ_FEATURE:
			psObj = IdToFeature(gameWorld.objects, destid, ANYPLAYER);
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
	auto w = NETbeginEncode(NETgameQueue(realSelectedPlayer), GAME_DEBUG_REMOVE_DROID);
	{
		uint32_t id = psDroid->id;

		// Send the droid's ID
		debug(LOG_DEATH, "Requested all players to destroy droid %u", (unsigned int)id);
		NETuint32_t(w, id);
	}
	return NETend(w);
}

// ////////////////////////////////////////////////////////////////////////////
// Accept a droid which was destroyed on another machine
bool recvDestroyDroid(NETQUEUE queue)
{
	uint32_t id = 0;

	auto r = NETbeginDecode(queue, GAME_DEBUG_REMOVE_DROID);
	NETuint32_t(r, id);
	if (!NETend(r))
	{
		if (recordInvalidMessage(queue.index, GAME_DEBUG_REMOVE_DROID))
		{
			debug(LOG_INFO, "Ignoring truncated GAME_DEBUG_REMOVE_DROID from %d - further invalid ones will not be logged", (int)queue.index);
		}
		return false;
	}

	// Retrieve the droid
	DROID *psDroid = IdToDroid(gameWorld.objects, id, ANYPLAYER);
	if (!psDroid)
	{
		debug(LOG_DEATH, "droid %d on request from player %d can't be found? Must be dead already?",
		      id, queue.index);
		return false;
	}

	const DebugInputManager& dbgInputManager = gInputManager.debugManager();
	if (!dbgInputManager.debugMappingsAllowed() && bMultiPlayer)
	{
		debug(LOG_WARNING, "Failed to remove droid for player %u.", NetPlay.players[queue.index].position);
		return false;
	}

	// If the droid has not died on our machine yet, destroy it
	if (!psDroid->died)
	{
		turnOffMultiMsg(true);
		debug(LOG_DEATH, "Killing droid %d on request from player %d - huh?", psDroid->id, queue.index);
		destroyDroid(psDroid, gameTime - deltaGameTime + 1, gameWorld);  // deltaGameTime is actually 0 here, since we're between updates. However, the value of gameTime - deltaGameTime + 1 will not change when we start the next tick.
		turnOffMultiMsg(false);
	}
	else
	{
		debug(LOG_DEATH, "droid %d is confirmed dead by player %d.", psDroid->id, queue.index);
	}

	return true;
}
