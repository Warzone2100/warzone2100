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

#include <nlohmann/json.hpp> // Must come before WZ includes

#include "gamestate_savegame.h"
#include "gamestate_serialize.h" // gamestate::StateError

#include "lib/framework/frame.h"      // selectedPlayer, MAX_PLAYERS
#include "lib/framework/string_ext.h" // sstrcpy
#include "lib/netplay/netplay.h"      // NetPlay, PLAYER, AIDifficulty, FactionID; NETGet{Major,Minor}Version
#include "lib/netplay/nettypes.h"     // NETgameQueueCapturePending/RestorePending, MAX_GAMEQUEUE_SLOTS
#include "lib/netplay/sync_debug.h"   // syncCrcDetailArmOnSaveOrLoad (CRC-trace detail diagnostic)

#include "multiplay.h"      // game, ingame, bMultiPlayer, MULTIPLAYERGAME, MULTISTRUCTLIMITS
#include "multiplaydefs.h"  // clampPlayerReconnectWaitSeconds, PLAYER_LEAVE_MODE, BLIND_MODE
#include "campaigninfo.h"   // getCampaignName/Number, setCampaignName/Number
#include "challenge.h"      // challengeActive / challengeFileName (setup header)
#include "frontend.h"       // aLevelName
#include "version.h"        // version_getVersionString/LatestTag/VcsFullHash (informational engine block)
#include "screens/guidescreen.h" // save/restoreLoadedGuideTopics + guide popup toggle
#include "map.h"            // builtInMap, useTerrainOverrides, mapReloadGroundTypes
#include "game_world.h"     // gameWorld
#include "structure.h"      // initFactoryNumFlag
#include "message.h"        // releaseAllProxDisp
#include "display3d.h"      // playerPos (camera) - local view state
#include "radar.h"          // Get/SetRadarZoom - local view state
#include "mission.h"        // Cheated - local meta flag
#include "effects.h"        // serialize/restoreActiveEffects - local display state
#include "multistat.h"      // loadMultiStats, setMultiStats, getMultiStats
#include "modding.h"        // getLoadedMods, setOverrideMods, clearOverrideMods
#include "init.h"           // rebuildSearchPath, buildMapList, searchPathMode
#include "levels.h"         // levShutDown, levInitialise, levFindDataSet, makeLevLoadDataLoadingTask
#include "gamedef.h"        // GAME_TYPE, GTYPE_SAVE_START
#include "lib/gamelib/gtime.h"      // gameTime, setGameTime
#include "lib/ivis_opengl/piepalette.h" // pal_Init
#include "lib/framework/file.h"     // saveFile, loadFileToBufferVector

#include <physfs.h>
#include "ZipIOProvider.h"  // WzMapZipIO - libzip-backed .wz zip container

#include <ctime>
#include <map>
#include <memory>
#include <optional>
#include <unordered_set>

namespace gamestate
{
namespace savegame
{

// MARK: - Section: setup / identity header
//
// The single most important addition over the state snapshot: a cold disk-load has no
// existing instance, lobby, or peer to inherit game setup from, so it must persist the level
// identity, the game options, the per-slot player/AI setup, the mod list, and the structure
// limits/flags.
//
// This mirrors the field inventory of the legacy game.cpp writeMainFile (main.json) but is
// a fresh format folded into one "setup" section - it does not reuse the legacy layout.

constexpr uint32_t SETUP_SECTION_VERSION = 1;

std::vector<SavegameMod> collectLoadedMods()
{
	std::vector<SavegameMod> result;
	std::unordered_set<std::string> seen;
	// getLoadedMods() returns a const reference; copy it so we can call the (non-const,
	// lazily-hashing) getHash(). De-duplicate by name+hash while preserving load order.
	std::vector<WzMods::LoadedMod> mods = getLoadedMods();
	for (WzMods::LoadedMod &m : mods)
	{
		const std::string hash = m.getHash().toString();
		const std::string key = m.name + std::string("\n") + hash;
		if (!seen.insert(key).second)
		{
			continue; // duplicate - keep the first occurrence's position
		}
		result.push_back(SavegameMod{ m.name, hash });
	}
	return result;
}

static nlohmann::ordered_json writePlayers()
{
	nlohmann::ordered_json jplayers = nlohmann::ordered_json::array();
	for (unsigned p = 0; p < MAX_PLAYERS && p < NetPlay.players.size(); ++p)
	{
		const PLAYER &pl = NetPlay.players[p];
		nlohmann::ordered_json jp = nlohmann::ordered_json::object();
		jp["index"] = p;
		// Raw slot fields, for an exact round-trip. The cold-load orchestration may later
		// re-route some of these through setters (setPlayerName/setPlayerColour) once a game
		// is being booted - the header itself just preserves the authoritative values.
		jp["name"] = std::string(pl.name);
		jp["position"] = pl.position;
		jp["colour"] = pl.colour;
		jp["team"] = pl.team;
		jp["faction"] = static_cast<uint8_t>(pl.faction);
		jp["difficulty"] = static_cast<int>(static_cast<int8_t>(pl.difficulty));
		jp["ai"] = static_cast<int>(pl.ai);
		jp["allocated"] = pl.allocated;
		jp["isSpectator"] = pl.isSpectator;
		jp["autoGame"] = pl.autoGame;
		jp["ready"] = pl.ready;
		jplayers.push_back(std::move(jp));
	}
	return jplayers;
}

static void readPlayers(const nlohmann::ordered_json &jplayers)
{
	if (!jplayers.is_array())
	{
		throw StateError("setup.players must be an array");
	}
	for (const nlohmann::ordered_json &jp : jplayers)
	{
		const int idx = jp.at("index").get<int>();
		if (idx < 0 || idx >= static_cast<int>(MAX_PLAYERS) || idx >= static_cast<int>(NetPlay.players.size()))
		{
			continue; // slot not present on this build/runtime - skip rather than fail
		}
		PLAYER &pl = NetPlay.players[idx];
		sstrcpy(pl.name, jp.at("name").get<std::string>().c_str());
		pl.position = jp.at("position").get<int32_t>();
		pl.colour = jp.at("colour").get<int32_t>();
		pl.team = jp.at("team").get<int32_t>();
		pl.faction = static_cast<FactionID>(jp.at("faction").get<uint8_t>());
		pl.difficulty = static_cast<AIDifficulty>(static_cast<int8_t>(jp.at("difficulty").get<int>()));
		pl.ai = static_cast<int8_t>(jp.at("ai").get<int>());
		pl.allocated = jp.at("allocated").get<bool>();
		pl.isSpectator = jp.at("isSpectator").get<bool>();
		pl.autoGame = jp.at("autoGame").get<bool>();
		pl.ready = jp.at("ready").get<bool>();
	}
}

static nlohmann::ordered_json writeStructureLimits()
{
	nlohmann::ordered_json jlimits = nlohmann::ordered_json::array();
	for (const MULTISTRUCTLIMITS &lim : ingame.structureLimits)
	{
		jlimits.push_back(nlohmann::ordered_json::array({ lim.id, lim.limit }));
	}
	return jlimits;
}

static void readStructureLimits(const nlohmann::ordered_json &jlimits)
{
	if (!jlimits.is_array())
	{
		throw StateError("setup.structureLimits must be an array");
	}
	ingame.structureLimits.clear();
	ingame.structureLimits.reserve(jlimits.size());
	for (const nlohmann::ordered_json &jl : jlimits)
	{
		MULTISTRUCTLIMITS lim;
		lim.id = jl.at(0).get<uint32_t>();
		lim.limit = jl.at(1).get<uint32_t>();
		ingame.structureLimits.push_back(lim);
	}
}

nlohmann::ordered_json writeSetupHeader(SaveType saveType)
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = SETUP_SECTION_VERSION;
	j["saveType"] = static_cast<uint8_t>(saveType);

	// Session identity that MP would have established at setup.
	j["multiplayer"] = bMultiPlayer;
	j["selectedPlayer"] = selectedPlayer;
	j["hostPlayer"] = NetPlay.hostPlayer;
	j["playerCount"] = NetPlay.playercount;
	j["bComms"] = NetPlay.bComms;

