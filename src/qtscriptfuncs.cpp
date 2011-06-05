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

#include "lib/framework/wzapp.h"
#include "lib/sound/audio.h"
#include "lib/netplay/netplay.h"
#include "qtscriptfuncs.h"

#include <QtScript/QScriptValue>
#include <QtCore/QStringList>

#include "console.h"
#include "map.h"
#include "mission.h"
#include "group.h"
#include "transporter.h"
#include "message.h"
#include "display3d.h"
#include "intelmap.h"
#include "hci.h"
#include "wrappers.h"
#include "challenge.h"
#include "research.h"
#include "multilimit.h"

// ----------------------------------------------------------------------------------------
// Utility functions -- not called directly from scripts

#define SCRIPT_ASSERT(context, expr, ...) \
	do { bool _wzeval = (expr); if (!_wzeval) { debug(LOG_ERROR, __VA_ARGS__); context->throwError(QScriptContext::ReferenceError, QString(#expr) +  " failed in " + QString(__FUNCTION__) + " at line " + QString::number(__LINE__)); return QScriptValue(); } } while (0)

QScriptValue convStructure(STRUCTURE *psStruct, QScriptEngine *engine)
{
	QScriptValue value = engine->newObject();
	ASSERT_OR_RETURN(value, psStruct, "No object for conversion");
	value.setProperty("id", psStruct->id, QScriptValue::ReadOnly);
	value.setProperty("x", psStruct->pos.x, QScriptValue::ReadOnly);
	value.setProperty("y", psStruct->pos.y, QScriptValue::ReadOnly);
	value.setProperty("z", psStruct->pos.z, QScriptValue::ReadOnly);
	value.setProperty("player", psStruct->player, QScriptValue::ReadOnly);
	return value;
}

QScriptValue convDroid(DROID *psDroid, QScriptEngine *engine)
{
	QScriptValue value = engine->newObject();
	ASSERT_OR_RETURN(value, psDroid, "No object for conversion");
	value.setProperty("id", psDroid->id, QScriptValue::ReadOnly);
	value.setProperty("x", psDroid->pos.x, QScriptValue::ReadOnly);
	value.setProperty("y", psDroid->pos.y, QScriptValue::ReadOnly);
	value.setProperty("z", psDroid->pos.z, QScriptValue::ReadOnly);
	value.setProperty("player", psDroid->player, QScriptValue::ReadOnly);
	return value;
}

QScriptValue convObj(BASE_OBJECT *psObj, QScriptEngine *engine)
{
	QScriptValue value = engine->newObject();
	ASSERT_OR_RETURN(value, psObj, "No object for conversion");
	value.setProperty("id", psObj->id, QScriptValue::ReadOnly);
	value.setProperty("x", psObj->pos.x, QScriptValue::ReadOnly);
	value.setProperty("y", psObj->pos.y, QScriptValue::ReadOnly);
	value.setProperty("z", psObj->pos.z, QScriptValue::ReadOnly);
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
	QList<STRUCTURE *> matches;
	int player = -1, looking = -1;
	QString statsName;

	switch (context->argumentCount())
	{
	default:
	case 3: looking = context->argument(2).toInt32(); // fall-through
	case 2: statsName = context->argument(1).toString(); // fall-through
	case 1: player = context->argument(0).toInt32(); break;
	case 0: player = engine->globalObject().property("me").toInt32();
	}

	SCRIPT_ASSERT(context, player < MAX_PLAYERS && player >= 0, "Target player index out of range: %d", player);
	SCRIPT_ASSERT(context, looking < MAX_PLAYERS && looking >= -1, "Looking player index out of range: %d", looking);
	for (STRUCTURE *psStruct = apsStructLists[player]; psStruct; psStruct = psStruct->psNext)
	{
		if ((looking == -1 || psStruct->visible[looking])
		    && (statsName.isEmpty() || statsName.compare(psStruct->pStructureType->pName) == 0))
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

static QScriptValue js_enumDroid(QScriptContext *context, QScriptEngine *engine)
{
	QList<DROID *> matches;
	int player = -1, looking = -1;
	DROID_TYPE droidType = DROID_ANY;

	switch (context->argumentCount())
	{
	default:
	case 3: looking = context->argument(2).toInt32(); // fall-through
	case 2: droidType = (DROID_TYPE)context->argument(1).toInt32(); // fall-through
	case 1: player = context->argument(0).toInt32(); break;
	case 0: player = engine->globalObject().property("me").toInt32();
	}

	SCRIPT_ASSERT(context, player < MAX_PLAYERS && player >= 0, "Target player index out of range: %d", player);
	SCRIPT_ASSERT(context, looking < MAX_PLAYERS && looking >= -1, "Looking player index out of range: %d", looking);
	for (DROID *psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
	{
		if ((looking == -1 || psDroid->visible[looking]) && (droidType == DROID_ANY || droidType == psDroid->droidType))
		{
			matches.push_back(psDroid);
		}
	}
	QScriptValue result = engine->newArray(matches.size());
	for (int i = 0; i < matches.size(); i++)
	{
		DROID *psDroid = matches.at(i);
		result.setProperty(i, convDroid(psDroid, engine));
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

static QScriptValue js_setMissionTime(QScriptContext *context, QScriptEngine *)
{
	int value = context->argument(0).toInt32() * 100;
	mission.time = value;
	setMissionCountDown();
	if (mission.time >= 0)
	{
		mission.startTime = gameTime;
		addMissionTimerInterface();
	}
	else
	{
		intRemoveMissionTimer();
		mission.cheatTime = 0;
	}
	return QScriptValue();
}

static QScriptValue js_setReinforcementTime(QScriptContext *context, QScriptEngine *)
{
	int value = context->argument(0).toInt32() * 100;
	SCRIPT_ASSERT(context, value == LZ_COMPROMISED_TIME || value < 60 * 60 * GAME_TICKS_PER_SEC,
	                 "The transport timer cannot be set to more than 1 hour!");
	mission.ETA = value;
	if (missionCanReEnforce())
	{
		addTransporterTimerInterface();
	}
	if (value < 0)
	{
		DROID *psDroid;

		intRemoveTransporterTimer();
		/* Only remove the launch if haven't got a transporter droid since the scripts set the 
		 * time to -1 at the between stage if there are not going to be reinforcements on the submap  */
		for (psDroid = apsDroidLists[selectedPlayer]; psDroid != NULL; psDroid = psDroid->psNext)
		{
			if (psDroid->droidType == DROID_TRANSPORTER)
			{
				break;
			}
		}
		// if not found a transporter, can remove the launch button
		if (psDroid ==  NULL)
		{
			intRemoveTransporterLaunch();
		}
	}
	return QScriptValue();
}

static QScriptValue js_setStructureLimits(QScriptContext *context, QScriptEngine *engine)
{
	QString building = context->argument(0).toString();
	int limit = context->argument(1).toInt32();
	int player;
	int structInc = getStructStatFromName(building.toUtf8().constData());
	if (context->argumentCount() > 2)
	{
		player = context->argument(2).toInt32();
	}
	else
	{
		player = engine->globalObject().property("me").toInt32();
	}
	SCRIPT_ASSERT(context, player < MAX_PLAYERS && player >= 0, "Invalid player number");
	SCRIPT_ASSERT(context, limit < LOTS_OF && limit >= 0, "Invalid limit");
	SCRIPT_ASSERT(context, structInc < numStructureStats && structInc >= 0, "Invalid structure");

	STRUCTURE_LIMITS *psStructLimits = asStructLimits[player];
	psStructLimits[structInc].limit = limit;
	psStructLimits[structInc].globalLimit = limit;

	return QScriptValue();
}

static QScriptValue js_centreView(QScriptContext *context, QScriptEngine *engine)
{
	int x = context->argument(0).toInt32();
	int y = context->argument(1).toInt32();
	setViewPos(map_coord(x), map_coord(y), false);
	Q_UNUSED(engine);
	return QScriptValue();
}

static QScriptValue js_playSound(QScriptContext *context, QScriptEngine *engine)
{
	int player = engine->globalObject().property("me").toInt32();
	if (player != selectedPlayer)
	{
		return QScriptValue();
	}
	QString sound = context->argument(0).toString();
	int soundID = audio_GetTrackID(sound.toUtf8().constData());
	if (context->argumentCount() > 1)
	{
		int x = context->argument(1).toInt32();
		int y = context->argument(2).toInt32();
		int z = context->argument(3).toInt32();
		audio_QueueTrackPos(soundID, x, y, z);
	}
	else
	{
		audio_QueueTrack(soundID);
		/*  -- FIXME properly (from original script func)
		if(bInTutorial)
		{
			audio_QueueTrack(ID_SOUND_OF_SILENCE);
		}
		*/
	}
	return QScriptValue();
}

static QScriptValue js_gameOverMessage(QScriptContext *context, QScriptEngine *engine)
{
	int player = engine->globalObject().property("me").toInt32();
	const MESSAGE_TYPE msgType = MSG_MISSION;	// always
	bool gameWon = context->argument(0).toBool();
	VIEWDATA *psViewData;
	if (gameWon)
	{
		 psViewData = getViewData("WIN");
		addConsoleMessage(_("YOU ARE VICTORIOUS!"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	}
	else
	{
		psViewData = getViewData("WIN");
		addConsoleMessage(_("YOU WERE DEFEATED!"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	}
	ASSERT(psViewData, "Viewdata not found");
	MESSAGE *psMessage = addMessage(msgType, false, player);
	if (psMessage)
	{
		//set the data
		psMessage->pViewData = (MSG_VIEWDATA *)psViewData;
		displayImmediateMessage(psMessage);
		stopReticuleButtonFlash(IDRET_INTEL_MAP);

		//we need to set this here so the VIDEO_QUIT callback is not called
		setScriptWinLoseVideo(gameWon ? PLAY_WIN : PLAY_LOSE);
	}
	displayGameOver(gameWon);
	if (challengeActive)
	{
		updateChallenge(gameWon);
	}
	return QScriptValue();
}

static QScriptValue js_completeResearch(QScriptContext *context, QScriptEngine *engine)
{
	QString researchName = context->argument(0).toString();
	int player;
	if (context->argumentCount() > 1)
	{
		player = context->argument(1).toInt32();
	}
	else
	{
		player = engine->globalObject().property("me").toInt32();
	}
	RESEARCH *psResearch = getResearch(researchName.toUtf8().constData());
	SCRIPT_ASSERT(context, psResearch, "No such research %s for player %d", researchName.toUtf8().constData(), player);
	int researchIndex = psResearch - asResearch;
	SCRIPT_ASSERT(context, researchIndex < numResearch, "Research index out of bounds");
	if (bMultiMessages && (gameTime > 2))
	{
		SendResearch(player, researchIndex, false);
		// Wait for our message before doing anything.
	}
	else
	{
		researchResult(researchIndex, player, false, NULL, false);
	}
	return QScriptValue();
}

static QScriptValue js_enableResearch(QScriptContext *context, QScriptEngine *engine)
{
	QString researchName = context->argument(0).toString();
	int player;
	if (context->argumentCount() > 1)
	{
		player = context->argument(1).toInt32();
	}
	else
	{
		player = engine->globalObject().property("me").toInt32();
	}
	RESEARCH *psResearch = getResearch(researchName.toUtf8().constData());
	SCRIPT_ASSERT(context, psResearch, "No such research %s for player %d", researchName.toUtf8().constData(), player);
	if (!enableResearch(psResearch, player))
	{
		debug(LOG_ERROR, "Unable to enable research %s for player %d", researchName.toUtf8().constData(), player);
	}
	return QScriptValue();
}

static QScriptValue js_setPower(QScriptContext *context, QScriptEngine *engine)
{
	int power = context->argument(0).toInt32();
	int player;
	if (context->argumentCount() > 1)
	{
		player = context->argument(1).toInt32();
	}
	else
	{
		player = engine->globalObject().property("me").toInt32();
	}
	setPower(player, power);
	return QScriptValue();
}

static QScriptValue js_enableStructure(QScriptContext *context, QScriptEngine *engine)
{
	QString building = context->argument(0).toString();
	int index = getStructStatFromName(building.toUtf8().constData());
	int player;
	if (context->argumentCount() > 1)
	{
		player = context->argument(1).toInt32();
	}
	else
	{
		player = engine->globalObject().property("me").toInt32();
	}
	SCRIPT_ASSERT(context, index >= 0 && index < numStructureStats, "Invalid structure stat");
	// enable the appropriate structure
	apStructTypeLists[player][index] = AVAILABLE;
	return QScriptValue();
}

static QScriptValue js_addReticuleButton(QScriptContext *context, QScriptEngine *engine)
{
	int button = context->argument(0).toInt32();
	SCRIPT_ASSERT(context, button != IDRET_OPTIONS, "Invalid button");
	widgReveal(psWScreen, button);
	return QScriptValue();
}

static QScriptValue js_applyLimitSet(QScriptContext *context, QScriptEngine *engine)
{
	Q_UNUSED(context);
	Q_UNUSED(engine);
	applyLimitSet();
	return QScriptValue();
}

static void setComponent(QString name, int player, int value)
{
	int type = -1;
	int compInc = -1;
	for (int j = COMP_BODY; j < COMP_NUMCOMPONENTS && compInc == -1; j++)
	{
		// this is very inefficient, but I am so not giving in to the deranged nature of the components code
		// and convoluting the new script system for its sake
		compInc = getCompFromName(j, name.toUtf8().constData());
		type = j;
	}
	ASSERT_OR_RETURN(, compInc != -1 && type != -1, "Bad component value");
	apCompLists[player][type][compInc] = value;
}

static QScriptValue js_enableComponent(QScriptContext *context, QScriptEngine *engine)
{
	QString componentName = context->argument(0).toString();
	int player = context->argument(1).toInt32();

	SCRIPT_ASSERT(context, player < MAX_PLAYERS && player >= 0, "Invalid player");
	setComponent(componentName, player, FOUND);
	return QScriptValue();
}

static QScriptValue js_makeComponentAvailable(QScriptContext *context, QScriptEngine *engine)
{
	QString componentName = context->argument(0).toString();
	int player = context->argument(1).toInt32();

	SCRIPT_ASSERT(context, player < MAX_PLAYERS && player >= 0, "Invalid player");
	setComponent(componentName, player, AVAILABLE);
	return QScriptValue();
}

static QScriptValue js_allianceExistsBetween(QScriptContext *context, QScriptEngine *engine)
{
	int player1 = context->argument(0).toInt32();
	int player2 = context->argument(1).toInt32();
	SCRIPT_ASSERT(context, player1 < MAX_PLAYERS && player1 >= 0, "Invalid player");
	SCRIPT_ASSERT(context, player2 < MAX_PLAYERS && player2 >= 0, "Invalid player");
	return QScriptValue(alliances[player1][player2] == ALLIANCE_FORMED);
}

static QScriptValue js_translate(QScriptContext *context, QScriptEngine *engine)
{
	return QScriptValue(gettext(context->argument(0).toString().toUtf8().constData()));
}

// ----------------------------------------------------------------------------------------
// Register functions with scripting system

bool registerFunctions(QScriptEngine *engine)
{
	// Register functions to the script engine here
	engine->globalObject().setProperty("_", engine->newFunction(js_translate));

	// General functions -- geared for use in AI scripts
	//engine->globalObject().setProperty("getDerrick", engine->newFunction(js_getDerrick));
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
	engine->globalObject().setProperty("DORDER_ATTACK", DORDER_ATTACK, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_MOVE", DORDER_MOVE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_SCOUT", DORDER_SCOUT, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_BUILD", DORDER_BUILD, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("mapWidth", mapWidth, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("mapHeight", mapHeight, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("COMMAND", IDRET_COMMAND, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("OPTIONS", IDRET_COMMAND, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("BUILD", IDRET_BUILD, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("MANUFACTURE", IDRET_MANUFACTURE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("RESEARCH", IDRET_RESEARCH, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("INTELMAP", IDRET_INTEL_MAP, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DESIGN", IDRET_DESIGN, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("CANCEL", IDRET_CANCEL, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("CAMP_CLEAN", CAMP_CLEAN, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("CAMP_BASE", CAMP_BASE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("CAMP_WALLS", CAMP_WALLS, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("NO_ALLIANCES", NO_ALLIANCES, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("ALLIANCES", ALLIANCES, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("ALLIANCES_TEAMS", ALLIANCES_TEAMS, QScriptValue::ReadOnly | QScriptValue::Undeletable);

	return true;
}
