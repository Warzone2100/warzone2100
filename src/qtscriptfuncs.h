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

#ifndef __INCLUDED_QTSCRIPTFUNCS_H__
#define __INCLUDED_QTSCRIPTFUNCS_H__


#include "lib/framework/frame.h"
#include "qtscript.h"
#include "featuredef.h"

enum SCRIPT_TYPES
{
	POSITION = OBJ_NUM_TYPES,
	AREA,
};

#include <QtScript/QScriptEngine>

// ----------------------------------------------
// Private to scripting module functions below

/// Register functions to engine context
bool registerFunctions(QScriptEngine *engine);

// Utility conversion functions
QScriptValue convDroid(DROID *psDroid, QScriptEngine *engine);
QScriptValue convStructure(STRUCTURE *psStruct, QScriptEngine *engine);
QScriptValue convObj(BASE_OBJECT *psObj, QScriptEngine *engine);
QScriptValue convFeature(FEATURE *psFeature, QScriptEngine *engine);
QScriptValue convMax(BASE_OBJECT *psObj, QScriptEngine *engine);
QScriptValue convResearch(RESEARCH *psResearch, QScriptEngine *engine, int player);

#endif