	// Level identity: The terrain itself is serialized separately (mapTerrain section of the
	// GameState) - name + hash here are for validation, the load-menu UI, and locating the
	// level's non-terrain data (rules/stats/VIEWDATA/scripts) via levFindDataSet(name, hash).
	nlohmann::ordered_json jlevel = nlohmann::ordered_json::object();
	jlevel["levelName"] = std::string(aLevelName);
	jlevel["campaignName"] = getCampaignName();
	jlevel["campaignNumber"] = getCampaignNumber();
	jlevel["gameType"] = static_cast<uint8_t>(game.type);
	jlevel["mapName"] = std::string(game.map);
	jlevel["mapHash"] = game.hash.toString();
	jlevel["builtInMap"] = builtInMap;
	jlevel["useTerrainOverrides"] = useTerrainOverrides;
	jlevel["mapHasScavengers"] = game.mapHasScavengers;
	jlevel["isMapMod"] = game.isMapMod;
	jlevel["isRandom"] = game.isRandom;
	// Challenge-mode identity (challenge.cpp): scenario configuration, so it lives here with the rest of
	// the level/game identity rather than in localState. This is also the slot MP challenge config would be
	// reconstructed from (like mapName/gameType), if multiplayer challenges are ever supported.
	jlevel["challengeActive"] = challengeActive;
	jlevel["challengeFileName"] = challengeFileName.toUtf8();
	j["level"] = std::move(jlevel);

	// Mods / addon campaigns: an array of {name, hash}, de-duped preserving load order.
	nlohmann::ordered_json jmods = nlohmann::ordered_json::array();
	for (const SavegameMod &m : collectLoadedMods())
	{
		nlohmann::ordered_json jm = nlohmann::ordered_json::object();
		jm["name"] = m.name;
		jm["hash"] = m.hash;
		jmods.push_back(std::move(jm));
	}
	j["mods"] = std::move(jmods);

	// Game options (the gate-affecting subset of MULTIPLAYERGAME).
	nlohmann::ordered_json jopt = nlohmann::ordered_json::object();
	jopt["power"] = game.power;
	jopt["base"] = game.base;
	jopt["alliance"] = game.alliance;
	jopt["scavengers"] = game.scavengers;
	jopt["techLevel"] = game.techLevel;
	jopt["maxPlayers"] = game.maxPlayers;
	jopt["name"] = std::string(game.name);
	jopt["blindMode"] = static_cast<uint8_t>(game.blindMode);
	jopt["gameTimeLimitMinutes"] = game.gameTimeLimitMinutes;
	jopt["inactivityMinutes"] = game.inactivityMinutes;
	jopt["playerLeaveMode"] = static_cast<uint8_t>(game.playerLeaveMode);
	jopt["playerReconnectWaitSeconds"] = game.playerReconnectWaitSeconds;
	jopt["pathfindingBackend"] = game.pathfindingBackend;
	j["options"] = std::move(jopt);

	j["players"] = writePlayers();
	j["structureLimits"] = writeStructureLimits();
	j["flags"] = ingame.flags;

	// Engine provenance (informational)
	// - Not currently hard-checked on read (the per-section format versions are what actually gate compatibility)
	nlohmann::ordered_json jengine = nlohmann::ordered_json::object();
	jengine["versionString"] = std::string(version_getVersionString());
	jengine["vcsHash"] = std::string(version_getVcsFullHash());
	jengine["latestTag"] = std::string(version_getLatestTag());
	jengine["netcodeMajor"] = NETGetMajorVersion();
	jengine["netcodeMinor"] = NETGetMinorVersion();
	j["engine"] = std::move(jengine);

