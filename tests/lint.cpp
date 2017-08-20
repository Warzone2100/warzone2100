/*
	This file is part of Warzone 2100.
	Copyright (C) 2013-2017  Warzone 2100 Project

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
 * @file lint.cpp
 *
 * New scripting system tester
 */

#include "lint.h"

#include <QtCore/QTextStream>
#include <QtCore/QHash>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>
#include <QtScript/QScriptValueIterator>
#include <QtScript/QScriptSyntaxCheckResult>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QSettings>
#include <QtCore/QFile>

#define CUR_PLAYERS 4

enum OBJECT_TYPE
{
	OBJ_DROID,      ///< Droids
	OBJ_STRUCTURE,  ///< All Buildings
	OBJ_FEATURE,    ///< Things like roads, trees, bridges, fires
	OBJ_PROJECTILE, ///< Comes out of guns, stupid :-)
	OBJ_TARGET,     ///< for the camera tracking
	OBJ_NUM_TYPES,   ///< number of object types
	POSITION = OBJ_NUM_TYPES,
	AREA
};

enum STRUCT_STATES
{
	SS_BEING_BUILT,
	SS_BUILT,
	SS_BEING_DEMOLISHED
};

struct timerNode
{
	QString function;
	QScriptEngine *engine;
	int player;
	timerNode() {}
	timerNode(QScriptEngine *caller, QString val, int plr) : function(val) { player = plr; engine = caller; }
	bool operator== (const timerNode &t) { return function == t.function && player == t.player; }
};
static QList<timerNode> timers;

// Pseudorandom values
static int obj_uid = 11;
#define MAX_PLAYERS 11

#define ASSERT(_cond, ...) do { if (!_cond) qFatal(__VA_ARGS__); } while(0)

// ----------------------------------------------------------------------------------------
// Utility functions -- not called directly from scripts

