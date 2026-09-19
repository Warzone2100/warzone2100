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
/** \file
 *  Serialization of the full match simulation state ("GameState").
 *
 *  This is the foundation for serializing a complete, point-in-time match snapshot to
 *  support resuming the lockstep-deterministic simulation.
 *
 *  (It is deliberately NOT the old game.cpp save/load.)
 *
 *  Format: versioned JSON (via nlohmann-json)
 *  - Uses nlohmann::ordered_json (insertion-order keys), so a JS script object's
 *    property order round-trips through save/restore
 *  - Output is byte-stable because every writer inserts keys in a fixed order (C++ code order / JS enumeration order)
 *  - All sim-authoritative numbers are encoded as integers
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json_fwd.hpp>

namespace gamestate
{

/// Top-level container format version. (Bumped for incompatible container-layout changes.)
constexpr uint32_t GAMESTATE_FORMAT_VERSION = 1u;

/// Tag stored in the snapshot to identify it as a GameState document.
constexpr const char *GAMESTATE_FORMAT_TAG = "wz-gamestate";

/// Thrown on malformed/unsupported snapshot data
class StateError : public std::runtime_error
{
public:
	explicit StateError(const std::string &what) : std::runtime_error(what) {}
};

/// Which script instances the "scripting" section covers (write side: which to serialize; read side:
/// how to bind them). The rest of the GameState is identical across scopes.
enum class ScriptScope
{
	/// Every script instance - the rules/global script AND all AI bots - serialized and restored bound
	/// verbatim to each script's own player. For disk savegames and the in-process round-trip, which
	/// reload on a machine that runs all AI locally, so every instance's VM state must be reproduced.
	AllInstances,
	/// Only the local rules/global script: serialized for the host's selectedPlayer, and on restore
	/// rebound onto the receiver's selectedPlayer (matched by scriptName).
	LocalPlayerOnly,
};

/// Build a JSON document representing the current live match state.
nlohmann::ordered_json gameStateToJson(ScriptScope scriptScope = ScriptScope::AllInstances);

/// Restore live match state from a JSON document. Throws StateError on bad data.
/// deferMessages skips the "messages" section (intel/proximity), for the disk cold-load where the level's
/// VIEWDATA is not yet loaded when this runs. The caller replays it later via applyGameStateMessages().
void gameStateFromJson(const nlohmann::ordered_json &j, ScriptScope scriptScope = ScriptScope::AllInstances, bool deferMessages = false);

/// Serialize the current live match state to a (canonical, compact) JSON string.
std::string serializeGameState(ScriptScope scriptScope = ScriptScope::AllInstances);

/// Restore live match state from a JSON string. Throws StateError on bad data.
void deserializeGameState(const std::string &jsonText, ScriptScope scriptScope = ScriptScope::AllInstances);

/// Parse a JSON document with a hard nesting-depth cap, throwing StateError past the limit.
/// Used at the untrusted-input ingress parses.
nlohmann::ordered_json parseJsonBounded(const char *begin, const char *end);

// --- Per-section read/write helpers (operate on live globals via accessors) ---
nlohmann::ordered_json writeDeterminismCore();
void readDeterminismCore(const nlohmann::ordered_json &j, uint32_t version);

/// Apply ONLY the "scripting" section of a GameState document. The disk savegame cold-load
/// restores the world at level-load time (when scripts are not yet prepared, so gameStateFromJson
/// silently skips scripting) and then replays the scripting section here, after the engine has
/// instantiated the scripts in stageThreeInitialise. No-op if the section is absent or scripts
/// are not ready.
/// Defaults to AllInstances (the disk-savegame cold-load restores every script instance).
void applyGameStateScripting(const nlohmann::ordered_json &gameStateDoc, ScriptScope scriptScope = ScriptScope::AllInstances);

/// Apply ONLY the "messages" section of a GameState document. Used by the disk cold-load to replay
/// messages after the level's VIEWDATA has finished loading (gameStateFromJson deferMessages skips it
/// during the early world restore). No-op if the section is absent.
void applyGameStateMessages(const nlohmann::ordered_json &gameStateDoc);

/// In-engine determinism harness self-test (round-trip + RNG-sequence fidelity + byte-stability).
/// Needs no game data - safe to run very early. Returns true if all checks pass.
bool runGameStateSelfTest();

/// Non-destructive in-game check: serialize the live match and re-parse it, reporting size
/// and object counts to the console. Safe to run during a game; does not mutate state.
void runGameStateLiveWriteCheck();

/// Destructive in-game reconstruct-fidelity test: serialize -> clear+reconstruct -> re-serialize,
/// requiring byte-identical output. Returns true on success. Intended for headless autogame + other testing.
bool runGameStateRoundTripTest();

/// Configure a game tick at which to auto-run the round-trip test and exit (0 = disabled).
void gamestateSetRoundTripTestTick(uint32_t tick);

/// Per-tick hook: runs the round-trip test and exits when the configured tick is reached.
void gamestateMaybeRunRoundTripTest();

} // namespace gamestate