	return j;
}

SetupHeaderInfo readSetupHeader(const nlohmann::ordered_json &j)
{
	if (!j.is_object())
	{
		throw StateError("setup header must be a JSON object");
	}
	if (j.value("version", 0u) != SETUP_SECTION_VERSION)
	{
		throw StateError("unsupported setup header section version");
	}

	SetupHeaderInfo info;
	info.saveType = static_cast<SaveType>(j.value("saveType", static_cast<uint8_t>(SaveType::Skirmish)));

	bMultiPlayer = j.at("multiplayer").get<bool>();
	selectedPlayer = j.at("selectedPlayer").get<uint32_t>();
	NetPlay.hostPlayer = j.at("hostPlayer").get<uint32_t>();
	NetPlay.playercount = j.at("playerCount").get<uint32_t>();
	NetPlay.bComms = j.at("bComms").get<bool>();

	const nlohmann::ordered_json &jlevel = j.at("level");
	info.levelName = jlevel.at("levelName").get<std::string>();
	sstrcpy(aLevelName, info.levelName.c_str());
	setCampaignNumber(jlevel.at("campaignNumber").get<uint32_t>());
	const std::string campaignName = jlevel.value("campaignName", std::string());
	if (!campaignName.empty())
	{
		setCampaignName(campaignName);
	}
	else
	{
		clearCampaignName();
	}
	game.type = static_cast<LEVEL_TYPE>(jlevel.at("gameType").get<uint8_t>());
	sstrcpy(game.map, jlevel.at("mapName").get<std::string>().c_str());
	game.hash.fromString(jlevel.at("mapHash").get<std::string>());
	info.mapHash = game.hash;
	builtInMap = jlevel.at("builtInMap").get<bool>();
	info.builtInMap = builtInMap;
	useTerrainOverrides = jlevel.at("useTerrainOverrides").get<bool>();
	game.mapHasScavengers = jlevel.at("mapHasScavengers").get<bool>();
	game.isMapMod = jlevel.at("isMapMod").get<bool>();
	game.isRandom = jlevel.at("isRandom").get<bool>();
	challengeActive = jlevel.value("challengeActive", false);
	if (jlevel.contains("challengeFileName"))
	{
		challengeFileName = WzString::fromUtf8(jlevel.at("challengeFileName").get<std::string>());
	}

	// Mods: parse into the info struct (for the cold-load remount/validation step) and rebuild
	// game.modHashes from the recorded hashes so a resumed game knows what it expects.
	const nlohmann::ordered_json &jmods = j.at("mods");
	if (!jmods.is_array())
	{
		throw StateError("setup.mods must be an array");
	}
	game.modHashes.clear();
	for (const nlohmann::ordered_json &jm : jmods)
	{
		SavegameMod m;
		m.name = jm.at("name").get<std::string>();
		m.hash = jm.value("hash", std::string());
		info.mods.push_back(m);
		if (!m.hash.empty())
		{
			Sha256 h;
			h.fromString(m.hash);
			if (!h.isZero())
			{
				game.modHashes.push_back(h);
			}
		}
	}

	const nlohmann::ordered_json &jopt = j.at("options");
	game.power = jopt.at("power").get<uint32_t>();
	game.base = jopt.at("base").get<uint8_t>();
	game.alliance = jopt.at("alliance").get<uint8_t>();
	game.scavengers = jopt.at("scavengers").get<uint8_t>();
	game.techLevel = jopt.at("techLevel").get<uint32_t>();
	game.maxPlayers = jopt.at("maxPlayers").get<uint8_t>();
	sstrcpy(game.name, jopt.at("name").get<std::string>().c_str());
	game.blindMode = static_cast<BLIND_MODE>(jopt.at("blindMode").get<uint8_t>());
	game.gameTimeLimitMinutes = jopt.at("gameTimeLimitMinutes").get<uint32_t>();
	game.inactivityMinutes = jopt.at("inactivityMinutes").get<uint32_t>();
	game.playerLeaveMode = static_cast<PLAYER_LEAVE_MODE>(jopt.at("playerLeaveMode").get<uint8_t>());
	game.playerReconnectWaitSeconds = clampPlayerReconnectWaitSeconds(jopt.at("playerReconnectWaitSeconds").get<uint32_t>());
	// default to the legacy A* planner for snapshots written before this setting existed
	game.pathfindingBackend = jopt.value("pathfindingBackend", static_cast<uint8_t>(0));

	readPlayers(j.at("players"));
	readStructureLimits(j.at("structureLimits"));
	ingame.flags = j.at("flags").get<uint8_t>();

	// Engine provenance: informational. Never gates the restore, just surfaced for diagnostics
	// and logged when it differs from the running build.
	if (j.contains("engine") && j.at("engine").is_object())
	{
		const nlohmann::ordered_json &jengine = j.at("engine");
		info.engineVersionString = jengine.value("versionString", std::string());
		info.engineLatestTag = jengine.value("latestTag", std::string());
		if (!info.engineVersionString.empty() && info.engineVersionString != version_getVersionString())
		{
			debug(LOG_INFO, "GameState savegame was written by engine \"%s\" (tag %s); running \"%s\"",
				info.engineVersionString.c_str(),
				info.engineLatestTag.empty() ? "?" : info.engineLatestTag.c_str(),
				version_getVersionString());
		}
	}

	return info;
}

// MARK: - Self-test
//
// Round-trips the setup header: set known live setup globals, write, perturb, read back,
// and assert each is restored. Needs no loaded game data; safe to run early (the CLI
// --gamestate-selftest path resizes NetPlay.players so the per-slot fields are exercised).

bool runSavegameHeaderSelfTest()
{
	bool ok = true;
	const auto check = [&ok](bool cond, const char *msg)
	{
		if (!cond)
		{
			fprintf(stderr, "[savegame-header-selftest] FAIL: %s\n", msg);
			ok = false;
		}
	};

	// Ensure there are player slots to exercise (the early CLI self-test runs before NETinit).
	if (NetPlay.players.size() < MAX_PLAYERS)
	{
		NetPlay.players.resize(MAX_PLAYERS);
	}

	// Arrange: a known, non-default setup configuration.
	bMultiPlayer = true;
	selectedPlayer = 3;
	NetPlay.hostPlayer = 1;
	NetPlay.playercount = 7;
	NetPlay.bComms = true;
	sstrcpy(aLevelName, "Sk-MyTestLevel");
	setCampaignNumber(2);
	setCampaignName("hdr-selftest-campaign");
	game.type = LEVEL_TYPE::SKIRMISH;
	sstrcpy(game.map, "MyTestMap");
	game.hash.fromString("0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
	builtInMap = false;
	useTerrainOverrides = true;
	game.mapHasScavengers = true;
	game.isMapMod = false;
	game.isRandom = true;
	game.power = 1500;
	game.base = 2;
	game.alliance = 1;
	game.scavengers = 1;
	game.techLevel = 3;
	game.maxPlayers = 6;
	sstrcpy(game.name, "My Test Game");
	game.blindMode = BLIND_MODE::NONE;
	game.gameTimeLimitMinutes = 45;
	game.inactivityMinutes = 10;
	game.playerLeaveMode = PLAYER_LEAVE_MODE::SPLIT_WITH_TEAM;
	game.playerReconnectWaitSeconds = 30;
	ingame.flags = 0x15;
	ingame.structureLimits.clear();
	ingame.structureLimits.push_back(MULTISTRUCTLIMITS{ 4u, 10u });
	ingame.structureLimits.push_back(MULTISTRUCTLIMITS{ 9u, 0u });

	PLAYER &p0 = NetPlay.players[0];
	sstrcpy(p0.name, "Alice");
	p0.position = 0;
	p0.colour = 4;
	p0.team = 0;
	p0.faction = FACTION_NORMAL;
	p0.difficulty = AIDifficulty::HUMAN;
	p0.ai = -2;
	p0.allocated = true;
	p0.isSpectator = false;
	p0.autoGame = false;
	p0.ready = true;

	PLAYER &p1 = NetPlay.players[1];
	sstrcpy(p1.name, "BotBob");
	p1.position = 2;
	p1.colour = 1;
	p1.team = 1;
	p1.faction = FACTION_NEXUS;
	p1.difficulty = AIDifficulty::HARD;
	p1.ai = 5;
	p1.allocated = false;
	p1.isSpectator = false;
	p1.autoGame = true;
	p1.ready = false;

	nlohmann::ordered_json header;
	try
	{
		header = writeSetupHeader(SaveType::Skirmish);
	}
	catch (const std::exception &e)
	{
		check(false, e.what());
		return ok;
	}

	// Byte-stability: same live state must serialize identically.
	const std::string dump1 = header.dump();
	const std::string dump2 = writeSetupHeader(SaveType::Skirmish).dump();
	check(dump1 == dump2, "byte-stability: re-serialized setup header differs");

	// Act: perturb every captured field, then restore from the header.
	bMultiPlayer = false;
	selectedPlayer = 0;
	NetPlay.hostPlayer = 0;
	NetPlay.playercount = 0;
	NetPlay.bComms = false;
	sstrcpy(aLevelName, "wrong");
	setCampaignNumber(1);
	clearCampaignName();
	game.type = LEVEL_TYPE::CAMPAIGN;
	sstrcpy(game.map, "wrong");
	game.hash.setZero();
	builtInMap = true;
	useTerrainOverrides = false;
	game.mapHasScavengers = false;
	game.isMapMod = true;
	game.isRandom = false;
	game.power = 0;
	game.base = 0;
	game.alliance = 0;
	game.scavengers = 0;
	game.techLevel = 1;
	game.maxPlayers = 0;
	sstrcpy(game.name, "wrong");
	game.gameTimeLimitMinutes = 0;
	game.inactivityMinutes = 0;
	game.playerReconnectWaitSeconds = 0;
	ingame.flags = 0;
	ingame.structureLimits.clear();
	sstrcpy(NetPlay.players[0].name, "wrong");
	NetPlay.players[0].colour = 0;
	sstrcpy(NetPlay.players[1].name, "wrong");
	NetPlay.players[1].ai = 0;

	SetupHeaderInfo info;
	try
	{
		info = readSetupHeader(header);
	}
	catch (const std::exception &e)
	{
		check(false, e.what());
		return ok;
	}

	// Assert: session identity restored.
	check(bMultiPlayer == true, "multiplayer not restored");
	check(selectedPlayer == 3u, "selectedPlayer not restored");
	check(NetPlay.hostPlayer == 1u, "hostPlayer not restored");
	check(NetPlay.playercount == 7u, "playerCount not restored");
	check(NetPlay.bComms == true, "bComms not restored");

	// Assert: level identity restored.
	check(std::string(aLevelName) == "Sk-MyTestLevel", "levelName not restored");
	check(getCampaignNumber() == 2u, "campaignNumber not restored");
	check(getCampaignName() == "hdr-selftest-campaign", "campaignName not restored");
	check(game.type == LEVEL_TYPE::SKIRMISH, "gameType not restored");
	check(std::string(game.map) == "MyTestMap", "mapName not restored");
	check(game.hash.toString() == "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef", "mapHash not restored");
	check(builtInMap == false, "builtInMap not restored");
	check(useTerrainOverrides == true, "useTerrainOverrides not restored");
	check(game.isRandom == true, "isRandom not restored");

	// Assert: orchestration info matches.
	check(info.saveType == SaveType::Skirmish, "info.saveType wrong");
	check(info.levelName == "Sk-MyTestLevel", "info.levelName wrong");
	check(info.mapHash.toString() == game.hash.toString(), "info.mapHash wrong");

	// Assert: options restored.
	check(game.power == 1500u, "power not restored");
	check(game.base == 2, "base not restored");
	check(game.alliance == 1, "alliance not restored");
	check(game.techLevel == 3u, "techLevel not restored");
	check(game.maxPlayers == 6, "maxPlayers not restored");
	check(std::string(game.name) == "My Test Game", "game.name not restored");
	check(game.gameTimeLimitMinutes == 45u, "gameTimeLimitMinutes not restored");
	check(game.inactivityMinutes == 10u, "inactivityMinutes not restored");
	check(game.playerReconnectWaitSeconds == 30u, "playerReconnectWaitSeconds not restored");

	// Assert: flags + structure limits restored.
	check(ingame.flags == 0x15, "flags not restored");
	if (ingame.structureLimits.size() == 2)
	{
		check(ingame.structureLimits[0].id == 4u && ingame.structureLimits[0].limit == 10u, "structureLimits[0] not restored");
		check(ingame.structureLimits[1].id == 9u && ingame.structureLimits[1].limit == 0u, "structureLimits[1] not restored");
	}
	else
	{
		check(false, "structureLimits count not restored");
	}

	// Assert: per-slot player fields restored.
	check(std::string(NetPlay.players[0].name) == "Alice", "player0 name not restored");
	check(NetPlay.players[0].colour == 4, "player0 colour not restored");
	check(NetPlay.players[0].difficulty == AIDifficulty::HUMAN, "player0 difficulty not restored");
	check(NetPlay.players[0].ai == -2, "player0 ai not restored");
	check(NetPlay.players[0].allocated == true, "player0 allocated not restored");
	check(std::string(NetPlay.players[1].name) == "BotBob", "player1 name not restored");
	check(NetPlay.players[1].faction == FACTION_NEXUS, "player1 faction not restored");
	check(NetPlay.players[1].difficulty == AIDifficulty::HARD, "player1 difficulty not restored");
	check(NetPlay.players[1].ai == 5, "player1 ai not restored");
	check(NetPlay.players[1].autoGame == true, "player1 autoGame not restored");

	// Assert: round-trip stability (read mutated nothing that changes re-serialization).
	const std::string dump3 = writeSetupHeader(SaveType::Skirmish).dump();
	check(dump1 == dump3, "round-trip: re-serialized setup header differs after read");

	if (ok)
	{
		fprintf(stderr, "[savegame-header-selftest] PASS (%zu-byte setup header)\n", dump1.size());
	}
	return ok;
}

// MARK: - Container: zip wrapper (setup header + GameState document)
//
// The wrapper document is { format, version, saveType, setup, gameState, localState, pendingResume }, stored as a
// single "gamestate.json" entry inside a standard zip archive (the on-disk file keeps its .wz name).

constexpr uint32_t SAVEGAME_CONTAINER_VERSION = 1;
constexpr const char *SAVEGAME_FORMAT_TAG = "wz-savegame";
constexpr const char *SIDECAR_FORMAT_TAG = "wz-savegame-info";

static const char *kStateBlobFileName = "gamestate.wz";   // the on-disk zip archive
// New-format metadata sidecar. A distinct name from the legacy "save-info.json" the load menu still
// enumerates, so both can coexist in a dual-written folder without clobbering each other.
static const char *kSidecarFileName = "gamestate-info.json";
static const char *kContainerJsonName = "gamestate.json"; // the single entry inside the zip

// Upper bound on the decompressed container document. Real saves are far smaller; this caps a crafted
// archive that declares an enormous uncompressed size (zip-bomb / memory-exhaustion defence).
constexpr uint32_t SAVEGAME_MAX_UNCOMPRESSED = 512u * 1024u * 1024u;

// MARK: - Local view / meta state (disk savegame only)
//
// Per-client presentation + local meta that is deliberately NOT in the GameState document.
// None of this feeds the deterministic sim, so it lives only in the disk container and is applied late
// (after the map is loaded).
// The loaded (unlocked) guide topics + the popup-disable toggle are persisted here: they are in-game session state
// driven by the addGuideTopic() wzapi (campaign progression), matching the legacy per-savegame guidetopics.json.

static nlohmann::ordered_json writeLocalState()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	// Camera (view position + rotation).
	nlohmann::ordered_json cam = nlohmann::ordered_json::object();
	cam["px"] = playerPos.p.x; cam["py"] = playerPos.p.y; cam["pz"] = playerPos.p.z;
	cam["rx"] = playerPos.r.x; cam["ry"] = playerPos.r.y; cam["rz"] = playerPos.r.z;
	j["camera"] = std::move(cam);
	j["radarZoom"] = GetRadarZoom();
	j["cheated"] = Cheated; // meta: disables autosave / flags scores - not networked, not sim

	// (The in-flight game-action queue backlog lives in the separate pendingResume container section, not
	// here - it is pending future input, not local view/meta state.)

	// Loaded (unlocked) guide topics + the popup-disable toggle (see the section comment): in-game session
	// state a script unlocks via addGuideTopic(). Guide state is per-client, so this is disk-only here.
	{
		nlohmann::ordered_json guide = nlohmann::ordered_json::object();
		guide["loaded"] = saveLoadedGuideTopics(); // std::vector<std::string> -> JSON array
		guide["disableTopicPopups"] = getGameGuideDisableTopicPopups();
		j["guideTopics"] = std::move(guide);
	}

	// The live effect list (smoke, fire, explosions, debris). Purely so a loaded game looks like the
	// one that was saved, ex. a destroyed oil derrick still burning. Per-client and display-only, so
	// it belongs here rather than in the game state document. Empty in headless mode.
	j["effects"] = serializeActiveEffects();
	return j;
}

static void readLocalState(const nlohmann::ordered_json &j)
{
	if (!j.is_object())
	{
		return;
	}
	if (j.contains("camera"))
	{
		const nlohmann::ordered_json &cam = j.at("camera");
		playerPos.p.x = cam.value("px", playerPos.p.x);
		playerPos.p.y = cam.value("py", playerPos.p.y);
		playerPos.p.z = cam.value("pz", playerPos.p.z);
		playerPos.r.x = cam.value("rx", playerPos.r.x);
		playerPos.r.y = cam.value("ry", playerPos.r.y);
		playerPos.r.z = cam.value("rz", playerPos.r.z);
	}
	if (j.contains("radarZoom") && gameWorld.map.tiles)
	{
		SetRadarZoom(gameWorld.map, j.at("radarZoom").get<uint8_t>());
	}
	Cheated = j.value("cheated", Cheated);

	// Loaded (unlocked) guide topics + popup-disable toggle (see writeLocalState).
	if (j.contains("guideTopics") && j.at("guideTopics").is_object())
	{
		const nlohmann::ordered_json &guide = j.at("guideTopics");
		if (guide.contains("loaded") && guide.at("loaded").is_array())
		{
			restoreLoadedGuideTopics(guide.at("loaded").get<std::vector<std::string>>());
		}
		setGameGuideDisableTopicPopups(guide.value("disableTopicPopups", false));
	}

	// The saved effect list (see writeLocalState). Applied after the world restore, so it also clears
	// the effects that restore re-created. Absent for saves written before this section existed, in
	// which case the world restore's own effects are simply left alone.
	if (j.contains("effects") && j.at("effects").is_array())
	{
		restoreActiveEffects(j.at("effects").get<std::vector<nlohmann::ordered_json>>());
	}
}

// MARK: - Pending resume input (disk savegame only)
//
// The in-flight game-action command backlog: messages received or issued before the save tick but
// scheduled (by lockstep latency) to execute after it. This is NOT current world state - it is the
// pending future input that lets a cold-load continue the exact timeline the saving instance would have.
// A network joiner gets these fed by the host's GAME_* relay, so this section is disk-only. Raw
// net-message bytes are only format-compatible within one netcode version, so the version is stamped and
// checked on restore (mismatch drops the backlog and proceeds without the in-flight commands, a valid if
// not byte-identical continuation).

constexpr uint32_t PENDING_RESUME_SECTION_VERSION = 1;

static nlohmann::ordered_json writePendingResume()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = PENDING_RESUME_SECTION_VERSION;
	nlohmann::ordered_json gq = nlohmann::ordered_json::object();
	gq["netcodeMajor"] = NETGetMajorVersion();
	gq["netcodeMinor"] = NETGetMinorVersion();
	nlohmann::ordered_json players = nlohmann::ordered_json::array();
	for (unsigned p = 0; p < MAX_GAMEQUEUE_SLOTS; ++p)
	{
		nlohmann::ordered_json msgs = nlohmann::ordered_json::array();
		for (const std::vector<uint8_t> &raw : NETgameQueueCapturePending(p))
		{
			msgs.push_back(base64Encode(raw)); // one base64 string per raw net message
		}
		players.push_back(std::move(msgs));
	}
	gq["players"] = std::move(players);
	j["gameQueue"] = std::move(gq);
	return j;
}

