/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
/*!
 * \file structure.c
 *
 * Store Structure stats.
 * WARNING!!!!!!
 * By the picking of these code-bombs, something wicked this way comes. This
 * file is almost as evil as hci.c
 */
#include <string.h>
#include <algorithm>

#include "lib/framework/frame.h"
#include "lib/framework/geometry.h"
#include "lib/framework/wzconfig.h"
#include "lib/ivis_opengl/imd.h"
#include "objects.h"
#include "ai.h"
#include "map.h"
#include "lib/gamelib/gtime.h"
#include "visibility.h"
#include "structure.h"
#include "research.h"
#include "hci.h"
#include "power.h"
#include "miscimd.h"
#include "effects.h"
#include "combat.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "stats.h"
#include "lib/framework/math_ext.h"
#include "display3d.h"
#include "geometry.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/piematrix.h"
#include "lib/framework/fixedpoint.h"
#include "order.h"
#include "droid.h"
#include "lib/script/script.h"
#include "scriptcb.h"
#include "action.h"
#include "group.h"
#include "transporter.h"
#include "fpath.h"
#include "mission.h"
#include "levels.h"
#include "console.h"
#include "cmddroid.h"
#include "feature.h"
#include "mapgrid.h"
#include "projectile.h"
#include "intdisplay.h"
#include "display.h"
#include "difficulty.h"
#include "scriptextern.h"
#include "keymap.h"
#include "game.h"
#include "qtscript.h"
#include "multiplay.h"
#include "lib/netplay/netplay.h"
#include "multigifts.h"
#include "loop.h"
#include "template.h"
#include "scores.h"
#include "gateway.h"

#include "random.h"
#include <functional>

//Maximium slope of the terrain for building a structure
#define MAX_INCLINE		50//80//40

/* droid construction smoke cloud constants */
#define	DROID_CONSTRUCTION_SMOKE_OFFSET	30
#define	DROID_CONSTRUCTION_SMOKE_HEIGHT	20

//used to calculate how often to increase the resistance level of a structure
#define RESISTANCE_INTERVAL			2000

//Value is stored for easy access to this structure stat
UDWORD			factoryModuleStat;
UDWORD			powerModuleStat;
UDWORD			researchModuleStat;

//holder for all StructureStats
STRUCTURE_STATS		*asStructureStats;
UDWORD				numStructureStats;

//used to hold the modifiers cross refd by weapon effect and structureStrength
STRUCTSTRENGTH_MODIFIER		asStructStrengthModifier[WE_NUMEFFECTS][NUM_STRUCT_STRENGTH];

//specifies which numbers have been allocated for the assembly points for the factories
static std::vector<bool>       factoryNumFlag[MAX_PLAYERS][NUM_FLAG_TYPES];

// the number of different (types of) droids that can be put into a production run
#define MAX_IN_RUN		9

//the list of what to build - only for selectedPlayer
std::vector<ProductionRun> asProductionRun[NUM_FACTORY_TYPES];

//stores which player the production list has been set up for
SBYTE               productionPlayer;

/* destroy building construction droid stat pointer */
static	STRUCTURE_STATS	*g_psStatDestroyStruct = nullptr;

// the structure that was last hit
STRUCTURE	*psLastStructHit;

//flag for drawing all sat uplink sees
static		UBYTE	satUplinkExists[MAX_PLAYERS];
//flag for when the player has one built - either completely or partially
static		UBYTE	lasSatExists[MAX_PLAYERS];

static bool setFunctionality(STRUCTURE *psBuilding, STRUCTURE_TYPE functionType);
static void setFlagPositionInc(FUNCTIONALITY *pFunctionality, UDWORD player, UBYTE factoryType);
static void informPowerGen(STRUCTURE *psStruct);
static bool electronicReward(STRUCTURE *psStructure, UBYTE attackPlayer);
static void factoryReward(UBYTE losingPlayer, UBYTE rewardPlayer);
static void repairFacilityReward(UBYTE losingPlayer, UBYTE rewardPlayer);
static void findAssemblyPointPosition(UDWORD *pX, UDWORD *pY, UDWORD player);
static void removeStructFromMap(STRUCTURE *psStruct);
static void resetResistanceLag(STRUCTURE *psBuilding);
static int structureTotalReturn(const STRUCTURE *psStruct);
static void structureCompletedCallback(STRUCTURE_STATS *psStructType);

// last time the maximum units message was displayed
static UDWORD	lastMaxUnitMessage;

// max number of units
static int droidLimit[MAX_PLAYERS];
// max number of commanders
static int commanderLimit[MAX_PLAYERS];
// max number of constructors
static int constructorLimit[MAX_PLAYERS];

#define MAX_UNIT_MESSAGE_PAUSE 20000

static void auxStructureNonblocking(STRUCTURE *psStructure)
{
	StructureBounds b = getStructureBounds(psStructure);

	for (int i = 0; i < b.size.x; i++)
	{
		for (int j = 0; j < b.size.y; j++)
		{
			auxClearAll(b.map.x + i, b.map.y + j, AUXBITS_BLOCKING | AUXBITS_OUR_BUILDING | AUXBITS_NONPASSABLE);
		}
	}
}

static void auxStructureBlocking(STRUCTURE *psStructure)
{
	StructureBounds b = getStructureBounds(psStructure);

	for (int i = 0; i < b.size.x; i++)
	{
		for (int j = 0; j < b.size.y; j++)
		{
			auxSetAllied(b.map.x + i, b.map.y + j, psStructure->player, AUXBITS_OUR_BUILDING);
			auxSetAll(b.map.x + i, b.map.y + j, AUXBITS_BLOCKING | AUXBITS_NONPASSABLE);
		}
	}
}

static void auxStructureOpenGate(STRUCTURE *psStructure)
{
	StructureBounds b = getStructureBounds(psStructure);

	for (int i = 0; i < b.size.x; i++)
	{
		for (int j = 0; j < b.size.y; j++)
		{
			auxClearAll(b.map.x + i, b.map.y + j, AUXBITS_BLOCKING);
		}
	}
}

static void auxStructureClosedGate(STRUCTURE *psStructure)
{
	StructureBounds b = getStructureBounds(psStructure);

	for (int i = 0; i < b.size.x; i++)
	{
		for (int j = 0; j < b.size.y; j++)
		{
			auxSetEnemy(b.map.x + i, b.map.y + j, psStructure->player, AUXBITS_NONPASSABLE);
			auxSetAll(b.map.x + i, b.map.y + j, AUXBITS_BLOCKING);
		}
	}
}

bool IsStatExpansionModule(const STRUCTURE_STATS *psStats)
{
	// If the stat is any of the 3 expansion types ... then return true
	return psStats->type == REF_POWER_MODULE ||
	       psStats->type == REF_FACTORY_MODULE  ||
	       psStats->type == REF_RESEARCH_MODULE;
}

static int numStructureModules(STRUCTURE const *psStruct)
{
	return psStruct->capacity;
}

bool structureIsBlueprint(const STRUCTURE *psStructure)
{
	return (psStructure->status == SS_BLUEPRINT_VALID ||
	        psStructure->status == SS_BLUEPRINT_INVALID ||
	        psStructure->status == SS_BLUEPRINT_PLANNED ||
	        psStructure->status == SS_BLUEPRINT_PLANNED_BY_ALLY);
}

bool isBlueprint(const BASE_OBJECT *psObject)
{
	return psObject->type == OBJ_STRUCTURE && structureIsBlueprint((STRUCTURE *)psObject);
}

void initStructLimits()
{
	for (int i = 0; i < numStructureStats; i++)
	{
		memset(asStructureStats[i].curCount, 0, sizeof(asStructureStats[i].curCount));
	}
}

void structureInitVars()
{
	asStructureStats = nullptr;
	numStructureStats = 0;
	factoryModuleStat = 0;
	powerModuleStat = 0;
	researchModuleStat = 0;
	lastMaxUnitMessage = 0;

	initStructLimits();
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		droidLimit[i] = INT16_MAX;
		commanderLimit[i] = INT16_MAX;
		constructorLimit[i] = INT16_MAX;
		for (int j = 0; j < NUM_FLAG_TYPES; j++)
		{
			factoryNumFlag[i][j].clear();
		}
	}
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		satUplinkExists[i] = false;
		lasSatExists[i] = false;
	}
	//initialise the selectedPlayer's production run
	for (unsigned type = 0; type < NUM_FACTORY_TYPES; ++type)
	{
		asProductionRun[type].clear();
	}
	//set up at beginning of game which player will have a production list
	productionPlayer = (SBYTE)selectedPlayer;
}

/*Initialise the production list and set up the production player*/
void changeProductionPlayer(UBYTE player)
{
	//clear the production run
	for (unsigned type = 0; type < NUM_FACTORY_TYPES; ++type)
	{
		asProductionRun[type].clear();
	}
	//set this player to have the production list
	productionPlayer = player;
}


/*initialises the flag before a new data set is loaded up*/
void initFactoryNumFlag()
{
	for (unsigned i = 0; i < MAX_PLAYERS; i++)
	{
		//initialise the flag
		for (unsigned j = 0; j < NUM_FLAG_TYPES; j++)
		{
			factoryNumFlag[i][j].clear();
		}
	}
}

//called at start of missions
void resetFactoryNumFlag()
{
	for (unsigned int i = 0; i < MAX_PLAYERS; i++)
	{
		for (int type = 0; type < NUM_FLAG_TYPES; type++)
		{
			// reset them all to false
			factoryNumFlag[i][type].clear();
		}
		//look through the list of structures to see which have been used
		for (STRUCTURE *psStruct = apsStructLists[i]; psStruct != nullptr; psStruct = psStruct->psNext)
		{
			FLAG_TYPE type;
			switch (psStruct->pStructureType->type)
			{
			case REF_FACTORY:        type = FACTORY_FLAG; break;
			case REF_CYBORG_FACTORY: type = CYBORG_FLAG;  break;
			case REF_VTOL_FACTORY:   type = VTOL_FLAG;    break;
			case REF_REPAIR_FACILITY: type = REPAIR_FLAG; break;
			default: continue;
			}

			int inc = -1;
			if (type == REPAIR_FLAG)
			{
				REPAIR_FACILITY *psRepair = &psStruct->pFunctionality->repairFacility;
				if (psRepair->psDeliveryPoint != nullptr)
				{
					inc = psRepair->psDeliveryPoint->factoryInc;
				}
			}
			else
			{
				FACTORY *psFactory = &psStruct->pFunctionality->factory;
				if (psFactory->psAssemblyPoint != nullptr)
				{
					inc = psFactory->psAssemblyPoint->factoryInc;
				}
			}
			if (inc >= 0)
			{
				factoryNumFlag[i][type].resize(std::max<size_t>(factoryNumFlag[i][type].size(), inc + 1), false);
				factoryNumFlag[i][type][inc] = true;
			}
		}
	}
}

static const StringToEnum<STRUCTURE_TYPE> map_STRUCTURE_TYPE[] =
{
	{ "HQ",                 REF_HQ                  },
	{ "FACTORY",            REF_FACTORY             },
	{ "FACTORY MODULE",     REF_FACTORY_MODULE      },
	{ "RESEARCH",           REF_RESEARCH            },
	{ "RESEARCH MODULE",    REF_RESEARCH_MODULE     },
	{ "POWER GENERATOR",    REF_POWER_GEN           },
	{ "POWER MODULE",       REF_POWER_MODULE        },
	{ "RESOURCE EXTRACTOR", REF_RESOURCE_EXTRACTOR  },
	{ "DEFENSE",            REF_DEFENSE             },
	{ "WALL",               REF_WALL                },
	{ "CORNER WALL",        REF_WALLCORNER          },
	{ "REPAIR FACILITY",    REF_REPAIR_FACILITY     },
	{ "COMMAND RELAY",      REF_COMMAND_CONTROL     },
	{ "DEMOLISH",           REF_DEMOLISH            },
	{ "CYBORG FACTORY",     REF_CYBORG_FACTORY      },
	{ "VTOL FACTORY",       REF_VTOL_FACTORY        },
	{ "LAB",                REF_LAB                 },
	{ "GENERIC",            REF_GENERIC             },
	{ "REARM PAD",          REF_REARM_PAD           },
	{ "MISSILE SILO",       REF_MISSILE_SILO        },
	{ "SAT UPLINK",         REF_SAT_UPLINK          },
	{ "GATE",               REF_GATE                },
};

static const StringToEnum<STRUCT_STRENGTH> map_STRUCT_STRENGTH[] =
{
	{"SOFT",        STRENGTH_SOFT           },
	{"MEDIUM",      STRENGTH_MEDIUM         },
	{"HARD",        STRENGTH_HARD           },
	{"BUNKER",      STRENGTH_BUNKER         },
};

static void initModuleStats(unsigned i, STRUCTURE_TYPE type)
{
	//need to work out the stats for the modules - HACK! - But less hacky than what was here before.
	switch (type)
	{
	case REF_FACTORY_MODULE:
		//store the stat for easy access later on
		factoryModuleStat = i;
		break;

	case REF_RESEARCH_MODULE:
		//store the stat for easy access later on
		researchModuleStat = i;
		break;

	case REF_POWER_MODULE:
		//store the stat for easy access later on
		powerModuleStat = i;
		break;
	default:
		break;
	}
}

template <typename T, size_t N>
inline
size_t sizeOfArray(const T(&)[ N ])
{
	return N;
}

/* load the structure stats from the ini file */
bool loadStructureStats(const QString& filename)
{
	QMap<QString, STRUCTURE_TYPE> structType;
	for (int i = 0; i < sizeOfArray(map_STRUCTURE_TYPE); i++)
	{
		structType.insert(map_STRUCTURE_TYPE[i].string, map_STRUCTURE_TYPE[i].value);
	}

	QMap<QString, STRUCT_STRENGTH> structStrength;
	for (int i = 0; i < sizeOfArray(map_STRUCT_STRENGTH); i++)
	{
		structStrength.insert(map_STRUCT_STRENGTH[i].string, map_STRUCT_STRENGTH[i].value);
	}

	WzConfig ini(filename, WzConfig::ReadOnlyAndRequired);
	QStringList list = ini.childGroups();
	asStructureStats = new STRUCTURE_STATS[list.size()];
	numStructureStats = list.size();
	for (int inc = 0; inc < list.size(); ++inc)
	{
		ini.beginGroup(list[inc]);
		STRUCTURE_STATS *psStats = &asStructureStats[inc];
		loadStats(ini, psStats, inc);

		psStats->ref = REF_STRUCTURE_START + inc;

		// set structure type
		QString type = ini.value("type", "").toString();
		ASSERT_OR_RETURN(false, structType.contains(type), "Invalid type '%s' of structure '%s'", type.toUtf8().constData(), getID(psStats));
		psStats->type = structType[type];

		// save indexes of special structures for futher use
		initModuleStats(inc, psStats->type);  // This function looks like a hack. But slightly less hacky than before.

		if (ini.contains("userLimits"))
		{
			Vector3i limits = ini.vector3i("userLimits");
			psStats->minLimit = limits[0];
			psStats->maxLimit = limits[2];
			psStats->base.limit = limits[1];
		}
		else
		{
			psStats->minLimit = 0;
			psStats->maxLimit = LOTS_OF;
			psStats->base.limit = LOTS_OF;
		}
		psStats->base.research = ini.value("researchPoints", 0).toInt();
		psStats->base.moduleResearch = ini.value("moduleResearchPoints", 0).toInt();
		psStats->base.production = ini.value("productionPoints", 0).toInt();
		psStats->base.moduleProduction = ini.value("moduleProductionPoints", 0).toInt();
		psStats->base.repair = ini.value("repairPoints", 0).toInt();
		psStats->base.power = ini.value("powerPoints", 0).toInt();
		psStats->base.modulePower = ini.value("modulePowerPoints", 0).toInt();
		psStats->base.rearm = ini.value("rearmPoints", 0).toInt();
		psStats->base.resistance = ini.value("resistance", 0).toUInt();
		psStats->base.hitpoints = ini.value("hitpoints", 1).toUInt();
		psStats->base.armour = ini.value("armour", 0).toUInt();
		psStats->base.thermal = ini.value("thermal", 0).toUInt();
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			psStats->upgrade[i].limit = psStats->base.limit;
			psStats->upgrade[i].research = psStats->base.research;
			psStats->upgrade[i].moduleResearch = psStats->base.moduleResearch;
			psStats->upgrade[i].power = psStats->base.power;
			psStats->upgrade[i].modulePower = psStats->base.modulePower;
			psStats->upgrade[i].repair = psStats->base.repair;
			psStats->upgrade[i].production = psStats->base.production;
			psStats->upgrade[i].moduleProduction = psStats->base.moduleProduction;
			psStats->upgrade[i].rearm = psStats->base.rearm;
			psStats->upgrade[i].resistance = ini.value("resistance", 0).toUInt();
			psStats->upgrade[i].hitpoints = ini.value("hitpoints", 1).toUInt();
			psStats->upgrade[i].armour = ini.value("armour", 0).toUInt();
			psStats->upgrade[i].thermal = ini.value("thermal", 0).toUInt();
		}

		psStats->flags = 0;
		QStringList flags = ini.value("flags").toStringList();
		for (int i = 0; i < flags.size(); i++)
		{
			if (flags[i] == "Connected")
			{
				psStats->flags |= STRUCTURE_CONNECTED;
			}
		}

		// set structure strength
		QString strength = ini.value("strength", "").toString();
		ASSERT_OR_RETURN(false, structStrength.contains(strength), "Invalid strength '%s' of structure '%s'", strength.toUtf8().constData(), getID(psStats));
		psStats->strength = structStrength[strength];

		// set baseWidth
		psStats->baseWidth = ini.value("width", 0).toUInt();
		ASSERT_OR_RETURN(false, psStats->baseWidth <= 100, "Invalid width '%d' for structure '%s'", psStats->baseWidth, getID(psStats));

		// set baseBreadth
		psStats->baseBreadth = ini.value("breadth", 0).toUInt();
		ASSERT_OR_RETURN(false, psStats->baseBreadth < 100, "Invalid breadth '%d' for structure '%s'", psStats->baseBreadth, getID(psStats));

		psStats->height = ini.value("height").toUInt();
		psStats->powerToBuild = ini.value("buildPower").toUInt();
		psStats->buildPoints = ini.value("buildPoints").toUInt();

		// set structure models
		QStringList models = ini.value("structureModel").toStringList();
		for (int j = 0; j < models.size(); j++)
		{
			iIMDShape *imd = modelGet(models[j].trimmed());
			ASSERT(imd != nullptr, "Cannot find the PIE structureModel '%s' for structure '%s'", models[j].toUtf8().constData(), getID(psStats));
			psStats->pIMD.push_back(imd);
		}

		// set base model
		QString baseModel = ini.value("baseModel", "").toString();
		if (baseModel.compare("") != 0)
		{
			iIMDShape *imd = modelGet(baseModel);
			ASSERT(imd != nullptr, "Cannot find the PIE baseModel '%s' for structure '%s'", baseModel.toUtf8().constData(), getID(psStats));
			psStats->pBaseIMD = imd;
		}

		int ecm = getCompFromName(COMP_ECM, ini.value("ecmID", "ZNULLECM").toString());
		ASSERT(ecm >= 0, "Invalid ECM found for '%s'", getID(psStats));
		psStats->pECM = asECMStats + ecm;

		int sensor = getCompFromName(COMP_SENSOR, ini.value("sensorID", "ZNULLSENSOR").toString());
		ASSERT(sensor >= 0, "Invalid sensor found for structure '%s'", getID(psStats));
		psStats->pSensor = asSensorStats + sensor;

		// set list of weapons
		std::fill_n(psStats->psWeapStat, MAX_WEAPONS, (WEAPON_STATS *)nullptr);
		QStringList weapons = ini.value("weapons").toStringList();
		ASSERT_OR_RETURN(false, weapons.size() <= MAX_WEAPONS, "Too many weapons are attached to structure '%s'. Maximum is %d", getID(psStats), MAX_WEAPONS);
		psStats->numWeaps = weapons.size();
		for (int j = 0; j < psStats->numWeaps; j++)
		{
			QString weaponsID = weapons[j].trimmed();
			int weapon = getCompFromName(COMP_WEAPON, weaponsID);
			ASSERT_OR_RETURN(false, weapon >= 0, "Invalid item '%s' in list of weapons of structure '%s' ", weaponsID.toUtf8().constData(), getID(psStats));
			WEAPON_STATS *pWeap = asWeaponStats + weapon;
			psStats->psWeapStat[j] = pWeap;
		}

		// check used structure turrets
		int types = 0;
		types += psStats->numWeaps != 0;
		types += psStats->pECM != nullptr && psStats->pECM->location == LOC_TURRET;
		types += psStats->pSensor != nullptr && psStats->pSensor->location == LOC_TURRET;
		ASSERT(types <= 1, "Too many turret types for structure '%s'", getID(psStats));

		ini.endGroup();
	}

	/* get global dummy stat pointer - GJ */
	g_psStatDestroyStruct = nullptr;
	for (int iID = 0; iID < numStructureStats; iID++)
	{
		if (asStructureStats[iID].type == REF_DEMOLISH)
		{
			g_psStatDestroyStruct = asStructureStats + iID;
			break;
		}
	}
	ASSERT_OR_RETURN(false, g_psStatDestroyStruct, "Destroy structure stat not found");

	return true;
}

/* set the current number of structures of each type built */
void setCurrentStructQuantity(bool displayError)
{
	for (unsigned player = 0; player < MAX_PLAYERS; player++)
	{
		for (unsigned inc = 0; inc < numStructureStats; inc++)
		{
			asStructureStats[inc].curCount[player] = 0;
		}
		for (const STRUCTURE *psCurr = apsStructLists[player]; psCurr != nullptr; psCurr = psCurr->psNext)
		{
			unsigned inc = psCurr->pStructureType - asStructureStats;
			asStructureStats[inc].curCount[player]++;
			if (displayError)
			{
				//check quantity never exceeds the limit
				ASSERT(asStructureStats[inc].curCount[player] <= asStructureStats[inc].upgrade[player].limit,
				       "There appears to be too many %s on this map!", getName(&asStructureStats[inc]));
			}
		}
	}
}

/*Load the Structure Strength Modifiers from the file exported from Access*/
bool loadStructureStrengthModifiers(const char *pFileName)
{
	//initialise to 100%
	for (unsigned i = 0; i < WE_NUMEFFECTS; ++i)
	{
		for (unsigned j = 0; j < NUM_STRUCT_STRENGTH; ++j)
		{
			asStructStrengthModifier[i][j] = 100;
		}
	}
	WzConfig ini(pFileName, WzConfig::ReadOnlyAndRequired);
	QStringList list = ini.childGroups();
	for (int i = 0; i < list.size(); i++)
	{
		WEAPON_EFFECT effectInc;
		ini.beginGroup(list[i]);
		if (!getWeaponEffect(list[i].toUtf8().constData(), &effectInc))
		{
			debug(LOG_FATAL, "Invalid Weapon Effect - %s", list[i].toUtf8().constData());
			ini.endGroup();
			continue;
		}
		QStringList keys = ini.childKeys();
		for (int j = 0; j < keys.size(); j++)
		{
			const QString& strength = keys.at(j);
			int modifier = ini.value(strength).toInt();
			// FIXME - add support for dynamic categories
			if (strength.compare("SOFT") == 0)
			{
				asStructStrengthModifier[effectInc][0] = modifier;
			}
			else if (strength.compare("MEDIUM") == 0)
			{
				asStructStrengthModifier[effectInc][1] = modifier;
			}
			else if (strength.compare("HARD") == 0)
			{
				asStructStrengthModifier[effectInc][2] = modifier;
			}
			else if (strength.compare("BUNKER") == 0)
			{
				asStructStrengthModifier[effectInc][3] = modifier;
			}
			else
			{
				debug(LOG_ERROR, "Unsupported structure strength %s", strength.toUtf8().constData());
			}
		}
		ini.endGroup();
	}
	return true;
}

bool structureStatsShutDown()
{
	delete[] asStructureStats;
	asStructureStats = nullptr;
	numStructureStats = 0;
	return true;
}

// TODO: The abandoned code needs to be factored out, see: saveMissionData
void handleAbandonedStructures()
{
	// TODO: do something here
}

/* Deals damage to a Structure.
 * \param psStructure structure to deal damage to
 * \param damage amount of damage to deal
 * \param weaponClass the class of the weapon that deals the damage
 * \param weaponSubClass the subclass of the weapon that deals the damage
 * \return < 0 when the dealt damage destroys the structure, > 0 when the structure survives
 */
int32_t structureDamage(STRUCTURE *psStructure, unsigned damage, WEAPON_CLASS weaponClass, WEAPON_SUBCLASS weaponSubClass, unsigned impactTime, bool isDamagePerSecond, int minDamage)
{
	int32_t relativeDamage;

	CHECK_STRUCTURE(psStructure);

	debug(LOG_ATTACK, "structure id %d, body %d, armour %d, damage: %d",
	      psStructure->id, psStructure->body, objArmour(psStructure, weaponClass), damage);

	relativeDamage = objDamage(psStructure, damage, structureBody(psStructure), weaponClass, weaponSubClass, isDamagePerSecond, minDamage);

	// If the shell did sufficient damage to destroy the structure
	if (relativeDamage < 0)
	{
		debug(LOG_ATTACK, "Structure (id %d) DESTROYED", psStructure->id);
		destroyStruct(psStructure, impactTime);
	}
	else
	{
		// Survived
		CHECK_STRUCTURE(psStructure);
	}

	return relativeDamage;
}

int32_t getStructureDamage(const STRUCTURE *psStructure)
{
	CHECK_STRUCTURE(psStructure);

	unsigned maxBody = structureBodyBuilt(psStructure);

	int64_t health = (int64_t)65536 * psStructure->body / MAX(1, maxBody);
	CLIP(health, 0, 65536);

	return 65536 - health;
}

/// Add buildPoints to the structures currentBuildPts, due to construction work by the droid
/// Also can deconstruct (demolish) a building if passed negative buildpoints
void structureBuild(STRUCTURE *psStruct, DROID *psDroid, int buildPoints, int buildRate)
{
	bool checkResearchButton = psStruct->status == SS_BUILT;  // We probably just started demolishing, if this is true.
	int prevResearchState = 0;
	if (checkResearchButton)
	{
		prevResearchState = intGetResearchState();
	}

	if (psDroid && !aiCheckAlliances(psStruct->player, psDroid->player))
	{
		// Enemy structure
		return;
	}
	else if (psStruct->pStructureType->type != REF_FACTORY_MODULE)
	{
		for (unsigned player = 0; player < MAX_PLAYERS; player++)
		{
			for (DROID *psCurr = apsDroidLists[player]; psCurr != nullptr; psCurr = psCurr->psNext)
			{
				// An enemy droid is blocking it
				if ((STRUCTURE *) orderStateObj(psCurr, DORDER_BUILD) == psStruct
				    && !aiCheckAlliances(psStruct->player, psCurr->player))
				{
					return;
				}
			}
		}
	}
	psStruct->buildRate += buildRate;  // buildRate = buildPoints/GAME_UPDATES_PER_SEC, but might be rounded up or down each tick, so can't use buildPoints to get a stable number.
	if (psStruct->currentBuildPts <= 0 && buildPoints > 0)
	{
		// Just starting to build structure, need power for it.
		bool haveEnoughPower = requestPowerFor(psStruct, structPowerToBuild(psStruct));
		if (!haveEnoughPower)
		{
			buildPoints = 0;  // No power to build.
		}
		triggerEventStructBeginBuilt(psStruct, false, psDroid);
	}

	if (psStruct->body == 1000 && buildPoints < 0)
	{
		triggerEventStructBeginBuilt(psStruct, true, psDroid);
	}

	int newBuildPoints = psStruct->currentBuildPts + buildPoints;
	ASSERT(newBuildPoints <= 1 + 3 * (int)psStruct->pStructureType->buildPoints, "unsigned int underflow?");
	CLIP(newBuildPoints, 0, psStruct->pStructureType->buildPoints);

	if (psStruct->currentBuildPts > 0 && newBuildPoints <= 0)
	{
		// Demolished structure, return some power.
		addPower(psStruct->player, structureTotalReturn(psStruct));
	}

	ASSERT(newBuildPoints <= 1 + 3 * (int)psStruct->pStructureType->buildPoints, "unsigned int underflow?");
	CLIP(newBuildPoints, 0, psStruct->pStructureType->buildPoints);

	int deltaBody = quantiseFraction(9 * structureBody(psStruct), 10 * psStruct->pStructureType->buildPoints, newBuildPoints, psStruct->currentBuildPts);
	psStruct->currentBuildPts = newBuildPoints;
	psStruct->body = std::max<int>(psStruct->body + deltaBody, 1);

	//check if structure is built
	if (buildPoints > 0 && psStruct->currentBuildPts >= (SDWORD)psStruct->pStructureType->buildPoints)
	{
		buildingComplete(psStruct);

		if (psDroid)
		{
			intBuildFinished(psDroid);
		}

		//only play the sound if selected player
		if (psDroid &&
		    psStruct->player == selectedPlayer
		    && (psDroid->order.type != DORDER_LINEBUILD
		        || map_coord(psDroid->order.pos) == map_coord(psDroid->order.pos2)))
		{
			audio_QueueTrackPos(ID_SOUND_STRUCTURE_COMPLETED,
			                    psStruct->pos.x, psStruct->pos.y, psStruct->pos.z);
			intRefreshScreen();		// update any open interface bars.
		}

		/* must reset here before the callback, droid must have DACTION_NONE
		     in order to be able to start a new built task, doubled in actionUpdateDroid() */
		if (psDroid)
		{
			DROID	*psIter;

			// Clear all orders for helping hands. Needed for AI script which runs next frame.
			for (psIter = apsDroidLists[psDroid->player]; psIter; psIter = psIter->psNext)
			{
				if ((psIter->order.type == DORDER_BUILD || psIter->order.type == DORDER_HELPBUILD || psIter->order.type == DORDER_LINEBUILD)
				    && psIter->order.psObj == psStruct
				    && (psIter->order.type != DORDER_LINEBUILD || map_coord(psIter->order.pos) == map_coord(psIter->order.pos2)))
				{
					objTrace(psIter->id, "Construction order %s complete (%d, %d -> %d, %d)", getDroidOrderName(psDroid->order.type),
					         psIter->order.pos2.x, psIter->order.pos.y, psIter->order.pos2.x, psIter->order.pos2.y);
					psIter->action = DACTION_NONE;
					psIter->order = DroidOrder(DORDER_NONE);
					setDroidActionTarget(psIter, nullptr, 0);
				}
			}

			/* Notify scripts we just finished building a structure, pass builder and what was built */
			psScrCBNewStruct	= psStruct;
			psScrCBNewStructTruck	= psDroid;
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_STRUCTBUILT);

			audio_StopObjTrack(psDroid, ID_SOUND_CONSTRUCTION_LOOP);
		}
		triggerEventStructBuilt(psStruct, psDroid);

		/* Not needed, but left for backward compatibility */
		structureCompletedCallback(psStruct->pStructureType);
	}
	else
	{
		STRUCT_STATES prevStatus = psStruct->status;
		psStruct->status = SS_BEING_BUILT;
		if (prevStatus == SS_BUILT)
		{
			// Starting to demolish.
			switch (psStruct->pStructureType->type)
			{
			case REF_POWER_GEN:
				releasePowerGen(psStruct);
				break;
			case REF_RESOURCE_EXTRACTOR:
				releaseResExtractor(psStruct);
				break;
			default:
				break;
			}
		}
	}
	if (buildPoints < 0 && psStruct->currentBuildPts == 0)
	{
		triggerEvent(TRIGGER_OBJECT_RECYCLED, psStruct);
		removeStruct(psStruct, true);
	}

	if (checkResearchButton)
	{
		intNotifyResearchButton(prevResearchState);
	}
}

