/*
	This file is part of Warzone 2100.
	Copyright (C) 2011-2021  Warzone 2100 Project

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
 * @file qtscript.cpp
 *
 * New scripting system
 */

// Documentation stuff follows. The build system will parse the comment prefixes
// and sort the comments into the appropriate Markdown documentation files.

//== # Globals
//==
//== This section describes global variables (or 'globals' for short) that are
//== available from all scripts. You typically cannot write to these variables,
//== they are read-only.
//==
//__ # Events
//__
//__ This section describes event callbacks (or 'events' for short) that are
//__ called from the game when something specific happens. Which scripts
//__ receive them is usually filtered by player. Call ```receiveAllEvents(true)```
//__ to start receiving all events unfiltered.
//__

#include "lib/framework/wzapp.h"
#include "lib/framework/wzconfig.h"
#include "lib/framework/wzpaths.h"

#include "qtscript.h"

#include "lib/framework/file.h"
#include "lib/gamelib/gtime.h"
#include "lib/netplay/netplay.h"
#include "multiplay.h"
#include "levels.h"
#include "map.h"
#include "difficulty.h"
#include "console.h"
#include "clparse.h"
#include "mission.h"
#include "modding.h"
#include "version.h"
#include "game.h"
#include "warzoneconfig.h"
#include "challenge.h"

#include <set>
#include <memory>
#include <utility>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <queue>
#include <limits>

#include "wzscriptdebug.h"
#include "quickjs_backend.h"

#define ATTACK_THROTTLE 1000

/// selection changes are too often and too erratic to trigger immediately,
/// so until we have a queue system for events, delay triggering this way.
static bool selectionChanged = false;

void scripting_engine::GROUPMAP::saveLoadSetLastNewGroupId(int value)
{
	ASSERT_OR_RETURN(, value >= 0, "Invalid value: %d", value);
	lastNewGroupId = value;
}

int scripting_engine::GROUPMAP::newGroupID()
{
	groupID newId = lastNewGroupId + 1;
	while (m_groups.count(newId) != 0 && m_groups.at(newId).size() > 0)
	{
		if (newId < std::numeric_limits<int>::max())
		{
			newId++;
		}
		else
		{
			// loop back around
			// NOTE: group zero is reserved
			newId = 1;
		}
	}
	lastNewGroupId = newId;
	return newId;
}

void scripting_engine::GROUPMAP::insertObjectIntoGroup(const BASE_OBJECT *psObj, scripting_engine::GROUPMAP::groupID groupId)
{
	std::pair<ObjectToGroupMap::iterator,bool> result = m_map.insert(std::pair<const BASE_OBJECT *, scripting_engine::GROUPMAP::groupID>(psObj, groupId));
	if (result.second)
	{
		auto groupSetResult = m_groups[groupId].insert(psObj);
		ASSERT(groupSetResult.second, "Object already exists in group!");
	}
}

size_t scripting_engine::GROUPMAP::groupSize(GROUPMAP::groupID groupId) const
{
	auto it = m_groups.find(groupId);
	if (it != m_groups.end())
	{
		return it->second.size();
	}
	return 0;
}

optional<scripting_engine::GROUPMAP::groupID> scripting_engine::GROUPMAP::removeObjectFromGroup(const BASE_OBJECT *psObj)
{
	auto it = m_map.find(psObj);
	if (it != m_map.end())
	{
		groupID groupId = it->second;
		m_map.erase(it);

		size_t numItemsErased = m_groups[groupId].erase(psObj);
		ASSERT(numItemsErased == 1, "Object did not exist in group set??");
		return optional<groupID>(groupId);
	}
	return optional<groupID>();
}

std::vector<const BASE_OBJECT *> scripting_engine::GROUPMAP::getGroupObjects(groupID groupId) const
{
	std::vector<const BASE_OBJECT *> result;
	auto it = m_groups.find(groupId);
	if (it != m_groups.end())
	{
		result.assign(it->second.begin(), it->second.end());
	}
	return result;
}

scripting_engine::timerNode::timerNode(wzapi::scripting_instance* caller, const TimerFunc& func, const std::string& timerName, int plr, int frame, std::unique_ptr<timerAdditionalData> additionalParam /*= nullptr*/)
: function(func), timerName(timerName), instance(caller), baseobj(-1), baseobjtype(OBJ_NUM_TYPES), additionalTimerFuncParam(std::move(additionalParam)),
	frameTime(frame + gameTime), ms(frame), player(plr), calls(0), type(TIMER_REPEAT)
{}

scripting_engine::timerNode::~timerNode()
{
	if (additionalTimerFuncParam)
	{
		additionalTimerFuncParam->onTimerDelete(timerID, IdToObject(baseobjtype, baseobj, player));
	}
}

scripting_engine::timerNode::timerNode(timerNode&& rhs)           // move constructor
: timerNode()
{
	swap(rhs);
}

void scripting_engine::timerNode::swap(timerNode& _rhs)
{
	std::swap(timerID, _rhs.timerID);
	std::swap(function, _rhs.function);
	std::swap(timerName, _rhs.timerName);
	std::swap(instance, _rhs.instance);
	std::swap(baseobj, _rhs.baseobj);
	std::swap(baseobjtype, _rhs.baseobjtype);
	std::swap(additionalTimerFuncParam, _rhs.additionalTimerFuncParam);
	std::swap(frameTime, _rhs.frameTime);
	std::swap(ms, _rhs.ms);
	std::swap(player, _rhs.player);
	std::swap(calls, _rhs.calls);
	std::swap(type, _rhs.type);
}

scripting_engine::area_by_values_or_area_label_lookup::area_by_values_or_area_label_lookup() { }
scripting_engine::area_by_values_or_area_label_lookup::area_by_values_or_area_label_lookup(const std::string &label)
: m_isLabel(true)
, m_label(label)
{ }
scripting_engine::area_by_values_or_area_label_lookup::area_by_values_or_area_label_lookup(int x1, int y1, int x2, int y2)
: m_isLabel(false)
, x1(x1), y1(y1), x2(x2), y2(y2)
{ }

#define MAX_US 20000
#define HALF_MAX_US 10000


uniqueTimerID scripting_engine::getNextAvailableTimerID()
{
	do {
		++lastTimerID;
	} while (timerIDMap.count(lastTimerID) > 0);
	return lastTimerID;
}

uniqueTimerID scripting_engine::setTimer(wzapi::scripting_instance *caller, const TimerFunc& timerFunc, int player, int milliseconds, std::string timerName /*= ""*/, const BASE_OBJECT * obj /*= nullptr*/, timerType type /*= TIMER_REPEAT*/, std::unique_ptr<timerAdditionalData> additionalParam /*= nullptr*/)
{
	uniqueTimerID newTimerID = getNextAvailableTimerID();
	std::shared_ptr<timerNode> node = std::make_shared<timerNode>(caller, timerFunc, timerName, player, milliseconds, std::move(additionalParam));
	if (obj != nullptr)
	{
		node->baseobj = obj->id;
		node->baseobjtype = obj->type;
	}
	node->type = type;
	node->timerID = newTimerID;
	auto inserted_iter = timers.emplace(timers.end(), std::move(node));
	timerIDMap[newTimerID] = inserted_iter;
	return newTimerID;
}

// internal-only function that adds a Timer node (used for restoring saved games)
void scripting_engine::addTimerNode(std::shared_ptr<scripting_engine::timerNode>&& node)
{
	ASSERT(timerIDMap.count(node->timerID) == 0, "Duplicate timerID found: %s", WzString::number(node->timerID).toUtf8().c_str());
	auto inserted_iter = timers.emplace(timers.end(), std::move(node));
	timerIDMap[(*inserted_iter)->timerID] = inserted_iter;
}

/// Scripting engine (what others call the scripting context, but QtScript's nomenclature is different).
static std::vector<wzapi::scripting_instance *> scripts;

/// Whether the scripts have been set up or not
static bool scriptsReady = false;

/// Structure for research events put on hold
struct researchEvent
{
	RESEARCH *research;
	STRUCTURE *structure;
	int player;

	researchEvent(RESEARCH *r, STRUCTURE *s, int p): research(r), structure(s), player(p) {}
};
/// Research events that are put on hold until the scripts are ready
static std::queue<struct researchEvent> eventQueue;

typedef struct monitor_bin
{
	int worst;
	uint32_t worstGameTime;
	int calls;
	int overMaxTimeCalls;
	int overHalfMaxTimeCalls;
	uint64_t time;
	monitor_bin() : worst(0),  worstGameTime(0), calls(0), overMaxTimeCalls(0), overHalfMaxTimeCalls(0), time(0) {}
} MONITOR_BIN;
typedef std::unordered_map<std::string, MONITOR_BIN> MONITOR;
static std::unordered_map<wzapi::scripting_instance *, MONITOR *> monitors;

static bool globalDialog = false;

bool bInTutorial = false;

// ----------------------------------------------------------

Vector2i positions[MAX_PLAYERS];
static std::unordered_set<uint16_t> derricks;

void scriptSetStartPos(int position, int x, int y)
{
	positions[position].x = x;
	positions[position].y = y;
	debug(LOG_SCRIPT, "Setting start position %d to (%d, %d)", position, x, y);
}

void scriptSetDerrickPos(int x, int y)
{
	const auto mx = map_coord(x);
	const auto my = map_coord(y);
	// MAX_TILE_TEXTURES is 255, so 2 bytes are enough
	// to describe a map position
	static_assert(MAX_TILE_TEXTURES <= 255, "If MAX_TILE_TEXTURES is raised above 255, this code will need to change");
	uint16_t out = (mx << 8) | my;
	derricks.insert(out);
}

bool scriptInit()
{
	int i;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		scriptSetStartPos(i, 0, 0);
	}
	derricks.clear();
	derricks.reserve(8 * MAX_PLAYERS);
	return true;
}

Vector2i getPlayerStartPosition(int player)
{
	return positions[player];
}

nlohmann::json scripting_engine::constructDerrickPositions()
{
	// Static map knowledge about start positions
	//== * ```derrickPositions``` A set of derrick starting positions on the current map. Each item in the set is an
	//== object containing the x and y variables for a derrick.
	nlohmann::json derrickPositions = nlohmann::json::array(); //engine->newArray(derricks.size());
	for (uint16_t pos: derricks)
	{
		nlohmann::json vector = nlohmann::json::object();
		const auto mx = (pos & static_cast<uint16_t>(0xff00)) >> 8;
		const auto my = pos & (static_cast<uint16_t>(0x00ff));
		vector["x"] = mx;
		vector["y"] = my;
		vector["type"] = SCRIPT_POSITION;
		derrickPositions.push_back(std::move(vector));
	}
	return derrickPositions;
}

nlohmann::json scripting_engine::constructStartPositions()
{
	// Static map knowledge about start positions
	//== * ```startPositions``` An array of player start positions on the current map. Each item in the array is an
	//== object containing the x and y variables for a player start position.
	nlohmann::json startPositions = nlohmann::json::array(); //engine->newArray(game.maxPlayers);
	for (int i = 0; i < game.maxPlayers; i++)
	{
		nlohmann::json vector = nlohmann::json::object();
		vector["x"] = map_coord(positions[i].x);
		vector["y"] = map_coord(positions[i].y);
		vector["type"] = SCRIPT_POSITION;
		startPositions.push_back(vector);
	}
	return startPositions;
}

// ----------------------------------------------------------

bool scripting_engine::removeTimer(uniqueTimerID timerID)
{
	auto it = timerIDMap.find(timerID);
	if (it != timerIDMap.end())
	{
		(*it->second)->type = TIMER_REMOVED; // in case a timer is removed while running timers
		timers.erase(it->second);
		timerIDMap.erase(it);
		return true;
	}
	return false;
}

void scriptRemoveObject(const BASE_OBJECT *psObj)
{
	// Weed out timers with dead objects
	scripting_engine::instance().removeTimersIf([psObj](const scripting_engine::timerNode& node)
	{
		return (node.baseobj == psObj->id);
	});
	scripting_engine::instance().groupRemoveObject(psObj);
}