// Re-inject the pending game-queue backlog. Runs in coldLoadRestoreWorld, after stageTwoInitialise
// allocated the (empty) game queues and before the first post-load tick, so re-pushed messages read back
// in their original order at their scheduled gameTimes.
static void applyPendingResume(const nlohmann::ordered_json &j)
{
	if (!j.is_object() || !j.contains("gameQueue"))
	{
		return;
	}
	const nlohmann::ordered_json &gq = j.at("gameQueue");
	const uint32_t savedMajor = gq.value("netcodeMajor", 0u);
	const uint32_t savedMinor = gq.value("netcodeMinor", 0u);
	if (savedMajor != NETGetMajorVersion() || savedMinor != NETGetMinorVersion())
	{
		debug(LOG_INFO, "Savegame netcode version (0x%" PRIx32 ", 0x%" PRIx32 ") differs from current "
			"(0x%" PRIx32 ", 0x%" PRIx32 "); not restoring pending game-queue messages (proceeding "
			"without in-flight commands)", savedMajor, savedMinor, NETGetMajorVersion(), NETGetMinorVersion());
		return;
	}
	if (!gq.contains("players") || !gq.at("players").is_array())
	{
		return;
	}
	const nlohmann::ordered_json &players = gq.at("players");
	size_t attempted = 0, restored = 0;
	for (unsigned p = 0; p < players.size() && p < MAX_GAMEQUEUE_SLOTS; ++p)
	{
		const nlohmann::ordered_json &msgs = players[p];
		if (!msgs.is_array())
		{
			continue;
		}
		std::vector<std::vector<uint8_t>> raws;
		for (const nlohmann::ordered_json &one : msgs)
		{
			raws.push_back(base64Decode(one.get<std::string>()));
		}
		attempted += raws.size();
		restored += NETgameQueueRestorePending(p, raws);
	}
	if (restored != attempted)
	{
		debug(LOG_INFO, "Restored only %zu of %zu pending game-queue message(s) (queues not ready or "
			"malformed data)", restored, attempted);
	}
	else if (restored > 0)
	{
		debug(LOG_INFO, "Restored %zu pending in-flight game-queue message(s) from savegame", restored);
	}
}

