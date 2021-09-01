/*
	This file is part of Warzone 2100.
	Copyright (C) 2011-2020  Warzone 2100 Project

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
 * @file qtscriptfuncs.cpp
 *
 * New scripting system -- script functions
 */

#include "lib/framework/wzapp.h"
#include "lib/framework/wzconfig.h"
#include "lib/framework/wzpaths.h"
#include "lib/framework/fixedpoint.h"
#include "lib/framework/file.h"
#include "lib/sound/audio.h"
#include "lib/sound/cdaudio.h"
#include "lib/netplay/netplay.h"
#include "qtscript.h"
#include "lib/ivis_opengl/tex.h"

#include "action.h"
#include "clparse.h"
#include "combat.h"
#include "console.h"
#include "design.h"
#include "display3d.h"
#include "map.h"
#include "mission.h"
#include "move.h"
#include "order.h"
#include "transporter.h"
#include "message.h"
#include "display3d.h"
#include "intelmap.h"
#include "hci.h"
#include "wrappers.h"
#include "challenge.h"
#include "research.h"
#include "multilimit.h"
#include "multigifts.h"
#include "multimenu.h"
#include "template.h"
#include "lighting.h"
#include "radar.h"
#include "random.h"
#include "frontend.h"
#include "loop.h"
#include "gateway.h"
#include "mapgrid.h"
#include "lighting.h"
#include "atmos.h"
#include "warcam.h"
#include "projectile.h"
#include "component.h"
#include "seqdisp.h"
#include "ai.h"
#include "advvis.h"
#include "loadsave.h"
#include "wzapi.h"
#include "order.h"
#include "chat.h"
#include "scores.h"
#include "data.h"

#include <list>

/// Assert for scripts that give useful backtraces and other info.
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

#define ALL_PLAYERS -1
#define ALLIES -2
#define ENEMIES -3


BASE_OBJECT *IdToObject(OBJECT_TYPE type, int id, int player)
{
	switch (type)
	{
	case OBJ_DROID: return IdToDroid(id, player);
	case OBJ_FEATURE: return IdToFeature(id, player);
	case OBJ_STRUCTURE: return IdToStruct(id, player);
	default: return nullptr;
	}
}

wzapi::scripting_instance::scripting_instance(int player, const std::string& scriptName, const std::string& scriptPath)
: m_player(player)
, m_scriptName(scriptName)
, m_scriptPath(scriptPath)
{ }
wzapi::scripting_instance::~scripting_instance()
{ }

// Loads a file.
// (Intended for use from implementations of things like "include" functions.)
//
// Lookup order is as follows (based on the value of `searchFlags`):
// - 1.) The filePath is checked relative to the read-only data dir search paths (LoadFileSearchOptions::DataDir)
// - 2.) The filePath is checked relative to "<user's config dir>/script/" (LoadFileSearchOptions::ConfigScriptDir)
// - 3.) The filename *only* is checked relative to the main scriptPath (LoadFileSearchOptions::ScriptPath_FileNameOnlyBackwardsCompat) - for backwards-compat only
// - 4.) The filePath is checked relative to the main scriptPath (LoadFileSearchOptions::ScriptPath)
bool wzapi::scripting_instance::loadFileForInclude(const std::string& filePath, std::string& loadedFilePath, char **ppFileData, UDWORD *pFileSize, uint32_t searchFlags /*= LoadFileSearchOptions::All*/)
{
	WzPathInfo filePathInfo = WzPathInfo::fromPlatformIndependentPath(filePath);
	std::string path;

	if (path.empty() && (searchFlags & LoadFileSearchOptions::DataDir))
	{
		if (PHYSFS_exists(filePathInfo.filePath().c_str()))
		{
			path = filePathInfo.filePath(); // use this path instead (from read-only data dir)
		}
	}
	if (path.empty() && (searchFlags & LoadFileSearchOptions::ConfigScriptDir))
	{
		if (PHYSFS_exists((std::string("scripts/") + filePathInfo.filePath()).c_str()))
		{
			path = "scripts/" + filePathInfo.filePath(); // use this path instead (in user write dir)
		}
	}
	if (path.empty() && (searchFlags & LoadFileSearchOptions::ScriptPath_FileNameOnlyBackwardsCompat))
	{
		// to provide backwards-compat, start by checking the scriptPath for the passed-in filename *only*
		path = scriptPath() + "/" + filePathInfo.fileName();
		if (!PHYSFS_exists(path.c_str()))
		{
			path.clear();
		}
	}
	if (path.empty() && (searchFlags & LoadFileSearchOptions::ScriptPath))
	{
		path = scriptPath() + "/" + filePathInfo.filePath();
		if (!PHYSFS_exists(path.c_str()))
		{
			path.clear();
		}
	}
	if (path.empty())
	{
		debug(LOG_ERROR, "Failed to find file: %s", filePath.c_str());
		*ppFileData = nullptr;
		*pFileSize = 0;
		return false;
	}
	if (!::loadFile(path.c_str(), ppFileData, pFileSize))
	{
		debug(LOG_ERROR, "Failed to read file \"%s\" (name=\"%s\")", path.c_str(), filePathInfo.filePath().c_str());
		*ppFileData = nullptr;
		*pFileSize = 0;
		return false;
	}
	calcDataHash(reinterpret_cast<const uint8_t *>(*ppFileData), *pFileSize, DATA_SCRIPT);

	loadedFilePath = path;
	return true;
}

std::unordered_map<std::string, wzapi::scripting_instance::DebugSpecialStringType> wzapi::scripting_instance::debugGetScriptGlobalSpecialStringValues()
{
	return {};
}
void wzapi::scripting_instance::dumpScriptLog(const std::string &info)
{
	dumpScriptLog(info, player());
}
void wzapi::scripting_instance::dumpScriptLog(const std::string &info, int me)
{
	WzString path = PHYSFS_getWriteDir();
	path += WzString("/logs/") + WzString::fromUtf8(scriptName()) + "." + WzString::number(me) + ".log";
	FILE *fp = fopen(path.toUtf8().c_str(), "a"); // TODO: This will fail for unicode paths on Windows. Should use PHYSFS to open / write files
	if (fp)
	{
		fputs(info.c_str(), fp);
		fclose(fp);
	}
}

wzapi::execution_context_base::~execution_context_base()
{ }
wzapi::execution_context::~execution_context()
{ }
int wzapi::execution_context::player() const
{
	return currentInstance()->player();
}
void wzapi::execution_context::set_isReceivingAllEvents(bool value) const
{
	return currentInstance()->setReceiveAllEvents(value);
}
bool wzapi::execution_context::get_isReceivingAllEvents() const
{
	return currentInstance()->isReceivingAllEvents();
}

wzapi::object_request::object_request()
: requestType(wzapi::object_request::RequestType::INVALID_REQUEST)
{}
wzapi::object_request::object_request(const std::string& label)
: requestType(wzapi::object_request::RequestType::LABEL_REQUEST)
, str(label)
{}
wzapi::object_request::object_request(int x, int y)
: requestType(wzapi::object_request::RequestType::MAPPOS_REQUEST)
, val1(x)
, val2(y)
{}
wzapi::object_request::object_request(OBJECT_TYPE type, int player, int id)
: requestType(wzapi::object_request::RequestType::OBJECTID_REQUEST)
, val1(type)
, val2(player)
, val3(id)
{}

const std::string& wzapi::object_request::getLabel() const
{
	ASSERT(requestType == wzapi::object_request::RequestType::LABEL_REQUEST, "Not a label request");
	return str;
}
scr_position wzapi::object_request::getMapPosition() const
{
	ASSERT(requestType == wzapi::object_request::RequestType::MAPPOS_REQUEST, "Not a map position request");
	return scr_position{val1, val2};
}
std::tuple<OBJECT_TYPE, int, int> wzapi::object_request::getObjectIDRequest() const
{
	ASSERT(requestType == wzapi::object_request::RequestType::OBJECTID_REQUEST, "Not an object ID request");
	return std::tuple<OBJECT_TYPE, int, int>{(OBJECT_TYPE)val1, val2, val3};
}

// MARK: - Labels



// MARK: -

//-- ## _(string)
//--
//-- Mark string for translation.
//--
std::string wzapi::translate(WZAPI_PARAMS(std::string str))
{
	return std::string(gettext(str.c_str()));
}

//-- ## syncRandom(limit)
//--
//-- Generate a synchronized random number in range 0...(limit - 1) that will be the same if this function is
//-- run on all network peers in the same game frame. If it is called on just one peer (such as would be
//-- the case for AIs, for instance), then game sync will break. (3.2+ only)
//--
int32_t wzapi::syncRandom(WZAPI_PARAMS(uint32_t limit))
{
	return gameRand(limit);
}

//-- ## setAlliance(player1, player2, value)
//--
//-- Set alliance status between two players to either true or false. (3.2+ only)
//--
bool wzapi::setAlliance(WZAPI_PARAMS(int player1, int player2, bool value))
{
	if (value)
	{
		formAlliance(player1, player2, true, false, true);
	}
	else
	{
		breakAlliance(player1, player2, true, true);
	}
	return true;
}

//-- ## sendAllianceRequest(player)
//--
//-- Send an alliance request to a player. (3.3+ only)
//--
wzapi::no_return_value wzapi::sendAllianceRequest(WZAPI_PARAMS(int player2))
{
	if (!alliancesFixed(game.alliance))
	{
		requestAlliance(context.player(), player2, true, true);
	}
	return wzapi::no_return_value();
}

//-- ## orderDroid(droid, order)
//--
//-- Give a droid an order to do something. (3.2+ only)
//--
bool wzapi::orderDroid(WZAPI_PARAMS(DROID* psDroid, int order))
{
	SCRIPT_ASSERT(false, context, psDroid, "No valid droid provided");
	SCRIPT_ASSERT(false, context, order == DORDER_HOLD || order == DORDER_RTR || order == DORDER_STOP
	              || order == DORDER_RTB || order == DORDER_REARM || order == DORDER_RECYCLE,
	              "Invalid order: %s", getDroidOrderName((DROID_ORDER)order));

	DROID_ORDER_DATA *droidOrder = &psDroid->order;
	if (droidOrder->type == order)
	{
		return true;
	}
	if (order == DORDER_REARM)
	{
		if (STRUCTURE *psStruct = findNearestReArmPad(psDroid, psDroid->psBaseStruct, false))
		{
			orderDroidObj(psDroid, (DROID_ORDER)order, psStruct, ModeQueue);
		}
		else
		{
			orderDroid(psDroid, DORDER_RTB, ModeQueue);
		}
	}
	else
	{
		orderDroid(psDroid, (DROID_ORDER)order, ModeQueue);
	}
	return true;
}

//-- ## orderDroidBuild(droid, order, structure type, x, y[, direction])
//--
//-- Give a droid an order to build something at the given position. Returns true if allowed.
//--
bool wzapi::orderDroidBuild(WZAPI_PARAMS(DROID* psDroid, int order, std::string statName, int x, int y, optional<float> _direction))
{
	SCRIPT_ASSERT(false, context, psDroid, "No valid droid provided");

	int index = getStructStatFromName(WzString::fromUtf8(statName));
	SCRIPT_ASSERT(false, context, index >= 0, "%s not found", statName.c_str());
	STRUCTURE_STATS	*psStats = &asStructureStats[index];

	SCRIPT_ASSERT(false, context, order == DORDER_BUILD, "Invalid order");
	SCRIPT_ASSERT(false, context, psStats->id.compare("A0ADemolishStructure") != 0, "Cannot build demolition");

	uint16_t uint_direction = 0;
	if (_direction.has_value())
	{
		uint_direction = static_cast<uint16_t>(DEG(_direction.value()));
	}

	DROID_ORDER_DATA *droidOrder = &psDroid->order;
	if (droidOrder->type == order && psDroid->actionPos.x == world_coord(x) && psDroid->actionPos.y == world_coord(y))
	{
		return true;
	}
	orderDroidStatsLocDir(psDroid, (DROID_ORDER)order, psStats, world_coord(x) + TILE_UNITS / 2, world_coord(y) + TILE_UNITS / 2, uint_direction, ModeQueue);
	return true;
}

//-- ## setAssemblyPoint(structure, x, y)
//--
//-- Set the assembly point droids go to when built for the specified structure. (3.2+ only)
//--
bool wzapi::setAssemblyPoint(WZAPI_PARAMS(STRUCTURE *psStruct, int x, int y))
{
	SCRIPT_ASSERT(false, context, psStruct, "No valid structure provided");
	SCRIPT_ASSERT(false, context, psStruct->pStructureType->type == REF_FACTORY
	              || psStruct->pStructureType->type == REF_CYBORG_FACTORY
	              || psStruct->pStructureType->type == REF_VTOL_FACTORY, "Structure not a factory");
	setAssemblyPoint(((FACTORY *)psStruct->pFunctionality)->psAssemblyPoint, x, y, psStruct->player, true);
	return true;
}

//-- ## setSunPosition(x, y, z)
//--
//-- Move the position of the Sun, which in turn moves where shadows are cast. (3.2+ only)
//--
bool wzapi::setSunPosition(WZAPI_PARAMS(float x, float y, float z))
{
	setTheSun(Vector3f(x, y, z));
	return true;
}

//-- ## setSunIntensity(ambient r, g, b, diffuse r, g, b, specular r, g, b)
//--
//-- Set the ambient, diffuse and specular colour intensities of the Sun lighting source. (3.2+ only)
//--
bool wzapi::setSunIntensity(WZAPI_PARAMS(float ambient_r, float ambient_g, float ambient_b, float diffuse_r, float diffuse_g, float diffuse_b, float specular_r, float specular_g, float specular_b))
{
	float ambient[4];
	float diffuse[4];
	float specular[4];
	ambient[0] = ambient_r;
	ambient[1] = ambient_g;
	ambient[2] = ambient_b;
	ambient[3] = 1.0f;
	diffuse[0] = diffuse_r;
	diffuse[1] = diffuse_g;
	diffuse[2] = diffuse_b;
	diffuse[3] = 1.0f;
	specular[0] = specular_r;
	specular[1] = specular_g;
	specular[2] = specular_b;
	specular[3] = 1.0f;
	pie_Lighting0(LIGHT_AMBIENT, ambient);
	pie_Lighting0(LIGHT_DIFFUSE, diffuse);
	pie_Lighting0(LIGHT_SPECULAR, specular);
	return true;
}

//-- ## setWeather(weather type)
//--
//-- Set the current weather. This should be one of WEATHER_RAIN, WEATHER_SNOW or WEATHER_CLEAR. (3.2+ only)
//--
bool wzapi::setWeather(WZAPI_PARAMS(int weather))
{
	SCRIPT_ASSERT(false, context, weather >= 0 && weather <= WT_NONE, "Bad weather type");
	atmosSetWeatherType((WT_CLASS)weather);
	return true;
}

//-- ## setSky(texture file, wind speed, skybox scale)
//--
//-- Change the skybox. (3.2+ only)
//--
bool wzapi::setSky(WZAPI_PARAMS(std::string page, float wind, float scale))
{
	setSkyBox(page.c_str(), wind, scale);
	return true; // TODO: modify setSkyBox to return bool, success / failure
}

//-- ## cameraSlide(x, y)
//--
//-- Slide the camera over to the given position on the map. (3.2+ only)
//--
bool wzapi::cameraSlide(WZAPI_PARAMS(float x, float y))
{
	requestRadarTrack(static_cast<SDWORD>(x), static_cast<SDWORD>(y));
	return true;
}

//-- ## cameraZoom(z, speed)
//--
//-- Slide the camera to the given zoom distance. Normal camera zoom ranges between 500 and 5000. (3.2+ only)
//--
bool wzapi::cameraZoom(WZAPI_PARAMS(float viewDistance, float speed))
{
	animateToViewDistance(viewDistance, speed);
	return true;
}

//-- ## cameraTrack(droid)
//--
//-- Make the camera follow the given droid object around. Pass in a null object to stop. (3.2+ only)
//--
bool wzapi::cameraTrack(WZAPI_PARAMS(optional<DROID *> _targetDroid))
{
	if (!_targetDroid.has_value())
	{
		setWarCamActive(false);
	}
	else
	{
		DROID *targetDroid = _targetDroid.value();
		SCRIPT_ASSERT(false, context, targetDroid, "No valid droid provided");
		SCRIPT_ASSERT(false, context, selectedPlayer < MAX_PLAYERS, "Invalid selectedPlayer for current client: %" PRIu32 "", selectedPlayer);
		for (DROID *psDroid = apsDroidLists[selectedPlayer]; psDroid != nullptr; psDroid = psDroid->psNext)
		{
			psDroid->selected = (psDroid == targetDroid); // select only the target droid
		}
		setWarCamActive(true);
	}
	return true;
}

//-- ## addSpotter(x, y, player, range, type, expiry)
//--
//-- Add an invisible viewer at a given position for given player that shows map in given range. ```type```
//-- is zero for vision reveal, or one for radar reveal. The difference is that a radar reveal can be obstructed
//-- by ECM jammers. ```expiry```, if non-zero, is the game time at which the spotter shall automatically be
//-- removed. The function returns a unique ID that can be used to remove the spotter with ```removeSpotter```. (3.2+ only)
//--
uint32_t wzapi::addSpotter(WZAPI_PARAMS(int x, int y, int player, int range, bool radar, uint32_t expiry))
{
	uint32_t id = ::addSpotter(x, y, player, range, radar, expiry);
	return id;
}

//-- ## removeSpotter(id)
//--
//-- Remove a spotter given its unique ID. (3.2+ only)
//--
bool wzapi::removeSpotter(WZAPI_PARAMS(uint32_t id))
{
	return ::removeSpotter(id);
}

//-- ## syncRequest(req_id, x, y[, obj[, obj2]])
//--
//-- Generate a synchronized event request that is sent over the network to all clients and executed simultaneously.
//-- Must be caught in an eventSyncRequest() function. All sync requests must be validated when received, and always
//-- take care only to define sync requests that can be validated against cheating. (3.2+ only)
//--
bool wzapi::syncRequest(WZAPI_PARAMS(int32_t req_id, int32_t _x, int32_t _y, optional<const BASE_OBJECT *> _psObj, optional<const BASE_OBJECT *> _psObj2))
{
	int32_t x = world_coord(_x);
	int32_t y = world_coord(_y);
	const BASE_OBJECT *psObj = nullptr, *psObj2 = nullptr;
	if (_psObj.has_value())
	{
		psObj = _psObj.value();
		SCRIPT_ASSERT(false, context, psObj, "No valid object (obj1) provided");
	}
	if (_psObj2.has_value())
	{
		psObj2 = _psObj2.value();
		SCRIPT_ASSERT(false, context, psObj2, "No valid object (obj2) provided");
	}
	sendSyncRequest(req_id, x, y, psObj, psObj2);
	return true;
}

//-- ## replaceTexture(old_filename, new_filename)
//--
//-- Replace one texture with another. This can be used to for example give buildings on a specific tileset different
//-- looks, or to add variety to the looks of droids in campaign missions. (3.2+ only)
//--
bool wzapi::replaceTexture(WZAPI_PARAMS(std::string oldfile, std::string newfile))
{
	return replaceTexture(WzString::fromUtf8(oldfile), WzString::fromUtf8(newfile));
}

//-- ## changePlayerColour(player, colour)
//--
//-- Change a player's colour slot. The current player colour can be read from the ```playerData``` array. There are as many
//-- colour slots as the maximum number of players. (3.2.3+ only)
//--
bool wzapi::changePlayerColour(WZAPI_PARAMS(int player, int colour))
{
	return setPlayerColour(player, colour);
}

//-- ## setHealth(object, health)
//--
//-- Change the health of the given game object, in percentage. Does not take care of network sync, so for multiplayer games,
//-- needs wrapping in a syncRequest. (3.2.3+ only.)
//--
bool wzapi::setHealth(WZAPI_PARAMS(BASE_OBJECT* psObject, int health)) MULTIPLAY_SYNCREQUEST_REQUIRED
{
	SCRIPT_ASSERT(false, context, psObject, "No valid object provided");
	SCRIPT_ASSERT(false, context, health >= 1, "Bad health value %d", health);
	int id = psObject->id;
	int player = psObject->player;
	OBJECT_TYPE type = psObject->type;
	SCRIPT_ASSERT(false, context, type == OBJ_DROID || type == OBJ_STRUCTURE || type == OBJ_FEATURE, "Bad object type");
	if (type == OBJ_DROID)
	{
		DROID *psDroid = (DROID *)psObject;
		SCRIPT_ASSERT(false, context, psDroid, "No such droid id %d belonging to player %d", id, player);
		psDroid->body = static_cast<UDWORD>(health * (double)psDroid->originalBody / 100);
	}
	else if (type == OBJ_STRUCTURE)
	{
		STRUCTURE *psStruct = (STRUCTURE *)psObject;
		SCRIPT_ASSERT(false, context, psStruct, "No such structure id %d belonging to player %d", id, player);
		psStruct->body = health * MAX(1, structureBody(psStruct)) / 100;
	}
	else
	{
		FEATURE *psFeat = (FEATURE *)psObject;
		SCRIPT_ASSERT(false, context, psFeat, "No such feature id %d belonging to player %d", id, player);
		psFeat->body = health * psFeat->psStats->body / 100;
	}
	return true;
}

//-- ## useSafetyTransport(flag)
//--
//-- Change if the mission transporter will fetch droids in non offworld missions
//-- setReinforcementTime() is be used to hide it before coming back after the set time
//-- which is handled by the campaign library in the victory data section (3.3+ only).
//--
bool wzapi::useSafetyTransport(WZAPI_PARAMS(bool flag))
{
	setDroidsToSafetyFlag(flag);
	return true;
}

