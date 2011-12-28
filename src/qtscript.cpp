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
 * @file qtscript.cpp
 *
 * New scripting system
 */

#include "qtscript.h"

#include <QtCore/QTextStream>
#include <QtCore/QHash>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>
#include <QtScript/QScriptValueIterator>
#include <QtScript/QScriptSyntaxCheckResult>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include "lib/framework/wzapp.h"
#include "lib/framework/wzconfig.h"
#include "lib/framework/file.h"
#include "lib/gamelib/gtime.h"
#include "multiplay.h"

#include "qtscriptfuncs.h"

enum timerType
{
	TIMER_REPEAT, TIMER_ONESHOT_READY, TIMER_ONESHOT_DONE
};

struct timerNode
{
	QString function;
	QScriptEngine *engine;
	int baseobj;
	int frameTime;
	int ms;
	int player;
	timerType type;
	timerNode() {}
	timerNode(QScriptEngine *caller, QString val, int plr, int frame)
		: function(val), engine(caller), baseobj(-1), frameTime(frame + gameTime), ms(frame), player(plr), type(TIMER_REPEAT) {}
	bool operator== (const timerNode &t) { return function == t.function && player == t.player; }
	// implement operator less TODO
};

struct bindNode
{
	QString funcName;
	int type;
	int id;
	int player;
	QScriptEngine *engine;
};

/// Bind functions to primary object functions.
static QHash<int, bindNode> bindings;

/// List of timer events for scripts. Before running them, we sort the list then run as many as we have time for.
/// In this way, we implement load balancing of events and keep frame rates tidy for users. Since scripts run on the
/// host, we do not need to worry about each peer simulating the world differently.
static QList<timerNode> timers;

/// Scripting engine (what others call the scripting context, but QtScript's nomenclature is different).
static QList<QScriptEngine *> scripts;

/// Remember what names are used internally in the scripting engine, we don't want to save these to the savegame
static QHash<QString, int> internalNamespace;

// Call a function by name
static bool callFunction(QScriptEngine *engine, const QString &function, const QScriptValueList &args)
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
			debug(LOG_ERROR, "%d : %s", i, bt.at(i).toAscii().constData());
		}
		ASSERT(false, "Uncaught exception calling function \"%s\" at line %d: %s",
		       function.toAscii().constData(), line, result.toString().toAscii().constData());
		return false;
	}
	return true;
}

static QScriptValue js_removeTimer(QScriptContext *context, QScriptEngine *engine)
{
	QString function = context->argument(0).toString();
	int i, size = timers.size();
	for (i = 0; i < size; ++i)
	{
		timerNode node = timers.at(i);
		if (node.function == function)
		{
			timers.removeAt(i);
			break;
		}
	}
	if (i == size)
	{
		// Friendly warning
		QString warnName = function.left(15) + "...";
		debug(LOG_ERROR, "Did not find timer %s to remove", warnName.toAscii().constData());
	}
	return QScriptValue();
}

/// Special scripting function that registers a timer event. Note: Functions must be passed
/// quoted, otherwise they will be inlined! If a third parameter is passed, this must be an
/// object, which is then passed to the timer. If the object is dead, the timer stops.
static QScriptValue js_setTimer(QScriptContext *context, QScriptEngine *engine)
{
	QString funcName = context->argument(0).toString();
	QScriptValue ms = context->argument(1);
	int player = engine->globalObject().property("me").toInt32();
	timerNode node(engine, funcName, player, ms.toInt32());
	if (context->argumentCount() == 3)
	{
		QScriptValue obj = context->argument(2);
		node.baseobj = obj.property("id").toInt32();
	}
	node.type = TIMER_REPEAT;
	timers.push_back(node);
	return QScriptValue();
}

