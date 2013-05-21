/*
	This file is part of Warzone 2100.
	Copyright (C) 2013  Warzone 2100 Project

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
#include "basedef.h"
#include "droiddef.h"
#include "structuredef.h"
#include "researchdef.h"
#include "featuredef.h"

class QScriptEngine;

enum SCRIPT_TRIGGER_TYPE
{
	TRIGGER_GAME_INIT,
	TRIGGER_START_LEVEL,
	TRIGGER_TRANSPORTER_ARRIVED,
	TRIGGER_TRANSPORTER_LANDED,
	TRIGGER_TRANSPORTER_LAUNCH,
	TRIGGER_TRANSPORTER_EXIT,
	TRIGGER_TRANSPORTER_DONE,
	TRIGGER_VIDEO_QUIT,
	TRIGGER_MISSION_TIMEOUT,
	TRIGGER_GAME_LOADED,
	TRIGGER_GAME_SAVING,
	TRIGGER_GAME_SAVED,
	TRIGGER_OBJECT_RECYCLED
};

// ----------------------------------------------
// State functions

/// Initialize script system
bool initScripts();

/// Shutdown script system
bool shutdownScripts();

/// Run after all data is loaded, but before game is started.
bool prepareScripts();

/// Run this each logical frame to update frame-dependent script states
bool updateScripts();

// Load and evaluate the given script, kept in memory
bool loadGlobalScript(QString path);
QScriptEngine *loadPlayerScript(QString path, int player, int difficulty);

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

/// Choose autogame AI
void jsAutogame();

/// Run-time code from user
bool jsEvaluate(QScriptEngine *engine, const QString &text);

// ----------------------------------------------
// Event functions

/// For generic, parameter-less triggers, using an enum to avoid declaring a ton of parameter-less functions
bool triggerEvent(SCRIPT_TRIGGER_TYPE trigger, BASE_OBJECT *psObj = NULL);

// For each trigger with function parameters, a function to trigger it here
bool triggerEventDroidBuilt(DROID *psDroid, STRUCTURE *psFactory);
bool triggerEventAttacked(BASE_OBJECT *psVictim, BASE_OBJECT *psAttacker, int lastHit);
bool triggerEventResearched(RESEARCH *psResearch, STRUCTURE *psStruct, int player);
bool triggerEventStructBuilt(STRUCTURE *psStruct, DROID *psDroid);
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
bool triggerEventArea(QString label, DROID *psDroid);
bool triggerEventSelected();
bool triggerEventPlayerLeft(int id);
bool triggerEventDesignCreated(DROID_TEMPLATE *psTemplate);

#endif