static bool structureHasModules(const STRUCTURE *psStruct)
{
	return psStruct->capacity != 0;
}

// Power returned on demolish. Not sure why it is done this way. FIXME.
static int structureTotalReturn(const STRUCTURE *psStruct)
{
	int power = structPowerToBuild(psStruct);
	return psStruct->capacity ? power : power / 2;
}

void structureDemolish(STRUCTURE *psStruct, DROID *psDroid, int buildPoints)
{
	structureBuild(psStruct, psDroid, -buildPoints);
}

void structureRepair(STRUCTURE *psStruct, DROID *psDroid, int buildRate)
{
	int repairAmount = gameTimeAdjustedAverage(buildRate * structureBody(psStruct), psStruct->pStructureType->buildPoints);
	/*	(droid construction power * current max hitpoints [incl. upgrades])
			/ construction power that was necessary to build structure in the first place

	=> to repair a building from 1HP to full health takes as much time as building it.
	=> if buildPoints = 1 and structureBody < buildPoints, repairAmount might get truncated to zero.
		This happens with expensive, but weak buildings like mortar pits. In this case, do nothing
		and notify the caller (read: droid) of your idleness by returning false.
	*/
	psStruct->body = clip(psStruct->body + repairAmount, 0, structureBody(psStruct));
}

/* Set the type of droid for a factory to build */
bool structSetManufacture(STRUCTURE *psStruct, DROID_TEMPLATE *psTempl, QUEUE_MODE mode)
{
	FACTORY *psFact;

	CHECK_STRUCTURE(psStruct);

	ASSERT_OR_RETURN(false, StructIsFactory(psStruct), "structure not a factory");

	/* psTempl might be NULL if the build is being cancelled in the middle */
	ASSERT_OR_RETURN(false, !psTempl
	                 || (validTemplateForFactory(psTempl, psStruct, true) && researchedTemplate(psTempl, psStruct->player, true, true))
	                 || psStruct->player == scavengerPlayer() || !bMultiPlayer,
	                 "Wrong template for player %d factory, type %d.", psStruct->player, psStruct->pStructureType->type);

	psFact = &psStruct->pFunctionality->factory;

	if (mode == ModeQueue)
	{
		sendStructureInfo(psStruct, STRUCTUREINFO_MANUFACTURE, psTempl);
		setStatusPendingStart(*psFact, psTempl);
		return true;  // Wait for our message before doing anything.
	}

	//assign it to the Factory
	psFact->psSubject = psTempl;

	//set up the start time and build time
	if (psTempl != nullptr)
	{
		//only use this for non selectedPlayer
		if (psStruct->player != selectedPlayer)
		{
			//set quantity to produce
			psFact->productionLoops = 1;
		}

		psFact->timeStarted = ACTION_START_TIME;//gameTime;
		psFact->timeStartHold = 0;

		psFact->buildPointsRemaining = calcTemplateBuild(psTempl);
		//check for zero build time - usually caused by 'silly' data! If so, set to 1 build point - ie very fast!
		psFact->buildPointsRemaining = std::max(psFact->buildPointsRemaining, 1);
	}
	if (psStruct->player == productionPlayer)
	{
		intUpdateManufacture(psStruct);
	}
	return true;
}


/*****************************************************************************/
/*
* All this wall type code is horrible, but I really can't think of a better way to do it.
*        John.
*/

enum WallOrientation
{
	WallConnectNone = 0,
	WallConnectLeft = 1,
	WallConnectRight = 2,
	WallConnectUp = 4,
	WallConnectDown = 8,
};

// Orientations are:
//
//  0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
//                  |   |   |   |                   |   |   |   |
//  *  -*   *- -*-  *  -*   *- -*-  *  -*   *- -*-  *  -*   *- -*-
//                                  |   |   |   |   |   |   |   |

// IMDs are:
//
//  0   1   2   3
//      |   |   |
// -*- -*- -*- -*
//      |

// Orientations are:                   IMDs are:
// 0 1 2 3 4 5 6 7 8 9 A B C D E F     0 1 2 3
//   ╴ ╶ ─ ╵ ┘ └ ┴ ╷ ┐ ┌ ┬ │ ┤ ├ ┼     ─ ┼ ┴ ┘

static uint16_t wallDir(WallOrientation orient)
{
	const uint16_t d0 = DEG(0), d1 = DEG(90), d2 = DEG(180), d3 = DEG(270);  // d1 = rotate ccw, d3 = rotate cw
	//                    0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	uint16_t dirs[16] = {d0, d0, d2, d0, d3, d0, d3, d0, d1, d1, d2, d2, d3, d1, d3, d0};
	return dirs[orient];
}

static uint16_t wallType(WallOrientation orient)
{
	//               0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
	int types[16] = {0, 0, 0, 0, 0, 3, 3, 2, 0, 3, 3, 2, 0, 2, 2, 1};
	return types[orient];
}

// look at where other walls are to decide what type of wall to build
static WallOrientation structWallScan(bool aWallPresent[5][5], int x, int y)
{
	WallOrientation left  = aWallPresent[x - 1][y] ? WallConnectLeft  : WallConnectNone;
	WallOrientation right = aWallPresent[x + 1][y] ? WallConnectRight : WallConnectNone;
	WallOrientation up    = aWallPresent[x][y - 1] ? WallConnectUp    : WallConnectNone;
	WallOrientation down  = aWallPresent[x][y + 1] ? WallConnectDown  : WallConnectNone;
	return WallOrientation(left | right | up | down);
}

static bool isWallCombiningStructureType(STRUCTURE_STATS const *pStructureType)
{
	STRUCTURE_TYPE type = pStructureType->type;
	STRUCT_STRENGTH strength = pStructureType->strength;
	return type == REF_WALL ||
	       type == REF_GATE ||
	       type == REF_WALLCORNER ||
	       (type == REF_DEFENSE && strength == STRENGTH_HARD) ||
	       (type == REF_GENERIC && strength == STRENGTH_HARD);  // fortresses
}

bool isWall(STRUCTURE_TYPE type)
{
	return type == REF_WALL || type == REF_WALLCORNER;
}

bool isBuildableOnWalls(STRUCTURE_TYPE type)
{
	return type == REF_DEFENSE || type == REF_GATE;
}

static void structFindWalls(unsigned player, Vector2i map, bool aWallPresent[5][5], STRUCTURE *apsStructs[5][5])
{
	for (int y = -2; y <= 2; ++y)
		for (int x = -2; x <= 2; ++x)
		{
			STRUCTURE *psStruct = castStructure(mapTile(map.x + x, map.y + y)->psObject);
			if (psStruct != nullptr && isWallCombiningStructureType(psStruct->pStructureType) && aiCheckAlliances(player, psStruct->player))
			{
				aWallPresent[x + 2][y + 2] = true;
				apsStructs[x + 2][y + 2] = psStruct;
			}
		}
	// add in the wall about to be built
	aWallPresent[2][2] = true;
}

static void structFindWallBlueprints(Vector2i map, bool aWallPresent[5][5])
{
	for (int y = -2; y <= 2; ++y)
		for (int x = -2; x <= 2; ++x)
		{
			STRUCTURE_STATS const *stats = getTileBlueprintStats(map.x + x, map.y + y);
			if (stats != nullptr && isWallCombiningStructureType(stats))
			{
				aWallPresent[x + 2][y + 2] = true;
			}
		}
}

static bool wallBlockingTerrainJoin(Vector2i map)
{
	MAPTILE *psTile = mapTile(map);
	return terrainType(psTile) == TER_WATER || terrainType(psTile) == TER_CLIFFFACE || psTile->psObject != nullptr;
}

static WallOrientation structWallScanTerrain(bool aWallPresent[5][5], Vector2i map)
{
	WallOrientation orientation = structWallScan(aWallPresent, 2, 2);

	if (orientation == WallConnectNone)
	{
		// If neutral, try choosing horizontal or vertical based on terrain, but don't change to corner type.
		aWallPresent[2][1] = wallBlockingTerrainJoin(map + Vector2i(0, -1));
		aWallPresent[2][3] = wallBlockingTerrainJoin(map + Vector2i(0,  1));
		aWallPresent[1][2] = wallBlockingTerrainJoin(map + Vector2i(-1,  0));
		aWallPresent[3][2] = wallBlockingTerrainJoin(map + Vector2i(1,  0));
		orientation = structWallScan(aWallPresent, 2, 2);
		if ((orientation & (WallConnectLeft | WallConnectRight)) != 0 && (orientation & (WallConnectUp | WallConnectDown)) != 0)
		{
			orientation = WallConnectNone;
		}
	}

	return orientation;
}

static WallOrientation structChooseWallTypeBlueprint(Vector2i map)
{
	bool            aWallPresent[5][5];
	STRUCTURE      *apsStructs[5][5];

	// scan around the location looking for walls
	memset(aWallPresent, 0, sizeof(aWallPresent));
	structFindWalls(selectedPlayer, map, aWallPresent, apsStructs);
	structFindWallBlueprints(map, aWallPresent);

	// finally return the type for this wall
	return structWallScanTerrain(aWallPresent, map);
}

// Choose a type of wall for a location - and update any neighbouring walls
static WallOrientation structChooseWallType(unsigned player, Vector2i map)
{
	bool		aWallPresent[5][5];
	STRUCTURE	*psStruct;
	STRUCTURE	*apsStructs[5][5];

	// scan around the location looking for walls
	memset(aWallPresent, 0, sizeof(aWallPresent));
	structFindWalls(player, map, aWallPresent, apsStructs);

	// now make sure that all the walls around this one are OK
	for (int x = 1; x <= 3; ++x)
	{
		for (int y = 1; y <= 3; ++y)
		{
			// do not look at walls diagonally from this wall
			if (((x == 2 && y != 2) ||
			     (x != 2 && y == 2)) &&
			    aWallPresent[x][y])
			{
				// figure out what type the wall currently is
				psStruct = apsStructs[x][y];
				if (psStruct->pStructureType->type != REF_WALL && psStruct->pStructureType->type != REF_GATE)
				{
					// do not need to adjust anything apart from walls
					continue;
				}

				// see what type the wall should be
				WallOrientation scanType = structWallScan(aWallPresent, x, y);

				// Got to change the wall
				if (scanType != WallConnectNone)
				{
					psStruct->pFunctionality->wall.type = wallType(scanType);
					psStruct->rot.direction = wallDir(scanType);
					psStruct->sDisplay.imd = psStruct->pStructureType->pIMD[std::min<unsigned>(psStruct->pFunctionality->wall.type, psStruct->pStructureType->pIMD.size() - 1)];
				}
			}
		}
	}

	// finally return the type for this wall
	return structWallScanTerrain(aWallPresent, map);
}


/* For now all this does is work out what height the terrain needs to be set to
An actual foundation structure may end up being placed down
The x and y passed in are the CENTRE of the structure*/
static int foundationHeight(const STRUCTURE *psStruct)
{
	StructureBounds b = getStructureBounds(psStruct);

	//check the terrain is the correct type return -1 if not

	//may also have to check that overlapping terrain can be set to the average height
	//eg water - don't want it to 'flow' into the structure if this effect is coded!

	//initialise the starting values so they get set in loop
	int foundationMin = INT32_MAX;
	int foundationMax = INT32_MIN;

	for (int breadth = 0; breadth <= b.size.y; breadth++)
	{
		for (int width = 0; width <= b.size.x; width++)
		{
			int height = map_TileHeight(b.map.x + width, b.map.y + breadth);
			foundationMin = std::min(foundationMin, height);
			foundationMax = std::max(foundationMax, height);
		}
	}
	//return the average of max/min height
	return (foundationMin + foundationMax) / 2;
}


static void buildFlatten(STRUCTURE *pStructure, int h)
{
	StructureBounds b = getStructureBounds(pStructure);

	for (int breadth = 0; breadth <= b.size.y; ++breadth)
	{
		for (int width = 0; width <= b.size.x; ++width)
		{
			setTileHeight(b.map.x + width, b.map.y + breadth, h);
			// We need to raise features on raised tiles to the new height
			if (TileHasFeature(mapTile(b.map.x + width, b.map.y + breadth)))
			{
				getTileFeature(b.map.x + width, b.map.y + breadth)->pos.z = h;
			}
		}
	}
}

static bool isPulledToTerrain(const STRUCTURE *psBuilding)
{
	STRUCTURE_TYPE type = psBuilding->pStructureType->type;
	return type == REF_DEFENSE || type == REF_GATE || type == REF_WALL || type == REF_WALLCORNER || type == REF_REARM_PAD;
}

void alignStructure(STRUCTURE *psBuilding)
{
	/* DEFENSIVE structures are pulled to the terrain */
	if (!isPulledToTerrain(psBuilding))
	{
		int mapH = foundationHeight(psBuilding);

		buildFlatten(psBuilding, mapH);
		psBuilding->pos.z = mapH;
		psBuilding->foundationDepth = psBuilding->pos.z;

		// Align surrounding structures.
		StructureBounds b = getStructureBounds(psBuilding);
		syncDebug("Flattened (%d+%d, %d+%d) to %d for %d(p%d)", b.map.x, b.size.x, b.map.y, b.size.y, mapH, psBuilding->id, psBuilding->player);
		for (int breadth = -1; breadth <= b.size.y; ++breadth)
		{
			for (int width = -1; width <= b.size.x; ++width)
			{
				STRUCTURE *neighbourStructure = castStructure(mapTile(b.map.x + width, b.map.y + breadth)->psObject);
				if (neighbourStructure != nullptr && isPulledToTerrain(neighbourStructure))
				{
					alignStructure(neighbourStructure);  // Recursive call, but will go to the else case, so will not re-recurse.
				}
			}
		}
	}
	else
	{
		// Sample points around the structure to find a good depth for the foundation
		iIMDShape *s = psBuilding->sDisplay.imd;

		psBuilding->pos.z = TILE_MIN_HEIGHT;
		psBuilding->foundationDepth = TILE_MAX_HEIGHT;

		Vector2i dir = iSinCosR(psBuilding->rot.direction, 1);
		// Rotate s->max.{x, z} and s->min.{x, z} by angle rot.direction.
		Vector2i p1{s->max.x * dir.y - s->max.z * dir.x, s->max.x * dir.x + s->max.z * dir.y};
		Vector2i p2{s->min.x * dir.y - s->min.z * dir.x, s->min.x * dir.x + s->min.z * dir.y};

		int h1 = map_Height(psBuilding->pos.x + p1.x, psBuilding->pos.y + p2.y);
		int h2 = map_Height(psBuilding->pos.x + p1.x, psBuilding->pos.y + p1.y);
		int h3 = map_Height(psBuilding->pos.x + p2.x, psBuilding->pos.y + p1.y);
		int h4 = map_Height(psBuilding->pos.x + p2.x, psBuilding->pos.y + p2.y);
		int minH = std::min({h1, h2, h3, h4});
		int maxH = std::max({h1, h2, h3, h4});
		psBuilding->pos.z = std::max(psBuilding->pos.z, maxH);
		psBuilding->foundationDepth = std::min<float>(psBuilding->foundationDepth, minH);
		syncDebug("minH=%d,maxH=%d,pointHeight=%d", minH, maxH, psBuilding->pos.z);  // s->max is based on floats! If this causes desynchs, need to fix!
	}
}

/*Builds an instance of a Structure - the x/y passed in are in world coords. */
STRUCTURE *buildStructure(STRUCTURE_STATS *pStructureType, UDWORD x, UDWORD y, UDWORD player, bool FromSave)
{
	return buildStructureDir(pStructureType, x, y, 0, player, FromSave);
}

STRUCTURE *buildStructureDir(STRUCTURE_STATS *pStructureType, UDWORD x, UDWORD y, uint16_t direction, UDWORD player, bool FromSave)
{
	STRUCTURE *psBuilding = nullptr;
	const Vector2i size = pStructureType->size(direction);

	ASSERT_OR_RETURN(nullptr, pStructureType && pStructureType->type != REF_DEMOLISH, "You cannot build demolition!");

	if (IsStatExpansionModule(pStructureType) == false)
	{
		SDWORD preScrollMinX = 0, preScrollMinY = 0, preScrollMaxX = 0, preScrollMaxY = 0;
		UDWORD	max = pStructureType - asStructureStats;
		int	i;

		ASSERT_OR_RETURN(nullptr, max <= numStructureStats, "Invalid structure type");

		// Don't allow more than interface limits
		if (asStructureStats[max].curCount[player] + 1 > asStructureStats[max].upgrade[player].limit)
		{
			debug(LOG_ERROR, "Player %u: Building %s could not be built due to building limits (has %u, max %u)!",
			      player, getName(pStructureType), asStructureStats[max].curCount[player],
			      asStructureStats[max].upgrade[player].limit);
			return nullptr;
		}

		// snap the coords to a tile
		x = (x & ~TILE_MASK) + size.x % 2 * TILE_UNITS / 2;
		y = (y & ~TILE_MASK) + size.y % 2 * TILE_UNITS / 2;

		//check not trying to build too near the edge
		if (map_coord(x) < TOO_NEAR_EDGE || map_coord(x) > (mapWidth - TOO_NEAR_EDGE))
		{
			debug(LOG_WARNING, "attempting to build too closely to map-edge, "
			      "x coord (%u) too near edge (req. distance is %u)", x, TOO_NEAR_EDGE);
			return nullptr;
		}
		if (map_coord(y) < TOO_NEAR_EDGE || map_coord(y) > (mapHeight - TOO_NEAR_EDGE))
		{
			debug(LOG_WARNING, "attempting to build too closely to map-edge, "
			      "y coord (%u) too near edge (req. distance is %u)", y, TOO_NEAR_EDGE);
			return nullptr;
		}

		WallOrientation wallOrientation = WallConnectNone;
		if (!FromSave && isWallCombiningStructureType(pStructureType))
		{
			for (int dy = 0; dy < size.y; ++dy)
				for (int dx = 0; dx < size.x; ++dx)
				{
					Vector2i pos = map_coord(Vector2i(x, y) - size * TILE_UNITS / 2) + Vector2i(dx, dy);
					wallOrientation = structChooseWallType(player, pos);  // This makes neighbouring walls match us, even if we're a hardpoint, not a wall.
				}
		}

		// allocate memory for and initialize a structure object
		psBuilding = new STRUCTURE(generateSynchronisedObjectId(), player);
		if (psBuilding == nullptr)
		{
			return nullptr;
		}

		//fill in other details
		psBuilding->pStructureType = pStructureType;

		psBuilding->pos.x = x;
		psBuilding->pos.y = y;
		psBuilding->rot.direction = (direction + 0x2000) & 0xC000;
		psBuilding->rot.pitch = 0;
		psBuilding->rot.roll = 0;

		//This needs to be done before the functionality bit...
		//load into the map data and structure list if not an upgrade
		Vector2i map = map_coord(Vector2i(x, y)) - size / 2;

		//set up the imd to use for the display
		psBuilding->sDisplay.imd = pStructureType->pIMD[0];

		psBuilding->state = SAS_NORMAL;
		psBuilding->lastStateTime = gameTime;

		/* if resource extractor - need to remove oil feature first, but do not do any
		 * consistency checking here - save games do not have any feature to remove
		 * to remove when placing oil derricks! */
		if (pStructureType->type == REF_RESOURCE_EXTRACTOR)
		{
			FEATURE *psFeature = getTileFeature(map_coord(x), map_coord(y));

			if (psFeature && psFeature->psStats->subType == FEAT_OIL_RESOURCE)
			{
				if (fireOnLocation(psFeature->pos.x, psFeature->pos.y))
				{
					// Can't build on burning oil resource
					delete psBuilding;
					return nullptr;
				}
				// remove it from the map
				turnOffMultiMsg(true); // don't send this one!
				removeFeature(psFeature);
				turnOffMultiMsg(false);
			}
		}

		for (int y = map.y; y < map.y + size.y; ++y)
		{
			for (int x = map.x; x < map.x + size.x; ++x)
			{
				MAPTILE *psTile = mapTile(x, y);

				/* Remove any walls underneath the building. You can build defense buildings on top
				 * of walls, you see. This is not the place to test whether we own it! */
				if (isBuildableOnWalls(pStructureType->type) && TileHasWall(psTile))
				{
					removeStruct((STRUCTURE *)psTile->psObject, true);
				}
				else if (TileHasStructure(psTile))
				{
					debug(LOG_ERROR, "Player %u (%s): is building %s at (%d, %d) but found %s already at (%d, %d)",
					      player, isHumanPlayer(player) ? "Human" : "AI", getName(pStructureType), map.x, map.y,
					      getName(getTileStructure(x, y)->pStructureType), x, y);
					delete psBuilding;
					return nullptr;
				}
			}
		}
		for (int y = map.y; y < map.y + size.y; ++y)
		{
			for (int x = map.x; x < map.x + size.x; ++x)
			{
				// We now know the previous loop didn't return early, so it is safe to save references to psBuilding now.
				MAPTILE *psTile = mapTile(x, y);
				psTile->psObject = psBuilding;

				// if it's a tall structure then flag it in the map.
				if (psBuilding->sDisplay.imd->max.y > TALLOBJECT_YMAX)
				{
					auxSetBlocking(x, y, AIR_BLOCKED);
				}
			}
		}

		switch (pStructureType->type)
		{
		case REF_REARM_PAD:
			break;  // Not blocking.
		default:
			auxStructureBlocking(psBuilding);
			break;
		}

		//set up the rest of the data
		for (i = 0; i < MAX_WEAPONS; i++)
		{
			psBuilding->asWeaps[i].rot.direction = 0;
			psBuilding->asWeaps[i].rot.pitch = 0;
			psBuilding->asWeaps[i].rot.roll = 0;
			psBuilding->asWeaps[i].prevRot = psBuilding->asWeaps[i].rot;
			psBuilding->asWeaps[i].origin = ORIGIN_UNKNOWN;
			psBuilding->psTarget[i] = nullptr;
		}

		psBuilding->periodicalDamageStart = 0;
		psBuilding->periodicalDamage = 0;

		psBuilding->status = SS_BEING_BUILT;
		psBuilding->currentBuildPts = 0;

		alignStructure(psBuilding);

		/* Store the weapons */
		psBuilding->numWeaps = 0;
		if (pStructureType->numWeaps > 0)
		{
			UDWORD weapon;

			for (weapon = 0; weapon < pStructureType->numWeaps; weapon++)
			{
				if (pStructureType->psWeapStat[weapon])
				{
					psBuilding->asWeaps[weapon].lastFired = 0;
					psBuilding->asWeaps[weapon].shotsFired = 0;
					//in multiPlayer make the Las-Sats require re-loading from the start
					if (bMultiPlayer && pStructureType->psWeapStat[0]->weaponSubClass == WSC_LAS_SAT)
					{
						psBuilding->asWeaps[0].lastFired = gameTime;
					}
					psBuilding->asWeaps[weapon].nStat =	pStructureType->psWeapStat[weapon] - asWeaponStats;
					psBuilding->asWeaps[weapon].ammo = (asWeaponStats + psBuilding->asWeaps[weapon].nStat)->upgrade[psBuilding->player].numRounds;
					psBuilding->numWeaps++;
				}
			}
		}
		else
		{
			if (pStructureType->psWeapStat[0])
			{
				psBuilding->asWeaps[0].lastFired = 0;
				psBuilding->asWeaps[0].shotsFired = 0;
				//in multiPlayer make the Las-Sats require re-loading from the start
				if (bMultiPlayer && pStructureType->psWeapStat[0]->weaponSubClass == WSC_LAS_SAT)
				{
					psBuilding->asWeaps[0].lastFired = gameTime;
				}
				psBuilding->asWeaps[0].nStat =	pStructureType->psWeapStat[0] - asWeaponStats;
				psBuilding->asWeaps[0].ammo = (asWeaponStats + psBuilding->asWeaps[0].nStat)->upgrade[psBuilding->player].numRounds;
			}
		}

		psBuilding->resistance = (UWORD)structureResistance(pStructureType, (UBYTE)player);
		psBuilding->lastResistance = ACTION_START_TIME;

		// Do the visibility stuff before setFunctionality - so placement of DP's can work
		memset(psBuilding->seenThisTick, 0, sizeof(psBuilding->seenThisTick));

		// Structure is visible to anyone with shared vision.
		for (unsigned vPlayer = 0; vPlayer < MAX_PLAYERS; ++vPlayer)
		{
			psBuilding->visible[vPlayer] = hasSharedVision(vPlayer, player) ? UINT8_MAX : 0;
		}

		// Reveal any tiles that can be seen by the structure
		visTilesUpdate(psBuilding);

		/*if we're coming from a SAVEGAME and we're on an Expand_Limbo mission,
		any factories that were built previously for the selectedPlayer will
		have DP's in an invalid location - the scroll limits will have been
		changed to not include them. This is the only HACK I can think of to
		enable them to be loaded up. So here goes...*/
		if (FromSave && player == selectedPlayer && missionLimboExpand())
		{
			//save the current values
			preScrollMinX = scrollMinX;
			preScrollMinY = scrollMinY;
			preScrollMaxX = scrollMaxX;
			preScrollMaxY = scrollMaxY;
			//set the current values to mapWidth/mapHeight
			scrollMinX = 0;
			scrollMinY = 0;
			scrollMaxX = mapWidth;
			scrollMaxY = mapHeight;
			// NOTE: resizeRadar() may be required here, since we change scroll limits?
		}
		//set the functionality dependent on the type of structure
		if (!setFunctionality(psBuilding, pStructureType->type))
		{
			removeStructFromMap(psBuilding);
			delete psBuilding;
			//better reset these if you couldn't build the structure!
			if (FromSave && player == selectedPlayer && missionLimboExpand())
			{
				//reset the current values
				scrollMinX = preScrollMinX;
				scrollMinY = preScrollMinY;
				scrollMaxX = preScrollMaxX;
				scrollMaxY = preScrollMaxY;
				// NOTE: resizeRadar() may be required here, since we change scroll limits?
			}
			return nullptr;
		}

		//reset the scroll values if adjusted
		if (FromSave && player == selectedPlayer && missionLimboExpand())
		{
			//reset the current values
			scrollMinX = preScrollMinX;
			scrollMinY = preScrollMinY;
			scrollMaxX = preScrollMaxX;
			scrollMaxY = preScrollMaxY;
			// NOTE: resizeRadar() may be required here, since we change scroll limits?
		}

		// rotate a wall if necessary
		if (!FromSave && (pStructureType->type == REF_WALL || pStructureType->type == REF_GATE))
		{
			psBuilding->pFunctionality->wall.type = wallType(wallOrientation);
			if (wallOrientation != WallConnectNone)
			{
				psBuilding->rot.direction = wallDir(wallOrientation);
				psBuilding->sDisplay.imd = psBuilding->pStructureType->pIMD[std::min<unsigned>(psBuilding->pFunctionality->wall.type, psBuilding->pStructureType->pIMD.size() - 1)];
			}
		}

		psBuilding->body = (UWORD)structureBody(psBuilding);
		psBuilding->expectedDamage = 0;  // Begin life optimistically.

		//add the structure to the list - this enables it to be drawn whilst being built
		addStructure(psBuilding);

		asStructureStats[max].curCount[player]++;

		if (isLasSat(psBuilding->pStructureType))
		{
			psBuilding->asWeaps[0].ammo = 1; // ready to trigger the fire button
		}

		// Move any delivery points under the new structure.
		StructureBounds bounds = getStructureBounds(psBuilding);
		for (unsigned player = 0; player < MAX_PLAYERS; ++player)
		{
			for (STRUCTURE *psStruct = apsStructLists[player]; psStruct != nullptr; psStruct = psStruct->psNext)
			{
				FLAG_POSITION *fp = nullptr;
				if (StructIsFactory(psStruct))
				{
					fp = psStruct->pFunctionality->factory.psAssemblyPoint;
				}
				else if (psStruct->pStructureType->type == REF_REPAIR_FACILITY)
				{
					fp = psStruct->pFunctionality->repairFacility.psDeliveryPoint;
				}
				if (fp != nullptr)
				{
					Vector2i pos = map_coord(fp->coords.xy);
					if (unsigned(pos.x - bounds.map.x) < unsigned(bounds.size.x) && unsigned(pos.y - bounds.map.y) < unsigned(bounds.size.y))
					{
						// Delivery point fp is under the new structure. Need to move it.
						setAssemblyPoint(fp, fp->coords.x, fp->coords.y, player, true);
					}
				}
			}
		}
	}
	else //its an upgrade
	{
		bool		bUpgraded = false;
		int32_t         bodyDiff = 0;

		//don't create the Structure use existing one
		psBuilding = getTileStructure(map_coord(x), map_coord(y));

		if (!psBuilding)
		{
			return nullptr;
		}

		int prevResearchState = intGetResearchState();

		if (pStructureType->type == REF_FACTORY_MODULE)
		{
			if (psBuilding->pStructureType->type != REF_FACTORY &&
			    psBuilding->pStructureType->type != REF_VTOL_FACTORY)
			{
				return nullptr;
			}
			//increment the capacity and output for the owning structure
			if (psBuilding->capacity < SIZE_SUPER_HEAVY)
			{
				//store the % difference in body points before upgrading
				bodyDiff = 65536 - getStructureDamage(psBuilding);

				psBuilding->capacity++;
				bUpgraded = true;
				//put any production on hold
				holdProduction(psBuilding, ModeImmediate);
			}
		}

		if (pStructureType->type == REF_RESEARCH_MODULE)
		{
			if (psBuilding->pStructureType->type != REF_RESEARCH)
			{
				return nullptr;
			}
			//increment the capacity and research points for the owning structure
			if (psBuilding->capacity == 0)
			{
				//store the % difference in body points before upgrading
				bodyDiff = 65536 - getStructureDamage(psBuilding);

				psBuilding->capacity++;
				bUpgraded = true;
				//cancel any research - put on hold now
				if (psBuilding->pFunctionality->researchFacility.psSubject)
				{
					//cancel the topic
					holdResearch(psBuilding, ModeImmediate);
				}
			}
		}

		if (pStructureType->type == REF_POWER_MODULE)
		{
			if (psBuilding->pStructureType->type != REF_POWER_GEN)
			{
				return nullptr;
			}
			//increment the capacity and research points for the owning structure
			if (psBuilding->capacity == 0)
			{
				//store the % difference in body points before upgrading
				bodyDiff = 65536 - getStructureDamage(psBuilding);

				//increment the power output, multiplier and capacity
				psBuilding->capacity++;
				bUpgraded = true;

				//need to inform any res Extr associated that not digging until complete
				releasePowerGen(psBuilding);
			}
		}
		if (bUpgraded)
		{
			std::vector<iIMDShape *> &IMDs = psBuilding->pStructureType->pIMD;
			int imdIndex = std::min<int>(psBuilding->capacity * 2, IMDs.size() - 1) - 1;  // *2-1 because even-numbered IMDs are structures, odd-numbered IMDs are just the modules, and we want just the module since we cache the fully-built part of the building in psBuilding->prebuiltImd.
			psBuilding->prebuiltImd = psBuilding->sDisplay.imd;
			psBuilding->sDisplay.imd = IMDs[imdIndex];

			//calculate the new body points of the owning structure
			psBuilding->body = (uint64_t)structureBody(psBuilding) * bodyDiff / 65536;

			//initialise the build points
			psBuilding->currentBuildPts = 0;
			//start building again
			psBuilding->status = SS_BEING_BUILT;
			psBuilding->buildRate = 1;  // Don't abandon the structure first tick, so set to nonzero.
			if (psBuilding->player == selectedPlayer && !FromSave)
			{
				intRefreshScreen();
			}
		}
		intNotifyResearchButton(prevResearchState);
	}
	if (pStructureType->type != REF_WALL && pStructureType->type != REF_WALLCORNER)
	{
		if (player == selectedPlayer)
		{
			scoreUpdateVar(WD_STR_BUILT);
		}
	}

	/* why is this necessary - it makes tiles under the structure visible */
	setUnderTilesVis(psBuilding, player);

	psBuilding->prevTime = gameTime - deltaGameTime;  // Structure hasn't been updated this tick, yet.
	psBuilding->time = psBuilding->prevTime - 1;      // -1, so the times are different, even before updating.

	return psBuilding;
}

