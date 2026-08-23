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

#include "gamestate_serialize.h"

#include "lib/framework/frame.h"
#include "lib/framework/math_ext.h" // clip (clamp restored droid positions onto the map)
#include "lib/framework/crc.h" // base64Encode / base64Decode (binary section fields)
#include "lib/gamelib/gtime.h"
#include "lib/netplay/sync_debug.h" // syncDebugGetCrc / setResumeSyncDebugCrc (resume sync-CRC continuity)
#include "lib/netplay/netplay.h"   // NetPlay.scriptSetPlayerDataStrings (scriptPlayerData section)

#include "random.h"
#include "objmem.h"
#include "mechanics.h"       // mechanicsPurgeDestroyedObjects (restore teardown)
#include "ai.h"
#include "power.h"
#include "research.h"
#include "researchdef.h"
#include "stats.h"
#include "structure.h"
#include "structuredef.h"
#include "template.h"
#include "droiddef.h"
#include "game_world.h"
#include "feature.h"
#include "featuredef.h"
#include "mapgrid.h"
#include "map.h"
#include "gateway.h"
#include "loop.h"
#include "console.h"
#include "weapondef.h"
#include "droid.h"
#include "order.h"
#include "orderdef.h"
#include "move.h"
#include "transporter.h"    // transporterGet/SetLaunchTime (simMisc section)
#include "action.h"
#include "group.h"
#include "cmddroid.h"
#include "formation.h"
#include "fpath.h"
#include "visibility.h"
#include "projectile.h"
#include "projectiledef.h"
#include "mission.h"
#include "missiondef.h"
#include "qtscript.h"
#include "message.h"
#include "messagedef.h"
#include "multiplay.h" // CreateBeaconViewData
#include "multistat.h" // PLAYERSTATS, get/setMultiStats
#include "scores.h"    // missionData
#include "wrappers.h"  // testPlayerHasWon/Lost
#include "lighting.h"  // getTheSun/setTheSun (presentation section)
#include "atmos.h"     // atmosGet/SetWeatherType (presentation section)
#include "display3d.h" // setSkyBox / getCurrentSkybox* / radarPermitted (presentation section)
#include "advvis.h"    // get/setRevealStatus (presentation section)
#include "component.h" // get/setPlayerColour (presentation section)
#include "campaigninfo.h" // get/setCampaignNumber + get/setCamTweakOptions (campaign section)
#include "lib/framework/physfs_ext.h" // PHYSFS_exists (skybox page existence guard)
#include "lib/ivis_opengl/pietypes.h"  // LIGHTING_TYPE / PIELIGHT
#include "lib/ivis_opengl/piedef.h"    // pie_GetLighting0 / pie_Lighting0
#include "lib/ivis_opengl/piestate.h"  // pie_Get/SetFogColour

#include <vector>
#include <algorithm>
#include <array>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <cstdlib>
#include <queue>

namespace gamestate
{

// MARK: - Parsed-input validation helpers
//
// Readers validate structural assumptions (array shape, index range, resolved object type) before
// indexing or casting, turning any violation into a StateError. nlohmann's const operator[](size_t)
// does NOT bounds-check the underlying array, so positional j[i] access must be size-guarded first.

static const nlohmann::ordered_json &reqArray(const nlohmann::ordered_json &j, size_t minSize)
{
	if (!j.is_array() || j.size() < minSize)
	{
		throw StateError("expected a JSON array of at least " + std::to_string(minSize) + " element(s)");
	}
	return j;
}

// Read a player index and confirm it is within [0, MAX_PLAYERS).
static unsigned reqPlayer(const nlohmann::ordered_json &j)
{
	const unsigned p = j.get<unsigned>();
	if (p >= MAX_PLAYERS)
	{
		throw StateError("player index out of range");
	}
	return p;
}

// Confirm v is within the inclusive [lo, hi] range (used to bound enum casts). Returns v.
static int reqRange(int v, int lo, int hi)
{
	if (v < lo || v > hi)
	{
		throw StateError("value out of range");
	}
	return v;
}

// Little-endian byte packing for the base64-encoded binary fields (RNG state, map terrain, danger
// overlays). Bulk fixed-width arrays are stored as base64 blobs rather than JSON number arrays. That is
// far smaller uncompressed and faster to parse. Raw bytes have no inherent byte order, so pin
// little-endian here for cross-platform determinism.
static void appendU16le(std::vector<uint8_t> &v, uint16_t x)
{
	v.push_back(static_cast<uint8_t>(x));
	v.push_back(static_cast<uint8_t>(x >> 8));
}
static void appendU32le(std::vector<uint8_t> &v, uint32_t x)
{
	v.push_back(static_cast<uint8_t>(x));
	v.push_back(static_cast<uint8_t>(x >> 8));
	v.push_back(static_cast<uint8_t>(x >> 16));
	v.push_back(static_cast<uint8_t>(x >> 24));
}
static uint16_t readU16le(const uint8_t *p)
{
	return static_cast<uint16_t>(static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8));
}
static uint32_t readU32le(const uint8_t *p)
{
	return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8)
	     | (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

// Decode a base64 field to raw bytes, requiring exactly expectedBytes. The field must be a string, and
// its encoded length is checked against the canonical base64 length for expectedBytes BEFORE decoding.
// That rejects a malformed or oversized field up front and bounds the decode allocation to the expected
// size.
static std::vector<uint8_t> decodeBase64Field(const nlohmann::ordered_json &j, size_t expectedBytes, const char *what)
{
	if (!j.is_string())
	{
		throw StateError(std::string(what) + " must be a base64 string");
	}
	const nlohmann::ordered_json::string_t &s = j.get_ref<const nlohmann::ordered_json::string_t &>();
	if (s.size() != (expectedBytes + 2) / 3 * 4)
	{
		throw StateError(std::string(what) + " base64 length mismatch");
	}
	std::vector<uint8_t> bytes = base64Decode(s);
	if (bytes.size() != expectedBytes)
	{
		throw StateError(std::string(what) + " base64 decoded length mismatch");
	}
	return bytes;
}

// MARK: - Section: determinism core

constexpr uint32_t DETERMINISM_CORE_VERSION = 1;

nlohmann::ordered_json writeDeterminismCore()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = DETERMINISM_CORE_VERSION;

	j["gameTime"] = static_cast<uint32_t>(gameTime);

	// Sync-CRC accumulator at the save boundary (= this tick's object syncDebug, which is attributed to
	// the NEXT sync boundary's CRC). A restored client resets its sync log, so without this its first
	// post-resume boundary CRC - and the GAME_GAME_TIME checkCrc that echoes it - diverges from the
	// serializing instance. Re-seeded after resetSyncDebug() on restore (see applyResumeSyncDebugCrc).
	j["syncDebugCrc"] = syncDebugGetCrc();

	const ObjectIdState ids = getObjectIdState();
	j["synchObjID"] = ids.synchObjID;
	j["unsynchObjID"] = ids.unsynchObjID;

	// SKIRMISH danger-map (AI threat) recompute schedule - file-statics in map.cpp that gate the 2s
	// round-robin in mapUpdate(). Not advanced by reconstruction, so applied with the clock (early).
	j["lastDangerUpdate"] = static_cast<uint32_t>(getLastDangerUpdate());
	j["lastDangerPlayer"] = getLastDangerPlayer();

	// Lockstep network-timing state (latency negotiation + per-queue command scheduling), so a resumed
	// client keeps the same latency instead of renegotiating from defaults (see GameTimeNetState).
	const GameTimeNetState net = getGameTimeNetState();
	nlohmann::ordered_json jnet = nlohmann::ordered_json::object();
	jnet["chosenLatency"] = net.chosenLatency;
	jnet["discreteChosenLatency"] = net.discreteChosenLatency;
	jnet["wantedLatency"] = net.wantedLatency;
	jnet["wantedLatencies"] = net.wantedLatencies;
	jnet["gameQueueTime"] = net.gameQueueTime;
	jnet["gameQueueCheckTime"] = net.gameQueueCheckTime;
	jnet["gameQueueCheckCrc"] = net.gameQueueCheckCrc;
	j["netTiming"] = std::move(jnet);

	const GameRandomState rng = getGameRandomState();
	nlohmann::ordered_json jrng = nlohmann::ordered_json::object();
	jrng["lastSeed"] = rng.lastSeed;
	jrng["offset"] = rng.offset;
	std::vector<uint8_t> rngBytes;
	rngBytes.reserve(RNG_STATE_WORDS * 4);
	for (uint32_t word : rng.state)
	{
		appendU32le(rngBytes, word);
	}
	jrng["state"] = base64Encode(rngBytes);
	j["rng"] = std::move(jrng);

	return j;
}

// The determinism core is applied in two stages during a full restore:
//  - the clock (gameTime) BEFORE object reconstruction, so reconstruction sees the right time
//  - the object-ID counters and RNG state AFTER reconstruction, because reconstruction itself
//    advances synchObjID (e.g. building structure modules) and may consume gameRand. Applying
//    them last makes the restored counters/RNG exactly the authoritative post-snapshot values.

static void applyDeterminismClock(const nlohmann::ordered_json &j, uint32_t version)
{
	if (version != DETERMINISM_CORE_VERSION)
	{
		throw StateError("unsupported determinismCore section version");
	}
	setGameTime(j.at("gameTime").get<uint32_t>());
	// Stash the saved sync-CRC accumulator - the restore path re-seeds it after resetSyncDebug() so the
	// first post-resume sync boundary reproduces a synced CRC (see setResumeSyncDebugCrc).
	setResumeSyncDebugCrc(j.at("syncDebugCrc").get<uint32_t>());

	// Danger-map recompute schedule (see writeDeterminismCore).
	setLastDangerUpdate(j.at("lastDangerUpdate").get<uint32_t>());
	setLastDangerPlayer(reqRange(j.at("lastDangerPlayer").get<int>(), -1, MAX_PLAYERS - 1));

	// Lockstep network-timing state.
	// Restored so the resumed client continues the same latency negotiation / command scheduling.
	if (j.contains("netTiming"))
	{
		const nlohmann::ordered_json &jnet = j.at("netTiming");
		GameTimeNetState net;
		net.chosenLatency = jnet.value("chosenLatency", static_cast<uint16_t>(0));
		net.discreteChosenLatency = jnet.value("discreteChosenLatency", static_cast<uint16_t>(0));
		net.wantedLatency = jnet.value("wantedLatency", static_cast<uint16_t>(0));
		net.wantedLatencies = jnet.value("wantedLatencies", std::vector<uint16_t>{});
		net.gameQueueTime = jnet.value("gameQueueTime", std::vector<uint32_t>{});
		net.gameQueueCheckTime = jnet.value("gameQueueCheckTime", std::vector<uint32_t>{});
		net.gameQueueCheckCrc = jnet.value("gameQueueCheckCrc", std::vector<uint32_t>{});
		setGameTimeNetState(net);
	}
}

static void applyDeterminismCounters(const nlohmann::ordered_json &j, uint32_t version)
{
	if (version != DETERMINISM_CORE_VERSION)
	{
		throw StateError("unsupported determinismCore section version");
	}
	ObjectIdState ids;
	ids.synchObjID = j.at("synchObjID").get<uint32_t>();
	ids.unsynchObjID = j.at("unsynchObjID").get<uint32_t>();

	const nlohmann::ordered_json &jrng = j.at("rng");
	GameRandomState rng;
	rng.lastSeed = jrng.at("lastSeed").get<uint32_t>();
	rng.offset = jrng.at("offset").get<int32_t>();
	const std::vector<uint8_t> stateBytes = decodeBase64Field(jrng.at("state"), RNG_STATE_WORDS * 4, "determinismCore: rng.state");
	for (size_t i = 0; i < RNG_STATE_WORDS; ++i)
	{
		rng.state[i] = readU32le(&stateBytes[i * 4]);
	}

	setObjectIdState(ids);
	setGameRandomState(rng);
}

void readDeterminismCore(const nlohmann::ordered_json &j, uint32_t version)
{
	applyDeterminismClock(j, version);
	applyDeterminismCounters(j, version);
}

// MARK: - Section: diplomacy

constexpr uint32_t DIPLOMACY_SECTION_VERSION = 1;

static nlohmann::ordered_json writeDiplomacy()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = DIPLOMACY_SECTION_VERSION;

	nlohmann::ordered_json jall = nlohmann::ordered_json::array();
	for (unsigned a = 0; a < MAX_PLAYER_SLOTS; ++a)
	{
		nlohmann::ordered_json row = nlohmann::ordered_json::array();
		for (unsigned b = 0; b < MAX_PLAYER_SLOTS; ++b)
		{
			row.push_back(alliances[a][b]);
		}
		jall.push_back(std::move(row));
	}
	j["alliances"] = std::move(jall);

	nlohmann::ordered_json jbits = nlohmann::ordered_json::array();
	for (unsigned a = 0; a < MAX_PLAYER_SLOTS; ++a)
	{
		jbits.push_back(alliancebits[a]);
	}
	j["allianceBits"] = std::move(jbits);

	j["satUplinkBits"] = satuplinkbits;
	return j;
}

static void readDiplomacy(const nlohmann::ordered_json &j, uint32_t version)
{
	if (version != DIPLOMACY_SECTION_VERSION)
	{
		throw StateError("unsupported diplomacy section version");
	}

	const nlohmann::ordered_json &jall = j.at("alliances");
	if (!jall.is_array() || jall.size() != MAX_PLAYER_SLOTS)
	{
		throw StateError("diplomacy.alliances size mismatch");
	}
	for (unsigned a = 0; a < MAX_PLAYER_SLOTS; ++a)
	{
		const nlohmann::ordered_json &row = jall[a];
		if (!row.is_array() || row.size() != MAX_PLAYER_SLOTS)
		{
			throw StateError("diplomacy.alliances row size mismatch");
		}
		for (unsigned b = 0; b < MAX_PLAYER_SLOTS; ++b)
		{
			alliances[a][b] = row[b].get<uint8_t>();
		}
	}

	const nlohmann::ordered_json &jbits = j.at("allianceBits");
	if (!jbits.is_array() || jbits.size() != MAX_PLAYER_SLOTS)
	{
		throw StateError("diplomacy.allianceBits size mismatch");
	}
	for (unsigned a = 0; a < MAX_PLAYER_SLOTS; ++a)
	{
		alliancebits[a] = static_cast<PlayerMask>(jbits[a].get<uint32_t>());
	}

	satuplinkbits = static_cast<PlayerMask>(j.at("satUplinkBits").get<uint32_t>());
}

// MARK: - Section: per-player power

constexpr uint32_t POWER_SECTION_VERSION = 1;

static nlohmann::ordered_json writePower()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = POWER_SECTION_VERSION;

	nlohmann::ordered_json jplayers = nlohmann::ordered_json::array();
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		const PlayerPowerState s = getPlayerPowerState(p);
		nlohmann::ordered_json jp = nlohmann::ordered_json::object();
		jp["currentPower"] = s.currentPower;
		jp["maxStorage"] = s.maxStorage;
		jp["extractedPower"] = s.extractedPower;
		jp["wastedPower"] = s.wastedPower;
		jp["powerGeneratedLastUpdate"] = s.powerGeneratedLastUpdate;
		jp["powerModifier"] = s.powerModifier;
		nlohmann::ordered_json queue = nlohmann::ordered_json::array();
		for (const PowerRequestSave &r : getPlayerPowerQueue(p))
		{
			queue.push_back(nlohmann::ordered_json::array({ r.structId, r.amount }));
		}
		jp["queue"] = std::move(queue);
		jplayers.push_back(std::move(jp));
	}
	j["players"] = std::move(jplayers);
	return j;
}

static void readPower(const nlohmann::ordered_json &j, uint32_t version)
{
	if (version != POWER_SECTION_VERSION)
	{
		throw StateError("unsupported power section version");
	}

	const nlohmann::ordered_json &jplayers = j.at("players");
	if (!jplayers.is_array() || jplayers.size() != MAX_PLAYERS)
	{
		throw StateError("power.players must have MAX_PLAYERS entries");
	}
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		const nlohmann::ordered_json &jp = jplayers[p];
		PlayerPowerState s;
		s.currentPower = jp.at("currentPower").get<int64_t>();
		s.maxStorage = jp.at("maxStorage").get<int64_t>();
		s.extractedPower = jp.at("extractedPower").get<int64_t>();
		s.wastedPower = jp.at("wastedPower").get<int64_t>();
		s.powerGeneratedLastUpdate = jp.at("powerGeneratedLastUpdate").get<int64_t>();
		s.powerModifier = jp.at("powerModifier").get<int>();
		setPlayerPowerState(p, s);

		// The per-player power request queue is restored separately (reapplyPowerQueue), AFTER world
		// reconstruction: the queue is keyed by structure id and structure (re)building mutates it
		// (delPowerRequest/requestPowerFor), so restoring it here - before objects exist - would be
		// clobbered.
	}
}

// Re-apply the authoritative per-player power request queue. Called after the world objects are
// reconstructed (see readPower): structure (re)building runs delPowerRequest/requestPowerFor, which
// would otherwise perturb a queue restored earlier. The queue entries are pure {structId, amount}
// snapshot data and do not require the structures to exist to be stored.
static void reapplyPowerQueue(const nlohmann::ordered_json &j)
{
	const nlohmann::ordered_json &jplayers = j.at("players");
	if (!jplayers.is_array() || jplayers.size() != MAX_PLAYERS)
	{
		throw StateError("power.players must have MAX_PLAYERS entries");
	}
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		const nlohmann::ordered_json &jp = jplayers[p];
		std::vector<PowerRequestSave> queue;
		if (jp.contains("queue"))
		{
			for (const nlohmann::ordered_json &jr : jp.at("queue"))
			{
				reqArray(jr, 2);
				queue.push_back(PowerRequestSave{ jr[0].get<uint32_t>(), jr[1].get<int64_t>() });
			}
		}
		setPlayerPowerQueue(p, queue);
	}
}

// MARK: - Section: research + default component availability

constexpr uint32_t RESEARCH_SECTION_VERSION = 1;

static nlohmann::ordered_json writeResearch()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = RESEARCH_SECTION_VERSION;

	nlohmann::ordered_json jplayers = nlohmann::ordered_json::array();
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		nlohmann::ordered_json jtopics = nlohmann::ordered_json::array();
		for (const PLAYER_RESEARCH &r : asPlayerResList[p])
		{
			nlohmann::ordered_json jt = nlohmann::ordered_json::object();
			jt["points"] = r.currentPoints;
			jt["status"] = r.ResearchStatus;
			jt["possible"] = r.possible;
			jtopics.push_back(std::move(jt));
		}
		jplayers.push_back(std::move(jtopics));
	}
	j["players"] = std::move(jplayers);

	nlohmann::ordered_json jsensor = nlohmann::ordered_json::array();
	nlohmann::ordered_json jecm = nlohmann::ordered_json::array();
	nlohmann::ordered_json jrepair = nlohmann::ordered_json::array();
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		jsensor.push_back(aDefaultSensor[p]);
		jecm.push_back(aDefaultECM[p]);
		jrepair.push_back(aDefaultRepair[p]);
	}
	nlohmann::ordered_json jdef = nlohmann::ordered_json::object();
	jdef["sensor"] = std::move(jsensor);
	jdef["ecm"] = std::move(jecm);
	jdef["repair"] = std::move(jrepair);
	j["defaults"] = std::move(jdef);

	return j;
}

static void readResearch(const nlohmann::ordered_json &j, uint32_t version)
{
	if (version != RESEARCH_SECTION_VERSION)
	{
		throw StateError("unsupported research section version");
	}

	const nlohmann::ordered_json &jplayers = j.at("players");
	if (!jplayers.is_array() || jplayers.size() != MAX_PLAYERS)
	{
		throw StateError("research.players must have MAX_PLAYERS entries");
	}
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		const nlohmann::ordered_json &jtopics = jplayers[p];
		if (!jtopics.is_array())
		{
			throw StateError("research.players entry must be an array");
		}
		// The topic count must match the loaded research definitions (same research data/mods).
		if (jtopics.size() != asPlayerResList[p].size())
		{
			throw StateError("research topic count mismatch (different research data/mods?)");
		}
		for (size_t t = 0; t < jtopics.size(); ++t)
		{
			PLAYER_RESEARCH &r = asPlayerResList[p][t];
			const bool resAlreadyCompleted = IsResearchCompleted(&r);
			r.currentPoints = jtopics[t].at("points").get<uint32_t>();
			r.ResearchStatus = jtopics[t].at("status").get<uint8_t>();
			r.possible = jtopics[t].at("possible").get<uint8_t>();
			// Re-apply the UPGRADE EFFECTS of completed research. We serialize only the research STATUS;
			// the derived upgrade values (structure/component power, research, production, body, ...) live
			// in the global stat tables and, on a cold load, start at their un-researched defaults. Without
			// this a loaded game runs with wrong rates - i.e. getBuildingPowerPoints (upgrade.power +
			// upgrade.modulePower*capacity) is too low, so power generation, economy and research progress
			// all diverge. Mirrors loadSaveResearch. The !resAlreadyCompleted guard avoids double-applying
			// when the research is already applied in the live globals (the in-process round-trip).
			// bDisplay/bTrigger=false: no console/UI and no TRIGGER_RESEARCH script event (those fired
			// in the original run) - any RNG it consumes is overwritten by the determinism counters
			// applied last in gameStateFromJson.
			if (IsResearchCompleted(&r) && !resAlreadyCompleted)
			{
				researchResult(static_cast<UDWORD>(t), static_cast<UBYTE>(p), false, nullptr, false);
			}
		}
	}

	const nlohmann::ordered_json &jdef = j.at("defaults");
	const auto readDefaults = [&jdef](const char *key, UDWORD (&dst)[MAX_PLAYERS], size_t statCount)
	{
		const nlohmann::ordered_json &arr = jdef.at(key);
		if (!arr.is_array() || arr.size() != MAX_PLAYERS)
		{
			throw StateError("research.defaults size mismatch");
		}
		for (unsigned p = 0; p < MAX_PLAYERS; ++p)
		{
			const UDWORD idx = arr[p].get<UDWORD>();
			// The index is used to index the stat table later. Validate only when stats of this type are
			// loaded (statCount == 0 is the degenerate no-data case, i.e. the headless self-test, where
			// the default is inert and never dereferenced).
			if (statCount > 0 && idx >= statCount)
			{
				throw StateError("research.defaults component index out of range");
			}
			dst[p] = idx;
		}
	};
	readDefaults("sensor", aDefaultSensor, asSensorStats.size());
	readDefaults("ecm", aDefaultECM, asECMStats.size());
	readDefaults("repair", aDefaultRepair, asRepairStats.size());
}

// MARK: - Section: component / structure-type availability

constexpr uint32_t AVAILABILITY_SECTION_VERSION = 1;

/// Number of loaded stats for a given component type (the length of apCompLists[player][comp])
static size_t compStatCount(unsigned comp)
{
	switch (comp)
	{
	case COMP_BODY:       return asBodyStats.size();
	case COMP_BRAIN:      return asBrainStats.size();
	case COMP_PROPULSION: return asPropulsionStats.size();
	case COMP_REPAIRUNIT: return asRepairStats.size();
	case COMP_ECM:        return asECMStats.size();
	case COMP_SENSOR:     return asSensorStats.size();
	case COMP_CONSTRUCT:  return asConstructStats.size();
	case COMP_WEAPON:     return asWeaponStats.size();
	default:              return 0;
	}
}

static nlohmann::ordered_json writeAvailability()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = AVAILABILITY_SECTION_VERSION;

	nlohmann::ordered_json jcomp = nlohmann::ordered_json::array();
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		nlohmann::ordered_json jplayer = nlohmann::ordered_json::array();
		for (unsigned c = 0; c < COMP_NUMCOMPONENTS; ++c)
		{
			nlohmann::ordered_json jlist = nlohmann::ordered_json::array();
			const size_t count = compStatCount(c);
			for (size_t i = 0; i < count; ++i)
			{
				jlist.push_back(apCompLists[p][c][i]);
			}
			jplayer.push_back(std::move(jlist));
		}
		jcomp.push_back(std::move(jplayer));
	}
	j["comp"] = std::move(jcomp);

	nlohmann::ordered_json jstruct = nlohmann::ordered_json::array();
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		nlohmann::ordered_json jlist = nlohmann::ordered_json::array();
		for (unsigned i = 0; i < numStructureStats; ++i)
		{
			jlist.push_back(apStructTypeLists[p][i]);
		}
		jstruct.push_back(std::move(jlist));
	}
	j["structType"] = std::move(jstruct);

	return j;
}

static void readAvailability(const nlohmann::ordered_json &j, uint32_t version)
{
	if (version != AVAILABILITY_SECTION_VERSION)
	{
		throw StateError("unsupported availability section version");
	}

	const nlohmann::ordered_json &jcomp = j.at("comp");
	if (!jcomp.is_array() || jcomp.size() != MAX_PLAYERS)
	{
		throw StateError("availability.comp must have MAX_PLAYERS entries");
	}
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		const nlohmann::ordered_json &jplayer = jcomp[p];
		if (!jplayer.is_array() || jplayer.size() != COMP_NUMCOMPONENTS)
		{
			throw StateError("availability.comp player entry size mismatch");
		}
		for (unsigned c = 0; c < COMP_NUMCOMPONENTS; ++c)
		{
			const nlohmann::ordered_json &jlist = jplayer[c];
			const size_t count = compStatCount(c);
			if (!jlist.is_array() || jlist.size() != count)
			{
				throw StateError("availability.comp list size mismatch (different stats/mods?)");
			}
			for (size_t i = 0; i < count; ++i)
			{
				apCompLists[p][c][i] = jlist[i].get<uint8_t>();
			}
		}
	}

	const nlohmann::ordered_json &jstruct = j.at("structType");
	if (!jstruct.is_array() || jstruct.size() != MAX_PLAYERS)
	{
		throw StateError("availability.structType must have MAX_PLAYERS entries");
	}
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		const nlohmann::ordered_json &jlist = jstruct[p];
		if (!jlist.is_array() || jlist.size() != numStructureStats)
		{
			throw StateError("availability.structType list size mismatch (different stats/mods?)");
		}
		for (unsigned i = 0; i < numStructureStats; ++i)
		{
			apStructTypeLists[p][i] = jlist[i].get<uint8_t>();
		}
	}
}

// MARK: - Section: limits
// Authoritative caps only. Per-structure curCount is derivable from the object lists and
// is rebuilt in a later phase, so it is not serialized here.

constexpr uint32_t LIMITS_SECTION_VERSION = 1;

