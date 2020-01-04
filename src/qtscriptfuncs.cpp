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
 * @file qtscriptfuncs.cpp
 *
 * New scripting system -- script functions
 */

#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__) && (9 <= __GNUC__)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdeprecated-copy" // Workaround Qt < 5.13 `deprecated-copy` issues with GCC 9
#endif

// **NOTE: Qt headers _must_ be before platform specific headers so we don't get conflicts.
#include <QtScript/QScriptValue>
#include <QtCore/QStringList>
#include <QtCore/QJsonArray>
#include <QtGui/QStandardItemModel>
#include <QtCore/QPointer>

#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__) && (9 <= __GNUC__)
# pragma GCC diagnostic pop // Workaround Qt < 5.13 `deprecated-copy` issues with GCC 9
#endif

#include "lib/framework/wzapp.h"
#include "lib/framework/wzconfig.h"
#include "lib/framework/fixedpoint.h"
#include "lib/sound/audio.h"
#include "lib/sound/cdaudio.h"
#include "lib/netplay/netplay.h"
#include "qtscriptfuncs.h"
#include "lib/ivis_opengl/tex.h"

#include "action.h"
#include "clparse.h"
#include "combat.h"
#include "console.h"
#include "design.h"
#include "display3d.h"
#include "map.h"
#include "mission.h"
#include "move.h"
#include "order.h"
#include "transporter.h"
#include "message.h"
#include "display3d.h"
#include "intelmap.h"
#include "hci.h"
#include "wrappers.h"
#include "challenge.h"
#include "research.h"
#include "multilimit.h"
#include "multigifts.h"
#include "multimenu.h"
#include "template.h"
#include "lighting.h"
#include "radar.h"
#include "random.h"
#include "frontend.h"
#include "loop.h"
#include "gateway.h"
#include "mapgrid.h"
#include "lighting.h"
#include "atmos.h"
#include "warcam.h"
#include "projectile.h"
#include "component.h"
#include "seqdisp.h"
#include "ai.h"
#include "advvis.h"
#include "loadsave.h"
#include "wzapi.h"

#define ALL_PLAYERS -1
#define ALLIES -2
#define ENEMIES -3

enum Scrcb {
	SCRCB_FIRST = COMP_NUMCOMPONENTS,
	SCRCB_RES = SCRCB_FIRST,  // Research upgrade
	SCRCB_MODULE_RES,  // Research module upgrade
	SCRCB_REP,  // Repair upgrade
	SCRCB_POW,  // Power upgrade
	SCRCB_MODULE_POW,  // And so on...
	SCRCB_CON,
	SCRCB_MODULE_CON,
	SCRCB_REA,
	SCRCB_ARM,
	SCRCB_HEA,
	SCRCB_ELW,
	SCRCB_HIT,
	SCRCB_LIMIT,
	SCRCB_LAST = SCRCB_LIMIT
};

Vector2i positions[MAX_PLAYERS];
std::vector<Vector2i> derricks;

void scriptSetStartPos(int position, int x, int y)
{
	positions[position].x = x;
	positions[position].y = y;
	debug(LOG_SCRIPT, "Setting start position %d to (%d, %d)", position, x, y);
}

void scriptSetDerrickPos(int x, int y)
{
	Vector2i pos(x, y);
	derricks.push_back(pos);
}

bool scriptInit()
{
	int i;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		scriptSetStartPos(i, 0, 0);
	}
	derricks.clear();
	derricks.reserve(8 * MAX_PLAYERS);
	return true;
}

Vector2i getPlayerStartPosition(int player)
{
	return positions[player];
}

// private qtscript bureaucracy
typedef std::map<BASE_OBJECT *, int> GROUPMAP;
typedef std::map<QScriptEngine *, GROUPMAP *> ENGINEMAP;
static ENGINEMAP groups;

GROUPMAP* getGroupMap(QScriptEngine *engine)
{
	auto groupIt = groups.find(engine);
	GROUPMAP *psMap = (groupIt != groups.end()) ? groupIt->second : nullptr;
	return psMap;
}

struct LABEL
{
	Vector2i p1, p2;
	int id;
	int type;
	int player;
	int subscriber;
	std::vector<int> idlist;
	int triggered;

	bool operator==(const LABEL &o) const
	{
		return id == o.id && type == o.type && player == o.player;
	}
};
typedef std::map<std::string, LABEL> LABELMAP;
static LABELMAP labels;
static QPointer<QStandardItemModel> labelModel;

#define SCRIPT_ASSERT_PLAYER(_context, _player) \
	SCRIPT_ASSERT(_context, _player >= 0 && _player < MAX_PLAYERS, "Invalid player index %d", _player);

#define WzStringToQScriptValue(_engine, _wzstring) \
	QScriptValue(_engine, QString::fromUtf8(_wzstring.toUtf8().c_str()))

// ----------------------------------------------------------------------------------------
// Utility functions -- not called directly from scripts

static QScriptValue mapJsonObjectToQScriptValue(QScriptEngine *engine, const nlohmann::json &obj, QScriptValue::PropertyFlags flags)
{
	QScriptValue value = engine->newObject();
	for (auto it = obj.begin(); it != obj.end(); ++it)
	{
		value.setProperty(QString::fromUtf8(it.key().c_str()), mapJsonToQScriptValue(engine, it.value(), flags), flags);
	}
	return value;
}

static QScriptValue mapJsonArrayToQScriptValue(QScriptEngine *engine, const nlohmann::json &array, QScriptValue::PropertyFlags flags)
{
	QScriptValue value = engine->newArray(array.size());
	for (int i = 0; i < array.size(); i++)
	{
		value.setProperty(i, mapJsonToQScriptValue(engine, array.at(i), flags), flags);
	}
	return value;
}

QScriptValue mapJsonToQScriptValue(QScriptEngine *engine, const nlohmann::json &instance, QScriptValue::PropertyFlags flags)
{
	switch (instance.type())
	{
		// IMPORTANT: To match the prior behavior of loading a QVariant from the JSON value and using engine->toScriptValue(QVariant)
		//			  to convert to a QScriptValue, "null" JSON values *MUST* map to QScriptValue::UndefinedValue.
		//
		//			  If they are set to QScriptValue::NullValue, it causes issues for libcampaign.js. (As the values become "defined".)
		//
		case json::value_t::null : return QScriptValue::UndefinedValue;
		case json::value_t::boolean : return engine->toScriptValue(instance.get<bool>());
		case json::value_t::number_integer: return engine->toScriptValue(instance.get<int>());
		case json::value_t::number_unsigned: return engine->toScriptValue(instance.get<unsigned>());
		case json::value_t::number_float: return engine->toScriptValue(instance.get<double>());
		case json::value_t::string	: return engine->toScriptValue(QString::fromUtf8(instance.get<WzString>().toUtf8().c_str()));
		case json::value_t::array : return mapJsonArrayToQScriptValue(engine, instance, flags);
		case json::value_t::object : return mapJsonObjectToQScriptValue(engine, instance, flags);
		case json::value_t::binary :
			debug(LOG_ERROR, "Unexpected binary value type");
			return QScriptValue::UndefinedValue;
		case json::value_t::discarded : return QScriptValue::UndefinedValue;
	}
	return QScriptValue::UndefinedValue; // should never be reached
}

static void updateLabelModel()
{
	if (!labelModel)
	{
		return;
	}
	labelModel->setRowCount(0);
	labelModel->setRowCount(labels.size());
	int nextRow = 0;
	for (LABELMAP::iterator i = labels.begin(); i != labels.end(); i++)
	{
		const LABEL &l = i->second;
		labelModel->setItem(nextRow, 0, new QStandardItem(QString::fromStdString(i->first)));
		const char *c = "?";
		switch (l.type)
		{
		case OBJ_DROID: c = "DROID"; break;
		case OBJ_FEATURE: c = "FEATURE"; break;
		case OBJ_STRUCTURE: c = "STRUCTURE"; break;
		case SCRIPT_POSITION: c = "POSITION"; break;
		case SCRIPT_AREA: c = "AREA"; break;
		case SCRIPT_RADIUS: c = "RADIUS"; break;
		case SCRIPT_GROUP: c = "GROUP"; break;
		case SCRIPT_PLAYER:
		case SCRIPT_RESEARCH:
		case OBJ_PROJECTILE:
		case SCRIPT_COUNT: c = "ERROR"; break;
		}
		labelModel->setItem(nextRow, 1, new QStandardItem(QString(c)));
		switch (l.triggered)
		{
		case -1: labelModel->setItem(nextRow, 2, new QStandardItem("N/A")); break;
		case 0: labelModel->setItem(nextRow, 2, new QStandardItem("Active")); break;
		default: labelModel->setItem(nextRow, 2, new QStandardItem("Done")); break;
		}
		if (l.player == ALL_PLAYERS)
		{
			labelModel->setItem(nextRow, 3, new QStandardItem("ALL"));
		}
		else
		{
			labelModel->setItem(nextRow, 3, new QStandardItem(QString::number(l.player)));
		}
		if (l.subscriber == ALL_PLAYERS)
		{
			labelModel->setItem(nextRow, 4, new QStandardItem("ALL"));
		}
		else
		{
			labelModel->setItem(nextRow, 4, new QStandardItem(QString::number(l.subscriber)));
		}
		nextRow++;
	}
}

QStandardItemModel *createLabelModel()
{
	labelModel = new QStandardItemModel(0, 5);
	labelModel->setHeaderData(0, Qt::Horizontal, QString("Label"));
	labelModel->setHeaderData(1, Qt::Horizontal, QString("Type"));
	labelModel->setHeaderData(2, Qt::Horizontal, QString("Trigger"));
	labelModel->setHeaderData(3, Qt::Horizontal, QString("Owner"));
	labelModel->setHeaderData(4, Qt::Horizontal, QString("Subscriber"));
	updateLabelModel();
	return labelModel;
}

void clearMarks()
{
	for (int x = 0; x < mapWidth; x++) // clear old marks
	{
		for (int y = 0; y < mapHeight; y++)
		{
			MAPTILE *psTile = mapTile(x, y);
			psTile->tileInfoBits &= ~BITS_MARKED;
		}
	}
}

void markAllLabels(bool only_active)
{
	for (const auto &it : labels)
	{
		const auto &key = it.first;
		const LABEL &l = it.second;
		if (!only_active || l.triggered <= 0)
		{
			showLabel(key, false, false);
		}
	}
}

void showLabel(const std::string &key, bool clear_old, bool jump_to)
{
	if (labels.count(key) == 0)
	{
		debug(LOG_ERROR, "label %s not found", key.c_str());
		return;
	}
	LABEL &l = labels[key];
	if (clear_old)
	{
		clearMarks();
	}
	if (l.type == SCRIPT_AREA || l.type == SCRIPT_POSITION)
	{
		if (jump_to)
		{
			setViewPos(map_coord(l.p1.x), map_coord(l.p1.y), false); // move camera position
		}
		int maxx = map_coord(l.p2.x);
		int maxy = map_coord(l.p2.y);
		if (l.type == SCRIPT_POSITION)
		{
			maxx = MIN(mapWidth, maxx + 1);
			maxy = MIN(mapHeight, maxy + 1);
		}
		for (int x = map_coord(l.p1.x); x < maxx; x++) // make new ones
		{
			for (int y = map_coord(l.p1.y); y < maxy; y++)
			{
				MAPTILE *psTile = mapTile(x, y);
				psTile->tileInfoBits |= BITS_MARKED;
			}
		}
	}
	else if (l.type == SCRIPT_RADIUS)
	{
		if (jump_to)
		{
			setViewPos(map_coord(l.p1.x), map_coord(l.p1.y), false); // move camera position
		}
		for (int x = MAX(map_coord(l.p1.x - l.p2.x), 0); x < MIN(map_coord(l.p1.x + l.p2.x), mapWidth); x++)
		{
			for (int y = MAX(map_coord(l.p1.y - l.p2.x), 0); y < MIN(map_coord(l.p1.y + l.p2.x), mapHeight); y++) // l.p2.x is radius, not a bug
			{
				if (iHypot(map_coord(l.p1) - Vector2i(x, y)) < map_coord(l.p2.x))
				{
					MAPTILE *psTile = mapTile(x, y);
					psTile->tileInfoBits |= BITS_MARKED;
				}
			}
		}
	}
	else if (l.type == OBJ_DROID || l.type == OBJ_FEATURE || l.type == OBJ_STRUCTURE)
	{
		BASE_OBJECT *psObj = IdToObject((OBJECT_TYPE)l.type, l.id, l.player);
		if (psObj)
		{
			if (jump_to)
			{
				setViewPos(map_coord(psObj->pos.x), map_coord(psObj->pos.y), false); // move camera position
			}
			MAPTILE *psTile = mapTile(map_coord(psObj->pos.x), map_coord(psObj->pos.y));
			psTile->tileInfoBits |= BITS_MARKED;
		}
	}
	else if (l.type == SCRIPT_GROUP)
	{
		bool cameraMoved = false;
		for (ENGINEMAP::iterator i = groups.begin(); i != groups.end(); ++i)
		{
			const GROUPMAP *pGroupMap = i->second;
			for (GROUPMAP::const_iterator iter = pGroupMap->begin(); iter != pGroupMap->end(); ++iter)
			{
				if (iter->second == l.id)
				{
					BASE_OBJECT *psObj = iter->first;
					if (!cameraMoved && jump_to)
					{
						setViewPos(map_coord(psObj->pos.x), map_coord(psObj->pos.y), false); // move camera position
						cameraMoved = true;
					}
					MAPTILE *psTile = mapTile(map_coord(psObj->pos.x), map_coord(psObj->pos.y));
					psTile->tileInfoBits |= BITS_MARKED;
				}
			}
		}
	}
}

// The bool return value is true when an object callback needs to be called.
// The int return value holds group id when a group callback needs to be called, 0 otherwise.
std::pair<bool, int> seenLabelCheck(QScriptEngine *engine, BASE_OBJECT *seen, BASE_OBJECT *viewer)
{
	GROUPMAP *psMap = getGroupMap(engine);
	ASSERT_OR_RETURN(std::make_pair(false, 0), psMap != nullptr, "Non-existent groupmap for engine");
	auto seenObjIt = psMap->find(seen);
	int groupId = (seenObjIt != psMap->end()) ? seenObjIt->second : 0;
	bool foundObj = false, foundGroup = false;
	for (auto &it : labels)
	{
		LABEL &l = it.second;
		if (l.triggered != 0 || !(l.subscriber == ALL_PLAYERS || l.subscriber == viewer->player))
		{
			continue;
		}

		// Don't let a seen game object ID which matches a group label ID to prematurely
		// trigger a group label.
		if (l.type != SCRIPT_GROUP && l.id == seen->id)
		{
			l.triggered = viewer->id; // record who made the discovery
			foundObj = true;
		}
		else if (l.type == SCRIPT_GROUP && l.id == groupId)
		{
			l.triggered = viewer->id; // record who made the discovery
			foundGroup = true;
		}
	}
	if (foundObj || foundGroup)
	{
		updateLabelModel();
	}
	return std::make_pair(foundObj, foundGroup ? groupId : 0);
}

bool areaLabelCheck(DROID *psDroid)
{
	int x = psDroid->pos.x;
	int y = psDroid->pos.y;
	bool activated = false;
	for (LABELMAP::iterator i = labels.begin(); i != labels.end(); i++)
	{
		LABEL &l = i->second;
		if (l.triggered == 0 && (l.subscriber == ALL_PLAYERS || l.subscriber == psDroid->player)
		    && ((l.type == SCRIPT_AREA && l.p1.x < x && l.p1.y < y && l.p2.x > x && l.p2.y > y)
		        || (l.type == SCRIPT_RADIUS && iHypot(l.p1 - psDroid->pos.xy()) < l.p2.x)))
		{
			// We're inside an untriggered area
			activated = true;
			l.triggered = psDroid->id;
			triggerEventArea(i->first, psDroid);
		}
	}
	if (activated)
	{
		updateLabelModel();
	}
	return activated;
}

static void removeFromGroup(QScriptEngine *engine, GROUPMAP *psMap, BASE_OBJECT *psObj)
{
	auto it = psMap->find(psObj);
	if (it != psMap->end())
	{
		int groupId = it->second;
		psMap->erase(it);
		QScriptValue groupMembers = engine->globalObject().property("groupSizes");
		const int newValue = groupMembers.property(groupId).toInt32() - 1;
		ASSERT(newValue >= 0, "Bad group count in group %d (was %d)", groupId, newValue + 1);
		groupMembers.setProperty(groupId, newValue, QScriptValue::ReadOnly);
		triggerEventGroupLoss(psObj, groupId, newValue, engine);
	}
}

void groupRemoveObject(BASE_OBJECT *psObj)
{
	for (ENGINEMAP::iterator i = groups.begin(); i != groups.end(); ++i)
	{
		removeFromGroup(i->first, i->second, psObj);
	}
}

static bool groupAddObject(BASE_OBJECT *psObj, int groupId, QScriptEngine *engine)
{
	ASSERT_OR_RETURN(false, psObj && engine, "Bad parameter");
	GROUPMAP *psMap = getGroupMap(engine);
	removeFromGroup(engine, psMap, psObj);
	QScriptValue groupMembers = engine->globalObject().property("groupSizes");
	int prev = groupMembers.property(QString::number(groupId)).toInt32();
	groupMembers.setProperty(QString::number(groupId), prev + 1, QScriptValue::ReadOnly);
	psMap->insert(std::pair<BASE_OBJECT *, int>(psObj, groupId));
	return true; // inserted
}

//;; ## Research
//;;
//;; Describes a research item. The following properties are defined:
//;;
//;; * ```power``` Number of power points needed for starting the research.
//;; * ```points``` Number of research points needed to complete the research.
//;; * ```started``` A boolean saying whether or not this research has been started by current player or any of its allies.
//;; * ```done``` A boolean saying whether or not this research has been completed.
//;; * ```name``` A string containing the full name of the research.
//;; * ```id``` A string containing the index name of the research.
//;; * ```type``` The type will always be ```RESEARCH_DATA```.
//;;
QScriptValue convResearch(const RESEARCH *psResearch, QScriptEngine *engine, int player)
{
	if (psResearch == nullptr)
	{
		return QScriptValue::NullValue;
	}
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
	value.setProperty("done", IsResearchCompleted(&asPlayerResList[player][psResearch->index]));
	value.setProperty("fullname", WzStringToQScriptValue(engine, psResearch->name)); // temporary
	value.setProperty("name", WzStringToQScriptValue(engine, psResearch->id)); // will be changed to contain fullname
	value.setProperty("id", WzStringToQScriptValue(engine, psResearch->id));
	value.setProperty("type", SCRIPT_RESEARCH);
	value.setProperty("results", mapJsonToQScriptValue(engine, psResearch->results, QScriptValue::ReadOnly | QScriptValue::Undeletable));
	return value;
}

//;; ## Structure
//;;
//;; Describes a structure (building). It inherits all the properties of the base object (see below).
//;; In addition, the following properties are defined:
//;;
//;; * ```status``` The completeness status of the structure. It will be one of ```BEING_BUILT``` and ```BUILT```.
//;; * ```type``` The type will always be ```STRUCTURE```.
//;; * ```cost``` What it would cost to build this structure. (3.2+ only)
//;; * ```stattype``` The stattype defines the type of structure. It will be one of ```HQ```, ```FACTORY```, ```POWER_GEN```,
//;; ```RESOURCE_EXTRACTOR```, ```LASSAT```, ```DEFENSE```, ```WALL```, ```RESEARCH_LAB```, ```REPAIR_FACILITY```,
//;; ```CYBORG_FACTORY```, ```VTOL_FACTORY```, ```REARM_PAD```, ```SAT_UPLINK```, ```GATE``` and ```COMMAND_CONTROL```.
//;; * ```modules``` If the stattype is set to one of the factories, ```POWER_GEN``` or ```RESEARCH_LAB```, then this property is set to the
//;; number of module upgrades it has.
//;; * ```canHitAir``` True if the structure has anti-air capabilities. (3.2+ only)
//;; * ```canHitGround``` True if the structure has anti-ground capabilities. (3.2+ only)
//;; * ```isSensor``` True if the structure has sensor ability. (3.2+ only)
//;; * ```isCB``` True if the structure has counter-battery ability. (3.2+ only)
//;; * ```isRadarDetector``` True if the structure has radar detector ability. (3.2+ only)
//;; * ```range``` Maximum range of its weapons. (3.2+ only)
//;; * ```hasIndirect``` One or more of the structure's weapons are indirect. (3.2+ only)
//;;
QScriptValue convStructure(const STRUCTURE *psStruct, QScriptEngine *engine)
{
	bool aa = false;
	bool ga = false;
	bool indirect = false;
	int range = -1;
	for (int i = 0; i < psStruct->numWeaps; i++)
	{
		if (psStruct->asWeaps[i].nStat)
		{
			WEAPON_STATS *psWeap = &asWeaponStats[psStruct->asWeaps[i].nStat];
			aa = aa || psWeap->surfaceToAir & SHOOT_IN_AIR;
			ga = ga || psWeap->surfaceToAir & SHOOT_ON_GROUND;
			indirect = indirect || psWeap->movementModel == MM_INDIRECT || psWeap->movementModel == MM_HOMINGINDIRECT;
			range = MAX(proj_GetLongRange(psWeap, psStruct->player), range);
		}
	}
	QScriptValue value = convObj(psStruct, engine);
	value.setProperty("isCB", structCBSensor(psStruct), QScriptValue::ReadOnly);
	value.setProperty("isSensor", structStandardSensor(psStruct), QScriptValue::ReadOnly);
	value.setProperty("canHitAir", aa, QScriptValue::ReadOnly);
	value.setProperty("canHitGround", ga, QScriptValue::ReadOnly);
	value.setProperty("hasIndirect", indirect, QScriptValue::ReadOnly);
	value.setProperty("isRadarDetector", objRadarDetector(psStruct), QScriptValue::ReadOnly);
	value.setProperty("range", range, QScriptValue::ReadOnly);
	value.setProperty("status", (int)psStruct->status, QScriptValue::ReadOnly);
	value.setProperty("health", 100 * psStruct->body / MAX(1, structureBody(psStruct)), QScriptValue::ReadOnly);
	value.setProperty("cost", psStruct->pStructureType->powerToBuild, QScriptValue::ReadOnly);
	switch (psStruct->pStructureType->type) // don't bleed our source insanities into the scripting world
	{
	case REF_WALL:
	case REF_WALLCORNER:
	case REF_GATE:
		value.setProperty("stattype", (int)REF_WALL, QScriptValue::ReadOnly);
		break;
	case REF_GENERIC:
	case REF_DEFENSE:
		value.setProperty("stattype", (int)REF_DEFENSE, QScriptValue::ReadOnly);
		break;
	default:
		value.setProperty("stattype", (int)psStruct->pStructureType->type, QScriptValue::ReadOnly);
		break;
	}
	if (psStruct->pStructureType->type == REF_FACTORY || psStruct->pStructureType->type == REF_CYBORG_FACTORY
	    || psStruct->pStructureType->type == REF_VTOL_FACTORY
	    || psStruct->pStructureType->type == REF_RESEARCH
	    || psStruct->pStructureType->type == REF_POWER_GEN)
	{
		value.setProperty("modules", psStruct->capacity, QScriptValue::ReadOnly);
	}
	else
	{
		value.setProperty("modules", QScriptValue::NullValue);
	}
	QScriptValue weaponlist = engine->newArray(psStruct->numWeaps);
	for (int j = 0; j < psStruct->numWeaps; j++)
	{
		QScriptValue weapon = engine->newObject();
		const WEAPON_STATS *psStats = asWeaponStats + psStruct->asWeaps[j].nStat;
		weapon.setProperty("fullname", WzStringToQScriptValue(engine, psStats->name), QScriptValue::ReadOnly);
		weapon.setProperty("name", WzStringToQScriptValue(engine, psStats->id), QScriptValue::ReadOnly); // will be changed to contain full name
		weapon.setProperty("id", WzStringToQScriptValue(engine, psStats->id), QScriptValue::ReadOnly);
		weapon.setProperty("lastFired", psStruct->asWeaps[j].lastFired, QScriptValue::ReadOnly);
		weaponlist.setProperty(j, weapon, QScriptValue::ReadOnly);
	}
	value.setProperty("weapons", weaponlist, QScriptValue::ReadOnly);
	return value;
}

