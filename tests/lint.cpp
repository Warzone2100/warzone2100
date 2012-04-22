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

// TODO -- parse and check that all setTimer, removeTimer and queue calls have quoted first parameters

// TODO -- these should go into a common header
#define CAMP_CLEAN                              0                       // campaign subtypes
#define CAMP_BASE                               1
#define CAMP_WALLS                              2
#define NO_ALLIANCES            0                       //alliance possibilities for games.
#define ALLIANCES                       1
#define ALLIANCES_TEAMS         2                       //locked teams
#define LEV_LOW                                 400                     // how many points to allocate for res levels???
#define LEV_MED                                 700
#define LEV_HI                                  1000

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

static QScriptValue convObj(QScriptEngine *engine)
{
	QScriptValue value = engine->newObject();
	value.setProperty("id", obj_uid++, QScriptValue::ReadOnly);
	value.setProperty("x", 11, QScriptValue::ReadOnly);
	value.setProperty("y", 11, QScriptValue::ReadOnly);
	value.setProperty("z", 0, QScriptValue::ReadOnly);
	value.setProperty("player", 1, QScriptValue::ReadOnly);
	value.setProperty("selected", 0, QScriptValue::ReadOnly);
	return value;
}

// These functions invent new imaginary objects
static QScriptValue convStructure(QScriptEngine *engine)
{
	QScriptValue value = convObj(engine);
	value.setProperty("status", (int)SS_BUILT, QScriptValue::ReadOnly);
	return value;
}

static QScriptValue convDroid(QScriptEngine *engine)
{
	QScriptValue value = convObj(engine);
	value.setProperty("action", 0, QScriptValue::ReadOnly);
	value.setProperty("order", 0, QScriptValue::ReadOnly);
	return value;
}