std::vector<uint8_t> serializeSavegameContainer(SaveType saveType)
{
	nlohmann::ordered_json doc = nlohmann::ordered_json::object();
	doc["format"] = SAVEGAME_FORMAT_TAG;
	doc["version"] = SAVEGAME_CONTAINER_VERSION;
	doc["saveType"] = static_cast<uint8_t>(saveType);
	// Setup/identity header first (parseable before mods/level), then the full GameState document, the
	// disk-only local view/meta state (camera, radar zoom, cheated), and the disk-only pending resume
	// input (the in-flight game-queue backlog).
	doc["setup"] = writeSetupHeader(saveType);
	doc["gameState"] = gameStateToJson();
	doc["localState"] = writeLocalState();
	doc["pendingResume"] = writePendingResume();

	const std::string json = doc.dump();

	// Store the container document as a single "gamestate.json" entry inside an in-memory zip archive.
	// createZipArchiveMemory hands back the finished archive bytes through the on-close closure, which
	// runs when the writer's last reference is released (end of the block below). fixedLastMod keeps the
	// archive deterministic (no wall-clock mtime), so identical state produces identical bytes.
	std::unique_ptr<std::vector<uint8_t>> zipBytes;
	{
		auto zip = WzMapZipIO::createZipArchiveMemory(
			[&zipBytes](std::unique_ptr<std::vector<uint8_t>> contents) { zipBytes = std::move(contents); },
			/*fixedLastMod=*/true);
		if (!zip)
		{
			throw StateError("failed to create in-memory savegame zip");
		}
		if (!zip->writeFullFile(kContainerJsonName, json.data(), static_cast<uint32_t>(json.size())))
		{
			throw StateError("failed to write savegame container document into zip");
		}
	} // writer released here -> on-close closure populates zipBytes

	if (!zipBytes)
	{
		throw StateError("savegame zip finalization produced no data");
	}
	return std::move(*zipBytes);
}

SetupHeaderInfo parseSavegameContainer(const uint8_t *data, size_t len, nlohmann::ordered_json &outGameStateDoc, nlohmann::ordered_json *outLocalStateDoc, nlohmann::ordered_json *outPendingResumeDoc)
{
	if (data == nullptr || len == 0)
	{
		throw StateError("savegame container empty");
	}

	// Open the .wz as a standard zip and extract the single container document entry. The maxFileSize
	// cap bounds the decompressed size, so a crafted archive claiming an enormous entry is rejected up
	// front rather than driving an unbounded allocation.
	auto zipContents = std::make_unique<std::vector<uint8_t>>(data, data + len);
	auto zip = WzMapZipIO::openZipArchiveMemory(std::move(zipContents));
	if (!zip)
	{
		throw StateError("not a savegame container (failed to open as zip)");
	}

	std::vector<char> jsonBuf;
	const WzMap::IOProvider::LoadFullFileResult rc =
		zip->loadFullFile(kContainerJsonName, jsonBuf, SAVEGAME_MAX_UNCOMPRESSED, /*appendNullCharacter=*/false);
	if (rc == WzMap::IOProvider::LoadFullFileResult::FAILURE_EXCEEDS_MAXFILESIZE)
	{
		throw StateError("savegame container document exceeds maximum allowed size");
	}
	if (rc != WzMap::IOProvider::LoadFullFileResult::SUCCESS)
	{
		throw StateError(std::string("savegame container missing/unreadable entry '") + kContainerJsonName + "'");
	}

	nlohmann::ordered_json doc;
	try
	{
		// Depth-bound the whole container document (including the nested gameState/scripting sections) at
		// this single ingress parse, so downstream restore cannot be driven into unbounded native recursion.
		doc = parseJsonBounded(jsonBuf.data(), jsonBuf.data() + jsonBuf.size());
	}
	catch (const nlohmann::ordered_json::exception &e)
	{
		throw StateError(std::string("failed to parse savegame container JSON: ") + e.what());
	}
	if (!doc.is_object() || doc.value("format", std::string()) != SAVEGAME_FORMAT_TAG)
	{
		throw StateError("not a savegame container (bad format tag)");
	}
	if (doc.value("version", 0u) != SAVEGAME_CONTAINER_VERSION)
	{
		throw StateError("unsupported savegame container document version");
	}
	if (!doc.contains("setup") || !doc.contains("gameState"))
	{
		throw StateError("savegame container missing setup/gameState sections");
	}

	// Restore the setup/identity globals; hand back the GameState document to apply after level load.
	SetupHeaderInfo info = readSetupHeader(doc.at("setup"));
	outGameStateDoc = doc.at("gameState");
	// Local view/meta state and the pending resume input are both applied late. Tolerate a caller that
	// does not request them (null out-params) or a container without those sections.
	if (outLocalStateDoc != nullptr)
	{
		*outLocalStateDoc = doc.contains("localState") ? doc.at("localState") : nlohmann::ordered_json::object();
	}
	if (outPendingResumeDoc != nullptr)
	{
		*outPendingResumeDoc = doc.contains("pendingResume") ? doc.at("pendingResume") : nlohmann::ordered_json::object();
	}
	return info;
}

// MARK: - Metadata sidecar

nlohmann::ordered_json buildMetadataSidecar(const SavegameMetadata &meta)
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["format"] = SIDECAR_FORMAT_TAG;
	j["version"] = SAVEGAME_CONTAINER_VERSION;
	j["saveName"] = meta.saveName;
	j["timestampEpoch"] = meta.timestampEpoch;
	j["timestampHuman"] = meta.timestampHuman;
	j["buildTag"] = meta.buildTag;
	j["latestTagArray"] = nlohmann::ordered_json::array({ meta.latestTagArray[0], meta.latestTagArray[1], meta.latestTagArray[2] });
	j["gameTime"] = meta.gameTime;
	j["levelName"] = meta.levelName;
	j["playerCount"] = meta.playerCount;
	j["saveType"] = static_cast<uint8_t>(meta.saveType);
	nlohmann::ordered_json jmods = nlohmann::ordered_json::array();
	for (const SavegameMod &m : meta.mods)
	{
		nlohmann::ordered_json jm = nlohmann::ordered_json::object();
		jm["name"] = m.name;
		jm["hash"] = m.hash;
		jmods.push_back(std::move(jm));
	}
	j["mods"] = std::move(jmods);
	return j;
}

SavegameMetadata parseMetadataSidecar(const nlohmann::ordered_json &j)
{
	if (!j.is_object() || j.value("format", std::string()) != SIDECAR_FORMAT_TAG)
	{
		throw StateError("not a savegame metadata sidecar (bad format tag)");
	}
	SavegameMetadata meta;
	meta.saveName = j.value("saveName", std::string());
	meta.timestampEpoch = j.value("timestampEpoch", static_cast<int64_t>(0));
	meta.timestampHuman = j.value("timestampHuman", std::string());
	meta.buildTag = j.value("buildTag", std::string());
	if (j.contains("latestTagArray") && j.at("latestTagArray").is_array() && j.at("latestTagArray").size() == 3)
	{
		const nlohmann::ordered_json &lta = j.at("latestTagArray");
		meta.latestTagArray = { lta[0].get<uint16_t>(), lta[1].get<uint16_t>(), lta[2].get<uint16_t>() };
	}
	meta.gameTime = j.value("gameTime", 0u);
	meta.levelName = j.value("levelName", std::string());
	meta.playerCount = j.value("playerCount", 0u);
	meta.saveType = static_cast<SaveType>(j.value("saveType", static_cast<uint8_t>(SaveType::Skirmish)));
	if (j.contains("mods") && j.at("mods").is_array())
	{
		for (const nlohmann::ordered_json &jm : j.at("mods"))
		{
			meta.mods.push_back(SavegameMod{ jm.value("name", std::string()), jm.value("hash", std::string()) });
		}
	}
	return meta;
}

// MARK: - Folder container I/O (PHYSFS)