//;; ## Feature
//;;
//;; Describes a feature (a **game object** not owned by any player). It inherits all the properties of the base object (see below).
//;; In addition, the following properties are defined:
//;; * ```type``` It will always be ```FEATURE```.
//;; * ```stattype``` The type of feature. Defined types are ```OIL_RESOURCE```, ```OIL_DRUM``` and ```ARTIFACT```.
//;; * ```damageable``` Can this feature be damaged?
//;;
QScriptValue convFeature(const FEATURE *psFeature, QScriptEngine *engine)
{
	QScriptValue value = convObj(psFeature, engine);
	const FEATURE_STATS *psStats = psFeature->psStats;
	value.setProperty("health", 100 * psStats->body / MAX(1, psFeature->body), QScriptValue::ReadOnly);
	value.setProperty("damageable", psStats->damageable, QScriptValue::ReadOnly);
	value.setProperty("stattype", psStats->subType, QScriptValue::ReadOnly);
	return value;
}

//;; ## Droid
//;;
//;; Describes a droid. It inherits all the properties of the base object (see below).
//;; In addition, the following properties are defined:
//;;
//;; * ```type``` It will always be ```DROID```.
//;; * ```order``` The current order of the droid. This is its plan. The following orders are defined:
//;;   * ```DORDER_ATTACK``` Order a droid to attack something.
//;;   * ```DORDER_MOVE``` Order a droid to move somewhere.
//;;   * ```DORDER_SCOUT``` Order a droid to move somewhere and stop to attack anything on the way.
//;;   * ```DORDER_BUILD``` Order a droid to build something.
//;;   * ```DORDER_HELPBUILD``` Order a droid to help build something.
//;;   * ```DORDER_LINEBUILD``` Order a droid to build something repeatedly in a line.
//;;   * ```DORDER_REPAIR``` Order a droid to repair something.
//;;   * ```DORDER_PATROL``` Order a droid to patrol.
//;;   * ```DORDER_DEMOLISH``` Order a droid to demolish something.
//;;   * ```DORDER_EMBARK``` Order a droid to embark on a transport.
//;;   * ```DORDER_DISEMBARK``` Order a transport to disembark its units at the given position.
//;;   * ```DORDER_FIRESUPPORT``` Order a droid to fire at whatever the target sensor is targeting. (3.2+ only)
//;;   * ```DORDER_COMMANDERSUPPORT``` Assign the droid to a commander. (3.2+ only)
//;;   * ```DORDER_STOP``` Order a droid to stop whatever it is doing. (3.2+ only)
//;;   * ```DORDER_RTR``` Order a droid to return for repairs. (3.2+ only)
//;;   * ```DORDER_RTB``` Order a droid to return to base. (3.2+ only)
//;;   * ```DORDER_HOLD``` Order a droid to hold its position. (3.2+ only)
//;;   * ```DORDER_REARM``` Order a VTOL droid to rearm. If given a target, will go to specified rearm pad. If not, will go to nearest rearm pad. (3.2+ only)
//;;   * ```DORDER_OBSERVE``` Order a droid to keep a target in sensor view. (3.2+ only)
//;;   * ```DORDER_RECOVER``` Order a droid to pick up something. (3.2+ only)
//;;   * ```DORDER_RECYCLE``` Order a droid to factory for recycling. (3.2+ only)
//;; * ```action``` The current action of the droid. This is how it intends to carry out its plan. The
//;; C++ code may change the action frequently as it tries to carry out its order. You never want to set
//;; the action directly, but it may be interesting to look at what it currently is.
//;; * ```droidType``` The droid's type. The following types are defined:
//;;   * ```DROID_CONSTRUCT``` Trucks and cyborg constructors.
//;;   * ```DROID_WEAPON``` Droids with weapon turrets, except cyborgs.
//;;   * ```DROID_PERSON``` Non-cyborg two-legged units, like scavengers.
//;;   * ```DROID_REPAIR``` Units with repair turret, including repair cyborgs.
//;;   * ```DROID_SENSOR``` Units with sensor turret.
//;;   * ```DROID_ECM``` Unit with ECM jammer turret.
//;;   * ```DROID_CYBORG``` Cyborgs with weapons.
//;;   * ```DROID_TRANSPORTER``` Cyborg transporter.
//;;   * ```DROID_SUPERTRANSPORTER``` Droid transporter.
//;;   * ```DROID_COMMAND``` Commanders.
//;; * ```group``` The group this droid is member of. This is a numerical ID. If not a member of any group, will be set to \emph{null}.
//;; * ```armed``` The percentage of weapon capability that is fully armed. Will be \emph{null} for droids other than VTOLs.
//;; * ```experience``` Amount of experience this droid has, based on damage it has dealt to enemies.
//;; * ```cost``` What it would cost to build the droid. (3.2+ only)
//;; * ```isVTOL``` True if the droid is VTOL. (3.2+ only)
//;; * ```canHitAir``` True if the droid has anti-air capabilities. (3.2+ only)
//;; * ```canHitGround``` True if the droid has anti-ground capabilities. (3.2+ only)
//;; * ```isSensor``` True if the droid has sensor ability. (3.2+ only)
//;; * ```isCB``` True if the droid has counter-battery ability. (3.2+ only)
//;; * ```isRadarDetector``` True if the droid has radar detector ability. (3.2+ only)
//;; * ```hasIndirect``` One or more of the droid's weapons are indirect. (3.2+ only)
//;; * ```range``` Maximum range of its weapons. (3.2+ only)
//;; * ```body``` The body component of the droid. (3.2+ only)
//;; * ```propulsion``` The propulsion component of the droid. (3.2+ only)
//;; * ```weapons``` The weapon components of the droid, as an array. Contains 'name', 'id', 'armed' percentage and 'lastFired' properties. (3.2+ only)
//;; * ```cargoCapacity``` Defined for transporters only: Total cargo capacity (number of items that will fit may depend on their size). (3.2+ only)
//;; * ```cargoSpace``` Defined for transporters only: Cargo capacity left. (3.2+ only)
//;; * ```cargoCount``` Defined for transporters only: Number of individual \emph{items} in the cargo hold. (3.2+ only)
//;; * ```cargoSize``` The amount of cargo space the droid will take inside a transport. (3.2+ only)
//;;
QScriptValue convDroid(const DROID *psDroid, QScriptEngine *engine)
{
	bool aa = false;
	bool ga = false;
	bool indirect = false;
	int range = -1;
	const BODY_STATS *psBodyStats = &asBodyStats[psDroid->asBits[COMP_BODY]];

	for (int i = 0; i < psDroid->numWeaps; i++)
	{
		if (psDroid->asWeaps[i].nStat)
		{
			WEAPON_STATS *psWeap = &asWeaponStats[psDroid->asWeaps[i].nStat];
			aa = aa || psWeap->surfaceToAir & SHOOT_IN_AIR;
			ga = ga || psWeap->surfaceToAir & SHOOT_ON_GROUND;
			indirect = indirect || psWeap->movementModel == MM_INDIRECT || psWeap->movementModel == MM_HOMINGINDIRECT;
			range = MAX(proj_GetLongRange(psWeap, psDroid->player), range);
		}
	}
	DROID_TYPE type = psDroid->droidType;
	QScriptValue value = convObj(psDroid, engine);
	value.setProperty("action", (int)psDroid->action, QScriptValue::ReadOnly);
	if (range >= 0)
	{
		value.setProperty("range", range, QScriptValue::ReadOnly);
	}
	else
	{
		value.setProperty("range", QScriptValue::NullValue);
	}
	value.setProperty("order", (int)psDroid->order.type, QScriptValue::ReadOnly);
	value.setProperty("cost", calcDroidPower(psDroid), QScriptValue::ReadOnly);
	value.setProperty("hasIndirect", indirect, QScriptValue::ReadOnly);
	switch (psDroid->droidType) // hide some engine craziness
	{
	case DROID_CYBORG_CONSTRUCT:
		type = DROID_CONSTRUCT; break;
	case DROID_CYBORG_SUPER:
		type = DROID_CYBORG; break;
	case DROID_DEFAULT:
		type = DROID_WEAPON; break;
	case DROID_CYBORG_REPAIR:
		type = DROID_REPAIR; break;
	default:
		break;
	}
	value.setProperty("bodySize", psBodyStats->size, QScriptValue::ReadOnly);
	if (isTransporter(psDroid))
	{
		value.setProperty("cargoCapacity", TRANSPORTER_CAPACITY, QScriptValue::ReadOnly);
		value.setProperty("cargoLeft", calcRemainingCapacity(psDroid), QScriptValue::ReadOnly);
		value.setProperty("cargoCount", psDroid->psGroup != nullptr? psDroid->psGroup->getNumMembers() : 0, QScriptValue::ReadOnly);
	}
	value.setProperty("isRadarDetector", objRadarDetector(psDroid), QScriptValue::ReadOnly);
	value.setProperty("isCB", cbSensorDroid(psDroid), QScriptValue::ReadOnly);
	value.setProperty("isSensor", standardSensorDroid(psDroid), QScriptValue::ReadOnly);
	value.setProperty("canHitAir", aa, QScriptValue::ReadOnly);
	value.setProperty("canHitGround", ga, QScriptValue::ReadOnly);
	value.setProperty("isVTOL", isVtolDroid(psDroid), QScriptValue::ReadOnly);
	value.setProperty("droidType", (int)type, QScriptValue::ReadOnly);
	value.setProperty("experience", (double)psDroid->experience / 65536.0, QScriptValue::ReadOnly);
	value.setProperty("health", 100.0 / (double)psDroid->originalBody * (double)psDroid->body, QScriptValue::ReadOnly);
	value.setProperty("body", WzStringToQScriptValue(engine, asBodyStats[psDroid->asBits[COMP_BODY]].id), QScriptValue::ReadOnly);
	value.setProperty("propulsion", WzStringToQScriptValue(engine, asPropulsionStats[psDroid->asBits[COMP_PROPULSION]].id), QScriptValue::ReadOnly);
	value.setProperty("armed", 0.0, QScriptValue::ReadOnly); // deprecated!
	QScriptValue weaponlist = engine->newArray(psDroid->numWeaps);
	for (int j = 0; j < psDroid->numWeaps; j++)
	{
		int armed = droidReloadBar(psDroid, &psDroid->asWeaps[j], j);
		QScriptValue weapon = engine->newObject();
		const WEAPON_STATS *psStats = asWeaponStats + psDroid->asWeaps[j].nStat;
		weapon.setProperty("fullname", WzStringToQScriptValue(engine, psStats->name), QScriptValue::ReadOnly);
		weapon.setProperty("id", WzStringToQScriptValue(engine, psStats->id), QScriptValue::ReadOnly); // will be changed to full name
		weapon.setProperty("name", WzStringToQScriptValue(engine, psStats->id), QScriptValue::ReadOnly);
		weapon.setProperty("lastFired", psDroid->asWeaps[j].lastFired, QScriptValue::ReadOnly);
		weapon.setProperty("armed", armed, QScriptValue::ReadOnly);
		weaponlist.setProperty(j, weapon, QScriptValue::ReadOnly);
	}
	value.setProperty("weapons", weaponlist, QScriptValue::ReadOnly);
	value.setProperty("cargoSize", transporterSpaceRequired(psDroid), QScriptValue::ReadOnly);
	return value;
}

//;; ## Base Object
//;;
//;; Describes a basic object. It will always be a droid, structure or feature, but sometimes the
//;; difference does not matter, and you can treat any of them simply as a basic object. These
//;; fields are also inherited by the droid, structure and feature objects.
//;; The following properties are defined:
//;;
//;; * ```type``` It will be one of ```DROID```, ```STRUCTURE``` or ```FEATURE```.
//;; * ```id``` The unique ID of this object.
//;; * ```x``` X position of the object in tiles.
//;; * ```y``` Y position of the object in tiles.
//;; * ```z``` Z (height) position of the object in tiles.
//;; * ```player``` The player owning this object.
//;; * ```selected``` A boolean saying whether 'selectedPlayer' has selected this object.
//;; * ```name``` A user-friendly name for this object.
//;; * ```health``` Percentage that this object is damaged (where 100 means not damaged at all).
//;; * ```armour``` Amount of armour points that protect against kinetic weapons.
//;; * ```thermal``` Amount of thermal protection that protect against heat based weapons.
//;; * ```born``` The game time at which this object was produced or came into the world. (3.2+ only)
//;;
QScriptValue convObj(const BASE_OBJECT *psObj, QScriptEngine *engine)
{
	QScriptValue value = engine->newObject();
	ASSERT_OR_RETURN(value, psObj, "No object for conversion");
	value.setProperty("id", psObj->id, QScriptValue::ReadOnly);
	value.setProperty("x", map_coord(psObj->pos.x), QScriptValue::ReadOnly);
	value.setProperty("y", map_coord(psObj->pos.y), QScriptValue::ReadOnly);
	value.setProperty("z", map_coord(psObj->pos.z), QScriptValue::ReadOnly);
	value.setProperty("player", psObj->player, QScriptValue::ReadOnly);
	value.setProperty("armour", objArmour(psObj, WC_KINETIC), QScriptValue::ReadOnly);
	value.setProperty("thermal", objArmour(psObj, WC_HEAT), QScriptValue::ReadOnly);
	value.setProperty("type", psObj->type, QScriptValue::ReadOnly);
	value.setProperty("selected", psObj->selected, QScriptValue::ReadOnly);
	value.setProperty("name", objInfo(psObj), QScriptValue::ReadOnly);
	value.setProperty("born", psObj->born, QScriptValue::ReadOnly);
	GROUPMAP *psMap = getGroupMap(engine);
	if (psMap != nullptr && psMap->count(const_cast<BASE_OBJECT *>(psObj)) > 0) // FIXME:
	{
		int group = psMap->at(const_cast<BASE_OBJECT *>(psObj)); // FIXME:
		value.setProperty("group", group, QScriptValue::ReadOnly);
	}
	else
	{
		value.setProperty("group", QScriptValue::NullValue);
	}
	return value;
}

//;; ## Template
//;;
//;; Describes a template type. Templates are droid designs that a player has created.
//;; The following properties are defined:
//;;
//;; * ```id``` The ID of this object.
//;; * ```name``` Name of the template.
//;; * ```cost``` The power cost of the template if put into production.
//;; * ```droidType``` The type of droid that would be created.
//;; * ```body``` The name of the body type.
//;; * ```propulsion``` The name of the propulsion type.
//;; * ```brain``` The name of the brain type.
//;; * ```repair``` The name of the repair type.
//;; * ```ecm``` The name of the ECM (electronic counter-measure) type.
//;; * ```construct``` The name of the construction type.
//;; * ```weapons``` An array of weapon names attached to this template.
QScriptValue convTemplate(const DROID_TEMPLATE *psTempl, QScriptEngine *engine)
{
	QScriptValue value = engine->newObject();
	ASSERT_OR_RETURN(value, psTempl, "No object for conversion");
	value.setProperty("fullname", WzStringToQScriptValue(engine, psTempl->name), QScriptValue::ReadOnly);
	value.setProperty("name", WzStringToQScriptValue(engine, psTempl->id), QScriptValue::ReadOnly);
	value.setProperty("id", WzStringToQScriptValue(engine, psTempl->id), QScriptValue::ReadOnly);
	value.setProperty("points", calcTemplateBuild(psTempl), QScriptValue::ReadOnly);
	value.setProperty("power", calcTemplatePower(psTempl), QScriptValue::ReadOnly); // deprecated, use cost below
	value.setProperty("cost", calcTemplatePower(psTempl), QScriptValue::ReadOnly);
	value.setProperty("droidType", psTempl->droidType, QScriptValue::ReadOnly);
	value.setProperty("body", WzStringToQScriptValue(engine, (asBodyStats + psTempl->asParts[COMP_BODY])->id), QScriptValue::ReadOnly);
	value.setProperty("propulsion", WzStringToQScriptValue(engine, (asPropulsionStats + psTempl->asParts[COMP_PROPULSION])->id), QScriptValue::ReadOnly);
	value.setProperty("brain", WzStringToQScriptValue(engine, (asBrainStats + psTempl->asParts[COMP_BRAIN])->id), QScriptValue::ReadOnly);
	value.setProperty("repair", WzStringToQScriptValue(engine, (asRepairStats + psTempl->asParts[COMP_REPAIRUNIT])->id), QScriptValue::ReadOnly);
	value.setProperty("ecm", WzStringToQScriptValue(engine, (asECMStats + psTempl->asParts[COMP_ECM])->id), QScriptValue::ReadOnly);
	value.setProperty("sensor", WzStringToQScriptValue(engine, (asSensorStats + psTempl->asParts[COMP_SENSOR])->id), QScriptValue::ReadOnly);
	value.setProperty("construct", WzStringToQScriptValue(engine, (asConstructStats + psTempl->asParts[COMP_CONSTRUCT])->id), QScriptValue::ReadOnly);
	QScriptValue weaponlist = engine->newArray(psTempl->numWeaps);
	for (int j = 0; j < psTempl->numWeaps; j++)
	{
		weaponlist.setProperty(j, WzStringToQScriptValue(engine, (asWeaponStats + psTempl->asWeaps[j])->id), QScriptValue::ReadOnly);
	}
	value.setProperty("weapons", weaponlist);
	return value;
}

QScriptValue convMax(const BASE_OBJECT *psObj, QScriptEngine *engine)
{
	if (!psObj)
	{
		return QScriptValue::NullValue;
	}
	switch (psObj->type)
	{
	case OBJ_DROID: return convDroid((const DROID *)psObj, engine);
	case OBJ_STRUCTURE: return convStructure((const STRUCTURE *)psObj, engine);
	case OBJ_FEATURE: return convFeature((const FEATURE *)psObj, engine);
	default: ASSERT(false, "No such supported object type"); return convObj(psObj, engine);
	}
}

BASE_OBJECT *IdToObject(OBJECT_TYPE type, int id, int player)
{
	switch (type)
	{
	case OBJ_DROID: return IdToDroid(id, player);
	case OBJ_FEATURE: return IdToFeature(id, player);
	case OBJ_STRUCTURE: return IdToStruct(id, player);
	default: return nullptr;
	}
}

// ----------------------------------------------------------------------------------------
// Group system
//

bool loadGroup(QScriptEngine *engine, int groupId, int objId)
{
	BASE_OBJECT *psObj = IdToPointer(objId, ANYPLAYER);
	ASSERT_OR_RETURN(false, psObj, "Non-existent object %d in group %d in savegame", objId, groupId);
	return groupAddObject(psObj, groupId, engine);
}

bool saveGroups(WzConfig &ini, QScriptEngine *engine)
{
	// Save group info as a list of group memberships for each droid
	GROUPMAP *psMap = getGroupMap(engine);
	ASSERT_OR_RETURN(false, psMap, "Non-existent groupmap for engine");
	for (GROUPMAP::const_iterator i = psMap->begin(); i != psMap->end(); ++i)
	{
		std::vector<WzString> value;
		BASE_OBJECT *psObj = i->first;
		ASSERT(!isDead(psObj), "Wanted to save dead %s to savegame!", objInfo(psObj));
		if (ini.contains(WzString::number(psObj->id)))
		{
			value.push_back(ini.value(WzString::number(psObj->id)).toWzString());
		}
		value.push_back(WzString::number(i->second));
		ini.setValue(WzString::number(psObj->id), value);
	}
	return true;
}

// ----------------------------------------------------------------------------------------
// Label system (function defined in qtscript.h header)
//