// do not want to call this 'init', since scripts are often loaded before we get here
bool prepareScripts(bool loadGame)
{
	debug(LOG_WZ, "Scripts prepared");
	if (!loadGame) // labels saved in savegame
	{
		scripting_engine::instance().prepareLabels();
	}
	// Assume that by this point all scripts are loaded
	scriptsReady = true;
	while (!eventQueue.empty())
	{
		researchEvent& resEvent = eventQueue.front();
		triggerEventResearched(resEvent.research, resEvent.structure, resEvent.player);
		eventQueue.pop();
	}
	return true;
}

bool initScripts()
{
	return true;
}

bool shutdownScripts()
{
	return scripting_engine::instance().shutdownScripts();
}

bool scripting_engine::shutdownScripts()
{
	scriptsReady = false;
	jsDebugShutdown();
	globalDialog = false;
	for (auto *instance : scripts)
	{
		MONITOR *monitor = monitors.at(instance);
		WzString scriptName = WzString::fromUtf8(instance->scriptName());
		instance->dumpScriptLog("=== PERFORMANCE DATA ===\n");
		instance->dumpScriptLog("    calls | avg (usec) | worst (usec) | worst call at | >=limit | >=limit/2 | function\n");
		for (MONITOR::const_iterator iter = monitor->begin(); iter != monitor->end(); ++iter)
		{
			const std::string &function = iter->first;
			MONITOR_BIN m = iter->second;
			std::ostringstream info;
			info << std::right << std::setw(9) << m.calls << " | ";
			info << std::right << std::setw(10) << (m.time / m.calls) << " | ";
			info << std::right << std::setw(12) << m.worst << " | ";
			info << std::right << std::setw(13) << m.worstGameTime << " | ";
			info << std::right << std::setw(7) << m.overMaxTimeCalls << " | ";
			info << std::right << std::setw(9) << m.overHalfMaxTimeCalls << " | ";
			info << function << "\n";
			instance->dumpScriptLog(info.str());
		}
		monitor->clear();
		delete monitor;
		unregisterFunctions(instance);
	}
	timers.clear();
	lastTimerID = 0;
	timerIDMap.clear();
	monitors.clear();
	for (auto& script : scripts)
	{
		delete script;
	}
	scripts.clear();
	return true;
}

bool updateScripts()
{
	return scripting_engine::instance().updateScripts();
}

bool scripting_engine::updateScripts()
{
	// Call delayed triggers here
	if (selectionChanged)
	{
		for (auto *instance : scripts)
		{
			instance->handle_eventSelectionChanged(wzapi::enumSelected());
		}
		selectionChanged = false;
	}

	// Update gameTime
	for (auto *instance : scripts)
	{
		instance->updateGameTime(gameTime);
	}
	// Weed out dead timers
	removeTimersIf([](const timerNode& node)
	{
		return (node.type == TIMER_ONESHOT_DONE);
	});
	// Check for timers, and run them if applicable.
	// TODO - load balancing
	std::vector<std::shared_ptr<timerNode>> runlist; // make a new list here, since we might trample all over the timer list during execution
	for (auto &node : timers)
	{
		if (node->frameTime <= gameTime)
		{
			node->frameTime = node->ms + gameTime;	// update for next invokation
			if (node->type == TIMER_ONESHOT_READY)
			{
				node->type = TIMER_ONESHOT_DONE; // unless there is none
			}
			node->calls++;
			runlist.push_back(node);
		}
	}

	for (auto &node : runlist)
	{
		// IMPORTANT: A queued function can delete a timer that is in the runlist!
		// So we must verify that the node is not one of the deleted ones.
		if (node->type == TIMER_REMOVED)
		{
			continue; // skip
		}
		node->function(node->timerID, IdToObject(node->baseobjtype, node->baseobj, node->player), node->additionalTimerFuncParam.get());
	}

	return true;
}

wzapi::scripting_instance* loadPlayerScript(const WzString& path, int player, AIDifficulty difficulty)
{
	return scripting_engine::instance().loadPlayerScript(path, player, difficulty);
}

static wzapi::scripting_instance* loadPlayerScriptByBackend(const WzString& path, int player, int realDifficulty)
{
	// JS scripts:
	switch (war_getJSBackend())
	{
		case JS_BACKEND::quickjs:
			return createQuickJSScriptInstance(path, player, realDifficulty);
		case JS_BACKEND::num_backends:
			debug(LOG_ERROR, "Invalid js backend value"); // should not happen
	}
	return nullptr;
}

wzapi::scripting_instance* scripting_engine::loadPlayerScript(const WzString& path, int player, AIDifficulty difficulty)
{
	ASSERT_OR_RETURN(nullptr, player >= 0 && (player < MAX_PLAYERS || player == selectedPlayer), "Player index %d out of bounds", player);

	debug(LOG_SCRIPT, "loadPlayerScript[%d]: %s", player, path.toUtf8().c_str());

	int realDifficulty;
	if (game.type == LEVEL_TYPE::SKIRMISH)
	{
		realDifficulty = static_cast<int8_t>(difficulty);
	}
	else // campaign
	{
		realDifficulty = (int)getDifficultyLevel();
	}

	wzapi::scripting_instance* pNewInstance = loadPlayerScriptByBackend(path, player, realDifficulty);
	if (!pNewInstance)
	{
		// failed to create new scripting instance
		debug(LOG_ERROR, "Failed to create new scripting instance: path:\"%s\", player: %d, difficulty: %d", path.toUtf8().c_str(), player, static_cast<int>(difficulty));
		return nullptr;
	}

	// Create group map
	GROUPMAP *psMap = new GROUPMAP;
	auto insert_result = groups.insert(ENGINEMAP::value_type(pNewInstance, psMap));
	ASSERT(insert_result.second, "Entry for this engine %p already exists in ENGINEMAP!", static_cast<void *>(pNewInstance));

	json globalVars = json::object();
	// Special global variables
	//== * ```version``` Current version of the game, set in *major.minor* format.
	globalVars["version"] = version_getVersionString();
	//== * ```selectedPlayer``` The player controlled by the client on which the script runs.
	globalVars["selectedPlayer"] = selectedPlayer;
	//== * ```gameTime``` The current game time. Updated before every invokation of a script.
	pNewInstance->setSpecifiedGlobalVariable("gameTime", gameTime, wzapi::GlobalVariableFlags::ReadOnlyUpdatedFromApp | wzapi::GlobalVariableFlags::DoNotSave);
	//== * ```modList``` The current loaded mods.
	globalVars["modList"] = getModList();


	//== * ```difficulty``` The currently set campaign difficulty, or the current AI's difficulty setting. It will be one of
	//== ```SUPEREASY``` (campaign only), ```EASY```, ```MEDIUM```, ```HARD``` or ```INSANE```.
	if (game.type == LEVEL_TYPE::SKIRMISH)
	{
		globalVars["difficulty"] = static_cast<int8_t>(difficulty);
	}
	else // campaign
	{
		globalVars["difficulty"] = (int)getDifficultyLevel();
	}
	//== * ```mapName``` The name of the current map.
	globalVars["mapName"] = game.map;
	//== * ```tilesetType``` The area name of the map.
	std::string tilesetType("CUSTOM");
	switch (currentMapTileset)
	{
		case MAP_TILESET::ARIZONA:
			tilesetType = "ARIZONA";
			break;
		case MAP_TILESET::URBAN:
			tilesetType = "URBAN";
			break;
		case MAP_TILESET::ROCKIES:
			tilesetType = "ROCKIES";
			break;
	}
	globalVars["tilesetType"] = tilesetType;
	//== * ```baseType``` The type of base that the game starts with. It will be one of ```CAMP_CLEAN```, ```CAMP_BASE``` or ```CAMP_WALLS```.
	globalVars["baseType"] = game.base;
	//== * ```alliancesType``` The type of alliances permitted in this game. It will be one of ```NO_ALLIANCES```, ```ALLIANCES```, ```ALLIANCES_UNSHARED``` or ```ALLIANCES_TEAMS```.
	globalVars["alliancesType"] = game.alliance;
	//== * ```powerType``` The power level set for this game.
	globalVars["powerType"] = game.power;
	//== * ```maxPlayers``` The number of active players in this game.
	globalVars["maxPlayers"] = game.maxPlayers;
	//== * ```scavengers``` Whether or not scavengers are activated in this game, and, if so, which type.
	globalVars["scavengers"] = game.scavengers;
	//== * ```mapWidth``` Width of map in tiles.
	globalVars["mapWidth"] = mapWidth;
	//== * ```mapHeight``` Height of map in tiles.
	globalVars["mapHeight"] = mapHeight;
	//== * ```scavengerPlayer``` Index of scavenger player. (3.2+ only)
	globalVars["scavengerPlayer"] = scavengerSlot();
	//== * ```isMultiplayer``` If the current game is a online multiplayer game or not. (3.2+ only)
	globalVars["isMultiplayer"] = NetPlay.bComms;
	//== * ```challenge``` If the current game is a challenge. (4.1.4+ only)
	globalVars["challenge"] = challengeActive;
	//== * ```idleTime``` The amount of game time without active play before a player should be considered "inactive". (0 = disable activity alerts / AFK check) (4.2.0+ only)
	globalVars["idleTime"] = game.inactivityMinutes * 60 * 1000;

	pNewInstance->setSpecifiedGlobalVariables(globalVars, wzapi::GlobalVariableFlags::ReadOnly | wzapi::GlobalVariableFlags::DoNotSave);

	// Register 'Stats' object. It is a read-only representation of basic game component states.
	pNewInstance->setSpecifiedGlobalVariable("Stats", wzapi::constructStatsObject(), wzapi::GlobalVariableFlags::ReadOnly | wzapi::GlobalVariableFlags::DoNotSave);

	// Register 'MapTiles' two-dimensional array. It is a read-only representation of static map tile states.
	pNewInstance->setSpecifiedGlobalVariable("MapTiles", wzapi::constructMapTilesArray(), wzapi::GlobalVariableFlags::ReadOnly | wzapi::GlobalVariableFlags::DoNotSave);

	// Set some useful constants
	pNewInstance->setSpecifiedGlobalVariables(wzapi::getUsefulConstants(), wzapi::GlobalVariableFlags::ReadOnly | wzapi::GlobalVariableFlags::DoNotSave);

	/// Place to store group sizes
	//== * ```groupSizes``` A sparse array of group sizes. If a group has never been used, the entry in this array will
	//== be undefined.
	pNewInstance->setSpecifiedGlobalVariable("groupSizes", nlohmann::json::object(), wzapi::GlobalVariableFlags::ReadOnly | wzapi::GlobalVariableFlags::DoNotSave);

	// Static knowledge about players
	pNewInstance->setSpecifiedGlobalVariable("playerData", wzapi::constructStaticPlayerData(), wzapi::GlobalVariableFlags::ReadOnly | wzapi::GlobalVariableFlags::DoNotSave);

	// Static map knowledge about start positions
	pNewInstance->setSpecifiedGlobalVariable("derrickPositions", constructDerrickPositions(), wzapi::GlobalVariableFlags::ReadOnly | wzapi::GlobalVariableFlags::DoNotSave);
	pNewInstance->setSpecifiedGlobalVariable("startPositions", constructStartPositions(), wzapi::GlobalVariableFlags::ReadOnly | wzapi::GlobalVariableFlags::DoNotSave);

	WzPathInfo basename = WzPathInfo::fromPlatformIndependentPath(path.toUtf8());
	json globalVarsToSave = json::object();
	// We need to always save the 'me' special variable.
	//== * ```me``` The player the script is currently running as.
	globalVarsToSave["me"] = player;

	// We also need to save the special 'scriptName' variable.
	//== * ```scriptName``` Base name of the script that is running.
	globalVarsToSave["scriptName"] = basename.baseName();

	// We also need to save the special 'scriptPath' variable.
	//== * ```scriptPath``` Base path of the script that is running.
	globalVarsToSave["scriptPath"] = basename.path();

	pNewInstance->setSpecifiedGlobalVariables(globalVarsToSave, wzapi::GlobalVariableFlags::ReadOnly); // ensure these are saved

	// Clear previous log file
	PHYSFS_delete((std::string("logs/") + pNewInstance->scriptName() + ".log").c_str());

	// Attempt to ready instance for execution
	if (!pNewInstance->readyInstanceForExecution())
	{
		delete pNewInstance;
		debug(LOG_ERROR, "Unable to ready instance for execution: %s", path.toUtf8().c_str());
		return nullptr;
	}

	// Register script
	scripts.push_back(pNewInstance);

	MONITOR *monitor = new MONITOR;
	monitors[pNewInstance] = monitor;

	debug(LOG_SAVE, "Created script engine %zu for player %d from %s", scripts.size() - 1, player, path.toUtf8().c_str());
	return pNewInstance;
}

