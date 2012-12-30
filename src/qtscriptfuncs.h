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
	SCRIPT_POSITION = OBJ_NUM_TYPES,
	SCRIPT_AREA,
	SCRIPT_PLAYER,
	SCRIPT_RESEARCH,
	SCRIPT_GROUP
};

#include <QtScript/QScriptEngine>

// ----------------------------------------------
// Private to scripting module functions below

void groupRemoveObject(BASE_OBJECT *psObj);

/// Register functions to engine context
bool registerFunctions(QScriptEngine *engine, QString scriptName);
bool unregisterFunctions(QScriptEngine *engine);

bool saveGroups(WzConfig &ini, QScriptEngine *engine);
bool loadGroup(QScriptEngine *engine, int groupId, int objId);
void prepareLabels();

bool areaLabelCheck(DROID *psDroid);

// Utility conversion functions
QScriptValue convDroid(DROID *psDroid, QScriptEngine *engine);
QScriptValue convStructure(STRUCTURE *psStruct, QScriptEngine *engine);
QScriptValue convObj(BASE_OBJECT *psObj, QScriptEngine *engine);
QScriptValue convFeature(FEATURE *psFeature, QScriptEngine *engine);
QScriptValue convMax(BASE_OBJECT *psObj, QScriptEngine *engine);
QScriptValue convResearch(RESEARCH *psResearch, QScriptEngine *engine, int player);
BASE_OBJECT *IdToObject(OBJECT_TYPE type, int id, int player);

// Script functions useful also in qtscript.cpp
QScriptValue js_enumSelected(QScriptContext *, QScriptEngine *engine);

/// Assert for scripts that give useful backtraces and other info.
#define SCRIPT_ASSERT(context, expr, ...) \
	do { bool _wzeval = (expr); \
	     if (!_wzeval) { debug(LOG_ERROR, __VA_ARGS__); \
	       context->throwError(QScriptContext::ReferenceError, QString(#expr) +  " failed in " + QString(__FUNCTION__) + " at line " + QString::number(__LINE__)); \
	     return QScriptValue::NullValue; } } while (0)
#endif
