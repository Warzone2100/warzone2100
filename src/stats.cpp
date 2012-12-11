/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
 * @file stats.c
 *
 * Store common stats for weapons, components, brains, etc.
 *
 */
#include <string.h>
#include <algorithm>

#include "lib/framework/frame.h"
#include "lib/framework/strres.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/wzconfig.h"
#include "lib/gamelib/gtime.h"
#include "objects.h"
#include "stats.h"
#include "map.h"
#include "main.h"
#include "lib/sound/audio_id.h"
#include "projectile.h"
#include "text.h"

#define WEAPON_TIME		100

/* The stores for the different stats */
BODY_STATS		*asBodyStats;
BRAIN_STATS		*asBrainStats;
PROPULSION_STATS	*asPropulsionStats;
SENSOR_STATS		*asSensorStats;
ECM_STATS		*asECMStats;
REPAIR_STATS		*asRepairStats;
WEAPON_STATS		*asWeaponStats;
CONSTRUCT_STATS		*asConstructStats;
PROPULSION_TYPES	*asPropulsionTypes;
static TERRAIN_TABLE	*asTerrainTable;

//used to hold the modifiers cross refd by weapon effect and propulsion type
WEAPON_MODIFIER		asWeaponModifier[WE_NUMEFFECTS][PROPULSION_TYPE_NUM];
WEAPON_MODIFIER		asWeaponModifierBody[WE_NUMEFFECTS][SIZE_NUM];

//used to hold the current upgrade level per player per weapon subclass
WEAPON_UPGRADE		asWeaponUpgrade[MAX_PLAYERS][WSC_NUM_WEAPON_SUBCLASSES];
SENSOR_UPGRADE		asSensorUpgrade[MAX_PLAYERS];
ECM_UPGRADE		asECMUpgrade[MAX_PLAYERS];
REPAIR_UPGRADE		asRepairUpgrade[MAX_PLAYERS];
CONSTRUCTOR_UPGRADE	asConstUpgrade[MAX_PLAYERS];
BODY_UPGRADE		asBodyUpgrade[MAX_PLAYERS][BODY_TYPE];

/* The number of different stats stored */
UDWORD		numBodyStats;
UDWORD		numBrainStats;
UDWORD		numPropulsionStats;
UDWORD		numSensorStats;
UDWORD		numECMStats;
UDWORD		numRepairStats;
UDWORD		numWeaponStats;
UDWORD		numConstructStats;

//the max values of the stats used in the design screen
static UDWORD	maxComponentWeight;
static UDWORD	maxBodyArmour;
static UDWORD	maxBodyPower;
static UDWORD	maxBodyPoints;
static UDWORD	maxSensorRange;
static UDWORD	maxECMRange;
static UDWORD	maxConstPoints;
static UDWORD	maxRepairPoints;
static UDWORD	maxWeaponRange;
static UDWORD	maxWeaponDamage;
static UDWORD	maxWeaponROF;
static UDWORD	maxPropulsionSpeed;

//stores for each players component states - can be either UNAVAILABLE, REDUNDANT, FOUND or AVAILABLE
UBYTE		*apCompLists[MAX_PLAYERS][COMP_NUMCOMPONENTS];

//store for each players Structure states
UBYTE		*apStructTypeLists[MAX_PLAYERS];

static bool getMovementModel(const char* movementModel, MOVEMENT_MODEL* model);
static void storeSpeedFactor(UDWORD terrainType, UDWORD propulsionType, UDWORD speedFactor);

//Access functions for the max values to be used in the Design Screen
static void setMaxComponentWeight(UDWORD weight);
static void setMaxBodyArmour(UDWORD armour);
static void setMaxBodyPower(UDWORD power);
static void setMaxBodyPoints(UDWORD points);
static void setMaxSensorRange(UDWORD range);
static void setMaxECMRange(UDWORD power);
static void setMaxConstPoints(UDWORD points);
static void setMaxRepairPoints(UDWORD repair);
static void setMaxWeaponRange(UDWORD range);
static void setMaxWeaponDamage(UDWORD damage);
static void setMaxWeaponROF(UDWORD rof);
static void setMaxPropulsionSpeed(UDWORD speed);

//determine the effect this upgrade would have on the max values
static void updateMaxWeaponStats(UWORD maxValue);
static void updateMaxSensorStats(UWORD maxRange);
static void updateMaxRepairStats(UWORD maxValue);
static void updateMaxECMStats(UWORD maxValue);
static void updateMaxBodyStats(UWORD maxBody, UWORD maxPower, UWORD maxArmour);
static void updateMaxConstStats(UWORD maxValue);

BASE_STATS::BASE_STATS(unsigned ref, std::string const &str)
	: ref(ref)
	, pName(allocateName(str.c_str()))
{}


// This constructor parses the file data in buffer, and stores a pointer to the buffer, along with a list of the starts and ends of each cell in the table.
TableView::TableView(char const *buffer, unsigned size)
	: buffer(buffer)
{
	size = std::min<unsigned>(size, UINT32_MAX - 1);  // Shouldn't be a problem...

	char const *bufferEnd = buffer + size;
	bufferEnd = std::find(buffer, bufferEnd, '\0');  // Truncate buffer at first null character, if any.

	// Split into lines.
	char const *lineNext = buffer;
	while (lineNext != bufferEnd)
	{
		char const *lineBegin = lineNext;
		char const *lineEnd = std::find(lineBegin, bufferEnd, '\n');
		lineNext = lineEnd + (lineEnd != bufferEnd);

		// Remove stuff that isn't data.
		lineEnd = std::find(lineBegin, lineEnd, '#');  // Remove comments, if present.
		while (lineBegin != lineEnd && uint8_t(*(lineEnd - 1)) <= ' ')
		{
			--lineEnd;  // Remove trailing whitespace, if present.
		}
		while (lineBegin != lineEnd && uint8_t(*lineBegin) <= ' ')
		{
			++lineBegin;  // Remove leading whitespace, if present.
		}
		if (lineBegin == lineEnd)
		{
			continue;  // Remove empty lines.
		}

		// Split into cells.
		size_t firstCell = cells.size();
		char const *cellNext = lineBegin;
		while (cellNext != lineEnd)
		{
			char const *cellBegin = cellNext;
			char const *cellEnd = std::find(cellBegin, lineEnd, ',');
			cellNext = cellEnd + (cellEnd != lineEnd);

			cells.push_back(cellBegin - buffer);  // Save the offset of the cell. The size of the cell is then the next offset minus 1 (the ',') minus this offset.
		}
		cells.push_back(lineEnd - buffer + 1);  // Save the end of the last cell, must add 1 to skip a fake ',', because the code later assumes it's the start of a following cell.
		lines.push_back(std::make_pair(firstCell, cells.size() - firstCell));
	}
}

void LineView::setError(unsigned index, char const *error)
{
	if (!table.parseError.isEmpty())
	{
		return;  // Already have an error.
	}

	if (index < numCells)
	{
		char const *cellBegin = table.buffer + cells[index];
		char const *cellEnd = table.buffer + (cells[index + 1] - 1);

		// Print the location and contents of the cell along with the error message.
		char cellDesc[150];
		ssprintf(cellDesc, "Line %u, column %d \"%.*s\": ", lineNumber, index, std::min<unsigned>(100, cellEnd - cellBegin), cellBegin);
		table.parseError = QString::fromUtf8((std::string(cellDesc) + error).c_str());
	}
	else
	{
		// The cell does not exist. Print the location of where the cell would have been along with the error message.
		char cellDesc[50];
		ssprintf(cellDesc, "Line %u, column %d: ", lineNumber, index);
		table.parseError = QString::fromUtf8((std::string(cellDesc) + error).c_str());
	}
}

bool LineView::checkRange(unsigned index)
{
	if (index < numCells)
	{
		return true;
	}

	setError(index, "Not enough cells.");
	return false;
}

int64_t LineView::i(unsigned index, int64_t min, int64_t max)
{
	int errorReturn = std::min(std::max<int64_t>(0, min), max);  // On error, return 0 if possible.

	if (!checkRange(index))
	{
		return errorReturn;
	}

	char const *cellBegin = table.buffer + cells[index];
	char const *cellEnd   = table.buffer + (cells[index + 1] - 1);
	if (cellBegin != cellEnd)
	{
		bool negative = false;
		switch (*cellBegin++)
		{
			case '-':
				negative = true;
				break;
			case '+':
				break;
			default:
				--cellBegin;
				break;
		}
		int64_t absolutePart = 0;
		while (cellBegin != cellEnd && absolutePart < int64_t(1000000000)*1000000000)
		{
			unsigned digit = *cellBegin - '0';
			if (digit > 9)
			{
				break;
			}
			absolutePart = absolutePart*10 + digit;
			++cellBegin;
		}
		if (cellBegin == cellEnd)
		{
			int64_t result = negative? -absolutePart : absolutePart;
			if (result >= min && result <= max)
			{
				return result;  // Maybe should just have copied the string to null-terminate it, and used scanf...
			}
			setError(index, "Integer out of range.");
			return errorReturn;
		}
	}

	setError(index, "Expected integer.");
	return errorReturn;
}

float LineView::f(unsigned index, float min, float max)
{
	float errorReturn = std::min(std::max(0.f, min), max);  // On error, return 0 if possible.

	std::string const &str = s(index);
	if (!str.empty())
	{
		int parsed = 0;
		float result;
		sscanf(str.c_str(), "%f%n", &result, &parsed);
		if ((unsigned)parsed == str.size())
		{
			if (result >= min && result <= max)
			{
				return result;
			}
			setError(index, "Float out of range.");
			return errorReturn;
		}
	}

	setError(index, "Expected float.");
	return errorReturn;
}

std::string const &LineView::s(unsigned index)
{
	if (!checkRange(index))
	{
		table.returnString.clear();
		return table.returnString;
	}

	char const *cellBegin = table.buffer + cells[index];
	char const *cellEnd   = table.buffer + (cells[index + 1] - 1);
	table.returnString.assign(cellBegin, cellEnd);
	return table.returnString;
}

iIMDShape *LineView::imdShape(unsigned int index, bool accept0AsNULL)
{
	std::string const &str = s(index);
	if (accept0AsNULL && str == "0")
	{
		return NULL;
	}
	iIMDShape *result = (iIMDShape *)resGetData("IMD", str.c_str());
	if (result == NULL)
	{
		setError(index, "Expected PIE shape.");
	}
	return result;
}

std::vector<iIMDShape *> LineView::imdShapes(unsigned int index)
{
	std::string const &str = s(index);
	std::vector<iIMDShape *> result;
	int begin = 0;
	do
	{
		int end = str.find_first_of('@', begin);
		if (end == std::string::npos)
		{
			end = str.size();
		}
		result.push_back((iIMDShape *)resGetData("IMD", str.substr(begin, end - begin).c_str()));
		if (result.back() == NULL)
		{
			setError(index, "Expected PIE shape.");
		}
		begin = end + 1;
	} while (begin != str.size() + 1);
	return result;
}

static inline bool stringToEnumFindFunction(std::pair<char const *, unsigned> const &a, char const *b)
{
	return strcmp(a.first, b) < 0;
}

unsigned LineView::eu(unsigned int index, std::vector<std::pair<char const *, unsigned> > const &map)
{
	typedef std::vector<std::pair<char const *, unsigned> > M;

	std::string const &str = s(index);
	M::const_iterator i = std::lower_bound(map.begin(), map.end(), str.c_str(), stringToEnumFindFunction);
	if (i != map.end() && strcmp(i->first, str.c_str()) == 0)
	{
		return i->second;
	}

	// Didn't find it, give error and return 0.
	if (table.parseError.isEmpty())
	{
		std::string values = "Expected one of";
		for (i = map.begin(); i != map.end(); ++i)
		{
			values += std::string(" \"") + i->first + '"';
		}
		values = values + '.';
		setError(index, "Expected one of");
	}
	return 0;
}

