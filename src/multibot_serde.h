/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
 * @file multibot_serde.h
 *
 * Wire representation of GAME_DROIDINFO.
 * Contains only the on-the-wire shape and its (de)serialization, with no dependency on live game object lists.
 *
 * NOTE: any change here is a NETWORK PROTOCOL CHANGE and also a REPLAY FORMAT CHANGE.
 * See lib/netplay/netreplay.cpp (currentReplayFormatVer).
 */

#ifndef __INCLUDED_SRC_MULTIBOT_SERDE_H__
#define __INCLUDED_SRC_MULTIBOT_SERDE_H__

#include "lib/framework/frame.h"
#include "lib/framework/vector.h"
#include "lib/netplay/nettypes.h"

#include "basedef.h"
#include "orderdef.h"
#include "ordersource_wire.h"

#include <type_traits>

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
		// Provenance participates too, so a group never mixes two origins.
		const int provComp = provenance.compare(z.provenance);
		if (provComp != 0)
		{
			return provComp;
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

	/// Provenance - see ordersource_wire.h.
	OrderProvenanceWire provenance;
};

template <typename SerdeContext, typename T>
struct SerdeFnArgT
{
	using reference_type = std::conditional_t<std::is_same<SerdeContext, MessageReader>::value, T&, const T&>;
};

/// Does not read/write info->droidId!
template <typename SerdeContext>
inline void NETQueuedDroidInfo(SerdeContext& c, typename SerdeFnArgT<SerdeContext, QueuedDroidInfo>::reference_type info)
{
	static_assert(std::is_same<SerdeContext, MessageReader>::value || std::is_same<SerdeContext, MessageWriter>::value,
		"SerdeContext is expected to be either MessageReader or MessageWriter");

	NETuint8_t(c, info.player);
	NETenum(c, info.subType);
	switch (info.subType)
	{
	case ObjOrder:
	case LocOrder:
		NETenum(c, info.order);
		if (info.subType == ObjOrder)
		{
			NETuint32_t(c, info.destId);
			NETenum(c, info.destType);
		}
		else
		{
			NETVector2i(c, info.pos);
		}
		if (info.order == DORDER_BUILD || info.order == DORDER_LINEBUILD)
		{
			NETuint32_t(c, info.structRef);
			NETuint16_t(c, info.direction);
		}
		if (info.order == DORDER_LINEBUILD)
		{
			NETVector2i(c, info.pos2);
		}
		if (info.order == DORDER_BUILDMODULE)
		{
			NETuint32_t(c, info.index);
		}
		NETbool(c, info.add);
		break;
	case SecondaryOrder:
		NETenum(c, info.secOrder);
		NETenum(c, info.secState);
		break;
	}

	NETOrderProvenance(c, info.provenance);
}

#endif // __INCLUDED_SRC_MULTIBOT_SERDE_H__