bool loadGlobalScript(WzString path)
{
	return loadPlayerScript(std::move(path), selectedPlayer, AIDifficulty::DISABLED);
}

bool saveScriptStates(const char *filename)
{
	return scripting_engine::instance().saveScriptStates(filename);
}

bool scripting_engine::saveScriptStates(const char *filename)
{
	WzConfig ini(filename, WzConfig::ReadAndWrite);
	for (int i = 0; i < scripts.size(); ++i)
	{
		wzapi::scripting_instance* instance = scripts.at(i);

		nlohmann::json globalsResult = nlohmann::json::object();
		instance->saveScriptGlobals(globalsResult);
		// 'scriptName' and 'me' should be saved implicitly by the backend's saveScriptGlobals
		ASSERT(globalsResult.contains("me"), "Missing required global \"me\"");
		ASSERT(globalsResult.contains("scriptName"), "Missing required global \"scriptName\"");
		ini.setValue("globals_" + WzString::number(i), globalsResult);

		// we have to save 'scriptName' and 'me' explicitly
		nlohmann::json groupsResult = nlohmann::json::object();
		saveGroups(groupsResult, instance);
		groupsResult["me"] = instance->player();
		groupsResult["scriptName"] = instance->scriptName();
		ini.setValue("groups_" + WzString::number(i), std::move(groupsResult));
	}
	size_t timerIdx = 0;
	for (const auto& node : timers)
	{
		nlohmann::json nodeInfo = nlohmann::json::object();
		nodeInfo["timerID"] = node->timerID;
		nodeInfo["timerName"] = node->timerName;
		// we have to save 'scriptName' and 'me' explicitly
		nodeInfo["me"] = node->player;
		nodeInfo["scriptName"] = node->instance->scriptName();
		nodeInfo["functionRestoreInfo"] = node->instance->saveTimerFunction(node->timerID, node->timerName, node->additionalTimerFuncParam.get());
		if (node->baseobj >= 0)
		{
			nodeInfo["object"] = node->baseobj;
			nodeInfo["objectType"] = (int)node->baseobjtype;
		}
		nodeInfo["frame"] = node->frameTime;
		nodeInfo["ms"] = node->ms;
		nodeInfo["calls"] = node->calls;
		nodeInfo["type"] = (int)node->type;

		ini.setValue("triggers_" + WzString::number(timerIdx), std::move(nodeInfo));
		++timerIdx;
	}
	return true;
}

wzapi::scripting_instance* scripting_engine::findInstanceForPlayer(int match, const WzString& _scriptName)
{
	WzString scriptName = _scriptName.normalized(WzString::NormalizationForm_KD);
	for (auto *instance : scripts)
	{
		int player = instance->player();
		WzString matchName = WzString::fromUtf8(instance->scriptName()).normalized(WzString::NormalizationForm_KD);
		if (match == player && (matchName.compare(scriptName) == 0 || scriptName.isEmpty()))
		{
			return instance;
		}
	}
	ASSERT(false, "Script context for player %d and script name %s not found",
	       match, scriptName.toUtf8().c_str());
	return nullptr;
}

bool loadScriptStates(const char *filename)
{
	return scripting_engine::instance().loadScriptStates(filename);
}

bool scripting_engine::loadScriptStates(const char *filename)
{
	uniqueTimerID maxRestoredTimerID = 0;
	WzConfig ini(filename, WzConfig::ReadOnly);
	std::vector<WzString> list = ini.childGroups();
	debug(LOG_SAVE, "Loading script states for %zu script contexts", scripts.size());
	for (size_t i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		int player = ini.value("me").toInt();
		WzString scriptName = ini.value("scriptName").toWzString();
		wzapi::scripting_instance* instance = findInstanceForPlayer(player, scriptName);
		if (instance && list[i].startsWith("triggers_"))
		{
			std::shared_ptr<timerNode> node = std::make_shared<timerNode>();
			if (ini.contains("timerID"))
			{
				node->timerID = ini.value("timerID").toInt();
			}
			else
			{
				// backwards-compat with old saves
				node->timerID = getNextAvailableTimerID();
			}
			if (ini.contains("timerName"))
			{
				node->timerName = ini.value("timerName").toWzString().toStdString();
			}
			else
			{
				// backwards-compat with old saves
				node->timerName = ini.value("function").toWzString().toStdString();
			}
			node->instance = instance;
			debug(LOG_SAVE, "Registering trigger %zu for player %d, script %s",
			      i, player, scriptName.toUtf8().c_str());
			node->baseobj = ini.value("baseobj", -1).toInt();
			node->baseobjtype = (OBJECT_TYPE)ini.value("objectType", (int)OBJ_NUM_TYPES).toInt();
			node->frameTime = ini.value("frame").toInt();
			node->ms = ini.value("ms").toInt();
			node->player = player;
			node->calls = ini.value("calls").toInt();
			node->type = (timerType)ini.value("type", TIMER_REPEAT).toInt();

			std::tuple<TimerFunc, std::unique_ptr<timerAdditionalData>> restoredTimerInfo;
			try
			{
				if (ini.contains("functionRestoreInfo"))
				{
					restoredTimerInfo = instance->restoreTimerFunction(ini.value("functionRestoreInfo").jsonValue());
				}
				else
				{
					// backwards-compat with old saves
					// construct a JS-compatible functionRestoreInfo using the "function" key, which should be set in an older save
					nlohmann::json backwardsCompatJSFunctionRestoreInfo = nlohmann::json::object();
					if (!ini.contains("function"))
					{
						ASSERT(false, "Invalid trigger in save (%s) - missing new functionRestoreInfo block, and old function parameter", list[i].toUtf8().c_str());
						continue;
					}
					backwardsCompatJSFunctionRestoreInfo["function"] = ini.value("function").jsonValue();
					restoredTimerInfo = instance->restoreTimerFunction(backwardsCompatJSFunctionRestoreInfo);
				}
			}
			catch (const std::exception& e)
			{
				ASSERT(false, "Failed to restore saved timer function info: %s", e.what());
				continue;
			}

			node->function = std::get<0>(restoredTimerInfo);
			node->additionalTimerFuncParam = std::move(std::get<1>(restoredTimerInfo));

			maxRestoredTimerID = std::max<uniqueTimerID>(maxRestoredTimerID, node->timerID);

			addTimerNode(std::move(node));
		}
		else if (instance && list[i].startsWith("globals_"))
		{
			nlohmann::json result = ini.currentJsonValue();
			debug(LOG_SAVE, "Loading script globals for player %d, script %s -- found %zu values",
				  instance->player(), instance->scriptName().c_str(), result.size());
			// filter out "scriptName" and "me" variables
			result.erase("me");
			result.erase("scriptName");
			instance->loadScriptGlobals(result);
		}
		else if (instance && list[i].startsWith("groups_"))
		{
			std::vector<WzString> keys = ini.childKeys();
			for (size_t j = 0; j < keys.size(); ++j)
			{
				std::vector<WzString> values = ini.value(keys.at(j)).toWzStringList();
				bool ok = false; // check if number
				int droidId = keys.at(j).toInt(&ok);
				for (size_t k = 0; ok && k < values.size(); k++)
				{
					int groupId = values.at(k).toInt();
					loadGroup(instance, groupId, droidId);
				}
			}
			bool bHasLastNewGroupId = false;
			int lastNewGroupId = ini.value("lastNewGroupId").toInt(&bHasLastNewGroupId);
			if (bHasLastNewGroupId)
			{
				GROUPMAP *psMap = getGroupMap(instance);
				if (psMap)
				{
					psMap->saveLoadSetLastNewGroupId(lastNewGroupId);
				}
			}
		}
		else
		{
			if (instance)
			{
				debug(LOG_WARNING, "Encountered unexpected group '%s' in loadScriptStates", list[i].toUtf8().c_str());
			}
		}
		ini.endGroup();
	}
	lastTimerID = maxRestoredTimerID;
	return true;
}

std::unordered_map<wzapi::scripting_instance *, nlohmann::json> scripting_engine::debug_GetGlobalsSnapshot() const
{
	MODELMAP debug_globals;
	for (auto *instance : scripts)
	{
		json scriptGlobals = instance->debugGetAllScriptGlobals();
		debug_globals[instance] = std::move(scriptGlobals);
	}
	return debug_globals;
}

std::vector<scripting_engine::timerNodeSnapshot> scripting_engine::debug_GetTimersSnapshot() const
{
	std::vector<scripting_engine::timerNodeSnapshot> debug_timer_snapshot;
	for (auto& timer : timers)
	{
		debug_timer_snapshot.emplace_back(timerNodeSnapshot(timer));
	}
	return debug_timer_snapshot;
}

void jsAutogameSpecific(const WzString &name, int player, AIDifficulty difficulty)
{
	wzapi::scripting_instance* instance = loadPlayerScript(name, player, difficulty);
	if (!instance)
	{
		console(_("Failed to load selected AI! Check your logs to see why."));
		return;
	}
	console(_("Loaded the %s AI script for current player!"), name.toUtf8().c_str());
	instance->handle_eventGameInit();
	instance->handle_eventStartLevel();
}

void jsHandleDebugClosed()
{
	globalDialog = false;
}

void jsShowDebug()
{
	globalDialog = true;
	class make_shared_enabler : public scripting_engine::DebugInterface { };
	bool isSpectator = NetPlay.players[selectedPlayer].isSpectator;
	jsDebugCreate(std::make_shared<make_shared_enabler>(), jsHandleDebugClosed, isSpectator);
}

// ----------------------------------------------------------------------------------------
// Events