/***********************************************************************************
*	Dealloc the extra storage tables
***********************************************************************************/
//Deallocate the storage assigned for the Propulsion Types table
static void deallocPropulsionTypes(void)
{
	free(asPropulsionTypes);
	asPropulsionTypes = NULL;
}

//dealloc the storage assigned for the terrain table
static void deallocTerrainTable(void)
{
	free(asTerrainTable);
	asTerrainTable = NULL;
}

/*******************************************************************************
*		Generic stats macros/functions
*******************************************************************************/

/* Macro to allocate memory for a set of stats */
#define ALLOC_STATS(numEntries, list, listSize, type) \
	ASSERT( (numEntries) < REF_RANGE, \
	"allocStats: number of stats entries too large for " #type );\
	if ((list))	free((list));	\
	(list) = (type *)malloc(sizeof(type) * (numEntries)); \
	(listSize) = (numEntries); \
	return true

/*Macro to Deallocate stats*/
#define STATS_DEALLOC(list, listSize) \
	free((COMPONENT_STATS*)(list)); \
	listSize = 0; \
	(list) = NULL 

void statsInitVars(void)
{
	/* The number of different stats stored */
	numBodyStats = 0;
	numBrainStats = 0;
	numPropulsionStats = 0;
	numSensorStats = 0;
	numECMStats = 0;
	numRepairStats = 0;
	numWeaponStats = 0;
	numConstructStats = 0;

	//initialise the upgrade structures
	memset(asWeaponUpgrade, 0, sizeof(asWeaponUpgrade));
	memset(asSensorUpgrade, 0, sizeof(asSensorUpgrade));
	memset(asECMUpgrade, 0, sizeof(asECMUpgrade));
	memset(asRepairUpgrade, 0, sizeof(asRepairUpgrade));
	memset(asConstUpgrade, 0, sizeof(asConstUpgrade));
	memset(asBodyUpgrade, 0, sizeof(asBodyUpgrade));

	// init the max values
	maxComponentWeight = maxBodyArmour = maxBodyPower =
		maxBodyPoints = maxSensorRange = maxECMRange =
		maxConstPoints = maxRepairPoints = maxWeaponRange = maxWeaponDamage =
		maxPropulsionSpeed = 0;
}

static void allocateStatName(BASE_STATS* pStat, const char *Name)
{
	pStat->pName = allocateName(Name);
}

/* body stats need the extra list removing */
static void deallocBodyStats(void)
{
	BODY_STATS *psStat;
	UDWORD		inc;

	for (inc = 0; inc < numBodyStats; inc++)
	{
		psStat = &asBodyStats[inc];
		free(psStat->ppIMDList);
	}
	free(asBodyStats);
	asBodyStats = NULL;
	numBodyStats = 0;
}

/*Deallocate all the stats assigned from input data*/
bool statsShutDown(void)
{
	STATS_DEALLOC(asWeaponStats, numWeaponStats);
	deallocBodyStats();
	STATS_DEALLOC(asBrainStats, numBrainStats);
	STATS_DEALLOC(asPropulsionStats, numPropulsionStats);
	STATS_DEALLOC(asSensorStats, numSensorStats);
	STATS_DEALLOC(asECMStats, numECMStats);
	STATS_DEALLOC(asRepairStats, numRepairStats);
	STATS_DEALLOC(asConstructStats, numConstructStats);
	deallocPropulsionTypes();
	deallocTerrainTable();

	return true;
}

/* Macro to set the stats for a particular ref
 * The macro uses the ref number in the stats structure to
 * index the correct array entry
 */
#define SET_STATS(stats, list, index, type, refStart) \
	ASSERT( ((stats)->ref >= (refStart)) && ((stats)->ref < (refStart) + REF_RANGE), \
		"setStats: Invalid " #type " ref number" ); \
	memcpy((list) + (index), (stats), sizeof(type))

/*******************************************************************************
*		Set stats functions
*******************************************************************************/
/* Set the stats for a particular weapon type */
static void statsSetWeapon(WEAPON_STATS	*psStats, UDWORD index)
{
	SET_STATS(psStats, asWeaponStats, index, WEAPON_STATS, REF_WEAPON_START);
}
/* Set the stats for a particular body type */
static void statsSetBody(BODY_STATS	*psStats, UDWORD index)
{
	SET_STATS(psStats, asBodyStats, index, BODY_STATS, REF_BODY_START);
}
/* Set the stats for a particular brain type */
static void statsSetBrain(BRAIN_STATS	*psStats, UDWORD index)
{
	SET_STATS(psStats, asBrainStats, index, BRAIN_STATS, REF_BRAIN_START);
}
/* Set the stats for a particular power type */
static void statsSetPropulsion(PROPULSION_STATS	*psStats, UDWORD index)
{
	SET_STATS(psStats, asPropulsionStats, index, PROPULSION_STATS,
		REF_PROPULSION_START);
}
/* Set the stats for a particular sensor type */
static void statsSetSensor(SENSOR_STATS	*psStats, UDWORD index)
{
	SET_STATS(psStats, asSensorStats, index, SENSOR_STATS, REF_SENSOR_START);
}
/* Set the stats for a particular ecm type */
static void statsSetECM(ECM_STATS	*psStats, UDWORD index)
{
	SET_STATS(psStats, asECMStats, index, ECM_STATS, REF_ECM_START);
}
/* Set the stats for a particular repair type */
static void statsSetRepair(REPAIR_STATS	*psStats, UDWORD index)
{
	SET_STATS(psStats, asRepairStats, index, REPAIR_STATS, REF_REPAIR_START);
}
/* Set the stats for a particular construct type */
static void statsSetConstruct(CONSTRUCT_STATS	*psStats, UDWORD index)
{
	SET_STATS(psStats, asConstructStats, index, CONSTRUCT_STATS, REF_CONSTRUCT_START);
}

/* Return the number of newlines in a file buffer */
UDWORD numCR(const char *pFileBuffer, UDWORD fileSize)
{
	UDWORD  lines=0;

	while (fileSize-- > 0)
	{
		if (*pFileBuffer++ == '\n')
		{
			lines++;
		}
	}

	return lines;
}


/*******************************************************************************
*		Allocate stats functions
*******************************************************************************/
/* Allocate Weapon stats */
bool statsAllocWeapons(UDWORD	numStats)
{
	ALLOC_STATS(numStats, asWeaponStats, numWeaponStats, WEAPON_STATS);
}
/* Allocate Body Stats */
bool statsAllocBody(UDWORD	numStats)
{
	ALLOC_STATS(numStats, asBodyStats, numBodyStats, BODY_STATS);
}
/* Allocate Brain Stats */
bool statsAllocBrain(UDWORD	numStats)
{
	ALLOC_STATS(numStats, asBrainStats, numBrainStats, BRAIN_STATS);
}
/* Allocate Propulsion Stats */
bool statsAllocPropulsion(UDWORD	numStats)
{
	ALLOC_STATS(numStats, asPropulsionStats, numPropulsionStats, PROPULSION_STATS);
}
/* Allocate Sensor Stats */
bool statsAllocSensor(UDWORD	numStats)
{
	ALLOC_STATS(numStats, asSensorStats, numSensorStats, SENSOR_STATS);
}
/* Allocate Ecm Stats */
bool statsAllocECM(UDWORD	numStats)
{
	ALLOC_STATS(numStats, asECMStats, numECMStats, ECM_STATS);
}

/* Allocate Repair Stats */
bool statsAllocRepair(UDWORD	numStats)
{
	ALLOC_STATS(numStats, asRepairStats, numRepairStats, REPAIR_STATS);
}

/* Allocate Construct Stats */
bool statsAllocConstruct(UDWORD	numStats)
{
	ALLOC_STATS(numStats, asConstructStats, numConstructStats, CONSTRUCT_STATS);
}

const char *getStatName(const void * Stat)
{
	const BASE_STATS * const psStats= (const BASE_STATS*)Stat;

	return getName(psStats->pName);
}




/*******************************************************************************
*		Load stats functions
*******************************************************************************/

static iIMDShape *statsGetIMD(WzConfig &ini, BASE_STATS *psStats, QString key)
{
	iIMDShape *retval = NULL;
	if (ini.contains(key))
	{
		QString model = ini.value(key).toString();
		retval = (iIMDShape *)resGetData("IMD", model.toUtf8().constData());
		ASSERT(retval != NULL, "Cannot find the PIE model %s for stat %s in %s", 
		       model.toUtf8().constData(), getStatName(psStats), ini.fileName().toUtf8().constData());
	}
	return retval;
}

/*Load the weapon stats from the file exported from Access*/
bool loadWeaponStats(const char *pFileName)
{
	WEAPON_STATS	sStats, * const psStats = &sStats;
	UDWORD			i, rotate, maxElevation, surfaceToAir;
	SDWORD			minElevation;
	UDWORD numAttackRuns;

	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
	}
	QStringList list = ini.childGroups();
	if (!statsAllocWeapons(list.size()))
	{
		return false;
	}
	// Hack to make sure ZNULLWEAPON is always first in list
	int nullweapon = list.indexOf("ZNULLWEAPON");
	ASSERT_OR_RETURN(false, nullweapon >= 0, "ZNULLWEAPON is mandatory");
	if (nullweapon > 0)
	{
		list.swap(nullweapon, 0);
	}

	for (i=0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		memset(psStats, 0, sizeof(WEAPON_STATS));

		psStats->buildPower = ini.value("buildPower", 0).toUInt();
		psStats->buildPoints = ini.value("buildPoints", 0).toUInt();
		psStats->weight = ini.value("weight", 0).toUInt();
		psStats->body = ini.value("body", 0).toUInt();
		psStats->longRange = ini.value("longRange").toUInt();
		psStats->longHit = ini.value("longHit").toUInt();
		psStats->firePause = ini.value("firePause").toUInt();
		psStats->numExplosions = ini.value("numExplosions").toUInt();
		psStats->numRounds = ini.value("numRounds").toUInt();
		psStats->reloadTime = ini.value("reloadTime").toUInt();
		psStats->damage = ini.value("damage").toUInt();
		psStats->radius = ini.value("radius").toUInt();
		psStats->radiusHit = ini.value("radiusHit").toUInt();
		psStats->radiusDamage = ini.value("radiusDamage").toUInt();
		psStats->incenTime = ini.value("incenTime").toUInt();
		psStats->incenDamage = ini.value("incenDamage").toUInt();
		psStats->incenRadius = ini.value("incenRadius").toUInt();
		psStats->radiusLife = ini.value("radiusLife").toUInt();
		psStats->flightSpeed = ini.value("flightSpeed", 1).toUInt();
		rotate = ini.value("rotate").toUInt();
		minElevation = ini.value("minElevation").toInt();
		maxElevation = ini.value("maxElevation").toUInt();
		psStats->recoilValue = ini.value("recoilValue").toUInt();
		psStats->minRange = ini.value("minRange").toUInt();
		psStats->effectSize = ini.value("effectSize").toUInt();
		surfaceToAir = ini.value("surfaceToAir").toUInt();
		numAttackRuns = ini.value("numAttackRuns").toUInt();
		psStats->designable = ini.value("designable").toBool();
		psStats->penetrate = ini.value("penetrate").toBool();
		// weapon size limitation
		psStats->weaponSize = (WEAPON_SIZE)ini.value("weaponSize", WEAPON_SIZE_ANY).toInt();

		ASSERT(psStats->flightSpeed > 0, "Invalid flight speed for %s", list[i].toUtf8().constData());

		allocateStatName((BASE_STATS *)psStats, list[i].toUtf8().constData());
		psStats->ref = REF_WEAPON_START + i;

		//multiply time stats
		psStats->firePause *= WEAPON_TIME;
		psStats->incenTime *= WEAPON_TIME;
		psStats->radiusLife *= WEAPON_TIME;
		psStats->reloadTime *= WEAPON_TIME;

		//get the IMD for the component
		psStats->pIMD = statsGetIMD(ini, psStats, "model");
		psStats->pMountGraphic = statsGetIMD(ini, psStats, "mountModel");
		if (GetGameMode() == GS_NORMAL)
		{
			psStats->pMuzzleGraphic = statsGetIMD(ini, psStats, "muzzleGfx");
			psStats->pInFlightGraphic = statsGetIMD(ini, psStats, "flightGfx");
			psStats->pTargetHitGraphic = statsGetIMD(ini, psStats, "hitGfx");
			psStats->pTargetMissGraphic = statsGetIMD(ini, psStats, "missGfx");
			psStats->pWaterHitGraphic = statsGetIMD(ini, psStats, "waterGfx");
			psStats->pTrailGraphic = statsGetIMD(ini, psStats, "trailGfx");
		}
		psStats->fireOnMove = ini.value("fireOnMove", true).toBool();

		QString weaponClass = ini.value("weaponClass").toString();
		//set the weapon class
		if (weaponClass.compare("KINETIC") == 0)
		{
			psStats->weaponClass = WC_KINETIC;
		}
		else if (weaponClass.compare("EXPLOSIVE") == 0)
		{
			//psStats->weaponClass = WC_EXPLOSIVE;
			psStats->weaponClass = WC_KINETIC; 	// explosives were removed from release version of Warzone
		}
		else if (weaponClass.compare("HEAT") == 0)
		{
			psStats->weaponClass = WC_HEAT;
		}
		else if (weaponClass.compare("MISC") == 0)
		{
			//psStats->weaponClass = WC_MISC;
			psStats->weaponClass = WC_HEAT;		// removed from release version of Warzone
		}
		else
		{
			debug( LOG_ERROR, "Invalid weapon class for weapon %s - assuming KINETIC", getStatName(psStats) );
			psStats->weaponClass = WC_KINETIC;
		}
		//set the subClass
		if (!getWeaponSubClass(ini.value("weaponSubClass").toString().toUtf8().data(), &psStats->weaponSubClass))
		{
			return false;
		}

		//set the movement model
		if (!getMovementModel(ini.value("movement").toString().toUtf8().constData(), &psStats->movementModel))
		{
			return false;
		}

		//set the weapon effect
		if (!getWeaponEffect(ini.value("weaponEffect").toString().toUtf8().constData(), &psStats->weaponEffect))
		{
			debug(LOG_FATAL, "loadWepaonStats: Invalid weapon effect for weapon %s", getStatName(psStats));
			return false;
		}

		// set the face Player value
		psStats->facePlayer = ini.value("facePlayer", false).toBool();

		// set the In flight face Player value
		psStats->faceInFlight = ini.value("faceInFlight", false).toBool();

		//set the light world value
		psStats->lightWorld = ini.value("lightWorld").toBool();

		//set the rotate angle
		if (rotate > UBYTE_MAX)
		{
			ASSERT( false,"loadWeaponStats: rotate is greater than 255 for weapon %s",
				getStatName(psStats) );
			return false;
		}
		psStats->rotate = (UBYTE)rotate;

		//set the minElevation
		if (minElevation > SBYTE_MAX || minElevation < SBYTE_MIN)
		{
			ASSERT( false,"loadWeaponStats: minElevation is outside of limits for weapon %s",
				getStatName(psStats) );
			return false;
		}
		psStats->minElevation = (SBYTE)minElevation;

		//set the maxElevation
		if (maxElevation > UBYTE_MAX)
		{
			ASSERT( false,"loadWeaponStats: maxElevation is outside of limits for weapon %s",
				getStatName(psStats) );
			return false;
		}
		psStats->maxElevation = (UBYTE)maxElevation;

		//set the surfaceAir
		if (surfaceToAir > UBYTE_MAX)
		{
			ASSERT( false, "loadWeaponStats: Surface to Air is outside of limits for weapon %s",
				getStatName(psStats) );
			return false;
		}
		if (surfaceToAir == 0)
		{
			psStats->surfaceToAir = (UBYTE)SHOOT_ON_GROUND;
		}
		else if (surfaceToAir <= 50)
		{
			psStats->surfaceToAir = (UBYTE)SHOOT_IN_AIR;
		}
		else
		{
			psStats->surfaceToAir = (UBYTE)(SHOOT_ON_GROUND | SHOOT_IN_AIR);
		}

		//set the attackRuns for VTOLs
		if (numAttackRuns > UBYTE_MAX)
		{
			ASSERT( false, "loadWeaponStats: num of attack runs is outside of limits for weapon %s",
				getStatName(psStats) );
			return false;
		}
		psStats->vtolAttackRuns = (UBYTE)numAttackRuns;

		//set the weapon sounds to default value
		psStats->iAudioFireID = NO_SOUND;
		psStats->iAudioImpactID = NO_SOUND;

		//save the stats
		statsSetWeapon(psStats, i);

		// Set the max stat values for the design screen
		if (psStats->designable)
		{
			setMaxWeaponRange(psStats->longRange);
			setMaxWeaponDamage(psStats->damage);
			setMaxWeaponROF(weaponROF(psStats, -1));
			setMaxComponentWeight(psStats->weight);
		}

		ini.endGroup();
	}

	return true;
}

/*Load the Body stats from the file exported from Access*/
bool loadBodyStats(const char *pFileName)
{
	BODY_STATS sStats, * const psStats = &sStats;
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
	}
	QStringList list = ini.childGroups();
	statsAllocBody(list.size());
	// Hack to make sure ZNULLBODY is always first in list
	int nullbody = list.indexOf("ZNULLBODY");
	ASSERT_OR_RETURN(false, nullbody >= 0, "ZNULLBODY is mandatory");
	if (nullbody > 0)
	{
		list.swap(nullbody, 0);
	}
	for (int i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		memset(psStats, 0, sizeof(*psStats));
		psStats->weight = ini.value("weight", 0).toInt();
		psStats->buildPower = ini.value("buildPower", 0).toInt();
		psStats->buildPoints = ini.value("buildPoints", 0).toInt();
		psStats->body = ini.value("hitpoints").toInt();
		psStats->weaponSlots = ini.value("weaponSlots").toInt();
		psStats->powerOutput = ini.value("powerOutput").toInt();
		psStats->armourValue[WC_KINETIC] = ini.value("armourKinetic").toInt();
		psStats->armourValue[WC_HEAT] = ini.value("armourHeat").toInt();
		psStats->designable = ini.value("designable", false).toBool();
		QString dtype = ini.value("droidType", "DROID").toString();
		psStats->droidTypeOverride = DROID_DEFAULT;
		if (dtype.compare("PERSON") == 0)
		{
			psStats->droidTypeOverride = DROID_PERSON;
		}
		else if (dtype.compare("TRANSPORTER") == 0)
		{
			psStats->droidTypeOverride = DROID_TRANSPORTER;
		}
		else if (dtype.compare("CYBORG") == 0)
		{
			psStats->droidTypeOverride = DROID_CYBORG;
		}
		else if (dtype.compare("CYBORG_SUPER") == 0)
		{
			psStats->droidTypeOverride = DROID_CYBORG_SUPER;
		}
		else if (dtype.compare("CYBORG_CONSTRUCT") == 0)
		{
			psStats->droidTypeOverride = DROID_CYBORG_CONSTRUCT;
		}
		else if (dtype.compare("CYBORG_REPAIR") == 0)
		{
			psStats->droidTypeOverride = DROID_CYBORG_REPAIR;
		}
		psStats->ref = REF_BODY_START + i;
		if (!getBodySize(ini.value("size").toString().toUtf8().constData(), &psStats->size))
		{
			ASSERT(false, "Unknown body size for %s", getStatName(psStats));
			return false;
		}
		psStats->pIMD = statsGetIMD(ini, psStats, "model");
		psStats->pFlameIMD = statsGetIMD(ini, psStats, "flameModel");

		ini.endGroup();

		allocateStatName((BASE_STATS *)psStats, list[i].toUtf8().constData());
		statsSetBody(psStats, i);	// save the stats

		//set the max stat values for the design screen
		if (psStats->designable)
		{
			setMaxBodyArmour(psStats->armourValue[WC_KINETIC]);
			setMaxBodyArmour(psStats->armourValue[WC_HEAT]);
			setMaxBodyPower(psStats->powerOutput);
			setMaxBodyPoints(psStats->body);
			setMaxComponentWeight(psStats->weight);
		}
	}
	return true;
}