static nlohmann::ordered_json writeLimits()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = LIMITS_SECTION_VERSION;

	nlohmann::ordered_json jdroid = nlohmann::ordered_json::array();
	nlohmann::ordered_json jcmd = nlohmann::ordered_json::array();
	nlohmann::ordered_json jconstr = nlohmann::ordered_json::array();
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		jdroid.push_back(getMaxDroids(p));
		jcmd.push_back(getMaxCommanders(p));
		jconstr.push_back(getMaxConstructors(p));
	}
	j["maxDroids"] = std::move(jdroid);
	j["maxCommanders"] = std::move(jcmd);
	j["maxConstructors"] = std::move(jconstr);

	// Per-structure-type, per-player build limit (the live, possibly-upgraded value).
	nlohmann::ordered_json jstructLimits = nlohmann::ordered_json::array();
	for (unsigned i = 0; i < numStructureStats; ++i)
	{
		nlohmann::ordered_json row = nlohmann::ordered_json::array();
		for (unsigned p = 0; p < MAX_PLAYERS; ++p)
		{
			row.push_back(asStructureStats[i].upgrade[p].limit);
		}
		jstructLimits.push_back(std::move(row));
	}
	j["structLimits"] = std::move(jstructLimits);

	return j;
}

static void readLimits(const nlohmann::ordered_json &j, uint32_t version)
{
	if (version != LIMITS_SECTION_VERSION)
	{
		throw StateError("unsupported limits section version");
	}

	const auto readPerPlayer = [&j](const char *key, void (*setter)(UDWORD, int))
	{
		const nlohmann::ordered_json &arr = j.at(key);
		if (!arr.is_array() || arr.size() != MAX_PLAYERS)
		{
			throw StateError("limits per-player array size mismatch");
		}
		for (unsigned p = 0; p < MAX_PLAYERS; ++p)
		{
			setter(p, arr[p].get<int>());
		}
	};
	readPerPlayer("maxDroids", setMaxDroids);
	readPerPlayer("maxCommanders", setMaxCommanders);
	readPerPlayer("maxConstructors", setMaxConstructors);

	const nlohmann::ordered_json &jstructLimits = j.at("structLimits");
	if (!jstructLimits.is_array() || jstructLimits.size() != numStructureStats)
	{
		throw StateError("limits.structLimits size mismatch (different stats/mods?)");
	}
	for (unsigned i = 0; i < numStructureStats; ++i)
	{
		const nlohmann::ordered_json &row = jstructLimits[i];
		if (!row.is_array() || row.size() != MAX_PLAYERS)
		{
			throw StateError("limits.structLimits row size mismatch");
		}
		for (unsigned p = 0; p < MAX_PLAYERS; ++p)
		{
			asStructureStats[i].upgrade[p].limit = row[p].get<uint32_t>();
		}
	}
}

// MARK: - Section: droid templates
// Reuses the engine's own per-template JSON (saveTemplateCommon/loadTemplateCommon),
// which lowers components to stat ids, plus the per-template bookkeeping fields.

constexpr uint32_t TEMPLATES_SECTION_VERSION = 1;

static nlohmann::ordered_json writeTemplates()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = TEMPLATES_SECTION_VERSION;

	nlohmann::ordered_json jplayers = nlohmann::ordered_json::array();
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		nlohmann::ordered_json jlist = nlohmann::ordered_json::array();
		// enumerateTemplates iterates droidTemplates[p] (a std::map keyed by multiPlayerID),
		// so iteration order is deterministic.
		enumerateTemplates(p, [&jlist](DROID_TEMPLATE *psTempl)
		{
			nlohmann::ordered_json jt = saveTemplateCommon(psTempl);
			jt["multiPlayerID"] = psTempl->multiPlayerID;
			jt["enabled"] = psTempl->enabled;
			jt["stored"] = psTempl->stored;
			jt["prefab"] = psTempl->prefab;
			jlist.push_back(std::move(jt));
			return true;
		});
		jplayers.push_back(std::move(jlist));
	}
	j["players"] = std::move(jplayers);
	return j;
}

static void readTemplates(const nlohmann::ordered_json &j, uint32_t version)
{
	if (version != TEMPLATES_SECTION_VERSION)
	{
		throw StateError("unsupported templates section version");
	}

	const nlohmann::ordered_json &jplayers = j.at("players");
	if (!jplayers.is_array() || jplayers.size() != MAX_PLAYERS)
	{
		throw StateError("templates.players must have MAX_PLAYERS entries");
	}
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		const nlohmann::ordered_json &jlist = jplayers[p];
		if (!jlist.is_array())
		{
			throw StateError("templates.players entry must be an array");
		}
		clearTemplates(p);
		for (const nlohmann::ordered_json &jt : jlist)
		{
			auto psTempl = std::make_unique<DROID_TEMPLATE>();
			if (!loadTemplateCommon(jt, *psTempl))
			{
				throw StateError("template contains an unknown component (different stats/mods?)");
			}
			psTempl->multiPlayerID = jt.at("multiPlayerID").get<uint32_t>();
			psTempl->enabled = jt.at("enabled").get<bool>();
			psTempl->stored = jt.at("stored").get<bool>();
			psTempl->prefab = jt.at("prefab").get<bool>();
			addTemplate(p, std::move(psTempl));
		}
	}
}

// MARK: - Section: production runs
// asProductionRun[factoryType][factoryInc] is a queue of {quantity, built, template}.
// Templates are referenced by multiPlayerID and resolved after templates are restored.

constexpr uint32_t PRODUCTION_SECTION_VERSION = 1;

static nlohmann::ordered_json writeProduction()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = PRODUCTION_SECTION_VERSION;

	nlohmann::ordered_json jtypes = nlohmann::ordered_json::array();
	for (unsigned ft = 0; ft < NUM_FACTORY_TYPES; ++ft)
	{
		nlohmann::ordered_json jfactories = nlohmann::ordered_json::array();
		for (const ProductionRun &run : asProductionRun[ft])
		{
			nlohmann::ordered_json jentries = nlohmann::ordered_json::array();
			for (const ProductionRunEntry &e : run)
			{
				nlohmann::ordered_json je = nlohmann::ordered_json::object();
				je["quantity"] = e.quantity;
				je["built"] = e.built;
				je["templateId"] = (e.psTemplate != nullptr) ? e.psTemplate->multiPlayerID : 0u;
				jentries.push_back(std::move(je));
			}
			jfactories.push_back(std::move(jentries));
		}
		jtypes.push_back(std::move(jfactories));
	}
	j["runs"] = std::move(jtypes);
	return j;
}

static void readProduction(const nlohmann::ordered_json &j, uint32_t version)
{
	if (version != PRODUCTION_SECTION_VERSION)
	{
		throw StateError("unsupported production section version");
	}

	const nlohmann::ordered_json &jtypes = j.at("runs");
	if (!jtypes.is_array() || jtypes.size() != NUM_FACTORY_TYPES)
	{
		throw StateError("production.runs must have NUM_FACTORY_TYPES entries");
	}
	for (unsigned ft = 0; ft < NUM_FACTORY_TYPES; ++ft)
	{
		const nlohmann::ordered_json &jfactories = jtypes[ft];
		if (!jfactories.is_array())
		{
			throw StateError("production.runs entry must be an array");
		}
		asProductionRun[ft].clear();
		asProductionRun[ft].resize(jfactories.size());
		for (size_t fi = 0; fi < jfactories.size(); ++fi)
		{
			const nlohmann::ordered_json &jentries = jfactories[fi];
			if (!jentries.is_array())
			{
				throw StateError("production factory entry must be an array");
			}
			ProductionRun &run = asProductionRun[ft][fi];
			for (const nlohmann::ordered_json &je : jentries)
			{
				ProductionRunEntry e;
				e.quantity = je.at("quantity").get<int>();
				e.built = je.at("built").get<int>();
				const uint32_t tid = je.at("templateId").get<uint32_t>();
				e.psTemplate = (tid != 0) ? getTemplateFromMultiPlayerID(tid) : nullptr;
				run.push_back(e);
			}
		}
	}
}

// MARK: - Section: world objects
//
// Serializes the live gameWorld's simulation objects.
//
// Employs the construction-with-preserved-id + reverse-rebuild pattern for all
// object types (addObjectToList prepends, so rebuilding in reverse reproduces
// the exact list order, which matters for deterministic iteration).

constexpr uint32_t WORLD_SECTION_VERSION = 1;

static nlohmann::ordered_json writeVector3i(const Vector3i &v)
{
	return nlohmann::ordered_json::array({ v.x, v.y, v.z });
}

static Vector3i readVector3i(const nlohmann::ordered_json &j)
{
	reqArray(j, 3);
	return Vector3i(j[0].get<int32_t>(), j[1].get<int32_t>(), j[2].get<int32_t>());
}

static nlohmann::ordered_json writeBaseObjectCommon(const BASE_OBJECT *psObj)
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["id"] = psObj->id;
	j["player"] = psObj->player;
	j["pos"] = writeVector3i(psObj->pos);
	j["rot"] = nlohmann::ordered_json::array({ psObj->rot.direction, psObj->rot.pitch, psObj->rot.roll });
	j["body"] = psObj->body;
	j["born"] = psObj->born;
	j["died"] = psObj->died;
	j["time"] = psObj->time;
	j["periodicalDamage"] = psObj->periodicalDamage;
	j["periodicalDamageStart"] = psObj->periodicalDamageStart;
	j["timeLastHit"] = psObj->timeLastHit;
	// The weapon subclass that last hit this object - partners timeLastHit. EMP-disable and several
	// combat/AI paths gate on the (lastHitWeapon == WSC_EMP && ...) pair, so restoring timeLastHit
	// without this would let an EMP-frozen unit resume acting.
	j["lastHitWeapon"] = static_cast<uint8_t>(psObj->lastHitWeapon);
	// OBJECT_FLAG bitset. OBJECT_FLAG_DIRTY is a deferred body/speed-upgrade trigger consumed on the
	// object's next update; off-world/limbo units never update, so a pending flag must survive restore.
	j["flags"] = static_cast<uint32_t>(psObj->flags.to_ulong());
	nlohmann::ordered_json vis = nlohmann::ordered_json::array();
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		vis.push_back(psObj->visible[p]);
	}
	j["visible"] = std::move(vis);
	return j;
}

/// Applies the common BASE_OBJECT fields from JSON onto an already-constructed object.
static void readBaseObjectCommon(const nlohmann::ordered_json &j, BASE_OBJECT *psObj)
{
	// Restore the saved position. x/y already match the construction placement, but z (the foundation
	// height for structures/features, or the current height for droids) is otherwise recomputed by the
	// build path and does not reproduce the saved value exactly (ex: structure foundation flattening),
	// so restore it explicitly for an exact round-trip / deterministic resume.
	psObj->pos = readVector3i(j.at("pos"));
	const nlohmann::ordered_json &jrot = reqArray(j.at("rot"), 3);
	psObj->rot.direction = jrot[0].get<uint16_t>();
	psObj->rot.pitch = jrot[1].get<uint16_t>();
	psObj->rot.roll = jrot[2].get<uint16_t>();
	// Objects may sit in [0, MAX_PLAYER_SLOTS): players are < MAX_PLAYERS, features use PLAYER_FEATURE
	// (MAX_PLAYERS + 1), anything beyond that would be out of range.
	const unsigned objPlayer = j.at("player").get<uint8_t>();
	if (objPlayer >= MAX_PLAYER_SLOTS)
	{
		throw StateError("object player index out of range");
	}
	psObj->player = static_cast<uint8_t>(objPlayer);
	psObj->body = j.at("body").get<uint32_t>();
	psObj->born = j.at("born").get<uint32_t>();
	psObj->died = j.at("died").get<uint32_t>();
	psObj->time = j.at("time").get<uint32_t>();
	psObj->periodicalDamage = j.at("periodicalDamage").get<uint32_t>();
	psObj->periodicalDamageStart = j.at("periodicalDamageStart").get<uint32_t>();
	psObj->timeLastHit = j.at("timeLastHit").get<uint32_t>();
	const unsigned lastHitWeapon = j.at("lastHitWeapon").get<uint8_t>();
	if (lastHitWeapon > WSC_NUM_WEAPON_SUBCLASSES)  // sentinel WSC_NUM_WEAPON_SUBCLASSES = "no weapon"
	{
		throw StateError("object lastHitWeapon out of range");
	}
	psObj->lastHitWeapon = static_cast<WEAPON_SUBCLASS>(lastHitWeapon);
	psObj->flags = std::bitset<OBJECT_FLAG_COUNT>(j.at("flags").get<uint32_t>());
	const nlohmann::ordered_json &jvis = j.at("visible");
	if (!jvis.is_array() || jvis.size() != MAX_PLAYERS)
	{
		throw StateError("object.visible size mismatch");
	}
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		psObj->visible[p] = jvis[p].get<uint8_t>();
	}
}

// Object reference token: an intra-sim pointer lowered to (type, id, player).
// Resolved back to a live pointer via getBaseObjFromData() once all objects exist.
static nlohmann::ordered_json writeObjRef(const BASE_OBJECT *psObj)
{
	if (psObj == nullptr)
	{
		return nlohmann::ordered_json(nullptr);
	}
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["id"] = psObj->id;
	j["player"] = psObj->player;
	j["type"] = static_cast<int>(psObj->type);
	return j;
}

static BASE_OBJECT *readObjRef(const nlohmann::ordered_json &j)
{
	if (j.is_null())
	{
		return nullptr;
	}
	const uint32_t id = j.at("id").get<uint32_t>();
	const unsigned player = j.at("player").get<unsigned>();
	const OBJECT_TYPE type = static_cast<OBJECT_TYPE>(j.at("type").get<int>());
	return getBaseObjFromData(id, player, type);
}

// Resolve an object ref and confirm the resolved object is of the expected class before casting.
// readObjRef trusts the ref's own "type" field, so callers that need a specific class must verify it.
// Returns nullptr on a null ref, an unresolved id, or a class mismatch.
template <typename T>
static T *readObjRefTyped(const nlohmann::ordered_json &j, OBJECT_TYPE want)
{
	BASE_OBJECT *obj = readObjRef(j);
	if (obj == nullptr || obj->type != want)
	{
		return nullptr;
	}
	return static_cast<T *>(obj);
}

/// Look up a research topic index by its stat id string.
static int findResearchIndexById(const WzString &id)
{
	for (size_t i = 0; i < asResearch.size(); ++i)
	{
		if (asResearch[i].id == id)
		{
			return static_cast<int>(i);
		}
	}
	return -1;
}

/// Remove every object from a world and reset map tile object pointers (the freeAll*
/// functions do not clear MAPTILE::psObject, which would otherwise dangle after a restore).
static void clearWorldObjects(GameWorld &world)
{
	// Decrement structure build counts for the structures about to be bulk-freed. freeAllStructs does
	// not go through the normal removal path that maintains asStructureStats[].curCount, so otherwise
	// the counts stay inflated and buildStructureDir would refuse to rebuild during restore ("could
	// not be built due to building limits"). Mirrors the per-structure decrement in removeStruct.
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		for (const STRUCTURE *psStruct : world.objects.structures[p])
		{
			const unsigned inc = static_cast<unsigned>(psStruct->pStructureType - asStructureStats);
			if (inc < numStructureStats && asStructureStats[inc].curCount[p] > 0)
			{
				asStructureStats[inc].curCount[p]--;
			}
		}
	}

	// freeAllDroids/freeAllStructs remove each object's tile visibility against THIS world's map before
	// erasing it (flushing the pending-removal queue first), so we do not need to do that here.
	// (Sensor/watcher tile counts are rebuilt from objects on reconstruction and are not serialized, so
	// this does not affect the round-trip.)
	freeAllDroids(world);
	freeAllStructs(world);
	freeAllFeatures(world);
	freeAllFlagPositions(world.objects);
	// Clear the non-owning functional index lists too. They point into the structures/features just
	// freed above, but freeAllStructs/freeAllFeatures bulk-free the lists without going through
	// removeStructureFromList/removeFeatureFromList, so these indices would otherwise be left holding
	// dangling pointers (i.e. checkForResExtractors would then walk freed extractors and crash).
	// Mirrors the legacy load reset (game.cpp): extractors per-player, sensors[0], oils[0].
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		world.objects.extractors[p].clear();
	}
	world.objects.sensors[0].clear();
	world.objects.oils[0].clear();
	if (world.map.tiles)
	{
		const size_t n = static_cast<size_t>(world.map.width) * static_cast<size_t>(world.map.height);
		for (size_t i = 0; i < n; ++i)
		{
			MAPTILE &t = world.map.tiles[i];
			t.psObject = nullptr;
			// Defense-in-depth: force the per-tile, object-derived visibility counts to a clean 0 baseline
			// before objects are rebuilt (visTilesUpdate adds onto these). freeAllDroids/freeAllStructs
			// above already decrement them per object, but that relies on the existing objects' watched
			// tiles being consistent with the current map - if the map was reallocated underneath them
			// (dimension change with stale objects) the decrement could underflow. Zeroing here makes the
			// rebuilt coverage exact regardless. tileExploredBits (fog) is NOT object-derived - it is
			// authoritatively reapplied by readMapDynamic - so it is left untouched.
			t.sensorBits = 0;
			t.jammerBits = 0;
			for (unsigned p = 0; p < MAX_PLAYERS; ++p)
			{
				t.sensors[p] = 0;
				t.watchers[p] = 0;
				t.jammers[p] = 0;
			}
		}
	}
}

/// Rebuild transient/derived state that is recomputed rather than serialized.
static void rebuildDerivedState()
{
	if (!gameWorld.map.tiles)
	{
		return; // no map loaded (i.e. the early-CLI self-test) - nothing to rebuild
	}
	gridReset(gameWorld);
	countUpdate(true);
}

static nlohmann::ordered_json writeFeature(const FEATURE *psFeature)
{
	nlohmann::ordered_json j = writeBaseObjectCommon(psFeature);
	j["statId"] = psFeature->psStats->id.toUtf8();
	return j;
}

static void readFeature(GameWorld &world, const nlohmann::ordered_json &j)
{
	const WzString statId = WzString::fromUtf8(j.at("statId").get<std::string>());
	const SDWORD statIndex = getFeatureStatFromName(statId);
	if (statIndex < 0)
	{
		throw StateError("unknown feature stat id: " + statId.toStdString());
	}
	FEATURE_STATS *psStats = &asFeatureStats[statIndex];

	const uint32_t id = j.at("id").get<uint32_t>();
	const nlohmann::ordered_json &jpos = reqArray(j.at("pos"), 2);
	const UDWORD x = static_cast<UDWORD>(jpos[0].get<int32_t>());
	const UDWORD y = static_cast<UDWORD>(jpos[1].get<int32_t>());

	FEATURE *psFeature = buildFeature(world, psStats, x, y, true, id);
	if (psFeature == nullptr)
	{
		throw StateError("failed to reconstruct feature id " + std::to_string(id));
	}
	readBaseObjectCommon(j, psFeature);
	// pos.z (foundation depth) is derived from the map by buildFeature - x/y are authoritative.
}

// MARK: - Structures

static bool isFactoryType(STRUCTURE_TYPE t)
{
	return t == REF_FACTORY || t == REF_VTOL_FACTORY || t == REF_CYBORG_FACTORY;
}

static nlohmann::ordered_json writeAssemblyPoint(const FLAG_POSITION *psFlag)
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["pos"] = writeVector3i(psFlag->coords);
	j["selected"] = psFlag->selected;
	j["number"] = psFlag->factoryInc;
	return j;
}

static nlohmann::ordered_json writeStructure(const STRUCTURE *psStruct)
{
	nlohmann::ordered_json j = writeBaseObjectCommon(psStruct);
	j["statId"] = psStruct->pStructureType->id.toUtf8();
	j["status"] = static_cast<int>(psStruct->status);
	j["resistance"] = psStruct->resistance;
	j["lastResistance"] = psStruct->lastResistance; // regen-timer anchor - without it the post-restore regen schedule diverges
	j["capacity"] = psStruct->capacity;
	j["currentBuildPts"] = psStruct->currentBuildPts;
	j["productToGroup"] = psStruct->productToGroup;
	// buildRate is reset to 0 each structureUpdate and re-accumulated by building trucks
	// at a tick boundary a not-currently-built structure holds 0. buildStructure forces it to 1,
	// which for a restored SS_BEING_BUILT structure would wrongly delay the give-up /
	// slow-deconstruct branch (structure.cpp) by a tick, so restore the real value verbatim.
	j["buildRate"] = psStruct->buildRate;
	// Gate animation state drives tile blocking (REF_GATE): SAS_OPEN clears AUXBITS_BLOCKING, every other
	// state blocks. Currently always SAS_NORMAL for non-gates, but serialized uniformly.
	j["gateState"] = static_cast<int>(psStruct->state);
	j["lastStateTime"] = psStruct->lastStateTime;

	nlohmann::ordered_json weapons = nlohmann::ordered_json::array();
	for (unsigned w = 0; w < psStruct->numWeaps; ++w)
	{
		nlohmann::ordered_json jw = nlohmann::ordered_json::object();
		jw["ammo"] = psStruct->asWeaps[w].ammo;
		jw["lastFired"] = psStruct->asWeaps[w].lastFired;
		jw["shotsFired"] = psStruct->asWeaps[w].shotsFired;
		jw["rot"] = nlohmann::ordered_json::array({ psStruct->asWeaps[w].rot.direction, psStruct->asWeaps[w].rot.pitch, psStruct->asWeaps[w].rot.roll });
		jw["target"] = writeObjRef(psStruct->psTarget[w]);
		weapons.push_back(std::move(jw));
	}
	j["weapons"] = std::move(weapons);

	const STRUCTURE_TYPE type = psStruct->pStructureType->type;
	if (isFactoryType(type))
	{
		const FACTORY *f = &psStruct->pFunctionality->factory;
		nlohmann::ordered_json jf = nlohmann::ordered_json::object();
		jf["productionLoops"] = f->productionLoops;
		jf["timeStarted"] = f->timeStarted;
		jf["buildPointsRemaining"] = f->buildPointsRemaining;
		jf["timeStartHold"] = f->timeStartHold;
		jf["loopsPerformed"] = f->loopsPerformed;
		jf["secondaryOrder"] = f->secondaryOrder;
		if (f->psSubject != nullptr)
		{
			jf["templateId"] = f->psSubject->multiPlayerID;
		}
		if (f->psAssemblyPoint != nullptr)
		{
			jf["assembly"] = writeAssemblyPoint(f->psAssemblyPoint);
		}
		jf["commander"] = writeObjRef(f->psCommander); // resolved in pass 2
		j["factory"] = std::move(jf);
	}
	else if (type == REF_RESEARCH)
	{
		const RESEARCH_FACILITY *r = &psStruct->pFunctionality->researchFacility;
		nlohmann::ordered_json jr = nlohmann::ordered_json::object();
		if (r->psSubject != nullptr)
		{
			jr["target"] = r->psSubject->id.toUtf8();
			jr["timeStartHold"] = r->timeStartHold;
		}
		// The best (highest-point) topic researched so far. Accumulated over the game and consumed by
		// researchReward() when this player is defeated, so it must persist to reproduce the reward.
		if (r->psBestTopic != nullptr)
		{
			jr["bestTopic"] = r->psBestTopic->id.toUtf8();
		}
		j["research"] = std::move(jr);
	}
	else if (type == REF_REPAIR_FACILITY)
	{
		const REPAIR_FACILITY *rp = &psStruct->pFunctionality->repairFacility;
		nlohmann::ordered_json jrp = nlohmann::ordered_json::object();
		jrp["target"] = writeObjRef(rp->psObj); // resolved in pass 2
		// The Idle/Repairing state machine must be restored alongside psObj: only the Repairing state
		// runs the clear transitions (droid healed / moved away / died -> psObj = nullptr). Without it a
		// restored mid-repair pad loads as Idle and never clears its (stale) psObj (see aiUpdateRepair).
		jrp["state"] = static_cast<int>(rp->state);
		if (rp->psDeliveryPoint != nullptr)
		{
			jrp["delivery"] = writeAssemblyPoint(rp->psDeliveryPoint);
		}
		j["repair"] = std::move(jrp);
	}
	else if (type == REF_REARM_PAD)
	{
		const REARM_PAD *ra = &psStruct->pFunctionality->rearmPad;
		nlohmann::ordered_json jra = nlohmann::ordered_json::object();
		jra["timeStarted"] = ra->timeStarted;
		jra["timeLastUpdated"] = ra->timeLastUpdated;
		jra["target"] = writeObjRef(ra->psObj); // resolved in pass 2
		j["rearm"] = std::move(jra);
	}
	else if (type == REF_WALL || type == REF_GATE)
	{
		nlohmann::ordered_json jw = nlohmann::ordered_json::object();
		jw["type"] = psStruct->pFunctionality->wall.type;
		j["wall"] = std::move(jw);
	}
	else if (type == REF_POWER_GEN)
	{
		// Authoritative derrick<->power-gen slotting. On restore, buildingComplete()'s greedy
		// checkForResExtractors rebuild is order-sensitive (depends on structure-list/build order)
		// and produces a valid-but-different distribution that diverges from the original and breaks
		// lockstep CRC. We serialize the exact slot->extractor map and re-apply it (restorePowerLinkage).
		const POWER_GEN *pg = &psStruct->pFunctionality->powerGenerator;
		nlohmann::ordered_json slots = nlohmann::ordered_json::array();
		for (int i = 0; i < NUM_POWER_MODULES; ++i)
		{
			slots.push_back(writeObjRef(pg->apResExtractors[i]));
		}
		j["resExtractors"] = std::move(slots);
	}
	return j;
}