//-- ## restoreLimboMissionData()
//--
//-- Swap mission type and bring back units previously stored at the start
//-- of the mission (see cam3-c mission). (3.3+ only).
//--
bool wzapi::restoreLimboMissionData(WZAPI_NO_PARAMS)
{
	resetLimboMission();
	return true;
}

//-- ## getMultiTechLevel()
//--
//-- Returns the current multiplayer tech level. (3.3+ only)
//--
uint32_t wzapi::getMultiTechLevel(WZAPI_NO_PARAMS)
{
	return game.techLevel;
}

//-- ## setCampaignNumber(num)
//--
//-- Set the campaign number. (3.3+ only)
//--
bool wzapi::setCampaignNumber(WZAPI_PARAMS(int num))
{
	::setCampaignNumber(num);
	return true;
}

//-- ## getMissionType()
//--
//-- Return the current mission type. (3.3+ only)
//--
int32_t wzapi::getMissionType(WZAPI_NO_PARAMS)
{
	return (int32_t)mission.type;
}

//-- ## getRevealStatus()
//--
//-- Return the current fog reveal status. (3.3+ only)
//--
bool wzapi::getRevealStatus(WZAPI_NO_PARAMS)
{
	return ::getRevealStatus();
}

//-- ## setRevealStatus(bool)
//--
//-- Set the fog reveal status. (3.3+ only)
//--
bool wzapi::setRevealStatus(WZAPI_PARAMS(bool status))
{
	::setRevealStatus(status);
	preProcessVisibility();
	return true;
}

//-- ## autoSave()
//--
//-- Perform automatic save
//--
bool wzapi::autoSave(WZAPI_NO_PARAMS)
{
	return ::autoSave();
}

// MARK: - horrible hacks follow -- do not rely on these being present!

//-- ## hackNetOff()
//--
//-- Turn off network transmissions. FIXME - find a better way.
//--
wzapi::no_return_value wzapi::hackNetOff(WZAPI_NO_PARAMS)
{
	bMultiPlayer = false;
	bMultiMessages = false;
	return {};
}

//-- ## hackNetOn()
//--
//-- Turn on network transmissions. FIXME - find a better way.
//--
wzapi::no_return_value wzapi::hackNetOn(WZAPI_NO_PARAMS)
{
	bMultiPlayer = true;
	bMultiMessages = true;
	return {};
}

//-- ## hackAddMessage(message, type, player, immediate)
//--
//-- See wzscript docs for info, to the extent any exist. (3.2+ only)
//--
wzapi::no_return_value wzapi::hackAddMessage(WZAPI_PARAMS(std::string message, int type, int player, bool immediate))
{
	MESSAGE_TYPE msgType = (MESSAGE_TYPE)type;
	SCRIPT_ASSERT_PLAYER({}, context, player);
	MESSAGE *psMessage = addMessage(msgType, false, player);
	if (psMessage)
	{
		VIEWDATA *psViewData = getViewData(WzString::fromUtf8(message));
		SCRIPT_ASSERT({}, context, psViewData, "Viewdata not found");
		psMessage->pViewData = psViewData;
		debug(LOG_MSG, "Adding %s pViewData=%p", psViewData->name.toUtf8().c_str(), static_cast<void *>(psMessage->pViewData));
		if (msgType == MSG_PROXIMITY)
		{
			VIEW_PROXIMITY *psProx = (VIEW_PROXIMITY *)psViewData->pData;
			// check the z value is at least the height of the terrain
			int height = map_Height(psProx->x, psProx->y);
			if (psProx->z < height)
			{
				psProx->z = height;
			}
		}
		if (immediate)
		{
			displayImmediateMessage(psMessage);
		}
	}
//	jsDebugMessageUpdate();
	return {};
}

//-- ## hackRemoveMessage(message, type, player)
//--
//-- See wzscript docs for info, to the extent any exist. (3.2+ only)
//--
wzapi::no_return_value wzapi::hackRemoveMessage(WZAPI_PARAMS(std::string message, int type, int player))
{
	MESSAGE_TYPE msgType = (MESSAGE_TYPE)type;
	SCRIPT_ASSERT_PLAYER({}, context, player);
	VIEWDATA *psViewData = getViewData(WzString::fromUtf8(message));
	SCRIPT_ASSERT({}, context, psViewData, "Viewdata not found");
	MESSAGE *psMessage = findMessage(psViewData, msgType, player);
	if (psMessage)
	{
		debug(LOG_MSG, "Removing %s", psViewData->name.toUtf8().c_str());
		removeMessage(psMessage, player);
	}
	else
	{
		debug(LOG_ERROR, "cannot find message - %s", psViewData->name.toUtf8().c_str());
	}
//	jsDebugMessageUpdate();
	return {};
}

//-- ## hackGetObj(type, player, id)
//--
//-- Function to find and return a game object of DROID, FEATURE or STRUCTURE types, if it exists.
//-- Otherwise, it will return null. This function is deprecated by getObject(). (3.2+ only)
//--
wzapi::returned_nullable_ptr<const BASE_OBJECT> wzapi::hackGetObj(WZAPI_PARAMS(int _type, int player, int id)) WZAPI_DEPRECATED
{
	OBJECT_TYPE type = (OBJECT_TYPE)_type;
	SCRIPT_ASSERT_PLAYER(nullptr, context, player);
	return IdToObject(type, id, player);
}

//-- ## hackAssert(condition, message...)
//--
//-- Function to perform unit testing. It will throw a script error and a game assert. (3.2+ only)
//--
wzapi::no_return_value wzapi::hackAssert(WZAPI_PARAMS(bool condition, va_list_treat_as_strings message))
{
	if (condition)
	{
		return {}; // pass
	}
	// fail
	std::string result;
	for (const auto & s : message.strings)
	{
		if (!result.empty())
		{
			result.append(" ");
		}
		result.append(s);
	}
	context.throwError(result.c_str(), __LINE__, "hackAssert");
	return {};
}

//-- ## receiveAllEvents(bool)
//--
//-- Make the current script receive all events, even those not meant for 'me'. (3.2+ only)
//--
bool wzapi::receiveAllEvents(WZAPI_PARAMS(optional<bool> enabled))
{
	if (enabled.has_value())
	{
		context.set_isReceivingAllEvents(enabled.value());
	}
	return context.get_isReceivingAllEvents();
}

//-- ## hackDoNotSave(name)
//--
//-- Do not save the given global given by name to savegames. Must be
//-- done again each time game is loaded, since this too is not saved.
//--
wzapi::no_return_value wzapi::hackDoNotSave(WZAPI_PARAMS(std::string name))
{
	context.doNotSaveGlobal(name);
	return {};
}

//-- ## hackPlayIngameAudio()
//--
//-- (3.3+ only)
//--
wzapi::no_return_value wzapi::hackPlayIngameAudio(WZAPI_NO_PARAMS)
{
	debug(LOG_SOUND, "Script wanted music to start");
	cdAudio_PlayTrack(SONG_INGAME);
	return {};
}

//-- ## hackStopIngameAudio()
//--
//-- Stop the in-game music. (3.3+ only)
//-- This should be called from the eventStartLevel() event (or later).
//-- Currently only used from the tutorial.
//--
wzapi::no_return_value wzapi::hackStopIngameAudio(WZAPI_NO_PARAMS)
{
	debug(LOG_SOUND, "Script wanted music to stop");
	cdAudio_Stop();
	return {};
}

//-- ## hackMarkTiles([label | x, y[, x2, y2]])
//--
//-- Mark the given tile(s) on the map. Either give a POSITION or AREA label,
//-- or a tile x, y position, or four positions for a square area. If no parameter
//-- is given, all marked tiles are cleared. (3.2+ only)
//--
wzapi::no_return_value wzapi::hackMarkTiles(WZAPI_PARAMS(optional<wzapi::label_or_position_values> _tilePosOrArea))
{
	if (_tilePosOrArea.has_value())
	{
		label_or_position_values& tilePosOrArea = _tilePosOrArea.value();
		if (tilePosOrArea.isLabel()) // label
		{
			const std::string& label = tilePosOrArea.label;
			return scripting_engine::instance().hackMarkTiles_ByLabel(context, label);
		}
		else if (tilePosOrArea.isPositionValues())
		{
			if (tilePosOrArea.x2.has_value() || tilePosOrArea.y2.has_value())
			{
				SCRIPT_ASSERT({}, context, tilePosOrArea.x2.has_value() && tilePosOrArea.y2.has_value(), "If x2 or y2 are provided, *both* must be provided");
				int x1 = tilePosOrArea.x1;
				int y1 = tilePosOrArea.y1;
				int x2 = tilePosOrArea.x2.value();
				int y2 = tilePosOrArea.y2.value();
				for (int x = x1; x < x2; x++)
				{
					for (int y = y1; y < y2; y++)
					{
						MAPTILE *psTile = mapTile(x, y);
						psTile->tileInfoBits |= BITS_MARKED;
					}
				}
			}
			else // single tile
			{
				int x = tilePosOrArea.x1;
				int y = tilePosOrArea.y1;
				MAPTILE *psTile = mapTile(x, y);
				psTile->tileInfoBits |= BITS_MARKED;
			}
		}
	}
	else // clear all marks
	{
		clearMarks();
	}
	return {};
}

// MARK: - General functions -- geared for use in AI scripts

//-- ## dump(string...)
//--
//-- Output text to a debug file. (3.2+ only)
//--
wzapi::no_return_value wzapi::dump(WZAPI_PARAMS(va_list_treat_as_strings strings))
{
	std::string result;
	for (size_t idx = 0; idx < strings.strings.size(); idx++)
	{
		if (idx != 0)
		{
			result.append(" ");
		}
		result.append(strings.strings[idx].c_str());
	}
	result += "\n";

	int me = context.player();
	context.currentInstance()->dumpScriptLog(result, me);
	return {};
}

//-- ## debug(string...)
//--
//-- Output text to the command line.
//--
wzapi::no_return_value wzapi::debugOutputStrings(WZAPI_PARAMS(wzapi::va_list_treat_as_strings strings))
{
	for (size_t idx = 0; idx < strings.strings.size(); idx++)
	{
		if (idx != 0)
		{
			fprintf(stderr, " %s", strings.strings[idx].c_str());
		}
		else
		{
			fprintf(stderr, "%s", strings.strings[idx].c_str());
		}
	}
	fprintf(stderr, "\n");
	return {};
}

//-- ## console(strings...)
//--
//-- Print text to the player console.
//--
// TODO, should cover scrShowConsoleText, scrAddConsoleText, scrTagConsoleText and scrConsole
bool wzapi::console(WZAPI_PARAMS(va_list_treat_as_strings strings))
{
	int player = context.player();
	if (player == selectedPlayer)
	{
		std::string result;
		for (const auto & s : strings.strings)
		{
			if (!result.empty())
			{
				result.append(" ");
			}
			result.append(s);
		}
		//permitNewConsoleMessages(true);
		//setConsolePermanence(true,true);
		addConsoleMessage(result.c_str(), CENTRE_JUSTIFY, SYSTEM_MESSAGE);
		//permitNewConsoleMessages(false);
	}
	return true;
}

//-- ## clearConsole()
//--
//-- Clear the console. (3.3+ only)
//--
bool wzapi::clearConsole(WZAPI_NO_PARAMS)
{
	flushConsoleMessages();
	return true;
}

//-- ## structureIdle(structure)
//--
//-- Is given structure idle?
//--
bool wzapi::structureIdle(WZAPI_PARAMS(const STRUCTURE *psStruct))
{
	SCRIPT_ASSERT(false, context, psStruct, "No valid structure provided");
	return ::structureIdle(psStruct);
}

std::vector<const STRUCTURE *> _enumStruct_fromList(WZAPI_PARAMS(optional<int> _player, optional<wzapi::STRUCTURE_TYPE_or_statsName_string> _structureType, optional<int> _looking), STRUCTURE **psStructLists)
{
	std::vector<const STRUCTURE *> matches;
	int player = -1, looking = -1;
	WzString statsName;
	STRUCTURE_TYPE type = NUM_DIFF_BUILDINGS;

	player = (_player.has_value()) ? _player.value() : context.player();

	if (_structureType.has_value())
	{
		type = _structureType.value().type;
		statsName = WzString::fromUtf8(_structureType.value().statsName);
	}
	if (_looking.has_value())
	{
		looking = _looking.value();
	}

	SCRIPT_ASSERT_PLAYER({}, context, player);
	SCRIPT_ASSERT({}, context, looking < MAX_PLAYERS && looking >= -1, "Looking player index out of range: %d", looking);
	for (STRUCTURE *psStruct = psStructLists[player]; psStruct; psStruct = psStruct->psNext)
	{
		if ((looking == -1 || psStruct->visible[looking])
		    && !psStruct->died
		    && (type == NUM_DIFF_BUILDINGS || type == psStruct->pStructureType->type)
		    && (statsName.isEmpty() || statsName.compare(psStruct->pStructureType->id) == 0))
		{
			matches.push_back(psStruct);
		}
	}

	return matches;
}

//-- ## enumStruct([player[, structure type[, looking player]]])
//--
//-- Returns an array of structure objects. If no parameters given, it will
//-- return all of the structures for the current player. The second parameter
//-- can be either a string with the name of the structure type as defined in
//-- "structures.json", or a stattype as defined in ```Structure```. The
//-- third parameter can be used to filter by visibility, the default is not
//-- to filter.
//--
std::vector<const STRUCTURE *> wzapi::enumStruct(WZAPI_PARAMS(optional<int> _player, optional<STRUCTURE_TYPE_or_statsName_string> _structureType, optional<int> _looking))
{
	return _enumStruct_fromList(context, _player, _structureType, _looking, apsStructLists);
}

//-- ## enumStructOffWorld([player[, structure type[, looking player]]])
//--
//-- Returns an array of structure objects in your base when on an off-world mission, NULL otherwise.
//-- If no parameters given, it will return all of the structures for the current player.
//-- The second parameter can be either a string with the name of the structure type as defined
//-- in "structures.json", or a stattype as defined in ```Structure```.
//-- The third parameter can be used to filter by visibility, the default is not
//-- to filter.
//--
std::vector<const STRUCTURE *> wzapi::enumStructOffWorld(WZAPI_PARAMS(optional<int> _player, optional<STRUCTURE_TYPE_or_statsName_string> _structureType, optional<int> _looking))
{
	return _enumStruct_fromList(context, _player, _structureType, _looking, (mission.apsStructLists));
}

//-- ## enumDroid([player[, droid type[, looking player]]])
//--
//-- Returns an array of droid objects. If no parameters given, it will
//-- return all of the droids for the current player. The second, optional parameter
//-- is the name of the droid type. The third parameter can be used to filter by
//-- visibility - the default is not to filter.
//--
std::vector<const DROID *> wzapi::enumDroid(WZAPI_PARAMS(optional<int> _player, optional<int> _droidType, optional<int> _looking))
{
	std::vector<const DROID *> matches;
	int player = -1, looking = -1;
	DROID_TYPE droidType = DROID_ANY;
	DROID_TYPE droidType2;

	player = (_player.has_value()) ? _player.value() : context.player();

	if (_droidType.has_value())
	{
		droidType = (DROID_TYPE)_droidType.value();
	}
	if (_looking.has_value())
	{
		looking = _looking.value();
	}

	switch (droidType) // hide some engine craziness
	{
	case DROID_CONSTRUCT:
		droidType2 = DROID_CYBORG_CONSTRUCT; break;
	case DROID_WEAPON:
		droidType2 = DROID_CYBORG_SUPER; break;
	case DROID_REPAIR:
		droidType2 = DROID_CYBORG_REPAIR; break;
	case DROID_CYBORG:
		droidType2 = DROID_CYBORG_SUPER; break;
	default:
		droidType2 = droidType;
		break;
	}
	SCRIPT_ASSERT_PLAYER({}, context, player);
	SCRIPT_ASSERT({}, context, looking < MAX_PLAYERS && looking >= -1, "Looking player index out of range: %d", looking);
	for (DROID *psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
	{
		if ((looking == -1 || psDroid->visible[looking])
		    && !psDroid->died
		    && (droidType == DROID_ANY || droidType == psDroid->droidType || droidType2 == psDroid->droidType))
		{
			matches.push_back(psDroid);
		}
	}
	return matches;
}

//-- ## enumFeature(player[, name])
//--
//-- Returns an array of all features seen by player of given name, as defined in "features.json".
//-- If player is ```ALL_PLAYERS```, it will return all features irrespective of visibility to any player. If
//-- name is empty, it will return any feature.
//--
std::vector<const FEATURE *> wzapi::enumFeature(WZAPI_PARAMS(int looking, optional<std::string> _statsName))
{
	SCRIPT_ASSERT({}, context, looking < MAX_PLAYERS && looking >= -1, "Looking player index out of range: %d", looking);
	WzString statsName;
	if (_statsName.has_value())
	{
		statsName = WzString::fromUtf8(_statsName.value());
	}

	std::vector<const FEATURE *> matches;
	for (FEATURE *psFeat = apsFeatureLists[0]; psFeat; psFeat = psFeat->psNext)
	{
		if ((looking == -1 || psFeat->visible[looking])
		    && !psFeat->died
		    && (statsName.isEmpty() || statsName.compare(psFeat->psStats->id) == 0))
		{
			matches.push_back(psFeat);
		}
	}
	return matches;
}

//-- ## enumBlips(player)
//--
//-- Return an array containing all the non-transient radar blips that the given player
//-- can see. This includes sensors revealed by radar detectors, as well as ECM jammers.
//-- It does not include units going out of view.
//--
std::vector<scr_position> wzapi::enumBlips(WZAPI_PARAMS(int player))
{
	SCRIPT_ASSERT_PLAYER({}, context, player);
	std::vector<scr_position> matches;
	for (BASE_OBJECT *psSensor = apsSensorList[0]; psSensor; psSensor = psSensor->psNextFunc)
	{
		if (psSensor->visible[player] > 0 && psSensor->visible[player] < UBYTE_MAX)
		{
			matches.push_back({map_coord(psSensor->pos.x), map_coord(psSensor->pos.y)});
		}
	}
	return matches;
}

//-- ## enumSelected()
//--
//-- Return an array containing all game objects currently selected by the host player. (3.2+ only)
//--
std::vector<const BASE_OBJECT *> wzapi::enumSelected(WZAPI_NO_PARAMS_NO_CONTEXT)
{
	std::vector<const BASE_OBJECT *> matches;
	if (selectedPlayer >= MAX_PLAYERS)
	{
		return matches;
	}
	for (DROID *psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->selected)
		{
			matches.push_back(psDroid);
		}
	}
	for (STRUCTURE *psStruct = apsStructLists[selectedPlayer]; psStruct; psStruct = psStruct->psNext)
	{
		if (psStruct->selected)
		{
			matches.push_back(psStruct);
		}
	}
	// TODO - also add selected delivery points
	return matches;
}

//-- ## enumGateways()
//--
//-- Return an array containing all the gateways on the current map. The array contains object with the properties
//-- x1, y1, x2 and y2. (3.2+ only)
//--
GATEWAY_LIST wzapi::enumGateways(WZAPI_NO_PARAMS)
{
	return gwGetGateways();
}

//-- ## getResearch(research[, player])
//--
//-- Fetch information about a given technology item, given by a string that matches
//-- its definition in "research.json". If not found, returns null.
//--
wzapi::researchResult wzapi::getResearch(WZAPI_PARAMS(std::string resName, optional<int> _player))
{
	researchResult result;
	int player = (_player.has_value()) ? _player.value() : context.player();

	result.psResearch = ::getResearch(resName.c_str());
	result.player = player;
	return result;
}

//-- ## enumResearch()
//--
//-- Returns an array of all research objects that are currently and immediately available for research.
//--
wzapi::researchResults wzapi::enumResearch(WZAPI_NO_PARAMS)
{
	researchResults result;
	int player = context.player();
	for (int i = 0; i < asResearch.size(); i++)
	{
		RESEARCH *psResearch = &asResearch[i];
		if (!IsResearchCompleted(&asPlayerResList[player][i]) && researchAvailable(i, player, ModeQueue))
		{
			result.resList.push_back(psResearch);
		}
	}
	result.player = player;
	return result;
}

//-- ## enumRange(x, y, range[, filter[, seen]])
//--
//-- Returns an array of game objects seen within range of given position that passes the optional filter
//-- which can be one of a player index, ALL_PLAYERS, ALLIES or ENEMIES. By default, filter is
//-- ALL_PLAYERS. Finally an optional parameter can specify whether only visible objects should be
//-- returned; by default only visible objects are returned. Calling this function is much faster than
//-- iterating over all game objects using other enum functions. (3.2+ only)
//--
std::vector<const BASE_OBJECT *> wzapi::enumRange(WZAPI_PARAMS(int _x, int _y, int _range, optional<int> _filter, optional<bool> _seen))
{
	int player = context.player();
	int x = world_coord(_x);
	int y = world_coord(_y);
	int range = world_coord(_range);
	int filter = (_filter.has_value()) ? _filter.value() : ALL_PLAYERS;
	bool seen = (_seen.has_value()) ? _seen.value() : true;

	static GridList gridList;  // static to avoid allocations. // WARNING: THREAD-SAFETY
	gridList = gridStartIterate(x, y, range);
	std::vector<const BASE_OBJECT *> list;
	for (GridIterator gi = gridList.begin(); gi != gridList.end(); ++gi)
	{
		const BASE_OBJECT *psObj = *gi;
		if ((psObj->visible[player] || !seen) && !psObj->died)
		{
			if ((filter >= 0 && psObj->player == filter) || filter == ALL_PLAYERS
			    || (filter == ALLIES && psObj->type != OBJ_FEATURE && aiCheckAlliances(psObj->player, player))
			    || (filter == ENEMIES && psObj->type != OBJ_FEATURE && !aiCheckAlliances(psObj->player, player)))
			{
				list.push_back(psObj);
			}
		}
	}
	return list;
}