/*Load the Brain stats from the file exported from Access*/
bool loadBrainStats(const char *pFileName)
{
	BRAIN_STATS sStats, * const psStats = &sStats;
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
	}

	QStringList list = ini.childGroups();
	statsAllocBrain(list.size());
	// Hack to make sure ZNULLBRAIN is always first in list
	int nullbrain = list.indexOf("ZNULLBRAIN");
	ASSERT_OR_RETURN(false, nullbrain >= 0, "ZNULLBRAIN is mandatory");
	if (nullbrain > 0)
	{
		list.swap(nullbrain, 0);
	}
	for (int i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		memset(psStats, 0, sizeof(BRAIN_STATS));

		psStats->buildPower = ini.value("buildPower", 0).toInt();
		psStats->buildPoints = ini.value("buildPoints", 0).toInt();
		psStats->weight = ini.value("weight", 0).toInt();
		psStats->maxDroids = ini.value("maxDroids").toInt();
		psStats->maxDroidsMult = ini.value("maxDroidsMult").toInt();
		psStats->ref = REF_BRAIN_START + i;
		allocateStatName((BASE_STATS *)psStats, list[i].toUtf8().constData());

		// check weapon attached
		psStats->psWeaponStat = NULL;
		if (ini.contains("turret"))
		{
			int weapon = getCompFromName(COMP_WEAPON, ini.value("turret").toString().toUtf8().constData());
			ASSERT_OR_RETURN(false, weapon >= 0, "Unable to find weapon for brain %s", psStats->pName);
			psStats->psWeaponStat = asWeaponStats + weapon;
		}
		psStats->designable = ini.value("designable", false).toBool();
		ini.endGroup();
		// save the stats
		statsSetBrain(psStats, i);
	}
	return true;
}