/// Pass 1: construct the structure and restore its scalar + functionality state.
/// Object cross-references (targets, commander, repair/rearm target) are resolved in pass 2.
static void readStructurePass1(GameWorld &world, const nlohmann::ordered_json &j)
{
	const WzString name = WzString::fromUtf8(j.at("statId").get<std::string>());
	STRUCTURE_STATS *psStats = std::find_if(asStructureStats, asStructureStats + numStructureStats,
		[&name](STRUCTURE_STATS &s) { return s.id == name; });
	if (psStats == asStructureStats + numStructureStats)
	{
		throw StateError("unknown structure stat id: " + name.toStdString());
	}

	const uint32_t id = j.at("id").get<uint32_t>();
	const nlohmann::ordered_json &jpos = reqArray(j.at("pos"), 2);
	const UDWORD x = static_cast<UDWORD>(jpos[0].get<int32_t>());
	const UDWORD y = static_cast<UDWORD>(jpos[1].get<int32_t>());
	const uint16_t direction = j.at("rot")[0].get<uint16_t>();
	const unsigned player = j.at("player").get<unsigned>();

	STRUCTURE *psStruct = buildStructureDir(world, psStats, x, y, direction, player, true, id);
	if (psStruct == nullptr)
	{
		throw StateError("failed to reconstruct structure id " + std::to_string(id));
	}
	readBaseObjectCommon(j, psStruct);
	psStruct->resistance = j.at("resistance").get<int>();
	psStruct->lastResistance = j.at("lastResistance").get<uint32_t>();
	psStruct->productToGroup = j.at("productToGroup").get<uint8_t>();
	const int capacity = j.at("capacity").get<int>();
	psStruct->capacity = 0; // incremented as modules are (re)built

	const STRUCTURE_TYPE type = psStruct->pStructureType->type;
	const auto buildModules = [&]()
	{
		if (capacity > 0)
		{
			STRUCTURE_STATS *psModule = getModuleStat(psStruct);
			for (int m = 0; m < capacity; ++m)
			{
				buildStructure(world, psModule, psStruct->pos.x, psStruct->pos.y, psStruct->player, true);
			}
		}
	};

	if (isFactoryType(type))
	{
		FACTORY *f = &psStruct->pFunctionality->factory;
		const nlohmann::ordered_json &jf = j.at("factory");
		f->productionLoops = jf.at("productionLoops").get<uint8_t>();
		f->timeStarted = jf.at("timeStarted").get<uint32_t>();
		f->buildPointsRemaining = jf.at("buildPointsRemaining").get<int>();
		f->timeStartHold = jf.at("timeStartHold").get<uint32_t>();
		f->loopsPerformed = jf.at("loopsPerformed").get<uint8_t>();
		f->secondaryOrder = jf.at("secondaryOrder").get<uint32_t>();
		buildModules();
		if (jf.contains("templateId"))
		{
			f->psSubject = getTemplateFromMultiPlayerID(jf.at("templateId").get<uint32_t>());
		}
		if (jf.contains("assembly"))
		{
			const nlohmann::ordered_json &ja = jf.at("assembly");
			const nlohmann::ordered_json &jap = ja.at("pos");
			setAssemblyPoint(world, f->psAssemblyPoint, jap[0].get<int32_t>(), jap[1].get<int32_t>(), player, false);
			// setAssemblyPoint recomputes coords.z from map_Height(). During reconstruct the terrain is
			// still mid-restore (object builds re-flatten foundations, see restampTerrainHeights), so it
			// can sample a +-1-perturbed height. The saved z is authoritative, so restore it explicitly.
			f->psAssemblyPoint->coords.z = jap[2].get<int32_t>();
			f->psAssemblyPoint->selected = ja.at("selected").get<bool>();
			f->psAssemblyPoint->factoryInc = ja.at("number").get<int>();
		}
		// Production runs are restored by the dedicated "production" section, not here.
	}
	else if (type == REF_RESEARCH)
	{
		RESEARCH_FACILITY *r = &psStruct->pFunctionality->researchFacility;
		buildModules();
		r->psSubject = nullptr;
		r->psBestTopic = nullptr;
		r->timeStartHold = 0;
		const nlohmann::ordered_json &jr = j.at("research");
		if (jr.contains("target"))
		{
			const WzString resId = WzString::fromUtf8(jr.at("target").get<std::string>());
			const int researchIdx = findResearchIndexById(resId);
			if (researchIdx >= 0)
			{
				r->psSubject = &asResearch[researchIdx];
				r->timeStartHold = jr.at("timeStartHold").get<uint32_t>();
			}
			else
			{
				throw StateError("unknown research target: " + resId.toStdString());
			}
		}
		if (jr.contains("bestTopic"))
		{
			const WzString bestId = WzString::fromUtf8(jr.at("bestTopic").get<std::string>());
			const int bestIdx = findResearchIndexById(bestId);
			if (bestIdx >= 0)
			{
				r->psBestTopic = &asResearch[bestIdx];
			}
			else
			{
				throw StateError("unknown research bestTopic: " + bestId.toStdString());
			}
		}
	}
	else if (type == REF_POWER_GEN)
	{
		buildModules();
	}
	else if (type == REF_REPAIR_FACILITY)
	{
		REPAIR_FACILITY *rp = &psStruct->pFunctionality->repairFacility;
		const nlohmann::ordered_json &jrp = j.at("repair");
		// Restore the repair state machine (psObj itself is resolved in pass 2).
		rp->state = static_cast<RepairState>(jrp.value("state", static_cast<int>(RepairState::Idle)));
		if (jrp.contains("delivery"))
		{
			const nlohmann::ordered_json &jd = jrp.at("delivery");
			const nlohmann::ordered_json &jdp = jd.at("pos");
			setAssemblyPoint(world, rp->psDeliveryPoint, jdp[0].get<int32_t>(), jdp[1].get<int32_t>(), player, false);
			rp->psDeliveryPoint->coords.z = jdp[2].get<int32_t>(); // authoritative z (see factory assembly above)
			rp->psDeliveryPoint->selected = jd.at("selected").get<bool>();
		}
	}
	else if (type == REF_REARM_PAD)
	{
		REARM_PAD *ra = &psStruct->pFunctionality->rearmPad;
		const nlohmann::ordered_json &jra = j.at("rearm");
		ra->timeStarted = jra.at("timeStarted").get<uint32_t>();
		ra->timeLastUpdated = jra.at("timeLastUpdated").get<uint32_t>();
	}
	else if (type == REF_WALL || type == REF_GATE)
	{
		psStruct->pFunctionality->wall.type = j.at("wall").at("type").get<unsigned>();
		psStruct->sDisplay.imd = psStruct->pStructureType->pIMD[std::min<unsigned>(psStruct->pFunctionality->wall.type, psStruct->pStructureType->pIMD.size() - 1)];
	}

	// Re-apply the saved (absolute) body + time after modules are built: building a module calls
	// buildStructure, which resets the parent's body and its time (time = gameTime - deltaGameTime - 1)
	// at construction, clobbering the values readBaseObjectCommon restored (matches loadSaveStructure2
	// ordering). buildingComplete() below does not touch time, so restoring it here is sufficient.
	psStruct->body = j.at("body").get<uint32_t>();
	psStruct->currentBuildPts = j.at("currentBuildPts").get<uint32_t>();
	psStruct->time = j.at("time").get<uint32_t>();
	// Restore the real per-tick build rate, overriding the protective buildStructure default of 1.
	psStruct->buildRate = j.value("buildRate", psStruct->buildRate);

	// Weapons:
	const nlohmann::ordered_json &weapons = j.at("weapons");
	for (unsigned w = 0; w < psStruct->numWeaps && w < weapons.size(); ++w)
	{
		if (psStruct->asWeaps[w].nStat > 0)
		{
			psStruct->asWeaps[w].ammo = weapons[w].at("ammo").get<uint32_t>();
			psStruct->asWeaps[w].lastFired = weapons[w].at("lastFired").get<uint32_t>();
			psStruct->asWeaps[w].shotsFired = weapons[w].at("shotsFired").get<uint32_t>();
			const nlohmann::ordered_json &jr = weapons[w].at("rot");
			psStruct->asWeaps[w].rot.direction = jr[0].get<uint16_t>();
			psStruct->asWeaps[w].rot.pitch = jr[1].get<uint16_t>();
			psStruct->asWeaps[w].rot.roll = jr[2].get<uint16_t>();
		}
	}

	psStruct->status = static_cast<STRUCT_STATES>(j.at("status").get<int>());
	if (psStruct->status == SS_BUILT)
	{
		// buildingComplete() rebuilds the derrick<->power-gen linkage via the order-dependent
		// checkForResExtractors/checkForPowerGen (so explicit calls here would be redundant).
		// restorePowerLinkage() (after pass 2) overrides that with the exact saved slotting,
		// which is what lockstep determinism requires.
		buildingComplete(psStruct, world);
		// buildingComplete() -> releaseProduction/releaseResearch clears a restored production/research
		// HOLD (zeroes timeStartHold, advances timeStarted). Re-apply the saved timing so a facility
		// saved on hold stays held after restore (aiUpdateStructure gates on timeStartHold).
		if (type == REF_FACTORY || type == REF_CYBORG_FACTORY || type == REF_VTOL_FACTORY)
		{
			const nlohmann::ordered_json &jf = j.at("factory");
			FACTORY *f = &psStruct->pFunctionality->factory;
			f->timeStarted = jf.at("timeStarted").get<uint32_t>();
			f->timeStartHold = jf.at("timeStartHold").get<uint32_t>();
		}
		else if (type == REF_RESEARCH)
		{
			const nlohmann::ordered_json &jr = j.at("research");
			RESEARCH_FACILITY *r = &psStruct->pFunctionality->researchFacility;
			if (r->psSubject != nullptr && jr.contains("timeStartHold"))
			{
				r->timeStartHold = jr.at("timeStartHold").get<uint32_t>();
			}
		}
	}
	// Restore gate animation state and reapply matching tile blocking. buildingComplete() forced every
	// gate closed+blocking; a gate saved SAS_OPEN must clear the block (no-op for non-gate structures).
	psStruct->state = static_cast<STRUCT_ANIM_STATES>(j.at("gateState").get<int>());
	psStruct->lastStateTime = j.at("lastStateTime").get<uint32_t>();
	structureApplyGateStateBlocking(psStruct, world.map);
	// body is the absolute value already restored by readBaseObjectCommon
}

/// Pass 2: resolve object cross-references now that every object exists.
static void readStructurePass2(const nlohmann::ordered_json &j)
{
	const uint32_t id = j.at("id").get<uint32_t>();
	const unsigned player = j.at("player").get<unsigned>();
	STRUCTURE *psStruct = static_cast<STRUCTURE *>(getBaseObjFromData(id, player, OBJ_STRUCTURE));
	if (psStruct == nullptr)
	{
		throw StateError("structure pass 2: object not found, id " + std::to_string(id));
	}

	// Re-stamp the authoritative saved position. readBaseObjectCommon already restored pos in pass 1,
	// but pulled-to-terrain structures (walls/gates/defenses) have pos.z recomputed by alignStructure
	// from the terrain beneath them, and a non-wall structure built later re-aligns its wall/gate
	// neighbours (alignStructure's neighbour recursion) against the by-then foundation-flattened
	// terrain - clobbering that restored pos.z. Pass 2 runs after all structure building/alignment, so
	// re-applying the saved pos here is stable. (No-op for structures whose pos.z was not re-aligned.)
	psStruct->pos = readVector3i(j.at("pos"));

	const nlohmann::ordered_json &weapons = j.at("weapons");
	for (unsigned w = 0; w < psStruct->numWeaps && w < weapons.size(); ++w)
	{
		BASE_OBJECT *target = readObjRef(weapons[w].at("target"));
		if (target != nullptr)
		{
			setStructureTarget(psStruct, target, w, ORIGIN_UNKNOWN);
		}
	}

	const STRUCTURE_TYPE type = psStruct->pStructureType->type;
	if (isFactoryType(type))
	{
		const nlohmann::ordered_json &jf = j.at("factory");
		if (jf.contains("commander"))
		{
			DROID *psCommander = readObjRefTyped<DROID>(jf.at("commander"), OBJ_DROID);
			if (psCommander != nullptr)
			{
				assignFactoryCommandDroid(psStruct, psCommander);
			}
		}
	}
	else if (type == REF_REPAIR_FACILITY)
	{
		REPAIR_FACILITY *rp = &psStruct->pFunctionality->repairFacility;
		rp->psObj = readObjRefTyped<DROID>(j.at("repair").at("target"), OBJ_DROID);
	}
	else if (type == REF_REARM_PAD)
	{
		REARM_PAD *ra = &psStruct->pFunctionality->rearmPad;
		ra->psObj = readObjRefTyped<DROID>(j.at("rearm").at("target"), OBJ_DROID);
	}
}

/// Override the order-dependent derrick<->power-gen linkage with the exact saved slotting.
/// buildingComplete() (pass 1) rebuilds the linkage greedily via checkForResExtractors/
/// checkForPowerGen, whose result depends on structure-list/build order and so diverges from
/// the original after restore (same total power, different per-gen grouping) - breaking lockstep CRC
/// (i.e. updateCurrentPower's syncDebug). Runs after pass 2, when every object exists, so it can
/// resolve extractor references. Every snapshot emits "resExtractors" for every power gen - a
/// record missing it (corrupt snapshot) simply stays unlinked until the next build event.
static void restorePowerLinkage(GameWorld &world, const nlohmann::ordered_json &jstructures)
{
	// 1) Clear all power linkage established by the greedy rebuild, before re-establishing any,
	//    so a derrick moved between gens is never clobbered by a later gen's clear. Operate on the
	//    world being restored - this runs once per world (main gameWorld AND mission.gameWorld), so
	//    clearing the global gameWorld here would wipe the main world's links during the mission pass.
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		for (STRUCTURE *psStruct : world.objects.structures[p])
		{
			const STRUCTURE_TYPE type = psStruct->pStructureType->type;
			if (type == REF_POWER_GEN)
			{
				POWER_GEN *pg = &psStruct->pFunctionality->powerGenerator;
				for (int i = 0; i < NUM_POWER_MODULES; ++i)
				{
					pg->apResExtractors[i] = nullptr;
				}
			}
			else if (type == REF_RESOURCE_EXTRACTOR)
			{
				psStruct->pFunctionality->resourceExtractor.psPowerGen = nullptr;
			}
		}
	}

	// 2) Re-apply the exact saved slot->extractor map (and the extractor's back-pointer).
	for (const nlohmann::ordered_json &js : jstructures)
	{
		if (!js.contains("resExtractors"))
		{
			continue;
		}
		const uint32_t id = js.at("id").get<uint32_t>();
		const unsigned player = js.at("player").get<unsigned>();
		STRUCTURE *gen = static_cast<STRUCTURE *>(getBaseObjFromData(id, player, OBJ_STRUCTURE));
		if (gen == nullptr || gen->pStructureType->type != REF_POWER_GEN)
		{
			continue;
		}
		POWER_GEN *pg = &gen->pFunctionality->powerGenerator;
		const nlohmann::ordered_json &slots = js.at("resExtractors");
		for (int i = 0; i < NUM_POWER_MODULES && i < static_cast<int>(slots.size()); ++i)
		{
			// Confirm the resolved object is specifically a resource extractor before treating it as one
			STRUCTURE *psExt = readObjRefTyped<STRUCTURE>(slots[i], OBJ_STRUCTURE);
			if (psExt == nullptr || psExt->pStructureType->type != REF_RESOURCE_EXTRACTOR || psExt->pFunctionality == nullptr)
			{
				continue;
			}
			pg->apResExtractors[i] = psExt;
			psExt->pFunctionality->resourceExtractor.psPowerGen = gen;
		}
	}
}

// MARK: - Droids

static nlohmann::ordered_json writeVector2i(const Vector2i &v)
{
	return nlohmann::ordered_json::array({ v.x, v.y });
}

static Vector2i readVector2i(const nlohmann::ordered_json &j)
{
	reqArray(j, 2);
	return Vector2i(j[0].get<int32_t>(), j[1].get<int32_t>());
}

static nlohmann::ordered_json writeDroidOrder(const DroidOrder &o)
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["type"] = static_cast<int>(o.type);
	j["pos"] = writeVector2i(o.pos);
	j["pos2"] = writeVector2i(o.pos2);
	j["direction"] = o.direction;
	j["index"] = o.index;
	j["rtrType"] = static_cast<int>(o.rtrType);
	j["obj"] = writeObjRef(o.psObj);
	if (o.psStats != nullptr)
	{
		j["statId"] = o.psStats->id.toUtf8();
	}
	return j;
}

static void readDroidOrder(const nlohmann::ordered_json &j, DroidOrder &o)
{
	o.type = static_cast<DroidOrderType>(reqRange(j.at("type").get<int>(), DORDER_NONE, DORDER_MAX));
	o.pos = readVector2i(j.at("pos"));
	o.pos2 = readVector2i(j.at("pos2"));
	o.direction = j.at("direction").get<uint16_t>();
	o.index = j.at("index").get<uint32_t>();
	o.rtrType = static_cast<RTR_DATA_TYPE>(reqRange(j.at("rtrType").get<int>(), RTR_TYPE_NO_RESULT, RTR_DATA_TYPE_MAX));
	o.psObj = readObjRef(j.at("obj"));
	o.psStats = nullptr;
	if (j.contains("statId"))
	{
		const int sid = getStructStatFromName(WzString::fromUtf8(j.at("statId").get<std::string>()));
		if (sid >= 0)
		{
			o.psStats = &asStructureStats[sid];
		}
	}
}

// MARK: - Section: formations (movement formations, restored VERBATIM)
//
// A FORMATION's member slots (asMembers[].line/next/dist + the asLines chains + the free list)
// are allocated LAZILY by formationGetPos as each member moves, so they are trajectory-dependent
// hidden state: re-deriving them on restore (formationFind/formationJoin + lazy re-allocation)
// reproduces a valid-but-different slotting than the original instance built up over the
// match -> different formationCalcPos movement targets -> CRC divergence. Same class as the
// derrick<->power-gen linkage: serialize the RESULT verbatim. iSpeed is likewise
// trajectory-dependent (formationLeave recomputes it over slot-holders only).
//
// Restore protocol: formations are rebuilt verbatim BEFORE the world sections (slot droid ids
// stashed here) - the old formations die naturally as clearWorldObjects frees the droids that
// reference them (droid teardown calls formationLeave). Each rebuilt droid then re-ATTACHES to
// its restored formation - pointer + slot resolution only, NO formationJoin (refCount/iSpeed/
// size/rankDist are already verbatim). Every snapshot carries this section (empty when there
// are no formations). A droid referencing a formation the section does not carry is an
// invariant violation handled fail-soft at attach.

constexpr uint32_t FORMATIONS_SECTION_VERSION = 1;

struct RestoredFormationInfo
{
	std::array<uint32_t, F_MAXMEMBERS> slotDroidIds; // serialized asMembers[].psDroid ids (0 = empty)
	int expectedMembers = 0;                         // serialized refCount
	int attachedMembers = 0;                         // droids that re-attached during this restore
};
static std::unordered_map<FORMATION *, RestoredFormationInfo> g_restoredFormations;

static nlohmann::ordered_json writeFormations()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = FORMATIONS_SECTION_VERSION;
	nlohmann::ordered_json arr = nlohmann::ordered_json::array();
	for (const FORMATION *f : formationEnumerateAll())
	{
		nlohmann::ordered_json o = nlohmann::ordered_json::object();
		o["player"] = f->player;
		o["x"] = f->x;
		o["y"] = f->y;
		o["direction"] = f->direction;
		o["refCount"] = f->refCount;
		o["size"] = f->size;
		o["rankDist"] = f->rankDist;
		o["numLines"] = f->numLines;
		o["maxRank"] = f->maxRank;
		o["free"] = f->free;
		o["iSpeed"] = f->iSpeed;
		nlohmann::ordered_json lines = nlohmann::ordered_json::array();
		for (unsigned i = 0; i < F_MAXLINES; ++i)
		{
			lines.push_back(nlohmann::ordered_json::array({ f->asLines[i].xoffset, f->asLines[i].yoffset,
			                                        f->asLines[i].direction, f->asLines[i].member }));
		}
		o["lines"] = std::move(lines);
		nlohmann::ordered_json members = nlohmann::ordered_json::array();
		for (unsigned i = 0; i < F_MAXMEMBERS; ++i)
		{
			const F_MEMBER &m = f->asMembers[i];
			members.push_back(nlohmann::ordered_json::array({ m.line, m.next, m.dist,
			                                          m.psDroid != nullptr ? m.psDroid->id : 0u }));
		}
		o["members"] = std::move(members);
		arr.push_back(std::move(o));
	}
	j["formations"] = std::move(arr);
	return j;
}

// Validate the slot bookkeeping of a restored formation: every slot index is reachable exactly
// once via the free list + the per-line member chains (no cycles, no orphans, no double-links),
// and a slot is on a line if and only if it carries a droid id.
//
// Throws StateError on violation - a corrupt chain would otherwise loop or null-deref inside
// formationGetPos/formationLeave at sim time.
static void validateFormationChains(const FORMATION *f, const std::array<uint32_t, F_MAXMEMBERS> &slotDroidIds)
{
	std::array<bool, F_MAXMEMBERS> visited;
	visited.fill(false);
	auto walkChain = [&](SBYTE head, int lineIdx) // lineIdx: -1 = the free list
	{
		int steps = 0;
		for (SBYTE cur = head; cur != -1; cur = f->asMembers[cur].next)
		{
			// reqRange already bounded every next/member/free to [-1, F_MAXMEMBERS)
			if (visited[cur] || ++steps > F_MAXMEMBERS)
			{
				throw StateError("formation slot chain is cyclic or double-linked");
			}
			visited[cur] = true;
			if ((slotDroidIds[cur] != 0) != (lineIdx >= 0))
			{
				throw StateError("formation slot occupancy does not match its chain");
			}
			// An occupied slot's recorded line must be the chain it sits on, or formationLeave's
			// chain walk (which trusts asMembers[unit].line) would run off the end of the wrong chain.
			if (lineIdx >= 0 && f->asMembers[cur].line != lineIdx)
			{
				throw StateError("formation slot line does not match its chain");
			}
		}
	};
	walkChain(f->free, -1);
	for (int line = 0; line < f->numLines; ++line)
	{
		walkChain(f->asLines[line].member, line);
	}
	for (unsigned i = 0; i < F_MAXMEMBERS; ++i)
	{
		if (!visited[i])
		{
			throw StateError("formation slot not reachable from free list or any line");
		}
	}
}

static void readFormations(const nlohmann::ordered_json &j, uint32_t version)
{
	if (version != FORMATIONS_SECTION_VERSION)
	{
		throw StateError("unsupported formations section version");
	}
	if (!j.contains("formations"))
	{
		return;
	}
	const nlohmann::ordered_json &arr = reqArray(j.at("formations"), 0);
	// Insert REVERSED: formationRestoreInsert push_fronts (like formationNew), so reverse
	// insertion reproduces the serialized (live) per-player list order.
	for (auto it = arr.rbegin(); it != arr.rend(); ++it)
	{
		const nlohmann::ordered_json &o = *it;
		std::unique_ptr<FORMATION> f(new FORMATION()); // value-init: all fields zeroed
		f->player = reqPlayer(o.at("player"));
		f->x = o.at("x").get<int32_t>();
		f->y = o.at("y").get<int32_t>();
		f->direction = o.at("direction").get<uint16_t>();
		f->refCount = static_cast<SWORD>(reqRange(o.at("refCount").get<int>(), 0, 4096));
		f->size = static_cast<SWORD>(reqRange(o.at("size").get<int>(), 0, INT16_MAX));
		f->rankDist = static_cast<SWORD>(reqRange(o.at("rankDist").get<int>(), 0, INT16_MAX));
		f->numLines = static_cast<SWORD>(reqRange(o.at("numLines").get<int>(), 0, F_MAXLINES));
		f->maxRank = static_cast<UBYTE>(reqRange(o.at("maxRank").get<int>(), 0, UINT8_MAX));
		f->free = static_cast<SBYTE>(reqRange(o.at("free").get<int>(), -1, F_MAXMEMBERS - 1));
		f->iSpeed = o.at("iSpeed").get<uint32_t>();
		const nlohmann::ordered_json &lines = reqArray(o.at("lines"), F_MAXLINES);
		for (unsigned i = 0; i < F_MAXLINES; ++i)
		{
			const nlohmann::ordered_json &l = reqArray(lines[i], 4);
			f->asLines[i].xoffset = static_cast<SWORD>(reqRange(l[0].get<int>(), INT16_MIN, INT16_MAX));
			f->asLines[i].yoffset = static_cast<SWORD>(reqRange(l[1].get<int>(), INT16_MIN, INT16_MAX));
			f->asLines[i].direction = l[2].get<uint16_t>();
			f->asLines[i].member = static_cast<SBYTE>(reqRange(l[3].get<int>(), -1, F_MAXMEMBERS - 1));
		}
		RestoredFormationInfo info;
		const nlohmann::ordered_json &members = reqArray(o.at("members"), F_MAXMEMBERS);
		for (unsigned i = 0; i < F_MAXMEMBERS; ++i)
		{
			const nlohmann::ordered_json &m = reqArray(members[i], 4);
			f->asMembers[i].line = static_cast<SBYTE>(reqRange(m[0].get<int>(), 0, F_MAXLINES - 1));
			f->asMembers[i].next = static_cast<SBYTE>(reqRange(m[1].get<int>(), -1, F_MAXMEMBERS - 1));
			f->asMembers[i].dist = static_cast<SWORD>(reqRange(m[2].get<int>(), INT16_MIN, INT16_MAX));
			f->asMembers[i].psDroid = nullptr; // resolved as each rebuilt droid re-attaches
			info.slotDroidIds[i] = m[3].get<uint32_t>();
		}
		info.expectedMembers = f->refCount;
		validateFormationChains(f.get(), info.slotDroidIds);
		FORMATION *raw = f.release();
		formationRestoreInsert(raw);
		g_restoredFormations.emplace(raw, std::move(info));
	}
}

// Exact-coordinate lookup among the verbatim-restored formations. formationFind() matches within
// FIND_RANGE, which could bind a droid to a nearby-but-wrong formation - live data never holds two
// formations that share exact (player, x, y) (formationNew only runs after formationFind misses).
static FORMATION *findRestoredFormationExact(unsigned player, int x, int y)
{
	for (const auto &kv : g_restoredFormations)
	{
		if (kv.first->player == player && kv.first->x == x && kv.first->y == y)
		{
			return kv.first;
		}
	}
	return nullptr;
}

// Attach a rebuilt droid to its verbatim-restored formation: set the droid's back-pointer and
// resolve its slot's psDroid (if it held one). Deliberately NOT formationJoin - refCount, iSpeed,
// size and rankDist were restored verbatim and must not be re-derived.
static void formationRestoreAttachDroid(FORMATION *psFormation, DROID *psDroid)
{
	psDroid->sMove.psFormation = psFormation;
	auto it = g_restoredFormations.find(psFormation);
	ASSERT_OR_RETURN(, it != g_restoredFormations.end(), "Attach to a non-restored formation in verbatim mode");
	it->second.attachedMembers += 1;
	for (unsigned slot = 0; slot < F_MAXMEMBERS; ++slot)
	{
		if (it->second.slotDroidIds[slot] == psDroid->id)
		{
			psFormation->asMembers[slot].psDroid = psDroid;
			break;
		}
	}
}

// Post-pass after ALL droid restoration (active world + off-world + limbo):
// - verify every serialized slot id was claimed and every member re-attached
// - fail-soft on a violating (corrupt) snapshot by releasing unclaimed slots to the free list
//   and re-basing refCount on the droids actually attached, so no formation leaks or carries a
//   null-droid slot into the sim
static void finalizeRestoredFormations()
{
	for (auto &kv : g_restoredFormations)
	{
		FORMATION *f = kv.first;
		RestoredFormationInfo &info = kv.second;
		for (unsigned slot = 0; slot < F_MAXMEMBERS; ++slot)
		{
			if (info.slotDroidIds[slot] == 0 || f->asMembers[slot].psDroid != nullptr)
			{
				continue;
			}
			debug(LOG_ERROR, "Restored formation (player %u at %d,%d): slot %u droid id %u never re-attached; releasing slot",
			      f->player, f->x, f->y, slot, info.slotDroidIds[slot]);
			// Unlink the orphaned slot from its line chain and push it onto the free list
			// (formationLeave's surgery, keyed by slot index - chains are acyclic)
			const SBYTE line = f->asMembers[slot].line;
			SBYTE prev = -1;
			SBYTE cur = f->asLines[line].member;
			while (cur != -1 && cur != static_cast<SBYTE>(slot))
			{
				prev = cur;
				cur = f->asMembers[cur].next;
			}
			if (cur == static_cast<SBYTE>(slot))
			{
				if (prev == -1)
				{
					f->asLines[line].member = f->asMembers[slot].next;
				}
				else
				{
					f->asMembers[prev].next = f->asMembers[slot].next;
				}
			}
			f->asMembers[slot].next = f->free;
			f->free = static_cast<SBYTE>(slot);
		}
		if (info.attachedMembers != info.expectedMembers)
		{
			debug(LOG_ERROR, "Restored formation (player %u at %d,%d): refCount %d != re-attached members %d; re-basing",
			      f->player, f->x, f->y, info.expectedMembers, info.attachedMembers);
			f->refCount = static_cast<SWORD>(info.attachedMembers);
		}
	}
	g_restoredFormations.clear();
}