/// For generic, parameter-less triggers
//__ ## eventGameInit()
//__
//__ An event that is run once as the game is initialized. Not all game state may have been
//__ properly initialized by this time, so use this only to initialize script state.
//__
//__ ## eventStartLevel()
//__
//__ An event that is run once the game has started and all game data has been loaded.
//__
//__ ## eventMissionTimeout()
//__
//__ An event that is run when the mission timer has run out.
//__
//__ ## eventVideoDone()
//__
//__ An event that is run when a video show stopped playing.
//__
//__ ## eventGameLoaded()
//__
//__ An event that is run when game is loaded from a saved game. There is usually no need to use this event.
//__
//__ ## eventGameSaving()
//__
//__ An event that is run before game is saved. There is usually no need to use this event.
//__
//__ ## eventGameSaved()
//__
//__ An event that is run after game is saved. There is usually no need to use this event.
//__
//__ ## eventDeliveryPointMoving()
//__
//__ An event that is run when the current player starts to move a delivery point.
//__
//__ ## eventDeliveryPointMoved()
//__
//__ An event that is run after the current player has moved a delivery point.
//__
//__ ## eventTransporterLaunch(transport)
//__
//__ An event that is run when the mission transporter has been ordered to fly off.
//__
//__ ## eventTransporterArrived(transport)
//__
//__ An event that is run when the mission transporter has arrived at the map edge with reinforcements.
//__
//__ ## eventTransporterExit(transport)
//__
//__ An event that is run when the mission transporter has left the map.
//__
//__ ## eventTransporterDone(transport)
//__
//__ An event that is run when the mission transporter has no more reinforcements to deliver.
//__
//__ ## eventTransporterLanded(transport)
//__
//__ An event that is run when the mission transporter has landed with reinforcements.
//__
//__ ## eventDesignBody()
//__
//__ An event that is run when current user picks a body in the design menu.
//__
//__ ## eventDesignPropulsion()
//__
//__ An event that is run when current user picks a propulsion in the design menu.
//__
//__ ## eventDesignWeapon()
//__
//__ An event that is run when current user picks a weapon in the design menu.
//__
//__ ## eventDesignCommand()
//__
//__ An event that is run when current user picks a command turret in the design menu.
//__
//__ ## eventDesignSystem()
//__
//__ An event that is run when current user picks a system other than command turret in the design menu.
//__
//__ ## eventDesignQuit()
//__
//__ An event that is run when current user leaves the design menu.
//__
//__ ## eventMenuBuildSelected()
//__
//__ An event that is run when current user picks something new in the build menu.
//__
//__ ## eventMenuBuild()
//__
//__ An event that is run when current user opens the build menu.
//__
//__ ## eventMenuResearch()
//__
//__ An event that is run when current user opens the research menu.
//__
//__ ## eventMenuManufacture()
//__
//__ An event that is run when current user opens the manufacture menu.
//__
bool triggerEvent(SCRIPT_TRIGGER_TYPE trigger, BASE_OBJECT *psObj)
{
	// HACK: TRIGGER_VIDEO_QUIT is called before scripts for initial campaign video
	ASSERT(scriptsReady || trigger == TRIGGER_VIDEO_QUIT, "Scripts not initialized yet");
	for (auto *instance : scripts)
	{
		if (psObj)
		{
			int player = instance->player();
			bool receiveAll = instance->isReceivingAllEvents();
			if (player != psObj->player && !receiveAll)
			{
				continue;
			}
		}

		switch (trigger)
		{
		case TRIGGER_GAME_INIT:
			instance->handle_eventGameInit();
			break;
		case TRIGGER_START_LEVEL:
			processVisibility(); // make sure we initialize visibility first
			instance->handle_eventStartLevel();
			break;
		case TRIGGER_TRANSPORTER_LAUNCH:
			instance->handle_eventLaunchTransporter(); // deprecated!
			instance->handle_eventTransporterLaunch(psObj);
			break;
		case TRIGGER_TRANSPORTER_ARRIVED:
			instance->handle_eventReinforcementsArrived(); // deprecated!
			instance->handle_eventTransporterArrived(psObj);
			break;
		case TRIGGER_DELIVERY_POINT_MOVING:
			instance->handle_eventDeliveryPointMoving(psObj);
			break;
		case TRIGGER_DELIVERY_POINT_MOVED:
			instance->handle_eventDeliveryPointMoved(psObj);
			break;
		case TRIGGER_OBJECT_RECYCLED:
			instance->handle_eventObjectRecycled(psObj);
			break;
		case TRIGGER_TRANSPORTER_EXIT:
			instance->handle_eventTransporterExit(psObj);
			break;
		case TRIGGER_TRANSPORTER_DONE:
			instance->handle_eventTransporterDone(psObj);
			break;
		case TRIGGER_TRANSPORTER_LANDED:
			instance->handle_eventTransporterLanded(psObj);
			break;
		case TRIGGER_MISSION_TIMEOUT:
			instance->handle_eventMissionTimeout();
			break;
		case TRIGGER_VIDEO_QUIT:
			instance->handle_eventVideoDone();
			break;
		case TRIGGER_GAME_LOADED:
			instance->handle_eventGameLoaded();
			break;
		case TRIGGER_GAME_SAVING:
			instance->handle_eventGameSaving();
			break;
		case TRIGGER_GAME_SAVED:
			instance->handle_eventGameSaved();
			break;
		case TRIGGER_DESIGN_BODY:
			instance->handle_eventDesignBody();
			break;
		case TRIGGER_DESIGN_PROPULSION:
			instance->handle_eventDesignPropulsion();
			break;
		case TRIGGER_DESIGN_WEAPON:
			instance->handle_eventDesignWeapon();
			break;
		case TRIGGER_DESIGN_COMMAND:
			instance->handle_eventDesignCommand();
			break;
		case TRIGGER_DESIGN_SYSTEM:
			instance->handle_eventDesignSystem();
			break;
		case TRIGGER_DESIGN_QUIT:
			instance->handle_eventDesignQuit();
			break;
		case TRIGGER_MENU_BUILD_SELECTED:
			instance->handle_eventMenuBuildSelected();
			break;
		case TRIGGER_MENU_RESEARCH_SELECTED:
			instance->handle_eventMenuResearchSelected();
			break;
		case TRIGGER_MENU_BUILD_UP:
			instance->handle_eventMenuBuild();
			break;
		case TRIGGER_MENU_RESEARCH_UP:
			instance->handle_eventMenuResearch();
			break;
		case TRIGGER_MENU_DESIGN_UP:
			instance->handle_eventMenuDesign();
			break;
		case TRIGGER_MENU_MANUFACTURE_UP:
			instance->handle_eventMenuManufacture();
			break;
		}
	}

	if ((trigger == TRIGGER_START_LEVEL || trigger == TRIGGER_GAME_LOADED) && !saveandquit_enabled().empty())
	{
		saveGame(saveandquit_enabled().c_str(), GTYPE_SAVE_START);
		wzQuit(0);
	}

	return true;
}

//__ ## eventPlayerLeft(player)
//__
//__ An event that is run after a player has left the game.
//__
bool triggerEventPlayerLeft(int player)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *instance : scripts)
	{
		instance->handle_eventPlayerLeft(player);
	}
	return true;
}

//__ ## eventCheatMode(entered)
//__
//__ Game entered or left cheat/debug mode.
//__ The entered parameter is true if cheat mode entered, false otherwise.
//__
bool triggerEventCheatMode(bool entered)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *instance : scripts)
	{
		instance->handle_eventCheatMode(entered);
	}
	return true;
}

//__ ## eventDroidIdle(droid)
//__
//__ A droid should be given new orders.
//__
bool triggerEventDroidIdle(DROID *psDroid)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *instance : scripts)
	{
		int player = instance->player();
		if (player == psDroid->player)
		{
			instance->handle_eventDroidIdle(psDroid);
		}
	}
	return true;
}

//__ ## eventDroidBuilt(droid[, structure])
//__
//__ An event that is run every time a droid is built. The structure parameter is set
//__ if the droid was produced in a factory. It is not triggered for droid theft or
//__ gift (check ```eventObjectTransfer``` for that).
//__
bool triggerEventDroidBuilt(DROID *psDroid, STRUCTURE *psFactory)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	optional<const STRUCTURE *> opt_factory = (psFactory) ? optional<const STRUCTURE *>(psFactory) : nullopt;
	for (auto *instance : scripts)
	{
		int player = instance->player();
		bool receiveAll = instance->isReceivingAllEvents();
		if (player == psDroid->player || receiveAll)
		{
			instance->handle_eventDroidBuilt(psDroid, opt_factory);
		}
	}
	return true;
}

//__ ## eventStructureBuilt(structure[, droid])
//__
//__ An event that is run every time a structure is produced. The droid parameter is set
//__ if the structure was built by a droid. It is not triggered for building theft
//__ (check ```eventObjectTransfer``` for that).
//__
bool triggerEventStructBuilt(STRUCTURE *psStruct, DROID *psDroid)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	optional<const DROID *> opt_droid = (psDroid) ? optional<const DROID *>(psDroid) : nullopt;
	for (auto *instance : scripts)
	{
		int player = instance->player();
		bool receiveAll = instance->isReceivingAllEvents();
		if (player == psStruct->player || receiveAll)
		{
			instance->handle_eventStructureBuilt(psStruct, opt_droid);
		}
	}
	return true;
}

//__ ## eventStructureDemolish(structure[, droid])
//__
//__ An event that is run every time a structure begins to be demolished. This does
//__ not trigger again if the structure is partially demolished.
//__
bool triggerEventStructDemolish(STRUCTURE *psStruct, DROID *psDroid)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	optional<const DROID *> opt_droid = (psDroid) ? optional<const DROID *>(psDroid) : nullopt;
	for (auto *instance : scripts)
	{
		int player = instance->player();
		bool receiveAll = instance->isReceivingAllEvents();
		if (player == psStruct->player || receiveAll)
		{
			instance->handle_eventStructureDemolish(psStruct, opt_droid);
		}
	}
	return true;
}

//__ ## eventStructureReady(structure)
//__
//__ An event that is run every time a structure is ready to perform some
//__ special ability. It will only fire once, so if the time is not right,
//__ register your own timer to keep checking.
//__
bool triggerEventStructureReady(STRUCTURE *psStruct)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *instance : scripts)
	{
		int player = instance->player();
		bool receiveAll = instance->isReceivingAllEvents();
		if (player == psStruct->player || receiveAll)
		{
			instance->handle_eventStructureReady(psStruct);
		}
	}
	return true;
}

//__ ## eventStructureUpgradeStarted(structure)
//__
//__ An event that is run every time a structure starts to be upgraded.
//__
bool triggerEventStructureUpgradeStarted(STRUCTURE *psStruct)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *instance : scripts)
	{
		int player = instance->player();
		bool receiveAll = instance->isReceivingAllEvents();
		if (player == psStruct->player || receiveAll)
		{
			instance->handle_eventStructureUpgradeStarted(psStruct);
		}
	}
	return true;
}

//__ ## eventAttacked(victim, attacker)
//__
//__ An event that is run when an object belonging to the script's controlling player is
//__ attacked. The attacker parameter may be either a structure or a droid.
//__
bool triggerEventAttacked(BASE_OBJECT *psVictim, BASE_OBJECT *psAttacker, int lastHit)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	if (!psAttacker)
	{
		// do not fire off this event if there is no attacker -- nothing do respond to
		// (FIXME -- consider this carefully)
		return false;
	}
	// throttle the event for performance
	if (gameTime - lastHit < ATTACK_THROTTLE)
	{
		return false;
	}
	for (auto *instance : scripts)
	{
		int player = instance->player();
		bool receiveAll = instance->isReceivingAllEvents();
		if (player == psVictim->player || receiveAll)
		{
			instance->handle_eventAttacked(psVictim, psAttacker);
		}
	}
	return true;
}

//__ ## eventResearched(research, structure, player)
//__
//__ An event that is run whenever a new research is available. The structure
//__ parameter is set if the research comes from a research lab owned by the
//__ current player. If an ally does the research, the structure parameter will
//__ be set to null. The player parameter gives the player it is called for.
//__
bool triggerEventResearched(RESEARCH *psResearch, STRUCTURE *psStruct, int player)
{
	//HACK: This event can be triggered when loading savegames, before the script engines are initialized.
	// if this is the case, we need to store these events and replay them later
	if (!scriptsReady)
	{
		eventQueue.emplace(psResearch, psStruct, player);
		return true;
	}
	for (auto *instance : scripts)
	{
		int me = instance->player();
		bool receiveAll = instance->isReceivingAllEvents();
		if (me == player || receiveAll)
		{
			instance->handle_eventResearched(wzapi::researchResult(psResearch, player), psStruct, player);
		}
	}
	return true;
}

//__ ## eventDestroyed(object)
//__
//__ An event that is run whenever an object is destroyed. Careful passing
//__ the parameter object around, since it is about to vanish!
//__
bool triggerEventDestroyed(BASE_OBJECT *psVictim)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	if (!psVictim) { return true; }
	for (auto *instance : scripts)
	{
		instance->handle_eventDestroyed(psVictim);
	}
	return true;
}

//__ ## eventPickup(feature, droid)
//__
//__ An event that is run whenever a feature is picked up. It is called for
//__ all players / scripts.
//__ Careful passing the parameter object around, since it is about to vanish! (3.2+ only)
//__
bool triggerEventPickup(FEATURE *psFeat, DROID *psDroid)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *instance : scripts)
	{
		instance->handle_eventPickup(psFeat, psDroid);
	}
	return true;
}

//__ ## eventObjectSeen(viewer, seen)
//__
//__ An event that is run sometimes when an object, which was marked by an object label,
//__ which was reset through resetLabel() to subscribe for events, goes from not seen to seen.
//__ An event that is run sometimes when an objectm  goes from not seen to seen.
//__ First parameter is **game object** doing the seeing, the next the game
//__ object being seen.
//__
//__ ## eventGroupSeen(viewer, group)
//__
//__ An event that is run sometimes when a member of a group, which was marked by a group label,
//__ which was reset through resetLabel() to subscribe for events, goes from not seen to seen.
//__ First parameter is **game object** doing the seeing, the next the id of the group
//__ being seen.
//__
bool triggerEventSeen(BASE_OBJECT *psViewer, BASE_OBJECT *psSeen)
{
	return scripting_engine::instance().triggerEventSeen(psViewer, psSeen);
}
bool scripting_engine::triggerEventSeen(BASE_OBJECT *psViewer, BASE_OBJECT *psSeen)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	if (!psSeen || !psViewer) { return true; }
	for (auto *instance : scripts)
	{
		std::pair<bool, int> callbacks = scripting_engine::instance().seenLabelCheck(instance, psSeen, psViewer);
		if (callbacks.first)
		{
			instance->handle_eventObjectSeen(psViewer, psSeen);
		}
		if (callbacks.second)
		{
			int groupId = callbacks.second;
			instance->handle_eventGroupSeen(psViewer, groupId);
		}
	}
	return true;
}

