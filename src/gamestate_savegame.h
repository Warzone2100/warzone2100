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
 *  Disk savegame format built on the GameState serializer.
 *
 *  This file owns everything a cold disk-load needs that the main snapshot / restore
 *  (gamestate_serialize.{cpp,h}) deliberately omits:
 *  - the session-setup / identity header
 *  - the on-disk folder container + uncompressed metadata sidecar
 *  - the cold-load orchestration (remount mods -> rescan levels -> load dataset ->
 *    set up players/AI -> apply state -> resume)
 *  - the save/load entry-point routing
 *
 */

#pragma once

#include <cstdint>
#include <array>
#include <string>
#include <vector>

#include <nlohmann/json_fwd.hpp>

#include "lib/framework/crc.h" // Sha256
#include "lib/framework/resource_loading_controller.h" // LoadingTask, ResourceLoadingController

namespace gamestate
{
namespace savegame
{

/// Which flavor of single-player game produced the save. Distinct from LEVEL_TYPE
/// (game.type): a challenge is a skirmish-type level flagged challengeActive, and the
/// cold-load orchestration selects the mod mount mode (campaign vs multiplay) from this.
enum class SaveType : uint8_t
{
	Skirmish = 0,
	Campaign = 1,
	Challenge = 2,
};

/// A mod recorded in the header: display name + SHA256 hash (as a hex string in the JSON).
/// Built from getLoadedMods(), de-duplicated while preserving load order.
struct SavegameMod
{
	std::string name;
	std::string hash; // hex (Sha256::toString); empty/zero if unknown
};

/// The subset of header data the cold-load orchestration needs that is NOT simply
/// restored into a live global by readSetupHeader() (it must act on it: remount mods,
/// rescan the level list, locate the dataset by name+hash).
struct SetupHeaderInfo
{
	SaveType saveType = SaveType::Skirmish;
	std::string levelName;
	Sha256 mapHash;
	bool builtInMap = false;
	std::vector<SavegameMod> mods;
	// Informational engine provenance recorded at save time.
	// Not used to gate the restore; surfaced for diagnostics only.
	std::string engineVersionString;
	std::string engineLatestTag;
};

/// Build the setup/identity header section for the current live game-setup state
/// (game options, players, level identity, mods, structure limits/flags).
nlohmann::ordered_json writeSetupHeader(SaveType saveType);

/// Restore live game-setup globals from a setup header section, and return the
/// orchestration-relevant subset (save type, level name + map hash, mod list).
/// Throws gamestate::StateError on malformed data.
SetupHeaderInfo readSetupHeader(const nlohmann::ordered_json &j);

/// Collect the currently loaded mods as {name, hash}, de-duplicated while preserving
/// load order. Exposed for the header writer and the load-menu sidecar.
std::vector<SavegameMod> collectLoadedMods();

/// In-engine self-test: round-trips the setup header (write -> perturb -> read) and
/// asserts the live setup globals are restored exactly. Needs no loaded game data.
bool runSavegameHeaderSelfTest();

// MARK: - Container (.wz zip archive: setup header + GameState document)
//
// A savegame is a folder holding (a) gamestate.wz, a zip archive whose single gamestate.json entry
// carries BOTH the setup/identity header AND the full GameState simulation document, and (b) an
// uncompressed metadata sidecar the load menu can read without opening the archive. The archive and its
// header are parseable *before* mods are mounted / the dataset is loaded. The embedded GameState is
// applied only *after* the level is loaded (see parseSavegameContainer).

/// Serialize the current live match to the .wz container archive bytes (setup header + GameState).
std::vector<uint8_t> serializeSavegameContainer(SaveType saveType);

/// Parse a container archive: restores the setup/identity globals and returns the
/// orchestration info, while handing back the embedded GameState document via outGameStateDoc
/// for the caller to apply AFTER the level/dataset has been loaded. Does NOT apply the GameState
/// (that is a separate, staged step). outLocalStateDoc (optional) receives the disk-only local
/// view/meta section (camera, radar zoom, cheated) and outPendingResumeDoc (optional) the disk-only
/// pending-resume section (the in-flight game-queue backlog), both for late application after the map is
/// loaded; neither is part of the networked GameState. Throws gamestate::StateError on malformed data.
SetupHeaderInfo parseSavegameContainer(const uint8_t *data, size_t len, nlohmann::ordered_json &outGameStateDoc, nlohmann::ordered_json *outLocalStateDoc = nullptr, nlohmann::ordered_json *outPendingResumeDoc = nullptr);

// MARK: - Metadata sidecar (uncompressed, readable without opening the archive)

/// The load-menu-facing metadata stored uncompressed alongside the state blob.
struct SavegameMetadata
{
	std::string saveName;
	int64_t timestampEpoch = 0; // seconds since epoch (caller-provided; kept out of the sim path)
	std::string timestampHuman;
	std::string buildTag;                                // version_getVersionString() at save time
	std::array<uint16_t, 3> latestTagArray = {0, 0, 0};  // [major, minor, patch] of the latest release tag
	uint32_t gameTime = 0;
	std::string levelName;
	uint32_t playerCount = 0;
	SaveType saveType = SaveType::Skirmish;
	std::vector<SavegameMod> mods; // so the menu can warn "needs mod X" before loading
};

nlohmann::ordered_json buildMetadataSidecar(const SavegameMetadata &meta);
SavegameMetadata parseMetadataSidecar(const nlohmann::ordered_json &j);

// MARK: - Folder container I/O (PHYSFS)

/// Write a savegame folder at folderPath (a PHYSFS write path): the compressed state blob plus
/// the uncompressed metadata sidecar. Creates the folder if needed. Returns false on I/O error.
bool writeSavegameFolder(const std::string &folderPath, SaveType saveType, const SavegameMetadata &meta);

/// Read a savegame folder: restores setup globals, returns the header + embedded GameState
/// document (to apply after level load) + the metadata sidecar. Returns false on I/O error;
/// throws gamestate::StateError on malformed content.
bool readSavegameFolder(const std::string &folderPath, SetupHeaderInfo &outHeader,
                        nlohmann::ordered_json &outGameStateDoc, SavegameMetadata &outMeta);

// MARK: - Cold-load orchestration

/// Remount the save's mods (warn-and-continue on missing/mismatched), re-scan the level list so
/// addon-campaign content is discoverable, and resolve the level dataset by name + map hash.
/// Mirrors the legacy campaign-mod remount (game.cpp setOverrideMods -> rebuildSearchPath ->
/// buildMapList -> levFindDataSet). Returns true if the dataset was resolved. Engine-state
/// touching (search paths / level list / palettes); NOT exercised by the headless self-test.
/// The caller then drives the standard level-data loading task and applies the GameState document.
bool coldLoadRemountModsAndResolveLevel(const SetupHeaderInfo &header);

/// In-engine self-test: round-trips the container blob (serialize -> parse -> apply GameState)
/// and the metadata sidecar, asserting setup + a determinism field restore and byte-stability.
/// Needs no loaded game data (the embedded GameState is the empty headless one).
bool runSavegameContainerSelfTest();

// MARK: - Save side

/// Write the current live match to a savegame folder at folderPath: builds the metadata sidecar
/// from live state (game time, level name, player count, mods) + the caller-supplied name/type,
/// then writes the folder (compressed state blob + sidecar). Returns false on I/O error.
/// (Stand-alone container; used once the legacy save/load is excised.)
bool writeCurrentGameToFolder(const std::string &folderPath, const std::string &saveName, SaveType saveType);

// MARK: - Coexistence with the legacy folder save
//
// The legacy save is already a folder with its own save-info.json that the load menu enumerates.
// saveGame() always additionally writes the GameState blob (gamestate.wz) into that same folder,
// and the load path prefers the blob when present (isNewFormatSaveFolder).

/// Write just the GameState blob (gamestate.wz) into an existing save folder, alongside the legacy
/// files. Does not touch the folder's save-info.json. Returns false on I/O error.
bool writeGameStateBlobToFolder(const std::string &folderPath, SaveType saveType);

// MARK: - Cold-load (level load + game start) wiring

/// True if folderPath looks like a new-format savegame folder (contains the state blob). Used by
/// the load entry point to route new-format saves here while leaving legacy .gam saves untouched.
bool isNewFormatSaveFolder(const std::string &folderPath);

/// Cold-load entry, mirroring loadGameInit for the new format. Reads the folder (restoring the
/// setup/identity globals), remounts the save's mods + resolves the level, loads the level dataset
/// (so rules/stats/VIEWDATA/scripts/tileset exist), then applies the GameState world/objects. The
/// scripting section is intentionally NOT applied here (scripts are not prepared until
/// stageThreeInitialise) - it is deferred to applyDeferredScripting(). Returns load_fail() on error.
LoadingTask<> coldLoadGameInit(ResourceLoadingController &controller, const std::string &folderPath);

/// Replace the just-loaded scenario world with the stashed cold-load snapshot (terrain + objects +
/// power/research/etc., but NOT scripting). Called by levLoadData in reconstruct mode right after the
/// scenario load (which provides the map's display layer) and BEFORE stageThreeInitialise, so
/// prepareScripts/TRIGGER_GAME_LOADED and grid/visibility init all run on the restored world. Resets
/// the global derived state the scenario placement dirtied and recomputes display ground types.
/// Returns false on failure.
bool coldLoadRestoreWorld();

/// Consume the "this game start is a new-format cold-load reconstruct" marker: returns true once for a
/// cold load (set at the end of coldLoadRestoreWorld), false otherwise, and clears it. stageThreeInitialise
/// calls this to learn the world/scripting were restored from a snapshot (ex. so it does not clobber the
/// restored bInTutorial). Consuming here also clears the marker for the next, non-cold load.
bool takeColdLoadReconstructFlag();

/// Restore the cold-load's deferred scripting section. Call once from levFinalizeLevelLoad, AFTER all level
/// data (and thus the script instances) is loaded but BEFORE stageThreeInitialise/prepareScripts, mirroring
/// the legacy loadScriptState timing. No-op unless a cold load stashed a document.
void applyDeferredScripting();

/// Replay the cold-load's deferred messages section. Call once, AFTER the level's data files (VIEWDATA)
/// have loaded and BEFORE the deferred scripting pass. No-op unless a cold load is pending.
void applyDeferredColdLoadMessages();

} // namespace savegame
} // namespace gamestate