static nlohmann::ordered_json writeDroid(const DROID *d, bool onMission)
{
	nlohmann::ordered_json j = writeBaseObjectCommon(d);
	j["name"] = d->aName;
	j["originalBody"] = d->originalBody;
	j["droidType"] = static_cast<int>(d->droidType);
	j["numWeaps"] = d->numWeaps;

	nlohmann::ordered_json parts = nlohmann::ordered_json::object();
	parts["body"] = d->getBodyStats()->id.toUtf8();
	parts["brain"] = d->getBrainStats()->id.toUtf8();
	parts["propulsion"] = d->getPropulsionStats()->id.toUtf8();
	parts["repair"] = d->getRepairStats()->id.toUtf8();
	parts["ecm"] = d->getECMStats()->id.toUtf8();
	parts["sensor"] = d->getSensorStats()->id.toUtf8();
	parts["construct"] = d->getConstructStats()->id.toUtf8();
	nlohmann::ordered_json partWeapons = nlohmann::ordered_json::array();
	for (unsigned w = 0; w < d->numWeaps; ++w)
	{
		partWeapons.push_back(d->getWeaponStats(w)->id.toUtf8());
	}
	parts["weapons"] = std::move(partWeapons);
	j["parts"] = std::move(parts);

	nlohmann::ordered_json weapons = nlohmann::ordered_json::array();
	for (unsigned w = 0; w < d->numWeaps; ++w)
	{
		nlohmann::ordered_json jw = nlohmann::ordered_json::object();
		jw["ammo"] = d->asWeaps[w].ammo;
		jw["lastFired"] = d->asWeaps[w].lastFired;
		jw["shotsFired"] = d->asWeaps[w].shotsFired;
		jw["usedAmmo"] = d->asWeaps[w].usedAmmo;
		jw["rot"] = nlohmann::ordered_json::array({ d->asWeaps[w].rot.direction, d->asWeaps[w].rot.pitch, d->asWeaps[w].rot.roll });
		weapons.push_back(std::move(jw));
	}
	j["weapons"] = std::move(weapons);

	// Per-weapon-slot gameTime of the last failed nearest-target search - an intra-tick throttle.
	// Technically should be harmless across a tick boundary, but serialized so it restores exactly
	// (in the event this assumption changes in the future).
	nlohmann::ordered_json lcntf = nlohmann::ordered_json::array();
	for (unsigned w = 0; w < d->numWeaps; ++w)
	{
		lcntf.push_back(d->lastCheckNearestTargetFailed[w]);
	}
	j["lastCheckNearestTargetFailed"] = std::move(lcntf);

	j["experience"] = d->experience;
	j["kills"] = d->kills;
	j["shieldPoints"] = d->shieldPoints;
	j["shieldRegenTime"] = d->shieldRegenTime;
	j["shieldInterruptRegenTime"] = d->shieldInterruptRegenTime;
	j["lastFrustratedTime"] = d->lastFrustratedTime;
	j["resistance"] = d->resistance;
	j["secondaryOrder"] = d->secondaryOrder;
	j["action"] = static_cast<int>(d->action);
	j["actionPos"] = writeVector2i(d->actionPos);
	j["actionStarted"] = d->actionStarted;
	j["actionPoints"] = d->actionPoints;
	j["group"] = d->group;
	j["repairGroup"] = d->repairGroup;
	if (d->psGroup != nullptr)
	{
		j["aigroup"] = d->psGroup->id;
		j["aigroupType"] = static_cast<int>(d->psGroup->type);
	}
	if (hasCommander(d) && d->psGroup->psCommander->died <= NOT_CURRENT_LIST)
	{
		j["commander"] = d->psGroup->psCommander->id;
	}
	// For a command droid, serialize its group's member ids in psList order. Command-group iteration
	// order is sync-relevant (order.cpp syncDebug + DORDER_RECOVER tie-break), and the members re-attach
	// from the flat per-player list in an order that would not otherwise reproduce psList. The commander
	// itself is psCommander, not a psList entry. Restored via the reverse-add pass in readDroidList.
	if (d->droidType == DROID_COMMAND && d->psGroup != nullptr && d->psGroup->type == GT_COMMAND
	    && d->psGroup->psCommander == d)
	{
		nlohmann::ordered_json members = nlohmann::ordered_json::array();
		for (const DROID *psMember : d->psGroup->psList)
		{
			members.push_back(psMember->id);
		}
		j["cmdGroupMembers"] = std::move(members);
	}

	j["order"] = writeDroidOrder(d->order);
	nlohmann::ordered_json orderList = nlohmann::ordered_json::array();
	for (int i = 0; i < d->listSize; ++i)
	{
		orderList.push_back(writeDroidOrder(d->asOrderList[i]));
	}
	j["orderList"] = std::move(orderList);
	j["listSize"] = d->listSize;

	nlohmann::ordered_json actionTarget = nlohmann::ordered_json::array();
	for (int w = 0; w < MAX_WEAPONS; ++w)
	{
		actionTarget.push_back(writeObjRef(d->psActionTarget[w]));
	}
	j["actionTarget"] = std::move(actionTarget);
	j["baseStruct"] = writeObjRef(d->psBaseStruct);

	nlohmann::ordered_json mv = nlohmann::ordered_json::object();
	mv["status"] = static_cast<int>(d->sMove.Status);
	mv["pathIndex"] = d->sMove.pathIndex;
	nlohmann::ordered_json path = nlohmann::ordered_json::array();
	for (const Vector2i &p : d->sMove.asPath)
	{
		path.push_back(writeVector2i(p));
	}
	mv["path"] = std::move(path);
	mv["destination"] = writeVector2i(d->sMove.destination);
	mv["src"] = writeVector2i(d->sMove.src);
	mv["target"] = writeVector2i(d->sMove.target);
	mv["speed"] = d->sMove.speed;
	mv["moveDir"] = d->sMove.moveDir;
	mv["bumpDir"] = d->sMove.bumpDir;
	mv["vertSpeed"] = d->sMove.iVertSpeed;
	mv["bumpTime"] = d->sMove.bumpTime;
	mv["shuffleStart"] = d->sMove.shuffleStart;
	mv["lastBump"] = d->sMove.lastBump;
	mv["pauseTime"] = d->sMove.pauseTime;
	mv["bumpPos"] = writeVector2i(d->sMove.bumpPos.xy());
	// Waypoint-give-up counter: accumulates over time to loosen the "reached waypoint" threshold so a
	// lingering droid eventually advances (moveReachedWayPoint). If reset to 0 on restore, a droid mid-
	// accumulation would advance a different tick than the original, diverging its path progress.
	mv["tolerance"] = d->sMove.tolerance;
	// The record `tolerance` is grown from at the end of a route.
	// Restoring the counter while its record started over would grow the total again from a different tick.
	mv["settleTime"] = d->sMove.settleTime;
	mv["settleBest"] = d->sMove.settleBest;
	j["move"] = std::move(mv);

	if (d->sMove.psFormation != nullptr)
	{
		nlohmann::ordered_json f = nlohmann::ordered_json::object();
		f["direction"] = d->sMove.psFormation->direction;
		f["x"] = d->sMove.psFormation->x;
		f["y"] = d->sMove.psFormation->y;
		j["formation"] = std::move(f);
	}

	j["underRepair"] = d->underRepair;
	j["onMission"] = onMission;
	return j;
}

// Serialize a per-player droid list (with transporter cargo). onMission marks droids that are
// not live on the active map (off-world / limbo), affecting how they are reconstructed.
static nlohmann::ordered_json writeDroidList(const PerPlayerDroidLists &lists, bool onMission)
{
	nlohmann::ordered_json arr = nlohmann::ordered_json::array();
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		for (const DROID *psDroid : lists[p])
		{
			arr.push_back(writeDroid(psDroid, onMission));
			if (psDroid->isTransporter() && psDroid->psGroup != nullptr)
			{
				for (const DROID *psCargo : psDroid->psGroup->psList)
				{
					if (psCargo != psDroid)
					{
						arr.push_back(writeDroid(psCargo, onMission));
					}
				}
			}
		}
	}
	return arr;
}

/// Resolve a component stat index by id string, falling back to the null component (index 0)
template <typename T>
static T compIndexFromId(COMPONENT_TYPE compType, const WzString &name)
{
	const int idx = getCompFromID(compType, name);
	return static_cast<T>(idx >= 0 ? idx : 0);
}

/// Pass 1: construct the droid (from a fake template), restore scalar/movement state and group membership.
/// Records id -> droid for pass 2. Does NOT add to the per-player list (done separately).
static void readDroidPass1(GameWorld &world, const nlohmann::ordered_json &j, std::unordered_map<int, DROID_GROUP *> &groupMap,
                           std::unordered_map<uint32_t, DROID *> &droidById)
{
	DROID_TEMPLATE templ;
	templ.name = WzString::fromUtf8(j.at("name").get<std::string>());
	templ.droidType = static_cast<DROID_TYPE>(j.at("droidType").get<int>());
	// Clamp to [0, MAX_WEAPONS]: numWeaps is copied to the unsigned BASE_OBJECT::numWeaps and bounds the
	// asWeaps[MAX_WEAPONS] restore loop, so a negative or overlarge value must not get through.
	templ.numWeaps = static_cast<int8_t>(std::clamp(j.at("numWeaps").get<int>(), 0, static_cast<int>(MAX_WEAPONS)));
	const nlohmann::ordered_json &parts = j.at("parts");
	const auto part = [&parts](COMPONENT_TYPE compType, const char *key, const char *nullName)
	{
		return compIndexFromId<uint8_t>(compType, WzString::fromUtf8(parts.value(key, nullName)));
	};
	templ.asParts[COMP_BODY] = part(COMP_BODY, "body", "ZNULLBODY");
	templ.asParts[COMP_BRAIN] = part(COMP_BRAIN, "brain", "ZNULLBRAIN");
	templ.asParts[COMP_PROPULSION] = part(COMP_PROPULSION, "propulsion", "ZNULLPROP");
	templ.asParts[COMP_REPAIRUNIT] = part(COMP_REPAIRUNIT, "repair", "ZNULLREPAIR");
	templ.asParts[COMP_ECM] = part(COMP_ECM, "ecm", "ZNULLECM");
	templ.asParts[COMP_SENSOR] = part(COMP_SENSOR, "sensor", "ZNULLSENSOR");
	templ.asParts[COMP_CONSTRUCT] = part(COMP_CONSTRUCT, "construct", "ZNULLCONSTRUCT");
	const nlohmann::ordered_json &partWeapons = parts.at("weapons");
	for (int w = 0; w < MAX_WEAPONS; ++w)
	{
		const std::string wid = (static_cast<size_t>(w) < partWeapons.size()) ? partWeapons[w].get<std::string>() : "ZNULLWEAPON";
		templ.asWeaps[w] = compIndexFromId<uint32_t>(COMP_WEAPON, WzString::fromUtf8(wid));
	}

	const uint32_t id = j.at("id").get<uint32_t>();
	const nlohmann::ordered_json &jpos = j.at("pos");
	Position pos(jpos[0].get<int32_t>(), jpos[1].get<int32_t>(), jpos[2].get<int32_t>());
	const nlohmann::ordered_json &jrot = j.at("rot");
	const Rotation rot(jrot[0].get<uint16_t>(), jrot[1].get<uint16_t>(), jrot[2].get<uint16_t>());
	const unsigned player = j.at("player").get<unsigned>();
	const bool onMission = j.value("onMission", false);

	// Mirror loadSaveDroid: an off-map coordinate (ex. a flying-in transporter's cargo parked at
	// INVALID_XY) would make reallyBuildDroid's map_Height() query assert. Clamp non-mission positions
	// into the map first - the cargo's real position is driven by its transporter, not this stored value.
	if (!onMission)
	{
		pos.x = clip(pos.x, world_coord(1), world_coord(world.map.width - 1));
		pos.y = clip(pos.y, world_coord(1), world_coord(world.map.height - 1));
	}

	DROID *d = reallyBuildDroid(world, &templ, pos, player, onMission, rot, id);
	if (d == nullptr)
	{
		throw StateError("failed to reconstruct droid id " + std::to_string(id));
	}
	droidById[id] = d;

	d->originalBody = j.at("originalBody").get<uint32_t>(); // must precede body (CHECK_DROID)
	readBaseObjectCommon(j, d);
	d->experience = j.at("experience").get<uint32_t>();
	d->kills = j.at("kills").get<uint32_t>();
	if (j.contains("lastCheckNearestTargetFailed"))
	{
		const nlohmann::ordered_json &lcntf = j.at("lastCheckNearestTargetFailed");
		for (unsigned w = 0; w < d->numWeaps && w < lcntf.size() && w < MAX_WEAPONS; ++w)
		{
			d->lastCheckNearestTargetFailed[w] = lcntf[w].get<uint32_t>();
		}
	}
	d->shieldPoints = j.at("shieldPoints").get<int32_t>();
	d->shieldRegenTime = j.at("shieldRegenTime").get<uint32_t>();
	d->shieldInterruptRegenTime = j.at("shieldInterruptRegenTime").get<uint32_t>();
	d->lastFrustratedTime = j.at("lastFrustratedTime").get<uint32_t>();
	d->resistance = j.at("resistance").get<int>();
	d->secondaryOrder = j.at("secondaryOrder").get<uint32_t>();
	d->secondaryOrderPending = d->secondaryOrder;
	d->action = static_cast<DROID_ACTION>(j.at("action").get<int>());
	d->actionPos = readVector2i(j.at("actionPos"));
	d->actionStarted = j.at("actionStarted").get<uint32_t>();
	d->actionPoints = j.at("actionPoints").get<int>();
	d->group = j.at("group").get<uint8_t>();
	d->repairGroup = j.at("repairGroup").get<uint8_t>();

	const nlohmann::ordered_json &weapons = j.at("weapons");
	for (unsigned w = 0; w < d->numWeaps && w < weapons.size() && w < MAX_WEAPONS; ++w)
	{
		if (d->asWeaps[w].nStat > 0)
		{
			d->asWeaps[w].ammo = weapons[w].at("ammo").get<uint32_t>();
			d->asWeaps[w].lastFired = weapons[w].at("lastFired").get<uint32_t>();
			d->asWeaps[w].shotsFired = weapons[w].at("shotsFired").get<uint32_t>();
			d->asWeaps[w].usedAmmo = weapons[w].at("usedAmmo").get<uint32_t>();
			const nlohmann::ordered_json &jr = weapons[w].at("rot");
			d->asWeaps[w].rot.direction = jr[0].get<uint16_t>();
			d->asWeaps[w].rot.pitch = jr[1].get<uint16_t>();
			d->asWeaps[w].rot.roll = jr[2].get<uint16_t>();
		}
	}

	// Group membership (mirrors loadSaveDroid). Transporter/command droids were created with
	// their own group by reallyBuildDroid - members map back to it by saved group id.
	const int aigroup = j.value("aigroup", -1);
	if (aigroup >= 0)
	{
		auto it = groupMap.find(aigroup);
		if (it != groupMap.end())
		{
			// Transporter cargo AND command-group members are deferred to readDroidList's reverse
			// insertion pass: DROID_GROUP::add prepends (push_front), so adding them here in forward
			// (priority-sorted) order would not reproduce the saved psList order. Both are serialized in
			// psList order (cargo inline after its transporter, command members via "cmdGroupMembers" on
			// the commander) and re-added in reverse there. Other group types (GT_NORMAL) add here.
			const bool isTransporterCargo = (it->second->type == GT_TRANSPORTER && !d->isTransporter());
			const bool isCommandMember = (it->second->type == GT_COMMAND);
			if (!isTransporterCargo && !isCommandMember)
			{
				it->second->add(d);
			}
			else if (d->psGroup != nullptr)
			{
				// A command droid that is itself transporter cargo was given its own fresh group by
				// reallyBuildDroid. Its real group is the transporter's (re-added in the deferred reverse
				// pass), so drop that spurious group now: otherwise it lingers until the reverse pass and
				// its lowest-free id can collide with a later owner's grpReassignId (ex. another command
				// group that saved the same id).
				d->psGroup->remove(d);
			}
		}
		else if (d->isTransporter() || d->droidType == DROID_COMMAND)
		{
			if (d->psGroup != nullptr)
			{
				// Restore the group's saved id (grpCreate assigned a fresh lowest-free id during
				// reallyBuildDroid - the id round-trips as the "aigroup" membership key).
				// Collision-free: owners are built before their members (droidLoadPriority sort),
				// and every group already in the manager holds its final saved id, so the target
				// slot is never occupied.
				grpReassignId(d->psGroup, aigroup);
				groupMap[aigroup] = d->psGroup;
			}
		}
		if (d->psGroup != nullptr && d->psGroup->type == GT_TRANSPORTER)
		{
			d->selected = false;
			if (!d->isTransporter())
			{
				visRemoveVisibility(d, world.map);
			}
		}
	}
	else if (!(d->isTransporter() || d->droidType == DROID_COMMAND))
	{
		d->psGroup = nullptr;
	}

	// Movement state:
	const nlohmann::ordered_json &mv = j.at("move");
	d->sMove.Status = static_cast<MOVE_STATUS>(mv.at("status").get<int>());
	d->sMove.pathIndex = mv.at("pathIndex").get<int>();
	const nlohmann::ordered_json &path = mv.at("path");
	d->sMove.asPath.resize(path.size());
	for (size_t p = 0; p < path.size(); ++p)
	{
		d->sMove.asPath[p] = readVector2i(path[p]);
	}
	d->sMove.destination = readVector2i(mv.at("destination"));
	d->sMove.src = readVector2i(mv.at("src"));
	d->sMove.target = readVector2i(mv.at("target"));
	d->sMove.speed = mv.at("speed").get<int>();
	d->sMove.moveDir = mv.at("moveDir").get<uint16_t>();
	d->sMove.bumpDir = mv.at("bumpDir").get<uint16_t>();
	d->sMove.iVertSpeed = mv.at("vertSpeed").get<int>();
	d->sMove.bumpTime = mv.at("bumpTime").get<uint32_t>();
	d->sMove.shuffleStart = mv.at("shuffleStart").get<uint32_t>();
	d->sMove.lastBump = mv.at("lastBump").get<uint32_t>();
	d->sMove.pauseTime = mv.at("pauseTime").get<uint32_t>();
	const Vector2i bp = readVector2i(mv.at("bumpPos"));
	d->sMove.bumpPos = Vector3i(bp.x, bp.y, 0);
	d->sMove.tolerance = mv.at("tolerance").get<uint32_t>();
	d->sMove.settleTime = mv.value("settleTime", static_cast<uint32_t>(0));
	d->sMove.settleBest = mv.value("settleBest", static_cast<int32_t>(0));
	if (d->isVtol() && d->sMove.Status != MOVEINACTIVE)
	{
		d->rot.pitch = 0;
	}

	d->underRepair = static_cast<uint16_t>(j.at("underRepair").get<uint32_t>());

	// Formation (movement formation), if any: attach to the verbatim-restored formation
	// (exact-coordinate match; formationFind's FIND_RANGE could bind a nearby wrong one).
	// No formationJoin - its state is already exact (see the formations section above).
	if (j.contains("formation"))
	{
		const nlohmann::ordered_json &f = j.at("formation");
		const int fx = f.at("x").get<int>();
		const int fy = f.at("y").get<int>();
		FORMATION *restored = findRestoredFormationExact(d->player, fx, fy);
		if (restored != nullptr)
		{
			formationRestoreAttachDroid(restored, d);
		}
		else
		{
			// Invariant violation (droid references a formation the section does not carry):
			// fail soft by re-deriving so the droid is at least tracked. Find-first so
			// co-members of the same lost formation share one re-derived formation.
			debug(LOG_ERROR, "Droid %u references formation at (%d,%d) missing from the formations section", d->id, fx, fy);
			d->sMove.psFormation = formationFind(d->player, fx, fy);
			if (d->sMove.psFormation != nullptr)
			{
				formationJoin(d->sMove.psFormation, d);
			}
			else if (formationNew(&d->sMove.psFormation, d->player, FT_LINE, fx, fy, f.at("direction").get<uint16_t>()))
			{
				formationJoin(d->sMove.psFormation, d);
			}
		}
	}

	// NOTE: Unlike legacy loadSaveDroid we do NOT re-issue a path-finding route here for a droid in
	// MOVEWAITROUTE. Legacy saves omit the movement path, so they must re-path on load, but the snapshot
	// serializes the complete sMove (status, path, pathIndex, destination), so MOVEWAITROUTE is
	// restored verbatim. The droid is not stuck: the next tick's moveUpdateDroid MOVEWAITROUTE case
	// calls moveDroidTo(), re-issuing the route exactly as the original run did on its next tick.
	// Re-pathing here would instead force MOVEINACTIVE/MOVENAVIGATE and, on route failure,
	// actionDroid(DACTION_SULK) - clobbering the authoritative restored action/actionStarted and
	// breaking the round-trip / deterministic resume.

	// Sync the render interpolation baseline. A freshly reconstructed droid keeps the prevSpacetime
	// that buildDroid set at construction time, which can match the restored time and trigger a
	// "Spacetime overlap!" assert (prevSpacetime.time == time) on the first render frame. Mirror the
	// engine idiom: prevSpacetime = current spacetime with time - 1, so interpolation is stable and
	// non-overlapping (same pos/rot, distinct time).
	d->prevSpacetime = getSpacetime(d);
	d->prevSpacetime.time = d->time - 1;
}

/// Pass 2: resolve cross-references now that all objects exist (and droids are in lists).
static void readDroidPass2(const nlohmann::ordered_json &j, std::unordered_map<uint32_t, DROID *> &droidById)
{
	const uint32_t id = j.at("id").get<uint32_t>();
	auto it = droidById.find(id);
	if (it == droidById.end())
	{
		throw StateError("droid pass 2: object not found, id " + std::to_string(id));
	}
	DROID *d = it->second;

	d->listSize = std::max(0, std::min(j.at("listSize").get<int>(), 10000));
	const nlohmann::ordered_json &orderList = j.at("orderList");
	d->asOrderList.resize(d->listSize);
	for (int i = 0; i < d->listSize && static_cast<size_t>(i) < orderList.size(); ++i)
	{
		readDroidOrder(orderList[i], d->asOrderList[i]);
	}
	d->listPendingBegin = 0;

	const nlohmann::ordered_json &actionTarget = j.at("actionTarget");
	for (int w = 0; w < MAX_WEAPONS && static_cast<size_t>(w) < actionTarget.size(); ++w)
	{
		d->psActionTarget[w] = readObjRef(actionTarget[w]);
	}

	BASE_OBJECT *base = readObjRef(j.at("baseStruct"));
	if (base != nullptr && base->type == OBJ_STRUCTURE)
	{
		setSaveDroidBase(d, static_cast<STRUCTURE *>(base));
	}

	// NOTE: Command-group membership is NOT re-established here via cmdDroidAddDroid. The droid was
	// already added to its commander's group in pass 1 (the same psGroup->add() call cmdDroidAddDroid
	// makes), and hasCommander()/the commander relationship derive from that group (psGroup->psCommander).
	// cmdDroidAddDroid is the gameplay "assign unit to commander" path: it issues orderDroidObj(DORDER_GUARD),
	// syncs secondary states from the commander, and resets group/repairGroup - all of which would clobber
	// the authoritative restored action/order/secondaryOrder/group state (and double-add the droid).
	// The saved "commander" field is therefore advisory only - membership comes from "aigroup".

	readDroidOrder(j.at("order"), d->order);
}

/// Construction priority: transporters/commanders must be built before the droids they hold/lead.
static int droidLoadPriority(const nlohmann::ordered_json &j)
{
	switch (static_cast<DROID_TYPE>(j.at("droidType").get<int>()))
	{
	case DROID_TRANSPORTER:      return 3;
	case DROID_SUPERTRANSPORTER: return 2;
	case DROID_COMMAND:          return 1;
	default:                     return 0;
	}
}

// Reconstruct a droid list: construct in priority order (transporters/commanders first so their groups exist),
// insert into targetList in the host's order (reverse rebuild of the prepending list), then resolve cross-references.
// constructWorld supplies the build context (container/map).
static void readDroidList(GameWorld &constructWorld, PerPlayerDroidLists &targetList, const nlohmann::ordered_json &jdroids)
{
	std::unordered_map<int, DROID_GROUP *> groupMap;
	std::unordered_map<uint32_t, DROID *> droidById;
	std::vector<size_t> order(jdroids.size());
	for (size_t i = 0; i < jdroids.size(); ++i)
	{
		order[i] = i;
	}
	std::stable_sort(order.begin(), order.end(),
		[&jdroids](size_t a, size_t b) { return droidLoadPriority(jdroids[a]) > droidLoadPriority(jdroids[b]); });
	for (size_t idx : order)
	{
		readDroidPass1(constructWorld, jdroids[idx], groupMap, droidById);
	}
	// Insert in reverse order (prepend -> reproduces original order). Transporter cargo is kept out of
	// the per-player list and instead added to its transporter's group here, also in reverse order, so
	// DROID_GROUP::add's push_front reproduces the saved psList order (cargo is serialized in psList
	// order). Cargo membership is deferred to here (not pass 1) precisely so it can be added in reverse.
	for (size_t i = jdroids.size(); i-- > 0; )
	{
		const nlohmann::ordered_json &jd = jdroids[i];
		DROID *d = droidById.at(jd.at("id").get<uint32_t>());
		DROID_GROUP *transporterGrp = nullptr;
		const int aigroup = jd.value("aigroup", -1);
		if (aigroup >= 0 && !d->isTransporter())
		{
			auto it = groupMap.find(aigroup);
			if (it != groupMap.end() && it->second->type == GT_TRANSPORTER)
			{
				transporterGrp = it->second;
			}
		}
		if (transporterGrp != nullptr)
		{
			transporterGrp->add(d);
			d->selected = false;
			visRemoveVisibility(d, constructWorld.map);
		}
		else
		{
			addDroid(d, targetList);
		}
	}
	// Re-establish command-group membership in the saved psList order (deferred out of pass 1). Members
	// were serialized in psList order on their commander ("cmdGroupMembers") - DROID_GROUP::add prepends,
	// so add them in reverse to reproduce that order (sync-relevant: order.cpp iterates psList). The
	// commander is psCommander (not a psList entry) and its group already exists from pass 1.
	for (const nlohmann::ordered_json &jd : jdroids)
	{
		if (!jd.contains("cmdGroupMembers"))
		{
			continue;
		}
		DROID *psCommander = droidById.at(jd.at("id").get<uint32_t>());
		if (psCommander->psGroup == nullptr)
		{
			continue;
		}
		const nlohmann::ordered_json &members = jd.at("cmdGroupMembers");
		for (size_t i = members.size(); i-- > 0; )
		{
			auto it = droidById.find(members[i].get<uint32_t>());
			if (it != droidById.end())
			{
				psCommander->psGroup->add(it->second);
			}
		}
	}
	for (const nlohmann::ordered_json &jd : jdroids)
	{
		readDroidPass2(jd, droidById);
	}
}