//__ ## eventObjectTransfer(object, from)
//__
//__ An event that is run whenever an object is transferred between players,
//__ for example due to a Nexus Link weapon. The event is called after the
//__ object has been transferred, so the target player is in object.player.
//__ The event is called for both players.
//__
bool triggerEventObjectTransfer(BASE_OBJECT *psObj, int from)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	if (!psObj) { return true; }
	for (auto *instance : scripts)
	{
		int me = instance->player();
		bool receiveAll = instance->isReceivingAllEvents();
		if (me == psObj->player || me == from || receiveAll)
		{
			instance->handle_eventObjectTransfer(psObj, from);
		}
	}
	return true;
}

//__ ## eventChat(from, to, message)
//__
//__ An event that is run whenever a chat message is received. The ```from``` parameter is the
//__ player sending the chat message. For the moment, the ```to``` parameter is always the script
//__ player.
//__
bool triggerEventChat(int from, int to, const char *message)
{
	if (!scriptsReady)
	{
		// just ignore chat messages before scripts are ready / initialized
		return false;
	}
	ASSERT_OR_RETURN(false, message, "No message provided");
	for (auto *instance : scripts)
	{
		int me = instance->player();
		bool receiveAll = instance->isReceivingAllEvents();
		if (me == to || (receiveAll && to == from))
		{
			instance->handle_eventChat(from, to, message);
		}
	}
	return true;
}

//__ ## eventBeacon(x, y, from, to[, message])
//__
//__ An event that is run whenever a beacon message is received. The ```from``` parameter is the
//__ player sending the beacon. For the moment, the ```to``` parameter is always the script player.
//__ Message may be undefined.
//__
bool triggerEventBeacon(int from, int to, const char *message, int x, int y)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	optional<const char *> opt_message = (message) ? optional<const char *>(message) : nullopt;
	for (auto *instance : scripts)
	{
		int me = instance->player();
		bool receiveAll = instance->isReceivingAllEvents();
		if (me == to || receiveAll)
		{
			instance->handle_eventBeacon(map_coord(x), map_coord(y), from, to, opt_message);
		}
	}
	return true;
}

//__ ## eventBeaconRemoved(from, to)
//__
//__ An event that is run whenever a beacon message is removed. The ```from``` parameter is the
//__ player sending the beacon. For the moment, the ```to``` parameter is always the script player.
//__
bool triggerEventBeaconRemoved(int from, int to)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *instance : scripts)
	{
		int me = instance->player();
		bool receiveAll = instance->isReceivingAllEvents();
		if (me == to || receiveAll)
		{
			instance->handle_eventBeaconRemoved(from, to);
		}
	}
	return true;
}

//__ ## eventSelectionChanged(objects)
//__
//__ An event that is triggered whenever the host player selects one or more game objects.
//__ The ```objects``` parameter contains an array of the currently selected game objects.
//__ Keep in mind that the player may drag and drop select many units at once, select one
//__ unit specifically, or even add more selections to a current selection one at a time.
//__ This event will trigger once for each user action, not once for each selected or
//__ deselected object. If all selected game objects are deselected, ```objects``` will
//__ be empty.
//__
bool triggerEventSelected()
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	selectionChanged = true;
	return true;
}

//__ ## eventGroupLoss(object, groupId, newSize)
//__
//__ An event that is run whenever a group becomes empty. Input parameter
//__ is the about to be killed object, the group's id, and the new group size.
//__
// Since groups are entities local to one context, we do not iterate over them here.
bool triggerEventGroupLoss(const BASE_OBJECT *psObj, int group, int size, wzapi::scripting_instance *instance)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	return instance->handle_eventGroupLoss(psObj, group, size);
}

// This is not a trigger yet.
bool triggerEventDroidMoved(DROID *psDroid, int oldx, int oldy)
{
	return scripting_engine::instance().areaLabelCheck(psDroid);
}

//__ ## eventArea<label>(droid)
//__
//__ An event that is run whenever a droid enters an area label. The area is then
//__ deactived. Call resetArea() to reactivate it. The name of the event is
//__ `eventArea${label}`.
//__
bool triggerEventArea(const std::string& label, DROID *psDroid)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *instance : scripts)
	{
		instance->handle_eventArea(label, psDroid);
	}
	return true;
}

//__ ## eventDesignCreated(template)
//__
//__ An event that is run whenever a new droid template is created. It is only
//__ run on the client of the player designing the template.
//__
bool triggerEventDesignCreated(DROID_TEMPLATE *psTemplate)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *instance : scripts)
	{
		instance->handle_eventDesignCreated(psTemplate);
	}
	return true;
}

//__ ## eventAllianceOffer(from, to)
//__
//__ An event that is called whenever an alliance offer is requested.
//__
bool triggerEventAllianceOffer(uint8_t from, uint8_t to)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *instance : scripts)
	{
		instance->handle_eventAllianceOffer(from, to);
	}
	return true;
}

//__ ## eventAllianceAccepted(from, to)
//__
//__ An event that is called whenever an alliance is accepted.
//__
bool triggerEventAllianceAccepted(uint8_t from, uint8_t to)
{
	if (!scriptsReady)
	{
		return false; //silently ignore
	}
	for (auto *instance : scripts)
	{
		instance->handle_eventAllianceAccepted(from, to);
	}
	return true;
}

//__ ## eventAllianceBroken(from, to)
//__
//__ An event that is called whenever an alliance is broken.
//__
bool triggerEventAllianceBroken(uint8_t from, uint8_t to)
{
	if (!scriptsReady)
	{
		return false; //silently ignore
	}
	for (auto *instance : scripts)
	{
		instance->handle_eventAllianceBroken(from, to);
	}
	return true;
}

//__ ## eventSyncRequest(req_id, x, y, obj_id, obj_id2)
//__
//__ An event that is called from a script and synchronized with all other scripts and hosts
//__ to prevent desync from happening. Sync requests must be carefully validated to prevent
//__ cheating!
//__
bool triggerEventSyncRequest(int from, int req_id, int x, int y, BASE_OBJECT *psObj, BASE_OBJECT *psObj2)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *instance : scripts)
	{
		instance->handle_eventSyncRequest(from, req_id, x, y, psObj, psObj2);
	}
	return true;
}

//__ ## eventKeyPressed(meta, key)
//__
//__ An event that is called whenever user presses a key in the game, not counting chat
//__ or other pop-up user interfaces. The key values are currently undocumented.
bool triggerEventKeyPressed(int meta, int key)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *instance : scripts)
	{
		instance->handle_eventKeyPressed(meta, key);
	}
	return true;
}

// ----

#define ALL_PLAYERS -1
#define ALLIES -2
#define ENEMIES -3

scripting_engine& scripting_engine::instance()
{
	static scripting_engine engine = scripting_engine();
	return engine;
}

std::vector<scripting_engine::LabelInfo> scripting_engine::debug_GetLabelInfo() const
{
	std::vector<scripting_engine::LabelInfo> results;
	for (LABELMAP::const_iterator i = labels.cbegin(); i != labels.cend(); i++)
	{
		const LABEL &l = i->second;
		scripting_engine::LabelInfo labelInfo;
		labelInfo.label = WzString::fromUtf8(i->first);
		const char *c = "?";
		switch (l.type)
		{
		case OBJ_DROID: c = "DROID"; break;
		case OBJ_FEATURE: c = "FEATURE"; break;
		case OBJ_STRUCTURE: c = "STRUCTURE"; break;
		case SCRIPT_POSITION: c = "POSITION"; break;
		case SCRIPT_AREA: c = "AREA"; break;
		case SCRIPT_RADIUS: c = "RADIUS"; break;
		case SCRIPT_GROUP: c = "GROUP"; break;
		case SCRIPT_PLAYER:
		case SCRIPT_RESEARCH:
		case OBJ_PROJECTILE:
		case SCRIPT_COUNT: c = "ERROR"; break;
		}
		labelInfo.type = WzString::fromUtf8(c);
		switch (l.triggered)
		{
		case -1: labelInfo.trigger = "N/A"; break;
		case 0: labelInfo.trigger = "Active"; break;
		default: labelInfo.trigger = "Done"; break;
		}
		if (l.player == ALL_PLAYERS)
		{
			labelInfo.owner = "ALL";
		}
		else
		{
			labelInfo.owner = WzString::number(l.player);
		}
		if (l.subscriber == ALL_PLAYERS)
		{
			labelInfo.subscriber = "ALL";
		}
		else
		{
			labelInfo.subscriber = WzString::number(l.subscriber);
		}
		results.push_back(std::move(labelInfo));
	}
	return results;
}

void clearMarks()
{
	for (int x = 0; x < mapWidth; x++) // clear old marks
	{
		for (int y = 0; y < mapHeight; y++)
		{
			MAPTILE *psTile = mapTile(x, y);
			psTile->tileInfoBits &= ~BITS_MARKED;
		}
	}
}

void scripting_engine::markAllLabels(bool only_active)
{
	for (const auto &it : labels)
	{
		const auto &key = it.first;
		const LABEL &l = it.second;
		if (!only_active || l.triggered <= 0)
		{
			showLabel(key, false, false);
		}
	}
}

#include "display3d.h"

void scripting_engine::showLabel(const std::string &key, bool clear_old, bool jump_to)
{
	if (labels.count(key) == 0)
	{
		debug(LOG_ERROR, "label %s not found", key.c_str());
		return;
	}
	LABEL &l = labels[key];
	if (clear_old)
	{
		clearMarks();
	}
	if (l.type == SCRIPT_AREA || l.type == SCRIPT_POSITION)
	{
		if (jump_to)
		{
			setViewPos(map_coord(l.p1.x), map_coord(l.p1.y), false); // move camera position
		}
		int maxx = map_coord(l.p2.x);
		int maxy = map_coord(l.p2.y);
		if (l.type == SCRIPT_POSITION)
		{
			maxx = MIN(mapWidth, maxx + 1);
			maxy = MIN(mapHeight, maxy + 1);
		}
		for (int x = map_coord(l.p1.x); x < maxx; x++) // make new ones
		{
			for (int y = map_coord(l.p1.y); y < maxy; y++)
			{
				MAPTILE *psTile = mapTile(x, y);
				psTile->tileInfoBits |= BITS_MARKED;
			}
		}
	}
	else if (l.type == SCRIPT_RADIUS)
	{
		if (jump_to)
		{
			setViewPos(map_coord(l.p1.x), map_coord(l.p1.y), false); // move camera position
		}
		for (int x = MAX(map_coord(l.p1.x - l.p2.x), 0); x < MIN(map_coord(l.p1.x + l.p2.x), mapWidth); x++)
		{
			for (int y = MAX(map_coord(l.p1.y - l.p2.x), 0); y < MIN(map_coord(l.p1.y + l.p2.x), mapHeight); y++) // l.p2.x is radius, not a bug
			{
				if (iHypot(map_coord(l.p1) - Vector2i(x, y)) < map_coord(l.p2.x))
				{
					MAPTILE *psTile = mapTile(x, y);
					psTile->tileInfoBits |= BITS_MARKED;
				}
			}
		}
	}
	else if (l.type == OBJ_DROID || l.type == OBJ_FEATURE || l.type == OBJ_STRUCTURE)
	{
		BASE_OBJECT *psObj = IdToObject((OBJECT_TYPE)l.type, l.id, l.player);
		if (psObj)
		{
			if (jump_to)
			{
				setViewPos(map_coord(psObj->pos.x), map_coord(psObj->pos.y), false); // move camera position
			}
			MAPTILE *psTile = mapTile(map_coord(psObj->pos.x), map_coord(psObj->pos.y));
			psTile->tileInfoBits |= BITS_MARKED;
		}
	}
	else if (l.type == SCRIPT_GROUP)
	{
		bool cameraMoved = false;
		for (ENGINEMAP::iterator i = groups.begin(); i != groups.end(); ++i)
		{
			const GROUPMAP *pGroupMap = i->second;
			for (auto iter = pGroupMap->map().cbegin(); iter != pGroupMap->map().cend(); ++iter)
			{
				if (iter->second == l.id)
				{
					const BASE_OBJECT *psObj = iter->first;
					if (!cameraMoved && jump_to)
					{
						setViewPos(map_coord(psObj->pos.x), map_coord(psObj->pos.y), false); // move camera position
						cameraMoved = true;
					}
					MAPTILE *psTile = mapTile(map_coord(psObj->pos.x), map_coord(psObj->pos.y));
					psTile->tileInfoBits |= BITS_MARKED;
				}
			}
		}
	}
}