/// Special scripting function that queues up a function to call once at a later time frame.
/// Note: Functions must be passed quoted, otherwise they will be inlined! If a third
/// parameter is passed, this must be an object, which is then passed to the timer. If
/// the object is dead, the timer stops.
static QScriptValue js_queue(QScriptContext *context, QScriptEngine *engine)
{
	QString funcName = context->argument(0).toString();
	int ms = 0;
	if (context->argumentCount() > 1)
	{
		ms = context->argument(1).toInt32();
	}
	int player = engine->globalObject().property("me").toInt32();
	timerNode node(engine, funcName, player, ms);
	if (context->argumentCount() == 3)
	{
		QScriptValue obj = context->argument(2);
		node.baseobj = obj.property("id").toInt32();
	}
	node.type = TIMER_ONESHOT_READY;
	timers.push_back(node);
	return QScriptValue();
}

void scriptRemoveObject(const BASE_OBJECT *psObj)
{
	QHash<int, bindNode>::iterator i = bindings.find(psObj->id);
	while (i != bindings.end() && i.key() == psObj->id)
	{
		int id = i.key();
		bindNode node = i.value();
		BASE_OBJECT *psObj = IdToPointer(id, node.player);
		if (psObj && !psObj->died)
		{
			QScriptValueList args;
			args += convMax(psObj, node.engine);
			callFunction(node.engine, node.funcName, args);
		}
		i = bindings.erase(i);
	}
}

/// Special scripting function that binds a function call to an object. The function
/// is called before the object is destroyed.
/// Note: Functions must be passed quoted, otherwise they will be inlined! If a third
/// parameter is passed, this must be a player, which may be used for filtering.
static QScriptValue js_bind(QScriptContext *context, QScriptEngine *engine)
{
	bindNode node;
	QScriptValue objv = context->argument(1);
	node.type = objv.property("type").toInt32();
	node.player = -1;
	node.funcName = context->argument(0).toString();
	node.engine = engine;
	node.player = objv.property("player").toInt32();
	node.id = objv.property("id").toInt32();
	if (node.type == OBJ_DROID || node.type == OBJ_STRUCTURE || node.type == OBJ_FEATURE)
	{
		BASE_OBJECT *psObj = IdToPointer(node.id, node.player);
		if (psObj && !psObj->died)
		{
			bindings.insert(node.id, node);
		}
	}
	else
	{
		debug(LOG_ERROR, "Trying to bind to illegal object type");
	}
	return QScriptValue();
}

// Currently breaks the lint test, so use with care
static QScriptValue js_include(QScriptContext *context, QScriptEngine *engine)
{
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
	return QScriptValue();
}

bool initScripts()
{
	return true;
}

bool shutdownScripts()
{
	timers.clear();
	bindings.clear();
	internalNamespace.clear();
	while (!scripts.isEmpty())
	{
		delete scripts.takeFirst();
	}
	return true;
}