//-- ## pursueResearch(lab, research)
//--
//-- Start researching the first available technology on the way to the given technology.
//-- First parameter is the structure to research in, which must be a research lab. The
//-- second parameter is the technology to pursue, as a text string as defined in "research.json".
//-- The second parameter may also be an array of such strings. The first technology that has
//-- not yet been researched in that list will be pursued.
//--
bool wzapi::pursueResearch(WZAPI_PARAMS(const STRUCTURE *psStruct, string_or_string_list research))
{
	SCRIPT_ASSERT(false, context, psStruct, "No valid structure provided");
	int player = psStruct->player;

	RESEARCH *psResearch = nullptr;  // Dummy initialisation.
	for (const auto& resName : research.strings)
	{
		RESEARCH *psCurrResearch = ::getResearch(resName.c_str());
		SCRIPT_ASSERT(false, context, psCurrResearch, "No such research: %s", resName.c_str());
		PLAYER_RESEARCH *plrRes = &asPlayerResList[player][psCurrResearch->index];
		if (!IsResearchStartedPending(plrRes) && !IsResearchCompleted(plrRes))
		{
			// use this one
			psResearch = psCurrResearch;
			break;
		}
	}
	if (psResearch == nullptr)
	{
		if (research.strings.size() == 1)
		{
			debug(LOG_SCRIPT, "%s has already been researched!", research.strings.front().c_str());
		}
		else
		{
			debug(LOG_SCRIPT, "Exhausted research list -- doing nothing");
		}
		return false;
	}

	SCRIPT_ASSERT(false, context, psStruct->pStructureType->type == REF_RESEARCH, "Not a research lab: %s", objInfo(psStruct));
	RESEARCH_FACILITY *psResLab = (RESEARCH_FACILITY *)psStruct->pFunctionality;
	SCRIPT_ASSERT(false, context, psResLab->psSubject == nullptr, "Research lab not ready");
	// Go down the requirements list for the desired tech
	std::list<RESEARCH *> reslist;
	RESEARCH *cur = psResearch;
	int iterations = 0;  // Only used to assert we're not stuck in the loop.
	while (cur)
	{
		if (researchAvailable(cur->index, player, ModeQueue))
		{
			bool started = false;
			for (int i = 0; i < game.maxPlayers; i++)
			{
				if (i == player || (aiCheckAlliances(player, i) && alliancesSharedResearch(game.alliance)))
				{
					int bits = asPlayerResList[i][cur->index].ResearchStatus;
					started = started || (bits & STARTED_RESEARCH) || (bits & STARTED_RESEARCH_PENDING)
					          || (bits & RESBITS_PENDING_ONLY) || (bits & RESEARCHED);
				}
			}
			if (!started) // found relevant item on the path?
			{
				sendResearchStatus(psStruct, cur->index, player, true);
#if defined (DEBUG)
				char sTemp[128];
				snprintf(sTemp, sizeof(sTemp), "player:%d starts topic from script: %s", player, getID(cur));
				NETlogEntry(sTemp, SYNC_FLAG, 0);
#endif
				debug(LOG_SCRIPT, "Started research in %d's %s(%d) of %s", player,
				      objInfo(psStruct), psStruct->id, getStatsName(cur));
				return true;
			}
		}
		RESEARCH *prev = cur;
		cur = nullptr;
		if (!prev->pPRList.empty())
		{
			cur = &asResearch[prev->pPRList[0]]; // get first pre-req
		}
		for (int i = 1; i < prev->pPRList.size(); i++)
		{
			// push any other pre-reqs on the stack
			reslist.push_back(&asResearch[prev->pPRList[i]]);
		}
		if (!cur && !reslist.empty())
		{
			cur = reslist.front(); // retrieve options from the stack
			reslist.pop_front();
		}
		ASSERT_OR_RETURN(false, ++iterations < asResearch.size() * 100 || !cur, "Possible cyclic dependencies in prerequisites, possibly of research \"%s\".", getStatsName(cur));
	}
	debug(LOG_SCRIPT, "No research topic found for %s(%d)", objInfo(psStruct), psStruct->id);
	return false; // none found
}

//-- ## findResearch(research, [player])
//--
//-- Return list of research items remaining to be researched for the given research item. (3.2+ only)
//-- (Optional second argument 3.2.3+ only)
//--
wzapi::researchResults wzapi::findResearch(WZAPI_PARAMS(std::string resName, optional<int> _player))
{
	int player = (_player.has_value()) ? _player.value() : context.player();
	SCRIPT_ASSERT_PLAYER({}, context, player);

	researchResults result;
	result.player = player;

	RESEARCH *psTarget = ::getResearch(resName.c_str());
	SCRIPT_ASSERT({}, context, psTarget, "No such research: %s", resName.c_str());
	PLAYER_RESEARCH *plrRes = &asPlayerResList[player][psTarget->index];
	if (IsResearchStartedPending(plrRes) || IsResearchCompleted(plrRes))
	{
		debug(LOG_SCRIPT, "Find reqs for %s for player %d - research pending or completed", resName.c_str(), player);
		return result; // return empty array
	}
	debug(LOG_SCRIPT, "Find reqs for %s for player %d", resName.c_str(), player);
	// Go down the requirements list for the desired tech
	std::list<RESEARCH *> reslist;
	RESEARCH *cur = psTarget;
	while (cur)
	{
		if (!(asPlayerResList[player][cur->index].ResearchStatus & RESEARCHED))
		{
			debug(LOG_SCRIPT, "Added research in %d's %s for %s", player, getID(cur), getID(psTarget));
			result.resList.push_back(cur);
		}
		RESEARCH *prev = cur;
		cur = nullptr;
		if (!prev->pPRList.empty())
		{
			cur = &asResearch[prev->pPRList[0]]; // get first pre-req
		}
		for (int i = 1; i < prev->pPRList.size(); i++)
		{
			// push any other pre-reqs on the stack
			reslist.push_back(&asResearch[prev->pPRList[i]]);
		}
		if (!cur && !reslist.empty())
		{
			// retrieve options from the stack
			cur = reslist.front();
			reslist.pop_front();
		}
	}
	return result;
}

//-- ## distBetweenTwoPoints(x1, y1, x2, y2)
//--
//-- Return distance between two points.
//--
int32_t wzapi::distBetweenTwoPoints(WZAPI_PARAMS(int32_t x1, int32_t y1, int32_t x2, int32_t y2))
{
	return iHypot(x1 - x2, y1 - y2);
}

//-- ## orderDroidLoc(droid, order, x, y)
//--
//-- Give a droid an order to do something at the given location.
//--
bool wzapi::orderDroidLoc(WZAPI_PARAMS(DROID *psDroid, int order_, int x, int y))
{
	SCRIPT_ASSERT(false, context, psDroid, "No valid droid provided");
	DROID_ORDER order = (DROID_ORDER)order_;
	SCRIPT_ASSERT(false, context, validOrderForLoc(order), "Invalid location based order: %s", getDroidOrderName(order));
	SCRIPT_ASSERT(false, context, tileOnMap(x, y), "Outside map bounds (%d, %d)", x, y);
	DROID_ORDER_DATA *droidOrder = &psDroid->order;
	if (droidOrder->type == order && psDroid->actionPos.x == world_coord(x) && psDroid->actionPos.y == world_coord(y))
	{
		return true;
	}
	::orderDroidLoc(psDroid, order, world_coord(x), world_coord(y), ModeQueue);
	return true;
}

//-- ## playerPower(player)
//--
//-- Return amount of power held by the given player.
//--
int32_t wzapi::playerPower(WZAPI_PARAMS(int player))
{
	SCRIPT_ASSERT_PLAYER(-1, context, player);
	return getPower(player);
}

//-- ## queuedPower(player)
//--
//-- Return amount of power queued up for production by the given player. (3.2+ only)
//--
int wzapi::queuedPower(WZAPI_PARAMS(int player))
{
	SCRIPT_ASSERT_PLAYER(-1, context, player);
	return getQueuedPower(player);
}

//-- ## isStructureAvailable(structure type[, player])
//--
//-- Returns true if given structure can be built. It checks both research and unit limits.
//--
bool wzapi::isStructureAvailable(WZAPI_PARAMS(std::string structName, optional<int> _player))
{
	int index = getStructStatFromName(WzString::fromUtf8(structName));
	SCRIPT_ASSERT(false, context, index >= 0, "%s not found", structName.c_str());
	int player = (_player.has_value()) ? _player.value() : context.player();

	int status = apStructTypeLists[player][index];
	return ((status == AVAILABLE || status == REDUNDANT)
	                    && asStructureStats[index].curCount[player] < asStructureStats[index].upgrade[player].limit);
}

// additional structure check
static bool structDoubleCheck(BASE_STATS *psStat, UDWORD xx, UDWORD yy, SDWORD maxBlockingTiles)
{
	UDWORD		x, y, xTL, yTL, xBR, yBR;
	UBYTE		count = 0;
	STRUCTURE_STATS	*psBuilding = (STRUCTURE_STATS *)psStat;

	xTL = xx - 1;
	yTL = yy - 1;
	xBR = (xx + psBuilding->baseWidth);
	yBR = (yy + psBuilding->baseBreadth);

	// check against building in a gateway, as this can seriously block AI passages
	for (auto psGate : gwGetGateways())
	{
		for (x = xx; x <= xBR; x++)
		{
			for (y = yy; y <= yBR; y++)
			{
				if ((x >= psGate->x1 && x <= psGate->x2) && (y >= psGate->y1 && y <= psGate->y2))
				{
					return false;
				}
			}
		}
	}

	// can you get past it?
	y = yTL;	// top
	for (x = xTL; x != xBR + 1; x++)
	{
		if (fpathBlockingTile(x, y, PROPULSION_TYPE_WHEELED))
		{
			count++;
			break;
		}
	}

	y = yBR;	// bottom
	for (x = xTL; x != xBR + 1; x++)
	{
		if (fpathBlockingTile(x, y, PROPULSION_TYPE_WHEELED))
		{
			count++;
			break;
		}
	}

	x = xTL;	// left
	for (y = yTL + 1; y != yBR; y++)
	{
		if (fpathBlockingTile(x, y, PROPULSION_TYPE_WHEELED))
		{
			count++;
			break;
		}
	}

	x = xBR;	// right
	for (y = yTL + 1; y != yBR; y++)
	{
		if (fpathBlockingTile(x, y, PROPULSION_TYPE_WHEELED))
		{
			count++;
			break;
		}
	}

	//make sure this location is not blocked from too many sides
	if ((count <= maxBlockingTiles) || (maxBlockingTiles == -1))
	{
		return true;
	}
	return false;
}

//-- ## pickStructLocation(droid, structure type, x, y[, maxBlockingTiles])
//--
//-- Pick a location for constructing a certain type of building near some given position.
//-- Returns an object containing "type" POSITION, and "x" and "y" values, if successful.
//--
optional<scr_position> wzapi::pickStructLocation(WZAPI_PARAMS(const DROID *psDroid, std::string statName, int startX, int startY, optional<int> _maxBlockingTiles))
{
	SCRIPT_ASSERT({}, context, psDroid, "No valid droid provided");
	const int player = psDroid->player;
	SCRIPT_ASSERT_PLAYER({}, context, player);
	int index = getStructStatFromName(WzString::fromUtf8(statName));
	SCRIPT_ASSERT({}, context, index >= 0, "%s not found", statName.c_str());
	STRUCTURE_STATS	*psStat = &asStructureStats[index];
	SCRIPT_ASSERT({}, context, psStat, "No such stat found: %s", statName.c_str());

	int numIterations = 30;
	bool found = false;
	int incX, incY, x, y;
	int maxBlockingTiles = 0;

	SCRIPT_ASSERT({}, context, startX >= 0 && startX < mapWidth && startY >= 0 && startY < mapHeight, "Bad position (%d, %d)", startX, startY);

	if (_maxBlockingTiles.has_value())
	{
		maxBlockingTiles = _maxBlockingTiles.value();
	}

	x = startX;
	y = startY;

	Vector2i offset(psStat->baseWidth * (TILE_UNITS / 2), psStat->baseBreadth * (TILE_UNITS / 2));

	// save a lot of typing... checks whether a position is valid
#define LOC_OK(_x, _y) (tileOnMap(_x, _y) && \
                        (!psDroid || fpathCheck(psDroid->pos, Vector3i(world_coord(_x), world_coord(_y), 0), PROPULSION_TYPE_WHEELED)) \
                        && validLocation(psStat, world_coord(Vector2i(_x, _y)) + offset, 0, player, false) && structDoubleCheck(psStat, _x, _y, maxBlockingTiles))

	// first try the original location
	if (LOC_OK(startX, startY))
	{
		found = true;
	}

	// try some locations nearby
	for (incX = 1, incY = 1; incX < numIterations && !found; incX++, incY++)
	{
		y = startY - incY;	// top
		for (x = startX - incX; x < startX + incX; x++)
		{
			if (LOC_OK(x, y))
			{
				found = true;
				goto endstructloc;
			}
		}
		x = startX + incX;	// right
		for (y = startY - incY; y < startY + incY; y++)
		{
			if (LOC_OK(x, y))
			{
				found = true;
				goto endstructloc;
			}
		}
		y = startY + incY;	// bottom
		for (x = startX + incX; x > startX - incX; x--)
		{
			if (LOC_OK(x, y))
			{
				found = true;
				goto endstructloc;
			}
		}
		x = startX - incX;	// left
		for (y = startY + incY; y > startY - incY; y--)
		{
			if (LOC_OK(x, y))
			{
				found = true;
				goto endstructloc;
			}
		}
	}

endstructloc:
	if (found)
	{
		return optional<scr_position>({x + map_coord(offset.x), y + map_coord(offset.y)});
	}
	else
	{
		debug(LOG_SCRIPT, "Did not find valid positioning for %s", getStatsName(psStat));
	}
	return {};
}

//-- ## droidCanReach(droid, x, y)
//--
//-- Return whether or not the given droid could possibly drive to the given position. Does
//-- not take player built blockades into account.
//--
bool wzapi::droidCanReach(WZAPI_PARAMS(const DROID *psDroid, int x, int y))
{
	SCRIPT_ASSERT(false, context, psDroid, "No valid droid provided");
	const PROPULSION_STATS *psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION];
	return fpathCheck(psDroid->pos, Vector3i(world_coord(x), world_coord(y), 0), psPropStats->propulsionType);
}

//-- ## propulsionCanReach(propulsion, x1, y1, x2, y2)
//--
//-- Return true if a droid with a given propulsion is able to travel from (x1, y1) to (x2, y2).
//-- Does not take player built blockades into account. (3.2+ only)
//--
bool wzapi::propulsionCanReach(WZAPI_PARAMS(std::string propulsionName, int x1, int y1, int x2, int y2))
{
	int propulsion = getCompFromName(COMP_PROPULSION, WzString::fromUtf8(propulsionName));
	SCRIPT_ASSERT(false, context, propulsion > 0, "No such propulsion: %s", propulsionName.c_str());
	const PROPULSION_STATS *psPropStats = asPropulsionStats + propulsion;
	return fpathCheck(Vector3i(world_coord(x1), world_coord(y1), 0), Vector3i(world_coord(x2), world_coord(y2), 0), psPropStats->propulsionType);
}

//-- ## terrainType(x, y)
//--
//-- Returns tile type of a given map tile, such as TER_WATER for water tiles or TER_CLIFFFACE for cliffs.
//-- Tile types regulate which units may pass through this tile. (3.2+ only)
//--
int wzapi::terrainType(WZAPI_PARAMS(int x, int y))
{
	return ::terrainType(mapTile(x, y));
}

//-- ## tileIsBurning(x, y)
//--
//-- Returns whether the given map tile is burning. (3.5+ only)
//--
bool wzapi::tileIsBurning(WZAPI_PARAMS(int x, int y))
{
	const MAPTILE *psTile = mapTile(x, y);
	SCRIPT_ASSERT(false, context, psTile, "Checking fire on tile outside the map (%d, %d)", x, y);
	return ::TileIsBurning(psTile);
}

//-- ## orderDroidObj(droid, order, object)
//--
//-- Give a droid an order to do something to something.
//--
bool wzapi::orderDroidObj(WZAPI_PARAMS(DROID *psDroid, int _order, BASE_OBJECT *psObj))
{
	SCRIPT_ASSERT(false, context, psDroid, "No valid droid provided");
	DROID_ORDER order = (DROID_ORDER)_order;
	SCRIPT_ASSERT(false, context, psObj, "No valid object provided");
	SCRIPT_ASSERT(false, context, validOrderForObj(order), "Invalid order: %s", getDroidOrderName(order));
	DROID_ORDER_DATA *droidOrder = &psDroid->order;
	if (droidOrder->type == order && psDroid->order.psObj == psObj)
	{
		return true;
	}
	orderDroidObj(psDroid, order, psObj, ModeQueue);
	return true;
}

static int get_first_available_component(int player, int capacity, const wzapi::string_or_string_list& list, COMPONENT_TYPE type, bool strict)
{
	for (const auto& compName : list.strings)
	{
		int result = getCompFromName(type, WzString::fromUtf8(compName));
		if (result >= 0)
		{
			int status = apCompLists[player][type][result];
			if (((status == AVAILABLE || status == REDUNDANT) || !strict)
				&& (type != COMP_BODY || asBodyStats[result].size <= capacity))
			{
				return result; // found one!
			}
		}
		if (result < 0)
		{
			debug(LOG_ERROR, "No such component: %s", compName.c_str());
		}
	}
	return -1; // no available component found in list
}

static std::unique_ptr<DROID_TEMPLATE> makeTemplate(int player, const std::string &templName, const wzapi::string_or_string_list& _body, const wzapi::string_or_string_list& _propulsion, const wzapi::va_list<wzapi::string_or_string_list>& _turrets, int capacity, bool strict)
{
	std::unique_ptr<DROID_TEMPLATE> psTemplate = std::unique_ptr<DROID_TEMPLATE>(new DROID_TEMPLATE);
	size_t numTurrets = _turrets.va_list.size();
	int result;

	memset(psTemplate->asParts, 0, sizeof(psTemplate->asParts)); // reset to defaults
	memset(psTemplate->asWeaps, 0, sizeof(psTemplate->asWeaps));
	int body = get_first_available_component(player, capacity, _body, COMP_BODY, strict);
	if (body < 0)
	{
		debug(LOG_SCRIPT, "Wanted to build %s but body types all unavailable",
		      templName.c_str());
		return nullptr; // no component available
	}
	int prop = get_first_available_component(player, capacity, _propulsion, COMP_PROPULSION, strict);
	if (prop < 0)
	{
		debug(LOG_SCRIPT, "Wanted to build %s but propulsion types all unavailable",
		      templName.c_str());
		return nullptr; // no component available
	}
	psTemplate->asParts[COMP_BODY] = body;
	psTemplate->asParts[COMP_PROPULSION] = prop;

	psTemplate->numWeaps = 0;
	numTurrets = std::min<size_t>(numTurrets, asBodyStats[body].weaponSlots); // Restrict max no. turrets
	if (asBodyStats[body].droidTypeOverride != DROID_ANY)
	{
		psTemplate->droidType = asBodyStats[body].droidTypeOverride; // set droidType based on body
	}
	// Find first turret component type (assume every component in list is same type)
	if (_turrets.va_list.empty() || _turrets.va_list[0].strings.empty())
	{
		debug(LOG_SCRIPT, "Wanted to build %s but no turrets provided", templName.c_str());
		return nullptr;
	}
	std::string compName = _turrets.va_list[0].strings[0];
	COMPONENT_STATS *psComp = getCompStatsFromName(WzString::fromUtf8(compName.c_str()));
	if (psComp == nullptr)
	{
		debug(LOG_ERROR, "Wanted to build %s but %s does not exist", templName.c_str(), compName.c_str());
		return nullptr;
	}
	if (psComp->droidTypeOverride != DROID_ANY)
	{
		psTemplate->droidType = psComp->droidTypeOverride; // set droidType based on component
	}
	if (psComp->compType == COMP_WEAPON)
	{
		for (size_t i = 0; i < std::min<size_t>(numTurrets, MAX_WEAPONS); i++) // may be multi-weapon
		{
			result = get_first_available_component(player, SIZE_NUM, _turrets.va_list[i], COMP_WEAPON, strict);
			if (result < 0)
			{
				debug(LOG_SCRIPT, "Wanted to build %s but no weapon available", templName.c_str());
				return nullptr;
			}
			psTemplate->asWeaps[i] = result;
			psTemplate->numWeaps++;
		}
	}
	else
	{
		if (psComp->compType == COMP_BRAIN)
		{
			psTemplate->numWeaps = 1; // hack, necessary to pass intValidTemplate
		}
		result = get_first_available_component(player, SIZE_NUM, _turrets.va_list[0], psComp->compType, strict);
		if (result < 0)
		{
			debug(LOG_SCRIPT, "Wanted to build %s but turret unavailable", templName.c_str());
			return nullptr;
		}
		psTemplate->asParts[psComp->compType] = result;
	}
	bool valid = intValidTemplate(psTemplate.get(), templName.c_str(), true, player);
	if (valid)
	{
		return psTemplate;
	}
	else
	{
		debug(LOG_ERROR, "Invalid template %s", templName.c_str());
		return nullptr;
	}
}