bool writeSavegameFolder(const std::string &folderPath, SaveType saveType, const SavegameMetadata &meta)
{
	if (!PHYSFS_exists(folderPath.c_str()))
	{
		if (PHYSFS_mkdir(folderPath.c_str()) == 0)
		{
			debug(LOG_ERROR, "Failed to create savegame folder %s", folderPath.c_str());
			return false;
		}
	}

	const std::vector<uint8_t> blob = serializeSavegameContainer(saveType);
	const std::string blobPath = folderPath + "/" + kStateBlobFileName;
	if (!saveFile(blobPath.c_str(), reinterpret_cast<const char *>(blob.data()), static_cast<UDWORD>(blob.size())))
	{
		debug(LOG_ERROR, "Failed to write savegame state blob %s", blobPath.c_str());
		return false;
	}

	const std::string infoStr = buildMetadataSidecar(meta).dump(4);
	const std::string infoPath = folderPath + "/" + kSidecarFileName;
	if (!saveFile(infoPath.c_str(), infoStr.c_str(), static_cast<UDWORD>(infoStr.size())))
	{
		debug(LOG_ERROR, "Failed to write savegame metadata %s", infoPath.c_str());
		return false;
	}
	return true;
}

bool readSavegameFolder(const std::string &folderPath, SetupHeaderInfo &outHeader,
                        nlohmann::ordered_json &outGameStateDoc, SavegameMetadata &outMeta)
{
	// Metadata sidecar (plain JSON) first - cheap, and lets a caller bail before opening the state zip.
	std::vector<char> infoBuf;
	const std::string infoPath = folderPath + "/" + kSidecarFileName;
	if (!loadFileToBufferVector(infoPath.c_str(), infoBuf, false, false))
	{
		debug(LOG_ERROR, "Failed to read savegame metadata %s", infoPath.c_str());
		return false;
	}
	try
	{
		outMeta = parseMetadataSidecar(nlohmann::ordered_json::parse(infoBuf.begin(), infoBuf.end()));
	}
	catch (const nlohmann::ordered_json::exception &e)
	{
		throw StateError(std::string("failed to parse savegame metadata: ") + e.what());
	}

	// Compressed state blob.
	std::vector<char> blobBuf;
	const std::string blobPath = folderPath + "/" + kStateBlobFileName;
	if (!loadFileToBufferVector(blobPath.c_str(), blobBuf, false, false))
	{
		debug(LOG_ERROR, "Failed to read savegame state blob %s", blobPath.c_str());
		return false;
	}
	outHeader = parseSavegameContainer(reinterpret_cast<const uint8_t *>(blobBuf.data()), blobBuf.size(), outGameStateDoc);
	return true;
}

// MARK: - Cold-load orchestration

/// Warn (do not refuse) when a recorded mod is missing or its hash differs from what is loaded.
static void validateModsWarn(const std::vector<SavegameMod> &expected)
{
	std::map<std::string, std::string> loaded; // name -> hash
	for (const SavegameMod &m : collectLoadedMods())
	{
		loaded[m.name] = m.hash;
	}
	for (const SavegameMod &m : expected)
	{
		auto it = loaded.find(m.name);
		if (it == loaded.end())
		{
			debug(LOG_ERROR, "Savegame expects mod '%s' which is not loaded - loading anyway; stat ids may not resolve", m.name.c_str());
		}
		else if (!m.hash.empty() && !it->second.empty() && m.hash != it->second)
		{
			debug(LOG_ERROR, "Savegame mod '%s' version differs (expected %s, loaded %s) - loading anyway", m.name.c_str(), m.hash.c_str(), it->second.c_str());
		}
	}
}

bool coldLoadRemountModsAndResolveLevel(const SetupHeaderInfo &header)
{
	// 1. Tell the mod system which mods this save expects (overrides the current selection).
	if (!header.mods.empty())
	{
		std::string modlist;
		for (const SavegameMod &m : header.mods)
		{
			if (!modlist.empty()) { modlist += ", "; }
			modlist += m.name;
		}
		std::vector<char> buf(modlist.begin(), modlist.end());
		buf.push_back('\0');
		setOverrideMods(buf.data());
	}
	else
	{
		clearOverrideMods();
	}

	// 2. Re-scan the level list with the save's mods mounted, so addon-campaign content (its
	//    .lev/gamedesc.lev entries, rules/stats/VIEWDATA/scripts, campaign json) is discoverable.
	//    Mirrors the legacy campaign-mod remount in game.cpp.
	const searchPathMode mode = (header.saveType == SaveType::Campaign) ? mod_campaign : mod_multiplay;
	levShutDown();
	levInitialise();
	rebuildSearchPath(mode, true);
	pal_Init(); // palettes can come from mods
	if (!buildMapList(mode == mod_campaign))
	{
		debug(LOG_ERROR, "Failed to rebuild level list while loading savegame");
	}

	// 3. Warn-and-continue mod validation (never refuse: players legitimately update mods mid-play, such as bundled mods w/ WZ updates).
	validateModsWarn(header.mods);

	// 4. Resolve the level dataset by name (+ map hash for non-built-in maps).
	Sha256 hash = header.mapHash;
	LEVEL_DATASET *ds = levFindDataSet(header.levelName.c_str(), header.builtInMap ? nullptr : &hash);
	if (ds == nullptr)
	{
		debug(LOG_ERROR, "Could not resolve level '%s' for savegame (missing map or mod?)", header.levelName.c_str());
		return false;
	}
	return true;
}

// MARK: - Container self-test

bool runSavegameContainerSelfTest()
{
	bool ok = true;
	const auto check = [&ok](bool cond, const char *msg)
	{
		if (!cond)
		{
			fprintf(stderr, "[savegame-container-selftest] FAIL: %s\n", msg);
			ok = false;
		}
	};

	if (NetPlay.players.size() < MAX_PLAYERS)
	{
		NetPlay.players.resize(MAX_PLAYERS);
	}

	// Arrange a known setup + a known determinism clock (carried by the embedded GameState).
	selectedPlayer = 5;
	sstrcpy(aLevelName, "Sk-ContainerLevel");
	game.type = LEVEL_TYPE::SKIRMISH;
	game.maxPlayers = 4;
	setGameTime(777777u);

	std::vector<uint8_t> blob1;
	try
	{
		blob1 = serializeSavegameContainer(SaveType::Skirmish);
	}
	catch (const std::exception &e)
	{
		check(false, e.what());
		return ok;
	}

	// Byte-stability: same live state must produce the same blob.
	std::vector<uint8_t> blob2 = serializeSavegameContainer(SaveType::Skirmish);
	check(blob1 == blob2, "byte-stability: re-serialized container blob differs");

	// Perturb both the setup globals and the determinism clock.
	selectedPlayer = 0;
	sstrcpy(aLevelName, "wrong");
	game.maxPlayers = 0;
	setGameTime(1u);

	// Stage 1: parse restores setup globals but NOT the GameState (no level load needed here).
	nlohmann::ordered_json gsDoc;
	SetupHeaderInfo info;
	try
	{
		info = parseSavegameContainer(blob1.data(), blob1.size(), gsDoc);
	}
	catch (const std::exception &e)
	{
		check(false, e.what());
		return ok;
	}
	check(selectedPlayer == 5u, "container: setup selectedPlayer not restored");
	check(std::string(aLevelName) == "Sk-ContainerLevel", "container: setup levelName not restored");
	check(info.levelName == "Sk-ContainerLevel", "container: info.levelName wrong");
	check(info.saveType == SaveType::Skirmish, "container: info.saveType wrong");
	check(gameTime == 1u, "container: GameState must not be applied during parse");

	// Stage 2: applying the embedded GameState restores the determinism clock.
	try
	{
		gameStateFromJson(gsDoc);
	}
	catch (const std::exception &e)
	{
		check(false, e.what());
		return ok;
	}
	check(gameTime == 777777u, "container: GameState clock not restored after apply");

	// Metadata sidecar round-trip.
	SavegameMetadata meta;
	meta.saveName = "My Save";
	meta.timestampEpoch = 1700000000;
	meta.timestampHuman = "2023-11-14 22:13:20";
	meta.buildTag = "test-build";
	meta.gameTime = 777777u;
	meta.levelName = "Sk-ContainerLevel";
	meta.playerCount = 4;
	meta.saveType = SaveType::Campaign;
	meta.mods.push_back(SavegameMod{ "addon-campaign", "abcd" });
	const nlohmann::ordered_json jmeta = buildMetadataSidecar(meta);
	SavegameMetadata meta2;
	try
	{
		meta2 = parseMetadataSidecar(jmeta);
	}
	catch (const std::exception &e)
	{
		check(false, e.what());
		return ok;
	}
	check(meta2.saveName == "My Save", "sidecar: saveName not restored");
	check(meta2.timestampEpoch == 1700000000, "sidecar: timestampEpoch not restored");
	check(meta2.gameTime == 777777u, "sidecar: gameTime not restored");
	check(meta2.levelName == "Sk-ContainerLevel", "sidecar: levelName not restored");
	check(meta2.playerCount == 4u, "sidecar: playerCount not restored");
	check(meta2.saveType == SaveType::Campaign, "sidecar: saveType not restored");
	check(meta2.mods.size() == 1 && meta2.mods[0].name == "addon-campaign" && meta2.mods[0].hash == "abcd", "sidecar: mods not restored");

	if (ok)
	{
		fprintf(stderr, "[savegame-container-selftest] PASS (%zu-byte container blob)\n", blob1.size());
	}
	return ok;
}