// The bool return value is true when an object callback needs to be called.
// The int return value holds group id when a group callback needs to be called, 0 otherwise.
std::pair<bool, int> scripting_engine::seenLabelCheck(wzapi::scripting_instance *instance, const BASE_OBJECT *seen, const BASE_OBJECT *viewer)
{
	GROUPMAP *psMap = getGroupMap(instance);
	ASSERT_OR_RETURN(std::make_pair(false, 0), psMap != nullptr, "Non-existent groupmap for engine");
	auto seenObjIt = psMap->map().find(seen);
	int groupId = (seenObjIt != psMap->map().end()) ? seenObjIt->second : 0;
	bool foundObj = false, foundGroup = false;
	for (auto &it : labels)
	{
		LABEL &l = it.second;
		if (l.triggered != 0 || !(l.subscriber == ALL_PLAYERS || l.subscriber == viewer->player))
		{
			continue;
		}

		// Don't let a seen game object ID which matches a group label ID to prematurely
		// trigger a group label.
		if (l.type != SCRIPT_GROUP && l.id == seen->id)
		{
			l.triggered = viewer->id; // record who made the discovery
			foundObj = true;
		}
		else if (l.type == SCRIPT_GROUP && l.id == groupId)
		{
			l.triggered = viewer->id; // record who made the discovery
			foundGroup = true;
		}
	}
	if (foundObj || foundGroup)
	{
		jsDebugUpdateLabels();
	}
	return std::make_pair(foundObj, foundGroup ? groupId : 0);
}

bool scripting_engine::areaLabelCheck(DROID *psDroid)
{
	int x = psDroid->pos.x;
	int y = psDroid->pos.y;
	bool activated = false;
	for (LABELMAP::iterator i = labels.begin(); i != labels.end(); i++)
	{
		LABEL &l = i->second;
		if (l.triggered == 0 && (l.subscriber == ALL_PLAYERS || l.subscriber == psDroid->player)
		    && ((l.type == SCRIPT_AREA && l.p1.x < x && l.p1.y < y && l.p2.x > x && l.p2.y > y)
		        || (l.type == SCRIPT_RADIUS && iHypot(l.p1 - psDroid->pos.xy()) < l.p2.x)))
		{
			// We're inside an untriggered area
			activated = true;
			l.triggered = psDroid->id;
			triggerEventArea(i->first, psDroid);
		}
	}
	if (activated)
	{
		jsDebugUpdateLabels();
	}
	return activated;
}

// ----------------------------------------------------------------------------------------
// Group system
//

scripting_engine::GROUPMAP* scripting_engine::getGroupMap(wzapi::scripting_instance *instance)
{
	auto groupIt = groups.find(instance);
	GROUPMAP *psMap = (groupIt != groups.end()) ? groupIt->second : nullptr;
	return psMap;
}

void scripting_engine::removeFromGroup(wzapi::scripting_instance *instance, GROUPMAP *psMap, const BASE_OBJECT *psObj)
{
	auto result = psMap->removeObjectFromGroup(psObj);
	if (result.has_value())
	{
		int groupId = result.value();
		const int newGroupSize = psMap->groupSize(groupId);
		instance->updateGroupSizes(groupId, newGroupSize);
		triggerEventGroupLoss(psObj, groupId, newGroupSize, instance);
	}
}

void scripting_engine::groupRemoveObject(const BASE_OBJECT *psObj)
{
	for (ENGINEMAP::iterator i = groups.begin(); i != groups.end(); ++i)
	{
		removeFromGroup(i->first, i->second, psObj);
	}
}

bool scripting_engine::groupAddObject(const BASE_OBJECT *psObj, int groupId, wzapi::scripting_instance *instance)
{
	ASSERT_OR_RETURN(false, psObj && instance, "Bad parameter");
	GROUPMAP *psMap = getGroupMap(instance);
	removeFromGroup(instance, psMap, psObj);
	psMap->insertObjectIntoGroup(psObj, groupId);
	instance->updateGroupSizes(groupId, psMap->groupSize(groupId));
	return true; // inserted
}

bool scripting_engine::loadGroup(wzapi::scripting_instance *instance, int groupId, int objId)
{
	BASE_OBJECT *psObj = IdToPointer(objId, ANYPLAYER);
	ASSERT_OR_RETURN(false, psObj, "Non-existent object %d in group %d in savegame", objId, groupId);
	return groupAddObject(psObj, groupId, instance);
}

bool scripting_engine::saveGroups(nlohmann::json &result, wzapi::scripting_instance *instance)
{
	// Save group info as a list of group memberships for each droid
	GROUPMAP *psMap = getGroupMap(instance);
	ASSERT_OR_RETURN(false, psMap, "Non-existent groupmap for engine");
	result["lastNewGroupId"] = psMap->getLastNewGroupId();
	for (auto i = psMap->map().begin(); i != psMap->map().end(); ++i)
	{
		const BASE_OBJECT *psObj = i->first;
		ASSERT(!isDead(psObj), "Wanted to save dead %s to savegame!", objInfo(psObj));
		std::vector<WzString> value = json_getValue(result, WzString::number(psObj->id)).toWzStringList();
		value.push_back(WzString::number(i->second));
		result[WzString::number(psObj->id).toUtf8()] = value;
	}
	return true;
}

// ----------------------------------------------------------------------------------------
// Label system (function defined in qtscript.h header)
//

bool loadLabels(const char *filename)
{
	return scripting_engine::instance().loadLabels(filename);
}

// Load labels
bool scripting_engine::loadLabels(const char *filename)
{
	int groupidx = -1;

	if (!PHYSFS_exists(filename))
	{
		debug(LOG_SAVE, "No %s found -- not adding any labels", filename);
		return false;
	}
	WzConfig ini(filename, WzConfig::ReadOnly);
	labels.clear();
	std::vector<WzString> list = ini.childGroups();
	debug(LOG_SAVE, "Loading %zu labels...", list.size());
	for (int i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		LABEL p;
		std::string label = ini.value("label").toWzString().toUtf8();
		if (labels.count(label) > 0)
		{
			debug(LOG_ERROR, "Duplicate label found");
		}
		else if (list[i].startsWith("position"))
		{
			p.p1 = ini.vector2i("pos");
			p.p2 = p.p1;
			p.type = SCRIPT_POSITION;
			p.player = ALL_PLAYERS;
			p.id = -1;
			p.triggered = ini.value("triggered", -1).toInt(); // deactivated by default
			p.subscriber = ALL_PLAYERS;
			labels[label] = p;
		}
		else if (list[i].startsWith("area"))
		{
			p.p1 = ini.vector2i("pos1");
			p.p2 = ini.vector2i("pos2");
			p.type = SCRIPT_AREA;
			p.player = ini.value("player", ALL_PLAYERS).toInt();
			p.triggered = ini.value("triggered", 0).toInt(); // activated by default
			p.id = -1;
			p.subscriber = ini.value("subscriber", ALL_PLAYERS).toInt();
			labels[label] = p;
		}
		else if (list[i].startsWith("radius"))
		{
			p.p1 = ini.vector2i("pos");
			p.p2.x = ini.value("radius").toInt();
			p.p2.y = 0; // unused
			p.type = SCRIPT_RADIUS;
			p.player = ini.value("player", ALL_PLAYERS).toInt();
			p.triggered = ini.value("triggered", 0).toInt(); // activated by default
			p.subscriber = ini.value("subscriber", ALL_PLAYERS).toInt();
			p.id = -1;
			labels[label] = p;
		}
		else if (list[i].startsWith("object"))
		{
			auto id = ini.value("id").toInt();
			const auto player = ini.value("player").toInt();
			const auto it = moduleToBuilding[player].find(id);
			if (it != moduleToBuilding[player].end())
			{
				// replace moduleId with its building id
				debug(LOG_NEVER, "replaced with %i;%i", id, it->second);
				id = it->second;
			}
			p.id = id;
			p.type = ini.value("type").toInt();
			p.player = player;
			p.triggered = ini.value("triggered", -1).toInt(); // deactivated by default
			p.subscriber = ini.value("subscriber", ALL_PLAYERS).toInt();
			labels[label] = p;
		}
		else if (list[i].startsWith("group"))
		{
			p.id = groupidx--;
			p.type = SCRIPT_GROUP;
			p.player = ini.value("player").toInt();
			std::vector<WzString> memberList = ini.value("members").toWzStringList();
			for (WzString const &j : memberList)
			{
				int id = j.toInt();
				BASE_OBJECT *psObj = IdToPointer(id, p.player);
				ASSERT(psObj, "Unit %d belonging to player %d not found from label %s",
				       id, p.player, list[i].toUtf8().c_str());
				p.idlist.push_back(id);
			}
			p.triggered = ini.value("triggered", -1).toInt(); // deactivated by default
			p.subscriber = ini.value("subscriber", ALL_PLAYERS).toInt();
			labels[label] = p;
		}
		else
		{
			debug(LOG_ERROR, "Misnamed group in %s", filename);
		}
		ini.endGroup();
	}
	return true;
}

bool writeLabels(const char *filename)
{
	return scripting_engine::instance().writeLabels(filename);
}

bool scripting_engine::writeLabels(const char *filename)
{
	int c[5]; // make unique, incremental section names
	memset(c, 0, sizeof(c));
	WzConfig ini(filename, WzConfig::ReadAndWrite);
	for (LABELMAP::const_iterator i = labels.begin(); i != labels.end(); i++)
	{
		const std::string& key = i->first;
		const LABEL &l = i->second;
		if (l.type == SCRIPT_POSITION)
		{
			ini.beginGroup("position_" + WzString::number(c[0]++));
			ini.setVector2i("pos", l.p1);
			ini.setValue("label", WzString::fromUtf8(key));
			ini.setValue("triggered", l.triggered);
			ini.endGroup();
		}
		else if (l.type == SCRIPT_AREA)
		{
			ini.beginGroup("area_" + WzString::number(c[1]++));
			ini.setVector2i("pos1", l.p1);
			ini.setVector2i("pos2", l.p2);
			ini.setValue("label", WzString::fromUtf8(key));
			ini.setValue("player", l.player);
			ini.setValue("triggered", l.triggered);
			ini.setValue("subscriber", l.subscriber);
			ini.endGroup();
		}
		else if (l.type == SCRIPT_RADIUS)
		{
			ini.beginGroup("radius_" + WzString::number(c[2]++));
			ini.setVector2i("pos", l.p1);
			ini.setValue("radius", l.p2.x);
			ini.setValue("label", WzString::fromUtf8(key));
			ini.setValue("player", l.player);
			ini.setValue("triggered", l.triggered);
			ini.setValue("subscriber", l.subscriber);
			ini.endGroup();
		}
		else if (l.type == SCRIPT_GROUP)
		{
			ini.beginGroup("group_" + WzString::number(c[3]++));
			ini.setValue("player", l.player);
			ini.setValue("triggered", l.triggered);
			std::vector<WzString> list;
			list.reserve(l.idlist.size());
			for (int val : l.idlist)
			{
				list.push_back(WzString::number(val));
			}
			ini.setValue("members", list);
			ini.setValue("label", WzString::fromUtf8(key));
			ini.setValue("subscriber", l.subscriber);
			ini.endGroup();
		}
		else
		{
			auto id = l.id;
			auto player = l.player;
			ini.beginGroup("object_" + WzString::number(c[4]++));
			ini.setValue("id", id);
			ini.setValue("player", player);
			ini.setValue("type", l.type);
			ini.setValue("label", WzString::fromUtf8(key));
			ini.setValue("triggered", l.triggered);
			ini.endGroup();
		}
	}
	return true;
}

//