#define SCRIPT_ASSERT(context, expr, ...) \
	do { bool _wzeval = (expr); if (!_wzeval) { qWarning(__VA_ARGS__); context->throwError(QScriptContext::ReferenceError, QString(#expr) +  " failed in " + QString(__FUNCTION__) + " at line " + QString::number(__LINE__)); return QScriptValue(); } } while (0)

#define ARG_NUMBER(vnum) \
	SCRIPT_ASSERT(context, context->argument(vnum).isNumber(), "Argument %d should be a number", vnum)

#define ARG_STRING(vnum) \
	SCRIPT_ASSERT(context, context->argument(vnum).isString(), "Argument %d should be a string", vnum)

#define ARG_BOOL(vnum) \
	SCRIPT_ASSERT(context, context->argument(vnum).isBool(), "Argument %d should be a boolean", vnum)

#define ARG_COUNT_EXACT(vnum) \
	SCRIPT_ASSERT(context, context->argumentCount() == vnum, "Wrong number of arguments - must be exactly %d", vnum);

#define ARG_COUNT_VAR(vmin, vmax) \
	SCRIPT_ASSERT(context, context->argumentCount() >= vmin && context->argumentCount() <= vmax, "Wrong number of arguments - must be between %d and %d", vmin, vmax);

#define ARG_PLAYER(vnum) \
	do { ARG_NUMBER(vnum); int vplayer = context->argument(vnum).toInt32(); SCRIPT_ASSERT(context, vplayer < MAX_PLAYERS && vplayer >= 0, "Invalid player %d", vplayer); } while(0)

#define ARG_ORDER(vnum) \
	ARG_NUMBER(vnum)

#define ARG_OBJ(vnum) do { \
	SCRIPT_ASSERT(context, context->argument(vnum).isObject(), "Argument %d should be an object", vnum); \
	QScriptValue vval = context->argument(vnum); \
	SCRIPT_ASSERT(context, vval.property("id").isNumber(), "Invalid object ID argument %d", vnum); \
	SCRIPT_ASSERT(context, vval.property("player").isNumber(), "Invalid object player argument %d", vnum); \
	int vplayer = vval.property("player").toInt32(); \
	SCRIPT_ASSERT(context, vplayer < MAX_PLAYERS && vplayer >= 0, "Invalid object player %d", vplayer); \
	} while(0)

#define ARG_DROID(vnum)	ARG_OBJ(vnum)
#define ARG_STRUCT(vnum) ARG_OBJ(vnum)

static QScriptValue convResearch(QScriptEngine *engine)
{
	QScriptValue value = engine->newObject();
	value.setProperty("power", 100);
	value.setProperty("points", 100);
	value.setProperty("started", 0); // including whether an ally has started it
	value.setProperty("done", false);
	value.setProperty("name", "Fake");
	return value;
}

static QScriptValue convObj(QScriptEngine *engine)
{
	QScriptValue value = engine->newObject();
	value.setProperty("id", obj_uid++, QScriptValue::ReadOnly);
	value.setProperty("x", 11, QScriptValue::ReadOnly);
	value.setProperty("y", 11, QScriptValue::ReadOnly);
	value.setProperty("z", 0, QScriptValue::ReadOnly);
	value.setProperty("player", 1, QScriptValue::ReadOnly);
	value.setProperty("selected", 0, QScriptValue::ReadOnly);
	value.setProperty("player", 0, QScriptValue::ReadOnly);
	value.setProperty("armour", 0, QScriptValue::ReadOnly);
	value.setProperty("thermal", 0, QScriptValue::ReadOnly);
	value.setProperty("selected", false, QScriptValue::ReadOnly);
	value.setProperty("name", "Fake", QScriptValue::ReadOnly);
	return value;
}

// These functions invent new imaginary objects
static QScriptValue convStructure(QScriptEngine *engine)
{
	QScriptValue value = convObj(engine);
	value.setProperty("type", OBJ_STRUCTURE, QScriptValue::ReadOnly);
	value.setProperty("status", (int)SS_BUILT, QScriptValue::ReadOnly);
	value.setProperty("isCB", false, QScriptValue::ReadOnly);
	value.setProperty("isSensor", false, QScriptValue::ReadOnly);
	value.setProperty("canHitAir", false, QScriptValue::ReadOnly);
	value.setProperty("canHitGround", true, QScriptValue::ReadOnly);
	value.setProperty("hasIndirect", false, QScriptValue::ReadOnly);
	value.setProperty("isRadarDetector", false, QScriptValue::ReadOnly);
	value.setProperty("range", 100, QScriptValue::ReadOnly);
	value.setProperty("health", 100, QScriptValue::ReadOnly);
	value.setProperty("cost", 100, QScriptValue::ReadOnly);
	value.setProperty("modules", QScriptValue::NullValue);
	return value;
}

static QScriptValue convDroid(QScriptEngine *engine)
{
	QScriptValue value = convObj(engine);
	value.setProperty("type", OBJ_DROID, QScriptValue::ReadOnly);
	value.setProperty("action", 0, QScriptValue::ReadOnly);
	value.setProperty("order", 0, QScriptValue::ReadOnly);
	value.setProperty("action", 0, QScriptValue::ReadOnly);
	value.setProperty("isCB", false, QScriptValue::ReadOnly);
	value.setProperty("isSensor", false, QScriptValue::ReadOnly);
	value.setProperty("canHitAir", false, QScriptValue::ReadOnly);
	value.setProperty("canHitGround", true, QScriptValue::ReadOnly);
	value.setProperty("hasIndirect", false, QScriptValue::ReadOnly);
	value.setProperty("isRadarDetector", false, QScriptValue::ReadOnly);
	value.setProperty("range", 100, QScriptValue::ReadOnly);
	value.setProperty("health", 100, QScriptValue::ReadOnly);
	value.setProperty("cost", 100, QScriptValue::ReadOnly);
	value.setProperty("experience", 1000, QScriptValue::ReadOnly);
	value.setProperty("group", QScriptValue::NullValue);
	value.setProperty("armed", QScriptValue::NullValue);
	return value;
}

static QScriptValue convFeature(QScriptEngine *engine)
{
	QScriptValue value = convObj(engine);
	value.setProperty("type", OBJ_FEATURE, QScriptValue::ReadOnly);
	value.setProperty("health", 100, QScriptValue::ReadOnly);
	value.setProperty("damageable", true, QScriptValue::ReadOnly);
	return value;
}

// Call a function by name
static bool callFunction(QScriptEngine *engine, QString function, QScriptValueList args, bool required=false)
{
	QScriptValue value = engine->globalObject().property(function);
	if (!value.isValid() || !value.isFunction())
	{
		ASSERT(!required, "Function %s not found", function.toUtf8().constData());
		return false;	// not necessarily an error, may just be a trigger that is not defined (ie not needed)
	}
	QScriptValue result = value.call(QScriptValue(), args);
	if (engine->hasUncaughtException())
	{
		int line = engine->uncaughtExceptionLineNumber();
		QStringList bt = engine->uncaughtExceptionBacktrace();
		for (int i = 0; i < bt.size(); i++)
		{
			fprintf(stderr, "%d : %s\n", i, bt.at(i).toUtf8().constData());
		}
		fprintf(stderr, "Uncaught exception calling function \"%s\" at line %d: %s\n",
		       function.toUtf8().constData(), line, result.toString().toUtf8().constData());
		return false;
	}
	return true;
}

static QScriptValue js_enumLabels(QScriptContext *, QScriptEngine *)
{
	return QScriptValue();
}

static QScriptValue js_label(QScriptContext *, QScriptEngine *engine)
{
	QScriptValue ret = engine->newObject();
	ret.setProperty("x", 5);
	ret.setProperty("y", 5);
	ret.setProperty("x2", 15);
	ret.setProperty("y2", 15);
	return ret;
}

static QScriptValue js_enumBlips(QScriptContext *context, QScriptEngine *engine)
{
	ARG_PLAYER(0);
	QScriptValue result = engine->newArray(0);
	return result;
}

static QScriptValue js_enumGateways(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue result = engine->newArray(0);
	return result;
}

static QScriptValue js_enumGroup(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(1);
	ARG_NUMBER(0);
	QScriptValue result = engine->newArray(3);
	for (int i = 0; i < 3; i++)
	{
		result.setProperty(i, convDroid(engine));
	}
	return result;
}

static QScriptValue js_newGroup(QScriptContext *context, QScriptEngine *engine)
{
	return QScriptValue(1);
}

static QScriptValue js_enumStruct(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_VAR(0, 3);
	switch (context->argumentCount())
	{
	default:
	case 3: ARG_PLAYER(2); // fall-through
	case 2: // fall-through
	case 1: ARG_PLAYER(0); break;
	}
	QScriptValue result = engine->newArray(3);
	for (int i = 0; i < 3; i++)
	{
		result.setProperty(i, convStructure(engine));
	}
	return result;
}

static QScriptValue js_enumFeature(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue result = engine->newArray(3);
	for (int i = 0; i < 3; i++)
	{
		result.setProperty(i, convFeature(engine));
	}
	return result;
}

static QScriptValue js_pickStructLocation(QScriptContext *context, QScriptEngine *engine)
{
	ARG_DROID(0);
	return QScriptValue(true);
}

static QScriptValue js_enumStructOffWorld(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_VAR(0, 3);
	switch (context->argumentCount())
	{
	default:
	case 3: ARG_PLAYER(2); // fall-through
	case 2: // fall-through
	case 1: ARG_PLAYER(0); break;
	}
	QScriptValue result = engine->newArray(3);
	for (int i = 0; i < 3; i++)
	{
		result.setProperty(i, convStructure(engine));
	}
	return result;
}

static QScriptValue js_enumDroid(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_VAR(0, 3);
	switch (context->argumentCount())
	{
	default:
	case 3: ARG_PLAYER(2); // fall-through
	case 2: ARG_NUMBER(1); // fall-through
	case 1: ARG_PLAYER(0); break;
	}
	QScriptValue result = engine->newArray(3);
	for (int i = 0; i < 3; i++)
	{
		result.setProperty(i, convDroid(engine));
	}
	return result;
}

static QScriptValue js_activateStructure(QScriptContext *context, QScriptEngine *)
{
	ARG_COUNT_VAR(1, 3);
	ARG_STRUCT(0);
	return QScriptValue();
}

static QScriptValue js_pursueResearch(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(2);
	ARG_STRUCT(0);
	return QScriptValue(true);
}

static QScriptValue js_getResearch(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(1);
	return convResearch(engine);
}

static QScriptValue js_enumResearch(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue result = engine->newArray(1);
	result.setProperty(0, convResearch(engine));
	return result;
}

static QScriptValue js_componentAvailable(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(2);
	return QScriptValue(true);
}

static QScriptValue js_addDroid(QScriptContext *context, QScriptEngine *engine)
{
	ARG_PLAYER(0);
	return QScriptValue(true);
}

static QScriptValue js_addFeature(QScriptContext *context, QScriptEngine *engine)
{
	ARG_STRING(2);
	return QScriptValue(convFeature(engine));
}

static QScriptValue js_debug(QScriptContext *context, QScriptEngine *engine)
{
	return QScriptValue();
}

static QScriptValue js_structureIdle(QScriptContext *context, QScriptEngine *)
{
	ARG_COUNT_EXACT(1);
	ARG_STRUCT(0);
	return QScriptValue(true);
}

static QScriptValue js_removeStruct(QScriptContext *context, QScriptEngine *)
{
	ARG_COUNT_EXACT(1);
	ARG_STRUCT(0);
	fprintf(stderr, "removeStruct() is deprecated since version 3.2");
	return QScriptValue(true);
}

static QScriptValue js_removeObject(QScriptContext *context, QScriptEngine *)
{
	ARG_COUNT_VAR(1, 2);
	ARG_OBJ(0);
	if (context->argumentCount() > 1)
	{
		ARG_BOOL(1);
	}
	return QScriptValue(true);
}

static QScriptValue js_console(QScriptContext *context, QScriptEngine *engine)
{
	return QScriptValue();
}

static QScriptValue js_buildDroid(QScriptContext *context, QScriptEngine *)
{
	ARG_STRUCT(0);
	ARG_STRING(1);
	return QScriptValue(true);
}

static QScriptValue js_groupAddArea(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(5);
	ARG_NUMBER(0);
	ARG_NUMBER(1);
	ARG_NUMBER(2);
	ARG_NUMBER(3);
	ARG_NUMBER(4);
	return QScriptValue();
}

static QScriptValue js_groupAddDroid(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(2);
	ARG_NUMBER(0);
	ARG_DROID(1);
	return QScriptValue();
}

static QScriptValue js_distBetweenTwoPoints(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(4);
	ARG_NUMBER(0);
	ARG_NUMBER(1);
	ARG_NUMBER(2);
	ARG_NUMBER(3);
	return QScriptValue(10);
}

static QScriptValue js_groupSize(QScriptContext *context, QScriptEngine *)
{
	ARG_COUNT_EXACT(1);
	ARG_NUMBER(0);
	return QScriptValue(3);
}

static QScriptValue js_droidCanReach(QScriptContext *context, QScriptEngine *)
{
	ARG_COUNT_EXACT(3);
	ARG_DROID(0);
	ARG_NUMBER(1);
	ARG_NUMBER(2);
	return QScriptValue(true);
}

static QScriptValue js_orderDroid(QScriptContext *context, QScriptEngine *)
{
	ARG_COUNT_EXACT(2);
	ARG_DROID(0);
	ARG_ORDER(1);
	return QScriptValue(true);
}

static QScriptValue js_orderDroidObj(QScriptContext *context, QScriptEngine *)
{
	ARG_COUNT_EXACT(3);
	ARG_DROID(0);
	ARG_ORDER(1);
	ARG_OBJ(2);
	return QScriptValue(true);
}

static QScriptValue js_orderDroidBuild(QScriptContext *context, QScriptEngine *)
{
	ARG_COUNT_EXACT(5);
	ARG_DROID(0);
	ARG_ORDER(1);
	ARG_NUMBER(2);
	ARG_NUMBER(3);
	ARG_NUMBER(4);
	return QScriptValue(true);
}

static QScriptValue js_orderDroidLoc(QScriptContext *context, QScriptEngine *)
{
	ARG_COUNT_EXACT(4);
	ARG_DROID(0);
	ARG_ORDER(1);
	ARG_NUMBER(2);
	ARG_NUMBER(3);
	return QScriptValue(true);
}

static QScriptValue js_setMissionTime(QScriptContext *context, QScriptEngine *)
{
	ARG_COUNT_EXACT(1);
	ARG_NUMBER(0);
	return QScriptValue();
}

static QScriptValue js_setReinforcementTime(QScriptContext *context, QScriptEngine *)
{
	ARG_COUNT_EXACT(1);
	ARG_NUMBER(0);
	return QScriptValue();
}

static QScriptValue js_setStructureLimits(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_VAR(2, 3);
	ARG_STRING(0);
	ARG_NUMBER(1);
	if (context->argumentCount() > 2)
	{
		ARG_PLAYER(2);
	}
	return QScriptValue();
}

static QScriptValue js_centreView(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(2);
	ARG_NUMBER(0);
	ARG_NUMBER(1);
	return QScriptValue();
}

static QScriptValue js_playSound(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_VAR(1, 4);
	ARG_STRING(0);
	if (context->argumentCount() != 1)
	{
		SCRIPT_ASSERT(context, context->argumentCount() == 4, "Arguments must be either 1 or 4");
		ARG_NUMBER(1);
		ARG_NUMBER(2);
		ARG_NUMBER(3);
	}
	return QScriptValue();
}

static QScriptValue js_gameOverMessage(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(1);
	ARG_BOOL(0);
	return QScriptValue();
}

static QScriptValue js_completeResearch(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_VAR(1, 2);
	ARG_STRING(0);
	if (context->argumentCount() > 1)
	{
		ARG_PLAYER(1);
	}
	return QScriptValue();
}

static QScriptValue js_enableResearch(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_VAR(1, 2);
	ARG_STRING(0);
	if (context->argumentCount() > 1)
	{
		ARG_PLAYER(1);
	}
	return QScriptValue();
}

static QScriptValue js_setPower(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_VAR(1, 2);
	ARG_NUMBER(0);
	if (context->argumentCount() > 1)
	{
		ARG_PLAYER(1);
	}
	return QScriptValue();
}

static QScriptValue js_enableStructure(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_VAR(1, 2);
	ARG_STRING(0);
	if (context->argumentCount() > 1)
	{
		ARG_PLAYER(1);
	}
	return QScriptValue();
}

static QScriptValue js_setTutorialMode(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(1);
	ARG_BOOL(0);
	return QScriptValue();
}

static QScriptValue js_setMiniMap(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(1);
	ARG_BOOL(0);
	return QScriptValue();
}

static QScriptValue js_setDesign(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(1);
	ARG_BOOL(0);
	return QScriptValue();
}

static QScriptValue js_addReticuleButton(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(1);
	ARG_NUMBER(0);
	return QScriptValue();
}

static QScriptValue js_removeReticuleButton(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(1);
	ARG_NUMBER(0);
	return QScriptValue();
}

static QScriptValue js_applyLimitSet(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(0);
	return QScriptValue();
}

static QScriptValue js_enableTemplate(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(1);
	ARG_STRING(0);
	return QScriptValue();
}

static QScriptValue js_enableComponent(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(2);
	ARG_STRING(0);
	ARG_PLAYER(1);
	return QScriptValue();
}

static QScriptValue js_makeComponentAvailable(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(2);
	ARG_STRING(0);
	ARG_PLAYER(1);
	return QScriptValue();
}

static QScriptValue js_allianceExistsBetween(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(2);
	ARG_PLAYER(0);
	ARG_PLAYER(1);
	return QScriptValue(false);
}

static QScriptValue js_objFromId(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(1);
	ARG_NUMBER(0);
	return QScriptValue(convDroid(engine));
}

static QScriptValue js_setDroidExperience(QScriptContext *context, QScriptEngine *)
{
	ARG_COUNT_EXACT(2);
	ARG_DROID(0);
	ARG_NUMBER(1);
	return QScriptValue();
}

static QScriptValue js_safeDest(QScriptContext *context, QScriptEngine *)
{
	ARG_COUNT_EXACT(3);
	ARG_NUMBER(0);
	ARG_NUMBER(1);
	ARG_NUMBER(2);
	return QScriptValue(true);
}

static QScriptValue js_addStructure(QScriptContext *context, QScriptEngine *)
{
	ARG_COUNT_EXACT(4);
	ARG_NUMBER(0);
	ARG_PLAYER(1);
	ARG_NUMBER(2);
	ARG_NUMBER(3);
	return QScriptValue(true);
}

static QScriptValue js_setNoGoArea(QScriptContext *context, QScriptEngine *)
{
	ARG_COUNT_EXACT(5);
	ARG_NUMBER(0);
	ARG_NUMBER(1);
	ARG_NUMBER(2);
	ARG_NUMBER(3);
	ARG_PLAYER(4);
	return QScriptValue(true);
}

static QScriptValue js_setScrollParams(QScriptContext *context, QScriptEngine *)
{
	ARG_COUNT_EXACT(4);
	ARG_NUMBER(0);
	ARG_NUMBER(1);
	ARG_NUMBER(2);
	ARG_NUMBER(3);
	return QScriptValue();
}

static QScriptValue js_loadLevel(QScriptContext *context, QScriptEngine *)
{
	ARG_COUNT_EXACT(1);
	ARG_STRING(0);
	return QScriptValue();
}

static QScriptValue js_chat(QScriptContext *context, QScriptEngine *)
{
	ARG_COUNT_EXACT(2);
	ARG_PLAYER(0);
	ARG_STRING(1);
	return QScriptValue(true);
}

static QScriptValue js_hackNetOff(QScriptContext *context, QScriptEngine *)
{
	return QScriptValue();
}

static QScriptValue js_hackNetOn(QScriptContext *context, QScriptEngine *)
{
	return QScriptValue();
}

static QScriptValue js_removeTimer(QScriptContext *context, QScriptEngine *)
{
	ARG_COUNT_EXACT(1);
	ARG_STRING(0);
	QString function = context->argument(0).toString();
	int i, size = timers.size();
	for (i = 0; i < size; ++i)
	{
		timerNode node = timers.at(i);
		if (node.function == function)
		{
			return QScriptValue();
		}
	}
	QString warnName = function.left(15) + "...";
	SCRIPT_ASSERT(context, false, "Did not find timer %s to remove", warnName.toUtf8().constData());
	return QScriptValue();
}

/// Special scripting function that registers a non-specific timer event. Note: Functions must be passed
/// quoted, otherwise they will be inlined!
static QScriptValue js_setTimer(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_VAR(2, 3);
	ARG_STRING(0);
	ARG_NUMBER(1);
	if (context->argumentCount() == 3) ARG_OBJ(2);
	QString funcName = context->argument(0).toString();
	// TODO - check that a function by that name exists
	// TODO - object argument
	int player = engine->globalObject().property("me").toInt32();
	timerNode node(engine, funcName, player);
	timers.push_back(node);
	return QScriptValue();
}

static QScriptValue js_bind(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_VAR(2, 3);
	ARG_STRING(0);
	ARG_OBJ(1);
	// TBD implement it for testing
	return QScriptValue();
}

/// Special scripting function that registers a object-specific timer event. Note: Functions must be passed
/// quoted, otherwise they will be inlined!
static QScriptValue js_queue(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_VAR(1, 3);
	ARG_STRING(0);
	if (context->argumentCount() > 1) ARG_NUMBER(1);
	if (context->argumentCount() == 3) ARG_OBJ(2);
	QString funcName = context->argument(0).toString();
	// TODO - check that a function by that name exists
	// TODO - object argument
	int player = engine->globalObject().property("me").toInt32();
	timerNode node(engine, funcName, player);
	timers.push_back(node);
	return QScriptValue();
}

static QScriptValue js_include(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(1);
	ARG_STRING(0);
	// TODO -- implement this somehow -- not sure how to handle paths here
#if 0
	QString path = context->argument(0).toString();
	UDWORD size;
	char *bytes = NULL;
	if (!loadFile(path.toAscii().constData(), &bytes, &size))
	{
		debug(LOG_ERROR, "Failed to read include file \"%s\"", path.toAscii().constData());
		return false;
	}
	QString source = QString::fromAscii(bytes, size);
	free(bytes);
	QScriptSyntaxCheckResult syntax = QScriptEngine::checkSyntax(source);
	if (syntax.state() != QScriptSyntaxCheckResult::Valid)
	{
		debug(LOG_ERROR, "Syntax error in include %s line %d: %s", 
		      path.toAscii().constData(), syntax.errorLineNumber(), syntax.errorMessage().toAscii().constData());
		return false;
	}
	context->setActivationObject(engine->globalObject());
	context->setThisObject(engine->globalObject());
	QScriptValue result = engine->evaluate(source, path);
	if (engine->hasUncaughtException())
	{
		int line = engine->uncaughtExceptionLineNumber();
		debug(LOG_ERROR, "Uncaught exception at line %d, include file %s: %s",
		      line, path.toAscii().constData(), result.toString().toAscii().constData());
		return false;
	}
#endif
	return QScriptValue();
}

static QScriptValue js_translate(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(1);
	ARG_STRING(0);
	return QScriptValue(context->argument(0));
}

static QScriptValue js_playerPower(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(1);
	ARG_PLAYER(0);
	return QScriptValue(1000);
}

static QScriptValue js_isStructureAvailable(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_VAR(1, 2);
	ARG_STRING(0);
	if (context->argumentCount() > 1) ARG_PLAYER(1);
	return QScriptValue(true);
}

static QScriptValue js_isVTOL(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(1);
	ARG_DROID(0);
	return QScriptValue(true);
}

static QScriptValue js_setAlliance(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(3);
	ARG_PLAYER(0);
	ARG_PLAYER(1);
	ARG_BOOL(2);
	return QScriptValue(true);
}

static QScriptValue js_setAssemblyPoint(QScriptContext *context, QScriptEngine *engine)
{
	ARG_COUNT_EXACT(3);
	ARG_STRUCT(0);
	ARG_NUMBER(1);
	ARG_NUMBER(2);
	return QScriptValue(true);
}

bool testPlayerScript(QString path, int player, int difficulty)
{
	QScriptEngine *engine = new QScriptEngine();
	QFile input(path);
	input.open(QIODevice::ReadOnly);
	QString source(QString::fromUtf8(input.readAll()));
	input.close();

	// Special functions
	engine->globalObject().setProperty("setTimer", engine->newFunction(js_setTimer));
	engine->globalObject().setProperty("queue", engine->newFunction(js_queue));
	engine->globalObject().setProperty("removeTimer", engine->newFunction(js_removeTimer));
	engine->globalObject().setProperty("include", engine->newFunction(js_include));
	engine->globalObject().setProperty("bind", engine->newFunction(js_bind));

	// Special global variables
	engine->globalObject().setProperty("version", 1, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("selectedPlayer", 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("gameTime", 2, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("difficulty", 1, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("mapName", "TEST", QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("baseType", 1, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("alliancesType", 2, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("powerType", 1, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("maxPlayers", CUR_PLAYERS, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("scavengers", true, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("mapWidth", 80, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("mapHeight", 80, QScriptValue::ReadOnly | QScriptValue::Undeletable);

	// Most special global
	engine->globalObject().setProperty("me", player, QScriptValue::ReadOnly | QScriptValue::Undeletable);

	// Register functions to the script engine here
	engine->globalObject().setProperty("_", engine->newFunction(js_translate));
	engine->globalObject().setProperty("label", engine->newFunction(js_label));
	engine->globalObject().setProperty("enumLabels", engine->newFunction(js_enumLabels));
	engine->globalObject().setProperty("enumGateways", engine->newFunction(js_enumGateways));

	// horrible hacks follow -- do not rely on these being present!
	engine->globalObject().setProperty("hackNetOff", engine->newFunction(js_hackNetOff));
	engine->globalObject().setProperty("hackNetOn", engine->newFunction(js_hackNetOn));
	engine->globalObject().setProperty("objFromId", engine->newFunction(js_objFromId));

	// General functions -- geared for use in AI scripts
	engine->globalObject().setProperty("debug", engine->newFunction(js_debug));
	engine->globalObject().setProperty("console", engine->newFunction(js_console));
	engine->globalObject().setProperty("structureIdle", engine->newFunction(js_structureIdle));
	engine->globalObject().setProperty("enumStruct", engine->newFunction(js_enumStruct));
	engine->globalObject().setProperty("enumStructOffWorld", engine->newFunction(js_enumStructOffWorld));
	engine->globalObject().setProperty("enumDroid", engine->newFunction(js_enumDroid));
	engine->globalObject().setProperty("enumGroup", engine->newFunction(js_enumGroup));
	engine->globalObject().setProperty("enumFeature", engine->newFunction(js_enumFeature));
	engine->globalObject().setProperty("enumBlips", engine->newFunction(js_enumBlips));
	engine->globalObject().setProperty("enumResearch", engine->newFunction(js_enumResearch));
	engine->globalObject().setProperty("getResearch", engine->newFunction(js_getResearch));
	engine->globalObject().setProperty("pursueResearch", engine->newFunction(js_pursueResearch));
	engine->globalObject().setProperty("distBetweenTwoPoints", engine->newFunction(js_distBetweenTwoPoints));
	engine->globalObject().setProperty("newGroup", engine->newFunction(js_newGroup));
	engine->globalObject().setProperty("groupAddArea", engine->newFunction(js_groupAddArea));
	engine->globalObject().setProperty("groupAddDroid", engine->newFunction(js_groupAddDroid));
	engine->globalObject().setProperty("groupSize", engine->newFunction(js_groupSize));
	engine->globalObject().setProperty("orderDroidLoc", engine->newFunction(js_orderDroidLoc));
	engine->globalObject().setProperty("playerPower", engine->newFunction(js_playerPower));
	engine->globalObject().setProperty("isStructureAvailable", engine->newFunction(js_isStructureAvailable));
	engine->globalObject().setProperty("pickStructLocation", engine->newFunction(js_pickStructLocation));
	engine->globalObject().setProperty("droidCanReach", engine->newFunction(js_droidCanReach));
	engine->globalObject().setProperty("orderDroidStatsLoc", engine->newFunction(js_orderDroidBuild)); // deprecated
	engine->globalObject().setProperty("orderDroidBuild", engine->newFunction(js_orderDroidBuild));
	engine->globalObject().setProperty("orderDroidObj", engine->newFunction(js_orderDroidObj));
	engine->globalObject().setProperty("orderDroid", engine->newFunction(js_orderDroid));
	engine->globalObject().setProperty("buildDroid", engine->newFunction(js_buildDroid));
	engine->globalObject().setProperty("addDroid", engine->newFunction(js_addDroid));
	engine->globalObject().setProperty("addFeature", engine->newFunction(js_addFeature));
	engine->globalObject().setProperty("componentAvailable", engine->newFunction(js_componentAvailable));
	engine->globalObject().setProperty("isVTOL", engine->newFunction(js_isVTOL));
	engine->globalObject().setProperty("safeDest", engine->newFunction(js_safeDest));
	engine->globalObject().setProperty("activateStructure", engine->newFunction(js_activateStructure));
	engine->globalObject().setProperty("chat", engine->newFunction(js_chat));

	// Functions that operate on the current player only
	engine->globalObject().setProperty("centreView", engine->newFunction(js_centreView));
	engine->globalObject().setProperty("playSound", engine->newFunction(js_playSound));
	engine->globalObject().setProperty("gameOverMessage", engine->newFunction(js_gameOverMessage));

	// Global state manipulation -- not for use with skirmish AI (unless you want it to cheat, obviously)
	engine->globalObject().setProperty("setStructureLimits", engine->newFunction(js_setStructureLimits));
	engine->globalObject().setProperty("applyLimitSet", engine->newFunction(js_applyLimitSet));
	engine->globalObject().setProperty("setMissionTime", engine->newFunction(js_setMissionTime));
	engine->globalObject().setProperty("setReinforcementTime", engine->newFunction(js_setReinforcementTime));
	engine->globalObject().setProperty("completeResearch", engine->newFunction(js_completeResearch));
	engine->globalObject().setProperty("enableResearch", engine->newFunction(js_enableResearch));
	engine->globalObject().setProperty("setPower", engine->newFunction(js_setPower));
	engine->globalObject().setProperty("setTutorialMode", engine->newFunction(js_setTutorialMode));
	engine->globalObject().setProperty("setDesign", engine->newFunction(js_setDesign));
	engine->globalObject().setProperty("setMiniMap", engine->newFunction(js_setMiniMap));
	engine->globalObject().setProperty("addReticuleButton", engine->newFunction(js_addReticuleButton));
	engine->globalObject().setProperty("removeReticuleButton", engine->newFunction(js_removeReticuleButton));
	engine->globalObject().setProperty("enableStructure", engine->newFunction(js_enableStructure));
	engine->globalObject().setProperty("makeComponentAvailable", engine->newFunction(js_makeComponentAvailable));
	engine->globalObject().setProperty("enableComponent", engine->newFunction(js_enableComponent));
	engine->globalObject().setProperty("enableTemplate", engine->newFunction(js_enableTemplate));
	engine->globalObject().setProperty("allianceExistsBetween", engine->newFunction(js_allianceExistsBetween));
	engine->globalObject().setProperty("removeStruct", engine->newFunction(js_removeStruct));
	engine->globalObject().setProperty("removeObject", engine->newFunction(js_removeObject));
	engine->globalObject().setProperty("setScrollParams", engine->newFunction(js_setScrollParams));
	engine->globalObject().setProperty("addStructure", engine->newFunction(js_addStructure));
	engine->globalObject().setProperty("loadLevel", engine->newFunction(js_loadLevel));
	engine->globalObject().setProperty("setDroidExperience", engine->newFunction(js_setDroidExperience));
	engine->globalObject().setProperty("setNoGoArea", engine->newFunction(js_setNoGoArea));
	engine->globalObject().setProperty("setAlliance", engine->newFunction(js_setAlliance));
	engine->globalObject().setProperty("setAssemblyPoint", engine->newFunction(js_setAssemblyPoint));

	// Set some useful constants
	engine->globalObject().setProperty("DORDER_ATTACK", 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_MOVE", 1, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_SCOUT", 2, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_BUILD", 2, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_HELPBUILD", 3, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_LINEBUILD", 4, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_REPAIR", 5, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_RETREAT", 6, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_PATROL", 7, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_DEMOLISH", 8, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_EMBARK", 9, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_DISEMBARK", 10, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_FIRESUPPORT", 11, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_HOLD", 12, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_RTR", 13, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_RTB", 14, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_STOP", 15, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_REARM", 16, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("COMMAND", 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("BUILD", 1, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("MANUFACTURE", 2, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("RESEARCH", 3, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("INTELMAP", 4, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DESIGN", 5, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("CANCEL", 6, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("CAMP_CLEAN", 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("CAMP_BASE", 1, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("CAMP_WALLS", 2, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("NO_ALLIANCES", 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("ALLIANCES", 1, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("ALLIANCES_TEAMS", 2, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("BEING_BUILT", SS_BEING_BUILT, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("BUILT", SS_BUILT, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("BEING_DEMOLISHED", SS_BEING_DEMOLISHED, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DROID_CONSTRUCT", 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DROID_WEAPON", 1, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DROID_PERSON", 2, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DROID_REPAIR", 3, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DROID_SENSOR", 4, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DROID_ECM", 5, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DROID_CYBORG", 6, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DROID_TRANSPORTER", 7, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DROID_COMMAND", 8, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("HQ", 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("FACTORY", 1, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("POWER_GEN", 2, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("RESOURCE_EXTRACTOR", 3, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DEFENSE", 4, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("LASSAT", 5, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("WALL", 6, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("RESEARCH_LAB", 7, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("REPAIR_FACILITY", 8, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("CYBORG_FACTORY", 9, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("VTOL_FACTORY", 10, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("REARM_PAD", 11, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("SAT_UPLINK", 12, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("GATE", 13, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("COMMAND_CONTROL", 14, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("EASY", 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("MEDIUM", 1, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("HARD", 2, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("INSANE", 3, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("STRUCTURE", OBJ_STRUCTURE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DROID", OBJ_DROID, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("FEATURE", OBJ_FEATURE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("POSITION", POSITION, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("AREA", AREA, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("ALL_PLAYERS", -1, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("ALLIES", -2, QScriptValue::ReadOnly | QScriptValue::Undeletable);

	QScriptValue playerData = engine->newArray(CUR_PLAYERS);
	for (int i = 0; i < CUR_PLAYERS; i++)
	{
		QScriptValue vector = engine->newObject();
		vector.setProperty("difficulty", 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		vector.setProperty("colour", 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		vector.setProperty("position", i, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		vector.setProperty("team", i, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		vector.setProperty("isAI", i != 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		vector.setProperty("isHuman", i == 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		playerData.setProperty(i, vector, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	}
	engine->globalObject().setProperty("playerData", playerData, QScriptValue::ReadOnly | QScriptValue::Undeletable);

	// Static map knowledge about start positions
	//== \item[derrickPositions] An array of derrick starting positions on the current map. Each item in the array is an
	//== object containing the x and y variables for a derrick.
	//== \item[startPositions] An array of player start positions on the current map. Each item in the array is an
	//== object containing the x and y variables for a player start position.
	QScriptValue startPositions = engine->newArray(CUR_PLAYERS);
	for (int i = 0; i < CUR_PLAYERS; i++)
	{
		QScriptValue vector = engine->newObject();
		vector.setProperty("x", 40, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		vector.setProperty("y", 40, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		startPositions.setProperty(i, vector, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	}
	QScriptValue derrickPositions = engine->newArray(6);
	for (int i = 0; i < 6; i++)
	{
		QScriptValue vector = engine->newObject();
		vector.setProperty("x", 40, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		vector.setProperty("y", 40, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		derrickPositions.setProperty(i, vector, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	}
	engine->globalObject().setProperty("derrickPositions", derrickPositions, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("startPositions", startPositions, QScriptValue::ReadOnly | QScriptValue::Undeletable);

	QScriptSyntaxCheckResult syntax = QScriptEngine::checkSyntax(source);
	if (syntax.state() != QScriptSyntaxCheckResult::Valid)
	{
        qFatal("Syntax error in %s line %d: %s", path.toUtf8().constData(), syntax.errorLineNumber(), syntax.errorMessage().toUtf8().constData());
		return false;
	}
	QScriptValue result = engine->evaluate(source, path);
	if (engine->hasUncaughtException())
	{
		int line = engine->uncaughtExceptionLineNumber();
		qFatal("Uncaught exception at line %d, file %s: %s", line, path.toUtf8().constData(), result.toString().toUtf8().constData());
		return false;
	}

	// Call init
	callFunction(engine, "eventGameInit", QScriptValueList());

	// Now set gameTime to something proper
	engine->globalObject().setProperty("gameTime", 10101, QScriptValue::ReadOnly | QScriptValue::Undeletable);

	callFunction(engine, "eventStartLevel", QScriptValueList());
	callFunction(engine, "eventLaunchTransporter", QScriptValueList());
	callFunction(engine, "eventReinforcementsArrived", QScriptValueList());
	callFunction(engine, "eventMissionTimeout", QScriptValueList());
	callFunction(engine, "eventVideoDone", QScriptValueList());

	// Call other events
	{
		QScriptValueList args;
		args += convDroid(engine);
		args += convStructure(engine);
		callFunction(engine, "eventDroidBuilt", args);
	}
	{
		QScriptValueList args;
		args += convStructure(engine);
		args += convObj(engine);
		callFunction(engine, "eventStructureAttacked", args);
	}
	{
		QScriptValueList args;
		args += convResearch(engine);
		args += convStructure(engine);
		callFunction(engine, "eventResearched", args);
	}
	{
		QScriptValueList args;
		args += convObj(engine);
		args += convObj(engine);
		callFunction(engine, "eventAttacked", args);
	}
	{
		QScriptValueList args;
		args += convStructure(engine);
		args += convDroid(engine);
		callFunction(engine, "eventStructureBuilt", args);
	}
	{
		QScriptValueList args;
		args += convDroid(engine);
		callFunction(engine, "eventDroidIdle", args);
	}
	{
		QScriptValueList args;
		args += QScriptValue(0);
		args += QScriptValue(1);
		args += QScriptValue("message");
		callFunction(engine, "eventChat", args);
	}
	{
		QScriptValueList args;
		args += convObj(engine);
		args += convObj(engine);
		callFunction(engine, "eventObjectSeen", args);
	}

	// Now test timers
	// TODO -- implement object parameters
	for (int i = 0; i < timers.size(); ++i)
	{
		timerNode node = timers.at(i);
		callFunction(node.engine, node.function, QScriptValueList(), true);
	}

	// Clean up
	delete engine;
	timers.clear();
	return true;
}

bool testGlobalScript(QString path)
{
	return testPlayerScript(path, 0, 0);
}