/*returns the propulsion type based on the string name passed in */
bool getPropulsionType(const char* typeName, PROPULSION_TYPE* type)
{
	if      (strcmp(typeName, "Wheeled") == 0)
	{
		*type = PROPULSION_TYPE_WHEELED;
	}
	else if (strcmp(typeName, "Tracked") == 0)
	{
		*type = PROPULSION_TYPE_TRACKED;
	}
	else if (strcmp(typeName, "Legged") == 0)
	{
		*type = PROPULSION_TYPE_LEGGED;
	}
	else if (strcmp(typeName, "Hover") == 0)
	{
		*type = PROPULSION_TYPE_HOVER;
	}
	else if (strcmp(typeName, "Ski") == 0)
	{
		*type = PROPULSION_TYPE_SKI;
	}
	else if (strcmp(typeName, "Lift") == 0)
	{
		*type = PROPULSION_TYPE_LIFT;
	}
	else if (strcmp(typeName, "Propellor") == 0)
	{
		*type = PROPULSION_TYPE_PROPELLOR;
	}
	else if (strcmp(typeName, "Half-Tracked") == 0)
	{
		*type = PROPULSION_TYPE_HALF_TRACKED;
	}
	else if (strcmp(typeName, "Jump") == 0)
	{
		*type = PROPULSION_TYPE_JUMP;
	}
	else
	{
		debug( LOG_ERROR, "getPropulsionType: Invalid Propulsion type %s - assuming Hover", typeName );
		*type = PROPULSION_TYPE_HOVER;
	}

	return true;
}

/*Load the Propulsion stats from the file exported from Access*/
bool loadPropulsionStats(const char *pFileName)
{
	PROPULSION_STATS sStats, *const psStats = &sStats;
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
	}
	QStringList list = ini.childGroups();
	statsAllocPropulsion(list.size());
	// Hack to make sure ZNULLPROP is always first in list
	int nullprop = list.indexOf("ZNULLPROP");
	ASSERT_OR_RETURN(false, nullprop >= 0, "ZNULLPROP is mandatory");
	if (nullprop > 0)
	{
		list.swap(nullprop, 0);
	}
	for (int i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		memset(psStats, 0, sizeof(*psStats));
		psStats->buildPower = ini.value("buildPower").toInt();
		psStats->buildPoints = ini.value("buildPoints").toInt();
		psStats->weight = ini.value("weight").toInt();
		psStats->body = ini.value("hitpoints").toInt();
		psStats->maxSpeed = ini.value("speed").toInt();
		psStats->designable = ini.value("designable", false).toBool();
		psStats->ref = REF_PROPULSION_START + i;
		psStats->turnSpeed = ini.value("turnSpeed", DEG(1)/3).toInt();
		psStats->spinSpeed = ini.value("spinSpeed", DEG(3)/4).toInt();
		psStats->spinAngle = ini.value("spinAngle", 180).toInt();
		psStats->acceleration = ini.value("acceleration", 250).toInt();
		psStats->deceleration = ini.value("deceleration", 800).toInt();
		psStats->skidDeceleration = ini.value("skidDeceleration", 600).toInt();
		psStats->pIMD = NULL;
		psStats->pIMD = statsGetIMD(ini, psStats, "model");
		if (!getPropulsionType(ini.value("type").toString().toUtf8().constData(), &psStats->propulsionType))
		{
			debug(LOG_FATAL, "loadPropulsionStats: Invalid Propulsion type for %s", getStatName(psStats));
			return false;
		}
		ini.endGroup();

		allocateStatName((BASE_STATS *)psStats, list[i].toUtf8().constData());
		// save the stats
		statsSetPropulsion(psStats, i);

		// set the max stat values for the design screen
		if (psStats->designable)
		{
			setMaxPropulsionSpeed(psStats->maxSpeed);
		}
	}

	/* since propulsion weight is a multiple of body weight we may need to adjust the max component weight value */
	if (asBodyStats && asPropulsionStats)	// check we've loaded them both in
	{
		//check against each body stat
		for (int i = 0; i < numBodyStats; ++i)
		{
			//check stat is designable
			if (asBodyStats[i].designable)
			{
				//check against each propulsion stat
				for (int j = 0; j < numPropulsionStats; ++j)
				{
					//check stat is designable
					if (asPropulsionStats[j].designable)
					{
						setMaxComponentWeight(asPropulsionStats[j].weight * asBodyStats[i].weight / 100);
					}
				}
			}
		}
	}

	return true;
}

/*Load the Sensor stats from the file exported from Access*/
bool loadSensorStats(const char *pFileName)
{
	SENSOR_STATS sStats, * const psStats = &sStats;
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
	}
	QStringList list = ini.childGroups();
	if (!statsAllocSensor(list.size()))
	{
		return false;
	}
	// Hack to make sure ZNULLSENSOR is always first in list
	int nullsensor = list.indexOf("ZNULLSENSOR");
	ASSERT_OR_RETURN(false, nullsensor >= 0, "ZNULLSENSOR is mandatory");
	if (nullsensor > 0)
	{
		list.swap(nullsensor, 0);
	}

	for (int i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		memset(psStats, 0, sizeof(SENSOR_STATS));
		psStats->buildPower = ini.value("buildPower", 0).toInt();
		psStats->buildPoints = ini.value("buildPoints", 0).toInt();
		psStats->weight = ini.value("weight", 0).toInt();
		psStats->body = ini.value("bodyPoints", 0).toInt();
		psStats->range = ini.value("range").toInt();
		psStats->time = ini.value("time").toInt();
		psStats->power = ini.value("power").toInt();
		psStats->designable = ini.value("designable", false).toBool();

		allocateStatName((BASE_STATS *)psStats, list[i].toUtf8().constData());
		psStats->ref = REF_SENSOR_START + i;

		QString location = ini.value("location").toString();
		if (location.compare("DEFAULT") == 0)
		{
			psStats->location = LOC_DEFAULT;
		}
		else if (location.compare("TURRET") == 0)
		{
			psStats->location = LOC_TURRET;
		}
		else
		{
			ASSERT( false, "Invalid Sensor location" );
		}
		QString type = ini.value("type").toString();
		if (type.compare("STANDARD") == 0)
		{
			psStats->type = STANDARD_SENSOR;
		}
		else if (type.compare("INDIRECT CB") == 0)
		{
			psStats->type = INDIRECT_CB_SENSOR;
		}
		else if (type.compare("VTOL CB") == 0)
		{
			psStats->type = VTOL_CB_SENSOR;
		}
		else if (type.compare("VTOL INTERCEPT") == 0)
		{
			psStats->type = VTOL_INTERCEPT_SENSOR;
		}
		else if (type.compare("SUPER") == 0)
		{
			psStats->type = SUPER_SENSOR;
		}
		else if (type.compare("RADAR DETECTOR") == 0)
		{
			psStats->type = RADAR_DETECTOR_SENSOR;
		}
		else
		{
			ASSERT( false, "Invalid Sensor type" );
		}
		//multiply time stats
		psStats->time *= WEAPON_TIME;

		//get the IMD for the component
		psStats->pIMD = statsGetIMD(ini, psStats, "sensorModel");
		psStats->pMountGraphic = statsGetIMD(ini, psStats, "mountModel");

		ini.endGroup();
		//save the stats
		statsSetSensor(psStats, i);

		// set the max stat values for the design screen
		if (psStats->designable)
		{
			setMaxSensorRange(psStats->range);
			setMaxComponentWeight(psStats->weight);
		}

	}
	return true;
}

/*Load the ECM stats from the file exported from Access*/
bool loadECMStats(const char *pFileName)
{
	ECM_STATS sStats, * const psStats = &sStats;
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
	}
	QStringList list = ini.childGroups();
	if (!statsAllocECM(list.size()))
	{
		return false;
	}
	// Hack to make sure ZNULLECM is always first in list
	int nullecm = list.indexOf("ZNULLECM");
	ASSERT_OR_RETURN(false, nullecm >= 0, "ZNULLECM is mandatory");
	if (nullecm > 0)
	{
		list.swap(nullecm, 0);
	}
	for (int i=0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		memset(psStats, 0, sizeof(ECM_STATS));

		psStats->buildPower = ini.value("buildPower", 0).toInt();
		psStats->buildPoints = ini.value("buildPoints", 0).toInt();
		psStats->weight = ini.value("weight", 0).toInt();
		psStats->body = ini.value("bodyPoints", 0).toInt();
		psStats->range = ini.value("range").toInt();
		psStats->power = ini.value("power").toInt();
		psStats->designable = ini.value("designable", false).toBool();

		allocateStatName((BASE_STATS *)psStats, list[i].toUtf8().constData());
		psStats->ref = REF_ECM_START + i;

		QString location = ini.value("location").toString();
		if (location.compare("DEFAULT") == 0)
		{
			psStats->location = LOC_DEFAULT;
		}
		else if (location.compare("TURRET") == 0)
		{
			psStats->location = LOC_TURRET;
		}
		else
		{
			ASSERT( false, "Invalid ECM location" );
		}

		//get the IMD for the component
		psStats->pIMD = statsGetIMD(ini, psStats, "sensorModel");
		psStats->pMountGraphic = statsGetIMD(ini, psStats, "mountModel");

		ini.endGroup();
		//save the stats
		statsSetECM(psStats, i);

		// Set the max stat values for the design screen
		if (psStats->designable)
		{
			setMaxECMRange(psStats->range);
			setMaxComponentWeight(psStats->weight);
		}
	}
	return true;
}

/*Load the Repair stats from the file exported from Access*/
bool loadRepairStats(const char *pFileName)
{
	REPAIR_STATS sStats, * const psStats = &sStats;
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
	}
	QStringList list = ini.childGroups();
	if (!statsAllocRepair(list.size()))
	{
		return false;
	}
	// Hack to make sure ZNULLREPAIR is always first in list
	int nullrepair = list.indexOf("ZNULLREPAIR");
	ASSERT_OR_RETURN(false, nullrepair >= 0, "ZNULLREPAIR is mandatory");
	if (nullrepair > 0)
	{
		list.swap(nullrepair, 0);
	}

	for (int i=0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		memset(psStats, 0, sizeof(REPAIR_STATS));

		psStats->buildPower = ini.value("buildPower", 0).toInt();
		psStats->buildPoints = ini.value("buildPoints", 0).toInt();
		psStats->weight = ini.value("weight", 0).toInt();
		psStats->repairArmour = ini.value("repairArmour").toBool();
		psStats->repairPoints = ini.value("repairPoints").toInt();
		psStats->time = ini.value("time", 0).toInt() * WEAPON_TIME;
		psStats->designable = ini.value("designable", false).toBool();

		allocateStatName((BASE_STATS *)psStats, list[i].toUtf8().constData());
		psStats->ref = REF_REPAIR_START + i;

		QString location = ini.value("location").toString();
		if (location.compare("DEFAULT") == 0)
		{
			psStats->location = LOC_DEFAULT;
		}
		else if (location.compare("TURRET") == 0)
		{
			psStats->location = LOC_TURRET;
		}
		else
		{
			ASSERT( false, "Invalid Repair location" );
		}

		//check its not 0 since we will be dividing by it at a later stage
		ASSERT_OR_RETURN(false, psStats->time > 0, "Repair delay cannot be zero for %s", getStatName(psStats));

		//get the IMD for the component
		psStats->pIMD = statsGetIMD(ini, psStats, "model");
		psStats->pMountGraphic = statsGetIMD(ini, psStats, "mountModel");

		ini.endGroup();
		//save the stats
		statsSetRepair(psStats, i);

		//set the max stat values for the design screen
		if (psStats->designable)
		{
			setMaxRepairPoints(psStats->repairPoints);
			setMaxComponentWeight(psStats->weight);
		}

	}
	return true;
}