#if defined(SCRIPT_ASSERT)
#undef SCRIPT_ASSERT
#endif
#define SCRIPT_ASSERT(retval, execution_context, expr, ...) \
	do { bool _wzeval = (expr); \
		if (!_wzeval) { debug(LOG_ERROR, __VA_ARGS__); \
			context.throwError(#expr, __LINE__, __FUNCTION__); \
			return retval; } } while (0)

#if defined(SCRIPT_ASSERT_PLAYER)
#undef SCRIPT_ASSERT_PLAYER
#endif
#define SCRIPT_ASSERT_PLAYER(retval, _context, _player) \
	SCRIPT_ASSERT(retval, _context, _player >= 0 && _player < MAX_PLAYERS, "Invalid player index %d", _player);

//-- ## resetLabel(labelName[, playerFilter])
//--
//-- Reset the trigger on an label. Next time a unit enters the area, it will trigger
//-- an area event. Next time an object or a group is seen, it will trigger a seen event.
//-- Optionally add a filter on it in the second parameter, which can
//-- be a specific player to watch for, or ```ALL_PLAYERS``` by default.
//-- This is a fast operation of O(log n) algorithmic complexity. (3.2+ only)
//--
//-- ## resetArea(labelName[, playerFilter])
//--
//-- Reset the trigger on an area. Next time a unit enters the area, it will trigger
//-- an area event. Optionally add a filter on it in the second parameter, which can
//-- be a specific player to watch for, or ```ALL_PLAYERS``` by default.
//-- This is a fast operation of O(log n) algorithmic complexity. DEPRECATED - use resetLabel instead. (3.2+ only)
//--
wzapi::no_return_value scripting_engine::resetLabel(WZAPI_PARAMS(std::string labelName, optional<int> playerFilter))
{
	LABELMAP& labels = scripting_engine::instance().labels;
	SCRIPT_ASSERT({}, context, labels.count(labelName) > 0, "Label %s not found", labelName.c_str());
	LABEL &label = labels[labelName];
	label.triggered = 0; // make active again
	label.subscriber = playerFilter.value_or(ALL_PLAYERS);
	return {};
}

//-- ## enumLabels([filterLabelType])
//--
//-- Returns a string list of labels that exist for this map. The optional filter
//-- parameter can be used to only return labels of one specific type. (3.2+ only)
//--
std::vector<std::string> scripting_engine::enumLabels(WZAPI_PARAMS(optional<int> filterLabelType))
{
	const LABELMAP& labels = scripting_engine::instance().labels;
	std::vector<std::string> matches;
	if (filterLabelType.has_value()) // filter
	{
		SCRIPT_TYPE type = (SCRIPT_TYPE)filterLabelType.value();
		for (LABELMAP::const_iterator i = labels.begin(); i != labels.end(); i++)
		{
			const LABEL &label = i->second;
			if (label.type == type)
			{
				matches.push_back(i->first);
			}
		}
	}
	else // fast path, give all
	{
		matches.reserve(labels.size());
		for (LABELMAP::const_iterator i = labels.begin(); i != labels.end(); i++)
		{
			matches.push_back(i->first);
		}
	}
	return matches;
}

generic_script_object::generic_script_object()
: p1(Vector2i(0,0))
, p2(Vector2i(0,0))
, id(-1)
, player(-1)
, type(-1)
{ }

generic_script_object generic_script_object::Null()
{
	return generic_script_object();
}

generic_script_object generic_script_object::fromRadius(int x, int y, int radius)
{
	generic_script_object result;
	result.type = SCRIPT_RADIUS;
	result.p1.x = x;
	result.p1.y = y;
	result.p2.x = radius;
	return result;
}

scr_radius generic_script_object::getRadius() const // if type == SCRIPT_RADIUS, returns the radius
{
	ASSERT(isRadius(), "generic_script_object is not a radius; type: %d", type);
	return scr_radius {p1.x, p1.y, p2.x};
}

generic_script_object generic_script_object::fromArea(int x, int y, int x2, int y2)
{
	generic_script_object result;
	result.type = SCRIPT_AREA;
	result.p1.x = x;
	result.p1.y = y;
	result.p2.x = x2;
	result.p2.y = y2;
	return result;
}

scr_area generic_script_object::getArea() const // if type == SCRIPT_AREA, returns the area
{
	ASSERT(isArea(), "generic_script_object is not an area; type: %d", type);
	return scr_area {p1.x, p1.y, p2.x, p2.y};
}

generic_script_object generic_script_object::fromPosition(int x, int y)
{
	generic_script_object result;
	result.type = SCRIPT_POSITION;
	result.p1.x = x;
	result.p1.y = y;
	return result;
}

scr_position generic_script_object::getPosition() const // if type == SCRIPT_POSITION, returns the position
{
	ASSERT(isPosition(), "generic_script_object is not a position; type: %d", type);
	return scr_position {p1.x, p1.y};
}

generic_script_object generic_script_object::fromGroup(int groupId)
{
	generic_script_object result;
	result.type = SCRIPT_GROUP;
	result.id = groupId;
	return result;
}

int generic_script_object::getGroupId() const // if type == SCRIPT_GROUP, returns the groupId
{
	ASSERT(isGroup(), "generic_script_object is not a group; type: %d", type);
	return id;
}

generic_script_object generic_script_object::fromObject(const BASE_OBJECT *psObj)
{
	generic_script_object result;
	if (psObj == nullptr)
	{
		return generic_script_object::Null();
	}
	result.type = psObj->type;
	result.id = psObj->id;
	result.player = psObj->player;
	return result;
}

BASE_OBJECT * generic_script_object::getObject() const // if type == OBJ_DROID, OBJ_FEATURE, OBJ_STRUCTURE, returns the game object
{
	ASSERT(isObject(), "generic_script_object is not an object; type: %d", type);
	return IdToObject((OBJECT_TYPE)type, id, player);
}

LABEL generic_script_object::toNewLabel() const
{
	LABEL value;
	value.type = type;
	value.id = id;
	value.player = player;
	value.p1.x = world_coord(p1.x);
	value.p1.y = world_coord(p1.y);
	if (type == SCRIPT_AREA)
	{
		value.p2.x = world_coord(p2.x);
		value.p2.y = world_coord(p2.y);
	}
	else if (type == SCRIPT_RADIUS)
	{
		value.p2.x = world_coord(p2.x);
	}

	value.triggered = -1; // default off

	return value;
}

//-- ## addLabel(object, label[, triggered])
//--
//-- Add a label to a game object. If there already is a label by that name, it is overwritten.
//-- This is a fast operation of O(log n) algorithmic complexity. (3.2+ only)
//-- Can optionally specify an initial "triggered" value for the label. (3.4+ only)
//--
wzapi::no_return_value scripting_engine::addLabel(WZAPI_PARAMS(generic_script_object object, std::string label, optional<int> _triggered))
{
	LABELMAP& labels = scripting_engine::instance().labels;
	LABEL value = object.toNewLabel();

	if (value.type == OBJ_DROID || value.type == OBJ_STRUCTURE || value.type == OBJ_FEATURE)
	{
		BASE_OBJECT *psObj = IdToObject((OBJECT_TYPE)value.type, value.id, value.player);
		SCRIPT_ASSERT({}, context, psObj, "Object id %d not found belonging to player %d", value.id, value.player);
	}

	value.triggered = -1; // default off
	if (_triggered.has_value())
	{
		SCRIPT_ASSERT({}, context, value.type != SCRIPT_POSITION, "Cannot assign a trigger to a position");
		value.triggered = _triggered.value();
	}

	labels[label] = value;
	jsDebugUpdateLabels();
	return {};
}

//-- ## removeLabel(label)
//--
//-- Remove a label from the game. Returns the number of labels removed, which should normally be
//-- either 1 (label found) or 0 (label not found). (3.2+ only)
//--
int scripting_engine::removeLabel(WZAPI_PARAMS(std::string label))
{
	LABELMAP& labels = scripting_engine::instance().labels;
	int result = labels.erase(label);
	jsDebugUpdateLabels();
	return result;
}

//-- ## getLabel(object)
//--
//-- Get a label string belonging to a game object. If the object has multiple labels, only the first
//-- label found will be returned. If the object has no labels, undefined is returned.
//-- This is a relatively slow operation of O(n) algorithmic complexity. (3.2+ only)
//--
optional<std::string> scripting_engine::getLabel(WZAPI_PARAMS(const BASE_OBJECT *psObj))
{
	ASSERT_OR_RETURN(nullopt, psObj, "No valid object provided");
	wzapi::game_object_identifier tmp;
	tmp.id = psObj->id;
	tmp.player = psObj->player;
	tmp.type = psObj->type;
	return _findMatchingLabel(tmp);
}
optional<std::string> scripting_engine::getLabelJS(WZAPI_PARAMS(wzapi::game_object_identifier obj_id))
{
	return _findMatchingLabel(obj_id);
}
optional<std::string> scripting_engine::_findMatchingLabel(wzapi::game_object_identifier obj_id)
{
	const LABELMAP& labels = scripting_engine::instance().labels;
	LABEL value;
	value.id = obj_id.id;
	value.player = obj_id.player;
	value.type = obj_id.type;
	debug(LOG_NEVER, "looking for label %i;%i;%i", obj_id.id, obj_id.player, obj_id.type);
	std::string label;
	for (const auto &it : labels)
	{
		if (it.second == value)
		{
			label = it.first;
			break;
		}
	}

	optional<std::string> retLabel;
	if (!label.empty())
	{
		retLabel = std::string(label);
	}
	return retLabel;
}

//-- ## getObject(label | x, y | type, player, id)
//--
//-- Fetch something denoted by a label, a map position or its object ID. A label refers to an area,
//-- a position or a **game object** on the map defined using the map editor and stored
//-- together with the map. In this case, the only argument is a text label. The function
//-- returns an object that has a type variable defining what it is (in case this is
//-- unclear). This type will be one of DROID, STRUCTURE, FEATURE, AREA, GROUP or POSITION.
//-- The AREA has defined 'x', 'y', 'x2', and 'y2', while POSITION has only defined 'x' and 'y'.
//-- The GROUP type has defined 'type' and 'id' of the group, which can be passed to enumGroup().
//-- This is a fast operation of O(log n) algorithmic complexity. If the label is not found, an
//-- undefined value is returned. If whatever object the label should point at no longer exists,
//-- a null value is returned.
//--
//-- You can also fetch a STRUCTURE or FEATURE type game object from a given map position (if any).
//-- This is a very fast operation of O(1) algorithmic complexity. Droids cannot be fetched in this
//-- manner, since they do not have a unique placement on map tiles. Finally, you can fetch an object using
//-- its ID, in which case you need to pass its type, owner and unique object ID. This is an
//-- operation of O(n) algorithmic complexity. (3.2+ only)
//--
generic_script_object scripting_engine::getObject(WZAPI_PARAMS(wzapi::object_request request))
{
	if (request.requestType == wzapi::object_request::RequestType::MAPPOS_REQUEST)
	{
		auto pos = request.getMapPosition();
		int x = pos.x;
		int y = pos.y;
		SCRIPT_ASSERT({}, context, tileOnMap(x, y), "Map position (%d, %d) not on the map!", x, y);
		const MAPTILE *psTile = mapTile(x, y);
		return generic_script_object::fromObject(psTile->psObject);
	}
	else if (request.requestType == wzapi::object_request::RequestType::OBJECTID_REQUEST)
	{
		auto type_player_id = request.getObjectIDRequest();
		OBJECT_TYPE type = std::get<0>(type_player_id);
		int player = std::get<1>(type_player_id);
		int id = std::get<2>(type_player_id);
		SCRIPT_ASSERT_PLAYER({}, context, player);
		return generic_script_object::fromObject(IdToObject(type, id, player));
	}
	else if (request.requestType == wzapi::object_request::RequestType::LABEL_REQUEST)
	{
		return scripting_engine::instance().getObjectFromLabel(context, request.getLabel());
	}
	return {};
}

generic_script_object scripting_engine::getObjectFromLabel(WZAPI_PARAMS(const std::string& label))
{
	// get by label case
	BASE_OBJECT *psObj = nullptr;
	if (labels.count(label) > 0)
	{
		const LABEL &p = labels[label];
		switch (p.type)
		{
		case SCRIPT_RADIUS:
			return generic_script_object::fromRadius(map_coord(p.p1.x), map_coord(p.p1.y), map_coord(p.p2.x));
			break;
		case SCRIPT_AREA:
			return generic_script_object::fromArea(map_coord(p.p1.x), map_coord(p.p1.y), map_coord(p.p2.x), map_coord(p.p2.y));
			break;
		case SCRIPT_POSITION:
			return generic_script_object::fromPosition(map_coord(p.p1.x), map_coord(p.p1.y));
			break;
		case SCRIPT_GROUP:
			return generic_script_object::fromGroup(p.id);
			break;
		case OBJ_DROID:
		case OBJ_FEATURE:
		case OBJ_STRUCTURE:
			psObj = IdToObject((OBJECT_TYPE)p.type, p.id, p.player);
			if (!psObj)
			{
				return generic_script_object::Null();
			}
			return generic_script_object::fromObject(psObj);
		default:
			SCRIPT_ASSERT({}, context, false, "Bad object label type found for label %s!", label.c_str());
			break;
		}
	}
	return generic_script_object::Null();
}

//-- ## enumArea(<x1, y1, x2, y2 | label>[, playerFilter[, seen]])
//--
//-- Returns an array of game objects seen within the given area that passes the optional filter
//-- which can be one of a player index, ```ALL_PLAYERS```, ```ALLIES``` or ```ENEMIES```. By default, filter is
//-- ```ALL_PLAYERS```. Finally an optional parameter can specify whether only visible objects should be
//-- returned; by default only visible objects are returned. The label can either be actual
//-- positions or a label to an AREA. Calling this function is much faster than iterating over all
//-- game objects using other enum functions. (3.2+ only)
//--
std::vector<const BASE_OBJECT *> scripting_engine::enumAreaByLabel(WZAPI_PARAMS(std::string label, optional<int> _playerFilter, optional<bool> _seen))
{
	SCRIPT_ASSERT({}, context, instance().labels.count(label) > 0, "Label %s not found", label.c_str());
	const LABEL &p = instance().labels[label];
	SCRIPT_ASSERT({}, context, p.type == SCRIPT_AREA, "Wrong label type for %s", label.c_str());
	int x1 = p.p1.x;
	int y1 = p.p1.y;
	int x2 = p.p2.x;
	int y2 = p.p2.y;
	return _enumAreaWorldCoords(context, x1, y1, x2, y2, _playerFilter, _seen);
}

typedef std::vector<BASE_OBJECT *> GridList;
#include "mapgrid.h"

std::vector<const BASE_OBJECT *> scripting_engine::enumArea(WZAPI_PARAMS(scr_area area, optional<int> _playerFilter, optional<bool> _seen))
{
	int x1 = world_coord(area.x1);
	int y1 = world_coord(area.y1);
	int x2 = world_coord(area.x2);
	int y2 = world_coord(area.y2);
	return _enumAreaWorldCoords(context, x1, y1, x2, y2, _playerFilter, _seen);
}

std::vector<const BASE_OBJECT *> scripting_engine::_enumAreaWorldCoords(WZAPI_PARAMS(int x1, int y1, int x2, int y2, optional<int> _playerFilter, optional<bool> _seen))
{
	int player = context.player();
	int playerFilter = _playerFilter.value_or(ALL_PLAYERS);
	bool seen = _seen.value_or(true);

	static GridList gridList;  // static to avoid allocations. // not thread-safe
	gridList = gridStartIterateArea(x1, y1, x2, y2);
	std::vector<const BASE_OBJECT *> list;
	for (GridIterator gi = gridList.begin(); gi != gridList.end(); ++gi)
	{
		BASE_OBJECT *psObj = *gi;
		if ((psObj->visible[player] || !seen) && !psObj->died)
		{
			if ((playerFilter >= 0 && psObj->player == playerFilter) || playerFilter == ALL_PLAYERS
			    || (playerFilter == ALLIES && psObj->type != OBJ_FEATURE && aiCheckAlliances(psObj->player, player))
			    || (playerFilter == ENEMIES && psObj->type != OBJ_FEATURE && !aiCheckAlliances(psObj->player, player)))
			{
				list.push_back(psObj);
			}
		}
	}
	return list;
}

std::vector<const BASE_OBJECT *> scripting_engine::enumAreaJS(WZAPI_PARAMS(scripting_engine::area_by_values_or_area_label_lookup area_lookup, optional<int> playerFilter, optional<bool> seen))
{
	if (area_lookup.isLabel())
	{
		auto optLabel = area_lookup.label();
		ASSERT(optLabel.has_value(), "optional label is null");
		return enumAreaByLabel(context, optLabel.value(), playerFilter, seen);
	}
	else
	{
		auto optArea = area_lookup.area();
		ASSERT(optArea.has_value(), "optional area is null");
		return enumArea(context, optArea.value(), playerFilter, seen);
	}
}

//-- ## enumGroup(groupId)
//--
//-- Return an array containing all the members of a given group.
//--
std::vector<const BASE_OBJECT *> scripting_engine::enumGroup(WZAPI_PARAMS(int groupId))
{
	std::vector<const BASE_OBJECT *> matches;
	GROUPMAP *psMap = instance().getGroupMap(context.currentInstance());

	if (psMap != nullptr)
	{
		matches = psMap->getGroupObjects(groupId);
	}
	return matches;
}

//-- ## newGroup()
//--
//-- Allocate a new group. Returns its numerical ID. Deprecated since 3.2 - you should now
//-- use your own number scheme for groups.
//--
int scripting_engine::newGroup(WZAPI_NO_PARAMS)
{
	GROUPMAP *psMap = instance().getGroupMap(context.currentInstance());
	int i = psMap->newGroupID();
	// NOTE: group zero is reserved
	SCRIPT_ASSERT(1, context, i != 0, "Group 0 is reserved - error in WZ code");
	return i;
}

//-- ## groupAddArea(groupId, x1, y1, x2, y2)
//--
//-- Add any droids inside the given area to the given group. (3.2+ only)
//--
wzapi::no_return_value scripting_engine::groupAddArea(WZAPI_PARAMS(int groupId, int _x1, int _y1, int _x2, int _y2))
{
	int player = context.player();
	int x1 = world_coord(_x1);
	int y1 = world_coord(_y1);
	int x2 = world_coord(_x2);
	int y2 = world_coord(_y2);

	for (DROID *psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->pos.x >= x1 && psDroid->pos.x <= x2 && psDroid->pos.y >= y1 && psDroid->pos.y <= y2)
		{
			scripting_engine::instance().groupAddObject(psDroid, groupId, context.currentInstance());
		}
	}
	return {};
}