// MARK: - Per-tile dynamic map state
//
// Only the authoritative state that is NOT rebuilt from objects:
// - the explored (fog-of-war) bitmask and fire (BITS_ON_FIRE + fireEndTime)
//
// Sensor/jammer/watcher coverage is rebuilt by visTilesUpdate during object reconstruction.
// Terrain (texture/height/water) comes from the map file. Continents/aux maps are recomputed.
// Applied AFTER objects so the explored set is exactly the saved (authoritative) one rather
// than only what restored objects re-reveal.

static nlohmann::ordered_json writeMapDynamic(const GameWorld &world)
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["width"] = world.map.width;
	j["height"] = world.map.height;
	nlohmann::ordered_json explored = nlohmann::ordered_json::array();
	nlohmann::ordered_json fire = nlohmann::ordered_json::array();
	if (world.map.tiles)
	{
		const size_t n = static_cast<size_t>(world.map.width) * static_cast<size_t>(world.map.height);
		for (size_t i = 0; i < n; ++i)
		{
			const MAPTILE &t = world.map.tiles[i];
			explored.push_back(t.tileExploredBits);
			if (t.tileInfoBits & BITS_ON_FIRE)
			{
				nlohmann::ordered_json f = nlohmann::ordered_json::object();
				f["i"] = static_cast<uint32_t>(i);
				f["t"] = t.fireEndTime;
				fire.push_back(std::move(f));
			}
		}
	}
	j["explored"] = std::move(explored);
	j["fire"] = std::move(fire);
	return j;
}

static void readMapDynamic(GameWorld &world, const nlohmann::ordered_json &j)
{
	if (!world.map.tiles)
	{
		return; // no map loaded (i.e. the early-CLI self-test)
	}
	const int w = j.at("width").get<int>();
	const int h = j.at("height").get<int>();
	if (w != world.map.width || h != world.map.height)
	{
		throw StateError("map dynamic state dimensions mismatch");
	}
	const size_t n = static_cast<size_t>(w) * static_cast<size_t>(h);
	const nlohmann::ordered_json &explored = j.at("explored");
	if (!explored.is_array() || explored.size() != n)
	{
		throw StateError("map dynamic explored array size mismatch");
	}
	for (size_t i = 0; i < n; ++i)
	{
		world.map.tiles[i].tileExploredBits = static_cast<PlayerMask>(explored[i].get<uint32_t>());
		world.map.tiles[i].tileInfoBits &= ~BITS_ON_FIRE;
		world.map.tiles[i].fireEndTime = 0;
	}
	for (const nlohmann::ordered_json &f : j.at("fire"))
	{
		const size_t i = f.at("i").get<uint32_t>();
		if (i < n)
		{
			world.map.tiles[i].fireEndTime = f.at("t").get<uint16_t>();
			world.map.tiles[i].tileInfoBits |= BITS_ON_FIRE;
		}
	}
}

// MARK: - Danger maps (Skirmish/MP AI threat/danger overlay)
//
// The skirmish danger system (map.cpp) refreshes one player's threat/danger overlay per
// GAME_TICKS_FOR_DANGER in a round-robin, so a running client holds STAGGERED per-player maps (each up
// to maxPlayers*2s stale). The schedule (lastDangerUpdate / lastDangerPlayer) round-trips in the
// determinism core, but the staggered CONTENT is otherwise recomputed all-fresh-at-tick-T by mapInit on
// cold load and would diverge for any player whose threat footprint changed since its last slot. astar
// reads AUXBITS_THREAT for AI ground moves and safeDest() reads AUXBITS_DANGER, so a single diverged AI
// path cascades into a permanent desync. Campaign is exempt (no danger thread).
// We serialize:
//   - Per-player auxMap[p] DANGER|THREAT|AATHREAT bits, p in [0, MAX_PLAYERS) - the harvested overlays
//     (the full range mapInit() initializes. Players in [game.maxPlayers, MAX_PLAYERS) are never refreshed
//     by the round-robin but keep init-time danger that fpath still reads, so they must be saved too).
//   - The in-flight working buffer auxMap[MAX_PLAYERS+AUX_DANGERMAP] (verbatim) + blockMap[AUX_DANGERMAP],
//     i.e. the inputs dangerFloodFill() reads for lastDangerPlayer (its DANGER/TEMPORARY scratch bits are
//     recomputed by the worker on restart, so they ride along harmlessly).
// The worker's DANGER output for the in-flight player is NOT stored: on restore the restarted thread
// re-floods it deterministically from the restored working THREAT/NONPASSABLE + blockMap + start pos.
constexpr uint32_t DANGER_SECTION_VERSION = 1;
constexpr uint8_t DANGER_OVERLAY_BITS = AUXBITS_DANGER | AUXBITS_THREAT | AUXBITS_AATHREAT;

static nlohmann::ordered_json writeDangerMaps(const GameWorld &world)
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = DANGER_SECTION_VERSION;
	// Only SKIRMISH runs the danger thread/overlay (campaign is exempt) - nothing to store otherwise.
	if (game.type != LEVEL_TYPE::SKIRMISH || !world.map.tiles || !world.map.auxMap[0])
	{
		j["present"] = false;
		return j;
	}
	j["present"] = true;
	j["width"] = world.map.width;
	j["height"] = world.map.height;
	// mapInit() initializes the danger overlay for ALL MAX_PLAYERS players, but mapUpdate's round-robin
	// only REFRESHES game.maxPlayers of them (% game.maxPlayers). Players in [maxPlayers, MAX_PLAYERS) thus
	// keep static init-time danger that fpath still reads for any droids they own (astar AUXBITS_THREAT).
	// On cold-load the snapshot-aware mapInit skips the re-init, so we must serialize the FULL MAX_PLAYERS
	// range - storing only game.maxPlayers loses those players' overlay and desyncs their AI pathfinding.
	j["maxPlayers"] = game.maxPlayers; // informational (not used to size the overlay array on read)
	const int numOverlays = MAX_PLAYERS;
	j["numPlayerOverlays"] = numOverlays;
	const size_t n = static_cast<size_t>(world.map.width) * static_cast<size_t>(world.map.height);

	// Per-player harvested overlay bits. auxMap[p] (p < MAX_PLAYERS) is only written by the main thread
	// (mapUpdate's auxMapRestore), so these reads do not race the worker.
	// Per-player harvested overlay bits, one base64 byte blob per player.
	nlohmann::ordered_json players = nlohmann::ordered_json::array();
	for (int p = 0; p < numOverlays; ++p)
	{
		std::vector<uint8_t> bytes(n);
		const uint8_t *aux = world.map.auxMap[p].get();
		for (size_t i = 0; i < n; ++i)
		{
			bytes[i] = static_cast<uint8_t>(aux[i] & DANGER_OVERLAY_BITS);
		}
		players.push_back(base64Encode(bytes));
	}
	j["players"] = std::move(players);

	// In-flight working buffer + danger blocking snapshot. The worker WRITES the working buffer, so park
	// it for the duration of this read (no-op when no worker is running, i.e. the headless self-test).
	const bool parked = mapDangerSerializeBegin();
	std::vector<uint8_t> work(n);
	std::vector<uint8_t> blockDanger(n);
	const uint8_t *wb = world.map.auxMap[MAX_PLAYERS + AUX_DANGERMAP].get();
	const uint8_t *bd = world.map.blockMap[AUX_DANGERMAP].get();
	for (size_t i = 0; i < n; ++i)
	{
		work[i] = wb[i];
		blockDanger[i] = bd[i];
	}
	mapDangerSerializeEnd(parked);
	j["work"] = base64Encode(work);
	j["blockDanger"] = base64Encode(blockDanger);
	return j;
}

static void readDangerMaps(GameWorld &world, const nlohmann::ordered_json &j, uint32_t version)
{
	if (version != DANGER_SECTION_VERSION)
	{
		throw StateError("unsupported dangerMaps section version");
	}
	if (!j.value("present", false))
	{
		return; // campaign / no danger overlay serialized - mapInit recomputes fresh as usual
	}
	if (!world.map.tiles || !world.map.auxMap[0])
	{
		return; // no map (headless self-test) - nothing to apply
	}
	const int w = j.at("width").get<int>();
	const int h = j.at("height").get<int>();
	if (w != world.map.width || h != world.map.height)
	{
		throw StateError("dangerMaps dimensions mismatch");
	}
	// Number of per-player overlays stored.
	const int numOverlays = j.at("numPlayerOverlays").get<int>();
	if (numOverlays < 0 || numOverlays > MAX_PLAYERS)
	{
		throw StateError("dangerMaps player overlay count out of range");
	}
	const size_t n = static_cast<size_t>(w) * static_cast<size_t>(h);

	// Masked-merge each player's overlay into auxMap[p]: only the DANGER|THREAT|AATHREAT bits are ours.
	// The blocking bits (NONPASSABLE/OUR_BUILDING/BLOCKING) were deterministically rebuilt from the
	// restored structures and must be preserved.
	const nlohmann::ordered_json &players = j.at("players");
	if (!players.is_array() || static_cast<int>(players.size()) != numOverlays)
	{
		throw StateError("dangerMaps players array size mismatch");
	}
	for (int p = 0; p < numOverlays; ++p)
	{
		const std::vector<uint8_t> bytes = decodeBase64Field(players[p], n, "dangerMaps per-player overlay");
		uint8_t *aux = world.map.auxMap[p].get();
		for (size_t i = 0; i < n; ++i)
		{
			const uint8_t v = static_cast<uint8_t>(bytes[i] & DANGER_OVERLAY_BITS);
			aux[i] = static_cast<uint8_t>((aux[i] & ~DANGER_OVERLAY_BITS) | v);
		}
	}

	// In-flight working buffer (verbatim) + danger blocking snapshot. The danger thread is stopped during
	// reconstruct (mapStopDangerThreadForReconstruct), so these writes do not race it - mapInit restarts
	// the worker, which re-floods lastDangerPlayer from exactly these inputs.
	const std::vector<uint8_t> work = decodeBase64Field(j.at("work"), n, "dangerMaps work buffer");
	const std::vector<uint8_t> blockDanger = decodeBase64Field(j.at("blockDanger"), n, "dangerMaps blockDanger");
	uint8_t *wb = world.map.auxMap[MAX_PLAYERS + AUX_DANGERMAP].get();
	uint8_t *bd = world.map.blockMap[AUX_DANGERMAP].get();
	for (size_t i = 0; i < n; ++i)
	{
		wb[i] = work[i];
		bd[i] = blockDanger[i];
	}

	// Tell the next mapInit() to preserve this restored content (and the schedule) instead of recomputing.
	mapNoteDangerRestoredFromSnapshot();
}

static nlohmann::ordered_json writeWorldObjects(const GameWorld &world, bool onMission)
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = WORLD_SECTION_VERSION;
	j["mapDynamic"] = writeMapDynamic(world);

	nlohmann::ordered_json jfeatures = nlohmann::ordered_json::array();
	for (const FEATURE *psFeature : world.objects.features[0])
	{
		jfeatures.push_back(writeFeature(psFeature));
	}
	j["features"] = std::move(jfeatures);

	nlohmann::ordered_json jstructures = nlohmann::ordered_json::array();
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		for (const STRUCTURE *psStruct : world.objects.structures[p])
		{
			jstructures.push_back(writeStructure(psStruct));
		}
	}
	j["structures"] = std::move(jstructures);

	j["droids"] = writeDroidList(world.objects.droids, onMission);

	return j;
}

static void readWorldObjects(GameWorld &world, const nlohmann::ordered_json &j, uint32_t version)
{
	if (version != WORLD_SECTION_VERSION)
	{
		throw StateError("unsupported world section version");
	}

	// Clear the existing world (objects + map tile pointers) before reconstructing.
	clearWorldObjects(world);

	const nlohmann::ordered_json &jfeatures = j.at("features");
	const nlohmann::ordered_json &jstructures = j.at("structures");
	const nlohmann::ordered_json &jdroids = j.at("droids");
	if (!jfeatures.is_array() || !jstructures.is_array() || !jdroids.is_array())
	{
		throw StateError("world.features/structures/droids must be arrays");
	}
	// PASS 1: Construct objects. Rebuild in reverse: addObjectToList prepends
	// (emplace_front), so reverse rebuild reproduces the original list order.
	for (size_t i = jfeatures.size(); i-- > 0; )
	{
		readFeature(world, jfeatures[i]);
	}
	for (size_t i = jstructures.size(); i-- > 0; )
	{
		readStructurePass1(world, jstructures[i]);
	}
	resetFactoryNumFlag(world.objects);

	// Droids: construct in priority order, insert in host order, resolve cross-refs.
	readDroidList(world, world.objects.droids, jdroids);

	// Structure cross-references (targets/commander/repair-rearm) need all objects + lists.
	for (const nlohmann::ordered_json &js : jstructures)
	{
		readStructurePass2(js);
	}

	// Override the greedy derrick<->power-gen rebuild with the exact saved slotting (lockstep CRC).
	restorePowerLinkage(world, jstructures);

	// Apply authoritative per-tile dynamic state last (fog/fire), overriding what restored
	// objects re-revealed, so the explored set exactly matches the saved one.
	if (j.contains("mapDynamic"))
	{
		readMapDynamic(world, j.at("mapDynamic"));
	}
}

// MARK: - Section: pendingRoutes (in-flight path results)
//
// A droid in MOVEWAITROUTE is waiting on an async fpath request whose result is engine-global and not
// part of the object graph. Re-deriving it on restore (resubmitUncoveredRoutes) is NOT exact:
// the worker's per-thread A* PathfindContext for a co-destination cohort is built incrementally and is
// also shaped by cohort jobs that already COMPLETED this tick (those droids have left MOVEWAITROUTE and
// are not re-submitted), so a re-derived tail job builds from a different context and can pick a
// different equal-cost path -> desync. Instead we serialize the RESULT itself and replay it directly:
// the droid consumes it on its first resumed tick with no re-derivation, so order / context / thread
// count are all irrelevant.
//
// fpathTakePendingResult force-completes the still-pending future (deterministic: the value is fixed by
// the frozen end-of-tick context) and re-populates it so a host that keeps simulating is unaffected.
constexpr uint32_t PENDING_ROUTES_VERSION = 1;

static nlohmann::ordered_json writePendingRoutes()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = PENDING_ROUTES_VERSION;
	nlohmann::ordered_json routes = nlohmann::ordered_json::array();
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		for (const DROID *d : gameWorld.objects.droids[p])
		{
			if (d->sMove.Status != MOVEWAITROUTE)
			{
				continue;
			}
			FPathPendingResult r;
			if (!fpathTakePendingResult(d->id, r))
			{
				// Invariant violation (MOVEWAITROUTE always has a pending request).
				// Skip - the restore-side fallback (resubmitUncoveredRoutes) will re-derive if no result is recorded for it.
				debug(LOG_WARNING, "MOVEWAITROUTE droid %u has no pending path result to serialize", d->id);
				continue;
			}
			nlohmann::ordered_json e = nlohmann::ordered_json::object();
			e["id"] = d->id;
			e["retval"] = static_cast<int>(r.retval);
			e["dest"] = writeVector2i(r.destination);
			e["origDest"] = writeVector2i(r.originalDest);
			nlohmann::ordered_json path = nlohmann::ordered_json::array();
			for (const Vector2i &pt : r.path)
			{
				path.push_back(writeVector2i(pt));
			}
			e["path"] = std::move(path);
			routes.push_back(std::move(e));
		}
	}
	j["routes"] = std::move(routes);
	return j;
}

// Restore the serialized in-flight path results into the fpath result table, and return the set of droid
// ids covered (so the caller can re-derive any MOVEWAITROUTE droid that was NOT covered - i.e. the
// invariant-violation skip above).
static std::unordered_set<uint32_t> readPendingRoutes(const nlohmann::ordered_json &j, uint32_t version)
{
	std::unordered_set<uint32_t> restored;
	if (version != PENDING_ROUTES_VERSION)
	{
		throw StateError("unsupported pendingRoutes section version");
	}
	const nlohmann::ordered_json &routes = j.at("routes");
	if (!routes.is_array())
	{
		throw StateError("pendingRoutes.routes must be an array");
	}
	for (const nlohmann::ordered_json &e : routes)
	{
		FPathPendingResult r;
		r.retval = static_cast<FPATH_RETVAL>(e.at("retval").get<int>());
		r.destination = readVector2i(e.at("dest"));
		r.originalDest = readVector2i(e.at("origDest"));
		const nlohmann::ordered_json &path = e.at("path");
		r.path.reserve(path.size());
		for (const nlohmann::ordered_json &pt : path)
		{
			r.path.push_back(readVector2i(pt));
		}
		const uint32_t id = e.at("id").get<uint32_t>();
		fpathSetPendingResult(id, r);
		restored.insert(id);
	}
	return restored;
}

// Re-derive any active-world MOVEWAITROUTE droid not covered by a serialized result. Normally
// a no-op for a snapshot (every MOVEWAITROUTE droid should be in pendingRoutes) - used for the
// invariant-violation skip.
static void resubmitUncoveredRoutes(const std::unordered_set<uint32_t> &covered)
{
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		for (DROID *d : gameWorld.objects.droids[p])
		{
			if (d->sMove.Status == MOVEWAITROUTE && covered.find(d->id) == covered.end())
			{
				const Vector2i dest = d->sMove.destination;
				d->sMove.Status = MOVEINACTIVE;
				moveDroidTo(d, dest.x, dest.y);
			}
		}
	}
}

// MARK: - Section: Projectiles
//
// In-flight projectiles are global (one active world). They reference source/dest/damaged
// objects, so they are restored AFTER all world objects exist (resolved via getBaseObjFromData).
// proj_AddActiveProjectile appends, so saved order is reproduced by appending in order.

constexpr uint32_t PROJECTILES_SECTION_VERSION = 1;

static nlohmann::ordered_json writeRotation(const Rotation &r)
{
	return nlohmann::ordered_json::array({ r.direction, r.pitch, r.roll });
}

static Rotation readRotation(const nlohmann::ordered_json &j)
{
	reqArray(j, 3);
	return Rotation(j[0].get<uint16_t>(), j[1].get<uint16_t>(), j[2].get<uint16_t>());
}

static nlohmann::ordered_json writeProjectile(const PROJECTILE *p)
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["id"] = p->id;
	j["player"] = p->player;
	j["pos"] = writeVector3i(p->pos);
	j["rot"] = writeRotation(p->rot);
	j["born"] = p->born;
	j["died"] = p->died;
	j["time"] = p->time;
	j["state"] = p->state;
	j["bVisible"] = p->bVisible;
	if (p->psWStats != nullptr)
	{
		j["weaponId"] = p->psWStats->id.toUtf8();
	}
	j["source"] = writeObjRef(p->psSource);
	j["dest"] = writeObjRef(p->psDest);
	nlohmann::ordered_json damaged = nlohmann::ordered_json::array();
	for (const BASE_OBJECT *o : p->psDamaged)
	{
		damaged.push_back(writeObjRef(o));
	}
	j["damaged"] = std::move(damaged);
	j["src"] = writeVector3i(p->src);
	j["dst"] = writeVector3i(p->dst);
	j["vXY"] = p->vXY;
	j["vZ"] = p->vZ;
	nlohmann::ordered_json pst = nlohmann::ordered_json::object();
	pst["time"] = p->prevSpacetime.time;
	pst["pos"] = writeVector3i(p->prevSpacetime.pos);
	pst["rot"] = writeRotation(p->prevSpacetime.rot);
	j["prevSpacetime"] = std::move(pst);
	j["expectedDamageCaused"] = p->expectedDamageCaused;
	j["partVisible"] = p->partVisible;
	// Selects the impact damage type (DAM_PENETRATE_IMPACT vs DAM_IMPACT), which drives attacker experience gain.
	// Set unconditionally on the live-fire path but skipped by proj_AllocForRestore.
	j["penetrating"] = p->penetratingProjectile;
	return j;
}

static void readProjectile(const nlohmann::ordered_json &j)
{
	PROJECTILE *p = proj_AllocForRestore(j.at("id").get<uint32_t>(), reqPlayer(j.at("player")));
	p->pos = readVector3i(j.at("pos"));
	p->rot = readRotation(j.at("rot"));
	p->born = j.at("born").get<uint32_t>();
	p->died = j.at("died").get<uint32_t>();
	p->time = j.at("time").get<uint32_t>();
	p->state = j.at("state").get<uint8_t>();
	p->bVisible = j.at("bVisible").get<uint8_t>();
	p->psWStats = nullptr;
	if (j.contains("weaponId"))
	{
		const int widx = getCompFromID(COMP_WEAPON, WzString::fromUtf8(j.at("weaponId").get<std::string>()));
		if (widx >= 0)
		{
			p->psWStats = &asWeaponStats[widx];
		}
	}
	p->psSource = readObjRef(j.at("source"));
	p->psDest = readObjRef(j.at("dest"));
	p->psDamaged.clear();
	for (const nlohmann::ordered_json &jd : j.at("damaged"))
	{
		BASE_OBJECT *o = readObjRef(jd);
		if (o != nullptr)
		{
			p->psDamaged.push_back(o);
		}
	}
	p->src = readVector3i(j.at("src"));
	p->dst = readVector3i(j.at("dst"));
	p->vXY = j.at("vXY").get<int32_t>();
	p->vZ = j.at("vZ").get<int32_t>();
	const nlohmann::ordered_json &pst = j.at("prevSpacetime");
	p->prevSpacetime.time = pst.at("time").get<uint32_t>();
	p->prevSpacetime.pos = readVector3i(pst.at("pos"));
	p->prevSpacetime.rot = readRotation(pst.at("rot"));
	p->expectedDamageCaused = j.at("expectedDamageCaused").get<uint32_t>();
	p->partVisible = j.at("partVisible").get<int>();
	p->penetratingProjectile = j.at("penetrating").get<bool>();
	// Rebuild the target's incoming-damage accumulator (DROID::expectedDamage{Direct,Indirect} /
	// STRUCTURE::expectedDamage) from this projectile. The engine maintains it via
	// aiObjectAddExpectedDamage as projectiles launch/retarget/impact - freshly reconstructed targets
	// start at 0. An impacted projectile has expectedDamageCaused == 0 (zeroed on impact), so re-adding
	// for every restored projectile reproduces the live accumulator exactly: in-flight projectiles
	// contribute their value, impacted ones add nothing. Without this the restored in-flight
	// projectiles later subtract on impact and drive the accumulator negative (asserting in ai.cpp).
	if (p->psWStats != nullptr && p->expectedDamageCaused != 0)
	{
		aiObjectAddExpectedDamage(p->psDest, static_cast<SDWORD>(p->expectedDamageCaused), proj_Direct(p->psWStats));
	}
	proj_AddActiveProjectile(p);
}

static nlohmann::ordered_json writeProjectiles()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = PROJECTILES_SECTION_VERSION;
	nlohmann::ordered_json list = nlohmann::ordered_json::array();
	for (PROJECTILE *p = proj_GetFirst(); p != nullptr; p = proj_GetNext())
	{
		list.push_back(writeProjectile(p));
	}
	j["list"] = std::move(list);
	return j;
}

static void readProjectiles(const nlohmann::ordered_json &j, uint32_t version)
{
	if (version != PROJECTILES_SECTION_VERSION)
	{
		throw StateError("unsupported projectiles section version");
	}
	proj_FreeAllProjectiles();
	const nlohmann::ordered_json &list = j.at("list");
	if (!list.is_array())
	{
		throw StateError("projectiles.list must be an array");
	}
	for (const nlohmann::ordered_json &jp : list)
	{
		readProjectile(jp);
	}
}

// MARK: - Section: Spotters

constexpr uint32_t SPOTTERS_SECTION_VERSION = 1;

// Script-created timed spotters (addSpotter). They reveal objects around a point each visibility tick
// (setSeenBy -> visible[]), which gates deterministic target acquisition, so they are authoritative
// sim state. Watched against the active gameWorld map. Scope-independent (all players' spotters).
static nlohmann::ordered_json writeSpotters()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = SPOTTERS_SECTION_VERSION;
	nlohmann::ordered_json list = nlohmann::ordered_json::array();
	for (const SpotterSaveData &s : spotterEnumerateForSave())
	{
		nlohmann::ordered_json js = nlohmann::ordered_json::object();
		js["id"] = s.id;
		js["x"] = s.x;
		js["y"] = s.y;
		js["player"] = s.player;
		js["radius"] = s.sensorRadius;
		js["type"] = s.sensorType;
		js["expiry"] = s.expiryTime;
		list.push_back(std::move(js));
	}
	j["list"] = std::move(list);
	return j;
}

static void readSpotters(const nlohmann::ordered_json &j, uint32_t version)
{
	if (version != SPOTTERS_SECTION_VERSION)
	{
		throw StateError("unsupported spotters section version");
	}
	// The pre-restore spotters were already torn down at the top of gameStateFromJson (removeSpotters).
	const nlohmann::ordered_json &list = j.at("list");
	if (!list.is_array())
	{
		throw StateError("spotters.list must be an array");
	}
	for (const nlohmann::ordered_json &js : list)
	{
		SpotterSaveData d;
		d.id = js.at("id").get<uint32_t>();
		d.x = js.at("x").get<int>();
		d.y = js.at("y").get<int>();
		d.player = reqPlayer(js.at("player"));
		d.sensorRadius = js.at("radius").get<int>();
		d.sensorType = js.at("type").get<int>();
		d.expiryTime = js.at("expiry").get<uint32_t>();
		spotterRestore(gameWorld.map, d);
	}
}

// MARK: - Section: mission (off-world / campaign)
//
// The MISSION bookkeeping struct + statics, plus the off-world world's objects (mission.gameWorld).
// In skirmish mission.gameWorld is empty, so this is a no-op there.

constexpr uint32_t MISSION_SECTION_VERSION = 1;

