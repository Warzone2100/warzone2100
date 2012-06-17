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
#include "map.h"

#include "qtscriptfuncs.h"

#define ATTACK_THROTTLE 100

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

#define MAX_MS 50

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
static bool callFunction(QScriptEngine *engine, const QString &function, const QScriptValueList &args, bool required = false)
{
	code_part level = required ? LOG_ERROR : LOG_WARNING;
	QScriptValue value = engine->globalObject().property(function);
	if (!value.isValid() || !value.isFunction())
	{
		// not necessarily an error, may just be a trigger that is not defined (ie not needed)
		// or it could be a typo in the function name or ...
		debug(level, "called function (%s) not defined", function.toUtf8().constData());
		return false;
	}
	int ticks = wzGetTicks();
	QScriptValue result = value.call(QScriptValue(), args);
	ticks = wzGetTicks() - ticks;
	if (ticks > MAX_MS)
	{
		debug(LOG_SCRIPT, "%s took %d ms at time %d", function.toAscii().constData(), ticks, wzGetTicks());
	}
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

//-- \subsection{setTimer(function, milliseconds[, object])}
//-- Set a function to run repeated at some given time interval. The function to run 
//-- is the first parameter, and it \underline{must be quoted}, otherwise the function will
//-- be inlined. The second parameter is the interval, in milliseconds. A third, optional
//-- parameter can be a \emph{game object} to pass to the timer function. If the \emph{game object}
//-- dies, the timer stops running. The minimum number of milliseconds is 100, but such
//-- fast timers are strongly discouraged as they may deteriorate the game performance.
//--
//-- \begin{lstlisting}
//--   function conDroids()
//--   {
//--      ... do stuff ...
//--   }
//--   // call conDroids every 4 seconds
//--   setTimer("conDroids", 4000);
//-- \end{lstlisting}
static QScriptValue js_setTimer(QScriptContext *context, QScriptEngine *engine)
{
	SCRIPT_ASSERT(context, context->argument(0).isString(), "Timer functions must be quoted");
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

//-- \subsection{removeTimer(function)}
//-- Removes an existing timer. The first parameter is the function timer to remove,
//-- and its name \underline{must be quoted}.
static QScriptValue js_removeTimer(QScriptContext *context, QScriptEngine *engine)
{
	SCRIPT_ASSERT(context, context->argument(0).isString(), "Timer functions must be quoted");
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

//-- \subsection{queue(function[, milliseconds[, object]])}
//-- Queues up a function to run at a later game frame. This is useful to prevent
//-- stuttering during the game, which can happen if too much script processing is
//-- done at once.  The function to run is the first parameter, and it 
//-- \underline{must be quoted}, otherwise the function will be inlined.
//-- The second parameter is the delay in milliseconds, if it is omitted or 0,
//-- the function will be run at a later frame.  A third optional
//-- parameter can be a \emph{game object} to pass to the queued function. If the \emph{game object}
//-- dies before the queued call runs, nothing happens.
// TODO, check if an identical call is already queued up - and in this case, 
// do not add anything.
static QScriptValue js_queue(QScriptContext *context, QScriptEngine *engine)
{
	SCRIPT_ASSERT(context, context->argument(0).isString(), "Queued functions must be quoted");
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
			callFunction(node.engine, node.funcName, args, true);
		}
		i = bindings.erase(i);
	}
}

//-- \subsection{bind(function, object[, player])}
//-- Bind a function call to an object. The function is called before the 
//-- object is destroyed. The function to run is the first parameter, and it 
//-- \underline{must be quoted}, otherwise the function will be inlined. The
//-- second argument is the object to bind to. A third, optional player parameter
//-- may be passed, which may be used for filtering, depending on the object type.
//-- \emph{NOTE: This function is under construction and is subject to total change!}
static QScriptValue js_bind(QScriptContext *context, QScriptEngine *engine)
{
	SCRIPT_ASSERT(context, context->argument(0).isString(), "Bound functions must be quoted");
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

//-- \subsection{include(file)}
//-- Includes another source code file at this point. This is experimental, and breaks the
//-- lint tool, so use with care.
static QScriptValue js_include(QScriptContext *context, QScriptEngine *engine)
{
	QString path = context->argument(0).toString();
	UDWORD size;
	char *bytes = NULL;
	if (!loadFile(path.toAscii().constData(), &bytes, &size))
	{
		debug(LOG_ERROR, "Failed to read include file \"%s\"", path.toAscii().constData());
		return QScriptValue(false);
	}
	QString source = QString::fromAscii(bytes, size);
	free(bytes);
	QScriptSyntaxCheckResult syntax = QScriptEngine::checkSyntax(source);
	if (syntax.state() != QScriptSyntaxCheckResult::Valid)
	{
		debug(LOG_ERROR, "Syntax error in include %s line %d: %s", 
		      path.toAscii().constData(), syntax.errorLineNumber(), syntax.errorMessage().toAscii().constData());
		return QScriptValue(false);
	}
	context->setActivationObject(engine->globalObject());
	context->setThisObject(engine->globalObject());
	QScriptValue result = engine->evaluate(source, path);
	if (engine->hasUncaughtException())
	{
		int line = engine->uncaughtExceptionLineNumber();
		debug(LOG_ERROR, "Uncaught exception at line %d, include file %s: %s",
		      line, path.toAscii().constData(), result.toString().toAscii().constData());
		return QScriptValue(false);
	}
	return QScriptValue(true);
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
		callFunction(iter->engine, iter->function, args, true);
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
	// Special functions
	engine->globalObject().setProperty("setTimer", engine->newFunction(js_setTimer));
	engine->globalObject().setProperty("queue", engine->newFunction(js_queue));
	engine->globalObject().setProperty("removeTimer", engine->newFunction(js_removeTimer));
	engine->globalObject().setProperty("include", engine->newFunction(js_include));
	engine->globalObject().setProperty("bind", engine->newFunction(js_bind));

	// Special global variables
	//== \item[version] Current version of the game, set in \emph{major.minor} format.
	engine->globalObject().setProperty("version", "3.2", QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//== \item[selectedPlayer] The player ontrolled by the client on which the script runs.
	engine->globalObject().setProperty("selectedPlayer", selectedPlayer, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//== \item[gameTime] The current game time. Updated before every invokation of a script.
	engine->globalObject().setProperty("gameTime", gameTime, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//== \item[difficulty] The currently set campaign difficulty, or the current AI's difficulty setting. It will be one of
	//== EASY, MEDIUM, HARD or INSANE.
	engine->globalObject().setProperty("difficulty", difficulty, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//== \item[mapName] The name of the current map.
	engine->globalObject().setProperty("mapName", game.map, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//== \item[baseType] The type of base that the game starts with. It will be one of CAMP_CLEAN, CAMP_BASE or CAMP_WALLS.
	engine->globalObject().setProperty("baseType", game.base, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//== \item[alliancesType] The type of alliances permitted in this game. It will be one of NO_ALLIANCES, ALLIANCES or ALLIANCES_TEAMS.
	engine->globalObject().setProperty("alliancesType", game.alliance, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//== \item[powerType] The power level set for this game.
	engine->globalObject().setProperty("powerType", game.power, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//== \item[maxPlayers] The number of active players in this game.
	engine->globalObject().setProperty("maxPlayers", game.maxPlayers, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//== \item[scavengers] Whether or not scavengers are activated in this game.
	engine->globalObject().setProperty("scavengers", game.scavengers, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//== \item[mapWidth] Width of map in tiles.
	engine->globalObject().setProperty("mapWidth", mapWidth, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//== \item[mapHeight] Height of map in tiles.
	engine->globalObject().setProperty("mapHeight", mapHeight, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//== \item[scavengerPlayer] Index of scavenger player. (3.2+ only)
	engine->globalObject().setProperty("scavengerPlayer", scavengerPlayer(), QScriptValue::ReadOnly | QScriptValue::Undeletable);

	// Regular functions
	registerFunctions(engine);

	// Remember internal, reserved names
	QScriptValueIterator it(engine->globalObject());
	while (it.hasNext())
	{
		it.next();
		internalNamespace.insert(it.name(), 1);
	}
	// We need to always save the 'me' special variable.
	//== \item[me] The player the script is currently running as.
	engine->globalObject().setProperty("me", player, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	QScriptValue result = engine->evaluate(source, path);
	ASSERT_OR_RETURN(false, !engine->hasUncaughtException(), "Uncaught exception at line %d, file %s: %s", 
	                 engine->uncaughtExceptionLineNumber(), path.toAscii().constData(), result.toString().toAscii().constData());

	// Register script
	scripts.push_back(engine);

	debug(LOG_SAVE, "Created script engine %d for player %d from %s", scripts.size() - 1, player, path.toUtf8().constData());
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
			if (node.engine)
			{
				node.function = ini.value("function").toString();
				node.baseobj = ini.value("baseobj", -1).toInt();
				node.type = (timerType)ini.value("type", TIMER_REPEAT).toInt();
				timers.push_back(node);
			}
			ini.endGroup();
		}
		else if (list[i].startsWith("globals_"))
		{
			ini.beginGroup(list[i]);
			int player = ini.value("me").toInt();
			QStringList keys = ini.childKeys();
			debug(LOG_SAVE, "Loading script globals for player %d -- found %d values", player, keys.size());
			QScriptEngine *engine = findEngineForPlayer(player);
			if (engine)
			{
				for (int j = 0; j < keys.size(); ++j)
				{
					engine->globalObject().setProperty(keys.at(j), engine->toScriptValue(ini.value(keys.at(j))));
				}
			}
			ini.endGroup();
		}
	}
	return true;
}

// ----------------------------------------------------------------------------------------
// Events

/// For generic, parameter-less triggers
//__ \subsection{eventGameInit()}
//__ An event that is run once as the game is initialized. Not all game may have been 
//__ properly initialized by this time, so use this only to initialize script state.
//__ \subsection{eventStartLevel()}
//__ An event that is run once the game has started and all game data has been loaded.
//__ \subsection{eventLaunchTransporter()}
//__ An event that is run when the transporter has been ordered to fly off in a mission.
//__ \subsection{eventReinforcementsArrived()}
//__ An event that is run when the transporter has arrived with reinforcements in a mission.
//__ \subsection{eventMissionTimeout()}
//__ An event that is run when the mission timer has run out.
//__ \subsection{eventVideoDone()}
//__ An event that is run when a video show stopped playing.
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
			processVisibility(); // make sure we initialize visibility first
			callFunction(engine, "eventStartLevel", QScriptValueList());
			break;
		case TRIGGER_LAUNCH_TRANSPORTER:
			callFunction(engine, "eventLaunchTransporter", QScriptValueList());
			break;
		case TRIGGER_REINFORCEMENTS_ARRIVED:
			callFunction(engine, "eventReinforcementsArrived", QScriptValueList());
			break;
		case TRIGGER_MISSION_TIMEOUT:
			callFunction(engine, "eventMissionTimeout", QScriptValueList());
			break;
		case TRIGGER_VIDEO_QUIT:
			callFunction(engine, "eventVideoDone", QScriptValueList());
			break;
		}
	}
	return true;
}

//__ \subsection{eventDroidIdle(droid)} A droid should be given new orders.
bool triggerEventDroidIdle(DROID *psDroid)
{
	for (int i = 0; i < scripts.size(); ++i)
	{
		QScriptEngine *engine = scripts.at(i);
		int player = engine->globalObject().property("me").toInt32();
		if (player == psDroid->player)
		{
			QScriptValueList args;
			args += convDroid(psDroid, engine);
			callFunction(engine, "eventDroidIdle", args);
		}
	}
	return true;
}

//__ \subsection{eventDroidBuilt(droid[, structure])}
//__ An event that is run every time a droid is built. The structure parameter is set
//__ if the droid was produced in a factory. It is not triggered for droid theft or
//__ gift (check \emph{eventObjectTransfer} for that).
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
			if (psFactory)
			{
				args += convStructure(psFactory, engine);
			}
			callFunction(engine, "eventDroidBuilt", args);
		}
	}
	return true;
}

//__ \subsection{eventStructureBuilt(structure[, droid])}
//__ An event that is run every time a structure is produced. The droid parameter is set
//__ if the structure was built by a droid. It is not triggered for building theft
//__ (check \emph{eventObjectTransfer} for that).
bool triggerEventStructBuilt(STRUCTURE *psStruct, DROID *psDroid)
{
	for (int i = 0; i < scripts.size(); ++i)
	{
		QScriptEngine *engine = scripts.at(i);
		int player = engine->globalObject().property("me").toInt32();
		if (player == psStruct->player)
		{
			QScriptValueList args;
			args += convStructure(psStruct, engine);
			if (psDroid)
			{
				args += convDroid(psDroid, engine);
			}
			callFunction(engine, "eventStructureBuilt", args);
		}
	}
	return true;
}

//__ \subsection{eventStructureReady(structure)}
//__ An event that is run every time a structure is ready to perform some
//__ special ability. It will only fire once, so if the time is not right,
//__ register your own timer to keep checking.
bool triggerEventStructureReady(STRUCTURE *psStruct)
{
	for (int i = 0; i < scripts.size(); ++i)
	{
		QScriptEngine *engine = scripts.at(i);
		int player = engine->globalObject().property("me").toInt32();
		if (player == psStruct->player)
		{
			QScriptValueList args;
			args += convStructure(psStruct, engine);
			callFunction(engine, "eventStructureReady", args);
		}
	}
	return true;
}

//__ \subsection{eventAttacked(victim, attacker)}
//__ An event that is run when an object belonging to the script's controlling player is
//__ attacked. The attacker parameter may be either a structure or a droid.
bool triggerEventAttacked(BASE_OBJECT *psVictim, BASE_OBJECT *psAttacker, int lastHit)
{
	if (!psAttacker)
	{
		// do not fire off this event if there is no attacker -- nothing do respond to
		// (FIXME -- consider this carefully)
		return false;
	}
	// throttle the event for performance
	if (gameTime - lastHit < ATTACK_THROTTLE)
	{
		return false;
	}
	for (int i = 0; i < scripts.size(); ++i)
	{
		QScriptEngine *engine = scripts.at(i);
		int player = engine->globalObject().property("me").toInt32();
		if (player == psVictim->player)
		{
			QScriptValueList args;
			args += convMax(psVictim, engine);
			args += convMax(psAttacker, engine);
			callFunction(engine, "eventAttacked", args);
		}
	}
	return true;
}

//__ \subsection{eventResearched(research[, structure])}
//__ An event that is run whenever a new research is available. The structure
//__ parameter is set if the research comes from a research lab.
bool triggerEventResearched(RESEARCH *psResearch, STRUCTURE *psStruct, int player)
{
	for (int i = 0; i < scripts.size() && psStruct; ++i)
	{
		QScriptEngine *engine = scripts.at(i);
		int me = engine->globalObject().property("me").toInt32();
		if (me == player)
		{
			QScriptValueList args;
			args += convResearch(psResearch, engine, player);
			if (psStruct)
			{
				args += convStructure(psStruct, engine);
			}
			callFunction(engine, "eventResearched", args);
		}
	}
	return true;
}

//__ \subsection{eventDestroyed(object)}
//__ An event that is run whenever an object is destroyed for the owning player.
//__ Careful passing the parameter object around, since it is about to vanish!
bool triggerEventDestroyed(BASE_OBJECT *psVictim)
{
	for (int i = 0; i < scripts.size() && psVictim; ++i)
	{
		QScriptEngine *engine = scripts.at(i);
		int me = engine->globalObject().property("me").toInt32();
		if (me == psVictim->player)
		{
			QScriptValueList args;
			args += convMax(psVictim, engine);
			callFunction(engine, "eventDestroyed", args);
		}
	}
	return true;
}

//__ \subsection{eventPickup(feature, droid)}
//__ An event that is run whenever a feature is picked up. It is called for
//__ all players / scripts.
//__ Careful passing the parameter object around, since it is about to vanish! (3.2+ only)
bool triggerEventPickup(FEATURE *psFeat, DROID *psDroid)
{
	for (int i = 0; i < scripts.size(); ++i)
	{
		QScriptEngine *engine = scripts.at(i);
		QScriptValueList args;
		args += convFeature(psFeat, engine);
		args += convDroid(psDroid, engine);
		callFunction(engine, "eventPickup", args);
	}
	return true;
}

//__ \subsection{eventObjectSeen(viewer, seen)}
//__ An event that is run whenever an object goes from not seen to seen.
//__ First parameter is \emph{game object} doing the seeing, the next the game
//__ object being seen.
bool triggerEventSeen(BASE_OBJECT *psViewer, BASE_OBJECT *psSeen)
{
	for (int i = 0; i < scripts.size() && psSeen && psViewer; ++i)
	{
		QScriptEngine *engine = scripts.at(i);
		int me = engine->globalObject().property("me").toInt32();
		if (me == psViewer->player)
		{
			QScriptValueList args;
			args += convMax(psViewer, engine);
			args += convMax(psSeen, engine);
			callFunction(engine, "eventObjectSeen", args);
		}
	}
	return true;
}

//__ \subsection{eventObjectTransfer(object, from)}
//__ An event that is run whenever an object is transferred between players,
//__ for example due to a Nexus Link weapon. The event is called after the
//__ object has been transferred, so the target player is in object.player.
//__ The event is called for both players.
bool triggerEventObjectTransfer(BASE_OBJECT *psObj, int from)
{
	for (int i = 0; i < scripts.size() && psObj; ++i)
	{
		QScriptEngine *engine = scripts.at(i);
		int me = engine->globalObject().property("me").toInt32();
		if (me == psObj->player || me == from)
		{
			QScriptValueList args;
			args += convMax(psObj, engine);
			args += QScriptValue(from);
			callFunction(engine, "eventObjectTransfer", args);
		}
	}
	return true;
}

//__ \subsection{eventChat(from, to, message)}
//__ An event that is run whenever a chat message is received. The \emph{from} parameter is the
//__ player sending the chat message. For the moment, the \emph{to} parameter is always the script
//__ player.
bool triggerEventChat(int from, int to, const char *message)
{
	for (int i = 0; i < scripts.size() && message; ++i)
	{
		QScriptEngine *engine = scripts.at(i);
		int me = engine->globalObject().property("me").toInt32();
		if (me == to)
		{
			QScriptEngine *engine = scripts.at(i);
			QScriptValueList args;
			args += QScriptValue(from);
			args += QScriptValue(to);
			args += QScriptValue(message);
			callFunction(engine, "eventChat", args);
		}
	}
	return true;
}
