// SPDX-License-Identifier: GPL-2.0-or-later

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
 * @file ordersource_wire.h
 *
 * The on-the-wire form of order provenance, shared by GAME_DROIDINFO, GAME_RESEARCHSTATUS and
 * GAME_STRUCTUREINFO. Kept separate from ordersource.h so the token has no netplay dependency.
 */

#ifndef __INCLUDED_SRC_ORDERSOURCE_WIRE_H__
#define __INCLUDED_SRC_ORDERSOURCE_WIRE_H__

#include "lib/netplay/nettypes.h"

#include "ordersource.h"

#include <cstdint>

struct OrderProvenanceWire
{
	uint8_t  origin = 0;          // an OrderOrigin, validated on decode
	bool     hasViewPos = false;
	uint16_t viewX = 0;
	uint16_t viewY = 0;

	bool operator ==(const OrderProvenanceWire &z) const
	{
		return origin == z.origin && hasViewPos == z.hasViewPos
		       && (!hasViewPos || (viewX == z.viewX && viewY == z.viewY));
	}
	bool operator !=(const OrderProvenanceWire &z) const { return !(*this == z); }
	// Total order, so callers that group messages can compare cheaply.
	int compare(const OrderProvenanceWire &z) const
	{
		if (origin != z.origin) { return origin < z.origin ? -1 : 1; }
		if (hasViewPos != z.hasViewPos) { return hasViewPos < z.hasViewPos ? -1 : 1; }
		if (hasViewPos)
		{
			if (viewX != z.viewX) { return viewX < z.viewX ? -1 : 1; }
			if (viewY != z.viewY) { return viewY < z.viewY ? -1 : 1; }
		}
		return 0;
	}
};

// Bit layout of the provenance byte. Bits 4-7 are reserved and must be zero. A decoder ignores them if set.
constexpr uint8_t ORDER_PROVENANCE_ORIGIN_MASK = 0x07;
constexpr uint8_t ORDER_PROVENANCE_HAS_VIEWPOS = 0x08;

inline OrderProvenanceWire orderProvenanceFromSource(const OrderSource &source)
{
	OrderProvenanceWire p;
	p.origin = static_cast<uint8_t>(source.origin()) & ORDER_PROVENANCE_ORIGIN_MASK;
	const InputEventInfo &w = source.eventInfo();
	p.hasViewPos = w.hasViewPos;
	p.viewX = w.viewX;
	p.viewY = w.viewY;
	return p;
}

inline OrderSource orderProvenanceToSource(const OrderProvenanceWire &p)
{
	return OrderSource::fromWire(static_cast<OrderOrigin>(p.origin), p.hasViewPos, p.viewX, p.viewY);
}

inline void NETOrderProvenance(MessageWriter &w, const OrderProvenanceWire &p)
{
	uint8_t packed = p.origin & ORDER_PROVENANCE_ORIGIN_MASK;
	if (p.hasViewPos)
	{
		packed |= ORDER_PROVENANCE_HAS_VIEWPOS;
	}
	NETuint8_t(w, packed);
	if (p.hasViewPos)
	{
		uint16_t x = p.viewX, y = p.viewY;
		NETuint16_t(w, x);
		NETuint16_t(w, y);
	}
}

inline void NETOrderProvenance(MessageReader &r, OrderProvenanceWire &p)
{
	uint8_t packed = 0;
	NETuint8_t(r, packed);
	const uint8_t rawOrigin = packed & ORDER_PROVENANCE_ORIGIN_MASK;
	p.origin = (rawOrigin < static_cast<uint8_t>(OrderOrigin::Count_))
	           ? rawOrigin : static_cast<uint8_t>(OrderOrigin::Simulation);
	p.hasViewPos = (packed & ORDER_PROVENANCE_HAS_VIEWPOS) != 0;
	if (p.hasViewPos)
	{
		NETuint16_t(r, p.viewX);
		NETuint16_t(r, p.viewY);
	}
}

#endif // __INCLUDED_SRC_ORDERSOURCE_WIRE_H__
