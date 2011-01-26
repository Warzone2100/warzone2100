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
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>
#include <QtScript/QScriptSyntaxCheckResult>
#include <QtCore/QList>
#include <QtCore/QString>

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

static bool callFunction(QScriptEngine *engine, QString function, QScriptValueList args)
{
	QScriptValue value = engine->globalObject().property(function);
	if (value.isValid() && value.isFunction())
	{
		QScriptValue result = value.call(QScriptValue(), args);
		if (engine->hasUncaughtException())
		{
			// TODO, get filename to output here somehow
			int line = engine->uncaughtExceptionLineNumber();
			debug(LOG_ERROR, "Uncaught exception calling event %s at line %d: %s",
			      value.toString().toAscii().constData(), line, result.toString().toAscii().constData());
			return false;
		}
	}
	else
	{
		debug(LOG_ERROR, "Invalid function type for \"%s\"", function.toAscii().constData());
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

/// Special scripting function that registers a non-specific timer event
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

/// Special scripting function that registers a object-specific timer event
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
	while (!scripts.isEmpty())
	{
		delete scripts.takeFirst();
	}
	return true;
}

bool updateScripts()
{
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
	}
	return true;
}

bool loadPlayerScript(const char *filename, int player, int difficulty)
{
	QString path(QString("multiplay/skirmish/") + filename);
	ASSERT_OR_RETURN(false, player < MAX_PLAYERS, "Player index %d out of bounds", player);
	QScriptEngine *engine = new QScriptEngine();
	UDWORD size;
	char *bytes;
	if (!loadFile(path.toAscii().constData(), &bytes, &size))
	{
		debug(LOG_ERROR, "Failed to read script file \"%s\"", path.toAscii().constData());
		return false;
	}
	QString source = QString::fromAscii(bytes, size);
	QScriptSyntaxCheckResult syntax = QScriptEngine::checkSyntax(source);
	if (syntax.state() != QScriptSyntaxCheckResult::Valid)
	{
		debug(LOG_ERROR, "Syntax error in %s line %d: %s", filename, syntax.errorLineNumber(), syntax.errorMessage().toAscii().constData());
		free(bytes);
		return false;
	}
	QScriptValue result = engine->evaluate(source, filename);
	if (engine->hasUncaughtException())
	{
		int line = engine->uncaughtExceptionLineNumber();
		debug(LOG_ERROR, "Uncaught exception at line %d, file %s: %s", line, filename, result.toString().toAscii().constData());
		free(bytes);
		return false;
	}
	engine->globalObject().setProperty("setGlobalTimer", engine->newFunction(js_setGlobalTimer));
	engine->globalObject().setProperty("setObjectTimer", engine->newFunction(js_setObjectTimer));
	engine->globalObject().setProperty("removeTimer", engine->newFunction(js_removeTimer));
	engine->globalObject().setProperty("me", player, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("difficulty", difficulty, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	registerFunctions(engine);
	scripts.push_back(engine);
	free(bytes);
	return true;
}

bool loadGlobalScript(const char *filename)
{
	return loadPlayerScript(filename, 0, 0);
}

bool loadScriptStates(const char *filename)
{
	Q_UNUSED(filename);
	return true;
}

bool saveScriptStates(const char *filename)
{
	Q_UNUSED(filename);
	return true;
}

/// Update scripting state associated with the given object
bool updateObject(BASE_OBJECT *psObj)
{
	Q_UNUSED(psObj);
	return true;
}

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
