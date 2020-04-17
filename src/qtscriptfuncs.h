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

#ifndef __INCLUDED_QTSCRIPTFUNCS_H__
#define __INCLUDED_QTSCRIPTFUNCS_H__

#include "lib/framework/frame.h"
#include "qtscript.h"
#include "featuredef.h"

// ----------------------------------------------
// Private to scripting module functions below

// Utility conversion functions
BASE_OBJECT *IdToObject(OBJECT_TYPE type, int id, int player);

/// Dump script-relevant log info to separate file
void dumpScriptLog(const WzString &scriptName, int me, const std::string &info);

/// Clear all map markers (used by label marking, for instance)
void clearMarks();

wzapi::scripting_instance* createQtScriptInstance(const WzString& path, int player, int difficulty);
ScriptMapData runMapScript_QtScript(WzString const &path, uint64_t seed, bool preview);

#endif
