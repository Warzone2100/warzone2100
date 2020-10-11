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

// additional structure check
static bool structDoubleCheck(BASE_STATS *psStat, UDWORD xx, UDWORD yy, SDWORD maxBlockingTiles)
{
	UDWORD		x, y, xTL, yTL, xBR, yBR;
	UBYTE		count = 0;
	STRUCTURE_STATS	*psBuilding = (STRUCTURE_STATS *)psStat;

	xTL = xx - 1;
	yTL = yy - 1;
	xBR = (xx + psBuilding->baseWidth);
	yBR = (yy + psBuilding->baseBreadth);

	// check against building in a gateway, as this can seriously block AI passages
	for (auto psGate : gwGetGateways())
	{
		for (x = xx; x <= xBR; x++)
		{
			for (y = yy; y <= yBR; y++)
			{
				if ((x >= psGate->x1 && x <= psGate->x2) && (y >= psGate->y1 && y <= psGate->y2))
				{
					return false;
				}
			}
		}
	}

	// can you get past it?
	y = yTL;	// top
	for (x = xTL; x != xBR + 1; x++)
	{
		if (fpathBlockingTile(x, y, PROPULSION_TYPE_WHEELED))
		{
			count++;
			break;
		}
	}

	y = yBR;	// bottom
	for (x = xTL; x != xBR + 1; x++)
	{
		if (fpathBlockingTile(x, y, PROPULSION_TYPE_WHEELED))
		{
			count++;
			break;
		}
	}

	x = xTL;	// left
	for (y = yTL + 1; y != yBR; y++)
	{
		if (fpathBlockingTile(x, y, PROPULSION_TYPE_WHEELED))
		{
			count++;
			break;
		}
	}

	x = xBR;	// right
	for (y = yTL + 1; y != yBR; y++)
	{
		if (fpathBlockingTile(x, y, PROPULSION_TYPE_WHEELED))
		{
			count++;
			break;
		}
	}

	//make sure this location is not blocked from too many sides
	if ((count <= maxBlockingTiles) || (maxBlockingTiles == -1))
	{
		return true;
	}
	return false;
}

// private qtscript bureaucracy
typedef QMap<BASE_OBJECT *, int> GROUPMAP;
typedef QMap<QScriptEngine *, GROUPMAP *> ENGINEMAP;
static ENGINEMAP groups;

struct LABEL
{
	Vector2i p1, p2;
	int id;
	int type;
	int player;
	int subscriber;
	QList<int> idlist;
	int triggered;

	bool operator==(const LABEL &o) const
	{
		return id == o.id && type == o.type && player == o.player;
	}
};
typedef QMap<QString, LABEL> LABELMAP;
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
		const LABEL &l = i.value();
		labelModel->setItem(nextRow, 0, new QStandardItem(i.key()));
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
	for (const auto &key : labels.keys())
	{
		const LABEL &l = labels[key];
		if (!only_active || l.triggered <= 0)
		{
			showLabel(key, false, false);
		}
	}
}