STRUCTURE *buildBlueprint(STRUCTURE_STATS const *psStats, Vector2i xy, uint16_t direction, unsigned moduleIndex, STRUCT_STATES state)
{
	STRUCTURE *blueprint;

	ASSERT_OR_RETURN(nullptr, psStats != nullptr, "No blueprint stats");
	ASSERT_OR_RETURN(nullptr, psStats->pIMD[0] != nullptr, "No blueprint model for %s", getName(psStats));

	Vector3i pos(xy, INT32_MIN);
	Rotation rot((direction + 0x2000) & 0xC000, 0, 0); // Round direction to nearest 90°.

	StructureBounds b = getStructureBounds(psStats, xy, direction);
	for (int j = 0; j <= b.size.y; ++j)
		for (int i = 0; i <= b.size.x; ++i)
		{
			pos.z = std::max(pos.z, map_TileHeight(b.map.x + i, b.map.y + j));
		}

	int moduleNumber = 0;
	std::vector<iIMDShape *> const *pIMD = &psStats->pIMD;
	if (IsStatExpansionModule(psStats))
	{
		STRUCTURE *baseStruct = castStructure(worldTile(xy)->psObject);
		if (baseStruct != nullptr)
		{
			if (moduleIndex == 0)
			{
				moduleIndex = nextModuleToBuild(baseStruct, 0);
			}
			int baseModuleNumber = moduleIndex * 2 - 1; // *2-1 because even-numbered IMDs are structures, odd-numbered IMDs are just the modules.
			std::vector<iIMDShape *> const *basepIMD = &baseStruct->pStructureType->pIMD;
			if ((unsigned)baseModuleNumber < basepIMD->size())
			{
				// Draw the module.
				moduleNumber = baseModuleNumber;
				pIMD = basepIMD;
				pos = baseStruct->pos;
				rot = baseStruct->rot;
			}
		}
	}

	blueprint = new STRUCTURE(0, selectedPlayer);
	// construct the fake structure
	blueprint->pStructureType = const_cast<STRUCTURE_STATS *>(psStats);  // Couldn't be bothered to fix const correctness everywhere.
	blueprint->visible[selectedPlayer] = UBYTE_MAX;
	blueprint->sDisplay.imd = (*pIMD)[std::min<int>(moduleNumber, pIMD->size() - 1)];
	blueprint->pos = pos;
	blueprint->rot = rot;
	blueprint->selected = false;

	blueprint->numWeaps = 0;
	blueprint->asWeaps[0].nStat = 0;

	// give defensive structures a weapon
	if (psStats->psWeapStat[0])
	{
		blueprint->asWeaps[0].nStat = psStats->psWeapStat[0] - asWeaponStats;
	}
	// things with sensors or ecm (or repair facilities) need these set, even if they have no official weapon
	blueprint->numWeaps = 0;
	blueprint->asWeaps[0].lastFired = 0;
	blueprint->asWeaps[0].rot.pitch = 0;
	blueprint->asWeaps[0].rot.direction = 0;
	blueprint->asWeaps[0].rot.roll = 0;
	blueprint->asWeaps[0].prevRot = blueprint->asWeaps[0].rot;

	blueprint->expectedDamage = 0;

	// Times must be different, but don't otherwise matter.
	blueprint->time = 23;
	blueprint->prevTime = 42;

	blueprint->status = state;

	// Rotate wall if needed.
	if (blueprint->pStructureType->type == REF_WALL || blueprint->pStructureType->type == REF_GATE)
	{
		WallOrientation scanType = structChooseWallTypeBlueprint(map_coord(blueprint->pos.xy));
		unsigned type = wallType(scanType);
		if (scanType != WallConnectNone)
		{
			blueprint->rot.direction = wallDir(scanType);
			blueprint->sDisplay.imd = blueprint->pStructureType->pIMD[std::min<unsigned>(type, blueprint->pStructureType->pIMD.size() - 1)];
		}
	}

	return blueprint;
}

static bool setFunctionality(STRUCTURE	*psBuilding, STRUCTURE_TYPE functionType)
{
	ASSERT_OR_RETURN(false, psBuilding != nullptr, "Invalid pointer");
	CHECK_STRUCTURE(psBuilding);

	switch (functionType)
	{
	case REF_FACTORY:
	case REF_CYBORG_FACTORY:
	case REF_VTOL_FACTORY:
	case REF_RESEARCH:
	case REF_POWER_GEN:
	case REF_RESOURCE_EXTRACTOR:
	case REF_REPAIR_FACILITY:
	case REF_REARM_PAD:
	case REF_WALL:
	case REF_GATE:
		// Allocate space for the buildings functionality
		psBuilding->pFunctionality = (FUNCTIONALITY *)calloc(1, sizeof(*psBuilding->pFunctionality));
		ASSERT_OR_RETURN(false, psBuilding != nullptr, "Out of memory");
		break;

	default:
		psBuilding->pFunctionality = nullptr;
		break;
	}

	switch (functionType)
	{
	case REF_FACTORY:
	case REF_CYBORG_FACTORY:
	case REF_VTOL_FACTORY:
		{
			FACTORY *psFactory = &psBuilding->pFunctionality->factory;
			unsigned int x, y;

			psFactory->psSubject = nullptr;

			// Default the secondary order - AB 22/04/99
			psFactory->secondaryOrder = DSS_REPLEV_NEVER | DSS_ALEV_ALWAYS;

			// Create the assembly point for the factory
			if (!createFlagPosition(&psFactory->psAssemblyPoint, psBuilding->player))
			{
				return false;
			}

			// initialise the assembly point position
			const Vector2i size = psBuilding->size();
			x = map_coord(psBuilding->pos.x + (size.x + 1) * TILE_UNITS / 2);
			y = map_coord(psBuilding->pos.y + (size.y + 1) * TILE_UNITS / 2);

			// Set the assembly point
			setAssemblyPoint(psFactory->psAssemblyPoint, world_coord(x), world_coord(y), psBuilding->player, true);

			// Add the flag to the list
			addFlagPosition(psFactory->psAssemblyPoint);
			switch (functionType)
			{
			case REF_FACTORY:
				setFlagPositionInc(psBuilding->pFunctionality, psBuilding->player, FACTORY_FLAG);
				break;
			case REF_CYBORG_FACTORY:
				setFlagPositionInc(psBuilding->pFunctionality, psBuilding->player, CYBORG_FLAG);
				break;
			case REF_VTOL_FACTORY:
				setFlagPositionInc(psBuilding->pFunctionality, psBuilding->player, VTOL_FLAG);
				break;
			default:
				ASSERT_OR_RETURN(false, false, "Invalid factory type");
			}
			break;
		}
	case REF_POWER_GEN:
	case REF_HQ:
	case REF_REARM_PAD:
		{
			break;
		}
	case REF_RESOURCE_EXTRACTOR:
		{
			RES_EXTRACTOR *psResExtracter = &psBuilding->pFunctionality->resourceExtractor;

			// Make the structure inactive
			psResExtracter->psPowerGen = nullptr;
			break;
		}
	case REF_REPAIR_FACILITY:
		{
			REPAIR_FACILITY *psRepairFac = &psBuilding->pFunctionality->repairFacility;
			unsigned int x, y;

			psRepairFac->psObj = nullptr;
			psRepairFac->droidQueue = 0;
			psRepairFac->psGroup = grpCreate();

			// Add NULL droid to the group
			psRepairFac->psGroup->add(nullptr);

			// Create an assembly point for repaired droids
			if (!createFlagPosition(&psRepairFac->psDeliveryPoint, psBuilding->player))
			{
				return false;
			}

			// Initialise the assembly point
			const Vector2i size = psBuilding->size();
			x = map_coord(psBuilding->pos.x + (size.x + 1) * TILE_UNITS / 2);
			y = map_coord(psBuilding->pos.y + (size.y + 1) * TILE_UNITS / 2);

			// Set the assembly point
			setAssemblyPoint(psRepairFac->psDeliveryPoint, world_coord(x),
			                 world_coord(y), psBuilding->player, true);

			// Add the flag (triangular marker on the ground) at the delivery point
			addFlagPosition(psRepairFac->psDeliveryPoint);
			setFlagPositionInc(psBuilding->pFunctionality, psBuilding->player, REPAIR_FLAG);
			break;
		}
	// Structure types without a FUNCTIONALITY
	default:
		break;
	}

	return true;
}


// Set the command droid that factory production should go to
void assignFactoryCommandDroid(STRUCTURE *psStruct, DROID *psCommander)
{
	FACTORY			*psFact;
	FLAG_POSITION	*psFlag, *psNext, *psPrev;
	SDWORD			factoryInc, typeFlag;

	CHECK_STRUCTURE(psStruct);
	ASSERT_OR_RETURN(, StructIsFactory(psStruct), "structure not a factory");

	psFact = &psStruct->pFunctionality->factory;

	switch (psStruct->pStructureType->type)
	{
	case REF_FACTORY:
		typeFlag = FACTORY_FLAG;
		break;
	case REF_VTOL_FACTORY:
		typeFlag = VTOL_FLAG;
		break;
	case REF_CYBORG_FACTORY:
		typeFlag = CYBORG_FLAG;
		break;
	default:
		ASSERT(!"unknown factory type", "unknown factory type");
		typeFlag = FACTORY_FLAG;
		break;
	}

	// removing a commander from a factory
	if (psFact->psCommander != nullptr)
	{
		if (typeFlag == FACTORY_FLAG)
		{
			secondarySetState(psFact->psCommander, DSO_CLEAR_PRODUCTION,
			                  (SECONDARY_STATE)(1 << (psFact->psAssemblyPoint->factoryInc + DSS_ASSPROD_SHIFT)));
		}
		else if (typeFlag == CYBORG_FLAG)
		{
			secondarySetState(psFact->psCommander, DSO_CLEAR_PRODUCTION,
			                  (SECONDARY_STATE)(1 << (psFact->psAssemblyPoint->factoryInc + DSS_ASSPROD_CYBORG_SHIFT)));
		}
		else
		{
			secondarySetState(psFact->psCommander, DSO_CLEAR_PRODUCTION,
			                  (SECONDARY_STATE)(1 << (psFact->psAssemblyPoint->factoryInc + DSS_ASSPROD_VTOL_SHIFT)));
		}

		psFact->psCommander = nullptr;
		// TODO: Synchronise .psCommander.
		//syncDebug("Removed commander from factory %d", psStruct->id);
		if (!missionIsOffworld())
		{
			addFlagPosition(psFact->psAssemblyPoint);	// add the assembly point back into the list
		}
		else
		{
			psFact->psAssemblyPoint->psNext = mission.apsFlagPosLists[psFact->psAssemblyPoint->player];
			mission.apsFlagPosLists[psFact->psAssemblyPoint->player] = psFact->psAssemblyPoint;
		}
	}

	if (psCommander != nullptr)
	{
		ASSERT_OR_RETURN(, !missionIsOffworld(), "cannot assign a commander to a factory when off world");

		factoryInc = psFact->psAssemblyPoint->factoryInc;
		psPrev = nullptr;

		for (psFlag = apsFlagPosLists[psStruct->player]; psFlag; psFlag = psNext)
		{
			psNext = psFlag->psNext;

			if ((psFlag->factoryInc == factoryInc) && (psFlag->factoryType == typeFlag))
			{
				if (psFlag != psFact->psAssemblyPoint)
				{
					removeFlagPosition(psFlag);
				}
				else
				{
					// need to keep the assembly point(s) for the factory
					// but remove it(the primary) from the list so it doesn't get
					// displayed
					if (psPrev == nullptr)
					{
						apsFlagPosLists[psStruct->player] = psFlag->psNext;
					}
					else
					{
						psPrev->psNext = psFlag->psNext;
					}
					psFlag->psNext = nullptr;
				}
			}
			else
			{
				psPrev = psFlag;
			}
		}
		psFact->psCommander = psCommander;
		syncDebug("Assigned commander %d to factory %d", psCommander->id, psStruct->id);
	}
}