// Load labels
bool loadLabels(const char *filename)
{
	int groupidx = -1;

	if (!PHYSFS_exists(filename))
	{
		debug(LOG_SAVE, "No %s found -- not adding any labels", filename);
		return false;
	}
	WzConfig ini(filename, WzConfig::ReadOnly);
	labels.clear();
	std::vector<WzString> list = ini.childGroups();
	debug(LOG_SAVE, "Loading %zu labels...", list.size());
	for (int i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		LABEL p;
		std::string label = ini.value("label").toWzString().toUtf8();
		if (labels.count(label) > 0)
		{
			debug(LOG_ERROR, "Duplicate label found");
		}
		else if (list[i].startsWith("position"))
		{
			p.p1 = ini.vector2i("pos");
			p.p2 = p.p1;
			p.type = SCRIPT_POSITION;
			p.player = ALL_PLAYERS;
			p.id = -1;
			p.triggered = -1; // always deactivated
			labels[label] = p;
			p.triggered = ini.value("triggered", -1).toInt(); // deactivated by default
		}
		else if (list[i].startsWith("area"))
		{
			p.p1 = ini.vector2i("pos1");
			p.p2 = ini.vector2i("pos2");
			p.type = SCRIPT_AREA;
			p.player = ini.value("player", ALL_PLAYERS).toInt();
			p.triggered = ini.value("triggered", 0).toInt(); // activated by default
			p.id = -1;
			p.subscriber = ini.value("subscriber", ALL_PLAYERS).toInt();
			labels[label] = p;
		}
		else if (list[i].startsWith("radius"))
		{
			p.p1 = ini.vector2i("pos");
			p.p2.x = ini.value("radius").toInt();
			p.p2.y = 0; // unused
			p.type = SCRIPT_RADIUS;
			p.player = ini.value("player", ALL_PLAYERS).toInt();
			p.triggered = ini.value("triggered", 0).toInt(); // activated by default
			p.subscriber = ini.value("subscriber", ALL_PLAYERS).toInt();
			p.id = -1;
			labels[label] = p;
			p.triggered = ini.value("triggered", -1).toInt(); // deactivated by default
		}
		else if (list[i].startsWith("object"))
		{
			p.id = ini.value("id").toInt();
			p.type = ini.value("type").toInt();
			p.player = ini.value("player").toInt();
			labels[label] = p;
			p.triggered = ini.value("triggered", -1).toInt(); // deactivated by default
			p.subscriber = ini.value("subscriber", ALL_PLAYERS).toInt();
		}
		else if (list[i].startsWith("group"))
		{
			p.id = groupidx--;
			p.type = SCRIPT_GROUP;
			p.player = ini.value("player").toInt();
			std::vector<WzString> memberList = ini.value("members").toWzStringList();
			for (WzString const &j : memberList)
			{
				int id = j.toInt();
				BASE_OBJECT *psObj = IdToPointer(id, p.player);
				ASSERT(psObj, "Unit %d belonging to player %d not found from label %s",
				       id, p.player, list[i].toUtf8().c_str());
				p.idlist.push_back(id);
			}
			labels[label] = p;
			p.triggered = ini.value("triggered", -1).toInt(); // deactivated by default
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
	int c[5]; // make unique, incremental section names
	memset(c, 0, sizeof(c));
	WzConfig ini(filename, WzConfig::ReadAndWrite);
	for (LABELMAP::const_iterator i = labels.begin(); i != labels.end(); i++)
	{
		const std::string& key = i->first;
		const LABEL &l = i->second;
		if (l.type == SCRIPT_POSITION)
		{
			ini.beginGroup("position_" + WzString::number(c[0]++));
			ini.setVector2i("pos", l.p1);
			ini.setValue("label", WzString::fromUtf8(key));
			ini.setValue("triggered", l.triggered);
			ini.endGroup();
		}
		else if (l.type == SCRIPT_AREA)
		{
			ini.beginGroup("area_" + WzString::number(c[1]++));
			ini.setVector2i("pos1", l.p1);
			ini.setVector2i("pos2", l.p2);
			ini.setValue("label", WzString::fromUtf8(key));
			ini.setValue("player", l.player);
			ini.setValue("triggered", l.triggered);
			ini.setValue("subscriber", l.subscriber);
			ini.endGroup();
		}
		else if (l.type == SCRIPT_RADIUS)
		{
			ini.beginGroup("radius_" + WzString::number(c[2]++));
			ini.setVector2i("pos", l.p1);
			ini.setValue("radius", l.p2.x);
			ini.setValue("label", WzString::fromUtf8(key));
			ini.setValue("player", l.player);
			ini.setValue("triggered", l.triggered);
			ini.setValue("subscriber", l.subscriber);
			ini.endGroup();
		}
		else if (l.type == SCRIPT_GROUP)
		{
			ini.beginGroup("group_" + WzString::number(c[3]++));
			ini.setValue("player", l.player);
			ini.setValue("triggered", l.triggered);
			QStringList list;
			for (int i : l.idlist)
			{
				list += QString::number(i);
			}
			std::list<WzString> wzlist;
			std::transform(list.constBegin(), list.constEnd(), std::back_inserter(wzlist),
						   [](const QString& qs) -> WzString { return QStringToWzString(qs); });
			ini.setValue("members", wzlist);
			ini.setValue("label", WzString::fromUtf8(key));
			ini.setValue("subscriber", l.subscriber);
			ini.endGroup();
		}
		else
		{
			ini.beginGroup("object_" + WzString::number(c[4]++));
			ini.setValue("id", l.id);
			ini.setValue("player", l.player);
			ini.setValue("type", l.type);
			ini.setValue("label", WzString::fromUtf8(key));
			ini.setValue("triggered", l.triggered);
			ini.endGroup();
		}
	}
	return true;
}

// ----------------------------------------------------------------------------------------

	class qtscript_execution_context : public wzapi::execution_context
	{
	private:
		QScriptContext *context = nullptr;
		QScriptEngine *engine = nullptr;
	public:
		qtscript_execution_context(QScriptContext *context, QScriptEngine *engine)
		: context(context), engine(engine)
		{ }
		~qtscript_execution_context() { }
	public:
		virtual int player() const override
		{
			return engine->globalObject().property("me").toInt32();
		}

		virtual void throwError(const char *expr, int line, const char *function) const override
		{
			context->throwError(QScriptContext::ReferenceError, QString(expr) +  " failed in " + QString(function) + " at line " + QString::number(line));
		}

		virtual playerCallbackFunc getNamedScriptCallback(const WzString& func) const override
		{
			QScriptEngine *pEngine = engine;
			return [pEngine, func](const int player) {
				::namedScriptCallback(pEngine, func, player);
			};
		}

		virtual void hack_setMe(int player) const override
		{
			engine->globalObject().setProperty("me", player);
		}

		virtual void set_isReceivingAllEvents(bool value) const override
		{
			engine->globalObject().setProperty("isReceivingAllEvents", value, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		}

		virtual bool get_isReceivingAllEvents() const override
		{
			return (engine->globalObject().property("isReceivingAllEvents")).toBool();
		}

		virtual void doNotSaveGlobal(const std::string &name) const override
		{
			::doNotSaveGlobal(QString::fromStdString(name));
		}
	};

	/// Assert for scripts that give useful backtraces and other info.
	#define UNBOX_SCRIPT_ASSERT(context, expr, ...) \
		do { bool _wzeval = (expr); \
			if (!_wzeval) { debug(LOG_ERROR, __VA_ARGS__); \
				context->throwError(QScriptContext::ReferenceError, QString(#expr) +  " failed when converting argument " + QString::number(idx) + " for " + QString(function)); \
				break; } } while (0)

	namespace
	{
		template<typename T>
		struct unbox {
			//T operator()(size_t& idx, QScriptContext *context) const;
		};

		template<>
		struct unbox<int>
		{
			int operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				return context->argument(idx++).toInt32();
			}
		};

		template<>
		struct unbox<unsigned int>
		{
			unsigned int operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				return context->argument(idx++).toUInt32();
			}
		};

		template<>
		struct unbox<bool>
		{
			bool operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				return 	context->argument(idx++).toBool();
			}
		};



		template<>
		struct unbox<float>
		{
			float operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				return context->argument(idx++).toNumber();
			}
		};

		template<>
		struct unbox<double>
		{
			double operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				return context->argument(idx++).toNumber();
			}
		};

		template<>
		struct unbox<DROID*>
		{
			DROID* operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				QScriptValue droidVal = context->argument(idx++);
				int id = droidVal.property("id").toInt32();
				int player = droidVal.property("player").toInt32();
				DROID *psDroid = IdToDroid(id, player);
				UNBOX_SCRIPT_ASSERT(context, psDroid, "No such droid id %d belonging to player %d", id, player);
				return psDroid;
			}
		};

		template<>
		struct unbox<const DROID*>
		{
			const DROID* operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				return unbox<DROID*>()(idx, context, engine, function);
			}
		};

		template<>
		struct unbox<STRUCTURE*>
		{
			STRUCTURE* operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				QScriptValue structVal = context->argument(idx++);
				int id = structVal.property("id").toInt32();
				int player = structVal.property("player").toInt32();
				STRUCTURE *psStruct = IdToStruct(id, player);
				UNBOX_SCRIPT_ASSERT(context, psStruct, "No such structure id %d belonging to player %d", id, player);
				return psStruct;
			}
		};

		template<>
		struct unbox<const STRUCTURE*>
		{
			const STRUCTURE* operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				return unbox<STRUCTURE*>()(idx, context, engine, function);
			}
		};

		template<>
		struct unbox<BASE_OBJECT*>
		{
			BASE_OBJECT* operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				QScriptValue objVal = context->argument(idx++);
				int oid = objVal.property("id").toInt32();
				int oplayer = objVal.property("player").toInt32();
				OBJECT_TYPE otype = (OBJECT_TYPE)objVal.property("type").toInt32();
				BASE_OBJECT* psObj = IdToObject(otype, oid, oplayer);
				UNBOX_SCRIPT_ASSERT(context, psObj, "No such object id %d belonging to player %d", oid, oplayer);
				return psObj;
			}
		};

		template<>
		struct unbox<const BASE_OBJECT*>
		{
			const BASE_OBJECT* operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				return unbox<BASE_OBJECT*>()(idx, context, engine, function);
			}
		};

		template<>
		struct unbox<std::string>
		{
			std::string operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				return context->argument(idx++).toString().toStdString();
			}
		};

		template<>
		struct unbox<wzapi::STRUCTURE_TYPE_or_statsName_string>
		{
			wzapi::STRUCTURE_TYPE_or_statsName_string operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				wzapi::STRUCTURE_TYPE_or_statsName_string result;
				if (context->argumentCount() <= idx)
					return result;
				QScriptValue val = context->argument(idx++);
				if (val.isNumber())
				{
					result.type = (STRUCTURE_TYPE)val.toInt32();
				}
				else
				{
					result.statsName = val.toString().toStdString();
				}
				return result;
			}
		};

		template<typename OptionalType>
		struct unbox<optional<OptionalType>>
		{
			optional<OptionalType> operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				return optional<OptionalType>(unbox<OptionalType>()(idx, context, engine, function));
			}
		};

		template<>
		struct unbox<wzapi::reservedParam>
		{
			wzapi::reservedParam operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				// just ignore parameter value, and increment idx
				idx++;
				return {};
			}
		};


		template<>
		struct unbox<wzapi::droid_id_player>
		{
			wzapi::droid_id_player operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() < idx)
					return {};
				QScriptValue droidVal = context->argument(idx++);
				wzapi::droid_id_player result;
				result.id = droidVal.property("id").toInt32();
				result.player = droidVal.property("player").toInt32();
				return result;
			}
		};

		template<>
		struct unbox<wzapi::string_or_string_list>
		{
			wzapi::string_or_string_list operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				wzapi::string_or_string_list strings;

				QScriptValue list_or_string = context->argument(idx++);
				if (list_or_string.isArray())
				{
					int length = list_or_string.property("length").toInt32();
					for (int k = 0; k < length; k++)
					{
						QString resName = list_or_string.property(k).toString();
						strings.strings.push_back(resName.toStdString());
					}
				}
				else
				{
					QString resName = list_or_string.toString();
					strings.strings.push_back(resName.toStdString());
				}
				return strings;
			}
		};

		template<>
		struct unbox<wzapi::va_list_treat_as_strings>
		{
			wzapi::va_list_treat_as_strings operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				wzapi::va_list_treat_as_strings strings;
				for (; idx < context->argumentCount(); idx++)
				{
					QString s = context->argument(idx).toString();
					if (context->state() == QScriptContext::ExceptionState)
					{
						break;
					}
					strings.strings.push_back(s.toStdString());
				}
				return strings;
			}
		};

		template<typename ContainedType>
		struct unbox<wzapi::va_list<ContainedType>>
		{
			wzapi::va_list<ContainedType> operator()(size_t& idx, QScriptContext *context, QScriptEngine *engine, const char *function)
			{
				if (context->argumentCount() <= idx)
					return {};
				wzapi::va_list<ContainedType> result;
				for (; idx < context->argumentCount(); idx++)
				{
					result.va_list.push_back(unbox<ContainedType>()(idx, context, engine, function));
				}
				return result;
			}
		};

		template<typename T>
		QScriptValue box(T a, QScriptEngine*)
		{
			return QScriptValue(a);
		}

		QScriptValue box(std::string str, QScriptEngine* engine)
		{
			// The redundant QString cast is a workaround for a Qt5 bug, the QScriptValue(char const *) constructor interprets as Latin1 instead of UTF-8!
			return QScriptValue(QString::fromUtf8(str.c_str()));
		}

		QScriptValue box(wzapi::no_return_value, QScriptEngine* engine)
		{
			return QScriptValue();
		}

		QScriptValue box(const BASE_OBJECT * psObj, QScriptEngine* engine)
		{
			if (!psObj)
			{
				return QScriptValue::NullValue;
			}
			return convMax(psObj, engine);
		}

		QScriptValue box(const STRUCTURE * psStruct, QScriptEngine* engine)
		{
			if (!psStruct)
			{
				return QScriptValue::NullValue;
			}
			return convStructure(psStruct, engine);
		}

		QScriptValue box(const DROID * psDroid, QScriptEngine* engine)
		{
			if (!psDroid)
			{
				return QScriptValue::NullValue;
			}
			return convDroid(psDroid, engine);
		}

		QScriptValue box(const FEATURE * psFeat, QScriptEngine* engine)
		{
			if (!psFeat)
			{
				return QScriptValue::NullValue;
			}
			return convFeature(psFeat, engine);
		}

		QScriptValue box(const DROID_TEMPLATE * psTemplate, QScriptEngine* engine)
		{
			if (!psTemplate)
			{
				return QScriptValue::NullValue;
			}
			return convTemplate(psTemplate, engine);
		}

		QScriptValue box(Position p, QScriptEngine* engine)
		{
			QScriptValue v = engine->newObject();
			v.setProperty("x", map_coord(p.x), QScriptValue::ReadOnly);
			v.setProperty("y", map_coord(p.y), QScriptValue::ReadOnly);
			v.setProperty("type", SCRIPT_POSITION, QScriptValue::ReadOnly);
			return v;
		}

		QScriptValue box(wzapi::position_in_map_coords p, QScriptEngine* engine)
		{
			QScriptValue v = engine->newObject();
			v.setProperty("x", p.x, QScriptValue::ReadOnly);
			v.setProperty("y", p.y, QScriptValue::ReadOnly);
			v.setProperty("type", SCRIPT_POSITION, QScriptValue::ReadOnly);
			return v;
		}

		template<typename VectorType>
		QScriptValue box(const std::vector<VectorType>& value, QScriptEngine* engine)
		{
			QScriptValue result = engine->newArray(value.size());
			for (int i = 0; i < value.size(); i++)
			{
				VectorType item = value.at(i);
				result.setProperty(i, box(item, engine));
			}
			return result;
		}

		QScriptValue box(const wzapi::researchResult& result, QScriptEngine* engine)
		{
			if (result.psResearch == nullptr)
			{
				return QScriptValue::NullValue;
			}
			return convResearch(result.psResearch, engine, result.player);
		}

		QScriptValue box(const wzapi::researchResults& results, QScriptEngine* engine)
		{
			QScriptValue result = engine->newArray(results.resList.size());
			for (int i = 0; i < results.resList.size(); i++)
			{
				const RESEARCH *psResearch = results.resList.at(i);
				result.setProperty(i, convResearch(psResearch, engine, results.player));
			}
			return result;
		}

		template<typename OptionalType>
		QScriptValue box(const optional<OptionalType>& result, QScriptEngine* engine)
		{
			if (result.has_value())
			{
				return box(result.value(), engine);
			}
			else
			{
				return QScriptValue(); // "undefined" variable
				//return QScriptValue::NullValue;
			}
		}

		template<typename PtrType>
		QScriptValue box(std::unique_ptr<PtrType> result, QScriptEngine* engine)
		{
			if (result)
			{
				return box(result.get(), engine);
			}
			else
			{
				return QScriptValue::NullValue;
			}
		}

		#include <cstddef>
		#include <tuple>
		#include <type_traits>
		#include <utility>

		template<size_t N>
		struct Apply {
			template<typename F, typename T, typename... A>
			static inline auto apply(F && f, T && t, A &&... a)
				-> decltype(Apply<N-1>::apply(
					::std::forward<F>(f), ::std::forward<T>(t),
					::std::get<N-1>(::std::forward<T>(t)), ::std::forward<A>(a)...
				))
			{
				return Apply<N-1>::apply(::std::forward<F>(f), ::std::forward<T>(t),
					::std::get<N-1>(::std::forward<T>(t)), ::std::forward<A>(a)...
				);
			}
		};

		template<>
		struct Apply<0> {
			template<typename F, typename T, typename... A>
			static inline auto apply(F && f, T &&, A &&... a)
				-> decltype(::std::forward<F>(f)(::std::forward<A>(a)...))
			{
				return ::std::forward<F>(f)(::std::forward<A>(a)...);
			}
		};

		template<typename F, typename T>
		inline auto apply(F && f, T && t)
			-> decltype(Apply< ::std::tuple_size<
				typename ::std::decay<T>::type
			>::value>::apply(::std::forward<F>(f), ::std::forward<T>(t)))
		{
			return Apply< ::std::tuple_size<
				typename ::std::decay<T>::type
			>::value>::apply(::std::forward<F>(f), ::std::forward<T>(t));
		}

		MSVC_PRAGMA(warning( push )) // see matching "pop" below
		MSVC_PRAGMA(warning( disable : 4189 )) // disable "warning C4189: 'idx': local variable is initialized but not referenced"
		
		template<typename R, typename...Args>
		QScriptValue wrap__(R(*f)(const wzapi::execution_context&, Args...), WZ_DECL_UNUSED const char *wrappedFunctionName, QScriptContext *context, QScriptEngine *engine)
		{
			size_t idx WZ_DECL_UNUSED = 0; // unused when Args... is empty
			qtscript_execution_context execution_context(context, engine);
			return box(apply(f, std::tuple<const wzapi::execution_context&, Args...>{static_cast<const wzapi::execution_context&>(execution_context), unbox<Args>{}(idx, context, engine, wrappedFunctionName)...}), engine);
		}

		MSVC_PRAGMA(warning( pop ))

		#define wrap_(wzapi_function, context, engine) \
		wrap__(wzapi_function, #wzapi_function, context, engine)
	}

// ----------------------------------------------------------------------------------------
// Script functions
//
// All script functions should be prefixed with "js_" then followed by same name as in script.

//-- ## getWeaponInfo(weapon id)
//--
//-- Return information about a particular weapon type. DEPRECATED - query the Stats object instead. (3.2+ only)
//--
static QScriptValue js_getWeaponInfo(QScriptContext *context, QScriptEngine *engine)
{
	QString id = context->argument(0).toString();
	int idx = getCompFromName(COMP_WEAPON, QStringToWzString(id));
	SCRIPT_ASSERT(context, idx >= 0, "No such weapon: %s", id.toUtf8().constData());
	WEAPON_STATS *psStats = asWeaponStats + idx;
	QScriptValue info = engine->newObject();
	info.setProperty("id", id);
	info.setProperty("name", WzStringToQScriptValue(engine, psStats->name));
	info.setProperty("impactClass", psStats->weaponClass == WC_KINETIC ? "KINETIC" : "HEAT");
	info.setProperty("damage", psStats->base.damage);
	info.setProperty("firePause", psStats->base.firePause);
	info.setProperty("fireOnMove", psStats->fireOnMove);
	return QScriptValue(info);
}

//-- ## resetLabel(label[, filter])
//--
//-- Reset the trigger on an label. Next time a unit enters the area, it will trigger
//-- an area event. Next time an object or a group is seen, it will trigger a seen event.
//-- Optionally add a filter on it in the second parameter, which can
//-- be a specific player to watch for, or ALL_PLAYERS by default.
//-- This is a fast operation of O(log n) algorithmic complexity. (3.2+ only)
//-- ## resetArea(label[, filter])
//-- Reset the trigger on an area. Next time a unit enters the area, it will trigger
//-- an area event. Optionally add a filter on it in the second parameter, which can
//-- be a specific player to watch for, or ALL_PLAYERS by default.
//-- This is a fast operation of O(log n) algorithmic complexity. DEPRECATED - use resetLabel instead. (3.2+ only)
//--
static QScriptValue js_resetLabel(QScriptContext *context, QScriptEngine *)
{
	std::string labelName = context->argument(0).toString().toStdString();
	SCRIPT_ASSERT(context, labels.count(labelName) > 0, "Label %s not found", labelName.c_str());
	LABEL &l = labels[labelName];
	l.triggered = 0; // make active again
	if (context->argumentCount() > 1)
	{
		l.subscriber = context->argument(1).toInt32();
	}
	return QScriptValue();
}

//-- ## enumLabels([filter])
//--
//-- Returns a string list of labels that exist for this map. The optional filter
//-- parameter can be used to only return labels of one specific type. (3.2+ only)
//--
static QScriptValue js_enumLabels(QScriptContext *context, QScriptEngine *engine)
{
	if (context->argumentCount() > 0) // filter
	{
		QStringList matches;
		SCRIPT_TYPE type = (SCRIPT_TYPE)context->argument(0).toInt32();
		for (LABELMAP::const_iterator i = labels.begin(); i != labels.end(); i++)
		{
			const LABEL &l = i->second;
			if (l.type == type)
			{
				matches += QString::fromStdString(i->first);
			}
		}
		QScriptValue result = engine->newArray(matches.size());
		for (int i = 0; i < matches.size(); i++)
		{
			result.setProperty(i, QScriptValue(matches[i]), QScriptValue::ReadOnly);
		}
		return result;
	}
	else // fast path, give all
	{
		QScriptValue result = engine->newArray(labels.size());
		int num = 0;
		for (LABELMAP::const_iterator i = labels.begin(); i != labels.end(); i++)
		{
			result.setProperty(num++, QScriptValue(QString::fromStdString(i->first)), QScriptValue::ReadOnly);
		}
		return result;
	}
}

//-- ## addLabel(object, label)
//--
//-- Add a label to a game object. If there already is a label by that name, it is overwritten.
//-- This is a fast operation of O(log n) algorithmic complexity. (3.2+ only)
//--
static QScriptValue js_addLabel(QScriptContext *context, QScriptEngine *engine)
{
	LABEL value;
	QScriptValue qval = context->argument(0);
	value.type = qval.property("type").toInt32();
	value.id = qval.property("id").toInt32();
	value.player = qval.property("player").toInt32();
	value.p1.x = world_coord(qval.property("x").toInt32());
	value.p1.y = world_coord(qval.property("y").toInt32());
	if (value.type == SCRIPT_AREA)
	{
		value.p2.x = world_coord(qval.property("x2").toInt32());
		value.p2.y = world_coord(qval.property("y2").toInt32());
	}
	else if (value.type == SCRIPT_RADIUS)
	{
		value.p2.x = world_coord(qval.property("radius").toInt32());
	}
	value.triggered = -1; // default off
	QScriptValue triggered = qval.property("triggered");
	if (triggered.isNumber())
	{
		SCRIPT_ASSERT(context, value.type != SCRIPT_POSITION, "Cannot assign a trigger to a position");
		value.triggered = triggered.toInt32();
	}
	if (value.type == OBJ_DROID || value.type == OBJ_STRUCTURE || value.type == OBJ_FEATURE)
	{
		BASE_OBJECT *psObj = IdToObject((OBJECT_TYPE)value.type, value.id, value.player);
		SCRIPT_ASSERT(context, psObj, "Object id %d not found belonging to player %d", value.id, value.player);
	}
	std::string key = context->argument(1).toString().toStdString();
	labels[key] = value;
	updateLabelModel();
	return QScriptValue();
}

//-- ## removeLabel(label)
//--
//-- Remove a label from the game. Returns the number of labels removed, which should normally be
//-- either 1 (label found) or 0 (label not found). (3.2+ only)
//--
static QScriptValue js_removeLabel(QScriptContext *context, QScriptEngine *engine)
{
	std::string key = context->argument(0).toString().toStdString();
	int result = labels.erase(key);
	updateLabelModel();
	return QScriptValue(result);
}

//-- ## getLabel(object)
//--
//-- Get a label string belonging to a game object. If the object has multiple labels, only the first
//-- label found will be returned. If the object has no labels, null is returned.
//-- This is a relatively slow operation of O(n) algorithmic complexity. (3.2+ only)
//--
static QScriptValue js_getLabel(QScriptContext *context, QScriptEngine *engine)
{
	LABEL value;
	QScriptValue objparam = context->argument(0);
	value.id = objparam.property("id").toInt32();
	value.player = objparam.property("player").toInt32();
	value.type = (OBJECT_TYPE)objparam.property("type").toInt32();
	std::string label;
	for (const auto &it : labels)
	{
		if (it.second == value)
		{
			label = it.first;
			break;
		}
	}
	if (!label.empty())
	{
		return QScriptValue(QString::fromStdString(label));
	}
	return QScriptValue::NullValue;
}

//-- ## getObject(label | x, y | type, player, id)
//--
//-- Fetch something denoted by a label, a map position or its object ID. A label refers to an area,
//-- a position or a **game object** on the map defined using the map editor and stored
//-- together with the map. In this case, the only argument is a text label. The function
//-- returns an object that has a type variable defining what it is (in case this is
//-- unclear). This type will be one of DROID, STRUCTURE, FEATURE, AREA, GROUP or POSITION.
//-- The AREA has defined 'x', 'y', 'x2', and 'y2', while POSITION has only defined 'x' and 'y'.
//-- The GROUP type has defined 'type' and 'id' of the group, which can be passed to enumGroup().
//-- This is a fast operation of O(log n) algorithmic complexity. If the label is not found, an
//-- undefined value is returned. If whatever object the label should point at no longer exists,
//-- a null value is returned.
//--
//-- You can also fetch a STRUCTURE or FEATURE type game object from a given map position (if any).
//-- This is a very fast operation of O(1) algorithmic complexity. Droids cannot be fetched in this
//-- manner, since they do not have a unique placement on map tiles. Finally, you can fetch an object using
//-- its ID, in which case you need to pass its type, owner and unique object ID. This is an
//-- operation of O(n) algorithmic complexity. (3.2+ only)
//--
static QScriptValue js_getObject(QScriptContext *context, QScriptEngine *engine)
{
	if (context->argumentCount() == 2) // get at position case
	{
		int x = context->argument(0).toInt32();
		int y = context->argument(1).toInt32();
		SCRIPT_ASSERT(context, tileOnMap(x, y), "Map position (%d, %d) not on the map!", x, y);
		const MAPTILE *psTile = mapTile(x, y);
		return QScriptValue(convMax(psTile->psObject, engine));
	}
	else if (context->argumentCount() == 3) // get by ID case
	{
		OBJECT_TYPE type = (OBJECT_TYPE)context->argument(0).toInt32();
		int player = context->argument(1).toInt32();
		int id = context->argument(2).toInt32();
		SCRIPT_ASSERT_PLAYER(context, player);
		return QScriptValue(convMax(IdToObject(type, id, player), engine));
	}
	// get by label case
	BASE_OBJECT *psObj;
	std::string label = context->argument(0).toString().toStdString();
	QScriptValue ret;
	if (labels.count(label) > 0)
	{
		ret = engine->newObject();
		const LABEL &p = labels[label];
		ret.setProperty("type", p.type, QScriptValue::ReadOnly);
		switch (p.type)
		{
		case SCRIPT_RADIUS:
			ret.setProperty("x", map_coord(p.p1.x), QScriptValue::ReadOnly);
			ret.setProperty("y", map_coord(p.p1.y), QScriptValue::ReadOnly);
			ret.setProperty("radius", map_coord(p.p2.x), QScriptValue::ReadOnly);
			ret.setProperty("triggered", p.triggered);
			ret.setProperty("subscriber", p.subscriber);
			break;
		case SCRIPT_AREA:
			ret.setProperty("x2", map_coord(p.p2.x), QScriptValue::ReadOnly);
			ret.setProperty("y2", map_coord(p.p2.y), QScriptValue::ReadOnly);
			ret.setProperty("subscriber", p.subscriber);
		// fall through
		case SCRIPT_POSITION:
			ret.setProperty("triggered", p.triggered);
			ret.setProperty("x", map_coord(p.p1.x), QScriptValue::ReadOnly);
			ret.setProperty("y", map_coord(p.p1.y), QScriptValue::ReadOnly);
			break;
		case SCRIPT_GROUP:
			ret.setProperty("triggered", p.triggered);
			ret.setProperty("subscriber", p.subscriber);
			ret.setProperty("id", p.id, QScriptValue::ReadOnly);
			break;
		case OBJ_DROID:
		case OBJ_FEATURE:
		case OBJ_STRUCTURE:
			psObj = IdToObject((OBJECT_TYPE)p.type, p.id, p.player);
			return convMax(psObj, engine);
		default:
			SCRIPT_ASSERT(context, false, "Bad object label type found for label %s!", label.c_str());
			break;
		}
	}
	return ret;
}