// Call a function by name
static bool callFunction(QScriptEngine *engine, QString function, QScriptValueList args)
{
	QScriptValue value = engine->globalObject().property(function);
	if (!value.isValid() || !value.isFunction())
	{
		return false;	// not necessarily an error, may just be a trigger that is not defined (ie not needed)
	}
	QScriptValue result = value.call(QScriptValue(), args);
	if (engine->hasUncaughtException())
	{
		int line = engine->uncaughtExceptionLineNumber();
		QStringList bt = engine->uncaughtExceptionBacktrace();
		for (int i = 0; i < bt.size(); i++)
		{
			fprintf(stderr, "%d : %s\n", i, bt.at(i).toAscii().constData());
		}
		fprintf(stderr, "Uncaught exception calling function \"%s\" at line %d: %s\n",
		       function.toAscii().constData(), line, result.toString().toAscii().constData());
		return false;
	}
	return true;
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
	case 2: ARG_STRING(1); // fall-through
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

static QScriptValue js_console(QScriptContext *context, QScriptEngine *engine)
{
	return QScriptValue();
}

/* Build a droid template in the specified factory */
static QScriptValue js_buildDroid(QScriptContext *context, QScriptEngine *)
{
	ARG_COUNT_EXACT(2);
	ARG_STRING(0);
	ARG_STRUCT(1);
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

static QScriptValue js_orderDroidLoc(QScriptContext *context, QScriptEngine *)
{
	ARG_COUNT_EXACT(4);
	ARG_DROID(0);
	ARG_NUMBER(1);
	ARG_NUMBER(2);
	ARG_NUMBER(3);
	return QScriptValue();
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

static QScriptValue js_addReticuleButton(QScriptContext *context, QScriptEngine *engine)
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

static QScriptValue js_removeTimer(QScriptContext *context, QScriptEngine *engine)
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
	SCRIPT_ASSERT(context, false, "Did not find timer %s to remove", warnName.toAscii().constData());
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
	ARG_COUNT_EXACT(2);
	ARG_STRING(0);
	ARG_PLAYER(1);
	return QScriptValue(true);
}

bool testPlayerScript(QString path, int player, int difficulty)
{
	QScriptEngine *engine = new QScriptEngine();
	QFile input(path);
	input.open(QIODevice::ReadOnly);
	QString source(QString::fromUtf8(input.readAll()));
	input.close();
	QScriptSyntaxCheckResult syntax = QScriptEngine::checkSyntax(source);
	if (syntax.state() != QScriptSyntaxCheckResult::Valid)
	{
		fprintf(stderr, "Syntax error in %s line %d: %s\n", path.toAscii().constData(), syntax.errorLineNumber(), syntax.errorMessage().toAscii().constData());
		return false;
	}
	QScriptValue result = engine->evaluate(source, path);
	if (engine->hasUncaughtException())
	{
		int line = engine->uncaughtExceptionLineNumber();
		fprintf(stderr, "Uncaught exception at line %d, file %s: %s\n", line, path.toAscii().constData(), result.toString().toAscii().constData());
		return false;
	}
	// Special functions
	engine->globalObject().setProperty("setTimer", engine->newFunction(js_setTimer));
	engine->globalObject().setProperty("queue", engine->newFunction(js_queue));
	engine->globalObject().setProperty("removeTimer", engine->newFunction(js_removeTimer));
	engine->globalObject().setProperty("include", engine->newFunction(js_include));

	// Special global variables
	engine->globalObject().setProperty("me", player, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("gameTime", 2, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("difficulty", difficulty, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("mapName", "Test", QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("baseType", CAMP_BASE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("alliancesType", ALLIANCES_TEAMS, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("powerType", LEV_MED, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("maxPlayers", 4, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("scavengers", true, QScriptValue::ReadOnly | QScriptValue::Undeletable);

	engine->globalObject().setProperty("_", engine->newFunction(js_translate));

	// General functions -- geared for use in AI scripts
	engine->globalObject().setProperty("debug", engine->newFunction(js_debug));
	engine->globalObject().setProperty("console", engine->newFunction(js_console));
	engine->globalObject().setProperty("structureIdle", engine->newFunction(js_structureIdle));
	engine->globalObject().setProperty("buildDroid", engine->newFunction(js_buildDroid));
	engine->globalObject().setProperty("enumStruct", engine->newFunction(js_enumStruct));
	engine->globalObject().setProperty("enumDroid", engine->newFunction(js_enumDroid));
	engine->globalObject().setProperty("enumGroup", engine->newFunction(js_enumGroup));
	engine->globalObject().setProperty("distBetweenTwoPoints", engine->newFunction(js_distBetweenTwoPoints));
	engine->globalObject().setProperty("newGroup", engine->newFunction(js_newGroup));
	engine->globalObject().setProperty("groupAddArea", engine->newFunction(js_groupAddArea));
	engine->globalObject().setProperty("groupAddDroid", engine->newFunction(js_groupAddDroid));
	engine->globalObject().setProperty("groupSize", engine->newFunction(js_groupSize));
	engine->globalObject().setProperty("orderDroidLoc", engine->newFunction(js_orderDroidLoc));
	engine->globalObject().setProperty("playerPower", engine->newFunction(js_playerPower));
	engine->globalObject().setProperty("isStructureAvailable", engine->newFunction(js_isStructureAvailable));

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
	engine->globalObject().setProperty("addReticuleButton", engine->newFunction(js_addReticuleButton));
	engine->globalObject().setProperty("enableStructure", engine->newFunction(js_enableStructure));
	engine->globalObject().setProperty("makeComponentAvailable", engine->newFunction(js_makeComponentAvailable));
	engine->globalObject().setProperty("enableComponent", engine->newFunction(js_enableComponent));
	engine->globalObject().setProperty("allianceExistsBetween", engine->newFunction(js_allianceExistsBetween));

	// Set some useful constants
	engine->globalObject().setProperty("DORDER_ATTACK", 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_MOVE", 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_SCOUT", 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_BUILD", 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("mapWidth", 64, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("mapHeight", 64, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("COMMAND", 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("OPTIONS", 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("BUILD", 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("MANUFACTURE", 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("RESEARCH", 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("INTELMAP", 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DESIGN", 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("CANCEL", 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("CAMP_CLEAN", CAMP_CLEAN, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("CAMP_BASE", CAMP_BASE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("CAMP_WALLS", CAMP_WALLS, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("NO_ALLIANCES", NO_ALLIANCES, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("ALLIANCES", ALLIANCES, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("ALLIANCES_TEAMS", ALLIANCES_TEAMS, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("BEING_BUILT", SS_BEING_BUILT, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("BUILT", SS_BUILT, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("BEING_DEMOLISHED", SS_BEING_DEMOLISHED, QScriptValue::ReadOnly | QScriptValue::Undeletable);

	// Call init
	callFunction(engine, "eventGameInit", QScriptValueList());

	// Now set gameTime to something proper
	engine->globalObject().setProperty("gameTime", 10101, QScriptValue::ReadOnly | QScriptValue::Undeletable);

	callFunction(engine, "eventStartLevel", QScriptValueList());

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

	// Now test timers
	// TODO -- implement object parameters
	QMutableListIterator<timerNode> iter(timers);
	while (iter.hasNext())
	{
		timerNode node = iter.next();
		callFunction(node.engine, node.function, QScriptValueList());
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