//-- ## buildDroid(factory, name, body, propulsion, reserved, reserved, turrets...)
//--
//-- Start factory production of new droid with the given name, body, propulsion and turrets.
//-- The reserved parameter should be passed **null** for now. The components can be
//-- passed as ordinary strings, or as a list of strings. If passed as a list, the first available
//-- component in the list will be used. The second reserved parameter used to be a droid type.
//-- It is now unused and in 3.2+ should be passed "", while in 3.1 it should be the
//-- droid type to be built. Returns a boolean that is true if production was started.
//--
bool wzapi::buildDroid(WZAPI_PARAMS(STRUCTURE *psFactory, std::string templName, string_or_string_list body, string_or_string_list propulsion, reservedParam reserved1, reservedParam reserved2, va_list<string_or_string_list> turrets))
{
	STRUCTURE *psStruct = psFactory;
	SCRIPT_ASSERT(false, context, psStruct, "No valid structure provided");
	SCRIPT_ASSERT(false, context, (psStruct->pStructureType->type == REF_FACTORY || psStruct->pStructureType->type == REF_CYBORG_FACTORY
	                        || psStruct->pStructureType->type == REF_VTOL_FACTORY), "Structure %s is not a factory", objInfo(psStruct));
	int player = psStruct->player;
	SCRIPT_ASSERT_PLAYER(false, context, player);
	const int capacity = psStruct->capacity; // body size limit
	SCRIPT_ASSERT(false, context, !turrets.va_list.empty() && !turrets.va_list[0].strings.empty(), "No turrets provided");
	std::unique_ptr<DROID_TEMPLATE> psTemplate = ::makeTemplate(player, templName, body, propulsion, turrets, capacity, true);
	if (psTemplate)
	{
		SCRIPT_ASSERT(false, context, validTemplateForFactory(psTemplate.get(), psStruct, true),
		              "Invalid template %s for factory %s",
		              getStatsName(psTemplate), getStatsName(psStruct->pStructureType));
		// Delete similar template from existing list before adding this one
		for (auto t : apsTemplateList)
		{
			if (t->name.compare(psTemplate->name) == 0)
			{
				debug(LOG_SCRIPT, "deleting %s for player %d", getStatsName(t), player);
				deleteTemplateFromProduction(t, player, ModeQueue); // duplicate? done below?
				break;
			}
		}
		// Add to list
		debug(LOG_SCRIPT, "adding template %s for player %d", getStatsName(psTemplate), player);
		psTemplate->multiPlayerID = generateNewObjectId();
		DROID_TEMPLATE *psAddedTemplate = addTemplate(player, std::move(psTemplate));
		if (!structSetManufacture(psStruct, psAddedTemplate, ModeQueue))
		{
			debug(LOG_ERROR, "Could not produce template %s in %s", getStatsName(psTemplate), objInfo(psStruct));
			return false;
		}
		return true;
	}
	return false;
}

//-- ## addDroid(player, x, y, name, body, propulsion, reserved, reserved, turrets...)
//--
//-- Create and place a droid at the given x, y position as belonging to the given player, built with
//-- the given components. Currently does not support placing droids in multiplayer, doing so will
//-- cause a desync. Returns the created droid on success, otherwise returns null. Passing "" for
//-- reserved parameters is recommended. In 3.2+ only, to create droids in off-world (campaign mission list),
//-- pass -1 as both x and y.
//--
wzapi::returned_nullable_ptr<const DROID> wzapi::addDroid(WZAPI_PARAMS(int player, int x, int y, std::string templName, string_or_string_list body, string_or_string_list propulsion, reservedParam reserved1, reservedParam reserved2, va_list<string_or_string_list> turrets)) MUTLIPLAY_UNSAFE
{
	SCRIPT_ASSERT_PLAYER(nullptr, context, player);
	bool onMission = (x == -1) && (y == -1);
	SCRIPT_ASSERT(nullptr, context, (onMission || (x >= 0 && y >= 0)), "Invalid coordinates (%d, %d) for droid", x, y);
	SCRIPT_ASSERT(nullptr, context, !turrets.va_list.empty() && !turrets.va_list[0].strings.empty(), "No turrets provided");
	std::unique_ptr<DROID_TEMPLATE> psTemplate = ::makeTemplate(player, templName, body, propulsion, turrets, SIZE_NUM, false);
	if (psTemplate)
	{
		DROID *psDroid = nullptr;
		bool oldMulti = bMultiMessages;
		bMultiMessages = false; // ugh, fixme
		if (onMission)
		{
			psDroid = ::buildMissionDroid(psTemplate.get(), 128, 128, player);
			if (psDroid)
			{
				debug(LOG_LIFE, "Created mission-list droid %s by script for player %d: %u", objInfo(psDroid), player, psDroid->id);
			}
			else
			{
				debug(LOG_ERROR, "Invalid droid %s", templName.c_str());
			}
		}
		else
		{
			psDroid = ::buildDroid(psTemplate.get(), world_coord(x) + TILE_UNITS / 2, world_coord(y) + TILE_UNITS / 2, player, onMission, nullptr);
			if (psDroid)
			{
				addDroid(psDroid, apsDroidLists);
				debug(LOG_LIFE, "Created droid %s by script for player %d: %u", objInfo(psDroid), player, psDroid->id);
			}
			else
			{
				debug(LOG_ERROR, "Invalid droid %s", templName.c_str());
			}
		}
		bMultiMessages = oldMulti; // ugh
		return psDroid;
	}
	return nullptr;
}

//-- ## makeTemplate(player, name, body, propulsion, reserved, turrets...)
//--
//-- Create a template (virtual droid) with the given components. Can be useful for calculating the cost
//-- of droids before putting them into production, for instance. Will fail and return null if template
//-- could not possibly be built using current research. (3.2+ only)
//--
std::unique_ptr<const DROID_TEMPLATE> wzapi::makeTemplate(WZAPI_PARAMS(int player, std::string templName, string_or_string_list body, string_or_string_list propulsion, reservedParam reserved1, va_list<string_or_string_list> turrets))
{
	SCRIPT_ASSERT_PLAYER(nullptr, context, player);
	SCRIPT_ASSERT(nullptr, context, !turrets.va_list.empty() && !turrets.va_list[0].strings.empty(), "No turrets provided");
	std::unique_ptr<DROID_TEMPLATE> psTemplate = ::makeTemplate(player, templName, body, propulsion, turrets, SIZE_NUM, true);
	return std::unique_ptr<const DROID_TEMPLATE>(std::move(psTemplate));
}

//-- ## addDroidToTransporter(transporter, droid)
//--
//-- Load a droid, which is currently located on the campaign off-world mission list,
//-- into a transporter, which is also currently on the campaign off-world mission list.
//-- (3.2+ only)
//--
bool wzapi::addDroidToTransporter(WZAPI_PARAMS(game_object_identifier transporter, game_object_identifier droid))
{
	int transporterId = transporter.id;
	int transporterPlayer = transporter.player;
	DROID *psTransporter = IdToMissionDroid(transporterId, transporterPlayer);
	SCRIPT_ASSERT(false, context, psTransporter, "No such transporter id %d belonging to player %d", transporterId, transporterPlayer);
	SCRIPT_ASSERT(false, context, isTransporter(psTransporter), "Droid id %d belonging to player %d is not a transporter", transporterId, transporterPlayer);
	int droidId = droid.id;
	int droidPlayer = droid.player;
	DROID *psDroid = IdToMissionDroid(droidId, droidPlayer);
	SCRIPT_ASSERT(false, context, psDroid, "No such droid id %d belonging to player %d", droidId, droidPlayer);
	SCRIPT_ASSERT(false, context, checkTransporterSpace(psTransporter, psDroid), "Not enough room in transporter %d for droid %d", transporterId, droidId);
	bool removeSuccessful = droidRemove(psDroid, mission.apsDroidLists);
	SCRIPT_ASSERT(false, context, removeSuccessful, "Could not remove droid id %d from mission list", droidId);
	psTransporter->psGroup->add(psDroid);
	return true;
}

//-- ## addFeature(name, x, y)
//--
//-- Create and place a feature at the given x, y position. Will cause a desync in multiplayer.
//-- Returns the created game object on success, null otherwise. (3.2+ only)
//--
wzapi::returned_nullable_ptr<const FEATURE> wzapi::addFeature(WZAPI_PARAMS(std::string featName, int x, int y)) MUTLIPLAY_UNSAFE
{
	int feature = getFeatureStatFromName(WzString::fromUtf8(featName));
	FEATURE_STATS *psStats = &asFeatureStats[feature];
	for (FEATURE *psFeat = apsFeatureLists[0]; psFeat; psFeat = psFeat->psNext)
	{
		SCRIPT_ASSERT(nullptr, context, map_coord(psFeat->pos.x) != x || map_coord(psFeat->pos.y) != y,
		              "Building feature on tile already occupied");
	}
	FEATURE *psFeature = buildFeature(psStats, world_coord(x), world_coord(y), false);
	return psFeature;
}

//-- ## componentAvailable([component type,] component name)
//--
//-- Checks whether a given component is available to the current player. The first argument is
//-- optional and deprecated.
//--
bool wzapi::componentAvailable(WZAPI_PARAMS(std::string arg1, optional<std::string> arg2))
{
	int player = context.player();
	std::string &id = (arg2.has_value()) ? arg2.value() : arg1;
	COMPONENT_STATS *psComp = getCompStatsFromName(WzString::fromUtf8(id.c_str()));
	SCRIPT_ASSERT(false, context, psComp, "No such component: %s", id.c_str());
	int status = apCompLists[player][psComp->compType][psComp->index];
	return (status == AVAILABLE || status == REDUNDANT);
}

//-- ## isVTOL(droid)
//--
//-- Returns true if given droid is a VTOL (not including transports).
//--
bool wzapi::isVTOL(WZAPI_PARAMS(const DROID *psDroid))
{
	SCRIPT_ASSERT(false, context, psDroid, "No valid droid provided");
	return isVtolDroid(psDroid);
}

//-- ## safeDest(player, x, y)
//--
//-- Returns true if given player is safe from hostile fire at the given location, to
//-- the best of that player's map knowledge. Does not work in campaign at the moment.
//--
bool wzapi::safeDest(WZAPI_PARAMS(int player, int x, int y))
{
	SCRIPT_ASSERT_PLAYER(false, context, player);
	SCRIPT_ASSERT(false, context, tileOnMap(x, y), "Out of bounds coordinates(%d, %d)", x, y);
	return (!(auxTile(x, y, player) & AUXBITS_DANGER));
}

//-- ## activateStructure(structure, [target[, ability]])
//--
//-- Activate a special ability on a structure. Currently only works on the lassat.
//-- The lassat needs a target.
//--
bool wzapi::activateStructure(WZAPI_PARAMS(STRUCTURE *psStruct, optional<BASE_OBJECT *> _psTarget))
{
	SCRIPT_ASSERT(false, context, psStruct, "No valid structure provided");
	int player = psStruct->player;
	// ... and then do nothing with psStruct yet
	BASE_OBJECT *psTarget = (_psTarget.has_value()) ? _psTarget.value() : nullptr;
	SCRIPT_ASSERT(false, context, psTarget, "No valid target provided");
	orderStructureObj(player, psTarget);
	return true;
}

//-- ## chat(target player, message)
//--
//-- Send a message to target player. Target may also be ```ALL_PLAYERS``` or ```ALLIES```.
//-- Returns a boolean that is true on success. (3.2+ only)
//--
bool wzapi::chat(WZAPI_PARAMS(int target, std::string message))
{
	int player = context.player();
	SCRIPT_ASSERT(false, context, target >= 0 || target == ALL_PLAYERS || target == ALLIES, "Message to invalid player %d", target);
	auto chatMessage = InGameChatMessage(player, message.c_str());
	if (target == ALLIES) // allies
	{
		chatMessage.toAllies = true;
	}
	else if (target != ALL_PLAYERS) // specific player
	{
		chatMessage.addReceiverByIndex(target);
	}

	chatMessage.send();
	return true;
}

//-- ## addBeacon(x, y, target player[, message])
//--
//-- Send a beacon message to target player. Target may also be ```ALLIES```.
//-- Message is currently unused. Returns a boolean that is true on success. (3.2+ only)
//--
bool wzapi::addBeacon(WZAPI_PARAMS(int _x, int _y, int target, optional<std::string> _message))
{
	int x = world_coord(_x);
	int y = world_coord(_y);

	std::string message;
	if (_message.has_value())
	{
		message = _message.value();
	}
	int me = context.player();
	SCRIPT_ASSERT(false, context, target >= 0 || target == ALLIES, "Message to invalid player %d", target);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (i != me && (i == target || (target == ALLIES && aiCheckAlliances(i, me))))
		{
			debug(LOG_MSG, "adding script beacon to %d from %d", i, me);
			sendBeaconToPlayer(x, y, i, me, message.c_str());
		}
	}
	return true;
}

//-- ## removeBeacon(target player)
//--
//-- Remove a beacon message sent to target player. Target may also be ```ALLIES```.
//-- Returns a boolean that is true on success. (3.2+ only)
//--
bool wzapi::removeBeacon(WZAPI_PARAMS(int target))
{
	int me = context.player();

	SCRIPT_ASSERT(false, context, target >= 0 || target == ALLIES, "Message to invalid player %d", target);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (i == target || (target == ALLIES && aiCheckAlliances(i, me)))
		{
			MESSAGE *psMessage = findBeaconMsg(i, me);
			if (psMessage)
			{
				removeMessage(psMessage, i);
				triggerEventBeaconRemoved(me, i);
			}
		}
	}
//	jsDebugMessageUpdate();
	return true;
}

//-- ## getDroidProduction(factory)
//--
//-- Return droid in production in given factory. Note that this droid is fully
//-- virtual, and should never be passed anywhere. (3.2+ only)
//--
std::unique_ptr<const DROID> wzapi::getDroidProduction(WZAPI_PARAMS(const STRUCTURE *_psFactory))
{
	const STRUCTURE *psStruct = _psFactory;
	SCRIPT_ASSERT(nullptr, context, psStruct, "No valid structure provided");
	int player = psStruct->player;
	SCRIPT_ASSERT(nullptr, context, psStruct->pStructureType->type == REF_FACTORY
				  || psStruct->pStructureType->type == REF_CYBORG_FACTORY
				  || psStruct->pStructureType->type == REF_VTOL_FACTORY, "Structure not a factory");
	const FACTORY *psFactory = &psStruct->pFunctionality->factory;
	const DROID_TEMPLATE *psTemp = psFactory->psSubject;
	if (!psTemp)
	{
		return nullptr;
	}
	DROID *psDroid = new DROID(0, player);
	psDroid->pos = psStruct->pos;
	psDroid->rot = psStruct->rot;
	psDroid->experience = 0;
	droidSetName(psDroid, getStatsName(psTemp));
	droidSetBits(psTemp, psDroid);
	psDroid->weight = calcDroidWeight(psTemp);
	psDroid->baseSpeed = calcDroidBaseSpeed(psTemp, psDroid->weight, player);
	return std::unique_ptr<const DROID>(psDroid);
}

//-- ## getDroidLimit([player[, unit type]])
//--
//-- Return maximum number of droids that this player can produce. This limit is usually
//-- fixed throughout a game and the same for all players. If no arguments are passed,
//-- returns general unit limit for the current player. If a second, unit type argument
//-- is passed, the limit for this unit type is returned, which may be different from
//-- the general unit limit (eg for commanders and construction droids). (3.2+ only)
//--
int wzapi::getDroidLimit(WZAPI_PARAMS(optional<int> _player, optional<int> _unitType))
{
	int player = (_player.has_value()) ? _player.value() : context.player();
	SCRIPT_ASSERT_PLAYER(false, context, player);
	if (_unitType.has_value())
	{
		DROID_TYPE type = (DROID_TYPE)_unitType.value();
		if (type == DROID_COMMAND)
		{
			return getMaxCommanders(player);
		}
		else if (type == DROID_CONSTRUCT)
		{
			return getMaxConstructors(player);
		}
		// else return general unit limit
	}
	return getMaxDroids(player);
}

//-- ## getExperienceModifier(player)
//--
//-- Get the percentage of experience this player droids are going to gain. (3.2+ only)
//--
int wzapi::getExperienceModifier(WZAPI_PARAMS(int player))
{
	SCRIPT_ASSERT_PLAYER(false, context, player);
	return getExpGain(player);
}

//-- ## setDroidLimit(player, value[, droid type])
//--
//-- Set the maximum number of droids that this player can produce. If a third
//-- parameter is added, this is the droid type to limit. It can be DROID_ANY
//-- for droids in general, DROID_CONSTRUCT for constructors, or DROID_COMMAND
//-- for commanders. (3.2+ only)
//--
bool wzapi::setDroidLimit(WZAPI_PARAMS(int player, int value, optional<int> _droidType))
{
	SCRIPT_ASSERT_PLAYER(false, context, player);
	DROID_TYPE type = (_droidType.has_value()) ? (DROID_TYPE)_droidType.value() : DROID_ANY;

	switch (type)
	{
	case DROID_CONSTRUCT:
		setMaxConstructors(player, value);
		break;
	case DROID_COMMAND:
		setMaxCommanders(player, value);
		break;
	default:
	case DROID_ANY:
		setMaxDroids(player, value);
		break;
	}
	return true;
}

//-- ## setCommanderLimit(player, value)
//--
//-- Set the maximum number of commanders that this player can produce.
//-- THIS FUNCTION IS DEPRECATED AND WILL BE REMOVED! (3.2+ only)
//--
bool wzapi::setCommanderLimit(WZAPI_PARAMS(int player, int value))
{
	SCRIPT_ASSERT_PLAYER(false, context, player);
	setMaxCommanders(player, value);
	return true;
}

//-- ## setConstructorLimit(player, value)
//--
//-- Set the maximum number of constructors that this player can produce.
//-- THIS FUNCTION IS DEPRECATED AND WILL BE REMOVED! (3.2+ only)
//--
bool wzapi::setConstructorLimit(WZAPI_PARAMS(int player, int value))
{
	SCRIPT_ASSERT_PLAYER(false, context, player);
	setMaxConstructors(player, value);
	return true;
}

//-- ## setExperienceModifier(player, percent)
//--
//-- Set the percentage of experience this player droids are going to gain. (3.2+ only)
//--
bool wzapi::setExperienceModifier(WZAPI_PARAMS(int player, int percent))
{
	SCRIPT_ASSERT_PLAYER(false, context, player);
	setExpGain(player, percent);
	return true;
}

//-- ## enumCargo(transport droid)
//--
//-- Returns an array of droid objects inside given transport. (3.2+ only)
//--
std::vector<const DROID *> wzapi::enumCargo(WZAPI_PARAMS(const DROID *psDroid))
{
	SCRIPT_ASSERT({}, context, psDroid, "No valid droid provided");
	SCRIPT_ASSERT({}, context, isTransporter(psDroid), "Wrong droid type (expecting: transporter)");
	std::vector<const DROID *> result;
	result.reserve(psDroid->psGroup->getNumMembers());
	int i = 0;
	for (DROID *psCurr = psDroid->psGroup->psList; psCurr; psCurr = psCurr->psGrpNext, i++)
	{
		if (psDroid != psCurr)
		{
			result.push_back(psCurr);
		}
	}
	return result;
}

//-- ## isSpectator(player)
//--
//-- Returns whether a particular player is a spectator. (4.2+ only)
//-- Can pass -1 as player to get the spectator status of the client running the script. (Useful for the "rules" scripts.)
//--
bool wzapi::isSpectator(WZAPI_PARAMS(int player))
{
	SCRIPT_ASSERT(false, _context, player == -1 || (player >= 0 && (player < NetPlay.players.size() || player == selectedPlayer)), "Invalid player index %d", player);
	if (player == -1 || player == selectedPlayer)
	{
		// TODO: Offers the ability to store this outside of NetPlayer.players later
		// For now, it's stored in NetPlay.players[selectedPlayer]
		return NetPlay.players[selectedPlayer].isSpectator;
	}
	else if (player >= 0 && player < NetPlay.players.size())
	{
		return NetPlay.players[player].isSpectator;
	}
	return true;
}

//-- ## getWeaponInfo(weapon id)
//--
//-- Return information about a particular weapon type. DEPRECATED - query the Stats object instead. (3.2+ only)
//--
nlohmann::json wzapi::getWeaponInfo(WZAPI_PARAMS(std::string weaponID)) WZAPI_DEPRECATED
{
	int idx = getCompFromName(COMP_WEAPON, WzString::fromUtf8(weaponID));
	SCRIPT_ASSERT(nlohmann::json(), context, idx >= 0, "No such weapon: %s", weaponID.c_str());
	WEAPON_STATS *psStats = asWeaponStats + idx;
	nlohmann::json result = nlohmann::json::object();
	result["id"] = weaponID;
	result["name"] = psStats->name;
	result["impactClass"] = psStats->weaponClass == WC_KINETIC ? "KINETIC" : "HEAT";
	result["damage"] = psStats->base.damage;
	result["firePause"] = psStats->base.firePause;
	result["fireOnMove"] = psStats->fireOnMove;
	return result;
}

// MARK: - Functions that operate on the current player only

//-- ## centreView(x, y)
//--
//-- Center the player's camera at the given position.
//--
bool wzapi::centreView(WZAPI_PARAMS(int x, int y))
{
	setViewPos(x, y, false);
	return true;
}

//-- ## playSound(sound[, x, y, z])
//--
//-- Play a sound, optionally at a location.
//--
bool wzapi::playSound(WZAPI_PARAMS(std::string sound, optional<int> _x, optional<int> _y, optional<int> _z))
{
	int player = context.player();
	if (player != selectedPlayer)
	{
		return false;
	}
	int soundID = audio_GetTrackID(sound.c_str());
	if (soundID == SAMPLE_NOT_FOUND)
	{
		soundID = audio_SetTrackVals(sound.c_str(), false, 100, 1800);
	}
	if (_x.has_value())
	{
		int x = world_coord(_x.value());
		int y = world_coord((_y.has_value()) ? _y.value() : 0);
		int z = world_coord((_z.has_value()) ? _z.value() : 0);
		audio_QueueTrackPos(soundID, x, y, z);
	}
	else
	{
		audio_QueueTrack(soundID);
	}
	return true;
}