// Static terrain for a world (used for both the main gameWorld and mission.gameWorld). Captures the
// game-authoritative terrain (geometry, per-tile texture/height/water, scroll limits, gateways) so a
// receiver without that map loaded can reconstruct it (derived state is rebuilt, not stored). If the
// world has no map loaded (i.e. mission.gameWorld in skirmish/MP, or the headless self-test) the block
// is empty and restore is a no-op. Display-only state (ground/tileset/lightmap) is regenerated by the
// normal map-load path when the world is actually entered & rendered.
// primaryMap: the active gameWorld map, whose tileset + terrain-type table (engine globals, not
// per-tile) are stored so the terrain is self-describing. The off-world mission map passes false - there
// is only one currentMapTileset / terrainTypes global, owned by the active map.
static nlohmann::ordered_json writeMapTerrain(const WorldMapState &map, bool primaryMap)
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	if (!map.tiles)
	{
		return j; // no off-world map loaded (the common MP/skirmish case)
	}
	const size_t n = static_cast<size_t>(map.width) * static_cast<size_t>(map.height);
	j["width"] = map.width;
	j["height"] = map.height;
	j["scroll"] = nlohmann::ordered_json::array({ map.scroll.minX, map.scroll.minY, map.scroll.maxX, map.scroll.maxY });

	// Per-tile static terrain as base64 LE blobs (texture u16, height i32, waterLevel i32).
	std::vector<uint8_t> tex; tex.reserve(n * 2);
	std::vector<uint8_t> hgt; hgt.reserve(n * 4);
	std::vector<uint8_t> water; water.reserve(n * 4);
	for (size_t i = 0; i < n; ++i)
	{
		appendU16le(tex, map.tiles[i].texture);
		appendU32le(hgt, static_cast<uint32_t>(map.tiles[i].height));
		appendU32le(water, static_cast<uint32_t>(map.tiles[i].waterLevel));
	}
	j["texture"] = base64Encode(tex);
	j["tileHeight"] = base64Encode(hgt); // distinct from the scalar geometry "height" above
	j["water"] = base64Encode(water);

	nlohmann::ordered_json gws = nlohmann::ordered_json::array();
	for (const GATEWAY *gw : map.gateways)
	{
		gws.push_back(nlohmann::ordered_json::array({ gw->x1, gw->y1, gw->x2, gw->y2 }));
	}
	j["gateways"] = std::move(gws);

	if (primaryMap)
	{
		// Tileset id + the terrain-type table (tile texture -> movement TER_ type). terrainTypes drives
		// pathfinding, so it is sim-authoritative. Storing both lets a restore reproduce the map's terrain
		// data without re-deriving it from the installed map file.
		j["tileset"] = static_cast<int>(currentMapTileset);
		std::vector<uint8_t> tt(terrainTypes, terrainTypes + MAX_TILE_TEXTURES);
		j["terrainTypes"] = base64Encode(tt);
	}
	return j;
}

static void readMapTerrain(WorldMapState &map, const nlohmann::ordered_json &j, bool primaryMap)
{
	if (!j.contains("width"))
	{
		return; // no terrain serialized for this world (i.e. off-world in skirmish/MP, or no map loaded)
	}
	const int w = j.at("width").get<int>();
	const int h = j.at("height").get<int>();
	if (w <= 0 || h <= 0 || w > MAP_MAXWIDTH || h > MAP_MAXHEIGHT)
	{
		throw StateError("map terrain dimensions out of range");
	}
	const size_t n = static_cast<size_t>(w) * static_cast<size_t>(h);
	const std::vector<uint8_t> tex = decodeBase64Field(j.at("texture"), n * 2, "map terrain texture");
	const std::vector<uint8_t> hgt = decodeBase64Field(j.at("tileHeight"), n * 4, "map terrain tileHeight");
	const std::vector<uint8_t> water = decodeBase64Field(j.at("water"), n * 4, "map terrain water");

	// The serialized terrain is AUTHORITATIVE - always overwrite. A receiver that already has a map
	// loaded (a cold-load that just loaded the level's base map, or the in-process round-trip) might
	// hold a different terrain (random/edited map), so we never trust the loaded tiles. When the
	// dimensions already match we overwrite the static tile fields in place (preserving the tile
	// array, since objects + the dynamic fog/fire overlay are restored onto it afterwards) - otherwise
	// we (re)allocate. Either way, the aux/blocking maps + continents are rebuilt below from the
	// overwritten terrain. Dynamic per-tile state (fog/fire/explored/sensors) is applied later by
	// readMapDynamic and is untouched here.
	if (!map.tiles || map.width != w || map.height != h)
	{
		map.tiles = std::make_unique<MAPTILE[]>(n);
		map.width = w;
		map.height = h;
	}
	for (size_t i = 0; i < n; ++i)
	{
		const uint16_t texture = readU16le(&tex[i * 2]);
		// terrainType() reads terrainTypes[TileNumber_tile(texture)], and that table has MAX_TILE_TEXTURES
		// entries, so reject a tile whose texture number would index past it (TILE_NUMMASK allows a larger
		// range than the table).
		if (TileNumber_tile(texture) >= MAX_TILE_TEXTURES)
		{
			throw StateError("map terrain tile texture number out of range");
		}
		map.tiles[i].texture = texture;
		map.tiles[i].height = static_cast<int32_t>(readU32le(&hgt[i * 4]));
		map.tiles[i].waterLevel = static_cast<int32_t>(readU32le(&water[i * 4]));
	}

	if (primaryMap)
	{
		// Restore the sim-authoritative terrain-type table (overrides whatever the installed map load set,
		// which matters if that map was updated or regenerated differently) and record the tileset id.
		currentMapTileset = static_cast<MAP_TILESET>(reqRange(j.at("tileset").get<int>(), 0, 2));
		const std::vector<uint8_t> tt = decodeBase64Field(j.at("terrainTypes"), MAX_TILE_TEXTURES, "map terrain terrainTypes");
		for (uint8_t t : tt)
		{
			if (t > TER_MAX)
			{
				throw StateError("map terrain terrainTypes value out of range");
			}
		}
		std::copy(tt.begin(), tt.end(), terrainTypes);
	}

	const nlohmann::ordered_json &sc = reqArray(j.at("scroll"), 4);
	map.scroll.minX = sc[0].get<int32_t>();
	map.scroll.minY = sc[1].get<int32_t>();
	map.scroll.maxX = sc[2].get<int32_t>();
	map.scroll.maxY = sc[3].get<int32_t>();

	// Gateways must be added before the aux/blocking setup (matches the map-load order).
	if (!map.gateways.empty())
	{
		gwShutDown(map);
	}
	for (const nlohmann::ordered_json &g : j.at("gateways"))
	{
		// Restore verbatim, not via gwNewGateway: its smallest-first reorder + edge clamp are not
		// idempotent on already-stored gateways (a 1-tile edge gateway is stored inverted), so re-adding
		// would shift a coordinate and break the round-trip.
		reqArray(g, 4);
		gwRestoreGateway(map, g[0].get<int>(), g[1].get<int>(), g[2].get<int>(), g[3].get<int>());
	}

	if (!mapReinitGameStateAfterTerrainRestore(map))
	{
		throw StateError("map terrain restore failed (invalid dimensions)");
	}
}

// Re-stamp the authoritative saved tile heights over the map. Object reconstruction perturbs the
// terrain: a structure's build path runs alignStructure() -> buildFlatten(), which re-flattens the
// foundation tiles to foundationHeight()'s (min+max)/2 average. That average is integer-truncating
// and so *not* idempotent against already-flattened terrain (the saved terrain is already flattened),
// drifting foundation tiles by +-1. The serialized terrain is authoritative and each object restores
// its own pos.z explicitly, so the build path's re-flatten is pure noise, thus: undo it by re-applying
// saved heights once all objects are placed. Heights only - texture/water are untouched by building,
// and the aux/blocking maps + continents were already computed (in readMapTerrain) from these exact
// heights, so no reinit is needed.
static void restampTerrainHeights(WorldMapState &map, const nlohmann::ordered_json &j)
{
	if (!map.tiles || !j.contains("tileHeight"))
	{
		return;
	}
	const nlohmann::ordered_json &hgt = j.at("tileHeight");
	const size_t n = static_cast<size_t>(map.width) * static_cast<size_t>(map.height);
	if (hgt.size() != n)
	{
		return;
	}
	for (size_t i = 0; i < n; ++i)
	{
		map.tiles[i].height = hgt[i].get<int32_t>();
	}
}

static nlohmann::ordered_json writeMission()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = MISSION_SECTION_VERSION;
	j["type"] = static_cast<int>(mission.type);
	j["startTime"] = mission.startTime;
	j["time"] = mission.time;
	j["ETA"] = mission.ETA;
	j["cheatTime"] = mission.cheatTime;
	j["homeLZ_X"] = mission.homeLZ_X;
	j["homeLZ_Y"] = mission.homeLZ_Y;
	j["playerX"] = mission.playerX;
	j["playerY"] = mission.playerY;

	nlohmann::ordered_json power = nlohmann::ordered_json::array();
	nlohmann::ordered_json transp = nlohmann::ordered_json::array();
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		power.push_back(mission.asCurrentPower[p]);
		nlohmann::ordered_json t = nlohmann::ordered_json::object();
		t["entryX"] = mission.iTranspEntryTileX[p];
		t["entryY"] = mission.iTranspEntryTileY[p];
		t["exitX"] = mission.iTranspExitTileX[p];
		t["exitY"] = mission.iTranspExitTileY[p];
		transp.push_back(std::move(t));
	}
	j["asCurrentPower"] = std::move(power);
	j["transporter"] = std::move(transp);

	// Statics (via accessors where the storage is file-local)
	j["offWorldKeepLists"] = offWorldKeepLists;
	j["missionResUp"] = MissionResUp;
	j["droidsToSafety"] = getDroidsToSafetyFlag();
	j["reinforcementTime"] = missionGetReinforcementTime();
	j["playCountDown"] = getPlayCountDown();
	// Mission-countdown bitfield: which timer-warning audio cues have played + the ACTIVATED bit.
	// setMissionCountDown() can only recompute the time-derived bits, so round-trip the raw value.
	j["missionCountDown"] = getMissionCountDown();

	nlohmann::ordered_json zones = nlohmann::ordered_json::array();
	for (int i = 0; i < MAX_NOGO_AREAS; ++i)
	{
		const LANDING_ZONE *z = getLandingZone(i);
		zones.push_back(nlohmann::ordered_json::array({ z->x1, z->y1, z->x2, z->y2 }));
	}
	j["landingZones"] = std::move(zones);

	j["mapTerrain"] = writeMapTerrain(mission.gameWorld.map, false);
	j["world"] = writeWorldObjects(mission.gameWorld, true);
	// Limbo droids: held between campaign limbo-expand missions (off the active map).
	j["limboDroids"] = writeDroidList(apsLimboDroids, true);
	return j;
}

static void readMission(const nlohmann::ordered_json &j, uint32_t version)
{
	if (version != MISSION_SECTION_VERSION)
	{
		throw StateError("unsupported mission section version");
	}
	mission.type = static_cast<LEVEL_TYPE>(reqRange(j.at("type").get<int>(),
		static_cast<int>(LEVEL_TYPE::LDS_COMPLETE), static_cast<int>(LEVEL_TYPE_MAX)));
	mission.startTime = j.at("startTime").get<uint32_t>();
	mission.time = j.at("time").get<int32_t>();
	mission.ETA = j.at("ETA").get<int32_t>();
	mission.cheatTime = j.at("cheatTime").get<uint32_t>();
	mission.homeLZ_X = j.at("homeLZ_X").get<uint16_t>();
	mission.homeLZ_Y = j.at("homeLZ_Y").get<uint16_t>();
	mission.playerX = j.at("playerX").get<int32_t>();
	mission.playerY = j.at("playerY").get<int32_t>();

	const nlohmann::ordered_json &power = reqArray(j.at("asCurrentPower"), MAX_PLAYERS);
	const nlohmann::ordered_json &transp = reqArray(j.at("transporter"), MAX_PLAYERS);
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		mission.asCurrentPower[p] = power[p].get<int32_t>();
		mission.iTranspEntryTileX[p] = transp[p].at("entryX").get<uint16_t>();
		mission.iTranspEntryTileY[p] = transp[p].at("entryY").get<uint16_t>();
		mission.iTranspExitTileX[p] = transp[p].at("exitX").get<uint16_t>();
		mission.iTranspExitTileY[p] = transp[p].at("exitY").get<uint16_t>();
	}

	offWorldKeepLists = j.at("offWorldKeepLists").get<bool>();
	MissionResUp = j.at("missionResUp").get<bool>();
	setDroidsToSafetyFlag(j.at("droidsToSafety").get<bool>());
	missionSetReinforcementTime(j.at("reinforcementTime").get<uint32_t>());
	setPlayCountDown(static_cast<UBYTE>(j.at("playCountDown").get<bool>() ? 1 : 0));
	setMissionCountDownValue(j.value("missionCountDown", static_cast<UBYTE>(0)));

	const nlohmann::ordered_json &zones = j.at("landingZones");
	for (int i = 0; i < MAX_NOGO_AREAS && static_cast<size_t>(i) < zones.size(); ++i)
	{
		const nlohmann::ordered_json &z = reqArray(zones[i], 4);
		setNoGoArea(z[0].get<uint8_t>(), z[1].get<uint8_t>(),
		            z[2].get<uint8_t>(), z[3].get<uint8_t>(), static_cast<UBYTE>(i));
	}

	// Restore off-world terrain (if any) before the objects, so the map exists for object placement
	// and the dynamic fog/fire overlay (applied inside readWorldObjects).
	if (j.contains("mapTerrain"))
	{
		readMapTerrain(mission.gameWorld.map, j.at("mapTerrain"), false);
	}

	const nlohmann::ordered_json &jworld = j.at("world");
	readWorldObjects(mission.gameWorld, jworld, jworld.value("version", 0u));

	// Undo the foundation-flattening the object build path applied to the off-world terrain
	// (see restampTerrainHeights); the saved terrain is authoritative.
	if (j.contains("mapTerrain"))
	{
		restampTerrainHeights(mission.gameWorld.map, j.at("mapTerrain"));
	}

	// Limbo droids: free existing then reconstruct into apsLimboDroids. These are off the
	// active map (onMission), so the active gameWorld supplies only the build context.
	freeAllLimboDroids();
	if (j.contains("limboDroids"))
	{
		readDroidList(gameWorld, apsLimboDroids, j.at("limboDroids"));
	}
}

// MARK: - Section: scripting (core/rules script state)
//
// In MP only the host runs AI bots - this captures the script-engine state (globals, groups,
// timers) of whatever scripts are loaded plus the tutorial flag. Restored after objects exist
// (groups/timers reference object ids). The research event queue is intentionally omitted:
// it is only populated before scripts are ready (savegame-load replay) and is empty mid-game.

constexpr uint32_t SCRIPTING_SECTION_VERSION = 1;

static nlohmann::ordered_json writeScripting(ScriptScope scriptScope)
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = SCRIPTING_SECTION_VERSION;
	j["inTutorial"] = bInTutorial;
	if (scriptsAreReady())
	{
		nlohmann::ordered_json states;
		// AllInstances (disk savegame / round-trip): serialize every script - the rules/global script
		// AND every AI bot - so a machine that runs all AI locally can reproduce each instance's VM state.
		// LocalPlayerOnly (snapshot to another client, which lacks the host's AI instances):
		// serialize just the host's local rules/global script (selectedPlayer).
		const int onlyPlayer = (scriptScope == ScriptScope::LocalPlayerOnly) ? static_cast<int>(selectedPlayer) : -1;
		saveScriptStates(states, onlyPlayer);
		j["states"] = std::move(states);
	}
	return j;
}

// requireScriptsReady gates the state restore on scriptsAreReady(). The general restore (gameStateFromJson)
// keeps it true: during a disk cold load it runs mid-way through the world reconstruct, before the scripts
// are instantiated, so scripting must be skipped there and replayed later. The explicit deferred replay
// (applyGameStateScripting) passes false: its caller guarantees the script instances exist (all level data
// is loaded) even though scriptsReady is not set yet, mirroring the legacy loadScriptState timing.
static void readScripting(const nlohmann::ordered_json &j, uint32_t version, ScriptScope scriptScope, bool requireScriptsReady = true)
{
	if (version != SCRIPTING_SECTION_VERSION)
	{
		throw StateError("unsupported scripting section version");
	}
	bInTutorial = j.value("inTutorial", false);
	if (j.contains("states") && (!requireScriptsReady || scriptsAreReady()))
	{
		// AllInstances: restore every script verbatim per saved player (targetPlayer = -1) - rules + every AI bot.
		// LocalPlayerOnly: rebind the single saved rules/global script onto this selectedPlayer.
		// Either way, loadScriptStates clears each restored instance's existing timers/groups first,
		// so saved state replaces rather than duplicates them.
		const int targetPlayer = (scriptScope == ScriptScope::LocalPlayerOnly) ? static_cast<int>(selectedPlayer) : -1;
		// A false return means the restore completed but skipped one or more malformed entries: the state is
		// self-consistent, just not byte-faithful to the document. The disk savegame / in-process round-trip
		// (AllInstances) accept the repaired result and proceed. LocalPlayerOnly instead rejects it -
		// throwing here aborts the whole snapshot apply.
		if (!loadScriptStates(j.at("states"), targetPlayer))
		{
			debug(LOG_ERROR, "Script state restore skipped malformed entries (targetPlayer %d)", targetPlayer);
			if (scriptScope == ScriptScope::LocalPlayerOnly)
			{
				throw StateError("script state restore skipped malformed entries - rejecting snapshot");
			}
		}
	}
}

void applyGameStateScripting(const nlohmann::ordered_json &gameStateDoc, ScriptScope scriptScope)
{
	if (!gameStateDoc.is_object() || !gameStateDoc.contains("scripting"))
	{
		return;
	}
	const nlohmann::ordered_json &sec = gameStateDoc.at("scripting");
	readScripting(sec, sec.value("version", 0u), scriptScope, /*requireScriptsReady=*/false);
}

// MARK: - Section: win/lose flags, mission stats, score stats
//
// End-screen / scoreboard bookkeeping that the simulation does not read back, so its absence would not
// cause a desync - this is a savegame-completeness section, not a determinism one. (The sim-critical
// recycled-experience queue lives in its own "recycledExperience" section below.)

constexpr uint32_t SCORES_SECTION_VERSION = 1;

static nlohmann::ordered_json writeScores()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = SCORES_SECTION_VERSION;
	j["playerHasWon"] = testPlayerHasWon();
	j["playerHasLost"] = testPlayerHasLost();

	// Global campaign mission tally.
	nlohmann::ordered_json md = nlohmann::ordered_json::object();
	md["unitsBuilt"] = missionData.unitsBuilt;
	md["unitsKilled"] = missionData.unitsKilled;
	md["unitsLost"] = missionData.unitsLost;
	md["strBuilt"] = missionData.strBuilt;
	md["strKilled"] = missionData.strKilled;
	md["strLost"] = missionData.strLost;
	md["artefactsFound"] = missionData.artefactsFound;
	md["missionStarted"] = missionData.missionStarted;
	md["missionEnded"] = missionData.missionEnded;
	md["shotsOnTarget"] = missionData.shotsOnTarget;
	md["shotsOffTarget"] = missionData.shotsOffTarget;
	md["babasMowedDown"] = missionData.babasMowedDown;
	j["missionData"] = std::move(md);

	// Per-player in-match ("recent*") score stats only. Career totals (played/wins/losses/total*) and
	// the identity key are profile data and are left untouched on restore.
	nlohmann::ordered_json stats = nlohmann::ordered_json::array();
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		const PLAYERSTATS &s = getMultiStats(p);
		nlohmann::ordered_json o = nlohmann::ordered_json::object();
		o["recentKills"] = s.recentKills;
		o["recentDroidsKilled"] = s.recentDroidsKilled;
		o["recentDroidsLost"] = s.recentDroidsLost;
		o["recentDroidsBuilt"] = s.recentDroidsBuilt;
		o["recentStructuresKilled"] = s.recentStructuresKilled;
		o["recentStructuresLost"] = s.recentStructuresLost;
		o["recentStructuresBuilt"] = s.recentStructuresBuilt;
		o["recentScore"] = s.recentScore;
		o["recentResearchComplete"] = s.recentResearchComplete;
		o["recentPowerLost"] = s.recentPowerLost;
		o["recentDroidPowerLost"] = s.recentDroidPowerLost;
		o["recentStructurePowerLost"] = s.recentStructurePowerLost;
		o["recentPowerWon"] = s.recentPowerWon;
		o["recentResearchPotential"] = s.recentResearchPotential;
		o["recentResearchPerformance"] = s.recentResearchPerformance;
		stats.push_back(std::move(o));
	}
	j["playerStats"] = std::move(stats);
	return j;
}

static void readScores(const nlohmann::ordered_json &j, uint32_t version, ScriptScope scriptScope)
{
	if (version != SCORES_SECTION_VERSION)
	{
		throw StateError("unsupported scores section version");
	}

	// Win/lose are selectedPlayer-local flow flags. Apply them for a full restore (disk savegame /
	// round-trip). For a host snapshot onto a joining client (LocalPlayerOnly) the host's flags are
	// not the joiner's, so leave the joiner's own.
	if (scriptScope == ScriptScope::AllInstances)
	{
		setPlayerHasWon(j.value("playerHasWon", false));
		setPlayerHasLost(j.value("playerHasLost", false));
	}

	if (j.contains("missionData"))
	{
		const nlohmann::ordered_json &md = j.at("missionData");
		missionData.unitsBuilt = md.value("unitsBuilt", 0u);
		missionData.unitsKilled = md.value("unitsKilled", 0u);
		missionData.unitsLost = md.value("unitsLost", 0u);
		missionData.strBuilt = md.value("strBuilt", 0u);
		missionData.strKilled = md.value("strKilled", 0u);
		missionData.strLost = md.value("strLost", 0u);
		missionData.artefactsFound = md.value("artefactsFound", 0u);
		missionData.missionStarted = md.value("missionStarted", 0u);
		missionData.missionEnded = md.value("missionEnded", 0u);
		missionData.shotsOnTarget = md.value("shotsOnTarget", 0u);
		missionData.shotsOffTarget = md.value("shotsOffTarget", 0u);
		missionData.babasMowedDown = md.value("babasMowedDown", 0u);
	}

	if (j.contains("playerStats"))
	{
		const nlohmann::ordered_json &stats = j.at("playerStats");
		for (unsigned p = 0; p < MAX_PLAYERS && p < stats.size(); ++p)
		{
			const nlohmann::ordered_json &o = stats[p];
			PLAYERSTATS s = getMultiStats(p); // copy: preserves identity + career totals
			s.recentKills = o.value("recentKills", 0u);
			s.recentDroidsKilled = o.value("recentDroidsKilled", 0u);
			s.recentDroidsLost = o.value("recentDroidsLost", 0u);
			s.recentDroidsBuilt = o.value("recentDroidsBuilt", 0u);
			s.recentStructuresKilled = o.value("recentStructuresKilled", 0u);
			s.recentStructuresLost = o.value("recentStructuresLost", 0u);
			s.recentStructuresBuilt = o.value("recentStructuresBuilt", 0u);
			s.recentScore = o.value("recentScore", 0u);
			s.recentResearchComplete = o.value("recentResearchComplete", 0u);
			s.recentPowerLost = o.value("recentPowerLost", static_cast<uint64_t>(0));
			s.recentDroidPowerLost = o.value("recentDroidPowerLost", static_cast<uint64_t>(0));
			s.recentStructurePowerLost = o.value("recentStructurePowerLost", static_cast<uint64_t>(0));
			s.recentPowerWon = o.value("recentPowerWon", static_cast<uint64_t>(0));
			s.recentResearchPotential = o.value("recentResearchPotential", static_cast<uint64_t>(0));
			s.recentResearchPerformance = o.value("recentResearchPerformance", static_cast<uint64_t>(0));
			setMultiStats(p, std::move(s), true); // bLocal = true: no network send
		}
	}
}

// MARK: - Section: recycled-droid experience (deterministic)
//
// recycled_experience[] banks the experience of recycled units. reallyBuildDroid hands the top value to
// each newly built droid, and experience feeds combat - so unlike the scores section this IS part of the
// deterministic sim and must round-trip exactly.
//
// Restore ordering is critical: the queues are CLEARED (clearAllRecycledExperience) BEFORE any
// world/mission object is restored, and POPULATED only AFTER all object restoration. reallyBuildDroid
// pops a banked value for each non-construct/non-transporter unit it builds unless gameTimeIsStopped(),
// and the GameState reconstruction does NOT stop the clock - so a populated queue would be consumed by
// the very droids being restored (each such droid's experience is then overwritten from the snapshot,
// but the queue would be left wrongly depleted). Clearing first guarantees restored units take nothing,
// and populating last reproduces the snapshot's queues exactly.

constexpr uint32_t RECYCLED_XP_SECTION_VERSION = 1;

static nlohmann::ordered_json writeRecycledExperience()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = RECYCLED_XP_SECTION_VERSION;
	// Drain a copy of each per-player max-heap - pop order is descending and deterministic, so the
	// round-trip is byte-stable (restore re-heapifies on push).
	nlohmann::ordered_json arr = nlohmann::ordered_json::array();
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		nlohmann::ordered_json q = nlohmann::ordered_json::array();
		std::priority_queue<int> copy = copy_experience_queue(static_cast<int>(p));
		while (!copy.empty())
		{
			q.push_back(copy.top());
			copy.pop();
		}
		arr.push_back(std::move(q));
	}
	j["queues"] = std::move(arr);
	return j;
}

static void clearAllRecycledExperience()
{
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		clear_experience_queue(static_cast<int>(p));
	}
}

static void readRecycledExperience(const nlohmann::ordered_json &j, uint32_t version)
{
	if (version != RECYCLED_XP_SECTION_VERSION)
	{
		throw StateError("unsupported recycledExperience section version");
	}
	if (!j.contains("queues"))
	{
		return;
	}
	const nlohmann::ordered_json &arr = j.at("queues");
	for (unsigned p = 0; p < MAX_PLAYERS && p < arr.size(); ++p)
	{
		clear_experience_queue(static_cast<int>(p)); // already cleared pre-world; kept idempotent
		for (const auto &v : arr[p])
		{
			add_to_experience_queue(static_cast<int>(p), v.get<int>());
		}
	}
}

// MARK: - Section: command-group fire-support designators
//
// apsCmdDesignator[player] is the commander droid currently designated for fire support. It steers AI
// target selection (ai.cpp), projectile homing (projectile.cpp) and command-droid orders (order.cpp),
// so it is deterministic sim state. Per-player commander droid id (0 = none). Restored after the world so
// the droid id resolves.

constexpr uint32_t DESIGNATORS_SECTION_VERSION = 1;

static nlohmann::ordered_json writeCommandDesignators()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = DESIGNATORS_SECTION_VERSION;
	nlohmann::ordered_json arr = nlohmann::ordered_json::array();
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		const DROID *d = cmdDroidGetDesignator(p);
		arr.push_back(d != nullptr ? d->id : 0u);
	}
	j["designators"] = std::move(arr);
	return j;
}

static void readCommandDesignators(const nlohmann::ordered_json &j, uint32_t version)
{
	if (version != DESIGNATORS_SECTION_VERSION)
	{
		throw StateError("unsupported commandDesignators section version");
	}
	const nlohmann::ordered_json &arr = j.at("designators");
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		cmdDroidClearDesignator(p); // clear first - drops any stale pointer left by clearWorldObjects
		if (p < arr.size())
		{
			const uint32_t id = arr[p].get<uint32_t>();
			if (id != 0)
			{
				BASE_OBJECT *obj = getBaseObjFromData(id, p, OBJ_DROID);
				if (obj != nullptr)
				{
					cmdDroidSetDesignator(static_cast<DROID *>(obj)); // no-op unless it's a command droid
				}
			}
		}
	}
}

