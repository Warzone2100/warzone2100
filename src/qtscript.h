/*
	This file is part of Warzone 2100.
	Copyright (C) 2011  Warzone 2100 Project

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

enum SCRIPT_TRIGGER_TYPE
{
	TRIGGER_GAME_INIT,
	TRIGGER_START_LEVEL
};

// ----------------------------------------------
// State functions

/// Initialize script system
bool initScripts();

/// Shutdown script system
bool shutdownScripts();

/// Run this each logical frame to update frame-dependent script states
bool updateScripts();

// Load and evaluate the given script, kept in memory
bool loadGlobalScript(QString path);
bool loadPlayerScript(QString path, int player, int difficulty);

// Set/write variables in the script's global context, run after loading script,
// but before triggering any events.
bool loadScriptStates(const char *filename);
bool saveScriptStates(const char *filename);

/// Load map labels (implemented in qtscriptfuncs.cpp)
bool loadLabels(const char *filename);

/// Write map labels to savegame (implemented in qtscriptfuncs.cpp)
bool writeLabels(const char *filename);

/// Tell script system that an object has been removed.
void scriptRemoveObject(const BASE_OBJECT *psObj);

// ----------------------------------------------
// Event functions

/// For generic, parameter-less triggers, using an enum to avoid declaring a ton of parameter-less functions
bool triggerEvent(SCRIPT_TRIGGER_TYPE trigger);

// For each trigger with function parameters, a function to trigger it here
bool triggerEventDroidBuilt(DROID *psDroid, STRUCTURE *psFactory);
bool triggerStructureAttacked(STRUCTURE *psVictim, BASE_OBJECT *psAttacker);
bool triggerResearched(RESEARCH *psResearch, STRUCTURE *psStruct, int player);
bool triggerEventStructBuilt(STRUCTURE *psStruct, DROID *psDroid);

// bool triggerEventReachedLocation(ORDER order, DROID *psDroid);
// ...

#endif