bool updateScripts()
{
	// Update gameTime
	for (int i = 0; i < scripts.size(); ++i)
	{
		QScriptEngine *engine = scripts.at(i);

		engine->globalObject().setProperty("gameTime", gameTime, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	}
	// Weed out dead timers
	for (int i = 0; i < timers.count(); )
	{
		const timerNode node = timers.at(i);
		if (node.type == TIMER_ONESHOT_DONE || (node.baseobj > 0 && !IdToPointer(node.baseobj, node.player)))
		{
			timers.removeAt(i);
		}
		else
		{
			i++;
		}
	}
	// Check for timers, and run them if applicable.
	// TODO - load balancing
	QList<timerNode> runlist; // make a new list here, since we might trample all over the timer list during execution
	QList<timerNode>::iterator iter;
	// Weed out dead timers
	for (iter = timers.begin(); iter != timers.end(); iter++)
	{
		if (iter->frameTime <= gameTime)
		{
			iter->frameTime = iter->ms + gameTime;	// update for next invokation
			if (iter->type == TIMER_ONESHOT_READY)
			{
				iter->type = TIMER_ONESHOT_DONE; // unless there is none
			}
			runlist.append(*iter);
		}
	}
	for (iter = runlist.begin(); iter != runlist.end(); iter++)
	{
		QScriptValueList args;
		if (iter->baseobj > 0)
		{
			args += convObj(IdToPointer(iter->baseobj, iter->player), iter->engine);
		}
		callFunction(iter->engine, iter->function, args);
	}
	return true;
}

bool loadPlayerScript(QString path, int player, int difficulty)
{
	ASSERT_OR_RETURN(false, player < MAX_PLAYERS, "Player index %d out of bounds", player);
	QScriptEngine *engine = new QScriptEngine();
	UDWORD size;
	char *bytes = NULL;
	if (!loadFile(path.toAscii().constData(), &bytes, &size))
	{
		debug(LOG_ERROR, "Failed to read script file \"%s\"", path.toAscii().constData());
		return false;
	}
	QString source = QString::fromAscii(bytes, size);
	free(bytes);
	QScriptSyntaxCheckResult syntax = QScriptEngine::checkSyntax(source);
	ASSERT_OR_RETURN(false, syntax.state() == QScriptSyntaxCheckResult::Valid, "Syntax error in %s line %d: %s", 
	                 path.toAscii().constData(), syntax.errorLineNumber(), syntax.errorMessage().toAscii().constData());
	// Remember internal, reserved names
	QScriptValueIterator it(engine->globalObject());
	while (it.hasNext())
	{
		it.next();
		internalNamespace.insert(it.name(), 1);
	}
	QScriptValue result = engine->evaluate(source, path);
	ASSERT_OR_RETURN(false, !engine->hasUncaughtException(), "Uncaught exception at line %d, file %s: %s", 
	                 engine->uncaughtExceptionLineNumber(), path.toAscii().constData(), result.toString().toAscii().constData());
	// Special functions
	engine->globalObject().setProperty("setTimer", engine->newFunction(js_setTimer));
	engine->globalObject().setProperty("queue", engine->newFunction(js_queue));
	engine->globalObject().setProperty("removeTimer", engine->newFunction(js_removeTimer));
	engine->globalObject().setProperty("include", engine->newFunction(js_include));
	engine->globalObject().setProperty("bind", engine->newFunction(js_bind));

	// Special global variables
	engine->globalObject().setProperty("me", player, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("selectedPlayer", selectedPlayer, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("gameTime", gameTime, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("difficulty", difficulty, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("mapName", game.map, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("baseType", game.base, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("alliancesType", game.alliance, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("powerType", game.power, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("maxPlayers", game.maxPlayers, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("scavengers", game.scavengers, QScriptValue::ReadOnly | QScriptValue::Undeletable);

	// Regular functions
	registerFunctions(engine);

	// Register script
	scripts.push_back(engine);

	return true;
}

bool loadGlobalScript(QString path)
{
	return loadPlayerScript(path, selectedPlayer, 0);
}

bool saveScriptStates(const char *filename)
{
	WzConfig ini(filename);
	if (!ini.isWritable())
	{
		debug(LOG_ERROR, "Savegame file not writable: %s", filename);
		return false;
	}
	for (int i = 0; i < scripts.size(); ++i)
	{
		QScriptEngine *engine = scripts.at(i);
		QScriptValueIterator it(engine->globalObject());
		ini.beginGroup(QString("globals_") + QString::number(i));
		while (it.hasNext())
		{
			it.next();
			if (!internalNamespace.contains(it.name()) && !it.value().isFunction())
			{
				ini.setValue(it.name(), it.value().toVariant());
			}
		}
		ini.endGroup();
	}
	for (int i = 0; i < timers.size(); ++i)
	{
		timerNode node = timers.at(i);
		ini.beginGroup(QString("triggers_") + QString::number(i));
		ini.setValue("player", node.player);
		ini.setValue("function", node.function.toUtf8().constData());
		if (!node.baseobj >= 0)
		{
			ini.setValue("object", QVariant(node.baseobj));
		}
		ini.setValue("frame", node.frameTime);
		ini.setValue("type", (int)node.type);
		ini.setValue("ms", node.ms);
		ini.endGroup();
	}
	return true;
}

static QScriptEngine *findEngineForPlayer(int match)
{
	for (int i = 0; i < scripts.size(); ++i)
	{
		QScriptEngine *engine = scripts.at(i);
		int player = engine->globalObject().property("me").toInt32();
		if (match == player)
		{
			return engine;
		}
	}
	ASSERT(false, "Script context for player %d not found", match);
	return NULL;
}

bool loadScriptStates(const char *filename)
{
	WzConfig ini(filename);
	QStringList list = ini.childGroups();
	debug(LOG_SAVE, "Loading script states for %d script contexts", scripts.size());
	for (int i = 0; i < list.size(); ++i)
	{
		if (list[i].startsWith("triggers_"))
		{
			ini.beginGroup(list[i]);
			timerNode node;
			node.player = ini.value("player").toInt();
			node.ms = ini.value("ms").toInt();
			node.frameTime = ini.value("frame").toInt();
			debug(LOG_SAVE, "Registering trigger %d for player %d", i, node.player);
			node.engine = findEngineForPlayer(node.player);
			node.function = ini.value("function").toString();
			node.baseobj = ini.value("baseobj", -1).toInt();
			node.type = (timerType)ini.value("type", TIMER_REPEAT).toInt();
			timers.push_back(node);
			ini.endGroup();
		}
		else if (list[i].startsWith("globals_"))
		{
			ini.beginGroup(list[i]);
			int player = ini.value("me").toInt();
			QScriptEngine *engine = findEngineForPlayer(player);
			QStringList keys = ini.childKeys();
			debug(LOG_SAVE, "Loading script globals for player %d -- found %d values", player, keys.size());
			for (int j = 0; j < keys.size(); ++j)
			{
				engine->globalObject().setProperty(keys.at(j), engine->toScriptValue(ini.value(keys.at(j))));
			}
			ini.endGroup();
		}
	}
	return true;
}

// ----------------------------------------------------------------------------------------
// Events

/// For generic, parameter-less triggers
bool triggerEvent(SCRIPT_TRIGGER_TYPE trigger)
{
	for (int i = 0; i < scripts.size(); ++i)
	{
		QScriptEngine *engine = scripts.at(i);

		switch (trigger)
		{
		case TRIGGER_GAME_INIT:
			callFunction(engine, "eventGameInit", QScriptValueList());
			break;
		case TRIGGER_START_LEVEL:
			callFunction(engine, "eventStartLevel", QScriptValueList());
			break;
		}
	}
	return true;
}

bool triggerEventDroidBuilt(DROID *psDroid, STRUCTURE *psFactory)
{
	for (int i = 0; i < scripts.size(); ++i)
	{
		QScriptEngine *engine = scripts.at(i);
		int player = engine->globalObject().property("me").toInt32();
		if (player == psDroid->player)
		{
			QScriptValueList args;
			args += convDroid(psDroid, engine);
			args += convStructure(psFactory, engine);
			callFunction(engine, "eventDroidBuilt", args);
		}
	}
	return true;
}

bool triggerStructureAttacked(STRUCTURE *psVictim, BASE_OBJECT *psAttacker)
{
	if (!psAttacker)
	{
		// do not fire off this event if there is no attacker -- nothing do respond to
		// (FIXME -- consider this carefully)
		return false;
	}
	for (int i = 0; i < scripts.size(); ++i)
	{
		QScriptEngine *engine = scripts.at(i);
		int player = engine->globalObject().property("me").toInt32();
		if (player == psVictim->player)
		{
			QScriptValueList args;
			args += convStructure(psVictim, engine);
			args += convObj(psAttacker, engine);
			callFunction(engine, "eventStructureAttacked", args);
		}
	}
	return true;
}

bool triggerResearched(STRUCTURE *psStruct)
{
	for (int i = 0; i < scripts.size() && psStruct; ++i)
	{
		QScriptEngine *engine = scripts.at(i);
		int player = engine->globalObject().property("me").toInt32();
		if (player == psStruct->player)
		{
			QScriptEngine *engine = scripts.at(i);
			QScriptValueList args;
			args += convStructure(psStruct, engine);
			callFunction(engine, "eventResearched", args);
		}
	}
	return true;
}