/*Load the Construct stats from the file exported from Access*/
bool loadConstructStats(const char *pFileName)
{
	CONSTRUCT_STATS sStats, * const psStats = &sStats;
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
	}
	QStringList list = ini.childGroups();
	if (!statsAllocConstruct(list.size()))
	{
		return false;
	}
	// Hack to make sure ZNULLCONSTRUCT is always first in list
	int nullconstruct = list.indexOf("ZNULLCONSTRUCT");
	ASSERT_OR_RETURN(false, nullconstruct >= 0, "ZNULLCONSTRUCT is mandatory");
	if (nullconstruct > 0)
	{
		list.swap(nullconstruct, 0);
	}
	for (int i=0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		memset(psStats, 0, sizeof(CONSTRUCT_STATS));

		psStats->buildPower = ini.value("buildPower", 0).toInt();
		psStats->buildPoints = ini.value("buildPoints", 0).toInt();
		psStats->weight = ini.value("weight", 0).toInt();
		psStats->body = ini.value("bodyPoints", 0).toInt();
		psStats->constructPoints = ini.value("constructPoints").toInt();
		psStats->designable = ini.value("designable", false).toBool();

		allocateStatName((BASE_STATS *)psStats, list[i].toUtf8().constData());
		psStats->ref = REF_CONSTRUCT_START + i;

		//get the IMD for the component
		psStats->pIMD = statsGetIMD(ini, psStats, "sensorModel");
		psStats->pMountGraphic = statsGetIMD(ini, psStats, "mountModel");

		ini.endGroup();
		//save the stats
		statsSetConstruct(psStats, i);

		// Set the max stat values for the design screen
		if (psStats->designable)
		{
			setMaxConstPoints(psStats->constructPoints);
			setMaxComponentWeight(psStats->weight);
		}
	}
	return true;
}


/*Load the Propulsion Types from the file exported from Access*/
bool loadPropulsionTypes(const char *pFileName)
{
	const unsigned int NumTypes = PROPULSION_TYPE_NUM;
	PROPULSION_TYPES *pPropType;
	unsigned int multiplier;
	PROPULSION_TYPE type;

	//allocate storage for the stats
	asPropulsionTypes = (PROPULSION_TYPES *)malloc(sizeof(PROPULSION_TYPES)*NumTypes);
	memset(asPropulsionTypes, 0, (sizeof(PROPULSION_TYPES)*NumTypes));
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
	}
	QStringList list = ini.childGroups();

	for (int i=0; i < NumTypes; ++i)
	{
		ini.beginGroup(list[i]);
		multiplier = ini.value("multiplier").toInt();

		//set the pointer for this record based on the name
		if (!getPropulsionType(list[i].toUtf8().constData(), &type))
		{
			debug(LOG_FATAL, "Invalid Propulsion type - %s", list[i].toUtf8().constData());
			return false;
		}

		pPropType = asPropulsionTypes + type;

		QString flightName = ini.value("flightName").toString();
		if (flightName.compare("GROUND") == 0)
		{
			pPropType->travel = GROUND;
		}
		else if (flightName.compare("AIR") == 0)
		{
			pPropType->travel = AIR;
		}
		else
		{
			ASSERT( false, "Invalid travel type for Propulsion" );
		}

		//don't care about this anymore! AB FRIDAY 13/11/98
		//want it back again! AB 27/11/98
		if (multiplier > UWORD_MAX)
		{
			ASSERT( false, "loadPropulsionTypes: power Ratio multiplier too high" );
			//set to a default value since not life threatening!
			multiplier = 100;
		}
		pPropType->powerRatioMult = (UWORD)multiplier;

		//initialise all the sound variables
		pPropType->startID = NO_SOUND;
		pPropType->idleID = NO_SOUND;
		pPropType->moveOffID = NO_SOUND;
		pPropType->moveID = NO_SOUND;
		pPropType->hissID = NO_SOUND;
		pPropType->shutDownID = NO_SOUND;

		ini.endGroup();
	}

	return true;
}


/*Load the Terrain Table from the file exported from Access*/
bool loadTerrainTable(const char *pFileName)
{
	unsigned int i;
	UDWORD	terrainType;
	QString propulsionType, speedFactor;

	//allocate storage for the stats
	asTerrainTable = (TERRAIN_TABLE *)malloc(sizeof(*asTerrainTable) * PROPULSION_TYPE_NUM * TER_MAX);

	//initialise the storage to 100
	for (i = 0; i < TER_MAX; ++i)
	{
		for (int j = 0; j < PROPULSION_TYPE_NUM; ++j)
		{
			TERRAIN_TABLE * const pTerrainTable = &asTerrainTable[i * PROPULSION_TYPE_NUM + j];
			pTerrainTable->speedFactor = 100;
		}
	}
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
	}
	QStringList list = ini.childGroups();
	for (i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		terrainType = list[i].toUInt();
		propulsionType = ini.value("propulsionType").toString();
		speedFactor = ini.value("speedFactor").toString();

		QStringList propulsionTypes = propulsionType.split(",");
		QStringList speedFactors = speedFactor.split(",");
		for (int x=0; x< propulsionTypes.size(); ++x)
		{
			storeSpeedFactor(terrainType, propulsionTypes[x].toUInt(), speedFactors[x].toUInt());
		}
		ini.endGroup();
	}

	//check that none of the entries are 0 otherwise this will stop a droid dead in its tracks
	//and it will not be able to move again!
	for (i = 0; i < TER_MAX; ++i)
	{
		for (int j = 0; j < PROPULSION_TYPE_NUM; ++j)
		{
			TERRAIN_TABLE * const pTerrainTable = asTerrainTable + (i * PROPULSION_TYPE_NUM + j);
			if (pTerrainTable->speedFactor == 0)
			{
				debug(LOG_FATAL, "Invalid propulsion/terrain table entry");
				return false;
			}
		}
	}

	return true;
}

/* load the IMDs to use for each body-propulsion combination */
bool loadBodyPropulsionIMDs(const char *pFileName)
{
	BODY_STATS *psBodyStat = asBodyStats;
	PROPULSION_STATS *psPropulsionStat = asPropulsionStats;
	unsigned int i, numStats;
	QString propulsionName, leftIMD, rightIMD;
	iIMDShape **startIMDs;
	bool found;

	//check that the body and propulsion stats have already been read in

	ASSERT( asBodyStats != NULL, "Body Stats have not been set up" );
	ASSERT( asPropulsionStats != NULL, "Propulsion Stats have not been set up" );

	//allocate space
	for (numStats = 0; numStats < numBodyStats; ++numStats)
	{
		psBodyStat = &asBodyStats[numStats];
		psBodyStat->ppIMDList = (iIMDShape **) malloc(numPropulsionStats * NUM_PROP_SIDES * sizeof(iIMDShape *));
		//initialise the pointer space
		memset(psBodyStat->ppIMDList, 0, (numPropulsionStats *
			NUM_PROP_SIDES * sizeof(iIMDShape *)));
	}

	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
	}
	QStringList list = ini.childGroups();

	for (i=0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		propulsionName = ini.value("propulsion").toString();
		QStringList propulsionNames = propulsionName.split(",");
		leftIMD = ini.value("leftIMD").toString();
		QStringList leftIMDs = leftIMD.split(",");
		rightIMD = ini.value("rightIMD").toString();
		QStringList rightIMDs = rightIMD.split(",");

		//get the body stats
		found = false;
		for (numStats = 0; numStats < numBodyStats; ++numStats)
		{
			psBodyStat = &asBodyStats[numStats];
			if (!strcmp(psBodyStat->pName, list[i].toUtf8().constData()))
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			debug(LOG_FATAL, "Invalid body name %s", list[i].toUtf8().constData());
			return false;
		}

		for (int x=0; x< propulsionNames.size(); ++x)
		{
			//get the propulsion stats
			found = false;
			for (numStats = 0; numStats < numPropulsionStats; numStats++)
			{
				psPropulsionStat = &asPropulsionStats[numStats];
				if (!strcmp(psPropulsionStat->pName, propulsionNames[x].toUtf8().constData()))
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				debug(LOG_FATAL, "Invalid propulsion name %s", propulsionNames[x].toUtf8().constData());
				return false;
			}

			//allocate the left and right propulsion IMDs
			startIMDs = psBodyStat->ppIMDList;
			psBodyStat->ppIMDList += (numStats * NUM_PROP_SIDES);
			if (strcmp(leftIMDs[x].toUtf8().constData(), "0"))
			{
				*psBodyStat->ppIMDList = (iIMDShape *) resGetData("IMD", leftIMDs[x].toUtf8().constData());
				if (*psBodyStat->ppIMDList == NULL)
				{
					debug(LOG_FATAL, "Cannot find the left propulsion PIE for body %s", list[i].toUtf8().constData());
					return false;
				}
			}
			else
			{
				*psBodyStat->ppIMDList = NULL;
			}

			psBodyStat->ppIMDList++;
			//right IMD might not be there
			if (strcmp(rightIMDs[x].toUtf8().constData(), "0"))
			{
				*psBodyStat->ppIMDList = (iIMDShape *) resGetData("IMD", rightIMDs[x].toUtf8().constData());
				if (*psBodyStat->ppIMDList == NULL)
				{
					debug(LOG_FATAL, "Cannot find the right propulsion PIE for body %s", list[i].toUtf8().constData());
					return false;
				}
			}
			else
			{
				*psBodyStat->ppIMDList = NULL;
			}

			//reset the IMDList pointer
			psBodyStat->ppIMDList = startIMDs;
		}
		ini.endGroup();
	}

	return(true);
}



static bool statsGetAudioIDFromString(const char *szStatName, const char *szWavName, SDWORD *piWavID)
{
	if ( strcmp( szWavName, "-1" ) == 0 )
	{
		*piWavID = NO_SOUND;
	}
	else
	{
		if ( (*piWavID = audio_GetIDFromStr(szWavName)) == NO_SOUND )
		{
			debug(LOG_FATAL, "Could not get ID %d for sound %s", *piWavID, szWavName);
			return false;
		}
	}
	if ((*piWavID < 0
	  || *piWavID > ID_MAX_SOUND)
	 && *piWavID != NO_SOUND)
	{
		debug(LOG_FATAL, "Invalid ID - %d for sound %s", *piWavID, szStatName);
		return false;
	}

	return true;
}



/*Load the weapon sounds from the file exported from Access*/
bool loadWeaponSounds(const char *pFileName)
{
	SDWORD weaponSoundID, explosionSoundID, inc;
	QString szWeaponWav, szExplosionWav;

	ASSERT( asWeaponStats != NULL, "loadWeaponSounds: Weapon stats not loaded" );

	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
	}
	QStringList list = ini.childGroups();
	for (int i=0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		szWeaponWav = ini.value("szWeaponWav").toString();
		QStringList szWeaponWavs = szWeaponWav.split(",");
		szExplosionWav = ini.value("szExplosionWav").toString();
		QStringList szExplosionWavs = szExplosionWav.split(",");

		for (int x=0; x< szWeaponWavs.size(); ++x)
		{
			if (!statsGetAudioIDFromString(list[i].toUtf8().constData(), szWeaponWavs[x].toUtf8().constData(), &weaponSoundID))
			{
				return false;
			}
			if (!statsGetAudioIDFromString(list[i].toUtf8().constData(), szExplosionWavs[x].toUtf8().constData(), &explosionSoundID))
			{
				return false;
			}
			for (inc = 0; inc < (SDWORD)numWeaponStats; ++inc)
			{
				if (list[i].compare(asWeaponStats[inc].pName) == 0)
				{
					asWeaponStats[inc].iAudioFireID = weaponSoundID;
					asWeaponStats[inc].iAudioImpactID = explosionSoundID;
					break;
				}
			}
			ASSERT_OR_RETURN(false, inc != (SDWORD)numWeaponStats, "Weapon stat not found - %s", list[i].toUtf8().constData());
		}
		ini.endGroup();
	}

	return true;
}