//-- ## gameOverMessage(won, showBackDrop, showOutro)
//--
//-- End game in victory or defeat.
//--
bool wzapi::gameOverMessage(WZAPI_PARAMS(bool gameWon, optional<bool> _showBackDrop, optional<bool> _showOutro))
{
	int player = context.player();
	const MESSAGE_TYPE msgType = MSG_MISSION;	// always
	bool showOutro = false;
	bool showBackDrop = true;
	if (_showBackDrop.has_value())
	{
		showBackDrop = _showBackDrop.value();
	}
	if (_showOutro.has_value())
	{
		showOutro = _showOutro.value();
	}
	VIEWDATA *psViewData;
	if (gameWon)
	{
		//Quick hack to stop assert when trying to play outro in campaign.
		psViewData = !bMultiPlayer && showOutro ? getViewData("END") : getViewData("WIN");
		addConsoleMessage(_("YOU ARE VICTORIOUS!"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	}
	else
	{
		psViewData = getViewData("END");	// FIXME: rename to FAILED|LOST ?
		if (!testPlayerHasLost()) // check for whether the player started as a spectator or already lost (in either case the player is already marked as having lost)
		{
			addConsoleMessage(_("YOU WERE DEFEATED!"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
		}
	}
	ASSERT(psViewData, "Viewdata not found");
	MESSAGE *psMessage = addMessage(msgType, false, player);
	if (!bMultiPlayer && psMessage)
	{
		//we need to set this here so the VIDEO_QUIT callback is not called
		setScriptWinLoseVideo(gameWon ? PLAY_WIN : PLAY_LOSE);
		seq_ClearSeqList();
		if (gameWon && showOutro)
		{
			showBackDrop = false;
			seq_AddSeqToList("outro.ogg", nullptr, "outro.txa", false);
			seq_StartNextFullScreenVideo();
		}
		else
		{
			//set the data
			psMessage->pViewData = psViewData;
			displayImmediateMessage(psMessage);
			stopReticuleButtonFlash(IDRET_INTEL_MAP);
		}
	}
//	jsDebugMessageUpdate();
	displayGameOver(gameWon, showBackDrop);
	if (challengeActive)
	{
		updateChallenge(gameWon);
	}
	if (autogame_enabled())
	{
		debug(LOG_WARNING, "Autogame completed successfully!");
		if (headlessGameMode())
		{
			stdOutGameSummary(0);
		}
		exit(0);
	}
	return true;
}

// MARK: - Global state manipulation -- not for use with skirmish AI (unless you want it to cheat, obviously)

//-- ## setStructureLimits(structure type, limit[, player])
//--
//-- Set build limits for a structure.
//--
bool wzapi::setStructureLimits(WZAPI_PARAMS(std::string building, int limit, optional<int> _player))
{
	int structInc = getStructStatFromName(WzString::fromUtf8(building));
	int player = (_player.has_value()) ? _player.value() : context.player();
	SCRIPT_ASSERT_PLAYER(false, context, player);
	SCRIPT_ASSERT(false, context, limit < LOTS_OF && limit >= 0, "Invalid limit");
	SCRIPT_ASSERT(false, context, structInc < numStructureStats && structInc >= 0, "Invalid structure");

	asStructureStats[structInc].upgrade[player].limit = limit;

	return true;
}

//-- ## applyLimitSet()
//--
//-- Mix user set limits with script set limits and defaults.
//--
bool wzapi::applyLimitSet(WZAPI_NO_PARAMS)
{
	return ::applyLimitSet();
}

//-- ## setMissionTime(time)
//--
//-- Set mission countdown in seconds.
//--
wzapi::no_return_value wzapi::setMissionTime(WZAPI_PARAMS(int _value))
{
	int value = _value * GAME_TICKS_PER_SEC;
	mission.startTime = gameTime;
	mission.time = value;
	setMissionCountDown();
	if (mission.time >= 0)
	{
		mission.startTime = gameTime;
		addMissionTimerInterface();
	}
	else
	{
		intRemoveMissionTimer();
		mission.cheatTime = 0;
	}
	return {};
}

//-- ## getMissionTime()
//--
//-- Get time remaining on mission countdown in seconds. (3.2+ only)
//--
int wzapi::getMissionTime(WZAPI_NO_PARAMS)
{
	return ((mission.time - (gameTime - mission.startTime)) / GAME_TICKS_PER_SEC);
}

//-- ## setReinforcementTime(time)
//--
//-- Set time for reinforcements to arrive. If time is negative, the reinforcement GUI
//-- is removed and the timer stopped. Time is in seconds.
//-- If time equals to the magic LZ_COMPROMISED_TIME constant, reinforcement GUI ticker
//-- is set to "--:--" and reinforcements are suppressed until this function is called
//-- again with a regular time value.
//--
wzapi::no_return_value wzapi::setReinforcementTime(WZAPI_PARAMS(int _value))
{
	int value = _value * GAME_TICKS_PER_SEC;
	SCRIPT_ASSERT({}, context, value == LZ_COMPROMISED_TIME || value < 60 * 60 * GAME_TICKS_PER_SEC,
	              "The transport timer cannot be set to more than 1 hour!");
	SCRIPT_ASSERT({}, context, selectedPlayer < MAX_PLAYERS, "Invalid selectedPlayer for current client: %" PRIu32 "", selectedPlayer);
	mission.ETA = value;
	if (missionCanReEnforce())
	{
		addTransporterTimerInterface();
	}
	if (value < 0)
	{
		DROID *psDroid;

		intRemoveTransporterTimer();
		/* Only remove the launch if haven't got a transporter droid since the scripts set the
		 * time to -1 at the between stage if there are not going to be reinforcements on the submap  */
		for (psDroid = apsDroidLists[selectedPlayer]; psDroid != nullptr; psDroid = psDroid->psNext)
		{
			if (isTransporter(psDroid))
			{
				break;
			}
		}
		// if not found a transporter, can remove the launch button
		if (psDroid ==  nullptr)
		{
			intRemoveTransporterLaunch();
		}
	}
	return {};
}

//-- ## completeResearch(research[, player [, forceResearch]])
//--
//-- Finish a research for the given player.
//-- forceResearch will allow a research topic to be researched again. 3.3+
//--
wzapi::no_return_value wzapi::completeResearch(WZAPI_PARAMS(std::string researchName, optional<int> _player, optional<bool> _forceResearch))
{
	int player = (_player.has_value()) ? _player.value() : context.player();
	SCRIPT_ASSERT_PLAYER({}, context, player);
	bool forceIt = (_forceResearch.has_value()) ? _forceResearch.value() : false;
	RESEARCH *psResearch = ::getResearch(researchName.c_str());
	SCRIPT_ASSERT({}, context, psResearch, "No such research %s for player %d", researchName.c_str(), player);
	SCRIPT_ASSERT({}, context, psResearch->index < asResearch.size(), "Research index out of bounds");
	PLAYER_RESEARCH *plrRes = &asPlayerResList[player][psResearch->index];
	if (!forceIt && IsResearchCompleted(plrRes))
	{
		return {};
	}
	if (bMultiMessages && (gameTime > 2)) // ??? "gameTime > 2" ??
	{
		SendResearch(player, psResearch->index, false);
		// Wait for our message before doing anything.
	}
	else
	{
		::researchResult(psResearch->index, player, false, nullptr, false);
	}
	return {};
}

//-- ## completeAllResearch([player])
//--
//-- Finish all researches for the given player.
//--
wzapi::no_return_value wzapi::completeAllResearch(WZAPI_PARAMS(optional<int> _player))
{
	int player = (_player.has_value()) ? _player.value() : context.player();
	SCRIPT_ASSERT_PLAYER({}, context, player);
	for (int i = 0; i < asResearch.size(); i++)
	{
		RESEARCH *psResearch = &asResearch[i];
		PLAYER_RESEARCH *plrRes = &asPlayerResList[player][psResearch->index];
		if (!IsResearchCompleted(plrRes))
		{
			if (bMultiMessages && (gameTime > 2))
			{
				SendResearch(player, psResearch->index, false);
				// Wait for our message before doing anything.
			}
			else
			{
				::researchResult(psResearch->index, player, false, nullptr, false);
			}
		}
	}
	return {};
}

//-- ## enableResearch(research[, player])
//--
//-- Enable a research for the given player, allowing it to be researched.
//--
bool wzapi::enableResearch(WZAPI_PARAMS(std::string researchName, optional<int> _player))
{
	int player = (_player.has_value()) ? _player.value() : context.player();
	SCRIPT_ASSERT_PLAYER(false, context, player);
	RESEARCH *psResearch = ::getResearch(researchName.c_str());
	SCRIPT_ASSERT(false, context, psResearch, "No such research %s for player %d", researchName.c_str(), player);
	if (!::enableResearch(psResearch, player))
	{
		debug(LOG_ERROR, "Unable to enable research %s for player %d", researchName.c_str(), player);
		return false;
	}
	return true;
}

//-- ## setPower(power[, player])
//--
//-- Set a player's power directly. (Do not use this in an AI script.)
//--
wzapi::no_return_value wzapi::setPower(WZAPI_PARAMS(int power, optional<int> _player)) WZAPI_AI_UNSAFE
{
	int player = (_player.has_value()) ? _player.value() : context.player();
	SCRIPT_ASSERT_PLAYER({}, context, player);
	::setPower(player, power);
	return {};
}

//-- ## setPowerModifier(power[, player])
//--
//-- Set a player's power modifier percentage. (Do not use this in an AI script.) (3.2+ only)
//--
wzapi::no_return_value wzapi::setPowerModifier(WZAPI_PARAMS(int power, optional<int> _player)) WZAPI_AI_UNSAFE
{
	int player = (_player.has_value()) ? _player.value() : context.player();
	SCRIPT_ASSERT_PLAYER({}, context, player);
	::setPowerModifier(player, power);
	return {};
}

//-- ## setPowerStorageMaximum(maximum[, player])
//--
//-- Set a player's power storage maximum. (Do not use this in an AI script.) (3.2+ only)
//--
wzapi::no_return_value wzapi::setPowerStorageMaximum(WZAPI_PARAMS(int power, optional<int> _player)) WZAPI_AI_UNSAFE
{
	int player = (_player.has_value()) ? _player.value() : context.player();
	SCRIPT_ASSERT_PLAYER({}, context, player);
	::setPowerMaxStorage(player, power);
	return {};
}

//-- ## extraPowerTime(time, player)
//--
//-- Increase a player's power as if that player had power income equal to current income
//-- over the given amount of extra time. (3.2+ only)
//--
wzapi::no_return_value wzapi::extraPowerTime(WZAPI_PARAMS(int time, optional<int> _player))
{
	int ticks = time * GAME_UPDATES_PER_SEC;
	int player = (_player.has_value()) ? _player.value() : context.player();
	SCRIPT_ASSERT_PLAYER({}, context, player);
	updatePlayerPower(player, ticks);
	return {};
}

//-- ## setTutorialMode(bool)
//--
//-- Sets a number of restrictions appropriate for tutorial if set to true.
//--
wzapi::no_return_value wzapi::setTutorialMode(WZAPI_PARAMS(bool tutorialMode))
{
	bInTutorial = tutorialMode;
	return {};
}

//-- ## setDesign(bool)
//--
//-- Whether to allow player to design stuff.
//--
wzapi::no_return_value wzapi::setDesign(WZAPI_PARAMS(bool allowDesignValue))
{
	DROID_TEMPLATE *psCurr;
	// Switch on or off future templates
	// FIXME: This dual data structure for templates is just plain insane.
	enumerateTemplates(selectedPlayer, [allowDesignValue](DROID_TEMPLATE * psTempl) {
		bool researched = researchedTemplate(psTempl, selectedPlayer, true);
		psTempl->enabled = (researched || allowDesignValue);
		return true;
	});
	for (auto &localTemplate : localTemplates)
	{
		psCurr = &localTemplate;
		bool researched = researchedTemplate(psCurr, selectedPlayer, true);
		psCurr->enabled = (researched || allowDesignValue);
	}
	return {};
}

//-- ## enableTemplate(template name)
//--
//-- Enable a specific template (even if design is disabled).
//--
bool wzapi::enableTemplate(WZAPI_PARAMS(std::string _templateName))
{
	DROID_TEMPLATE *psCurr;
	WzString templateName = WzString::fromUtf8(_templateName);
	bool found = false;
	// FIXME: This dual data structure for templates is just plain insane.
	enumerateTemplates(selectedPlayer, [&templateName, &found](DROID_TEMPLATE * psTempl) {
		if (templateName.compare(psTempl->id) == 0)
		{
			psTempl->enabled = true;
			found = true;
			return false; // break;
		}
		return true;
	});
	if (!found)
	{
		debug(LOG_ERROR, "Template %s was not found!", templateName.toUtf8().c_str());
		return false;
	}
	for (auto &localTemplate : localTemplates)
	{
		psCurr = &localTemplate;
		if (templateName.compare(psCurr->id) == 0)
		{
			psCurr->enabled = true;
			break;
		}
	}
	return true;
}

//-- ## removeTemplate(template name)
//--
//-- Remove a template.
//--
bool wzapi::removeTemplate(WZAPI_PARAMS(std::string _templateName))
{
	DROID_TEMPLATE *psCurr;
	WzString templateName = WzString::fromUtf8(_templateName);
	bool found = false;
	// FIXME: This dual data structure for templates is just plain insane.
	enumerateTemplates(selectedPlayer, [&templateName, &found](DROID_TEMPLATE * psTempl) {
		if (templateName.compare(psTempl->id) == 0)
		{
			psTempl->enabled = false;
			found = true;
			return false; // break;
		}
		return true;
	});
	if (!found)
	{
		debug(LOG_ERROR, "Template %s was not found!", templateName.toUtf8().c_str());
		return false;
	}
	for (std::list<DROID_TEMPLATE>::iterator i = localTemplates.begin(); i != localTemplates.end(); ++i)
	{
		psCurr = &*i;
		if (templateName.compare(psCurr->id) == 0)
		{
			localTemplates.erase(i);
			break;
		}
	}
	return true;
}

//-- ## setMiniMap(bool)
//--
//-- Turns visible minimap on or off in the GUI.
//--
wzapi::no_return_value wzapi::setMiniMap(WZAPI_PARAMS(bool visible))
{
	radarPermitted = visible;
	return {};
}

//-- ## setReticuleButton(id, tooltip, filename, filenameHigh, callback)
//--
//-- Add reticule button. id is which button to change, where zero is zero is the middle button, then going clockwise from the
//-- uppermost button. filename is button graphics and filenameHigh is for highlighting. The tooltip is the text you see when
//-- you mouse over the button. Finally, the callback is which scripting function to call. Hide and show the user interface
//-- for such changes to take effect. (3.2+ only)
//--
wzapi::no_return_value wzapi::setReticuleButton(WZAPI_PARAMS(int buttonID, std::string tooltip, std::string filename, std::string filenameDown, optional<std::string> callbackFuncName))
{
	int button = buttonID;
	SCRIPT_ASSERT({}, context, button >= 0 && button <= 6, "Invalid button %d", button);

	WzString func;
	if (callbackFuncName.has_value())
	{
		func = WzString::fromUtf8(callbackFuncName.value());
	}
	if (MissionResUp)
	{
		return {}; // no-op
	}
	setReticuleStats(button, tooltip, filename, filenameDown, (!func.isEmpty()) ? context.getNamedScriptCallback(func) : nullptr);
	return {};
}

//-- ## setReticuleFlash(id, flash)
//--
//-- Set reticule flash on or off. (3.2.3+ only)
//--
wzapi::no_return_value wzapi::setReticuleFlash(WZAPI_PARAMS(int button, bool flash))
{
	SCRIPT_ASSERT({}, context, button >= 0 && button <= 6, "Invalid button %d", button);
	::setReticuleFlash(button, flash);
	return {};
}

//-- ## showReticuleWidget(id)
//--
//-- Open the reticule menu widget. (3.3+ only)
//--
wzapi::no_return_value wzapi::showReticuleWidget(WZAPI_PARAMS(int button))
{
	SCRIPT_ASSERT({}, context, button >= 0 && button <= 6, "Invalid button %d", button);
	intShowWidget(button);
	return {};
}

//-- ## showInterface()
//--
//-- Show user interface. (3.2+ only)
//--
wzapi::no_return_value wzapi::showInterface(WZAPI_NO_PARAMS)
{
	intAddReticule();
	intShowPowerBar();
	return {};
}

//-- ## hideInterface()
//--
//-- Hide user interface. (3.2+ only)
//--
wzapi::no_return_value wzapi::hideInterface(WZAPI_NO_PARAMS)
{
	intRemoveReticule();
	intHidePowerBar();
	return {};
}

//-- ## enableStructure(structure type[, player])
//--
//-- The given structure type is made available to the given player. It will appear in the
//-- player's build list.
//--
wzapi::no_return_value wzapi::enableStructure(WZAPI_PARAMS(std::string structureName, optional<int> _player))
{
	int index = getStructStatFromName(WzString::fromUtf8(structureName));
	int player = (_player.has_value()) ? _player.value() : context.player();
	SCRIPT_ASSERT_PLAYER({}, context, player);
	SCRIPT_ASSERT({}, context, index >= 0 && index < numStructureStats, "Invalid structure stat");
	// enable the appropriate structure
	apStructTypeLists[player][index] = AVAILABLE;
	return {};
}

static void setComponent(const std::string& name, int player, int value)
{
	COMPONENT_STATS *psComp = getCompStatsFromName(WzString::fromUtf8(name));
	ASSERT_OR_RETURN(, psComp, "Bad component %s", name.c_str());
	apCompLists[player][psComp->compType][psComp->index] = value;
}

//-- ## enableComponent(component, player)
//--
//-- The given component is made available for research for the given player.
//--
wzapi::no_return_value wzapi::enableComponent(WZAPI_PARAMS(std::string componentName, int player))
{
	SCRIPT_ASSERT_PLAYER({}, context, player);
	setComponent(componentName, player, FOUND);
	return {};
}

//-- ## makeComponentAvailable(component, player)
//--
//-- The given component is made available to the given player. This means the player can
//-- actually build designs with it.
//--
wzapi::no_return_value wzapi::makeComponentAvailable(WZAPI_PARAMS(std::string componentName, int player))
{
	SCRIPT_ASSERT_PLAYER({}, context, player);
	setComponent(componentName, player, AVAILABLE);
	return {};
}

//-- ## allianceExistsBetween(player, player)
//--
//-- Returns true if an alliance exists between the two players, or they are the same player.
//--
bool wzapi::allianceExistsBetween(WZAPI_PARAMS(int player1, int player2))
{
	SCRIPT_ASSERT(false, context, player1 < MAX_PLAYERS && player1 >= 0, "Invalid player");
	SCRIPT_ASSERT(false, context, player2 < MAX_PLAYERS && player2 >= 0, "Invalid player");
	return (alliances[player1][player2] == ALLIANCE_FORMED);
}

//-- ## removeStruct(structure)
//--
//-- Immediately remove the given structure from the map. Returns a boolean that is true on success.
//-- No special effects are applied. Deprecated since 3.2. Use `removeObject` instead.
//--
bool wzapi::removeStruct(WZAPI_PARAMS(STRUCTURE *psStruct)) WZAPI_DEPRECATED
{
	SCRIPT_ASSERT(false, context, psStruct, "No valid structure provided");
	return removeStruct(psStruct, true);
}

//-- ## removeObject(game object[, special effects?])
//--
//-- Remove the given game object with special effects. Returns a boolean that is true on success.
//-- A second, optional boolean parameter specifies whether special effects are to be applied. (3.2+ only)
//--
bool wzapi::removeObject(WZAPI_PARAMS(BASE_OBJECT *psObj, optional<bool> _sfx))
{
	SCRIPT_ASSERT(false, context, psObj, "No valid object provided");
	bool sfx = (_sfx.has_value()) ? _sfx.value() : false;

	bool retval = false;
	if (sfx)
	{
		switch (psObj->type)
		{
		case OBJ_STRUCTURE: destroyStruct((STRUCTURE *)psObj, gameTime); break;
		case OBJ_DROID: retval = destroyDroid((DROID *)psObj, gameTime); break;
		case OBJ_FEATURE: retval = destroyFeature((FEATURE *)psObj, gameTime); break;
		default: SCRIPT_ASSERT(false, context, false, "Wrong game object type"); break;
		}
	}
	else
	{
		switch (psObj->type)
		{
		case OBJ_STRUCTURE: retval = removeStruct((STRUCTURE *)psObj, true); break;
		case OBJ_DROID: retval = removeDroidBase((DROID *)psObj); break;
		case OBJ_FEATURE: retval = removeFeature((FEATURE *)psObj); break;
		default: SCRIPT_ASSERT(false, context, false, "Wrong game object type"); break;
		}
	}
	return retval;
}

//-- ## setScrollLimits(x1, y1, x2, y2)
//--
//-- Limit the scrollable area of the map to the given rectangle. (3.2+ only)
//--
wzapi::no_return_value wzapi::setScrollLimits(WZAPI_PARAMS(int x1, int y1, int x2, int y2))
{
	const int minX = x1;
	const int minY = y1;
	const int maxX = x2;
	const int maxY = y2;

	SCRIPT_ASSERT({}, context, minX >= 0, "Minimum scroll x value %d is less than zero - ", minX);
	SCRIPT_ASSERT({}, context, minY >= 0, "Minimum scroll y value %d is less than zero - ", minY);
	SCRIPT_ASSERT({}, context, maxX <= mapWidth, "Maximum scroll x value %d is greater than mapWidth %d", maxX, (int)mapWidth);
	SCRIPT_ASSERT({}, context, maxY <= mapHeight, "Maximum scroll y value %d is greater than mapHeight %d", maxY, (int)mapHeight);

	const int prevMinX = scrollMinX;
	const int prevMinY = scrollMinY;
	const int prevMaxX = scrollMaxX;
	const int prevMaxY = scrollMaxY;

	scrollMinX = minX;
	scrollMaxX = maxX;
	scrollMinY = minY;
	scrollMaxY = maxY;

	// When the scroll limits change midgame - need to redo the lighting
	initLighting(prevMinX < scrollMinX ? prevMinX : scrollMinX,
	             prevMinY < scrollMinY ? prevMinY : scrollMinY,
	             prevMaxX < scrollMaxX ? prevMaxX : scrollMaxX,
	             prevMaxY < scrollMaxY ? prevMaxY : scrollMaxY);

	// need to reset radar to take into account of new size
	resizeRadar();
	return {};
}

//-- ## getScrollLimits()
//--
//-- Get the limits of the scrollable area of the map as an area object. (3.2+ only)
//--
scr_area wzapi::getScrollLimits(WZAPI_NO_PARAMS)
{
	scr_area limits;
	limits.x1 = scrollMinX;
	limits.y1 = scrollMinY;
	limits.x2 = scrollMaxX;
	limits.y2 = scrollMaxY;
	return limits;
}

//-- ## addStructure(structure id, player, x, y)
//--
//-- Create a structure on the given position. Returns the structure on success, null otherwise.
//-- Position uses world coordinates, if you want use position based on Map Tiles, then
//-- use as addStructure(structure id, players, x*128, y*128)
//--
wzapi::returned_nullable_ptr<const STRUCTURE> wzapi::addStructure(WZAPI_PARAMS(std::string structureName, int player, int x, int y))
{
	int index = getStructStatFromName(WzString::fromUtf8(structureName.c_str()));
	SCRIPT_ASSERT(nullptr, context, index >= 0, "%s not found", structureName.c_str());
	SCRIPT_ASSERT_PLAYER(nullptr, context, player);

	STRUCTURE_STATS *psStat = &asStructureStats[index];
	STRUCTURE *psStruct = buildStructure(psStat, x, y, player, false);
	if (psStruct)
	{
		psStruct->status = SS_BUILT;
		buildingComplete(psStruct);
		return psStruct;
	}
	return nullptr;
}

//-- ## getStructureLimit(structure type[, player])
//--
//-- Returns build limits for a structure.
//--
unsigned int wzapi::getStructureLimit(WZAPI_PARAMS(std::string structureName, optional<int> _player))
{
	int index = getStructStatFromName(WzString::fromUtf8(structureName.c_str()));
	SCRIPT_ASSERT(0, context, index >= 0, "%s not found", structureName.c_str());
	int player = (_player.has_value()) ? _player.value() : context.player();
	SCRIPT_ASSERT_PLAYER(0, context, player);
	return (asStructureStats[index].upgrade[player].limit);
}

//-- ## countStruct(structure type[, player])
//--
//-- Count the number of structures of a given type.
//-- The player parameter can be a specific player, ALL_PLAYERS, ALLIES or ENEMIES.
//--
int wzapi::countStruct(WZAPI_PARAMS(std::string structureName, optional<int> _player))
{
	int index = getStructStatFromName(WzString::fromUtf8(structureName.c_str()));
	SCRIPT_ASSERT(-1, context, index < numStructureStats && index >= 0, "Structure %s not found", structureName.c_str());
	int me = context.player();
	int player = (_player.has_value()) ? _player.value() : me;

	int quantity = 0;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (player == i || player == ALL_PLAYERS
		    || (player == ALLIES && aiCheckAlliances(i, me))
		    || (player == ENEMIES && !aiCheckAlliances(i, me)))
		{
			quantity += asStructureStats[index].curCount[i];
		}
	}
	return quantity;
}

//-- ## countDroid([droid type[, player]])
//--
//-- Count the number of droids that a given player has. Droid type must be either
//-- DROID_ANY, DROID_COMMAND or DROID_CONSTRUCT.
//-- The player parameter can be a specific player, ALL_PLAYERS, ALLIES or ENEMIES.
//--
int wzapi::countDroid(WZAPI_PARAMS(optional<int> _type, optional<int> _player))
{
	int type = (_type.has_value()) ? _type.value() : DROID_ANY;
	SCRIPT_ASSERT(-1, context, type <= DROID_ANY, "Bad droid type parameter");
	int me = context.player();
	int player = (_player.has_value()) ? _player.value() : me;

	int quantity = 0;
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (player == i || player == ALL_PLAYERS
		    || (player == ALLIES && aiCheckAlliances(i, me))
		    || (player == ENEMIES && !aiCheckAlliances(i, me)))
		{
			if (type == DROID_ANY)
			{
				quantity += getNumDroids(i);
			}
			else if (type == DROID_CONSTRUCT)
			{
				quantity += getNumConstructorDroids(i);
			}
			else if (type == DROID_COMMAND)
			{
				quantity += getNumCommandDroids(i);
			}
		}
	}
	return quantity;
}

//-- ## loadLevel(level name)
//--
//-- Load the level with the given name.
//--
wzapi::no_return_value wzapi::loadLevel(WZAPI_PARAMS(std::string levelName))
{
	sstrcpy(aLevelName, levelName.c_str());

	// Find the level dataset
	LEVEL_DATASET *psNewLevel = levFindDataSet(levelName.c_str());
	SCRIPT_ASSERT({}, context, psNewLevel, "Could not find level data for %s", levelName.c_str());

	// Get the mission rolling...
	nextMissionType = psNewLevel->type;
	loopMissionState = LMS_CLEAROBJECTS;
	return {};
}

//-- ## setDroidExperience(droid, experience)
//--
//-- Set the amount of experience a droid has. Experience is read using floating point precision.
//--
wzapi::no_return_value wzapi::setDroidExperience(WZAPI_PARAMS(DROID *psDroid, double experience))
{
	SCRIPT_ASSERT({}, context, psDroid, "No valid droid provided");
	psDroid->experience = static_cast<uint32_t>(experience * 65536);
	return {};
}

//-- ## donateObject(object, to)
//--
//-- Donate a game object (restricted to droids before 3.2.3) to another player. Returns true if
//-- donation was successful. May return false if this donation would push the receiving player
//-- over unit limits. (3.2+ only)
//--
bool wzapi::donateObject(WZAPI_PARAMS(BASE_OBJECT *psObject, int toPlayer))
{
	SCRIPT_ASSERT(false, context, psObject, "No valid object provided");
	SCRIPT_ASSERT_PLAYER(false, context, toPlayer);

	uint32_t object_id = psObject->id;
	uint8_t from = psObject->player;
	uint8_t to = static_cast<uint8_t>(toPlayer);
	uint8_t giftType = 0;
	if (psObject->type == OBJ_DROID)
	{
		// Check unit limits.
		DROID *psDroid = (DROID *)psObject;
		if ((psDroid->droidType == DROID_COMMAND && getNumCommandDroids(to) + 1 > getMaxCommanders(to))
		    || (psDroid->droidType == DROID_CONSTRUCT && getNumConstructorDroids(to) + 1 > getMaxConstructors(to))
		    || getNumDroids(to) + 1 > getMaxDroids(to))
		{
			return false;
		}
		giftType = DROID_GIFT;
	}
	else if (psObject->type == OBJ_STRUCTURE)
	{
		STRUCTURE *psStruct = (STRUCTURE *)psObject;
		const int statidx = psStruct->pStructureType - asStructureStats;
		if (asStructureStats[statidx].curCount[to] + 1 > asStructureStats[statidx].upgrade[to].limit)
		{
			return false;
		}
		giftType = STRUCTURE_GIFT;
	}
	else
	{
		return false;
	}
	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_GIFT);
	NETuint8_t(&giftType);
	NETuint8_t(&from);
	NETuint8_t(&to);
	NETuint32_t(&object_id);
	NETend();
	return true;
}

//-- ## donatePower(amount, to)
//--
//-- Donate power to another player. Returns true. (3.2+ only)
//--
bool wzapi::donatePower(WZAPI_PARAMS(int amount, int toPlayer))
{
	int from = context.player();
	giftPower(from, toPlayer, amount, true);
	return true;
}

//-- ## setNoGoArea(x1, y1, x2, y2, player)
//--
//-- Creates an area on the map on which nothing can be built. If player is zero,
//-- then landing lights are placed. If player is -1, then a limbo landing zone
//-- is created and limbo droids placed.
//--
// FIXME: missing a way to call initNoGoAreas(); check if we can call this in
// every level start instead of through scripts
wzapi::no_return_value wzapi::setNoGoArea(WZAPI_PARAMS(int x1, int y1, int x2, int y2, int player))
{
	SCRIPT_ASSERT({}, context, x1 >= 0, "Minimum scroll x value %d is less than zero - ", x1);
	SCRIPT_ASSERT({}, context, y1 >= 0, "Minimum scroll y value %d is less than zero - ", y1);
	SCRIPT_ASSERT({}, context, x2 <= mapWidth, "Maximum scroll x value %d is greater than mapWidth %d", x2, (int)mapWidth);
	SCRIPT_ASSERT({}, context, y2 <= mapHeight, "Maximum scroll y value %d is greater than mapHeight %d", y2, (int)mapHeight);
	SCRIPT_ASSERT({}, context, player < MAX_PLAYERS && player >= -1, "Bad player value %d", player);

	if (player == -1)
	{
		::setNoGoArea(x1, y1, x2, y2, LIMBO_LANDING);
		placeLimboDroids();	// this calls the Droids from the Limbo list onto the map
	}
	else
	{
		::setNoGoArea(x1, y1, x2, y2, player);
	}
	return {};
}

//-- ## startTransporterEntry(x, y, player)
//--
//-- Set the entry position for the mission transporter, and make it start flying in
//-- reinforcements. If you want the camera to follow it in, use cameraTrack() on it.
//-- The transport needs to be set up with the mission droids, and the first transport
//-- found will be used. (3.2+ only)
//--
wzapi::no_return_value wzapi::startTransporterEntry(WZAPI_PARAMS(int x, int y, int player))
{
	SCRIPT_ASSERT_PLAYER({}, context, player);
	missionSetTransporterEntry(player, x, y);
	missionFlyTransportersIn(player, false);
	return {};
}

//-- ## setTransporterExit(x, y, player)
//--
//-- Set the exit position for the mission transporter. (3.2+ only)
//--
wzapi::no_return_value wzapi::setTransporterExit(WZAPI_PARAMS(int x, int y, int player))
{
	SCRIPT_ASSERT_PLAYER({}, context, player);
	missionSetTransporterExit(player, x, y);
	return {};
}

//-- ## setObjectFlag(object, flag, value)
//--
//-- Set or unset an object flag on a given game object. Does not take care of network sync, so for multiplayer games,
//-- needs wrapping in a syncRequest. (3.3+ only.)
//-- Recognized object flags: OBJECT_FLAG_UNSELECTABLE - makes object unavailable for selection from player UI.
//--
wzapi::no_return_value wzapi::setObjectFlag(WZAPI_PARAMS(BASE_OBJECT *psObj, int _flag, bool value)) MULTIPLAY_SYNCREQUEST_REQUIRED
{
	SCRIPT_ASSERT({}, context, psObj, "No valid object provided");
	SCRIPT_ASSERT({}, context, psObj->type == OBJ_DROID || psObj->type == OBJ_STRUCTURE || psObj->type == OBJ_FEATURE, "Bad object type");
	OBJECT_FLAG flag = (OBJECT_FLAG)_flag;
	SCRIPT_ASSERT({}, context, flag >= 0 && flag < OBJECT_FLAG_COUNT, "Bad flag value %d", flag);
	psObj->flags.set(flag, value);
	return {};
}

//-- ## fireWeaponAtLoc(weapon, x, y[, player])
//--
//-- Fires a weapon at the given coordinates (3.3+ only). The player is who owns the projectile.
//-- Please use fireWeaponAtObj() to damage objects as multiplayer and campaign
//-- may have different friendly fire logic for a few weapons (like the lassat).
//--
wzapi::no_return_value wzapi::fireWeaponAtLoc(WZAPI_PARAMS(std::string weaponName, int x, int y, optional<int> _player))
{
	int weapon = getCompFromName(COMP_WEAPON, WzString::fromUtf8(weaponName));
	SCRIPT_ASSERT({}, context, weapon > 0, "No such weapon: %s", weaponName.c_str());

	int xLocation = x;
	int yLocation = y;

	int player = (_player.has_value()) ? _player.value() : context.player();
	SCRIPT_ASSERT_PLAYER({}, context, player);

	Vector3i target;
	target.x = world_coord(xLocation);
	target.y = world_coord(yLocation);
	target.z = map_Height(xLocation, yLocation);

	WEAPON sWeapon;
	sWeapon.nStat = weapon;

	proj_SendProjectile(&sWeapon, nullptr, player, target, nullptr, true, 0);
	return {};
}

//-- ## fireWeaponAtObj(weapon, game object[, player])
//--
//-- Fires a weapon at a game object (3.3+ only). The player is who owns the projectile.
//--
wzapi::no_return_value wzapi::fireWeaponAtObj(WZAPI_PARAMS(std::string weaponName, BASE_OBJECT *psObj, optional<int> _player))
{
	int weapon = getCompFromName(COMP_WEAPON, WzString::fromUtf8(weaponName));
	SCRIPT_ASSERT({}, context, weapon > 0, "No such weapon: %s", weaponName.c_str());
	SCRIPT_ASSERT({}, context, psObj, "No valid object provided");

	int player = (_player.has_value()) ? _player.value() : context.player();
	SCRIPT_ASSERT_PLAYER({}, context, player);

	Vector3i target = psObj->pos;

	WEAPON sWeapon;
	sWeapon.nStat = weapon;

	proj_SendProjectile(&sWeapon, nullptr, player, target, psObj, true, 0);
	return {};
}

//-- ## transformPlayerToSpectator(player)
//--
//-- Transform a player to a spectator. (4.2+ only)
//-- This is a one-time transformation, destroys the player's HQ and all of their remaining units, and must occur deterministically on all clients.
//--
bool wzapi::transformPlayerToSpectator(WZAPI_PARAMS(int player))
{
	SCRIPT_ASSERT_PLAYER(false, context, player);
	return makePlayerSpectator(static_cast<uint32_t>(player), false, false);
}

// flag all droids as requiring update on next frame
static void dirtyAllDroids(int player)
{
	for (DROID *psDroid = apsDroidLists[player]; psDroid != nullptr; psDroid = psDroid->psNext)
	{
		psDroid->flags.set(OBJECT_FLAG_DIRTY);
	}
	for (DROID *psDroid = mission.apsDroidLists[player]; psDroid != nullptr; psDroid = psDroid->psNext)
	{
		psDroid->flags.set(OBJECT_FLAG_DIRTY);
	}
	for (DROID *psDroid = apsLimboDroids[player]; psDroid != nullptr; psDroid = psDroid->psNext)
	{
		psDroid->flags.set(OBJECT_FLAG_DIRTY);
	}
}

static void dirtyAllStructures(int player)
{
	for (STRUCTURE *psCurr = apsStructLists[player]; psCurr; psCurr = psCurr->psNext)
	{
		psCurr->flags.set(OBJECT_FLAG_DIRTY);
	}
	for (STRUCTURE *psCurr = mission.apsStructLists[player]; psCurr; psCurr = psCurr->psNext)
	{
		psCurr->flags.set(OBJECT_FLAG_DIRTY);
	}
}

enum Scrcb {
	SCRCB_FIRST = COMP_NUMCOMPONENTS,
	SCRCB_RES = SCRCB_FIRST,  // Research upgrade
	SCRCB_MODULE_RES,  // Research module upgrade
	SCRCB_REP,  // Repair upgrade
	SCRCB_POW,  // Power upgrade
	SCRCB_MODULE_POW,  // And so on...
	SCRCB_CON,
	SCRCB_MODULE_CON,
	SCRCB_REA,
	SCRCB_ARM,
	SCRCB_HEA,
	SCRCB_ELW,
	SCRCB_HIT,
	SCRCB_LIMIT,
	SCRCB_LAST = SCRCB_LIMIT
};

bool wzapi::setUpgradeStats(WZAPI_BASE_PARAMS(int player, const std::string& name, int type, unsigned index, const nlohmann::json& newValue))
{
	int value = json_variant(newValue).toInt();
	syncDebug("stats[p%d,t%d,%s,i%d] = %d", player, type, name.c_str(), index, value);
	if (type == COMP_BODY)
	{
		SCRIPT_ASSERT(false, context, index < numBodyStats, "Bad index");
		BODY_STATS *psStats = asBodyStats + index;
		if (name == "HitPoints")
		{
			psStats->upgrade[player].hitpoints = value;
			dirtyAllDroids(player);
		}
		else if (name == "HitPointPct")
		{
			psStats->upgrade[player].hitpointPct = value;
			dirtyAllDroids(player);
		}
		else if (name == "Armour")
		{
			psStats->upgrade[player].armour = value;
		}
		else if (name == "Thermal")
		{
			psStats->upgrade[player].thermal = value;
		}
		else if (name == "Power")
		{
			psStats->upgrade[player].power = value;
			dirtyAllDroids(player);
		}
		else if (name == "Resistance")
		{
			// TBD FIXME - not updating resistance points in droids...
			psStats->upgrade[player].resistance = value;
		}
		else
		{
			SCRIPT_ASSERT(false, context, false, "Upgrade component %s not found", name.c_str());
		}
	}
	else if (type == COMP_BRAIN)
	{
		SCRIPT_ASSERT(false, context, index < numBrainStats, "Bad index");
		BRAIN_STATS *psStats = asBrainStats + index;
		if (name == "BaseCommandLimit")
		{
			psStats->upgrade[player].maxDroids = value;
		}
		else if (name == "CommandLimitByLevel")
		{
			psStats->upgrade[player].maxDroidsMult = value;
		}
		else if (name == "RankThresholds")
		{
			SCRIPT_ASSERT(false, context, newValue.is_array(), "Level thresholds not an array!");
			size_t length = newValue.size();
			SCRIPT_ASSERT(false, context, length <= psStats->upgrade[player].rankThresholds.size(), "Invalid thresholds array length");
			for (size_t i = 0; i < length; i++)
			{
				// Use json_variant to support conversion from other value types to an int
				psStats->upgrade[player].rankThresholds[i] = json_variant(newValue[i]).toInt();
			}
		}
		else if (name == "HitPoints")
		{
			psStats->upgrade[player].hitpoints = value;
			dirtyAllDroids(player);
		}
		else if (name == "HitPointPct")
		{
			psStats->upgrade[player].hitpointPct = value;
			dirtyAllDroids(player);
		}
		else
		{
			SCRIPT_ASSERT(false, context, false, "Upgrade component %s not found", name.c_str());
		}
	}
	else if (type == COMP_SENSOR)
	{
		SCRIPT_ASSERT(false, context, index < numSensorStats, "Bad index");
		SENSOR_STATS *psStats = asSensorStats + index;
		if (name == "Range")
		{
			psStats->upgrade[player].range = value;
			dirtyAllDroids(player);
			dirtyAllStructures(player);
		}
		else if (name == "HitPoints")
		{
			psStats->upgrade[player].hitpoints = value;
			dirtyAllDroids(player);
		}
		else if (name == "HitPointPct")
		{
			psStats->upgrade[player].hitpointPct = value;
			dirtyAllDroids(player);
		}
		else
		{
			SCRIPT_ASSERT(false, context, false, "Upgrade component %s not found", name.c_str());
		}
	}
	else if (type == COMP_ECM)
	{
		SCRIPT_ASSERT(false, context, index < numECMStats, "Bad index");
		ECM_STATS *psStats = asECMStats + index;
		if (name == "Range")
		{
			psStats->upgrade[player].range = value;
			dirtyAllDroids(player);
			dirtyAllStructures(player);
		}
		else if (name == "HitPoints")
		{
			psStats->upgrade[player].hitpoints = value;
			dirtyAllDroids(player);
		}
		else if (name == "HitPointPct")
		{
			psStats->upgrade[player].hitpointPct = value;
			dirtyAllDroids(player);
		}
		else
		{
			SCRIPT_ASSERT(false, context, false, "Upgrade component %s not found", name.c_str());
		}
	}
	else if (type == COMP_PROPULSION)
	{
		SCRIPT_ASSERT(false, context, index < numPropulsionStats, "Bad index");
		PROPULSION_STATS *psStats = asPropulsionStats + index;
		if (name == "HitPoints")
		{
			psStats->upgrade[player].hitpoints = value;
			dirtyAllDroids(player);
		}
		else if (name == "HitPointPct")
		{
			psStats->upgrade[player].hitpointPct = value;
			dirtyAllDroids(player);
		}
		else if (name == "HitPointPctOfBody")
		{
			psStats->upgrade[player].hitpointPctOfBody = value;
			dirtyAllDroids(player);
		}
		else
		{
			SCRIPT_ASSERT(false, context, false, "Upgrade component %s not found", name.c_str());
		}
	}
	else if (type == COMP_CONSTRUCT)
	{
		SCRIPT_ASSERT(false, context, index < numConstructStats, "Bad index");
		CONSTRUCT_STATS *psStats = asConstructStats + index;
		if (name == "ConstructorPoints")
		{
			psStats->upgrade[player].constructPoints = value;
		}
		else if (name == "HitPoints")
		{
			psStats->upgrade[player].hitpoints = value;
			dirtyAllDroids(player);
		}
		else if (name == "HitPointPct")
		{
			psStats->upgrade[player].hitpointPct = value;
			dirtyAllDroids(player);
		}
		else
		{
			SCRIPT_ASSERT(false, context, false, "Upgrade component %s not found", name.c_str());
		}
	}
	else if (type == COMP_REPAIRUNIT)
	{
		SCRIPT_ASSERT(false, context, index < numRepairStats, "Bad index");
		REPAIR_STATS *psStats = asRepairStats + index;
		if (name == "RepairPoints")
		{
			psStats->upgrade[player].repairPoints = value;
		}
		else if (name == "HitPoints")
		{
			psStats->upgrade[player].hitpoints = value;
			dirtyAllDroids(player);
		}
		else if (name == "HitPointPct")
		{
			psStats->upgrade[player].hitpointPct = value;
			dirtyAllDroids(player);
		}
		else
		{
			SCRIPT_ASSERT(false, context, false, "Upgrade component %s not found", name.c_str());
		}
	}
	else if (type == COMP_WEAPON)
	{
		SCRIPT_ASSERT(false, context, index < numWeaponStats, "Bad index");
		WEAPON_STATS *psStats = asWeaponStats + index;
		if (name == "MaxRange")
		{
			psStats->upgrade[player].maxRange = value;
		}
		else if (name == "ShortRange")
		{
			psStats->upgrade[player].shortRange = value;
		}
		else if (name == "MinRange")
		{
			psStats->upgrade[player].minRange = value;
		}
		else if (name == "HitChance")
		{
			psStats->upgrade[player].hitChance = value;
		}
		else if (name == "ShortHitChance")
		{
			psStats->upgrade[player].shortHitChance = value;
		}
		else if (name == "FirePause")
		{
			psStats->upgrade[player].firePause = value;
		}
		else if (name == "Rounds")
		{
			psStats->upgrade[player].numRounds = value;
		}
		else if (name == "ReloadTime")
		{
			psStats->upgrade[player].reloadTime = value;
		}
		else if (name == "Damage")
		{
			psStats->upgrade[player].damage = value;
		}
		else if (name == "MinimumDamage")
		{
			psStats->upgrade[player].minimumDamage = value;
		}
		else if (name == "Radius")
		{
			psStats->upgrade[player].radius = value;
		}
		else if (name == "RadiusDamage")
		{
			psStats->upgrade[player].radiusDamage = value;
		}
		else if (name == "RepeatDamage")
		{
			psStats->upgrade[player].periodicalDamage = value;
		}
		else if (name == "RepeatTime")
		{
			psStats->upgrade[player].periodicalDamageTime = value;
		}
		else if (name == "RepeatRadius")
		{
			psStats->upgrade[player].periodicalDamageRadius = value;
		}
		else if (name == "HitPoints")
		{
			psStats->upgrade[player].hitpoints = value;
			dirtyAllDroids(player);
		}
		else if (name == "HitPointPct")
		{
			psStats->upgrade[player].hitpointPct = value;
			dirtyAllDroids(player);
		}
		else
		{
			SCRIPT_ASSERT(false, context, false, "Invalid weapon method");
		}
	}
	else if (type >= SCRCB_FIRST && type <= SCRCB_LAST)
	{
		SCRIPT_ASSERT(false, context, index < numStructureStats, "Bad index");
		STRUCTURE_STATS *psStats = asStructureStats + index;
		switch ((Scrcb)type)
		{
		case SCRCB_RES: psStats->upgrade[player].research = value; break;
		case SCRCB_MODULE_RES: psStats->upgrade[player].moduleResearch = value; break;
		case SCRCB_REP: psStats->upgrade[player].repair = value; break;
		case SCRCB_POW: psStats->upgrade[player].power = value; break;
		case SCRCB_MODULE_POW: psStats->upgrade[player].modulePower = value; break;
		case SCRCB_CON: psStats->upgrade[player].production = value; break;
		case SCRCB_MODULE_CON: psStats->upgrade[player].moduleProduction = value; break;
		case SCRCB_REA: psStats->upgrade[player].rearm = value; break;
		case SCRCB_HEA: psStats->upgrade[player].thermal = value; break;
		case SCRCB_ARM: psStats->upgrade[player].armour = value; break;
		case SCRCB_ELW:
			// Update resistance points for all structures, to avoid making them damaged
			// FIXME - this is _really_ slow! we could be doing this for dozens of buildings one at a time!
			for (STRUCTURE *psCurr = apsStructLists[player]; psCurr; psCurr = psCurr->psNext)
			{
				if (psStats == psCurr->pStructureType && psStats->upgrade[player].resistance < value)
				{
					psCurr->resistance = value;
				}
			}
			for (STRUCTURE *psCurr = mission.apsStructLists[player]; psCurr; psCurr = psCurr->psNext)
			{
				if (psStats == psCurr->pStructureType && psStats->upgrade[player].resistance < value)
				{
					psCurr->resistance = value;
				}
			}
			psStats->upgrade[player].resistance = value;
			break;
		case SCRCB_HIT:
			// Update body points for all structures, to avoid making them damaged
			// FIXME - this is _really_ slow! we could be doing this for
			// dozens of buildings one at a time!
			for (STRUCTURE *psCurr = apsStructLists[player]; psCurr; psCurr = psCurr->psNext)
			{
				if (psStats == psCurr->pStructureType && psStats->upgrade[player].hitpoints < value)
				{
					psCurr->body = (psCurr->body * value) / psStats->upgrade[player].hitpoints;
				}
			}
			for (STRUCTURE *psCurr = mission.apsStructLists[player]; psCurr; psCurr = psCurr->psNext)
			{
				if (psStats == psCurr->pStructureType && psStats->upgrade[player].hitpoints < value)
				{
					psCurr->body = (psCurr->body * value) / psStats->upgrade[player].hitpoints;
				}
			}
			psStats->upgrade[player].hitpoints = value;
			break;
		case SCRCB_LIMIT:
			psStats->upgrade[player].limit = value; break;
		}
	}
	else
	{
		SCRIPT_ASSERT(false, context, false, "Component type not found for upgrade");
	}

	return true;
}

nlohmann::json wzapi::getUpgradeStats(WZAPI_BASE_PARAMS(int player, const std::string& name, int type, unsigned index))
{
	if (type == COMP_BODY)
	{
		SCRIPT_ASSERT(nlohmann::json(), context, index < numBodyStats, "Bad index");
		const BODY_STATS *psStats = asBodyStats + index;
		if (name == "HitPoints")
		{
			return psStats->upgrade[player].hitpoints;
		}
		else if (name == "HitPointPct")
		{
			return psStats->upgrade[player].hitpointPct;
		}
		else if (name == "Armour")
		{
			return psStats->upgrade[player].armour;
		}
		else if (name == "Thermal")
		{
			return psStats->upgrade[player].thermal;
		}
		else if (name == "Power")
		{
			return psStats->upgrade[player].power;
		}
		else if (name == "Resistance")
		{
			return psStats->upgrade[player].resistance;
		}
		else
		{
			SCRIPT_ASSERT(nlohmann::json(), context, false, "Upgrade component %s not found", name.c_str());
		}
	}
	else if (type == COMP_BRAIN)
	{
		SCRIPT_ASSERT(nlohmann::json(), context, index < numBrainStats, "Bad index");
		const BRAIN_STATS *psStats = asBrainStats + index;
		if (name == "BaseCommandLimit")
		{
			return psStats->upgrade[player].maxDroids;
		}
		else if (name == "CommandLimitByLevel")
		{
			return psStats->upgrade[player].maxDroidsMult;
		}
		else if (name == "RankThresholds")
		{
			size_t length = psStats->upgrade[player].rankThresholds.size();
			nlohmann::json value = nlohmann::json::array();
			for (size_t i = 0; i < length; i++)
			{
				value.push_back(psStats->upgrade[player].rankThresholds[i]);
			}
			return value;
		}
		else if (name == "HitPoints")
		{
			return psStats->upgrade[player].hitpoints;
		}
		else if (name == "HitPointPct")
		{
			return psStats->upgrade[player].hitpointPct;
		}
		else
		{
			SCRIPT_ASSERT(nlohmann::json(), context, false, "Upgrade component %s not found", name.c_str());
		}
	}
	else if (type == COMP_SENSOR)
	{
		SCRIPT_ASSERT(nlohmann::json(), context, index < numSensorStats, "Bad index");
		const SENSOR_STATS *psStats = asSensorStats + index;
		if (name == "Range")
		{
			return psStats->upgrade[player].range;
		}
		else if (name == "HitPoints")
		{
			return psStats->upgrade[player].hitpoints;
		}
		else if (name == "HitPointPct")
		{
			return psStats->upgrade[player].hitpointPct;
		}
		else
		{
			SCRIPT_ASSERT(nlohmann::json(), context, false, "Upgrade component %s not found", name.c_str());
		}
	}
	else if (type == COMP_ECM)
	{
		SCRIPT_ASSERT(nlohmann::json(), context, index < numECMStats, "Bad index");
		const ECM_STATS *psStats = asECMStats + index;
		if (name == "Range")
		{
			return psStats->upgrade[player].range;
		}
		else if (name == "HitPoints")
		{
			return psStats->upgrade[player].hitpoints;
		}
		else if (name == "HitPointPct")
		{
			return psStats->upgrade[player].hitpointPct;
		}
		else
		{
			SCRIPT_ASSERT(nlohmann::json(), context, false, "Upgrade component %s not found", name.c_str());
		}
	}
	else if (type == COMP_PROPULSION)
	{
		SCRIPT_ASSERT(nlohmann::json(), context, index < numPropulsionStats, "Bad index");
		const PROPULSION_STATS *psStats = asPropulsionStats + index;
		if (name == "HitPoints")
		{
			return psStats->upgrade[player].hitpoints;
		}
		else if (name == "HitPointPct")
		{
			return psStats->upgrade[player].hitpointPct;
		}
		else if (name == "HitPointPctOfBody")
		{
			return psStats->upgrade[player].hitpointPctOfBody;
		}
		else
		{
			SCRIPT_ASSERT(nlohmann::json(), context, false, "Upgrade component %s not found", name.c_str());
		}
	}
	else if (type == COMP_CONSTRUCT)
	{
		SCRIPT_ASSERT(nlohmann::json(), context, index < numConstructStats, "Bad index");
		const CONSTRUCT_STATS *psStats = asConstructStats + index;
		if (name == "ConstructorPoints")
		{
			return psStats->upgrade[player].constructPoints;
		}
		else if (name == "HitPoints")
		{
			return psStats->upgrade[player].hitpoints;
		}
		else if (name == "HitPointPct")
		{
			return psStats->upgrade[player].hitpointPct;
		}
		else
		{
			SCRIPT_ASSERT(nlohmann::json(), context, false, "Upgrade component %s not found", name.c_str());
		}
	}
	else if (type == COMP_REPAIRUNIT)
	{
		SCRIPT_ASSERT(nlohmann::json(), context, index < numRepairStats, "Bad index");
		const REPAIR_STATS *psStats = asRepairStats + index;
		if (name == "RepairPoints")
		{
			return psStats->upgrade[player].repairPoints;
		}
		else if (name == "HitPoints")
		{
			return psStats->upgrade[player].hitpoints;
		}
		else if (name == "HitPointPct")
		{
			return psStats->upgrade[player].hitpointPct;
		}
		else
		{
			SCRIPT_ASSERT(nlohmann::json(), context, false, "Upgrade component %s not found", name.c_str());
		}
	}
	else if (type == COMP_WEAPON)
	{
		SCRIPT_ASSERT(nlohmann::json(), context, index < numWeaponStats, "Bad index");
		const WEAPON_STATS *psStats = asWeaponStats + index;
		if (name == "MaxRange")
		{
			return psStats->upgrade[player].maxRange;
		}
		else if (name == "ShortRange")
		{
			return psStats->upgrade[player].shortRange;
		}
		else if (name == "MinRange")
		{
			return psStats->upgrade[player].minRange;
		}
		else if (name == "HitChance")
		{
			return psStats->upgrade[player].hitChance;
		}
		else if (name == "ShortHitChance")
		{
			return psStats->upgrade[player].shortHitChance;
		}
		else if (name == "FirePause")
		{
			return psStats->upgrade[player].firePause;
		}
		else if (name == "Rounds")
		{
			return psStats->upgrade[player].numRounds;
		}
		else if (name == "ReloadTime")
		{
			return psStats->upgrade[player].reloadTime;
		}
		else if (name == "Damage")
		{
			return psStats->upgrade[player].damage;
		}
		else if (name == "MinimumDamage")
		{
			return psStats->upgrade[player].minimumDamage;
		}
		else if (name == "Radius")
		{
			return psStats->upgrade[player].radius;
		}
		else if (name == "RadiusDamage")
		{
			return psStats->upgrade[player].radiusDamage;
		}
		else if (name == "RepeatDamage")
		{
			return psStats->upgrade[player].periodicalDamage;
		}
		else if (name == "RepeatTime")
		{
			return psStats->upgrade[player].periodicalDamageTime;
		}
		else if (name == "RepeatRadius")
		{
			return psStats->upgrade[player].periodicalDamageRadius;
		}
		else if (name == "HitPoints")
		{
			return psStats->upgrade[player].hitpoints;
		}
		else if (name == "HitPointPct")
		{
			return psStats->upgrade[player].hitpointPct;
		}
		else
		{
			SCRIPT_ASSERT(nlohmann::json(), context, false, "Invalid weapon method");
		}
	}
	else if (type >= SCRCB_FIRST && type <= SCRCB_LAST)
	{
		SCRIPT_ASSERT(nlohmann::json(), context, index < numStructureStats, "Bad index");
		const STRUCTURE_STATS *psStats = asStructureStats + index;
		switch (type)
		{
		case SCRCB_RES: return psStats->upgrade[player].research; break;
		case SCRCB_MODULE_RES: return psStats->upgrade[player].moduleResearch; break;
		case SCRCB_REP: return psStats->upgrade[player].repair; break;
		case SCRCB_POW: return psStats->upgrade[player].power; break;
		case SCRCB_MODULE_POW: return psStats->upgrade[player].modulePower; break;
		case SCRCB_CON: return psStats->upgrade[player].production; break;
		case SCRCB_MODULE_CON: return psStats->upgrade[player].moduleProduction; break;
		case SCRCB_REA: return psStats->upgrade[player].rearm; break;
		case SCRCB_ELW: return psStats->upgrade[player].resistance; break;
		case SCRCB_HEA: return psStats->upgrade[player].thermal; break;
		case SCRCB_ARM: return psStats->upgrade[player].armour; break;
		case SCRCB_HIT: return psStats->upgrade[player].hitpoints;
		case SCRCB_LIMIT: return psStats->upgrade[player].limit;
		default: SCRIPT_ASSERT(nlohmann::json(), context, false, "Component type not found for upgrade"); break;
		}
	}
	return nlohmann::json();
}


wzapi::GameEntityRules::value_type wzapi::GameEntityRules::getPropertyValue(const wzapi::execution_context_base& context, const std::string& name) const
{
	auto it = propertyNameToTypeMap.find(name);
	if (it == propertyNameToTypeMap.end())
	{
		// Failed to find `name`
		return GameEntityRules::value_type();
	}
	int type = it->second;
	return wzapi::getUpgradeStats(context, getPlayer(), name, type, getIndex());
}

wzapi::GameEntityRules::value_type wzapi::GameEntityRules::setPropertyValue(const wzapi::execution_context_base& context, const std::string& name, const value_type& newValue)
{
	auto it = propertyNameToTypeMap.find(name);
	if (it == propertyNameToTypeMap.end())
	{
		// Failed to find `name`
		return GameEntityRules::value_type();
	}
	int type = it->second;
	return wzapi::setUpgradeStats(context, getPlayer(), name, type, getIndex(), newValue);
}

std::vector<wzapi::PerPlayerUpgrades> wzapi::getUpgradesObject()
{
	//== * ```Upgrades``` A special array containing per-player rules information for game entity types,
	//== which can be written to in order to implement upgrades and other dynamic rules changes. Each item in the
	//== array contains a subset of the sparse array of rules information in the ```Stats``` global.
	//== These values are defined:
	std::vector<PerPlayerUpgrades> upgrades;
	upgrades.reserve(MAX_PLAYERS);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		upgrades.push_back(PerPlayerUpgrades(i));
		PerPlayerUpgrades& node = upgrades.back();

		//==   * ```Body``` Droid bodies
		GameEntityRuleContainer bodybase;
		for (unsigned j = 0; j < numBodyStats; j++)
		{
			BODY_STATS *psStats = asBodyStats + j;
			GameEntityRules body(i, j, {
				{"HitPoints", COMP_BODY},
				{"HitPointPct", COMP_BODY},
				{"Power", COMP_BODY},
				{"Armour", COMP_BODY},
				{"Thermal", COMP_BODY},
				{"Resistance", COMP_BODY},
			});
			bodybase.addRules(psStats->name.toUtf8(), std::move(body));
		}
		node.addGameEntity("Body", std::move(bodybase));

		//==   * ```Sensor``` Sensor turrets
		GameEntityRuleContainer sensorbase;
		for (unsigned j = 0; j < numSensorStats; j++)
		{
			SENSOR_STATS *psStats = asSensorStats + j;
			GameEntityRules sensor(i, j, {
				{"HitPoints", COMP_SENSOR},
				{"HitPointPct", COMP_SENSOR},
				{"Range", COMP_SENSOR},
			});
			sensorbase.addRules(psStats->name.toUtf8(), std::move(sensor));
		}
		node.addGameEntity("Sensor", std::move(sensorbase));

		//==   * ```Propulsion``` Propulsions
		GameEntityRuleContainer propbase;
		for (unsigned j = 0; j < numPropulsionStats; j++)
		{
			PROPULSION_STATS *psStats = asPropulsionStats + j;
			GameEntityRules v(i, j, {
				{"HitPoints", COMP_PROPULSION},
				{"HitPointPct", COMP_PROPULSION},
				{"HitPointPctOfBody", COMP_PROPULSION},
			});
			propbase.addRules(psStats->name.toUtf8(), std::move(v));
		}
		node.addGameEntity("Propulsion", std::move(propbase));

		//==   * ```ECM``` ECM (Electronic Counter-Measure) turrets
		GameEntityRuleContainer ecmbase;
		for (unsigned j = 0; j < numECMStats; j++)
		{
			ECM_STATS *psStats = asECMStats + j;
			GameEntityRules ecm(i, j, {
				{"Range", COMP_ECM},
				{"HitPoints", COMP_ECM},
				{"HitPointPct", COMP_ECM},
			});
			ecmbase.addRules(psStats->name.toUtf8(), std::move(ecm));
		}
		node.addGameEntity("ECM", std::move(ecmbase));

		//==   * ```Repair``` Repair turrets (not used, incidentally, for repair centers)
		GameEntityRuleContainer repairbase;
		for (unsigned j = 0; j < numRepairStats; j++)
		{
			REPAIR_STATS *psStats = asRepairStats + j;
			GameEntityRules repair(i, j, {
				{"RepairPoints", COMP_REPAIRUNIT},
				{"HitPoints", COMP_REPAIRUNIT},
				{"HitPointPct", COMP_REPAIRUNIT},
			});
			repairbase.addRules(psStats->name.toUtf8(), std::move(repair));
		}
		node.addGameEntity("Repair", std::move(repairbase));

		//==   * ```Construct``` Constructor turrets (eg for trucks)
		GameEntityRuleContainer conbase;
		for (unsigned j = 0; j < numConstructStats; j++)
		{
			CONSTRUCT_STATS *psStats = asConstructStats + j;
			GameEntityRules con(i, j, {
				{"ConstructorPoints", COMP_CONSTRUCT},
				{"HitPoints", COMP_CONSTRUCT},
				{"HitPointPct", COMP_CONSTRUCT},
			});
			conbase.addRules(psStats->name.toUtf8(), std::move(con));
		}
		node.addGameEntity("Construct", std::move(conbase));

		//==   * ```Brain``` Brains
		//== BaseCommandLimit: How many droids a commander can command. CommandLimitByLevel: How many extra droids
		//== a commander can command for each of its rank levels. RankThresholds: An array describing how many
		//== kills are required for this brain to level up to the next rank. To alter this from scripts, you must
		//== set the entire array at once. Setting each item in the array will not work at the moment.
		GameEntityRuleContainer brainbase;
		for (unsigned j = 0; j < numBrainStats; j++)
		{
			BRAIN_STATS *psStats = asBrainStats + j;
			GameEntityRules br(i, j, {
				{"BaseCommandLimit", COMP_BRAIN},
				{"CommandLimitByLevel", COMP_BRAIN},
				{"RankThresholds", COMP_BRAIN},
				{"HitPoints", COMP_BRAIN},
				{"HitPointPct", COMP_BRAIN},
			});
			brainbase.addRules(psStats->name.toUtf8(), std::move(br));
		}
		node.addGameEntity("Brain", std::move(brainbase));

		//==   * ```Weapon``` Weapon turrets
		GameEntityRuleContainer wbase;
		for (unsigned j = 0; j < numWeaponStats; j++)
		{
			WEAPON_STATS *psStats = asWeaponStats + j;
			GameEntityRules weap(i, j, {
				{"MaxRange", COMP_WEAPON},
				{"ShortRange", COMP_WEAPON},
				{"MinRange", COMP_WEAPON},
				{"HitChance", COMP_WEAPON},
				{"ShortHitChance", COMP_WEAPON},
				{"FirePause", COMP_WEAPON},
				{"ReloadTime", COMP_WEAPON},
				{"Rounds", COMP_WEAPON},
				{"Radius", COMP_WEAPON},
				{"Damage", COMP_WEAPON},
				{"MinimumDamage", COMP_WEAPON},
				{"RadiusDamage", COMP_WEAPON},
				{"RepeatDamage", COMP_WEAPON},
				{"RepeatTime", COMP_WEAPON},
				{"RepeatRadius", COMP_WEAPON},
				{"HitPoints", COMP_WEAPON},
				{"HitPointPct", COMP_WEAPON},
			});
			wbase.addRules(psStats->name.toUtf8(), std::move(weap));
		}
		node.addGameEntity("Weapon", std::move(wbase));

		//==   * ```Building``` Buildings
		GameEntityRuleContainer structbase;
		for (unsigned j = 0; j < numStructureStats; j++)
		{
			STRUCTURE_STATS *psStats = asStructureStats + j;
			GameEntityRules strct(i, j, {
				{"ResearchPoints", SCRCB_RES},
				{"ModuleResearchPoints", SCRCB_MODULE_RES},
				{"RepairPoints", SCRCB_REP},
				{"PowerPoints", SCRCB_POW},
				{"ModulePowerPoints", SCRCB_MODULE_POW},
				{"ProductionPoints", SCRCB_CON},
				{"ModuleProductionPoints", SCRCB_MODULE_CON},
				{"RearmPoints", SCRCB_REA},
				{"Armour", SCRCB_ARM},
				{"Resistance", SCRCB_ELW},
				{"Thermal", SCRCB_HEA},
				{"HitPoints", SCRCB_HIT},
				{"Limit", SCRCB_LIMIT},
			});
			structbase.addRules(psStats->name.toUtf8(), std::move(strct));
		}
		node.addGameEntity("Building", std::move(structbase));
	}
	return upgrades;
}