void showLabel(const QString &key, bool clear_old, bool jump_to)
{
	if (!labels.contains(key))
	{
		debug(LOG_ERROR, "label %s not found", key.toUtf8().constData());
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
			for (GROUPMAP::const_iterator iter = i.value()->constBegin(); iter != i.value()->constEnd(); ++iter)
			{
				if (iter.value() == l.id)
				{
					BASE_OBJECT *psObj = iter.key();
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
	GROUPMAP *psMap = groups.value(engine);
	ASSERT_OR_RETURN(std::make_pair(false, 0), psMap != nullptr, "Non-existent groupmap for engine");
	int groupId = psMap->value(seen);
	bool foundObj = false, foundGroup = false;
	for (auto &l : labels)
	{
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
		LABEL &l = i.value();
		if (l.triggered == 0 && (l.subscriber == ALL_PLAYERS || l.subscriber == psDroid->player)
		    && ((l.type == SCRIPT_AREA && l.p1.x < x && l.p1.y < y && l.p2.x > x && l.p2.y > y)
		        || (l.type == SCRIPT_RADIUS && iHypot(l.p1 - psDroid->pos.xy()) < l.p2.x)))
		{
			// We're inside an untriggered area
			activated = true;
			l.triggered = psDroid->id;
			triggerEventArea(i.key(), psDroid);
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
	if (psMap->contains(psObj))
	{
		int groupId = psMap->take(psObj); // take and remove item
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
		removeFromGroup(i.key(), i.value(), psObj);
	}
}

static bool groupAddObject(BASE_OBJECT *psObj, int groupId, QScriptEngine *engine)
{
	ASSERT_OR_RETURN(false, psObj && engine, "Bad parameter");
	GROUPMAP *psMap = groups.value(engine);
	removeFromGroup(engine, psMap, psObj);
	QScriptValue groupMembers = engine->globalObject().property("groupSizes");
	int prev = groupMembers.property(QString::number(groupId)).toInt32();
	groupMembers.setProperty(QString::number(groupId), prev + 1, QScriptValue::ReadOnly);
	psMap->insert(psObj, groupId);
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
QScriptValue convStructure(STRUCTURE *psStruct, QScriptEngine *engine)
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
QScriptValue convFeature(FEATURE *psFeature, QScriptEngine *engine)
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
QScriptValue convDroid(DROID *psDroid, QScriptEngine *engine)
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
QScriptValue convObj(BASE_OBJECT *psObj, QScriptEngine *engine)
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
	GROUPMAP *psMap = groups.value(engine);
	if (psMap != nullptr && psMap->contains(psObj))
	{
		int group = psMap->value(psObj);
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
QScriptValue convTemplate(DROID_TEMPLATE *psTempl, QScriptEngine *engine)
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

QScriptValue convMax(BASE_OBJECT *psObj, QScriptEngine *engine)
{
	if (!psObj)
	{
		return QScriptValue::NullValue;
	}
	switch (psObj->type)
	{
	case OBJ_DROID: return convDroid((DROID *)psObj, engine);
	case OBJ_STRUCTURE: return convStructure((STRUCTURE *)psObj, engine);
	case OBJ_FEATURE: return convFeature((FEATURE *)psObj, engine);
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
	GROUPMAP *psMap = groups.value(engine);
	ASSERT_OR_RETURN(false, psMap, "Non-existent groupmap for engine");
	for (GROUPMAP::const_iterator i = psMap->constBegin(); i != psMap->constEnd(); ++i)
	{
		std::vector<WzString> value;
		BASE_OBJECT *psObj = i.key();
		ASSERT(!isDead(psObj), "Wanted to save dead %s to savegame!", objInfo(psObj));
		if (ini.contains(WzString::number(psObj->id)))
		{
			value.push_back(ini.value(WzString::number(psObj->id)).toWzString());
		}
		value.push_back(WzString::number(i.value()));
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
		QString label(QString::fromUtf8(ini.value("label").toWzString().toUtf8().c_str()));
		if (labels.contains(label))
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
			labels.insert(label, p);
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
			labels.insert(label, p);
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
			labels.insert(label, p);
			p.triggered = ini.value("triggered", -1).toInt(); // deactivated by default
		}
		else if (list[i].startsWith("object"))
		{
			p.id = ini.value("id").toInt();
			p.type = ini.value("type").toInt();
			p.player = ini.value("player").toInt();
			labels.insert(label, p);
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
				p.idlist += id;
			}
			labels.insert(label, p);
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
	for (LABELMAP::const_iterator i = labels.constBegin(); i != labels.constEnd(); i++)
	{
		const QString& key = i.key();
		LABEL l = i.value();
		if (l.type == SCRIPT_POSITION)
		{
			ini.beginGroup("position_" + WzString::number(c[0]++));
			ini.setVector2i("pos", l.p1);
			ini.setValue("label", QStringToWzString(key));
			ini.setValue("triggered", l.triggered);
			ini.endGroup();
		}
		else if (l.type == SCRIPT_AREA)
		{
			ini.beginGroup("area_" + WzString::number(c[1]++));
			ini.setVector2i("pos1", l.p1);
			ini.setVector2i("pos2", l.p2);
			ini.setValue("label", QStringToWzString(key));
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
			ini.setValue("label", QStringToWzString(key));
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
			ini.setValue("label", QStringToWzString(key));
			ini.setValue("subscriber", l.subscriber);
			ini.endGroup();
		}
		else
		{
			ini.beginGroup("object_" + WzString::number(c[4]++));
			ini.setValue("id", l.id);
			ini.setValue("player", l.player);
			ini.setValue("type", l.type);
			ini.setValue("label", QStringToWzString(key));
			ini.setValue("triggered", l.triggered);
			ini.endGroup();
		}
	}
	return true;
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
	QString labelName = context->argument(0).toString();
	SCRIPT_ASSERT(context, labels.contains(labelName), "Label %s not found", labelName.toUtf8().constData());
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
	QStringList matches;
	if (context->argumentCount() > 0) // filter
	{
		SCRIPT_TYPE type = (SCRIPT_TYPE)context->argument(0).toInt32();
		for (LABELMAP::iterator i = labels.begin(); i != labels.end(); i++)
		{
			LABEL &l = (*i);
			if (l.type == type)
			{
				matches += i.key();
			}
		}
	}
	else // fast path, give all
	{
		matches = labels.keys();
	}
	QScriptValue result = engine->newArray(matches.size());
	for (int i = 0; i < matches.size(); i++)
	{
		result.setProperty(i, QScriptValue(matches[i]), QScriptValue::ReadOnly);
	}
	return result;
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
	QString key = context->argument(1).toString();
	labels.insert(key, value);
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
	QString key = context->argument(0).toString();
	int result = labels.remove(key);
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
	QString label = labels.key(value, QString());
	if (!label.isEmpty())
	{
		return QScriptValue(label);
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
	QString label = context->argument(0).toString();
	QScriptValue ret;
	if (labels.contains(label))
	{
		ret = engine->newObject();
		LABEL p = labels.value(label);
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
			SCRIPT_ASSERT(context, false, "Bad object label type found for label %s!", label.toUtf8().constData());
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
	QList<Position> matches;
	int player = context->argument(0).toInt32();
	SCRIPT_ASSERT_PLAYER(context, player);
	for (BASE_OBJECT *psSensor = apsSensorList[0]; psSensor; psSensor = psSensor->psNextFunc)
	{
		if (psSensor->visible[player] > 0 && psSensor->visible[player] < UBYTE_MAX)
		{
			matches.push_back(psSensor->pos);
		}
	}
	QScriptValue result = engine->newArray(matches.size());
	for (int i = 0; i < matches.size(); i++)
	{
		Position p = matches.at(i);
		QScriptValue v = engine->newObject();
		v.setProperty("x", map_coord(p.x), QScriptValue::ReadOnly);
		v.setProperty("y", map_coord(p.y), QScriptValue::ReadOnly);
		v.setProperty("type", SCRIPT_POSITION, QScriptValue::ReadOnly);
		result.setProperty(i, v);
	}
	return result;
}

//-- ## enumSelected()
//--
//-- Return an array containing all game objects currently selected by the host player. (3.2+ only)
//--
QScriptValue js_enumSelected(QScriptContext *, QScriptEngine *engine)
{
	QList<BASE_OBJECT *> matches;
	for (DROID *psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		if (psDroid->selected)
		{
			matches.push_back(psDroid);
		}
	}
	for (STRUCTURE *psStruct = apsStructLists[selectedPlayer]; psStruct; psStruct = psStruct->psNext)
	{
		if (psStruct->selected)
		{
			matches.push_back(psStruct);
		}
	}
	// TODO - also add selected delivery points
	QScriptValue result = engine->newArray(matches.size());
	for (int i = 0; i < matches.size(); i++)
	{
		result.setProperty(i, convMax(matches.at(i), engine));
	}
	return result;
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
	GROUPMAP *psMap = groups.value(engine);

	if (psMap != nullptr)
	{
		for (GROUPMAP::const_iterator i = psMap->constBegin(); i != psMap->constEnd(); ++i)
		{
			if (i.value() == groupId)
			{
				matches.push_back(i.key());
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
static QScriptValue js_activateStructure(QScriptContext *context, QScriptEngine *)
{
	QScriptValue structVal = context->argument(0);
	int id = structVal.property("id").toInt32();
	int player = structVal.property("player").toInt32();
	STRUCTURE *psStruct = IdToStruct(id, player);
	SCRIPT_ASSERT(context, psStruct, "No such structure id %d belonging to player %d", id, player);
	// ... and then do nothing with psStruct yet
	QScriptValue objVal = context->argument(1);
	int oid = objVal.property("id").toInt32();
	int oplayer = objVal.property("player").toInt32();
	OBJECT_TYPE otype = (OBJECT_TYPE)objVal.property("type").toInt32();
	BASE_OBJECT *psObj = IdToObject(otype, oid, oplayer);
	SCRIPT_ASSERT(context, psObj, "No such object id %d belonging to player %d", oid, oplayer);
	orderStructureObj(player, psObj);
	return QScriptValue(true);
}

//-- ## findResearch(research, [player])
//--
//-- Return list of research items remaining to be researched for the given research item. (3.2+ only)
//-- (Optional second argument 3.2.3+ only)
//--
static QScriptValue js_findResearch(QScriptContext *context, QScriptEngine *engine)
{
	QList<RESEARCH *> list;
	QString resName = context->argument(0).toString();
	int player = engine->globalObject().property("me").toInt32();
	if (context->argumentCount() == 2)
	{
		player = context->argument(1).toInt32();
	}
	RESEARCH *psTarget = getResearch(resName.toUtf8().constData());
	SCRIPT_ASSERT(context, psTarget, "No such research: %s", resName.toUtf8().constData());
	PLAYER_RESEARCH *plrRes = &asPlayerResList[player][psTarget->index];
	if (IsResearchStartedPending(plrRes) || IsResearchCompleted(plrRes))
	{
		return engine->newArray(0); // return empty array
	}
	debug(LOG_SCRIPT, "Find reqs for %s for player %d", resName.toUtf8().constData(), player);
	// Go down the requirements list for the desired tech
	QList<RESEARCH *> reslist;
	RESEARCH *cur = psTarget;
	while (cur)
	{
		if (!(asPlayerResList[player][cur->index].ResearchStatus & RESEARCHED))
		{
			debug(LOG_SCRIPT, "Added research in %d's %s for %s", player, getID(cur), getID(psTarget));
			list.append(cur);
		}
		RESEARCH *prev = cur;
		cur = nullptr;
		if (!prev->pPRList.empty())
		{
			cur = &asResearch[prev->pPRList[0]]; // get first pre-req
		}
		for (int i = 1; i < prev->pPRList.size(); i++)
		{
			// push any other pre-reqs on the stack
			reslist += &asResearch[prev->pPRList[i]];
		}
		if (!cur && !reslist.empty())
		{
			cur = reslist.takeFirst(); // retrieve options from the stack
		}
	}
	QScriptValue retval = engine->newArray(list.size());
	for (int i = 0; i < list.size(); i++)
	{
		retval.setProperty(i, convResearch(list[i], engine, player));
	}
	return retval;
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
	QScriptValue structVal = context->argument(0);
	int id = structVal.property("id").toInt32();
	int player = structVal.property("player").toInt32();
	STRUCTURE *psStruct = IdToStruct(id, player);
	SCRIPT_ASSERT(context, psStruct, "No such structure id %d belonging to player %d", id, player);
	QScriptValue list = context->argument(1);
	RESEARCH *psResearch = nullptr;  // Dummy initialisation.
	if (list.isArray())
	{
		int length = list.property("length").toInt32();
		int k;
		for (k = 0; k < length; k++)
		{
			QString resName = list.property(k).toString();
			psResearch = getResearch(resName.toUtf8().constData());
			SCRIPT_ASSERT(context, psResearch, "No such research: %s", resName.toUtf8().constData());
			PLAYER_RESEARCH *plrRes = &asPlayerResList[player][psResearch->index];
			if (!IsResearchStartedPending(plrRes) && !IsResearchCompleted(plrRes))
			{
				break; // use this one
			}
		}
		if (k == length)
		{
			debug(LOG_SCRIPT, "Exhausted research list -- doing nothing");
			return QScriptValue(false);
		}
	}
	else
	{
		QString resName = list.toString();
		psResearch = getResearch(resName.toUtf8().constData());
		SCRIPT_ASSERT(context, psResearch, "No such research: %s", resName.toUtf8().constData());
		PLAYER_RESEARCH *plrRes = &asPlayerResList[player][psResearch->index];
		if (IsResearchStartedPending(plrRes) || IsResearchCompleted(plrRes))
		{
			debug(LOG_SCRIPT, "%s has already been researched!", resName.toUtf8().constData());
			return QScriptValue(false);
		}
	}
	SCRIPT_ASSERT(context, psStruct->pStructureType->type == REF_RESEARCH, "Not a research lab: %s", objInfo(psStruct));
	RESEARCH_FACILITY *psResLab = (RESEARCH_FACILITY *)psStruct->pFunctionality;
	SCRIPT_ASSERT(context, psResLab->psSubject == nullptr, "Research lab not ready");
	// Go down the requirements list for the desired tech
	QList<RESEARCH *> reslist;
	RESEARCH *cur = psResearch;
	int iterations = 0;  // Only used to assert we're not stuck in the loop.
	while (cur)
	{
		if (researchAvailable(cur->index, player, ModeQueue))
		{
			bool started = false;
			for (int i = 0; i < game.maxPlayers; i++)
			{
				if (i == player || (aiCheckAlliances(player, i) && alliancesSharedResearch(game.alliance)))
				{
					int bits = asPlayerResList[i][cur->index].ResearchStatus;
					started = started || (bits & STARTED_RESEARCH) || (bits & STARTED_RESEARCH_PENDING)
					          || (bits & RESBITS_PENDING_ONLY) || (bits & RESEARCHED);
				}
			}
			if (!started) // found relevant item on the path?
			{
				sendResearchStatus(psStruct, cur->index, player, true);
#if defined (DEBUG)
				char sTemp[128];
				snprintf(sTemp, sizeof(sTemp), "player:%d starts topic from script: %s", player, getID(cur));
				NETlogEntry(sTemp, SYNC_FLAG, 0);
#endif
				debug(LOG_SCRIPT, "Started research in %d's %s(%d) of %s", player,
				      objInfo(psStruct), psStruct->id, getName(cur));
				return QScriptValue(true);
			}
		}
		RESEARCH *prev = cur;
		cur = nullptr;
		if (!prev->pPRList.empty())
		{
			cur = &asResearch[prev->pPRList[0]]; // get first pre-req
		}
		for (int i = 1; i < prev->pPRList.size(); i++)
		{
			// push any other pre-reqs on the stack
			reslist += &asResearch[prev->pPRList[i]];
		}
		if (!cur && !reslist.empty())
		{
			cur = reslist.takeFirst(); // retrieve options from the stack
		}
		ASSERT_OR_RETURN(QScriptValue(false), ++iterations < asResearch.size() * 100 || !cur, "Possible cyclic dependencies in prerequisites, possibly of research \"%s\".", getName(cur));
	}
	debug(LOG_SCRIPT, "No research topic found for %s(%d)", objInfo(psStruct), psStruct->id);
	return QScriptValue(false); // none found
}

//-- ## getResearch(research[, player])
//--
//-- Fetch information about a given technology item, given by a string that matches
//-- its definition in "research.json". If not found, returns null.
//--
static QScriptValue js_getResearch(QScriptContext *context, QScriptEngine *engine)
{
	int player = 0;
	if (context->argumentCount() == 2)
	{
		player = context->argument(1).toInt32();
	}
	else
	{
		player = engine->globalObject().property("me").toInt32();
	}
	QString resName = context->argument(0).toString();
	RESEARCH *psResearch = getResearch(resName.toUtf8().constData());
	if (!psResearch)
	{
		return QScriptValue::NullValue;
	}
	return convResearch(psResearch, engine, player);
}

//-- ## enumResearch()
//--
//-- Returns an array of all research objects that are currently and immediately available for research.
//--
static QScriptValue js_enumResearch(QScriptContext *context, QScriptEngine *engine)
{
	QList<RESEARCH *> reslist;
	int player = engine->globalObject().property("me").toInt32();
	for (int i = 0; i < asResearch.size(); i++)
	{
		RESEARCH *psResearch = &asResearch[i];
		if (!IsResearchCompleted(&asPlayerResList[player][i]) && researchAvailable(i, player, ModeQueue))
		{
			reslist += psResearch;
		}
	}
	QScriptValue result = engine->newArray(reslist.size());
	for (int i = 0; i < reslist.size(); i++)
	{
		result.setProperty(i, convResearch(reslist[i], engine, player));
	}
	return result;
}

//-- ## componentAvailable([component type,] component name)
//--
//-- Checks whether a given component is available to the current player. The first argument is
//-- optional and deprecated.
//--
static QScriptValue js_componentAvailable(QScriptContext *context, QScriptEngine *engine)
{
	int player = engine->globalObject().property("me").toInt32();
	QString id = (context->argumentCount() == 1) ? context->argument(0).toString() : context->argument(1).toString();
	COMPONENT_STATS *psComp = getCompStatsFromName(WzString::fromUtf8(id.toUtf8().constData()));
	SCRIPT_ASSERT(context, psComp, "No such component: %s", id.toUtf8().constData());
	int status = apCompLists[player][psComp->compType][psComp->index];
	return QScriptValue(status == AVAILABLE || status == REDUNDANT);
}

//-- ## addFeature(name, x, y)
//--
//-- Create and place a feature at the given x, y position. Will cause a desync in multiplayer.
//-- Returns the created game object on success, null otherwise. (3.2+ only)
//--
static QScriptValue js_addFeature(QScriptContext *context, QScriptEngine *engine)
{
	QString featName = context->argument(0).toString();
	int x = context->argument(1).toInt32();
	int y = context->argument(2).toInt32();
	int feature = getFeatureStatFromName(QStringToWzString(featName));
	FEATURE_STATS *psStats = &asFeatureStats[feature];
	for (FEATURE *psFeat = apsFeatureLists[0]; psFeat; psFeat = psFeat->psNext)
	{
		SCRIPT_ASSERT(context, map_coord(psFeat->pos.x) != x || map_coord(psFeat->pos.y) != y,
		              "Building feature on tile already occupied");
	}
	FEATURE *psFeature = buildFeature(psStats, world_coord(x), world_coord(y), false);
	return convFeature(psFeature, engine);
}

static int get_first_available_component(int player, int capacity, const QScriptValue &list, COMPONENT_TYPE type, bool strict)
{
	if (list.isArray())
	{
		int length = list.property("length").toInt32();
		int k;
		for (k = 0; k < length; k++)
		{
			QString compName = list.property(k).toString();
			int result = getCompFromName(type, QStringToWzString(compName));
			if (result >= 0)
			{
				int status = apCompLists[player][type][result];
				if (((status == AVAILABLE || status == REDUNDANT) || !strict)
					&& (type != COMP_BODY || asBodyStats[result].size <= capacity))
				{
					return result; // found one!
				}
			}
			if (result < 0)
			{
				debug(LOG_ERROR, "No such component: %s", compName.toUtf8().constData());
			}
		}
	}
	else if (list.isString())
	{
		int result = getCompFromName(type, QStringToWzString(list.toString()));
		if (result >= 0)
		{
			int status = apCompLists[player][type][result];
			if (((status == AVAILABLE || status == REDUNDANT) || !strict)
				&& (type != COMP_BODY || asBodyStats[result].size <= capacity))
			{
				return result; // found it!
			}
		}
		if (result < 0)
		{
			debug(LOG_ERROR, "No such component: %s", list.toString().toUtf8().constData());
		}
	}
	return -1; // no available component found in list
}

static DROID_TEMPLATE *makeTemplate(int player, const QString &templName, QScriptContext *context, int paramstart, int capacity, bool strict)
{
	const int firstTurret = paramstart + 4; // index position of first turret parameter
	DROID_TEMPLATE *psTemplate = new DROID_TEMPLATE;
	int numTurrets = context->argumentCount() - firstTurret; // anything beyond first six parameters, are turrets
	int result;

	memset(psTemplate->asParts, 0, sizeof(psTemplate->asParts)); // reset to defaults
	memset(psTemplate->asWeaps, 0, sizeof(psTemplate->asWeaps));
	int body = get_first_available_component(player, capacity, context->argument(paramstart), COMP_BODY, strict);
	if (body < 0)
	{
		debug(LOG_SCRIPT, "Wanted to build %s but body types all unavailable",
		      templName.toUtf8().constData());
		delete psTemplate;
		return nullptr; // no component available
	}
	int prop = get_first_available_component(player, capacity, context->argument(paramstart + 1), COMP_PROPULSION, strict);
	if (prop < 0)
	{
		debug(LOG_SCRIPT, "Wanted to build %s but propulsion types all unavailable",
		      templName.toUtf8().constData());
		delete psTemplate;
		return nullptr; // no component available
	}
	psTemplate->asParts[COMP_BODY] = body;
	psTemplate->asParts[COMP_PROPULSION] = prop;

	psTemplate->numWeaps = 0;
	numTurrets = MIN(numTurrets, asBodyStats[body].weaponSlots); // Restrict max no. turrets
	if (asBodyStats[body].droidTypeOverride != DROID_ANY)
	{
		psTemplate->droidType = asBodyStats[body].droidTypeOverride; // set droidType based on body
	}
	// Find first turret component type (assume every component in list is same type)
	QString compName;
	if (context->argument(firstTurret).isArray())
	{
		compName = context->argument(firstTurret).property(0).toString();
	}
	else // must be string
	{
		compName = context->argument(firstTurret).toString();
	}
	COMPONENT_STATS *psComp = getCompStatsFromName(WzString::fromUtf8(compName.toUtf8().constData()));
	if (psComp == nullptr)
	{
		debug(LOG_ERROR, "Wanted to build %s but %s does not exist", templName.toUtf8().constData(), compName.toUtf8().constData());
		delete psTemplate;
		return nullptr;
	}
	if (psComp->droidTypeOverride != DROID_ANY)
	{
		psTemplate->droidType = psComp->droidTypeOverride; // set droidType based on component
	}
	if (psComp->compType == COMP_WEAPON)
	{
		for (int i = 0; i < numTurrets; i++) // may be multi-weapon
		{
			result = get_first_available_component(player, SIZE_NUM, context->argument(firstTurret + i), COMP_WEAPON, strict);
			if (result < 0)
			{
				debug(LOG_SCRIPT, "Wanted to build %s but no weapon available", templName.toUtf8().constData());
				delete psTemplate;
				return nullptr;
			}
			psTemplate->asWeaps[i] = result;
			psTemplate->numWeaps++;
		}
	}
	else
	{
		if (psComp->compType == COMP_BRAIN)
		{
			psTemplate->numWeaps = 1; // hack, necessary to pass intValidTemplate
		}
		result = get_first_available_component(player, SIZE_NUM, context->argument(firstTurret), psComp->compType, strict);
		if (result < 0)
		{
			debug(LOG_SCRIPT, "Wanted to build %s but turret unavailable", templName.toUtf8().constData());
			delete psTemplate;
			return nullptr;
		}
		psTemplate->asParts[psComp->compType] = result;
	}
	bool valid = intValidTemplate(psTemplate, templName.toUtf8().constData(), true, player);
	if (valid)
	{
		return psTemplate;
	}
	else
	{
		delete psTemplate;
		debug(LOG_ERROR, "Invalid template %s", templName.toUtf8().constData());
		return nullptr;
	}
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
	int player = context->argument(0).toInt32();
	SCRIPT_ASSERT_PLAYER(context, player);
	int x = context->argument(1).toInt32();
	int y = context->argument(2).toInt32();
	QString templName = context->argument(3).toString();
	bool onMission = (x == -1) && (y == -1);
	SCRIPT_ASSERT(context, (onMission || (x >= 0 && y >= 0)), "Invalid coordinates (%d, %d) for droid", x, y);
	DROID_TEMPLATE *psTemplate = makeTemplate(player, templName, context, 4, SIZE_NUM, false);
	if (psTemplate)
	{
		DROID *psDroid = nullptr;
		bool oldMulti = bMultiMessages;
		bMultiMessages = false; // ugh, fixme
		if (onMission)
		{
			psDroid = buildMissionDroid(psTemplate, 128, 128, player);
			if (psDroid)
			{
				debug(LOG_LIFE, "Created mission-list droid %s by script for player %d: %u", objInfo(psDroid), player, psDroid->id);
			}
			else
			{
				debug(LOG_ERROR, "Invalid droid %s", templName.toUtf8().constData());
			}
		}
		else
		{
			psDroid = buildDroid(psTemplate, world_coord(x) + TILE_UNITS / 2, world_coord(y) + TILE_UNITS / 2, player, onMission, nullptr);
			if (psDroid)
			{
				addDroid(psDroid, apsDroidLists);
				debug(LOG_LIFE, "Created droid %s by script for player %d: %u", objInfo(psDroid), player, psDroid->id);
			}
			else
			{
				debug(LOG_ERROR, "Invalid droid %s", templName.toUtf8().constData());
			}
		}
		bMultiMessages = oldMulti; // ugh
		delete psTemplate;
		return psDroid ? QScriptValue(convDroid(psDroid, engine)) : QScriptValue::NullValue;
	}
	return QScriptValue::NullValue;
}

//-- ## addDroidToTransporter(transporter, droid)
//--
//-- Load a droid, which is currently located on the campaign off-world mission list,
//-- into a transporter, which is also currently on the campaign off-world mission list.
//-- (3.2+ only)
//--
static QScriptValue js_addDroidToTransporter(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue transporterVal = context->argument(0);
	int transporterId = transporterVal.property("id").toInt32();
	int transporterPlayer = transporterVal.property("player").toInt32();
	DROID *psTransporter = IdToMissionDroid(transporterId, transporterPlayer);
	SCRIPT_ASSERT(context, psTransporter, "No such transporter id %d belonging to player %d", transporterId, transporterPlayer);
	SCRIPT_ASSERT(context, isTransporter(psTransporter), "Droid id %d belonging to player %d is not a transporter", transporterId, transporterPlayer);
	QScriptValue droidVal = context->argument(1);
	int droidId = droidVal.property("id").toInt32();
	int droidPlayer = droidVal.property("player").toInt32();
	DROID *psDroid = IdToMissionDroid(droidId, droidPlayer);
	SCRIPT_ASSERT(context, psDroid, "No such droid id %d belonging to player %d", droidId, droidPlayer);
	SCRIPT_ASSERT(context, checkTransporterSpace(psTransporter, psDroid), "Not enough room in transporter %d for droid %d", transporterId, droidId);
	bool removeSuccessful = droidRemove(psDroid, mission.apsDroidLists);
	SCRIPT_ASSERT(context, removeSuccessful, "Could not remove droid id %d from mission list", droidId);
	psTransporter->psGroup->add(psDroid);
	return QScriptValue();
}

//-- ## makeTemplate(player, name, body, propulsion, reserved, turrets...)
//--
//-- Create a template (virtual droid) with the given components. Can be useful for calculating the cost
//-- of droids before putting them into production, for instance. Will fail and return null if template
//-- could not possibly be built using current research. (3.2+ only)
//--
static QScriptValue js_makeTemplate(QScriptContext *context, QScriptEngine *engine)
{
	int player = context->argument(0).toInt32();
	QString templName = context->argument(1).toString();
	DROID_TEMPLATE *psTemplate = makeTemplate(player, templName, context, 2, SIZE_NUM, true);
	if (!psTemplate)
	{
		return QScriptValue::NullValue;
	}
	QScriptValue retval = convTemplate(psTemplate, engine);
	delete psTemplate;
	return QScriptValue(retval);
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
	QScriptValue structVal = context->argument(0);
	int id = structVal.property("id").toInt32();
	int player = structVal.property("player").toInt32();
	STRUCTURE *psStruct = IdToStruct(id, player);
	SCRIPT_ASSERT(context, psStruct, "No such structure id %d belonging to player %d", id, player);
	SCRIPT_ASSERT(context, (psStruct->pStructureType->type == REF_FACTORY || psStruct->pStructureType->type == REF_CYBORG_FACTORY
	                        || psStruct->pStructureType->type == REF_VTOL_FACTORY), "Structure %s is not a factory", objInfo(psStruct));
	QString templName = context->argument(1).toString();
	const int capacity = psStruct->capacity; // body size limit
	DROID_TEMPLATE *psTemplate = makeTemplate(player, templName, context, 2, capacity, true);
	if (psTemplate)
	{
		SCRIPT_ASSERT(context, validTemplateForFactory(psTemplate, psStruct, true),
		              "Invalid template %s for factory %s",
		              getName(psTemplate), getName(psStruct->pStructureType));
		// Delete similar template from existing list before adding this one
		for (auto t : apsTemplateList)
		{
			if (t->name.compare(psTemplate->name) == 0)
			{
				debug(LOG_SCRIPT, "deleting %s for player %d", getName(t), player);
				deleteTemplateFromProduction(t, player, ModeQueue); // duplicate? done below?
				break;
			}
		}
		// Add to list
		debug(LOG_SCRIPT, "adding template %s for player %d", getName(psTemplate), player);
		psTemplate->multiPlayerID = generateNewObjectId();
		addTemplate(player, psTemplate);
		if (!structSetManufacture(psStruct, psTemplate, ModeQueue))
		{
			debug(LOG_ERROR, "Could not produce template %s in %s", getName(psTemplate), objInfo(psStruct));
			return QScriptValue(false);
		}
	}
	return QScriptValue(psTemplate != nullptr);
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
	QList<STRUCTURE *> matches;
	int player = -1, looking = -1;
	WzString statsName;
	QScriptValue val;
	STRUCTURE_TYPE type = NUM_DIFF_BUILDINGS;

	switch (context->argumentCount())
	{
	default:
	case 3: looking = context->argument(2).toInt32(); // fall-through
	case 2: val = context->argument(1);
		if (val.isNumber())
		{
			type = (STRUCTURE_TYPE)val.toInt32();
		}
		else
		{
			statsName = WzString::fromUtf8(val.toString().toUtf8().constData());
		} // fall-through
	case 1: player = context->argument(0).toInt32(); break;
	case 0: player = engine->globalObject().property("me").toInt32();
	}

	SCRIPT_ASSERT_PLAYER(context, player);
	SCRIPT_ASSERT(context, looking < MAX_PLAYERS && looking >= -1, "Looking player index out of range: %d", looking);
	for (STRUCTURE *psStruct = apsStructLists[player]; psStruct; psStruct = psStruct->psNext)
	{
		if ((looking == -1 || psStruct->visible[looking])
		    && !psStruct->died
		    && (type == NUM_DIFF_BUILDINGS || type == psStruct->pStructureType->type)
		    && (statsName.isEmpty() || statsName.compare(psStruct->pStructureType->id) == 0))
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
	QList<STRUCTURE *> matches;
	int player = -1, looking = -1;
	WzString statsName;
	QScriptValue val;
	STRUCTURE_TYPE type = NUM_DIFF_BUILDINGS;

	switch (context->argumentCount())
	{
	default:
	case 3: looking = context->argument(2).toInt32(); // fall-through
	case 2: val = context->argument(1);
		if (val.isNumber())
		{
			type = (STRUCTURE_TYPE)val.toInt32();
		}
		else
		{
			statsName = WzString::fromUtf8(val.toString().toUtf8().constData());
		} // fall-through
	case 1: player = context->argument(0).toInt32(); break;
	case 0: player = engine->globalObject().property("me").toInt32();
	}

	SCRIPT_ASSERT(context, player < MAX_PLAYERS && player >= 0, "Target player index out of range: %d", player);
	SCRIPT_ASSERT(context, looking < MAX_PLAYERS && looking >= -1, "Looking player index out of range: %d", looking);
	for (STRUCTURE *psStruct = mission.apsStructLists[player]; psStruct; psStruct = psStruct->psNext)
	{
		if ((looking == -1 || psStruct->visible[looking])
		    && !psStruct->died
		    && (type == NUM_DIFF_BUILDINGS || type == psStruct->pStructureType->type)
		    && (statsName.isEmpty() || statsName.compare(psStruct->pStructureType->id) == 0))
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

//-- ## enumFeature(player[, name])
//--
//-- Returns an array of all features seen by player of given name, as defined in "features.json".
//-- If player is ```ALL_PLAYERS```, it will return all features irrespective of visibility to any player. If
//-- name is empty, it will return any feature.
//--
static QScriptValue js_enumFeature(QScriptContext *context, QScriptEngine *engine)
{
	QList<FEATURE *> matches;
	int looking = context->argument(0).toInt32();
	WzString statsName;
	if (context->argumentCount() > 1)
	{
		statsName = WzString::fromUtf8(context->argument(1).toString().toUtf8().constData());
	}
	SCRIPT_ASSERT(context, looking < MAX_PLAYERS && looking >= -1, "Looking player index out of range: %d", looking);
	for (FEATURE *psFeat = apsFeatureLists[0]; psFeat; psFeat = psFeat->psNext)
	{
		if ((looking == -1 || psFeat->visible[looking])
		    && !psFeat->died
		    && (statsName.isEmpty() || statsName.compare(psFeat->psStats->id) == 0))
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

//-- ## enumCargo(transport droid)
//--
//-- Returns an array of droid objects inside given transport. (3.2+ only)
//--
static QScriptValue js_enumCargo(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue droidVal = context->argument(0);
	int id = droidVal.property("id").toInt32();
	int player = droidVal.property("player").toInt32();
	DROID *psDroid = IdToDroid(id, player);
	SCRIPT_ASSERT(context, psDroid, "No such droid id %d belonging to player %d", id, player);
	SCRIPT_ASSERT(context, isTransporter(psDroid), "Wrong droid type");
	QScriptValue result = engine->newArray(psDroid->psGroup->getNumMembers());
	int i = 0;
	for (DROID *psCurr = psDroid->psGroup->psList; psCurr; psCurr = psCurr->psGrpNext, i++)
	{
		if (psDroid != psCurr)
		{
			result.setProperty(i, convDroid(psCurr, engine));
		}
	}
	return result;
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
	QList<DROID *> matches;
	int player = -1, looking = -1;
	DROID_TYPE droidType = DROID_ANY;
	DROID_TYPE droidType2;

	switch (context->argumentCount())
	{
	default:
	case 3: looking = context->argument(2).toInt32(); // fall-through
	case 2: droidType = (DROID_TYPE)context->argument(1).toInt32(); // fall-through
	case 1: player = context->argument(0).toInt32(); break;
	case 0: player = engine->globalObject().property("me").toInt32();
	}
	switch (droidType) // hide some engine craziness
	{
	case DROID_CONSTRUCT:
		droidType2 = DROID_CYBORG_CONSTRUCT; break;
	case DROID_WEAPON:
		droidType2 = DROID_CYBORG_SUPER; break;
	case DROID_REPAIR:
		droidType2 = DROID_CYBORG_REPAIR; break;
	case DROID_CYBORG:
		droidType2 = DROID_CYBORG_SUPER; break;
	default:
		droidType2 = droidType;
		break;
	}
	SCRIPT_ASSERT_PLAYER(context, player);
	SCRIPT_ASSERT(context, looking < MAX_PLAYERS && looking >= -1, "Looking player index out of range: %d", looking);
	for (DROID *psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
	{
		if ((looking == -1 || psDroid->visible[looking])
		    && !psDroid->died
		    && (droidType == DROID_ANY || droidType == psDroid->droidType || droidType2 == psDroid->droidType))
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
	QScriptValue droidVal = context->argument(0);
	const int id = droidVal.property("id").toInt32();
	const int player = droidVal.property("player").toInt32();
	DROID *psDroid = IdToDroid(id, player);
	QString statName = context->argument(1).toString();
	int index = getStructStatFromName(QStringToWzString(statName));
	SCRIPT_ASSERT(context, index >= 0, "%s not found", statName.toUtf8().constData());
	STRUCTURE_STATS	*psStat = &asStructureStats[index];
	const int startX = context->argument(2).toInt32();
	const int startY = context->argument(3).toInt32();
	int numIterations = 30;
	bool found = false;
	int incX, incY, x, y;
	int maxBlockingTiles = 0;

	SCRIPT_ASSERT(context, psDroid, "No such droid id %d belonging to player %d", id, player);
	SCRIPT_ASSERT(context, psStat, "No such stat found: %s", statName.toUtf8().constData());
	SCRIPT_ASSERT_PLAYER(context, player);
	SCRIPT_ASSERT(context, startX >= 0 && startX < mapWidth && startY >= 0 && startY < mapHeight, "Bad position (%d, %d)", startX, startY);

	if (context->argumentCount() > 4) // final optional argument
	{
		maxBlockingTiles = context->argument(4).toInt32();
	}

	x = startX;
	y = startY;

	Vector2i offset(psStat->baseWidth * (TILE_UNITS / 2), psStat->baseBreadth * (TILE_UNITS / 2));

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
		retval.setProperty("x", x + map_coord(offset.x), QScriptValue::ReadOnly);
		retval.setProperty("y", y + map_coord(offset.y), QScriptValue::ReadOnly);
		retval.setProperty("type", SCRIPT_POSITION, QScriptValue::ReadOnly);
		return retval;
	}
	else
	{
		debug(LOG_SCRIPT, "Did not find valid positioning for %s", getName(psStat));
	}
	return QScriptValue();
}

//-- ## structureIdle(structure)
//--
//-- Is given structure idle?
//--
static QScriptValue js_structureIdle(QScriptContext *context, QScriptEngine *)
{
	QScriptValue structVal = context->argument(0);
	int id = structVal.property("id").toInt32();
	int player = structVal.property("player").toInt32();
	STRUCTURE *psStruct = IdToStruct(id, player);
	SCRIPT_ASSERT(context, psStruct, "No such structure id %d belonging to player %d", id, player);
	return QScriptValue(structureIdle(psStruct));
}

//-- ## removeStruct(structure)
//--
//-- Immediately remove the given structure from the map. Returns a boolean that is true on success.
//-- No special effects are applied. Deprecated since 3.2.
//--
static QScriptValue js_removeStruct(QScriptContext *context, QScriptEngine *)
{
	QScriptValue structVal = context->argument(0);
	int id = structVal.property("id").toInt32();
	int player = structVal.property("player").toInt32();
	STRUCTURE *psStruct = IdToStruct(id, player);
	SCRIPT_ASSERT(context, psStruct, "No such structure id %d belonging to player %d", id, player);
	return QScriptValue(removeStruct(psStruct, true));
}

//-- ## removeObject(game object[, special effects?])
//--
//-- Remove the given game object with special effects. Returns a boolean that is true on success.
//-- A second, optional boolean parameter specifies whether special effects are to be applied. (3.2+ only)
//--
static QScriptValue js_removeObject(QScriptContext *context, QScriptEngine *)
{
	QScriptValue qval = context->argument(0);
	int id = qval.property("id").toInt32();
	int player = qval.property("player").toInt32();
	OBJECT_TYPE type = (OBJECT_TYPE)qval.property("type").toInt32();
	BASE_OBJECT *psObj = IdToObject(type, id, player);
	SCRIPT_ASSERT(context, psObj, "Object id %d not found belonging to player %d", id, player);
	bool sfx = false;
	if (context->argumentCount() > 1)
	{
		sfx = context->argument(1).toBool();
	}
	bool retval = false;
	if (sfx)
	{
		switch (psObj->type)
		{
		case OBJ_STRUCTURE: destroyStruct((STRUCTURE *)psObj, gameTime); break;
		case OBJ_DROID: retval = destroyDroid((DROID *)psObj, gameTime); break;
		case OBJ_FEATURE: retval = destroyFeature((FEATURE *)psObj, gameTime); break;
		default: SCRIPT_ASSERT(context, false, "Wrong game object type"); break;
		}
	}
	else
	{
		switch (psObj->type)
		{
		case OBJ_STRUCTURE: retval = removeStruct((STRUCTURE *)psObj, true); break;
		case OBJ_DROID: retval = removeDroidBase((DROID *)psObj); break;
		case OBJ_FEATURE: retval = removeFeature((FEATURE *)psObj); break;
		default: SCRIPT_ASSERT(context, false, "Wrong game object type"); break;
		}
	}
	return QScriptValue(retval);
}

//-- ## clearConsole()
//--
//-- Clear the console. (3.3+ only)
//--
static QScriptValue js_clearConsole(QScriptContext *context, QScriptEngine *engine)
{
	flushConsoleMessages();
	return QScriptValue();
}

//-- ## console(strings...)
//--
//-- Print text to the player console.
//--
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
		addConsoleMessage(result.toUtf8().constData(), CENTRE_JUSTIFY, SYSTEM_MESSAGE);
		//permitNewConsoleMessages(false);
	}
	return QScriptValue();
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
	int32_t x1 = context->argument(0).toInt32();
	int32_t y1 = context->argument(1).toInt32();
	int32_t x2 = context->argument(2).toInt32();
	int32_t y2 = context->argument(3).toInt32();
	return QScriptValue(iHypot(x1 - x2, y1 - y2));
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
static QScriptValue js_droidCanReach(QScriptContext *context, QScriptEngine *)
{
	QScriptValue droidVal = context->argument(0);
	int id = droidVal.property("id").toInt32();
	int player = droidVal.property("player").toInt32();
	int x = context->argument(1).toInt32();
	int y = context->argument(2).toInt32();
	DROID *psDroid = IdToDroid(id, player);
	SCRIPT_ASSERT(context, psDroid, "Droid id %d not found belonging to player %d", id, player);
	const PROPULSION_STATS *psPropStats = asPropulsionStats + psDroid->asBits[COMP_PROPULSION];
	return QScriptValue(fpathCheck(psDroid->pos, Vector3i(world_coord(x), world_coord(y), 0), psPropStats->propulsionType));
}

//-- ## propulsionCanReach(propulsion, x1, y1, x2, y2)
//--
//-- Return true if a droid with a given propulsion is able to travel from (x1, y1) to (x2, y2).
//-- Does not take player built blockades into account. (3.2+ only)
//--
static QScriptValue js_propulsionCanReach(QScriptContext *context, QScriptEngine *)
{
	QScriptValue propulsionValue = context->argument(0);
	int propulsion = getCompFromName(COMP_PROPULSION, QStringToWzString(propulsionValue.toString()));
	SCRIPT_ASSERT(context, propulsion > 0, "No such propulsion: %s", propulsionValue.toString().toUtf8().constData());
	int x1 = context->argument(1).toInt32();
	int y1 = context->argument(2).toInt32();
	int x2 = context->argument(3).toInt32();
	int y2 = context->argument(4).toInt32();
	const PROPULSION_STATS *psPropStats = asPropulsionStats + propulsion;
	return QScriptValue(fpathCheck(Vector3i(world_coord(x1), world_coord(y1), 0), Vector3i(world_coord(x2), world_coord(y2), 0), psPropStats->propulsionType));
}

//-- ## terrainType(x, y)
//--
//-- Returns tile type of a given map tile, such as TER_WATER for water tiles or TER_CLIFFFACE for cliffs.
//-- Tile types regulate which units may pass through this tile. (3.2+ only)
//--
static QScriptValue js_terrainType(QScriptContext *context, QScriptEngine *)
{
	int x = context->argument(0).toInt32();
	int y = context->argument(1).toInt32();
	return QScriptValue(terrainType(mapTile(x, y)));
}

//-- ## orderDroid(droid, order)
//--
//-- Give a droid an order to do something. (3.2+ only)
//--
static QScriptValue js_orderDroid(QScriptContext *context, QScriptEngine *)
{
	QScriptValue droidVal = context->argument(0);
	int id = droidVal.property("id").toInt32();
	int player = droidVal.property("player").toInt32();
	DROID *psDroid = IdToDroid(id, player);
	SCRIPT_ASSERT(context, psDroid, "Droid id %d not found belonging to player %d", id, player);
	DROID_ORDER order = (DROID_ORDER)context->argument(1).toInt32();
	SCRIPT_ASSERT(context, order == DORDER_HOLD || order == DORDER_RTR || order == DORDER_STOP
	              || order == DORDER_RTB || order == DORDER_REARM || order == DORDER_RECYCLE,
	              "Invalid order: %s", getDroidOrderName(order));
	DROID_ORDER_DATA *droidOrder = &psDroid->order;
	if (droidOrder->type == order)
	{
		return QScriptValue(true);
	}
	if (order == DORDER_REARM)
	{
		if (STRUCTURE *psStruct = findNearestReArmPad(psDroid, psDroid->psBaseStruct, false))
		{
			orderDroidObj(psDroid, order, psStruct, ModeQueue);
		}
		else
		{
			orderDroid(psDroid, DORDER_RTB, ModeQueue);
		}
	}
	else
	{
		orderDroid(psDroid, order, ModeQueue);
	}
	return QScriptValue(true);
}

//-- ## orderDroidObj(droid, order, object)
//--
//-- Give a droid an order to do something to something.
//--
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
	OBJECT_TYPE otype = (OBJECT_TYPE)objVal.property("type").toInt32();
	BASE_OBJECT *psObj = IdToObject(otype, oid, oplayer);
	SCRIPT_ASSERT(context, psObj, "Object id %d not found belonging to player %d", oid, oplayer);
	SCRIPT_ASSERT(context, validOrderForObj(order), "Invalid order: %s", getDroidOrderName(order));
	DROID_ORDER_DATA *droidOrder = &psDroid->order;
	if (droidOrder->type == order && psDroid->order.psObj == psObj)
	{
		return QScriptValue(true);
	}
	orderDroidObj(psDroid, order, psObj, ModeQueue);
	return QScriptValue(true);
}

//-- ## orderDroidBuild(droid, order, structure type, x, y[, direction])
//--
//-- Give a droid an order to build something at the given position. Returns true if allowed.
//--
static QScriptValue js_orderDroidBuild(QScriptContext *context, QScriptEngine *)
{
	QScriptValue droidVal = context->argument(0);
	int id = droidVal.property("id").toInt32();
	int player = droidVal.property("player").toInt32();
	DROID *psDroid = IdToDroid(id, player);
	DROID_ORDER order = (DROID_ORDER)context->argument(1).toInt32();
	QString statName = context->argument(2).toString();
	int index = getStructStatFromName(QStringToWzString(statName));
	SCRIPT_ASSERT(context, index >= 0, "%s not found", statName.toUtf8().constData());
	STRUCTURE_STATS	*psStats = &asStructureStats[index];
	int x = context->argument(3).toInt32();
	int y = context->argument(4).toInt32();
	uint16_t direction = 0;

	SCRIPT_ASSERT(context, order == DORDER_BUILD, "Invalid order");
	SCRIPT_ASSERT(context, psStats->id.compare("A0ADemolishStructure") != 0, "Cannot build demolition");
	if (context->argumentCount() > 5)
	{
		direction = DEG(context->argument(5).toNumber());
	}
	DROID_ORDER_DATA *droidOrder = &psDroid->order;
	if (droidOrder->type == order && psDroid->actionPos.x == world_coord(x) && psDroid->actionPos.y == world_coord(y))
	{
		return QScriptValue(true);
	}
	orderDroidStatsLocDir(psDroid, order, psStats, world_coord(x) + TILE_UNITS / 2, world_coord(y) + TILE_UNITS / 2, direction, ModeQueue);
	return QScriptValue(true);
}

//-- ## orderDroidLoc(droid, order, x, y)
//--
//-- Give a droid an order to do something at the given location.
//--
static QScriptValue js_orderDroidLoc(QScriptContext *context, QScriptEngine *)
{
	QScriptValue droidVal = context->argument(0);
	int id = droidVal.property("id").toInt32();
	int player = droidVal.property("player").toInt32();
	QScriptValue orderVal = context->argument(1);
	int x = context->argument(2).toInt32();
	int y = context->argument(3).toInt32();
	DROID_ORDER order = (DROID_ORDER)orderVal.toInt32();
	SCRIPT_ASSERT(context, validOrderForLoc(order), "Invalid location based order: %s", getDroidOrderName(order));
	DROID *psDroid = IdToDroid(id, player);
	SCRIPT_ASSERT(context, psDroid, "Droid id %d not found belonging to player %d", id, player);
	SCRIPT_ASSERT(context, tileOnMap(x, y), "Outside map bounds (%d, %d)", x, y);
	DROID_ORDER_DATA *droidOrder = &psDroid->order;
	if (droidOrder->type == order && psDroid->actionPos.x == world_coord(x) && psDroid->actionPos.y == world_coord(y))
	{
		return QScriptValue();
	}
	orderDroidLoc(psDroid, order, world_coord(x), world_coord(y), ModeQueue);
	return QScriptValue();
}

//-- ## setMissionTime(time)
//--
//-- Set mission countdown in seconds.
//--
static QScriptValue js_setMissionTime(QScriptContext *context, QScriptEngine *)
{
	int value = context->argument(0).toInt32() * GAME_TICKS_PER_SEC;
	mission.startTime = gameTime;
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

//-- ## getMissionTime()
//--
//-- Get time remaining on mission countdown in seconds. (3.2+ only)
//--
static QScriptValue js_getMissionTime(QScriptContext *, QScriptEngine *)
{
	return QScriptValue((mission.time - (gameTime - mission.startTime)) / GAME_TICKS_PER_SEC);
}

//-- ## setTransporterExit(x, y, player)
//--
//-- Set the exit position for the mission transporter. (3.2+ only)
//--
static QScriptValue js_setTransporterExit(QScriptContext *context, QScriptEngine *)
{
	int x = context->argument(0).toInt32();
	int y = context->argument(1).toInt32();
	int player = context->argument(2).toInt32();
	SCRIPT_ASSERT_PLAYER(context, player);
	missionSetTransporterExit(player, x, y);
	return QScriptValue();
}

//-- ## startTransporterEntry(x, y, player)
//--
//-- Set the entry position for the mission transporter, and make it start flying in
//-- reinforcements. If you want the camera to follow it in, use cameraTrack() on it.
//-- The transport needs to be set up with the mission droids, and the first transport
//-- found will be used. (3.2+ only)
//--
static QScriptValue js_startTransporterEntry(QScriptContext *context, QScriptEngine *)
{
	int x = context->argument(0).toInt32();
	int y = context->argument(1).toInt32();
	int player = context->argument(2).toInt32();
	SCRIPT_ASSERT_PLAYER(context, player);
	missionSetTransporterEntry(player, x, y);
	missionFlyTransportersIn(player, false);
	return QScriptValue();
}

//-- ## useSafetyTransport(flag)
//--
//-- Change if the mission transporter will fetch droids in non offworld missions
//-- setReinforcementTime() is be used to hide it before coming back after the set time
//-- which is handled by the campaign library in the victory data section (3.3+ only).
//--
static QScriptValue js_useSafetyTransport(QScriptContext *context, QScriptEngine *)
{
	bool flag = context->argument(0).toBool();
	setDroidsToSafetyFlag(flag);
	return QScriptValue();
}

//-- ## restoreLimboMissionData()
//--
//-- Swap mission type and bring back units previously stored at the start
//-- of the mission (see cam3-c mission). (3.3+ only).
//--
static QScriptValue js_restoreLimboMissionData(QScriptContext *context, QScriptEngine *)
{
	resetLimboMission();
	return QScriptValue();
}

//-- ## setReinforcementTime(time)
//--
//-- Set time for reinforcements to arrive. If time is negative, the reinforcement GUI
//-- is removed and the timer stopped. Time is in seconds.
//-- If time equals to the magic LZ_COMPROMISED_TIME constant, reinforcement GUI ticker
//-- is set to "--:--" and reinforcements are suppressed until this function is called
//-- again with a regular time value.
//--
static QScriptValue js_setReinforcementTime(QScriptContext *context, QScriptEngine *)
{
	int value = context->argument(0).toInt32() * GAME_TICKS_PER_SEC;
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
		for (psDroid = apsDroidLists[selectedPlayer]; psDroid != nullptr; psDroid = psDroid->psNext)
		{
			if (isTransporter(psDroid))
			{
				break;
			}
		}
		// if not found a transporter, can remove the launch button
		if (psDroid ==  nullptr)
		{
			intRemoveTransporterLaunch();
		}
	}
	return QScriptValue();
}

//-- ## setStructureLimits(structure type, limit[, player])
//--
//-- Set build limits for a structure.
//--
static QScriptValue js_setStructureLimits(QScriptContext *context, QScriptEngine *engine)
{
	QString building = context->argument(0).toString();
	int limit = context->argument(1).toInt32();
	int player;
	int structInc = getStructStatFromName(QStringToWzString(building));
	if (context->argumentCount() > 2)
	{
		player = context->argument(2).toInt32();
	}
	else
	{
		player = engine->globalObject().property("me").toInt32();
	}
	SCRIPT_ASSERT_PLAYER(context, player);
	SCRIPT_ASSERT(context, limit < LOTS_OF && limit >= 0, "Invalid limit");
	SCRIPT_ASSERT(context, structInc < numStructureStats && structInc >= 0, "Invalid structure");

	asStructureStats[structInc].upgrade[player].limit = limit;

	return QScriptValue();
}

//-- ## centreView(x, y)
//--
//-- Center the player's camera at the given position.
//--
static QScriptValue js_centreView(QScriptContext *context, QScriptEngine *)
{
	int x = context->argument(0).toInt32();
	int y = context->argument(1).toInt32();
	setViewPos(x, y, false);
	return QScriptValue();
}

//-- ## hackPlayIngameAudio()
//--
//-- (3.3+ only)
//--
static QScriptValue js_hackPlayIngameAudio(QScriptContext *context, QScriptEngine *)
{
	debug(LOG_SOUND, "Script wanted music to start");
	cdAudio_PlayTrack(SONG_INGAME);
	return QScriptValue();
}

//-- ## hackStopIngameAudio()
//--
//-- (3.3+ only)
//--
static QScriptValue js_hackStopIngameAudio(QScriptContext *context, QScriptEngine *)
{
	debug(LOG_SOUND, "Script wanted music to stop");
	cdAudio_Stop();
	return QScriptValue();
}

//-- ## playSound(sound[, x, y, z])
//--
//-- Play a sound, optionally at a location.
//--
static QScriptValue js_playSound(QScriptContext *context, QScriptEngine *engine)
{
	int player = engine->globalObject().property("me").toInt32();
	if (player != selectedPlayer)
	{
		return QScriptValue();
	}
	QString sound = context->argument(0).toString();
	int soundID = audio_GetTrackID(sound.toUtf8().constData());
	if (soundID == SAMPLE_NOT_FOUND)
	{
		soundID = audio_SetTrackVals(sound.toUtf8().constData(), false, 100, 1800);
	}
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
	}
	return QScriptValue();
}

//-- ## gameOverMessage(won, showBackDrop, showOutro)
//--
//-- End game in victory or defeat.
//--
static QScriptValue js_gameOverMessage(QScriptContext *context, QScriptEngine *engine)
{
	int player = engine->globalObject().property("me").toInt32();
	const MESSAGE_TYPE msgType = MSG_MISSION;	// always
	bool gameWon = context->argument(0).toBool();
	bool showOutro = false;
	bool showBackDrop = true;
	if (context->argumentCount() > 1)
	{
		showBackDrop = context->argument(1).toBool();
	}
	if (context->argumentCount() > 2)
	{
		showOutro = context->argument(2).toBool();
	}
	VIEWDATA *psViewData;
	if (gameWon)
	{
		//Quick hack to stop assert when trying to play outro in campaign.
		psViewData = !bMultiPlayer && showOutro ? getViewData("END") : getViewData("WIN");
		addConsoleMessage(_("YOU ARE VICTORIOUS!"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	}
	else
	{
		psViewData = getViewData("END");	// FIXME: rename to FAILED|LOST ?
		addConsoleMessage(_("YOU WERE DEFEATED!"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	}
	ASSERT(psViewData, "Viewdata not found");
	MESSAGE *psMessage = addMessage(msgType, false, player);
	if (!bMultiPlayer && psMessage)
	{
		//we need to set this here so the VIDEO_QUIT callback is not called
		setScriptWinLoseVideo(gameWon ? PLAY_WIN : PLAY_LOSE);
		seq_ClearSeqList();
		if (gameWon && showOutro)
		{
			showBackDrop = false;
			seq_AddSeqToList("outro.ogg", nullptr, "outro.txa", false);
			seq_StartNextFullScreenVideo();
		}
		else
		{
			//set the data
			psMessage->pViewData = psViewData;
			displayImmediateMessage(psMessage);
			stopReticuleButtonFlash(IDRET_INTEL_MAP);
		}
	}
	jsDebugMessageUpdate();
	displayGameOver(gameWon, showBackDrop);
	if (challengeActive)
	{
		updateChallenge(gameWon);
	}
	if (autogame_enabled())
	{
		debug(LOG_WARNING, "Autogame completed successfully!");
		exit(0);
	}
	return QScriptValue();
}

//-- ## completeResearch(research[, player [, forceResearch]])
//--
//-- Finish a research for the given player.
//-- forceResearch will allow a research topic to be researched again. 3.3+
//--
static QScriptValue js_completeResearch(QScriptContext *context, QScriptEngine *engine)
{
	QString researchName = context->argument(0).toString();
	int player;
	bool forceIt = false;
	if (context->argumentCount() > 1)
	{
		player = context->argument(1).toInt32();
	}
	else
	{
		player = engine->globalObject().property("me").toInt32();
	}
	if (context->argumentCount() > 2)
	{
		forceIt = context->argument(2).toBool();
	}
	RESEARCH *psResearch = getResearch(researchName.toUtf8().constData());
	SCRIPT_ASSERT(context, psResearch, "No such research %s for player %d", researchName.toUtf8().constData(), player);
	SCRIPT_ASSERT(context, psResearch->index < asResearch.size(), "Research index out of bounds");
	PLAYER_RESEARCH *plrRes = &asPlayerResList[player][psResearch->index];
	if (!forceIt && IsResearchCompleted(plrRes))
	{
		return QScriptValue();
	}
	if (bMultiMessages && (gameTime > 2))
	{
		SendResearch(player, psResearch->index, false);
		// Wait for our message before doing anything.
	}
	else
	{
		researchResult(psResearch->index, player, false, nullptr, false);
	}
	return QScriptValue();
}

//-- ## completeAllResearch([player])
//--
//-- Finish all researches for the given player.
//--
static QScriptValue js_completeAllResearch(QScriptContext *context, QScriptEngine *engine)
{
	int player;
	if (context->argumentCount() > 0)
	{
		player = context->argument(0).toInt32();
	}
	else
	{
		player = engine->globalObject().property("me").toInt32();
	}
	for (int i = 0; i < asResearch.size(); i++)
	{
		RESEARCH *psResearch = &asResearch[i];
		PLAYER_RESEARCH *plrRes = &asPlayerResList[player][psResearch->index];
		if (!IsResearchCompleted(plrRes))
		{
			if (bMultiMessages && (gameTime > 2))
			{
				SendResearch(player, psResearch->index, false);
				// Wait for our message before doing anything.
			}
			else
			{
				researchResult(psResearch->index, player, false, nullptr, false);
			}
		}
	}
	return QScriptValue();
}

//-- ## enableResearch(research[, player])
//--
//-- Enable a research for the given player, allowing it to be researched.
//--
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

//-- ## extraPowerTime(time, player)
//--
//-- Increase a player's power as if that player had power income equal to current income
//-- over the given amount of extra time. (3.2+ only)
//--
static QScriptValue js_extraPowerTime(QScriptContext *context, QScriptEngine *engine)
{
	int ticks = context->argument(0).toInt32() * GAME_UPDATES_PER_SEC;
	int player;
	if (context->argumentCount() > 1)
	{
		player = context->argument(1).toInt32();
		SCRIPT_ASSERT_PLAYER(context, player);
	}
	else
	{
		player = engine->globalObject().property("me").toInt32();
	}
	updatePlayerPower(player, ticks);
	return QScriptValue();
}

//-- ## setPower(power[, player])
//--
//-- Set a player's power directly. (Do not use this in an AI script.)
//--
static QScriptValue js_setPower(QScriptContext *context, QScriptEngine *engine)
{
	int power = context->argument(0).toInt32();
	int player;
	if (context->argumentCount() > 1)
	{
		player = context->argument(1).toInt32();
		SCRIPT_ASSERT_PLAYER(context, player);
	}
	else
	{
		player = engine->globalObject().property("me").toInt32();
	}
	setPower(player, power);
	return QScriptValue();
}

//-- ## setPowerModifier(power[, player])
//--
//-- Set a player's power modifier percentage. (Do not use this in an AI script.) (3.2+ only)
//--
static QScriptValue js_setPowerModifier(QScriptContext *context, QScriptEngine *engine)
{
	int power = context->argument(0).toInt32();
	int player;
	if (context->argumentCount() > 1)
	{
		player = context->argument(1).toInt32();
		SCRIPT_ASSERT_PLAYER(context, player);
	}
	else
	{
		player = engine->globalObject().property("me").toInt32();
	}
	setPowerModifier(player, power);
	return QScriptValue();
}

//-- ## setPowerStorageMaximum(maximum[, player])
//--
//-- Set a player's power storage maximum. (Do not use this in an AI script.) (3.2+ only)
//--
static QScriptValue js_setPowerStorageMaximum(QScriptContext *context, QScriptEngine *engine)
{
	int power = context->argument(0).toInt32();
	int player;
	if (context->argumentCount() > 1)
	{
		player = context->argument(1).toInt32();
		SCRIPT_ASSERT_PLAYER(context, player);
	}
	else
	{
		player = engine->globalObject().property("me").toInt32();
	}
	setPowerMaxStorage(player, power);
	return QScriptValue();
}

//-- ## enableStructure(structure type[, player])
//--
//-- The given structure type is made available to the given player. It will appear in the
//-- player's build list.
//--
static QScriptValue js_enableStructure(QScriptContext *context, QScriptEngine *engine)
{
	QString building = context->argument(0).toString();
	int index = getStructStatFromName(QStringToWzString(building));
	int player;
	if (context->argumentCount() > 1)
	{
		player = context->argument(1).toInt32();
		SCRIPT_ASSERT_PLAYER(context, player);
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

//-- ## setTutorialMode(bool)
//--
//-- Sets a number of restrictions appropriate for tutorial if set to true.
//--
static QScriptValue js_setTutorialMode(QScriptContext *context, QScriptEngine *engine)
{
	bInTutorial = context->argument(0).toBool();
	return QScriptValue();
}

//-- ## setMiniMap(bool)
//--
//-- Turns visible minimap on or off in the GUI.
//--
static QScriptValue js_setMiniMap(QScriptContext *context, QScriptEngine *engine)
{
	radarPermitted = context->argument(0).toBool();
	return QScriptValue();
}

//-- ## setDesign(bool)
//--
//-- Whether to allow player to design stuff.
//--
static QScriptValue js_setDesign(QScriptContext *context, QScriptEngine *engine)
{
	DROID_TEMPLATE *psCurr;
	allowDesign = context->argument(0).toBool();
	// Switch on or off future templates
	// FIXME: This dual data structure for templates is just plain insane.
	for (auto &keyvaluepair : droidTemplates[selectedPlayer])
	{
		bool researched = researchedTemplate(keyvaluepair.second, selectedPlayer, true);
		keyvaluepair.second->enabled = (researched || allowDesign);
	}
	for (auto &localTemplate : localTemplates)
	{
		psCurr = &localTemplate;
		bool researched = researchedTemplate(psCurr, selectedPlayer, true);
		psCurr->enabled = (researched || allowDesign);
	}
	return QScriptValue();
}

//-- ## enableTemplate(template name)
//--
//-- Enable a specific template (even if design is disabled).
//--
static QScriptValue js_enableTemplate(QScriptContext *context, QScriptEngine *engine)
{
	DROID_TEMPLATE *psCurr;
	WzString templateName = WzString::fromUtf8(context->argument(0).toString().toUtf8().constData());
	bool found = false;
	// FIXME: This dual data structure for templates is just plain insane.
	for (auto &keyvaluepair : droidTemplates[selectedPlayer])
	{
		if (templateName.compare(keyvaluepair.second->id) == 0)
		{
			keyvaluepair.second->enabled = true;
			found = true;
			break;
		}
	}
	if (!found)
	{
		debug(LOG_ERROR, "Template %s was not found!", templateName.toUtf8().c_str());
		return QScriptValue(false);
	}
	for (auto &localTemplate : localTemplates)
	{
		psCurr = &localTemplate;
		if (templateName.compare(psCurr->id) == 0)
		{
			psCurr->enabled = true;
			break;
		}
	}
	return QScriptValue();
}

//-- ## removeTemplate(template name)
//--
//-- Remove a template.
//--
static QScriptValue js_removeTemplate(QScriptContext *context, QScriptEngine *engine)
{
	DROID_TEMPLATE *psCurr;
	WzString templateName = WzString::fromUtf8(context->argument(0).toString().toUtf8().constData());
	bool found = false;
	// FIXME: This dual data structure for templates is just plain insane.
	for (auto &keyvaluepair : droidTemplates[selectedPlayer])
	{
		if (templateName.compare(keyvaluepair.second->id) == 0)
		{
			keyvaluepair.second->enabled = false;
			found = true;
			break;
		}
	}
	if (!found)
	{
		debug(LOG_ERROR, "Template %s was not found!", templateName.toUtf8().c_str());
		return QScriptValue(false);
	}
	for (std::list<DROID_TEMPLATE>::iterator i = localTemplates.begin(); i != localTemplates.end(); ++i)
	{
		psCurr = &*i;
		if (templateName.compare(psCurr->id) == 0)
		{
			localTemplates.erase(i);
			break;
		}
	}
	return QScriptValue();
}

//-- ## setReticuleButton(id, filename, filenameHigh, tooltip, callback)
//--
//-- Add reticule button. id is which button to change, where zero is zero is the middle button, then going clockwise from the
//-- uppermost button. filename is button graphics and filenameHigh is for highlighting. The tooltip is the text you see when
//-- you mouse over the button. Finally, the callback is which scripting function to call. Hide and show the user interface
//-- for such changes to take effect. (3.2+ only)
//--
static QScriptValue js_setReticuleButton(QScriptContext *context, QScriptEngine *engine)
{
	int button = context->argument(0).toInt32();
	SCRIPT_ASSERT(context, button >= 0 && button <= 6, "Invalid button %d", button);
	std::string tip = std::string(context->argument(1).toString().toUtf8().constData());
	std::string file = std::string(context->argument(2).toString().toUtf8().constData());
	std::string fileDown = std::string(context->argument(3).toString().toUtf8().constData());
	WzString func;
	if (context->argumentCount() > 4)
	{
		func = QStringToWzString(context->argument(4).toString());
	}
	setReticuleStats(button, tip, file, fileDown, func, engine);
	return QScriptValue();
}

//-- ## showReticuleWidget(id)
//--
//-- Open the reticule menu widget. (3.3+ only)
//--
static QScriptValue js_showReticuleWidget(QScriptContext *context, QScriptEngine *engine)
{
	int button = context->argument(0).toInt32();
	SCRIPT_ASSERT(context, button >= 0 && button <= 6, "Invalid button %d", button);
	intShowWidget(button);
	return QScriptValue();
}

//-- ## setReticuleFlash(id, flash)
//--
//-- Set reticule flash on or off. (3.2.3+ only)
//--
static QScriptValue js_setReticuleFlash(QScriptContext *context, QScriptEngine *engine)
{
	int button = context->argument(0).toInt32();
	SCRIPT_ASSERT(context, button >= 0 && button <= 6, "Invalid button %d", button);
	bool flash = context->argument(1).toBoolean();
	setReticuleFlash(button, flash);
	return QScriptValue();
}

//-- ## showInterface()
//--
//-- Show user interface. (3.2+ only)
//--
static QScriptValue js_showInterface(QScriptContext *context, QScriptEngine *engine)
{
	intAddReticule();
	intShowPowerBar();
	return QScriptValue();
}

//-- ## hideInterface(button type)
//--
//-- Hide user interface. (3.2+ only)
//--
static QScriptValue js_hideInterface(QScriptContext *context, QScriptEngine *engine)
{
	intRemoveReticule();
	intHidePowerBar();
	return QScriptValue();
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
	Q_UNUSED(context);
	Q_UNUSED(engine);
	applyLimitSet();
	return QScriptValue();
}

static void setComponent(const QString& name, int player, int value)
{
	COMPONENT_STATS *psComp = getCompStatsFromName(WzString::fromUtf8(name.toUtf8().constData()));
	ASSERT_OR_RETURN(, psComp, "Bad component %s", name.toUtf8().constData());
	apCompLists[player][psComp->compType][psComp->index] = value;
}

//-- ## enableComponent(component, player)
//--
//-- The given component is made available for research for the given player.
//--
static QScriptValue js_enableComponent(QScriptContext *context, QScriptEngine *engine)
{
	QString componentName = context->argument(0).toString();
	int player = context->argument(1).toInt32();

	SCRIPT_ASSERT_PLAYER(context, player);
	setComponent(componentName, player, FOUND);
	return QScriptValue();
}

//-- ## makeComponentAvailable(component, player)
//--
//-- The given component is made available to the given player. This means the player can
//-- actually build designs with it.
//--
static QScriptValue js_makeComponentAvailable(QScriptContext *context, QScriptEngine *engine)
{
	QString componentName = context->argument(0).toString();
	int player = context->argument(1).toInt32();

	SCRIPT_ASSERT_PLAYER(context, player);
	setComponent(componentName, player, AVAILABLE);
	return QScriptValue();
}

//-- ## allianceExistsBetween(player, player)
//--
//-- Returns true if an alliance exists between the two players, or they are the same player.
//--
static QScriptValue js_allianceExistsBetween(QScriptContext *context, QScriptEngine *engine)
{
	int player1 = context->argument(0).toInt32();
	int player2 = context->argument(1).toInt32();
	SCRIPT_ASSERT(context, player1 < MAX_PLAYERS && player1 >= 0, "Invalid player");
	SCRIPT_ASSERT(context, player2 < MAX_PLAYERS && player2 >= 0, "Invalid player");
	return QScriptValue(alliances[player1][player2] == ALLIANCE_FORMED);
}

//-- ## _(string)
//--
//-- Mark string for translation.
//--
static QScriptValue js_translate(QScriptContext *context, QScriptEngine *engine)
{
	// The redundant QString cast is a workaround for a Qt5 bug, the QScriptValue(char const *) constructor interprets as Latin1 instead of UTF-8!
	return QScriptValue(QString(gettext(context->argument(0).toString().toUtf8().constData())));
}

//-- ## playerPower(player)
//--
//-- Return amount of power held by the given player.
//--
static QScriptValue js_playerPower(QScriptContext *context, QScriptEngine *engine)
{
	int player = context->argument(0).toInt32();
	SCRIPT_ASSERT_PLAYER(context, player);
	return QScriptValue(getPower(player));
}

//-- ## queuedPower(player)
//--
//-- Return amount of power queued up for production by the given player. (3.2+ only)
//--
static QScriptValue js_queuedPower(QScriptContext *context, QScriptEngine *engine)
{
	int player = context->argument(0).toInt32();
	SCRIPT_ASSERT_PLAYER(context, player);
	return QScriptValue(getQueuedPower(player));
}

//-- ## isStructureAvailable(structure type[, player])
//--
//-- Returns true if given structure can be built. It checks both research and unit limits.
//--
static QScriptValue js_isStructureAvailable(QScriptContext *context, QScriptEngine *engine)
{
	QString building = context->argument(0).toString();
	int index = getStructStatFromName(QStringToWzString(building));
	SCRIPT_ASSERT(context, index >= 0, "%s not found", building.toUtf8().constData());
	int player;
	if (context->argumentCount() > 1)
	{
		player = context->argument(1).toInt32();
	}
	else
	{
		player = engine->globalObject().property("me").toInt32();
	}
	int status = apStructTypeLists[player][index];
	return QScriptValue((status == AVAILABLE || status == REDUNDANT)
	                    && asStructureStats[index].curCount[player] < asStructureStats[index].upgrade[player].limit);
}

//-- ## isVTOL(droid)
//--
//-- Returns true if given droid is a VTOL (not including transports).
//--
static QScriptValue js_isVTOL(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue droidVal = context->argument(0);
	int id = droidVal.property("id").toInt32();
	int player = droidVal.property("player").toInt32();
	DROID *psDroid = IdToDroid(id, player);
	SCRIPT_ASSERT(context, psDroid, "No such droid id %d belonging to player %d", id, player);
	return QScriptValue(isVtolDroid(psDroid));
}

//-- ## hackGetObj(type, player, id)
//--
//-- Function to find and return a game object of DROID, FEATURE or STRUCTURE types, if it exists.
//-- Otherwise, it will return null. This function is deprecated by getObject(). (3.2+ only)
//--
static QScriptValue js_hackGetObj(QScriptContext *context, QScriptEngine *engine)
{
	OBJECT_TYPE type = (OBJECT_TYPE)context->argument(0).toInt32();
	int player = context->argument(1).toInt32();
	int id = context->argument(2).toInt32();
	SCRIPT_ASSERT_PLAYER(context, player);
	return QScriptValue(convMax(IdToObject(type, id, player), engine));
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
	int me = context->argument(0).toInt32();
	SCRIPT_ASSERT_PLAYER(context, me);
	engine->globalObject().setProperty("me", me);
	return QScriptValue();
}

//-- ## receiveAllEvents(bool)
//--
//-- Make the current script receive all events, even those not meant for 'me'. (3.2+ only)
//--
static QScriptValue js_receiveAllEvents(QScriptContext *context, QScriptEngine *engine)
{
	if (context->argumentCount() > 0)
	{
		bool value = context->argument(0).toBool();
		engine->globalObject().setProperty("isReceivingAllEvents", value, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	}
	return engine->globalObject().property("isReceivingAllEvents");
}

//-- ## hackAssert(condition, message...)
//--
//-- Function to perform unit testing. It will throw a script error and a game assert. (3.2+ only)
//--
static QScriptValue js_hackAssert(QScriptContext *context, QScriptEngine *engine)
{
	bool condition = context->argument(0).toBool();
	if (condition)
	{
		return QScriptValue(); // pass
	}
	// fail
	QString result;
	for (int i = 1; i < context->argumentCount(); ++i)
	{
		if (i != 1)
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
	context->throwError(QScriptContext::ReferenceError, result +  " in " + QString(__FUNCTION__) + " at line " + QString::number(__LINE__));
	return QScriptValue();
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
	QScriptValue droidVal = context->argument(0);
	int id = droidVal.property("id").toInt32();
	int player = droidVal.property("player").toInt32();
	DROID *psDroid = IdToDroid(id, player);
	SCRIPT_ASSERT(context, psDroid, "No such droid id %d belonging to player %d", id, player);
	psDroid->experience = context->argument(1).toNumber() * 65536;
	return QScriptValue();
}

//-- ## donateObject(object, to)
//--
//-- Donate a game object (restricted to droids before 3.2.3) to another player. Returns true if
//-- donation was successful. May return false if this donation would push the receiving player
//-- over unit limits. (3.2+ only)
//--
static QScriptValue js_donateObject(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue val = context->argument(0);
	uint32_t id = val.property("id").toUInt32();
	uint8_t player = val.property("player").toInt32();
	OBJECT_TYPE type = (OBJECT_TYPE)val.property("type").toInt32();
	uint8_t to = context->argument(1).toInt32();
	uint8_t giftType = 0;
	if (type == OBJ_DROID)
	{
		// Check unit limits.
		DROID *psDroid = IdToDroid(id, player);
		SCRIPT_ASSERT(context, psDroid, "No such droid id %u belonging to player %u", id, player);
		if ((psDroid->droidType == DROID_COMMAND && getNumCommandDroids(to) + 1 > getMaxCommanders(to))
		    || (psDroid->droidType == DROID_CONSTRUCT && getNumConstructorDroids(to) + 1 > getMaxConstructors(to))
		    || getNumDroids(to) + 1 > getMaxDroids(to))
		{
			return QScriptValue(false);
		}
		giftType = DROID_GIFT;
	}
	else if (type == OBJ_STRUCTURE)
	{
		STRUCTURE *psStruct = IdToStruct(id, player);
		SCRIPT_ASSERT(context, psStruct, "No such struct id %u belonging to player %u", id, player);
		const int statidx = psStruct->pStructureType - asStructureStats;
		if (asStructureStats[statidx].curCount[to] + 1 > asStructureStats[statidx].upgrade[to].limit)
		{
			return QScriptValue(false);
		}
		giftType = STRUCTURE_GIFT;
	}
	else
	{
		return QScriptValue(false);
	}
	NETbeginEncode(NETgameQueue(selectedPlayer), GAME_GIFT);
	NETuint8_t(&giftType);
	NETuint8_t(&player);
	NETuint8_t(&to);
	NETuint32_t(&id);
	NETend();
	return QScriptValue(true);
}

//-- ## donatePower(amount, to)
//--
//-- Donate power to another player. Returns true. (3.2+ only)
//--
static QScriptValue js_donatePower(QScriptContext *context, QScriptEngine *engine)
{
	int amount = context->argument(0).toInt32();
	int to = context->argument(1).toInt32();
	int from = engine->globalObject().property("me").toInt32();
	giftPower(from, to, amount, true);
	return QScriptValue(true);
}

//-- ## safeDest(player, x, y)
//--
//-- Returns true if given player is safe from hostile fire at the given location, to
//-- the best of that player's map knowledge. Does not work in campaign at the moment.
//--
static QScriptValue js_safeDest(QScriptContext *context, QScriptEngine *engine)
{
	int player = context->argument(0).toInt32();
	SCRIPT_ASSERT_PLAYER(context, player);
	int x = context->argument(1).toInt32();
	int y = context->argument(2).toInt32();
	SCRIPT_ASSERT(context, tileOnMap(x, y), "Out of bounds coordinates(%d, %d)", x, y);
	return QScriptValue(!(auxTile(x, y, player) & AUXBITS_DANGER));
}

//-- ## addStructure(structure id, player, x, y)
//--
//-- Create a structure on the given position. Returns the structure on success, null otherwise.
//-- Position uses world coordinates, if you want use position based on Map Tiles, then
//-- use as addStructure(structure id, players, x*128, y*128)
//--
static QScriptValue js_addStructure(QScriptContext *context, QScriptEngine *engine)
{
	QString building = context->argument(0).toString();
	int index = getStructStatFromName(QStringToWzString(building));
	SCRIPT_ASSERT(context, index >= 0, "%s not found", building.toUtf8().constData());
	int player = context->argument(1).toInt32();
	SCRIPT_ASSERT_PLAYER(context, player);
	int x = context->argument(2).toInt32();
	int y = context->argument(3).toInt32();
	STRUCTURE_STATS *psStat = &asStructureStats[index];
	STRUCTURE *psStruct = buildStructure(psStat, x, y, player, false);
	if (psStruct)
	{
		psStruct->status = SS_BUILT;
		buildingComplete(psStruct);
		return QScriptValue(convStructure(psStruct, engine));
	}
	return QScriptValue::NullValue;
}

//-- ## getStructureLimit(structure type[, player])
//--
//-- Returns build limits for a structure.
//--
static QScriptValue js_getStructureLimit(QScriptContext *context, QScriptEngine *engine)
{
	QString building = context->argument(0).toString();
	int index = getStructStatFromName(QStringToWzString(building));
	SCRIPT_ASSERT(context, index >= 0, "%s not found", building.toUtf8().constData());
	int player;
	if (context->argumentCount() > 1)
	{
		player = context->argument(1).toInt32();
	}
	else
	{
		player = engine->globalObject().property("me").toInt32();
	}
	return QScriptValue(asStructureStats[index].upgrade[player].limit);
}

//-- ## countStruct(structure type[, player])
//--
//-- Count the number of structures of a given type.
//-- The player parameter can be a specific player, ALL_PLAYERS, ALLIES or ENEMIES.
//--
static QScriptValue js_countStruct(QScriptContext *context, QScriptEngine *engine)
{
	QString building = context->argument(0).toString();
	int index = getStructStatFromName(QStringToWzString(building));
	int me = engine->globalObject().property("me").toInt32();
	int player = me;
	int quantity = 0;
	if (context->argumentCount() > 1)
	{
		player = context->argument(1).toInt32();
	}
	SCRIPT_ASSERT(context, index < numStructureStats && index >= 0, "Structure %s not found", building.toUtf8().constData());
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (player == i || player == ALL_PLAYERS
		    || (player == ALLIES && aiCheckAlliances(i, me))
		    || (player == ENEMIES && !aiCheckAlliances(i, me)))
		{
			quantity += asStructureStats[index].curCount[i];
		}
	}
	return QScriptValue(quantity);
}

//-- ## countDroid([droid type[, player]])
//--
//-- Count the number of droids that a given player has. Droid type must be either
//-- DROID_ANY, DROID_COMMAND or DROID_CONSTRUCT.
//-- The player parameter can be a specific player, ALL_PLAYERS, ALLIES or ENEMIES.
//--
static QScriptValue js_countDroid(QScriptContext *context, QScriptEngine *engine)
{
	int me = engine->globalObject().property("me").toInt32();
	int player = me;
	int quantity = 0;
	int type = DROID_ANY;
	if (context->argumentCount() > 0)
	{
		type = context->argument(0).toInt32();
	}
	SCRIPT_ASSERT(context, type <= DROID_ANY, "Bad droid type parameter");
	if (context->argumentCount() > 1)
	{
		player = context->argument(1).toInt32();
	}
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (player == i || player == ALL_PLAYERS
		    || (player == ALLIES && aiCheckAlliances(i, me))
		    || (player == ENEMIES && !aiCheckAlliances(i, me)))
		{
			if (type == DROID_ANY)
			{
				quantity += getNumDroids(i);
			}
			else if (type == DROID_CONSTRUCT)
			{
				quantity += getNumConstructorDroids(i);
			}
			else if (type == DROID_COMMAND)
			{
				quantity += getNumCommandDroids(i);
			}
		}
	}
	return QScriptValue(quantity);
}

//-- ## setNoGoArea(x1, y1, x2, y2, player)
//--
//-- Creates an area on the map on which nothing can be built. If player is zero,
//-- then landing lights are placed. If player is -1, then a limbo landing zone
//-- is created and limbo droids placed.
//--
// FIXME: missing a way to call initNoGoAreas(); check if we can call this in
// every level start instead of through scripts
static QScriptValue js_setNoGoArea(QScriptContext *context, QScriptEngine *)
{
	const int x1 = context->argument(0).toInt32();
	const int y1 = context->argument(1).toInt32();
	const int x2 = context->argument(2).toInt32();
	const int y2 = context->argument(3).toInt32();
	const int player = context->argument(4).toInt32();

	SCRIPT_ASSERT(context, x1 >= 0, "Minimum scroll x value %d is less than zero - ", x1);
	SCRIPT_ASSERT(context, y1 >= 0, "Minimum scroll y value %d is less than zero - ", y1);
	SCRIPT_ASSERT(context, x2 <= mapWidth, "Maximum scroll x value %d is greater than mapWidth %d", x2, (int)mapWidth);
	SCRIPT_ASSERT(context, y2 <= mapHeight, "Maximum scroll y value %d is greater than mapHeight %d", y2, (int)mapHeight);
	SCRIPT_ASSERT(context, player < MAX_PLAYERS && player >= -1, "Bad player value %d", player);

	if (player == -1)
	{
		setNoGoArea(x1, y1, x2, y2, LIMBO_LANDING);
		placeLimboDroids();	// this calls the Droids from the Limbo list onto the map
	}
	else
	{
		setNoGoArea(x1, y1, x2, y2, player);
	}
	return QScriptValue();
}

//-- ## setScrollLimits(x1, y1, x2, y2)
//--
//-- Limit the scrollable area of the map to the given rectangle. (3.2+ only)
//--
static QScriptValue js_setScrollLimits(QScriptContext *context, QScriptEngine *)
{
	const int minX = context->argument(0).toInt32();
	const int minY = context->argument(1).toInt32();
	const int maxX = context->argument(2).toInt32();
	const int maxY = context->argument(3).toInt32();

	SCRIPT_ASSERT(context, minX >= 0, "Minimum scroll x value %d is less than zero - ", minX);
	SCRIPT_ASSERT(context, minY >= 0, "Minimum scroll y value %d is less than zero - ", minY);
	SCRIPT_ASSERT(context, maxX <= mapWidth, "Maximum scroll x value %d is greater than mapWidth %d", maxX, (int)mapWidth);
	SCRIPT_ASSERT(context, maxY <= mapHeight, "Maximum scroll y value %d is greater than mapHeight %d", maxY, (int)mapHeight);

	const int prevMinX = scrollMinX;
	const int prevMinY = scrollMinY;
	const int prevMaxX = scrollMaxX;
	const int prevMaxY = scrollMaxY;

	scrollMinX = minX;
	scrollMaxX = maxX;
	scrollMinY = minY;
	scrollMaxY = maxY;

	// When the scroll limits change midgame - need to redo the lighting
	initLighting(prevMinX < scrollMinX ? prevMinX : scrollMinX,
	             prevMinY < scrollMinY ? prevMinY : scrollMinY,
	             prevMaxX < scrollMaxX ? prevMaxX : scrollMaxX,
	             prevMaxY < scrollMaxY ? prevMaxY : scrollMaxY);

	// need to reset radar to take into account of new size
	resizeRadar();
	return QScriptValue();
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
static QScriptValue js_loadLevel(QScriptContext *context, QScriptEngine *)
{
	QString level = context->argument(0).toString();

	sstrcpy(aLevelName, level.toUtf8().constData());

	// Find the level dataset
	LEVEL_DATASET *psNewLevel = levFindDataSet(level.toUtf8().constData());
	SCRIPT_ASSERT(context, psNewLevel, "Could not find level data for %s", level.toUtf8().constData());

	// Get the mission rolling...
	nextMissionType = psNewLevel->type;
	loopMissionState = LMS_CLEAROBJECTS;
	return QScriptValue();
}

//-- ## autoSave()
//--
//-- Perform automatic save
//--
static QScriptValue js_autoSave(QScriptContext *, QScriptEngine *engine)
{
	autoSave();
	return QScriptValue();
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
	int player = engine->globalObject().property("me").toInt32();
	int x = world_coord(context->argument(0).toInt32());
	int y = world_coord(context->argument(1).toInt32());
	int range = world_coord(context->argument(2).toInt32());
	int filter = ALL_PLAYERS;
	bool seen = true;
	if (context->argumentCount() > 3)
	{
		filter = context->argument(3).toInt32();
	}
	if (context->argumentCount() > 4)
	{
		seen = context->argument(4).toBool();
	}
	static GridList gridList;  // static to avoid allocations.
	gridList = gridStartIterate(x, y, range);
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
		QString label = context->argument(0).toString();
		nextparam = 1;
		SCRIPT_ASSERT(context, labels.contains(label), "Label %s not found", label.toUtf8().constData());
		LABEL p = labels.value(label);
		SCRIPT_ASSERT(context, p.type == SCRIPT_AREA, "Wrong label type for %s", label.toUtf8().constData());
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
	int x = world_coord(context->argument(0).toInt32());
	int y = world_coord(context->argument(1).toInt32());
	int target = context->argument(2).toInt32();
	QString message = context->argument(3).toString();
	int me = engine->globalObject().property("me").toInt32();
	SCRIPT_ASSERT(context, target >= 0 || target == ALLIES, "Message to invalid player %d", target);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (i != me && (i == target || (target == ALLIES && aiCheckAlliances(i, me))))
		{
			debug(LOG_MSG, "adding script beacon to %d from %d", i, me);
			sendBeaconToPlayer(x, y, i, me, message.toUtf8().constData());
		}
	}
	return QScriptValue(true);
}

//-- ## removeBeacon(target player)
//--
//-- Remove a beacon message sent to target player. Target may also be ```ALLIES```.
//-- Returns a boolean that is true on success. (3.2+ only)
//--
static QScriptValue js_removeBeacon(QScriptContext *context, QScriptEngine *engine)
{
	int me = engine->globalObject().property("me").toInt32();
	int target = context->argument(0).toInt32();
	SCRIPT_ASSERT(context, target >= 0 || target == ALLIES, "Message to invalid player %d", target);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (i == target || (target == ALLIES && aiCheckAlliances(i, me)))
		{
			MESSAGE *psMessage = findBeaconMsg(i, me);
			if (psMessage)
			{
				removeMessage(psMessage, i);
				triggerEventBeaconRemoved(me, i);
			}
		}
	}
	jsDebugMessageUpdate();
	return QScriptValue(true);
}

//-- ## chat(target player, message)
//--
//-- Send a message to target player. Target may also be ```ALL_PLAYERS``` or ```ALLIES```.
//-- Returns a boolean that is true on success. (3.2+ only)
//--
static QScriptValue js_chat(QScriptContext *context, QScriptEngine *engine)
{
	int player = engine->globalObject().property("me").toInt32();
	int target = context->argument(0).toInt32();
	QString message = context->argument(1).toString();
	SCRIPT_ASSERT(context, target >= 0 || target == ALL_PLAYERS || target == ALLIES, "Message to invalid player %d", target);
	if (target == ALL_PLAYERS) // all
	{
		return QScriptValue(sendTextMessage(message.toUtf8().constData(), true, player));
	}
	else if (target == ALLIES) // allies
	{
		return QScriptValue(sendTextMessage(QString(". " + message).toUtf8().constData(), false, player));
	}
	else // specific player
	{
		QString tmp = QString::number(NetPlay.players[target].position) + message;
		return QScriptValue(sendTextMessage(tmp.toUtf8().constData(), false, player));
	}
}

//-- ## setAlliance(player1, player2, value)
//--
//-- Set alliance status between two players to either true or false. (3.2+ only)
//--
static QScriptValue js_setAlliance(QScriptContext *context, QScriptEngine *engine)
{
	int player1 = context->argument(0).toInt32();
	int player2 = context->argument(1).toInt32();
	bool value = context->argument(2).toBool();
	if (value)
	{
		formAlliance(player1, player2, true, false, true);
	}
	else
	{
		breakAlliance(player1, player2, true, true);
	}
	return QScriptValue(true);
}

//-- ## sendAllianceRequest(player)
//--
//-- Send an alliance request to a player. (3.3+ only)
//--
static QScriptValue js_sendAllianceRequest(QScriptContext *context, QScriptEngine *engine)
{
	if (!alliancesFixed(game.alliance))
	{
		int player1 = engine->globalObject().property("me").toInt32();
		int player2 = context->argument(0).toInt32();
		requestAlliance(player1, player2, true, true);
	}
	return QScriptValue();
}

//-- ## setAssemblyPoint(structure, x, y)
//--
//-- Set the assembly point droids go to when built for the specified structure. (3.2+ only)
//--
static QScriptValue js_setAssemblyPoint(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue structVal = context->argument(0);
	int id = structVal.property("id").toInt32();
	int player = structVal.property("player").toInt32();
	STRUCTURE *psStruct = IdToStruct(id, player);
	SCRIPT_ASSERT(context, psStruct, "No such structure id %d belonging to player %d", id, player);
	int x = context->argument(1).toInt32();
	int y = context->argument(2).toInt32();
	SCRIPT_ASSERT(context, psStruct->pStructureType->type == REF_FACTORY
	              || psStruct->pStructureType->type == REF_CYBORG_FACTORY
	              || psStruct->pStructureType->type == REF_VTOL_FACTORY, "Structure not a factory");
	setAssemblyPoint(((FACTORY *)psStruct->pFunctionality)->psAssemblyPoint, x, y, player, true);
	return QScriptValue(true);
}

//-- ## hackNetOff()
//--
//-- Turn off network transmissions. FIXME - find a better way.
//--
static QScriptValue js_hackNetOff(QScriptContext *, QScriptEngine *)
{
	bMultiPlayer = false;
	bMultiMessages = false;
	return QScriptValue();
}

//-- ## hackNetOn()
//--
//-- Turn on network transmissions. FIXME - find a better way.
//--
static QScriptValue js_hackNetOn(QScriptContext *, QScriptEngine *)
{
	bMultiPlayer = true;
	bMultiMessages = true;
	return QScriptValue();
}

//-- ## getDroidProduction(factory)
//--
//-- Return droid in production in given factory. Note that this droid is fully
//-- virtual, and should never be passed anywhere. (3.2+ only)
//--
static QScriptValue js_getDroidProduction(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue structVal = context->argument(0);
	int id = structVal.property("id").toInt32();
	int player = structVal.property("player").toInt32();
	STRUCTURE *psStruct = IdToStruct(id, player);
	SCRIPT_ASSERT(context, psStruct, "No such structure id %d belonging to player %d", id, player);
	FACTORY *psFactory = &psStruct->pFunctionality->factory;
	DROID_TEMPLATE *psTemp = psFactory->psSubject;
	if (!psTemp)
	{
		return QScriptValue::NullValue;
	}
	DROID sDroid(0, player), *psDroid = &sDroid;
	psDroid->pos = psStruct->pos;
	psDroid->rot = psStruct->rot;
	psDroid->experience = 0;
	droidSetName(psDroid, getName(psTemp));
	droidSetBits(psTemp, psDroid);
	psDroid->weight = calcDroidWeight(psTemp);
	psDroid->baseSpeed = calcDroidBaseSpeed(psTemp, psDroid->weight, player);
	return convDroid(psDroid, engine);
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
	if (context->argumentCount() > 1)
	{
		DROID_TYPE type = (DROID_TYPE)context->argument(1).toInt32();
		if (type == DROID_COMMAND)
		{
			return QScriptValue(getMaxCommanders(context->argument(0).toInt32()));
		}
		else if (type == DROID_CONSTRUCT)
		{
			return QScriptValue(getMaxConstructors(context->argument(0).toInt32()));
		}
		// else return general unit limit
	}
	if (context->argumentCount() > 0)
	{
		return QScriptValue(getMaxDroids(context->argument(0).toInt32()));
	}
	return QScriptValue(getMaxDroids(engine->globalObject().property("me").toInt32()));
}

//-- ## getExperienceModifier(player)
//--
//-- Get the percentage of experience this player droids are going to gain. (3.2+ only)
//--
static QScriptValue js_getExperienceModifier(QScriptContext *context, QScriptEngine *)
{
	int player = context->argument(0).toInt32();
	return QScriptValue(getExpGain(player));
}

//-- ## setExperienceModifier(player, percent)
//--
//-- Set the percentage of experience this player droids are going to gain. (3.2+ only)
//--
static QScriptValue js_setExperienceModifier(QScriptContext *context, QScriptEngine *)
{
	int player = context->argument(0).toInt32();
	int percent = context->argument(1).toInt32();
	setExpGain(player, percent);
	return QScriptValue();
}

//-- ## setDroidLimit(player, value[, droid type])
//--
//-- Set the maximum number of droids that this player can produce. If a third
//-- parameter is added, this is the droid type to limit. It can be DROID_ANY
//-- for droids in general, DROID_CONSTRUCT for constructors, or DROID_COMMAND
//-- for commanders. (3.2+ only)
//--
static QScriptValue js_setDroidLimit(QScriptContext *context, QScriptEngine *)
{
	int player = context->argument(0).toInt32();
	int value = context->argument(1).toInt32();
	DROID_TYPE type = DROID_ANY;
	if (context->argumentCount() > 1)
	{
		type = (DROID_TYPE)context->argument(2).toInt32();
	}
	switch (type)
	{
	case DROID_CONSTRUCT:
		setMaxConstructors(player, value);
		break;
	case DROID_COMMAND:
		setMaxCommanders(player, value);
		break;
	default:
	case DROID_ANY:
		setMaxDroids(player, value);
		break;
	}
	return QScriptValue();
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
static QScriptValue js_hackAddMessage(QScriptContext *context, QScriptEngine *)
{
	QString mess = context->argument(0).toString();
	MESSAGE_TYPE msgType = (MESSAGE_TYPE)context->argument(1).toInt32();
	int player = context->argument(2).toInt32();
	bool immediate = context->argument(3).toBool();
	MESSAGE *psMessage = addMessage(msgType, false, player);
	if (psMessage)
	{
		VIEWDATA *psViewData = getViewData(QStringToWzString(mess));
		SCRIPT_ASSERT(context, psViewData, "Viewdata not found");
		psMessage->pViewData = psViewData;
		debug(LOG_MSG, "Adding %s pViewData=%p", psViewData->name.toUtf8().c_str(), static_cast<void *>(psMessage->pViewData));
		if (msgType == MSG_PROXIMITY)
		{
			VIEW_PROXIMITY *psProx = (VIEW_PROXIMITY *)psViewData->pData;
			// check the z value is at least the height of the terrain
			int height = map_Height(psProx->x, psProx->y);
			if (psProx->z < height)
			{
				psProx->z = height;
			}
		}
		if (immediate)
		{
			displayImmediateMessage(psMessage);
		}
	}
	jsDebugMessageUpdate();
	return QScriptValue();
}

//-- ## hackRemoveMessage(message, type, player)
//--
//-- See wzscript docs for info, to the extent any exist. (3.2+ only)
//--
static QScriptValue js_hackRemoveMessage(QScriptContext *context, QScriptEngine *)
{
	QString mess = context->argument(0).toString();
	MESSAGE_TYPE msgType = (MESSAGE_TYPE)context->argument(1).toInt32();
	int player = context->argument(2).toInt32();
	VIEWDATA *psViewData = getViewData(QStringToWzString(mess));
	SCRIPT_ASSERT(context, psViewData, "Viewdata not found");
	MESSAGE *psMessage = findMessage(psViewData, msgType, player);
	if (psMessage)
	{
		debug(LOG_MSG, "Removing %s", psViewData->name.toUtf8().c_str());
		removeMessage(psMessage, player);
	}
	else
	{
		debug(LOG_ERROR, "cannot find message - %s", psViewData->name.toUtf8().c_str());
	}
	jsDebugMessageUpdate();
	return QScriptValue();
}

//-- ## setSunPosition(x, y, z)
//--
//-- Move the position of the Sun, which in turn moves where shadows are cast. (3.2+ only)
//--
static QScriptValue js_setSunPosition(QScriptContext *context, QScriptEngine *)
{
	float x = context->argument(0).toNumber();
	float y = context->argument(1).toNumber();
	float z = context->argument(2).toNumber();
	setTheSun(Vector3f(x, y, z));
	return QScriptValue();
}

//-- ## setSunIntensity(ambient r, g, b, diffuse r, g, b, specular r, g, b)
//--
//-- Set the ambient, diffuse and specular colour intensities of the Sun lighting source. (3.2+ only)
//--
static QScriptValue js_setSunIntensity(QScriptContext *context, QScriptEngine *)
{
	float ambient[4];
	float diffuse[4];
	float specular[4];
	ambient[0] = context->argument(0).toNumber();
	ambient[1] = context->argument(1).toNumber();
	ambient[2] = context->argument(2).toNumber();
	ambient[3] = 1.0f;
	diffuse[0] = context->argument(3).toNumber();
	diffuse[1] = context->argument(4).toNumber();
	diffuse[2] = context->argument(5).toNumber();
	diffuse[3] = 1.0f;
	specular[0] = context->argument(6).toNumber();
	specular[1] = context->argument(7).toNumber();
	specular[2] = context->argument(8).toNumber();
	specular[3] = 1.0f;
	pie_Lighting0(LIGHT_AMBIENT, ambient);
	pie_Lighting0(LIGHT_DIFFUSE, diffuse);
	pie_Lighting0(LIGHT_SPECULAR, specular);
	return QScriptValue();
}

//-- ## setWeather(weather type)
//--
//-- Set the current weather. This should be one of WEATHER_RAIN, WEATHER_SNOW or WEATHER_CLEAR. (3.2+ only)
//--
static QScriptValue js_setWeather(QScriptContext *context, QScriptEngine *)
{
	WT_CLASS weather = (WT_CLASS)context->argument(0).toInt32();
	SCRIPT_ASSERT(context, weather >= 0 && weather <= WT_NONE, "Bad weather type");
	atmosSetWeatherType(weather);
	return QScriptValue();
}

//-- ## setSky(texture file, wind speed, skybox scale)
//--
//-- Change the skybox. (3.2+ only)
//--
static QScriptValue js_setSky(QScriptContext *context, QScriptEngine *)
{
	QString page = context->argument(0).toString();
	float wind = context->argument(1).toNumber();
	float scale = context->argument(2).toNumber();
	setSkyBox(page.toUtf8().constData(), wind, scale);
	return QScriptValue();
}

//-- ## hackDoNotSave(name)
//--
//-- Do not save the given global given by name to savegames. Must be
//-- done again each time game is loaded, since this too is not saved.
//--
static QScriptValue js_hackDoNotSave(QScriptContext *context, QScriptEngine *)
{
	QString name = context->argument(0).toString();
	doNotSaveGlobal(name);
	return QScriptValue();
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
		QString label = context->argument(0).toString();
		SCRIPT_ASSERT(context, labels.contains(label), "Label %s not found", label.toUtf8().constData());
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
static QScriptValue js_cameraSlide(QScriptContext *context, QScriptEngine *)
{
	float x = context->argument(0).toNumber();
	float y = context->argument(1).toNumber();
	requestRadarTrack(x, y);
	return QScriptValue();
}

//-- ## cameraZoom(z, speed)
//--
//-- Slide the camera to the given zoom distance. Normal camera zoom ranges between 500 and 5000. (3.2+ only)
//--
static QScriptValue js_cameraZoom(QScriptContext *context, QScriptEngine *)
{
	float viewDistance = context->argument(0).toNumber();
	float speed = context->argument(1).toNumber();
	animateToViewDistance(viewDistance, speed);
	return QScriptValue();
}

//-- ## cameraTrack(droid)
//--
//-- Make the camera follow the given droid object around. Pass in a null object to stop. (3.2+ only)
//--
static QScriptValue js_cameraTrack(QScriptContext *context, QScriptEngine *)
{
	if (context->argument(0).isNull())
	{
		setWarCamActive(false);
	}
	else
	{
		QScriptValue droidVal = context->argument(0);
		int id = droidVal.property("id").toInt32();
		int player = droidVal.property("player").toInt32();
		DROID *targetDroid = IdToDroid(id, player);
		SCRIPT_ASSERT(context, targetDroid, "No such droid id %d belonging to player %d", id, player);
		for (DROID *psDroid = apsDroidLists[selectedPlayer]; psDroid != nullptr; psDroid = psDroid->psNext)
		{
			psDroid->selected = (psDroid == targetDroid); // select only the target droid
		}
		setWarCamActive(true);
	}
	return QScriptValue();
}

//-- ## setHealth(object, health)
//--
//-- Change the health of the given game object, in percentage. Does not take care of network sync, so for multiplayer games,
//-- needs wrapping in a syncRequest. (3.2.3+ only.)
//--
static QScriptValue js_setHealth(QScriptContext *context, QScriptEngine *)
{
	QScriptValue objVal = context->argument(0);
	int health = context->argument(1).toInt32();
	SCRIPT_ASSERT(context, health >= 1, "Bad health value %d", health);
	int id = objVal.property("id").toInt32();
	int player = objVal.property("player").toInt32();
	OBJECT_TYPE type = (OBJECT_TYPE)objVal.property("type").toInt32();
	SCRIPT_ASSERT(context, type == OBJ_DROID || type == OBJ_STRUCTURE || type == OBJ_FEATURE, "Bad object type");
	if (type == OBJ_DROID)
	{
		DROID *psDroid = IdToDroid(id, player);
		SCRIPT_ASSERT(context, psDroid, "No such droid id %d belonging to player %d", id, player);
		psDroid->body = health * (double)psDroid->originalBody / 100;
	}
	else if (type == OBJ_STRUCTURE)
	{
		STRUCTURE *psStruct = IdToStruct(id, player);
		SCRIPT_ASSERT(context, psStruct, "No such structure id %d belonging to player %d", id, player);
		psStruct->body = health * MAX(1, structureBody(psStruct)) / 100;
	}
	else
	{
		FEATURE *psFeat = IdToFeature(id, player);
		SCRIPT_ASSERT(context, psFeat, "No such feature id %d belonging to player %d", id, player);
		psFeat->body = health * psFeat->psStats->body / 100;
	}
	return QScriptValue();
}

//-- ## setObjectFlag(object, flag, value)
//--
//-- Set or unset an object flag on a given game object. Does not take care of network sync, so for multiplayer games,
//-- needs wrapping in a syncRequest. (3.3+ only.)
//-- Recognized object flags: OBJECT_FLAG_UNSELECTABLE - makes object unavailable for selection from player UI.
//--
static QScriptValue js_setObjectFlag(QScriptContext *context, QScriptEngine *)
{
	QScriptValue obj = context->argument(0);
	OBJECT_FLAG flag = (OBJECT_FLAG)context->argument(1).toInt32();
	SCRIPT_ASSERT(context, flag < OBJECT_FLAG_COUNT, "Bad flag value %d", context->argument(1).toInt32());
	int id = obj.property("id").toInt32();
	int player = obj.property("player").toInt32();
	OBJECT_TYPE type = (OBJECT_TYPE)obj.property("type").toInt32();
	SCRIPT_ASSERT(context, type == OBJ_DROID || type == OBJ_STRUCTURE || type == OBJ_FEATURE, "Bad object type");
	BASE_OBJECT *psObj = IdToObject(type, id, player);
	SCRIPT_ASSERT(context, psObj, "Object not found!");
	bool value = context->argument(2).toBool();
	psObj->flags.set(flag, value);
	return QScriptValue();
}

//-- ## addSpotter(x, y, player, range, type, expiry)
//--
//-- Add an invisible viewer at a given position for given player that shows map in given range. ```type```
//-- is zero for vision reveal, or one for radar reveal. The difference is that a radar reveal can be obstructed
//-- by ECM jammers. ```expiry```, if non-zero, is the game time at which the spotter shall automatically be
//-- removed. The function returns a unique ID that can be used to remove the spotter with ```removeSpotter```. (3.2+ only)
//--
static QScriptValue js_addSpotter(QScriptContext *context, QScriptEngine *)
{
	int x = context->argument(0).toInt32();
	int y = context->argument(1).toInt32();
	int player = context->argument(2).toInt32();
	int range = context->argument(3).toInt32();
	bool radar = context->argument(4).toBool();
	uint32_t expiry = context->argument(5).toUInt32();
	uint32_t id = addSpotter(x, y, player, range, radar, expiry);
	return QScriptValue(id);
}

//-- ## removeSpotter(id)
//--
//-- Remove a spotter given its unique ID. (3.2+ only)
//--
static QScriptValue js_removeSpotter(QScriptContext *context, QScriptEngine *)
{
	uint32_t id = context->argument(0).toUInt32();
	removeSpotter(id);
	return QScriptValue();
}

//-- ## syncRandom(limit)
//--
//-- Generate a synchronized random number in range 0...(limit - 1) that will be the same if this function is
//-- run on all network peers in the same game frame. If it is called on just one peer (such as would be
//-- the case for AIs, for instance), then game sync will break. (3.2+ only)
//--
static QScriptValue js_syncRandom(QScriptContext *context, QScriptEngine *)
{
	uint32_t limit = context->argument(0).toInt32();
	return QScriptValue(gameRand(limit));
}

//-- ## syncRequest(req_id, x, y[, obj[, obj2]])
//--
//-- Generate a synchronized event request that is sent over the network to all clients and executed simultaneously.
//-- Must be caught in an eventSyncRequest() function. All sync requests must be validated when received, and always
//-- take care only to define sync requests that can be validated against cheating. (3.2+ only)
//--
static QScriptValue js_syncRequest(QScriptContext *context, QScriptEngine *)
{
	int32_t req_id = context->argument(0).toInt32();
	int32_t x = world_coord(context->argument(1).toInt32());
	int32_t y = world_coord(context->argument(2).toInt32());
	BASE_OBJECT *psObj = nullptr, *psObj2 = nullptr;
	if (context->argumentCount() > 3)
	{
		QScriptValue objVal = context->argument(3);
		int oid = objVal.property("id").toInt32();
		int oplayer = objVal.property("player").toInt32();
		OBJECT_TYPE otype = (OBJECT_TYPE)objVal.property("type").toInt32();
		psObj = IdToObject(otype, oid, oplayer);
		SCRIPT_ASSERT(context, psObj, "No such object id %d belonging to player %d", oid, oplayer);
	}
	if (context->argumentCount() > 4)
	{
		QScriptValue objVal = context->argument(4);
		int oid = objVal.property("id").toInt32();
		int oplayer = objVal.property("player").toInt32();
		OBJECT_TYPE otype = (OBJECT_TYPE)objVal.property("type").toInt32();
		psObj2 = IdToObject(otype, oid, oplayer);
		SCRIPT_ASSERT(context, psObj2, "No such object id %d belonging to player %d", oid, oplayer);
	}
	sendSyncRequest(req_id, x, y, psObj, psObj2);
	return QScriptValue();
}

//-- ## replaceTexture(old_filename, new_filename)
//--
//-- Replace one texture with another. This can be used to for example give buildings on a specific tileset different
//-- looks, or to add variety to the looks of droids in campaign missions. (3.2+ only)
//--
static QScriptValue js_replaceTexture(QScriptContext *context, QScriptEngine *)
{
	QString oldfile = context->argument(0).toString();
	QString newfile = context->argument(1).toString();
	replaceTexture(WzString::fromUtf8(oldfile.toUtf8().constData()), WzString::fromUtf8(newfile.toUtf8().constData()));
	return QScriptValue();
}

//-- ## fireWeaponAtLoc(weapon, x, y[, player])
//--
//-- Fires a weapon at the given coordinates (3.3+ only). The player is who owns the projectile.
//-- Please use fireWeaponAtObj() to damage objects as multiplayer and campaign
//-- may have different friendly fire logic for a few weapons (like the lassat).
//--
static QScriptValue js_fireWeaponAtLoc(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue weaponValue = context->argument(0);
	int weapon = getCompFromName(COMP_WEAPON, QStringToWzString(weaponValue.toString()));
	SCRIPT_ASSERT(context, weapon > 0, "No such weapon: %s", weaponValue.toString().toUtf8().constData());

	int xLocation = context->argument(1).toInt32();
	int yLocation = context->argument(2).toInt32();

	int player;
	if (context->argumentCount() > 3)
	{
		player = context->argument(3).toInt32();
	}
	else
	{
		player = engine->globalObject().property("me").toInt32();
	}

	Vector3i target;
	target.x = world_coord(xLocation);
	target.y = world_coord(yLocation);
	target.z = map_Height(xLocation, yLocation);

	WEAPON sWeapon;
	sWeapon.nStat = weapon;

	proj_SendProjectile(&sWeapon, nullptr, player, target, nullptr, true, 0);
	return QScriptValue();
}

//-- ## fireWeaponAtObj(weapon, game object[, player])
//--
//-- Fires a weapon at a game object (3.3+ only). The player is who owns the projectile.
//--
static QScriptValue js_fireWeaponAtObj(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue weaponValue = context->argument(0);
	int weapon = getCompFromName(COMP_WEAPON, QStringToWzString(weaponValue.toString()));
	SCRIPT_ASSERT(context, weapon > 0, "No such weapon: %s", weaponValue.toString().toUtf8().constData());

	BASE_OBJECT *psObj = nullptr;
	QScriptValue objVal = context->argument(1);
	int oid = objVal.property("id").toInt32();
	int oplayer = objVal.property("player").toInt32();
	OBJECT_TYPE otype = (OBJECT_TYPE)objVal.property("type").toInt32();
	psObj = IdToObject(otype, oid, oplayer);
	SCRIPT_ASSERT(context, psObj, "No such object id %d belonging to player %d", oid, oplayer);

	int player;
	if (context->argumentCount() > 3)
	{
		player = context->argument(3).toInt32();
	}
	else
	{
		player = engine->globalObject().property("me").toInt32();
	}

	Vector3i target = psObj->pos;

	WEAPON sWeapon;
	sWeapon.nStat = weapon;

	proj_SendProjectile(&sWeapon, nullptr, player, target, psObj, true, 0);
	return QScriptValue();
}

//-- ## changePlayerColour(player, colour)
//--
//-- Change a player's colour slot. The current player colour can be read from the ```playerData``` array. There are as many
//-- colour slots as the maximum number of players. (3.2.3+ only)
//--
static QScriptValue js_changePlayerColour(QScriptContext *context, QScriptEngine *)
{
	int player = context->argument(0).toInt32();
	int colour = context->argument(1).toInt32();
	setPlayerColour(player, colour);
	return QScriptValue();
}

//-- ## getMultiTechLevel()
//--
//-- Returns the current multiplayer tech level. (3.3+ only)
//--
static QScriptValue js_getMultiTechLevel(QScriptContext *context, QScriptEngine *)
{
	return QScriptValue(game.techLevel);
}

//-- ## setCampaignNumber(num)
//--
//-- Set the campaign number. (3.3+ only)
//--
static QScriptValue js_setCampaignNumber(QScriptContext *context, QScriptEngine *)
{
	int num = context->argument(0).toInt32();
	setCampaignNumber(num);
	return QScriptValue();
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
static QScriptValue js_setRevealStatus(QScriptContext *context, QScriptEngine *)
{
	bool status = context->argument(0).toBool();
	setRevealStatus(status);
	preProcessVisibility();
	return QScriptValue();
}

// ----------------------------------------------------------------------------------------
// Register functions with scripting system

bool unregisterFunctions(QScriptEngine *engine)
{
	GROUPMAP *psMap = groups.value(engine);
	int num = groups.remove(engine);
	delete psMap;
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
		QScriptEngine *engine = iter.key();
		for (LABELMAP::iterator i = labels.begin(); i != labels.end(); ++i)
		{
			LABEL l = i.value();
			if (l.type == SCRIPT_GROUP)
			{
				QScriptValue groupMembers = iter.key()->globalObject().property("groupSizes");
				groupMembers.setProperty(l.id, l.idlist.length(), QScriptValue::ReadOnly);
				for (QList<int>::iterator j = l.idlist.begin(); j != l.idlist.end(); j++)
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
	groups.insert(engine, psMap);

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
	engine->globalObject().setProperty("_", engine->newFunction(js_translate));
	engine->globalObject().setProperty("dump", engine->newFunction(js_dump));
	engine->globalObject().setProperty("syncRandom", engine->newFunction(js_syncRandom));
	engine->globalObject().setProperty("label", engine->newFunction(js_getObject)); // deprecated
	engine->globalObject().setProperty("getObject", engine->newFunction(js_getObject));
	engine->globalObject().setProperty("addLabel", engine->newFunction(js_addLabel));
	engine->globalObject().setProperty("removeLabel", engine->newFunction(js_removeLabel));
	engine->globalObject().setProperty("getLabel", engine->newFunction(js_getLabel));
	engine->globalObject().setProperty("enumLabels", engine->newFunction(js_enumLabels));
	engine->globalObject().setProperty("enumGateways", engine->newFunction(js_enumGateways));
	engine->globalObject().setProperty("enumTemplates", engine->newFunction(js_enumTemplates));
	engine->globalObject().setProperty("makeTemplate", engine->newFunction(js_makeTemplate));
	engine->globalObject().setProperty("setAlliance", engine->newFunction(js_setAlliance));
	engine->globalObject().setProperty("sendAllianceRequest", engine->newFunction(js_sendAllianceRequest));
	engine->globalObject().setProperty("setAssemblyPoint", engine->newFunction(js_setAssemblyPoint));
	engine->globalObject().setProperty("setSunPosition", engine->newFunction(js_setSunPosition));
	engine->globalObject().setProperty("setSunIntensity", engine->newFunction(js_setSunIntensity));
	engine->globalObject().setProperty("setWeather", engine->newFunction(js_setWeather));
	engine->globalObject().setProperty("setSky", engine->newFunction(js_setSky));
	engine->globalObject().setProperty("cameraSlide", engine->newFunction(js_cameraSlide));
	engine->globalObject().setProperty("cameraTrack", engine->newFunction(js_cameraTrack));
	engine->globalObject().setProperty("cameraZoom", engine->newFunction(js_cameraZoom));
	engine->globalObject().setProperty("resetArea", engine->newFunction(js_resetLabel)); // deprecated
	engine->globalObject().setProperty("resetLabel", engine->newFunction(js_resetLabel));
	engine->globalObject().setProperty("addSpotter", engine->newFunction(js_addSpotter));
	engine->globalObject().setProperty("removeSpotter", engine->newFunction(js_removeSpotter));
	engine->globalObject().setProperty("syncRequest", engine->newFunction(js_syncRequest));
	engine->globalObject().setProperty("replaceTexture", engine->newFunction(js_replaceTexture));
	engine->globalObject().setProperty("changePlayerColour", engine->newFunction(js_changePlayerColour));
	engine->globalObject().setProperty("setHealth", engine->newFunction(js_setHealth));
	engine->globalObject().setProperty("useSafetyTransport", engine->newFunction(js_useSafetyTransport));
	engine->globalObject().setProperty("restoreLimboMissionData", engine->newFunction(js_restoreLimboMissionData));
	engine->globalObject().setProperty("getMultiTechLevel", engine->newFunction(js_getMultiTechLevel));
	engine->globalObject().setProperty("setCampaignNumber", engine->newFunction(js_setCampaignNumber));
	engine->globalObject().setProperty("getMissionType", engine->newFunction(js_getMissionType));
	engine->globalObject().setProperty("getRevealStatus", engine->newFunction(js_getRevealStatus));
	engine->globalObject().setProperty("setRevealStatus", engine->newFunction(js_setRevealStatus));
	engine->globalObject().setProperty("autoSave", engine->newFunction(js_autoSave));

	// horrible hacks follow -- do not rely on these being present!
	engine->globalObject().setProperty("hackNetOff", engine->newFunction(js_hackNetOff));
	engine->globalObject().setProperty("hackNetOn", engine->newFunction(js_hackNetOn));
	engine->globalObject().setProperty("hackAddMessage", engine->newFunction(js_hackAddMessage));
	engine->globalObject().setProperty("hackRemoveMessage", engine->newFunction(js_hackRemoveMessage));
	engine->globalObject().setProperty("objFromId", engine->newFunction(js_objFromId));
	engine->globalObject().setProperty("hackGetObj", engine->newFunction(js_hackGetObj));
	engine->globalObject().setProperty("hackChangeMe", engine->newFunction(js_hackChangeMe));
	engine->globalObject().setProperty("hackAssert", engine->newFunction(js_hackAssert));
	engine->globalObject().setProperty("hackMarkTiles", engine->newFunction(js_hackMarkTiles));
	engine->globalObject().setProperty("receiveAllEvents", engine->newFunction(js_receiveAllEvents));
	engine->globalObject().setProperty("hackDoNotSave", engine->newFunction(js_hackDoNotSave));
	engine->globalObject().setProperty("hackPlayIngameAudio", engine->newFunction(js_hackPlayIngameAudio));
	engine->globalObject().setProperty("hackStopIngameAudio", engine->newFunction(js_hackStopIngameAudio));

	// General functions -- geared for use in AI scripts
	engine->globalObject().setProperty("debug", engine->newFunction(js_debug));
	engine->globalObject().setProperty("console", engine->newFunction(js_console));
	engine->globalObject().setProperty("clearConsole", engine->newFunction(js_clearConsole));
	engine->globalObject().setProperty("structureIdle", engine->newFunction(js_structureIdle));
	engine->globalObject().setProperty("enumStruct", engine->newFunction(js_enumStruct));
	engine->globalObject().setProperty("enumStructOffWorld", engine->newFunction(js_enumStructOffWorld));
	engine->globalObject().setProperty("enumDroid", engine->newFunction(js_enumDroid));
	engine->globalObject().setProperty("enumGroup", engine->newFunction(js_enumGroup));
	engine->globalObject().setProperty("enumFeature", engine->newFunction(js_enumFeature));
	engine->globalObject().setProperty("enumBlips", engine->newFunction(js_enumBlips));
	engine->globalObject().setProperty("enumSelected", engine->newFunction(js_enumSelected));
	engine->globalObject().setProperty("enumResearch", engine->newFunction(js_enumResearch));
	engine->globalObject().setProperty("enumRange", engine->newFunction(js_enumRange));
	engine->globalObject().setProperty("enumArea", engine->newFunction(js_enumArea));
	engine->globalObject().setProperty("getResearch", engine->newFunction(js_getResearch));
	engine->globalObject().setProperty("pursueResearch", engine->newFunction(js_pursueResearch));
	engine->globalObject().setProperty("findResearch", engine->newFunction(js_findResearch));
	engine->globalObject().setProperty("distBetweenTwoPoints", engine->newFunction(js_distBetweenTwoPoints));
	engine->globalObject().setProperty("newGroup", engine->newFunction(js_newGroup));
	engine->globalObject().setProperty("groupAddArea", engine->newFunction(js_groupAddArea));
	engine->globalObject().setProperty("groupAddDroid", engine->newFunction(js_groupAddDroid));
	engine->globalObject().setProperty("groupAdd", engine->newFunction(js_groupAdd));
	engine->globalObject().setProperty("groupSize", engine->newFunction(js_groupSize));
	engine->globalObject().setProperty("orderDroidLoc", engine->newFunction(js_orderDroidLoc));
	engine->globalObject().setProperty("playerPower", engine->newFunction(js_playerPower));
	engine->globalObject().setProperty("queuedPower", engine->newFunction(js_queuedPower));
	engine->globalObject().setProperty("isStructureAvailable", engine->newFunction(js_isStructureAvailable));
	engine->globalObject().setProperty("pickStructLocation", engine->newFunction(js_pickStructLocation));
	engine->globalObject().setProperty("droidCanReach", engine->newFunction(js_droidCanReach));
	engine->globalObject().setProperty("propulsionCanReach", engine->newFunction(js_propulsionCanReach));
	engine->globalObject().setProperty("terrainType", engine->newFunction(js_terrainType));
	engine->globalObject().setProperty("orderDroidBuild", engine->newFunction(js_orderDroidBuild));
	engine->globalObject().setProperty("orderDroidObj", engine->newFunction(js_orderDroidObj));
	engine->globalObject().setProperty("orderDroid", engine->newFunction(js_orderDroid));
	engine->globalObject().setProperty("buildDroid", engine->newFunction(js_buildDroid));
	engine->globalObject().setProperty("addDroid", engine->newFunction(js_addDroid));
	engine->globalObject().setProperty("addDroidToTransporter", engine->newFunction(js_addDroidToTransporter));
	engine->globalObject().setProperty("addFeature", engine->newFunction(js_addFeature));
	engine->globalObject().setProperty("componentAvailable", engine->newFunction(js_componentAvailable));
	engine->globalObject().setProperty("isVTOL", engine->newFunction(js_isVTOL));
	engine->globalObject().setProperty("safeDest", engine->newFunction(js_safeDest));
	engine->globalObject().setProperty("activateStructure", engine->newFunction(js_activateStructure));
	engine->globalObject().setProperty("chat", engine->newFunction(js_chat));
	engine->globalObject().setProperty("addBeacon", engine->newFunction(js_addBeacon));
	engine->globalObject().setProperty("removeBeacon", engine->newFunction(js_removeBeacon));
	engine->globalObject().setProperty("getDroidProduction", engine->newFunction(js_getDroidProduction));
	engine->globalObject().setProperty("getDroidLimit", engine->newFunction(js_getDroidLimit));
	engine->globalObject().setProperty("getExperienceModifier", engine->newFunction(js_getExperienceModifier));
	engine->globalObject().setProperty("setDroidLimit", engine->newFunction(js_setDroidLimit));
	engine->globalObject().setProperty("setCommanderLimit", engine->newFunction(js_setCommanderLimit));
	engine->globalObject().setProperty("setConstructorLimit", engine->newFunction(js_setConstructorLimit));
	engine->globalObject().setProperty("setExperienceModifier", engine->newFunction(js_setExperienceModifier));
	engine->globalObject().setProperty("getWeaponInfo", engine->newFunction(js_getWeaponInfo));
	engine->globalObject().setProperty("enumCargo", engine->newFunction(js_enumCargo));

	// Functions that operate on the current player only
	engine->globalObject().setProperty("centreView", engine->newFunction(js_centreView));
	engine->globalObject().setProperty("playSound", engine->newFunction(js_playSound));
	engine->globalObject().setProperty("gameOverMessage", engine->newFunction(js_gameOverMessage));

	// Global state manipulation -- not for use with skirmish AI (unless you want it to cheat, obviously)
	engine->globalObject().setProperty("setStructureLimits", engine->newFunction(js_setStructureLimits));
	engine->globalObject().setProperty("applyLimitSet", engine->newFunction(js_applyLimitSet));
	engine->globalObject().setProperty("setMissionTime", engine->newFunction(js_setMissionTime));
	engine->globalObject().setProperty("getMissionTime", engine->newFunction(js_getMissionTime));
	engine->globalObject().setProperty("setReinforcementTime", engine->newFunction(js_setReinforcementTime));
	engine->globalObject().setProperty("completeResearch", engine->newFunction(js_completeResearch));
	engine->globalObject().setProperty("completeAllResearch", engine->newFunction(js_completeAllResearch));
	engine->globalObject().setProperty("enableResearch", engine->newFunction(js_enableResearch));
	engine->globalObject().setProperty("setPower", engine->newFunction(js_setPower));
	engine->globalObject().setProperty("setPowerModifier", engine->newFunction(js_setPowerModifier));
	engine->globalObject().setProperty("setPowerStorageMaximum", engine->newFunction(js_setPowerStorageMaximum));
	engine->globalObject().setProperty("extraPowerTime", engine->newFunction(js_extraPowerTime));
	engine->globalObject().setProperty("setTutorialMode", engine->newFunction(js_setTutorialMode));
	engine->globalObject().setProperty("setDesign", engine->newFunction(js_setDesign));
	engine->globalObject().setProperty("enableTemplate", engine->newFunction(js_enableTemplate));
	engine->globalObject().setProperty("removeTemplate", engine->newFunction(js_removeTemplate));
	engine->globalObject().setProperty("setMiniMap", engine->newFunction(js_setMiniMap));
	engine->globalObject().setProperty("setReticuleButton", engine->newFunction(js_setReticuleButton));
	engine->globalObject().setProperty("setReticuleFlash", engine->newFunction(js_setReticuleFlash));
	engine->globalObject().setProperty("showReticuleWidget", engine->newFunction(js_showReticuleWidget));
	engine->globalObject().setProperty("showInterface", engine->newFunction(js_showInterface));
	engine->globalObject().setProperty("hideInterface", engine->newFunction(js_hideInterface));
	engine->globalObject().setProperty("addReticuleButton", engine->newFunction(js_removeReticuleButton)); // deprecated!!
	engine->globalObject().setProperty("removeReticuleButton", engine->newFunction(js_removeReticuleButton));
	engine->globalObject().setProperty("enableStructure", engine->newFunction(js_enableStructure));
	engine->globalObject().setProperty("makeComponentAvailable", engine->newFunction(js_makeComponentAvailable));
	engine->globalObject().setProperty("enableComponent", engine->newFunction(js_enableComponent));
	engine->globalObject().setProperty("allianceExistsBetween", engine->newFunction(js_allianceExistsBetween));
	engine->globalObject().setProperty("removeStruct", engine->newFunction(js_removeStruct));
	engine->globalObject().setProperty("removeObject", engine->newFunction(js_removeObject));
	engine->globalObject().setProperty("setScrollParams", engine->newFunction(js_setScrollLimits)); // deprecated!!
	engine->globalObject().setProperty("setScrollLimits", engine->newFunction(js_setScrollLimits));
	engine->globalObject().setProperty("getScrollLimits", engine->newFunction(js_getScrollLimits));
	engine->globalObject().setProperty("addStructure", engine->newFunction(js_addStructure));
	engine->globalObject().setProperty("getStructureLimit", engine->newFunction(js_getStructureLimit));
	engine->globalObject().setProperty("countStruct", engine->newFunction(js_countStruct));
	engine->globalObject().setProperty("countDroid", engine->newFunction(js_countDroid));
	engine->globalObject().setProperty("loadLevel", engine->newFunction(js_loadLevel));
	engine->globalObject().setProperty("setDroidExperience", engine->newFunction(js_setDroidExperience));
	engine->globalObject().setProperty("donateObject", engine->newFunction(js_donateObject));
	engine->globalObject().setProperty("donatePower", engine->newFunction(js_donatePower));
	engine->globalObject().setProperty("setNoGoArea", engine->newFunction(js_setNoGoArea));
	engine->globalObject().setProperty("startTransporterEntry", engine->newFunction(js_startTransporterEntry));
	engine->globalObject().setProperty("setTransporterExit", engine->newFunction(js_setTransporterExit));
	engine->globalObject().setProperty("setObjectFlag", engine->newFunction(js_setObjectFlag));
	engine->globalObject().setProperty("fireWeaponAtLoc", engine->newFunction(js_fireWeaponAtLoc));
	engine->globalObject().setProperty("fireWeaponAtObj", engine->newFunction(js_fireWeaponAtObj));

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
