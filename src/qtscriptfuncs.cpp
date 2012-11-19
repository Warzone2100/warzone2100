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
#include "lib/ivis_opengl/tex.h"

#include <QtScript/QScriptValue>
#include <QtCore/QStringList>

#include "action.h"
#include "console.h"
#include "design.h"
#include "display3d.h"
#include "map.h"
#include "mission.h"
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
#include "template.h"
#include "lighting.h"
#include "radar.h"
#include "frontend.h"
#include "loop.h"
#include "scriptextern.h"
#include "gateway.h"
#include "mapgrid.h"
#include "lighting.h"
#include "atmos.h"
#include "warcam.h"

#define FAKE_REF_LASSAT 999
#define ALL_PLAYERS -1
#define ALLIES -2
#define ENEMIES -3

// hack, this is used from scriptfuncs.cpp -- and we don't want to include any stinkin' wzscript headers here!
// TODO, move this stuff into a script common subsystem
extern bool structDoubleCheck(BASE_STATS *psStat,UDWORD xx,UDWORD yy, SDWORD maxBlockingTiles);
extern Vector2i positions[MAX_PLAYERS];
extern std::vector<Vector2i> derricks;
typedef QMap<DROID *, int> GROUPMAP;
typedef QMap<QScriptEngine *, GROUPMAP *> ENGINEMAP;
ENGINEMAP groups;

#define SCRIPT_ASSERT_PLAYER(_context, _player) \
	SCRIPT_ASSERT(_context, _player >= 0 && _player < MAX_PLAYERS, "Invalid player index %d", _player);

// ----------------------------------------------------------------------------------------
// Utility functions -- not called directly from scripts

void groupRemoveObject(const BASE_OBJECT *psObj)
{
	for (ENGINEMAP::iterator i = groups.begin(); i != groups.end(); ++i)
	{
		QScriptEngine *engine = i.key();
		GROUPMAP *psMap = i.value();
		if (psMap->contains((DROID *)psObj))
		{
			int groupId = psMap->take((DROID *)psObj); // take and remove item
			QScriptValue groupMembers = i.key()->globalObject().property("groupSizes");
			const int newValue = groupMembers.property(groupId).toInt32() - 1;
			ASSERT(newValue >= 0, "Bad group count in group %d (was %d)", groupId, newValue + 1);
			groupMembers.setProperty(groupId, newValue, QScriptValue::ReadOnly);
			if (newValue == 0)
			{
				triggerEventGroupEmpty(groupId, engine);
			}
		}
	}
}

static bool groupAddObject(DROID *psDroid, int groupId, QScriptEngine *engine)
{
	ASSERT_OR_RETURN(false, psDroid && groupId >= 0 && engine, "Bad parameter");
	GROUPMAP *psMap = groups.value(engine);
	if (!psMap->contains(psDroid))
	{
		QScriptValue groupMembers = engine->globalObject().property("groupSizes");
		int prev = groupMembers.property(QString::number(groupId)).toInt32();
		groupMembers.setProperty(QString::number(groupId), prev + 1, QScriptValue::ReadOnly);
		psMap->insert(psDroid, groupId);
		return true; // inserted
	}
	return false; // already had it
}

//;; \subsection{Research}
//;; Describes a research item. The following properties are defined:
//;; \begin{description}
//;; \item[power] Number of power points needed for starting the research.
//;; \item[points] Number of resarch points needed to complete the research.
//;; \item[started] A boolean saying whether or not this research has been started by current player or any of its allies.
//;; \item[done] A boolean saying whether or not this research has been completed.
//;; \item[name] A string containing the canonical name of the research.
//;; \end{description}
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
	value.setProperty("name", psResearch->pName);
	return value;
}

//;; \subsection{Structure\label{objects:structure}}
//;; Describes a structure (building). It inherits all the properties of the base object (see below).
//;; In addition, the following properties are defined:
//;; \begin{description}
//;; \item[status] The completeness status of the structure. It will be one of BEING_BUILT and BUILT.
//;; \item[type] The type will always be STRUCTURE.
//;; \item[cost] What it would cost to build this structure. (3.2+ only)
//;; \item[stattype] The stattype defines the type of structure. It will be one of HQ, FACTORY, POWER_GEN, RESOURCE_EXTRACTOR, 
//;; LASSAT, DEFENSE, WALL, RESEARCH_LAB, REPAIR_FACILITY, CYBORG_FACTORY, VTOL_FACTORY, REARM_PAD, SAT_UPLINK, GATE
//;; and COMMAND_CONTROL.
//;; \item[modules] If the stattype is set to one of the factories, POWER_GEN or RESEARCH_LAB, then this property is set to the
//;; number of module upgrades it has.
//;; \item[canHitAir] True if the structure has anti-air capabilities. (3.2+ only)
//;; \item[canHitGround] True if the structure has anti-ground capabilities. (3.2+ only)
//;; \item[isSensor] True if the structure has sensor ability. (3.2+ only)
//;; \item[isCB] True if the structure has counter-battery ability. (3.2+ only)
//;; \item[isRadarDetector] True if the structure has radar detector ability. (3.2+ only)
//;; \item[range] Maximum range of its weapons. (3.2+ only)
//;; \item[hasIndirect] One or more of the structure's weapons are indirect. (3.2+ only)
//;; \end{description}
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
			range = MAX((int)psWeap->longRange, range);
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
	value.setProperty("health", 100 * structureBody(psStruct) / MAX(1, psStruct->body), QScriptValue::ReadOnly);
	value.setProperty("cost", psStruct->pStructureType->powerToBuild, QScriptValue::ReadOnly);
	switch (psStruct->pStructureType->type) // don't bleed our source insanities into the scripting world
	{
	case REF_WALL:
	case REF_WALLCORNER:
	case REF_GATE:
		value.setProperty("stattype", (int)REF_WALL, QScriptValue::ReadOnly);
		break;
	case REF_BLASTDOOR:
	case REF_DEFENSE:
		if (isLasSat(psStruct->pStructureType))
		{
			value.setProperty("stattype", (int)FAKE_REF_LASSAT, QScriptValue::ReadOnly);
			break;
		}
		value.setProperty("stattype", (int)REF_DEFENSE, QScriptValue::ReadOnly);
		break;
	default:
		value.setProperty("stattype", (int)psStruct->pStructureType->type, QScriptValue::ReadOnly);
		break;
	}
	if (psStruct->pStructureType->type == REF_FACTORY || psStruct->pStructureType->type == REF_CYBORG_FACTORY
	    || psStruct->pStructureType->type == REF_VTOL_FACTORY)
	{
		FACTORY *psFactory = (FACTORY *)psStruct->pFunctionality;
		value.setProperty("modules", psFactory->capacity, QScriptValue::ReadOnly);
	}
	else if (psStruct->pStructureType->type == REF_RESEARCH)
	{
		value.setProperty("modules", ((RESEARCH_FACILITY *)psStruct->pFunctionality)->capacity, QScriptValue::ReadOnly);
	}
	else if (psStruct->pStructureType->type == REF_POWER_GEN)
	{
		value.setProperty("modules", ((POWER_GEN *)psStruct->pFunctionality)->capacity);
	}
	else
	{
		value.setProperty("modules", QScriptValue::NullValue);
	}
	return value;
}

//;; \subsection{Feature}
//;; Describes a feature (a \emph{game object} not owned by any player). It inherits all the properties of the base object (see below).
//;; In addition, the following properties are defined:
//;; \begin{description}
//;; \item[type] It will always be FEATURE.
//;; \item[damageable] Can this feature be damaged?
//;; \end{description}
QScriptValue convFeature(FEATURE *psFeature, QScriptEngine *engine)
{
	QScriptValue value = convObj(psFeature, engine);
	value.setProperty("health", 100 * psFeature->psStats->body / MAX(1, psFeature->body), QScriptValue::ReadOnly);
	value.setProperty("damageable", psFeature->psStats->damageable, QScriptValue::ReadOnly);
	return value;
}

