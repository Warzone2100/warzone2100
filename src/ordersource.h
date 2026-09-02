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
 * @file ordersource.h
 */

#ifndef __INCLUDED_SRC_ORDERSOURCE_H__
#define __INCLUDED_SRC_ORDERSOURCE_H__

#include "lib/framework/frame.h"

#include <cstdint>
#include <string>

// Where an order came from.
enum class OrderOrigin : uint8_t
{
	// Game logic: a repaired droid returning to its delivery point, a factory sending a new unit to its
	// assembly point, the automatic retreat when damaged. The fallback when no scope is active.
	Simulation = 0,
	// A mouse click or drag in the 3D view or on the radar.
	UiPointer,
	// A key mapping or gamepad binding.
	UiKeybind,
	// An in-game widget: the build, manufacture, research or order panels.
	UiWidget,
	// A script, through the wzapi surface.
	Script,

	Count_
};

const char *toString(OrderOrigin origin);

/**
 * The input event that produced this order. Populated for UiPointer and UiKeybind.
 *
 * The serial is a per-process counter of consumed input events, monotonic within a session.
 * The timestamp is realTime (wall clock), not gameTime. Only the normalized cursor position goes on the wire.
 */
struct InputEventInfo
{
	uint64_t serial = 0;      // monotonic input event counter, 0 means none
	uint32_t realTimeMs = 0;  // realTime at the input event
	int32_t  screenX = -1;    // cursor position in screen space, or -1
	int32_t  screenY = -1;
	int32_t  worldX = -1;     // the frame's world-space pick, or -1
	int32_t  worldY = -1;

	// Cursor position normalized to the viewport, 0..65535 on each axis.
	uint16_t viewX = 0;
	uint16_t viewY = 0;
	bool hasViewPos = false;

	bool valid() const { return serial != 0; }
};

class OrderSource
{
public:
	OrderSource() = delete;

	// Game logic. reason is a short static string for logs.
	static OrderSource simulation(const char *reason);
	// A script running in scriptPlayer's context.
	// hostDeclared is true for rules / global scripts.
	static OrderSource script(int scriptPlayer, bool hostDeclared);
	// A mouse click or drag.
	static OrderSource pointer(const InputEventInfo &eventInfo);
	// A key mapping or gamepad binding.
	static OrderSource keybind(const InputEventInfo &eventInfo);
	// An in-game widget.
	static OrderSource widget();
	// An in-game widget, carrying the cursor position that drove it. No input serial, since a widget scope
	// covers a frame's worth of processing rather than one input event.
	static OrderSource widgetAt(uint16_t viewX, uint16_t viewY);
	// Reconstructs a source from what a peer put on the wire. Only the origin and the normalized cursor
	// position survive the trip.
	static OrderSource fromWire(OrderOrigin origin, bool hasViewPos, uint16_t viewX, uint16_t viewY);

	OrderOrigin origin() const { return m_origin; }
	const InputEventInfo &eventInfo() const { return m_eventInfo; }
	// Valid only when origin() == Script.
	int scriptPlayer() const { return m_scriptPlayer; }
	// Valid only when origin() == Script. See wzapi::ScriptBinding.
	bool scriptIsHostDeclared() const { return m_scriptHostDeclared; }
	const char *reason() const { return m_reason; }

	bool isScript() const { return m_origin == OrderOrigin::Script; }
	bool isUserInput() const
	{
		return m_origin == OrderOrigin::UiPointer || m_origin == OrderOrigin::UiKeybind;
	}

	std::string toDescription() const;

private:
	OrderSource(OrderOrigin origin, const char *reason);

	OrderOrigin m_origin;
	const char *m_reason = "";
	InputEventInfo m_eventInfo;
	int m_scriptPlayer = -1;
	bool m_scriptHostDeclared = false;
};

/**
 * Establishes the ambient order source for the duration of the scope.
 *
 * Scopes nest: the innermost wins and the previous value is restored on exit.
 * The game loop is single-threaded, so this is a plain stack.
 */
class OrderSourceScope
{
public:
	explicit OrderSourceScope(const OrderSource &source);
	~OrderSourceScope();

	OrderSourceScope(const OrderSourceScope &) = delete;
	OrderSourceScope &operator=(const OrderSourceScope &) = delete;

private:
	size_t m_depth;
};

// The ambient order source. Simulation when no scope is active.
const OrderSource &currentOrderSource();

// Drops any active scopes. Called when a game ends, so state cannot leak between games.
void orderSourceReset();

// ---------------------------------------------------------------------------
// MARK: - Input event records
// ---------------------------------------------------------------------------

// Mints the record of an input event that has just been consumed. Called by the input layer only.
// Screen and world coordinates are supplied by the caller.
// Pass -1 for coordinates that do not apply (a keybind has no click position).
InputEventInfo mintInputEventInfo(int32_t screenX, int32_t screenY, int32_t worldX, int32_t worldY);

// Normalizes a cursor position against a viewport size, 0..65535 per axis.
// Returns false when there is no usable position, in which case the outputs are untouched.
bool orderSourceNormalizeViewPos(int32_t screenX, int32_t screenY,
                                 int32_t viewportWidth, int32_t viewportHeight,
                                 uint16_t &outX, uint16_t &outY);

// As mintInputEventInfo(), and additionally normalizes the cursor position against the given viewport size.
InputEventInfo mintPointerEventInfo(int32_t screenX, int32_t screenY,
                                int32_t viewportWidth, int32_t viewportHeight,
                                int32_t worldX, int32_t worldY);

// ---------------------------------------------------------------------------
// MARK: - Policy
// ---------------------------------------------------------------------------

bool orderSourcePermitsPlayerAction(unsigned targetPlayer, const OrderSource &source);

// ---------------------------------------------------------------------------
// MARK: - Provenance tally
// ---------------------------------------------------------------------------

// Records that a droid order for player was issued (or refused) from origin.
void orderProvenanceRecord(unsigned player, OrderOrigin origin, bool refused);

// Records the origin a peer reported on an order that arrived from the network.
void orderProvenanceRecordReported(unsigned player, OrderOrigin origin);

// Clears the tally. Called at game start.
void orderProvenanceReset();

// Human-readable dump of the tally, for the log at game end.
std::string orderProvenanceSummary();

// Number of orders refused for player, over this game.
uint64_t orderProvenanceRefusedCount(unsigned player);

#endif // __INCLUDED_SRC_ORDERSOURCE_H__