nlohmann::json register_common(COMPONENT_STATS *psStats)
{
	nlohmann::json v = nlohmann::json::object();
	v["Id"] = psStats->id;
	v["Weight"] = psStats->weight;
	v["BuildPower"] = psStats->buildPower;
	v["BuildTime"] = psStats->buildPoints;
	v["HitPoints"] = psStats->getBase().hitpoints;
	v["HitPointPct"] = psStats->getBase().hitpointPct;
	return v;
}

nlohmann::json wzapi::constructStatsObject()
{
	/// Register 'Stats' object. It is a read-only representation of basic game component states.
	//== * ```Stats``` A sparse, read-only array containing rules information for game entity types.
	//== (For now only the highest level member attributes are documented here. Use the 'jsdebug' cheat
	//== to see them all.)
	//== These values are defined:
	nlohmann::json stats = nlohmann::json::object();
	{
		//==   * ```Body``` Droid bodies
		nlohmann::json bodybase = nlohmann::json::object();
		for (int j = 0; j < numBodyStats; j++)
		{
			BODY_STATS *psStats = asBodyStats + j;
			nlohmann::json body = register_common(psStats);
			body["Power"] = psStats->base.power;
			body["Armour"] = psStats->base.armour;
			body["Thermal"] = psStats->base.thermal;
			body["Resistance"] = psStats->base.resistance;
			body["Size"] = psStats->size;
			body["WeaponSlots"] = psStats->weaponSlots;
			body["BodyClass"] = psStats->bodyClass;
			bodybase[psStats->name.toUtf8()] = std::move(body);
		}
		stats["Body"] = std::move(bodybase);

		//==   * ```Sensor``` Sensor turrets
		nlohmann::json sensorbase = nlohmann::json::object();
		for (int j = 0; j < numSensorStats; j++)
		{
			SENSOR_STATS *psStats = asSensorStats + j;
			nlohmann::json sensor = register_common(psStats);
			sensor["Range"] = psStats->base.range;
			sensorbase[psStats->name.toUtf8()] = std::move(sensor);
		}
		stats["Sensor"] = std::move(sensorbase);

		//==   * ```ECM``` ECM (Electronic Counter-Measure) turrets
		nlohmann::json ecmbase = nlohmann::json::object();
		for (int j = 0; j < numECMStats; j++)
		{
			ECM_STATS *psStats = asECMStats + j;
			nlohmann::json ecm = register_common(psStats);
			ecm["Range"] = psStats->base.range;
			ecmbase[psStats->name.toUtf8()] = std::move(ecm);
		}
		stats["ECM"] = std::move(ecmbase);

		//==   * ```Propulsion``` Propulsions
		nlohmann::json propbase = nlohmann::json::object();
		for (int j = 0; j < numPropulsionStats; j++)
		{
			PROPULSION_STATS *psStats = asPropulsionStats + j;
			nlohmann::json v = register_common(psStats);
			v["HitpointPctOfBody"] = psStats->base.hitpointPctOfBody;
			v["MaxSpeed"] = psStats->maxSpeed;
			v["TurnSpeed"] = psStats->turnSpeed;
			v["SpinSpeed"] = psStats->spinSpeed;
			v["SpinAngle"] = psStats->spinAngle;
			v["SkidDeceleration"] = psStats->skidDeceleration;
			v["Acceleration"] = psStats->acceleration;
			v["Deceleration"] = psStats->deceleration;
			propbase[psStats->name.toUtf8()] = std::move(v);
		}
		stats["Propulsion"] = std::move(propbase);

		//==   * ```Repair``` Repair turrets (not used, incidentally, for repair centers)
		nlohmann::json repairbase = nlohmann::json::object();
		for (int j = 0; j < numRepairStats; j++)
		{
			REPAIR_STATS *psStats = asRepairStats + j;
			nlohmann::json repair = register_common(psStats);
			repair["RepairPoints"] = psStats->base.repairPoints;
			repairbase[psStats->name.toUtf8()] = std::move(repair);
		}
		stats["Repair"] = std::move(repairbase);

		//==   * ```Construct``` Constructor turrets (eg for trucks)
		nlohmann::json conbase = nlohmann::json::object();
		for (int j = 0; j < numConstructStats; j++)
		{
			CONSTRUCT_STATS *psStats = asConstructStats + j;
			nlohmann::json con = register_common(psStats);
			con["ConstructorPoints"] = psStats->base.constructPoints;
			conbase[psStats->name.toUtf8()] = std::move(con);
		}
		stats["Construct"] = std::move(conbase);

		//==   * ```Brain``` Brains
		nlohmann::json brainbase = nlohmann::json::object();
		for (int j = 0; j < numBrainStats; j++)
		{
			BRAIN_STATS *psStats = asBrainStats + j;
			nlohmann::json br = register_common(psStats);
			br["BaseCommandLimit"] = psStats->base.maxDroids;
			br["CommandLimitByLevel"] = psStats->base.maxDroidsMult;
			nlohmann::json thresholds = nlohmann::json::array(); //engine->newArray(psStats->base.rankThresholds.size());
			for (int x = 0; x < psStats->base.rankThresholds.size(); x++)
			{
				thresholds.push_back(psStats->base.rankThresholds.at(x));
			}
			br["RankThresholds"] = thresholds;
			nlohmann::json ranks = nlohmann::json::array(); //engine->newArray(psStats->rankNames.size());
			for (int x = 0; x < psStats->rankNames.size(); x++)
			{
				ranks.push_back(psStats->rankNames.at(x));
			}
			br["RankNames"] = ranks;
			brainbase[psStats->name.toUtf8()] = std::move(br);
		}
		stats["Brain"] = std::move(brainbase);

		//==   * ```Weapon``` Weapon turrets
		nlohmann::json wbase = nlohmann::json::object();
		for (int j = 0; j < numWeaponStats; j++)
		{
			WEAPON_STATS *psStats = asWeaponStats + j;
			nlohmann::json weap = register_common(psStats);
			weap["MaxRange"] = psStats->base.maxRange;
			weap["ShortRange"] = psStats->base.shortRange;
			weap["MinRange"] = psStats->base.minRange;
			weap["HitChance"] = psStats->base.hitChance;
			weap["ShortHitChance"] = psStats->base.shortHitChance;
			weap["FirePause"] = psStats->base.firePause;
			weap["ReloadTime"] = psStats->base.reloadTime;
			weap["Rounds"] = psStats->base.numRounds;
			weap["Damage"] = psStats->base.damage;
			weap["MinimumDamage"] = psStats->base.minimumDamage;
			weap["RadiusDamage"] = psStats->base.radiusDamage;
			weap["RepeatDamage"] = psStats->base.periodicalDamage;
			weap["RepeatRadius"] = psStats->base.periodicalDamageRadius;
			weap["RepeatTime"] = psStats->base.periodicalDamageTime;
			weap["Radius"] = psStats->base.radius;
			weap["ImpactType"] = psStats->weaponClass == WC_KINETIC ? "KINETIC" : "HEAT";
			weap["RepeatType"] = psStats->periodicalDamageWeaponClass == WC_KINETIC ? "KINETIC" : "HEAT";
			weap["ImpactClass"] = getWeaponSubClass(psStats->weaponSubClass);
			weap["RepeatClass"] = getWeaponSubClass(psStats->periodicalDamageWeaponSubClass);
			weap["FireOnMove"] = psStats->fireOnMove;
			weap["Effect"] = getWeaponEffect(psStats->weaponEffect);
			weap["ShootInAir"] = static_cast<bool>((psStats->surfaceToAir & SHOOT_IN_AIR) != 0);
			weap["ShootOnGround"] = static_cast<bool>((psStats->surfaceToAir & SHOOT_ON_GROUND) != 0);
			weap["NoFriendlyFire"] = psStats->flags.test(WEAPON_FLAG_NO_FRIENDLY_FIRE);
			weap["FlightSpeed"] = psStats->flightSpeed;
			weap["Rotate"] = psStats->rotate;
			weap["MinElevation"] = psStats->minElevation;
			weap["MaxElevation"] = psStats->maxElevation;
			weap["Recoil"] = psStats->recoilValue;
			weap["Penetrate"] = psStats->penetrate;
			wbase[psStats->name.toUtf8()] = std::move(weap);
		}
		stats["Weapon"] = std::move(wbase);

		//==   * ```WeaponClass``` Defined weapon classes
		nlohmann::json weaptypes = nlohmann::json::array(); //engine->newArray(WSC_NUM_WEAPON_SUBCLASSES);
		for (int j = 0; j < WSC_NUM_WEAPON_SUBCLASSES; j++)
		{
			weaptypes.push_back(getWeaponSubClass((WEAPON_SUBCLASS)j));
		}
		stats["WeaponClass"] = std::move(weaptypes);

		//==   * ```Building``` Buildings
		nlohmann::json structbase = nlohmann::json::object();
		for (int j = 0; j < numStructureStats; j++)
		{
			STRUCTURE_STATS *psStats = asStructureStats + j;
			nlohmann::json strct = nlohmann::json::object();
			strct["Id"] = psStats->id;
			if (psStats->type == REF_DEFENSE || psStats->type == REF_WALL || psStats->type == REF_WALLCORNER
			    || psStats->type == REF_GENERIC || psStats->type == REF_GATE)
			{
				strct["Type"] = "Wall";
			}
			else if (psStats->type != REF_DEMOLISH)
			{
				strct["Type"] = "Structure";
			}
			else
			{
				strct["Type"] = "Demolish";
			}
			strct["ResearchPoints"] = psStats->base.research;
			strct["RepairPoints"] = psStats->base.repair;
			strct["PowerPoints"] = psStats->base.power;
			strct["ProductionPoints"] = psStats->base.production;
			strct["RearmPoints"] = psStats->base.rearm;
			strct["Armour"] = psStats->base.armour;
			strct["Thermal"] = psStats->base.thermal;
			strct["HitPoints"] = psStats->base.hitpoints;
			strct["Resistance"] = psStats->base.resistance;
			structbase[psStats->name.toUtf8()] = std::move(strct);
		}
		stats["Building"] = std::move(structbase);
	}
	return stats;
}

