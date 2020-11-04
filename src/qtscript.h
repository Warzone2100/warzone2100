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

#ifndef __INCLUDED_QTSCRIPT_H__
#define __INCLUDED_QTSCRIPT_H__

#include "lib/framework/frame.h"
#include "lib/netplay/netplay.h"
#include "random.h"

class QScriptEngine;
class QString;
class WzString;
struct BASE_OBJECT;
struct DROID;
struct DROID_TEMPLATE;
struct FEATURE;
struct RESEARCH;
struct STRUCTURE;

enum SCRIPT_TRIGGER_TYPE
{
	TRIGGER_GAME_INIT,
	TRIGGER_START_LEVEL,
	TRIGGER_TRANSPORTER_ARRIVED,
	TRIGGER_TRANSPORTER_LANDED,
	TRIGGER_TRANSPORTER_LAUNCH,
	TRIGGER_TRANSPORTER_EXIT,
	TRIGGER_TRANSPORTER_DONE,
	TRIGGER_DELIVERY_POINT_MOVING,
	TRIGGER_DELIVERY_POINT_MOVED,
	TRIGGER_VIDEO_QUIT,
	TRIGGER_MISSION_TIMEOUT,
	TRIGGER_GAME_LOADED,
	TRIGGER_GAME_SAVING,
	TRIGGER_GAME_SAVED,
	TRIGGER_DESIGN_BODY,
	TRIGGER_DESIGN_WEAPON,
	TRIGGER_DESIGN_COMMAND,
	TRIGGER_DESIGN_SYSTEM,
	TRIGGER_DESIGN_PROPULSION,
	TRIGGER_DESIGN_QUIT,
	TRIGGER_MENU_DESIGN_UP,
	TRIGGER_MENU_BUILD_UP,
	TRIGGER_MENU_BUILD_SELECTED,
	TRIGGER_MENU_MANUFACTURE_UP,
	TRIGGER_MENU_RESEARCH_UP,
	TRIGGER_MENU_RESEARCH_SELECTED,
	TRIGGER_OBJECT_RECYCLED
};

extern bool bInTutorial;

// ----------------------------------------------
// State functions

bool scriptInit();
void scriptSetStartPos(int position, int x, int  y);
void scriptSetDerrickPos(int x, int y);
Vector2i getPlayerStartPosition(int player);

/// Initialize script system
bool initScripts();

/// Shutdown script system
bool shutdownScripts();

/// Run after all data is loaded, but before game is started.
bool prepareScripts(bool loadGame);

/// Run this each logical frame to update frame-dependent script states
bool updateScripts();

// Load and evaluate the given script, kept in memory
bool loadGlobalScript(WzString path);
QScriptEngine *loadPlayerScript(const WzString& path, int player, AIDifficulty difficulty);

// Set/write variables in the script's global context, run after loading script,
// but before triggering any events.
bool loadScriptStates(const char *filename);
bool saveScriptStates(const char *filename);

/// Load map labels (implemented in qtscriptfuncs.cpp)
bool loadLabels(const char *filename);

/// Write map labels to savegame (implemented in qtscriptfuncs.cpp)
bool writeLabels(const char *filename);

/// Tell script system that an object has been removed.
void scriptRemoveObject(BASE_OBJECT *psObj);

/// Open debug GUI
void jsShowDebug();

/// Choose autogame AI with GUI
void jsAutogame();

/// Choose a specific autogame AI
void jsAutogameSpecific(const WzString &name, int player);

/// Run-time code from user
bool jsEvaluate(QScriptEngine *engine, const QString &text);

/// Run a named script callback
bool namedScriptCallback(QScriptEngine *engine, const WzString& func, int player);

// ----------------------------------------------
// Event functions

/// For generic, parameter-less triggers, using an enum to avoid declaring a ton of parameter-less functions
bool triggerEvent(SCRIPT_TRIGGER_TYPE trigger, BASE_OBJECT *psObj = nullptr);

// For each trigger with function parameters, a function to trigger it here
bool triggerEventDroidBuilt(DROID *psDroid, STRUCTURE *psFactory);
bool triggerEventAttacked(BASE_OBJECT *psVictim, BASE_OBJECT *psAttacker, int lastHit);
bool triggerEventResearched(RESEARCH *psResearch, STRUCTURE *psStruct, int player);
bool triggerEventStructBuilt(STRUCTURE *psStruct, DROID *psDroid);
bool triggerEventStructDemolish(STRUCTURE *psStruct, DROID *psDroid);
bool triggerEventDroidIdle(DROID *psDroid);
bool triggerEventDestroyed(BASE_OBJECT *psVictim);
bool triggerEventStructureReady(STRUCTURE *psStruct);
bool triggerEventSeen(BASE_OBJECT *psViewer, BASE_OBJECT *psSeen);
bool triggerEventObjectTransfer(BASE_OBJECT *psObj, int from);
bool triggerEventChat(int from, int to, const char *message);
bool triggerEventBeacon(int from, int to, const char *message, int x, int y);
bool triggerEventBeaconRemoved(int from, int to);
bool triggerEventPickup(FEATURE *psFeat, DROID *psDroid);
bool triggerEventCheatMode(bool entered);
bool triggerEventGroupLoss(BASE_OBJECT *psObj, int group, int size, QScriptEngine *engine);
bool triggerEventDroidMoved(DROID *psDroid, int oldx, int oldy);
bool triggerEventArea(const QString& label, DROID *psDroid);
bool triggerEventSelected();
bool triggerEventPlayerLeft(int id);
bool triggerEventDesignCreated(DROID_TEMPLATE *psTemplate);
bool triggerEventSyncRequest(int from, int req_id, int x, int y, BASE_OBJECT *psObj, BASE_OBJECT *psObj2);
bool triggerEventKeyPressed(int meta, int key);
bool triggerEventAllianceOffer(uint8_t from, uint8_t to);
bool triggerEventAllianceAccepted(uint8_t from, uint8_t to);
bool triggerEventAllianceBroken(uint8_t from, uint8_t to);

// ----------------------------------------------
// Debug functions

void jsDebugSelected(const BASE_OBJECT *psObj);
void jsDebugMessageUpdate();
void jsDebugUpdate();

struct ScriptMapData
{
	struct Structure
	{
		WzString name;
		Vector2i position;
		uint16_t direction;
		uint8_t modules;
		int8_t player;  // -1 = scavs
	};
	struct Droid
	{
		WzString name;
		Vector2i position;
		uint16_t direction;
		int8_t player;  // -1 = scavs
	};
	struct Feature
	{
		WzString name;
		Vector2i position;
		uint16_t direction;
	};

	uint32_t crcSumStructures(uint32_t crc) const;
	uint32_t crcSumDroids(uint32_t crc) const;
	uint32_t crcSumFeatures(uint32_t crc) const;

	bool valid = false;
	int mapWidth;
	int mapHeight;
	//int maxPlayers;
	std::vector<uint16_t> texture;
	std::vector<int16_t> height;
	std::vector<Structure> structures;
	std::vector<Droid> droids;
	std::vector<Feature> features;

	MersenneTwister mt;
};

ScriptMapData runMapScript(WzString const &path, uint64_t seed, bool preview);

#define QStringToWzString(_qstring) \
WzString::fromUtf8((_qstring).toUtf8().constData())

#endif
