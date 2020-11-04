/*
	This file is part of Warzone 2100.
	Copyright (C) 2011-2020  Warzone 2100 Project

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

// Documentation stuff follows. The build system will parse the comment prefixes
// and sort the comments into the appropriate Markdown documentation files.

//== # Globals
//==
//== This section describes global variables (or 'globals' for short) that are
//== available from all scripts. You typically cannot write to these variables,
//== they are read-only.
//==
//__ # Events
//__
//__ This section describes event callbacks (or 'events' for short) that are
//__ called from the game when something specific happens. Which scripts
//__ receive them is usually filtered by player. Call ```receiveAllEvents(true)```
//__ to start receiving all events unfiltered.
//__
//-- # Functions
//--
//-- This section describes functions that can be called from scripts to make
//-- things happen in the game (usually called our script 'API').
//--
//;; # Game objects
//;;
//;; This section describes various **game objects** defined by the script interface,
//;; and which are both accepted by functions and returned by them. Changing the
//;; fields of a **game object** has no effect on the game before it is passed to a
//;; function that does something with the **game object**.
//;;

#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__) && (9 <= __GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdeprecated-copy" // Workaround Qt < 5.13 `deprecated-copy` issues with GCC 9
#endif

// **NOTE: Qt headers _must_ be before platform specific headers so we don't get conflicts.
#include <QtCore/QTextStream>
#include <QtCore/QHash>
#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>
#include <QtScript/QScriptValueIterator>
#include <QtScript/QScriptSyntaxCheckResult>
#include <QtCore/QList>
#include <QtCore/QQueue>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QFileInfo>
#include <QtCore/QElapsedTimer>
#include <QtGui/QStandardItemModel>
#include <QtWidgets/QFileDialog>

#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__) && (9 <= __GNUC__)
# pragma GCC diagnostic pop // Workaround Qt < 5.13 `deprecated-copy` issues with GCC 9
#endif

#include "lib/framework/wzapp.h"
#include "lib/framework/wzconfig.h"

#include "qtscript.h"

#include "lib/framework/file.h"
#include "lib/gamelib/gtime.h"
#include "lib/netplay/netplay.h"
#include "multiplay.h"
#include "levels.h"
#include "map.h"
#include "difficulty.h"
#include "console.h"
#include "clparse.h"
#include "mission.h"
#include "modding.h"
#include "version.h"
#include "game.h"

#include <set>
#include <memory>
#include <utility>

#include "qtscriptdebug.h"
#include "qtscriptfuncs.h"

#define ATTACK_THROTTLE 1000

typedef QList<QStandardItem *> QStandardItemList;

/// selection changes are too often and too erratic to trigger immediately,
/// so until we have a queue system for events, delay triggering this way.
static bool selectionChanged = false;
extern bool doUpdateModels; // ugh-ly hack; fix with signal when moc-ing this file

enum timerType
{
	TIMER_REPEAT, TIMER_ONESHOT_READY, TIMER_ONESHOT_DONE
};

struct timerNode
{
	QString function;
	QScriptEngine *engine;
	int baseobj;
	OBJECT_TYPE baseobjtype;
	QString stringarg;
	int frameTime;
	int ms;
	int player;
	int calls;
	timerType type;
	timerNode() : engine(nullptr), baseobjtype(OBJ_NUM_TYPES) {}
	timerNode(QScriptEngine *caller, QString val, int plr, int frame)
		: function(std::move(val)), engine(caller), baseobj(-1), baseobjtype(OBJ_NUM_TYPES), frameTime(frame + gameTime), ms(frame), player(plr), calls(0), type(TIMER_REPEAT) {}
	bool operator== (const timerNode &t)
	{
		return function == t.function && player == t.player;
	}
	// implement operator less TODO
};

#define MAX_US 20000
#define HALF_MAX_US 10000

/// List of timer events for scripts. Before running them, we sort the list then run as many as we have time for.
/// In this way, we implement load balancing of events and keep frame rates tidy for users. Since scripts run on the
/// host, we do not need to worry about each peer simulating the world differently.
static QList<timerNode> timers;

/// Scripting engine (what others call the scripting context, but QtScript's nomenclature is different).
static QList<QScriptEngine *> scripts;

/// Whether the scripts have been set up or not
static bool scriptsReady = false;

/// Structure for research events put on hold
struct researchEvent
{
	RESEARCH *research;
	STRUCTURE *structure;
	int player;

	researchEvent(RESEARCH *r, STRUCTURE *s, int p): research(r), structure(s), player(p) {}
};
/// Research events that are put on hold until the scripts are ready
static QQueue<struct researchEvent> eventQueue;

/// Remember what names are used internally in the scripting engine, we don't want to save these to the savegame
static std::set<QString> internalNamespace;

typedef struct monitor_bin
{
	int worst;
	uint32_t worstGameTime;
	int calls;
	int overMaxTimeCalls;
	int overHalfMaxTimeCalls;
	uint64_t time;
	monitor_bin() : worst(0),  worstGameTime(0), calls(0), overMaxTimeCalls(0), overHalfMaxTimeCalls(0), time(0) {}
} MONITOR_BIN;
typedef QHash<QString, MONITOR_BIN> MONITOR;
static QHash<QScriptEngine *, MONITOR *> monitors;
static QHash<QScriptEngine *, QStringList> eventNamespaces; // separate event namespaces for libraries

static MODELMAP models;
static QStandardItemModel *triggerModel;
static bool globalDialog = false;

static void updateGlobalModels();

void to_json(nlohmann::json& j, const QVariant& value); // forward-declare

bool bInTutorial = false;

// ----------------------------------------------------------

void doNotSaveGlobal(const QString &global)
{
	internalNamespace.insert(global);
}

// Call a function by name
static QScriptValue callFunction(QScriptEngine *engine, const QString &function, const QScriptValueList &args, bool event = true)
{
	if (event)
	{
		// recurse into variants, if any
		for (const QString &s : eventNamespaces[engine])
		{
			const QScriptValue &value = engine->globalObject().property(s + function);
			if (value.isValid() && value.isFunction())
			{
				callFunction(engine, s + function, args, event);
			}
		}
	}
	code_part level = event ? LOG_SCRIPT : LOG_ERROR;
	QScriptValue value = engine->globalObject().property(function);
	if (!value.isValid() || !value.isFunction())
	{
		// not necessarily an error, may just be a trigger that is not defined (ie not needed)
		// or it could be a typo in the function name or ...
		debug(level, "called function (%s) not defined", function.toUtf8().constData());
		return false;
	}
	QElapsedTimer timer;
	timer.start();
	QScriptValue result = value.call(QScriptValue(), args);
	int ticks = timer.nsecsElapsed() / 1000;
	MONITOR *monitor = monitors.value(engine); // pick right one for this engine
	MONITOR_BIN m;
	if (monitor->contains(function))
	{
		m = monitor->value(function);
	}
	if (ticks > MAX_US)
	{
		debug(LOG_SCRIPT, "%s took %dus at time %d", function.toUtf8().constData(), ticks, wzGetTicks());
		m.overMaxTimeCalls++;
	}
	else if (ticks > HALF_MAX_US)
	{
		m.overHalfMaxTimeCalls++;
	}
	m.calls++;
	if (ticks > m.worst)
	{
		m.worst = ticks;
		m.worstGameTime = gameTime;
	}
	m.time += ticks;
	monitor->insert(function, m);
	if (engine->hasUncaughtException())
	{
		int line = engine->uncaughtExceptionLineNumber();
		QStringList bt = engine->uncaughtExceptionBacktrace();
		for (int i = 0; i < bt.size(); i++)
		{
			debug(LOG_ERROR, "%d : %s", i, bt.at(i).toUtf8().constData());
		}
		ASSERT(false, "Uncaught exception calling function \"%s\" at line %d: %s",
		       function.toUtf8().constData(), line, result.toString().toUtf8().constData());
		engine->clearExceptions();
		return QScriptValue();
	}
	return result;
}

//-- ## setTimer(function, milliseconds[, object])
//--
//-- Set a function to run repeated at some given time interval. The function to run
//-- is the first parameter, and it _must be quoted_, otherwise the function will
//-- be inlined. The second parameter is the interval, in milliseconds. A third, optional
//-- parameter can be a **game object** to pass to the timer function. If the **game object**
//-- dies, the timer stops running. The minimum number of milliseconds is 100, but such
//-- fast timers are strongly discouraged as they may deteriorate the game performance.
//--
//-- ```javascript
//--   function conDroids()
//--   {
//--      ... do stuff ...
//--   }
//--   // call conDroids every 4 seconds
//--   setTimer("conDroids", 4000);
//-- ```
//--
static QScriptValue js_setTimer(QScriptContext *context, QScriptEngine *engine)
{
	SCRIPT_ASSERT(context, context->argument(0).isString(), "Timer functions must be quoted");
	QString funcName = context->argument(0).toString();
	QScriptValue ms = context->argument(1);
	int player = engine->globalObject().property("me").toInt32();
	timerNode node(engine, funcName, player, ms.toInt32());
	QScriptValue value = engine->globalObject().property(funcName); // check existence
	SCRIPT_ASSERT(context, value.isValid() && value.isFunction(), "No such function: %s",
	              funcName.toUtf8().constData());
	if (context->argumentCount() == 3)
	{
		QScriptValue obj = context->argument(2);
		if (obj.isString())
		{
			node.stringarg = obj.toString();
		}
		else // is game object
		{
			node.baseobj = obj.property("id").toInt32();
			node.baseobjtype = (OBJECT_TYPE)obj.property("type").toInt32();
		}
	}
	node.type = TIMER_REPEAT;
	timers.push_back(node);
	return QScriptValue();
}

//-- ## removeTimer(function)
//--
//-- Removes an existing timer. The first parameter is the function timer to remove,
//-- and its name _must be quoted_.
//--
static QScriptValue js_removeTimer(QScriptContext *context, QScriptEngine *engine)
{
	SCRIPT_ASSERT(context, context->argument(0).isString(), "Timer functions must be quoted");
	QString function = context->argument(0).toString();
	int player = engine->globalObject().property("me").toInt32();
	int i, size = timers.size();
	for (i = 0; i < size; ++i)
	{
		timerNode node = timers.at(i);
		if (node.function == function && node.player == player)
		{
			timers.removeAt(i);
			break;
		}
	}
	if (i == size)
	{
		// Friendly warning
		QString warnName = function.left(15) + "...";
		debug(LOG_ERROR, "Did not find timer %s to remove", warnName.toUtf8().constData());
	}
	return QScriptValue();
}

//-- ## queue(function[, milliseconds[, object]])
//--
//-- Queues up a function to run at a later game frame. This is useful to prevent
//-- stuttering during the game, which can happen if too much script processing is
//-- done at once.  The function to run is the first parameter, and it
//-- _must be quoted_, otherwise the function will be inlined.
//-- The second parameter is the delay in milliseconds, if it is omitted or 0,
//-- the function will be run at a later frame.  A third optional
//-- parameter can be a **game object** to pass to the queued function. If the **game object**
//-- dies before the queued call runs, nothing happens.
//--
// TODO, check if an identical call is already queued up - and in this case,
// do not add anything.
static QScriptValue js_queue(QScriptContext *context, QScriptEngine *engine)
{
	SCRIPT_ASSERT(context, context->argument(0).isString(), "Queued functions must be quoted");
	QString funcName = context->argument(0).toString();
	QScriptValue value = engine->globalObject().property(funcName); // check existence
	SCRIPT_ASSERT(context, value.isValid() && value.isFunction(), "No such function: %s",
	              funcName.toUtf8().constData());
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
		if (obj.isString())
		{
			node.stringarg = obj.toString();
		}
		else // is game object
		{
			node.baseobj = obj.property("id").toInt32();
			node.baseobjtype = (OBJECT_TYPE)obj.property("type").toInt32();
		}
	}
	node.type = TIMER_ONESHOT_READY;
	timers.push_back(node);
	return QScriptValue();
}

//-- ## profile(function[, arguments])
//-- Calls a function with given arguments, measures time it took to evaluate the function,
//-- and adds this time to performance monitor statistics. Transparently returns the
//-- function's return value. The function to run is the first parameter, and it
//-- _must be quoted_. (3.2+ only)
//--
static QScriptValue js_profile(QScriptContext *context, QScriptEngine *engine)
{
	SCRIPT_ASSERT(context, context->argument(0).isString(), "Profiled functions must be quoted");
	QString funcName = context->argument(0).toString();
	QScriptValueList args;
	for (int i = 1; i < context->argumentCount(); ++i)
	{
		args.push_back(context->argument(i));
	}
	return callFunction(engine, funcName, args);
}

void scriptRemoveObject(BASE_OBJECT *psObj)
{
	// Weed out timers with dead objects
	for (int i = 0; i < timers.count();)
	{
		const timerNode node = timers.at(i);
		if (node.baseobj == psObj->id)
		{
			timers.removeAt(i);
		}
		else
		{
			i++;
		}
	}
	groupRemoveObject(psObj);
}

//-- ## namespace(prefix)
//-- Registers a new event namespace. All events can now have this prefix. This is useful for
//-- code libraries, to implement event that do not conflict with events in main code. This
//-- function should be called from global; do not (for hopefully obvious reasons) put it
//-- inside an event.
//--
static QScriptValue js_namespace(QScriptContext *context, QScriptEngine *engine)
{
	QString prefix(context->argument(0).toString());
	eventNamespaces[engine].append(prefix);
	return QScriptValue(true);
}

//-- ## include(file)
//-- Includes another source code file at this point. You should generally only specify the filename,
//-- not try to specify its path, here.
//--
static QScriptValue js_include(QScriptContext *context, QScriptEngine *engine)
{
	QString basePath = engine->globalObject().property("scriptPath").toString();
	QFileInfo basename(context->argument(0).toString());
	QString path = basePath + "/" + basename.fileName();
	// allow users to use subdirectories too
	if (PHYSFS_exists(basename.filePath().toUtf8().constData()))
	{
		path = basename.filePath(); // use this path instead (from read-only dir)
	}
	else if (PHYSFS_exists(QString("scripts/" + basename.filePath()).toUtf8().constData()))
	{
		path = "scripts/" + basename.filePath(); // use this path instead (in user write dir)
	}
	UDWORD size;
	char *bytes = nullptr;
	if (!loadFile(path.toUtf8().constData(), &bytes, &size))
	{
		debug(LOG_ERROR, "Failed to read include file \"%s\" (path=%s, name=%s)",
		      path.toUtf8().constData(), basePath.toUtf8().constData(), basename.filePath().toUtf8().constData());
		return QScriptValue(false);
	}
	QString source = QString::fromUtf8(bytes, size);
	free(bytes);
	QScriptSyntaxCheckResult syntax = QScriptEngine::checkSyntax(source);
	if (syntax.state() != QScriptSyntaxCheckResult::Valid)
	{
		debug(LOG_ERROR, "Syntax error in include %s line %d: %s",
		      path.toUtf8().constData(), syntax.errorLineNumber(), syntax.errorMessage().toUtf8().constData());
		return QScriptValue(false);
	}
	context->setActivationObject(engine->globalObject());
	context->setThisObject(engine->globalObject());
	QScriptValue result = engine->evaluate(source, path);
	if (engine->hasUncaughtException())
	{
		int line = engine->uncaughtExceptionLineNumber();
		debug(LOG_ERROR, "Uncaught exception at line %d, include file %s: %s",
		      line, path.toUtf8().constData(), result.toString().toUtf8().constData());
		return QScriptValue(false);
	}
	debug(LOG_SCRIPT, "Included new script file %s", path.toUtf8().constData());
	return QScriptValue(true);
}

// do not want to call this 'init', since scripts are often loaded before we get here
bool prepareScripts(bool loadGame)
{
	debug(LOG_WZ, "Scripts prepared");
	if (!loadGame) // labels saved in savegame
	{
		prepareLabels();
	}
	// Assume that by this point all scripts are loaded
	scriptsReady = true;
	while (!eventQueue.isEmpty())
	{
		researchEvent resEvent = eventQueue.dequeue();
		triggerEventResearched(resEvent.research, resEvent.structure, resEvent.player);
	}
	return true;
}

bool initScripts()
{
	return true;
}

bool shutdownScripts()
{
	scriptsReady = false;
	jsDebugShutdown();
	globalDialog = false;
	models.clear();
	triggerModel = nullptr;
	for (auto *engine : scripts)
	{
		MONITOR *monitor = monitors.value(engine);
		QString scriptName = engine->globalObject().property("scriptName").toString();
		int me = engine->globalObject().property("me").toInt32();
		dumpScriptLog(scriptName, me, "=== PERFORMANCE DATA ===\n");
		dumpScriptLog(scriptName, me, "    calls | avg (usec) | worst (usec) | worst call at | >=limit | >=limit/2 | function\n");
		for (MONITOR::const_iterator iter = monitor->constBegin(); iter != monitor->constEnd(); ++iter)
		{
			const QString& function = iter.key();
			MONITOR_BIN m = iter.value();
			QString info = QString("%1 | %2 | %3 | %4 | %5 | %6 | %7\n")
			               .arg(m.calls, 9).arg(m.time / m.calls, 10).arg(m.worst, 12)
			               .arg(m.worstGameTime, 13).arg(m.overMaxTimeCalls, 7)
			               .arg(m.overHalfMaxTimeCalls, 9).arg(function);
			dumpScriptLog(scriptName, me, info);
		}
		monitor->clear();
		delete monitor;
		unregisterFunctions(engine);
	}
	timers.clear();
	internalNamespace.clear();
	monitors.clear();
	while (!scripts.isEmpty())
	{
		delete scripts.takeFirst();
	}
	return true;
}

bool updateScripts()
{
	// Call delayed triggers here
	if (selectionChanged)
	{
		for (auto *engine : scripts)
		{
			QScriptValueList args;
			args += js_enumSelected(nullptr, engine);
			callFunction(engine, "eventSelectionChanged", args);
		}
		selectionChanged = false;
	}

	// Update gameTime
	for (auto *engine : scripts)
	{
		engine->globalObject().setProperty("gameTime", gameTime, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	}
	// Weed out dead timers
	for (int i = 0; i < timers.count();)
	{
		const timerNode node = timers.at(i);
		if (node.type == TIMER_ONESHOT_DONE)
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
	for (iter = timers.begin(); iter != timers.end(); iter++)
	{
		if (iter->frameTime <= gameTime)
		{
			iter->frameTime = iter->ms + gameTime;	// update for next invokation
			if (iter->type == TIMER_ONESHOT_READY)
			{
				iter->type = TIMER_ONESHOT_DONE; // unless there is none
			}
			iter->calls++;
			runlist.append(*iter);
		}
	}
	for (iter = runlist.begin(); iter != runlist.end(); iter++)
	{
		QScriptValueList args;
		if (iter->baseobj > 0)
		{
			args += convMax(IdToObject(iter->baseobjtype, iter->baseobj, iter->player), iter->engine);
		}
		else if (!iter->stringarg.isEmpty())
		{
			args += iter->stringarg;
		}
		callFunction(iter->engine, iter->function, args, true);
	}

	if (globalDialog && doUpdateModels)
	{
		updateGlobalModels();
		doUpdateModels = false;
	}

	return true;
}

uint32_t ScriptMapData::crcSumStructures(uint32_t crc) const
{
	for (auto &o : structures)
	{
		crc = crcSum(crc, o.name.toUtf8().data(), o.name.toUtf8().length());
		crc = crcSumVector2i(crc, &o.position, 1);
		crc = crcSumU16(crc, &o.direction, 1);
		crc = crcSum(crc, &o.modules, 1);
		crc = crcSum(crc, &o.player, 1);
	}
	return crc;
}

uint32_t ScriptMapData::crcSumDroids(uint32_t crc) const
{
	for (auto &o : droids)
	{
		crc = crcSum(crc, o.name.toUtf8().data(), o.name.toUtf8().length());
		crc = crcSumVector2i(crc, &o.position, 1);
		crc = crcSumU16(crc, &o.direction, 1);
		crc = crcSum(crc, &o.player, 1);
	}
	return crc;
}

uint32_t ScriptMapData::crcSumFeatures(uint32_t crc) const
{
	for (auto &o : features)
	{
		crc = crcSum(crc, o.name.toUtf8().data(), o.name.toUtf8().length());
		crc = crcSumVector2i(crc, &o.position, 1);
		crc = crcSumU16(crc, &o.direction, 1);
	}
	return crc;
}

ScriptMapData runMapScript(WzString const &path, uint64_t seed, bool preview)
{
	ScriptMapData data;
	data.valid = false;
	data.mt = MersenneTwister(seed);

	auto engine = std::unique_ptr<QScriptEngine>(new QScriptEngine());
	//auto engine = std::make_unique<QScriptEngine>();
	engine->setProcessEventsInterval(-1);
	UDWORD size;
	char *bytes = nullptr;
	if (!loadFile(path.toUtf8().c_str(), &bytes, &size))
	{
		debug(LOG_ERROR, "Failed to read script file \"%s\"", path.toUtf8().c_str());
		return data;
	}
	QString source = QString::fromUtf8(bytes, size);
	free(bytes);
	QScriptSyntaxCheckResult syntax = QScriptEngine::checkSyntax(source);
	ASSERT_OR_RETURN(data, syntax.state() == QScriptSyntaxCheckResult::Valid, "Syntax error in %s line %d: %s",
	                 path.toUtf8().c_str(), syntax.errorLineNumber(), syntax.errorMessage().toUtf8().constData());
	engine->globalObject().setProperty("preview", preview, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("XFLIP", TILE_XFLIP, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("YFLIP", TILE_YFLIP, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("ROTMASK", TILE_ROTMASK, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("ROTSHIFT", TILE_ROTSHIFT, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("TRIFLIP", TILE_TRIFLIP, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//engine->globalObject().setProperty("players", players, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("gameRand", engine->newFunction([](QScriptContext *context, QScriptEngine *, void *_data) -> QScriptValue {
		auto &data = *(ScriptMapData *)_data;
		uint32_t num = data.mt.u32();
		uint32_t mod = context->argument(0).toUInt32();
		return mod? num%mod : num;
	}, &data), QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("log", engine->newFunction([](QScriptContext *context, QScriptEngine *, void *_data) -> QScriptValue {
		auto &data = *(ScriptMapData *)_data;
		(void)data;
		auto str = context->argument(0).toString();
		debug(LOG_INFO, "game.js: \"%s\"", str.toUtf8().constData());
		return {};
	}, &data), QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("setMapData", engine->newFunction([](QScriptContext *context, QScriptEngine *, void *_data) -> QScriptValue {
		auto &data = *(ScriptMapData *)_data;
		data.valid = false;
		auto mapWidth = context->argument(0);
		auto mapHeight = context->argument(1);
		auto texture = context->argument(2);
		auto height = context->argument(3);
		auto structures = context->argument(4);
		auto droids = context->argument(5);
		auto features = context->argument(6);
		ASSERT_OR_RETURN(false, mapWidth.isNumber(), "mapWidth must be number");
		ASSERT_OR_RETURN(false, mapHeight.isNumber(), "mapHeight must be number");
		ASSERT_OR_RETURN(false, texture.isArray(), "texture must be array");
		ASSERT_OR_RETURN(false, height.isArray(), "height must be array");
		ASSERT_OR_RETURN(false, structures.isArray(), "structures must be array");
		ASSERT_OR_RETURN(false, droids.isArray(), "droids must be array");
		ASSERT_OR_RETURN(false, features.isArray(), "features must be array");
		data.mapWidth = mapWidth.toInt32();
		data.mapHeight = mapHeight.toInt32();
		ASSERT_OR_RETURN(false, data.mapWidth > 1 && data.mapHeight > 1 && (uint64_t)data.mapWidth*data.mapHeight <= 65536, "Map size out of bounds");
		size_t N = (size_t)data.mapWidth*data.mapHeight;
		data.texture.resize(N);
		data.height.resize(N);
		for (size_t n = 0; n < N; ++n)
		{
			data.texture[n] = texture.property(n).toUInt16();
			data.height[n] = height.property(n).toInt32();
		}
		uint16_t structureCount = structures.property("length").toUInt16();
		for (unsigned i = 0; i < structureCount; ++i) {
			auto structure = structures.property(i);
			auto position = structure.property("position");
			ScriptMapData::Structure sd;
			sd.name = structure.property("name").toString().toUtf8().constData();
			sd.position = {position.property(0).toInt32(), position.property(1).toInt32()};
			sd.direction = structure.property("direction").toInt32();
			sd.modules = structure.property("modules").toUInt32();
			sd.player = structure.property("player").toInt32();
			if (sd.player < -1 || sd.player >= MAX_PLAYERS) {
				ASSERT(false, "Invalid player");
				continue;
			}
			data.structures.push_back(std::move(sd));
		}
		uint16_t droidCount = droids.property("length").toUInt16();
		for (unsigned i = 0; i < droidCount; ++i) {
			auto droid = droids.property(i);
			auto position = droid.property("position");
			ScriptMapData::Droid sd;
			sd.name = droid.property("name").toString().toUtf8().constData();
			sd.position = {position.property(0).toInt32(), position.property(1).toInt32()};
			sd.direction = droid.property("direction").toInt32();
			sd.player = droid.property("player").toInt32();
			if (sd.player < -1 || sd.player >= MAX_PLAYERS) {
				ASSERT(false, "Invalid player");
				continue;
			}
			data.droids.push_back(std::move(sd));
		}
		uint16_t featureCount = features.property("length").toUInt16();
		for (unsigned i = 0; i < featureCount; ++i) {
			auto feature = features.property(i);
			auto position = feature.property("position");
			ScriptMapData::Feature sd;
			sd.name = feature.property("name").toString().toUtf8().constData();
			sd.position = {position.property(0).toInt32(), position.property(1).toInt32()};
			sd.direction = feature.property("direction").toInt32();
			data.features.push_back(std::move(sd));
		}
		data.valid = true;
		return true;
	}, &data), QScriptValue::ReadOnly | QScriptValue::Undeletable);

	QScriptValue result = engine->evaluate(source, QString::fromUtf8(path.toUtf8().c_str()));
	ASSERT_OR_RETURN(data, !engine->hasUncaughtException(), "Uncaught exception at line %d, file %s: %s",
	                 engine->uncaughtExceptionLineNumber(), path.toUtf8().c_str(), result.toString().toUtf8().constData());

	return data;
}

QScriptEngine *loadPlayerScript(WzString const &path, int player, AIDifficulty difficulty)
{
	ASSERT_OR_RETURN(nullptr, player < MAX_PLAYERS, "Player index %d out of bounds", player);
	QScriptEngine *engine = new QScriptEngine();
	// Set processEventsInterval to -1 because the interpreter should *never* call
	// QCoreApplication::processEvents() (or SDL builds will break in various ways).
	engine->setProcessEventsInterval(-1);
	UDWORD size;
	char *bytes = nullptr;
	if (!loadFile(path.toUtf8().c_str(), &bytes, &size))
	{
		debug(LOG_ERROR, "Failed to read script file \"%s\"", path.toUtf8().c_str());
		return nullptr;
	}
	QString source = QString::fromUtf8(bytes, size);
	free(bytes);
	QScriptSyntaxCheckResult syntax = QScriptEngine::checkSyntax(source);
	ASSERT_OR_RETURN(nullptr, syntax.state() == QScriptSyntaxCheckResult::Valid, "Syntax error in %s line %d: %s",
	                 path.toUtf8().c_str(), syntax.errorLineNumber(), syntax.errorMessage().toUtf8().constData());
	// Special functions
	engine->globalObject().setProperty("setTimer", engine->newFunction(js_setTimer));
	engine->globalObject().setProperty("queue", engine->newFunction(js_queue));
	engine->globalObject().setProperty("removeTimer", engine->newFunction(js_removeTimer));
	engine->globalObject().setProperty("profile", engine->newFunction(js_profile));
	engine->globalObject().setProperty("include", engine->newFunction(js_include));
	engine->globalObject().setProperty("namespace", engine->newFunction(js_namespace));

	// Special global variables
	//== * ```version``` Current version of the game, set in *major.minor* format.
	engine->globalObject().setProperty("version", QString(version_getVersionString()), QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//== * ```selectedPlayer``` The player controlled by the client on which the script runs.
	engine->globalObject().setProperty("selectedPlayer", selectedPlayer, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//== * ```gameTime``` The current game time. Updated before every invokation of a script.
	engine->globalObject().setProperty("gameTime", gameTime, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//== * ```modList``` The current loaded mods.
	engine->globalObject().setProperty("modList", QString(getModList().c_str()), QScriptValue::ReadOnly | QScriptValue::Undeletable);


	//== * ```difficulty``` The currently set campaign difficulty, or the current AI's difficulty setting. It will be one of
	//== ```EASY```, ```MEDIUM```, ```HARD``` or ```INSANE```.
	if (game.type == LEVEL_TYPE::SKIRMISH)
	{
		engine->globalObject().setProperty("difficulty", static_cast<int8_t>(difficulty), QScriptValue::ReadOnly | QScriptValue::Undeletable);
	}
	else // campaign
	{
		engine->globalObject().setProperty("difficulty", (int)getDifficultyLevel(), QScriptValue::ReadOnly | QScriptValue::Undeletable);
	}
	//== * ```mapName``` The name of the current map.
	engine->globalObject().setProperty("mapName", QString(game.map), QScriptValue::ReadOnly | QScriptValue::Undeletable);  // QString cast to work around bug in Qt5 QScriptValue(char *) constructor.
	//== * ```tilesetType``` The area name of the map.
	QString tilesetType("CUSTOM");
	if (strcmp(tilesetDir, "texpages/tertilesc1hw") == 0)
	{
		tilesetType = "ARIZONA";
	}
	else if (strcmp(tilesetDir, "texpages/tertilesc2hw") == 0)
	{
		tilesetType = "URBAN";
	}
	else if (strcmp(tilesetDir, "texpages/tertilesc3hw") == 0)
	{
		tilesetType = "ROCKIES";
	}
	engine->globalObject().setProperty("tilesetType", tilesetType, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//== * ```baseType``` The type of base that the game starts with. It will be one of ```CAMP_CLEAN```, ```CAMP_BASE``` or ```CAMP_WALLS```.
	engine->globalObject().setProperty("baseType", game.base, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//== * ```alliancesType``` The type of alliances permitted in this game. It will be one of ```NO_ALLIANCES```, ```ALLIANCES```, ```ALLIANCES_UNSHARED``` or ```ALLIANCES_TEAMS```.
	engine->globalObject().setProperty("alliancesType", game.alliance, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//== * ```powerType``` The power level set for this game.
	engine->globalObject().setProperty("powerType", game.power, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//== * ```maxPlayers``` The number of active players in this game.
	engine->globalObject().setProperty("maxPlayers", game.maxPlayers, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//== * ```scavengers``` Whether or not scavengers are activated in this game.
	engine->globalObject().setProperty("scavengers", game.scavengers, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//== * ```mapWidth``` Width of map in tiles.
	engine->globalObject().setProperty("mapWidth", mapWidth, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//== * ```mapHeight``` Height of map in tiles.
	engine->globalObject().setProperty("mapHeight", mapHeight, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//== * ```scavengerPlayer``` Index of scavenger player. (3.2+ only)
	engine->globalObject().setProperty("scavengerPlayer", scavengerSlot(), QScriptValue::ReadOnly | QScriptValue::Undeletable);
	//== * ```isMultiplayer``` If the current game is a online multiplayer game or not. (3.2+ only)
	engine->globalObject().setProperty("isMultiplayer", NetPlay.bComms, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	// un-documented placeholder variable
	engine->globalObject().setProperty("isReceivingAllEvents", false, QScriptValue::ReadOnly | QScriptValue::Undeletable);

	// Regular functions
	QFileInfo basename(QString::fromUtf8(path.toUtf8().c_str()));
	registerFunctions(engine, basename.baseName());

	// Remember internal, reserved names
	QScriptValueIterator it(engine->globalObject());
	while (it.hasNext())
	{
		it.next();
		internalNamespace.insert(it.name());
	}

	// We need to always save the 'me' special variable.
	//== * ```me``` The player the script is currently running as.
	engine->globalObject().setProperty("me", player, QScriptValue::ReadOnly | QScriptValue::Undeletable);

	// We also need to save the special 'scriptName' variable.
	//== * ```scriptName``` Base name of the script that is running.
	engine->globalObject().setProperty("scriptName", basename.baseName(), QScriptValue::ReadOnly | QScriptValue::Undeletable);

	// We also need to save the special 'scriptPath' variable.
	//== * ```scriptPath``` Base path of the script that is running.
	engine->globalObject().setProperty("scriptPath", basename.path(), QScriptValue::ReadOnly | QScriptValue::Undeletable);

	QScriptValue result = engine->evaluate(source, QString::fromUtf8(path.toUtf8().c_str()));
	ASSERT_OR_RETURN(nullptr, !engine->hasUncaughtException(), "Uncaught exception at line %d, file %s: %s",
	                 engine->uncaughtExceptionLineNumber(), path.toUtf8().c_str(), result.toString().toUtf8().constData());

	// Register script
	scripts.push_back(engine);

	MONITOR *monitor = new MONITOR;
	monitors.insert(engine, monitor);

	debug(LOG_SAVE, "Created script engine %d for player %d from %s", scripts.size() - 1, player, path.toUtf8().c_str());
	return engine;
}

bool loadGlobalScript(WzString path)
{
	return loadPlayerScript(std::move(path), selectedPlayer, AIDifficulty::DISABLED);
}

bool saveScriptStates(const char *filename)
{
	WzConfig ini(filename, WzConfig::ReadAndWrite);
	for (int i = 0; i < scripts.size(); ++i)
	{
		QScriptEngine *engine = scripts.at(i);
		QScriptValueIterator it(engine->globalObject());
		ini.beginGroup("globals_" + WzString::number(i));
		// we save 'scriptName' and 'me' implicitly
		while (it.hasNext())
		{
			it.next();
			if (internalNamespace.count(it.name()) == 0 && !it.value().isFunction()
			    && !it.value().equals(engine->globalObject()))
			{
				ini.setValue(WzString::fromUtf8(it.name().toUtf8().constData()), it.value().toVariant());
			}
		}
		ini.endGroup();
		ini.beginGroup("groups_" + WzString::number(i));
		// we have to save 'scriptName' and 'me' explicitly
		ini.setValue("me", engine->globalObject().property("me").toInt32());
		ini.setValue("scriptName", QStringToWzString(engine->globalObject().property("scriptName").toString()));
		saveGroups(ini, engine);
		ini.endGroup();
	}
	for (int i = 0; i < timers.size(); ++i)
	{
		timerNode node = timers.at(i);
		ini.beginGroup("triggers_" + WzString::number(i));
		// we have to save 'scriptName' and 'me' explicitly
		ini.setValue("me", node.player);
		ini.setValue("scriptName", QStringToWzString(node.engine->globalObject().property("scriptName").toString()));
		ini.setValue("function", QStringToWzString(node.function));
		if (node.baseobj >= 0)
		{
			ini.setValue("object", node.baseobj);
		}
		ini.setValue("frame", node.frameTime);
		ini.setValue("type", (int)node.type);
		ini.setValue("ms", node.ms);
		ini.endGroup();
	}
	return true;
}

static QScriptEngine *findEngineForPlayer(int match, const QString& scriptName)
{
	for (auto *engine : scripts)
	{
		int player = engine->globalObject().property("me").toInt32();
		QString matchName = engine->globalObject().property("scriptName").toString();
		if (match == player && (matchName.compare(scriptName, Qt::CaseInsensitive) == 0 || scriptName.isEmpty()))
		{
			return engine;
		}
	}
	ASSERT(false, "Script context for player %d and script name %s not found",
	       match, scriptName.toUtf8().constData());
	return nullptr;
}

bool loadScriptStates(const char *filename)
{
	WzConfig ini(filename, WzConfig::ReadOnly);
	std::vector<WzString> list = ini.childGroups();
	debug(LOG_SAVE, "Loading script states for %d script contexts", scripts.size());
	for (size_t i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		int player = ini.value("me").toInt();
		QString scriptName = QString::fromUtf8(ini.value("scriptName").toWzString().toUtf8().c_str());
		QScriptEngine *engine = findEngineForPlayer(player, scriptName);
		if (engine && list[i].startsWith("triggers_"))
		{
			timerNode node;
			node.player = player;
			node.ms = ini.value("ms").toInt();
			node.frameTime = ini.value("frame").toInt();
			node.engine = engine;
			debug(LOG_SAVE, "Registering trigger %zu for player %d, script %s",
			      i, node.player, scriptName.toUtf8().constData());
			node.function = QString::fromUtf8(ini.value("function").toWzString().toUtf8().c_str());
			node.baseobj = ini.value("baseobj", -1).toInt();
			node.type = (timerType)ini.value("type", TIMER_REPEAT).toInt();
			timers.push_back(node);
		}
		else if (engine && list[i].startsWith("globals_"))
		{
			std::vector<WzString> keys = ini.childKeys();
			debug(LOG_SAVE, "Loading script globals for player %d, script %s -- found %zu values",
			      player, scriptName.toUtf8().constData(), keys.size());
			for (size_t j = 0; j < keys.size(); ++j)
			{
				// IMPORTANT: "null" JSON values *MUST* map to QScriptValue::UndefinedValue.
				//			  If they are set to QScriptValue::NullValue, it causes issues for libcampaign.js. (As the values become "defined".)
				//			  (mapJsonToQScriptValue handles this properly.)
				engine->globalObject().setProperty(QString::fromUtf8(keys.at(j).toUtf8().c_str()), mapJsonToQScriptValue(engine, ini.json(keys.at(j)), QScriptValue::PropertyFlags()));
			}
		}
		else if (engine && list[i].startsWith("groups_"))
		{
			std::vector<WzString> keys = ini.childKeys();
			for (size_t j = 0; j < keys.size(); ++j)
			{
				std::vector<WzString> values = ini.value(keys.at(j)).toWzStringList();
				bool ok = false; // check if number
				int droidId = keys.at(j).toInt(&ok);
				for (size_t k = 0; ok && k < values.size(); k++)
				{
					int groupId = values.at(k).toInt();
					loadGroup(engine, groupId, droidId);
				}
			}
		}
		else
		{
			if (engine)
			{
				debug(LOG_WARNING, "Encountered unexpected group '%s' in loadScriptStates", list[i].toUtf8().c_str());
			}
		}
		ini.endGroup();
	}
	return true;
}

static QStandardItemList addModelItem(QScriptValueIterator &it)
{
	QStandardItemList l;
	QStandardItem *key = new QStandardItem(it.name());
	QStandardItem *value = nullptr;

	if (it.value().isObject() || it.value().isArray())
	{
		QScriptValueIterator obit(it.value());
		while (obit.hasNext())
		{
			obit.next();
			key->appendRow(addModelItem(obit));
		}
		value = new QStandardItem("[Object]");
	}
	else
	{
		value = new QStandardItem(it.value().toString());
	}
	l += key;
	l += value;
	return l;
}

static void updateGlobalModels()
{
	for (auto *engine : scripts)
	{
		QScriptValueIterator it(engine->globalObject());
		QStandardItemModel *m = models.value(engine);
		m->setRowCount(0);

		while (it.hasNext())
		{
			it.next();
			if ((internalNamespace.count(it.name()) == 0 && !it.value().isFunction()
			     && !it.value().equals(engine->globalObject()))
			    || it.name() == "Upgrades" || it.name() == "Stats")
			{
				QStandardItemList list = addModelItem(it);
				m->appendRow(list);
			}
		}
	}
	QStandardItemModel *m = triggerModel;
	m->setRowCount(0);
	for (const auto &node : timers)
	{
		int nextRow = m->rowCount();
		m->setRowCount(nextRow);
		m->setItem(nextRow, 0, new QStandardItem(node.function));
		QString scriptName = node.engine->globalObject().property("scriptName").toString();
		m->setItem(nextRow, 1, new QStandardItem(scriptName + ":" + QString::number(node.player)));
		if (node.baseobj >= 0)
		{
			m->setItem(nextRow, 2, new QStandardItem(QString::number(node.baseobj)));
		}
		else
		{
			m->setItem(nextRow, 2, new QStandardItem("-"));
		}
		m->setItem(nextRow, 3, new QStandardItem(QString::number(node.frameTime)));
		m->setItem(nextRow, 4, new QStandardItem(QString::number(node.ms)));
		if (node.type == TIMER_ONESHOT_READY)
		{
			m->setItem(nextRow, 5, new QStandardItem("Oneshot"));
		}
		else if (node.type == TIMER_ONESHOT_DONE)
		{
			m->setItem(nextRow, 5, new QStandardItem("Done"));
		}
		else
		{
			m->setItem(nextRow, 5, new QStandardItem("Repeat"));
		}
		m->setItem(nextRow, 6, new QStandardItem(QString::number(node.calls)));
	}
}

bool jsEvaluate(QScriptEngine *engine, const QString &text)
{
	QScriptSyntaxCheckResult syntax = QScriptEngine::checkSyntax(text);
	if (syntax.state() != QScriptSyntaxCheckResult::Valid)
	{
		debug(LOG_ERROR, "Syntax error in %s: %s",
		      text.toUtf8().constData(), syntax.errorMessage().toUtf8().constData());
		return false;
	}
	QScriptValue result = engine->evaluate(text);
	if (engine->hasUncaughtException())
	{
		debug(LOG_ERROR, "Uncaught exception in %s: %s",
		      text.toUtf8().constData(), result.toString().toUtf8().constData());
		return false;
	}
	console("%s", result.toString().toUtf8().constData());
	return true;
}

void jsAutogameSpecific(const WzString &name, int player)
{
	QScriptEngine *engine = loadPlayerScript(name, player, AIDifficulty::MEDIUM);
	if (!engine)
	{
		console("Failed to load selected AI! Check your logs to see why.");
		return;
	}
	console("Loaded the %s AI script for current player!", name.toUtf8().c_str());
	callFunction(engine, "eventGameInit", QScriptValueList());
	callFunction(engine, "eventStartLevel", QScriptValueList());
}

void jsAutogame()
{
	QString srcPath(PHYSFS_getWriteDir());
	srcPath += PHYSFS_getDirSeparator();
	srcPath += "scripts";
	QString path = QFileDialog::getOpenFileName(nullptr, "Choose AI script to load", srcPath, "Javascript files (*.js)");
	QFileInfo basename(path);
	if (path.isEmpty())
	{
		console("No file specified");
		return;
	}
	jsAutogameSpecific(QStringToWzString("scripts/" + basename.fileName()), selectedPlayer);
}

void jsHandleDebugClosed()
{
	globalDialog = false;
}

void jsShowDebug()
{
	// Add globals
	for (auto *engine : scripts)
	{
		QStandardItemModel *m = new QStandardItemModel(0, 2);
		m->setHeaderData(0, Qt::Horizontal, QString("Name"));
		m->setHeaderData(1, Qt::Horizontal, QString("Value"));
		models.insert(engine, m);
	}
	// Add triggers
	triggerModel = new QStandardItemModel(0, 7);
	triggerModel->setHeaderData(0, Qt::Horizontal, QString("Function"));
	triggerModel->setHeaderData(1, Qt::Horizontal, QString("Script"));
	triggerModel->setHeaderData(2, Qt::Horizontal, QString("Object"));
	triggerModel->setHeaderData(3, Qt::Horizontal, QString("Time"));
	triggerModel->setHeaderData(4, Qt::Horizontal, QString("Interval"));
	triggerModel->setHeaderData(5, Qt::Horizontal, QString("Type"));
	triggerModel->setHeaderData(6, Qt::Horizontal, QString("Calls"));

	globalDialog = true;
	updateGlobalModels();
	jsDebugCreate(models, triggerModel, createLabelModel(), jsHandleDebugClosed);
}

// ----------------------------------------------------------------------------------------
// Events

/// For generic, parameter-less triggers
//__ ## eventGameInit()
//__
//__ An event that is run once as the game is initialized. Not all game state may have been
//__ properly initialized by this time, so use this only to initialize script state.
//__
//__ ## eventStartLevel()
//__
//__ An event that is run once the game has started and all game data has been loaded.
//__
//__ ## eventMissionTimeout()
//__
//__ An event that is run when the mission timer has run out.
//__
//__ ## eventVideoDone()
//__
//__ An event that is run when a video show stopped playing.
//__
//__ ## eventGameLoaded()
//__
//__ An event that is run when game is loaded from a saved game. There is usually no need to use this event.
//__
//__ ## eventGameSaving()
//__
//__ An event that is run before game is saved. There is usually no need to use this event.
//__
//__ ## eventGameSaved()
//__
//__ An event that is run after game is saved. There is usually no need to use this event.
//__
//__ ## eventDeliveryPointMoving()
//__
//__ An event that is run when the current player starts to move a delivery point.
//__
//__ ## eventDeliveryPointMoved()
//__
//__ An event that is run after the current player has moved a delivery point.
//__
//__ ## eventTransporterLaunch(transport)
//__
//__ An event that is run when the mission transporter has been ordered to fly off.
//__
//__ ## eventTransporterArrived(transport)
//__
//__ An event that is run when the mission transporter has arrived at the map edge with reinforcements.
//__
//__ ## eventTransporterExit(transport)
//__
//__ An event that is run when the mission transporter has left the map.
//__
//__ ## eventTransporterDone(transport)
//__
//__ An event that is run when the mission transporter has no more reinforcements to deliver.
//__
//__ ## eventTransporterLanded(transport)
//__
//__ An event that is run when the mission transporter has landed with reinforcements.
//__
//__ ## eventDesignBody()
//__
//__An event that is run when current user picks a body in the design menu.
//__
//__ ## eventDesignPropulsion()
//__
//__An event that is run when current user picks a propulsion in the design menu.
//__
//__ ## eventDesignWeapon()
//__
//__An event that is run when current user picks a weapon in the design menu.
//__
//__ ## eventDesignCommand()
//__
//__An event that is run when current user picks a command turret in the design menu.
//__
//__ ## eventDesignSystem()
//__
//__An event that is run when current user picks a system other than command turret in the design menu.
//__
//__ ## eventDesignQuit()
//__
//__An event that is run when current user leaves the design menu.
//__
//__ ## eventMenuBuildSelected()
//__
//__An event that is run when current user picks something new in the build menu.
//__
//__ ## eventMenuBuild()
//__
//__An event that is run when current user opens the build menu.
//__
//__ ## eventMenuResearch()
//__
//__An event that is run when current user opens the research menu.
//__
//__ ## eventMenuManufacture()
//__An event that is run when current user opens the manufacture menu.
//__
bool triggerEvent(SCRIPT_TRIGGER_TYPE trigger, BASE_OBJECT *psObj)
{
	// HACK: TRIGGER_VIDEO_QUIT is called before scripts for initial campaign video
	ASSERT(scriptsReady || trigger == TRIGGER_VIDEO_QUIT, "Scripts not initialized yet");
	for (auto *engine : scripts)
	{
		QScriptValueList args;

		if (psObj)
		{
			int player = engine->globalObject().property("me").toInt32();
			bool receiveAll = engine->globalObject().property("isReceivingAllEvents").toBool();
			if (player != psObj->player && !receiveAll)
			{
				continue;
			}
			args += convMax(psObj, engine);
		}

		switch (trigger)
		{
		case TRIGGER_GAME_INIT:
			callFunction(engine, "eventGameInit", QScriptValueList());
			break;
		case TRIGGER_START_LEVEL:
			processVisibility(); // make sure we initialize visibility first
			callFunction(engine, "eventStartLevel", QScriptValueList());
			break;
		case TRIGGER_TRANSPORTER_LAUNCH:
			callFunction(engine, "eventLaunchTransporter", QScriptValueList()); // deprecated!
			callFunction(engine, "eventTransporterLaunch", args);
			break;
		case TRIGGER_TRANSPORTER_ARRIVED:
			callFunction(engine, "eventReinforcementsArrived", QScriptValueList()); // deprecated!
			callFunction(engine, "eventTransporterArrived", args);
			break;
		case TRIGGER_DELIVERY_POINT_MOVING:
			callFunction(engine, "eventDeliveryPointMoving", args);
			break;
		case TRIGGER_DELIVERY_POINT_MOVED:
			callFunction(engine, "eventDeliveryPointMoved", args);
			break;
		case TRIGGER_OBJECT_RECYCLED:
			callFunction(engine, "eventObjectRecycled", args);
			break;
		case TRIGGER_TRANSPORTER_EXIT:
			callFunction(engine, "eventTransporterExit", args);
			break;
		case TRIGGER_TRANSPORTER_DONE:
			callFunction(engine, "eventTransporterDone", args);
			break;
		case TRIGGER_TRANSPORTER_LANDED:
			callFunction(engine, "eventTransporterLanded", args);
			break;
		case TRIGGER_MISSION_TIMEOUT:
			callFunction(engine, "eventMissionTimeout", QScriptValueList());
			break;
		case TRIGGER_VIDEO_QUIT:
			callFunction(engine, "eventVideoDone", QScriptValueList());
			break;
		case TRIGGER_GAME_LOADED:
			callFunction(engine, "eventGameLoaded", QScriptValueList());
			break;
		case TRIGGER_GAME_SAVING:
			callFunction(engine, "eventGameSaving", QScriptValueList());
			break;
		case TRIGGER_GAME_SAVED:
			callFunction(engine, "eventGameSaved", QScriptValueList());
			break;
		case TRIGGER_DESIGN_BODY:
			callFunction(engine, "eventDesignBody", QScriptValueList());
			break;
		case TRIGGER_DESIGN_PROPULSION:
			callFunction(engine, "eventDesignPropulsion", QScriptValueList());
			break;
		case TRIGGER_DESIGN_WEAPON:
			callFunction(engine, "eventDesignWeapon", QScriptValueList());
			break;
		case TRIGGER_DESIGN_COMMAND:
			callFunction(engine, "eventDesignCommand", QScriptValueList());
			break;
		case TRIGGER_DESIGN_SYSTEM:
			callFunction(engine, "eventDesignSystem", QScriptValueList());
			break;
		case TRIGGER_DESIGN_QUIT:
			callFunction(engine, "eventDesignQuit", QScriptValueList());
			break;
		case TRIGGER_MENU_BUILD_SELECTED:
			callFunction(engine, "eventMenuBuildSelected", args);
			break;
		case TRIGGER_MENU_RESEARCH_SELECTED:
			callFunction(engine, "eventMenuResearchSelected", args);
			break;
		case TRIGGER_MENU_BUILD_UP:
			args += QScriptValue(true);
			callFunction(engine, "eventMenuBuild", args);
			break;
		case TRIGGER_MENU_RESEARCH_UP:
			args += QScriptValue(true);
			callFunction(engine, "eventMenuResearch", args);
			break;
		case TRIGGER_MENU_DESIGN_UP:
			args += QScriptValue(true);
			callFunction(engine, "eventMenuDesign", args);
			break;
		case TRIGGER_MENU_MANUFACTURE_UP:
			args += QScriptValue(true);
			callFunction(engine, "eventMenuManufacture", args);
			break;
		}
	}

	if ((trigger == TRIGGER_START_LEVEL || trigger == TRIGGER_GAME_LOADED) && !saveandquit_enabled().empty())
	{
		saveGame(saveandquit_enabled().c_str(), GTYPE_SAVE_START);
		exit(0);
	}

	return true;
}

//__ ## eventPlayerLeft(player index)
//__
//__ An event that is run after a player has left the game.
//__
bool triggerEventPlayerLeft(int id)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *engine : scripts)
	{
		QScriptValueList args;
		args += id;
		callFunction(engine, "eventPlayerLeft", args);
	}
	return true;
}

//__ ## eventCheatMode(entered)
//__
//__ Game entered or left cheat/debug mode.
//__ The entered parameter is true if cheat mode entered, false otherwise.
//__
bool triggerEventCheatMode(bool entered)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *engine : scripts)
	{
		QScriptValueList args;
		args += entered;
		callFunction(engine, "eventCheatMode", args);
	}
	return true;
}

//__ ## eventDroidIdle(droid)
//__
//__ A droid should be given new orders.
//__
bool triggerEventDroidIdle(DROID *psDroid)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *engine : scripts)
	{
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

//__ ## eventDroidBuilt(droid[, structure])
//__
//__ An event that is run every time a droid is built. The structure parameter is set
//__ if the droid was produced in a factory. It is not triggered for droid theft or
//__ gift (check ```eventObjectTransfer``` for that).
//__
bool triggerEventDroidBuilt(DROID *psDroid, STRUCTURE *psFactory)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *engine : scripts)
	{
		int player = engine->globalObject().property("me").toInt32();
		bool receiveAll = engine->globalObject().property("isReceivingAllEvents").toBool();
		if (player == psDroid->player || receiveAll)
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

//__ ## eventStructureBuilt(structure[, droid])
//__
//__ An event that is run every time a structure is produced. The droid parameter is set
//__ if the structure was built by a droid. It is not triggered for building theft
//__ (check ```eventObjectTransfer``` for that).
//__
bool triggerEventStructBuilt(STRUCTURE *psStruct, DROID *psDroid)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *engine : scripts)
	{
		int player = engine->globalObject().property("me").toInt32();
		bool receiveAll = engine->globalObject().property("isReceivingAllEvents").toBool();
		if (player == psStruct->player || receiveAll)
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

//__ ## eventStructureDemolish(structure[, droid])
//__
//__ An event that is run every time a structure begins to be demolished. This does
//__ not trigger again if the structure is partially demolished.
//__
bool triggerEventStructDemolish(STRUCTURE *psStruct, DROID *psDroid)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *engine : scripts)
	{
		int player = engine->globalObject().property("me").toInt32();
		bool receiveAll = engine->globalObject().property("isReceivingAllEvents").toBool();
		if (player == psStruct->player || receiveAll)
		{
			QScriptValueList args;
			args += convStructure(psStruct, engine);
			if (psDroid)
			{
				args += convDroid(psDroid, engine);
			}
			callFunction(engine, "eventStructureDemolish", args);
		}
	}
	return true;
}

//__ ## eventStructureReady(structure)
//__
//__ An event that is run every time a structure is ready to perform some
//__ special ability. It will only fire once, so if the time is not right,
//__ register your own timer to keep checking.
//__
bool triggerEventStructureReady(STRUCTURE *psStruct)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *engine : scripts)
	{
		int player = engine->globalObject().property("me").toInt32();
		bool receiveAll = engine->globalObject().property("isReceivingAllEvents").toBool();
		if (player == psStruct->player || receiveAll)
		{
			QScriptValueList args;
			args += convStructure(psStruct, engine);
			callFunction(engine, "eventStructureReady", args);
		}
	}
	return true;
}

//__ ## eventAttacked(victim, attacker)
//__
//__ An event that is run when an object belonging to the script's controlling player is
//__ attacked. The attacker parameter may be either a structure or a droid.
//__
bool triggerEventAttacked(BASE_OBJECT *psVictim, BASE_OBJECT *psAttacker, int lastHit)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
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
	for (auto *engine : scripts)
	{
		int player = engine->globalObject().property("me").toInt32();
		bool receiveAll = engine->globalObject().property("isReceivingAllEvents").toBool();
		if (player == psVictim->player || receiveAll)
		{
			QScriptValueList args;
			args += convMax(psVictim, engine);
			args += convMax(psAttacker, engine);
			callFunction(engine, "eventAttacked", args);
		}
	}
	return true;
}

//__ ## eventResearched(research, structure, player)
//__
//__ An event that is run whenever a new research is available. The structure
//__ parameter is set if the research comes from a research lab owned by the
//__ current player. If an ally does the research, the structure parameter will
//__ be set to null. The player parameter gives the player it is called for.
//__
bool triggerEventResearched(RESEARCH *psResearch, STRUCTURE *psStruct, int player)
{
	//HACK: This event can be triggered when loading savegames, before the script engines are initialized.
	// if this is the case, we need to store these events and replay them later
	if (!scriptsReady)
	{
		eventQueue.enqueue(researchEvent(psResearch, psStruct, player));
		return true;
	}
	for (auto *engine : scripts)
	{
		int me = engine->globalObject().property("me").toInt32();
		bool receiveAll = engine->globalObject().property("isReceivingAllEvents").toBool();
		if (me == player || receiveAll)
		{
			QScriptValueList args;
			args += convResearch(psResearch, engine, player);
			if (psStruct)
			{
				args += convStructure(psStruct, engine);
			}
			else
			{
				args += QScriptValue::NullValue;
			}
			args += QScriptValue(player);
			callFunction(engine, "eventResearched", args);
		}
	}
	return true;
}

//__ ## eventDestroyed(object)
//__
//__ An event that is run whenever an object is destroyed. Careful passing
//__ the parameter object around, since it is about to vanish!
//__
bool triggerEventDestroyed(BASE_OBJECT *psVictim)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (int i = 0; i < scripts.size() && psVictim; ++i)
	{
		QScriptEngine *engine = scripts.at(i);
		QScriptValueList args;
		args += convMax(psVictim, engine);
		callFunction(engine, "eventDestroyed", args);
	}
	return true;
}

//__ ## eventPickup(feature, droid)
//__
//__ An event that is run whenever a feature is picked up. It is called for
//__ all players / scripts.
//__ Careful passing the parameter object around, since it is about to vanish! (3.2+ only)
//__
bool triggerEventPickup(FEATURE *psFeat, DROID *psDroid)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *engine : scripts)
	{
		QScriptValueList args;
		args += convFeature(psFeat, engine);
		args += convDroid(psDroid, engine);
		callFunction(engine, "eventPickup", args);
	}
	return true;
}

//__ ## eventObjectSeen(viewer, seen)
//__
//__ An event that is run sometimes when an object, which was marked by an object label,
//__ which was reset through resetLabel() to subscribe for events, goes from not seen to seen.
//__ An event that is run sometimes when an objectm  goes from not seen to seen.
//__ First parameter is **game object** doing the seeing, the next the game
//__ object being seen.
//__
//__ ## eventGroupSeen(viewer, group)
//__
//__ An event that is run sometimes when a member of a group, which was marked by a group label,
//__ which was reset through resetLabel() to subscribe for events, goes from not seen to seen.
//__ First parameter is **game object** doing the seeing, the next the id of the group
//__ being seen.
//__
bool triggerEventSeen(BASE_OBJECT *psViewer, BASE_OBJECT *psSeen)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (int i = 0; i < scripts.size() && psSeen && psViewer; ++i)
	{
		QScriptEngine *engine = scripts.at(i);
		std::pair<bool, int> callbacks = seenLabelCheck(engine, psSeen, psViewer);
		if (callbacks.first)
		{
			QScriptValueList args;
			args += convMax(psViewer, engine);
			args += convMax(psSeen, engine);
			callFunction(engine, "eventObjectSeen", args);
		}
		if (callbacks.second)
		{
			QScriptValueList args;
			args += convMax(psViewer, engine);
			args += QScriptValue(callbacks.second); // group id
			callFunction(engine, "eventGroupSeen", args);
		}
	}
	return true;
}

//__ ## eventObjectTransfer(object, from)
//__
//__ An event that is run whenever an object is transferred between players,
//__ for example due to a Nexus Link weapon. The event is called after the
//__ object has been transferred, so the target player is in object.player.
//__ The event is called for both players.
//__
bool triggerEventObjectTransfer(BASE_OBJECT *psObj, int from)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (int i = 0; i < scripts.size() && psObj; ++i)
	{
		QScriptEngine *engine = scripts.at(i);
		int me = engine->globalObject().property("me").toInt32();
		bool receiveAll = engine->globalObject().property("isReceivingAllEvents").toBool();
		if (me == psObj->player || me == from || receiveAll)
		{
			QScriptValueList args;
			args += convMax(psObj, engine);
			args += QScriptValue(from);
			callFunction(engine, "eventObjectTransfer", args);
		}
	}
	return true;
}

//__ ## eventChat(from, to, message)
//__
//__ An event that is run whenever a chat message is received. The ```from``` parameter is the
//__ player sending the chat message. For the moment, the ```to``` parameter is always the script
//__ player.
//__
bool triggerEventChat(int from, int to, const char *message)
{
	for (int i = 0; scriptsReady && message && i < scripts.size(); ++i)
	{
		QScriptEngine *engine = scripts.at(i);
		int me = engine->globalObject().property("me").toInt32();
		bool receiveAll = engine->globalObject().property("isReceivingAllEvents").toBool();
		if (me == to || (receiveAll && to == from))
		{
			QScriptValueList args;
			args += QScriptValue(from);
			args += QScriptValue(to);
			args += QScriptValue(QString(message));
			callFunction(engine, "eventChat", args);
		}
	}
	return true;
}

//__ ## eventBeacon(x, y, from, to[, message])
//__
//__ An event that is run whenever a beacon message is received. The ```from``` parameter is the
//__ player sending the beacon. For the moment, the ```to``` parameter is always the script player.
//__ Message may be undefined.
//__
bool triggerEventBeacon(int from, int to, const char *message, int x, int y)
{
	for (auto *engine : scripts)
	{
		int me = engine->globalObject().property("me").toInt32();
		bool receiveAll = engine->globalObject().property("isReceivingAllEvents").toBool();
		if (me == to || receiveAll)
		{
			QScriptValueList args;
			args += QScriptValue(map_coord(x));
			args += QScriptValue(map_coord(y));
			args += QScriptValue(from);
			args += QScriptValue(to);
			if (message)
			{
				args += QScriptValue(QString(message));
			}
			callFunction(engine, "eventBeacon", args);
		}
	}
	return true;
}

//__ ## eventBeaconRemoved(from, to)
//__
//__ An event that is run whenever a beacon message is removed. The ```from``` parameter is the
//__ player sending the beacon. For the moment, the ```to``` parameter is always the script player.
//__
bool triggerEventBeaconRemoved(int from, int to)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *engine : scripts)
	{
		int me = engine->globalObject().property("me").toInt32();
		bool receiveAll = engine->globalObject().property("isReceivingAllEvents").toBool();
		if (me == to || receiveAll)
		{
			QScriptValueList args;
			args += QScriptValue(from);
			args += QScriptValue(to);
			callFunction(engine, "eventBeaconRemoved", args);
		}
	}
	return true;
}

//__ ## eventSelectionChanged(objects)
//__
//__ An event that is triggered whenever the host player selects one or more game objects.
//__ The ```objects``` parameter contains an array of the currently selected game objects.
//__ Keep in mind that the player may drag and drop select many units at once, select one
//__ unit specifically, or even add more selections to a current selection one at a time.
//__ This event will trigger once for each user action, not once for each selected or
//__ deselected object. If all selected game objects are deselected, ```objects``` will
//__ be empty.
//__
bool triggerEventSelected()
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	selectionChanged = true;
	return true;
}

//__ ## eventGroupLoss(object, group id, new size)
//__
//__ An event that is run whenever a group becomes empty. Input parameter
//__ is the about to be killed object, the group's id, and the new group size.
//__
// Since groups are entities local to one context, we do not iterate over them here.
bool triggerEventGroupLoss(BASE_OBJECT *psObj, int group, int size, QScriptEngine *engine)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	QScriptValueList args;
	args += convMax(psObj, engine);
	args += QScriptValue(group);
	args += QScriptValue(size);
	callFunction(engine, "eventGroupLoss", args);
	return true;
}

// This is not a trigger yet.
bool triggerEventDroidMoved(DROID *psDroid, int oldx, int oldy)
{
	return areaLabelCheck(psDroid);
}

//__ ## eventArea<label>(droid)
//__
//__ An event that is run whenever a droid enters an area label. The area is then
//__ deactived. Call resetArea() to reactivate it. The name of the event is
//__ eventArea + the name of the label.
//__
bool triggerEventArea(const QString& label, DROID *psDroid)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *engine : scripts)
	{
		QScriptValueList args;
		args += convDroid(psDroid, engine);
		QString funcname = QString("eventArea" + label);
		debug(LOG_SCRIPT, "Triggering %s for %s", funcname.toUtf8().constData(),
		      engine->globalObject().property("scriptName").toString().toUtf8().constData());
		callFunction(engine, funcname, args);
	}
	return true;
}

//__ ## eventDesignCreated(template)
//__
//__ An event that is run whenever a new droid template is created. It is only
//__ run on the client of the player designing the template.
//__
bool triggerEventDesignCreated(DROID_TEMPLATE *psTemplate)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *engine : scripts)
	{
		QScriptValueList args;
		args += convTemplate(psTemplate, engine);
		callFunction(engine, "eventDesignCreated", args);
	}
	return true;
}

//__ ## eventAllianceOffer(from, to)
//__
//__ An event that is called whenever an alliance offer is requested.
//__
bool triggerEventAllianceOffer(uint8_t from, uint8_t to)
{
	for (auto *engine : scripts)
	{
		QScriptValueList args;
		args += QScriptValue(from);
		args += QScriptValue(to);
		callFunction(engine, "eventAllianceOffer", args);
	}
	return true;
}

//__ ## eventAllianceAccepted(from, to)
//__
//__ An event that is called whenever an alliance is accepted.
//__
bool triggerEventAllianceAccepted(uint8_t from, uint8_t to)
{
	for (auto *engine : scripts)
	{
		QScriptValueList args;
		args += QScriptValue(from);
		args += QScriptValue(to);
		callFunction(engine, "eventAllianceAccepted", args);
	}
	return true;
}

//__ ## eventAllianceBroken(from, to)
//__
//__ An event that is called whenever an alliance is broken.
//__
bool triggerEventAllianceBroken(uint8_t from, uint8_t to)
{
	for (auto *engine : scripts)
	{
		QScriptValueList args;
		args += QScriptValue(from);
		args += QScriptValue(to);
		callFunction(engine, "eventAllianceBroken", args);
	}
	return true;
}

//__ ## eventSyncRequest(req_id, x, y, obj_id, obj_id2)
//__
//__ An event that is called from a script and synchronized with all other scripts and hosts
//__ to prevent desync from happening. Sync requests must be carefully validated to prevent
//__ cheating!
//__
bool triggerEventSyncRequest(int from, int req_id, int x, int y, BASE_OBJECT *psObj, BASE_OBJECT *psObj2)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *engine : scripts)
	{
		QScriptValueList args;
		args += QScriptValue(from);
		args += QScriptValue(req_id);
		args += QScriptValue(x);
		args += QScriptValue(y);
		if (psObj)
		{
			args += convMax(psObj, engine);
		}
		if (psObj2)
		{
			args += convMax(psObj2, engine);
		}
		callFunction(engine, "eventSyncRequest", args);
	}
	return true;
}

//__ ## eventKeyPressed(meta, key)
//__
//__ An event that is called whenever user presses a key in the game, not counting chat
//__ or other pop-up user interfaces. The key values are currently undocumented.
bool triggerEventKeyPressed(int meta, int key)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	for (auto *engine : scripts)
	{
		QScriptValueList args;
		args += QScriptValue(meta);
		args += QScriptValue(key);
		callFunction(engine, "eventKeyPressed", args);
	}
	return true;
}

bool namedScriptCallback(QScriptEngine *engine, const WzString& func, int player)
{
	ASSERT(scriptsReady, "Scripts not initialized yet");
	QScriptValueList args;
	args += QScriptValue(player);
	callFunction(engine, QString::fromUtf8(func.toUtf8().c_str()), args);
	return true;
}

// Enable JSON support for custom types

// QVariant
void to_json(nlohmann::json& j, const QVariant& value) {
	// IMPORTANT: This largely follows the Qt documentation on QJsonValue::fromVariant
	// See: http://doc.qt.io/qt-5/qjsonvalue.html#fromVariant
	//
	// The main change is that numeric types are independently handled (instead of
	// converting everything to `double`), because nlohmann::json handles them a bit
	// differently.

	// Note: Older versions of Qt 5.x (5.6?) do not define QMetaType::Nullptr,
	//		 so check value.isNull() instead.
	if (value.isNull())
	{
		j = nlohmann::json(); // null value
		return;
	}

	switch (value.userType())
	{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 9, 0))
		case QMetaType::Nullptr:
			j = nlohmann::json(); // null value
			break;
#endif
		case QMetaType::Bool:
			j = value.toBool();
			break;
		case QMetaType::Int:
			j = value.toInt();
			break;
		case QMetaType::UInt:
			j = value.toUInt();
			break;
		case QMetaType::LongLong:
			j = value.toLongLong();
			break;
		case QMetaType::ULongLong:
			j = value.toULongLong();
			break;
		case QMetaType::Float:
		case QVariant::Double:
			j = value.toDouble();
			break;
		case QMetaType::QString:
		{
			QString qstring = value.toString();
			j = json(qstring.toUtf8().constData());
			break;
		}
		case QMetaType::QStringList:
		case QMetaType::QVariantList:
		{
			// an array
			j = nlohmann::json::array();
			QList<QVariant> list = value.toList();
			for (QVariant& list_variant : list)
			{
				nlohmann::json list_variant_json_value;
				to_json(list_variant_json_value, list_variant);
				j.push_back(list_variant_json_value);
			}
			break;
		}
		case QMetaType::QVariantMap:
		{
			// an object
			j = nlohmann::json::object();
			QMap<QString, QVariant> map = value.toMap();
			for (QMap<QString, QVariant>::const_iterator i = map.constBegin(); i != map.constEnd(); ++i)
			{
				j[i.key().toUtf8().constData()] = i.value();
			}
			break;
		}
		case QMetaType::QVariantHash:
		{
			// an object
			j = nlohmann::json::object();
			QHash<QString, QVariant> hash = value.toHash();
			for (QHash<QString, QVariant>::const_iterator i = hash.constBegin(); i != hash.constEnd(); ++i)
			{
				j[i.key().toUtf8().constData()] = i.value();
			}
			break;
		}
		default:
			// For all other QVariant types a conversion to a QString will be attempted.
			QString qstring = value.toString();
			// If the returned string is empty, a Null QJsonValue will be stored, otherwise a String value using the returned QString.
			if (qstring.isEmpty())
			{
				j = nlohmann::json(); // null value
			}
			else
			{
				j = std::string(qstring.toUtf8().constData());
			}
	}
}