//;; \subsection{Droid}
//;; Describes a droid. It inherits all the properties of the base object (see below).
//;; In addition, the following properties are defined:
//;; \begin{description}
//;; \item[type] It will always be DROID.
//;; \item[order] The current order of the droid. This is its plan. The following orders are defined:
//;;  \begin{description}
//;;   \item[DORDER_ATTACK] Order a droid to attack something.
//;;   \item[DORDER_MOVE] Order a droid to move somewhere.
//;;   \item[DORDER_SCOUT] Order a droid to move somewhere and stop to attack anything on the way.
//;;   \item[DORDER_BUILD] Order a droid to build something.
//;;   \item[DORDER_HELPBUILD] Order a droid to help build something.
//;;   \item[DORDER_LINEBUILD] Order a droid to build something repeatedly in a line.
//;;   \item[DORDER_REPAIR] Order a droid to repair something.
//;;   \item[DORDER_RETREAT] Order a droid to retreat back to HQ.
//;;   \item[DORDER_PATROL] Order a droid to patrol.
//;;   \item[DORDER_DEMOLISH] Order a droid to demolish something.
//;;   \item[DORDER_EMBARK] Order a droid to embark on a transport.
//;;   \item[DORDER_DISEMBARK] Order a transport to disembark its units at the given position.
//;;   \item[DORDER_FIRESUPPORT] Order a droid to fire at whatever the target sensor is targeting. (3.2+ only)
//;;   \item[DORDER_STOP] Order a droid to stop whatever it is doing. (3.2+ only)
//;;   \item[DORDER_RTR] Order a droid to return for repairs. (3.2+ only)
//;;   \item[DORDER_RTB] Order a droid to return to base. (3.2+ only)
//;;   \item[DORDER_HOLD] Order a droid to hold its position. (3.2+ only)
//;;   \item[DORDER_REARM] Order a VTOL droid to rearm. If given a target, will go to specified rearm pad. If not, will go to nearest rearm pad. (3.2+ only)
//;;   \item[DORDER_OBSERVE] Order a droid to keep a target in sensor view. (3.2+ only)
//;;   \item[DORDER_RECOVER] Order a droid to pick up something. (3.2+ only)
//;;  \end{description}
//;; \item[action] The current action of the droid. This is how it intends to carry out its plan. The
//;; C++ code may change the action frequently as it tries to carry out its order. You never want to set
//;; the action directly, but it may be interesting to look at what it currently is.
//;; \item[droidType] The droid's type. The following types are defined:
//;;  \begin{description}
//;;   \item[DROID_CONSTRUCT] Trucks and cyborg constructors.
//;;   \item[DROID_WEAPON] Droids with weapon turrets, except cyborgs.
//;;   \item[DROID_PERSON] Non-cyborg two-legged units, like scavengers.
//;;   \item[DROID_REPAIR] Units with repair turret, including repair cyborgs.
//;;   \item[DROID_SENSOR] Units with sensor turret.
//;;   \item[DROID_ECM] Unit with ECM jammer turret.
//;;   \item[DROID_CYBORG] Cyborgs with weapons.
//;;   \item[DROID_TRANSPORTER] Cyborg transporter.
//;;   \item[DROID_SUPERTRANSPORTER] Droid transporter.
//;;   \item[DROID_COMMAND] Commanders.
//;;  \end{description}
//;; \item[group] The group this droid is member of. This is a numerical ID. If not a member of any group, will be set to \emph{null}.
//;; \item[armed] The percentage of weapon capability that is fully armed. Will be \emph{null} for droids other than VTOLs.
//;; \item[experience] Amount of experience this droid has, based on damage it has dealt to enemies.
//;; \item[cost] What it would cost to build the droid. (3.2+ only)
//;; \item[isVTOL] True if the droid is VTOL. (3.2+ only)
//;; \item[canHitAir] True if the droid has anti-air capabilities. (3.2+ only)
//;; \item[canHitGround] True if the droid has anti-ground capabilities. (3.2+ only)
//;; \item[isSensor] True if the droid has sensor ability. (3.2+ only)
//;; \item[isCB] True if the droid has counter-battery ability. (3.2+ only)
//;; \item[isRadarDetector] True if the droid has radar detector ability. (3.2+ only)
//;; \item[hasIndirect] One or more of the droid's weapons are indirect. (3.2+ only)
//;; \item[range] Maximum range of its weapons. (3.2+ only)
//;; \item[body] The body component of the droid. (3.2+ only)
//;; \item[propulsion] The propulsion component of the droid. (3.2+ only)
//;; \end{description}
QScriptValue convDroid(DROID *psDroid, QScriptEngine *engine)
{
	bool aa = false;
	bool ga = false;
	bool indirect = false;
	int range = -1;
	for (int i = 0; i < psDroid->numWeaps; i++)
	{
		if (psDroid->asWeaps[i].nStat)
		{
			WEAPON_STATS *psWeap = &asWeaponStats[psDroid->asWeaps[i].nStat];
			aa = aa || psWeap->surfaceToAir & SHOOT_IN_AIR;
			ga = ga || psWeap->surfaceToAir & SHOOT_ON_GROUND;
			indirect = indirect || psWeap->movementModel == MM_INDIRECT || psWeap->movementModel == MM_HOMINGINDIRECT;
			range = MAX((int)psWeap->longRange, range);
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
	case DROID_DEFAULT:
	case DROID_CYBORG_SUPER:
		type = DROID_WEAPON; break;
	case DROID_CYBORG_REPAIR:
		type = DROID_REPAIR; break;
	default:
		break;
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
	value.setProperty("body", asBodyStats[psDroid->asBits[COMP_BODY].nStat].pName);
	value.setProperty("propulsion", asBodyStats[psDroid->asBits[COMP_PROPULSION].nStat].pName);
	if (isVtolDroid(psDroid))
	{
		value.setProperty("armed", 100.0 / (double)MAX(asWeaponStats[psDroid->asWeaps[0].nStat].numRounds, 1)
		                  * (double)psDroid->asWeaps[0].ammo);
		if (psDroid->asWeaps[0].ammo > asWeaponStats[psDroid->asWeaps[0].nStat].numRounds)
			debug(LOG_ERROR, "%s has %d and %d", objInfo(psDroid), psDroid->asWeaps[0].ammo, asWeaponStats[psDroid->asWeaps[0].nStat].numRounds);
	}
	else
	{
		value.setProperty("armed", QScriptValue::NullValue);
	}
	GROUPMAP *psMap = groups.value(engine);
	if (psMap->contains(psDroid))
	{
		int group = psMap->value(psDroid, -1);
		ASSERT(group >= 0, "%s is member of invalid group %d", objInfo(psDroid), group);
		value.setProperty("group", group, QScriptValue::ReadOnly);
	}
	else
	{
		value.setProperty("group", QScriptValue::NullValue);
	}
	return value;
}

//;; \subsection{Base Object}
//;; Describes a basic object. It will always be a droid, structure or feature, but sometimes
//;; the difference does not matter, and you can treat any of them simply as a basic object.
//;; The following properties are defined:
//;; \begin{description}
//;; \item[type] It will be one of DROID, STRUCTURE or FEATURE.
//;; \item[id] The unique ID of this object.
//;; \item[x] X position of the object in tiles.
//;; \item[y] Y position of the object in tiles.
//;; \item[z] Z (height) position of the object in tiles.
//;; \item[player] The player owning this object.
//;; \item[selected] A boolean saying whether 'selectedPlayer' has selected this object.
//;; \item[name] A user-friendly name for this object.
//;; \item[health] Percentage that this object is damaged (where 100% means not damaged at all).
//;; \item[armour] Amount of armour points that protect against kinetic weapons.
//;; \item[thermal] Amount of thermal protection that protect against heat based weapons.
//;; \item[born] The game time at which this object was produced or came into the world. (3.2+ only)
//;; \end{description}
QScriptValue convObj(BASE_OBJECT *psObj, QScriptEngine *engine)
{
	QScriptValue value = engine->newObject();
	ASSERT_OR_RETURN(value, psObj, "No object for conversion");
	value.setProperty("id", psObj->id, QScriptValue::ReadOnly);
	value.setProperty("x", map_coord(psObj->pos.x), QScriptValue::ReadOnly);
	value.setProperty("y", map_coord(psObj->pos.y), QScriptValue::ReadOnly);
	value.setProperty("z", map_coord(psObj->pos.z), QScriptValue::ReadOnly);
	value.setProperty("player", psObj->player, QScriptValue::ReadOnly);
	value.setProperty("armour", psObj->armour[WC_KINETIC], QScriptValue::ReadOnly);
	value.setProperty("thermal", psObj->armour[WC_HEAT], QScriptValue::ReadOnly);
	value.setProperty("type", psObj->type, QScriptValue::ReadOnly);
	value.setProperty("selected", psObj->selected, QScriptValue::ReadOnly);
	value.setProperty("name", objInfo(psObj), QScriptValue::ReadOnly);
	value.setProperty("born", psObj->born, QScriptValue::ReadOnly);
	return value;
}

QScriptValue convTemplate(DROID_TEMPLATE *psTempl, QScriptEngine *engine)
{
	QScriptValue value = engine->newObject();
	ASSERT_OR_RETURN(value, psTempl, "No object for conversion");
	value.setProperty("id", psTempl->multiPlayerID, QScriptValue::ReadOnly);
	value.setProperty("name", psTempl->pName, QScriptValue::ReadOnly);
	value.setProperty("points", psTempl->buildPoints, QScriptValue::ReadOnly);
	value.setProperty("power", psTempl->powerPoints, QScriptValue::ReadOnly);
	value.setProperty("droidType", psTempl->droidType, QScriptValue::ReadOnly);
	value.setProperty("body", (asBodyStats + psTempl->asParts[COMP_BODY])->pName, QScriptValue::ReadOnly);
	value.setProperty("propulsion", (asPropulsionStats + psTempl->asParts[COMP_PROPULSION])->pName, QScriptValue::ReadOnly);
	value.setProperty("brain", (asBrainStats + psTempl->asParts[COMP_BRAIN])->pName, QScriptValue::ReadOnly);
	value.setProperty("repair", (asRepairStats + psTempl->asParts[COMP_REPAIRUNIT])->pName, QScriptValue::ReadOnly);
	value.setProperty("ecm", (asECMStats + psTempl->asParts[COMP_ECM])->pName, QScriptValue::ReadOnly);
	value.setProperty("sensor", (asSensorStats + psTempl->asParts[COMP_SENSOR])->pName, QScriptValue::ReadOnly);
	value.setProperty("construct", (asConstructStats + psTempl->asParts[COMP_CONSTRUCT])->pName, QScriptValue::ReadOnly);
	QScriptValue weaponlist = engine->newArray(psTempl->numWeaps);
	for (int j = 0; j < psTempl->numWeaps; j++)
	{
		weaponlist.setProperty(j, QScriptValue((asWeaponStats + psTempl->asWeaps[j])->pName), QScriptValue::ReadOnly);
	}
	value.setProperty("weapons", weaponlist);
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
// Group system
//

bool loadGroup(QScriptEngine *engine, int groupId, int droidId)
{
	DROID *psDroid = IdToDroid(droidId, ANYPLAYER);
	return groupAddObject(psDroid, groupId, engine);
}

bool saveGroups(WzConfig &ini, QScriptEngine *engine)
{
	// Save group info as a list of group memberships for each droid
	GROUPMAP *psMap = groups.value(engine);	
	for (GROUPMAP::const_iterator i = psMap->constBegin(); i != psMap->constEnd(); ++i)
	{
		QStringList value;
		DROID *psDroid = i.key();
		if (ini.contains(QString::number(psDroid->id)))
		{
			value.push_back(ini.value(QString::number(psDroid->id)).toString());
		}
		value.push_back(QString::number(i.value()));
		ini.setValue(QString::number(psDroid->id), value);
	}
	return true;
}

// ----------------------------------------------------------------------------------------
// Label system (function defined in qtscript.h header)
//

struct labeltype { Vector2i p1, p2; int id; int type; int player; };

static QHash<QString, labeltype> labels;

// Load labels
bool loadLabels(const char *filename)
{
	int groupidx = -1;

	if (!PHYSFS_exists(filename))
	{
		debug(LOG_SAVE, "No %s found -- not adding any labels", filename);
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
	debug(LOG_SAVE, "Loading %d labels...", list.size());
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
			p.type = SCRIPT_POSITION;
			p.player = -1;
			p.id = -1;
			labels.insert(label, p);
		}
		else if (list[i].startsWith("area"))
		{
			p.p1 = ini.vector2i("pos1");
			p.p2 = ini.vector2i("pos2");
			p.type = SCRIPT_AREA;
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
		else if (list[i].startsWith("group"))
		{
			p.id = groupidx--;
			p.type = SCRIPT_GROUP;
			p.player = ini.value("player").toInt();
			QStringList list = ini.value("members").toStringList();
			// load the group data into every scripting context, with the same negative group id
			for (ENGINEMAP::iterator iter = groups.begin(); iter != groups.end(); ++iter)
			{
				QScriptEngine *engine = iter.key();
				QScriptValue groupMembers = iter.key()->globalObject().property("groupSizes");
				groupMembers.setProperty(p.id, list.length(), QScriptValue::ReadOnly);
				for (QStringList::iterator j = list.begin(); j != list.end(); j++)
				{
					int id = (*j).toInt();
					DROID *psDroid = IdToDroid(id, p.player);
					groupAddObject(psDroid, p.id, engine);
				}
			}
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
		if (l.type == SCRIPT_POSITION)
		{
			ini.beginGroup("position_" + QString::number(c[0]++));
			ini.setVector2i("pos", l.p1);
			ini.setValue("label", key);
			ini.endGroup();
		}
		else if (l.type == SCRIPT_AREA)
		{
			ini.beginGroup("area_" + QString::number(c[1]++));
			ini.setVector2i("pos1", l.p1);
			ini.setVector2i("pos2", l.p2);
			ini.setValue("label", key);
			ini.endGroup();
		}
		else if (l.type == SCRIPT_GROUP)
		{
			ini.beginGroup("group_" + QString::number(c[1]++));
			ini.setValue("player", l.player);
			QStringList list;
			// there is no way for a script to add or change labelled groups, so just
			// take them from one scripting context, and save them as they are
			for (ENGINEMAP::iterator iter = groups.begin(); iter != groups.end(); ++iter)
			{
				GROUPMAP *psMap = iter.value();
				for (GROUPMAP::iterator j = psMap->begin(); j != psMap->end(); ++j)
				{
					int value = j.value();
					DROID *key = j.key();
					if (value < 0)
					{
						list += QString::number(key->id);
					}
				}
				break; // only do one context
			}
			ini.setValue("members", list);
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

//-- \subsection{enumLabels()}
//-- Returns a string list of labels that exist for this map. (3.2+ only)
static QScriptValue js_enumLabels(QScriptContext *, QScriptEngine *engine)
{
	QStringList matches = labels.keys();
	QScriptValue result = engine->newArray(matches.size());
	for (int i = 0; i < matches.size(); i++)
	{
		result.setProperty(i, QScriptValue(matches[i]), QScriptValue::ReadOnly);
	}
	return result;
}

//-- \subsection{label(key)}
//-- Fetch something denoted by a label. A label refers to an area, a position or a \emph{game object} on 
//-- the map defined using the map editor and stored together with the map. The only argument
//-- is a text label. The function returns an object that has a type variable defining what it
//-- is (in case this is unclear). This type will be one of DROID, STRUCTURE, FEATURE, AREA
//-- and POSITION. The AREA has defined 'x', 'y', 'x2', and 'y2', while POSITION has only
//-- defined 'x' and 'y'.
static QScriptValue js_label(QScriptContext *context, QScriptEngine *engine)
{
	QString label = context->argument(0).toString();
	QScriptValue ret = engine->newObject();
	if (labels.contains(label))
	{
		labeltype p = labels.value(label);
		if (p.type == SCRIPT_AREA || p.type == SCRIPT_POSITION)
		{
			ret.setProperty("x", map_coord(p.p1.x), QScriptValue::ReadOnly);
			ret.setProperty("y", map_coord(p.p1.y), QScriptValue::ReadOnly);
			ret.setProperty("type", p.type, QScriptValue::ReadOnly);
		}
		if (p.type == SCRIPT_AREA)
		{
			ret.setProperty("x2", map_coord(p.p2.x), QScriptValue::ReadOnly);
			ret.setProperty("xy", map_coord(p.p2.y), QScriptValue::ReadOnly);
			ret.setProperty("type", p.type, QScriptValue::ReadOnly);
		}
		else if (p.type == SCRIPT_GROUP)
		{
			ret.setProperty("id", p.id, QScriptValue::ReadOnly);
			ret.setProperty("type", p.type, QScriptValue::ReadOnly);
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

//-- \subsection{enumBlips(player)}
//-- Return an array containing all the non-transient radar blips that the given player 
//-- can see. This includes sensors revealed by radar detectors, as well as ECM jammers.
//-- It does not include units going out of view.
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
		result.setProperty(i, v);
	}
	return result;
}

//-- \subsection{enumGateways()}
//-- Return an array containing all the gateways on the current map. The array contains object with the properties
//-- x1, y1, x2 and y2.
static QScriptValue js_enumGateways(QScriptContext *, QScriptEngine *engine)
{
	QScriptValue result = engine->newArray(gwNumGateways());
	int i = 0;
	for (GATEWAY *psGateway = gwGetGateways(); psGateway; psGateway = psGateway->psNext)
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

//-- \subsection{enumTemplate(player)}
//-- Return an array containing all the buildable templates for the given player.
static QScriptValue js_enumTemplates(QScriptContext *context, QScriptEngine *engine)
{
	int player = context->argument(0).toInt32();
	int count = 0;
	for (DROID_TEMPLATE *psCurr = apsDroidTemplates[player]; psCurr != NULL; psCurr = psCurr->psNext) count++;
	QScriptValue result = engine->newArray(count);
	count = 0;
	for (DROID_TEMPLATE *psCurr = apsDroidTemplates[player]; psCurr != NULL; psCurr = psCurr->psNext)
	{
		result.setProperty(count++, convTemplate(psCurr, engine));
	}
	return result;
}

//-- \subsection{enumGroup(group)}
//-- Return an array containing all the droid members of a given group.
static QScriptValue js_enumGroup(QScriptContext *context, QScriptEngine *engine)
{
	int groupId = context->argument(0).toInt32();
	QList<DROID *> matches;
	GROUPMAP *psMap = groups.value(engine);

	for (GROUPMAP::const_iterator i = psMap->constBegin(); i != psMap->constEnd(); ++i)
	{
		if (i.value() == groupId)
		{
			matches.push_back(i.key());
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

//-- \subsection{newGroup()}
//-- Allocate a new group. Returns its numerical ID. DEPRECATED.
static QScriptValue js_newGroup(QScriptContext *, QScriptEngine *engine)
{
	static int i = 1; // group zero reserved
	return QScriptValue(i++);
}

//-- \subsection{activateStructure(structure, [target[, ability]])}
//-- Activate a special ability on a structure. Currently only works on the lassat.
//-- The lassat needs a target.
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
	BASE_OBJECT *psObj = IdToPointer(oid, oplayer);
	SCRIPT_ASSERT(context, psObj, "No such object id %d belonging to player %d", oid, oplayer);
	orderStructureObj(player, psObj);
	return QScriptValue(true);
}

//-- \subsection{findResearch(research)}
//-- Return list of research items remaining to research for the given research item. (3.2+ only)
static QScriptValue js_findResearch(QScriptContext *context, QScriptEngine *engine)
{
	QList<RESEARCH *> list;
	QString resName = context->argument(0).toString();
	int player = engine->globalObject().property("me").toInt32();
	RESEARCH *psTarget = getResearch(resName.toUtf8().constData());
	SCRIPT_ASSERT(context, psTarget, "No such research: %s", resName.toUtf8().constData());
	PLAYER_RESEARCH *plrRes = &asPlayerResList[player][psTarget->index];
	if (IsResearchStartedPending(plrRes) || IsResearchCompleted(plrRes))
	{
		return engine->newArray(0); // return empty array
	}
	// Go down the requirements list for the desired tech
	QList<RESEARCH *> reslist;
	RESEARCH *cur = psTarget;
	while (cur)
	{
		if (!(asPlayerResList[player][cur->index].ResearchStatus & RESEARCHED))
		{
			debug(LOG_SCRIPT, "Added research in %d's %s for %s", player, cur->pName, psTarget->pName);
			list.append(cur);
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
	QScriptValue retval = engine->newArray(list.size());
	for (int i = 0; i < list.size(); i++)
	{
		retval.setProperty(i, convResearch(list[i], engine, i));
	}
	return retval;
}	

//-- \subsection{pursueResearch(lab, research)}
//-- Start researching the first available technology on the way to the given technology.
//-- First parameter is the structure to research in, which must be a research lab. The
//-- second parameter is the technology to pursue, as a text string as defined in "research.txt".
//-- The second parameter may also be an array of such strings. The first technology that has
//-- not yet been researched in that list will be pursued.
static QScriptValue js_pursueResearch(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue structVal = context->argument(0);
	int id = structVal.property("id").toInt32();
	int player = structVal.property("player").toInt32();
	STRUCTURE *psStruct = IdToStruct(id, player);
	SCRIPT_ASSERT(context, psStruct, "No such structure id %d belonging to player %d", id, player);
	QScriptValue list = context->argument(1);
	RESEARCH *psResearch = NULL;  // Dummy initialisation.
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
	SCRIPT_ASSERT(context, psResLab->psSubject == NULL, "Research lab not ready");
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
				if (aiCheckAlliances(player, i) || i == player)
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
				sprintf(sTemp, "player:%d starts topic from script: %s", player, cur->pName);
				NETlogEntry(sTemp, SYNC_FLAG, 0);
#endif
				debug(LOG_SCRIPT, "Started research in %d's %s(%d) of %s", player, 
				      objInfo(psStruct), psStruct->id, cur->pName);
				return QScriptValue(true);
			}
		}
		RESEARCH *prev = cur;
		cur = NULL;
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
		ASSERT_OR_RETURN(QScriptValue(false), ++iterations < asResearch.size()*100 || !cur, "Possible cyclic dependencies in prerequisites, possibly of research \"%s\".", cur->pName);
	}
	debug(LOG_SCRIPT, "No research topic found for %s(%d)", objInfo(psStruct), psStruct->id);
	return QScriptValue(false); // none found
}

//-- \subsection{getResearch(research)}
//-- Fetch information about a given technology item, given by a string that matches
//-- its definition in "research.txt". If not found, returns null.
static QScriptValue js_getResearch(QScriptContext *context, QScriptEngine *engine)
{
	int player = engine->globalObject().property("me").toInt32();
	QString resName = context->argument(0).toString();
	RESEARCH *psResearch = getResearch(resName.toUtf8().constData());
	if (!psResearch)
	{
		return QScriptValue::NullValue;
	}
	return convResearch(psResearch, engine, player);
}

//-- \subsection{enumResearch()}
//-- Returns an array of all research objects that are currently and immediately available for research.
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

//-- \subsection{componentAvailable(component type, component name)}
//-- Checks whether a given component is available to the current player.
static QScriptValue js_componentAvailable(QScriptContext *context, QScriptEngine *engine)
{
	int player = engine->globalObject().property("me").toInt32();
	COMPONENT_TYPE comp = (COMPONENT_TYPE)context->argument(0).toInt32();
	QString compName = context->argument(1).toString();
	int result = getCompFromName(comp, compName.toUtf8().constData());
	SCRIPT_ASSERT(context, result >= 0, "No such component: %s", compName.toUtf8().constData());
	bool avail = apCompLists[player][comp][result] == AVAILABLE;
	return QScriptValue(avail);
}

//-- \subsection{addFeature(x, y, name)}
//-- Create and place a feature at the given x, y position. Will cause a desync in multiplayer.
//-- Returns the created game object on success, null otherwise. (3.2+ only)
static QScriptValue js_addFeature(QScriptContext *context, QScriptEngine *engine)
{
	int x = context->argument(0).toInt32();
	int y = context->argument(1).toInt32();
	QString featName = context->argument(2).toString();
	int feature = getFeatureStatFromName(featName.toUtf8().constData());
	FEATURE_STATS *psStats = &asFeatureStats[feature];
	for (FEATURE *psFeat = apsFeatureLists[0]; psFeat; psFeat = psFeat->psNext)
	{
		SCRIPT_ASSERT(context, map_coord(psFeat->pos.x) != x || map_coord(psFeat->pos.y) != y, 
		              "Building feature on tile already occupied");
		return QScriptValue::NullValue;
	}
	return convFeature(buildFeature(psStats, world_coord(x), world_coord(y), false), engine);
}

//-- \subsection{addDroid(player, x, y, name, body, propulsion, reserved, droid type, turrets...)}
//-- Create and place a droid at the given x, y position as belonging to the given player, built with
//-- the given components. Currently does not support placing droids in multiplayer, doing so will
//-- cause a desync. Returns a boolean that is true on success.
static QScriptValue js_addDroid(QScriptContext *context, QScriptEngine *engine)
{
	int player = context->argument(0).toInt32();
	SCRIPT_ASSERT_PLAYER(context, player);
	int x = context->argument(1).toInt32();
	int y = context->argument(2).toInt32();
	QString templName = context->argument(3).toString();
	DROID_TEMPLATE sTemplate, *psTemplate = &sTemplate;
	DROID_TYPE droidType = (DROID_TYPE)context->argument(7).toInt32();
	int numTurrets = context->argumentCount() - 8; // anything beyond first eight parameters, are turrets
	COMPONENT_TYPE compType = COMP_NUMCOMPONENTS;
	
	memset(psTemplate->asParts, 0, sizeof(psTemplate->asParts)); // reset to defaults
	memset(psTemplate->asWeaps, 0, sizeof(psTemplate->asWeaps));
	int body = getCompFromName(COMP_BODY, context->argument(4).toString().toUtf8().constData());
	int propulsion = getCompFromName(COMP_PROPULSION, context->argument(5).toString().toUtf8().constData());
	if (body < 0)
	{
		debug(LOG_ERROR, "Wanted to build %s, but body type unavailable", templName.toUtf8().constData());
		return QScriptValue(false); // no component available
	}
	if (propulsion < 0)
	{
		debug(LOG_ERROR, "Wanted to build %s, but propulsion type unavailable", templName.toUtf8().constData());
		return QScriptValue(false); // no component available
	}
	psTemplate->asParts[COMP_BODY] = body;
	psTemplate->asParts[COMP_PROPULSION] = propulsion;
	psTemplate->numWeaps = 0;
	for (int i = 0; i < numTurrets; i++)
	{
		context->argument(8 + i).toString();
		int j;
		switch (droidType)
		{
		case DROID_PERSON:
		case DROID_WEAPON:
		case DROID_CYBORG:
		case DROID_TRANSPORTER:
		case DROID_SUPERTRANSPORTER:
		case DROID_DEFAULT:
		case DROID_CYBORG_SUPER:
			j = getCompFromName(COMP_WEAPON, context->argument(8 + i).toString().toUtf8().constData());
			if (j < 0)
			{
				debug(LOG_SCRIPT, "Wanted to build %s at (%d, %d), but no weapon unavailable", 
				      templName.toUtf8().constData(), x, y);
				return QScriptValue(false);
			}
			psTemplate->asWeaps[i] = j;
			psTemplate->numWeaps++;
			continue;
		case DROID_CYBORG_CONSTRUCT:
		case DROID_CONSTRUCT:
			compType = COMP_CONSTRUCT;
			break;
		case DROID_COMMAND:
			compType = COMP_BRAIN;
			psTemplate->numWeaps = 1; // hack, necessary to pass intValidTemplate
			break;
		case DROID_REPAIR:
		case DROID_CYBORG_REPAIR:
			compType = COMP_REPAIRUNIT;
			break;
		case DROID_ECM:
			compType = COMP_ECM;
			break;
		case DROID_SENSOR:
			compType = COMP_SENSOR;
			break;
		case DROID_ANY:
			break; // wtf
		}
		j = getCompFromName(compType, context->argument(8 + i).toString().toUtf8().constData());
		if (j < 0)
		{
			debug(LOG_SCRIPT, "Wanted to build %s, but turret unavailable", templName.toUtf8().constData());
			return QScriptValue(false);
		}
		psTemplate->asParts[compType] = j;
	}
	psTemplate->droidType = droidType; // may be overwritten by the call below
	bool valid = intValidTemplate(psTemplate, templName.toUtf8().constData(), true);
	if (valid)
	{
		bool oldMulti = bMultiMessages;
		bMultiMessages = false; // ugh, fixme
		DROID *psDroid = buildDroid(psTemplate, world_coord(x), world_coord(y), player, false, NULL);
		if (psDroid)
		{
			addDroid(psDroid, apsDroidLists);
			debug(LOG_LIFE, "Created droid %s by script for player %d: %u", objInfo(psDroid), player, psDroid->id);
		}
		else
		{
			debug(LOG_ERROR, "Invalid droid %s", templName.toUtf8().constData());
		}
		bMultiMessages = oldMulti; // ugh
	}
	else
	{
		debug(LOG_ERROR, "Invalid template %s", templName.toUtf8().constData());
	}
	return QScriptValue(valid);
}

static int get_first_available_component(STRUCTURE *psFactory, const QScriptValue &list, COMPONENT_TYPE type)
{
	const int player = psFactory->player;
	const int capacity = psFactory->pFunctionality->factory.capacity; // check body size

	if (list.isArray())
	{
		int length = list.property("length").toInt32();
		int k;
		for (k = 0; k < length; k++)
		{
			QString compName = list.property(k).toString();
			int result = getCompFromName(type, compName.toUtf8().constData());
			if (result >= 0 && apCompLists[player][type][result] == AVAILABLE
			    && (type != COMP_BODY || asBodyStats[result].size <= capacity))
			{
				return result; // found one!
			}
			if (result < 0)
			{
				debug(LOG_ERROR, "No such component: %s", compName.toUtf8().constData());
			}
		}
	}
	else if (list.isString())
	{
		int result = getCompFromName(type, list.toString().toUtf8().constData());
		if (result >= 0 && apCompLists[player][type][result] == AVAILABLE
		    && (type != COMP_BODY || asBodyStats[result].size <= capacity))
		{
			return result; // found it!
		}
		if (result < 0)
		{
			debug(LOG_ERROR, "No such component: %s", list.toString().toUtf8().constData());
		}
	}
	return -1; // no available component found in list
}

//-- \subsection{buildDroid(factory, name, body, propulsion, reserved, reserved, turrets...)}
//-- Start factory production of new droid with the given name, body, propulsion and turrets.
//-- The reserved parameter should be passed \emph{null} for now. The components can be
//-- passed as ordinary strings, or as a list of strings. If passed as a list, the first available
//-- component in the list will be used. The second reserved parameter used to be a droid type.
//-- It is now unused and in 3.2+ should be passed \emph{null}, while in 3.1 it should be the
//-- droid type to be built. Returns a boolean that is true if production was started.
// TODO - fix memory leaks
static QScriptValue js_buildDroid(QScriptContext *context, QScriptEngine *engine)
{
	const int firstTurret = 6; // index position of first turret parameter
	QScriptValue structVal = context->argument(0);
	int id = structVal.property("id").toInt32();
	int player = structVal.property("player").toInt32();
	STRUCTURE *psStruct = IdToStruct(id, player);
	SCRIPT_ASSERT(context, psStruct, "No such structure id %d belonging to player %d", id, player);
	SCRIPT_ASSERT(context, (psStruct->pStructureType->type == REF_FACTORY || psStruct->pStructureType->type == REF_CYBORG_FACTORY
		       || psStruct->pStructureType->type == REF_VTOL_FACTORY), "Structure %s is not a factory", objInfo(psStruct));
	QString templName = context->argument(1).toString();
	DROID_TEMPLATE *psTemplate = new DROID_TEMPLATE;
	int numTurrets = context->argumentCount() - firstTurret; // anything beyond first six parameters, are turrets
	int result;

	memset(psTemplate->asParts, 0, sizeof(psTemplate->asParts)); // reset to defaults
	memset(psTemplate->asWeaps, 0, sizeof(psTemplate->asWeaps));
	int body = get_first_available_component(psStruct, context->argument(2), COMP_BODY);
	if (body < 0)
	{
		debug(LOG_SCRIPT, "Wanted to build %s at %s, but body type all unavailable",
		      templName.toUtf8().constData(), objInfo(psStruct));
		return QScriptValue(false); // no component available
	}
	int prop = get_first_available_component(psStruct, context->argument(3), COMP_PROPULSION);
	if (prop < 0)
	{
		debug(LOG_SCRIPT, "Wanted to build %s at %s, but propulsion type all unavailable",
		      templName.toUtf8().constData(), objInfo(psStruct));
		return QScriptValue(false); // no component available
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
	if ((result = getCompFromName(COMP_WEAPON, compName.toUtf8().constData())) >= 0)
	{
		for (int i = 0; i < numTurrets; i++) // may be multi-weapon
		{
			result = get_first_available_component(psStruct, context->argument(firstTurret + i), COMP_WEAPON);
			if (result < 0)
			{
				debug(LOG_SCRIPT, "Wanted to build %s at %s, but no weapon unavailable", 
				      templName.toUtf8().constData(), objInfo(psStruct));
				return QScriptValue(false);
			}
			psTemplate->asWeaps[i] = result;
			psTemplate->numWeaps++;
		}
	}
	else
	{
		COMPONENT_TYPE compType = COMP_NUMCOMPONENTS;

		if ((result = getCompFromName(COMP_CONSTRUCT, compName.toUtf8().constData())) >= 0)
		{
			compType = COMP_CONSTRUCT;
		}
		else if ((result = getCompFromName(COMP_BRAIN, compName.toUtf8().constData())) >= 0)
		{
			compType = COMP_BRAIN;
			psTemplate->numWeaps = 1; // hack, necessary to pass intValidTemplate
		}
		else if ((result = getCompFromName(COMP_REPAIRUNIT, compName.toUtf8().constData())) >= 0)
		{
			compType = COMP_REPAIRUNIT;
		}
		else if ((result = getCompFromName(COMP_ECM, compName.toUtf8().constData())) >= 0)
		{
			compType = COMP_ECM;
		}
		else if ((result = getCompFromName(COMP_SENSOR, compName.toUtf8().constData())) >= 0)
		{
			compType = COMP_SENSOR;
		}
		else
		{
			debug(LOG_ERROR, "No known component type found for %s", compName.toUtf8().constData());
			return QScriptValue(false);
		}
		result = get_first_available_component(psStruct, context->argument(firstTurret), compType);
		if (result < 0)
		{
			debug(LOG_SCRIPT, "Wanted to build %s at %s, but turret unavailable",
			      templName.toUtf8().constData(), objInfo(psStruct));
			return QScriptValue(false);
		}
		psTemplate->asParts[compType] = result;
	}
	bool valid = intValidTemplate(psTemplate, templName.toUtf8().constData(), true);
	if (valid)
	{
		SCRIPT_ASSERT(context, validTemplateForFactory(psTemplate, psStruct, true),
		              "Invalid template %s for factory %s",
		              psTemplate->aName, psStruct->pStructureType->pName);
		// Delete similar template from existing list before adding this one
		for (int j = 0; j < apsTemplateList.size(); j++)
		{
			DROID_TEMPLATE *t = apsTemplateList[j];
			if (strcmp(t->aName, psTemplate->aName) == 0)
			{
				debug(LOG_SCRIPT, "deleting %s for player %d", t->aName, player);
				deleteTemplateFromProduction(t, player, ModeQueue); // duplicate? done below?
				SendDestroyTemplate(t, player);
				break;
			}
		}
		// Add to list
		debug(LOG_SCRIPT, "adding template %s for player %d", psTemplate->aName, player);
		psTemplate->multiPlayerID = generateNewObjectId();
		psTemplate->psNext = apsDroidTemplates[player];
		apsDroidTemplates[player] = psTemplate;
		sendTemplate(player, psTemplate);
		if (!structSetManufacture(psStruct, psTemplate, ModeQueue))
		{
			debug(LOG_ERROR, "Could not produce template %s in %s", psTemplate->aName, objInfo(psStruct));
			valid = false;
		}
	}
	else
	{
		debug(LOG_ERROR, "Invalid template %s", templName.toUtf8().constData());
	}
	return QScriptValue(valid);
}

//-- \subsection{enumStruct([player[, structure type[, looking player]]])}
//-- Returns an array of structure objects. If no parameters given, it will
//-- return all of the structures for the current player. The second parameter
//-- can be either a string with the name of the structure type as defined in
//-- "structures.txt", or a stattype as defined in \ref{objects:structure}. The
//-- third parameter can be used to filter by visibility, the default is not
//-- to filter.
static QScriptValue js_enumStruct(QScriptContext *context, QScriptEngine *engine)
{
	QList<STRUCTURE *> matches;
	int player = -1, looking = -1;
	QString statsName;
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
			statsName = val.toString();
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

//-- \subsection{enumStructOffWorld([player[, structure type[, looking player]]])}
//-- Returns an array of structure objects in your base when on an off-world mission, NULL otherwise.
//-- If no parameters given, it will return all of the structures for the current player.
//-- The second parameter can be either a string with the name of the structure type as defined
//-- in "structures.txt", or a stattype as defined in \ref{objects:structure}.
//-- The third parameter can be used to filter by visibility, the default is not
//-- to filter.
static QScriptValue js_enumStructOffWorld(QScriptContext *context, QScriptEngine *engine)
{
	QList<STRUCTURE *> matches;
	int player = -1, looking = -1;
	QString statsName;
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
			statsName = val.toString();
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

//-- \subsection{enumFeature(player, name)}
//-- Returns an array of all features seen by player of given name, as defined in "features.txt".
//-- If player is \emph{ALL_PLAYERS}, it will return all features irrespective of visibility to any player. If
//-- name is empty, it will return any feature.
static QScriptValue js_enumFeature(QScriptContext *context, QScriptEngine *engine)
{
	QList<FEATURE *> matches;
	int looking = context->argument(0).toInt32();
	QString statsName = context->argument(1).toString();
	SCRIPT_ASSERT(context, looking < MAX_PLAYERS && looking >= -1, "Looking player index out of range: %d", looking);
	for (FEATURE *psFeat = apsFeatureLists[0]; psFeat; psFeat = psFeat->psNext)
	{
		if ((looking == -1 || psFeat->visible[looking])
		    && !psFeat->died
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

//-- \subsection{enumDroid([player[, droid type[, looking player]]])}
//-- Returns an array of droid objects. If no parameters given, it will
//-- return all of the droids for the current player. The second, optional parameter
//-- is the name of the droid type. The third parameter can be used to filter by
//-- visibility - the default is not to filter.
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

	SCRIPT_ASSERT_PLAYER(context, player);
	SCRIPT_ASSERT(context, looking < MAX_PLAYERS && looking >= -1, "Looking player index out of range: %d", looking);
	for (DROID *psDroid = apsDroidLists[player]; psDroid; psDroid = psDroid->psNext)
	{
		if ((looking == -1 || psDroid->visible[looking])
		    && !psDroid->died
		    && (droidType == DROID_ANY || droidType == psDroid->droidType))
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

//-- \subsection{debug(string...)}
//-- Output text to the command line.
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
	qWarning("%s", result.toAscii().constData());
	return QScriptValue();
}

//-- \subsection{pickStructLocation(droid, structure type, x, y)}
//-- Pick a location for constructing a certain type of building near some given position.
//-- Returns an object containing "type" POSITION, and "x" and "y" values, if successful.
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
		debug(LOG_SCRIPT, "Did not find valid positioning for %s", psStat->pName);
	}
	return QScriptValue();
}

//-- \subsection{structureIdle(structure)}
//-- Is given structure idle?
static QScriptValue js_structureIdle(QScriptContext *context, QScriptEngine *)
{
	QScriptValue structVal = context->argument(0);
	int id = structVal.property("id").toInt32();
	int player = structVal.property("player").toInt32();
	STRUCTURE *psStruct = IdToStruct(id, player);
	SCRIPT_ASSERT(context, psStruct, "No such structure id %d belonging to player %d", id, player);
	return QScriptValue(structureIdle(psStruct));
}

//-- \subsection{removeStruct(structure)}
//-- Immediately remove the given structure from the map. Returns a boolean that is true on success.
//-- No special effects are applied. Deprecated since 3.2.
static QScriptValue js_removeStruct(QScriptContext *context, QScriptEngine *)
{
	QScriptValue structVal = context->argument(0);
	int id = structVal.property("id").toInt32();
	int player = structVal.property("player").toInt32();
	STRUCTURE *psStruct = IdToStruct(id, player);
	SCRIPT_ASSERT(context, psStruct, "No such structure id %d belonging to player %d", id, player);
	return QScriptValue(removeStruct(psStruct, true));
}

//-- \subsection{removeObject(game object[, special effects?])}
//-- Remove the given game object with special effects. Returns a boolean that is true on success.
//-- A second, optional boolean parameter specifies whether special effects are to be applied. (3.2+ only)
static QScriptValue js_removeObject(QScriptContext *context, QScriptEngine *)
{
	QScriptValue structVal = context->argument(0);
	int id = structVal.property("id").toInt32();
	int player = structVal.property("player").toInt32();
	BASE_OBJECT *psObj = IdToPointer(id, player);
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

//-- \subsection{console(strings...)}
//-- Print text to the player console.
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

//-- \subsection{groupAddArea(group, x1, y1, x2, y2)}
//-- Add any droids inside the given area to the given group.
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

//-- \subsection{groupAddDroid(group, droid)}
//-- Add given droid to given group.
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

//-- \subsection{distBetweenTwoPoints(x1, y1, x2, y2)}
//-- Return distance between two points.
static QScriptValue js_distBetweenTwoPoints(QScriptContext *context, QScriptEngine *engine)
{
	int x1 = context->argument(0).toNumber();
	int y1 = context->argument(1).toNumber();
	int x2 = context->argument(2).toNumber();
	int y2 = context->argument(3).toNumber();
	return QScriptValue(iHypot(x1 - x2, y1 - y2));
}

//-- \subsection{groupSize(group)}
//-- Return the number of droids currently in the given group. DEPRECATED. Use groupSizes[] instead.
static QScriptValue js_groupSize(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue groups = engine->globalObject().property("groupSizes");
	int groupId = context->argument(0).toInt32();
	return groups.property(groupId).toInt32();
}

//-- \subsection{droidCanReach(droid, x, y)}
//-- Return whether or not the given droid could possibly drive to the given position. Does
//-- not take player built blockades into account.
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

//-- \subsection{orderDroid(droid, order)}
//-- Give a droid an order to do something.
static QScriptValue js_orderDroid(QScriptContext *context, QScriptEngine *)
{
	QScriptValue droidVal = context->argument(0);
	int id = droidVal.property("id").toInt32();
	int player = droidVal.property("player").toInt32();
	DROID *psDroid = IdToDroid(id, player);
	SCRIPT_ASSERT(context, psDroid, "Droid id %d not found belonging to player %d", id, player);
	DROID_ORDER order = (DROID_ORDER)context->argument(1).toInt32();
	SCRIPT_ASSERT(context, order == DORDER_TEMP_HOLD || order == DORDER_RTR || order == DORDER_STOP
		      || order == DORDER_RTB || order == DORDER_REARM,
	              "Invalid order: %s", getDroidOrderName(order));
	if (order == DORDER_REARM)
	{
		moveToRearm(psDroid);
	}
	else
	{
		orderDroid(psDroid, order, ModeQueue);
	}
	return QScriptValue(true);
}

//-- \subsection{orderDroidObj(droid, order, object)}
//-- Give a droid an order to do something to something.
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
	SCRIPT_ASSERT(context, validOrderForObj(order), "Invalid order: %s", getDroidOrderName(order));
	orderDroidObj(psDroid, order, psObj, ModeQueue);
	return QScriptValue(true);
}

//-- \subsection{orderDroidBuild(droid, order, structure type, x, y[, direction])}
//-- Give a droid an order to build someting at the given position. Returns true if allowed.
static QScriptValue js_orderDroidBuild(QScriptContext *context, QScriptEngine *)
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
	uint16_t direction = 0;

	SCRIPT_ASSERT(context, order == DORDER_BUILD, "Invalid order");
	SCRIPT_ASSERT(context, strcmp(psStats->pName, "A0ADemolishStructure") != 0, "Cannot build demolition");
	if (context->argumentCount() > 5)
	{
		direction = DEG(context->argument(5).toNumber());
	}
	orderDroidStatsLocDir(psDroid, order, psStats, world_coord(x) + TILE_UNITS / 2, world_coord(y) + TILE_UNITS / 2, direction, ModeQueue);
	return QScriptValue(true);
}

//-- \subsection{orderDroidLoc(droid, order, x, y)}
//-- Give a droid an order to do something at the given location.
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
	orderDroidLoc(psDroid, order, world_coord(x), world_coord(y), ModeQueue);
	return QScriptValue();
}

//-- \subsection{setMissionTime(time)} Set mission countdown.
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

//-- \subsection{setReinforcementTime(time)} Set time for reinforcements to arrive. If time is
//-- negative, the reinforcement GUI is removed and the timer stopped.
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
			if (psDroid->droidType == DROID_TRANSPORTER || psDroid->droidType == DROID_SUPERTRANSPORTER)
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

//-- \subsection{setStructureLimits(structure type, limit[, player])} Set build limits for a structure.
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
	SCRIPT_ASSERT_PLAYER(context, player);
	SCRIPT_ASSERT(context, limit < LOTS_OF && limit >= 0, "Invalid limit");
	SCRIPT_ASSERT(context, structInc < numStructureStats && structInc >= 0, "Invalid structure");

	STRUCTURE_LIMITS *psStructLimits = asStructLimits[player];
	psStructLimits[structInc].limit = limit;
	psStructLimits[structInc].globalLimit = limit;

	return QScriptValue();
}

//-- \subsection{centreView(x, y)}
//-- Center the player's camera at the given position.
static QScriptValue js_centreView(QScriptContext *context, QScriptEngine *engine)
{
	int x = context->argument(0).toInt32();
	int y = context->argument(1).toInt32();
	setViewPos(x, y, false);
	Q_UNUSED(engine);
	return QScriptValue();
}

//-- \subsection{playSound(sound[, x, y, z])}
//-- Play a sound, optionally at a location.
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
		soundID = audio_SetTrackVals(sound.toUtf8().constData(), false, 100, 1800);
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

//-- \subsection{gameOverMessage(won)}
//-- End game in victory or defeat.
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
		psViewData = getViewData("END");	// FIXME: rename to FAILED|LOST ?
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

//-- \subsection{completeResearch(research[, player])}
//-- Finish a research for the given player.
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

//-- \subsection{enableResearch(research[, player])}
//-- Enable a research for the given player, allowing it to be researched.
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

//-- \subsection{setPower(power[, player])}
//-- Set a player's power directly. (Do not use this in an AI script.)
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

//-- \subsection{enableStructure(structure type[, player])}
//-- The given structure type is made available to the given player. It will appear in the
//-- player's build list.
static QScriptValue js_enableStructure(QScriptContext *context, QScriptEngine *engine)
{
	QString building = context->argument(0).toString();
	int index = getStructStatFromName(building.toUtf8().constData());
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

//-- \subsection{setTutorialMode(bool)} Sets a number of restrictions appropriate for tutorial if set to true.
static QScriptValue js_setTutorialMode(QScriptContext *context, QScriptEngine *engine)
{
	bInTutorial = context->argument(0).toBool();
	return QScriptValue();
}

//-- \subsection{setMiniMap(bool)} Turns visible minimap on or off in the GUI.
static QScriptValue js_setMiniMap(QScriptContext *context, QScriptEngine *engine)
{
	radarPermitted = context->argument(0).toBool();
	return QScriptValue();
}

//-- \subsection{setDesign(bool)} Whether to allow player to design stuff.
static QScriptValue js_setDesign(QScriptContext *context, QScriptEngine *engine)
{
	DROID_TEMPLATE *psCurr;
	allowDesign = context->argument(0).toBool();
	// Switch on or off future templates
	// FIXME: This dual data structure for templates is just plain insane.
	for (psCurr = apsDroidTemplates[selectedPlayer]; psCurr != NULL; psCurr = psCurr->psNext)
	{
		bool researched = researchedTemplate(psCurr, selectedPlayer);
		psCurr->enabled = (researched || allowDesign);
	}
	for (std::list<DROID_TEMPLATE>::iterator i = localTemplates.begin(); i != localTemplates.end(); ++i)
	{
		psCurr = &*i;
		bool researched = researchedTemplate(psCurr, selectedPlayer);
		psCurr->enabled = (researched || allowDesign);
	}
	return QScriptValue();
}

//-- \subsection{enableTemplate(template name)} Enable a specific template (even if design is disabled).
static QScriptValue js_enableTemplate(QScriptContext *context, QScriptEngine *engine)
{
	DROID_TEMPLATE *psCurr;
	QString templateName = context->argument(0).toString();
	bool found = false;
	// FIXME: This dual data structure for templates is just plain insane.
	for (psCurr = apsDroidTemplates[selectedPlayer]; psCurr != NULL; psCurr = psCurr->psNext)
	{
		if (!templateName.compare(psCurr->pName))
		{
			psCurr->enabled = true;
			found = true;
		}
	}
	if (!found)
	{
		debug(LOG_ERROR, "Template %s was not found!", templateName.toAscii().constData());
		return QScriptValue(false);
	}
	for (std::list<DROID_TEMPLATE>::iterator i = localTemplates.begin(); i != localTemplates.end(); ++i)
	{
		psCurr = &*i;
		if (!templateName.compare(psCurr->pName))
		{
			psCurr->enabled = true;
		}
	}
	return QScriptValue();
}


//-- \subsection{addReticuleButton(button type)} Add reticule button. FIXME: This currently only works in tutorial.
//-- Valid parameters for this and \emph{removeReticuleButton}: MANUFACTURE, RESEARCH, BUILD, DESIGN, INTELMAP, COMMAND, CANCEL.
static QScriptValue js_addReticuleButton(QScriptContext *context, QScriptEngine *engine)
{
	int button = context->argument(0).toInt32();
	SCRIPT_ASSERT(context, button != IDRET_OPTIONS, "Invalid button");
	widgReveal(psWScreen, button);
	return QScriptValue();
}

//-- \subsection{removeReticuleButton(button type)} Remove reticule button. FIXME: This currently only works in tutorial.
static QScriptValue js_removeReticuleButton(QScriptContext *context, QScriptEngine *engine)
{
	int button = context->argument(0).toInt32();
	SCRIPT_ASSERT(context, button != IDRET_OPTIONS, "Invalid button");
	// if (bInTutorial && bReset) intResetScreen(true);
	widgHide(psWScreen, button);
	return QScriptValue();
}

//-- \subsection{applyLimitSet()} Mix user set limits with script set limits and defaults.
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

//-- \subsection{enableComponent(component, player)}
//-- The given component is made available for research for the given player.
static QScriptValue js_enableComponent(QScriptContext *context, QScriptEngine *engine)
{
	QString componentName = context->argument(0).toString();
	int player = context->argument(1).toInt32();

	SCRIPT_ASSERT_PLAYER(context, player);
	setComponent(componentName, player, FOUND);
	return QScriptValue();
}

//-- \subsection{makeComponentAvailable(component, player)}
//-- The given component is made available to the given player. This means the player can
//-- actually build designs with it.
static QScriptValue js_makeComponentAvailable(QScriptContext *context, QScriptEngine *engine)
{
	QString componentName = context->argument(0).toString();
	int player = context->argument(1).toInt32();

	SCRIPT_ASSERT_PLAYER(context, player);
	setComponent(componentName, player, AVAILABLE);
	return QScriptValue();
}

//-- \subsection{allianceExistsBetween(player, player)}
//-- Returns true if an alliance exists between the two players, or they are the same player.
static QScriptValue js_allianceExistsBetween(QScriptContext *context, QScriptEngine *engine)
{
	int player1 = context->argument(0).toInt32();
	int player2 = context->argument(1).toInt32();
	SCRIPT_ASSERT(context, player1 < MAX_PLAYERS && player1 >= 0, "Invalid player");
	SCRIPT_ASSERT(context, player2 < MAX_PLAYERS && player2 >= 0, "Invalid player");
	return QScriptValue(alliances[player1][player2] == ALLIANCE_FORMED);
}

//-- \subsection{_(string)}
//-- Mark string for translation.
static QScriptValue js_translate(QScriptContext *context, QScriptEngine *engine)
{
	return QScriptValue(gettext(context->argument(0).toString().toUtf8().constData()));
}

//-- \subsection{playerPower(player)}
//-- Return amount of power held by given player.
static QScriptValue js_playerPower(QScriptContext *context, QScriptEngine *engine)
{
	int player = context->argument(0).toInt32();
	SCRIPT_ASSERT_PLAYER(context, player);
	return QScriptValue(getPower(player));
}

//-- \subsection{isStructureAvailable(structure type[, player])}
static QScriptValue js_isStructureAvailable(QScriptContext *context, QScriptEngine *engine)
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
	return QScriptValue(apStructTypeLists[player][index] == AVAILABLE
			    && asStructLimits[player][index].currentQuantity < asStructLimits[player][index].limit);
}

//-- \subsection{isVTOL(droid)}
//-- Returns true if given droid is a VTOL (not including transports).
static QScriptValue js_isVTOL(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue droidVal = context->argument(0);
	int id = droidVal.property("id").toInt32();
	int player = droidVal.property("player").toInt32();
	DROID *psDroid = IdToDroid(id, player);
	SCRIPT_ASSERT(context, psDroid, "No such droid id %d belonging to player %d", id, player);
	return QScriptValue(isVtolDroid(psDroid));
}

//-- \subsection{objFromId(id)}
//-- Function to make porting from the old scripting system easier. Do not use for new code.
//-- Instead, use labels.
static QScriptValue js_objFromId(QScriptContext *context, QScriptEngine *engine)
{
	QScriptValue droidVal = context->argument(0);
	int id = droidVal.property("id").toInt32();
	BASE_OBJECT *psObj = getBaseObjFromId(id);
	SCRIPT_ASSERT(context, psObj, "No such object id %d", id);
	return QScriptValue(convMax(psObj, engine));
}

//-- \subsection{setDroidExperience(droid, experience)}
//-- Set the amount of experience a droid has. Experience is read using floating point precision.
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

//-- \subsection{safeDest(player, x, y)} Returns true if given player is safe from hostile fire at
//-- the given location, to the best of that player's map knowledge.
static QScriptValue js_safeDest(QScriptContext *context, QScriptEngine *engine)
{
	int player = context->argument(0).toInt32();
	SCRIPT_ASSERT_PLAYER(context, player);
	int x = context->argument(1).toInt32();
	int y = context->argument(2).toInt32();
	SCRIPT_ASSERT(context, tileOnMap(x, y), "Out of bounds coordinates(%d, %d)", x, y);
	return QScriptValue(!(auxTile(x, y, player) & AUXBITS_DANGER));
}

//-- \subsection{addStructure(structure type, player, x, y)}
//-- Create a structure on the given position. Returns true on success.
static QScriptValue js_addStructure(QScriptContext *context, QScriptEngine *)
{
	QString building = context->argument(0).toString();
	int index = getStructStatFromName(building.toUtf8().constData());
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
	}
	return QScriptValue(psStruct != NULL);
}

//-- \subsection{getStructureLimit(structure type[, player])}
//-- Create a structure on the given position. Returns true on success.
static QScriptValue js_getStructureLimit(QScriptContext *context, QScriptEngine *engine)
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
	return QScriptValue(asStructLimits[player][index].limit);
}

//-- \subsection{countStruct(structure type[, player])}
//-- Count the number of structures of a given type.
//-- The player parameter can be a specific player, ALL_PLAYERS, ALLIES or ENEMIES.
static QScriptValue js_countStruct(QScriptContext *context, QScriptEngine *engine)
{
	QString building = context->argument(0).toString();
	int index = getStructStatFromName(building.toUtf8().constData());
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
			quantity += asStructLimits[i][index].currentQuantity;
		}
	}
	return QScriptValue(quantity);
}

//-- \subsection{countDroid(droid type[, player])}
//-- Count the number of droids that a given player has. Droid type must be either
//-- DROID_ANY, DROID_COMMAND or DROID_CONSTRUCT.
//-- The player parameter can be a specific player, ALL_PLAYERS, ALLIES or ENEMIES.
static QScriptValue js_countDroid(QScriptContext *context, QScriptEngine *engine)
{
	int me = engine->globalObject().property("me").toInt32();
	int player = me;
	int quantity = 0;
	int type = context->argument(0).toInt32();
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

//-- \subsection{setNoGoArea(x1, y1, x2, y2, player)}
//-- Creates an area on the map on which nothing can be built. If player is zero,
//-- then landing lights are placed. If player is -1, then a limbo landing zone
//-- is created and limbo droids placed.
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

//-- \subsection{setScrollParams(x1, y1, x2, y2)}
//-- Limit the scrollable area of the map to the given rectangle.
static QScriptValue js_setScrollParams(QScriptContext *context, QScriptEngine *)
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

//-- \subsection{loadLevel(level name)}
//-- Load the level with the given name.
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

//-- \subsection{enumRange(x, y, range[, filter[, seen]])}
//-- Returns an array of game objects seen within range of given position that passes the optional filter
//-- which can be one of a player index, ALL_PLAYERS, ALLIES or ENEMIES. By default, filter is 
//-- ALL_PLAYERS. Finally an optional parameter can specify whether only visible objects should be 
//-- returned; by default only visible objects are returned. Calling this function is much faster than 
//-- iterating over all game objects using other enum functions. (3.2+ only)
static QScriptValue js_enumRange(QScriptContext *context, QScriptEngine *engine)
{
	int player = engine->globalObject().property("me").toInt32();
	int x = world_coord(context->argument(0).toInt32());
	int y = world_coord(context->argument(1).toInt32());
	int range = context->argument(2).toInt32();
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
	gridStartIterate(x, y, range);
	QList<BASE_OBJECT *> list;
	for (BASE_OBJECT *psObj = gridIterate(); psObj != NULL; psObj = gridIterate())
	{
		if ((psObj->visible[player] || !seen) && !psObj->died)
		{
			if ((filter >= 0 && psObj->player == filter) || filter == ALL_PLAYERS
			    || (filter == ALLIES && aiCheckAlliances(psObj->player, player))
			    || (filter == ENEMIES && !aiCheckAlliances(psObj->player, player)))
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

//-- \subsection{enumArea(<x1, y1, x2, y2 | label>[, filter[, seen]])}
//-- Returns an array of game objects seen within the given area that passes the optional filter
//-- which can be one of a player index, ALL_PLAYERS, ALLIES or ENEMIES. By default, filter is 
//-- ALL_PLAYERS. Finally an optional parameter can specify whether only visible objects should be 
//-- returned; by default only visible objects are returned. The label can either be actual 
//-- positions or a label to an AREA. Calling this function is much faster than iterating over all
//-- game objects using other enum functions. (3.2+ only)
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
		labeltype p = labels.value(label);
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
		filter = context->argument(3).toInt32();
	}
	if (context->argumentCount() > nextparam++)
	{
		seen = context->argument(4).toBool();
	}
	gridStartIterateArea(x1, y1, x2, y2);
	QList<BASE_OBJECT *> list;
	for (BASE_OBJECT *psObj = gridIterate(); psObj != NULL; psObj = gridIterate())
	{
		if ((psObj->visible[player] || !seen) && !psObj->died)
		{
			if ((filter >= 0 && psObj->player == filter) || filter == ALL_PLAYERS
			    || (filter == ALLIES && aiCheckAlliances(psObj->player, player))
			    || (filter == ENEMIES && !aiCheckAlliances(psObj->player, player)))
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

//-- \subsection{chat(target player, message)}
//-- Send a message to target player. Target may also be \emph{ALL_PLAYERS} or \emph{ALLIES}.
//-- Returns a boolean that is true on success. (3.2+ only)
static QScriptValue js_chat(QScriptContext *context, QScriptEngine *engine)
{
	int player = engine->globalObject().property("me").toInt32();
	int target = context->argument(0).toInt32();
	QString message = context->argument(1).toString();
	SCRIPT_ASSERT_PLAYER(context, player);
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
		return QScriptValue(sendTextMessage(QString(QString::number(target) + " " + message).toUtf8().constData(), false, player));
	}
}

//-- \subsection{setAlliance(player1, player2, value)}
//-- Set alliance status between two players to either true or false. (3.2+ only)
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

//-- \subsection{setAssemblyPoint(structure, x, y)}
//-- Set the assembly point droids go to when built for the specified structure. (3.2+ only)
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

//-- \subsection{hackNetOff()}
//-- Turn off network transmissions. FIXME - find a better way.
static QScriptValue js_hackNetOff(QScriptContext *, QScriptEngine *)
{
	bMultiPlayer = false;
	bMultiMessages = false;
	return QScriptValue();
}

//-- \subsection{hackNetOn()}
//-- Turn on network transmissions. FIXME - find a better way.
static QScriptValue js_hackNetOn(QScriptContext *, QScriptEngine *)
{
	bMultiPlayer = true;
	bMultiMessages = true;
	return QScriptValue();
}

//-- \subsection{getDroidProduction(factory)}
//-- Return droid in production in given factory. Note that this droid is fully
//-- virtual, and should never be passed anywhere.
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
	droidSetName(psDroid, psTemp->aName);
	droidSetBits(psTemp, psDroid);
	psDroid->weight = calcDroidWeight(psTemp);
	psDroid->baseSpeed = calcDroidBaseSpeed(psTemp, psDroid->weight, player);
	objSensorCache((BASE_OBJECT *)psDroid, asSensorStats + psTemp->asParts[COMP_SENSOR]);
	objEcmCache((BASE_OBJECT *)psDroid, asECMStats + psTemp->asParts[COMP_ECM]);
	return convDroid(psDroid, engine);
}

//-- \subsection{getDroidLimit([player[, unit type]])}
//-- Return maximum number of droids that this player can produce. This limit is usually
//-- fixed throughout a game and the same for all players. If no arguments are passed,
//-- returns general unit limit for the current player. If a second, unit type argument 
//-- is passed, the limit for this unit type is returned, which may be different from
//-- the general unit limit (eg for commanders and construction droids).
static QScriptValue js_getDroidLimit(QScriptContext *context, QScriptEngine *engine)
{
	if (context->argumentCount() > 1)
	{
		DROID_TYPE type = (DROID_TYPE)context->argument(1).toInt32();
		if (type == DROID_COMMAND)
		{
			return QScriptValue(MAX_COMMAND_DROIDS);
		}
		else if (type == DROID_CONSTRUCT)
		{
			return QScriptValue(MAX_CONSTRUCTOR_DROIDS);
		}
		// else return general unit limit
	}
	if (context->argumentCount() > 0)
	{
		return QScriptValue(getMaxDroids(context->argument(0).toInt32()));
	}
	return QScriptValue(getMaxDroids(engine->globalObject().property("me").toInt32()));
}

//-- \subsection{setDroidLimit(player, value)}
//-- Set the maximum number of droids that this player can produce.
static QScriptValue js_setDroidLimit(QScriptContext *context, QScriptEngine *)
{
	int player = context->argument(0).toInt32();
	int value = context->argument(1).toInt32();
	setMaxDroids(player, value);
	return QScriptValue();
}

//-- \subsection{setSunPosition(x, y, z)}
//-- Move the position of the Sun, which in turn moves where shadows are cast.
static QScriptValue js_setSunPosition(QScriptContext *context, QScriptEngine *)
{
	float x = context->argument(0).toNumber();
	float y = context->argument(1).toNumber();
	float z = context->argument(2).toNumber();
	setTheSun(Vector3f(x, y, z));
	return QScriptValue();
}

//-- \subsection{setSunIntensity(ambient r, g, b, diffuse r, g, b, specular r, g, b)}
//-- Set the ambient, diffuse and specular colour intensities of the Sun lighting source.
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

//-- \subsection{setWeather(weather type)}
//-- Set the current weather. This should be one of WEATHER_RAIN, WEATHER_SNOW or WEATHER_CLEAR.
static QScriptValue js_setWeather(QScriptContext *context, QScriptEngine *)
{
	WT_CLASS weather = (WT_CLASS)context->argument(0).toInt32();
	SCRIPT_ASSERT(context, weather >= 0 && weather <= WT_NONE, "Bad weather type");
	atmosSetWeatherType(weather);
	return QScriptValue();
}

//-- \subsection{setSky(texture page, wind speed, skybox scale)}
//-- Change the skybox. Default values are "page-25", 0.5, and 10000.0. Returns true on success.
static QScriptValue js_setSky(QScriptContext *context, QScriptEngine *)
{
	QString page = context->argument(0).toString();
	float wind = context->argument(1).toNumber();
	float scale = context->argument(2).toNumber();
	bool found = iV_GetTexture(page.toUtf8().constData()) >= 0;
	if (found)
	{
		setSkyBox(page.toUtf8().constData(), wind, scale);
	}
	return QScriptValue(found);
}

//-- \subsection{cameraSlide(x, y)}
//-- Slide the camera over to the given position on the map.
static QScriptValue js_cameraSlide(QScriptContext *context, QScriptEngine *)
{
	int x = context->argument(0).toNumber();
	int y = context->argument(1).toNumber();
	requestRadarTrack(x, y);
	return QScriptValue();
}

//-- \subsection{cameraTrack(droid)}
//-- Make the camera follow the given droid object around. Pass in a null object to stop.
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
		for (DROID *psDroid = apsDroidLists[selectedPlayer]; psDroid!=NULL; psDroid = psDroid->psNext)
		{
			psDroid->selected = (psDroid == targetDroid); // select only the target droid
		}
		setWarCamActive(true);
	}
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
	return true;
}

bool registerFunctions(QScriptEngine *engine)
{
	// Create group map
	GROUPMAP *psMap = new GROUPMAP;
	groups.insert(engine, psMap);

	// Register functions to the script engine here
	engine->globalObject().setProperty("_", engine->newFunction(js_translate));
	engine->globalObject().setProperty("label", engine->newFunction(js_label));
	engine->globalObject().setProperty("enumLabels", engine->newFunction(js_enumLabels));
	engine->globalObject().setProperty("enumGateways", engine->newFunction(js_enumGateways));
	engine->globalObject().setProperty("enumTemplates", engine->newFunction(js_enumTemplates));
	engine->globalObject().setProperty("setAlliance", engine->newFunction(js_setAlliance));
	engine->globalObject().setProperty("setAssemblyPoint", engine->newFunction(js_setAssemblyPoint));
	engine->globalObject().setProperty("setSunPosition", engine->newFunction(js_setSunPosition));
	engine->globalObject().setProperty("setSunIntensity", engine->newFunction(js_setSunIntensity));
	engine->globalObject().setProperty("setWeather", engine->newFunction(js_setWeather));
	engine->globalObject().setProperty("setSky", engine->newFunction(js_setSky));
	engine->globalObject().setProperty("cameraSlide", engine->newFunction(js_cameraSlide));
	engine->globalObject().setProperty("cameraTrack", engine->newFunction(js_cameraTrack));

	// horrible hacks follow -- do not rely on these being present!
	engine->globalObject().setProperty("hackNetOff", engine->newFunction(js_hackNetOff));
	engine->globalObject().setProperty("hackNetOn", engine->newFunction(js_hackNetOn));
	engine->globalObject().setProperty("objFromId", engine->newFunction(js_objFromId));

	// General functions -- geared for use in AI scripts
	engine->globalObject().setProperty("debug", engine->newFunction(js_debug));
	engine->globalObject().setProperty("console", engine->newFunction(js_console));
	engine->globalObject().setProperty("structureIdle", engine->newFunction(js_structureIdle));
	engine->globalObject().setProperty("enumStruct", engine->newFunction(js_enumStruct));
	engine->globalObject().setProperty("enumStructOffWorld", engine->newFunction(js_enumStructOffWorld));
	engine->globalObject().setProperty("enumDroid", engine->newFunction(js_enumDroid));
	engine->globalObject().setProperty("enumGroup", engine->newFunction(js_enumGroup));
	engine->globalObject().setProperty("enumFeature", engine->newFunction(js_enumFeature));
	engine->globalObject().setProperty("enumBlips", engine->newFunction(js_enumBlips));
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
	engine->globalObject().setProperty("groupSize", engine->newFunction(js_groupSize));
	engine->globalObject().setProperty("orderDroidLoc", engine->newFunction(js_orderDroidLoc));
	engine->globalObject().setProperty("playerPower", engine->newFunction(js_playerPower));
	engine->globalObject().setProperty("isStructureAvailable", engine->newFunction(js_isStructureAvailable));
	engine->globalObject().setProperty("pickStructLocation", engine->newFunction(js_pickStructLocation));
	engine->globalObject().setProperty("droidCanReach", engine->newFunction(js_droidCanReach));
	engine->globalObject().setProperty("orderDroidBuild", engine->newFunction(js_orderDroidBuild));
	engine->globalObject().setProperty("orderDroidObj", engine->newFunction(js_orderDroidObj));
	engine->globalObject().setProperty("orderDroid", engine->newFunction(js_orderDroid));
	engine->globalObject().setProperty("buildDroid", engine->newFunction(js_buildDroid));
	engine->globalObject().setProperty("addDroid", engine->newFunction(js_addDroid));
	engine->globalObject().setProperty("addFeature", engine->newFunction(js_addFeature));
	engine->globalObject().setProperty("componentAvailable", engine->newFunction(js_componentAvailable));
	engine->globalObject().setProperty("isVTOL", engine->newFunction(js_isVTOL));
	engine->globalObject().setProperty("safeDest", engine->newFunction(js_safeDest));
	engine->globalObject().setProperty("activateStructure", engine->newFunction(js_activateStructure));
	engine->globalObject().setProperty("chat", engine->newFunction(js_chat));
	engine->globalObject().setProperty("getDroidProduction", engine->newFunction(js_getDroidProduction));
	engine->globalObject().setProperty("getDroidLimit", engine->newFunction(js_getDroidLimit));
	engine->globalObject().setProperty("setDroidLimit", engine->newFunction(js_setDroidLimit));

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
	engine->globalObject().setProperty("setTutorialMode", engine->newFunction(js_setTutorialMode));
	engine->globalObject().setProperty("setDesign", engine->newFunction(js_setDesign));
	engine->globalObject().setProperty("enableTemplate", engine->newFunction(js_enableTemplate));
	engine->globalObject().setProperty("setMiniMap", engine->newFunction(js_setMiniMap));
	engine->globalObject().setProperty("addReticuleButton", engine->newFunction(js_addReticuleButton));
	engine->globalObject().setProperty("removeReticuleButton", engine->newFunction(js_removeReticuleButton));
	engine->globalObject().setProperty("enableStructure", engine->newFunction(js_enableStructure));
	engine->globalObject().setProperty("makeComponentAvailable", engine->newFunction(js_makeComponentAvailable));
	engine->globalObject().setProperty("enableComponent", engine->newFunction(js_enableComponent));
	engine->globalObject().setProperty("allianceExistsBetween", engine->newFunction(js_allianceExistsBetween));
	engine->globalObject().setProperty("removeStruct", engine->newFunction(js_removeStruct));
	engine->globalObject().setProperty("removeObject", engine->newFunction(js_removeObject));
	engine->globalObject().setProperty("setScrollParams", engine->newFunction(js_setScrollParams));
	engine->globalObject().setProperty("addStructure", engine->newFunction(js_addStructure));
	engine->globalObject().setProperty("getStructureLimit", engine->newFunction(js_getStructureLimit));
	engine->globalObject().setProperty("countStruct", engine->newFunction(js_countStruct));
	engine->globalObject().setProperty("countDroid", engine->newFunction(js_countDroid));
	engine->globalObject().setProperty("loadLevel", engine->newFunction(js_loadLevel));
	engine->globalObject().setProperty("setDroidExperience", engine->newFunction(js_setDroidExperience));
	engine->globalObject().setProperty("setNoGoArea", engine->newFunction(js_setNoGoArea));

	// Set some useful constants
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
	engine->globalObject().setProperty("DORDER_RETREAT", DORDER_RETREAT, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_PATROL", DORDER_PATROL, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_DEMOLISH", DORDER_DEMOLISH, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_EMBARK", DORDER_EMBARK, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_DISEMBARK", DORDER_DISEMBARK, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_FIRESUPPORT", DORDER_FIRESUPPORT, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_HOLD", DORDER_TEMP_HOLD, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_RTR", DORDER_RTR, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_RTB", DORDER_RTB, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_STOP", DORDER_STOP, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DORDER_REARM", DORDER_REARM, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("COMMAND", IDRET_COMMAND, QScriptValue::ReadOnly | QScriptValue::Undeletable);
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
	engine->globalObject().setProperty("HQ", REF_HQ, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("FACTORY", REF_FACTORY, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("POWER_GEN", REF_POWER_GEN, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("RESOURCE_EXTRACTOR", REF_RESOURCE_EXTRACTOR, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DEFENSE", REF_DEFENSE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("LASSAT", FAKE_REF_LASSAT, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("WALL", REF_WALL, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("RESEARCH_LAB", REF_RESEARCH, QScriptValue::ReadOnly | QScriptValue::Undeletable);
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
	engine->globalObject().setProperty("STRUCTURE", OBJ_STRUCTURE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("DROID", OBJ_DROID, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("FEATURE", OBJ_FEATURE, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("ALL_PLAYERS", ALL_PLAYERS, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("ALLIES", ALLIES, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("ENEMIES", ENEMIES, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("POSITION", SCRIPT_POSITION, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("AREA", SCRIPT_AREA, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("GROUP", SCRIPT_GROUP, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("PLAYER_DATA", SCRIPT_PLAYER, QScriptValue::ReadOnly | QScriptValue::Undeletable);
	engine->globalObject().setProperty("RESEARCH_DATA", SCRIPT_RESEARCH, QScriptValue::ReadOnly | QScriptValue::Undeletable);

	/// Place to store group sizes
	//== \item[groupSizes] A sparse array of group sizes. If a group has never been used, the entry in this array will
	//== be undefined.
	engine->globalObject().setProperty("groupSizes", engine->newObject());

	// Static knowledge about players
	//== \item[playerData] An array of information about the players in a game. Each item in the array is an object
	//== containing the following variables: difficulty (see \emph{difficulty} global constant), colour, position, 
	//== isAI (3.2+ only), isHuman (3.2+ only), name (3.2+ only), and team.
	QScriptValue playerData = engine->newArray(game.maxPlayers);
	for (int i = 0; i < game.maxPlayers; i++)
	{
		QScriptValue vector = engine->newObject();
		vector.setProperty("name", NetPlay.players[i].name, QScriptValue::ReadOnly | QScriptValue::Undeletable);
		vector.setProperty("difficulty", NetPlay.players[i].difficulty, QScriptValue::ReadOnly | QScriptValue::Undeletable);
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
	//== \item[derrickPositions] An array of derrick starting positions on the current map. Each item in the array is an 
	//== object containing the x and y variables for a derrick.
	//== \item[startPositions] An array of player start positions on the current map. Each item in the array is an 
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

	return true;
}