// MARK: - Section: messages / intelligence (apsMessages + proximity displays)
//
// Intel-screen messages, research notifications, and proximity blips (including MP beacons). Not
// sim-feedback (so absent messages would not desync), but part of a faithful savegame - especially
// campaign, where the skipped legacy loadGame no longer restores them. VIEWDATA is rebuilt per level
// and keyed by name, so messages are re-linked by name (not pointer) - object proximity messages store
// an (id, player, type) object ref resolved after the world is restored. Mirrors the legacy
// writeMessageFile / loadSaveMessage logic.

constexpr uint32_t MESSAGES_SECTION_VERSION = 1;

static nlohmann::ordered_json writeMessages()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = MESSAGES_SECTION_VERSION;
	nlohmann::ordered_json arr = nlohmann::ordered_json::array();
	// Every player's messages, scope-independent: research/script/proximity messages are added in
	// the lockstep path on every machine (i.e. addMessage(MSG_RESEARCH,...) in research.cpp is not
	// selectedPlayer-gated), so the snapshotter's per-player lists are authoritative and can be
	// restored by saved player index with no remap. Known exception: human-targeted beacon blips
	// (NET_BEACONMSG is point-to-point, recvBeacon runs only on the addressee's machine).
	for (int player = 0; player < MAX_PLAYERS; ++player)
	{
		for (const MESSAGE *psMessage : apsMessages[player])
		{
			nlohmann::ordered_json m = nlohmann::ordered_json::object();
			m["player"] = player;
			m["type"] = static_cast<int>(psMessage->type);
			m["dataType"] = static_cast<int>(psMessage->dataType);
			m["read"] = psMessage->read;
			if (psMessage->type == MSG_PROXIMITY)
			{
				// The matching proximity display tells us POS_PROXDATA (view data / beacon) vs POS_PROXOBJ.
				auto it = std::find_if(apsProxDisp[player].begin(), apsProxDisp[player].end(), [psMessage](PROXIMITY_DISPLAY *psProx)
				{
					return psProx->psMessage == psMessage;
				});
				if (it == apsProxDisp[player].end())
				{
					continue; // orphaned proximity message (no display) - skip, as the legacy path asserts
				}
				if ((*it)->type == POS_PROXDATA)
				{
					m["proxData"] = true;
					const VIEWDATA *vd = psMessage->pViewData;
					m["name"] = vd != nullptr ? vd->name.toUtf8() : std::string("NULL");
					if (psMessage->dataType == MSG_DATA_BEACON)
					{
						const VIEW_PROXIMITY *vp = vd != nullptr ? static_cast<VIEW_PROXIMITY *>(vd->pData) : nullptr;
						if (vp != nullptr)
						{
							m["beaconX"] = vp->x;
							m["beaconY"] = vp->y;
							m["sender"] = vp->sender;
						}
					}
				}
				else
				{
					m["proxData"] = false;
					const BASE_OBJECT *psObj = psMessage->psObj;
					if (psObj != nullptr)
					{
						m["objId"] = psObj->id;
						m["objPlayer"] = psObj->player;
						m["objType"] = static_cast<int>(psObj->type);
					}
				}
			}
			else
			{
				const VIEWDATA *vd = psMessage->pViewData;
				m["name"] = vd != nullptr ? vd->name.toUtf8() : std::string("NULL");
			}
			arr.push_back(std::move(m));
		}
	}
	j["messages"] = std::move(arr);
	return j;
}

static void readMessages(const nlohmann::ordered_json &j, uint32_t version)
{
	if (version != MESSAGES_SECTION_VERSION)
	{
		throw StateError("unsupported messages section version");
	}
	// Clear any existing messages first so the saved set is authoritative and not duplicated.
	// Messages are re-added in saved (canonical) list order - addMessageToList keeps the type-sorted
	// ordering, so the list round-trips exactly.
	freeMessages();
	if (!j.contains("messages"))
	{
		return;
	}
	for (const auto &m : j.at("messages"))
	{
		const int player = m.value("player", 0);
		if (player < 0 || player >= MAX_PLAYERS)
		{
			continue;
		}
		const MESSAGE_TYPE type = static_cast<MESSAGE_TYPE>(reqRange(m.value("type", 0), 0, MSG_TYPES - 1));
		const int dataType = m.value("dataType", static_cast<int>(MSG_DATA_DEFAULT));
		const bool read = m.value("read", false);
		if (type == MSG_PROXIMITY)
		{
			const bool proxData = m.value("proxData", true);
			if (!proxData)
			{
				// Object proximity: resolve the object ref (objects already restored).
				MESSAGE *psMessage = addMessage(type, true, player);
				if (psMessage != nullptr)
				{
					psMessage->read = read;
					psMessage->psObj = getBaseObjFromData(m.value("objId", 0), m.value("objPlayer", 0), static_cast<OBJECT_TYPE>(m.value("objType", 0)));
					ASSERT(psMessage->psObj, "Proximity message references missing object id %d", m.value("objId", 0));
				}
			}
			else
			{
				MESSAGE *psMessage = addMessage(type, false, player);
				if (psMessage != nullptr)
				{
					psMessage->read = read;
					VIEWDATA *psViewData = nullptr;
					if (dataType == MSG_DATA_BEACON)
					{
						psMessage->dataType = MSG_DATA_BEACON; // addMessage()/createMessage() default to DEFAULT
						psViewData = CreateBeaconViewData(m.value("sender", 0), m.value("beaconX", 0u), m.value("beaconY", 0u));
					}
					else if (m.contains("name") && m.at("name").get<std::string>() != "NULL")
					{
						psViewData = getViewData(WzString::fromUtf8(m.at("name").get<std::string>()));
					}
					if (psViewData != nullptr)
					{
						psMessage->pViewData = psViewData;
						// Keep the beacon/proximity z at or above terrain height.
						VIEW_PROXIMITY *vp = dynamic_cast<VIEW_PROXIMITY *>(psViewData->pData);
						if (vp != nullptr)
						{
							const int terrainHeight = map_Height(gameWorld.map, vp->x, vp->y);
							if (static_cast<int>(vp->z) < terrainHeight)
							{
								vp->z = terrainHeight;
							}
						}
					}
					else
					{
						removeMessage(psMessage, player); // unresolved view data (mod/data mismatch) - drop
					}
				}
			}
		}
		else
		{
			MESSAGE *psMessage = addMessage(type, false, player);
			if (psMessage != nullptr)
			{
				psMessage->read = read;
				VIEWDATA *psViewData = (m.contains("name") && m.at("name").get<std::string>() != "NULL")
					? getViewData(WzString::fromUtf8(m.at("name").get<std::string>())) : nullptr;
				if (psViewData != nullptr)
				{
					psMessage->pViewData = psViewData;
				}
				else
				{
					removeMessage(psMessage, player); // unresolved view data - drop
				}
			}
		}
	}
	jsDebugMessageUpdate();
}

void applyGameStateMessages(const nlohmann::ordered_json &gameStateDoc)
{
	if (!gameStateDoc.is_object() || !gameStateDoc.contains("messages"))
	{
		return;
	}
	const nlohmann::ordered_json &sec = gameStateDoc.at("messages");
	readMessages(sec, sec.value("version", 0u));
}

// MARK: - Section: combat modifiers (per-player experience gain)
//
// experienceGain[player] (projectile.cpp, default 100) is a per-player percentage that scales the
// experience every unit earns from dealing damage (proj_UpdateExperience). It changes how fast droids
// rank up, and rank feeds accuracy/damage/regen - so this is part of the deterministic simulation.
// It is set only by the setExperienceModifier script API and is otherwise reset to 100 at level init -
// without this section a cold-load resume would silently revert every player to 100%, diverging combat
// outcomes. Global sim state, so written/read for all players regardless of ScriptScope.

constexpr uint32_t COMBAT_MODIFIERS_SECTION_VERSION = 1;

static nlohmann::ordered_json writeCombatModifiers()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = COMBAT_MODIFIERS_SECTION_VERSION;
	nlohmann::ordered_json exp = nlohmann::ordered_json::array();
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		exp.push_back(getExpGain(p));
	}
	j["experienceGain"] = std::move(exp);
	return j;
}

static void readCombatModifiers(const nlohmann::ordered_json &j, uint32_t version)
{
	if (version != COMBAT_MODIFIERS_SECTION_VERSION)
	{
		throw StateError("unsupported combatModifiers section version");
	}
	if (j.contains("experienceGain"))
	{
		const nlohmann::ordered_json &exp = j.at("experienceGain");
		for (unsigned p = 0; p < MAX_PLAYERS && p < exp.size(); ++p)
		{
			setExpGain(p, exp[p].get<int>());
		}
	}
}

// MARK: - Section: Miscellaneous synchronised sim globals with no larger home
//
//  - formationSpeedLimiting: per-player toggle (GAME_SYNC_OPT_CHANGE) gating formation movement speed
//  - transporterLaunchTime: launch time of an in-flight transporter, deciding its DACTION_TRANSPORTOUT
//    arrival tick (gameTime > launchTime + TRANSPORTOUT_TIME)
//  - transporterOnMission: transporter UI-context flag read by the synchronised embark/disembark/launch
//    paths (which droid world-list they touch). Off-world (reinforcement) mode is single-player campaign
//    only, so it is always false in multiplayer.

constexpr uint32_t SIM_MISC_SECTION_VERSION = 1;

static nlohmann::ordered_json writeSimMisc()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = SIM_MISC_SECTION_VERSION;
	nlohmann::ordered_json fsl = nlohmann::ordered_json::array();
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		fsl.push_back(moveFormationSpeedLimitingOn(p));
	}
	j["formationSpeedLimiting"] = std::move(fsl);
	j["transporterLaunchTime"] = transporterGetLaunchTime();
	j["transporterOnMission"] = transporterGetOnMission();
	return j;
}

static void readSimMisc(const nlohmann::ordered_json &j, uint32_t version)
{
	if (version != SIM_MISC_SECTION_VERSION)
	{
		throw StateError("unsupported simMisc section version");
	}
	const nlohmann::ordered_json &fsl = j.at("formationSpeedLimiting");
	if (!fsl.is_array() || fsl.size() != MAX_PLAYERS)
	{
		throw StateError("simMisc.formationSpeedLimiting size mismatch");
	}
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		moveRestoreFormationSpeedLimiting(p, fsl[p].get<bool>());
	}
	transporterSetLaunchTime(j.at("transporterLaunchTime").get<uint32_t>());
	// The off-world reinforcement transporter mode does not exist in multiplayer - enforce the invariant
	// on restore so a malformed or campaign-authored snapshot can never leave it set in an MP session.
	bool onMission = j.value("transporterOnMission", false);
	if (bMultiPlayer)
	{
		onMission = false;
	}
	transporterRestoreOnMission(onMission);
}

// MARK: - Section: scriptPlayerData
//
// NetPlay.scriptSetPlayerDataStrings: per-player key/value strings set by the setPlayerData() wzapi
// (currently only "usertype"), consumed by the game story log and the player UI. It is script-driven and
// identical on every client. That makes it shared game state rather than per-client presentation, so it
// belongs in the GameState document rather than the disk-only local view/meta state. It is not
// sim-authoritative and does not feed the determinism CRC.

constexpr uint32_t SCRIPT_PLAYER_DATA_SECTION_VERSION = 1;

static nlohmann::ordered_json writeScriptPlayerData()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = SCRIPT_PLAYER_DATA_SECTION_VERSION;
	// Always emit exactly MAX_PLAYERS entries (matching the read side's invariant) so the round-trip is
	// byte-stable. Each per-player store is a std::unordered_map, so emit its keys sorted. Iterating it
	// directly would reorder keys by hash between the live and restored maps.
	nlohmann::ordered_json arr = nlohmann::ordered_json::array();
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		nlohmann::ordered_json obj = nlohmann::ordered_json::object();
		if (p < NetPlay.scriptSetPlayerDataStrings.size())
		{
			const auto &m = NetPlay.scriptSetPlayerDataStrings[p];
			std::vector<std::string> keys;
			keys.reserve(m.size());
			for (const auto &kv : m) { keys.push_back(kv.first); }
			std::sort(keys.begin(), keys.end());
			for (const auto &k : keys) { obj[k] = m.at(k); }
		}
		arr.push_back(std::move(obj));
	}
	j["players"] = std::move(arr);
	return j;
}

static void readScriptPlayerData(const nlohmann::ordered_json &j, uint32_t version)
{
	if (version != SCRIPT_PLAYER_DATA_SECTION_VERSION)
	{
		throw StateError("unsupported scriptPlayerData section version");
	}
	const nlohmann::ordered_json &arr = j.at("players");
	if (!arr.is_array())
	{
		throw StateError("scriptPlayerData.players must be an array");
	}
	// Keep the MAX_PLAYERS invariant (setGameStoryLogPlayerDataValue indexes by player), tolerating an
	// over- or under-sized array from untrusted input.
	NetPlay.scriptSetPlayerDataStrings.assign(MAX_PLAYERS, {});
	for (size_t idx = 0; idx < arr.size() && idx < MAX_PLAYERS; ++idx)
	{
		NetPlay.scriptSetPlayerDataStrings[idx] = arr[idx].get<std::unordered_map<std::string, std::string>>();
	}
}

// MARK: - Section: campaign
//
// Campaign sim state that scripts mutate and deterministic code reads:
//  - campaignNumber / campaignName: set by the setCampaignNumber() wzapi. campaignNumber is read by
//    deterministic sim - notably the oil-drum pickup power (move.cpp), gated to single-player campaign,
//    where Beta/Gamma give extra power per drum - plus mission flow and the (presentation-only) radar.
//    campaignName is normally derived from the number (setCampaignNumber re-derives it), but is persisted
//    explicitly too since it can be set independently (setCampaignName) and is restored after the number
//    so the saved value wins. These gamestate values are authoritative, and applied after any savegame
//    setup header values.
//  - tweaks: camTweakOptions (campaigninfo.cpp) - campaign-config gameplay toggles chosen at campaign
//    start (fastExp, heavilyDamagedPenalty, ps1Modifiers, autosavesOnly, etc), cleared at level init, so
//    without this a cold-load would resume with every tweak reverted to default. Campaign-only (empty for
//    skirmish/MP), but simulation-related (they alter experience/damage/behaviour).
// Restored before the world/combat resumes. Campaign-only (empty/default in MP/skirmish), but campaignNumber
// feeds deterministic sim in single-player.

constexpr uint32_t CAMPAIGN_SECTION_VERSION = 1;

static nlohmann::ordered_json writeCampaign()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = CAMPAIGN_SECTION_VERSION;
	j["campaignNumber"] = getCampaignNumber();
	j["campaignName"] = getCampaignName();
	// getCamTweakOptions() is a std::unordered_map, whose iteration order is non-deterministic and, worse,
	// differs between the live map and the map rebuilt from a restored snapshot - so iterating it directly
	// yields an unstable key order that breaks the byte-exact round-trip. Emit the keys sorted for a stable,
	// reproducible order. (Per-element assignment also sidesteps the cross-basic_json mapped-type trait: the
	// map's mapped-type is a *different* basic_json specialization than the document, so a whole-map
	// conversion mis-dispatches - element assignment uses the well-defined implicit conversion.)
	const auto &camTweaks = getCamTweakOptions();
	std::vector<std::string> tweakKeys;
	tweakKeys.reserve(camTweaks.size());
	for (const auto &kv : camTweaks)
	{
		tweakKeys.push_back(kv.first);
	}
	std::sort(tweakKeys.begin(), tweakKeys.end());
	nlohmann::ordered_json tweaks = nlohmann::ordered_json::object();
	for (const std::string &key : tweakKeys)
	{
		tweaks[key] = camTweaks.at(key); // nlohmann::json value -> ordered_json element
	}
	j["tweaks"] = std::move(tweaks);
	return j;
}

static void readCampaign(const nlohmann::ordered_json &j, uint32_t version)
{
	if (version != CAMPAIGN_SECTION_VERSION)
	{
		throw StateError("unsupported campaign section version");
	}
	setCampaignNumber(j.value("campaignNumber", 0u));
	// setCampaignNumber() re-derives campaignName from the number - override with the explicitly saved name
	// (it can be set independently via setCampaignName), applied after the number so the saved value wins
	if (j.contains("campaignName") && j.at("campaignName").is_string())
	{
		setCampaignName(j.at("campaignName").get<std::string>());
	}
	if (j.contains("tweaks") && j.at("tweaks").is_object())
	{
		// Decode into the engine's nlohmann::json-keyed map element-by-element (see writeCampaign):
		// a templated .get<unordered_map<string, nlohmann::json>>() on the document's ordered_json
		// mis-detects the differently-specialized map as an array type and throws.
		std::unordered_map<std::string, nlohmann::json> opts;
		const nlohmann::ordered_json &tweaksObj = j.at("tweaks");
		for (auto it = tweaksObj.begin(); it != tweaksObj.end(); ++it)
		{
			opts.emplace(it.key(), it.value()); // ordered_json value -> nlohmann::json
		}
		setCamTweakOptions(std::move(opts));
	}
}

// MARK: - Section: presentation state (renderer / UI display globals)
//
// Cosmetic renderer + UI state that scripts set via setSunPosition / setSunIntensity / setFogColour /
// setWeather / setSky / setRevealStatus / setMiniMap / changePlayerColour (wzapi.cpp). These are pure
// display globals - none feed the sync-CRC / lockstep simulation - so this is a savegame-fidelity
// section, NOT a determinism one: it lives outside the CRC-covered state, and a missing or malformed
// block simply leaves the engine/tileset defaults (which the level-data load re-applies on cold-load)
// in place. Written/read unconditionally (independent of ScriptScope) and safe to apply on any client.
// playerColour is per-player identity but display-only.
//
// setSky carries a texture page name (display3d.cpp now stores it) - on restore the page is verified
// against the VFS first, and a missing texture (ex: from an unmounted mod) is logged and skipped -
// cosmetic loss, never a load failure.
//
// A mod that re-applies these in eventGameLoaded overrides this restore harmlessly (script wins,
// exactly as before) - the fidelity gain is for state changed mid-match and never re-applied on load.

constexpr uint32_t PRESENTATION_SECTION_VERSION = 1;

static nlohmann::ordered_json writeLightVec4(const glm::vec4 &v)
{
	return nlohmann::ordered_json::array({ v.x, v.y, v.z, v.w });
}

static nlohmann::ordered_json writePresentation()
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["version"] = PRESENTATION_SECTION_VERSION;

	const Vector3f sun = getTheSun();
	j["sun"] = nlohmann::ordered_json::array({ sun.x, sun.y, sun.z });

	j["ambient"] = writeLightVec4(pie_GetLighting0(LIGHT_AMBIENT));
	j["diffuse"] = writeLightVec4(pie_GetLighting0(LIGHT_DIFFUSE));
	j["specular"] = writeLightVec4(pie_GetLighting0(LIGHT_SPECULAR));

	j["fogColour"] = pie_GetFogColour().rgba();
	j["weather"] = static_cast<int>(atmosGetWeatherType());

	nlohmann::ordered_json sky = nlohmann::ordered_json::object();
	sky["page"] = getCurrentSkyboxPage();
	sky["windSpeed"] = getCurrentSkyboxWindSpeed();
	sky["scale"] = getCurrentSkyboxScale();
	j["skybox"] = std::move(sky);

	j["revealActive"] = getRevealStatus();
	j["radarPermitted"] = radarPermitted;

	nlohmann::ordered_json colours = nlohmann::ordered_json::array();
	for (unsigned p = 0; p < MAX_PLAYERS; ++p)
	{
		colours.push_back(getPlayerColour(p));
	}
	j["playerColour"] = std::move(colours);
	return j;
}

static void applyLightVec4(const nlohmann::ordered_json &j, const char *key, LIGHTING_TYPE entry)
{
	if (!j.contains(key))
	{
		return;
	}
	const nlohmann::ordered_json &a = j.at(key);
	if (!a.is_array() || a.size() < 4)
	{
		return;
	}
	float v[4] = { a[0].get<float>(), a[1].get<float>(), a[2].get<float>(), a[3].get<float>() };
	pie_Lighting0(entry, v);
}

static void readPresentation(const nlohmann::ordered_json &j, uint32_t version)
{
	if (version != PRESENTATION_SECTION_VERSION)
	{
		throw StateError("unsupported presentation section version");
	}
	if (j.contains("sun"))
	{
		const nlohmann::ordered_json &a = j.at("sun");
		if (a.is_array() && a.size() >= 3)
		{
			// setTheSun re-normalises, so feeding back getTheSun()'s scaled vector preserves direction
			setTheSun(Vector3f(a[0].get<float>(), a[1].get<float>(), a[2].get<float>()));
		}
	}
	applyLightVec4(j, "ambient", LIGHT_AMBIENT);
	applyLightVec4(j, "diffuse", LIGHT_DIFFUSE);
	applyLightVec4(j, "specular", LIGHT_SPECULAR);
	if (j.contains("fogColour"))
	{
		const uint32_t rgba = j.at("fogColour").get<uint32_t>();
		PIELIGHT c;
		c.byte.r = static_cast<uint8_t>(rgba & 0xff);
		c.byte.g = static_cast<uint8_t>((rgba >> 8) & 0xff);
		c.byte.b = static_cast<uint8_t>((rgba >> 16) & 0xff);
		c.byte.a = static_cast<uint8_t>((rgba >> 24) & 0xff);
		pie_SetFogColour(c);
	}
	if (j.contains("weather"))
	{
		const int wt = j.at("weather").get<int>();
		if (wt >= 0 && wt <= WT_NONE)
		{
			atmosSetWeatherType(static_cast<WT_CLASS>(wt));
		}
	}
	if (j.contains("skybox"))
	{
		const nlohmann::ordered_json &sky = j.at("skybox");
		if (sky.is_object() && sky.contains("page"))
		{
			const std::string page = sky.at("page").get<std::string>();
			const float wind = sky.value("windSpeed", 0.0f);
			const float scale = sky.value("scale", 10000.0f);
			// Presentation-only: the skybox texture may come from a mod that is no longer mounted, so
			// verify it exists in the VFS first. If it's missing, log and keep the current skybox rather
			// than loading a null texture - a cosmetic loss, never a load failure.
			if (page.empty())
			{
				// nothing to restore
			}
			else if (PHYSFS_exists(WzString::fromUtf8(page)))
			{
				setSkyBox(page.c_str(), wind, scale);
			}
			else
			{
				debug(LOG_WARNING, "presentation: skybox page '%s' not found in VFS - keeping current skybox", page.c_str());
			}
		}
	}
	if (j.contains("revealActive"))
	{
		setRevealStatus(j.at("revealActive").get<bool>());
	}
	if (j.contains("radarPermitted"))
	{
		radarPermitted = j.at("radarPermitted").get<bool>();
	}
	if (j.contains("playerColour"))
	{
		const nlohmann::ordered_json &colours = j.at("playerColour");
		if (colours.is_array())
		{
			for (unsigned p = 0; p < MAX_PLAYERS && p < colours.size(); ++p)
			{
				setPlayerColour(p, colours[p].get<UDWORD>());
			}
		}
	}
}

// MARK: - Top-level document

nlohmann::ordered_json gameStateToJson(ScriptScope scriptScope)
{
	nlohmann::ordered_json j = nlohmann::ordered_json::object();
	j["format"] = GAMESTATE_FORMAT_TAG;
	j["formatVersion"] = GAMESTATE_FORMAT_VERSION;
	// Main-world static terrain, serialized first (restored first too) so the snapshot is
	// self-contained: the loader builds the world on this terrain before anything references tiles,
	// with no separate map-file data. (Off-world terrain stays nested in the mission section.)
	j["mapTerrain"] = writeMapTerrain(gameWorld.map, true);
	j["determinismCore"] = writeDeterminismCore();
	j["diplomacy"] = writeDiplomacy();
	j["power"] = writePower();
	j["research"] = writeResearch();
	j["combatModifiers"] = writeCombatModifiers();
	j["simMisc"] = writeSimMisc();
	j["campaign"] = writeCampaign();
	j["availability"] = writeAvailability();
	j["limits"] = writeLimits();
	j["templates"] = writeTemplates();
	j["production"] = writeProduction();
	j["scores"] = writeScores();
	j["recycledExperience"] = writeRecycledExperience();
	j["formations"] = writeFormations();
	j["world"] = writeWorldObjects(gameWorld, false);
	j["dangerMaps"] = writeDangerMaps(gameWorld);
	j["pendingRoutes"] = writePendingRoutes();
	j["mission"] = writeMission();
	j["projectiles"] = writeProjectiles();
	j["spotters"] = writeSpotters();
	j["commandDesignators"] = writeCommandDesignators();
	j["messages"] = writeMessages();
	j["presentation"] = writePresentation();
	j["scriptPlayerData"] = writeScriptPlayerData();
	j["scripting"] = writeScripting(scriptScope);
	return j;
}