//-- ## groupAddDroid(groupId, droid)
//--
//-- Add given droid to given group. Deprecated since 3.2 - use groupAdd() instead.
//--
wzapi::no_return_value scripting_engine::groupAddDroid(WZAPI_PARAMS(int groupId, const DROID *psDroid))
{
	SCRIPT_ASSERT({}, context, psDroid, "No valid droid provided");
	scripting_engine::instance().groupAddObject(psDroid, groupId, context.currentInstance());
	return {};
}

//-- ## groupAdd(groupId, object)
//--
//-- Add given game object to the given group.
//--
wzapi::no_return_value scripting_engine::groupAdd(WZAPI_PARAMS(int groupId, const BASE_OBJECT *psObj))
{
	SCRIPT_ASSERT({}, context, psObj, "No valid object provided");
	scripting_engine::instance().groupAddObject(psObj, groupId, context.currentInstance());
	return {};
}

//-- ## groupSize(groupId)
//--
//-- Return the number of droids currently in the given group. Note that you can use groupSizes[] instead.
//--
int scripting_engine::groupSize(WZAPI_PARAMS(int groupId))
{
	auto groupMap = instance().getGroupMap(context.currentInstance());
	ASSERT_OR_RETURN(0, groupMap, "Failed to obtain groupMap");
	return groupMap->groupSize(groupId);
}

// ----------------------------------------------------------------------------------------
// Register functions with scripting system

bool scripting_engine::unregisterFunctions(wzapi::scripting_instance *instance)
{
	int num = 0;
	auto it = groups.find(instance);
	if (it != groups.end())
	{
		GROUPMAP *psMap = groups.at(instance);
		groups.erase(it);
		delete psMap;
		num = 1;
	}
	ASSERT(num == 1, "Number of engines removed from group map is %d!", num);
	labels.clear();
	return true;
}

// Call this just before launching game; we can't do everything in registerFunctions()
// since all game state may not be fully loaded by then
void scripting_engine::prepareLabels()
{
	// load the label group data into every scripting context, with the same negative group id
	for (ENGINEMAP::iterator iter = groups.begin(); iter != groups.end(); ++iter)
	{
		wzapi::scripting_instance *instance = iter->first;
		for (LABELMAP::iterator i = labels.begin(); i != labels.end(); ++i)
		{
			const LABEL &l = i->second;
			if (l.type == SCRIPT_GROUP)
			{
				for (std::vector<int>::const_iterator j = l.idlist.begin(); j != l.idlist.end(); j++)
				{
					int id = (*j);
					BASE_OBJECT *psObj = IdToPointer(id, l.player);
					ASSERT(psObj, "Unit %d belonging to player %d not found", id, l.player);
					if (psObj)
					{
						groupAddObject(psObj, l.id, instance);
					}
				}
			}
		}
	}
	jsDebugUpdateLabels();
}

wzapi::no_return_value scripting_engine::hackMarkTiles_ByLabel(WZAPI_PARAMS(const std::string& label))
{
	SCRIPT_ASSERT({}, context, labels.count(label) > 0, "Label %s not found", label.c_str());

	const LABEL &l = labels[label];
	if (l.type == SCRIPT_AREA)
	{
		for (int x = map_coord(l.p1.x); x < map_coord(l.p2.x); x++)
		{
			for (int y = map_coord(l.p1.y); y < map_coord(l.p2.y); y++)
			{
				MAPTILE *psTile = mapTile(x, y);
				psTile->tileInfoBits |= BITS_MARKED;
			}
		}
	}
	else if (l.type == SCRIPT_RADIUS)
	{
		for (int x = map_coord(l.p1.x - l.p2.x); x < map_coord(l.p1.x + l.p2.x); x++)
		{
			for (int y = map_coord(l.p1.y - l.p2.x); y < map_coord(l.p1.y + l.p2.x); y++)
			{
				if (x >= -1 && x < mapWidth + 1 && y >= -1 && y < mapWidth + 1 && iHypot(map_coord(l.p1) - Vector2i(x, y)) < map_coord(l.p2.x))
				{
					MAPTILE *psTile = mapTile(x, y);
					psTile->tileInfoBits |= BITS_MARKED;
				}
			}
		}
	}

	return {};
}

void scripting_engine::logFunctionPerformance(wzapi::scripting_instance *instance, const std::string &function, int ticks)
{
	MONITOR *monitor = monitors.at(instance); // pick right one for this instance
	MONITOR_BIN m;
	MONITOR::iterator it = monitor->find(function);
	if (it != monitor->end())
	{
		m = it->second;
	}
	if (ticks > MAX_US)
	{
		debug(LOG_SCRIPT, "%s took %dus at time %d", function.c_str(), ticks, wzGetTicks());
		m.overMaxTimeCalls++;
	}
	else if (ticks > HALF_MAX_US)
	{
		m.overHalfMaxTimeCalls++;
	}
	m.calls++;
	if (ticks > m.worst)
	{
		m.worst = ticks;
		m.worstGameTime = gameTime;
	}
	m.time += ticks;
	(*monitor)[function] = m;
}

// MARK: - DebugInterface

std::unordered_map<wzapi::scripting_instance *, nlohmann::json> scripting_engine::DebugInterface::debug_GetGlobalsSnapshot() const
{
	return scripting_engine::instance().debug_GetGlobalsSnapshot();
}
std::vector<scripting_engine::timerNodeSnapshot> scripting_engine::DebugInterface::debug_GetTimersSnapshot() const
{
	return scripting_engine::instance().debug_GetTimersSnapshot();
}
std::vector<scripting_engine::LabelInfo> scripting_engine::DebugInterface::debug_GetLabelInfo() const
{
	return scripting_engine::instance().debug_GetLabelInfo();
}
/// Show all labels or all currently active labels
void scripting_engine::DebugInterface::markAllLabels(bool only_active)
{
	return scripting_engine::instance().markAllLabels(only_active);
}
/// Mark and show label
void scripting_engine::DebugInterface::showLabel(const std::string &key, bool clear_old /*= true*/, bool jump_to /*= true*/)
{
	return scripting_engine::instance().showLabel(key, clear_old, jump_to);
}
