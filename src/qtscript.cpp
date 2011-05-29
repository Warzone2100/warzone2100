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
#include <QtCore/QSettings>

#include "lib/framework/file.h"
#include "lib/gamelib/gtime.h"
#include "multiplay.h"

#include "qtscriptfuncs.h"

struct timerNode
{
	QString function;
	QScriptEngine *engine;
	QString baseobj;
	int frameTime;
	int ms;
	int player;
	timerNode() {}
	timerNode(QScriptEngine *caller, QString val, int plr, int frame) : function(val) { player = plr; ms = frame; frameTime = frame + gameTime; engine = caller; }
	bool operator== (const timerNode &t) { return function == t.function && player == t.player; }
	// implement operator less TODO
};

/// List of timer events for scripts. Before running them, we sort the list then run as many as we have time for.
/// In this way, we implement load balancing of events and keep frame rates tidy for users. Since scripts run on the
/// host, we do not need to worry about each peer simulating the world differently.
static QList<timerNode> timers;

/// Scripting engine (what others call the scripting context, but QtScript's nomenclature is different).
static QList<QScriptEngine *> scripts;

/// Remember what names are used internally in the scripting engine, we don't want to save these to the savegame
static QHash<QString, int> internalNamespace;

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

/// Special scripting function that registers a non-specific timer event. Note: Functions must be passed
/// quoted, otherwise they will be inlined!
static QScriptValue js_setGlobalTimer(QScriptContext *context, QScriptEngine *engine)
{
	QString funcName = context->argument(0).toString();
	QScriptValue ms = context->argument(1);
	int player = engine->globalObject().property("me").toInt32();
	timerNode node(engine, funcName, player, ms.toInt32() + gameTime);
	timers.push_back(node);
	return QScriptValue();
}

/// Special scripting function that registers a object-specific timer event. Note: Functions must be passed
/// quoted, otherwise they will be inlined!
static QScriptValue js_setObjectTimer(QScriptContext *context, QScriptEngine *engine)
{
	QString funcName = context->argument(0).toString();
	QScriptValue ms = context->argument(1);
	QScriptValue obj = context->argument(2);
	int player = engine->globalObject().property("me").toInt32();
	timerNode node(engine, funcName, player, ms.toInt32());
	node.baseobj = obj.toString();
	timers.push_back(node);
	return QScriptValue();
}

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
	// Check for timers, and run them if applicable.
	// TODO - load balancing
	QMutableListIterator<timerNode> iter(timers);
	while (iter.hasNext())
	{
		timerNode node = iter.next();
		if (node.frameTime <= gameTime)
		{
			node.frameTime = node.ms + gameTime;	// update for next invokation
			callFunction(node.engine, node.function, QScriptValueList());
		}
		// Node could have been brutally removed from underneath us at this point
		if (timers.contains(node))
		{
			iter.setValue(node);
		}
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
	engine->globalObject().setProperty("setGlobalTimer", engine->newFunction(js_setGlobalTimer));
	engine->globalObject().setProperty("setObjectTimer", engine->newFunction(js_setObjectTimer));
	engine->globalObject().setProperty("removeTimer", engine->newFunction(js_removeTimer));
	engine->globalObject().setProperty("include", engine->newFunction(js_include));

	// Special global variables
	engine->globalObject().setProperty("me", player, QScriptValue::ReadOnly | QScriptValue::Undeletable);
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
	return loadPlayerScript(path, 0, 0);
}

bool saveScriptStates(const char *filename)
{
	QSettings ini(QString("wz::") + filename, QSettings::IniFormat);
	if (!ini.isWritable())
	{
		debug(LOG_ERROR, "Savegame file not writable");
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
		if (!node.baseobj.isEmpty())
		{
			ini.setValue("object", node.baseobj.toUtf8().constData());
		}
		ini.setValue("frame", node.frameTime);
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
	QSettings ini(QString("wz::") + filename, QSettings::IniFormat);
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
			node.baseobj = ini.value("baseobj", QString()).toString();
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