// MARK: - Save side

// Build the metadata sidecar contents from the current live game state.
static SavegameMetadata buildLiveSavegameMetadata(const std::string &saveName, SaveType saveType)
{
	SavegameMetadata meta;
	meta.saveName = saveName;
	meta.timestampEpoch = static_cast<int64_t>(std::time(nullptr));
	char timebuf[64] = {0};
	const std::time_t now = static_cast<std::time_t>(meta.timestampEpoch);
	const std::tm *lt = std::localtime(&now);
	if (lt != nullptr)
	{
		std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", lt);
	}
	meta.timestampHuman = timebuf;
	meta.gameTime = static_cast<uint32_t>(gameTime);
	meta.levelName = aLevelName;
	meta.playerCount = NetPlay.playercount;
	meta.saveType = saveType;
	meta.mods = collectLoadedMods();
	meta.buildTag = version_getVersionString();
	// Numeric [major, minor, patch] of the latest release tag, for version comparison (matches the legacy
	// save-info.json). Zeroed if no tag is extractable.
	const TagVer tag = version_extractVersionNumberFromTag(version_getLatestTag()).value_or(TagVer());
	meta.latestTagArray = { tag.version[0], tag.version[1], tag.version[2] };
	return meta;
}

bool writeCurrentGameToFolder(const std::string &folderPath, const std::string &saveName, SaveType saveType)
{
	return writeSavegameFolder(folderPath, saveType, buildLiveSavegameMetadata(saveName, saveType));
}

// MARK: - Coexistence with the legacy folder save

/// Strip a trailing ".gam" (the legacy save extension the menu appends) to get the folder path.
static std::string saveFolderPathFromName(const std::string &name)
{
	const std::string ext = ".gam";
	if (name.size() > ext.size() && name.compare(name.size() - ext.size(), ext.size(), ext) == 0)
	{
		return name.substr(0, name.size() - ext.size());
	}
	return name;
}

bool writeGameStateBlobToFolder(const std::string &folderPath, SaveType saveType)
{
	const std::string dir = saveFolderPathFromName(folderPath);
	std::vector<uint8_t> blob;
	try
	{
		blob = serializeSavegameContainer(saveType);
	}
	catch (const std::exception &e)
	{
		debug(LOG_ERROR, "Failed to serialize GameState savegame blob: %s", e.what());
		return false;
	}
	const std::string blobPath = dir + "/" + kStateBlobFileName;
	if (!saveFile(blobPath.c_str(), reinterpret_cast<const char *>(blob.data()), static_cast<UDWORD>(blob.size())))
	{
		debug(LOG_ERROR, "Failed to write GameState savegame blob %s", blobPath.c_str());
		return false;
	}

	// Also write the new-format metadata sidecar under its own filename (so it coexists with the legacy
	// save-info.json). Nothing reads it yet - it back-fills the richer metadata (mods, save type, level,
	// game time) onto new saves for a later load menu that prefers it. Non-fatal: the blob is what a
	// cold-load needs. saveName is the folder's base name.
	const std::string saveName = dir.substr(dir.find_last_of('/') + 1);
	const std::string infoStr = buildMetadataSidecar(buildLiveSavegameMetadata(saveName, saveType)).dump(4);
	const std::string infoPath = dir + "/" + kSidecarFileName;
	if (!saveFile(infoPath.c_str(), infoStr.c_str(), static_cast<UDWORD>(infoStr.size())))
	{
		debug(LOG_ERROR, "Failed to write GameState savegame metadata %s (non-fatal)", infoPath.c_str());
	}

	// Arm the CRC-trace detail auto-dump (no-op unless --gamestate-crc-detail-on-save): captures the
	// next few ticks' full sync logs on the saving run, to diff against the loaded run (see below).
	syncCrcDetailArmOnSaveOrLoad();
	return true;
}

/// Read just the GameState blob (gamestate.wz) from a save folder, restoring the setup globals and
/// yielding the embedded GameState document. The folder's own (legacy) save-info.json is not read.
static bool readGameStateBlobFromFolder(const std::string &folderPath, SetupHeaderInfo &outHeader,
                                        nlohmann::ordered_json &outGameStateDoc, nlohmann::ordered_json *outLocalStateDoc = nullptr,
                                        nlohmann::ordered_json *outPendingResumeDoc = nullptr)
{
	const std::string dir = saveFolderPathFromName(folderPath);
	std::vector<char> blobBuf;
	const std::string blobPath = dir + "/" + kStateBlobFileName;
	if (!loadFileToBufferVector(blobPath.c_str(), blobBuf, false, false))
	{
		debug(LOG_ERROR, "Failed to read GameState savegame blob %s", blobPath.c_str());
		return false;
	}
	outHeader = parseSavegameContainer(reinterpret_cast<const uint8_t *>(blobBuf.data()), blobBuf.size(), outGameStateDoc, outLocalStateDoc, outPendingResumeDoc);
	return true;
}

// MARK: - Cold-load (level load + game start) wiring
//
// A cold load restores the world at level-load time. Its scripting section is applied a bit later in the
// same level load (levFinalizeLevelLoad, once all data files have instantiated the scripts) but still
// before stageThreeInitialise/prepareScripts. We stash the GameState document across those steps.

static std::unique_ptr<nlohmann::ordered_json> g_coldLoadGameStateDoc;
static std::unique_ptr<nlohmann::ordered_json> g_coldLoadLocalStateDoc; // disk-only view/meta, applied post-restore
static std::unique_ptr<nlohmann::ordered_json> g_coldLoadPendingResumeDoc; // disk-only pending game-queue backlog, re-injected post-restore
static bool g_coldLoadReconstruct = false; // set at end of coldLoadRestoreWorld, consumed by stageThreeInitialise

bool isNewFormatSaveFolder(const std::string &folderPath)
{
	const std::string blobPath = saveFolderPathFromName(folderPath) + "/" + kStateBlobFileName;
	return PHYSFS_exists(blobPath.c_str()) != 0;
}

static bool g_preferLegacyLoadOverride = false;

void setPreferLegacyLoadOverride(bool prefer)
{
	g_preferLegacyLoadOverride = prefer;
}

bool preferLegacyLoadOverride()
{
	return g_preferLegacyLoadOverride;
}

bool takeColdLoadReconstructFlag()
{
	const bool wasReconstruct = g_coldLoadReconstruct;
	g_coldLoadReconstruct = false;
	return wasReconstruct;
}

void applyDeferredScripting()
{
	if (!g_coldLoadGameStateDoc)
	{
		return;
	}
	try
	{
		// The script instances were created while the level data loaded (loadGlobalScript / loadMultiScripts),
		// so the scripting section's loadScriptStates() takes effect here even though scriptsReady is not set
		// yet (applyGameStateScripting passes requireScriptsReady=false). This restores the script globals
		// before prepareScripts drains the queued research events, so their handlers see the restored state.
		gamestate::applyGameStateScripting(*g_coldLoadGameStateDoc);
	}
	catch (const std::exception &e)
	{
		debug(LOG_ERROR, "Failed to apply deferred savegame scripting: %s", e.what());
	}
	g_coldLoadGameStateDoc.reset();
}

void applyDeferredColdLoadMessages()
{
	if (!g_coldLoadGameStateDoc)
	{
		return;
	}
	try
	{
		// Runs in levFinalizeLevelLoad, after the data-file loop, so the level's message VIEWDATA is
		// loaded and getViewData() resolves. Does NOT reset the stashed doc - the deferred scripting pass
		// (applyDeferredScripting) still needs it.
		gamestate::applyGameStateMessages(*g_coldLoadGameStateDoc);
	}
	catch (const std::exception &e)
	{
		debug(LOG_ERROR, "Failed to apply deferred savegame messages: %s", e.what());
	}
}

