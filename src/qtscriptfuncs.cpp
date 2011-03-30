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
/**
 * @file qtscriptfuncs.cpp
 *
 * New scripting system -- script functions
 */

#include "qtscriptfuncs.h"

#include <QtScript/QScriptValue>
#include <QtCore/QStringList>

#include "console.h"
#include "map.h"
#include "group.h"

// ----------------------------------------------------------------------------------------
// Utility functions -- not called directly from scripts

#define SCRIPT_ASSERT(context, expr, ...) \
	do { bool _wzeval = (expr); if (!_wzeval) { debug(LOG_ERROR, __VA_ARGS__); context->throwError(QScriptContext::ReferenceError, QString(#expr) +  " failed in " + QString(__FUNCTION__) + " at line " + QString::number(__LINE__)); return QScriptValue(); } } while (0)

QScriptValue convStructure(STRUCTURE *psStruct, QScriptEngine *engine)
{
	QScriptValue value = engine->newObject();
	value.setProperty("id", psStruct->id, QScriptValue::ReadOnly);
	value.setProperty("x", psStruct->pos.x, QScriptValue::ReadOnly);
	value.setProperty("y", psStruct->pos.y, QScriptValue::ReadOnly);
	value.setProperty("player", psStruct->player, QScriptValue::ReadOnly);
	return value;
}

QScriptValue convDroid(DROID *psDroid, QScriptEngine *engine)
{
	QScriptValue value = engine->newObject();
	value.setProperty("id", psDroid->id, QScriptValue::ReadOnly);
	value.setProperty("x", psDroid->pos.x, QScriptValue::ReadOnly);
	value.setProperty("y", psDroid->pos.y, QScriptValue::ReadOnly);
	value.setProperty("player", psDroid->player, QScriptValue::ReadOnly);
	return value;
}

QScriptValue convObj(BASE_OBJECT *psObj, QScriptEngine *engine)
{
	QScriptValue value = engine->newObject();
	value.setProperty("id", psObj->id, QScriptValue::ReadOnly);
	value.setProperty("x", psObj->pos.x, QScriptValue::ReadOnly);
	value.setProperty("y", psObj->pos.y, QScriptValue::ReadOnly);
	value.setProperty("player", psObj->player, QScriptValue::ReadOnly);
	return value;
}

// ----------------------------------------------------------------------------------------
// Script functions
//
// All script functions should be prefixed with "js_" then followed by same name as in script.

static QScriptValue js_enumGroup(QScriptContext *context, QScriptEngine *engine)
{
	int groupId = context->argument(0).toInt32();
	QList<DROID *> matches;
	DROID_GROUP *psGroup = grpFind(groupId);
	DROID *psCurr;

	SCRIPT_ASSERT(context, psGroup, "Invalid group index %d", groupId);
	for (psCurr = psGroup->psList; psCurr != NULL; psCurr = psCurr->psGrpNext)
	{
		matches.push_back(psCurr);
	}
	QScriptValue result = engine->newArray(matches.size());
	for (int i = 0; i < matches.size(); i++)
	{
		DROID *psDroid = matches.at(i);
		result.setProperty(i, convDroid(psDroid, engine));
	}
	return result;
}

static QScriptValue js_newGroup(QScriptContext *, QScriptEngine *)
{
	DROID_GROUP *newGrp = grpCreate();
	return QScriptValue(newGrp->id);
}

static QScriptValue js_enumStruct(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue statsName = context->argument(0);
	QScriptValue targetPlayer = context->argument(1);
	QScriptValue lookingPlayer = context->argument(2);
	QList<STRUCTURE *> matches;

	int player = targetPlayer.toInt32();
	int looking = -1;
	QString stat = statsName.toString();
	if (!lookingPlayer.isUndefined())	// third arg optional
	{
		looking = lookingPlayer.toInt32();
	}
	SCRIPT_ASSERT(context, player < MAX_PLAYERS && player >= 0, "Target player index out of range: %d", player);
	SCRIPT_ASSERT(context, looking < MAX_PLAYERS && looking >= -1, "Looking player index out of range: %d", looking);
	for (STRUCTURE *psStruct = apsStructLists[player]; psStruct; psStruct = psStruct->psNext)
	{
		if ((looking == -1 || psStruct->visible[looking]) && stat.compare(psStruct->pStructureType->pName) == 0)
		{
			matches.push_back(psStruct);
		}
	}
	QScriptValue result = engine->newArray(matches.size());
	for (int i = 0; i < matches.size(); i++)
	{
		STRUCTURE *psStruct = matches.at(i);
		result.setProperty(i, convStructure(psStruct, engine));
	}
	return result;
}

// is this really useful?
static QScriptValue js_debug(QScriptContext *context, QScriptEngine *engine)
{
	QString result;
	for (int i = 0; i < context->argumentCount(); ++i)
	{
		if (i != 0)
		{
			result.append(QLatin1String(" "));
		}
		QString s = context->argument(i).toString();
		if (context->state() == QScriptContext::ExceptionState)
		{
			break;
		}
		result.append(s);
	}
	qWarning(result.toAscii().constData());
	return QScriptValue();
}

static QScriptValue js_scavengerPlayer(QScriptContext *, QScriptEngine *)
{
	return QScriptValue(scavengerPlayer());
}

static QScriptValue js_structureIdle(QScriptContext *context, QScriptEngine *)
{
	QScriptValue structVal = context->argument(0);
	int id = structVal.property("id").toInt32();
	int player = structVal.property("player").toInt32();
	STRUCTURE *psStruct = IdToStruct(id, player);
	SCRIPT_ASSERT(context, psStruct, "No such structure id %d belonging to player %d", id, player);
	return QScriptValue(structureIdle(psStruct));
}

// TODO, should cover scrShowConsoleText, scrAddConsoleText, scrTagConsoleText and scrConsole
static QScriptValue js_console(QScriptContext *context, QScriptEngine *engine)
{
	int player = engine->globalObject().property("me").toInt32();
	if (player == selectedPlayer)
	{
		QString result;
		for (int i = 0; i < context->argumentCount(); ++i)
		{
			if (i != 0)
			{
				result.append(QLatin1String(" "));
			}
			QString s = context->argument(i).toString();
			if (context->state() == QScriptContext::ExceptionState)
			{
				break;
			}
			result.append(s);
		}
		//permitNewConsoleMessages(true);
		//setConsolePermanence(true,true);
		addConsoleMessage(result.toAscii().constData(), CENTRE_JUSTIFY, SYSTEM_MESSAGE);
		//permitNewConsoleMessages(false);
	}
	return QScriptValue();
}

/* Build a droid template in the specified factory */
static QScriptValue js_buildDroid(QScriptContext *context, QScriptEngine *)
{
	QScriptValue structVal = context->argument(1);
	int id = structVal.property("id").toInt32();
	int player = structVal.property("player").toInt32();
	QScriptValue templName = context->argument(0);
	DROID_TEMPLATE *psTemplate = getTemplateFromTranslatedNameNoPlayer(templName.toString().toUtf8().constData());
	STRUCTURE *psStruct = IdToStruct(id, player);

	SCRIPT_ASSERT(context, psStruct != NULL, "No factory object found for id %d, player %d", id, player);
	SCRIPT_ASSERT(context, psTemplate != NULL, "No template object found for %s sent to %s", templName.toString().toUtf8().constData(), objInfo(psStruct));
	SCRIPT_ASSERT(context, (psStruct->pStructureType->type == REF_FACTORY || psStruct->pStructureType->type == REF_CYBORG_FACTORY
		       || psStruct->pStructureType->type == REF_VTOL_FACTORY), "Structure %s is not a factory", objInfo(psStruct));
	SCRIPT_ASSERT(context, validTemplateForFactory(psTemplate, psStruct), "Invalid template - %s for factory - %s",
			 psTemplate->aName, psStruct->pStructureType->pName);

	return QScriptValue(structSetManufacture(psStruct, psTemplate, ModeQueue));
}

static QScriptValue js_groupAddArea(QScriptContext *context, QScriptEngine *engine)
{
	int groupId = context->argument(0).toInt32();
	int player = engine->globalObject().property("me").toInt32();
	int x1 = context->argument(1).toInt32();
	int y1 = context->argument(2).toInt32();
	int x2 = context->argument(3).toInt32();
	int y2 = context->argument(4).toInt32();
	DROID_GROUP *psGroup = grpFind(groupId);

	SCRIPT_ASSERT(context, psGroup, "Invalid group index %d", groupId);
	for (DROID *psDroid = apsDroidLists[player]; psGroup && psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->pos.x >= x1 && psDroid->pos.x <= x2 && psDroid->pos.y >= y1 && psDroid->pos.y <= y2
		    && psDroid->droidType != DROID_COMMAND && psDroid->droidType != DROID_TRANSPORTER)
		{
			psGroup->add(psDroid);
		}
	}
	return QScriptValue();
}

static QScriptValue js_groupAddDroid(QScriptContext *context, QScriptEngine *engine)
{
	int groupId = context->argument(0).toInt32();
	DROID_GROUP *psGroup = grpFind(groupId);
	QScriptValue droidVal = context->argument(1);
	int droidId = droidVal.property("id").toInt32();
	int droidPlayer = droidVal.property("player").toInt32();
	DROID *psDroid = IdToDroid(droidId, droidPlayer);

	SCRIPT_ASSERT(context, psGroup, "Invalid group index %d", groupId);
	SCRIPT_ASSERT(context, psDroid, "Invalid droid index %d", droidId);
	psGroup->add(psDroid);
	return QScriptValue();
}

#if 0
static QScriptValue js_getDerrick(QScriptContext *context, QScriptEngine *engine)
{
	BASE_OBJECT *psObj = NULL;
	QScriptValue param = context->argument(0);
	int i = param.toInt32();

	if (i < (int)derricks.size())
	{
		const int x = derricks[i].x;
		const int y = derricks[i].y;
		MAPTILE *psTile = worldTile(x, y);
		if (psTile)
		{
			psObj = psTile->psObject;
		}
	}
	return convStructure(psObj, engine);
}
#endif

static QScriptValue js_distBetweenTwoPoints(QScriptContext *context, QScriptEngine *engine)
{
	int x1 = context->argument(0).toInt32();
	int y1 = context->argument(1).toInt32();
	int x2 = context->argument(2).toInt32();
	int y2 = context->argument(3).toInt32();
	return QScriptValue(hypotf(x1 - x2, y1 - y2));
}

static QScriptValue js_groupSize(QScriptContext *context, QScriptEngine *)
{
	int groupId = context->argument(0).toInt32();
	DROID_GROUP *psGroup = grpFind(groupId);
	SCRIPT_ASSERT(context, psGroup, "Group %d not found", groupId);
	return QScriptValue(psGroup->refCount);
}

static QScriptValue js_orderDroidLoc(QScriptContext *context, QScriptEngine *)
{
	QScriptValue droidVal = context->argument(0);
	int id = droidVal.property("id").toInt32();
	int player = droidVal.property("player").toInt32();
	QScriptValue orderVal = context->argument(1);
	int x = context->argument(2).toInt32();
	int y = context->argument(3).toInt32();
	DROID_ORDER order = (DROID_ORDER)orderVal.toInt32();
	DROID *psDroid = IdToDroid(id, player);
	SCRIPT_ASSERT(context, psDroid, "Droid id %d not found belonging to player %d", id, player);
	SCRIPT_ASSERT(context, worldOnMap(x, y), "Outside map bounds (%d, %d)", x, y);
	orderDroidLoc(psDroid, order, x, y, ModeQueue);
	return QScriptValue();
}

// ----------------------------------------------------------------------------------------
// Register functions with scripting system

bool registerFunctions(QScriptEngine *engine)
{
	// Register functions to the script engine here
	//engine->globalObject().setProperty("getDerrick", engine->newFunction(js_getDerrick));
	engine->globalObject().setProperty("debug", engine->newFunction(js_debug));
	engine->globalObject().setProperty("console", engine->newFunction(js_console));
	engine->globalObject().setProperty("scavengerPlayer", engine->newFunction(js_scavengerPlayer));
	engine->globalObject().setProperty("structureIdle", engine->newFunction(js_structureIdle));
	engine->globalObject().setProperty("buildDroid", engine->newFunction(js_buildDroid));
	engine->globalObject().setProperty("enumStruct", engine->newFunction(js_enumStruct));
	engine->globalObject().setProperty("enumGroup", engine->newFunction(js_enumGroup));
	engine->globalObject().setProperty("distBetweenTwoPoints", engine->newFunction(js_distBetweenTwoPoints));
	engine->globalObject().setProperty("newGroup", engine->newFunction(js_newGroup));
	engine->globalObject().setProperty("groupAddArea", engine->newFunction(js_groupAddArea));
	engine->globalObject().setProperty("groupAddDroid", engine->newFunction(js_groupAddDroid));
	engine->globalObject().setProperty("groupSize", engine->newFunction(js_groupSize));
	engine->globalObject().setProperty("orderDroidLoc", engine->newFunction(js_orderDroidLoc));

	// Set some useful constants
	engine->globalObject().setProperty("DORDER_ATTACK", DORDER_ATTACK, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_MOVE", DORDER_MOVE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_SCOUT", DORDER_SCOUT, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_BUILD", DORDER_BUILD, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("mapWidth", mapWidth, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("mapHeight", mapHeight, QScriptValue::ReadOnly | QScriptValue::Undeletable);

	return true;
}