nlohmann::json wzapi::getUsefulConstants()
{
	nlohmann::json constants = nlohmann::json::object();

	constants["TER_WATER"] = TER_WATER;
	constants["TER_CLIFFFACE"] = TER_CLIFFFACE;
	constants["WEATHER_CLEAR"] = WT_NONE;
	constants["WEATHER_RAIN"] = WT_RAINING;
	constants["WEATHER_SNOW"] = WT_SNOWING;
	constants["DORDER_ATTACK"] = DORDER_ATTACK;
	constants["DORDER_OBSERVE"] = DORDER_OBSERVE;
	constants["DORDER_RECOVER"] = DORDER_RECOVER;
	constants["DORDER_MOVE"] = DORDER_MOVE;
	constants["DORDER_SCOUT"] = DORDER_SCOUT;
	constants["DORDER_BUILD"] = DORDER_BUILD;
	constants["DORDER_HELPBUILD"] = DORDER_HELPBUILD;
	constants["DORDER_LINEBUILD"] = DORDER_LINEBUILD;
	constants["DORDER_REPAIR"] = DORDER_REPAIR;
	constants["DORDER_PATROL"] = DORDER_PATROL;
	constants["DORDER_DEMOLISH"] = DORDER_DEMOLISH;
	constants["DORDER_EMBARK"] = DORDER_EMBARK;
	constants["DORDER_DISEMBARK"] = DORDER_DISEMBARK;
	constants["DORDER_FIRESUPPORT"] = DORDER_FIRESUPPORT;
	constants["DORDER_COMMANDERSUPPORT"] = DORDER_COMMANDERSUPPORT;
	constants["DORDER_HOLD"] = DORDER_HOLD;
	constants["DORDER_RTR"] = DORDER_RTR;
	constants["DORDER_RTB"] = DORDER_RTB;
	constants["DORDER_STOP"] = DORDER_STOP;
	constants["DORDER_REARM"] = DORDER_REARM;
	constants["DORDER_RECYCLE"] = DORDER_RECYCLE;
	constants["COMMAND"] = IDRET_COMMAND; // deprecated
	constants["BUILD"] = IDRET_BUILD; // deprecated
	constants["MANUFACTURE"] = IDRET_MANUFACTURE; // deprecated
	constants["RESEARCH"] = IDRET_RESEARCH; // deprecated
	constants["INTELMAP"] = IDRET_INTEL_MAP; // deprecated
	constants["DESIGN"] = IDRET_DESIGN; // deprecated
	constants["CANCEL"] = IDRET_CANCEL; // deprecated
	constants["CAMP_CLEAN"] = CAMP_CLEAN;
	constants["CAMP_BASE"] = CAMP_BASE;
	constants["CAMP_WALLS"] = CAMP_WALLS;
	constants["NO_ALLIANCES"] = NO_ALLIANCES;
	constants["ALLIANCES"] = ALLIANCES;
	constants["ALLIANCES_TEAMS"] = ALLIANCES_TEAMS;
	constants["ALLIANCES_UNSHARED"] = ALLIANCES_UNSHARED;
	constants["NO_SCAVENGERS"] = NO_SCAVENGERS;
	constants["SCAVENGERS"] = SCAVENGERS;
	constants["ULTIMATE_SCAVENGERS"] = ULTIMATE_SCAVENGERS;
	constants["BEING_BUILT"] = SS_BEING_BUILT;
	constants["BUILT"] = SS_BUILT;
	constants["DROID_CONSTRUCT"] = DROID_CONSTRUCT;
	constants["DROID_WEAPON"] = DROID_WEAPON;
	constants["DROID_PERSON"] = DROID_PERSON;
	constants["DROID_REPAIR"] = DROID_REPAIR;
	constants["DROID_SENSOR"] = DROID_SENSOR;
	constants["DROID_ECM"] = DROID_ECM;
	constants["DROID_CYBORG"] = DROID_CYBORG;
	constants["DROID_TRANSPORTER"] = DROID_TRANSPORTER;
	constants["DROID_SUPERTRANSPORTER"] = DROID_SUPERTRANSPORTER;
	constants["DROID_COMMAND"] = DROID_COMMAND;
	constants["DROID_ANY"] = DROID_ANY;
	constants["OIL_RESOURCE"] = FEAT_OIL_RESOURCE;
	constants["OIL_DRUM"] = FEAT_OIL_DRUM;
	constants["ARTIFACT"] = FEAT_GEN_ARTE;
	constants["BUILDING"] = FEAT_BUILDING;
	constants["HQ"] = REF_HQ;
	constants["FACTORY"] = REF_FACTORY;
	constants["POWER_GEN"] = REF_POWER_GEN;
	constants["RESOURCE_EXTRACTOR"] = REF_RESOURCE_EXTRACTOR;
	constants["DEFENSE"] = REF_DEFENSE;
	constants["LASSAT"] = REF_LASSAT;
	constants["WALL"] = REF_WALL;
	constants["RESEARCH_LAB"] = REF_RESEARCH;
	constants["REPAIR_FACILITY"] = REF_REPAIR_FACILITY;
	constants["CYBORG_FACTORY"] = REF_CYBORG_FACTORY;
	constants["VTOL_FACTORY"] = REF_VTOL_FACTORY;
	constants["REARM_PAD"] = REF_REARM_PAD;
	constants["SAT_UPLINK"] = REF_SAT_UPLINK;
	constants["GATE"] = REF_GATE;
	constants["COMMAND_CONTROL"] = REF_COMMAND_CONTROL;
	constants["EASY"] = static_cast<int8_t>(AIDifficulty::EASY);
	constants["MEDIUM"] = static_cast<int8_t>(AIDifficulty::MEDIUM);
	constants["HARD"] = static_cast<int8_t>(AIDifficulty::HARD);
	constants["INSANE"] = static_cast<int8_t>(AIDifficulty::INSANE);
	constants["STRUCTURE"] = OBJ_STRUCTURE;
	constants["DROID"] = OBJ_DROID;
	constants["FEATURE"] = OBJ_FEATURE;
	constants["ALL_PLAYERS"] = ALL_PLAYERS;
	constants["ALLIES"] = ALLIES;
	constants["ENEMIES"] = ENEMIES;
	constants["POSITION"] = SCRIPT_POSITION;
	constants["AREA"] = SCRIPT_AREA;
	constants["RADIUS"] = SCRIPT_RADIUS;
	constants["GROUP"] = SCRIPT_GROUP;
	constants["PLAYER_DATA"] = SCRIPT_PLAYER;
	constants["RESEARCH_DATA"] = SCRIPT_RESEARCH;
	constants["LZ_COMPROMISED_TIME"] = JS_LZ_COMPROMISED_TIME;
	constants["OBJECT_FLAG_UNSELECTABLE"] = OBJECT_FLAG_UNSELECTABLE;
	// the constants below are subject to change without notice...
	constants["PROX_MSG"] = MSG_PROXIMITY;
	constants["CAMP_MSG"] = MSG_CAMPAIGN;
	constants["MISS_MSG"] = MSG_MISSION;
	constants["RES_MSG"] = MSG_RESEARCH;
	constants["LDS_EXPAND_LIMBO"] = static_cast<int8_t>(LEVEL_TYPE::LDS_EXPAND_LIMBO);

	return constants;
}