void gameStateFromJson(const nlohmann::ordered_json &j, ScriptScope scriptScope, bool deferMessages)
{
	if (!j.is_object())
	{
		throw StateError("GameState snapshot must be a JSON object");
	}
	if (j.value("format", std::string()) != GAMESTATE_FORMAT_TAG)
	{
		throw StateError("not a GameState snapshot (bad format tag)");
	}
	const uint32_t fmtVer = j.value("formatVersion", 0u);
	if (fmtVer != GAMESTATE_FORMAT_VERSION)
	{
		throw StateError("unsupported GameState container format version");
	}

	// Each subsystem is a top-level section object carrying its own "version".
	// Unknown sections are ignored.

	// Stop the SKIRMISH danger worker (if one is running) BEFORE the world is torn down and the aux/block
	// maps are reallocated by readMapTerrain - the worker writes auxMap[MAX_PLAYERS+AUX_DANGERMAP] /
	// reads blockMap[AUX_DANGERMAP], so reallocating those underneath it would race / use-after-free.
	// No-op on the disk cold-load path (mapInit has not started a worker yet) - the in-process round-trip
	// and in-place resume are the cases with a live worker here. The danger overlay is re-applied by the
	// dangerMaps post-pass below and the worker is (re)started by the next mapInit.
	mapStopDangerThreadForReconstruct();

	// Purge the pending destroyed-object list before teardown. Objects killed in the tick(s) before the
	// snapshot were moved off the world lists onto the global psDestroyedObj, so clearWorldObjects never
	// frees them. Left in place, the next objmemUpdate would fire triggerEventDestroyed / free them on the
	// resumed tick - the pre-restore timeline leaking into the restored one. (No-op when the list is empty:
	// cold-load starts fresh, the in-process round-trip usually has none pending.)
	mechanicsPurgeDestroyedObjects();

	// Tear down any live script spotters before the world/map is reset. They hold watchedTiles into the
	// active map - clearWorldObjects/readMapTerrain zero and reallocate the per-tile watcher/sensor counts,
	// so a surviving spotter's ~SPOTTER would later underflow those counts. The snapshot's own spotters are
	// rebuilt by the "spotters" section (readSpotters) after the world is restored.
	removeSpotters();

	// Main-world static terrain is restored FIRST: the world is built on it and every object /
	// fog-fire overlay references tiles. The serialized terrain is authoritative and always overwrites
	// whatever the receiver had loaded (in place when dimensions match, else reallocating), then the
	// game-authoritative map state (aux/blocking maps + continents) is rebuilt from it.
	// Empty/no-op when no map was serialized (headless self-test).
	if (j.contains("mapTerrain"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: mapTerrain (main-world terrain + gateways)");
		readMapTerrain(gameWorld.map, j.at("mapTerrain"), true);
	}

	// The determinism clock is applied next - its counters (object IDs + RNG) are applied
	// LAST, after reconstruction (which advances them) - see end of this function.
	if (j.contains("determinismCore"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: determinismCore (clock)");
		const nlohmann::ordered_json &sec = j.at("determinismCore");
		applyDeterminismClock(sec, sec.value("version", 0u));
	}
	if (j.contains("diplomacy"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: diplomacy");
		const nlohmann::ordered_json &sec = j.at("diplomacy");
		readDiplomacy(sec, sec.value("version", 0u));
	}
	if (j.contains("power"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: power");
		const nlohmann::ordered_json &sec = j.at("power");
		readPower(sec, sec.value("version", 0u));
	}
	if (j.contains("research"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: research");
		const nlohmann::ordered_json &sec = j.at("research");
		readResearch(sec, sec.value("version", 0u));
	}
	// Per-player experience-gain multiplier. Reset to 100 at level init, so restore it here (after that
	// reset, before/independent of object building - it only affects runtime combat XP, not construction).
	if (j.contains("combatModifiers"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: combatModifiers (experience gain)");
		const nlohmann::ordered_json &sec = j.at("combatModifiers");
		readCombatModifiers(sec, sec.value("version", 0u));
	}
	if (j.contains("simMisc"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: simMisc");
		const nlohmann::ordered_json &sec = j.at("simMisc");
		readSimMisc(sec, sec.value("version", 0u));
	}
	// Per-player script-set story-log/UI strings (setPlayerData wzapi). Not sim-authoritative. Cleared at
	// level init, so restore it here.
	if (j.contains("scriptPlayerData"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: scriptPlayerData");
		const nlohmann::ordered_json &sec = j.at("scriptPlayerData");
		readScriptPlayerData(sec, sec.value("version", 0u));
	}
	// Campaign sim state: campaignNumber + gameplay tweaks (fastExp/ps1Modifiers).
	// Cleared at level init - restore before combat.
	if (j.contains("campaign"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: campaign");
		const nlohmann::ordered_json &sec = j.at("campaign");
		readCampaign(sec, sec.value("version", 0u));
	}
	if (j.contains("availability"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: availability");
		const nlohmann::ordered_json &sec = j.at("availability");
		readAvailability(sec, sec.value("version", 0u));
	}
	if (j.contains("limits"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: limits");
		const nlohmann::ordered_json &sec = j.at("limits");
		readLimits(sec, sec.value("version", 0u));
	}
	// Templates must be restored before production (production references templates by id).
	if (j.contains("templates"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: templates");
		const nlohmann::ordered_json &sec = j.at("templates");
		readTemplates(sec, sec.value("version", 0u));
	}
	if (j.contains("production"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: production");
		const nlohmann::ordered_json &sec = j.at("production");
		readProduction(sec, sec.value("version", 0u));
	}
	// Movement formations, restored VERBATIM before the world sections so rebuilt droids re-attach
	// to them (slot state exact, no formationJoin). The old formations die naturally as
	// clearWorldObjects frees the droids referencing them (droid teardown calls formationLeave).
	g_restoredFormations.clear();
	if (j.contains("formations"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: formations (verbatim, pre-world)");
		const nlohmann::ordered_json &sec = j.at("formations");
		readFormations(sec, sec.value("version", 0u));
	}
	// Empty the recycled-experience queues BEFORE any droid is rebuilt, so restored units do not consume
	// banked experience (reallyBuildDroid pops from them while the clock runs). They are repopulated from
	// the snapshot after all object restoration (see readRecycledExperience).
	clearAllRecycledExperience();
	if (j.contains("world"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: world (active gameWorld objects)");
		const nlohmann::ordered_json &sec = j.at("world");
		readWorldObjects(gameWorld, sec, sec.value("version", 0u));
	}
	if (j.contains("mission"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: mission (off-world terrain + objects + limbo)");
		const nlohmann::ordered_json &sec = j.at("mission");
		readMission(sec, sec.value("version", 0u));
	}
	// factoryNumFlag is a single global reflecting the ACTIVE world's factory/repair assembly slots.
	// readWorldObjects rebuilds it per world (resetFactoryNumFlag clears ALL players then refills from the
	// one world it is passed), so the last pass - the mission world, empty in skirmish - would otherwise
	// leave it bound to the wrong (or empty) world. Rebind it to the active gameWorld now that both are
	// restored, else a factory built post-restore reuses a colliding factoryInc.
	resetFactoryNumFlag(gameWorld.objects);
	// All droid restoration (active world + off-world + limbo) is done: verify every restored
	// formation's members re-attached, fail-soft on violations, and drop the attach stash.
	debug(LOG_GAMESTATE_SERIAL, "post-pass: finalizeRestoredFormations");
	finalizeRestoredFormations();
	// Undo the foundation-flattening the object build path applied to the main-world terrain
	// (see restampTerrainHeights). Done after all world/mission object restoration so no later
	// structure build can re-perturb the authoritative saved terrain.
	if (j.contains("mapTerrain"))
	{
		debug(LOG_GAMESTATE_SERIAL, "post-pass: restampTerrainHeights (main world)");
		restampTerrainHeights(gameWorld.map, j.at("mapTerrain"));
	}
	// Re-apply the power request queues now that all structures exist: structure (re)building during
	// the world restore mutates the per-player queue (delPowerRequest/requestPowerFor), so it must be
	// stamped from the snapshot after reconstruction rather than inside readPower (see reapplyPowerQueue).
	if (j.contains("power"))
	{
		debug(LOG_GAMESTATE_SERIAL, "post-pass: reapplyPowerQueue");
		reapplyPowerQueue(j.at("power"));
	}
	// Restore the in-flight path results for droids reconstructed mid-route (MOVEWAITROUTE).
	// Replay the serialized PATHRESULT directly into the fpath result table so the droid consumes it on
	// its first resumed tick - exact and order-/context-independent (see writePendingRoutes). Runs
	// after world reconstruction (the droids must exist).
	// Any MOVEWAITROUTE droid not covered (the invariant-violation skip) falls back to re-derivation,
	// which needs the full map + aux maps + restored gameTime, all available here.
	debug(LOG_GAMESTATE_SERIAL, "post-pass: pendingRoutes (restore in-flight path results)");
	std::unordered_set<uint32_t> coveredRoutes;
	if (j.contains("pendingRoutes"))
	{
		const nlohmann::ordered_json &sec = j.at("pendingRoutes");
		coveredRoutes = readPendingRoutes(sec, sec.value("version", 0u));
	}
	resubmitUncoveredRoutes(coveredRoutes);
	// Projectiles reference world objects, so restore them after the world sections.
	if (j.contains("projectiles"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: projectiles");
		const nlohmann::ordered_json &sec = j.at("projectiles");
		readProjectiles(sec, sec.value("version", 0u));
	}
	// Spotters watch active-map tiles: restore them after the world is built (tile watcher/sensor counts
	// zeroed by teardown, re-added by object rebuild) so their contribution lands on the correct baseline,
	// and before the determinism counters are re-applied last (each spotter's ctor bumps the object-ID
	// counter, which the counters-last restore corrects).
	if (j.contains("spotters"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: spotters");
		const nlohmann::ordered_json &sec = j.at("spotters");
		readSpotters(sec, sec.value("version", 0u));
	}
	// Messages re-link to objects (proximity-object refs) and to VIEWDATA (by name). Objects exist by
	// now, but the level's message VIEWDATA is loaded from later-index data files that the disk cold-load
	// has not processed yet at this point - so that path defers messages (deferMessages) and replays them
	// via applyGameStateMessages() once level data has finished loading.
	if (!deferMessages && j.contains("messages"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: messages");
		const nlohmann::ordered_json &sec = j.at("messages");
		readMessages(sec, sec.value("version", 0u));
	}
	// Renderer-only environment (sun/lighting/fog/weather). No object or terrain dependency, restored
	// after the level-data load has already re-applied tileset defaults, so this overrides them.
	if (j.contains("presentation"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: presentation (sun/lighting/fog/weather)");
		const nlohmann::ordered_json &sec = j.at("presentation");
		readPresentation(sec, sec.value("version", 0u));
	}
	// Repopulate the recycled-experience queues now that ALL droid-building is done (active world,
	// off-world + limbo). Cleared earlier (pre-world) so the restored units took nothing from them.
	if (j.contains("recycledExperience"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: recycledExperience (repopulate)");
		const nlohmann::ordered_json &sec = j.at("recycledExperience");
		readRecycledExperience(sec, sec.value("version", 0u));
	}
	// Fire-support designators reference commander droids, so restore them after the world.
	if (j.contains("commandDesignators"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: commandDesignators");
		const nlohmann::ordered_json &sec = j.at("commandDesignators");
		readCommandDesignators(sec, sec.value("version", 0u));
	}
	// Win/lose flags + mission/score stats. Restored AFTER all object building: reallyBuildDroid /
	// buildStructure call scoreUpdateVar(WD_UNITS_BUILT / WD_STR_BUILT), bumping missionData as the world
	// is reconstructed, so the saved tally must overwrite those bumps (not be applied before them).
	if (j.contains("scores"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: scores");
		const nlohmann::ordered_json &sec = j.at("scores");
		readScores(sec, sec.value("version", 0u), scriptScope);
	}
	// Scripting state (groups/timers reference object ids) is restored after objects exist.
	if (j.contains("scripting"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: scripting");
		const nlohmann::ordered_json &sec = j.at("scripting");
		readScripting(sec, sec.value("version", 0u), scriptScope);
	}
	// Rebuild derived state (spatial grid, counts) for the active world, once.
	debug(LOG_GAMESTATE_SERIAL, "post-pass: rebuildDerivedState");
	rebuildDerivedState();

	// Restore the SKIRMISH danger overlay. Must run AFTER readMapTerrain (which reallocates+zeroes the
	// aux/block maps) and after the structures are placed (so the masked merge preserves their rebuilt
	// per-player blocking bits). The worker is still stopped here (see the teardown at the top); the next
	// mapInit restarts it snapshot-aware, preserving this content. No-op for campaign / when absent.
	if (j.contains("dangerMaps"))
	{
		debug(LOG_GAMESTATE_SERIAL, "post-pass: dangerMaps (SKIRMISH threat/danger overlay)");
		const nlohmann::ordered_json &sec = j.at("dangerMaps");
		readDangerMaps(gameWorld, sec, sec.value("version", 0u));
	}

	// Apply the determinism counters (object-ID allocators + RNG state) LAST: object
	// reconstruction advances synchObjID (module building) and may consume gameRand, so the
	// authoritative post-snapshot values must overwrite whatever reconstruction left behind.
	if (j.contains("determinismCore"))
	{
		debug(LOG_GAMESTATE_SERIAL, "restoring section: determinismCore (counters: objIDs + RNG)");
		const nlohmann::ordered_json &sec = j.at("determinismCore");
		applyDeterminismCounters(sec, sec.value("version", 0u));
	}
	debug(LOG_GAMESTATE_SERIAL, "gameStateFromJson complete");
}

std::string serializeGameState(ScriptScope scriptScope)
{
	// Compact dump - ordered_json keeps insertion order, and every writer inserts in a fixed order, so
	// output is deterministic/byte-stable (and a JS object's property order round-trips).
	return gameStateToJson(scriptScope).dump();
}

// Hard cap on JSON nesting depth accepted at the ingress parse. nlohmann's parser and DOM destructor are
// both iterative, so a deep document does not overflow while parsing or being freed - but the downstream
// JSON->JS-VM converter recurses one native frame per level, so bounding depth here (at the single choke
// point) covers the whole document, including nested sections, in one place.
static constexpr int GAMESTATE_MAX_JSON_DEPTH = 128;

nlohmann::ordered_json parseJsonBounded(const char *begin, const char *end)
{
	const auto depthGuard = [](int depth, nlohmann::ordered_json::parse_event_t event, nlohmann::ordered_json & /*parsed*/) -> bool
	{
		// depth == ref_stack.size() at each container start; throw (rather than return false, which would
		// silently discard the over-deep subtree) so the whole load aborts on excessive nesting.
		if ((event == nlohmann::ordered_json::parse_event_t::object_start
		     || event == nlohmann::ordered_json::parse_event_t::array_start)
		    && depth > GAMESTATE_MAX_JSON_DEPTH)
		{
			throw StateError("JSON nesting depth exceeds maximum allowed");
		}
		return true;
	};
	return nlohmann::ordered_json::parse(begin, end, depthGuard);
}

void deserializeGameState(const std::string &jsonText, ScriptScope scriptScope)
{
	nlohmann::ordered_json j;
	try
	{
		j = parseJsonBounded(jsonText.data(), jsonText.data() + jsonText.size());
	}
	catch (const nlohmann::ordered_json::exception &e)
	{
		throw StateError(std::string("failed to parse GameState JSON: ") + e.what());
	}

	try
	{
		gameStateFromJson(j, scriptScope);
	}
	catch (const StateError &)
	{
		throw;
	}
	catch (const nlohmann::ordered_json::exception &e)
	{
		throw StateError(std::string("invalid GameState JSON structure: ") + e.what());
	}
}

// MARK: - In-game write-path check (non-destructive)
//
// Serializes the current live match (real objects) and re-parses it, reporting size
// and object counts. Safe to run any time: it does NOT mutate the game. Validates that
// serializing real droids/structures/features neither crashes nor produces invalid JSON.
// (The full destructive round-trip / per-tick CRC harness needs all object types
// serialized first, otherwise restoring would drop the not-yet-serialized types.)
void runGameStateLiveWriteCheck()
{
	std::string msg;
	try
	{
		const std::string buf = serializeGameState();
		const nlohmann::ordered_json j = nlohmann::ordered_json::parse(buf);
		const size_t nf = j.at("world").at("features").size();
		const size_t ns = j.at("world").at("structures").size();
		const size_t nd = j.at("world").at("droids").size();
		char tmp[256];
		snprintf(tmp, sizeof(tmp), "GameState write OK: %zu bytes (%zu features, %zu structures, %zu droids)",
		         buf.size(), nf, ns, nd);
		msg = tmp;
	}
	catch (const std::exception &e)
	{
		msg = std::string("GameState write FAILED: ") + e.what();
	}
	CONPRINTF("%s", msg.c_str());
}

// MARK: - In-game round-trip harness (reconstruct fidelity on real data)
//
// Serialize the live match -> clear+reconstruct the whole world from it -> re-serialize,
// and require the two snapshots to be byte-identical. This exercises the full reconstruct
// path on real objects, catching missing/incorrect fields and reconstruction that perturbs
// the determinism core.
//
// DESTRUCTIVE (replaces live objects) - intended for headless autogame & other dev/testing.

static uint32_t g_roundTripTestTick = 0;

void gamestateSetRoundTripTestTick(uint32_t tick)
{
	g_roundTripTestTick = tick;
}

bool runGameStateRoundTripTest()
{
	std::string buf1, buf2;
	try
	{
		buf1 = serializeGameState();
		deserializeGameState(buf1);
		// Mirror ALL of what the real restore paths do after reconstruction (cold-load in init.cpp, etc):
		// discard the syncDebug accumulated while REBUILDING the world, re-seed the accumulator with the
		// CRC captured at save time (stashed by readDeterminismCore's setResumeSyncDebugCrc), and floor
		// the sync-CRC check at the resume tick.
		// Without the first two the round-trip would re-serialize the reconstruction-shifted CRC (a
		// harness-only artifact). Without the floor, the reset log would make the resume-tick GAME_GAME_TIME
		// checks spuriously flag a desync.
		resetSyncDebug();
		applyResumeSyncDebugCrc();
		setSyncCheckFloorTime(gameTime);
		buf2 = serializeGameState();
	}
	catch (const std::exception &e)
	{
		CONPRINTF("GameState round-trip FAILED (exception): %s", e.what());
		debug(LOG_ERROR, "GameState round-trip exception: %s", e.what());
		return false;
	}

	if (buf1 == buf2)
	{
		CONPRINTF("GameState round-trip OK @ gameTime %u (%zu bytes reconstruct identically)", gameTime, buf1.size());
		return true;
	}

	size_t i = 0;
	const size_t minLen = std::min(buf1.size(), buf2.size());
	while (i < minLen && buf1[i] == buf2[i]) { ++i; }

	// Identify the object at the mismatch: the nearest preceding "id":<n> in each buffer. If these
	// differ, the lists are out of order / different length (a reorder or add/drop); if they match,
	// it's a field of the same object. Differing buffer sizes => an object was added or dropped.
	auto precedingId = [](const std::string &buf, size_t pos) -> long {
		const std::string key = "\"id\":";
		const size_t p = buf.rfind(key, pos);
		return (p == std::string::npos) ? -1 : strtol(buf.c_str() + p + key.size(), nullptr, 10);
	};
	const long id1 = precedingId(buf1, i);
	const long id2 = precedingId(buf2, i);

	CONPRINTF("GameState round-trip MISMATCH at byte %zu (sizes %zu vs %zu; ids %ld vs %ld)", i, buf1.size(), buf2.size(), id1, id2);
	const size_t start = (i > 200) ? i - 200 : 0;
	debug(LOG_ERROR, "GameState round-trip mismatch near byte %zu (buf sizes %zu vs %zu; nearest preceding \"id\": expected %ld, got %ld):\n  expected: ...%s...\n  got:      ...%s...",
	      i, buf1.size(), buf2.size(), id1, id2, buf1.substr(start, 400).c_str(), buf2.substr(start, 400).c_str());
	return false;
}

// Called once per game tick - when the configured target tick is reached, runs the
// round-trip test and exits the process with a status code (for headless CI use).
void gamestateMaybeRunRoundTripTest()
{
	if (g_roundTripTestTick == 0 || gameTime < g_roundTripTestTick)
	{
		return;
	}
	g_roundTripTestTick = 0; // run exactly once
	const bool ok = runGameStateRoundTripTest();
	debug(LOG_INFO, "GameState round-trip test %s; exiting.", ok ? "PASSED" : "FAILED");
	wzQuit(ok ? EXIT_SUCCESS : EXIT_FAILURE);
}

// MARK: - Self-test (determinism harness scaffold)

bool runGameStateSelfTest()
{
	bool ok = true;
	const auto check = [&ok](bool cond, const char *msg)
	{
		if (!cond)
		{
			fprintf(stderr, "[gamestate-selftest] FAIL: %s\n", msg);
			ok = false;
		}
	};

	// Arrange: drive the determinism core into a known, non-default configuration.
	gameSRand(0xDEADBEEFu);
	for (int i = 0; i < 1000; ++i)
	{
		(void)gameRandU32(); // advance well past a generate() boundary (624 words)
	}
	setObjectIdState(ObjectIdState{ 0x11112222u, 0x33334444u });
	setGameTime(123456u);

	// Known diplomacy + power state (these sections are flat, so we can verify restore directly).
	alliances[0][1] = ALLIANCE_FORMED;
	alliancebits[0] = 0x2;
	satuplinkbits = 0x5;
	PlayerPowerState refPow{};
	refPow.currentPower = 123456789LL;
	refPow.maxStorage = 999999999LL;
	refPow.extractedPower = 42LL;
	refPow.wastedPower = 7LL;
	refPow.powerGeneratedLastUpdate = 13LL;
	refPow.powerModifier = 137;
	setPlayerPowerState(0, refPow);

	// Known limits (flat per-player caps).
	setMaxDroids(0, 4242);
	setMaxCommanders(0, 17);
	setMaxConstructors(0, 99);

	// Known production run with template-less entries (exercises the production section
	// without needing loaded stats; template resolution is covered by the CRC harness later).
	asProductionRun[0].clear();
	asProductionRun[0].resize(2);
	{
		ProductionRunEntry e;
		e.quantity = 5;
		e.built = 2;
		e.psTemplate = nullptr;
		asProductionRun[0][1].push_back(e);
	}

	// Capture the reference RNG state, and the exact sequence that must follow it.
	const GameRandomState refRng = getGameRandomState();
	constexpr int kDraws = 32;
	std::vector<uint32_t> refSeq;
	refSeq.reserve(kDraws);
	for (int i = 0; i < kDraws; ++i)
	{
		refSeq.push_back(gameRandU32());
	}

	// Re-establish the reference point and snapshot from there.
	setGameRandomState(refRng);
	const std::string buf1 = serializeGameState();

	// Act: perturb every piece of live core state, then restore from the snapshot.
	gameSRand(0x01234567u);
	for (int i = 0; i < 50; ++i)
	{
		(void)gameRandU32();
	}
	setObjectIdState(ObjectIdState{ 0u, 0u });
	setGameTime(999u);
	alliances[0][1] = ALLIANCE_BROKEN;
	alliancebits[0] = 0xFF;
	satuplinkbits = 0xAA;
	setPlayerPowerState(0, PlayerPowerState{});
	setMaxDroids(0, 1);
	setMaxCommanders(0, 1);
	setMaxConstructors(0, 1);
	asProductionRun[0].clear();

	try
	{
		deserializeGameState(buf1);
	}
	catch (const StateError &e)
	{
		check(false, e.what());
		return ok;
	}

	// Assert: scalar state restored exactly.
	check(gameTime == 123456u, "gameTime not restored");
	const ObjectIdState ids = getObjectIdState();
	check(ids.synchObjID == 0x11112222u, "synchObjID not restored");
	check(ids.unsynchObjID == 0x33334444u, "unsynchObjID not restored");
	check(gameRand_GetSeed() == 0xDEADBEEFu, "RNG seed not restored");

	// Assert: diplomacy restored.
	check(alliances[0][1] == ALLIANCE_FORMED, "alliances not restored");
	check(alliancebits[0] == 0x2, "allianceBits not restored");
	check(satuplinkbits == 0x5, "satUplinkBits not restored");

	// Assert: power restored.
	const PlayerPowerState gotPow = getPlayerPowerState(0);
	check(gotPow.currentPower == 123456789LL, "power.currentPower not restored");
	check(gotPow.maxStorage == 999999999LL, "power.maxStorage not restored");
	check(gotPow.extractedPower == 42LL, "power.extractedPower not restored");
	check(gotPow.wastedPower == 7LL, "power.wastedPower not restored");
	check(gotPow.powerGeneratedLastUpdate == 13LL, "power.powerGeneratedLastUpdate not restored");
	check(gotPow.powerModifier == 137, "power.powerModifier not restored");

	// Assert: limits restored.
	check(getMaxDroids(0) == 4242, "limits.maxDroids not restored");
	check(getMaxCommanders(0) == 17, "limits.maxCommanders not restored");
	check(getMaxConstructors(0) == 99, "limits.maxConstructors not restored");

	// Assert: production restored.
	if (asProductionRun[0].size() == 2 && asProductionRun[0][1].size() == 1)
	{
		check(asProductionRun[0][1][0].quantity == 5, "production quantity not restored");
		check(asProductionRun[0][1][0].built == 2, "production built not restored");
	}
	else
	{
		check(false, "production run structure not restored");
	}

	// Assert: the restored RNG produces the exact same sequence as the reference.
	bool seqMatch = true;
	for (int i = 0; i < kDraws; ++i)
	{
		if (gameRandU32() != refSeq[i])
		{
			seqMatch = false;
			break;
		}
	}
	check(seqMatch, "RNG sequence diverged after restore");

	// Assert: byte-stability. Same live state must serialize identically...
	setGameRandomState(refRng);
	const std::string buf2 = serializeGameState();
	check(buf1 == buf2, "byte-stability: re-serialized JSON differs");

	// ...and a deserialize -> serialize round-trip must reproduce the snapshot byte-for-byte.
	try
	{
		deserializeGameState(buf1);
	}
	catch (const StateError &e)
	{
		check(false, e.what());
		return ok;
	}
	const std::string buf3 = serializeGameState();
	check(buf1 == buf3, "round-trip: deserialize->serialize JSON differs");

	// --- Assert: map terrain write/read round-trip on a synthetic map ---
	// The rest of the self-test runs with no map loaded, so writeMapTerrain/readMapTerrain (and the
	// "height" geometry vs per-tile-array key separation) are otherwise never exercised headlessly.
	{
		WorldMapState tm;
		tm.tiles = std::make_unique<MAPTILE[]>(16);
		tm.width = 4;
		tm.height = 4;
		tm.scroll.minX = 0; tm.scroll.minY = 0; tm.scroll.maxX = 4; tm.scroll.maxY = 4;
		for (size_t i = 0; i < 16; ++i)
		{
			tm.tiles[i].texture = static_cast<uint16_t>(i + 1);
			tm.tiles[i].height = static_cast<int>(100 + i);
			tm.tiles[i].waterLevel = static_cast<int>(i);
		}
		// Exercise the primaryMap path (tileset + terrainTypes). Save/restore the globals so the headless
		// self-test stays side-effect-free.
		const MAP_TILESET savedTileset = currentMapTileset;
		UBYTE savedTtypes[MAX_TILE_TEXTURES];
		std::copy(std::begin(terrainTypes), std::end(terrainTypes), savedTtypes);
		currentMapTileset = MAP_TILESET::URBAN;
		terrainTypes[6] = 3;
		const nlohmann::ordered_json tj = writeMapTerrain(tm, true);
		// Geometry "height" must stay a scalar; the per-tile heights live under "tileHeight".
		check(tj.contains("height") && tj.at("height").is_number(), "mapTerrain geometry height must be a number");
		check(tj.contains("tileHeight") && tj.at("tileHeight").is_string(), "mapTerrain per-tile heights must be a base64 string");
		check(tj.value("height", -1) == 4, "mapTerrain geometry height value wrong");
		check(tj.contains("tileset") && tj.at("tileset").is_number(), "mapTerrain tileset must be a number");
		check(tj.contains("terrainTypes") && tj.at("terrainTypes").is_string(), "mapTerrain terrainTypes must be a base64 string");
		// Perturb the globals so the read has something to restore over.
		currentMapTileset = MAP_TILESET::ARIZONA;
		terrainTypes[6] = 0;
		WorldMapState tm2;
		try
		{
			readMapTerrain(tm2, tj, true);
			check(tm2.width == 4 && tm2.height == 4, "terrain round-trip dimensions wrong");
			check(tm2.tiles != nullptr, "terrain round-trip did not allocate tiles");
			if (tm2.tiles)
			{
				check(tm2.tiles[5].texture == 6, "terrain round-trip texture wrong");
				check(tm2.tiles[5].height == 105, "terrain round-trip height wrong");
				check(tm2.tiles[5].waterLevel == 5, "terrain round-trip water wrong");
			}
			check(currentMapTileset == MAP_TILESET::URBAN, "terrain round-trip tileset wrong");
			check(terrainTypes[6] == 3, "terrain round-trip terrainTypes wrong");
		}
		catch (const std::exception &e)
		{
			check(false, e.what());
		}
		currentMapTileset = savedTileset;
		std::copy(std::begin(savedTtypes), std::end(savedTtypes), terrainTypes);
	}

	if (ok)
	{
		fprintf(stderr, "[gamestate-selftest] PASS (%zu-byte JSON)\n", buf1.size());
	}
	return ok;
}

} // namespace gamestate