bool coldLoadRestoreWorld()
{
	if (!g_coldLoadGameStateDoc)
	{
		debug(LOG_ERROR, "coldLoadRestoreWorld called with no stashed GameState");
		return false;
	}
	// Canonical reset of the global derived state the scenario load (which placed the map + starting
	// units) dirtied and that the snapshot does not reset on its own. Mirrors the legacy load reset
	// (game.cpp): factory-number flags and proximity-message displays. (Per-world object lists +
	// extractor/sensor/oil indices + structure curCount are reset inside gameStateFromJson's
	// clearWorldObjects; projectiles are reset by its projectiles section.)
	initFactoryNumFlag();
	releaseAllProxDisp();

	try
	{
		// Build the world from the snapshot: terrain (overwritten in place over the scenario map),
		// objects, power/research/production/etc. The scripting section is skipped here (this runs mid-way
		// through the data-file loop, before all scripts are instantiated) and is restored by
		// applyDeferredScripting() in levFinalizeLevelLoad, after the remaining data files have loaded.
		// Defer the messages section: the level's message VIEWDATA is loaded from later-index data files
		// that have not been processed yet here. applyDeferredColdLoadMessages() replays it in
		// levFinalizeLevelLoad once level data has finished loading (matching the legacy loadSaveMessage).
		gamestate::gameStateFromJson(*g_coldLoadGameStateDoc, gamestate::ScriptScope::AllInstances, /*deferMessages=*/true);
	}
	catch (const std::exception &e)
	{
		debug(LOG_ERROR, "Failed to reconstruct world from savegame: %s", e.what());
		return false;
	}

	// The scenario load computed the display ground types from its own per-tile textures; the snapshot
	// may have overwritten those textures (random/edited maps), so recompute ground from the restored
	// textures. No-op-equivalent for static maps (textures match); never touches sim heights/water.
	if (gameWorld.map.tiles)
	{
		mapReloadGroundTypes();
	}

	// Apply the disk-only local view/meta (camera, radar zoom, cheated) now that the map exists. Absent
	// for the network path (it never carries localState) and harmless if missing (no-op object).
	if (g_coldLoadLocalStateDoc)
	{
		readLocalState(*g_coldLoadLocalStateDoc);
		g_coldLoadLocalStateDoc.reset();
	}

	// Re-inject the pending game-queue backlog now that stageTwoInitialise allocated the (empty) queues
	// and before the first post-load tick. Disk-only, harmless if missing.
	if (g_coldLoadPendingResumeDoc)
	{
		applyPendingResume(*g_coldLoadPendingResumeDoc);
		g_coldLoadPendingResumeDoc.reset();
	}

	// Arm the CRC-trace detail auto-dump (no-op unless --gamestate-crc-detail-on-save): gameTime is now
	// restored to the save tick, so the loaded run's first traced ticks get their full sync logs dumped
	// to diff against the saving run's window (see syncCrcDetailArmOnSaveOrLoad).
	syncCrcDetailArmOnSaveOrLoad();

	// Mark this game start as a cold-load reconstruct. stageThreeInitialise consumes it (bInTutorial guard);
	// the scripting section is restored separately by applyDeferredScripting() in levFinalizeLevelLoad.
	g_coldLoadReconstruct = true;
	return true;
}

/// Re-establish the local player's multiplayer profile identity + stats. The load process resets the
/// per-slot PLAYERSTATS, so without this the local identity is empty and saveMultiStats() on quit
/// refuses to save ("Refusing to save profile with empty identity"). Mirrors the legacy save load
/// (game.cpp gameLoadV) and a normal skirmish start: load the profile by player name (generating an
/// identity if none is on disk) and set it for the local slot.
static void restoreLocalPlayerMultiStats()
{
	if (!bMultiPlayer || selectedPlayer >= MAX_PLAYERS)
	{
		return;
	}
	char playerName[StringSize];
	sstrcpy(playerName, getPlayerName(selectedPlayer));
	PLAYERSTATS stats = getMultiStats(selectedPlayer); // preserve an already-loaded identity, if any
	loadMultiStats(playerName, &stats);
	setMultiStats(selectedPlayer, stats, false);
	setMultiStats(selectedPlayer, stats, true);
}

LoadingTask<> coldLoadGameInit(ResourceLoadingController &controller, const std::string &folderPath)
{
	// Tear down any currently-loaded game first. For an in-game load, stopGameLoop() deliberately skips
	// levReleaseAll() when gameLoopStatus == GAMECODE_LOADGAME (the legacy gameLoad does it itself, after
	// its setup restore), so without this the previous match's data is still live: stageOne/TwoShutDown
	// (grid, stats, structures) and the "Main game loop" scene end never run, and the reload hits duplicate
	// stats / "gridInitialise already called" / out-of-order-scene asserts. Mirrors legacy gameLoad, and it
	// is a safe near-no-op when loading from the menu (nothing is loaded). Must run BEFORE the setup restore
	// below so it does not reset the restored globals.
	if (!levReleaseAll())
	{
		debug(LOG_ERROR, "levReleaseAll failed before cold load. Attempting to load anyway");
	}

	// Read the GameState blob (gamestate.wz) from the folder: restores the setup/identity globals
	// (incl. aLevelName + game.hash) and hands back the embedded GameState document to apply after
	// the level is loaded. The folder's legacy save-info.json / .json files are ignored.
	SetupHeaderInfo header;
	nlohmann::ordered_json gsDoc;
	nlohmann::ordered_json localDoc;
	nlohmann::ordered_json pendingDoc;
	bool readOk = false;
	try
	{
		readOk = readGameStateBlobFromFolder(folderPath, header, gsDoc, &localDoc, &pendingDoc);
	}
	catch (const std::exception &e)
	{
		debug(LOG_ERROR, "Failed to parse savegame '%s': %s", folderPath.c_str(), e.what());
		co_return load_fail();
	}
	if (!readOk)
	{
		debug(LOG_ERROR, "Failed to read savegame blob in '%s'", folderPath.c_str());
		co_return load_fail();
	}

	// Remount the save's mods (warn-and-continue), rescan the level list, resolve the dataset.
	if (!coldLoadRemountModsAndResolveLevel(header))
	{
		co_return load_fail();
	}

	// Stash the GameState document for the restore hook + the deferred scripting replay (both run
	// inside the level load below, not after it - see coldLoadRestoreWorld / applyDeferredScripting).
	// The disk-only local view/meta is stashed alongside and applied at the end of coldLoadRestoreWorld.
	g_coldLoadGameStateDoc = std::make_unique<nlohmann::ordered_json>(std::move(gsDoc));
	g_coldLoadLocalStateDoc = std::make_unique<nlohmann::ordered_json>(std::move(localDoc));
	g_coldLoadPendingResumeDoc = std::make_unique<nlohmann::ordered_json>(std::move(pendingDoc));
	g_coldLoadReconstruct = false;

	// Load the level in RECONSTRUCT mode: the normal scenario load runs - so the map's
	// display layer (tileset/textures/ground/lightmap) comes from the proven map-load path - and then,
	// right after it (before stageThreeInitialise), levLoadData calls coldLoadRestoreWorld() to replace
	// the just-placed world with the snapshot (clear scenario units, overwrite terrain in place, rebuild
	// saved objects). So prepareScripts(fromSave)/TRIGGER_GAME_LOADED/grid/visibility init all run on the
	// restored world, exactly as a legacy save load does. GTYPE_SAVE_MIDMISSION marks it as a save load:
	// GTYPE_SAVE_START is deprecated (all the save-load setup - levReleaseAll, the game.cpp/init.cpp
	// load-side init, and TRIGGER_GAME_LOADED - now gates on GTYPE_SAVE_MIDMISSION only), and pSaveName is
	// null here so the MIDMISSION savegame-load (loadGame) path is not taken - only its setup runs.
	std::optional<Sha256> hashOpt;
	if (!header.builtInMap)
	{
		hashOpt = header.mapHash;
	}
	if (!(co_await makeLevLoadDataLoadingTask(controller, header.levelName, hashOpt, nullptr, GTYPE_SAVE_MIDMISSION, true)))
	{
		g_coldLoadGameStateDoc.reset();
		g_coldLoadLocalStateDoc.reset();
		g_coldLoadPendingResumeDoc.reset();
		g_coldLoadReconstruct = false;
		debug(LOG_ERROR, "Failed to load/reconstruct savegame '%s'", folderPath.c_str());
		co_return load_fail();
	}

	// Re-establish the local player's profile identity/stats (the load reset them), so the end-of-game
	// saveMultiStats() doesn't bail with an empty identity.
	restoreLocalPlayerMultiStats();
	co_return load_ok();
}

} // namespace savegame
} // namespace gamestate
