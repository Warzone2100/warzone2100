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
 * @file ordersource.cpp
 */

#include "ordersource.h"

#include "lib/framework/frame.h"
#include "lib/gamelib/gtime.h"
#include "lib/netplay/netplay.h"

#include "multiplay.h"

#include <algorithm>
#include <array>
#include <vector>

// ---------------------------------------------------------------------------

const char *toString(OrderOrigin origin)
{
	switch (origin)
	{
	case OrderOrigin::Simulation: return "simulation";
	case OrderOrigin::UiPointer:  return "ui-pointer";
	case OrderOrigin::UiKeybind:  return "ui-keybind";
	case OrderOrigin::UiWidget:   return "ui-widget";
	case OrderOrigin::Script:     return "script";
	case OrderOrigin::Count_:     break;
	}
	return "unknown";
}

OrderSource::OrderSource(OrderOrigin origin, const char *reason)
	: m_origin(origin)
	, m_reason(reason != nullptr ? reason : "")
{
}

OrderSource OrderSource::simulation(const char *reason)
{
	return OrderSource(OrderOrigin::Simulation, reason);
}

OrderSource OrderSource::script(int scriptPlayer, bool hostDeclared)
{
	OrderSource s(OrderOrigin::Script, "script");
	s.m_scriptPlayer = scriptPlayer;
	s.m_scriptHostDeclared = hostDeclared;
	return s;
}

OrderSource OrderSource::pointer(const InputEventInfo &eventInfo)
{
	OrderSource s(OrderOrigin::UiPointer, "pointer");
	s.m_eventInfo = eventInfo;
	return s;
}

OrderSource OrderSource::keybind(const InputEventInfo &eventInfo)
{
	OrderSource s(OrderOrigin::UiKeybind, "keybind");
	s.m_eventInfo = eventInfo;
	return s;
}

OrderSource OrderSource::widget()
{
	return OrderSource(OrderOrigin::UiWidget, "widget");
}

OrderSource OrderSource::widgetAt(uint16_t viewX, uint16_t viewY)
{
	OrderSource s(OrderOrigin::UiWidget, "widget");
	s.m_eventInfo.hasViewPos = true;
	s.m_eventInfo.viewX = viewX;
	s.m_eventInfo.viewY = viewY;
	return s;
}

OrderSource OrderSource::fromWire(OrderOrigin origin, bool hasViewPos, uint16_t viewX, uint16_t viewY)
{
	if (origin >= OrderOrigin::Count_)
	{
		origin = OrderOrigin::Simulation;
	}
	OrderSource s(origin, "wire");
	s.m_eventInfo.hasViewPos = hasViewPos;
	s.m_eventInfo.viewX = viewX;
	s.m_eventInfo.viewY = viewY;
	return s;
}

std::string OrderSource::toDescription() const
{
	std::string out = toString(m_origin);
	if (m_origin == OrderOrigin::Script)
	{
		out += astringf("(player %d,%s)", m_scriptPlayer,
		                m_scriptHostDeclared ? "host-declared" : "player-script");
	}
	else if (m_eventInfo.valid())
	{
		out += astringf("(#%" PRIu64 " @%" PRIu32 "ms", m_eventInfo.serial, m_eventInfo.realTimeMs);
		if (m_eventInfo.hasViewPos)
		{
			out += astringf(" view %" PRIu32 ",%" PRIu32, static_cast<uint32_t>(m_eventInfo.viewX), static_cast<uint32_t>(m_eventInfo.viewY));
		}
		out += ")";
	}
	else if (m_eventInfo.hasViewPos)
	{
		out += astringf("(view %" PRIu32 ",%" PRIu32 ")", static_cast<uint32_t>(m_eventInfo.viewX), static_cast<uint32_t>(m_eventInfo.viewY));
	}
	else if (m_reason[0] != '\0')
	{
		out += astringf("(%s)", m_reason);
	}
	return out;
}

// ---------------------------------------------------------------------------
// MARK: - Ambient scope
// ---------------------------------------------------------------------------

namespace
{

/// The fallback when no scope is active.
const OrderSource &ambientDefault()
{
	static const OrderSource s = OrderSource::simulation("ambient-default");
	return s;
}

std::vector<OrderSource> &sourceStack()
{
	static std::vector<OrderSource> stack;
	return stack;
}

} // namespace

OrderSourceScope::OrderSourceScope(const OrderSource &source)
{
	sourceStack().push_back(source);
	m_depth = sourceStack().size();
}

OrderSourceScope::~OrderSourceScope()
{
	// Tolerate mismatched nesting rather than corrupting the stack: if something
	// unwound past us (ex: an exception through script code), leave what's
	// there alone instead of popping someone else's entry.
	if (sourceStack().size() == m_depth)
	{
		sourceStack().pop_back();
	}
	else
	{
		debug(LOG_ERROR, "OrderSourceScope unbalanced: depth %zu, stack %zu",
		      m_depth, sourceStack().size());
	}
}

const OrderSource &currentOrderSource()
{
	const auto &stack = sourceStack();
	return stack.empty() ? ambientDefault() : stack.back();
}

void orderSourceReset()
{
	sourceStack().clear();
}

// ---------------------------------------------------------------------------
// MARK: - Input event records
// ---------------------------------------------------------------------------

