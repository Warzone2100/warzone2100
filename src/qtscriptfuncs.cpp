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
#include "lib/framework/wzconfig.h"
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
#include "template.h"

// hack, this is used from scriptfuncs.cpp -- and we don't want to include any stinkin' wzscript headers here!
// TODO, move this stuff into a script common subsystem
extern bool structDoubleCheck(BASE_STATS *psStat,UDWORD xx,UDWORD yy, SDWORD maxBlockingTiles);
extern Vector2i positions[MAX_PLAYERS];
extern std::vector<Vector2i> derricks;

// ----------------------------------------------------------------------------------------
// Utility functions -- not called directly from scripts

#define SCRIPT_ASSERT(context, expr, ...) \
	do { bool _wzeval = (expr); if (!_wzeval) { debug(LOG_ERROR, __VA_ARGS__); context->throwError(QScriptContext::ReferenceError, QString(#expr) +  " failed in " + QString(__FUNCTION__) + " at line " + QString::number(__LINE__)); return QScriptValue(); } } while (0)

QScriptValue convResearch(RESEARCH *psResearch, QScriptEngine *engine, int player)
{
	QScriptValue value = engine->newObject();
	value.setProperty("power", (int)psResearch->researchPower);
	value.setProperty("points", (int)psResearch->researchPoints);
	bool started = false;
	for (int i = 0; i < game.maxPlayers; i++)
	{
		if (aiCheckAlliances(player, i) || player == i)
		{
			int bits = asPlayerResList[i][psResearch->index].ResearchStatus;
			started = started || (bits & STARTED_RESEARCH) || (bits & STARTED_RESEARCH_PENDING) || (bits & RESBITS_PENDING_ONLY);
		}
	}
	value.setProperty("started", started); // including whether an ally has started it
	value.setProperty("available", researchAvailable(psResearch->index, player));
	value.setProperty("name", psResearch->pName);
	return value;
}

QScriptValue convStructure(STRUCTURE *psStruct, QScriptEngine *engine)
{
	QScriptValue value = convObj(psStruct, engine);
	value.setProperty("status", (int)psStruct->status, QScriptValue::ReadOnly);
	value.setProperty("type", (int)OBJ_STRUCTURE, QScriptValue::ReadOnly);
	switch (psStruct->pStructureType->type) // don't bleed our source insanities into the scripting world
	{
	case REF_WALL:
	case REF_WALLCORNER:
	case REF_GATE:
		value.setProperty("stattype", (int)REF_WALL, QScriptValue::ReadOnly);
		break;
	case REF_BLASTDOOR:
		value.setProperty("stattype", (int)REF_DEFENSE, QScriptValue::ReadOnly);
		break;
	default:
		value.setProperty("stattype", (int)psStruct->pStructureType->type, QScriptValue::ReadOnly);
		break;
	}
	return value;
}

QScriptValue convFeature(FEATURE *psFeature, QScriptEngine *engine)
{
	QScriptValue value = convObj(psFeature, engine);
	value.setProperty("type", (int)OBJ_FEATURE, QScriptValue::ReadOnly);
	return value;
}

QScriptValue convDroid(DROID *psDroid, QScriptEngine *engine)
{
	QScriptValue value = convObj(psDroid, engine);
	value.setProperty("action", (int)psDroid->action, QScriptValue::ReadOnly);
	value.setProperty("order", (int)psDroid->order, QScriptValue::ReadOnly);
	value.setProperty("type", (int)OBJ_DROID, QScriptValue::ReadOnly);
	return value;
}

QScriptValue convObj(BASE_OBJECT *psObj, QScriptEngine *engine)
{
	QScriptValue value = engine->newObject();
	ASSERT_OR_RETURN(value, psObj, "No object for conversion");
	value.setProperty("id", psObj->id, QScriptValue::ReadOnly);
	value.setProperty("x", map_coord(psObj->pos.x), QScriptValue::ReadOnly);
	value.setProperty("y", map_coord(psObj->pos.y), QScriptValue::ReadOnly);
	value.setProperty("z", map_coord(psObj->pos.z), QScriptValue::ReadOnly);
	value.setProperty("player", psObj->player, QScriptValue::ReadOnly);
	value.setProperty("selected", psObj->selected, QScriptValue::ReadOnly);
	value.setProperty("name", objInfo(psObj));
	return value;
}

QScriptValue convMax(BASE_OBJECT *psObj, QScriptEngine *engine)
{
	switch (psObj->type)
	{
	case OBJ_DROID: return convDroid((DROID *)psObj, engine);
	case OBJ_STRUCTURE: return convStructure((STRUCTURE *)psObj, engine);
	case OBJ_FEATURE: return convFeature((FEATURE *)psObj, engine);
	default: ASSERT(false, "No such supported object type"); return convObj(psObj, engine);
	}
}

// ----------------------------------------------------------------------------------------
// Label system (function defined in qtscript.h header)
//

struct labeltype { Vector2i p1, p2; int id; int type; int player; };

static QHash<QString, labeltype> labels;

// Load labels
bool loadLabels(const char *filename)
{
	if (!PHYSFS_exists(filename))
	{
		debug(LOG_SAVE, "No %s found -- use fallback method", filename);
		return false;
	}
	WzConfig ini(filename);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "No label file %s", filename);
		return false;
	}
	labels.clear();
	QStringList list = ini.childGroups();
	for (int i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		labeltype p;
		QString label(ini.value("label").toString());
		if (labels.contains(label))
		{
			debug(LOG_ERROR, "Duplicate label found");
		}
		else if (list[i].startsWith("position"))
		{
			p.p1 = ini.vector2i("pos");
			p.p2 = p.p1;
			p.type = POSITION;
			p.player = -1;
			p.id = -1;
			labels.insert(label, p);
		}
		else if (list[i].startsWith("area"))
		{
			p.p1 = ini.vector2i("pos1");
			p.p2 = ini.vector2i("pos2");
			p.type = AREA;
			p.player = -1;
			p.id = -1;
			labels.insert(label, p);
		}
		else if (list[i].startsWith("object"))
		{
			p.id = ini.value("id").toInt();
			p.type = ini.value("type").toInt();
			p.player = ini.value("player").toInt();
			labels.insert(label, p);
		}
		else
		{
			debug(LOG_ERROR, "Misnamed group in %s", filename);
		}
		ini.endGroup();
	}
	return true;
}

bool writeLabels(const char *filename)
{
	int c[3];
	memset(c, 0, sizeof(c));
	WzConfig ini(filename);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", filename);
		return false;
	}
	for (QHash<QString, labeltype>::const_iterator i = labels.constBegin(); i != labels.constEnd(); i++)
	{
		QString key = i.key();
		labeltype l = i.value();
		if (l.type == POSITION)
		{
			ini.beginGroup("position_" + QString::number(c[0]++));
			ini.setVector2i("pos", l.p1);
			ini.setValue("label", key);
			ini.endGroup();
		}
		else if (l.type == AREA)
		{
			ini.beginGroup("area_" + QString::number(c[1]++));
			ini.setVector2i("pos1", l.p1);
			ini.setVector2i("pos2", l.p2);
			ini.setValue("label", key);
			ini.endGroup();
		}
		else
		{
			ini.beginGroup("object_" + QString::number(c[2]++));
			ini.setValue("id", l.id);
			ini.setValue("player", l.player);
			ini.setValue("type", l.type);
			ini.setValue("label", key);
			ini.endGroup();
		}
	}
	return true;
}

// ----------------------------------------------------------------------------------------
// Script functions
//
// All script functions should be prefixed with "js_" then followed by same name as in script.

static QScriptValue js_label(QScriptContext *context, QScriptEngine *engine)
{
	QString label = context->argument(0).toString();
	QScriptValue ret = engine->newObject();
	if (labels.contains(label))
	{
		labeltype p = labels.value(label);
		if (p.type == AREA || p.type == POSITION)
		{
			ret.setProperty("x", map_coord(p.p1.x), QScriptValue::ReadOnly);
			ret.setProperty("y", map_coord(p.p1.y), QScriptValue::ReadOnly);
		}
		if (p.type == AREA)
		{
			ret.setProperty("x2", map_coord(p.p2.x), QScriptValue::ReadOnly);
			ret.setProperty("xy", map_coord(p.p2.y), QScriptValue::ReadOnly);
		}
		else if (p.type == OBJ_DROID)
		{
			DROID *psDroid = IdToDroid(p.id, p.player);
			if (psDroid) ret = convDroid(psDroid, engine);
		}
		else if (p.type == OBJ_STRUCTURE)
		{
			STRUCTURE *psStruct = IdToStruct(p.id, p.player);
			if (psStruct) ret = convStructure(psStruct, engine);
		}
		else if (p.type == OBJ_FEATURE)
		{
			FEATURE *psFeature = IdToFeature(p.id, p.player);
			if (psFeature) ret = convFeature(psFeature, engine);
		}
	}
	else debug(LOG_ERROR, "label %s not found!", label.toUtf8().constData());
	return ret;
}

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

// TODO, allow passing in a research object instead of a string as second parameter
static QScriptValue js_pursueResearch(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue structVal = context->argument(0);
	int id = structVal.property("id").toInt32();
	int player = structVal.property("player").toInt32();
	STRUCTURE *psStruct = IdToStruct(id, player);
	SCRIPT_ASSERT(context, psStruct, "No such structure id %d belonging to player %d", id, player);
	QString resName = context->argument(1).toString();
	RESEARCH *psResearch = getResearch(resName.toUtf8().constData());
	SCRIPT_ASSERT(context, psResearch, "No such research: %s", resName.toUtf8().constData());
	PLAYER_RESEARCH *plrRes = &asPlayerResList[player][psResearch->index];
	SCRIPT_ASSERT(context, psStruct->pStructureType->type == REF_RESEARCH, "Not a research lab: %s", objInfo(psStruct));
	RESEARCH_FACILITY *psResLab = (RESEARCH_FACILITY *)psStruct->pFunctionality;
	SCRIPT_ASSERT(context, psResLab->psSubject == NULL, "Research lab not ready");
	SCRIPT_ASSERT(context, !(plrRes->ResearchStatus & RESEARCHED), "Research item already completed: %s", psResearch->pName);
	if (IsResearchStartedPending(plrRes) || IsResearchCompleted(plrRes))
	{
		return QScriptValue(false);
	}
	// Go down the requirements list for the desired tech
	QList<RESEARCH *> reslist;
	RESEARCH *cur = psResearch;
	while (cur)
	{
		if (researchAvailable(cur->index, player))
		{
			bool started = false;
			for (int i = 0; i < game.maxPlayers; i++)
			{
				if (aiCheckAlliances(player, i) || i == player)
				{
					int bits = asPlayerResList[i][cur->index].ResearchStatus;
					started = started || (bits & STARTED_RESEARCH) || (bits & STARTED_RESEARCH_PENDING)
							|| (bits & RESBITS_PENDING_ONLY) || (bits & RESEARCHED);
				}
			}
			if (!started) // found relevant item on the path?
			{
#if defined (DEBUG)
				char sTemp[128];
				sendResearchStatus(psStruct, cur->index, player, true);
				sprintf(sTemp, "player:%d starts topic from script: %s", player, cur->pName);
				NETlogEntry(sTemp, SYNC_FLAG, 0);
#endif
				return QScriptValue(true);
			}
		}
		RESEARCH *prev = cur;
		cur = NULL;
		if (prev->pPRList.size())
		{
			cur = &asResearch[prev->pPRList[0]]; // get first pre-req
		}
		for (int i = 1; i < prev->pPRList.size(); i++)
		{
			// push any other pre-reqs on the stack
			reslist += &asResearch[prev->pPRList[i]];
		}
		if (!cur && reslist.size())
		{
			cur = reslist.takeFirst(); // retrieve options from the stack
		}
	}
	return QScriptValue(false); // none found
}

static QScriptValue js_getResearch(QScriptContext *context, QScriptEngine *engine)
{
	int player = engine->globalObject().property("me").toInt32();
	QString resName = context->argument(0).toString();
	RESEARCH *psResearch = getResearch(resName.toUtf8().constData());
	SCRIPT_ASSERT(context, psResearch, "No such research: %s", resName.toUtf8().constData());
	return convResearch(psResearch, engine, player);
}

static QScriptValue js_enumResearch(QScriptContext *context, QScriptEngine *engine)
{
	QList<RESEARCH *> reslist;
	int player = engine->globalObject().property("me").toInt32();
	for (int i = 0; i < asResearch.size(); i++)
	{
		if (!IsResearchCompleted(&asPlayerResList[player][i]))
		{
			reslist += &asResearch[i];
		}
	}
	QScriptValue result = engine->newArray(reslist.size());
	for (int i = 0; i < reslist.size(); i++)
	{
		result.setProperty(i, convResearch(reslist[i], engine, player));
	}
	return result;
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

static QScriptValue js_enumFeature(QScriptContext *context, QScriptEngine *engine)
{
	QList<FEATURE *> matches;
	int looking = context->argument(0).toInt32();
	QString statsName = statsName = context->argument(1).toString();
	SCRIPT_ASSERT(context, looking < MAX_PLAYERS && looking >= -1, "Looking player index out of range: %d", looking);
	for (FEATURE *psFeat = apsFeatureLists[0]; psFeat; psFeat = psFeat->psNext)
	{
		if ((looking == -1 || psFeat->visible[looking])
		    && (statsName.isEmpty() || statsName.compare(psFeat->psStats->pName) == 0))
		{
			matches.push_back(psFeat);
		}
	}
	QScriptValue result = engine->newArray(matches.size());
	for (int i = 0; i < matches.size(); i++)
	{
		FEATURE *psFeat = matches.at(i);
		result.setProperty(i, convFeature(psFeat, engine));
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

static QScriptValue js_pickStructLocation(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue droidVal = context->argument(0);
	const int id = droidVal.property("id").toInt32();
	const int player = droidVal.property("player").toInt32();
	DROID *psDroid = IdToDroid(id, player);
	QString statName = context->argument(1).toString();
	int index = getStructStatFromName(statName.toUtf8().constData());
	STRUCTURE_STATS	*psStat = &asStructureStats[index];
	const int startX = context->argument(2).toInt32();
	const int startY = context->argument(3).toInt32();
	int numIterations = 30;
	bool found = false;
	int incX, incY, x, y;
	int maxBlockingTiles = 0;

	SCRIPT_ASSERT(context, psDroid, "No such droid id %d belonging to player %d", id, player);
	SCRIPT_ASSERT(context, psStat, "No such stat found: %s", statName.toUtf8().constData());
	SCRIPT_ASSERT(context, player < MAX_PLAYERS && player >= 0, "Invalid player number %d", player);
	SCRIPT_ASSERT(context, startX >= 0 && startX < mapWidth && startY >= 0 && startY < mapHeight, "Bad position (%d, %d)", startX, startY);

	if (context->argumentCount() > 4) // final optional argument
	{
		maxBlockingTiles = context->argument(4).toInt32();
	}

	x = startX;
	y = startY;

	Vector2i offset(psStat->baseWidth * (TILE_UNITS / 2), psStat->baseBreadth * (TILE_UNITS / 2));  // This presumably gets added to the chosen coordinates, somewhere, based on looking at what pickStructLocation does. No idea where it gets added, though, maybe inside the scripts?

	// save a lot of typing... checks whether a position is valid
	#define LOC_OK(_x, _y) (tileOnMap(_x, _y) && \
				(!psDroid || fpathCheck(psDroid->pos, Vector3i(world_coord(_x), world_coord(_y), 0), PROPULSION_TYPE_WHEELED)) \
				&& validLocation(psStat, world_coord(Vector2i(_x, _y)) + offset, 0, player, false) && structDoubleCheck(psStat, _x, _y, maxBlockingTiles))

	// first try the original location
	if (LOC_OK(startX, startY))
	{
		found = true;
	}

	// try some locations nearby
	for (incX = 1, incY = 1; incX < numIterations && !found; incX++, incY++)
	{
		y = startY - incY;	// top
		for (x = startX - incX; x < startX + incX; x++)
		{
			if (LOC_OK(x, y))
			{
				found = true;
				goto endstructloc;
			}
		}
		x = startX + incX;	// right
		for (y = startY - incY; y < startY + incY; y++)
		{
			if (LOC_OK(x, y))
			{
				found = true;
				goto endstructloc;
			}
		}
		y = startY + incY;	// bottom
		for (x = startX + incX; x > startX - incX; x--)
		{
			if (LOC_OK(x, y))
			{
				found = true;
				goto endstructloc;
			}
		}
		x = startX - incX;	// left
		for (y = startY + incY; y > startY - incY; y--)
		{
			if (LOC_OK(x, y))
			{
				found = true;
				goto endstructloc;
			}
		}
	}

endstructloc:
	if (found)
	{
		QScriptValue retval = engine->newObject();
		retval.setProperty("x", x, QScriptValue::ReadOnly);
		retval.setProperty("y", y, QScriptValue::ReadOnly);
		return retval;
	}
	else
	{
		debug(LOG_SCRIPT, "Did not find valid positioning for %s", psStat->pName);
	}
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

static QScriptValue js_removeStruct(QScriptContext *context, QScriptEngine *)
{
	QScriptValue structVal = context->argument(0);
	int id = structVal.property("id").toInt32();
	int player = structVal.property("player").toInt32();
	STRUCTURE *psStruct = IdToStruct(id, player);
	SCRIPT_ASSERT(context, psStruct, "No such structure id %d belonging to player %d", id, player);
	removeStruct(psStruct, true);
	return QScriptValue();
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
	int x1 = world_coord(context->argument(1).toInt32());
	int y1 = world_coord(context->argument(2).toInt32());
	int x2 = world_coord(context->argument(3).toInt32());
	int y2 = world_coord(context->argument(4).toInt32());
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

static QScriptValue js_distBetweenTwoPoints(QScriptContext *context, QScriptEngine *engine)
{
	int x1 = context->argument(0).toNumber();
	int y1 = context->argument(1).toNumber();
	int x2 = context->argument(2).toNumber();
	int y2 = context->argument(3).toNumber();
	return QScriptValue(iHypot(x1 - x2, y1 - y2));
}

static QScriptValue js_groupSize(QScriptContext *context, QScriptEngine *)
{
	int groupId = context->argument(0).toInt32();
	DROID_GROUP *psGroup = grpFind(groupId);
	SCRIPT_ASSERT(context, psGroup, "Group %d not found", groupId);
	return QScriptValue(psGroup->refCount);
}

static QScriptValue js_droidCanReach(QScriptContext *context, QScriptEngine *)
{
	QScriptValue droidVal = context->argument(0);
	int id = droidVal.property("id").toInt32();
	int player = droidVal.property("player").toInt32();
	int x = context->argument(1).toInt32();
	int y = context->argument(2).toInt32();
	DROID *psDroid = IdToDroid(id, player);
	SCRIPT_ASSERT(context, psDroid, "Droid id %d not found belonging to player %d", id, player);
	const PROPULSION_STATS *psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION].nStat;
	return QScriptValue(fpathCheck(psDroid->pos, Vector3i(world_coord(x), world_coord(y), 0), psPropStats->propulsionType));
}

// Give a Droid an order to an object
// droid, order, object
static QScriptValue js_orderDroidObj(QScriptContext *context, QScriptEngine *)
{
	QScriptValue droidVal = context->argument(0);
	int id = droidVal.property("id").toInt32();
	int player = droidVal.property("player").toInt32();
	DROID *psDroid = IdToDroid(id, player);
	SCRIPT_ASSERT(context, psDroid, "Droid id %d not found belonging to player %d", id, player);
	DROID_ORDER order = (DROID_ORDER)context->argument(1).toInt32();
	QScriptValue objVal = context->argument(2);
	int oid = objVal.property("id").toInt32();
	int oplayer = objVal.property("player").toInt32();
	BASE_OBJECT *psObj = IdToPointer(oid, oplayer);
	SCRIPT_ASSERT(context, psObj, "Object id %d not found belonging to player %d", oid, oplayer);
	SCRIPT_ASSERT(context, order == DORDER_ATTACK || order == DORDER_HELPBUILD ||
		      order == DORDER_DEMOLISH || order == DORDER_REPAIR ||
		      order == DORDER_OBSERVE || order == DORDER_EMBARK ||
		      order == DORDER_FIRESUPPORT || order == DORDER_DROIDREPAIR, "Invalid order");
	orderDroidObj(psDroid, order, psObj, ModeQueue);
	return true;
}

static QScriptValue js_orderDroidStatsLoc(QScriptContext *context, QScriptEngine *)
{
	QScriptValue droidVal = context->argument(0);
	int id = droidVal.property("id").toInt32();
	int player = droidVal.property("player").toInt32();
	DROID *psDroid = IdToDroid(id, player);
	DROID_ORDER order = (DROID_ORDER)context->argument(1).toInt32();
	QString statName = context->argument(2).toString();
	int index = getStructStatFromName(statName.toUtf8().constData());
	STRUCTURE_STATS	*psStats = &asStructureStats[index];
	int x = context->argument(3).toInt32();
	int y = context->argument(4).toInt32();

	SCRIPT_ASSERT(context, order == DORDER_BUILD, "Invalid order");
	SCRIPT_ASSERT(context, strcmp(psStats->pName, "A0ADemolishStructure") != 0, "Cannot build demolition");

	// Don't allow scripts to order structure builds if players structure limit has been reached.
	if (!IsPlayerStructureLimitReached(psDroid->player))
	{
		orderDroidStatsLocDir(psDroid, order, psStats, world_coord(x), world_coord(y), 0, ModeQueue);
	}
	return QScriptValue(true);
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
	SCRIPT_ASSERT(context, tileOnMap(x, y), "Outside map bounds (%d, %d)", x, y);
	orderDroidLoc(psDroid, order, world_coord(x), world_coord(y), ModeQueue);
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
	setViewPos(x, y, false);
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
		int x = world_coord(context->argument(1).toInt32());
		int y = world_coord(context->argument(2).toInt32());
		int z = world_coord(context->argument(3).toInt32());
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
	SCRIPT_ASSERT(context, psResearch->index < asResearch.size(), "Research index out of bounds");
	if (bMultiMessages && (gameTime > 2))
	{
		SendResearch(player, psResearch->index, false);
		// Wait for our message before doing anything.
	}
	else
	{
		researchResult(psResearch->index, player, false, NULL, false);
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

static QScriptValue js_playerPower(QScriptContext *context, QScriptEngine *engine)
{
	int player = context->argument(0).toInt32();
	return QScriptValue(getPower(player));
}

static QScriptValue js_isStructureAvailable(QScriptContext *context, QScriptEngine *engine)
{
	QString building = context->argument(0).toString();
	int index = getStructStatFromName(building.toUtf8().constData());
	int player = context->argument(1).toInt32();
	return QScriptValue(apStructTypeLists[player][index] == AVAILABLE
			    && asStructLimits[player][index].currentQuantity < asStructLimits[player][index].limit);
}

// FIXME - find a better way
static QScriptValue js_hackNetOff(QScriptContext *, QScriptEngine *)
{
	bMultiPlayer = false;
	bMultiMessages = false;
	return QScriptValue();
}

// FIXME - find a better way
static QScriptValue js_hackNetOn(QScriptContext *, QScriptEngine *)
{
	bMultiPlayer = true;
	bMultiMessages = true;
	return QScriptValue();
}

// ----------------------------------------------------------------------------------------
// Register functions with scripting system

bool registerFunctions(QScriptEngine *engine)
{
	// Register functions to the script engine here
	engine->globalObject().setProperty("_", engine->newFunction(js_translate));
	engine->globalObject().setProperty("label", engine->newFunction(js_label));

	// horrible hacks follow -- do not rely on these being present!
	engine->globalObject().setProperty("hackNetOff", engine->newFunction(js_hackNetOff));
	engine->globalObject().setProperty("hackNetOn", engine->newFunction(js_hackNetOn));

	// General functions -- geared for use in AI scripts
	engine->globalObject().setProperty("debug", engine->newFunction(js_debug));
	engine->globalObject().setProperty("console", engine->newFunction(js_console));
	engine->globalObject().setProperty("structureIdle", engine->newFunction(js_structureIdle));
	engine->globalObject().setProperty("buildDroid", engine->newFunction(js_buildDroid));
	engine->globalObject().setProperty("enumStruct", engine->newFunction(js_enumStruct));
	engine->globalObject().setProperty("enumDroid", engine->newFunction(js_enumDroid));
	engine->globalObject().setProperty("enumGroup", engine->newFunction(js_enumGroup));
	engine->globalObject().setProperty("enumFeature", engine->newFunction(js_enumFeature));
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
	engine->globalObject().setProperty("orderDroidStatsLoc", engine->newFunction(js_orderDroidStatsLoc));
	engine->globalObject().setProperty("orderDroidObj", engine->newFunction(js_orderDroidObj));

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
	engine->globalObject().setProperty("removeStruct", engine->newFunction(js_removeStruct));

	// Set some useful constants
	engine->globalObject().setProperty("DORDER_ATTACK", DORDER_ATTACK, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_MOVE", DORDER_MOVE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_SCOUT", DORDER_SCOUT, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_BUILD", DORDER_BUILD, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_HELPBUILD", DORDER_HELPBUILD, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_LINEBUILD", DORDER_LINEBUILD, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_REPAIR", DORDER_REPAIR, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_RETREAT", DORDER_RETREAT, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_PATROL", DORDER_PATROL, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_BUILDMODULE", DORDER_BUILDMODULE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
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
	engine->globalObject().setProperty("BEING_BUILT", SS_BEING_BUILT, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("BUILT", SS_BUILT, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("BEING_DEMOLISHED", SS_BEING_DEMOLISHED, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DROID_CONSTRUCT", DROID_CONSTRUCT, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("HQ", REF_HQ, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("FACTORY", REF_FACTORY, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("POWER_GEN", REF_POWER_GEN, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("RESOURCE_EXTRACTOR", REF_RESOURCE_EXTRACTOR, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DEFENSE", REF_DEFENSE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("WALL", REF_WALL, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("RESEARCH", REF_RESEARCH, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("REPAIR_FACILITY", REF_REPAIR_FACILITY, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("CYBORG_FACTORY", REF_CYBORG_FACTORY, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("VTOL_FACTORY", REF_VTOL_FACTORY, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("REARM_PAD", REF_REARM_PAD, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("SAT_UPLINK", REF_SAT_UPLINK, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("GATE", REF_GATE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("COMMAND_CONTROL", REF_COMMAND_CONTROL, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("EASY", DIFFICULTY_EASY, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("MEDIUM", DIFFICULTY_MEDIUM, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("HARD", DIFFICULTY_HARD, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("INSANE", DIFFICULTY_INSANE, QScriptValue::ReadOnly | QScriptValue::Undeletable);

	// Static knowledge about players
	QScriptValue playerData = engine->newArray(game.maxPlayers);
	for (int i = 0; i < game.maxPlayers; i++)
	{
		QScriptValue vector = engine->newObject();
		vector.setProperty("difficulty", NetPlay.players[i].difficulty, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		vector.setProperty("colour", NetPlay.players[i].colour, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		vector.setProperty("position", NetPlay.players[i].position, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		vector.setProperty("team", NetPlay.players[i].team, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		playerData.setProperty(i, vector, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	}
	engine->globalObject().setProperty("playerData", playerData, QScriptValue::ReadOnly | QScriptValue::Undeletable);

	// Static map knowledge about start positions
	QScriptValue startPositions = engine->newArray(game.maxPlayers);
	for (int i = 0; i < game.maxPlayers; i++)
	{
		QScriptValue vector = engine->newObject();
		vector.setProperty("x", map_coord(positions[i].x), QScriptValue::ReadOnly | QScriptValue::Undeletable);
		vector.setProperty("y", map_coord(positions[i].y), QScriptValue::ReadOnly | QScriptValue::Undeletable);
		startPositions.setProperty(i, vector, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	}
	QScriptValue derrickPositions = engine->newArray(derricks.size());
	for (int i = 0; i < derricks.size(); i++)
	{
		QScriptValue vector = engine->newObject();
		vector.setProperty("x", map_coord(derricks[i].x), QScriptValue::ReadOnly | QScriptValue::Undeletable);
		vector.setProperty("y", map_coord(derricks[i].y), QScriptValue::ReadOnly | QScriptValue::Undeletable);
		derrickPositions.setProperty(i, vector, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	}
	engine->globalObject().setProperty("derrickPositions", derrickPositions, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("startPositions", startPositions, QScriptValue::ReadOnly | QScriptValue::Undeletable);

	return true;
}