nlohmann::json wzapi::constructStaticPlayerData()
{
	// Static knowledge about players
	//== * ```playerData``` An array of information about the players in a game. Each item in the array is an object
	//== containing the following variables:
	//==   * ```difficulty``` (see ```difficulty``` global constant)
	//==   * ```colour``` number describing the colour of the player
	//==   * ```position``` number describing the position of the player in the game's setup screen
	//==   * ```isAI``` whether the player is an AI (3.2+ only)
	//==   * ```isHuman``` whether the player is human (3.2+ only)
	//==   * ```name``` the name of the player (3.2+ only)
	//==   * ```team``` the number of the team the player is part of
	nlohmann::json playerData = nlohmann::json::array(); //engine->newArray(game.maxPlayers);
	for (int i = 0; i < game.maxPlayers; i++)
	{
		nlohmann::json vector = nlohmann::json::object();
		vector["name"] = NetPlay.players[i].name;
		vector["difficulty"] = static_cast<int8_t>(NetPlay.players[i].difficulty);
		vector["faction"] = NetPlay.players[i].faction;
		vector["colour"] = NetPlay.players[i].colour;
		vector["position"] = NetPlay.players[i].position;
		vector["team"] = NetPlay.players[i].team;
		vector["isAI"] = !NetPlay.players[i].allocated && NetPlay.players[i].ai >= 0;
		vector["isHuman"] = NetPlay.players[i].allocated;
		vector["type"] = SCRIPT_PLAYER;
		playerData.push_back(std::move(vector));
	}
	return playerData;
}

nlohmann::json wzapi::constructMapTilesArray()
{
	// Static knowledge about map tiles
	//== * ```MapTiles``` A two-dimensional array of static information about the map tiles in a game. Each item in MapTiles[y][x] is an object
	//== containing the following variables:
	//==   * ```terrainType``` (see ```terrainType(x, y)``` function)
	//==   * ```height``` the height at the top left of the tile
	//==   * ```hoverContinent``` (For hover type propulsions)
	//==   * ```limitedContinent``` (For land or sea limited propulsion types)
	nlohmann::json mapTileArray = nlohmann::json::array();
	for (SDWORD y = 0; y < mapHeight; y++)
	{
		nlohmann::json mapRow = nlohmann::json::array();
		for (SDWORD x = 0; x < mapWidth; x++)
		{
			MAPTILE *psTile = mapTile(x, y);
			nlohmann::json mapTile = nlohmann::json::object();
			mapTile["terrainType"] = ::terrainType(psTile);
			mapTile["height"] = psTile->height;
			mapTile["hoverContinent"] = psTile->hoverContinent;
			mapTile["limitedContinent"] = psTile->limitedContinent;
			mapRow.push_back(std::move(mapTile));
		}
		mapTileArray.push_back(std::move(mapRow));
	}
	return mapTileArray;
}