bool loadWeaponModifiers(const char *pFileName)
{
	//initialise to 100%
	for (int i = 0; i < WE_NUMEFFECTS; i++)
	{
		for (int j = 0; j < PROPULSION_TYPE_NUM; j++)
		{
			asWeaponModifier[i][j] = 100;
		}
		for (int j = 0; j < SIZE_NUM; j++)
		{
			asWeaponModifierBody[i][j] = 100;
		}
	}
	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
	}
	QStringList list = ini.childGroups();
	for (int i = 0; i < list.size(); i++)
	{
		WEAPON_EFFECT effectInc;
		PROPULSION_TYPE propInc;

		ini.beginGroup(list[i]);
		//get the weapon effect inc
		if (!getWeaponEffect(list[i].toUtf8().constData(), &effectInc))
		{
			debug(LOG_FATAL, "Invalid Weapon Effect - %s", list[i].toUtf8().constData());
			continue;
		}
		QStringList keys = ini.childKeys();
		for (int j = 0; j < keys.size(); j++)
		{
			int modifier = ini.value(keys.at(j)).toInt();
			if (!getPropulsionType(keys.at(j).toUtf8().data(), &propInc))
			{
				// If not propulsion, must be body
				BODY_SIZE body = SIZE_NUM;
				if (!getBodySize(keys.at(j).toUtf8().data(), &body))
				{
					debug(LOG_FATAL, "Invalid Propulsion or Body type - %s", keys.at(j).toUtf8().constData());
					continue;
				}
				asWeaponModifierBody[effectInc][body] = modifier;
			}
			else // is propulsion
			{
				asWeaponModifier[effectInc][propInc] = modifier;
			}
		}
		ini.endGroup();
	}
	return true;
}

/*Load the propulsion type sounds from the file exported from Access*/
bool loadPropulsionSounds(const char *pFileName)
{
	SDWORD	i, startID, idleID, moveOffID, moveID, hissID, shutDownID;
	PROPULSION_TYPE type;
	PROPULSION_TYPES *pPropType;

	ASSERT( asPropulsionTypes != NULL, "loadPropulsionSounds: Propulsion type stats not loaded" );

	WzConfig ini(pFileName);
	if (ini.status() != QSettings::NoError)
	{
		debug(LOG_ERROR, "Could not open %s", pFileName);
	}
	QStringList list = ini.childGroups();
	for (i=0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		if (!statsGetAudioIDFromString(list[i].toUtf8().constData(), ini.value("szStart").toString().toUtf8().constData(), &startID))
		{
			return false;
		}
		if (!statsGetAudioIDFromString(list[i].toUtf8().constData(), ini.value("szIdle").toString().toUtf8().constData(), &idleID))
		{
			return false;
		}
		if (!statsGetAudioIDFromString(list[i].toUtf8().constData(), ini.value("szMoveOff").toString().toUtf8().constData(), &moveOffID))
		{
			return false;
		}
		if (!statsGetAudioIDFromString(list[i].toUtf8().constData(), ini.value("szMove").toString().toUtf8().constData(), &moveID))
		{
			return false;
		}
		if (!statsGetAudioIDFromString(list[i].toUtf8().constData(), ini.value("szHiss").toString().toUtf8().constData(), &hissID))
		{
			return false;
		}
		if (!statsGetAudioIDFromString(list[i].toUtf8().constData(), ini.value("szShutDown").toString().toUtf8().constData(), &shutDownID))
		{
			return false;
		}
		if (!getPropulsionType(list[i].toUtf8().constData(), &type))
		{
			debug(LOG_FATAL, "Invalid Propulsion type - %s", list[i].toUtf8().constData());
			return false;
		}
		pPropType = asPropulsionTypes + type;
		pPropType->startID = (SWORD)startID;
		pPropType->idleID = (SWORD)idleID;
		pPropType->moveOffID = (SWORD)moveOffID;
		pPropType->moveID = (SWORD)moveID;
		pPropType->hissID = (SWORD)hissID;
		pPropType->shutDownID = (SWORD)shutDownID;

		ini.endGroup();
	}

	return(true);
}

//store the speed Factor in the terrain table
static void storeSpeedFactor(UDWORD terrainType, UDWORD propulsionType, UDWORD speedFactor)
{
	TERRAIN_TABLE *pTerrainTable = asTerrainTable;

	ASSERT( propulsionType < PROPULSION_TYPE_NUM,
		"The propulsion type is too large" );

	pTerrainTable += (terrainType * PROPULSION_TYPE_NUM + propulsionType);
	pTerrainTable->speedFactor = speedFactor;
}

//get the speed factor for a given terrain type and propulsion type
UDWORD getSpeedFactor(UDWORD type, UDWORD propulsionType)
{
	TERRAIN_TABLE *pTerrainTable = asTerrainTable;

	ASSERT( propulsionType < PROPULSION_TYPE_NUM,
		"The propulsion type is too large" );

	pTerrainTable += (type * PROPULSION_TYPE_NUM + propulsionType);

	return pTerrainTable->speedFactor;
}

//return the type of stat this stat is!
UDWORD statType(UDWORD ref)
{
	if (ref >=REF_BODY_START && ref < REF_BODY_START +
		REF_RANGE)
	{
		return COMP_BODY;
	}
	if (ref >=REF_BRAIN_START && ref < REF_BRAIN_START +
		REF_RANGE)
	{
		return COMP_BRAIN;
	}
	if (ref >=REF_PROPULSION_START && ref <
		REF_PROPULSION_START + REF_RANGE)
	{
		return COMP_PROPULSION;
	}
	if (ref >=REF_SENSOR_START && ref < REF_SENSOR_START +
		REF_RANGE)
	{
		return COMP_SENSOR;
	}
	if (ref >=REF_ECM_START && ref < REF_ECM_START +
		REF_RANGE)
	{
		return COMP_ECM;
	}
	if (ref >=REF_REPAIR_START && ref < REF_REPAIR_START +
		REF_RANGE)
	{
		return COMP_REPAIRUNIT;
	}
	if (ref >=REF_WEAPON_START && ref < REF_WEAPON_START +
		REF_RANGE)
	{
		return COMP_WEAPON;
	}
	if (ref >=REF_CONSTRUCT_START && ref < REF_CONSTRUCT_START +
		REF_RANGE)
	{
		return COMP_CONSTRUCT;
	}
	//else
	ASSERT( false, "Invalid stat pointer - cannot determine Stat Type" );
	return COMP_UNKNOWN;
}

//return the REF_START value of this type of stat
UDWORD statRefStart(UDWORD stat)
{
	UDWORD start;

	switch (stat)
	{
		case COMP_BODY:
		{
			start = REF_BODY_START;
			break;
		}
		case COMP_BRAIN:
		{
			start = REF_BRAIN_START;
			break;
		}
		case COMP_PROPULSION:
		{
			start = REF_PROPULSION_START;
			break;
		}
		case COMP_SENSOR:
		{
			start = REF_SENSOR_START;
			break;
		}
		case COMP_ECM:
		{
			start = REF_ECM_START;
			break;
		}
		case COMP_REPAIRUNIT:
		{
			start = REF_REPAIR_START;
			break;
		}
		case COMP_WEAPON:
		{
			start = REF_WEAPON_START;
			break;
		}
		case COMP_CONSTRUCT:
		{
			start = REF_CONSTRUCT_START;
			break;
		}
		default:
		{
			//COMP_UNKNOWN should be an error
			debug(LOG_FATAL, "Invalid stat type");
			start = 0;
		}
	}
	return start;
}

/*Returns the component type based on the string - used for reading in data */
unsigned int componentType(const char* pType)
{
	if (!strcmp(pType, "BODY"))
	{
		return COMP_BODY;
	}
	if (!strcmp(pType, "PROPULSION"))
	{
		return COMP_PROPULSION;
	}
	if (!strcmp(pType, "BRAIN"))
	{
		return COMP_BRAIN;
	}
	if (!strcmp(pType, "REPAIR"))
	{
		return COMP_REPAIRUNIT;
	}
	if (!strcmp(pType, "ECM"))
	{
		return COMP_ECM;
	}
	if (!strcmp(pType, "SENSOR"))
	{
		return COMP_SENSOR;
	}
	if (!strcmp(pType, "WEAPON"))
	{
		return COMP_WEAPON;
	}
	if (!strcmp(pType, "CONSTRUCT"))
	{
		return COMP_CONSTRUCT;
	}

	ASSERT( false, "Unknown Component Type" );
	return 0; // Should never get here.
}

//get the component Inc for a stat based on the Resource name and type
//returns -1 if record not found
//used in Scripts
SDWORD	getCompFromResName(UDWORD compType, const char *pName)
{
	return getCompFromName(compType, pName);
}

static void getStatsDetails(UDWORD compType, BASE_STATS **ppsStats, UDWORD *pnumStats, UDWORD *pstatSize)
{

	switch (compType)
	{
	case COMP_BODY:
		*ppsStats = (BASE_STATS*)asBodyStats;
		*pnumStats = numBodyStats;
		*pstatSize = sizeof(BODY_STATS);
		break;
	case COMP_BRAIN:
		*ppsStats = (BASE_STATS*)asBrainStats;
		*pnumStats = numBrainStats;
		*pstatSize = sizeof(BRAIN_STATS);
		break;
	case COMP_PROPULSION:
		*ppsStats = (BASE_STATS*)asPropulsionStats;
		*pnumStats = numPropulsionStats;
		*pstatSize = sizeof(PROPULSION_STATS);
		break;
	case COMP_REPAIRUNIT:
		*ppsStats = (BASE_STATS*)asRepairStats;
		*pnumStats = numRepairStats;
		*pstatSize = sizeof(REPAIR_STATS);
		break;
	case COMP_ECM:
		*ppsStats = (BASE_STATS*)asECMStats;
		*pnumStats = numECMStats;
		*pstatSize = sizeof(ECM_STATS);
		break;
	case COMP_SENSOR:
		*ppsStats = (BASE_STATS*)asSensorStats;
		*pnumStats = numSensorStats;
		*pstatSize = sizeof(SENSOR_STATS);
		break;
	case COMP_CONSTRUCT:
		*ppsStats = (BASE_STATS*)asConstructStats;
		*pnumStats = numConstructStats;
		*pstatSize = sizeof(CONSTRUCT_STATS);
		break;
	case COMP_WEAPON:
		*ppsStats = (BASE_STATS*)asWeaponStats;
		*pnumStats = numWeaponStats;
		*pstatSize = sizeof(WEAPON_STATS);
		break;
	default:
		//COMP_UNKNOWN should be an error
		debug(LOG_FATAL, "Invalid component type");
	}
}


//get the component Inc for a stat based on the name and type
//returns -1 if record not found
SDWORD getCompFromName(UDWORD compType, const char *pName)
{
	BASE_STATS	*psStats = NULL;
	UDWORD		numStats = 0, count, statSize = 0;

	getStatsDetails(compType, &psStats,&numStats,&statSize);

	//find the stat with the same name
	for(count = 0; count < numStats; count++)
	{
		if (!strcmp(pName, psStats->pName))
		{
			return count;
		}
		psStats = (BASE_STATS*)((char*)psStats + statSize);
	}

	//return -1 if record not found or an invalid component type is passed in
	return -1;
}

/*return the name to display for the interface - valid for OBJECTS and STATS*/
const char* getName(const char *pNameID)
{
	/* See if the name has a string resource associated with it by trying
	 * to get the string resource.
	 */
	const char * const name = strresGetString(psStringRes, pNameID);
	if (!name)
	{
		debug( LOG_ERROR, "Unable to find string resource for %s", pNameID );
		return "Name Unknown";
	}

	return name;
}