//-- ## enumBlips(player)
//--
//-- Return an array containing all the non-transient radar blips that the given player
//-- can see. This includes sensors revealed by radar detectors, as well as ECM jammers.
//-- It does not include units going out of view.
//--
static QScriptValue js_enumBlips(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enumBlips, context, engine);
}

//-- ## enumSelected()
//--
//-- Return an array containing all game objects currently selected by the host player. (3.2+ only)
//--
QScriptValue js_enumSelected(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enumSelected, context, engine);
}

//-- ## enumGateways()
//--
//-- Return an array containing all the gateways on the current map. The array contains object with the properties
//-- x1, y1, x2 and y2. (3.2+ only)
//--
static QScriptValue js_enumGateways(QScriptContext *, QScriptEngine *engine)
{
	QScriptValue result = engine->newArray(gwNumGateways());
	int i = 0;
	for (auto psGateway : gwGetGateways())
	{
		QScriptValue v = engine->newObject();
		v.setProperty("x1", psGateway->x1, QScriptValue::ReadOnly);
		v.setProperty("y1", psGateway->y1, QScriptValue::ReadOnly);
		v.setProperty("x2", psGateway->x2, QScriptValue::ReadOnly);
		v.setProperty("y2", psGateway->y2, QScriptValue::ReadOnly);
		result.setProperty(i++, v);
	}
	return result;
}

//-- ## enumTemplates(player)
//--
//-- Return an array containing all the buildable templates for the given player. (3.2+ only)
//--
static QScriptValue js_enumTemplates(QScriptContext *context, QScriptEngine *engine)
{
	int player = context->argument(0).toInt32();
	QScriptValue result = engine->newArray(droidTemplates[player].size());
	int count = 0;
	for (auto &keyvaluepair : droidTemplates[player])
	{
		result.setProperty(count, convTemplate(keyvaluepair.second, engine));
		count++;
	}
	return result;
}

//-- ## enumGroup(group)
//--
//-- Return an array containing all the members of a given group.
//--
static QScriptValue js_enumGroup(QScriptContext *context, QScriptEngine *engine)
{
	int groupId = context->argument(0).toInt32();
	QList<BASE_OBJECT *> matches;
	GROUPMAP *psMap = getGroupMap(engine);

	if (psMap != nullptr)
	{
		for (GROUPMAP::const_iterator i = psMap->begin(); i != psMap->end(); ++i)
		{
			if (i->second == groupId)
			{
				matches.push_back(i->first);
			}
		}
	}
	QScriptValue result = engine->newArray(matches.size());
	for (int i = 0; i < matches.size(); i++)
	{
		BASE_OBJECT *psObj = matches.at(i);
		result.setProperty(i, convMax(psObj, engine));
	}
	return result;
}

//-- ## newGroup()
//--
//-- Allocate a new group. Returns its numerical ID. Deprecated since 3.2 - you should now
//-- use your own number scheme for groups.
//--
static QScriptValue js_newGroup(QScriptContext *, QScriptEngine *engine)
{
	static int i = 1; // group zero reserved
	return QScriptValue(i++);
}

//-- ## activateStructure(structure, [target[, ability]])
//--
//-- Activate a special ability on a structure. Currently only works on the lassat.
//-- The lassat needs a target.
//--
static QScriptValue js_activateStructure(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::activateStructure, context, engine);
}

//-- ## findResearch(research, [player])
//--
//-- Return list of research items remaining to be researched for the given research item. (3.2+ only)
//-- (Optional second argument 3.2.3+ only)
//--
static QScriptValue js_findResearch(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::findResearch, context, engine);
}

//-- ## pursueResearch(lab, research)
//--
//-- Start researching the first available technology on the way to the given technology.
//-- First parameter is the structure to research in, which must be a research lab. The
//-- second parameter is the technology to pursue, as a text string as defined in "research.json".
//-- The second parameter may also be an array of such strings. The first technology that has
//-- not yet been researched in that list will be pursued.
//--
static QScriptValue js_pursueResearch(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::pursueResearch, context, engine);
}

//-- ## getResearch(research[, player])
//--
//-- Fetch information about a given technology item, given by a string that matches
//-- its definition in "research.json". If not found, returns null.
//--
static QScriptValue js_getResearch(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::getResearch, context, engine);
}

//-- ## enumResearch()
//--
//-- Returns an array of all research objects that are currently and immediately available for research.
//--
static QScriptValue js_enumResearch(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enumResearch, context, engine);
}

//-- ## componentAvailable([component type,] component name)
//--
//-- Checks whether a given component is available to the current player. The first argument is
//-- optional and deprecated.
//--
static QScriptValue js_componentAvailable(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::componentAvailable, context, engine);
}

//-- ## addFeature(name, x, y)
//--
//-- Create and place a feature at the given x, y position. Will cause a desync in multiplayer.
//-- Returns the created game object on success, null otherwise. (3.2+ only)
//--
static QScriptValue js_addFeature(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::addFeature, context, engine);
}

//-- ## addDroid(player, x, y, name, body, propulsion, reserved, reserved, turrets...)
//--
//-- Create and place a droid at the given x, y position as belonging to the given player, built with
//-- the given components. Currently does not support placing droids in multiplayer, doing so will
//-- cause a desync. Returns the created droid on success, otherwise returns null. Passing "" for
//-- reserved parameters is recommended. In 3.2+ only, to create droids in off-world (campaign mission list),
//-- pass -1 as both x and y.
//--
static QScriptValue js_addDroid(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::addDroid, context, engine);
}

//-- ## addDroidToTransporter(transporter, droid)
//--
//-- Load a droid, which is currently located on the campaign off-world mission list,
//-- into a transporter, which is also currently on the campaign off-world mission list.
//-- (3.2+ only)
//--
static QScriptValue js_addDroidToTransporter(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::addDroidToTransporter, context, engine);
}

//-- ## makeTemplate(player, name, body, propulsion, reserved, turrets...)
//--
//-- Create a template (virtual droid) with the given components. Can be useful for calculating the cost
//-- of droids before putting them into production, for instance. Will fail and return null if template
//-- could not possibly be built using current research. (3.2+ only)
//--
static QScriptValue js_makeTemplate(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::makeTemplate, context, engine);
}

//-- ## buildDroid(factory, name, body, propulsion, reserved, reserved, turrets...)
//--
//-- Start factory production of new droid with the given name, body, propulsion and turrets.
//-- The reserved parameter should be passed **null** for now. The components can be
//-- passed as ordinary strings, or as a list of strings. If passed as a list, the first available
//-- component in the list will be used. The second reserved parameter used to be a droid type.
//-- It is now unused and in 3.2+ should be passed "", while in 3.1 it should be the
//-- droid type to be built. Returns a boolean that is true if production was started.
//--
static QScriptValue js_buildDroid(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::buildDroid, context, engine);
}

//-- ## enumStruct([player[, structure type[, looking player]]])
//--
//-- Returns an array of structure objects. If no parameters given, it will
//-- return all of the structures for the current player. The second parameter
//-- can be either a string with the name of the structure type as defined in
//-- "structures.json", or a stattype as defined in ```Structure```. The
//-- third parameter can be used to filter by visibility, the default is not
//-- to filter.
//--
static QScriptValue js_enumStruct(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enumStruct, context, engine);
}

//-- ## enumStructOffWorld([player[, structure type[, looking player]]])
//--
//-- Returns an array of structure objects in your base when on an off-world mission, NULL otherwise.
//-- If no parameters given, it will return all of the structures for the current player.
//-- The second parameter can be either a string with the name of the structure type as defined
//-- in "structures.json", or a stattype as defined in ```Structure```.
//-- The third parameter can be used to filter by visibility, the default is not
//-- to filter.
//--
static QScriptValue js_enumStructOffWorld(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enumStructOffWorld, context, engine);
}

//-- ## enumFeature(player[, name])
//--
//-- Returns an array of all features seen by player of given name, as defined in "features.json".
//-- If player is ```ALL_PLAYERS```, it will return all features irrespective of visibility to any player. If
//-- name is empty, it will return any feature.
//--
static QScriptValue js_enumFeature(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enumFeature, context, engine);
}

//-- ## enumCargo(transport droid)
//--
//-- Returns an array of droid objects inside given transport. (3.2+ only)
//--
static QScriptValue js_enumCargo(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enumCargo, context, engine);
}

//-- ## enumDroid([player[, droid type[, looking player]]])
//--
//-- Returns an array of droid objects. If no parameters given, it will
//-- return all of the droids for the current player. The second, optional parameter
//-- is the name of the droid type. The third parameter can be used to filter by
//-- visibility - the default is not to filter.
//--
static QScriptValue js_enumDroid(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enumDroid, context, engine);
}

void dumpScriptLog(const QString &scriptName, int me, const QString &info)
{
	QString path = PHYSFS_getWriteDir();
	path += "/logs/" + scriptName + "." + QString::number(me) + ".log";
	FILE *fp = fopen(path.toUtf8().constData(), "a");
	if (fp)
	{
		fputs(info.toUtf8().constData(), fp);
		fclose(fp);
	}
}

//-- ## dump(string...)
//--
//-- Output text to a debug file. (3.2+ only)
//--
static QScriptValue js_dump(QScriptContext *context, QScriptEngine *engine)
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
	result += "\n";

	QString scriptName = engine->globalObject().property("scriptName").toString();
	int me = engine->globalObject().property("me").toInt32();
	dumpScriptLog(scriptName, me, result);
	return QScriptValue();
}

//-- ## debug(string...)
//--
//-- Output text to the command line.
//--
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
	qWarning("%s", result.toUtf8().constData());
	return QScriptValue();
}

//-- ## pickStructLocation(droid, structure type, x, y)
//--
//-- Pick a location for constructing a certain type of building near some given position.
//-- Returns an object containing "type" POSITION, and "x" and "y" values, if successful.
//--
static QScriptValue js_pickStructLocation(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::pickStructLocation, context, engine);
}

//-- ## structureIdle(structure)
//--
//-- Is given structure idle?
//--
static QScriptValue js_structureIdle(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::structureIdle, context, engine);
}

//-- ## removeStruct(structure)
//--
//-- Immediately remove the given structure from the map. Returns a boolean that is true on success.
//-- No special effects are applied. Deprecated since 3.2.
//--
static QScriptValue js_removeStruct(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::removeStruct, context, engine);
}

//-- ## removeObject(game object[, special effects?])
//--
//-- Remove the given game object with special effects. Returns a boolean that is true on success.
//-- A second, optional boolean parameter specifies whether special effects are to be applied. (3.2+ only)
//--
static QScriptValue js_removeObject(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::removeObject, context, engine);
}

//-- ## clearConsole()
//--
//-- Clear the console. (3.3+ only)
//--
static QScriptValue js_clearConsole(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::clearConsole, context, engine);
}

//-- ## console(strings...)
//--
//-- Print text to the player console.
//--
// TODO, should cover scrShowConsoleText, scrAddConsoleText, scrTagConsoleText and scrConsole
static QScriptValue js_console(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::console, context, engine);
}

//-- ## groupAddArea(group, x1, y1, x2, y2)
//--
//-- Add any droids inside the given area to the given group. (3.2+ only)
//--
static QScriptValue js_groupAddArea(QScriptContext *context, QScriptEngine *engine)
{
	int groupId = context->argument(0).toInt32();
	int player = engine->globalObject().property("me").toInt32();
	int x1 = world_coord(context->argument(1).toInt32());
	int y1 = world_coord(context->argument(2).toInt32());
	int x2 = world_coord(context->argument(3).toInt32());
	int y2 = world_coord(context->argument(4).toInt32());
	QScriptValue groups = engine->globalObject().property("groupSizes");

	for (DROID *psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->pos.x >= x1 && psDroid->pos.x <= x2 && psDroid->pos.y >= y1 && psDroid->pos.y <= y2)
		{
			groupAddObject(psDroid, groupId, engine);
		}
	}
	return QScriptValue();
}

//-- ## groupAddDroid(group, droid)
//--
//-- Add given droid to given group. Deprecated since 3.2 - use groupAdd() instead.
//--
static QScriptValue js_groupAddDroid(QScriptContext *context, QScriptEngine *engine)
{
	int groupId = context->argument(0).toInt32();
	QScriptValue droidVal = context->argument(1);
	int droidId = droidVal.property("id").toInt32();
	int droidPlayer = droidVal.property("player").toInt32();
	DROID *psDroid = IdToDroid(droidId, droidPlayer);
	QScriptValue groups = engine->globalObject().property("groupSizes");
	SCRIPT_ASSERT(context, psDroid, "Invalid droid index %d", droidId);
	groupAddObject(psDroid, groupId, engine);
	return QScriptValue();
}

//-- ## groupAdd(group, object)
//--
//-- Add given game object to the given group.
//--
static QScriptValue js_groupAdd(QScriptContext *context, QScriptEngine *engine)
{
	int groupId = context->argument(0).toInt32();
	QScriptValue val = context->argument(1);
	int id = val.property("id").toInt32();
	int player = val.property("player").toInt32();
	OBJECT_TYPE type = (OBJECT_TYPE)val.property("type").toInt32();
	BASE_OBJECT *psObj = IdToObject(type, id, player);
	SCRIPT_ASSERT(context, psObj, "Invalid object index %d", id);
	QScriptValue groups = engine->globalObject().property("groupSizes");
	groupAddObject(psObj, groupId, engine);
	return QScriptValue();
}

//-- ## distBetweenTwoPoints(x1, y1, x2, y2)
//--
//-- Return distance between two points.
//--
static QScriptValue js_distBetweenTwoPoints(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::distBetweenTwoPoints, context, engine);
}

//-- ## groupSize(group)
//--
//-- Return the number of droids currently in the given group. Note that you can use groupSizes[] instead.
//--
static QScriptValue js_groupSize(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue groups = engine->globalObject().property("groupSizes");
	int groupId = context->argument(0).toInt32();
	return groups.property(groupId).toInt32();
}

//-- ## droidCanReach(droid, x, y)
//--
//-- Return whether or not the given droid could possibly drive to the given position. Does
//-- not take player built blockades into account.
//--
static QScriptValue js_droidCanReach(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::droidCanReach, context, engine);
}

//-- ## propulsionCanReach(propulsion, x1, y1, x2, y2)
//--
//-- Return true if a droid with a given propulsion is able to travel from (x1, y1) to (x2, y2).
//-- Does not take player built blockades into account. (3.2+ only)
//--
static QScriptValue js_propulsionCanReach(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::propulsionCanReach, context, engine);
}

//-- ## terrainType(x, y)
//--
//-- Returns tile type of a given map tile, such as TER_WATER for water tiles or TER_CLIFFFACE for cliffs.
//-- Tile types regulate which units may pass through this tile. (3.2+ only)
//--
static QScriptValue js_terrainType(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::terrainType, context, engine);
}

//-- ## orderDroid(droid, order)
//--
//-- Give a droid an order to do something. (3.2+ only)
//--
static QScriptValue js_orderDroid(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::orderDroid, context, engine);
}

//-- ## orderDroidObj(droid, order, object)
//--
//-- Give a droid an order to do something to something.
//--
static QScriptValue js_orderDroidObj(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::orderDroidObj, context, engine);
}

//-- ## orderDroidBuild(droid, order, structure type, x, y[, direction])
//--
//-- Give a droid an order to build something at the given position. Returns true if allowed.
//--
static QScriptValue js_orderDroidBuild(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::orderDroidBuild, context, engine);
}

//-- ## orderDroidLoc(droid, order, x, y)
//--
//-- Give a droid an order to do something at the given location.
//--
static QScriptValue js_orderDroidLoc(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::orderDroidLoc, context, engine);
}

//-- ## setMissionTime(time)
//--
//-- Set mission countdown in seconds.
//--
static QScriptValue js_setMissionTime(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setMissionTime, context, engine);
}

//-- ## getMissionTime()
//--
//-- Get time remaining on mission countdown in seconds. (3.2+ only)
//--
static QScriptValue js_getMissionTime(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::getMissionTime, context, engine);
}

//-- ## setTransporterExit(x, y, player)
//--
//-- Set the exit position for the mission transporter. (3.2+ only)
//--
static QScriptValue js_setTransporterExit(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setTransporterExit, context, engine);
}

//-- ## startTransporterEntry(x, y, player)
//--
//-- Set the entry position for the mission transporter, and make it start flying in
//-- reinforcements. If you want the camera to follow it in, use cameraTrack() on it.
//-- The transport needs to be set up with the mission droids, and the first transport
//-- found will be used. (3.2+ only)
//--
static QScriptValue js_startTransporterEntry(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::startTransporterEntry, context, engine);
}

//-- ## useSafetyTransport(flag)
//--
//-- Change if the mission transporter will fetch droids in non offworld missions
//-- setReinforcementTime() is be used to hide it before coming back after the set time
//-- which is handled by the campaign library in the victory data section (3.3+ only).
//--
static QScriptValue js_useSafetyTransport(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::useSafetyTransport, context, engine);
}

//-- ## restoreLimboMissionData()
//--
//-- Swap mission type and bring back units previously stored at the start
//-- of the mission (see cam3-c mission). (3.3+ only).
//--
static QScriptValue js_restoreLimboMissionData(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::restoreLimboMissionData, context, engine);
}

//-- ## setReinforcementTime(time)
//--
//-- Set time for reinforcements to arrive. If time is negative, the reinforcement GUI
//-- is removed and the timer stopped. Time is in seconds.
//-- If time equals to the magic LZ_COMPROMISED_TIME constant, reinforcement GUI ticker
//-- is set to "--:--" and reinforcements are suppressed until this function is called
//-- again with a regular time value.
//--
static QScriptValue js_setReinforcementTime(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setReinforcementTime, context, engine);
}

//-- ## setStructureLimits(structure type, limit[, player])
//--
//-- Set build limits for a structure.
//--
static QScriptValue js_setStructureLimits(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setStructureLimits, context, engine);
}

//-- ## centreView(x, y)
//--
//-- Center the player's camera at the given position.
//--
static QScriptValue js_centreView(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::centreView, context, engine);
}

//-- ## hackPlayIngameAudio()
//--
//-- (3.3+ only)
//--
static QScriptValue js_hackPlayIngameAudio(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::hackPlayIngameAudio, context, engine);
}

//-- ## hackStopIngameAudio()
//--
//-- (3.3+ only)
//--
static QScriptValue js_hackStopIngameAudio(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::hackStopIngameAudio, context, engine);
}

//-- ## playSound(sound[, x, y, z])
//--
//-- Play a sound, optionally at a location.
//--
static QScriptValue js_playSound(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::playSound, context, engine);
}

//-- ## gameOverMessage(won, showBackDrop, showOutro)
//--
//-- End game in victory or defeat.
//--
static QScriptValue js_gameOverMessage(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue retVal = wrap_(wzapi::gameOverMessage, context, engine);
	jsDebugMessageUpdate();
	return retVal;
}

//-- ## completeResearch(research[, player [, forceResearch]])
//--
//-- Finish a research for the given player.
//-- forceResearch will allow a research topic to be researched again. 3.3+
//--
static QScriptValue js_completeResearch(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::completeResearch, context, engine);
}

//-- ## completeAllResearch([player])
//--
//-- Finish all researches for the given player.
//--
static QScriptValue js_completeAllResearch(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::completeAllResearch, context, engine);
}

//-- ## enableResearch(research[, player])
//--
//-- Enable a research for the given player, allowing it to be researched.
//--
static QScriptValue js_enableResearch(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enableResearch, context, engine);
}

//-- ## extraPowerTime(time, player)
//--
//-- Increase a player's power as if that player had power income equal to current income
//-- over the given amount of extra time. (3.2+ only)
//--
static QScriptValue js_extraPowerTime(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::extraPowerTime, context, engine);
}

//-- ## setPower(power[, player])
//--
//-- Set a player's power directly. (Do not use this in an AI script.)
//--
static QScriptValue js_setPower(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setPower, context, engine);
}

//-- ## setPowerModifier(power[, player])
//--
//-- Set a player's power modifier percentage. (Do not use this in an AI script.) (3.2+ only)
//--
static QScriptValue js_setPowerModifier(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setPowerModifier, context, engine);
}

//-- ## setPowerStorageMaximum(maximum[, player])
//--
//-- Set a player's power storage maximum. (Do not use this in an AI script.) (3.2+ only)
//--
static QScriptValue js_setPowerStorageMaximum(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setPowerStorageMaximum, context, engine);
}

//-- ## enableStructure(structure type[, player])
//--
//-- The given structure type is made available to the given player. It will appear in the
//-- player's build list.
//--
static QScriptValue js_enableStructure(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enableStructure, context, engine);
}

//-- ## setTutorialMode(bool)
//--
//-- Sets a number of restrictions appropriate for tutorial if set to true.
//--
static QScriptValue js_setTutorialMode(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setTutorialMode, context, engine);
}

//-- ## setMiniMap(bool)
//--
//-- Turns visible minimap on or off in the GUI.
//--
static QScriptValue js_setMiniMap(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setMiniMap, context, engine);
}

//-- ## setDesign(bool)
//--
//-- Whether to allow player to design stuff.
//--
static QScriptValue js_setDesign(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setDesign, context, engine);
}

//-- ## enableTemplate(template name)
//--
//-- Enable a specific template (even if design is disabled).
//--
static QScriptValue js_enableTemplate(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enableTemplate, context, engine);
}

//-- ## removeTemplate(template name)
//--
//-- Remove a template.
//--
static QScriptValue js_removeTemplate(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::removeTemplate, context, engine);
}

//-- ## setReticuleButton(id, tooltip, filename, filenameHigh, callback)
//--
//-- Add reticule button. id is which button to change, where zero is zero is the middle button, then going clockwise from the
//-- uppermost button. filename is button graphics and filenameHigh is for highlighting. The tooltip is the text you see when
//-- you mouse over the button. Finally, the callback is which scripting function to call. Hide and show the user interface
//-- for such changes to take effect. (3.2+ only)
//--
static QScriptValue js_setReticuleButton(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setReticuleButton, context, engine);
}

//-- ## showReticuleWidget(id)
//--
//-- Open the reticule menu widget. (3.3+ only)
//--
static QScriptValue js_showReticuleWidget(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::showReticuleWidget, context, engine);
}

//-- ## setReticuleFlash(id, flash)
//--
//-- Set reticule flash on or off. (3.2.3+ only)
//--
static QScriptValue js_setReticuleFlash(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setReticuleFlash, context, engine);
}

//-- ## showInterface()
//--
//-- Show user interface. (3.2+ only)
//--
static QScriptValue js_showInterface(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::showInterface, context, engine);
}

//-- ## hideInterface()
//--
//-- Hide user interface. (3.2+ only)
//--
static QScriptValue js_hideInterface(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::hideInterface, context, engine);
}

//-- ## removeReticuleButton(button type)
//--
//-- Remove reticule button. DO NOT USE FOR ANYTHING.
//--
static QScriptValue js_removeReticuleButton(QScriptContext *context, QScriptEngine *engine)
{
	return QScriptValue();
}

//-- ## applyLimitSet()
//--
//-- Mix user set limits with script set limits and defaults.
//--
static QScriptValue js_applyLimitSet(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::applyLimitSet, context, engine);
}

//-- ## enableComponent(component, player)
//--
//-- The given component is made available for research for the given player.
//--
static QScriptValue js_enableComponent(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enableComponent, context, engine);
}

