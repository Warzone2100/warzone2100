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

#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <physfs.h>
#include "factionid.h"
#include "lib/framework/wzstring.h"
#include <nlohmann/json.hpp>

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

struct RESEARCH;
struct STRUCTURE;

class GameStoryLogger
{
public:
	enum class OutputKey
	{
		PlayerIndex,
		PlayerPosition
	};

	enum class OutputNaming
	{
		Default,
		AutohosterClassic
	};

	struct OutputModes
	{
		bool cmdInterface = false;
		bool logFile = false;

		inline bool anyEnabled() const { return cmdInterface || logFile; }
	};

	struct FixedPlayerAttributes
	{
		std::string name;
		int32_t position;
		int32_t team;
		int32_t colour;
		FactionID faction;
		std::string publicKey;
	};

	struct GameFrame
	{
		struct PlayerStats {
			uint32_t droidsLost = 0;
			uint32_t structuresLost = 0;
			uint32_t kills = 0;
			uint32_t structureKills = 0;
			uint32_t droidsBuilt = 0;
			uint32_t structuresBuilt = 0;
			uint32_t droids = 0;
			uint32_t structs = 0;
			uint32_t researchComplete = 0;
			uint32_t power = 0;
			int32_t  score = 0;
			uint32_t hp = 0;
			uint32_t summExp = 0;
			uint32_t oilRigs = 0;
			uint64_t recentPowerLost = 0;
			uint64_t recentDroidPowerLost = 0;
			uint64_t recentStructurePowerLost = 0;
			uint64_t recentPowerWon = 0;
			uint64_t recentResearchPotential = 0;
			uint64_t recentResearchPerformance = 0;
			std::string usertype; // from end_conditions.js
			optional<uint32_t> playerLeftGameTime = nullopt;
		};

		std::vector<PlayerStats> playerData;
		uint32_t currGameTime = 0;
	};

	struct ResearchEvent
	{
		WzString researchId;
		uint32_t structureId = 0;
		int player = -1;
		uint32_t gameTime = 0;
	};

	struct DebugModeEvent
	{
		bool entered = false;
		uint32_t gameTime = 0;
	};

protected:
	GameStoryLogger();

public:
	static GameStoryLogger& instance();

public:
	void reset();

	// handling events / logging
	void logStartGame();
	void logGameFrame();
	void logResearchCompleted(RESEARCH *psResearch, STRUCTURE *psStruct, int player);
	void logDebugModeChange(bool enabled);
	void logGameOver();

public:
	// accessing data
	const std::vector<FixedPlayerAttributes>& getFixedPlayerAttributes();
	const std::vector<GameFrame>& getGameFrames();
	const std::vector<ResearchEvent>& getResearchLog();

	// configuring output
	void setFrameLoggingInterval(uint32_t seconds);
	void setOutputKey(OutputKey key);
	void setOutputNaming(OutputNaming naming);
	void setOutputModes(OutputModes enabled);
	void configureOutput(OutputKey key, OutputNaming naming, OutputModes enabled);

	// (de)serializing (from/)to savegame data
	void saveToFile(const std::string& filename);
	void loadFromFile(const std::string& filename);

private:

	GameFrame genCurrentFrame() const;
	nlohmann::json genFrameReport(const GameFrame& frame, OutputKey key, OutputNaming naming);
	nlohmann::ordered_json genEndOfGameReport(OutputKey key, OutputNaming naming, bool timeout) const;
	std::string getLogOutputFilename() const;

	void outputLine(std::string&& line);

private:
	OutputModes outputModes;
	PHYSFS_file *fileHandle = nullptr;
	OutputKey outputKey = OutputKey::PlayerPosition;
	OutputNaming outputNaming = OutputNaming::Default;
	uint32_t frameLoggingInterval = 0;

	uint32_t lastRecordedGameFrameTime = 0;
	std::vector<FixedPlayerAttributes> startingPlayerAttributes;
	std::vector<GameFrame> gameFrames;
	std::vector<ResearchEvent> researchLog;
	std::vector<DebugModeEvent> debugModeLog;
	std::chrono::system_clock::time_point gameStartRealTime;
	std::chrono::system_clock::time_point gameEndRealTime;
	nlohmann::json cachedGameDetailsOutputJSON;
};