/*sets the store to the body size based on the name passed in - returns false
if doesn't compare with any*/
bool getBodySize(const char *pSize, BODY_SIZE *pStore)
{
	if (!strcmp(pSize, "LIGHT"))
	{
		*pStore = SIZE_LIGHT;
		return true;
	}
	else if (!strcmp(pSize, "MEDIUM"))
	{
		*pStore = SIZE_MEDIUM;
		return true;
	}
	else if (!strcmp(pSize, "HEAVY"))
	{
		*pStore = SIZE_HEAVY;
		return true;
	}
	else if (!strcmp(pSize, "SUPER HEAVY"))
	{
		*pStore = SIZE_SUPER_HEAVY;
		return true;
	}

	ASSERT( false, "Invalid size - %s", pSize );
	return false;
}

/*returns the weapon sub class based on the string name passed in */
bool getWeaponSubClass(const char* subClass, WEAPON_SUBCLASS* wclass)
{
	if      (strcmp(subClass, "CANNON") == 0)
	{
		*wclass = WSC_CANNON;
	}
	else if (strcmp(subClass, "MORTARS") == 0)
	{
		*wclass = WSC_MORTARS;
	}
	else if (strcmp(subClass, "MISSILE") == 0)
	{
		*wclass = WSC_MISSILE;
	}
	else if (strcmp(subClass, "ROCKET") == 0)
	{
		*wclass = WSC_ROCKET;
	}
	else if (strcmp(subClass, "ENERGY") == 0)
	{
		*wclass = WSC_ENERGY;
	}
	else if (strcmp(subClass, "GAUSS") == 0)
	{
		*wclass = WSC_GAUSS;
	}
	else if (strcmp(subClass, "FLAME") == 0)
	{
		*wclass = WSC_FLAME;
	}
	else if (strcmp(subClass, "HOWITZERS") == 0)
	{
		*wclass = WSC_HOWITZERS;
	}
	else if (strcmp(subClass, "MACHINE GUN") == 0)
	{
		*wclass = WSC_MGUN;
	}
	else if (strcmp(subClass, "ELECTRONIC") == 0)
	{
		*wclass = WSC_ELECTRONIC;
	}
	else if (strcmp(subClass, "A-A GUN") == 0)
	{
		*wclass = WSC_AAGUN;
	}
	else if (strcmp(subClass, "SLOW MISSILE") == 0)
	{
		*wclass = WSC_SLOWMISSILE;
	}
	else if (strcmp(subClass, "SLOW ROCKET") == 0)
	{
		*wclass = WSC_SLOWROCKET;
	}
	else if (strcmp(subClass, "LAS_SAT") == 0)
	{
		*wclass = WSC_LAS_SAT;
	}
	else if (strcmp(subClass, "BOMB") == 0)
	{
		*wclass = WSC_BOMB;
	}
	else if (strcmp(subClass, "COMMAND") == 0)
	{
		*wclass = WSC_COMMAND;
	}
	else if (strcmp(subClass, "EMP") == 0)
	{
		*wclass = WSC_EMP;
	}
        else
        {
	    ASSERT(!"Invalid weapon sub class", "Invalid weapon sub class: %s", subClass);
            return false;
        }

	return true;
}

/*returns the movement model based on the string name passed in */
bool getMovementModel(const char* movementModel, MOVEMENT_MODEL* model)
{
	if (strcmp(movementModel,"DIRECT") == 0)
	{
		*model = MM_DIRECT;
	}
	else if (strcmp(movementModel,"INDIRECT") == 0)
	{
		*model = MM_INDIRECT;
	}
	else if (strcmp(movementModel,"HOMING-DIRECT") == 0)
	{
		*model = MM_HOMINGDIRECT;
	}
	else if (strcmp(movementModel,"HOMING-INDIRECT") == 0)
	{
		*model = MM_HOMINGINDIRECT;
	}
	else if (strcmp(movementModel,"ERRATIC-DIRECT") == 0)
	{
		*model = MM_ERRATICDIRECT;
	}
	else if (strcmp(movementModel,"SWEEP") == 0)
	{
		*model = MM_SWEEP;
	}
	else
	{
		// We've got problem if we got here
		ASSERT(!"Invalid movement model", "Invalid movement model: %s", movementModel);
		return false;
	}

	return true;
}

const StringToEnum<WEAPON_EFFECT> mapUnsorted_WEAPON_EFFECT[] =
{
	{"ANTI PERSONNEL",      WE_ANTI_PERSONNEL       },
	{"ANTI TANK",           WE_ANTI_TANK            },
	{"BUNKER BUSTER",       WE_BUNKER_BUSTER        },
	{"ARTILLERY ROUND",     WE_ARTILLERY_ROUND      },
	{"FLAMER",              WE_FLAMER               },
	{"ANTI AIRCRAFT",       WE_ANTI_AIRCRAFT        },
	{"ALL ROUNDER",         WE_ANTI_AIRCRAFT        },  // Alternative name for WE_ANTI_AIRCRAFT.
};
const StringToEnumMap<WEAPON_EFFECT> map_WEAPON_EFFECT = mapUnsorted_WEAPON_EFFECT;

bool getWeaponEffect(const char* weaponEffect, WEAPON_EFFECT* effect)
{
	if      (strcmp(weaponEffect, "ANTI PERSONNEL") == 0)
	{
		*effect = WE_ANTI_PERSONNEL;
	}
	else if (strcmp(weaponEffect, "ANTI TANK") == 0)
	{
		*effect = WE_ANTI_TANK;
	}
	else if (strcmp(weaponEffect, "BUNKER BUSTER") == 0)
	{
		*effect = WE_BUNKER_BUSTER;
	}
	else if (strcmp(weaponEffect, "ARTILLERY ROUND") == 0)
	{
		*effect = WE_ARTILLERY_ROUND;
	}
	else if (strcmp(weaponEffect, "FLAMER") == 0)
	{
		*effect = WE_FLAMER;
	}
	else if (strcmp(weaponEffect, "ANTI AIRCRAFT") == 0 || strcmp(weaponEffect, "ALL ROUNDER") == 0)
	{
		*effect = WE_ANTI_AIRCRAFT;
	}
	else
	{
		ASSERT(!"Invalid weapon effect", "Invalid weapon effect: %s", weaponEffect);
		return false;
	}

	return true;
}


/*
looks up the name to get the resource associated with it - or allocates space
and stores the name. Eventually ALL names will be 'resourced' for translation
*/
char* allocateName(const char* name)
{
	/* Check whether the given string has a string resource associated with
	 * it.
	 */
	if (!strresGetString(psStringRes, name))
	{
		debug(LOG_FATAL, "Unable to find string resource for %s", name);
		return NULL;
	}
	return strdup(name);
}


/*Access functions for the upgradeable stats of a weapon*/
UDWORD	weaponFirePause(const WEAPON_STATS* psStats, UBYTE player)
{
	if(psStats->reloadTime == 0)
	{
		return (psStats->firePause - (psStats->firePause * asWeaponUpgrade[player][
			psStats->weaponSubClass].firePause)/100);
	}
	else
	{
		return psStats->firePause;	//fire pause is neglectable for weapons with reload time
	}
}

/* Reload time is reduced for weapons with salvo fire */
UDWORD	weaponReloadTime(WEAPON_STATS *psStats, UBYTE player)
{
	return (psStats->reloadTime - (psStats->reloadTime * asWeaponUpgrade[player][
		psStats->weaponSubClass].firePause)/100);
}

UDWORD	weaponLongHit(const WEAPON_STATS* psStats, UBYTE player)
{
	return (psStats->longHit + (psStats->longHit * asWeaponUpgrade[player][
		psStats->weaponSubClass].longHit)/100);
}

UDWORD	weaponDamage(const WEAPON_STATS* psStats, UBYTE player)
{
	return (psStats->damage + (psStats->damage * asWeaponUpgrade[player][
		psStats->weaponSubClass].damage)/100);
}

UDWORD	weaponRadDamage(WEAPON_STATS *psStats, UBYTE player)
{
	return  (psStats->radiusDamage + (psStats->radiusDamage * asWeaponUpgrade[player][
		psStats->weaponSubClass].radiusDamage)/100);
}

UDWORD	weaponIncenDamage(WEAPON_STATS *psStats, UBYTE player)
{
	return (psStats->incenDamage + (psStats->incenDamage * asWeaponUpgrade[player][
		psStats->weaponSubClass].incenDamage)/100);
}

UDWORD	weaponRadiusHit(WEAPON_STATS *psStats, UBYTE player)
{
	return (psStats->radiusHit + (psStats->radiusHit * asWeaponUpgrade[player][
		psStats->weaponSubClass].radiusHit)/100);
}

UDWORD	sensorRange(SENSOR_STATS *psStats, UBYTE player)
{
	return (UWORD)(psStats->range + (psStats->range * asSensorUpgrade[player].
		range)/100);
}

/*Access functions for the upgradeable stats of a ECM*/
UDWORD	ecmRange(ECM_STATS *psStats, UBYTE player)
{
	return (UWORD)(psStats->range + (psStats->range * asECMUpgrade[player].range)/100);
}

/*Access functions for the upgradeable stats of a repair*/
UDWORD	repairPoints(REPAIR_STATS *psStats, UBYTE player)
{
	return (psStats->repairPoints + (psStats->repairPoints *
		asRepairUpgrade[player].repairPoints)/100);
}

/*Access functions for the upgradeable stats of a constructor*/
UDWORD	constructorPoints(CONSTRUCT_STATS *psStats, UBYTE player)
{
	return (psStats->constructPoints + (psStats->constructPoints *
		asConstUpgrade[player].constructPoints)/100);
}

/*Access functions for the upgradeable stats of a body*/
UDWORD	bodyPower(BODY_STATS *psStats, UBYTE player, UBYTE bodyType)
{
	return (psStats->powerOutput + (psStats->powerOutput *
		asBodyUpgrade[player][bodyType].powerOutput)/100);
}

UDWORD bodyArmour(BODY_STATS *psStats, UBYTE player, UBYTE bodyType, WEAPON_CLASS weaponClass)
{
	switch (weaponClass)
	{
	case WC_KINETIC:
	//case WC_EXPLOSIVE:
		return (psStats->armourValue[WC_KINETIC] + (psStats->armourValue[WC_KINETIC] * asBodyUpgrade[player][bodyType].armourValue[WC_KINETIC]) / 100);
	case WC_HEAT:
	//case WC_MISC:
		return (psStats->armourValue[WC_HEAT] + (psStats->armourValue[WC_HEAT] * asBodyUpgrade[player][bodyType].armourValue[WC_HEAT]) / 100);
	default:
		break;
	}
	ASSERT(false, "Unknown weapon class");
	return 0;	// Should never get here.
}

//calculates the weapons ROF based on the fire pause and the salvos
UWORD weaponROF(WEAPON_STATS *psStat, SBYTE player)
{
	UWORD rof = 0;

	//if there are salvos
	if (psStat->numRounds)
	{
		if (psStat->reloadTime != 0)
		{
			// Rounds per salvo multiplied with the number of salvos per minute
			rof = (UWORD)(psStat->numRounds * 60 * GAME_TICKS_PER_SEC  /
				(player >= 0 ? weaponReloadTime(psStat, player) : psStat->reloadTime) );
		}
	}
	if (rof == 0)
	{
		rof = (UWORD)weaponFirePause(psStat, (UBYTE)selectedPlayer);
		if (rof != 0)
		{
			rof = (UWORD)(60 * GAME_TICKS_PER_SEC / rof);
		}
		//else leave it at 0
	}
	return rof;
}