//-- ## makeComponentAvailable(component, player)
//--
//-- The given component is made available to the given player. This means the player can
//-- actually build designs with it.
//--
static QScriptValue js_makeComponentAvailable(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::makeComponentAvailable, context, engine);
}

//-- ## allianceExistsBetween(player, player)
//--
//-- Returns true if an alliance exists between the two players, or they are the same player.
//--
static QScriptValue js_allianceExistsBetween(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::allianceExistsBetween, context, engine);
}

//-- ## _(string)
//--
//-- Mark string for translation.
//--
static QScriptValue js_translate(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::translate, context, engine);
}

//-- ## playerPower(player)
//--
//-- Return amount of power held by the given player.
//--
static QScriptValue js_playerPower(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::playerPower, context, engine);
}

//-- ## queuedPower(player)
//--
//-- Return amount of power queued up for production by the given player. (3.2+ only)
//--
static QScriptValue js_queuedPower(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::queuedPower, context, engine);
}

//-- ## isStructureAvailable(structure type[, player])
//--
//-- Returns true if given structure can be built. It checks both research and unit limits.
//--
static QScriptValue js_isStructureAvailable(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::isStructureAvailable, context, engine);
}

//-- ## isVTOL(droid)
//--
//-- Returns true if given droid is a VTOL (not including transports).
//--
static QScriptValue js_isVTOL(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::isVTOL, context, engine);
}

//-- ## hackGetObj(type, player, id)
//--
//-- Function to find and return a game object of DROID, FEATURE or STRUCTURE types, if it exists.
//-- Otherwise, it will return null. This function is deprecated by getObject(). (3.2+ only)
//--
static QScriptValue js_hackGetObj(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::hackGetObj, context, engine);
}

//-- ## hackChangeMe(player)
//--
//-- Change the 'me' who owns this script to the given player. This needs to be run
//-- first in ```eventGameInit``` to make sure things do not get out of control.
//--
// This is only intended for use in campaign scripts until we get a way to add
// scripts for each player. (3.2+ only)
static QScriptValue js_hackChangeMe(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::hackChangeMe, context, engine);
}

//-- ## receiveAllEvents(bool)
//--
//-- Make the current script receive all events, even those not meant for 'me'. (3.2+ only)
//--
static QScriptValue js_receiveAllEvents(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::receiveAllEvents, context, engine);
}

//-- ## hackAssert(condition, message...)
//--
//-- Function to perform unit testing. It will throw a script error and a game assert. (3.2+ only)
//--
static QScriptValue js_hackAssert(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::hackAssert, context, engine);
}

//-- ## objFromId(fake game object)
//--
//-- Broken function meant to make porting from the old scripting system easier. Do not use for new code.
//-- Instead, use labels.
//--
static QScriptValue js_objFromId(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue droidVal = context->argument(0);
	int id = droidVal.property("id").toInt32();
	BASE_OBJECT *psObj = getBaseObjFromId(id);
	SCRIPT_ASSERT(context, psObj, "No such object id %d", id);
	return QScriptValue(convMax(psObj, engine));
}

//-- ## setDroidExperience(droid, experience)
//--
//-- Set the amount of experience a droid has. Experience is read using floating point precision.
//--
static QScriptValue js_setDroidExperience(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setDroidExperience, context, engine);
}

//-- ## donateObject(object, to)
//--
//-- Donate a game object (restricted to droids before 3.2.3) to another player. Returns true if
//-- donation was successful. May return false if this donation would push the receiving player
//-- over unit limits. (3.2+ only)
//--
static QScriptValue js_donateObject(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::donateObject, context, engine);
}

//-- ## donatePower(amount, to)
//--
//-- Donate power to another player. Returns true. (3.2+ only)
//--
static QScriptValue js_donatePower(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::donatePower, context, engine);
}

//-- ## safeDest(player, x, y)
//--
//-- Returns true if given player is safe from hostile fire at the given location, to
//-- the best of that player's map knowledge. Does not work in campaign at the moment.
//--
static QScriptValue js_safeDest(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::safeDest, context, engine);
}

//-- ## addStructure(structure id, player, x, y)
//--
//-- Create a structure on the given position. Returns the structure on success, null otherwise.
//-- Position uses world coordinates, if you want use position based on Map Tiles, then
//-- use as addStructure(structure id, players, x*128, y*128)
//--
static QScriptValue js_addStructure(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::addStructure, context, engine);
}

//-- ## getStructureLimit(structure type[, player])
//--
//-- Returns build limits for a structure.
//--
static QScriptValue js_getStructureLimit(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::getStructureLimit, context, engine);
}

//-- ## countStruct(structure type[, player])
//--
//-- Count the number of structures of a given type.
//-- The player parameter can be a specific player, ALL_PLAYERS, ALLIES or ENEMIES.
//--
static QScriptValue js_countStruct(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::countStruct, context, engine);
}

//-- ## countDroid([droid type[, player]])
//--
//-- Count the number of droids that a given player has. Droid type must be either
//-- DROID_ANY, DROID_COMMAND or DROID_CONSTRUCT.
//-- The player parameter can be a specific player, ALL_PLAYERS, ALLIES or ENEMIES.
//--
static QScriptValue js_countDroid(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::countDroid, context, engine);
}

//-- ## setNoGoArea(x1, y1, x2, y2, player)
//--
//-- Creates an area on the map on which nothing can be built. If player is zero,
//-- then landing lights are placed. If player is -1, then a limbo landing zone
//-- is created and limbo droids placed.
//--
static QScriptValue js_setNoGoArea(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setNoGoArea, context, engine);
}

//-- ## setScrollLimits(x1, y1, x2, y2)
//--
//-- Limit the scrollable area of the map to the given rectangle. (3.2+ only)
//--
static QScriptValue js_setScrollLimits(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setScrollLimits, context, engine);
}

//-- ## getScrollLimits()
//--
//-- Get the limits of the scrollable area of the map as an area object. (3.2+ only)
//--
static QScriptValue js_getScrollLimits(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue ret = engine->newObject();
	ret.setProperty("x", scrollMinX, QScriptValue::ReadOnly);
	ret.setProperty("y", scrollMinY, QScriptValue::ReadOnly);
	ret.setProperty("x2", scrollMaxX, QScriptValue::ReadOnly);
	ret.setProperty("y2", scrollMaxY, QScriptValue::ReadOnly);
	ret.setProperty("type", SCRIPT_AREA, QScriptValue::ReadOnly);
	return ret;
}

//-- ## loadLevel(level name)
//--
//-- Load the level with the given name.
//--
static QScriptValue js_loadLevel(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::loadLevel, context, engine);
}

//-- ## autoSave()
//--
//-- Perform automatic save
//--
static QScriptValue js_autoSave(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::autoSave, context, engine);
}

//-- ## enumRange(x, y, range[, filter[, seen]])
//--
//-- Returns an array of game objects seen within range of given position that passes the optional filter
//-- which can be one of a player index, ALL_PLAYERS, ALLIES or ENEMIES. By default, filter is
//-- ALL_PLAYERS. Finally an optional parameter can specify whether only visible objects should be
//-- returned; by default only visible objects are returned. Calling this function is much faster than
//-- iterating over all game objects using other enum functions. (3.2+ only)
//--
static QScriptValue js_enumRange(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::enumRange, context, engine);
}

//-- ## enumArea(<x1, y1, x2, y2 | label>[, filter[, seen]])
//--
//-- Returns an array of game objects seen within the given area that passes the optional filter
//-- which can be one of a player index, ALL_PLAYERS, ALLIES or ENEMIES. By default, filter is
//-- ALL_PLAYERS. Finally an optional parameter can specify whether only visible objects should be
//-- returned; by default only visible objects are returned. The label can either be actual
//-- positions or a label to an AREA. Calling this function is much faster than iterating over all
//-- game objects using other enum functions. (3.2+ only)
//--
static QScriptValue js_enumArea(QScriptContext *context, QScriptEngine *engine)
{
	int player = engine->globalObject().property("me").toInt32();
	int x1, y1, x2, y2, nextparam;
	int filter = ALL_PLAYERS;
	bool seen = true;
	if (context->argument(0).isString())
	{
		std::string label = context->argument(0).toString().toStdString();
		nextparam = 1;
		SCRIPT_ASSERT(context, labels.count(label) > 0, "Label %s not found", label.c_str());
		const LABEL &p = labels[label];
		SCRIPT_ASSERT(context, p.type == SCRIPT_AREA, "Wrong label type for %s", label.c_str());
		x1 = p.p1.x;
		y1 = p.p1.y;
		x2 = p.p2.x;
		y2 = p.p2.y;
	}
	else
	{
		x1 = world_coord(context->argument(0).toInt32());
		y1 = world_coord(context->argument(1).toInt32());
		x2 = world_coord(context->argument(2).toInt32());
		y2 = world_coord(context->argument(3).toInt32());
		nextparam = 4;
	}
	if (context->argumentCount() > nextparam++)
	{
		filter = context->argument(nextparam - 1).toInt32();
	}
	if (context->argumentCount() > nextparam++)
	{
		seen = context->argument(nextparam - 1).toBool();
	}
	static GridList gridList;  // static to avoid allocations.
	gridList = gridStartIterateArea(x1, y1, x2, y2);
	QList<BASE_OBJECT *> list;
	for (GridIterator gi = gridList.begin(); gi != gridList.end(); ++gi)
	{
		BASE_OBJECT *psObj = *gi;
		if ((psObj->visible[player] || !seen) && !psObj->died)
		{
			if ((filter >= 0 && psObj->player == filter) || filter == ALL_PLAYERS
			    || (filter == ALLIES && psObj->type != OBJ_FEATURE && aiCheckAlliances(psObj->player, player))
			    || (filter == ENEMIES && psObj->type != OBJ_FEATURE && !aiCheckAlliances(psObj->player, player)))
			{
				list.append(psObj);
			}
		}
	}
	QScriptValue value = engine->newArray(list.size());
	for (int i = 0; i < list.size(); i++)
	{
		value.setProperty(i, convMax(list[i], engine), QScriptValue::ReadOnly);
	}
	return value;
}

//-- ## addBeacon(x, y, target player[, message])
//--
//-- Send a beacon message to target player. Target may also be ```ALLIES```.
//-- Message is currently unused. Returns a boolean that is true on success. (3.2+ only)
//--
static QScriptValue js_addBeacon(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::addBeacon, context, engine);
}

//-- ## removeBeacon(target player)
//--
//-- Remove a beacon message sent to target player. Target may also be ```ALLIES```.
//-- Returns a boolean that is true on success. (3.2+ only)
//--
static QScriptValue js_removeBeacon(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue retVal = wrap_(wzapi::removeBeacon, context, engine);
	if (retVal.isBool() && retVal.toBool())
	{
		jsDebugMessageUpdate();
	}
	return retVal;
}

//-- ## chat(target player, message)
//--
//-- Send a message to target player. Target may also be ```ALL_PLAYERS``` or ```ALLIES```.
//-- Returns a boolean that is true on success. (3.2+ only)
//--
static QScriptValue js_chat(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::chat, context, engine);
}

//-- ## setAlliance(player1, player2, value)
//--
//-- Set alliance status between two players to either true or false. (3.2+ only)
//--
static QScriptValue js_setAlliance(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setAlliance, context, engine);
}

//-- ## sendAllianceRequest(player)
//--
//-- Send an alliance request to a player. (3.3+ only)
//--
static QScriptValue js_sendAllianceRequest(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::sendAllianceRequest, context, engine);
}

//-- ## setAssemblyPoint(structure, x, y)
//--
//-- Set the assembly point droids go to when built for the specified structure. (3.2+ only)
//--
static QScriptValue js_setAssemblyPoint(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setAssemblyPoint, context, engine);
}

//-- ## hackNetOff()
//--
//-- Turn off network transmissions. FIXME - find a better way.
//--
static QScriptValue js_hackNetOff(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::hackNetOff, context, engine);
}

//-- ## hackNetOn()
//--
//-- Turn on network transmissions. FIXME - find a better way.
//--
static QScriptValue js_hackNetOn(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::hackNetOn, context, engine);
}

//-- ## getDroidProduction(factory)
//--
//-- Return droid in production in given factory. Note that this droid is fully
//-- virtual, and should never be passed anywhere. (3.2+ only)
//--
static QScriptValue js_getDroidProduction(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::getDroidProduction, context, engine);
}

//-- ## getDroidLimit([player[, unit type]])
//--
//-- Return maximum number of droids that this player can produce. This limit is usually
//-- fixed throughout a game and the same for all players. If no arguments are passed,
//-- returns general unit limit for the current player. If a second, unit type argument
//-- is passed, the limit for this unit type is returned, which may be different from
//-- the general unit limit (eg for commanders and construction droids). (3.2+ only)
//--
static QScriptValue js_getDroidLimit(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::getDroidLimit, context, engine);
}

//-- ## getExperienceModifier(player)
//--
//-- Get the percentage of experience this player droids are going to gain. (3.2+ only)
//--
static QScriptValue js_getExperienceModifier(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::getExperienceModifier, context, engine);
}

//-- ## setExperienceModifier(player, percent)
//--
//-- Set the percentage of experience this player droids are going to gain. (3.2+ only)
//--
static QScriptValue js_setExperienceModifier(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setExperienceModifier, context, engine);
}

//-- ## setDroidLimit(player, value[, droid type])
//--
//-- Set the maximum number of droids that this player can produce. If a third
//-- parameter is added, this is the droid type to limit. It can be DROID_ANY
//-- for droids in general, DROID_CONSTRUCT for constructors, or DROID_COMMAND
//-- for commanders. (3.2+ only)
//--
static QScriptValue js_setDroidLimit(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setDroidLimit, context, engine);
}

//-- ## setCommanderLimit(player, value)
//--
//-- Set the maximum number of commanders that this player can produce.
//-- THIS FUNCTION IS DEPRECATED AND WILL BE REMOVED! (3.2+ only)
//--
static QScriptValue js_setCommanderLimit(QScriptContext *context, QScriptEngine *)
{
	int player = context->argument(0).toInt32();
	int value = context->argument(1).toInt32();
	setMaxCommanders(player, value);
	return QScriptValue();
}

//-- ## setConstructorLimit(player, value)
//--
//-- Set the maximum number of constructors that this player can produce.
//-- THIS FUNCTION IS DEPRECATED AND WILL BE REMOVED! (3.2+ only)
//--
static QScriptValue js_setConstructorLimit(QScriptContext *context, QScriptEngine *)
{
	int player = context->argument(0).toInt32();
	int value = context->argument(1).toInt32();
	setMaxConstructors(player, value);
	return QScriptValue();
}

//-- ## hackAddMessage(message, type, player, immediate)
//--
//-- See wzscript docs for info, to the extent any exist. (3.2+ only)
//--
static QScriptValue js_hackAddMessage(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue retVal = wrap_(wzapi::hackAddMessage, context, engine);
	jsDebugMessageUpdate();
	return retVal;
}

//-- ## hackRemoveMessage(message, type, player)
//--
//-- See wzscript docs for info, to the extent any exist. (3.2+ only)
//--
static QScriptValue js_hackRemoveMessage(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue retVal = wrap_(wzapi::hackRemoveMessage, context, engine);
	jsDebugMessageUpdate();
	return retVal;
}

//-- ## setSunPosition(x, y, z)
//--
//-- Move the position of the Sun, which in turn moves where shadows are cast. (3.2+ only)
//--
static QScriptValue js_setSunPosition(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setSunPosition, context, engine);
}

//-- ## setSunIntensity(ambient r, g, b, diffuse r, g, b, specular r, g, b)
//--
//-- Set the ambient, diffuse and specular colour intensities of the Sun lighting source. (3.2+ only)
//--
static QScriptValue js_setSunIntensity(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setSunIntensity, context, engine);
}

//-- ## setWeather(weather type)
//--
//-- Set the current weather. This should be one of WEATHER_RAIN, WEATHER_SNOW or WEATHER_CLEAR. (3.2+ only)
//--
static QScriptValue js_setWeather(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setWeather, context, engine);
}

//-- ## setSky(texture file, wind speed, skybox scale)
//--
//-- Change the skybox. (3.2+ only)
//--
static QScriptValue js_setSky(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setSky, context, engine);
}

//-- ## hackDoNotSave(name)
//--
//-- Do not save the given global given by name to savegames. Must be
//-- done again each time game is loaded, since this too is not saved.
//--
static QScriptValue js_hackDoNotSave(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::hackDoNotSave, context, engine);
}

//-- ## hackMarkTiles([label | x, y[, x2, y2]])
//--
//-- Mark the given tile(s) on the map. Either give a POSITION or AREA label,
//-- or a tile x, y position, or four positions for a square area. If no parameter
//-- is given, all marked tiles are cleared. (3.2+ only)
//--
static QScriptValue js_hackMarkTiles(QScriptContext *context, QScriptEngine *)
{
	if (context->argumentCount() == 4) // square area
	{
		int x1 = context->argument(0).toInt32();
		int y1 = context->argument(1).toInt32();
		int x2 = context->argument(2).toInt32();
		int y2 = context->argument(3).toInt32();
		for (int x = x1; x < x2; x++)
		{
			for (int y = y1; y < y2; y++)
			{
				MAPTILE *psTile = mapTile(x, y);
				psTile->tileInfoBits |= BITS_MARKED;
			}
		}
	}
	else if (context->argumentCount() == 2) // single tile
	{
		int x = context->argument(0).toInt32();
		int y = context->argument(1).toInt32();
		MAPTILE *psTile = mapTile(x, y);
		psTile->tileInfoBits |= BITS_MARKED;
	}
	else if (context->argumentCount() == 1) // label
	{
		std::string label = context->argument(0).toString().toStdString();
		SCRIPT_ASSERT(context, labels.count(label) > 0, "Label %s not found", label.c_str());
		const LABEL &l = labels[label];
		if (l.type == SCRIPT_AREA)
		{
			for (int x = map_coord(l.p1.x); x < map_coord(l.p2.x); x++)
			{
				for (int y = map_coord(l.p1.y); y < map_coord(l.p2.y); y++)
				{
					MAPTILE *psTile = mapTile(x, y);
					psTile->tileInfoBits |= BITS_MARKED;
				}
			}
		}
		else if (l.type == SCRIPT_RADIUS)
		{
			for (int x = map_coord(l.p1.x - l.p2.x); x < map_coord(l.p1.x + l.p2.x); x++)
			{
				for (int y = map_coord(l.p1.y - l.p2.x); y < map_coord(l.p1.y + l.p2.x); y++)
				{
					if (x >= -1 && x < mapWidth + 1 && y >= -1 && y < mapWidth + 1 && iHypot(map_coord(l.p1) - Vector2i(x, y)) < map_coord(l.p2.x))
					{
						MAPTILE *psTile = mapTile(x, y);
						psTile->tileInfoBits |= BITS_MARKED;
					}
				}
			}
		}
	}
	else // clear all marks
	{
		clearMarks();
	}
	return QScriptValue();
}

//-- ## cameraSlide(x, y)
//--
//-- Slide the camera over to the given position on the map. (3.2+ only)
//--
static QScriptValue js_cameraSlide(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::cameraSlide, context, engine);
}

//-- ## cameraZoom(z, speed)
//--
//-- Slide the camera to the given zoom distance. Normal camera zoom ranges between 500 and 5000. (3.2+ only)
//--
static QScriptValue js_cameraZoom(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::cameraZoom, context, engine);
}

//-- ## cameraTrack(droid)
//--
//-- Make the camera follow the given droid object around. Pass in a null object to stop. (3.2+ only)
//--
static QScriptValue js_cameraTrack(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::cameraTrack, context, engine);
}

//-- ## setHealth(object, health)
//--
//-- Change the health of the given game object, in percentage. Does not take care of network sync, so for multiplayer games,
//-- needs wrapping in a syncRequest. (3.2.3+ only.)
//--
static QScriptValue js_setHealth(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setHealth, context, engine);
}

//-- ## setObjectFlag(object, flag, value)
//--
//-- Set or unset an object flag on a given game object. Does not take care of network sync, so for multiplayer games,
//-- needs wrapping in a syncRequest. (3.3+ only.)
//-- Recognized object flags: OBJECT_FLAG_UNSELECTABLE - makes object unavailable for selection from player UI.
//--
static QScriptValue js_setObjectFlag(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setObjectFlag, context, engine);
}

//-- ## addSpotter(x, y, player, range, type, expiry)
//--
//-- Add an invisible viewer at a given position for given player that shows map in given range. ```type```
//-- is zero for vision reveal, or one for radar reveal. The difference is that a radar reveal can be obstructed
//-- by ECM jammers. ```expiry```, if non-zero, is the game time at which the spotter shall automatically be
//-- removed. The function returns a unique ID that can be used to remove the spotter with ```removeSpotter```. (3.2+ only)
//--
static QScriptValue js_addSpotter(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::addSpotter, context, engine);
}

//-- ## removeSpotter(id)
//--
//-- Remove a spotter given its unique ID. (3.2+ only)
//--
static QScriptValue js_removeSpotter(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::removeSpotter, context, engine);
}

//-- ## syncRandom(limit)
//--
//-- Generate a synchronized random number in range 0...(limit - 1) that will be the same if this function is
//-- run on all network peers in the same game frame. If it is called on just one peer (such as would be
//-- the case for AIs, for instance), then game sync will break. (3.2+ only)
//--
static QScriptValue js_syncRandom(QScriptContext *context, QScriptEngine * engine)
{
	return wrap_(wzapi::syncRandom, context, engine);
}

//-- ## syncRequest(req_id, x, y[, obj[, obj2]])
//--
//-- Generate a synchronized event request that is sent over the network to all clients and executed simultaneously.
//-- Must be caught in an eventSyncRequest() function. All sync requests must be validated when received, and always
//-- take care only to define sync requests that can be validated against cheating. (3.2+ only)
//--
static QScriptValue js_syncRequest(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::syncRequest, context, engine);
}

//-- ## replaceTexture(old_filename, new_filename)
//--
//-- Replace one texture with another. This can be used to for example give buildings on a specific tileset different
//-- looks, or to add variety to the looks of droids in campaign missions. (3.2+ only)
//--
static QScriptValue js_replaceTexture(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::replaceTexture, context, engine);
}

//-- ## fireWeaponAtLoc(weapon, x, y[, player])
//--
//-- Fires a weapon at the given coordinates (3.3+ only). The player is who owns the projectile.
//-- Please use fireWeaponAtObj() to damage objects as multiplayer and campaign
//-- may have different friendly fire logic for a few weapons (like the lassat).
//--
static QScriptValue js_fireWeaponAtLoc(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::fireWeaponAtLoc, context, engine);
}

//-- ## fireWeaponAtObj(weapon, game object[, player])
//--
//-- Fires a weapon at a game object (3.3+ only). The player is who owns the projectile.
//--
static QScriptValue js_fireWeaponAtObj(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::fireWeaponAtObj, context, engine);
}

//-- ## changePlayerColour(player, colour)
//--
//-- Change a player's colour slot. The current player colour can be read from the ```playerData``` array. There are as many
//-- colour slots as the maximum number of players. (3.2.3+ only)
//--
static QScriptValue js_changePlayerColour(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::changePlayerColour, context, engine);
}

//-- ## getMultiTechLevel()
//--
//-- Returns the current multiplayer tech level. (3.3+ only)
//--
static QScriptValue js_getMultiTechLevel(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::getMultiTechLevel, context, engine);
}

//-- ## setCampaignNumber(num)
//--
//-- Set the campaign number. (3.3+ only)
//--
static QScriptValue js_setCampaignNumber(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setCampaignNumber, context, engine);
}

//-- ## getMissionType()
//--
//-- Return the current mission type. (3.3+ only)
//--
static QScriptValue js_getMissionType(QScriptContext *context, QScriptEngine *)
{
	return (int)mission.type;
}

//-- ## getRevealStatus()
//--
//-- Return the current fog reveal status. (3.3+ only)
//--
static QScriptValue js_getRevealStatus(QScriptContext *context, QScriptEngine *)
{
	return QScriptValue(getRevealStatus());
}

//-- ## setRevealStatus(bool)
//--
//-- Set the fog reveal status. (3.3+ only)
static QScriptValue js_setRevealStatus(QScriptContext *context, QScriptEngine *engine)
{
	return wrap_(wzapi::setRevealStatus, context, engine);
}

