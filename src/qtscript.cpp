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

#include "qtscriptfuncs.h"

struct timerNode
{
	QString function;
	QScriptEngine *engine;
	QString baseobj;
	int frameTime;
	int ms;
	int player;
	timerNode(QScriptEngine *caller, QString val, int plr, int frame) : function(val) { player = plr; ms = frame; frameTime = frame + gameTime; engine = caller; }
	// implement operator less TODO
};

/// List of timer events for scripts. Before running them, we sort the list then run as many as we have time for.
/// In this way, we implement load balancing of events and keep frame rates tidy for users. Since scripts run on the
/// host, we do not need to worry about each peer simulating the world differently.
QList<timerNode> timers;

/// Scripting engine (what others call the scripting context, but QtScript's nomenclature is different).
QList<QScriptEngine *> scripts;

/// Remember what names are used internally in the scripting engine, we don't want to save these to the savegame
QHash<QString, int> internalNamespace;

// Call a function by name
static bool callFunction(QScriptEngine *engine, QString function, QScriptValueList args)
{
	QScriptValue value = engine->globalObject().property(function);
	ASSERT_OR_RETURN(false, value.isValid() && value.isFunction(), "Invalid function type for \"%s\"", function.toAscii().constData());
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
	QScriptValue function = context->argument(0);
	QScriptValue ms = context->argument(1);
	int player = engine->globalObject().property("me").toInt32();
	QString funcName = function.toString();
	timerNode node(engine, funcName, player, ms.toInt32() + gameTime);
	timers.push_back(node);
	return QScriptValue();
}

/// Special scripting function that registers a object-specific timer event. Note: Functions must be passed
/// quoted, otherwise they will be inlined!
static QScriptValue js_setObjectTimer(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue function = context->argument(0);
	QScriptValue ms = context->argument(1);
	QScriptValue obj = context->argument(2);
	int player = engine->globalObject().property("me").toInt32();
	timerNode node(engine, function.toString(), player, ms.toInt32());
	node.baseobj = obj.toString();
	timers.push_back(node);
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
	for (int i = 0; i < timers.size(); ++i)
	{
		timerNode node = timers.at(i);
		if (node.frameTime <= gameTime)
		{
			node.frameTime = node.ms + gameTime;	// update for next invokation
			callFunction(node.engine, node.function, QScriptValueList());
		}
		timers.replace(i, node);
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
	if (syntax.state() != QScriptSyntaxCheckResult::Valid)
	{
		debug(LOG_ERROR, "Syntax error in %s line %d: %s", path.toAscii().constData(), syntax.errorLineNumber(), syntax.errorMessage().toAscii().constData());
		return false;
	}
	// Remember internal, reserved names
	QScriptValueIterator it(engine->globalObject());
	while (it.hasNext())
	{
		it.next();
		internalNamespace.insert(it.name(), 1);
	}
	QScriptValue result = engine->evaluate(source, path);
	if (engine->hasUncaughtException())
	{
		int line = engine->uncaughtExceptionLineNumber();
		debug(LOG_ERROR, "Uncaught exception at line %d, file %s: %s", line, path.toAscii().constData(), result.toString().toAscii().constData());
		return false;
	}
	// Special functions
	engine->globalObject().setProperty("setGlobalTimer", engine->newFunction(js_setGlobalTimer));
	engine->globalObject().setProperty("setObjectTimer", engine->newFunction(js_setObjectTimer));
	engine->globalObject().setProperty("removeTimer", engine->newFunction(js_removeTimer));

	// Special global variables
	engine->globalObject().setProperty("me", player, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("gameTime", gameTime, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("difficulty", difficulty, QScriptValue::ReadOnly | QScriptValue::Undeletable);

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
	QSettings ini(filename, QSettings::IniFormat);
	for (int i = 0; i < scripts.size(); ++i)
	{
		ini.beginGroup(QString("globals_") + QString::number(i));
		QScriptEngine *engine = scripts.at(i);
		QScriptValueIterator it(engine->globalObject());
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
	return true;
}

bool loadScriptStates(const char *filename)
{
	QSettings ini(filename, QSettings::IniFormat);
	for (int i = 0; i < scripts.size(); ++i)
	{
		ini.beginGroup(QString("globals_") + QString::number(i));
		QStringList keys = ini.childKeys();
		QScriptEngine *engine = scripts.at(i);
		for (int j = 0; j < keys.size(); ++j)
		{
			engine->globalObject().setProperty(keys.at(i), engine->toScriptValue(ini.value(keys.at(i))));
		}
		ini.endGroup();
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