InputEventInfo mintInputEventInfo(int32_t screenX, int32_t screenY, int32_t worldX, int32_t worldY)
{
	static uint64_t nextSerial = 1;

	InputEventInfo w;
	w.serial = nextSerial++;
	w.realTimeMs = realTime;
	w.screenX = screenX;
	w.screenY = screenY;
	w.worldX = worldX;
	w.worldY = worldY;
	return w;
}

bool orderSourceNormalizeViewPos(int32_t screenX, int32_t screenY,
                                 int32_t viewportWidth, int32_t viewportHeight,
                                 uint16_t &outX, uint16_t &outY)
{
	if (viewportWidth <= 0 || viewportHeight <= 0 || screenX < 0 || screenY < 0)
	{
		return false;
	}
	const int64_t nx = (static_cast<int64_t>(screenX) * 65535) / viewportWidth;
	const int64_t ny = (static_cast<int64_t>(screenY) * 65535) / viewportHeight;
	// Clamp: the cursor can sit slightly outside the buffer on some platforms.
	outX = static_cast<uint16_t>(std::min<int64_t>(std::max<int64_t>(nx, 0), 65535));
	outY = static_cast<uint16_t>(std::min<int64_t>(std::max<int64_t>(ny, 0), 65535));
	return true;
}

InputEventInfo mintPointerEventInfo(int32_t screenX, int32_t screenY,
                                int32_t viewportWidth, int32_t viewportHeight,
                                int32_t worldX, int32_t worldY)
{
	InputEventInfo w = mintInputEventInfo(screenX, screenY, worldX, worldY);
	w.hasViewPos = orderSourceNormalizeViewPos(screenX, screenY, viewportWidth, viewportHeight,
	                                           w.viewX, w.viewY);
	return w;
}

// ---------------------------------------------------------------------------
// MARK: - Policy
// ---------------------------------------------------------------------------

bool orderSourcePermitsPlayerAction(unsigned targetPlayer, const OrderSource &source)
{
	if (!source.isScript())
	{
		return true;
	}
	const bool bIsTrueMultiplayerGame = bMultiPlayer && NetPlay.bComms;
	if (!bIsTrueMultiplayerGame)
	{
		return true;
	}
	if (source.scriptIsHostDeclared())
	{
		return true;
	}
	if (targetPlayer >= NetPlay.players.size())
	{
		return true;
	}
	return !NetPlay.players[targetPlayer].allocated;
}

// ---------------------------------------------------------------------------
// MARK: - Provenance tally
// ---------------------------------------------------------------------------

namespace
{

struct PlayerTally
{
	std::array<uint64_t, static_cast<size_t>(OrderOrigin::Count_)> issued{};
	std::array<uint64_t, static_cast<size_t>(OrderOrigin::Count_)> refused{};
	/// What arrived over the network reported as this origin.
	std::array<uint64_t, static_cast<size_t>(OrderOrigin::Count_)> reported{};
};

std::array<PlayerTally, MAX_CONNECTED_PLAYERS> &tallies()
{
	static std::array<PlayerTally, MAX_CONNECTED_PLAYERS> t;
	return t;
}

} // namespace

void orderProvenanceRecord(unsigned player, OrderOrigin origin, bool refused)
{
	if (player >= MAX_CONNECTED_PLAYERS || origin >= OrderOrigin::Count_)
	{
		return;
	}
	auto &t = tallies()[player];
	const size_t idx = static_cast<size_t>(origin);
	if (refused)
	{
		t.refused[idx]++;
	}
	else
	{
		t.issued[idx]++;
	}
}

void orderProvenanceRecordReported(unsigned player, OrderOrigin origin)
{
	if (player >= MAX_CONNECTED_PLAYERS || origin >= OrderOrigin::Count_)
	{
		return;
	}
	tallies()[player].reported[static_cast<size_t>(origin)]++;
}

void orderProvenanceReset()
{
	tallies() = {};
}

uint64_t orderProvenanceRefusedCount(unsigned player)
{
	if (player >= MAX_CONNECTED_PLAYERS)
	{
		return 0;
	}
	uint64_t total = 0;
	for (uint64_t n : tallies()[player].refused)
	{
		total += n;
	}
	return total;
}

std::string orderProvenanceSummary()
{
	std::string out;
	for (unsigned p = 0; p < MAX_CONNECTED_PLAYERS; ++p)
	{
		const auto &t = tallies()[p];
		uint64_t issuedTotal = 0, refusedTotal = 0, reportedTotal = 0;
		for (size_t i = 0; i < t.issued.size(); ++i)
		{
			issuedTotal += t.issued[i];
			refusedTotal += t.refused[i];
			reportedTotal += t.reported[i];
		}
		if (issuedTotal == 0 && refusedTotal == 0 && reportedTotal == 0)
		{
			continue;
		}
		out += astringf("p%u:", p);
		for (size_t i = 0; i < t.issued.size(); ++i)
		{
			if (t.issued[i] != 0)
			{
				out += astringf(" %s=%" PRIu64, toString(static_cast<OrderOrigin>(i)), t.issued[i]);
			}
			if (t.refused[i] != 0)
			{
				out += astringf(" %s-REFUSED=%" PRIu64,
				                toString(static_cast<OrderOrigin>(i)), t.refused[i]);
			}
			if (t.reported[i] != 0)
			{
				out += astringf(" reported-%s=%" PRIu64,
				                toString(static_cast<OrderOrigin>(i)), t.reported[i]);
			}
		}
		out += "; ";
	}
	return out;
}