// ----------------------------------------------------------------------------------------
// Register functions with scripting system

bool unregisterFunctions(QScriptEngine *engine)
{
	int num = 0;
	auto it = groups.find(engine);
	if (it != groups.end())
	{
		GROUPMAP *psMap = groups.at(engine);
		groups.erase(it);
		delete psMap;
		num = 1;
	}
	ASSERT(num == 1, "Number of engines removed from group map is %d!", num);
	labels.clear();
	labelModel = nullptr;
	return true;
}

// Call this just before launching game; we can't do everything in registerFunctions()
// since all game state may not be fully loaded by then
void prepareLabels()
{
	// load the label group data into every scripting context, with the same negative group id
	for (ENGINEMAP::iterator iter = groups.begin(); iter != groups.end(); ++iter)
	{
		QScriptEngine *engine = iter->first;
		for (LABELMAP::iterator i = labels.begin(); i != labels.end(); ++i)
		{
			const LABEL &l = i->second;
			if (l.type == SCRIPT_GROUP)
			{
				QScriptValue groupMembers = engine->globalObject().property("groupSizes");
				groupMembers.setProperty(l.id, (int)l.idlist.size(), QScriptValue::ReadOnly);
				for (std::vector<int>::const_iterator j = l.idlist.begin(); j != l.idlist.end(); j++)
				{
					int id = (*j);
					BASE_OBJECT *psObj = IdToPointer(id, l.player);
					ASSERT(psObj, "Unit %d belonging to player %d not found", id, l.player);
					if (psObj)
					{
						groupAddObject(psObj, l.id, engine);
					}
				}
			}
		}
	}
	updateLabelModel();
}

// flag all droids as requiring update on next frame
static void dirtyAllDroids(int player)
{
	for (DROID *psDroid = apsDroidLists[player]; psDroid != nullptr; psDroid = psDroid->psNext)
	{
		psDroid->flags.set(OBJECT_FLAG_DIRTY);
	}
	for (DROID *psDroid = mission.apsDroidLists[player]; psDroid != nullptr; psDroid = psDroid->psNext)
	{
		psDroid->flags.set(OBJECT_FLAG_DIRTY);
	}
	for (DROID *psDroid = apsLimboDroids[player]; psDroid != nullptr; psDroid = psDroid->psNext)
	{
		psDroid->flags.set(OBJECT_FLAG_DIRTY);
	}
}

static void dirtyAllStructures(int player)
{
	for (STRUCTURE *psCurr = apsStructLists[player]; psCurr; psCurr = psCurr->psNext)
	{
		psCurr->flags.set(OBJECT_FLAG_DIRTY);
	}
	for (STRUCTURE *psCurr = mission.apsStructLists[player]; psCurr; psCurr = psCurr->psNext)
	{
		psCurr->flags.set(OBJECT_FLAG_DIRTY);
	}
}

QScriptValue js_stats(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue callee = context->callee();
	int type = callee.property("type").toInt32();
	int player = callee.property("player").toInt32();
	unsigned index = callee.property("index").toUInt32();
	QString name = callee.property("name").toString();
	if (context->argumentCount() == 1) // setter
	{
		int value = context->argument(0).toInt32();
		syncDebug("stats[p%d,t%d,%s,i%d] = %d", player, type, name.toStdString().c_str(), index, value);
		if (type == COMP_BODY)
		{
			SCRIPT_ASSERT(context, index < numBodyStats, "Bad index");
			BODY_STATS *psStats = asBodyStats + index;
			if (name == "HitPoints")
			{
				psStats->upgrade[player].hitpoints = value;
				dirtyAllDroids(player);
			}
			else if (name == "HitPointPct")
			{
				psStats->upgrade[player].hitpointPct = value;
				dirtyAllDroids(player);
			}
			else if (name == "Armour")
			{
				psStats->upgrade[player].armour = value;
			}
			else if (name == "Thermal")
			{
				psStats->upgrade[player].thermal = value;
			}
			else if (name == "Power")
			{
				psStats->upgrade[player].power = value;
			}
			else if (name == "Resistance")
			{
				// TBD FIXME - not updating resistance points in droids...
				psStats->upgrade[player].resistance = value;
			}
			else
			{
				SCRIPT_ASSERT(context, false, "Upgrade component %s not found", name.toUtf8().constData());
			}
		}
		else if (type == COMP_BRAIN)
		{
			SCRIPT_ASSERT(context, index < numBrainStats, "Bad index");
			BRAIN_STATS *psStats = asBrainStats + index;
			if (name == "BaseCommandLimit")
			{
				psStats->upgrade[player].maxDroids = value;
			}
			else if (name == "CommandLimitByLevel")
			{
				psStats->upgrade[player].maxDroidsMult = value;
			}
			else if (name == "RankThresholds")
			{
				SCRIPT_ASSERT(context, context->argument(0).isArray(), "Level thresholds not an array!");
				int length = context->argument(0).property("length").toInt32();
				for (int i = 0; i < length; i++)
				{
					psStats->upgrade[player].rankThresholds[i] = context->argument(0).property(i).toInt32();
				}
			}
			else if (name == "HitPoints")
			{
				psStats->upgrade[player].hitpoints = value;
				dirtyAllDroids(player);
			}
			else if (name == "HitPointPct")
			{
				psStats->upgrade[player].hitpointPct = value;
				dirtyAllDroids(player);
			}
			else
			{
				SCRIPT_ASSERT(context, false, "Upgrade component %s not found", name.toUtf8().constData());
			}
		}
		else if (type == COMP_SENSOR)
		{
			SCRIPT_ASSERT(context, index < numSensorStats, "Bad index");
			SENSOR_STATS *psStats = asSensorStats + index;
			if (name == "Range")
			{
				psStats->upgrade[player].range = value;
				dirtyAllDroids(player);
				dirtyAllStructures(player);
			}
			else if (name == "HitPoints")
			{
				psStats->upgrade[player].hitpoints = value;
				dirtyAllDroids(player);
			}
			else if (name == "HitPointPct")
			{
				psStats->upgrade[player].hitpointPct = value;
				dirtyAllDroids(player);
			}
			else
			{
				SCRIPT_ASSERT(context, false, "Upgrade component %s not found", name.toUtf8().constData());
			}
		}
		else if (type == COMP_ECM)
		{
			SCRIPT_ASSERT(context, index < numECMStats, "Bad index");
			ECM_STATS *psStats = asECMStats + index;
			if (name == "Range")
			{
				psStats->upgrade[player].range = value;
				dirtyAllDroids(player);
				dirtyAllStructures(player);
			}
			else if (name == "HitPoints")
			{
				psStats->upgrade[player].hitpoints = value;
				dirtyAllDroids(player);
			}
			else if (name == "HitPointPct")
			{
				psStats->upgrade[player].hitpointPct = value;
				dirtyAllDroids(player);
			}
			else
			{
				SCRIPT_ASSERT(context, false, "Upgrade component %s not found", name.toUtf8().constData());
			}
		}
		else if (type == COMP_PROPULSION)
		{
			SCRIPT_ASSERT(context, index < numPropulsionStats, "Bad index");
			PROPULSION_STATS *psStats = asPropulsionStats + index;
			if (name == "HitPoints")
			{
				psStats->upgrade[player].hitpoints = value;
				dirtyAllDroids(player);
			}
			else if (name == "HitPointPct")
			{
				psStats->upgrade[player].hitpointPct = value;
				dirtyAllDroids(player);
			}
			else if (name == "HitPointPctOfBody")
			{
				psStats->upgrade[player].hitpointPctOfBody = value;
				dirtyAllDroids(player);
			}
			else
			{
				SCRIPT_ASSERT(context, false, "Upgrade component %s not found", name.toUtf8().constData());
			}
		}
		else if (type == COMP_CONSTRUCT)
		{
			SCRIPT_ASSERT(context, index < numConstructStats, "Bad index");
			CONSTRUCT_STATS *psStats = asConstructStats + index;
			if (name == "ConstructorPoints")
			{
				psStats->upgrade[player].constructPoints = value;
			}
			else if (name == "HitPoints")
			{
				psStats->upgrade[player].hitpoints = value;
				dirtyAllDroids(player);
			}
			else if (name == "HitPointPct")
			{
				psStats->upgrade[player].hitpointPct = value;
				dirtyAllDroids(player);
			}
			else
			{
				SCRIPT_ASSERT(context, false, "Upgrade component %s not found", name.toUtf8().constData());
			}
		}
		else if (type == COMP_REPAIRUNIT)
		{
			SCRIPT_ASSERT(context, index < numRepairStats, "Bad index");
			REPAIR_STATS *psStats = asRepairStats + index;
			if (name == "RepairPoints")
			{
				psStats->upgrade[player].repairPoints = value;
			}
			else if (name == "HitPoints")
			{
				psStats->upgrade[player].hitpoints = value;
				dirtyAllDroids(player);
			}
			else if (name == "HitPointPct")
			{
				psStats->upgrade[player].hitpointPct = value;
				dirtyAllDroids(player);
			}
			else
			{
				SCRIPT_ASSERT(context, false, "Upgrade component %s not found", name.toUtf8().constData());
			}
		}
		else if (type == COMP_WEAPON)
		{
			SCRIPT_ASSERT(context, index < numWeaponStats, "Bad index");
			WEAPON_STATS *psStats = asWeaponStats + index;
			if (name == "MaxRange")
			{
				psStats->upgrade[player].maxRange = value;
			}
			else if (name == "ShortRange")
			{
				psStats->upgrade[player].shortRange = value;
			}
			else if (name == "MinRange")
			{
				psStats->upgrade[player].minRange = value;
			}
			else if (name == "HitChance")
			{
				psStats->upgrade[player].hitChance = value;
			}
			else if (name == "ShortHitChance")
			{
				psStats->upgrade[player].shortHitChance = value;
			}
			else if (name == "FirePause")
			{
				psStats->upgrade[player].firePause = value;
			}
			else if (name == "Rounds")
			{
				psStats->upgrade[player].numRounds = value;
			}
			else if (name == "ReloadTime")
			{
				psStats->upgrade[player].reloadTime = value;
			}
			else if (name == "Damage")
			{
				psStats->upgrade[player].damage = value;
			}
			else if (name == "MinimumDamage")
			{
				psStats->upgrade[player].minimumDamage = value;
			}
			else if (name == "Radius")
			{
				psStats->upgrade[player].radius = value;
			}
			else if (name == "RadiusDamage")
			{
				psStats->upgrade[player].radiusDamage = value;
			}
			else if (name == "RepeatDamage")
			{
				psStats->upgrade[player].periodicalDamage = value;
			}
			else if (name == "RepeatTime")
			{
				psStats->upgrade[player].periodicalDamageTime = value;
			}
			else if (name == "RepeatRadius")
			{
				psStats->upgrade[player].periodicalDamageRadius = value;
			}
			else if (name == "HitPoints")
			{
				psStats->upgrade[player].hitpoints = value;
				dirtyAllDroids(player);
			}
			else if (name == "HitPointPct")
			{
				psStats->upgrade[player].hitpointPct = value;
				dirtyAllDroids(player);
			}
			else
			{
				SCRIPT_ASSERT(context, false, "Invalid weapon method");
			}
		}
		else if (type >= SCRCB_FIRST && type <= SCRCB_LAST)
		{
			SCRIPT_ASSERT(context, index < numStructureStats, "Bad index");
			STRUCTURE_STATS *psStats = asStructureStats + index;
			switch ((Scrcb)type)
			{
			case SCRCB_RES: psStats->upgrade[player].research = value; break;
			case SCRCB_MODULE_RES: psStats->upgrade[player].moduleResearch = value; break;
			case SCRCB_REP: psStats->upgrade[player].repair = value; break;
			case SCRCB_POW: psStats->upgrade[player].power = value; break;
			case SCRCB_MODULE_POW: psStats->upgrade[player].modulePower = value; break;
			case SCRCB_CON: psStats->upgrade[player].production = value; break;
			case SCRCB_MODULE_CON: psStats->upgrade[player].moduleProduction = value; break;
			case SCRCB_REA: psStats->upgrade[player].rearm = value; break;
			case SCRCB_HEA: psStats->upgrade[player].thermal = value; break;
			case SCRCB_ARM: psStats->upgrade[player].armour = value; break;
			case SCRCB_ELW:
				// Update resistance points for all structures, to avoid making them damaged
				// FIXME - this is _really_ slow! we could be doing this for dozens of buildings one at a time!
				for (STRUCTURE *psCurr = apsStructLists[player]; psCurr; psCurr = psCurr->psNext)
				{
					if (psStats == psCurr->pStructureType && psStats->upgrade[player].resistance < value)
					{
						psCurr->resistance = value;
					}
				}
				for (STRUCTURE *psCurr = mission.apsStructLists[player]; psCurr; psCurr = psCurr->psNext)
				{
					if (psStats == psCurr->pStructureType && psStats->upgrade[player].resistance < value)
					{
						psCurr->resistance = value;
					}
				}
				psStats->upgrade[player].resistance = value;
				break;
			case SCRCB_HIT:
				// Update body points for all structures, to avoid making them damaged
				// FIXME - this is _really_ slow! we could be doing this for
				// dozens of buildings one at a time!
				for (STRUCTURE *psCurr = apsStructLists[player]; psCurr; psCurr = psCurr->psNext)
				{
					if (psStats == psCurr->pStructureType && psStats->upgrade[player].hitpoints < value)
					{
						psCurr->body = (psCurr->body * value) / psStats->upgrade[player].hitpoints;
					}
				}
				for (STRUCTURE *psCurr = mission.apsStructLists[player]; psCurr; psCurr = psCurr->psNext)
				{
					if (psStats == psCurr->pStructureType && psStats->upgrade[player].hitpoints < value)
					{
						psCurr->body = (psCurr->body * value) / psStats->upgrade[player].hitpoints;
					}
				}
				psStats->upgrade[player].hitpoints = value;
				break;
			case SCRCB_LIMIT:
				psStats->upgrade[player].limit = value; break;
			}
		}
		else
		{
			SCRIPT_ASSERT(context, false, "Component type not found for upgrade");
		}
	}
	// Now read value and return it
	if (type == COMP_BODY)
	{
		SCRIPT_ASSERT(context, index < numBodyStats, "Bad index");
		const BODY_STATS *psStats = asBodyStats + index;
		if (name == "HitPoints")
		{
			return psStats->upgrade[player].hitpoints;
		}
		else if (name == "HitPointPct")
		{
			return psStats->upgrade[player].hitpointPct;
		}
		else if (name == "Armour")
		{
			return psStats->upgrade[player].armour;
		}
		else if (name == "Thermal")
		{
			return psStats->upgrade[player].thermal;
		}
		else if (name == "Power")
		{
			return psStats->upgrade[player].power;
		}
		else if (name == "Resistance")
		{
			return psStats->upgrade[player].resistance;
		}
		else
		{
			SCRIPT_ASSERT(context, false, "Upgrade component %s not found", name.toUtf8().constData());
		}
	}
	else if (type == COMP_BRAIN)
	{
		SCRIPT_ASSERT(context, index < numBrainStats, "Bad index");
		const BRAIN_STATS *psStats = asBrainStats + index;
		if (name == "BaseCommandLimit")
		{
			return psStats->upgrade[player].maxDroids;
		}
		else if (name == "CommandLimitByLevel")
		{
			return psStats->upgrade[player].maxDroidsMult;
		}
		else if (name == "RankThresholds")
		{
			int length = psStats->upgrade[player].rankThresholds.size();
			QScriptValue value = engine->newArray(length);
			for (int i = 0; i < length; i++)
			{
				value.setProperty(i, psStats->upgrade[player].rankThresholds[i], QScriptValue::Undeletable | QScriptValue::ReadOnly);
			}
			return value;
		}
		else if (name == "HitPoints")
		{
			return psStats->upgrade[player].hitpoints;
		}
		else if (name == "HitPointPct")
		{
			return psStats->upgrade[player].hitpointPct;
		}
		else
		{
			SCRIPT_ASSERT(context, false, "Upgrade component %s not found", name.toUtf8().constData());
		}
	}
	else if (type == COMP_SENSOR)
	{
		SCRIPT_ASSERT(context, index < numSensorStats, "Bad index");
		const SENSOR_STATS *psStats = asSensorStats + index;
		if (name == "Range")
		{
			return psStats->upgrade[player].range;
		}
		else if (name == "HitPoints")
		{
			return psStats->upgrade[player].hitpoints;
		}
		else if (name == "HitPointPct")
		{
			return psStats->upgrade[player].hitpointPct;
		}
		else
		{
			SCRIPT_ASSERT(context, false, "Upgrade component %s not found", name.toUtf8().constData());
		}
	}
	else if (type == COMP_ECM)
	{
		SCRIPT_ASSERT(context, index < numECMStats, "Bad index");
		const ECM_STATS *psStats = asECMStats + index;
		if (name == "Range")
		{
			return psStats->upgrade[player].range;
		}
		else if (name == "HitPoints")
		{
			return psStats->upgrade[player].hitpoints;
		}
		else if (name == "HitPointPct")
		{
			return psStats->upgrade[player].hitpointPct;
		}
		else
		{
			SCRIPT_ASSERT(context, false, "Upgrade component %s not found", name.toUtf8().constData());
		}
	}
	else if (type == COMP_PROPULSION)
	{
		SCRIPT_ASSERT(context, index < numPropulsionStats, "Bad index");
		const PROPULSION_STATS *psStats = asPropulsionStats + index;
		if (name == "HitPoints")
		{
			return psStats->upgrade[player].hitpoints;
		}
		else if (name == "HitPointPct")
		{
			return psStats->upgrade[player].hitpointPct;
		}
		else if (name == "HitPointPctOfBody")
		{
			return psStats->upgrade[player].hitpointPctOfBody;
		}
		else
		{
			SCRIPT_ASSERT(context, false, "Upgrade component %s not found", name.toUtf8().constData());
		}
	}
	else if (type == COMP_CONSTRUCT)
	{
		SCRIPT_ASSERT(context, index < numConstructStats, "Bad index");
		const CONSTRUCT_STATS *psStats = asConstructStats + index;
		if (name == "ConstructorPoints")
		{
			return psStats->upgrade[player].constructPoints;
		}
		else if (name == "HitPoints")
		{
			return psStats->upgrade[player].hitpoints;
		}
		else if (name == "HitPointPct")
		{
			return psStats->upgrade[player].hitpointPct;
		}
		else
		{
			SCRIPT_ASSERT(context, false, "Upgrade component %s not found", name.toUtf8().constData());
		}
	}
	else if (type == COMP_REPAIRUNIT)
	{
		SCRIPT_ASSERT(context, index < numRepairStats, "Bad index");
		const REPAIR_STATS *psStats = asRepairStats + index;
		if (name == "RepairPoints")
		{
			return psStats->upgrade[player].repairPoints;
		}
		else if (name == "HitPoints")
		{
			return psStats->upgrade[player].hitpoints;
		}
		else if (name == "HitPointPct")
		{
			return psStats->upgrade[player].hitpointPct;
		}
		else
		{
			SCRIPT_ASSERT(context, false, "Upgrade component %s not found", name.toUtf8().constData());
		}
	}
	else if (type == COMP_WEAPON)
	{
		SCRIPT_ASSERT(context, index < numWeaponStats, "Bad index");
		const WEAPON_STATS *psStats = asWeaponStats + index;
		if (name == "MaxRange")
		{
			return psStats->upgrade[player].maxRange;
		}
		else if (name == "ShortRange")
		{
			return psStats->upgrade[player].shortRange;
		}
		else if (name == "MinRange")
		{
			return psStats->upgrade[player].minRange;
		}
		else if (name == "HitChance")
		{
			return psStats->upgrade[player].hitChance;
		}
		else if (name == "ShortHitChance")
		{
			return psStats->upgrade[player].shortHitChance;
		}
		else if (name == "FirePause")
		{
			return psStats->upgrade[player].firePause;
		}
		else if (name == "Rounds")
		{
			return psStats->upgrade[player].numRounds;
		}
		else if (name == "ReloadTime")
		{
			return psStats->upgrade[player].reloadTime;
		}
		else if (name == "Damage")
		{
			return psStats->upgrade[player].damage;
		}
		else if (name == "MinimumDamage")
		{
			return psStats->upgrade[player].minimumDamage;
		}
		else if (name == "Radius")
		{
			return psStats->upgrade[player].radius;
		}
		else if (name == "RadiusDamage")
		{
			return psStats->upgrade[player].radiusDamage;
		}
		else if (name == "RepeatDamage")
		{
			return psStats->upgrade[player].periodicalDamage;
		}
		else if (name == "RepeatTime")
		{
			return psStats->upgrade[player].periodicalDamageTime;
		}
		else if (name == "RepeatRadius")
		{
			return psStats->upgrade[player].periodicalDamageRadius;
		}
		else if (name == "HitPoints")
		{
			return psStats->upgrade[player].hitpoints;
		}
		else if (name == "HitPointPct")
		{
			return psStats->upgrade[player].hitpointPct;
		}
		else
		{
			SCRIPT_ASSERT(context, false, "Invalid weapon method");
		}
	}
	else if (type >= SCRCB_FIRST && type <= SCRCB_LAST)
	{
		SCRIPT_ASSERT(context, index < numStructureStats, "Bad index");
		const STRUCTURE_STATS *psStats = asStructureStats + index;
		switch (type)
		{
		case SCRCB_RES: return psStats->upgrade[player].research; break;
		case SCRCB_MODULE_RES: return psStats->upgrade[player].moduleResearch; break;
		case SCRCB_REP: return psStats->upgrade[player].repair; break;
		case SCRCB_POW: return psStats->upgrade[player].power; break;
		case SCRCB_MODULE_POW: return psStats->upgrade[player].modulePower; break;
		case SCRCB_CON: return psStats->upgrade[player].production; break;
		case SCRCB_MODULE_CON: return psStats->upgrade[player].moduleProduction; break;
		case SCRCB_REA: return psStats->upgrade[player].rearm; break;
		case SCRCB_ELW: return psStats->upgrade[player].resistance; break;
		case SCRCB_HEA: return psStats->upgrade[player].thermal; break;
		case SCRCB_ARM: return psStats->upgrade[player].armour; break;
		case SCRCB_HIT: return psStats->upgrade[player].hitpoints;
		case SCRCB_LIMIT: return psStats->upgrade[player].limit;
		default: SCRIPT_ASSERT(context, false, "Component type not found for upgrade"); break;
		}
	}
	return QScriptValue::NullValue;
}

static void setStatsFunc(QScriptValue &base, QScriptEngine *engine, const QString& name, int player, int type, int index)
{
	QScriptValue v = engine->newFunction(js_stats);
	base.setProperty(name, v, QScriptValue::PropertyGetter | QScriptValue::PropertySetter);
	v.setProperty("player", player, QScriptValue::SkipInEnumeration | QScriptValue::ReadOnly | QScriptValue::Undeletable);
	v.setProperty("type", type, QScriptValue::SkipInEnumeration | QScriptValue::ReadOnly | QScriptValue::Undeletable);
	v.setProperty("index", index, QScriptValue::SkipInEnumeration | QScriptValue::ReadOnly | QScriptValue::Undeletable);
	v.setProperty("name", name, QScriptValue::SkipInEnumeration | QScriptValue::ReadOnly | QScriptValue::Undeletable);
}