// remove all factories from a command droid
void clearCommandDroidFactory(DROID *psDroid)
{
	STRUCTURE	*psCurr;

	for (psCurr = apsStructLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
	{
		if ((psCurr->pStructureType->type == REF_FACTORY) ||
		    (psCurr->pStructureType->type == REF_CYBORG_FACTORY) ||
		    (psCurr->pStructureType->type == REF_VTOL_FACTORY))
		{
			if (psCurr->pFunctionality->factory.psCommander == psDroid)
			{
				assignFactoryCommandDroid(psCurr, nullptr);
			}
		}
	}
	for (psCurr = mission.apsStructLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
	{
		if ((psCurr->pStructureType->type == REF_FACTORY) ||
		    (psCurr->pStructureType->type == REF_CYBORG_FACTORY) ||
		    (psCurr->pStructureType->type == REF_VTOL_FACTORY))
		{
			if (psCurr->pFunctionality->factory.psCommander == psDroid)
			{
				assignFactoryCommandDroid(psCurr, nullptr);
			}
		}
	}
}

/* Check that a tile is vacant for a droid to be placed */
static bool structClearTile(UWORD x, UWORD y)
{
	UDWORD	player;
	DROID	*psCurr;

	/* Check for a structure */
	if (fpathBlockingTile(x, y, PROPULSION_TYPE_WHEELED))
	{
		debug(LOG_NEVER, "failed - blocked");
		return false;
	}

	/* Check for a droid */
	for (player = 0; player < MAX_PLAYERS; player++)
	{
		for (psCurr = apsDroidLists[player]; psCurr; psCurr = psCurr->psNext)
		{
			if (map_coord(psCurr->pos.x) == x
			    && map_coord(psCurr->pos.y) == y)
			{
				debug(LOG_NEVER, "failed - not vacant");
				return false;
			}
		}
	}

	debug(LOG_NEVER, "succeeded");
	return true;
}

/* An auxiliary function for std::stable_sort in placeDroid */
static bool comparePlacementPoints(Vector2i a, Vector2i b)
{
	return abs(a.x) + abs(a.y) < abs(b.x) + abs(b.y);
}

/* Find a location near to a structure to start the droid of */
bool placeDroid(STRUCTURE *psStructure, UDWORD *droidX, UDWORD *droidY)
{

	CHECK_STRUCTURE(psStructure);

	/* Find the four corners of the square */
	StructureBounds bounds = getStructureBounds(psStructure);
	int xmin = bounds.map.x - 1;
	int xmax = bounds.map.x + bounds.size.x;
	int ymin = bounds.map.y - 1;
	int ymax = bounds.map.y + bounds.size.y;
	if (xmin < 0)
	{
		xmin = 0;
	}
	if (xmax > (SDWORD)mapWidth)
	{
		xmax = (SWORD)mapWidth;
	}
	if (ymin < 0)
	{
		ymin = 0;
	}
	if (ymax > (SDWORD)mapHeight)
	{
		ymax = (SWORD)mapHeight;
	}

	/* Round direction to nearest 90°. */
	uint16_t direction = (psStructure->rot.direction + 0x2000) & 0xC000;

	/* We sort all adjacent tiles by their Manhattan distance to the
	target droid exit tile, misplaced by (1/3, 1/4) tiles.
	Since only whole coordinates are sorted, this makes sure sorting
	is deterministic. Target coordinates, multiplied by 12 to eliminate
	floats, are stored in (sx, sy). */
	int sx, sy;

	if (direction == 0x0)
	{
		sx = 12 * (xmin + 1) + 4;
		sy = 12 * ymax + 3;
	}
	else if (direction == 0x4000)
	{
		sx = 12 * xmax + 3;
		sy = 12 * (ymax - 1) - 4;
	}
	else if (direction == 0x8000)
	{
		sx = 12 * (xmax - 1) - 4;
		sy = 12 * ymin - 3;
	}
	else
	{
		sx = 12 * xmin - 3;
		sy = 12 * (ymin + 1) + 4;
	}

	std::vector<Vector2i> tiles;
	for (int x = xmin; x <= xmax; ++x)
	{
		for (int y = ymin; y <= ymax; ++y)
		{
			if (structClearTile(x, y))
			{
				tiles.push_back(Vector2i(12 * x - sx, 12 * y - sy));
			}
		}
	}

	if (tiles.empty())
	{
		return false;
	}

	std::sort(tiles.begin(), tiles.end(), comparePlacementPoints);

	/* Store best tile coordinates in (sx, sy),
	which are also map coordinates of its north-west corner.
	Store world coordinates of this tile's center in (wx, wy) */
	sx = (tiles[0].x + sx) / 12;
	sy = (tiles[0].y + sy) / 12;
	int wx = world_coord(sx) + TILE_UNITS / 2;
	int wy = world_coord(sy) + TILE_UNITS / 2;

	/* Finally, find world coordinates of the structure point closest to (mx, my).
	For simplicity, round to grid vertices. */
	if (2 * sx <= xmin + xmax)
	{
		wx += TILE_UNITS / 2;
	}
	if (2 * sx >= xmin + xmax)
	{
		wx -= TILE_UNITS / 2;
	}
	if (2 * sy <= ymin + ymax)
	{
		wy += TILE_UNITS / 2;
	}
	if (2 * sy >= ymin + ymax)
	{
		wy -= TILE_UNITS / 2;
	}

	*droidX = wx;
	*droidY = wy;
	return true;
}

/* Place a newly manufactured droid next to a factory  and then send if off
to the assembly point, returns true if droid was placed successfully */
static bool structPlaceDroid(STRUCTURE *psStructure, DROID_TEMPLATE *psTempl, DROID **ppsDroid)
{
	UDWORD			x, y;
	bool			placed;//bTemp = false;
	DROID			*psNewDroid;
	FACTORY			*psFact;
	FLAG_POSITION	*psFlag;
	Vector3i iVecEffect;
	UBYTE			factoryType;
	bool			assignCommander;

	CHECK_STRUCTURE(psStructure);

	placed = placeDroid(psStructure, &x, &y);

	if (placed)
	{
		INITIAL_DROID_ORDERS initialOrders = {psStructure->pFunctionality->factory.secondaryOrder, psStructure->pFunctionality->factory.psAssemblyPoint->coords.x, psStructure->pFunctionality->factory.psAssemblyPoint->coords.y, psStructure->id};
		//create a droid near to the structure
		syncDebug("Placing new droid at (%d,%d)", x, y);
		turnOffMultiMsg(true);
		psNewDroid = buildDroid(psTempl, x, y, psStructure->player, false, &initialOrders);
		turnOffMultiMsg(false);
		if (!psNewDroid)
		{
			*ppsDroid = nullptr;
			return false;
		}

		if (myResponsibility(psStructure->player))
		{
			uint32_t newState = psStructure->pFunctionality->factory.secondaryOrder;
			uint32_t diff = newState ^ psNewDroid->secondaryOrder;
			if ((diff & DSS_REPLEV_MASK) != 0)
			{
				secondarySetState(psNewDroid, DSO_REPAIR_LEVEL, (SECONDARY_STATE)(newState & DSS_REPLEV_MASK));
			}
			if ((diff & DSS_ALEV_MASK) != 0)
			{
				secondarySetState(psNewDroid, DSO_ATTACK_LEVEL, (SECONDARY_STATE)(newState & DSS_ALEV_MASK));
			}
			if ((diff & DSS_CIRCLE_MASK) != 0)
			{
				secondarySetState(psNewDroid, DSO_CIRCLE, (SECONDARY_STATE)(newState & DSS_CIRCLE_MASK));
			}
		}

		if (psStructure->visible[selectedPlayer])
		{
			/* add smoke effect to cover the droid's emergence from the factory */
			iVecEffect.x = psNewDroid->pos.x;
			iVecEffect.y = map_Height(psNewDroid->pos.x, psNewDroid->pos.y) + DROID_CONSTRUCTION_SMOKE_HEIGHT;
			iVecEffect.z = psNewDroid->pos.y;
			addEffect(&iVecEffect, EFFECT_CONSTRUCTION, CONSTRUCTION_TYPE_DRIFTING, false, nullptr, 0, gameTime - deltaGameTime + 1);
			iVecEffect.x = psNewDroid->pos.x - DROID_CONSTRUCTION_SMOKE_OFFSET;
			iVecEffect.z = psNewDroid->pos.y - DROID_CONSTRUCTION_SMOKE_OFFSET;
			addEffect(&iVecEffect, EFFECT_CONSTRUCTION, CONSTRUCTION_TYPE_DRIFTING, false, nullptr, 0, gameTime - deltaGameTime + 1);
			iVecEffect.z = psNewDroid->pos.y + DROID_CONSTRUCTION_SMOKE_OFFSET;
			addEffect(&iVecEffect, EFFECT_CONSTRUCTION, CONSTRUCTION_TYPE_DRIFTING, false, nullptr, 0, gameTime - deltaGameTime + 1);
			iVecEffect.x = psNewDroid->pos.x + DROID_CONSTRUCTION_SMOKE_OFFSET;
			addEffect(&iVecEffect, EFFECT_CONSTRUCTION, CONSTRUCTION_TYPE_DRIFTING, false, nullptr, 0, gameTime - deltaGameTime + 1);
			iVecEffect.z = psNewDroid->pos.y - DROID_CONSTRUCTION_SMOKE_OFFSET;
			addEffect(&iVecEffect, EFFECT_CONSTRUCTION, CONSTRUCTION_TYPE_DRIFTING, false, nullptr, 0, gameTime - deltaGameTime + 1);
		}
		/* add the droid to the list */
		addDroid(psNewDroid, apsDroidLists);
		*ppsDroid = psNewDroid;
		if (psNewDroid->player == selectedPlayer)
		{
			audio_QueueTrack(ID_SOUND_DROID_COMPLETED);
			intRefreshScreen();	// update any interface implications.
		}

		// update the droid counts
		incNumDroids(psNewDroid->player);
		switch (psNewDroid->droidType)
		{
		case DROID_COMMAND:
			incNumCommandDroids(psNewDroid->player);
			break;
		case DROID_CONSTRUCT:
		case DROID_CYBORG_CONSTRUCT:
			incNumConstructorDroids(psNewDroid->player);
			break;
		default:
			break;
		}

		psFact = &psStructure->pFunctionality->factory;

		// if we've built a command droid - make sure that it isn't assigned to another commander
		assignCommander = false;
		if ((psNewDroid->droidType == DROID_COMMAND) &&
		    (psFact->psCommander != nullptr))
		{
			assignFactoryCommandDroid(psStructure, nullptr);
			assignCommander = true;
		}

		if (isVtolDroid(psNewDroid) && !isTransporter(psNewDroid))
		{
			moveToRearm(psNewDroid);
		}
		if (psFact->psCommander != nullptr && myResponsibility(psStructure->player))
		{
			// TODO: Should synchronise .psCommander in all cases.
			//syncDebug("Has commander.");
			if (isTransporter(psNewDroid))
			{
				// Transporters can't be assigned to commanders, due to abuse of .psGroup. Try to land on the commander instead. Hopefully the transport is heavy enough to crush the commander.
				orderDroidLoc(psNewDroid, DORDER_MOVE, psFact->psCommander->pos.x, psFact->psCommander->pos.y, ModeQueue);
			}
			else if (idfDroid(psNewDroid) ||
			         isVtolDroid(psNewDroid))
			{
				orderDroidObj(psNewDroid, DORDER_FIRESUPPORT, psFact->psCommander, ModeQueue);
				//moveToRearm(psNewDroid);
			}
			else
			{
				orderDroidObj(psNewDroid, DORDER_COMMANDERSUPPORT, psFact->psCommander, ModeQueue);
			}
		}
		else
		{
			//check flag against factory type
			factoryType = FACTORY_FLAG;
			if (psStructure->pStructureType->type == REF_CYBORG_FACTORY)
			{
				factoryType = CYBORG_FLAG;
			}
			else if (psStructure->pStructureType->type == REF_VTOL_FACTORY)
			{
				factoryType = VTOL_FLAG;
			}
			//find flag in question.
			for (psFlag = apsFlagPosLists[psFact->psAssemblyPoint->player];
			     psFlag
			     && !(psFlag->factoryInc == psFact->psAssemblyPoint->factoryInc // correct fact.
			          && psFlag->factoryType == factoryType); // correct type
			     psFlag = psFlag->psNext) {}
			ASSERT(psFlag, "No flag found for %s at (%d, %d)", objInfo(psStructure), psStructure->pos.x, psStructure->pos.y);
			//if vtol droid - send it to ReArm Pad if one exists
			if (psFlag && isVtolDroid(psNewDroid))
			{
				Vector2i pos = psFlag->coords.xy;
				//find a suitable location near the delivery point
				actionVTOLLandingPos(psNewDroid, &pos);
				orderDroidLoc(psNewDroid, DORDER_MOVE, pos.x, pos.y, ModeQueue);
			}
			else if (psFlag)
			{
				orderDroidLoc(psNewDroid, DORDER_MOVE, psFlag->coords.x, psFlag->coords.y, ModeQueue);
			}
		}
		if (assignCommander)
		{
			assignFactoryCommandDroid(psStructure, psNewDroid);
		}
		if (psNewDroid->player == selectedPlayer)
		{
			eventFireCallbackTrigger((TRIGGER_TYPE)CALL_DROIDBUILT);
		}
		return true;
	}
	else
	{
		*ppsDroid = nullptr;
	}
	return false;
}


static bool IsFactoryCommanderGroupFull(const FACTORY *psFactory)
{
	if (bMultiPlayer)
	{
		// TODO: Synchronise .psCommander. Have to return false here, to avoid desynch.
		return false;
	}

	unsigned int DroidsInGroup;

	// If we don't have a commander return false (group not full)
	if (psFactory->psCommander == nullptr)
	{
		return false;
	}

	// allow any number of IDF droids
	if (templateIsIDF(psFactory->psSubject) || asPropulsionStats[psFactory->psSubject->asParts[COMP_PROPULSION]].propulsionType == PROPULSION_TYPE_LIFT)
	{
		return false;
	}

	// Get the number of droids in the commanders group
	DroidsInGroup = psFactory->psCommander->psGroup ? psFactory->psCommander->psGroup->getNumMembers() : 0;

	// if the number in group is less than the maximum allowed then return false (group not full)
	if (DroidsInGroup < cmdDroidMaxGroup(psFactory->psCommander))
	{
		return false;
	}

	// the number in group has reached the maximum
	return true;
}


// Disallow manufacture of units once these limits are reached,
// doesn't mean that these numbers can't be exceeded if units are
// put down in the editor or by the scripts.

void setMaxDroids(int player, int value)
{
	droidLimit[player] = value;
}

void setMaxCommanders(int player, int value)
{
	commanderLimit[player] = value;
}

void setMaxConstructors(int player, int value)
{
	constructorLimit[player] = value;
}

int getMaxDroids(int player)
{
	return droidLimit[player];
}

int getMaxCommanders(int player)
{
	return commanderLimit[player];
}

int getMaxConstructors(int player)
{
	return constructorLimit[player];
}

bool IsPlayerDroidLimitReached(int player)
{
	unsigned int numDroids = getNumDroids(player) + getNumMissionDroids(player) + getNumTransporterDroids(player);
	return numDroids >= getMaxDroids(player);
}

// Check for max number of units reached and halt production.
//
bool CheckHaltOnMaxUnitsReached(STRUCTURE *psStructure)
{
	CHECK_STRUCTURE(psStructure);

	char limitMsg[300];
	bool isLimit = false;

	DROID_TEMPLATE *templ = psStructure->pFunctionality->factory.psSubject;

	// if the players that owns the factory has reached his (or hers) droid limit
	// then put production on hold & return - we need a message to be displayed here !!!!!!!
	if (IsPlayerDroidLimitReached(psStructure->player))
	{
		isLimit = true;
		sstrcpy(limitMsg, _("Can't build anymore units, Unit Limit Reached — Production Halted"));
	}
	else switch (droidTemplateType(templ))
		{
		case DROID_COMMAND:
			if (getNumCommandDroids(psStructure->player) >= getMaxCommanders(psStructure->player))
			{
				isLimit = true;
				ssprintf(limitMsg, _("Can't build anymore \"%s\", Command Control Limit Reached — Production Halted"), templ->name.toUtf8().constData());
			}
			break;
		case DROID_CONSTRUCT:
		case DROID_CYBORG_CONSTRUCT:
			if (getNumConstructorDroids(psStructure->player) >= getMaxConstructors(psStructure->player))
			{
				isLimit = true;
				ssprintf(limitMsg, _("Can't build anymore \"%s\", Construction Unit Limit Reached — Production Halted"), templ->name.toUtf8().constData());
			}
			break;
		default:
			break;
		}

	if (isLimit && psStructure->player == selectedPlayer && lastMaxUnitMessage + MAX_UNIT_MESSAGE_PAUSE < gameTime)
	{
		addConsoleMessage(limitMsg, DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
		lastMaxUnitMessage = gameTime;
	}

	return isLimit;
}


static void aiUpdateStructure(STRUCTURE *psStructure, bool isMission)
{
	BASE_STATS			*pSubject = nullptr;
	UDWORD				pointsToAdd;//, iPower;
	RESEARCH			*pResearch;
	UDWORD				structureMode = 0;
	DROID				*psDroid;
	BASE_OBJECT			*psChosenObjs[MAX_WEAPONS] = {nullptr};
	BASE_OBJECT			*psChosenObj = nullptr;
	FACTORY				*psFactory;
	REPAIR_FACILITY		*psRepairFac = nullptr;
	RESEARCH_FACILITY	*psResFacility;
	Vector3i iVecEffect;
	bool				bDroidPlaced = false;
	WEAPON_STATS		*psWStats;
	bool				bDirect = false;
	SDWORD				xdiff, ydiff, mindist, currdist;
	UDWORD				i;
	TARGET_ORIGIN tmpOrigin = ORIGIN_UNKNOWN;

	CHECK_STRUCTURE(psStructure);

	if (psStructure->time == gameTime)
	{
		// This isn't supposed to happen, and really shouldn't be possible - if this happens, maybe a structure is being updated twice?
		int count1 = 0, count2 = 0;
		STRUCTURE *s;
		for (s =         apsStructLists[psStructure->player]; s != nullptr; s = s->psNext)
		{
			count1 += s == psStructure;
		}
		for (s = mission.apsStructLists[psStructure->player]; s != nullptr; s = s->psNext)
		{
			count2 += s == psStructure;
		}
		debug(LOG_ERROR, "psStructure->prevTime = %u, psStructure->time = %u, gameTime = %u, count1 = %d, count2 = %d", psStructure->prevTime, psStructure->time, gameTime, count1, count2);
		--psStructure->time;
	}
	psStructure->prevTime = psStructure->time;
	psStructure->time = gameTime;
	for (i = 0; i < MAX(1, psStructure->numWeaps); ++i)
	{
		psStructure->asWeaps[i].prevRot = psStructure->asWeaps[i].rot;
	}

	if (isMission)
	{
		switch (psStructure->pStructureType->type)
		{
		case REF_RESEARCH:
		case REF_FACTORY:
		case REF_CYBORG_FACTORY:
		case REF_VTOL_FACTORY:
			break;
		default:
			return; // nothing to do
		}
	}

	// Will go out into a building EVENT stats/text file
	/* Spin round yer sensors! */
	if (psStructure->numWeaps == 0)
	{
		if ((psStructure->asWeaps[0].nStat == 0) &&
		    (psStructure->pStructureType->type != REF_REPAIR_FACILITY))
		{

			//////
			// - radar should rotate every three seconds ... 'cause we timed it at Heathrow !
			// gameTime is in milliseconds - one rotation every 3 seconds = 1 rotation event 3000 millisecs
			psStructure->asWeaps[0].rot.direction = (uint16_t)((uint64_t)gameTime * 65536 / 3000);  // Cast wrapping intended.
			psStructure->asWeaps[0].rot.pitch = 0;
		}
	}

	/* Check lassat */
	if (isLasSat(psStructure->pStructureType)
	    && gameTime - psStructure->asWeaps[0].lastFired > weaponFirePause(&asWeaponStats[psStructure->asWeaps[0].nStat], psStructure->player)
	    && psStructure->asWeaps[0].ammo > 0)
	{
		triggerEventStructureReady(psStructure);
		psStructure->asWeaps[0].ammo = 0; // do not fire more than once
	}

	/* See if there is an enemy to attack */
	if (psStructure->numWeaps > 0)
	{
		//structures always update their targets
		for (i = 0; i < psStructure->numWeaps; i++)
		{
			bDirect = proj_Direct(asWeaponStats + psStructure->asWeaps[i].nStat);
			if (psStructure->asWeaps[i].nStat > 0 &&
			    asWeaponStats[psStructure->asWeaps[i].nStat].weaponSubClass != WSC_LAS_SAT)
			{
				if (aiChooseTarget(psStructure, &psChosenObjs[i], i, true, &tmpOrigin))
				{
					objTrace(psStructure->id, "Weapon %d is targeting %d at (%d, %d)", i, psChosenObjs[i]->id,
					         psChosenObjs[i]->pos.x, psChosenObjs[i]->pos.y);
					setStructureTarget(psStructure, psChosenObjs[i], i, tmpOrigin);
				}
				else
				{
					if (aiChooseTarget(psStructure, &psChosenObjs[0], 0, true, &tmpOrigin))
					{
						if (psChosenObjs[0])
						{
							objTrace(psStructure->id, "Weapon %d is supporting main weapon: %d at (%d, %d)", i,
							         psChosenObjs[0]->id, psChosenObjs[0]->pos.x, psChosenObjs[0]->pos.y);
							setStructureTarget(psStructure, psChosenObjs[0], i, tmpOrigin);
							psChosenObjs[i] = psChosenObjs[0];
						}
						else
						{
							setStructureTarget(psStructure, nullptr, i, ORIGIN_UNKNOWN);
							psChosenObjs[i] = nullptr;
						}
					}
					else
					{
						setStructureTarget(psStructure, nullptr, i, ORIGIN_UNKNOWN);
						psChosenObjs[i] = nullptr;
					}
				}

				if (psChosenObjs[i] != nullptr && !aiObjectIsProbablyDoomed(psChosenObjs[i], bDirect))
				{
					// get the weapon stat to see if there is a visible turret to rotate
					psWStats = asWeaponStats + psStructure->asWeaps[i].nStat;

					//if were going to shoot at something move the turret first then fire when locked on
					if (psWStats->pMountGraphic == nullptr)//no turret so lock on whatever
					{
						psStructure->asWeaps[i].rot.direction = calcDirection(psStructure->pos.x, psStructure->pos.y, psChosenObjs[i]->pos.x, psChosenObjs[i]->pos.y);
						combFire(&psStructure->asWeaps[i], psStructure, psChosenObjs[i], i);
					}
					else if (actionTargetTurret(psStructure, psChosenObjs[i], &psStructure->asWeaps[i]))
					{
						combFire(&psStructure->asWeaps[i], psStructure, psChosenObjs[i], i);
					}
				}
				else
				{
					// realign the turret
					if ((psStructure->asWeaps[i].rot.direction % DEG(90)) != 0 || psStructure->asWeaps[i].rot.pitch != 0)
					{
						actionAlignTurret(psStructure, i);
					}
				}
			}
		}
	}

	/* See if there is an enemy to attack for Sensor Towers that have weapon droids attached*/
	else if (psStructure->pStructureType->pSensor)
	{
		if (structStandardSensor(psStructure) || structVTOLSensor(psStructure) || objRadarDetector(psStructure))
		{
			if (aiChooseSensorTarget(psStructure, &psChosenObj))
			{
				objTrace(psStructure->id, "Sensing (%d)", psChosenObj->id);
				if (objRadarDetector(psStructure))
				{
					setStructureTarget(psStructure, psChosenObj, 0, ORIGIN_RADAR_DETECTOR);
				}
				else
				{
					setStructureTarget(psStructure, psChosenObj, 0, ORIGIN_SENSOR);
				}
			}
			else
			{
				setStructureTarget(psStructure, nullptr, 0, ORIGIN_UNKNOWN);
			}
			psChosenObj = psStructure->psTarget[0];
		}
		else
		{
			psChosenObj = psStructure->psTarget[0];
		}
	}
	//only interested if the Structure "does" something!
	if (psStructure->pFunctionality == nullptr)
	{
		return;
	}

	/* Process the functionality according to type
	* determine the subject stats (for research or manufacture)
	* or base object (for repair) or update power levels for resourceExtractor
	*/
	switch (psStructure->pStructureType->type)
	{
	case REF_RESEARCH:
		{
			pSubject = psStructure->pFunctionality->researchFacility.psSubject;
			structureMode = REF_RESEARCH;
			break;
		}
	case REF_FACTORY:
	case REF_CYBORG_FACTORY:
	case REF_VTOL_FACTORY:
		{
			pSubject = psStructure->pFunctionality->factory.psSubject;
			structureMode = REF_FACTORY;
			//check here to see if the factory's commander has died
			if (psStructure->pFunctionality->factory.psCommander &&
			    psStructure->pFunctionality->factory.psCommander->died)
			{
				//remove the commander from the factory
				assignFactoryCommandDroid(psStructure, nullptr);
			}
			break;
		}
	case REF_REPAIR_FACILITY: // FIXME FIXME FIXME: Magic numbers in this section
		{
			psRepairFac = &psStructure->pFunctionality->repairFacility;
			psChosenObj = psRepairFac->psObj;
			structureMode = REF_REPAIR_FACILITY;
			psDroid = (DROID *)psChosenObj;

			// If the droid we're repairing just died, find a new one
			if (psDroid && psDroid->died)
			{
				psDroid = nullptr;
				psChosenObj = nullptr;
				psRepairFac->psObj = nullptr;
			}

			// skip droids that are trying to get to other repair factories
			if (psDroid != nullptr
			    && (!orderState(psDroid, DORDER_RTR)
			        || psDroid->order.psObj != psStructure))
			{
				psDroid = (DROID *)psChosenObj;
				xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psStructure->pos.x;
				ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psStructure->pos.y;
				// unless it has orders to repair here, forget about it when it gets out of range
				if (xdiff * xdiff + ydiff * ydiff > (TILE_UNITS * 5 / 2) * (TILE_UNITS * 5 / 2))
				{
					psChosenObj = nullptr;
					psDroid = nullptr;
					psRepairFac->psObj = nullptr;
				}
			}

			// select next droid if none being repaired,
			// or look for a better droid if not repairing one with repair orders
			if (psChosenObj == nullptr ||
			    (((DROID *)psChosenObj)->order.type != DORDER_RTR && ((DROID *)psChosenObj)->order.type != DORDER_RTR_SPECIFIED))
			{
				//FIX ME: (doesn't look like we need this?)
				ASSERT(psRepairFac->psGroup != nullptr, "invalid repair facility group pointer");

				// Tries to find most important droid to repair
				// Lower dist = more important
				// mindist contains lowest dist found so far
				mindist = (TILE_UNITS * 8) * (TILE_UNITS * 8) * 3;
				if (psChosenObj)
				{
					// We already have a valid droid to repair, no need to look at
					// droids without a repair order.
					mindist = (TILE_UNITS * 8) * (TILE_UNITS * 8) * 2;
				}
				psRepairFac->droidQueue = 0;
				for (psDroid = apsDroidLists[psStructure->player]; psDroid; psDroid = psDroid->psNext)
				{
					BASE_OBJECT *const psTarget = orderStateObj(psDroid, DORDER_RTR);

					// Highest priority:
					// Take any droid with orders to Return to Repair (DORDER_RTR),
					// or that have been ordered to this repair facility (DORDER_RTR_SPECIFIED),
					// or any "lost" unit with one of those two orders.
					if (((psDroid->order.type == DORDER_RTR || (psDroid->order.type == DORDER_RTR_SPECIFIED
					                                            && (!psTarget || psTarget == psStructure)))
					     && psDroid->action != DACTION_WAITFORREPAIR && psDroid->action != DACTION_MOVETOREPAIRPOINT
					     && psDroid->action != DACTION_WAITDURINGREPAIR)
					    || (psTarget && psTarget == psStructure))
					{
						if (psDroid->body >= psDroid->originalBody)
						{
							objTrace(psStructure->id, "Repair not needed of droid %d", (int)psDroid->id);

							/* set droid points to max */
							psDroid->body = psDroid->originalBody;

							// if completely repaired reset order
							secondarySetState(psDroid, DSO_RETURN_TO_LOC, DSS_NONE);

							if (hasCommander(psDroid))
							{
								// return a droid to it's command group
								DROID	*psCommander = psDroid->psGroup->psCommander;

								orderDroidObj(psDroid, DORDER_GUARD, psCommander, ModeImmediate);
							}
							else if (psRepairFac->psDeliveryPoint != nullptr)
							{
								// move the droid out the way
								objTrace(psDroid->id, "Repair not needed - move to delivery point");
								orderDroidLoc(psDroid, DORDER_MOVE,
								              psRepairFac->psDeliveryPoint->coords.x,
								              psRepairFac->psDeliveryPoint->coords.y, ModeQueue);  // ModeQueue because delivery points are not yet synchronised!
							}
							continue;
						}
						xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psStructure->pos.x;
						ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psStructure->pos.y;
						currdist = xdiff * xdiff + ydiff * ydiff;
						if (currdist < mindist && currdist < (TILE_UNITS * 8) * (TILE_UNITS * 8))
						{
							mindist = currdist;
							psChosenObj = psDroid;
						}
						if (psTarget && psTarget == psStructure)
						{
							psRepairFac->droidQueue++;
						}
					}
					// Second highest priority:
					// Help out another nearby repair facility
					else if (psTarget && mindist > (TILE_UNITS * 8) * (TILE_UNITS * 8)
					         && psTarget != psStructure && psDroid->action == DACTION_WAITFORREPAIR)
					{
						int distLimit = mindist;
						if (psTarget->type == OBJ_STRUCTURE && ((STRUCTURE *)psTarget)->pStructureType->type == REF_REPAIR_FACILITY)  // Is a repair facility (not the HQ).
						{
							REPAIR_FACILITY *stealFrom = &((STRUCTURE *)psTarget)->pFunctionality->repairFacility;
							// make a wild guess about what is a good distance
							distLimit = world_coord(stealFrom->droidQueue) * world_coord(stealFrom->droidQueue) * 10;
						}

						xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psStructure->pos.x;
						ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psStructure->pos.y;
						currdist = xdiff * xdiff + ydiff * ydiff + (TILE_UNITS * 8) * (TILE_UNITS * 8); // lower priority
						if (currdist < mindist && currdist - (TILE_UNITS * 8) * (TILE_UNITS * 8) < distLimit)
						{
							mindist = currdist;
							psChosenObj = psDroid;
							psRepairFac->droidQueue++;	// shared queue
							objTrace(psChosenObj->id, "Stolen by another repair facility, currdist=%d, mindist=%d, distLimit=%d", (int)currdist, (int)mindist, distLimit);
						}
					}
					// Lowest priority:
					// Just repair whatever is nearby and needs repairing.
					else if (mindist > (TILE_UNITS * 8) * (TILE_UNITS * 8) * 2 && psDroid->body < psDroid->originalBody)
					{
						xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psStructure->pos.x;
						ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psStructure->pos.y;
						currdist = xdiff * xdiff + ydiff * ydiff + (TILE_UNITS * 8) * (TILE_UNITS * 8) * 2; // even lower priority
						if (currdist < mindist && currdist < (TILE_UNITS * 5 / 2) * (TILE_UNITS * 5 / 2) + (TILE_UNITS * 8) * (TILE_UNITS * 8) * 2)
						{
							mindist = currdist;
							psChosenObj = psDroid;
						}
					}
				}
				if (!psChosenObj) // Nothing to repair? Repair allied units!
				{
					mindist = (TILE_UNITS * 5 / 2) * (TILE_UNITS * 5 / 2);

					for (i = 0; i < MAX_PLAYERS; i++)
					{
						if (aiCheckAlliances(i, psStructure->player) && i != psStructure->player)
						{
							for (psDroid = apsDroidLists[i]; psDroid; psDroid = psDroid->psNext)
							{
								if (psDroid->body < psDroid->originalBody)
								{
									xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psStructure->pos.x;
									ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psStructure->pos.y;
									currdist = xdiff * xdiff + ydiff * ydiff;
									if (currdist < mindist)
									{
										mindist = currdist;
										psChosenObj = psDroid;
									}
								}
							}
						}
					}
				}
				psDroid = (DROID *)psChosenObj;
				if (psDroid)
				{
					if (psDroid->order.type == DORDER_RTR || psDroid->order.type == DORDER_RTR_SPECIFIED)
					{
						// Hey, droid, it's your turn! Stop what you're doing and get ready to get repaired!
						psDroid->action = DACTION_WAITFORREPAIR;
						psDroid->order.psObj = psStructure;
					}
					objTrace(psStructure->id, "Chose to repair droid %d", (int)psDroid->id);
					objTrace(psDroid->id, "Chosen to be repaired by repair structure %d", (int)psStructure->id);
				}
			}

			// send the droid to be repaired
			if (psDroid)
			{
				/* set chosen object */
				psChosenObj = psDroid;

				/* move droid to repair point at rear of facility */
				xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psStructure->pos.x;
				ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psStructure->pos.y;
				if (psDroid->action == DACTION_WAITFORREPAIR ||
				    (psDroid->action == DACTION_WAITDURINGREPAIR
				     && xdiff * xdiff + ydiff * ydiff > (TILE_UNITS * 5 / 2) * (TILE_UNITS * 5 / 2)))
				{
					objTrace(psStructure->id, "Requesting droid %d to come to us", (int)psDroid->id);
					actionDroid(psDroid, DACTION_MOVETOREPAIRPOINT,
					            psStructure, psStructure->pos.x, psStructure->pos.y);
				}
				/* reset repair started if we were previously repairing something else */
				if (psRepairFac->psObj != psDroid)
				{
					psRepairFac->psObj = psDroid;
				}
			}

			// update repair arm position
			if (psChosenObj)
			{
				actionTargetTurret(psStructure, psChosenObj, &psStructure->asWeaps[0]);
			}
			else if ((psStructure->asWeaps[0].rot.direction % DEG(90)) != 0 || psStructure->asWeaps[0].rot.pitch != 0)
			{
				// realign the turret
				actionAlignTurret(psStructure, 0);
			}

			break;
		}
	case REF_REARM_PAD:
		{
			REARM_PAD	*psReArmPad = &psStructure->pFunctionality->rearmPad;

			psChosenObj = psReArmPad->psObj;
			structureMode = REF_REARM_PAD;
			psDroid = nullptr;

			/* select next droid if none being rearmed*/
			if (psChosenObj == nullptr)
			{
				objTrace(psStructure->id, "Rearm pad idle - look for victim");
				for (psDroid = apsDroidLists[psStructure->player]; psDroid; psDroid = psDroid->psNext)
				{
					// move next droid waiting on ground to rearm pad
					if (vtolReadyToRearm(psDroid, psStructure) &&
					    (psChosenObj == nullptr || (((DROID *)psChosenObj)->actionStarted > psDroid->actionStarted)))
					{
						objTrace(psDroid->id, "rearm pad candidate");
						objTrace(psStructure->id, "we found %s to rearm", objInfo(psDroid));
						psChosenObj = psDroid;
					}
				}
				// None available? Try allies.
				for (int i = 0; i < MAX_PLAYERS && !psChosenObj; i++)
				{
					if (aiCheckAlliances(i, psStructure->player) && i != psStructure->player)
					{
						for (psDroid = apsDroidLists[i]; psDroid; psDroid = psDroid->psNext)
						{
							// move next droid waiting on ground to rearm pad
							if (vtolReadyToRearm(psDroid, psStructure))
							{
								psChosenObj = psDroid;
								objTrace(psDroid->id, "allied rearm pad candidate");
								objTrace(psStructure->id, "we found allied %s to rearm", objInfo(psDroid));
								break;
							}
						}
					}
				}
				psDroid = (DROID *)psChosenObj;
				if (psDroid != nullptr)
				{
					actionDroid(psDroid, DACTION_MOVETOREARMPOINT, psStructure);
				}
			}
			else
			{
				psDroid = (DROID *) psChosenObj;
				if ((psDroid->sMove.Status == MOVEINACTIVE ||
				     psDroid->sMove.Status == MOVEHOVER) &&
				    psDroid->action == DACTION_WAITFORREARM)
				{
					objTrace(psDroid->id, "supposed to go to rearm but not on our way -- fixing"); // this should never happen...
					actionDroid(psDroid, DACTION_MOVETOREARMPOINT, psStructure);
				}
			}

			// if found a droid to rearm assign it to the rearm pad
			if (psDroid != nullptr)
			{
				/* set chosen object */
				psChosenObj = psDroid;
				psReArmPad->psObj = psChosenObj;
				if (psDroid->action == DACTION_MOVETOREARMPOINT)
				{
					/* reset rearm started */
					psReArmPad->timeStarted = ACTION_START_TIME;
					psReArmPad->timeLastUpdated = 0;
				}
				auxStructureBlocking(psStructure);
			}
			else
			{
				auxStructureNonblocking(psStructure);
			}
			break;
		}
	default:
		break;
	}

	/* check subject stats (for research or manufacture) */
	if (pSubject != nullptr)
	{
		//if subject is research...
		if (structureMode == REF_RESEARCH)
		{
			psResFacility = &psStructure->pFunctionality->researchFacility;

			//if on hold don't do anything
			if (psResFacility->timeStartHold)
			{
				delPowerRequest(psStructure);
				return;
			}

			//electronic warfare affects the functionality of some structures in multiPlayer
			if (bMultiPlayer)
			{
				if (psStructure->resistance < structureResistance(psStructure->pStructureType, psStructure->player))
				{
					return;
				}
			}

			int researchIndex = pSubject->ref - REF_RESEARCH_START;

			PLAYER_RESEARCH *pPlayerRes = &asPlayerResList[psStructure->player][researchIndex];
			//check research has not already been completed by another structure
			if (!IsResearchCompleted(pPlayerRes))
			{
				pResearch = (RESEARCH *)pSubject;

				pointsToAdd = gameTimeAdjustedAverage(getBuildingResearchPoints(psStructure));
				pointsToAdd = MIN(pointsToAdd, pResearch->researchPoints - pPlayerRes->currentPoints);

				if (pointsToAdd > 0 && pPlayerRes->currentPoints == 0)
				{
					bool haveEnoughPower = requestPowerFor(psStructure, pResearch->researchPower);
					if (!haveEnoughPower)
					{
						pointsToAdd = 0;
					}
				}

				if (pointsToAdd > 0 && pResearch->researchPoints > 0)  // might be a "free" research
				{
					pPlayerRes->currentPoints += pointsToAdd;
				}
				syncDebug("Research at %u/%u.", pPlayerRes->currentPoints, pResearch->researchPoints);

				//check if Research is complete
				if (pPlayerRes->currentPoints >= pResearch->researchPoints)
				{
					int prevState = intGetResearchState();

					//store the last topic researched - if its the best
					if (psResFacility->psBestTopic == nullptr)
					{
						psResFacility->psBestTopic = psResFacility->psSubject;
					}
					else
					{
						if (pResearch->researchPoints > psResFacility->psBestTopic->researchPoints)
						{
							psResFacility->psBestTopic = psResFacility->psSubject;
						}
					}
					psResFacility->psSubject = nullptr;
					intResearchFinished(psStructure);
					researchResult(researchIndex, psStructure->player, true, psStructure, true);

					// Update allies research accordingly
					if (game.type == SKIRMISH && alliancesSharedResearch(game.alliance))
					{
						for (i = 0; i < MAX_PLAYERS; i++)
						{
							if (alliances[i][psStructure->player] == ALLIANCE_FORMED)
							{
								if (!IsResearchCompleted(&asPlayerResList[i][researchIndex]))
								{
									// Do the research for that player
									researchResult(researchIndex, i, false, nullptr, true);
								}
							}
						}
					}
					//check if this result has enabled another topic
					intNotifyResearchButton(prevState);
				}
			}
			else
			{
				//cancel this Structure's research since now complete
				psResFacility->psSubject = nullptr;
				intResearchFinished(psStructure);
				syncDebug("Research completed elsewhere.");
			}
		}
		//check for manufacture
		else if (structureMode == REF_FACTORY)
		{
			psFactory = &psStructure->pFunctionality->factory;

			//if on hold don't do anything
			if (psFactory->timeStartHold)
			{
				return;
			}

			//electronic warfare affects the functionality of some structures in multiPlayer
			if (bMultiPlayer)
			{
				if (psStructure->resistance < structureResistance(psStructure->pStructureType, psStructure->player))
				{
					return;
				}
			}

			if (psFactory->timeStarted == ACTION_START_TIME)
			{
				// also need to check if a command droid's group is full

				// If the factory commanders group is full - return
				if (IsFactoryCommanderGroupFull(psFactory))
				{
					return;
				}

				if (CheckHaltOnMaxUnitsReached(psStructure) == true)
				{
					return;
				}
			}

			/*must be enough power so subtract that required to build*/
			if (psFactory->timeStarted == ACTION_START_TIME)
			{
				//set the time started
				psFactory->timeStarted = gameTime;
			}

			if (psFactory->buildPointsRemaining > 0)
			{
				int progress = gameTimeAdjustedAverage(getBuildingProductionPoints(psStructure));
				if (psFactory->buildPointsRemaining == calcTemplateBuild(psFactory->psSubject) && progress > 0)
				{
					// We're just starting to build, check for power.
					bool haveEnoughPower = requestPowerFor(psStructure, calcTemplatePower(psFactory->psSubject));
					if (!haveEnoughPower)
					{
						progress = 0;
					}
				}
				psFactory->buildPointsRemaining -= progress;
			}

			//check for manufacture to be complete
			if ((psFactory->buildPointsRemaining <= 0) && !IsFactoryCommanderGroupFull(psFactory) && !CheckHaltOnMaxUnitsReached(psStructure))
			{
				if (isMission)
				{
					// put it in the mission list
					psDroid = buildMissionDroid((DROID_TEMPLATE *)pSubject,
					                            psStructure->pos.x, psStructure->pos.y,
					                            psStructure->player);
					if (psDroid)
					{
						setDroidBase(psDroid, psStructure);
						bDroidPlaced = true;
					}
				}
				else
				{
					// place it on the map
					bDroidPlaced = structPlaceDroid(psStructure, (DROID_TEMPLATE *)pSubject, &psDroid);
				}

				//reset the start time
				psFactory->timeStarted = ACTION_START_TIME;
				psFactory->psSubject = nullptr;

				doNextProduction(psStructure, (DROID_TEMPLATE *)pSubject, ModeImmediate);

				//script callback, must be called after factory was flagged as idle
				if (bDroidPlaced)
				{
					cbNewDroid(psStructure, psDroid);
				}
			}
		}
	}

	/* check base object (for repair / rearm) */
	if (psChosenObj != nullptr)
	{
		if (structureMode == REF_REPAIR_FACILITY)
		{
			psDroid = (DROID *) psChosenObj;
			ASSERT_OR_RETURN(, psDroid != nullptr, "invalid droid pointer");
			psRepairFac = &psStructure->pFunctionality->repairFacility;

			xdiff = (SDWORD)psDroid->pos.x - (SDWORD)psStructure->pos.x;
			ydiff = (SDWORD)psDroid->pos.y - (SDWORD)psStructure->pos.y;
			if (xdiff * xdiff + ydiff * ydiff <= (TILE_UNITS * 5 / 2) * (TILE_UNITS * 5 / 2))
			{
				//check droid is not healthy
				if (psDroid->body < psDroid->originalBody)
				{
					//if in multiPlayer, and a Transporter - make sure its on the ground before repairing
					if (bMultiPlayer && isTransporter(psDroid))
					{
						if (!(psDroid->sMove.Status == MOVEINACTIVE &&
						      psDroid->sMove.iVertSpeed == 0))
						{
							objTrace(psStructure->id, "Waiting for transporter to land");
							return;
						}
					}

					//don't do anything if the resistance is low in multiplayer
					if (bMultiPlayer)
					{
						if (psStructure->resistance < structureResistance(psStructure->pStructureType, psStructure->player))
						{
							objTrace(psStructure->id, "Resistance too low for repair");
							return;
						}
					}

					psDroid->body += gameTimeAdjustedAverage(getBuildingRepairPoints(psStructure));
				}

				if (psDroid->body >= psDroid->originalBody)
				{
					objTrace(psStructure->id, "Repair complete of droid %d", (int)psDroid->id);

					psRepairFac->psObj = nullptr;

					/* set droid points to max */
					psDroid->body = psDroid->originalBody;

					if ((psDroid->order.type == DORDER_RTR || psDroid->order.type == DORDER_RTR_SPECIFIED)
					    && psDroid->order.psObj == psStructure)
					{
						// if completely repaired reset order
						secondarySetState(psDroid, DSO_RETURN_TO_LOC, DSS_NONE);

						if (hasCommander(psDroid))
						{
							// return a droid to it's command group
							DROID	*psCommander = psDroid->psGroup->psCommander;

							objTrace(psDroid->id, "Repair complete - move to commander");
							orderDroidObj(psDroid, DORDER_GUARD, psCommander, ModeImmediate);
						}
						else if (psRepairFac->psDeliveryPoint != nullptr)
						{
							// move the droid out the way
							objTrace(psDroid->id, "Repair complete - move to delivery point");
							orderDroidLoc(psDroid, DORDER_MOVE,
							              psRepairFac->psDeliveryPoint->coords.x,
							              psRepairFac->psDeliveryPoint->coords.y, ModeQueue);  // ModeQueue because delivery points are not yet synchronised!
						}
					}
				}

				if (psStructure->visible[selectedPlayer] && psDroid->visible[selectedPlayer])
				{
					/* add plasma repair effect whilst being repaired */
					iVecEffect.x = psDroid->pos.x + (10 - rand() % 20);
					iVecEffect.y = psDroid->pos.z + (10 - rand() % 20);
					iVecEffect.z = psDroid->pos.y + (10 - rand() % 20);
					effectSetSize(100);
					addEffect(&iVecEffect, EFFECT_EXPLOSION, EXPLOSION_TYPE_SPECIFIED, true, getImdFromIndex(MI_FLAME), 0, gameTime - deltaGameTime + 1);
				}
			}
		}
		//check for rearming
		else if (structureMode == REF_REARM_PAD)
		{
			REARM_PAD	*psReArmPad = &psStructure->pFunctionality->rearmPad;
			UDWORD pointsAlreadyAdded;

			psDroid = (DROID *)psChosenObj;
			ASSERT_OR_RETURN(, psDroid != nullptr, "invalid droid pointer");
			ASSERT_OR_RETURN(, isVtolDroid(psDroid), "invalid droid type");

			//check hasn't died whilst waiting to be rearmed
			// also clear out any previously repaired droid
			if (psDroid->died || (psDroid->action != DACTION_MOVETOREARMPOINT && psDroid->action != DACTION_WAITDURINGREARM))
			{
				psReArmPad->psObj = nullptr;
				objTrace(psDroid->id, "VTOL has wrong action or is dead");
				return;
			}
			if (psDroid->action == DACTION_WAITDURINGREARM && psDroid->sMove.Status == MOVEINACTIVE)
			{
				if (psReArmPad->timeStarted == ACTION_START_TIME)
				{
					//set the time started and last updated
					psReArmPad->timeStarted = gameTime;
					psReArmPad->timeLastUpdated = gameTime;
				}
				pointsToAdd = getBuildingRearmPoints(psStructure) * (gameTime - psReArmPad->timeStarted) / GAME_TICKS_PER_SEC;
				pointsAlreadyAdded = getBuildingRearmPoints(psStructure) * (psReArmPad->timeLastUpdated - psReArmPad->timeStarted) / GAME_TICKS_PER_SEC;
				if (pointsToAdd >= psDroid->weight) // amount required is a factor of the droid weight
				{
					// We should be fully loaded by now.
					for (i = 0; i < psDroid->numWeaps; i++)
					{
						// set rearm value to no runs made
						psDroid->asWeaps[i].usedAmmo = 0;
						psDroid->asWeaps[i].ammo = asWeaponStats[psDroid->asWeaps[i].nStat].upgrade[psDroid->player].numRounds;
						psDroid->asWeaps[i].lastFired = 0;
					}
					objTrace(psDroid->id, "fully loaded");
				}
				else
				{
					for (i = 0; i < psDroid->numWeaps; i++)		// rearm one weapon at a time
					{
						// Make sure it's a rearmable weapon (and so we don't divide by zero)
						if (psDroid->asWeaps[i].usedAmmo > 0 && asWeaponStats[psDroid->asWeaps[i].nStat].upgrade[psDroid->player].numRounds > 0)
						{
							// Do not "simplify" this formula.
							// It is written this way to prevent rounding errors.
							int ammoToAddThisTime =
							    pointsToAdd * getNumAttackRuns(psDroid, i) / psDroid->weight -
							    pointsAlreadyAdded * getNumAttackRuns(psDroid, i) / psDroid->weight;
							psDroid->asWeaps[i].usedAmmo -= std::min<unsigned>(ammoToAddThisTime, psDroid->asWeaps[i].usedAmmo);
							if (ammoToAddThisTime)
							{
								// reset ammo and lastFired
								psDroid->asWeaps[i].ammo = asWeaponStats[psDroid->asWeaps[i].nStat].upgrade[psDroid->player].numRounds;
								psDroid->asWeaps[i].lastFired = 0;
								break;
							}
						}
					}
				}
				if (psDroid->body < psDroid->originalBody) // do repairs
				{
					psDroid->body += gameTimeAdjustedAverage(getBuildingRepairPoints(psStructure));
					if (psDroid->body >= psDroid->originalBody)
					{
						psDroid->body = psDroid->originalBody;
					}
				}
				psReArmPad->timeLastUpdated = gameTime;

				//check for fully armed and fully repaired
				if (vtolHappy(psDroid))
				{
					//clear the rearm pad
					psDroid->action = DACTION_NONE;
					psReArmPad->psObj = nullptr;
					auxStructureNonblocking(psStructure);
					triggerEventDroidIdle(psDroid);
					objTrace(psDroid->id, "VTOL happy and ready for action!");
				}
			}
		}
	}
}


/** Decides whether a structure should emit smoke when it's damaged */
static bool canSmoke(const STRUCTURE *psStruct)
{
	if (psStruct->pStructureType->type == REF_WALL || psStruct->pStructureType->type == REF_WALLCORNER
	    || psStruct->status == SS_BEING_BUILT || psStruct->pStructureType->type == REF_GATE)
	{
		return (false);
	}
	else
	{
		return (true);
	}
}

static float CalcStructureSmokeInterval(float damage)
{
	return (((1. - damage) + 0.1) * 10) * STRUCTURE_DAMAGE_SCALING;
}

void _syncDebugStructure(const char *function, STRUCTURE const *psStruct, char ch)
{
	int ref = 0;
	int refChr = ' ';

	// Print what the structure is producing, too.
	switch (psStruct->pStructureType->type)
	{
	case REF_RESEARCH:
		if (psStruct->pFunctionality->researchFacility.psSubject != nullptr)
		{
			ref = psStruct->pFunctionality->researchFacility.psSubject->ref;
			refChr = 'r';
		}
		break;
	case REF_FACTORY:
	case REF_CYBORG_FACTORY:
	case REF_VTOL_FACTORY:
		if (psStruct->pFunctionality->factory.psSubject != nullptr)
		{
			ref = psStruct->pFunctionality->factory.psSubject->multiPlayerID;
			refChr = 'p';
		}
		break;
	default:
		break;
	}

	int list[] =
	{
		ch,

		(int)psStruct->id,

		psStruct->player,
		psStruct->pos.x, psStruct->pos.y, psStruct->pos.z,
		(int)psStruct->status,
		(int)psStruct->pStructureType->type, refChr, ref,
		(int)psStruct->currentBuildPts,
		(int)psStruct->body,
	};
	_syncDebugIntList(function, "%c structure%d = p%d;pos(%d,%d,%d),status%d,type%d,%c%.0d,bld%d,body%d", list, ARRAY_SIZE(list));
}

int requestOpenGate(STRUCTURE *psStructure)
{
	if (psStructure->status != SS_BUILT || psStructure->pStructureType->type != REF_GATE)
	{
		return 0;  // Can't open.
	}

	switch (psStructure->state)
	{
	case SAS_NORMAL:
		psStructure->lastStateTime = gameTime;
		psStructure->state = SAS_OPENING;
		break;
	case SAS_OPEN:
		psStructure->lastStateTime = gameTime;
		return 0;  // Already open.
	case SAS_OPENING:
		break;
	case SAS_CLOSING:
		psStructure->lastStateTime = 2 * gameTime - psStructure->lastStateTime - SAS_OPEN_SPEED;
		psStructure->state = SAS_OPENING;
		return 0; // Busy
	}

	return psStructure->lastStateTime + SAS_OPEN_SPEED - gameTime;
}

int gateCurrentOpenHeight(const STRUCTURE *psStructure, uint32_t time, int minimumStub)
{
	STRUCTURE_STATS const *psStructureStats = psStructure->pStructureType;
	if (psStructureStats->type == REF_GATE)
	{
		int height = psStructure->sDisplay.imd->max.y;
		int openHeight;
		switch (psStructure->state)
		{
		case SAS_OPEN:
			openHeight = height;
			break;
		case SAS_OPENING:
			openHeight = (height * std::max<int>(time + GAME_TICKS_PER_UPDATE - psStructure->lastStateTime, 0)) / SAS_OPEN_SPEED;
			break;
		case SAS_CLOSING:
			openHeight = height - (height * std::max<int>(time - psStructure->lastStateTime, 0)) / SAS_OPEN_SPEED;
			break;
		default:
			return 0;
		}
		return std::max(std::min(openHeight, height - minimumStub), 0);
	}
	return 0;
}

/* The main update routine for all Structures */
void structureUpdate(STRUCTURE *psBuilding, bool mission)
{
	UDWORD widthScatter, breadthScatter;
	UDWORD emissionInterval, iPointsToAdd, iPointsRequired;
	Vector3i dv;
	int i;

	syncDebugStructure(psBuilding, '<');

	if (psBuilding->flags.test(OBJECT_FLAG_DIRTY) && !mission)
	{
		visTilesUpdate(psBuilding);
		psBuilding->flags.set(OBJECT_FLAG_DIRTY, false);
	}

	if (psBuilding->pStructureType->type == REF_GATE)
	{
		if (psBuilding->state == SAS_OPEN && psBuilding->lastStateTime + SAS_STAY_OPEN_TIME < gameTime)
		{
			bool		found = false;

			static GridList gridList;  // static to avoid allocations.
			gridList = gridStartIterate(psBuilding->pos.x, psBuilding->pos.y, TILE_UNITS);
			for (GridIterator gi = gridList.begin(); !found && gi != gridList.end(); ++gi)
			{
				found = isDroid(*gi);
			}

			if (!found)	// no droids on our tile, safe to close
			{
				psBuilding->state = SAS_CLOSING;
				auxStructureClosedGate(psBuilding);     // closed
				psBuilding->lastStateTime = gameTime;	// reset timer
			}
		}
		else if (psBuilding->state == SAS_OPENING && psBuilding->lastStateTime + SAS_OPEN_SPEED < gameTime)
		{
			psBuilding->state = SAS_OPEN;
			auxStructureOpenGate(psBuilding);       // opened
			psBuilding->lastStateTime = gameTime;	// reset timer
		}
		else if (psBuilding->state == SAS_CLOSING && psBuilding->lastStateTime + SAS_OPEN_SPEED < gameTime)
		{
			psBuilding->state = SAS_NORMAL;
			psBuilding->lastStateTime = gameTime;	// reset timer
		}
	}
	else if (psBuilding->pStructureType->type == REF_RESOURCE_EXTRACTOR)
	{
		if (!psBuilding->pFunctionality->resourceExtractor.psPowerGen
		    && psBuilding->animationEvent == ANIM_EVENT_ACTIVE) // no power generator connected
		{
			psBuilding->timeAnimationStarted = 0; // so turn off animation, if any
			psBuilding->animationEvent = ANIM_EVENT_NONE;
		}
		else if (psBuilding->pFunctionality->resourceExtractor.psPowerGen
		         && psBuilding->animationEvent == ANIM_EVENT_NONE) // we have a power generator, but no animation
		{
			psBuilding->timeAnimationStarted = gameTime;  // so start animation
			psBuilding->animationEvent = ANIM_EVENT_ACTIVE;
		}
	}

	// Remove invalid targets. This must be done each frame.
	for (i = 0; i < MAX_WEAPONS; i++)
	{
		if (psBuilding->psTarget[i] && psBuilding->psTarget[i]->died)
		{
			setStructureTarget(psBuilding, nullptr, i, ORIGIN_UNKNOWN);
		}
	}

	//update the manufacture/research of the building once complete
	if (psBuilding->status == SS_BUILT)
	{
		aiUpdateStructure(psBuilding, mission);
	}

	if (psBuilding->status != SS_BUILT)
	{
		if (psBuilding->selected)
		{
			psBuilding->selected = false;
		}
	}

	if (!mission)
	{
		if (psBuilding->status == SS_BEING_BUILT && psBuilding->buildRate == 0 && !structureHasModules(psBuilding))
		{
			if (psBuilding->pStructureType->powerToBuild == 0)
			{
				// Building is free, and not currently being built, so deconstruct slowly over 1 minute.
				psBuilding->currentBuildPts -= std::min<int>(psBuilding->currentBuildPts, gameTimeAdjustedAverage(psBuilding->pStructureType->buildPoints, 60));
			}

			if (psBuilding->currentBuildPts == 0)
			{
				removeStruct(psBuilding, true);  // If giving up on building something, remove the structure (and remove it from the power queue).
			}
		}
		psBuilding->lastBuildRate = psBuilding->buildRate;
		psBuilding->buildRate = 0;  // Reset to 0, each truck building us will add to our buildRate.
	}

	/* Only add smoke if they're visible and they can 'burn' */
	if (!mission && psBuilding->visible[selectedPlayer] && canSmoke(psBuilding))
	{
		const int32_t damage = getStructureDamage(psBuilding);

		// Is there any damage?
		if (damage > 0.)
		{
			emissionInterval = CalcStructureSmokeInterval(damage / 65536.f);
			unsigned effectTime = std::max(gameTime - deltaGameTime + 1, psBuilding->lastEmission + emissionInterval);
			if (gameTime >= effectTime)
			{
				const Vector2i size = psBuilding->size();
				widthScatter   = size.x * TILE_UNITS / 2 / 3;
				breadthScatter = size.y * TILE_UNITS / 2 / 3;
				dv.x = psBuilding->pos.x + widthScatter - rand() % (2 * widthScatter);
				dv.z = psBuilding->pos.y + breadthScatter - rand() % (2 * breadthScatter);
				dv.y = psBuilding->pos.z;
				dv.y += (psBuilding->sDisplay.imd->max.y * 3) / 4;
				addEffect(&dv, EFFECT_SMOKE, SMOKE_TYPE_DRIFTING_HIGH, false, nullptr, 0, effectTime);
				psBuilding->lastEmission = effectTime;
			}
		}
	}

	/* Update the fire damage data */
	if (psBuilding->periodicalDamageStart != 0 && psBuilding->periodicalDamageStart != gameTime - deltaGameTime)  // -deltaGameTime, since projectiles are updated after structures.
	{
		// The periodicalDamageStart has been set, but is not from the previous tick, so we must be out of the fire.
		psBuilding->periodicalDamage = 0;  // Reset burn damage done this tick.
		// Finished burning.
		psBuilding->periodicalDamageStart = 0;
	}

	//check the resistance level of the structure
	iPointsRequired = structureResistance(psBuilding->pStructureType, psBuilding->player);
	if (psBuilding->resistance < (SWORD)iPointsRequired)
	{
		//start the resistance increase
		if (psBuilding->lastResistance == ACTION_START_TIME)
		{
			psBuilding->lastResistance = gameTime;
		}
		//increase over time if low
		if ((gameTime - psBuilding->lastResistance) > RESISTANCE_INTERVAL)
		{
			psBuilding->resistance++;

			//in multiplayer, certain structures do not function whilst low resistance
			if (bMultiPlayer)
			{
				resetResistanceLag(psBuilding);
			}

			psBuilding->lastResistance = gameTime;
			//once the resistance is back up reset the last time increased
			if (psBuilding->resistance >= (SWORD)iPointsRequired)
			{
				psBuilding->lastResistance = ACTION_START_TIME;
			}
		}
	}
	else
	{
		//if selfrepair has been researched then check the health level of the
		//structure once resistance is fully up
		iPointsRequired = structureBody(psBuilding);
		if (selfRepairEnabled(psBuilding->player) && psBuilding->body < iPointsRequired && psBuilding->status != SS_BEING_BUILT)
		{
			//start the self repair off
			if (psBuilding->lastResistance == ACTION_START_TIME)
			{
				psBuilding->lastResistance = gameTime;
			}

			/*since self repair, then add half repair points depending on the time delay for the stat*/
			iPointsToAdd = (repairPoints(asRepairStats + aDefaultRepair[
			                                 psBuilding->player], psBuilding->player) / 4) * ((gameTime -
			                                         psBuilding->lastResistance) / (asRepairStats +
			                                                 aDefaultRepair[psBuilding->player])->time);

			//add the blue flashing effect for multiPlayer
			if (bMultiPlayer && ONEINTEN && !mission)
			{
				Vector3i position;
				Vector3f *point;
				SDWORD	realY;
				UDWORD	pointIndex;

				pointIndex = rand() % (psBuilding->sDisplay.imd->npoints - 1);
				point = &(psBuilding->sDisplay.imd->points[pointIndex]);
				position.x = psBuilding->pos.x + point->x;
				realY = structHeightScale(psBuilding) * point->y;
				position.y = psBuilding->pos.z + realY;
				position.z = psBuilding->pos.y - point->z;

				effectSetSize(30);
				addEffect(&position, EFFECT_EXPLOSION, EXPLOSION_TYPE_SPECIFIED, true, getImdFromIndex(MI_PLASMA), 0, gameTime - deltaGameTime + rand() % deltaGameTime);
			}

			if (iPointsToAdd)
			{
				psBuilding->body = (UWORD)(psBuilding->body + iPointsToAdd);
				psBuilding->lastResistance = gameTime;
				if (psBuilding->body > iPointsRequired)
				{
					psBuilding->body = (UWORD)iPointsRequired;
					psBuilding->lastResistance = ACTION_START_TIME;
				}
			}
		}
	}

	syncDebugStructure(psBuilding, '>');

	CHECK_STRUCTURE(psBuilding);
}

STRUCTURE::STRUCTURE(uint32_t id, unsigned player)
	: BASE_OBJECT(OBJ_STRUCTURE, id, player)
	, pFunctionality(nullptr)
	, buildRate(1)  // Initialise to 1 instead of 0, to make sure we don't get destroyed first tick due to inactivity.
	, lastBuildRate(0)
	, prebuiltImd(nullptr)
{
	pos = Vector3i(0, 0, 0);
	rot = Vector3i(0, 0, 0);
	capacity = 0;
}

/* Release all resources associated with a structure */
STRUCTURE::~STRUCTURE()
{
	// Make sure to get rid of some final references in the sound code to this object first
	audio_RemoveObj(this);

	STRUCTURE *psBuilding = this;

	// free up the space used by the functionality array
	free(psBuilding->pFunctionality);
	psBuilding->pFunctionality = nullptr;
}


/*
fills the list with Structure that can be built. There is a limit on how many can
be built at any one time. Pass back the number available.
There is now a limit of how many of each type of structure are allowed per mission
*/
UDWORD fillStructureList(STRUCTURE_STATS **ppList, UDWORD selectedPlayer, UDWORD limit)
{
	UDWORD			inc, count;
	bool			researchModule, factoryModule, powerModule;
	STRUCTURE		*psCurr;
	STRUCTURE_STATS	*psBuilding;

	//check to see if able to build research/factory modules
	researchModule = factoryModule = powerModule = false;

	//if currently on a mission can't build factory/research/power/derricks
	if (!missionIsOffworld())
	{
		for (psCurr = apsStructLists[selectedPlayer]; psCurr != nullptr; psCurr = psCurr->psNext)
		{
			if (psCurr->pStructureType->type == REF_RESEARCH && psCurr->status == SS_BUILT)
			{
				researchModule = true;
			}
			else if (psCurr->pStructureType->type == REF_FACTORY && psCurr->status == SS_BUILT)
			{
				factoryModule = true;
			}
			else if (psCurr->pStructureType->type == REF_POWER_GEN && psCurr->status == SS_BUILT)
			{
				powerModule = true;
			}
		}
	}

	count = 0;
	//set the list of Structures to build
	for (inc = 0; inc < numStructureStats; inc++)
	{
		//if the structure is flagged as available, add it to the list
		if (apStructTypeLists[selectedPlayer][inc] == AVAILABLE || (includeRedundantDesigns && apStructTypeLists[selectedPlayer][inc] == REDUNDANT))
		{
			//check not built the maximum allowed already
			if (asStructureStats[inc].curCount[selectedPlayer] < asStructureStats[inc].upgrade[selectedPlayer].limit)
			{
				psBuilding = asStructureStats + inc;

				//don't want corner wall to appear in list
				if (psBuilding->type == REF_WALLCORNER)
				{
					continue;
				}

				// Remove the demolish stat from the list for tutorial
				// tjc 4-dec-98  ...
				if (bInTutorial)
				{
					if (psBuilding->type == REF_DEMOLISH)
					{
						continue;
					}
				}

				//can't build list when offworld
				if (missionIsOffworld())
				{
					if (psBuilding->type == REF_FACTORY ||
					    psBuilding->type == REF_POWER_GEN ||
					    psBuilding->type == REF_RESOURCE_EXTRACTOR ||
					    psBuilding->type == REF_RESEARCH ||
					    psBuilding->type == REF_CYBORG_FACTORY ||
					    psBuilding->type == REF_VTOL_FACTORY)
					{
						continue;
					}
				}

				if (psBuilding->type == REF_RESEARCH_MODULE)
				{
					//don't add to list if Research Facility not presently built
					if (!researchModule)
					{
						continue;
					}
				}
				else if (psBuilding->type == REF_FACTORY_MODULE)
				{
					//don't add to list if Factory not presently built
					if (!factoryModule)
					{
						continue;
					}
				}
				else if (psBuilding->type == REF_POWER_MODULE)
				{
					//don't add to list if Power Gen not presently built
					if (!powerModule)
					{
						continue;
					}
				}

				debug(LOG_NEVER, "adding %s (%x)", getName(psBuilding), apStructTypeLists[selectedPlayer][inc]);
				ppList[count++] = psBuilding;
				if (count == limit)
				{
					return count;
				}
			}
		}
	}
	return count;
}


enum STRUCTURE_PACKABILITY
{
	PACKABILITY_EMPTY = 0, PACKABILITY_DEFENSE = 1, PACKABILITY_NORMAL = 2, PACKABILITY_REPAIR = 3
};

static inline bool canPack(STRUCTURE_PACKABILITY a, STRUCTURE_PACKABILITY b)
{
	return (int)a + (int)b <= 3;  // Defense can be put next to anything except repair facilities, normal base structures can't be put next to each other, and anything goes next to empty tiles.
}

static STRUCTURE_PACKABILITY baseStructureTypePackability(STRUCTURE_TYPE type)
{
	switch (type)
	{
	case REF_DEFENSE:
	case REF_WALL:
	case REF_WALLCORNER:
	case REF_GATE:
	case REF_REARM_PAD:
	case REF_MISSILE_SILO:
		return PACKABILITY_DEFENSE;
	default:
		return PACKABILITY_NORMAL;
	case REF_REPAIR_FACILITY:
		return PACKABILITY_REPAIR;
	}
}

static STRUCTURE_PACKABILITY baseObjectPackability(BASE_OBJECT *psObject)
{
	if (psObject == nullptr)
	{
		return PACKABILITY_EMPTY;
	}
	switch (psObject->type)
	{
	case OBJ_STRUCTURE: return baseStructureTypePackability(((STRUCTURE *)psObject)->pStructureType->type);
	case OBJ_FEATURE:   return ((FEATURE *)psObject)->psStats->subType == FEAT_OIL_RESOURCE ? PACKABILITY_NORMAL : PACKABILITY_EMPTY;
	default:            return PACKABILITY_EMPTY;
	}
}

bool isBlueprintTooClose(STRUCTURE_STATS const *stats1, Vector2i pos1, uint16_t dir1, STRUCTURE_STATS const *stats2, Vector2i pos2, uint16_t dir2)
{
	if (stats1 == stats2 && pos1 == pos2 && dir1 == dir2)
	{
		return false;  // Same blueprint, so ignore it.
	}

	bool packable = canPack(baseStructureTypePackability(stats1->type), baseStructureTypePackability(stats2->type));
	int minDist = packable ? 0 : 1;
	StructureBounds b1 = getStructureBounds(stats1, pos1, dir1);
	StructureBounds b2 = getStructureBounds(stats2, pos2, dir2);
	Vector2i delta12 = b2.map - (b1.map + b1.size);
	Vector2i delta21 = b1.map - (b2.map + b2.size);
	int dist = std::max(std::max(delta12.x, delta21.x), std::max(delta12.y, delta21.y));
	return dist < minDist;
}

bool validLocation(BASE_STATS *psStats, Vector2i pos, uint16_t direction, unsigned player, bool bCheckBuildQueue)
{
	STRUCTURE_STATS        *psBuilding = nullptr;
	DROID_TEMPLATE         *psTemplate = nullptr;

	StructureBounds b = getStructureBounds(psStats, pos, direction);

	if (psStats->ref >= REF_STRUCTURE_START && psStats->ref < REF_STRUCTURE_START + REF_RANGE)
	{
		psBuilding = (STRUCTURE_STATS *)psStats;  // Is a structure.
	}
	if (psStats->ref >= REF_TEMPLATE_START && psStats->ref < REF_TEMPLATE_START + REF_RANGE)
	{
		psTemplate = (DROID_TEMPLATE *)psStats;  // Is a template.
	}

	if (psBuilding != nullptr)
	{
		//if we're dragging the wall/defense we need to check along the current dragged size
		if (wallDrag.status != DRAG_INACTIVE && bCheckBuildQueue
		    && (psBuilding->type == REF_WALL || psBuilding->type == REF_DEFENSE || psBuilding->type == REF_REARM_PAD ||  psBuilding->type == REF_GATE)
		    && !isLasSat(psBuilding))
		{
			wallDrag.x2 = mouseTileX;  // Why must this be done here? If not doing it here, dragging works almost normally, except it suddenly stops working if the drag becomes invalid.
			wallDrag.y2 = mouseTileY;

			int dx = abs(wallDrag.x2 - wallDrag.x1);
			int dy = abs(wallDrag.y2 - wallDrag.y1);
			if (dx >= dy)
			{
				//build in x direction
				wallDrag.y2 = wallDrag.y1;
			}
			else
			{
				//build in y direction
				wallDrag.x2 = wallDrag.x1;
			}
			b.map.x = std::min(wallDrag.x1, wallDrag.x2);
			b.map.y = std::min(wallDrag.y1, wallDrag.y2);
			b.size.x = std::max(wallDrag.x1, wallDrag.x2) + 1 - b.map.x;
			b.size.y = std::max(wallDrag.y1, wallDrag.y2) + 1 - b.map.y;
		}
	}

	//make sure we are not too near map edge and not going to go over it
	if (b.map.x < scrollMinX + TOO_NEAR_EDGE || b.map.x + b.size.x > scrollMaxX - TOO_NEAR_EDGE ||
	    b.map.y < scrollMinY + TOO_NEAR_EDGE || b.map.y + b.size.y > scrollMaxY - TOO_NEAR_EDGE)
	{
		return false;
	}

	if (bCheckBuildQueue)
	{
		// cant place on top of a delivery point...
		for (FLAG_POSITION const *psCurrFlag = apsFlagPosLists[selectedPlayer]; psCurrFlag; psCurrFlag = psCurrFlag->psNext)
		{
			ASSERT_OR_RETURN(false, psCurrFlag->coords.x != ~0, "flag has invalid position");
			Vector2i flagTile = map_coord(psCurrFlag->coords.xy);
			if (flagTile.x >= b.map.x && flagTile.x < b.map.x + b.size.x && flagTile.y >= b.map.y && flagTile.y < b.map.y + b.size.y)
			{
				return false;
			}
		}
	}

	if (psBuilding != nullptr)
	{
		for (int j = 0; j < b.size.y; ++j)
			for (int i = 0; i < b.size.x; ++i)
			{
				// Don't allow building structures (allow delivery points, though) outside visible area in single-player with debug mode off. (Why..?)
				if (!bMultiPlayer && !getDebugMappingStatus() && !TEST_TILE_VISIBLE(player, mapTile(b.map.x + i, b.map.y + j)))
				{
					return false;
				}
			}

		switch (psBuilding->type)
		{
		case REF_DEMOLISH:
			break;
		case NUM_DIFF_BUILDINGS:
		case REF_BRIDGE:
			ASSERT(!"invalid structure type", "Bad structure type %u", psBuilding->type);
			break;
		case REF_HQ:
		case REF_FACTORY:
		case REF_LAB:
		case REF_RESEARCH:
		case REF_POWER_GEN:
		case REF_WALL:
		case REF_WALLCORNER:
		case REF_GATE:
		case REF_DEFENSE:
		case REF_REPAIR_FACILITY:
		case REF_COMMAND_CONTROL:
		case REF_CYBORG_FACTORY:
		case REF_VTOL_FACTORY:
		case REF_GENERIC:
		case REF_REARM_PAD:
		case REF_MISSILE_SILO:
		case REF_SAT_UPLINK:
			{
				/*need to check each tile the structure will sit on is not water*/
				for (int j = 0; j < b.size.y; ++j)
					for (int i = 0; i < b.size.x; ++i)
					{
						MAPTILE const *psTile = mapTile(b.map.x + i, b.map.y + j);
						if ((terrainType(psTile) == TER_WATER) ||
						    (terrainType(psTile) == TER_CLIFFFACE))
						{
							return false;
						}
					}
				//check not within landing zone
				for (int j = 0; j < b.size.y; ++j)
					for (int i = 0; i < b.size.x; ++i)
					{
						if (withinLandingZone(b.map.x + i, b.map.y + j))
						{
							return false;
						}
					}

				//walls/defensive structures can be built along any ground
				if (!(psBuilding->type == REF_REPAIR_FACILITY ||
				      psBuilding->type == REF_DEFENSE ||
				      psBuilding->type == REF_GATE ||
				      psBuilding->type == REF_WALL))
				{
					/*cannot build on ground that is too steep*/
					for (int j = 0; j < b.size.y; ++j)
						for (int i = 0; i < b.size.x; ++i)
						{
							int max, min;
							getTileMaxMin(b.map.x + i, b.map.y + j, &max, &min);
							if (max - min > MAX_INCLINE)
							{
								return false;
							}
						}
				}
				//don't bother checking if already found a problem
				STRUCTURE_PACKABILITY packThis = baseStructureTypePackability(psBuilding->type);

				// skirmish AIs don't build nondefensives next to anything. (route hack)
				if (packThis == PACKABILITY_NORMAL && bMultiPlayer && game.type == SKIRMISH && !isHumanPlayer(player))
				{
					packThis = PACKABILITY_REPAIR;
				}

				/* need to check there is one tile between buildings */
				for (int j = -1; j < b.size.y + 1; ++j)
					for (int i = -1; i < b.size.x + 1; ++i)
					{
						//skip the actual area the structure will cover
						if (i < 0 || i >= b.size.x || j < 0 || j >= b.size.y)
						{
							BASE_OBJECT *object = mapTile(b.map.x + i, b.map.y + j)->psObject;
							STRUCTURE *structure = castStructure(object);
							if (structure != nullptr && !structure->visible[player] && !aiCheckAlliances(player, structure->player))
							{
								continue;  // Ignore structures we can't see.
							}

							STRUCTURE_PACKABILITY packObj = baseObjectPackability(object);

							if (!canPack(packThis, packObj))
							{
								return false;
							}
						}
					}
				if (psBuilding->flags & STRUCTURE_CONNECTED)
				{
					bool connection = false;
					for (int j = -1; j < b.size.y + 1; ++j)
					{
						for (int i = -1; i < b.size.x + 1; ++i)
						{
							//skip the actual area the structure will cover
							if (i < 0 || i >= b.size.x || j < 0 || j >= b.size.y)
							{
								STRUCTURE const *psStruct = getTileStructure(b.map.x + i, b.map.y + j);
								if (psStruct != nullptr && psStruct->player == player && psStruct->status == SS_BUILT)
								{
									connection = true;
									break;
								}
							}
						}
					}
					if (!connection)
					{
						return false; // needed to be connected to another building
					}
				}

				/*need to check each tile the structure will sit on*/
				for (int j = 0; j < b.size.y; ++j)
					for (int i = 0; i < b.size.x; ++i)
					{
						MAPTILE const *psTile = mapTile(b.map.x + i, b.map.y + j);
						if (TileIsKnownOccupied(psTile, player))
						{
							if (TileHasWall(psTile) && (psBuilding->type == REF_DEFENSE || psBuilding->type == REF_GATE || psBuilding->type == REF_WALL))
							{
								STRUCTURE const *psStruct = getTileStructure(b.map.x + i, b.map.y + j);
								if (psStruct != nullptr && psStruct->player != player)
								{
									return false;
								}
							}
							else
							{
								return false;
							}
						}
					}
				break;
			}
		case REF_FACTORY_MODULE:
			if (TileHasStructure(worldTile(pos)))
			{
				STRUCTURE const *psStruct = getTileStructure(map_coord(pos.x), map_coord(pos.y));
				if (psStruct && (psStruct->pStructureType->type == REF_FACTORY ||
				                 psStruct->pStructureType->type == REF_VTOL_FACTORY) &&
				    psStruct->status == SS_BUILT && aiCheckAlliances(player, psStruct->player))
				{
					break;
				}
			}
			return false;
		case REF_RESEARCH_MODULE:
			if (TileHasStructure(worldTile(pos)))
			{
				STRUCTURE const *psStruct = getTileStructure(map_coord(pos.x), map_coord(pos.y));
				if (psStruct && psStruct->pStructureType->type == REF_RESEARCH &&
				    psStruct->status == SS_BUILT && aiCheckAlliances(player, psStruct->player))
				{
					break;
				}
			}
			return false;
		case REF_POWER_MODULE:
			if (TileHasStructure(worldTile(pos)))
			{
				STRUCTURE const *psStruct = getTileStructure(map_coord(pos.x), map_coord(pos.y));
				if (psStruct && psStruct->pStructureType->type == REF_POWER_GEN &&
				    psStruct->status == SS_BUILT && aiCheckAlliances(player, psStruct->player))
				{
					break;
				}
			}
			return false;
		case REF_RESOURCE_EXTRACTOR:
			if (TileHasFeature(worldTile(pos)))
			{
				FEATURE const *psFeat = getTileFeature(map_coord(pos.x), map_coord(pos.y));
				if (psFeat && psFeat->psStats->subType == FEAT_OIL_RESOURCE)
				{
					break;
				}
			}
			return false;
		}
		//if setting up a build queue need to check against future sites as well - AB 4/5/99
		if (ctrlShiftDown() && player == selectedPlayer && bCheckBuildQueue &&
		    anyBlueprintTooClose(psBuilding, pos, direction))
		{
			return false;
		}
	}
	else if (psTemplate != nullptr)
	{
		PROPULSION_STATS *psPropStats = asPropulsionStats + psTemplate->asParts[COMP_PROPULSION];

		if (fpathBlockingTile(b.map.x, b.map.y, psPropStats->propulsionType))
		{
			return false;
		}
	}
	else
	{
		// not positioning a structure or droid, ie positioning a feature
		if (fpathBlockingTile(b.map.x, b.map.y, PROPULSION_TYPE_WHEELED))
		{
			return false;
		}
	}

	return true;
}

/*
for a new structure, find a location along an edge which the droid can get
to and return this as the destination for the droid.
*/
bool getDroidDestination(BASE_STATS *psStats, UDWORD structX,
                         UDWORD structY, UDWORD *pDroidX, UDWORD *pDroidY)
{
	int32_t                         start;
	UDWORD				structTileX, structTileY, width = 0, breadth = 0;

	if (StatIsStructure(psStats))
	{
		width = ((STRUCTURE_STATS *)psStats)->baseWidth;
		breadth = ((STRUCTURE_STATS *)psStats)->baseBreadth;
	}
	else if (StatIsFeature(psStats))
	{
		width = ((FEATURE_STATS *)psStats)->baseWidth;
		breadth = ((FEATURE_STATS *)psStats)->baseBreadth;
	}
	ASSERT_OR_RETURN(false, width + breadth > 0, "Weird droid destination");

	//get a random starting place 0=top left
	start = gameRand((width + breadth) * 2);

	//search in a clockwise direction around the structure from the starting point
	// TODO Fix 4x code duplication.
	if (start == 0 || start < width)
	{
		//top side first
		structTileX = map_coord(structX);
		structTileY = map_coord(structY) - 1;
		if (checkWidth(width, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
		structTileX += width;
		structTileY += 1;

		if (checkLength(breadth, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
		structTileX = map_coord(structX);
		structTileY += breadth;

		if (checkWidth(width, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
		structTileX -= 1;
		structTileY = map_coord(structY);

		if (checkLength(breadth, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
	}
	else if (start == width || start < (width + breadth))
	{
		//right side first
		structTileX = (map_coord(structX)) + width;
		structTileY = map_coord(structY);

		if (checkLength(breadth, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
		structTileX = map_coord(structX);
		structTileY += breadth;

		if (checkWidth(width, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
		structTileX -= 1;
		structTileY = map_coord(structY);

		if (checkLength(breadth, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
		structTileX += 1;
		structTileY -= 1;

		if (checkWidth(width, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
	}
	else if (start == (width + breadth) || start < (width * breadth))
	{
		//bottom first
		structTileX = map_coord(structX);
		structTileY = map_coord(structY) + breadth;

		if (checkWidth(width, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
		structTileX -= 1;
		structTileY = map_coord(structY);

		if (checkLength(breadth, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
		structTileX += 1;
		structTileY -= 1;

		if (checkWidth(width, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
		structTileX += width;
		structTileY += 1;

		if (checkLength(breadth, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
	}
	else
	{
		//left side first
		structTileX = (map_coord(structX)) - 1;
		structTileY = map_coord(structY);

		if (checkLength(breadth, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
		structTileX += 1;
		structTileY -= 1;

		if (checkWidth(width, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
		structTileX += width;
		structTileY += 1;

		if (checkLength(breadth, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
		structTileX = map_coord(structX);
		structTileY += breadth;

		if (checkWidth(width, structTileX, structTileY, pDroidX, pDroidY))
		{
			return true;
		}
	}

	//not found a valid location so return false
	return false;
}

/* check along the width of a structure for an empty space */
bool checkWidth(UDWORD maxRange, UDWORD x, UDWORD y, UDWORD *pDroidX, UDWORD *pDroidY)
{
	UDWORD		side;

	for (side = 0; side < maxRange; side++)
	{
		if (x + side < mapWidth && y < mapHeight && !TileIsOccupied(mapTile(x + side, y)))
		{
			*pDroidX = world_coord(x + side);
			*pDroidY = world_coord(y);

			ASSERT_OR_RETURN(false, worldOnMap(*pDroidX, *pDroidY), "Insane droid position generated at width (%u, %u)", *pDroidX, *pDroidY);

			return true;
		}
	}

	return false;
}

/* check along the length of a structure for an empty space */
bool checkLength(UDWORD maxRange, UDWORD x, UDWORD y, UDWORD *pDroidX, UDWORD *pDroidY)
{
	UDWORD		side;

	for (side = 0; side < maxRange; side++)
	{
		if (y + side < mapHeight && x < mapWidth && !TileIsOccupied(mapTile(x, y + side)))
		{
			*pDroidX = world_coord(x);
			*pDroidY = world_coord(y + side);

			ASSERT_OR_RETURN(false, worldOnMap(*pDroidX, *pDroidY), "Insane droid position generated at length (%u, %u)", *pDroidX, *pDroidY);

			return true;
		}
	}

	return false;
}


//remove a structure from the map
static void removeStructFromMap(STRUCTURE *psStruct)
{
	auxStructureNonblocking(psStruct);

	/* set tiles drawing */
	StructureBounds b = getStructureBounds(psStruct);
	for (int j = 0; j < b.size.y; ++j)
	{
		for (int i = 0; i < b.size.x; ++i)
		{
			MAPTILE *psTile = mapTile(b.map.x + i, b.map.y + j);
			psTile->psObject = nullptr;
			auxClearBlocking(b.map.x + i, b.map.y + j, AIR_BLOCKED);
		}
	}
}

// remove a structure from a game without any visible effects
// bDestroy = true if the object is to be destroyed
// (for example used to change the type of wall at a location)
bool removeStruct(STRUCTURE *psDel, bool bDestroy)
{
	bool		resourceFound = false;
	FLAG_POSITION	*psAssemblyPoint = nullptr;

	ASSERT_OR_RETURN(false, psDel != nullptr, "Invalid structure pointer");

	int prevResearchState = intGetResearchState();

	if (bDestroy)
	{
		removeStructFromMap(psDel);
	}

	if (bDestroy)
	{
		//if the structure is a resource extractor, need to put the resource back in the map
		/*ONLY IF ANY POWER LEFT - HACK HACK HACK!!!! OIL POOLS NEED TO KNOW
		HOW MUCH IS THERE && NOT RES EXTRACTORS */
		if (psDel->pStructureType->type == REF_RESOURCE_EXTRACTOR)
		{
			FEATURE *psOil = buildFeature(oilResFeature, psDel->pos.x, psDel->pos.y, false);
			memcpy(psOil->seenThisTick, psDel->visible, sizeof(psOil->seenThisTick));
			resourceFound = true;
		}
	}

	if (psDel->pStructureType->type == REF_RESOURCE_EXTRACTOR)
	{
		//tell associated Power Gen
		releaseResExtractor(psDel);
	}

	if (psDel->pStructureType->type == REF_POWER_GEN)
	{
		//tell associated Res Extractors
		releasePowerGen(psDel);
	}

	//check for a research topic currently under way
	if (psDel->pStructureType->type == REF_RESEARCH)
	{
		if (psDel->pFunctionality->researchFacility.psSubject)
		{
			//cancel the topic
			cancelResearch(psDel, ModeImmediate);
		}
	}

	//subtract one from the structLimits list so can build another - don't allow to go less than zero!
	if (asStructureStats[psDel->pStructureType - asStructureStats].curCount[psDel->player])
	{
		asStructureStats[psDel->pStructureType - asStructureStats].curCount[psDel->player]--;
	}

	//if it is a factory - need to reset the factoryNumFlag
	if (StructIsFactory(psDel))
	{
		FACTORY *psFactory = &psDel->pFunctionality->factory;

		//need to initialise the production run as well
		cancelProduction(psDel, ModeImmediate);

		psAssemblyPoint = psFactory->psAssemblyPoint;
	}
	else if (psDel->pStructureType->type == REF_REPAIR_FACILITY)
	{
		psAssemblyPoint = psDel->pFunctionality->repairFacility.psDeliveryPoint;
	}

	if (psAssemblyPoint != nullptr)
	{
		if (psAssemblyPoint->factoryInc < factoryNumFlag[psDel->player][psAssemblyPoint->factoryType].size())
		{
			factoryNumFlag[psDel->player][psAssemblyPoint->factoryType][psAssemblyPoint->factoryInc] = false;
		}

		//need to cancel the repositioning of the DP if selectedPlayer and currently moving
		if (psDel->player == selectedPlayer && psAssemblyPoint->selected)
		{
			cancelDeliveryRepos();
		}
	}

	if (bDestroy)
	{
		debug(LOG_DEATH, "Killing off %s id %d (%p)", objInfo(psDel), psDel->id, psDel);
		killStruct(psDel);
	}

	if (psDel->player == selectedPlayer)
	{
		intRefreshScreen();
	}

	delPowerRequest(psDel);

	intNotifyResearchButton(prevResearchState);

	return resourceFound;
}

/* Remove a structure */
bool destroyStruct(STRUCTURE *psDel, unsigned impactTime)
{
	UDWORD			widthScatter, breadthScatter, heightScatter;

	const unsigned          burnDurationWall    =  1000;
	const unsigned          burnDurationOilWell = 60000;
	const unsigned          burnDurationOther   = 10000;

	CHECK_STRUCTURE(psDel);
	ASSERT(gameTime - deltaGameTime < impactTime, "Expected %u < %u, gameTime = %u, bad impactTime", gameTime - deltaGameTime, impactTime, gameTime);

	/* Firstly, are we dealing with a wall section */
	const STRUCTURE_TYPE type = psDel->pStructureType->type;
	const bool bMinor = type == REF_WALL || type == REF_WALLCORNER;
	const bool bDerrick = type == REF_RESOURCE_EXTRACTOR;
	const bool bPowerGen = type == REF_POWER_GEN;
	unsigned burnDuration = bMinor ? burnDurationWall : bDerrick ? burnDurationOilWell : burnDurationOther;
	if (psDel->status == SS_BEING_BUILT)
	{
		burnDuration = burnDuration * psDel->currentBuildPts / psDel->pStructureType->buildPoints;
	}

	/* Only add if visible */
	if (psDel->visible[selectedPlayer])
	{
		Vector3i pos;
		int      i;

		/* Set off some explosions, but not for walls */
		/* First Explosions */
		widthScatter = TILE_UNITS;
		breadthScatter = TILE_UNITS;
		heightScatter = TILE_UNITS;
		for (i = 0; i < (bMinor ? 2 : 4); ++i)  // only add two for walls - gets crazy otherwise
		{
			pos.x = psDel->pos.x + widthScatter - rand() % (2 * widthScatter);
			pos.z = psDel->pos.y + breadthScatter - rand() % (2 * breadthScatter);
			pos.y = psDel->pos.z + 32 + rand() % heightScatter;
			addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_MEDIUM, false, nullptr, 0, impactTime);
		}

		/* Get coordinates for everybody! */
		pos.x = psDel->pos.x;
		pos.z = psDel->pos.y;  // z = y [sic] intentional
		pos.y = map_Height(pos.x, pos.z);

		// Set off a fire, provide dimensions for the fire
		if (bMinor)
		{
			effectGiveAuxVar(world_coord(psDel->pStructureType->baseWidth) / 4);
		}
		else
		{
			effectGiveAuxVar(world_coord(psDel->pStructureType->baseWidth) / 3);
		}
		/* Give a duration */
		effectGiveAuxVarSec(burnDuration);
		if (bDerrick)  // oil resources
		{
			/* Oil resources burn AND puff out smoke AND for longer*/
			addEffect(&pos, EFFECT_FIRE, FIRE_TYPE_SMOKY, false, nullptr, 0, impactTime);
		}
		else  // everything else
		{
			addEffect(&pos, EFFECT_FIRE, FIRE_TYPE_LOCALISED, false, nullptr, 0, impactTime);
		}

		/* Power stations have their own destruction sequence */
		if (bPowerGen)
		{
			addEffect(&pos, EFFECT_DESTRUCTION, DESTRUCTION_TYPE_POWER_STATION, false, nullptr, 0, impactTime);
			pos.y += SHOCK_WAVE_HEIGHT;
			addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_SHOCKWAVE, false, nullptr, 0, impactTime);
		}
		/* As do wall sections */
		else if (bMinor)
		{
			addEffect(&pos, EFFECT_DESTRUCTION, DESTRUCTION_TYPE_WALL_SECTION, false, nullptr, 0, impactTime);
		}
		else // and everything else goes here.....
		{
			addEffect(&pos, EFFECT_DESTRUCTION, DESTRUCTION_TYPE_STRUCTURE, false, nullptr, 0, impactTime);
		}

		// and add a sound effect
		audio_PlayStaticTrack(psDel->pos.x, psDel->pos.y, ID_SOUND_EXPLOSION);
	}

	// Actually set the tiles on fire - even if the effect is not visible.
	tileSetFire(psDel->pos.x, psDel->pos.y, burnDuration);

	const bool resourceFound = removeStruct(psDel, true);
	psDel->died = impactTime;

	// Leave burn marks in the ground where building once stood
	if (psDel->visible[selectedPlayer] && !resourceFound && !bMinor)
	{
		StructureBounds b = getStructureBounds(psDel);
		for (int breadth = 0; breadth < b.size.y; ++breadth)
		{
			for (int width = 0; width < b.size.x; ++width)
			{
				MAPTILE *psTile = mapTile(b.map.x + width, b.map.y + breadth);
				if (TEST_TILE_VISIBLE(selectedPlayer, psTile))
				{
					psTile->illumination /= 2;
				}
			}
		}
	}

	// updates score stats only if not wall
	if (!bMinor)
	{
		if (psDel->player == selectedPlayer)
		{
			scoreUpdateVar(WD_STR_LOST);
		}
		else
		{
			scoreUpdateVar(WD_STR_KILLED);
		}
	}

	return true;
}


/* gets a structure stat from its name - relies on the name being unique (or it will
return the first one it finds!! */
int32_t getStructStatFromName(char const *pName)
{
	BASE_STATS *psStat = getCompStatsFromName(pName);
	if (psStat)
	{
		return psStat->index;
	}
	return -1;
}


/*check to see if the structure is 'doing' anything  - return true if idle*/
bool  structureIdle(STRUCTURE *psBuilding)
{
	BASE_STATS		*pSubject = nullptr;

	CHECK_STRUCTURE(psBuilding);

	if (psBuilding->pFunctionality == nullptr)
	{
		return true;
	}

	//determine the Subject
	switch (psBuilding->pStructureType->type)
	{
	case REF_RESEARCH:
		{
			pSubject = psBuilding->pFunctionality->researchFacility.psSubject;
			break;
		}
	case REF_FACTORY:
	case REF_CYBORG_FACTORY:
	case REF_VTOL_FACTORY:
		{
			pSubject = psBuilding->pFunctionality->factory.psSubject;
			break;
		}
	default:
		break;
	}

	if (pSubject != nullptr)
	{
		return false;
	}

	return true;
}


/*checks to see if any structure exists of a specified type with a specified status */
bool checkStructureStatus(STRUCTURE_STATS *psStats, UDWORD player, UDWORD status)
{
	STRUCTURE	*psStructure;
	bool		found = false;

	for (psStructure = apsStructLists[player]; psStructure != nullptr;
	     psStructure = psStructure->psNext)
	{
		if (psStructure->pStructureType->type == psStats->type)
		{
			//need to check if THIS instance of the type has the correct status
			if (psStructure->status == status)
			{
				found = true;
				break;
			}
		}
	}
	return found;
}


/*checks to see if a specific structure type exists -as opposed to a structure
stat type*/
bool checkSpecificStructExists(UDWORD structInc, UDWORD player)
{
	STRUCTURE	*psStructure;
	bool		found = false;

	ASSERT_OR_RETURN(false, structInc < numStructureStats, "Invalid structure inc");

	for (psStructure = apsStructLists[player]; psStructure != nullptr;
	     psStructure = psStructure->psNext)
	{
		if (psStructure->status == SS_BUILT)
		{
			if ((psStructure->pStructureType->ref - REF_STRUCTURE_START) ==
			    structInc)
			{
				found = true;
				break;
			}
		}
	}
	return found;
}


/*finds a suitable position for the assembly point based on one passed in*/
void findAssemblyPointPosition(UDWORD *pX, UDWORD *pY, UDWORD player)
{
	//set up a dummy stat pointer
	STRUCTURE_STATS     sStats;
	UDWORD              passes = 0;
	SDWORD	            i, j, startX, endX, startY, endY;

	sStats.ref = 0;
	sStats.baseWidth = 1;
	sStats.baseBreadth = 1;

	/* Initial box dimensions and set iteration count to zero */
	startX = endX = *pX;	startY = endY = *pY;
	passes = 0;

	//if the value passed in is not a valid location - find one!
	if (!validLocation(&sStats, world_coord(Vector2i(*pX, *pY)), 0, player, false))
	{
		/* Keep going until we get a tile or we exceed distance */
		while (passes < LOOK_FOR_EMPTY_TILE)
		{
			/* Process whole box */
			for (i = startX; i <= endX; i++)
			{
				for (j = startY; j <= endY; j++)
				{
					/* Test only perimeter as internal tested previous iteration */
					if (i == startX || i == endX || j == startY || j == endY)
					{
						/* Good enough? */
						if (validLocation(&sStats, world_coord(Vector2i(i, j)), 0, player, false))
						{
							/* Set exit conditions and get out NOW */
							*pX = i;
							*pY = j;
							//jump out of the loop
							return;
						}
					}
				}
			}
			/* Expand the box out in all directions - off map handled by validLocation() */
			startX--; startY--;	endX++;	endY++;	passes++;
		}
	}
	else
	{
		//the first location was valid
		return;
	}
	/* If we got this far, then we failed - passed in values will be unchanged */
	ASSERT(!"unable to find a valid location", "unable to find a valid location!");
}


/*sets the point new droids go to - x/y in world coords for a Factory
bCheck is set to true for initial placement of the Assembly Point*/
void setAssemblyPoint(FLAG_POSITION *psAssemblyPoint, UDWORD x, UDWORD y,
                      UDWORD player, bool bCheck)
{
	ASSERT_OR_RETURN(, psAssemblyPoint != nullptr, "invalid AssemblyPoint pointer");

	//check its valid
	x = map_coord(x);
	y = map_coord(y);
	if (bCheck)
	{
		findAssemblyPointPosition(&x, &y, player);
	}
	//add half a tile so the centre is in the middle of the tile
	x = world_coord(x) + TILE_UNITS / 2;
	y = world_coord(y) + TILE_UNITS / 2;

	psAssemblyPoint->coords.x = x;
	psAssemblyPoint->coords.y = y;

	// Deliv Point sits at the height of the tile it's centre is on + arbitrary amount!
	psAssemblyPoint->coords.z = map_Height(x, y) + ASSEMBLY_POINT_Z_PADDING;
}


/*sets the factory Inc for the Assembly Point*/
void setFlagPositionInc(FUNCTIONALITY *pFunctionality, UDWORD player, UBYTE factoryType)
{
	ASSERT_OR_RETURN(, player < MAX_PLAYERS, "invalid player number");

	//find the first vacant slot
	unsigned inc = std::find(factoryNumFlag[player][factoryType].begin(), factoryNumFlag[player][factoryType].end(), false) - factoryNumFlag[player][factoryType].begin();
	if (inc == factoryNumFlag[player][factoryType].size())
	{
		// first time init for this factory flag slot, set it to false
		factoryNumFlag[player][factoryType].push_back(false);
	}

	if (factoryType == REPAIR_FLAG)
	{
		// this is a special case, there are no flag numbers for this "factory"
		REPAIR_FACILITY *psRepair = &pFunctionality->repairFacility;
		psRepair->psDeliveryPoint->factoryInc = 0;
		psRepair->psDeliveryPoint->factoryType = factoryType;
		// factoryNumFlag[player][factoryType][inc] = true;
	}
	else
	{
		FACTORY *psFactory = &pFunctionality->factory;
		psFactory->psAssemblyPoint->factoryInc = inc;
		psFactory->psAssemblyPoint->factoryType = factoryType;
		factoryNumFlag[player][factoryType][inc] = true;
	}
}

/*called when a structure has been built - checks through the list of callbacks
for the scripts*/
static void structureCompletedCallback(STRUCTURE_STATS *psStructType)
{

	if (psStructType->type == REF_POWER_GEN)
	{
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_POWERGEN_BUILT);
	}
	if (psStructType->type == REF_RESOURCE_EXTRACTOR)
	{
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_RESEX_BUILT);
	}
	if (psStructType->type == REF_RESEARCH)
	{
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_RESEARCH_BUILT);
	}
	if (psStructType->type == REF_FACTORY ||
	    psStructType->type == REF_CYBORG_FACTORY ||
	    psStructType->type == REF_VTOL_FACTORY)
	{
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_FACTORY_BUILT);
	}
}


STRUCTURE_STATS *structGetDemolishStat()
{
	ASSERT_OR_RETURN(nullptr, g_psStatDestroyStruct != nullptr , "Demolish stat not initialised");
	return g_psStatDestroyStruct;
}


/*sets the flag to indicate a SatUplink Exists - so draw everything!*/
void setSatUplinkExists(bool state, UDWORD player)
{
	satUplinkExists[player] = (UBYTE)state;
	if (state)
	{
		satuplinkbits |= (1 << player);
	}
	else
	{
		satuplinkbits &= ~(1 << player);
	}
}


/*returns the status of the flag*/
bool getSatUplinkExists(UDWORD player)
{
	return satUplinkExists[player];
}


/*sets the flag to indicate a Las Sat Exists - ONLY EVER WANT ONE*/
void setLasSatExists(bool state, UDWORD player)
{
	lasSatExists[player] = (UBYTE)state;
}


/*returns the status of the flag*/
bool getLasSatExists(UDWORD player)
{
	return lasSatExists[player];
}


/* calculate muzzle base location in 3d world */
bool calcStructureMuzzleBaseLocation(STRUCTURE *psStructure, Vector3i *muzzle, int weapon_slot)
{
	iIMDShape *psShape = psStructure->pStructureType->pIMD[0];

	CHECK_STRUCTURE(psStructure);

	if (psShape && psShape->nconnectors)
	{
		Vector3i barrel(0, 0, 0);

		Affine3F af;

		af.Trans(psStructure->pos.x, -psStructure->pos.z, psStructure->pos.y);

		//matrix = the center of droid
		af.RotY(psStructure->rot.direction);
		af.RotX(psStructure->rot.pitch);
		af.RotZ(-psStructure->rot.roll);
		af.Trans(psShape->connectors[weapon_slot].x, -psShape->connectors[weapon_slot].z,
		         -psShape->connectors[weapon_slot].y);//note y and z flipped


		*muzzle = (af * barrel).xzy;
		muzzle->z = -muzzle->z;
	}
	else
	{
		*muzzle = psStructure->pos + Vector3i(0, 0, psStructure->sDisplay.imd->max.y);
	}

	return true;
}

/* calculate muzzle tip location in 3d world */
bool calcStructureMuzzleLocation(STRUCTURE *psStructure, Vector3i *muzzle, int weapon_slot)
{
	iIMDShape *psShape = psStructure->pStructureType->pIMD[0];

	CHECK_STRUCTURE(psStructure);

	if (psShape && psShape->nconnectors)
	{
		Vector3i barrel(0, 0, 0);
		unsigned int nWeaponStat = psStructure->asWeaps[weapon_slot].nStat;
		iIMDShape *psWeaponImd = nullptr, *psMountImd = nullptr;

		if (nWeaponStat)
		{
			psWeaponImd = asWeaponStats[nWeaponStat].pIMD;
			psMountImd = asWeaponStats[nWeaponStat].pMountGraphic;
		}

		Affine3F af;

		af.Trans(psStructure->pos.x, -psStructure->pos.z, psStructure->pos.y);

		//matrix = the center of droid
		af.RotY(psStructure->rot.direction);
		af.RotX(psStructure->rot.pitch);
		af.RotZ(-psStructure->rot.roll);
		af.Trans(psShape->connectors[weapon_slot].x, -psShape->connectors[weapon_slot].z,
		         -psShape->connectors[weapon_slot].y);//note y and z flipped

		//matrix = the weapon[slot] mount on the body
		af.RotY(psStructure->asWeaps[weapon_slot].rot.direction);  // +ve anticlockwise

		// process turret mount
		if (psMountImd && psMountImd->nconnectors)
		{
			af.Trans(psMountImd->connectors->x, -psMountImd->connectors->z, -psMountImd->connectors->y);
		}

		//matrix = the turret connector for the gun
		af.RotX(psStructure->asWeaps[weapon_slot].rot.pitch);      // +ve up

		//process the gun
		if (psWeaponImd && psWeaponImd->nconnectors)
		{
			unsigned int connector_num = 0;

			// which barrel is firing if model have multiple muzzle connectors?
			if (psStructure->asWeaps[weapon_slot].shotsFired && (psWeaponImd->nconnectors > 1))
			{
				// shoot first, draw later - substract one shot to get correct results
				connector_num = (psStructure->asWeaps[weapon_slot].shotsFired - 1) % (psWeaponImd->nconnectors);
			}

			barrel = Vector3i(psWeaponImd->connectors[connector_num].x, -psWeaponImd->connectors[connector_num].z, -psWeaponImd->connectors[connector_num].y);
		}

		*muzzle = (af * barrel).xzy;
		muzzle->z = -muzzle->z;
	}
	else
	{
		*muzzle = psStructure->pos + Vector3i(0, 0, 0 + psStructure->sDisplay.imd->max.y);
	}

	return true;
}


/*Looks through the list of structures to see if there are any inactive
resource extractors*/
void checkForResExtractors(STRUCTURE *psBuilding)
{
	ASSERT_OR_RETURN(, psBuilding->pStructureType->type == REF_POWER_GEN, "invalid structure type");

	// Find derricks, sorted by unused first, then ones attached to power generators without modules.
	typedef std::pair<int, STRUCTURE *> Derrick;
	typedef std::vector<Derrick> Derricks;
	Derricks derricks;
	derricks.reserve(NUM_POWER_MODULES + 1);
	for (STRUCTURE *currExtractor = apsExtractorLists[psBuilding->player]; currExtractor != nullptr; currExtractor = currExtractor->psNextFunc)
	{
		RES_EXTRACTOR *resExtractor = &currExtractor->pFunctionality->resourceExtractor;

		if (currExtractor->status != SS_BUILT)
		{
			continue;  // Derrick not complete.
		}
		int priority = resExtractor->psPowerGen != nullptr ? resExtractor->psPowerGen->capacity : -1;
		//auto d = std::find_if(derricks.begin(), derricks.end(), [priority](Derrick const &v) { return v.first <= priority; });
		Derricks::iterator d = derricks.begin();
		while (d != derricks.end() && d->first <= priority)
		{
			++d;
		}
		derricks.insert(d, Derrick(priority, currExtractor));
		derricks.resize(std::min<unsigned>(derricks.size(), NUM_POWER_MODULES));  // No point remembering more derricks than this.
	}

	// Attach derricks.
	Derricks::const_iterator d = derricks.begin();
	for (int i = 0; i < NUM_POWER_MODULES; ++i)
	{
		POWER_GEN *powerGen = &psBuilding->pFunctionality->powerGenerator;
		if (powerGen->apResExtractors[i] != nullptr)
		{
			continue;  // Slot full.
		}

		int priority = psBuilding->capacity;
		if (d == derricks.end() || d->first >= priority)
		{
			continue;  // No more derricks to transfer to this power generator.
		}

		STRUCTURE *derrick = d->second;
		RES_EXTRACTOR *resExtractor = &derrick->pFunctionality->resourceExtractor;
		if (resExtractor->psPowerGen != nullptr)
		{
			informPowerGen(derrick);  // Remove the derrick from the previous power generator.
		}
		// Assign the derrick to the power generator.
		powerGen->apResExtractors[i] = derrick;
		resExtractor->psPowerGen = psBuilding;

		++d;
	}
}


/*Looks through the list of structures to see if there are any Power Gens
with available slots for the new Res Ext*/
void checkForPowerGen(STRUCTURE *psBuilding)
{
	ASSERT_OR_RETURN(, psBuilding->pStructureType->type == REF_RESOURCE_EXTRACTOR, "invalid structure type");

	RES_EXTRACTOR *psRE = &psBuilding->pFunctionality->resourceExtractor;
	if (psRE->psPowerGen != nullptr)
	{
		return;
	}

	// Find a power generator, if possible with a power module.
	STRUCTURE *bestPowerGen = nullptr;
	int bestSlot = 0;
	for (STRUCTURE *psCurr = apsStructLists[psBuilding->player]; psCurr != nullptr; psCurr = psCurr->psNext)
	{
		if (psCurr->pStructureType->type == REF_POWER_GEN && psCurr->status == SS_BUILT)
		{
			if (bestPowerGen != nullptr && bestPowerGen->capacity >= psCurr->capacity)
			{
				continue;  // Power generator not better.
			}

			POWER_GEN *psPG = &psCurr->pFunctionality->powerGenerator;
			for (int i = 0; i < NUM_POWER_MODULES; ++i)
			{
				if (psPG->apResExtractors[i] == nullptr)
				{
					bestPowerGen = psCurr;
					bestSlot = i;
					break;
				}
			}
		}
	}

	if (bestPowerGen != nullptr)
	{
		// Attach the derrick to the power generator.
		POWER_GEN *psPG = &bestPowerGen->pFunctionality->powerGenerator;
		psPG->apResExtractors[bestSlot] = psBuilding;
		psRE->psPowerGen = bestPowerGen;
	}
}


/*initialise the slot the Resource Extractor filled in the owning Power Gen*/
void informPowerGen(STRUCTURE *psStruct)
{
	UDWORD		i;
	POWER_GEN	*psPowerGen;

	if (psStruct->pStructureType->type != REF_RESOURCE_EXTRACTOR)
	{
		ASSERT(!"invalid structure type", "invalid structure type");
		return;
	}

	//get the owning power generator
	psPowerGen = &psStruct->pFunctionality->resourceExtractor.psPowerGen->pFunctionality->powerGenerator;
	if (psPowerGen)
	{
		for (i = 0; i < NUM_POWER_MODULES; i++)
		{
			if (psPowerGen->apResExtractors[i] == psStruct)
			{
				//initialise the 'slot'
				psPowerGen->apResExtractors[i] = nullptr;
				break;
			}
		}
	}
}


/*called when a Res extractor is destroyed or runs out of power or is disconnected
adjusts the owning Power Gen so that it can link to a different Res Extractor if one
is available*/
void releaseResExtractor(STRUCTURE *psRelease)
{
	STRUCTURE	*psCurr;

	if (psRelease->pStructureType->type != REF_RESOURCE_EXTRACTOR)
	{
		ASSERT(!"invalid structure type", "Invalid structure type");
		return;
	}

	//tell associated Power Gen
	if (psRelease->pFunctionality->resourceExtractor.psPowerGen)
	{
		informPowerGen(psRelease);
	}

	psRelease->pFunctionality->resourceExtractor.psPowerGen = nullptr;

	//there may be spare resource extractors
	for (psCurr = apsExtractorLists[psRelease->player]; psCurr != nullptr; psCurr = psCurr->psNextFunc)
	{
		//check not connected and power left and built!
		if (psCurr != psRelease && psCurr->pFunctionality->resourceExtractor.psPowerGen == nullptr && psCurr->status == SS_BUILT)
		{
			checkForPowerGen(psCurr);
		}
	}
}


/*called when a Power Gen is destroyed or is disconnected
adjusts the associated Res Extractors so that they can link to different Power
Gens if any are available*/
void releasePowerGen(STRUCTURE *psRelease)
{
	STRUCTURE	*psCurr;
	POWER_GEN	*psPowerGen;
	UDWORD		i;

	if (psRelease->pStructureType->type != REF_POWER_GEN)
	{
		ASSERT(!"invalid structure type", "Invalid structure type");
		return;
	}

	psPowerGen = &psRelease->pFunctionality->powerGenerator;
	//go through list of res extractors, setting them to inactive
	for (i = 0; i < NUM_POWER_MODULES; i++)
	{
		if (psPowerGen->apResExtractors[i])
		{
			psPowerGen->apResExtractors[i]->pFunctionality->resourceExtractor.psPowerGen = nullptr;
			psPowerGen->apResExtractors[i] = nullptr;
		}
	}
	//may have a power gen with spare capacity
	for (psCurr = apsStructLists[psRelease->player]; psCurr != nullptr; psCurr =
	         psCurr->psNext)
	{
		if (psCurr->pStructureType->type == REF_POWER_GEN &&
		    psCurr != psRelease && psCurr->status == SS_BUILT)
		{
			checkForResExtractors(psCurr);
		}
	}
}


/*this is called whenever a structure has finished building*/
void buildingComplete(STRUCTURE *psBuilding)
{
	CHECK_STRUCTURE(psBuilding);

	int prevState = 0;
	if (psBuilding->pStructureType->type == REF_RESEARCH)
	{
		prevState = intGetResearchState();
	}

	psBuilding->currentBuildPts = psBuilding->pStructureType->buildPoints;
	psBuilding->status = SS_BUILT;

	visTilesUpdate(psBuilding);

	if (psBuilding->prebuiltImd != nullptr)
	{
		// We finished building a module, now use the combined IMD.
		std::vector<iIMDShape *> &IMDs = psBuilding->pStructureType->pIMD;
		int imdIndex = std::min<int>(numStructureModules(psBuilding) * 2, IMDs.size() - 1); // *2 because even-numbered IMDs are structures, odd-numbered IMDs are just the modules.
		psBuilding->prebuiltImd = nullptr;
		psBuilding->sDisplay.imd = IMDs[imdIndex];
	}

	switch (psBuilding->pStructureType->type)
	{
	case REF_POWER_GEN:
		checkForResExtractors(psBuilding);
		if (selectedPlayer == psBuilding->player)
		{
			audio_PlayObjStaticTrack(psBuilding, ID_SOUND_POWER_HUM);
		}
		break;
	case REF_RESOURCE_EXTRACTOR:
		checkForPowerGen(psBuilding);
		break;
	case REF_RESEARCH:
		//this deals with research facilities that are upgraded whilst mid-research
		releaseResearch(psBuilding, ModeImmediate);
		intNotifyResearchButton(prevState);
		break;
	case REF_FACTORY:
	case REF_CYBORG_FACTORY:
	case REF_VTOL_FACTORY:
		//this deals with factories that are upgraded whilst mid-production
		releaseProduction(psBuilding, ModeImmediate);
		break;
	case REF_SAT_UPLINK:
		revealAll(psBuilding->player);
		break;
	case REF_GATE:
		auxStructureNonblocking(psBuilding);  // Clear outdated flags.
		auxStructureClosedGate(psBuilding);  // Don't block for the sake of allied pathfinding.
		break;
	default:
		//do nothing
		break;
	}
}


/*for a given structure, return a pointer to its module stat */
STRUCTURE_STATS *getModuleStat(const STRUCTURE *psStruct)
{
	ASSERT_OR_RETURN(nullptr, psStruct != nullptr, "Invalid structure pointer");

	switch (psStruct->pStructureType->type)
	{
	case REF_POWER_GEN:
		return &asStructureStats[powerModuleStat];
	case REF_FACTORY:
	case REF_VTOL_FACTORY:
		return &asStructureStats[factoryModuleStat];
	case REF_RESEARCH:
		return &asStructureStats[researchModuleStat];
	default:
		//no other structures can have modules attached
		return nullptr;
	}
}

/**
 * Count the artillery and VTOL droids assigned to a structure.
 */
static unsigned int countAssignedDroids(const STRUCTURE *psStructure)
{
	const DROID *psCurr;
	unsigned int num;

	CHECK_STRUCTURE(psStructure);

	// For non-debug builds
	if (psStructure == nullptr)
	{
		return 0;
	}

	num = 0;
	for (psCurr = apsDroidLists[selectedPlayer]; psCurr; psCurr = psCurr->psNext)
	{
		if (psCurr->order.psObj
		    && psCurr->order.psObj->id == psStructure->id
		    && psCurr->player == psStructure->player)
		{
			const MOVEMENT_MODEL weapontype = asWeaponStats[psCurr->asWeaps[0].nStat].movementModel;

			if (weapontype == MM_INDIRECT
			    || weapontype == MM_HOMINGINDIRECT
			    || isVtolDroid(psCurr))
			{
				num++;
			}
		}
	}

	return num;
}

//print some info at the top of the screen dependent on the structure
void printStructureInfo(STRUCTURE *psStructure)
{
	unsigned int numConnected;
	POWER_GEN	*psPowerGen;

	ASSERT_OR_RETURN(, psStructure != nullptr, "Invalid Structure pointer");

	if (isBlueprint(psStructure))
	{
		return;  // Don't print anything about imaginary structures. Would crash, anyway.
	}

	switch (psStructure->pStructureType->type)
	{
	case REF_HQ:
		{
			unsigned int assigned_droids = countAssignedDroids(psStructure);
			console(ngettext("%s - %u Unit assigned - Hitpoints %d/%d", "%s - %u Units assigned - Hitpoints %d/%d", assigned_droids),
			        getName(psStructure->pStructureType), assigned_droids, psStructure->body, structureBody(psStructure));
			if (getDebugMappingStatus())
			{
				console("ID %d - sensor range %d - ECM %d", psStructure->id, structSensorRange(psStructure), structJammerPower(psStructure));
			}
			break;
		}
	case REF_DEFENSE:
		if (psStructure->pStructureType->pSensor != nullptr
		    && (psStructure->pStructureType->pSensor->type == STANDARD_SENSOR
		        || psStructure->pStructureType->pSensor->type == INDIRECT_CB_SENSOR
		        || psStructure->pStructureType->pSensor->type == VTOL_INTERCEPT_SENSOR
		        || psStructure->pStructureType->pSensor->type == VTOL_CB_SENSOR
		        || psStructure->pStructureType->pSensor->type == SUPER_SENSOR
		        || psStructure->pStructureType->pSensor->type == RADAR_DETECTOR_SENSOR)
		    && psStructure->pStructureType->pSensor->location == LOC_TURRET)
		{
			unsigned int assigned_droids = countAssignedDroids(psStructure);
			console(ngettext("%s - %u Unit assigned - Damage %3.0f%%", "%s - %u Units assigned - Hitpoints %d/%d", assigned_droids),
			        getName(psStructure->pStructureType), assigned_droids, psStructure->body, structureBody(psStructure));
		}
		else
		{
			console(_("%s - Hitpoints %d/%d"), getName(psStructure->pStructureType), psStructure->body, structureBody(psStructure));
		}
		if (getDebugMappingStatus())
		{
			console("ID %d - armour %d|%d - sensor range %d - ECM %d - born %u - depth %.02f",
			        psStructure->id, objArmour(psStructure, WC_KINETIC), objArmour(psStructure, WC_HEAT),
			        structSensorRange(psStructure), structJammerPower(psStructure), psStructure->born, psStructure->foundationDepth);
		}
		break;
	case REF_REPAIR_FACILITY:
		console(_("%s - Hitpoints %d/%d"), getName(psStructure->pStructureType), psStructure->body, structureBody(psStructure));
		if (getDebugMappingStatus())
		{
			console("ID %d - Queue %d", psStructure->id, psStructure->pFunctionality->repairFacility.droidQueue);
		}
		break;
	case REF_RESOURCE_EXTRACTOR:
		console(_("%s - Hitpoints %d/%d"), getName(psStructure->pStructureType), psStructure->body, structureBody(psStructure));
		if (getDebugMappingStatus())
		{
			console("ID %d - %s", psStructure->id, (auxTile(map_coord(psStructure->pos.x), map_coord(psStructure->pos.y), selectedPlayer) & AUXBITS_DANGER) ? "danger" : "safe");
		}
		break;
	case REF_POWER_GEN:
		psPowerGen = &psStructure->pFunctionality->powerGenerator;
		numConnected = 0;
		for (int i = 0; i < NUM_POWER_MODULES; i++)
		{
			if (psPowerGen->apResExtractors[i])
			{
				numConnected++;
			}
		}
		console(_("%s - Connected %u of %u - Hitpoints %d/%d"), getName(psStructure->pStructureType), numConnected,
		        NUM_POWER_MODULES, psStructure->body, structureBody(psStructure));
		if (getDebugMappingStatus())
		{
			console("ID %u - Multiplier: %u", psStructure->id, getBuildingPowerPoints(psStructure));
		}
		break;
	case REF_CYBORG_FACTORY:
	case REF_VTOL_FACTORY:
	case REF_FACTORY:
		console(_("%s - Hitpoints %d/%d"), getName(psStructure->pStructureType), psStructure->body, structureBody(psStructure));
		if (getDebugMappingStatus())
		{
			console("ID %u - Production Output: %u - BuildPointsRemaining: %u - Resistance: %d / %d", psStructure->id,
			        getBuildingProductionPoints(psStructure), psStructure->pFunctionality->factory.buildPointsRemaining,
			        psStructure->resistance, structureResistance(psStructure->pStructureType, psStructure->player));
		}
		break;
	case REF_RESEARCH:
		console(_("%s - Hitpoints %d/%d"), getName(psStructure->pStructureType), psStructure->body, structureBody(psStructure));
		if (getDebugMappingStatus())
		{
			console("ID %u - Research Points: %u", psStructure->id, getBuildingResearchPoints(psStructure));
		}
		break;
	case REF_REARM_PAD:
		console(_("%s - Hitpoints %d/%d"), getName(psStructure->pStructureType), psStructure->body, structureBody(psStructure));
		if (getDebugMappingStatus())
		{
			console("tile %d,%d - target %s", psStructure->pos.x / TILE_UNITS, psStructure->pos.y / TILE_UNITS,
			        objInfo(psStructure->pFunctionality->rearmPad.psObj));
		}
		break;
	default:
		console(_("%s - Hitpoints %d/%d"), getName(psStructure->pStructureType), psStructure->body, structureBody(psStructure));
		if (getDebugMappingStatus())
		{
			console("ID %u - sensor range %d - ECM %d", psStructure->id, structSensorRange(psStructure), structJammerPower(psStructure));
		}
		break;
	}
}


/*Checks the template type against the factory type - returns false
if not a good combination!*/
bool validTemplateForFactory(const DROID_TEMPLATE *psTemplate, STRUCTURE *psFactory, bool complain)
{
	ASSERT_OR_RETURN(false, psTemplate, "Invalid template!");
	enum code_part level = complain ? LOG_ERROR : LOG_NEVER;

	//not in multiPlayer! - AB 26/5/99
	//ignore Transporter Droids
	if (!bMultiPlayer && isTransporter(psTemplate))
	{
		debug(level, "Cannot build transporter in campaign.");
		return false;
	}

	//check if droid is a cyborg
	if (psTemplate->droidType == DROID_CYBORG ||
	    psTemplate->droidType == DROID_CYBORG_SUPER ||
	    psTemplate->droidType == DROID_CYBORG_CONSTRUCT ||
	    psTemplate->droidType == DROID_CYBORG_REPAIR)
	{
		if (psFactory->pStructureType->type != REF_CYBORG_FACTORY)
		{
			debug(level, "Cannot build cyborg except in cyborg factory, not in %s.", objInfo(psFactory));
			return false;
		}
	}
	//check for VTOL droid
	else if (psTemplate->asParts[COMP_PROPULSION] &&
	         ((asPropulsionStats + psTemplate->asParts[COMP_PROPULSION])->propulsionType == PROPULSION_TYPE_LIFT))
	{
		if (psFactory->pStructureType->type != REF_VTOL_FACTORY)
		{
			debug(level, "Cannot build vtol except in vtol factory, not in %s.", objInfo(psFactory));
			return false;
		}
	}

	//check if cyborg factory
	if (psFactory->pStructureType->type == REF_CYBORG_FACTORY)
	{
		if (!(psTemplate->droidType == DROID_CYBORG ||
		      psTemplate->droidType == DROID_CYBORG_SUPER ||
		      psTemplate->droidType == DROID_CYBORG_CONSTRUCT ||
		      psTemplate->droidType == DROID_CYBORG_REPAIR))
		{
			debug(level, "Can only build cyborg in cyborg factory, not droidType %d in %s.",
			      psTemplate->droidType, objInfo(psFactory));
			return false;
		}
	}
	//check if vtol factory
	else if (psFactory->pStructureType->type == REF_VTOL_FACTORY)
	{
		if (!psTemplate->asParts[COMP_PROPULSION] ||
		    ((asPropulsionStats + psTemplate->asParts[COMP_PROPULSION])->propulsionType != PROPULSION_TYPE_LIFT))
		{
			debug(level, "Can only build vtol in vtol factory, not in %s.", objInfo(psFactory));
			return false;
		}
	}

	//got through all the tests...
	return true;
}

/*calculates the damage caused to the resistance levels of structures - returns
true when captured*/
bool electronicDamage(BASE_OBJECT *psTarget, UDWORD damage, UBYTE attackPlayer)
{
	STRUCTURE   *psStructure;
	DROID       *psDroid;
	bool        bCompleted = true;
	Vector3i	pos;

	ASSERT_OR_RETURN(false, attackPlayer < MAX_PLAYERS, "Invalid player id %d", (int)attackPlayer);
	ASSERT_OR_RETURN(false, psTarget != nullptr, "Target is NULL");

	//structure electronic damage
	if (psTarget->type == OBJ_STRUCTURE)
	{
		psStructure = (STRUCTURE *)psTarget;
		bCompleted = false;

		if (psStructure->pStructureType->upgrade[psStructure->player].resistance == 0)
		{
			return false;	// this structure type cannot be taken over
		}

		//if resistance is already less than 0 don't do any more
		if (psStructure->resistance < 0)
		{
			bCompleted = true;
		}
		else
		{
			//store the time it was hit
			int lastHit = psStructure->timeLastHit;
			psStructure->timeLastHit = gameTime;

			psStructure->lastHitWeapon = WSC_ELECTRONIC;

			triggerEventAttacked(psStructure, g_pProjLastAttacker, lastHit);

			psStructure->resistance = (SWORD)(psStructure->resistance - damage);

			if (psStructure->resistance < 0)
			{
				//add a console message for the selected Player
				if (psStructure->player == selectedPlayer)
				{
					console(_("%s - Electronically Damaged"),
					        getName(psStructure->pStructureType));
					//tell the scripts if selectedPlayer has lost a structure
					eventFireCallbackTrigger((TRIGGER_TYPE)CALL_ELECTRONIC_TAKEOVER);
				}
				bCompleted = true;
				//give the structure to the attacking player
				(void)giftSingleStructure(psStructure, attackPlayer);
			}
		}
	}
	//droid electronic damage
	else if (psTarget->type == OBJ_DROID)
	{
		psDroid = (DROID *)psTarget;
		bCompleted = false;
		int lastHit = psDroid->timeLastHit;

		//in multiPlayer cannot attack a Transporter with EW
		if (bMultiPlayer)
		{
			ASSERT_OR_RETURN(true, !isTransporter(psDroid), "Cannot attack a Transporter in multiPlayer");
		}

		if (psDroid->resistance == ACTION_START_TIME)
		{
			//need to set the current resistance level since not been previously attacked (by EW)
			psDroid->resistance = droidResistance(psDroid);
		}

		if (psDroid->resistance < 0)
		{
			bCompleted = true;
		}
		else
		{
			triggerEventAttacked(psDroid, g_pProjLastAttacker, lastHit);

			psDroid->resistance = (SWORD)(psDroid->resistance - damage);

			if (psDroid->resistance <= 0)
			{
				//add a console message for the selected Player
				if (psDroid->player == selectedPlayer)
				{
					console(_("%s - Electronically Damaged"), "Unit");
					//tell the scripts if selectedPlayer has lost a droid
					eventFireCallbackTrigger((TRIGGER_TYPE)CALL_ELECTRONIC_TAKEOVER);
				}
				bCompleted = true;

				//give the droid to the attacking player

				if (psDroid->visible[selectedPlayer])
				{
					for (int i = 0; i < 5; i++)
					{
						pos.x = psDroid->pos.x + (30 - rand() % 60);
						pos.z = psDroid->pos.y + (30 - rand() % 60);
						pos.y = psDroid->pos.z + (rand() % 8);
						effectGiveAuxVar(80);
						addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_FLAMETHROWER, false, nullptr, 0, gameTime - deltaGameTime);
					}
				}
				if (!giftSingleDroid(psDroid, attackPlayer) && !isDead(psDroid))
				{
					// droid limit reached, recycle
					// don't check for transporter/mission coz multiplayer only issue.
					recycleDroid(psDroid);
				}
			}
		}
	}

	return bCompleted;
}


/* EW works differently in multiplayer mode compared with single player.*/
bool validStructResistance(const STRUCTURE *psStruct)
{
	bool    bTarget = false;

	ASSERT_OR_RETURN(false, psStruct != nullptr, "Invalid structure pointer");

	if (psStruct->pStructureType->upgrade[psStruct->player].resistance != 0)
	{
		/*certain structures will only provide rewards in multiplayer so
		before they can become valid targets their resistance must be at least
		half the base value*/
		if (bMultiPlayer)
		{
			switch (psStruct->pStructureType->type)
			{
			case REF_RESEARCH:
			case REF_FACTORY:
			case REF_VTOL_FACTORY:
			case REF_CYBORG_FACTORY:
			case REF_HQ:
			case REF_REPAIR_FACILITY:
				if (psStruct->resistance >= structureResistance(psStruct->pStructureType, psStruct->player) / 2)
				{
					bTarget = true;
				}
				break;
			default:
				bTarget = true;
			}
		}
		else
		{
			bTarget = true;
		}
	}

	return bTarget;
}

unsigned structureBodyBuilt(const STRUCTURE *psStructure)
{
	unsigned maxBody = structureBody(psStructure);

	if (psStructure->status == SS_BEING_BUILT)
	{
		// Calculate the body points the structure would have, if not damaged.
		unsigned unbuiltBody = (maxBody + 9) / 10;  // See droidStartBuild() in droid.cpp.
		unsigned deltaBody = (uint64_t)9 * maxBody * psStructure->currentBuildPts / (10 * psStructure->pStructureType->buildPoints);  // See structureBuild() in structure.cpp.
		maxBody = unbuiltBody + deltaBody;
	}

	return maxBody;
}

/*Access functions for the upgradeable stats of a structure*/
UDWORD structureBody(const STRUCTURE *psStructure)
{
	return psStructure->pStructureType->upgrade[psStructure->player].hitpoints;
}

UDWORD	structureArmour(const STRUCTURE_STATS *psStats, UBYTE player)
{
	return psStats->upgrade[player].armour;
}

UDWORD	structureResistance(const STRUCTURE_STATS *psStats, UBYTE player)
{
	return psStats->upgrade[player].resistance;
}


/*gives the attacking player a reward based on the type of structure that has
been attacked*/
bool electronicReward(STRUCTURE *psStructure, UBYTE attackPlayer)
{
	bool    bRewarded = false;

	switch (psStructure->pStructureType->type)
	{
	case REF_RESEARCH:
		researchReward(psStructure->player, attackPlayer);
		bRewarded = true;
		break;
	case REF_FACTORY:
	case REF_VTOL_FACTORY:
	case REF_CYBORG_FACTORY:
		factoryReward(psStructure->player, attackPlayer);
		bRewarded = true;
		break;
	case REF_HQ:
		hqReward(psStructure->player, attackPlayer);
		if (attackPlayer == selectedPlayer)
		{
			addConsoleMessage(_("Electronic Reward - Visibility Report"),	DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
		}
		bRewarded = true;
		break;
	case REF_REPAIR_FACILITY:
		repairFacilityReward(psStructure->player, attackPlayer);
		bRewarded = true;
		break;
	default:
		bRewarded = false;
	}

	return bRewarded;
}


/*find the 'best' prop/body/weapon component the losing player has and
'give' it to the reward player*/
void factoryReward(UBYTE losingPlayer, UBYTE rewardPlayer)
{
	unsigned comp = 0;

	//search through the propulsions first
	for (unsigned inc = 0; inc < numPropulsionStats; inc++)
	{
		if (apCompLists[losingPlayer][COMP_PROPULSION][inc] == AVAILABLE &&
		    apCompLists[rewardPlayer][COMP_PROPULSION][inc] != AVAILABLE)
		{
			if (asPropulsionStats[inc].buildPower > asPropulsionStats[comp].buildPower)
			{
				comp = inc;
			}
		}
	}
	if (comp != 0)
	{
		apCompLists[rewardPlayer][COMP_PROPULSION][comp] = AVAILABLE;
		if (rewardPlayer == selectedPlayer)
		{
			console("%s :- %s", _("Factory Reward - Propulsion"), getName(&asPropulsionStats[comp]));
		}
		return;
	}

	//haven't found a propulsion - look for a body
	for (unsigned inc = 0; inc < numBodyStats; inc++)
	{
		if (apCompLists[losingPlayer][COMP_BODY][inc] == AVAILABLE &&
		    apCompLists[rewardPlayer][COMP_BODY][inc] != AVAILABLE)
		{
			if (asBodyStats[inc].buildPower > asBodyStats[comp].buildPower)
			{
				comp = inc;
			}
		}
	}
	if (comp != 0)
	{
		apCompLists[rewardPlayer][COMP_BODY][comp] = AVAILABLE;
		if (rewardPlayer == selectedPlayer)
		{
			console("%s :- %s", _("Factory Reward - Body"), getName(&asBodyStats[comp]));
		}
		return;
	}

	//haven't found a body - look for a weapon
	for (unsigned inc = 0; inc < numWeaponStats; inc++)
	{
		if (apCompLists[losingPlayer][COMP_WEAPON][inc] == AVAILABLE &&
		    apCompLists[rewardPlayer][COMP_WEAPON][inc] != AVAILABLE)
		{
			if (asWeaponStats[inc].buildPower > asWeaponStats[comp].buildPower)
			{
				comp = inc;
			}
		}
	}
	if (comp != 0)
	{
		apCompLists[rewardPlayer][COMP_WEAPON][comp] = AVAILABLE;
		if (rewardPlayer == selectedPlayer)
		{
			console("%s :- %s", _("Factory Reward - Weapon"), getName(&asWeaponStats[comp]));
		}
		return;
	}

	//losing Player hasn't got anything better so don't gain anything!
	if (rewardPlayer == selectedPlayer)
	{
		addConsoleMessage(_("Factory Reward - Nothing"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	}
}

/*find the 'best' repair component the losing player has and
'give' it to the reward player*/
void repairFacilityReward(UBYTE losingPlayer, UBYTE rewardPlayer)
{
	unsigned comp = 0;

	//search through the repair stats
	for (unsigned inc = 0; inc < numRepairStats; inc++)
	{
		if (apCompLists[losingPlayer][COMP_REPAIRUNIT][inc] == AVAILABLE &&
		    apCompLists[rewardPlayer][COMP_REPAIRUNIT][inc] != AVAILABLE)
		{
			if (asRepairStats[inc].buildPower > asRepairStats[comp].buildPower)
			{
				comp = inc;
			}
		}
	}
	if (comp != 0)
	{
		apCompLists[rewardPlayer][COMP_REPAIRUNIT][comp] = AVAILABLE;
		if (rewardPlayer == selectedPlayer)
		{
			console("%s :- %s", _("Repair Facility Award - Repair"), getName(&asRepairStats[comp]));
		}
		return;
	}
	if (rewardPlayer == selectedPlayer)
	{
		addConsoleMessage(_("Repair Facility Award - Nothing"), DEFAULT_JUSTIFY, SYSTEM_MESSAGE);
	}
}


/*makes the losing players tiles/structures/features visible to the reward player*/
void hqReward(UBYTE losingPlayer, UBYTE rewardPlayer)
{
	STRUCTURE *psStruct;
	FEATURE	  *psFeat;
	DROID	  *psDroid;
	UDWORD	x, y, i;

	// share exploration info - pretty useless but perhaps a nice touch?
	for (x = 0; x < mapWidth; x++)
	{
		for (y = 0; y < mapHeight; y++)
		{
			MAPTILE *psTile = mapTile(x, y);
			if (TEST_TILE_VISIBLE(losingPlayer, psTile))
			{
				psTile->tileExploredBits |= alliancebits[rewardPlayer];
			}
		}
	}

	//struct
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		for (psStruct = apsStructLists[i]; psStruct != nullptr; psStruct = psStruct->psNext)
		{
			if (psStruct->visible[losingPlayer] && !psStruct->died)
			{
				psStruct->visible[rewardPlayer] = psStruct->visible[losingPlayer];
			}
		}

		//feature
		for (psFeat = apsFeatureLists[i]; psFeat != nullptr; psFeat = psFeat->psNext)
		{
			if (psFeat->visible[losingPlayer])
			{
				psFeat->visible[rewardPlayer] = psFeat->visible[losingPlayer];
			}
		}

		//droids.
		for (psDroid = apsDroidLists[i]; psDroid != nullptr; psDroid = psDroid->psNext)
		{
			if (psDroid->visible[losingPlayer] || psDroid->player == losingPlayer)
			{
				psDroid->visible[rewardPlayer] = UBYTE_MAX;
			}
		}
	}
}


// Return true if structure is a factory of any type.
//
bool StructIsFactory(const STRUCTURE *Struct)
{
	ASSERT_OR_RETURN(false, Struct != nullptr, "Invalid structure!");
	ASSERT_OR_RETURN(false, Struct->pStructureType != nullptr, "Invalid structureType!");

	return Struct->type == OBJ_STRUCTURE && (
		Struct->pStructureType->type == REF_FACTORY ||
		Struct->pStructureType->type == REF_CYBORG_FACTORY ||
		Struct->pStructureType->type == REF_VTOL_FACTORY);
}


// Return true if flag is a delivery point for a factory.
//
bool FlagIsFactory(const FLAG_POSITION *psCurrFlag)
{
	if ((psCurrFlag->factoryType == FACTORY_FLAG) || (psCurrFlag->factoryType == CYBORG_FLAG) ||
	    (psCurrFlag->factoryType == VTOL_FLAG))
	{
		return true;
	}

	return false;
}


// Find a structure's delivery point , only if it's a factory.
// Returns NULL if not found or the structure isn't a factory.
//
FLAG_POSITION *FindFactoryDelivery(const STRUCTURE *Struct)
{
	if (StructIsFactory(Struct))
	{
		// Find the factories delivery point.
		for (FLAG_POSITION *psCurrFlag = apsFlagPosLists[Struct->player]; psCurrFlag; psCurrFlag = psCurrFlag->psNext)
		{
			if (FlagIsFactory(psCurrFlag)
			    && Struct->pFunctionality->factory.psAssemblyPoint->factoryInc == psCurrFlag->factoryInc
			    && Struct->pFunctionality->factory.psAssemblyPoint->factoryType == psCurrFlag->factoryType)
			{
				return psCurrFlag;
			}
		}
	}

	return nullptr;
}


//Find the factory associated with the delivery point - returns NULL if none exist
STRUCTURE	*findDeliveryFactory(FLAG_POSITION *psDelPoint)
{
	STRUCTURE	*psCurr;
	FACTORY		*psFactory;
	REPAIR_FACILITY *psRepair;

	for (psCurr = apsStructLists[psDelPoint->player]; psCurr != nullptr; psCurr =
	         psCurr->psNext)
	{
		if (StructIsFactory(psCurr))
		{
			psFactory = &psCurr->pFunctionality->factory;
			if (psFactory->psAssemblyPoint->factoryInc == psDelPoint->factoryInc &&
			    psFactory->psAssemblyPoint->factoryType == psDelPoint->factoryType)
			{
				return psCurr;
			}
		}
		else if (psCurr->pStructureType->type == REF_REPAIR_FACILITY)
		{
			psRepair = &psCurr->pFunctionality->repairFacility;
			if (psRepair->psDeliveryPoint == psDelPoint)
			{
				return psCurr;
			}
		}
	}
	return nullptr;
}


/*cancels the production run for the factory and returns any power that was
accrued but not used*/
void cancelProduction(STRUCTURE *psBuilding, QUEUE_MODE mode, bool mayClearProductionRun)
{
	ASSERT_OR_RETURN(, StructIsFactory(psBuilding), "structure not a factory");

	FACTORY *psFactory = &psBuilding->pFunctionality->factory;

	if (psBuilding->player == productionPlayer && mayClearProductionRun)
	{
		//clear the production run for this factory
		if (psFactory->psAssemblyPoint->factoryInc < asProductionRun[psFactory->psAssemblyPoint->factoryType].size())
		{
			asProductionRun[psFactory->psAssemblyPoint->factoryType][psFactory->psAssemblyPoint->factoryInc].clear();
		}
		psFactory->productionLoops = 0;

		//tell the interface
		intManufactureFinished(psBuilding);
	}

	if (mode == ModeQueue)
	{
		sendStructureInfo(psBuilding, STRUCTUREINFO_CANCELPRODUCTION, nullptr);
		setStatusPendingCancel(*psFactory);

		return;
	}

	//check its the correct factory
	if (psFactory->psSubject)
	{
		if (psFactory->buildPointsRemaining < calcTemplateBuild(psFactory->psSubject))
		{
			// We started building, so give the power back that was used.
			addPower(psBuilding->player, calcTemplatePower(psFactory->psSubject));
		}

		//clear the factory's subject
		psFactory->psSubject = nullptr;
	}

	delPowerRequest(psBuilding);
}


/*set a factory's production run to hold*/
void holdProduction(STRUCTURE *psBuilding, QUEUE_MODE mode)
{
	FACTORY *psFactory;

	ASSERT_OR_RETURN(, StructIsFactory(psBuilding), "structure not a factory");

	psFactory = &psBuilding->pFunctionality->factory;

	if (mode == ModeQueue)
	{
		sendStructureInfo(psBuilding, STRUCTUREINFO_HOLDPRODUCTION, nullptr);
		setStatusPendingHold(*psFactory);

		return;
	}

	if (psFactory->psSubject)
	{
		//set the time the factory was put on hold
		psFactory->timeStartHold = gameTime;
		//play audio to indicate on hold
		if (psBuilding->player == selectedPlayer)
		{
			audio_PlayTrack(ID_SOUND_WINDOWCLOSE);
		}
	}

	delPowerRequest(psBuilding);
}

/*release a factory's production run from hold*/
void releaseProduction(STRUCTURE *psBuilding, QUEUE_MODE mode)
{
	ASSERT_OR_RETURN(, StructIsFactory(psBuilding), "structure not a factory");

	FACTORY *psFactory = &psBuilding->pFunctionality->factory;

	if (mode == ModeQueue)
	{
		sendStructureInfo(psBuilding, STRUCTUREINFO_RELEASEPRODUCTION, nullptr);
		setStatusPendingRelease(*psFactory);

		return;
	}

	if (psFactory->psSubject && psFactory->timeStartHold)
	{
		//adjust the start time for the current subject
		if (psFactory->timeStarted != ACTION_START_TIME)
		{
			psFactory->timeStarted += (gameTime - psFactory->timeStartHold);
		}
		psFactory->timeStartHold = 0;
	}
}

void doNextProduction(STRUCTURE *psStructure, DROID_TEMPLATE *current, QUEUE_MODE mode)
{
	DROID_TEMPLATE *psNextTemplate = factoryProdUpdate(psStructure, current);

	if (psNextTemplate != nullptr)
	{
		structSetManufacture(psStructure, psNextTemplate, ModeQueue);  // ModeQueue instead of mode, since production lists aren't currently synchronised.
	}
	else
	{
		cancelProduction(psStructure, mode);
	}
}

bool ProductionRunEntry::operator ==(DROID_TEMPLATE *t) const
{
	return psTemplate->multiPlayerID == t->multiPlayerID;
}

/*this is called when a factory produces a droid. The Template returned is the next
one to build - if any*/
DROID_TEMPLATE *factoryProdUpdate(STRUCTURE *psStructure, DROID_TEMPLATE *psTemplate)
{
	CHECK_STRUCTURE(psStructure);
	if (psStructure->player != productionPlayer)
	{
		return nullptr;  // Production lists not currently synchronised.
	}

	FACTORY *psFactory = &psStructure->pFunctionality->factory;
	if (psFactory->psAssemblyPoint->factoryInc >= asProductionRun[psFactory->psAssemblyPoint->factoryType].size())
	{
		return nullptr;  // Don't even have a production list.
	}
	ProductionRun &productionRun = asProductionRun[psFactory->psAssemblyPoint->factoryType][psFactory->psAssemblyPoint->factoryInc];

	if (psTemplate != nullptr)
	{
		//find the entry in the array for this template
		ProductionRun::iterator entry = std::find(productionRun.begin(), productionRun.end(), psTemplate);
		if (entry != productionRun.end())
		{
			entry->built = std::min(entry->built + 1, entry->quantity);
			if (!entry->isComplete())
			{
				return psTemplate;  // Build another of the same type.
			}
			if (psFactory->productionLoops == 0)
			{
				productionRun.erase(entry);
			}
		}
	}
	//find the next template to build - this just looks for the first uncompleted run
	for (unsigned inc = 0; inc < productionRun.size(); ++inc)
	{
		if (!productionRun[inc].isComplete())
		{
			return productionRun[inc].psTemplate;
		}
	}
	// Check that we aren't looping doing nothing.
	if (productionRun.empty())
	{
		if (psFactory->productionLoops != INFINITE_PRODUCTION)
		{
			psFactory->productionLoops = 0;  // Reset number of loops, unless set to infinite.
		}
	}
	else if (psFactory->productionLoops != 0)  //If you've got here there's nothing left to build unless factory is on loop production
	{
		//reduce the loop count if not infinite
		if (psFactory->productionLoops != INFINITE_PRODUCTION)
		{
			psFactory->productionLoops--;
		}

		//need to reset the quantity built for each entry in the production list
		std::for_each(productionRun.begin(), productionRun.end(), std::mem_fun_ref(&ProductionRunEntry::restart));

		//get the first to build again
		return productionRun[0].psTemplate;
	}
	//if got to here then nothing left to produce so clear the array
	productionRun.clear();
	return nullptr;
}


//adjust the production run for this template type
void factoryProdAdjust(STRUCTURE *psStructure, DROID_TEMPLATE *psTemplate, bool add)
{
	CHECK_STRUCTURE(psStructure);
	ASSERT_OR_RETURN(, psStructure->player == productionPlayer, "called for incorrect player");
	ASSERT_OR_RETURN(, psTemplate != nullptr, "NULL template");

	FACTORY *psFactory = &psStructure->pFunctionality->factory;
	if (psFactory->psAssemblyPoint->factoryInc >= asProductionRun[psFactory->psAssemblyPoint->factoryType].size())
	{
		asProductionRun[psFactory->psAssemblyPoint->factoryType].resize(psFactory->psAssemblyPoint->factoryInc + 1);  // Don't have a production list, create it.
	}
	ProductionRun &productionRun = asProductionRun[psFactory->psAssemblyPoint->factoryType][psFactory->psAssemblyPoint->factoryInc];

	//see if the template is already in the list
	ProductionRun::iterator entry = std::find(productionRun.begin(), productionRun.end(), psTemplate);

	if (entry != productionRun.end())
	{
		if (psFactory->productionLoops == 0)
		{
			entry->removeComplete();  // We are not looping, so remove the built droids from the list, so that quantity corresponds to the displayed number.
		}

		//adjust the prod run
		entry->quantity += add ? 1 : -1;
		entry->built = std::min(entry->built, entry->quantity);

		// Allows us to queue up more units up to MAX_IN_RUN instead of ignoring how many we have built from that queue
		// check to see if user canceled all orders in queue
		if (entry->quantity <= 0 || entry->quantity > MAX_IN_RUN)
		{
			productionRun.erase(entry);  // Entry empty, so get rid of it.
		}
	}
	else
	{
		//start off a new template
		ProductionRunEntry entry;
		entry.psTemplate = psTemplate;
		entry.quantity = add ? 1 : MAX_IN_RUN; //wrap around to max value
		entry.built = 0;
		productionRun.push_back(entry);
	}
	//if nothing is allocated then the current factory may have been cancelled
	if (productionRun.empty())
	{
		//must have cancelled everything - so tell the struct
		if (psFactory->productionLoops != INFINITE_PRODUCTION)
		{
			psFactory->productionLoops = 0;  // Reset number of loops, unless set to infinite.
		}
	}
}

/** checks the status of the production of a template
 */
ProductionRunEntry getProduction(STRUCTURE *psStructure, DROID_TEMPLATE *psTemplate)
{
	if (psStructure == nullptr || psStructure->player != productionPlayer || psTemplate == nullptr)
	{
		return ProductionRunEntry();  // Not producing any NULL pointers.
	}

	FACTORY *psFactory = &psStructure->pFunctionality->factory;
	if (psFactory->psAssemblyPoint->factoryInc >= asProductionRun[psFactory->psAssemblyPoint->factoryType].size())
	{
		return ProductionRunEntry();  // Don't have a production list.
	}
	ProductionRun &productionRun = asProductionRun[psFactory->psAssemblyPoint->factoryType][psFactory->psAssemblyPoint->factoryInc];

	//see if the template is in the list
	ProductionRun::iterator entry = std::find(productionRun.begin(), productionRun.end(), psTemplate);
	if (entry != productionRun.end())
	{
		return *entry;
	}

	//not in the list so none being produced
	return ProductionRunEntry();
}


/*looks through a players production list to see how many command droids
are being built*/
UBYTE checkProductionForCommand(UBYTE player)
{
	unsigned quantity = 0;

	if (player == productionPlayer)
	{
		//assumes Cyborg or VTOL droids are not Command types!
		unsigned factoryType = FACTORY_FLAG;

		for (unsigned factoryInc = 0; factoryInc < factoryNumFlag[player][factoryType].size(); ++factoryInc)
		{
			//check to see if there is a factory with a production run
			if (factoryNumFlag[player][factoryType][factoryInc] && factoryInc < asProductionRun[factoryType].size())
			{
				ProductionRun &productionRun = asProductionRun[factoryType][factoryInc];
				for (unsigned inc = 0; inc < productionRun.size(); ++inc)
				{
					if (productionRun[inc].psTemplate->droidType == DROID_COMMAND)
					{
						quantity += productionRun[inc].numRemaining();
					}
				}
			}
		}
	}
	return quantity;
}


// Count number of factories assignable to a command droid.
//
UWORD countAssignableFactories(UBYTE player, UWORD factoryType)
{
	UBYTE           quantity = 0;

	ASSERT_OR_RETURN(0, player == selectedPlayer, "%s should only be called for selectedPlayer", __FUNCTION__);

	for (unsigned factoryInc = 0; factoryInc < factoryNumFlag[player][factoryType].size(); ++factoryInc)
	{
		//check to see if there is a factory
		if (factoryNumFlag[player][factoryType][factoryInc])
		{
			quantity++;
		}
	}

	return quantity;
}


// check whether a factory of a certain number and type exists
bool checkFactoryExists(UDWORD player, UDWORD factoryType, UDWORD inc)
{
	ASSERT_OR_RETURN(false, player < MAX_PLAYERS, "Invalid player");
	ASSERT_OR_RETURN(false, factoryType < NUM_FACTORY_TYPES, "Invalid factoryType");

	return inc < factoryNumFlag[player][factoryType].size() && factoryNumFlag[player][factoryType][inc];
}


//check that delivery points haven't been put down in invalid location
void checkDeliveryPoints(UDWORD version)
{
	UBYTE			inc;
	STRUCTURE		*psStruct;
	FACTORY			*psFactory;
	REPAIR_FACILITY	*psRepair;
	UDWORD					x, y;

	//find any factories
	for (inc = 0; inc < MAX_PLAYERS; inc++)
	{
		//don't bother checking selectedPlayer's - causes problems when try and use
		//validLocation since it finds that the DP is on itself! And validLocation
		//will have been called to put in down in the first place
		if (inc != selectedPlayer)
		{
			for (psStruct = apsStructLists[inc]; psStruct != nullptr; psStruct =
			         psStruct->psNext)
			{
				if (StructIsFactory(psStruct))
				{
					//check the DP
					psFactory = &psStruct->pFunctionality->factory;
					if (psFactory->psAssemblyPoint == nullptr)//need to add one
					{
						ASSERT_OR_RETURN(, psFactory->psAssemblyPoint != nullptr, "no delivery point for factory");
					}
					else
					{
						setAssemblyPoint(psFactory->psAssemblyPoint, psFactory->psAssemblyPoint->
						                 coords.x, psFactory->psAssemblyPoint->coords.y, inc, true);
					}
				}
				else if (psStruct->pStructureType->type == REF_REPAIR_FACILITY)
				{
					psRepair = &psStruct->pFunctionality->repairFacility;

					if (psRepair->psDeliveryPoint == nullptr)//need to add one
					{
						if (version >= VERSION_19)
						{
							ASSERT_OR_RETURN(, psRepair->psDeliveryPoint != nullptr, "no delivery point for repair facility");
						}
						else
						{
							// add an assembly point
							if (!createFlagPosition(&psRepair->psDeliveryPoint, psStruct->player))
							{
								ASSERT(!"can't create new delivery point for repair facility", "unable to create new delivery point for repair facility");
								return;
							}
							addFlagPosition(psRepair->psDeliveryPoint);
							setFlagPositionInc(psStruct->pFunctionality, psStruct->player, REPAIR_FLAG);
							//initialise the assembly point position
							x = map_coord(psStruct->pos.x + 256);
							y = map_coord(psStruct->pos.y + 256);
							// Belt and braces - shouldn't be able to build too near edge
							setAssemblyPoint(psRepair->psDeliveryPoint, world_coord(x),
							                 world_coord(y), inc, true);
						}
					}
					else//check existing one
					{
						setAssemblyPoint(psRepair->psDeliveryPoint, psRepair->psDeliveryPoint->
						                 coords.x, psRepair->psDeliveryPoint->coords.y, inc, true);
					}
				}
			}
		}
	}
}


//adjust the loop quantity for this factory
void factoryLoopAdjust(STRUCTURE *psStruct, bool add)
{
	FACTORY		*psFactory;

	ASSERT_OR_RETURN(, StructIsFactory(psStruct), "structure is not a factory");
	ASSERT_OR_RETURN(, psStruct->player == selectedPlayer, "should only be called for selectedPlayer");

	psFactory = &psStruct->pFunctionality->factory;

	if (add)
	{
		//check for wrapping to infinite production
		if (psFactory->productionLoops == MAX_IN_RUN)
		{
			psFactory->productionLoops = 0;
		}
		else
		{
			//increment the count
			psFactory->productionLoops++;
			//check for limit - this caters for when on infinite production and want to wrap around
			if (psFactory->productionLoops > MAX_IN_RUN)
			{
				psFactory->productionLoops = INFINITE_PRODUCTION;
			}
		}
	}
	else
	{
		//decrement the count
		if (psFactory->productionLoops == 0)
		{
			psFactory->productionLoops = INFINITE_PRODUCTION;
		}
		else
		{
			psFactory->productionLoops--;
		}
	}
}


/*Used for determining how much of the structure to draw as being built or demolished*/
float structHeightScale(const STRUCTURE *psStruct)
{
	float retVal = (float)psStruct->currentBuildPts / (float)psStruct->pStructureType->buildPoints;

	return MAX(retVal, 0.05f);
}


/*compares the structure sensor type with the droid weapon type to see if the
FIRE_SUPPORT order can be assigned*/
bool structSensorDroidWeapon(const STRUCTURE *psStruct, const DROID *psDroid)
{
	//another crash when nStat is marked as 0xcd... FIXME: Why is nStat not initialized properly?
	//Added a safety check: Only units with weapons can be assigned.
	if (psDroid->numWeaps > 0)
	{
		//Standard Sensor Tower + indirect weapon droid (non VTOL)
		//else if (structStandardSensor(psStruct) && (psDroid->numWeaps &&
		if (structStandardSensor(psStruct) && (psDroid->asWeaps[0].nStat > 0 &&
		                                       !proj_Direct(asWeaponStats + psDroid->asWeaps[0].nStat)) &&
		    !isVtolDroid(psDroid))
		{
			return true;
		}
		//CB Sensor Tower + indirect weapon droid (non VTOL)
		//if (structCBSensor(psStruct) && (psDroid->numWeaps &&
		else if (structCBSensor(psStruct) && (psDroid->asWeaps[0].nStat > 0 &&
		                                      !proj_Direct(asWeaponStats + psDroid->asWeaps[0].nStat)) &&
		         !isVtolDroid(psDroid))
		{
			return true;
		}
		//VTOL Intercept Sensor Tower + any weapon VTOL droid
		//else if (structVTOLSensor(psStruct) && psDroid->numWeaps &&
		else if (structVTOLSensor(psStruct) && psDroid->asWeaps[0].nStat > 0 &&
		         isVtolDroid(psDroid))
		{
			return true;
		}
		//VTOL CB Sensor Tower + any weapon VTOL droid
		//else if (structVTOLCBSensor(psStruct) && psDroid->numWeaps &&
		else if (structVTOLCBSensor(psStruct) && psDroid->asWeaps[0].nStat > 0 &&
		         isVtolDroid(psDroid))
		{
			return true;
		}
	}

	//case not matched
	return false;
}


/*checks if the structure has a Counter Battery sensor attached - returns
true if it has*/
bool structCBSensor(const STRUCTURE *psStruct)
{
	// Super Sensor works as any type
	if (psStruct->pStructureType->pSensor
	    && (psStruct->pStructureType->pSensor->type == INDIRECT_CB_SENSOR
	        || psStruct->pStructureType->pSensor->type == SUPER_SENSOR)
	    && psStruct->pStructureType->pSensor->location == LOC_TURRET)
	{
		return true;
	}

	return false;
}


/*checks if the structure has a Standard Turret sensor attached - returns
true if it has*/
bool structStandardSensor(const STRUCTURE *psStruct)
{
	// Super Sensor works as any type
	if (psStruct->pStructureType->pSensor
	    && (psStruct->pStructureType->pSensor->type == STANDARD_SENSOR
	        || psStruct->pStructureType->pSensor->type == SUPER_SENSOR)
	    && psStruct->pStructureType->pSensor->location == LOC_TURRET)
	{
		return true;
	}

	return false;
}


/*checks if the structure has a VTOL Intercept sensor attached - returns
true if it has*/
bool structVTOLSensor(const STRUCTURE *psStruct)
{
	// Super Sensor works as any type
	if (psStruct->pStructureType->pSensor
	    && (psStruct->pStructureType->pSensor->type == VTOL_INTERCEPT_SENSOR
	        || psStruct->pStructureType->pSensor->type == SUPER_SENSOR)
	    && psStruct->pStructureType->pSensor->location == LOC_TURRET)
	{
		return true;
	}

	return false;
}


/*checks if the structure has a VTOL Counter Battery sensor attached - returns
true if it has*/
bool structVTOLCBSensor(const STRUCTURE *psStruct)
{
	// Super Sensor works as any type
	if (psStruct->pStructureType->pSensor
	    && (psStruct->pStructureType->pSensor->type == VTOL_CB_SENSOR
	        || psStruct->pStructureType->pSensor->type == RADAR_DETECTOR_SENSOR
	        || psStruct->pStructureType->pSensor->type == SUPER_SENSOR)
	    && psStruct->pStructureType->pSensor->location == LOC_TURRET)
	{
		return true;
	}

	return false;
}


// check whether a rearm pad is clear
bool clearRearmPad(const STRUCTURE *psStruct)
{
	return psStruct->pStructureType->type == REF_REARM_PAD
	       && (psStruct->pFunctionality->rearmPad.psObj == nullptr
	           || vtolHappy((DROID *)psStruct->pFunctionality->rearmPad.psObj));
}


// return the nearest rearm pad
// psTarget can be NULL
STRUCTURE *findNearestReArmPad(DROID *psDroid, STRUCTURE *psTarget, bool bClear)
{
	STRUCTURE		*psNearest, *psTotallyClear;
	SDWORD			xdiff, ydiff, mindist, currdist, totallyDist;
	SDWORD			cx, cy;

	ASSERT_OR_RETURN(nullptr, psDroid != nullptr, "No droid was passed.");

	if (psTarget != nullptr)
	{
		if (!vtolOnRearmPad(psTarget, psDroid))
		{
			return psTarget;
		}
		cx = (SDWORD)psTarget->pos.x;
		cy = (SDWORD)psTarget->pos.y;
	}
	else
	{
		cx = (SDWORD)psDroid->pos.x;
		cy = (SDWORD)psDroid->pos.y;
	}
	mindist = SDWORD_MAX;
	totallyDist = SDWORD_MAX;
	psNearest = nullptr;
	psTotallyClear = nullptr;
	for (STRUCTURE *psStruct = apsStructLists[psDroid->player]; psStruct; psStruct = psStruct->psNext)
	{
		if (psStruct->pStructureType->type == REF_REARM_PAD
		    && (!bClear || clearRearmPad(psStruct)))
		{
			xdiff = (SDWORD)psStruct->pos.x - cx;
			ydiff = (SDWORD)psStruct->pos.y - cy;
			currdist = xdiff * xdiff + ydiff * ydiff;
			if (bClear && !vtolOnRearmPad(psStruct, psDroid))
			{
				if (currdist < totallyDist)
				{
					totallyDist = currdist;
					psTotallyClear = psStruct;
				}
			}
			else
			{
				if (currdist < mindist)
				{
					mindist = currdist;
					psNearest = psStruct;
				}
			}
		}
	}
	if (bClear && (psTotallyClear != nullptr))
	{
		psNearest = psTotallyClear;
	}
	if (!psNearest)
	{
		objTrace(psDroid->id, "Failed to find a landing spot (%s)!", bClear ? "req clear" : "any");
	}
	return psNearest;
}


// clear a rearm pad for a droid to land on it
void ensureRearmPadClear(STRUCTURE *psStruct, DROID *psDroid)
{
	const int tx = map_coord(psStruct->pos.x);
	const int ty = map_coord(psStruct->pos.y);

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (aiCheckAlliances(psStruct->player, i))
		{
			for (DROID *psCurr = apsDroidLists[i]; psCurr; psCurr = psCurr->psNext)
			{
				if (psCurr != psDroid
				    && map_coord(psCurr->pos.x) == tx
				    && map_coord(psCurr->pos.y) == ty
				    && isVtolDroid(psCurr))
				{
					actionDroid(psCurr, DACTION_CLEARREARMPAD, psStruct);
				}
			}
		}
	}
}


// return whether a rearm pad has a vtol on it
bool vtolOnRearmPad(STRUCTURE *psStruct, DROID *psDroid)
{
	DROID	*psCurr;
	SDWORD	tx, ty;

	tx = map_coord(psStruct->pos.x);
	ty = map_coord(psStruct->pos.y);

	for (psCurr = apsDroidLists[psStruct->player]; psCurr; psCurr = psCurr->psNext)
	{
		if (psCurr != psDroid
		    && map_coord(psCurr->pos.x) == tx
		    && map_coord(psCurr->pos.y) == ty)
		{
			return true;
		}
	}

	return false;
}


/* Just returns true if the structure's present body points aren't as high as the original*/
bool	structIsDamaged(STRUCTURE *psStruct)
{
	return psStruct->body < structureBody(psStruct);
}

// give a structure from one player to another - used in Electronic Warfare
//returns pointer to the new structure
STRUCTURE *giftSingleStructure(STRUCTURE *psStructure, UBYTE attackPlayer, bool electronic_warfare)
{
	STRUCTURE           *psNewStruct, *psStruct;
	DROID               *psCurr;
	STRUCTURE_STATS     *psType, *psModule;
	UDWORD              x, y;
	UBYTE               capacity = 0, originalPlayer;
	SWORD               buildPoints = 0, i;
	bool                bPowerOn;
	UWORD               direction;

	CHECK_STRUCTURE(psStructure);
	visRemoveVisibility(psStructure);

	int prevState = intGetResearchState();

	if (bMultiPlayer)
	{
		//certain structures give specific results - the rest swap sides!
		if (!electronic_warfare || !electronicReward(psStructure, attackPlayer))
		{
			//tell the system the structure no longer exists
			(void)removeStruct(psStructure, false);

			// remove structure from one list
			removeStructureFromList(psStructure, apsStructLists);

			psStructure->selected = false;

			// change player id
			psStructure->player	= (UBYTE)attackPlayer;

			//restore the resistance value
			psStructure->resistance = (UWORD)structureResistance(psStructure->pStructureType, psStructure->player);

			// add to other list.
			addStructure(psStructure);

			//check through the 'attackPlayer' players list of droids to see if any are targetting it
			for (psCurr = apsDroidLists[attackPlayer]; psCurr != nullptr; psCurr = psCurr->psNext)
			{
				if (psCurr->order.psObj == psStructure)
				{
					orderDroid(psCurr, DORDER_STOP, ModeImmediate);
					break;
				}
				for (i = 0; i < psCurr->numWeaps; i++)
				{
					if (psCurr->psActionTarget[i] == psStructure)
					{
						orderDroid(psCurr, DORDER_STOP, ModeImmediate);
						break;
					}
				}
				//check through order list
				orderClearTargetFromDroidList(psCurr, psStructure);
			}

			//check through the 'attackPlayer' players list of structures to see if any are targetting it
			for (psStruct = apsStructLists[attackPlayer]; psStruct != nullptr; psStruct = psStruct->psNext)
			{
				if (psStruct->psTarget[0] == psStructure)
				{
					setStructureTarget(psStruct, nullptr, 0, ORIGIN_UNKNOWN);
				}
			}

			if (psStructure->status == SS_BUILT)
			{
				buildingComplete(psStructure);
			}
			//since the structure isn't being rebuilt, the visibility code needs to be adjusted
			//make sure this structure is visible to selectedPlayer
			psStructure->visible[attackPlayer] = UINT8_MAX;
			triggerEventObjectTransfer(psStructure, attackPlayer);
		}
		intNotifyResearchButton(prevState);
		return nullptr;
	}

	//save info about the structure
	psType = psStructure->pStructureType;
	x = psStructure->pos.x;
	y = psStructure->pos.y;
	direction = psStructure->rot.direction;
	originalPlayer = psStructure->player;
	//save how complete the build process is
	if (psStructure->status == SS_BEING_BUILT)
	{
		buildPoints = psStructure->currentBuildPts;
	}
	//check module not attached
	psModule = getModuleStat(psStructure);
	//get rid of the structure
	(void)removeStruct(psStructure, true);

	//make sure power is not used to build
	bPowerOn = powerCalculated;
	powerCalculated = false;
	//build a new one for the attacking player - set last element to true so it doesn't adjust x/y
	psNewStruct = buildStructure(psType, x, y, attackPlayer, true);
	capacity = psStructure->capacity;
	if (psNewStruct)
	{
		psNewStruct->rot.direction = direction;
		if (capacity)
		{
			switch (psType->type)
			{
			case REF_POWER_GEN:
			case REF_RESEARCH:
				//build the module for powerGen and research
				buildStructure(psModule, psNewStruct->pos.x, psNewStruct->pos.y, attackPlayer, false);
				break;
			case REF_FACTORY:
			case REF_VTOL_FACTORY:
				//build the appropriate number of modules
				while (capacity)
				{
					buildStructure(psModule, psNewStruct->pos.x, psNewStruct->pos.y, attackPlayer, false);
					capacity--;
				}
				break;
			default:
				break;
			}
		}
		if (buildPoints)
		{
			psNewStruct->status = SS_BEING_BUILT;
			psNewStruct->currentBuildPts = buildPoints;
		}
		else
		{
			psNewStruct->status = SS_BUILT;
			buildingComplete(psNewStruct);
			triggerEventStructBuilt(psStructure, nullptr);
		}

		if (!bMultiPlayer)
		{
			if (originalPlayer == selectedPlayer)
			{
				//make sure this structure is visible to selectedPlayer if the structure used to be selectedPlayers'
				psNewStruct->visible[selectedPlayer] = UBYTE_MAX;
			}
		}
	}
	powerCalculated = bPowerOn;
	intNotifyResearchButton(prevState);
	return psNewStruct;
}

/*returns the power cost to build this structure*/
UDWORD structPowerToBuild(const STRUCTURE *psStruct)
{
	if (psStruct->capacity)
	{
		STRUCTURE_STATS *psStats = getModuleStat(psStruct);
		return psStats->powerToBuild; // return the cost to build the module
	}
	// no module attached so building the base structure
	return psStruct->pStructureType->powerToBuild;
}


//for MULTIPLAYER ONLY
//this adjusts the time the relevant action started if the building is attacked by EW weapon
void resetResistanceLag(STRUCTURE *psBuilding)
{
	if (bMultiPlayer)
	{
		switch (psBuilding->pStructureType->type)
		{
		case REF_RESEARCH:
			break;
		case REF_FACTORY:
		case REF_VTOL_FACTORY:
		case REF_CYBORG_FACTORY:
			{
				FACTORY    *psFactory = &psBuilding->pFunctionality->factory;

				//if working on a unit
				if (psFactory->psSubject)
				{
					//adjust the start time for the current subject
					if (psFactory->timeStarted != ACTION_START_TIME)
					{
						psFactory->timeStarted += (gameTime - psBuilding->lastResistance);
					}
				}
			}
		default: //do nothing
			break;
		}
	}
}


/*checks the structure passed in is a Las Sat structure which is currently
selected - returns true if valid*/
bool lasSatStructSelected(STRUCTURE *psStruct)
{
	if ((psStruct->selected || (bMultiPlayer && !isHumanPlayer(psStruct->player)))
	    && psStruct->asWeaps[0].nStat
	    && (asWeaponStats[psStruct->asWeaps[0].nStat].weaponSubClass == WSC_LAS_SAT))
	{
		return true;
	}

	return false;
}

/* Call CALL_NEWDROID script callback */
void cbNewDroid(STRUCTURE *psFactory, DROID *psDroid)
{
	ASSERT_OR_RETURN(, psDroid != nullptr, "no droid assigned for CALL_NEWDROID callback");

	psScrCBNewDroid = psDroid;
	psScrCBNewDroidFact = psFactory;
	eventFireCallbackTrigger((TRIGGER_TYPE)CALL_NEWDROID);
	psScrCBNewDroid = nullptr;
	psScrCBNewDroidFact = nullptr;

	triggerEventDroidBuilt(psDroid, psFactory);
}

StructureBounds getStructureBounds(const STRUCTURE *object)
{
	const Vector2i size = object->size();
	const Vector2i map = map_coord(object->pos.xy) - size / 2;
	return StructureBounds(map, size);
}

StructureBounds getStructureBounds(const STRUCTURE_STATS *stats, Vector2i pos, uint16_t direction)
{
	const Vector2i size = stats->size(direction);
	const Vector2i map = map_coord(pos) - size / 2;
	return StructureBounds(map, size);
}

void checkStructure(const STRUCTURE *psStructure, const char *const location_description, const char *function, const int recurse)
{
	if (recurse < 0)
	{
		return;
	}

	ASSERT_HELPER(psStructure != nullptr, location_description, function, "CHECK_STRUCTURE: NULL pointer");
	ASSERT_HELPER(psStructure->id != 0, location_description, function, "CHECK_STRUCTURE: Structure with ID 0");
	ASSERT_HELPER(psStructure->type == OBJ_STRUCTURE, location_description, function, "CHECK_STRUCTURE: No structure (type num %u)", (unsigned int)psStructure->type);
	ASSERT_HELPER(psStructure->player < MAX_PLAYERS, location_description, function, "CHECK_STRUCTURE: Out of bound player num (%u)", (unsigned int)psStructure->player);
	ASSERT_HELPER(psStructure->pStructureType->type < NUM_DIFF_BUILDINGS, location_description, function, "CHECK_STRUCTURE: Out of bound structure type (%u)", (unsigned int)psStructure->pStructureType->type);
	ASSERT_HELPER(psStructure->numWeaps <= MAX_WEAPONS, location_description, function, "CHECK_STRUCTURE: Out of bound weapon count (%u)", (unsigned int)psStructure->numWeaps);

	for (unsigned i = 0; i < ARRAY_SIZE(psStructure->asWeaps); ++i)
	{
		if (psStructure->psTarget[i])
		{
			checkObject(psStructure->psTarget[i], location_description, function, recurse - 1);
		}
	}
}