//Access functions for the max values to be used in the Design Screen
void setMaxComponentWeight(UDWORD weight)
{
    if (weight > maxComponentWeight)
    {
        maxComponentWeight = weight;
    }
}
UDWORD getMaxComponentWeight(void)
{
    return maxComponentWeight;
}

void setMaxBodyArmour(UDWORD armour)
{
    if (armour > maxBodyArmour)
    {
        maxBodyArmour = armour;
    }
}
UDWORD getMaxBodyArmour(void)
{
    return maxBodyArmour;
}

void setMaxBodyPower(UDWORD power)
{
    if (power > maxBodyPower)
    {
        maxBodyPower = power;
    }
}
UDWORD getMaxBodyPower(void)
{
    return maxBodyPower;
}

void setMaxBodyPoints(UDWORD points)
{
    if (points > maxBodyPoints)
    {
        maxBodyPoints = points;
    }
}
UDWORD getMaxBodyPoints(void)
{
    return maxBodyPoints;
}

void setMaxSensorRange(UDWORD range)
{
    if (range > maxSensorRange)
    {
        maxSensorRange = range;
    }
}

UDWORD getMaxSensorRange(void)
{
	return maxSensorRange;
}

void setMaxECMRange(UDWORD range)
{
	if (range > maxECMRange)
	{
		maxECMRange = range;
	}
}

UDWORD getMaxECMRange(void)
{
	return maxECMRange;
}

void setMaxConstPoints(UDWORD points)
{
    if (points > maxConstPoints)
    {
        maxConstPoints = points;
    }
}
UDWORD getMaxConstPoints(void)
{
    return maxConstPoints;
}

void setMaxRepairPoints(UDWORD repair)
{
    if (repair > maxRepairPoints)
    {
        maxRepairPoints = repair;
    }
}
UDWORD getMaxRepairPoints(void)
{
    return maxRepairPoints;
}

void setMaxWeaponRange(UDWORD range)
{
    if (range > maxWeaponRange)
    {
        maxWeaponRange = range;
    }
}
UDWORD getMaxWeaponRange(void)
{
    return maxWeaponRange;
}

void setMaxWeaponDamage(UDWORD damage)
{
    if (damage > maxWeaponDamage)
    {
        maxWeaponDamage = damage;
    }
}
UDWORD getMaxWeaponDamage(void)
{
    return maxWeaponDamage;
}

void setMaxWeaponROF(UDWORD rof)
{
    if (rof > maxWeaponROF)
    {
        maxWeaponROF = rof;
    }
}
UDWORD getMaxWeaponROF(void)
{
    return maxWeaponROF;
}

void setMaxPropulsionSpeed(UDWORD speed)
{
    if (speed > maxPropulsionSpeed)
    {
        maxPropulsionSpeed = speed;
    }
}
UDWORD getMaxPropulsionSpeed(void)
{
    return maxPropulsionSpeed;
}

//determine the effect this upgrade would have on the max values
void updateMaxWeaponStats(UWORD maxValue)
{
    UDWORD currentMaxValue = getMaxWeaponDamage();

    if (currentMaxValue < (currentMaxValue + maxValue / 100))
    {
        currentMaxValue += currentMaxValue * maxValue / 100;
        setMaxWeaponDamage(currentMaxValue);
    }

    //the fire pause is dealt with differently
}

void updateMaxSensorStats(UWORD maxRange)
{
    UDWORD currentMaxValue = getMaxSensorRange();

    if (currentMaxValue < (currentMaxValue + currentMaxValue * maxRange / 100))
    {
        currentMaxValue += currentMaxValue * maxRange / 100;
        setMaxSensorRange(currentMaxValue);
    }
}

void updateMaxRepairStats(UWORD maxValue)
{
    UDWORD currentMaxValue = getMaxRepairPoints();

    if (currentMaxValue < (currentMaxValue + currentMaxValue * maxValue / 100))
    {
        currentMaxValue += currentMaxValue * maxValue / 100;
        setMaxRepairPoints(currentMaxValue);
    }
}

void updateMaxECMStats(UWORD maxValue)
{
	int currentMaxValue = getMaxECMRange();

	if (currentMaxValue < (currentMaxValue + currentMaxValue * maxValue / 100))
	{
		currentMaxValue += currentMaxValue * maxValue / 100;
		setMaxECMRange(currentMaxValue);
	}
}

void updateMaxBodyStats(UWORD maxBody, UWORD maxPower, UWORD maxArmour)
{
    UDWORD currentMaxValue = getMaxBodyPoints();

    if (currentMaxValue < (currentMaxValue + currentMaxValue * maxBody / 100))
    {
        currentMaxValue += currentMaxValue * maxBody / 100;
        setMaxBodyPoints(currentMaxValue);
    }

    currentMaxValue = getMaxBodyPower();
    if (currentMaxValue < (currentMaxValue + currentMaxValue * maxPower / 100))
    {
        currentMaxValue += currentMaxValue * maxPower / 100;
        setMaxBodyPower(currentMaxValue);
    }

    currentMaxValue = getMaxBodyArmour();
    if (currentMaxValue < (currentMaxValue + currentMaxValue * maxArmour / 100))
    {
        currentMaxValue += currentMaxValue * maxArmour / 100;
        setMaxBodyArmour(currentMaxValue);
    }
}

void updateMaxConstStats(UWORD maxValue)
{
    UDWORD currentMaxValue = getMaxConstPoints();

    if (currentMaxValue < (currentMaxValue + currentMaxValue * maxValue / 100))
    {
        currentMaxValue += currentMaxValue * maxValue / 100;
        setMaxConstPoints(currentMaxValue);
    }
}

//propulsion stats are not upgradeable

void adjustMaxDesignStats(void)
{
    UWORD       weaponDamage, sensorRange, repairPoints,
                ecmRange, constPoints, bodyPoints, bodyPower, bodyArmour, inc;

	// init all the values
	weaponDamage = sensorRange = repairPoints = ecmRange = constPoints = bodyPoints = bodyPower = bodyArmour = 0;

    //go thru' all the functions getting the max upgrade values for the stats
    for (inc = 0; inc < numFunctions; inc++)
    {
        switch(asFunctions[inc]->type)
        {
        case DROIDREPAIR_UPGRADE_TYPE:
            if (repairPoints < ((UPGRADE_FUNCTION *)asFunctions[inc])->upgradePoints)
            {
                repairPoints = ((UPGRADE_FUNCTION *)asFunctions[inc])->upgradePoints;
            }
            break;
        case DROIDECM_UPGRADE_TYPE:
            if (ecmRange < ((UPGRADE_FUNCTION *)asFunctions[inc])->upgradePoints)
            {
                ecmRange = ((UPGRADE_FUNCTION *)asFunctions[inc])->upgradePoints;
            }
            break;
        case DROIDBODY_UPGRADE_TYPE:
            if (bodyPoints < ((DROIDBODY_UPGRADE_FUNCTION *)asFunctions[inc])->body)
            {
                bodyPoints = ((DROIDBODY_UPGRADE_FUNCTION *)asFunctions[inc])->body;
            }
            if (bodyPower < ((DROIDBODY_UPGRADE_FUNCTION *)asFunctions[inc])->upgradePoints)
            {
                bodyPower = ((DROIDBODY_UPGRADE_FUNCTION *)asFunctions[inc])->upgradePoints;
            }
            if (bodyArmour < ((DROIDBODY_UPGRADE_FUNCTION *)asFunctions[inc])->armourValue[WC_KINETIC])
            {
                bodyArmour = ((DROIDBODY_UPGRADE_FUNCTION *)asFunctions[inc])->armourValue[WC_KINETIC];
            }
            if (bodyArmour < ((DROIDBODY_UPGRADE_FUNCTION *)asFunctions[inc])->armourValue[WC_HEAT])
            {
                bodyArmour = ((DROIDBODY_UPGRADE_FUNCTION *)asFunctions[inc])->armourValue[WC_HEAT];
            }
            break;
        case DROIDSENSOR_UPGRADE_TYPE:
            if (sensorRange < ((DROIDSENSOR_UPGRADE_FUNCTION *)asFunctions[inc])->range)
            {
                sensorRange = ((DROIDSENSOR_UPGRADE_FUNCTION *)asFunctions[inc])->range;
            }
            break;
        case DROIDCONST_UPGRADE_TYPE:
            if (constPoints < ((UPGRADE_FUNCTION *)asFunctions[inc])->upgradePoints)
            {
                constPoints = ((UPGRADE_FUNCTION *)asFunctions[inc])->upgradePoints;
            }
            break;
        case WEAPON_UPGRADE_TYPE:
            if (weaponDamage < ((WEAPON_UPGRADE_FUNCTION *)asFunctions[inc])->damage)
            {
                weaponDamage = ((WEAPON_UPGRADE_FUNCTION *)asFunctions[inc])->damage;
            }
            break;
        default:
            //not interested in other function types
            break;
        }
    }

    //determine the effect on the max values for the stats
    updateMaxWeaponStats(weaponDamage);
    updateMaxSensorStats(sensorRange);
    updateMaxRepairStats(repairPoints);
    updateMaxECMStats(ecmRange);
    updateMaxBodyStats(bodyPoints, bodyPower, bodyArmour);
    updateMaxConstStats(constPoints);
}

/* Check if an object has a weapon */
bool objHasWeapon(const BASE_OBJECT *psObj)
{

	//check if valid type
	if(psObj->type == OBJ_DROID)
	{
		if ( ((DROID *)psObj)->numWeaps > 0 )
		{
			return true;
		}
	}
	else if(psObj->type == OBJ_STRUCTURE)
	{
		if ( ((STRUCTURE *)psObj)->numWeaps > 0 )
		{
			return true;
		}
	}

	return false;
}

SENSOR_STATS *objActiveRadar(const BASE_OBJECT *psObj)
{
	SENSOR_STATS	*psStats = NULL;
	int				compIndex;

	switch (psObj->type)
	{
	case OBJ_DROID:
		if (((DROID *)psObj)->droidType != DROID_SENSOR && ((DROID *)psObj)->droidType != DROID_COMMAND)
		{
			return NULL;
		}
		compIndex = ((DROID *)psObj)->asBits[COMP_SENSOR].nStat;
		ASSERT_OR_RETURN( NULL, compIndex < numSensorStats, "Invalid range referenced for numSensorStats, %d > %d", compIndex, numSensorStats);
		psStats = asSensorStats + compIndex;
		break;
	case OBJ_STRUCTURE:
		psStats = ((STRUCTURE *)psObj)->pStructureType->pSensor;
		if (psStats == NULL || psStats->location != LOC_TURRET || ((STRUCTURE *)psObj)->status != SS_BUILT)
		{
			return NULL;
		}
		break;
	default:
		break;
	}
	return psStats;
}

bool objRadarDetector(const BASE_OBJECT *psObj)
{
	if (psObj->type == OBJ_STRUCTURE)
	{
		STRUCTURE *psStruct = (STRUCTURE *)psObj;

		return (psStruct->status == SS_BUILT && psStruct->pStructureType->pSensor && psStruct->pStructureType->pSensor->type == RADAR_DETECTOR_SENSOR);
	}
	else if (psObj->type == OBJ_DROID)
	{
		DROID *psDroid = (DROID *)psObj;
		SENSOR_STATS *psSensor = getSensorStats(psDroid);

		return (psSensor && psSensor->type == RADAR_DETECTOR_SENSOR);
	}
	return false;
}