QScriptValue register_common(QScriptEngine *engine, COMPONENT_STATS *psStats)
{
	QScriptValue v = engine->newObject();
	v.setProperty("Id", WzStringToQScriptValue(engine, psStats->id), QScriptValue::ReadOnly | QScriptValue::Undeletable);
	v.setProperty("Weight", psStats->weight, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	v.setProperty("BuildPower", psStats->buildPower, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	v.setProperty("BuildTime", psStats->buildPoints, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	v.setProperty("HitPoints", psStats->getBase().hitpoints, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	v.setProperty("HitPointPct", psStats->getBase().hitpointPct, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	return v;
}

bool registerFunctions(QScriptEngine *engine, const QString& scriptName)
{
	debug(LOG_WZ, "Loading functions for engine %p, script %s", static_cast<void *>(engine), scriptName.toUtf8().constData());

	// Create group map
	GROUPMAP *psMap = new GROUPMAP;
	auto insert_result = groups.insert(ENGINEMAP::value_type(engine, psMap));
	ASSERT(insert_result.second, "Entry for this engine %p already exists in ENGINEMAP!", static_cast<void *>(engine));

	/// Register 'Stats' object. It is a read-only representation of basic game component states.
	//== * ```Stats``` A sparse, read-only array containing rules information for game entity types.
	//== (For now only the highest level member attributes are documented here. Use the 'jsdebug' cheat
	//== to see them all.)
	//== These values are defined:
	QScriptValue stats = engine->newObject();
	{
		//==   * ```Body``` Droid bodies
		QScriptValue bodybase = engine->newObject();
		for (int j = 0; j < numBodyStats; j++)
		{
			BODY_STATS *psStats = asBodyStats + j;
			QScriptValue body = register_common(engine, psStats);
			body.setProperty("Power", psStats->base.power, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			body.setProperty("Armour", psStats->base.armour, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			body.setProperty("Thermal", psStats->base.thermal, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			body.setProperty("Resistance", psStats->base.resistance, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			body.setProperty("Size", psStats->size, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			body.setProperty("WeaponSlots", psStats->weaponSlots, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			body.setProperty("BodyClass", QString::fromUtf8(psStats->bodyClass.toUtf8().c_str()), QScriptValue::ReadOnly | QScriptValue::Undeletable);
			bodybase.setProperty(QString::fromUtf8(psStats->name.toUtf8().c_str()), body, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		}
		stats.setProperty("Body", bodybase, QScriptValue::ReadOnly | QScriptValue::Undeletable);

		//==   * ```Sensor``` Sensor turrets
		QScriptValue sensorbase = engine->newObject();
		for (int j = 0; j < numSensorStats; j++)
		{
			SENSOR_STATS *psStats = asSensorStats + j;
			QScriptValue sensor = register_common(engine, psStats);
			sensor.setProperty("Range", psStats->base.range, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			sensorbase.setProperty(QString::fromUtf8(psStats->name.toUtf8().c_str()), sensor, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		}
		stats.setProperty("Sensor", sensorbase, QScriptValue::ReadOnly | QScriptValue::Undeletable);

		//==   * ```ECM``` ECM (Electronic Counter-Measure) turrets
		QScriptValue ecmbase = engine->newObject();
		for (int j = 0; j < numECMStats; j++)
		{
			ECM_STATS *psStats = asECMStats + j;
			QScriptValue ecm = register_common(engine, psStats);
			ecm.setProperty("Range", psStats->base.range, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			ecmbase.setProperty(QString::fromUtf8(psStats->name.toUtf8().c_str()), ecm, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		}
		stats.setProperty("ECM", ecmbase, QScriptValue::ReadOnly | QScriptValue::Undeletable);

		//==   * ```Propulsion``` Propulsions
		QScriptValue propbase = engine->newObject();
		for (int j = 0; j < numPropulsionStats; j++)
		{
			PROPULSION_STATS *psStats = asPropulsionStats + j;
			QScriptValue v = register_common(engine, psStats);
			v.setProperty("HitpointPctOfBody", psStats->base.hitpointPctOfBody, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			v.setProperty("MaxSpeed", psStats->maxSpeed, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			v.setProperty("TurnSpeed", psStats->turnSpeed, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			v.setProperty("SpinSpeed", psStats->spinSpeed, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			v.setProperty("SpinAngle", psStats->spinAngle, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			v.setProperty("SkidDeceleration", psStats->skidDeceleration, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			v.setProperty("Acceleration", psStats->acceleration, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			v.setProperty("Deceleration", psStats->deceleration, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			propbase.setProperty(QString::fromUtf8(psStats->name.toUtf8().c_str()), v, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		}
		stats.setProperty("Propulsion", propbase, QScriptValue::ReadOnly | QScriptValue::Undeletable);

		//==   * ```Repair``` Repair turrets (not used, incidentally, for repair centers)
		QScriptValue repairbase = engine->newObject();
		for (int j = 0; j < numRepairStats; j++)
		{
			REPAIR_STATS *psStats = asRepairStats + j;
			QScriptValue repair = register_common(engine, psStats);
			repair.setProperty("RepairPoints", psStats->base.repairPoints, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			repairbase.setProperty(QString::fromUtf8(psStats->name.toUtf8().c_str()), repair, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		}
		stats.setProperty("Repair", repairbase, QScriptValue::ReadOnly | QScriptValue::Undeletable);

		//==   * ```Construct``` Constructor turrets (eg for trucks)
		QScriptValue conbase = engine->newObject();
		for (int j = 0; j < numConstructStats; j++)
		{
			CONSTRUCT_STATS *psStats = asConstructStats + j;
			QScriptValue con = register_common(engine, psStats);
			con.setProperty("ConstructorPoints", psStats->base.constructPoints, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			conbase.setProperty(QString::fromUtf8(psStats->name.toUtf8().c_str()), con, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		}
		stats.setProperty("Construct", conbase, QScriptValue::ReadOnly | QScriptValue::Undeletable);

		//==   * ```Brain``` Brains
		QScriptValue brainbase = engine->newObject();
		for (int j = 0; j < numBrainStats; j++)
		{
			BRAIN_STATS *psStats = asBrainStats + j;
			QScriptValue br = register_common(engine, psStats);
			br.setProperty("BaseCommandLimit", psStats->base.maxDroids, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			br.setProperty("CommandLimitByLevel", psStats->base.maxDroidsMult, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			QScriptValue thresholds = engine->newArray(psStats->base.rankThresholds.size());
			for (int x = 0; x < psStats->base.rankThresholds.size(); x++)
			{
				thresholds.setProperty(x, psStats->base.rankThresholds.at(x), QScriptValue::ReadOnly | QScriptValue::Undeletable);
			}
			br.setProperty("RankThresholds", thresholds, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			QScriptValue ranks = engine->newArray(psStats->rankNames.size());
			for (int x = 0; x < psStats->rankNames.size(); x++)
			{
				ranks.setProperty(x, QString::fromStdString(psStats->rankNames.at(x)), QScriptValue::ReadOnly | QScriptValue::Undeletable);
			}
			br.setProperty("RankNames", ranks, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			brainbase.setProperty(QString::fromUtf8(psStats->name.toUtf8().c_str()), br, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		}
		stats.setProperty("Brain", brainbase, QScriptValue::ReadOnly | QScriptValue::Undeletable);

		//==   * ```Weapon``` Weapon turrets
		QScriptValue wbase = engine->newObject();
		for (int j = 0; j < numWeaponStats; j++)
		{
			WEAPON_STATS *psStats = asWeaponStats + j;
			QScriptValue weap = register_common(engine, psStats);
			weap.setProperty("MaxRange", psStats->base.maxRange, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			weap.setProperty("ShortRange", psStats->base.shortRange, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			weap.setProperty("MinRange", psStats->base.minRange, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			weap.setProperty("HitChance", psStats->base.hitChance, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			weap.setProperty("ShortHitChance", psStats->base.shortHitChance, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			weap.setProperty("FirePause", psStats->base.firePause, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			weap.setProperty("ReloadTime", psStats->base.reloadTime, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			weap.setProperty("Rounds", psStats->base.numRounds, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			weap.setProperty("Damage", psStats->base.damage, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			weap.setProperty("MinimumDamage", psStats->base.minimumDamage, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			weap.setProperty("RadiusDamage", psStats->base.radiusDamage, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			weap.setProperty("RepeatDamage", psStats->base.periodicalDamage, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			weap.setProperty("RepeatRadius", psStats->base.periodicalDamageRadius, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			weap.setProperty("RepeatTime", psStats->base.periodicalDamageTime, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			weap.setProperty("Radius", psStats->base.radius, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			weap.setProperty("ImpactType", psStats->weaponClass == WC_KINETIC ? "KINETIC" : "HEAT",
			                 QScriptValue::ReadOnly | QScriptValue::Undeletable);
			weap.setProperty("RepeatType", psStats->periodicalDamageWeaponClass == WC_KINETIC ? "KINETIC" : "HEAT",
			                 QScriptValue::ReadOnly | QScriptValue::Undeletable);
			weap.setProperty("ImpactClass", getWeaponSubClass(psStats->weaponSubClass),
			                 QScriptValue::ReadOnly | QScriptValue::Undeletable);
			weap.setProperty("RepeatClass", getWeaponSubClass(psStats->periodicalDamageWeaponSubClass),
			                 QScriptValue::ReadOnly | QScriptValue::Undeletable);
			weap.setProperty("FireOnMove", psStats->fireOnMove, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			wbase.setProperty(QString::fromUtf8(psStats->name.toUtf8().c_str()), weap, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		}
		stats.setProperty("Weapon", wbase, QScriptValue::ReadOnly | QScriptValue::Undeletable);

		//==   * ```WeaponClass``` Defined weapon classes
		QScriptValue weaptypes = engine->newArray(WSC_NUM_WEAPON_SUBCLASSES);
		for (int j = 0; j < WSC_NUM_WEAPON_SUBCLASSES; j++)
		{
			weaptypes.setProperty(j, getWeaponSubClass((WEAPON_SUBCLASS)j), QScriptValue::ReadOnly | QScriptValue::Undeletable);
		}
		stats.setProperty("WeaponClass", weaptypes, QScriptValue::ReadOnly | QScriptValue::Undeletable);

		//==   * ```Building``` Buildings
		QScriptValue structbase = engine->newObject();
		for (int j = 0; j < numStructureStats; j++)
		{
			STRUCTURE_STATS *psStats = asStructureStats + j;
			QScriptValue strct = engine->newObject();
			strct.setProperty("Id", WzStringToQScriptValue(engine, psStats->id), QScriptValue::ReadOnly | QScriptValue::Undeletable);
			if (psStats->type == REF_DEFENSE || psStats->type == REF_WALL || psStats->type == REF_WALLCORNER
			    || psStats->type == REF_GENERIC || psStats->type == REF_GATE)
			{
				strct.setProperty("Type", "Wall", QScriptValue::ReadOnly | QScriptValue::Undeletable);
			}
			else if (psStats->type != REF_DEMOLISH)
			{
				strct.setProperty("Type", "Structure", QScriptValue::ReadOnly | QScriptValue::Undeletable);
			}
			else
			{
				strct.setProperty("Type", "Demolish", QScriptValue::ReadOnly | QScriptValue::Undeletable);
			}
			strct.setProperty("ResearchPoints", psStats->base.research, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			strct.setProperty("RepairPoints", psStats->base.repair, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			strct.setProperty("PowerPoints", psStats->base.power, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			strct.setProperty("ProductionPoints", psStats->base.production, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			strct.setProperty("RearmPoints", psStats->base.rearm, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			strct.setProperty("Armour", psStats->base.armour, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			strct.setProperty("Thermal", psStats->base.thermal, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			strct.setProperty("HitPoints", psStats->base.hitpoints, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			strct.setProperty("Resistance", psStats->base.resistance, QScriptValue::ReadOnly | QScriptValue::Undeletable);
			structbase.setProperty(QString::fromUtf8(psStats->name.toUtf8().c_str()), strct, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		}
		stats.setProperty("Building", structbase, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	}
	engine->globalObject().setProperty("Stats", stats, QScriptValue::ReadOnly | QScriptValue::Undeletable);

	//== * ```Upgrades``` A special array containing per-player rules information for game entity types,
	//== which can be written to in order to implement upgrades and other dynamic rules changes. Each item in the
	//== array contains a subset of the sparse array of rules information in the ```Stats``` global.
	//== These values are defined:
	QScriptValue upgrades = engine->newArray(MAX_PLAYERS);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		QScriptValue node = engine->newObject();

		//==   * ```Body``` Droid bodies
		QScriptValue bodybase = engine->newObject();
		for (int j = 0; j < numBodyStats; j++)
		{
			BODY_STATS *psStats = asBodyStats + j;
			QScriptValue body = engine->newObject();
			setStatsFunc(body, engine, "HitPoints", i, COMP_BODY, j);
			setStatsFunc(body, engine, "HitPointPct", i, COMP_BODY, j);
			setStatsFunc(body, engine, "Power", i, COMP_BODY, j);
			setStatsFunc(body, engine, "Armour", i, COMP_BODY, j);
			setStatsFunc(body, engine, "Thermal", i, COMP_BODY, j);
			setStatsFunc(body, engine, "Resistance", i, COMP_BODY, j);
			bodybase.setProperty(QString::fromUtf8(psStats->name.toUtf8().c_str()), body, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		}
		node.setProperty("Body", bodybase, QScriptValue::ReadOnly | QScriptValue::Undeletable);

		//==   * ```Sensor``` Sensor turrets
		QScriptValue sensorbase = engine->newObject();
		for (int j = 0; j < numSensorStats; j++)
		{
			SENSOR_STATS *psStats = asSensorStats + j;
			QScriptValue sensor = engine->newObject();
			setStatsFunc(sensor, engine, "HitPoints", i, COMP_SENSOR, j);
			setStatsFunc(sensor, engine, "HitPointPct", i, COMP_SENSOR, j);
			setStatsFunc(sensor, engine, "Range", i, COMP_SENSOR, j);
			sensorbase.setProperty(QString::fromUtf8(psStats->name.toUtf8().c_str()), sensor, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		}
		node.setProperty("Sensor", sensorbase, QScriptValue::ReadOnly | QScriptValue::Undeletable);

		//==   * ```Propulsion``` Propulsions
		QScriptValue propbase = engine->newObject();
		for (int j = 0; j < numPropulsionStats; j++)
		{
			PROPULSION_STATS *psStats = asPropulsionStats + j;
			QScriptValue v = engine->newObject();
			setStatsFunc(v, engine, "HitPoints", i, COMP_PROPULSION, j);
			setStatsFunc(v, engine, "HitPointPct", i, COMP_PROPULSION, j);
			setStatsFunc(v, engine, "HitPointPctOfBody", i, COMP_PROPULSION, j);
			propbase.setProperty(QString::fromUtf8(psStats->name.toUtf8().c_str()), v, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		}
		node.setProperty("Propulsion", propbase, QScriptValue::ReadOnly | QScriptValue::Undeletable);

		//==   * ```ECM``` ECM (Electronic Counter-Measure) turrets
		QScriptValue ecmbase = engine->newObject();
		for (int j = 0; j < numECMStats; j++)
		{
			ECM_STATS *psStats = asECMStats + j;
			QScriptValue ecm = engine->newObject();
			setStatsFunc(ecm, engine, "Range", i, COMP_ECM, j);
			setStatsFunc(ecm, engine, "HitPoints", i, COMP_ECM, j);
			setStatsFunc(ecm, engine, "HitPointPct", i, COMP_ECM, j);
			ecmbase.setProperty(QString::fromUtf8(psStats->name.toUtf8().c_str()), ecm, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		}
		node.setProperty("ECM", ecmbase, QScriptValue::ReadOnly | QScriptValue::Undeletable);

		//==   * ```Repair``` Repair turrets (not used, incidentally, for repair centers)
		QScriptValue repairbase = engine->newObject();
		for (int j = 0; j < numRepairStats; j++)
		{
			REPAIR_STATS *psStats = asRepairStats + j;
			QScriptValue repair = engine->newObject();
			setStatsFunc(repair, engine, "RepairPoints", i, COMP_REPAIRUNIT, j);
			setStatsFunc(repair, engine, "HitPoints", i, COMP_REPAIRUNIT, j);
			setStatsFunc(repair, engine, "HitPointPct", i, COMP_REPAIRUNIT, j);
			repairbase.setProperty(QString::fromUtf8(psStats->name.toUtf8().c_str()), repair, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		}
		node.setProperty("Repair", repairbase, QScriptValue::ReadOnly | QScriptValue::Undeletable);

		//==   * ```Construct``` Constructor turrets (eg for trucks)
		QScriptValue conbase = engine->newObject();
		for (int j = 0; j < numConstructStats; j++)
		{
			CONSTRUCT_STATS *psStats = asConstructStats + j;
			QScriptValue con = engine->newObject();
			setStatsFunc(con, engine, "ConstructorPoints", i, COMP_CONSTRUCT, j);
			setStatsFunc(con, engine, "HitPoints", i, COMP_CONSTRUCT, j);
			setStatsFunc(con, engine, "HitPointPct", i, COMP_CONSTRUCT, j);
			conbase.setProperty(QString::fromUtf8(psStats->name.toUtf8().c_str()), con, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		}
		node.setProperty("Construct", conbase, QScriptValue::ReadOnly | QScriptValue::Undeletable);

		//==   * ```Brain``` Brains
		//== BaseCommandLimit: How many droids a commander can command. CommandLimitByLevel: How many extra droids
		//== a commander can command for each of its rank levels. RankThresholds: An array describing how many
		//== kills are required for this brain to level up to the next rank. To alter this from scripts, you must
		//== set the entire array at once. Setting each item in the array will not work at the moment.
		QScriptValue brainbase = engine->newObject();
		for (int j = 0; j < numBrainStats; j++)
		{
			BRAIN_STATS *psStats = asBrainStats + j;
			QScriptValue br = engine->newObject();
			setStatsFunc(br, engine, "BaseCommandLimit", i, COMP_BRAIN, j);
			setStatsFunc(br, engine, "CommandLimitByLevel", i, COMP_BRAIN, j);
			setStatsFunc(br, engine, "RankThresholds", i, COMP_BRAIN, j);
			setStatsFunc(br, engine, "HitPoints", i, COMP_BRAIN, j);
			setStatsFunc(br, engine, "HitPointPct", i, COMP_BRAIN, j);
			brainbase.setProperty(QString::fromUtf8(psStats->name.toUtf8().c_str()), br, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		}
		node.setProperty("Brain", brainbase, QScriptValue::ReadOnly | QScriptValue::Undeletable);

		//==   * ```Weapon``` Weapon turrets
		QScriptValue wbase = engine->newObject();
		for (int j = 0; j < numWeaponStats; j++)
		{
			WEAPON_STATS *psStats = asWeaponStats + j;
			QScriptValue weap = engine->newObject();
			setStatsFunc(weap, engine, "MaxRange", i, COMP_WEAPON, j);
			setStatsFunc(weap, engine, "ShortRange", i, COMP_WEAPON, j);
			setStatsFunc(weap, engine, "MinRange", i, COMP_WEAPON, j);
			setStatsFunc(weap, engine, "HitChance", i, COMP_WEAPON, j);
			setStatsFunc(weap, engine, "ShortHitChance", i, COMP_WEAPON, j);
			setStatsFunc(weap, engine, "FirePause", i, COMP_WEAPON, j);
			setStatsFunc(weap, engine, "ReloadTime", i, COMP_WEAPON, j);
			setStatsFunc(weap, engine, "Rounds", i, COMP_WEAPON, j);
			setStatsFunc(weap, engine, "Radius", i, COMP_WEAPON, j);
			setStatsFunc(weap, engine, "Damage", i, COMP_WEAPON, j);
			setStatsFunc(weap, engine, "MinimumDamage", i, COMP_WEAPON, j);
			setStatsFunc(weap, engine, "RadiusDamage", i, COMP_WEAPON, j);
			setStatsFunc(weap, engine, "RepeatDamage", i, COMP_WEAPON, j);
			setStatsFunc(weap, engine, "RepeatTime", i, COMP_WEAPON, j);
			setStatsFunc(weap, engine, "RepeatRadius", i, COMP_WEAPON, j);
			setStatsFunc(weap, engine, "HitPoints", i, COMP_WEAPON, j);
			setStatsFunc(weap, engine, "HitPointPct", i, COMP_WEAPON, j);
			wbase.setProperty(QString::fromUtf8(psStats->name.toUtf8().c_str()), weap, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		}
		node.setProperty("Weapon", wbase, QScriptValue::ReadOnly | QScriptValue::Undeletable);

		//==   * ```Building``` Buildings
		QScriptValue structbase = engine->newObject();
		for (int j = 0; j < numStructureStats; j++)
		{
			STRUCTURE_STATS *psStats = asStructureStats + j;
			QScriptValue strct = engine->newObject();
			setStatsFunc(strct, engine, "ResearchPoints", i, SCRCB_RES, j);
			setStatsFunc(strct, engine, "ModuleResearchPoints", i, SCRCB_MODULE_RES, j);
			setStatsFunc(strct, engine, "RepairPoints", i, SCRCB_REP, j);
			setStatsFunc(strct, engine, "PowerPoints", i, SCRCB_POW, j);
			setStatsFunc(strct, engine, "ModulePowerPoints", i, SCRCB_MODULE_POW, j);
			setStatsFunc(strct, engine, "ProductionPoints", i, SCRCB_CON, j);
			setStatsFunc(strct, engine, "ModuleProductionPoints", i, SCRCB_MODULE_CON, j);
			setStatsFunc(strct, engine, "RearmPoints", i, SCRCB_REA, j);
			setStatsFunc(strct, engine, "Armour", i, SCRCB_ARM, j);
			setStatsFunc(strct, engine, "Resistance", i, SCRCB_ELW, j);
			setStatsFunc(strct, engine, "Thermal", i, SCRCB_HEA, j);
			setStatsFunc(strct, engine, "HitPoints", i, SCRCB_HIT, j);
			setStatsFunc(strct, engine, "Limit", i, SCRCB_LIMIT, j);
			structbase.setProperty(QString::fromUtf8(psStats->name.toUtf8().c_str()), strct, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		}
		node.setProperty("Building", structbase, QScriptValue::ReadOnly | QScriptValue::Undeletable);

		// Finally
		upgrades.setProperty(i, node, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	}
	engine->globalObject().setProperty("Upgrades", upgrades, QScriptValue::ReadOnly | QScriptValue::Undeletable);

	// Register functions to the script engine here
	engine->globalObject().setProperty("_", engine->newFunction(js_translate)); // WZAPI
	engine->globalObject().setProperty("dump", engine->newFunction(js_dump));
	engine->globalObject().setProperty("syncRandom", engine->newFunction(js_syncRandom)); // WZAPI
	engine->globalObject().setProperty("label", engine->newFunction(js_getObject)); // deprecated
	engine->globalObject().setProperty("getObject", engine->newFunction(js_getObject));
	engine->globalObject().setProperty("addLabel", engine->newFunction(js_addLabel));
	engine->globalObject().setProperty("removeLabel", engine->newFunction(js_removeLabel));
	engine->globalObject().setProperty("getLabel", engine->newFunction(js_getLabel));
	engine->globalObject().setProperty("enumLabels", engine->newFunction(js_enumLabels));
	engine->globalObject().setProperty("enumGateways", engine->newFunction(js_enumGateways));
	engine->globalObject().setProperty("enumTemplates", engine->newFunction(js_enumTemplates));
	engine->globalObject().setProperty("makeTemplate", engine->newFunction(js_makeTemplate)); // WZAPI
	engine->globalObject().setProperty("setAlliance", engine->newFunction(js_setAlliance)); // WZAPI
	engine->globalObject().setProperty("sendAllianceRequest", engine->newFunction(js_sendAllianceRequest)); // WZAPI
	engine->globalObject().setProperty("setAssemblyPoint", engine->newFunction(js_setAssemblyPoint)); // WZAPI
	engine->globalObject().setProperty("setSunPosition", engine->newFunction(js_setSunPosition)); // WZAPI
	engine->globalObject().setProperty("setSunIntensity", engine->newFunction(js_setSunIntensity)); // WZAPI
	engine->globalObject().setProperty("setWeather", engine->newFunction(js_setWeather)); // WZAPI
	engine->globalObject().setProperty("setSky", engine->newFunction(js_setSky)); // WZAPI
	engine->globalObject().setProperty("cameraSlide", engine->newFunction(js_cameraSlide)); // WZAPI
	engine->globalObject().setProperty("cameraTrack", engine->newFunction(js_cameraTrack)); // WZAPI
	engine->globalObject().setProperty("cameraZoom", engine->newFunction(js_cameraZoom)); // WZAPI
	engine->globalObject().setProperty("resetArea", engine->newFunction(js_resetLabel)); // deprecated
	engine->globalObject().setProperty("resetLabel", engine->newFunction(js_resetLabel));
	engine->globalObject().setProperty("addSpotter", engine->newFunction(js_addSpotter)); // WZAPI
	engine->globalObject().setProperty("removeSpotter", engine->newFunction(js_removeSpotter)); // WZAPI
	engine->globalObject().setProperty("syncRequest", engine->newFunction(js_syncRequest)); // WZAPI
	engine->globalObject().setProperty("replaceTexture", engine->newFunction(js_replaceTexture)); // WZAPI
	engine->globalObject().setProperty("changePlayerColour", engine->newFunction(js_changePlayerColour)); // WZAPI
	engine->globalObject().setProperty("setHealth", engine->newFunction(js_setHealth)); // WZAPI
	engine->globalObject().setProperty("useSafetyTransport", engine->newFunction(js_useSafetyTransport)); // WZAPI
	engine->globalObject().setProperty("restoreLimboMissionData", engine->newFunction(js_restoreLimboMissionData)); // WZAPI
	engine->globalObject().setProperty("getMultiTechLevel", engine->newFunction(js_getMultiTechLevel)); // WZAPI
	engine->globalObject().setProperty("setCampaignNumber", engine->newFunction(js_setCampaignNumber)); // WZAPI
	engine->globalObject().setProperty("getMissionType", engine->newFunction(js_getMissionType));
	engine->globalObject().setProperty("getRevealStatus", engine->newFunction(js_getRevealStatus));
	engine->globalObject().setProperty("setRevealStatus", engine->newFunction(js_setRevealStatus)); // WZAPI
	engine->globalObject().setProperty("autoSave", engine->newFunction(js_autoSave)); // WZAPI

	// horrible hacks follow -- do not rely on these being present!
	engine->globalObject().setProperty("hackNetOff", engine->newFunction(js_hackNetOff)); // WZAPI
	engine->globalObject().setProperty("hackNetOn", engine->newFunction(js_hackNetOn)); // WZAPI
	engine->globalObject().setProperty("hackAddMessage", engine->newFunction(js_hackAddMessage)); // WZAPI
	engine->globalObject().setProperty("hackRemoveMessage", engine->newFunction(js_hackRemoveMessage)); // WZAPI
	engine->globalObject().setProperty("objFromId", engine->newFunction(js_objFromId));
	engine->globalObject().setProperty("hackGetObj", engine->newFunction(js_hackGetObj)); // WZAPI
	engine->globalObject().setProperty("hackChangeMe", engine->newFunction(js_hackChangeMe)); // WZAPI
	engine->globalObject().setProperty("hackAssert", engine->newFunction(js_hackAssert)); // WZAPI
	engine->globalObject().setProperty("hackMarkTiles", engine->newFunction(js_hackMarkTiles));
	engine->globalObject().setProperty("receiveAllEvents", engine->newFunction(js_receiveAllEvents)); // WZAPI
	engine->globalObject().setProperty("hackDoNotSave", engine->newFunction(js_hackDoNotSave)); // WZAPI
	engine->globalObject().setProperty("hackPlayIngameAudio", engine->newFunction(js_hackPlayIngameAudio)); // WZAPI
	engine->globalObject().setProperty("hackStopIngameAudio", engine->newFunction(js_hackStopIngameAudio)); // WZAPI

	// General functions -- geared for use in AI scripts
	engine->globalObject().setProperty("debug", engine->newFunction(js_debug));
	engine->globalObject().setProperty("console", engine->newFunction(js_console)); // WZAPI
	engine->globalObject().setProperty("clearConsole", engine->newFunction(js_clearConsole)); // WZAPI
	engine->globalObject().setProperty("structureIdle", engine->newFunction(js_structureIdle)); // WZAPI
	engine->globalObject().setProperty("enumStruct", engine->newFunction(js_enumStruct)); // WZAPI
	engine->globalObject().setProperty("enumStructOffWorld", engine->newFunction(js_enumStructOffWorld)); // WZAPI
	engine->globalObject().setProperty("enumDroid", engine->newFunction(js_enumDroid)); // WZAPI
	engine->globalObject().setProperty("enumGroup", engine->newFunction(js_enumGroup));
	engine->globalObject().setProperty("enumFeature", engine->newFunction(js_enumFeature)); // WZAPI
	engine->globalObject().setProperty("enumBlips", engine->newFunction(js_enumBlips)); // WZAPI
	engine->globalObject().setProperty("enumSelected", engine->newFunction(js_enumSelected)); // WZAPI
	engine->globalObject().setProperty("enumResearch", engine->newFunction(js_enumResearch)); // WZAPI
	engine->globalObject().setProperty("enumRange", engine->newFunction(js_enumRange)); // WZAPI
	engine->globalObject().setProperty("enumArea", engine->newFunction(js_enumArea));
	engine->globalObject().setProperty("getResearch", engine->newFunction(js_getResearch)); // WZAPI
	engine->globalObject().setProperty("pursueResearch", engine->newFunction(js_pursueResearch)); // WZAPI
	engine->globalObject().setProperty("findResearch", engine->newFunction(js_findResearch)); // WZAPI
	engine->globalObject().setProperty("distBetweenTwoPoints", engine->newFunction(js_distBetweenTwoPoints)); // WZAPI
	engine->globalObject().setProperty("newGroup", engine->newFunction(js_newGroup));
	engine->globalObject().setProperty("groupAddArea", engine->newFunction(js_groupAddArea));
	engine->globalObject().setProperty("groupAddDroid", engine->newFunction(js_groupAddDroid));
	engine->globalObject().setProperty("groupAdd", engine->newFunction(js_groupAdd));
	engine->globalObject().setProperty("groupSize", engine->newFunction(js_groupSize));
	engine->globalObject().setProperty("orderDroidLoc", engine->newFunction(js_orderDroidLoc)); // WZAPI
	engine->globalObject().setProperty("playerPower", engine->newFunction(js_playerPower)); // WZAPI
	engine->globalObject().setProperty("queuedPower", engine->newFunction(js_queuedPower)); // WZAPI
	engine->globalObject().setProperty("isStructureAvailable", engine->newFunction(js_isStructureAvailable)); // WZAPI
	engine->globalObject().setProperty("pickStructLocation", engine->newFunction(js_pickStructLocation)); // WZAPI
	engine->globalObject().setProperty("droidCanReach", engine->newFunction(js_droidCanReach)); // WZAPI
	engine->globalObject().setProperty("propulsionCanReach", engine->newFunction(js_propulsionCanReach)); // WZAPI
	engine->globalObject().setProperty("terrainType", engine->newFunction(js_terrainType)); // WZAPI
	engine->globalObject().setProperty("orderDroidBuild", engine->newFunction(js_orderDroidBuild)); // WZAPI
	engine->globalObject().setProperty("orderDroidObj", engine->newFunction(js_orderDroidObj)); // WZAPI
	engine->globalObject().setProperty("orderDroid", engine->newFunction(js_orderDroid)); // WZAPI
	engine->globalObject().setProperty("buildDroid", engine->newFunction(js_buildDroid)); // WZAPI
	engine->globalObject().setProperty("addDroid", engine->newFunction(js_addDroid)); // WZAPI
	engine->globalObject().setProperty("addDroidToTransporter", engine->newFunction(js_addDroidToTransporter)); // WZAPI
	engine->globalObject().setProperty("addFeature", engine->newFunction(js_addFeature)); // WZAPI
	engine->globalObject().setProperty("componentAvailable", engine->newFunction(js_componentAvailable)); // WZAPI
	engine->globalObject().setProperty("isVTOL", engine->newFunction(js_isVTOL)); // WZAPI
	engine->globalObject().setProperty("safeDest", engine->newFunction(js_safeDest)); // WZAPI
	engine->globalObject().setProperty("activateStructure", engine->newFunction(js_activateStructure)); // WZAPI
	engine->globalObject().setProperty("chat", engine->newFunction(js_chat)); // WZAPI
	engine->globalObject().setProperty("addBeacon", engine->newFunction(js_addBeacon)); // WZAPI
	engine->globalObject().setProperty("removeBeacon", engine->newFunction(js_removeBeacon)); // WZAPI
	engine->globalObject().setProperty("getDroidProduction", engine->newFunction(js_getDroidProduction)); // WZAPI
	engine->globalObject().setProperty("getDroidLimit", engine->newFunction(js_getDroidLimit)); // WZAPI
	engine->globalObject().setProperty("getExperienceModifier", engine->newFunction(js_getExperienceModifier)); // WZAPI
	engine->globalObject().setProperty("setDroidLimit", engine->newFunction(js_setDroidLimit)); // WZAPI
	engine->globalObject().setProperty("setCommanderLimit", engine->newFunction(js_setCommanderLimit));
	engine->globalObject().setProperty("setConstructorLimit", engine->newFunction(js_setConstructorLimit));
	engine->globalObject().setProperty("setExperienceModifier", engine->newFunction(js_setExperienceModifier)); // WZAPI
	engine->globalObject().setProperty("getWeaponInfo", engine->newFunction(js_getWeaponInfo));
	engine->globalObject().setProperty("enumCargo", engine->newFunction(js_enumCargo)); // WZAPI

	// Functions that operate on the current player only
	engine->globalObject().setProperty("centreView", engine->newFunction(js_centreView)); // WZAPI
	engine->globalObject().setProperty("playSound", engine->newFunction(js_playSound)); // WZAPI
	engine->globalObject().setProperty("gameOverMessage", engine->newFunction(js_gameOverMessage)); // WZAPI

	// Global state manipulation -- not for use with skirmish AI (unless you want it to cheat, obviously)
	engine->globalObject().setProperty("setStructureLimits", engine->newFunction(js_setStructureLimits)); // WZAPI
	engine->globalObject().setProperty("applyLimitSet", engine->newFunction(js_applyLimitSet)); // WZAPI
	engine->globalObject().setProperty("setMissionTime", engine->newFunction(js_setMissionTime)); // WZAPI
	engine->globalObject().setProperty("getMissionTime", engine->newFunction(js_getMissionTime)); // WZAPI
	engine->globalObject().setProperty("setReinforcementTime", engine->newFunction(js_setReinforcementTime)); // WZAPI
	engine->globalObject().setProperty("completeResearch", engine->newFunction(js_completeResearch)); // WZAPI
	engine->globalObject().setProperty("completeAllResearch", engine->newFunction(js_completeAllResearch)); // WZAPI
	engine->globalObject().setProperty("enableResearch", engine->newFunction(js_enableResearch)); // WZAPI
	engine->globalObject().setProperty("setPower", engine->newFunction(js_setPower)); // WZAPI
	engine->globalObject().setProperty("setPowerModifier", engine->newFunction(js_setPowerModifier)); // WZAPI
	engine->globalObject().setProperty("setPowerStorageMaximum", engine->newFunction(js_setPowerStorageMaximum)); // WZAPI
	engine->globalObject().setProperty("extraPowerTime", engine->newFunction(js_extraPowerTime)); // WZAPI
	engine->globalObject().setProperty("setTutorialMode", engine->newFunction(js_setTutorialMode)); // WZAPI
	engine->globalObject().setProperty("setDesign", engine->newFunction(js_setDesign)); // WZAPI
	engine->globalObject().setProperty("enableTemplate", engine->newFunction(js_enableTemplate)); // WZAPI
	engine->globalObject().setProperty("removeTemplate", engine->newFunction(js_removeTemplate)); // WZAPI
	engine->globalObject().setProperty("setMiniMap", engine->newFunction(js_setMiniMap)); // WZAPI
	engine->globalObject().setProperty("setReticuleButton", engine->newFunction(js_setReticuleButton)); // WZAPI
	engine->globalObject().setProperty("setReticuleFlash", engine->newFunction(js_setReticuleFlash)); // WZAPI
	engine->globalObject().setProperty("showReticuleWidget", engine->newFunction(js_showReticuleWidget)); // WZAPI
	engine->globalObject().setProperty("showInterface", engine->newFunction(js_showInterface)); // WZAPI
	engine->globalObject().setProperty("hideInterface", engine->newFunction(js_hideInterface)); // WZAPI
	engine->globalObject().setProperty("addReticuleButton", engine->newFunction(js_removeReticuleButton)); // deprecated!!
	engine->globalObject().setProperty("removeReticuleButton", engine->newFunction(js_removeReticuleButton)); // deprecated!!
	engine->globalObject().setProperty("enableStructure", engine->newFunction(js_enableStructure)); // WZAPI
	engine->globalObject().setProperty("makeComponentAvailable", engine->newFunction(js_makeComponentAvailable)); // WZAPI
	engine->globalObject().setProperty("enableComponent", engine->newFunction(js_enableComponent)); // WZAPI
	engine->globalObject().setProperty("allianceExistsBetween", engine->newFunction(js_allianceExistsBetween)); // WZAPI
	engine->globalObject().setProperty("removeStruct", engine->newFunction(js_removeStruct)); // WZAPI // deprecated!!
	engine->globalObject().setProperty("removeObject", engine->newFunction(js_removeObject)); // WZAPI
	engine->globalObject().setProperty("setScrollParams", engine->newFunction(js_setScrollLimits)); // deprecated!!
	engine->globalObject().setProperty("setScrollLimits", engine->newFunction(js_setScrollLimits)); // WZAPI
	engine->globalObject().setProperty("getScrollLimits", engine->newFunction(js_getScrollLimits));
	engine->globalObject().setProperty("addStructure", engine->newFunction(js_addStructure)); // WZAPI
	engine->globalObject().setProperty("getStructureLimit", engine->newFunction(js_getStructureLimit)); // WZAPI
	engine->globalObject().setProperty("countStruct", engine->newFunction(js_countStruct)); // WZAPI
	engine->globalObject().setProperty("countDroid", engine->newFunction(js_countDroid)); // WZAPI
	engine->globalObject().setProperty("loadLevel", engine->newFunction(js_loadLevel)); // WZAPI
	engine->globalObject().setProperty("setDroidExperience", engine->newFunction(js_setDroidExperience)); // WZAPI
	engine->globalObject().setProperty("donateObject", engine->newFunction(js_donateObject)); // WZAPI
	engine->globalObject().setProperty("donatePower", engine->newFunction(js_donatePower)); // WZAPI
	engine->globalObject().setProperty("setNoGoArea", engine->newFunction(js_setNoGoArea)); // WZAPI
	engine->globalObject().setProperty("startTransporterEntry", engine->newFunction(js_startTransporterEntry)); // WZAPI
	engine->globalObject().setProperty("setTransporterExit", engine->newFunction(js_setTransporterExit)); // WZAPI
	engine->globalObject().setProperty("setObjectFlag", engine->newFunction(js_setObjectFlag)); // WZAPI
	engine->globalObject().setProperty("fireWeaponAtLoc", engine->newFunction(js_fireWeaponAtLoc)); // WZAPI
	engine->globalObject().setProperty("fireWeaponAtObj", engine->newFunction(js_fireWeaponAtObj)); // WZAPI

	// Set some useful constants
	engine->globalObject().setProperty("TER_WATER", TER_WATER, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("TER_CLIFFFACE", TER_CLIFFFACE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("WEATHER_CLEAR", WT_NONE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("WEATHER_RAIN", WT_RAINING, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("WEATHER_SNOW", WT_SNOWING, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_ATTACK", DORDER_ATTACK, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_OBSERVE", DORDER_OBSERVE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_RECOVER", DORDER_RECOVER, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_MOVE", DORDER_MOVE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_SCOUT", DORDER_SCOUT, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_BUILD", DORDER_BUILD, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_HELPBUILD", DORDER_HELPBUILD, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_LINEBUILD", DORDER_LINEBUILD, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_REPAIR", DORDER_REPAIR, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_PATROL", DORDER_PATROL, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_DEMOLISH", DORDER_DEMOLISH, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_EMBARK", DORDER_EMBARK, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_DISEMBARK", DORDER_DISEMBARK, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_FIRESUPPORT", DORDER_FIRESUPPORT, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_COMMANDERSUPPORT", DORDER_COMMANDERSUPPORT, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_HOLD", DORDER_HOLD, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_RTR", DORDER_RTR, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_RTB", DORDER_RTB, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_STOP", DORDER_STOP, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_REARM", DORDER_REARM, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_RECYCLE", DORDER_RECYCLE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("COMMAND", IDRET_COMMAND, QScriptValue::ReadOnly | QScriptValue::Undeletable); // deprecated
	engine->globalObject().setProperty("BUILD", IDRET_BUILD, QScriptValue::ReadOnly | QScriptValue::Undeletable); // deprecated
	engine->globalObject().setProperty("MANUFACTURE", IDRET_MANUFACTURE, QScriptValue::ReadOnly | QScriptValue::Undeletable); // deprecated
	engine->globalObject().setProperty("RESEARCH", IDRET_RESEARCH, QScriptValue::ReadOnly | QScriptValue::Undeletable); // deprecated
	engine->globalObject().setProperty("INTELMAP", IDRET_INTEL_MAP, QScriptValue::ReadOnly | QScriptValue::Undeletable); // deprecated
	engine->globalObject().setProperty("DESIGN", IDRET_DESIGN, QScriptValue::ReadOnly | QScriptValue::Undeletable); // deprecated
	engine->globalObject().setProperty("CANCEL", IDRET_CANCEL, QScriptValue::ReadOnly | QScriptValue::Undeletable); // deprecated
	engine->globalObject().setProperty("CAMP_CLEAN", CAMP_CLEAN, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("CAMP_BASE", CAMP_BASE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("CAMP_WALLS", CAMP_WALLS, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("NO_ALLIANCES", NO_ALLIANCES, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("ALLIANCES", ALLIANCES, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("ALLIANCES_TEAMS", ALLIANCES_TEAMS, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("ALLIANCES_UNSHARED", ALLIANCES_UNSHARED, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("BEING_BUILT", SS_BEING_BUILT, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("BUILT", SS_BUILT, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DROID_CONSTRUCT", DROID_CONSTRUCT, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DROID_WEAPON", DROID_WEAPON, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DROID_PERSON", DROID_PERSON, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DROID_REPAIR", DROID_REPAIR, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DROID_SENSOR", DROID_SENSOR, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DROID_ECM", DROID_ECM, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DROID_CYBORG", DROID_CYBORG, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DROID_TRANSPORTER", DROID_TRANSPORTER, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DROID_SUPERTRANSPORTER", DROID_SUPERTRANSPORTER, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DROID_COMMAND", DROID_COMMAND, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DROID_ANY", DROID_ANY, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("OIL_RESOURCE", FEAT_OIL_RESOURCE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("OIL_DRUM", FEAT_OIL_DRUM, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("ARTIFACT", FEAT_GEN_ARTE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("BUILDING", FEAT_BUILDING, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("HQ", REF_HQ, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("FACTORY", REF_FACTORY, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("POWER_GEN", REF_POWER_GEN, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("RESOURCE_EXTRACTOR", REF_RESOURCE_EXTRACTOR, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DEFENSE", REF_DEFENSE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("LASSAT", REF_LASSAT, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("WALL", REF_WALL, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("RESEARCH_LAB", REF_RESEARCH, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("REPAIR_FACILITY", REF_REPAIR_FACILITY, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("CYBORG_FACTORY", REF_CYBORG_FACTORY, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("VTOL_FACTORY", REF_VTOL_FACTORY, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("REARM_PAD", REF_REARM_PAD, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("SAT_UPLINK", REF_SAT_UPLINK, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("GATE", REF_GATE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("COMMAND_CONTROL", REF_COMMAND_CONTROL, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("EASY", static_cast<int8_t>(AIDifficulty::EASY), QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("MEDIUM", static_cast<int8_t>(AIDifficulty::MEDIUM), QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("HARD", static_cast<int8_t>(AIDifficulty::HARD), QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("INSANE", static_cast<int8_t>(AIDifficulty::INSANE), QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("STRUCTURE", OBJ_STRUCTURE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DROID", OBJ_DROID, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("FEATURE", OBJ_FEATURE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("ALL_PLAYERS", ALL_PLAYERS, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("ALLIES", ALLIES, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("ENEMIES", ENEMIES, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("POSITION", SCRIPT_POSITION, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("AREA", SCRIPT_AREA, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("RADIUS", SCRIPT_RADIUS, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("GROUP", SCRIPT_GROUP, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("PLAYER_DATA", SCRIPT_PLAYER, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("RESEARCH_DATA", SCRIPT_RESEARCH, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("LZ_COMPROMISED_TIME", JS_LZ_COMPROMISED_TIME, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("OBJECT_FLAG_UNSELECTABLE", OBJECT_FLAG_UNSELECTABLE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	// the constants below are subject to change without notice...
	engine->globalObject().setProperty("PROX_MSG", MSG_PROXIMITY, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("CAMP_MSG", MSG_CAMPAIGN, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("MISS_MSG", MSG_MISSION, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("RES_MSG", MSG_RESEARCH, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("LDS_EXPAND_LIMBO", static_cast<int8_t>(LEVEL_TYPE::LDS_EXPAND_LIMBO), QScriptValue::ReadOnly | QScriptValue::Undeletable);

	/// Place to store group sizes
	//== * ```groupSizes``` A sparse array of group sizes. If a group has never been used, the entry in this array will
	//== be undefined.
	engine->globalObject().setProperty("groupSizes", engine->newObject());

	// Static knowledge about players
	//== * ```playerData``` An array of information about the players in a game. Each item in the array is an object
	//== containing the following variables:
	//==   * ```difficulty``` (see ```difficulty``` global constant)
	//==   * ```colour``` number describing the colour of the player
	//==   * ```position``` number describing the position of the player in the game's setup screen
	//==   * ```isAI``` whether the player is an AI (3.2+ only)
	//==   * ```isHuman``` whether the player is human (3.2+ only)
	//==   * ```name``` the name of the player (3.2+ only)
	//==   * ```team``` the number of the team the player is part of
	QScriptValue playerData = engine->newArray(game.maxPlayers);
	for (int i = 0; i < game.maxPlayers; i++)
	{
		QScriptValue vector = engine->newObject();
		vector.setProperty("name", QString(NetPlay.players[i].name), QScriptValue::ReadOnly | QScriptValue::Undeletable);  // QString cast to work around bug in Qt5 QScriptValue(char *) constructor.
		vector.setProperty("difficulty", static_cast<int8_t>(NetPlay.players[i].difficulty), QScriptValue::ReadOnly | QScriptValue::Undeletable);
		vector.setProperty("colour", NetPlay.players[i].colour, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		vector.setProperty("position", NetPlay.players[i].position, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		vector.setProperty("team", NetPlay.players[i].team, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		vector.setProperty("isAI", !NetPlay.players[i].allocated && NetPlay.players[i].ai >= 0, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		vector.setProperty("isHuman", NetPlay.players[i].allocated, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		vector.setProperty("type", SCRIPT_PLAYER, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		playerData.setProperty(i, vector, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	}
	engine->globalObject().setProperty("playerData", playerData, QScriptValue::ReadOnly | QScriptValue::Undeletable);

	// Static map knowledge about start positions
	//== * ```derrickPositions``` An array of derrick starting positions on the current map. Each item in the array is an
	//== object containing the x and y variables for a derrick.
	//== * ```startPositions``` An array of player start positions on the current map. Each item in the array is an
	//== object containing the x and y variables for a player start position.
	QScriptValue startPositions = engine->newArray(game.maxPlayers);
	for (int i = 0; i < game.maxPlayers; i++)
	{
		QScriptValue vector = engine->newObject();
		vector.setProperty("x", map_coord(positions[i].x), QScriptValue::ReadOnly | QScriptValue::Undeletable);
		vector.setProperty("y", map_coord(positions[i].y), QScriptValue::ReadOnly | QScriptValue::Undeletable);
		vector.setProperty("type", SCRIPT_POSITION, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		startPositions.setProperty(i, vector, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	}
	QScriptValue derrickPositions = engine->newArray(derricks.size());
	for (int i = 0; i < derricks.size(); i++)
	{
		QScriptValue vector = engine->newObject();
		vector.setProperty("x", map_coord(derricks[i].x), QScriptValue::ReadOnly | QScriptValue::Undeletable);
		vector.setProperty("y", map_coord(derricks[i].y), QScriptValue::ReadOnly | QScriptValue::Undeletable);
		vector.setProperty("type", SCRIPT_POSITION, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		derrickPositions.setProperty(i, vector, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	}
	engine->globalObject().setProperty("derrickPositions", derrickPositions, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("startPositions", startPositions, QScriptValue::ReadOnly | QScriptValue::Undeletable);

	// Clear previous log file
	PHYSFS_delete(QString("logs/" + scriptName + ".log").toUtf8().constData());

	return true;
}
