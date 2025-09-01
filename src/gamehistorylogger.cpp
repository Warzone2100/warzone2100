/*
	This file is part of Warzone 2100.
	Copyright (C) 2023  Warzone 2100 Project

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

#include "gamehistorylogger.h"

#include "lib/framework/wzglobal.h" // required for config.h
#include "lib/framework/wzapp.h"
#include "lib/framework/physfs_ext.h"
#include "lib/framework/file.h"
#include "lib/netplay/netplay.h"
#include "objmem.h"
#include "loop.h"
#include "power.h"
#include "multiint.h"
#include "multistat.h"
#include "multilobbycommands.h"
#include "stdinreader.h"
#include "modding.h"
#include "version.h"
#include "mission.h"

#include <string>
#include <tuple>

constexpr size_t CurrentGameLogOutputJSONVersion = 11;

static uint32_t countAllStructures(uint32_t player)
{
	if (player >= MAX_PLAYERS)
	{
		return 0;
	}

	uint32_t total = 0;
	for (unsigned inc = 0; inc < numStructureStats; inc++)
	{
		total += asStructureStats[inc].curCount[player];
	}
	return total;
}

static std::tuple<uint32_t, uint32_t> getDroidHPPercentageAndExperience(uint32_t player)
{
	if (player >= MAX_PLAYERS)
	{
		return std::tuple<uint32_t, uint32_t>(0, 0);
	}

	uint64_t totalHP = 0;
	uint64_t totalExp = 0;
	uint64_t numDroids = 0;
	for (const DROID *psDroid : apsDroidLists[player])
	{
		if (psDroid->died)
		{
			continue;
		}
		if (psDroid->body > 0 && psDroid->originalBody > 0)
		{
			totalHP += static_cast<uint64_t>(100.0 / (double)psDroid->originalBody * (double)psDroid->body);
		}
		totalExp += static_cast<uint64_t>((double)psDroid->experience / 65536.0);
		numDroids++;
	}
	return std::tuple<uint32_t, uint32_t>((numDroids > 0) ? static_cast<uint32_t>(totalHP / numDroids) : 0, static_cast<uint32_t>(totalExp));
}

static uint32_t getNumOilRigs(uint32_t player)
{
	if (player >= MAX_PLAYERS)
	{
		return 0;
	}

	uint32_t result = 0;
	for (const STRUCTURE *psStruct : apsStructLists[player])
	{
		if (!psStruct->died
			&& (REF_RESOURCE_EXTRACTOR == psStruct->pStructureType->type))
		{
			result++;
		}
	}
	return result;
}

static nlohmann::json buildGameDetailsOutputJSON(std::chrono::system_clock::time_point gameStartRealTime)
{
	nlohmann::json gameObj = nlohmann::json::object();
	gameObj["version"] = version_getVersionString();
	gameObj["mapName"] = game.map;
	if (!game.hash.isZero())
	{
		gameObj["mapHash"] = game.hash.toString();
	}
	gameObj["baseType"] = game.base;
	gameObj["alliancesType"] = game.alliance;
	gameObj["powerType"] = game.power;
	gameObj["scavengers"] = game.scavengers;
	gameObj["multiTechLevel"] = game.techLevel;
	gameObj["startDate"] = std::chrono::duration_cast<std::chrono::milliseconds>(gameStartRealTime.time_since_epoch()).count();
	gameObj["idleTime"] = game.inactivityMinutes * 60 * 1000;
	gameObj["gameLimit"] = game.gameTimeLimitMinutes * 60 * 1000;
	gameObj["mods"] = getModList();
	//gameObj["modHashList"] = getModHashList();
	gameObj["maxPlayers"] = game.maxPlayers;
	return gameObj;
}

static nlohmann::json convertToOutputJSON(const std::vector<GameStoryLogger::ResearchEvent>& researchLog, const std::vector<GameStoryLogger::FixedPlayerAttributes>& fixedPlayerAttributes, GameStoryLogger::OutputKey outputKey)
{
	nlohmann::json result = nlohmann::json::array();

	for (const auto& event : researchLog)
	{
		auto j = nlohmann::json::object();
		j["name"] = event.researchId;
		j["struct"] = event.structureId;
		j["time"] = event.gameTime;
		switch (outputKey)
		{
			case GameStoryLogger::OutputKey::PlayerIndex:
				j["player"] = event.player;
				break;
			case GameStoryLogger::OutputKey::PlayerPosition:
				if (event.player < fixedPlayerAttributes.size())
				{
					const auto& f = fixedPlayerAttributes[event.player];
					j["position"] = f.position;
				}
				else
				{
					j["player"] = event.player;
				}
				break;
		}
		result.push_back(j);
	}

	return result;
}

enum class OutputKey
{
	PlayerIndex,
	PlayerPosition
};

const std::unordered_map<std::string, std::string> autohosterClassicNamingOverrides = {
	{"droidsLost", "droidLost"},
	{"structuresLost", "structureLost"},
	{"kills", "kills"},
	{"structureKills", "structureKill"},
	{"droidsBuilt", "droidBuilt"},
	{"structuresBuilt", "structBuilt"},
	{"droids", "droid"},
	{"structs", "struct"},
//	{"researchComplete", "researchComplete"},
//	{"power", "power"},
//	{"score", "score"},
//	{"hp", "hp"},
//	{"summExp", "summExp"},
//	{"oilRigs", "oilRigs"},
//	{"recentPowerLost", "recentPowerLost"},
//	{"recentPowerWon", "recentPowerWon"},
	{"recentResearchPotential", "labResearchPotential"},
	{"recentResearchPerformance", "labResearchPerformance"},
//	{"usertype", "usertype"}
};

static std::string mapPlayerDataOutputName(const std::string& inputName, GameStoryLogger::OutputNaming naming)
{
	switch (naming)
	{
		case GameStoryLogger::OutputNaming::Default:
			return inputName;
		case GameStoryLogger::OutputNaming::AutohosterClassic:
		{
			auto it = autohosterClassicNamingOverrides.find(inputName);
			if (it == autohosterClassicNamingOverrides.end())
			{
				return inputName;
			}
			return it->second;
		}
	}
	return inputName; // silence compiler warning
}

static std::string mapPlayerUserTypeOutputValue(const std::string& inputUserType, GameStoryLogger::OutputNaming naming)
{
	switch (naming)
	{
		case GameStoryLogger::OutputNaming::Default:
			return inputUserType;
		case GameStoryLogger::OutputNaming::AutohosterClassic:
		{
			if (inputUserType == "contender")
			{
				return "fighter";
			}
			return inputUserType;
		}
	}
	return inputUserType; // silence compiler warning
}

static nlohmann::json convertToOutputJSON(const GameStoryLogger::GameFrame& frame, const std::vector<GameStoryLogger::FixedPlayerAttributes>& fixedPlayerAttributes, GameStoryLogger::OutputKey outputKey, GameStoryLogger::OutputNaming naming)
{
	nlohmann::json result = nlohmann::json::array();

	for (size_t idx = 0; idx < frame.playerData.size(); idx++)
	{
		if (idx >= fixedPlayerAttributes.size())
		{
			break;
		}
		const auto& p = frame.playerData[idx];
		const auto& f = fixedPlayerAttributes[idx];
		nlohmann::json j = nlohmann::json::object();
		// fixed player data
		j["name"] = f.name;
		j["position"] = f.position;
		j["index"] = idx;
		j["team"] = f.team;
		j["colour"] = f.colour;
		j["faction"] = f.faction;
		j["publicKey"] = f.publicKey;
		// data from the frame
		j[mapPlayerDataOutputName("droidsLost", naming)] = p.droidsLost;
		j[mapPlayerDataOutputName("structuresLost", naming)] = p.structuresLost;
		j[mapPlayerDataOutputName("kills", naming)] = p.kills;
		j[mapPlayerDataOutputName("structureKills", naming)] = p.structureKills;
		j[mapPlayerDataOutputName("droidsBuilt", naming)] = p.droidsBuilt;
		j[mapPlayerDataOutputName("structuresBuilt", naming)] = p.structuresBuilt;
		j[mapPlayerDataOutputName("droids", naming)] = p.droids;
		j[mapPlayerDataOutputName("structs", naming)] = p.structs;
		j[mapPlayerDataOutputName("researchComplete", naming)] = p.researchComplete;
		j[mapPlayerDataOutputName("power", naming)] = p.power;
		j[mapPlayerDataOutputName("score", naming)] = p.score;
		j[mapPlayerDataOutputName("hp", naming)] = p.hp;
		j[mapPlayerDataOutputName("summExp", naming)] = p.summExp;
		j[mapPlayerDataOutputName("oilRigs", naming)] = p.oilRigs;
		j[mapPlayerDataOutputName("recentPowerLost", naming)] = p.recentPowerLost;
		j[mapPlayerDataOutputName("recentDroidPowerLost", naming)] = p.recentDroidPowerLost;
		j[mapPlayerDataOutputName("recentStructurePowerLost", naming)] = p.recentStructurePowerLost;
		j[mapPlayerDataOutputName("recentPowerWon", naming)] = p.recentPowerWon;
		j[mapPlayerDataOutputName("recentResearchPotential", naming)] = p.recentResearchPotential;
		j[mapPlayerDataOutputName("recentResearchPerformance", naming)] = p.recentResearchPerformance;
		j[mapPlayerDataOutputName("usertype", naming)] = mapPlayerUserTypeOutputValue(p.usertype, naming);

		if (p.playerLeftGameTime.has_value())
		{
			j["playerLeftGameTime"] = p.playerLeftGameTime.value();
		}

		size_t outputIndex = idx;
		switch (outputKey)
		{
			case GameStoryLogger::OutputKey::PlayerIndex:
				break;
			case GameStoryLogger::OutputKey::PlayerPosition:
				outputIndex = f.position;
				break;
		}
		result[outputIndex] = j;
	}

	return result;
}

GameStoryLogger& GameStoryLogger::instance()
{
	static GameStoryLogger _instance;
	return _instance;
}

GameStoryLogger::GameStoryLogger()
{
	frameLoggingInterval = 15 * GAME_TICKS_PER_SEC;
}

void GameStoryLogger::reset()
{
	lastRecordedGameFrameTime = 0;
	startingPlayerAttributes.clear();
	gameFrames.clear();
	gameFrames.reserve(128);
	researchLog.clear();
	gameStartRealTime = std::chrono::system_clock::time_point();
	gameEndRealTime = std::chrono::system_clock::time_point();
	cachedGameDetailsOutputJSON = nlohmann::json();
	if (fileHandle)
	{
		PHYSFS_close(fileHandle);
		fileHandle = nullptr;
	}
}

void GameStoryLogger::logStartGame()
{
	reset();

	if (!bMultiPlayer)
	{
		// skip for campaign, for now
		return;
	}

	gameStartRealTime = std::chrono::system_clock::now();

	if (outputModes.logFile)
	{
		WzString outputPath = WzString("logs/") + WzString::fromUtf8(getLogOutputFilename());
		fileHandle = PHYSFS_openWrite(outputPath.toUtf8().c_str());
		if (fileHandle)
		{
			WZ_PHYSFS_SETBUFFER(fileHandle, 4096)//;
		}
		else
		{
			debug(LOG_ERROR, "%s could not be opened: %s", outputPath.toUtf8().c_str(), WZ_PHYSFS_getLastError());
		}
	}

	for (int i = 0; i < game.maxPlayers; i++)
	{
		FixedPlayerAttributes playerAttrib;
		playerAttrib.name = (strlen(NetPlay.players[i].name) == 0) ? "" : getPlayerName(i);
		playerAttrib.position = NetPlay.players[i].position;
		playerAttrib.team = NetPlay.players[i].team;
		playerAttrib.colour = NetPlay.players[i].colour;
		playerAttrib.faction = NetPlay.players[i].faction;
		playerAttrib.publicKey = base64Encode(getOutputPlayerIdentity(i).toBytes(EcKey::Public));

		startingPlayerAttributes.push_back(playerAttrib);
	}
}

void GameStoryLogger::setFrameLoggingInterval(uint32_t seconds)
{
	frameLoggingInterval = seconds * GAME_TICKS_PER_SEC;
}

void GameStoryLogger::logGameFrame()
{
	if (!bMultiPlayer)
	{
		// skip for campaign, for now
		return;
	}

	// throttle recording of game frames
	// (if frameLoggingInterval == 0, ongoing frame logging is disabled - however, always output the first frame regardless!)
	if (lastRecordedGameFrameTime > 0 && ((frameLoggingInterval == 0) || (gameTime - lastRecordedGameFrameTime < frameLoggingInterval)))
	{
		return;
	}

	gameFrames.push_back(genCurrentFrame());
	lastRecordedGameFrameTime = gameTime;
	if (outputModes.anyEnabled() && NetPlay.players[selectedPlayer].isSpectator)
	{
		// output frame
		auto reportJSON = genFrameReport(gameFrames.back(), outputKey, outputNaming);
		std::string reportJSONStr = std::string("__REPORT__") + reportJSON.dump(-1, ' ', false, nlohmann::ordered_json::error_handler_t::replace) + "__ENDREPORT__";
		outputLine(std::move(reportJSONStr));
	}
}

void GameStoryLogger::logResearchCompleted(RESEARCH *psResearch, STRUCTURE *psStruct, int player)
{
	if (!bMultiPlayer)
	{
		// skip for campaign, for now
		return;
	}

	GameStoryLogger::ResearchEvent event;
	event.researchId = psResearch->id;
	event.structureId = (psStruct != nullptr) ? psStruct->id : 0;
	event.player = player;
	event.gameTime = gameTime;
	researchLog.push_back(event);
}

std::string GameStoryLogger::getLogOutputFilename() const
{
	return std::string("gamelog_") + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(gameStartRealTime.time_since_epoch()).count()) + ".log";
}

void GameStoryLogger::logDebugModeChange(bool enabled)
{
	DebugModeEvent event;
	event.entered = enabled;
	event.gameTime = gameTime;
	debugModeLog.push_back(event);

	if (outputModes.anyEnabled())
	{
		std::string reportJSONStr = std::string("__DEBUGMODE__") + ((enabled) ? "true" : "false") + "__ENDDEBUGMODE__";
		outputLine(std::move(reportJSONStr));
	}
}

void GameStoryLogger::logGameOver()
{
	if (!bMultiPlayer)
	{
		// skip for campaign, for now
		return;
	}

	gameEndRealTime = std::chrono::system_clock::now();

	gameFrames.push_back(genCurrentFrame());
	lastRecordedGameFrameTime = gameTime;

	if (outputModes.anyEnabled())
	{
		bool hitTimeout = (game.gameTimeLimitMinutes > 0) ? (gameTime >= (game.gameTimeLimitMinutes * 60 * 1000)) : false;
		auto reportJSON = genEndOfGameReport(outputKey, outputNaming, hitTimeout);
		std::string reportJSONStr = std::string("__REPORTextended__") + reportJSON.dump(-1, ' ', false, nlohmann::ordered_json::error_handler_t::replace) + "__ENDREPORTextended__";
		outputLine(std::move(reportJSONStr));
	}

	if (fileHandle)
	{
		PHYSFS_close(fileHandle);
		fileHandle = nullptr;
	}
}

void GameStoryLogger::setOutputKey(OutputKey key)
{
	outputKey = key;
}

void GameStoryLogger::setOutputNaming(OutputNaming naming)
{
	outputNaming = naming;
}

void GameStoryLogger::setOutputModes(OutputModes enabled)
{
	if (outputModes.logFile && !enabled.logFile && fileHandle != nullptr)
	{
		PHYSFS_close(fileHandle);
		fileHandle = nullptr;
	}
	outputModes = enabled;
}

void GameStoryLogger::configureOutput(OutputKey key, OutputNaming naming, OutputModes enabled)
{
	setOutputModes(enabled);
	setOutputKey(key);
	setOutputNaming(naming);
}

GameStoryLogger::GameFrame GameStoryLogger::genCurrentFrame() const
{
	GameFrame frame;
	for (int i = 0; i < game.maxPlayers; i++)
	{
		GameFrame::PlayerStats playerStats;
		const PLAYERSTATS& mStats = getMultiStats(i);

		playerStats.droidsLost = mStats.recentDroidsLost;
		playerStats.structuresLost = mStats.recentStructuresLost;
		playerStats.kills = mStats.recentDroidsKilled;
		playerStats.structureKills = mStats.recentStructuresKilled;
		playerStats.droidsBuilt = mStats.recentDroidsBuilt;
		playerStats.structuresBuilt = mStats.recentStructuresBuilt;
		playerStats.droids = getNumDroids(i);
		playerStats.structs = countAllStructures(i);
		playerStats.researchComplete = mStats.recentResearchComplete;
		playerStats.power = getPower(i);
		playerStats.score = static_cast<int32_t>(mStats.recentScore);
		auto hpAndSummExp = getDroidHPPercentageAndExperience(i);
		playerStats.hp = std::get<0>(hpAndSummExp);
		playerStats.summExp = std::get<1>(hpAndSummExp);
		playerStats.oilRigs = getNumOilRigs(i);
		playerStats.recentPowerLost = mStats.recentPowerLost;
		playerStats.recentDroidPowerLost = mStats.recentDroidPowerLost;
		playerStats.recentStructurePowerLost = mStats.recentStructurePowerLost;
		playerStats.recentPowerWon = mStats.recentPowerWon;
		playerStats.recentResearchPotential = mStats.recentResearchPotential;
		playerStats.recentResearchPerformance = mStats.recentResearchPerformance;

		if (i < NetPlay.scriptSetPlayerDataStrings.size())
		{
			const auto& scriptSetPlayerData = NetPlay.scriptSetPlayerDataStrings[i];
			auto it = scriptSetPlayerData.find("usertype");
			if (it != scriptSetPlayerData.end())
			{
				playerStats.usertype = it->second;
			}
		}

		if (i < ingame.playerLeftGameTime.size() && ingame.playerLeftGameTime[i].has_value())
		{
			playerStats.playerLeftGameTime = ingame.playerLeftGameTime[i];
		}

		frame.playerData.push_back(playerStats);
	}

	frame.currGameTime = gameTime;
	return frame;
}

nlohmann::json GameStoryLogger::genFrameReport(const GameFrame& frame, OutputKey key, OutputNaming naming)
{
	nlohmann::json report = nlohmann::json::object();

	report["JSONversion"] = CurrentGameLogOutputJSONVersion;
	report["gameTime"] = gameTime;
	report["playerData"] = convertToOutputJSON(frame, startingPlayerAttributes, key, naming);
	if (cachedGameDetailsOutputJSON.is_null())
	{
		cachedGameDetailsOutputJSON = buildGameDetailsOutputJSON(gameStartRealTime);
	}
	report["game"] = cachedGameDetailsOutputJSON;

	return report;
}

nlohmann::ordered_json GameStoryLogger::genEndOfGameReport(OutputKey key, OutputNaming naming, bool timeout) const
{
	nlohmann::ordered_json report = nlohmann::json::object();

	report["JSONversion"] = CurrentGameLogOutputJSONVersion;
	report["gameTime"] = gameTime;
	if (!gameFrames.empty())
	{
		report["playerData"] = convertToOutputJSON(gameFrames.back(), startingPlayerAttributes, key, naming);
	}
	report["researchComplete"] = convertToOutputJSON(researchLog, startingPlayerAttributes, key);
	report["game"] = buildGameDetailsOutputJSON(gameStartRealTime);
	report["game"]["timeGameEnd"] = gameTime;
	report["game"]["timeout"] = timeout;
	report["game"]["cheated"] = Cheated;
	report["endDate"] = std::chrono::duration_cast<std::chrono::milliseconds>(gameEndRealTime.time_since_epoch()).count();

	return report;
}

inline void to_json(nlohmann::json& j, const GameStoryLogger::FixedPlayerAttributes& p) {
	j = nlohmann::json::object();
	j["name"] = p.name;
	j["position"] = p.position;
	j["team"] = p.team;
	j["colour"] = p.colour;
	j["faction"] = p.faction;
	j["publicKey"] = p.publicKey;
}

inline void from_json(const nlohmann::json& j, GameStoryLogger::FixedPlayerAttributes& p) {
	p.name = j.at("name").get<std::string>();
	p.position = j.at("position").get<int32_t>();
	p.team = j.at("team").get<int32_t>();
	p.colour = j.at("colour").get<int32_t>();
	p.faction = static_cast<FactionID>(j.at("faction").get<uint8_t>());
	p.publicKey = j.at("publicKey").get<std::string>();
}

inline void to_json(nlohmann::json& j, const GameStoryLogger::GameFrame::PlayerStats& p) {
	j = nlohmann::json::object();
	j["droidsLost"] = p.droidsLost;
	j["structuresLost"] = p.structuresLost;
	j["kills"] = p.kills;
	j["structureKills"] = p.structureKills;
	j["droidsBuilt"] = p.droidsBuilt;
	j["structuresBuilt"] = p.structuresBuilt;
	j["droids"] = p.droids;
	j["structs"] = p.structs;
	j["researchComplete"] = p.researchComplete;
	j["power"] = p.power;
	j["score"] = p.score;
	j["hp"] = p.hp;
	j["summExp"] = p.summExp;
	j["oilRigs"] = p.oilRigs;
	j["recentPowerLost"] = p.recentPowerLost;
	j["recentDroidPowerLost"] = p.recentDroidPowerLost;
	j["recentStructurePowerLost"] = p.recentStructurePowerLost;
	j["recentPowerWon"] = p.recentPowerWon;
	j["recentResearchPotential"] = p.recentResearchPotential;
	j["recentResearchPerformance"] = p.recentResearchPerformance;
	j["usertype"] = p.usertype;
	if (p.playerLeftGameTime.has_value())
	{
		j["playerLeftGameTime"] = p.playerLeftGameTime.value();
	}
}

inline void from_json(const nlohmann::json& j, GameStoryLogger::GameFrame::PlayerStats& p) {
	p.droidsLost = j.at("droidsLost").get<uint32_t>();
	p.structuresLost = j.at("structuresLost").get<uint32_t>();
	p.kills = j.at("kills").get<uint32_t>();
	p.structureKills = j.at("structureKills").get<uint32_t>();
	p.droidsBuilt = j.at("droidsBuilt").get<uint32_t>();
	p.structuresBuilt = j.at("structuresBuilt").get<uint32_t>();
	p.droids = j.at("droids").get<uint32_t>();
	p.structs = j.at("structs").get<uint32_t>();
	p.researchComplete = j.at("researchComplete").get<uint32_t>();
	p.power = j.at("power").get<uint32_t>();
	p.score = j.at("score").get<int32_t>();
	p.hp = j.at("hp").get<uint32_t>();
	p.summExp = j.at("summExp").get<uint32_t>();
	p.oilRigs = j.at("oilRigs").get<uint32_t>();
	p.recentPowerLost = j.at("recentPowerLost").get<uint64_t>();
	p.recentDroidPowerLost = j.at("recentDroidPowerLost").get<uint64_t>();
	p.recentStructurePowerLost = j.at("recentStructurePowerLost").get<uint64_t>();
	p.recentPowerWon = j.at("recentPowerWon").get<uint64_t>();
	p.recentResearchPotential = j.at("recentResearchPotential").get<uint64_t>();
	p.recentResearchPerformance = j.at("recentResearchPerformance").get<uint64_t>();
	p.usertype = j.at("usertype").get<std::string>();

	p.playerLeftGameTime = nullopt;
	auto it = j.find("playerLeftGameTime");
	if (it != j.end())
	{
		p.playerLeftGameTime = it.value().get<uint32_t>();
	}
}

inline void to_json(nlohmann::json& j, const GameStoryLogger::GameFrame& p) {
	j = nlohmann::json::object();
	j["playerData"] = p.playerData;
	j["gameTime"] = p.currGameTime;
}

inline void from_json(const nlohmann::json& j, GameStoryLogger::GameFrame& p) {
	p.playerData = j.at("playerData").get<std::vector<GameStoryLogger::GameFrame::PlayerStats>>();
	p.currGameTime = j.at("gameTime").get<uint32_t>();
}

inline void to_json(nlohmann::json& j, const GameStoryLogger::ResearchEvent& p) {
	j = nlohmann::json::object();
	j["id"] = p.researchId;
	j["structureId"] = p.structureId;
	j["player"] = p.player;
	j["gameTime"] = p.gameTime;
}

inline void from_json(const nlohmann::json& j, GameStoryLogger::ResearchEvent& p) {
	p.researchId = j.at("id").get<WzString>();
	p.structureId = j.at("structureId").get<uint32_t>();
	p.player = j.at("player").get<int>();
	p.gameTime = j.at("gameTime").get<uint32_t>();
}

inline void to_json(nlohmann::json& j, const GameStoryLogger::DebugModeEvent& p) {
	j = nlohmann::json::object();
	j["entered"] = p.entered;
	j["gameTime"] = p.gameTime;
}

inline void from_json(const nlohmann::json& j, GameStoryLogger::DebugModeEvent& p) {
	p.entered = j.at("entered").get<bool>();
	p.gameTime = j.at("gameTime").get<uint32_t>();
}

void GameStoryLogger::outputLine(std::string &&line)
{
	line.append("\n");
	if (outputModes.cmdInterface)
	{
		wz_command_interface_output_str(line.c_str());
	}
	if (outputModes.logFile)
	{
		if (fileHandle)
		{
			if (WZ_PHYSFS_writeBytes(fileHandle, line.c_str(), line.size()) != line.size())
			{
				// Failed to write line to file
				debug(LOG_ERROR, "Could not write to output file; PHYSFS error: %s", WZ_PHYSFS_getLastError());
				PHYSFS_close(fileHandle);
				fileHandle = nullptr;
			}
			PHYSFS_flush(fileHandle);
		}
	}
}

void GameStoryLogger::saveToFile(const std::string& filename)
{
	// do not persist outputModes, outputKey, outputNaming - these are configured for each run
	nlohmann::json output = nlohmann::json::object();
	output["lastRecordedGameFrameTime"] = lastRecordedGameFrameTime;
	output["startingPlayerAttributes"] = startingPlayerAttributes;
	output["gameFrames"] = gameFrames;
	output["researchLog"] = researchLog;
	output["debugModeLog"] = debugModeLog;
	output["gameStartRealTime"] = std::chrono::duration_cast<std::chrono::milliseconds>(gameStartRealTime.time_since_epoch()).count();
	output["gameEndRealTime"] = std::chrono::duration_cast<std::chrono::milliseconds>(gameEndRealTime.time_since_epoch()).count();
}

void GameStoryLogger::loadFromFile(const std::string& filename)
{
	if (!PHYSFS_exists(filename.c_str()))
	{
		return;
	}

	reset();

	nlohmann::json obj = nlohmann::json::object();
	UDWORD size = 0;
	char *data = nullptr;
	if (loadFile(filename.c_str(), &data, &size))
	{
		try {
			obj = nlohmann::json::parse(data, data + size);
		}
		catch (const std::exception &e) {
			ASSERT(false, "JSON document from %s is invalid: %s", filename.c_str(), e.what());
		}
		catch (...) {
			debug(LOG_ERROR, "Unexpected exception parsing JSON %s", filename.c_str());
		}
		ASSERT(!obj.is_null(), "JSON document from %s is null", filename.c_str());
		ASSERT(obj.is_object(), "JSON document from %s is not an object. Read: \n%s", filename.c_str(), data);
		free(data);
	}
	else
	{
		debug(LOG_ERROR, "Could not open \"%s\"", filename.c_str());
		return;
	}

	try {
		// do not restore outputModes, outputKey, outputNaming - these are configured for each run
		lastRecordedGameFrameTime = obj.at("lastRecordedGameFrameTime").get<uint32_t>();
		startingPlayerAttributes = obj.at("startingPlayerAttributes").get<std::vector<FixedPlayerAttributes>>();
		gameFrames = obj.at("gameFrames").get<std::vector<GameFrame>>();
		researchLog = obj.at("researchLog").get<std::vector<ResearchEvent>>();
		debugModeLog = obj.at("debugModeLog").get<std::vector<DebugModeEvent>>();
		gameStartRealTime = std::chrono::system_clock::time_point{std::chrono::milliseconds{obj.at("gameStartRealTime").get<std::chrono::milliseconds::rep>()}};
		gameEndRealTime = std::chrono::system_clock::time_point{std::chrono::milliseconds{obj.at("gameEndRealTime").get<std::chrono::milliseconds::rep>()}};
	}
	catch (std::exception& e)
	{
		debug(LOG_ERROR, "Failed to load: %s; with error: %s", filename.c_str(), e.what());
		reset();
	}
}
