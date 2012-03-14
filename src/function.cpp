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
/*
 * Function.c
 *
 * Store function stats for the Structures etc.
 *
 */
#include "lib/framework/frame.h"
#include "lib/framework/strres.h"

#include "function.h"
#include "stats.h"
#include "structure.h"
#include "text.h"
#include "research.h"
#include "droid.h"
#include "group.h"

#include "multiplay.h"


//holder for all functions
FUNCTION **asFunctions;
UDWORD numFunctions;


typedef bool (*LoadFunction)(const char *pData);


/*Returns the Function type based on the string - used for reading in data */
static UDWORD functionType(const char* pType)
{
	if (!strcmp(pType, "Production"))
	{
		return PRODUCTION_TYPE;
	}
	if (!strcmp(pType, "Production Upgrade"))
	{
		return PRODUCTION_UPGRADE_TYPE;
	}
	if (!strcmp(pType, "Research"))
	{
		return RESEARCH_TYPE;
	}
	if (!strcmp(pType, "Research Upgrade"))
	{
		return RESEARCH_UPGRADE_TYPE;
	}
	if (!strcmp(pType, "Power Generator"))
	{
		return POWER_GEN_TYPE;
	}
	if (!strcmp(pType, "Resource"))
	{
		return RESOURCE_TYPE;
	}
	if (!strcmp(pType, "Repair Droid"))
	{
		return REPAIR_DROID_TYPE;
	}
	if (!strcmp(pType, "Weapon Upgrade"))
	{
		return WEAPON_UPGRADE_TYPE;
	}
	if (!strcmp(pType, "Wall Function"))
	{
		return WALL_TYPE;
	}
	if (!strcmp(pType, "Structure Upgrade"))
	{
		return STRUCTURE_UPGRADE_TYPE;
	}
	if (!strcmp(pType, "WallDefence Upgrade"))
	{
		return WALLDEFENCE_UPGRADE_TYPE;
	}
	if (!strcmp(pType, "Power Upgrade"))
	{
		return POWER_UPGRADE_TYPE;
	}
	if (!strcmp(pType, "Repair Upgrade"))
	{
		return REPAIR_UPGRADE_TYPE;
	}
	if (!strcmp(pType, "VehicleRepair Upgrade"))
	{
		return DROIDREPAIR_UPGRADE_TYPE;
	}
	if (!strcmp(pType, "VehicleECM Upgrade"))
	{
		return DROIDECM_UPGRADE_TYPE;
	}
	if (!strcmp(pType, "VehicleConst Upgrade"))
	{
		return DROIDCONST_UPGRADE_TYPE;
	}
	if (!strcmp(pType, "VehicleBody Upgrade"))
	{
		return DROIDBODY_UPGRADE_TYPE;
	}
	if (!strcmp(pType, "VehicleSensor Upgrade"))
	{
		return DROIDSENSOR_UPGRADE_TYPE;
	}
	if (!strcmp(pType, "ReArm"))
	{
		return REARM_TYPE;
	}
	if (!strcmp(pType, "ReArm Upgrade"))
	{
		return REARM_UPGRADE_TYPE;
	}

	ASSERT( false, "Unknown Function Type: %s", pType );
	return 0;
}

// Allocate storage for the name
static bool storeName(FUNCTION* pFunction, const char* pNameToStore)
{
	pFunction->pName = strdup(pNameToStore);
	if (pFunction->pName == NULL)
	{
		debug( LOG_FATAL, "Function Name - Out of memory" );
		abort();
		return false;
	}

	return true;
}


static bool loadProduction(const char *pData)
{
	PRODUCTION_FUNCTION*	psFunction;
	char					functionName[MAX_STR_LENGTH], bodySize[MAX_STR_LENGTH];
	UDWORD					productionOutput;

	psFunction = (PRODUCTION_FUNCTION *)malloc(sizeof(PRODUCTION_FUNCTION));
	if (psFunction == NULL)
	{
		debug( LOG_FATAL, "Production Function - Out of memory" );
		abort();
		return false;
	}
	memset(psFunction, 0, sizeof(PRODUCTION_FUNCTION));

	//store the pointer in the Function Array
	*asFunctions = (FUNCTION*)psFunction;
	psFunction->ref = REF_FUNCTION_START + numFunctions;
	numFunctions++;
	asFunctions++;

	//set the type of function
	psFunction->type = PRODUCTION_TYPE;

	//read the data in
	functionName[0] = '\0';
	bodySize[0] = '\0';
	sscanf(pData, "%255[^,'\r\n],%255[^,'\r\n],%d", functionName, bodySize,
		&productionOutput);

	//allocate storage for the name
	storeName((FUNCTION *)psFunction, functionName);

	if (!getBodySize(bodySize, &psFunction->capacity))
	{
		ASSERT( false, "loadProduction: unknown body size for %s",psFunction->pName );
		return false;
	}

	//check prod output < UWORD_MAX
	if (productionOutput < UWORD_MAX)
	{
		psFunction->productionOutput = (UWORD)productionOutput;
	}
	else
	{
		ASSERT( false, "loadProduction: production Output too big for %s",psFunction->pName );

		psFunction->productionOutput = 0;
	}

	return true;
}

static bool loadProductionUpgradeFunction(const char *pData)
{
	PRODUCTION_UPGRADE_FUNCTION*	psFunction;
	char							functionName[MAX_STR_LENGTH];
	UDWORD							factory, cyborg, vtol;
	UDWORD outputModifier;

	//allocate storage
	psFunction = (PRODUCTION_UPGRADE_FUNCTION *)malloc(sizeof
		(PRODUCTION_UPGRADE_FUNCTION));
	if (psFunction == NULL)
	{
		debug( LOG_FATAL, "Production Upgrade Function - Out of memory" );
		abort();
		return false;
	}
	memset(psFunction, 0, sizeof(PRODUCTION_UPGRADE_FUNCTION));

	//store the pointer in the Function Array
	*asFunctions = (FUNCTION *)psFunction;
	psFunction->ref = REF_FUNCTION_START + numFunctions;
	numFunctions++;
	asFunctions++;

	//set the type of function
	psFunction->type = PRODUCTION_UPGRADE_TYPE;

	//read the data in
	functionName[0] = '\0';
	sscanf(pData, "%255[^,'\r\n],%d,%d,%d,%d", functionName, &factory,
		&cyborg, &vtol,&outputModifier);


	psFunction->outputModifier=(UBYTE)outputModifier;
	//allocate storage for the name
	storeName((FUNCTION *)psFunction, functionName);

	//set the factory flags
	if (factory)
	{
		psFunction->factory = true;
	}
	else
	{
		psFunction->factory = false;
	}
	if (cyborg)
	{
		psFunction->cyborgFactory = true;
	}
	else
	{
		psFunction->cyborgFactory = false;
	}
	if (vtol)
	{
		psFunction->vtolFactory = true;
	}
	else
	{
		psFunction->vtolFactory = false;
	}

	//increment the number of upgrades
	//numProductionUpgrades++;
	return true;
}

static bool loadResearchFunction(const char *pData)
{
	RESEARCH_FUNCTION*			psFunction;
	char						functionName[MAX_STR_LENGTH];

	//allocate storage
	psFunction = (RESEARCH_FUNCTION *)malloc(sizeof(RESEARCH_FUNCTION));
	if (psFunction == NULL)
	{
		debug( LOG_FATAL, "Research Function - Out of memory" );
		abort();
		return false;
	}
	memset(psFunction, 0, sizeof(RESEARCH_FUNCTION));

	//store the pointer in the Function Array
	*asFunctions = (FUNCTION *)psFunction;
	psFunction->ref = REF_FUNCTION_START + numFunctions;
	numFunctions++;
	asFunctions++;

	//set the type of function
	psFunction->type = RESEARCH_TYPE;

	//read the data in
	functionName[0] = '\0';
	sscanf(pData, "%255[^,'\r\n],%d", functionName, &psFunction->researchPoints);

	//allocate storage for the name
	storeName((FUNCTION *)psFunction, functionName);

	return true;
}

static bool loadReArmFunction(const char *pData)
{
	REARM_FUNCTION*				psFunction;
	char						functionName[MAX_STR_LENGTH];

	//allocate storage
	psFunction = (REARM_FUNCTION *)malloc(sizeof(REARM_FUNCTION));
	if (psFunction == NULL)
	{
		debug( LOG_FATAL, "ReArm Function - Out of memory" );
		abort();
		return false;
	}
	memset(psFunction, 0, sizeof(REARM_FUNCTION));

	//store the pointer in the Function Array
	*asFunctions = (FUNCTION *)psFunction;
	psFunction->ref = REF_FUNCTION_START + numFunctions;
	numFunctions++;
	asFunctions++;

	//set the type of function
	psFunction->type = REARM_TYPE;

	//read the data in
	functionName[0] = '\0';
	sscanf(pData, "%255[^,'\r\n],%d", functionName, &psFunction->reArmPoints);

	//allocate storage for the name
	storeName((FUNCTION *)psFunction, functionName);

	return true;
}


//generic load function for upgrade type
static bool loadUpgradeFunction(const char *pData, UBYTE type)
{
	char						functionName[MAX_STR_LENGTH];
	UDWORD						modifier;
	UPGRADE_FUNCTION			*psFunction;

	//allocate storage
	psFunction = (UPGRADE_FUNCTION *)malloc(sizeof(UPGRADE_FUNCTION));
	if (psFunction == NULL)
	{
		debug( LOG_FATAL, "Upgrade Function - Out of memory" );
		abort();
		return false;
	}
	memset(psFunction, 0, sizeof(UPGRADE_FUNCTION));

	//store the pointer in the Function Array
	*asFunctions = (FUNCTION *)psFunction;
	psFunction->ref = REF_FUNCTION_START + numFunctions;
	numFunctions++;
	asFunctions++;

	//set the type of function
	psFunction->type = type;

	//read the data in
	functionName[0] = '\0';
	sscanf(pData, "%255[^,'\r\n],%d", functionName, &modifier);

	//allocate storage for the name
	storeName((FUNCTION *)psFunction, functionName);

	if (modifier > UWORD_MAX)
	{
		ASSERT( false, "loadUpgradeFunction: modifier too great for %s", functionName );
		return false;
	}

	//store the % upgrade
	psFunction->upgradePoints = (UWORD)modifier;

	return true;
}



static bool loadResearchUpgradeFunction(const char *pData)
{
	return loadUpgradeFunction(pData, RESEARCH_UPGRADE_TYPE);
}

static bool loadPowerUpgradeFunction(const char *pData)
{
	return loadUpgradeFunction(pData, POWER_UPGRADE_TYPE);
}

static bool loadRepairUpgradeFunction(const char *pData)
{
	return loadUpgradeFunction(pData, REPAIR_UPGRADE_TYPE);
}

static bool loadDroidRepairUpgradeFunction(const char *pData)
{
	return loadUpgradeFunction(pData, DROIDREPAIR_UPGRADE_TYPE);
}

static bool loadDroidECMUpgradeFunction(const char *pData)
{
	return loadUpgradeFunction(pData, DROIDECM_UPGRADE_TYPE);
}

static bool loadDroidConstUpgradeFunction(const char *pData)
{
	return loadUpgradeFunction(pData, DROIDCONST_UPGRADE_TYPE);
}

static bool loadReArmUpgradeFunction(const char *pData)
{
	return loadUpgradeFunction(pData, REARM_UPGRADE_TYPE);
}


static bool loadDroidBodyUpgradeFunction(const char *pData)
{
	DROIDBODY_UPGRADE_FUNCTION		*psFunction;
	char							functionName[MAX_STR_LENGTH];
	UDWORD							modifier, armourKinetic, armourHeat,
									body, droid, cyborg;

	//allocate storage
	psFunction = (DROIDBODY_UPGRADE_FUNCTION *)malloc(
		sizeof(DROIDBODY_UPGRADE_FUNCTION));
	if (psFunction == NULL)
	{
		debug( LOG_FATAL, "UnitBody Upgrade Function - Out of memory" );
		abort();
		return false;
	}
	memset(psFunction, 0, sizeof(DROIDBODY_UPGRADE_FUNCTION));

	//store the pointer in the Function Array
	*asFunctions = (FUNCTION *)psFunction;
	psFunction->ref = REF_FUNCTION_START + numFunctions;
	numFunctions++;
	asFunctions++;

	//set the type of function
	psFunction->type = DROIDBODY_UPGRADE_TYPE;

	//read the data in
	functionName[0] = '\0';
	sscanf(pData, "%255[^,'\r\n],%d,%d,%d,%d,%d,%d", functionName, &modifier,
		&body, &armourKinetic,	&armourHeat, &droid, &cyborg);

	//allocate storage for the name
	storeName((FUNCTION *)psFunction, functionName);

	if (modifier > UWORD_MAX || armourKinetic > UWORD_MAX ||
		armourHeat > UWORD_MAX || body > UWORD_MAX)
	{
		ASSERT( false,
			"loadUnitBodyUpgradeFunction: one or more modifiers too great" );
		return false;
	}

	//store the % upgrades
	psFunction->upgradePoints = (UWORD)modifier;
	psFunction->body = (UWORD)body;
	psFunction->armourValue[WC_KINETIC] = (UWORD)armourKinetic;
	psFunction->armourValue[WC_HEAT] = (UWORD)armourHeat;
	if (droid)
	{
		psFunction->droid = true;
	}
	else
	{
		psFunction->droid = false;
	}
	if (cyborg)
	{
		psFunction->cyborg = true;
	}
	else
	{
		psFunction->cyborg = false;
	}

	return true;
}

static bool loadDroidSensorUpgradeFunction(const char *pData)
{
	DROIDSENSOR_UPGRADE_FUNCTION	*psFunction;
	char							functionName[MAX_STR_LENGTH];
	UDWORD							modifier, range;

	//allocate storage
	psFunction = (DROIDSENSOR_UPGRADE_FUNCTION *)malloc(
		sizeof(DROIDSENSOR_UPGRADE_FUNCTION));
	if (psFunction == NULL)
	{
		debug( LOG_FATAL, "UnitSensor Upgrade Function - Out of memory" );
		abort();
		return false;
	}
	memset(psFunction, 0, sizeof(DROIDSENSOR_UPGRADE_FUNCTION));

	//store the pointer in the Function Array
	*asFunctions = (FUNCTION *)psFunction;
	psFunction->ref = REF_FUNCTION_START + numFunctions;
	numFunctions++;
	asFunctions++;

	//set the type of function
	psFunction->type = DROIDSENSOR_UPGRADE_TYPE;

	//read the data in
	functionName[0] = '\0';
	sscanf(pData, "%255[^,'\r\n],%d,%d", functionName, &modifier, &range);

	//allocate storage for the name
	storeName((FUNCTION *)psFunction, functionName);

	if (modifier > UWORD_MAX || range > UWORD_MAX)
	{
		ASSERT( false,
			"loadUnitSensorUpgradeFunction: one or more modifiers too great" );
		return false;
	}

	//store the % upgrades
	psFunction->upgradePoints = (UWORD)modifier;
	psFunction->range = (UWORD)range;

    return true;
}

static bool loadWeaponUpgradeFunction(const char *pData)
{
	WEAPON_UPGRADE_FUNCTION*	psFunction;
	char						functionName[MAX_STR_LENGTH],
								weaponSubClass[MAX_STR_LENGTH];
	UDWORD						firePause, shortHit, longHit, damage,
								radiusDamage, incenDamage, radiusHit;

	//allocate storage
	psFunction = (WEAPON_UPGRADE_FUNCTION *)malloc(sizeof
		(WEAPON_UPGRADE_FUNCTION));
	if (psFunction == NULL)
	{
		debug( LOG_FATAL, "Weapon Upgrade Function - Out of memory" );
		abort();
		return false;
	}
	memset(psFunction, 0, sizeof(WEAPON_UPGRADE_FUNCTION));

	//store the pointer in the Function Array
	*asFunctions = (FUNCTION *)psFunction;
	psFunction->ref = REF_FUNCTION_START + numFunctions;
	numFunctions++;
	asFunctions++;

	//set the type of function
	psFunction->type = WEAPON_UPGRADE_TYPE;

	//read the data in
	functionName[0] = '\0';
	weaponSubClass[0] = '\0';
	sscanf(pData, "%255[^,'\r\n],%255[^,'\r\n],%d,%d,%d,%d,%d,%d,%d", functionName,
		weaponSubClass, &firePause, &shortHit, &longHit, &damage, &radiusDamage,
		&incenDamage, &radiusHit);

	//allocate storage for the name
	storeName((FUNCTION *)psFunction, functionName);

	if (!getWeaponSubClass(weaponSubClass, &psFunction->subClass))
	{
		return false;
	}

	//check none of the %increases are over UBYTE max
	if (firePause > UBYTE_MAX ||
		shortHit > UWORD_MAX ||
		longHit > UWORD_MAX ||
		damage > UWORD_MAX ||
		radiusDamage > UWORD_MAX ||
		incenDamage > UWORD_MAX ||
		radiusHit > UWORD_MAX)
	{
		debug( LOG_ERROR, "A percentage increase for Weapon Upgrade function is too large" );

		return false;
	}

	//copy the data across
	psFunction->firePause = (UBYTE)firePause;
	psFunction->shortHit = (UWORD)shortHit;
	psFunction->longHit = (UWORD)longHit;
	psFunction->damage = (UWORD)damage;
	psFunction->radiusDamage = (UWORD)radiusDamage;
	psFunction->incenDamage = (UWORD)incenDamage;
	psFunction->radiusHit = (UWORD)radiusHit;

	//increment the number of upgrades
	//numWeaponUpgrades++;

    return true;
}

static bool loadStructureUpgradeFunction(const char *pData)
{
	STRUCTURE_UPGRADE_FUNCTION  *psFunction;
	char						functionName[MAX_STR_LENGTH];
	UDWORD						armour, body, resistance;

	//allocate storage
	psFunction = (STRUCTURE_UPGRADE_FUNCTION *)malloc(sizeof
		(STRUCTURE_UPGRADE_FUNCTION));
	if (psFunction == NULL)
	{
		debug( LOG_FATAL, "Structure Upgrade Function - Out of memory" );
		abort();
		return false;
	}
	memset(psFunction, 0, sizeof(STRUCTURE_UPGRADE_FUNCTION));

	//store the pointer in the Function Array
	*asFunctions = (FUNCTION *)psFunction;
	psFunction->ref = REF_FUNCTION_START + numFunctions;
	numFunctions++;
	asFunctions++;

	//set the type of function
	psFunction->type = STRUCTURE_UPGRADE_TYPE;

	//read the data in
	functionName[0] = '\0';
	sscanf(pData, "%255[^,'\r\n],%d,%d,%d", functionName, &armour, &body, &resistance);

	//allocate storage for the name
	storeName((FUNCTION *)psFunction, functionName);

	//check none of the %increases are over UWORD max
	if (armour > UWORD_MAX ||
		body > UWORD_MAX ||
		resistance > UWORD_MAX)
	{
		debug( LOG_ERROR, "A percentage increase for Structure Upgrade function is too large" );

		return false;
	}

	//copy the data across
	psFunction->armour = (UWORD)armour;
	psFunction->body = (UWORD)body;
	psFunction->resistance = (UWORD)resistance;

	return true;
}

static bool loadWallDefenceUpgradeFunction(const char *pData)
{
	WALLDEFENCE_UPGRADE_FUNCTION  *psFunction;
	char						functionName[MAX_STR_LENGTH];
	UDWORD						armour, body;

	//allocate storage
	psFunction = (WALLDEFENCE_UPGRADE_FUNCTION *)malloc(sizeof
		(WALLDEFENCE_UPGRADE_FUNCTION));
	if (psFunction == NULL)
	{
		debug( LOG_FATAL, "WallDefence Upgrade Function - Out of memory" );
		abort();
		return false;
	}
	memset(psFunction, 0, sizeof(WALLDEFENCE_UPGRADE_FUNCTION));

	//store the pointer in the Function Array
	*asFunctions = (FUNCTION *)psFunction;
	psFunction->ref = REF_FUNCTION_START + numFunctions;
	numFunctions++;
	asFunctions++;

	//set the type of function
	psFunction->type = WALLDEFENCE_UPGRADE_TYPE;

	//read the data in
	functionName[0] = '\0';
	sscanf(pData, "%255[^,'\r\n],%d,%d", functionName, &armour, &body);

	//allocate storage for the name
	storeName((FUNCTION *)psFunction, functionName);

	//check none of the %increases are over UWORD max
	if (armour > UWORD_MAX ||
		body > UWORD_MAX)
	{
		debug( LOG_ERROR, "A percentage increase for WallDefence Upgrade function is too large" );

		return false;
	}

	//copy the data across
	psFunction->armour = (UWORD)armour;
	psFunction->body = (UWORD)body;

	return true;
}


static bool loadPowerGenFunction(const char *pData)
{
	POWER_GEN_FUNCTION*			psFunction;
	char						functionName[MAX_STR_LENGTH];

	//allocate storage
	psFunction = (POWER_GEN_FUNCTION *)malloc(sizeof
		(POWER_GEN_FUNCTION));
	if (psFunction == NULL)
	{
		debug( LOG_FATAL, "Power Gen Function - Out of memory" );
		abort();
		return false;
	}
	memset(psFunction, 0, sizeof(POWER_GEN_FUNCTION));

	//store the pointer in the Function Array
	*asFunctions = (FUNCTION *)psFunction;
	psFunction->ref = REF_FUNCTION_START + numFunctions;
	numFunctions++;
	asFunctions++;

	//set the type of function
	psFunction->type = POWER_GEN_TYPE;

	//read the data in
	functionName[0] = '\0';
	sscanf(pData, "%255[^,'\r\n],%d,%d,%d,%d,%d,%d", functionName,
		&psFunction->powerOutput, &psFunction->powerMultiplier,
		&psFunction->criticalMassChance, &psFunction->criticalMassRadius,
		&psFunction->criticalMassDamage, &psFunction->radiationDecayTime);


	if(bMultiPlayer)
	{
		switch(game.power)
		{
			// Multiply by 3/4
			case LEV_LOW:
				psFunction->powerMultiplier *= 3;
				psFunction->powerMultiplier /= 4;
				break;
			// No change
			case LEV_MED:
				break;
			// Multiply by 5/4
			case LEV_HI:
				psFunction->powerMultiplier *= 5;
				psFunction->powerMultiplier /= 4;
				break;
			default:
				break;
		}
	}


	//allocate storage for the name
	storeName((FUNCTION *)psFunction, functionName);

	return true;
}

static bool loadResourceFunction(const char *pData)
{
	RESOURCE_FUNCTION			*psFunction;
	char						functionName[MAX_STR_LENGTH];

	//allocate storage
	psFunction = (RESOURCE_FUNCTION *)malloc(sizeof
		(RESOURCE_FUNCTION));
	if (psFunction == NULL)
	{
		debug( LOG_FATAL, "Resource Function - Out of memory" );
		abort();
		return false;
	}
	memset(psFunction, 0, sizeof(RESOURCE_FUNCTION));

	//store the pointer in the Function Array
	*asFunctions = (FUNCTION *)psFunction;
	psFunction->ref = REF_FUNCTION_START + numFunctions;
	numFunctions++;
	asFunctions++;

	//set the type of function
 	psFunction->type = RESOURCE_TYPE;

	//read the data in
	functionName[0] = '\0';
	sscanf(pData, "%255[^,'\r\n],%d", functionName, &psFunction->maxPower);

	//allocate storage for the name
	storeName((FUNCTION *)psFunction, functionName);

	return true;
}


static bool loadRepairDroidFunction(const char *pData)
{
	REPAIR_DROID_FUNCTION*		psFunction;
	char						functionName[MAX_STR_LENGTH];

	//allocate storage
	psFunction = (REPAIR_DROID_FUNCTION *)malloc(sizeof
		(REPAIR_DROID_FUNCTION));
	if (psFunction == NULL)
	{
		debug( LOG_FATAL, "Repair Droid Function - Out of memory" );
		abort();
		return false;
	}
	memset(psFunction, 0, sizeof(REPAIR_DROID_FUNCTION));

	//store the pointer in the Function Array
	*asFunctions = (FUNCTION *)psFunction;
	psFunction->ref = REF_FUNCTION_START + numFunctions;
	numFunctions++;
	asFunctions++;

	//set the type of function
	psFunction->type = REPAIR_DROID_TYPE;

	//read the data in
	functionName[0] = '\0';
	sscanf(pData, "%255[^,'\r\n],%d", functionName,
		&psFunction->repairPoints);

	//allocate storage for the name
	storeName((FUNCTION *)psFunction, functionName);

	return true;
}


/*loads the corner stat to use for a particular wall stat */
static bool loadWallFunction(const char *pData)
{
	WALL_FUNCTION			*psFunction;
//	UDWORD					i;
	char					functionName[MAX_STR_LENGTH];
	char					structureName[MAX_STR_LENGTH];
//	STRUCTURE_STATS			*pStructStat;

	//allocate storage
	psFunction = (WALL_FUNCTION *)malloc(sizeof(WALL_FUNCTION));
	if (psFunction == NULL)
	{
		debug( LOG_FATAL, "Wall Function - Out of memory" );
		abort();
		return false;
	}
	memset(psFunction, 0, sizeof(WALL_FUNCTION));

	//store the pointer in the Function Array
	*asFunctions = (FUNCTION*)psFunction;
	psFunction->ref = REF_FUNCTION_START + numFunctions;
	numFunctions++;
	asFunctions++;

	//set the type of function
	psFunction->type = WALL_TYPE;

	//read the data in
	functionName[0] = '\0';
	structureName[0] = '\0';
	sscanf(pData, "%255[^,'\r\n],%255[^,'\r\n],%*d", functionName, structureName);

	//allocate storage for the name
	storeName((FUNCTION *)psFunction, functionName);

	//store the structure name - cannot set the stat pointer here because structures
	//haven't been loaded in yet!
	psFunction->pStructName = allocateName(structureName);
	if (!psFunction->pStructName)
	{
		debug( LOG_ERROR, "Structure Stats Invalid for function - %s", functionName );

		return false;
	}
	psFunction->pCornerStat = NULL;

	return true;
}

void productionUpgrade(FUNCTION *pFunction, UBYTE player)
{
	PRODUCTION_UPGRADE_FUNCTION		*pUpgrade;

	pUpgrade = (PRODUCTION_UPGRADE_FUNCTION	*)pFunction;

	//check upgrades increase all values
	if (pUpgrade->factory)
	{
		if (asProductionUpgrade[player][FACTORY_FLAG].modifier <
			pUpgrade->outputModifier)
		{
			asProductionUpgrade[player][FACTORY_FLAG].modifier =
				pUpgrade->outputModifier;
		}
	}
	if (pUpgrade->cyborgFactory)
	{
		if (asProductionUpgrade[player][CYBORG_FLAG].modifier <
			pUpgrade->outputModifier)
		{
			asProductionUpgrade[player][CYBORG_FLAG].modifier =
				pUpgrade->outputModifier;
		}
	}
	if (pUpgrade->vtolFactory)
	{
		if (asProductionUpgrade[player][VTOL_FLAG].modifier <
			pUpgrade->outputModifier)
		{
			asProductionUpgrade[player][VTOL_FLAG].modifier =
				pUpgrade->outputModifier;
		}
	}
}

void researchUpgrade(FUNCTION *pFunction, UBYTE player)
{
	RESEARCH_UPGRADE_FUNCTION		*pUpgrade;

	pUpgrade = (RESEARCH_UPGRADE_FUNCTION	*)pFunction;

	//check upgrades increase all values
	if (asResearchUpgrade[player].modifier < pUpgrade->upgradePoints)
	{
		asResearchUpgrade[player].modifier = pUpgrade->upgradePoints;
	}
}

void repairFacUpgrade(FUNCTION *pFunction, UBYTE player)
{
	REPAIR_UPGRADE_FUNCTION		*pUpgrade;

	pUpgrade = (REPAIR_UPGRADE_FUNCTION	*)pFunction;

	//check upgrades increase all values
	if (asRepairFacUpgrade[player].modifier < pUpgrade->upgradePoints)
	{
		asRepairFacUpgrade[player].modifier = pUpgrade->upgradePoints;
	}
}

void powerUpgrade(FUNCTION *pFunction, UBYTE player)
{
	POWER_UPGRADE_FUNCTION		*pUpgrade;

	pUpgrade = (POWER_UPGRADE_FUNCTION	*)pFunction;

	//check upgrades increase all values
	if (asPowerUpgrade[player].modifier < pUpgrade->upgradePoints)
	{
		asPowerUpgrade[player].modifier = pUpgrade->upgradePoints;
	}
}

void reArmUpgrade(FUNCTION *pFunction, UBYTE player)
{
	REARM_UPGRADE_FUNCTION		*pUpgrade;

	pUpgrade = (REARM_UPGRADE_FUNCTION *)pFunction;

	//check upgrades increase all values
	if (asReArmUpgrade[player].modifier < pUpgrade->upgradePoints)
	{
		asReArmUpgrade[player].modifier = pUpgrade->upgradePoints;
	}
}

void structureBodyUpgrade(FUNCTION *pFunction, STRUCTURE *psBuilding)
{
	UWORD	increase, prevBaseBody, newBaseBody;

	switch(psBuilding->pStructureType->type)
	{
	case REF_WALL:
	case REF_WALLCORNER:
	case REF_DEFENSE:
	case REF_BLASTDOOR:
	case REF_GATE:
		increase = ((WALLDEFENCE_UPGRADE_FUNCTION *)pFunction)->body;
		break;
	default:
		increase = ((STRUCTURE_UPGRADE_FUNCTION *)pFunction)->body;
		break;
	}

	prevBaseBody = (UWORD)structureBody(psBuilding);
	//newBaseBody = (UWORD)(psBuilding->pStructureType->bodyPoints + (psBuilding->
	//	pStructureType->bodyPoints * increase) / 100);
	newBaseBody = (UWORD)(structureBaseBody(psBuilding) +
		(structureBaseBody(psBuilding) * increase) / 100);

	if (newBaseBody > prevBaseBody)
	{
		psBuilding->body = (UWORD)((psBuilding->body * newBaseBody) / prevBaseBody);
		//psBuilding->baseBodyPoints = newBaseBody;
	}
}

void structureArmourUpgrade(FUNCTION *pFunction, STRUCTURE *psBuilding)
{
	UWORD	increase, prevBaseArmour, newBaseArmour;

	switch(psBuilding->pStructureType->type)
	{
	case REF_WALL:
	case REF_WALLCORNER:
	case REF_DEFENSE:
	case REF_BLASTDOOR:
	case REF_GATE:
		increase = ((WALLDEFENCE_UPGRADE_FUNCTION *)pFunction)->armour;
		break;
	default:
		increase = ((STRUCTURE_UPGRADE_FUNCTION *)pFunction)->armour;
		break;
	}

	prevBaseArmour = (UWORD)structureArmour(psBuilding->pStructureType, psBuilding->player);
	newBaseArmour = (UWORD)(psBuilding->pStructureType->armourValue + (psBuilding->
		pStructureType->armourValue * increase) / 100);

	if (newBaseArmour > prevBaseArmour)
	{
		for (int j = 0; j < WC_NUM_WEAPON_CLASSES; j++)
		{
			psBuilding->armour[j] = (UWORD)((psBuilding->armour[j] * newBaseArmour) / prevBaseArmour);
		}
	}
}

void structureResistanceUpgrade(FUNCTION *pFunction, STRUCTURE *psBuilding)
{
	UWORD	increase, prevBaseResistance, newBaseResistance;

	increase = ((STRUCTURE_UPGRADE_FUNCTION *)pFunction)->resistance;

	prevBaseResistance = (UWORD)structureResistance(psBuilding->pStructureType,
		psBuilding->player);
	newBaseResistance = (UWORD)(psBuilding->pStructureType->resistance + (psBuilding
		->pStructureType->resistance * increase) / 100);

	if (newBaseResistance > prevBaseResistance)
	{
		psBuilding->resistance = (UWORD)((psBuilding->resistance * newBaseResistance) /
			prevBaseResistance);
	}
}

void structureProductionUpgrade(STRUCTURE *psBuilding)
{
	FACTORY						*pFact;
	PRODUCTION_FUNCTION			*pFactFunc;
	UDWORD						type, baseOutput, i;
    STRUCTURE_STATS             *psStat;

	switch(psBuilding->pStructureType->type)
	{
	case REF_FACTORY:
		type = FACTORY_FLAG;
		break;
	case REF_CYBORG_FACTORY:
		type = CYBORG_FLAG;
		break;
	case REF_VTOL_FACTORY:
		type = VTOL_FLAG;
		break;
	default:
		ASSERT(!"invalid or not a factory type", "structureProductionUpgrade: Invalid factory type");
		return;
	}

	//upgrade the Output
	pFact = &psBuilding->pFunctionality->factory;
	ASSERT( pFact != NULL,
		"structureProductionUpgrade: invalid Factory pointer" );

	pFactFunc = (PRODUCTION_FUNCTION *)psBuilding->pStructureType->asFuncList[0];
	ASSERT( pFactFunc != NULL,
		"structureProductionUpgrade: invalid Function pointer" );

    //current base value depends on whether there are modules attached to the structure
    baseOutput = pFactFunc->productionOutput;
    psStat = getModuleStat(psBuilding);
    if (psStat)
    {
        for (i = 0; i < pFact->capacity; i++)
        {
            baseOutput += ((PRODUCTION_FUNCTION*)psStat->asFuncList[0])->productionOutput;
        }
    }

	pFact->productionOutput = (UBYTE)(baseOutput + (pFactFunc->productionOutput *
		asProductionUpgrade[psBuilding->player][type].modifier) / 100);
}

void structureResearchUpgrade(STRUCTURE *psBuilding)
{
	RESEARCH_FACILITY			*pRes = &psBuilding->pFunctionality->researchFacility;
	RESEARCH_FUNCTION			*pResFunc;
    UDWORD                       baseOutput;
    STRUCTURE_STATS             *psStat;

	//upgrade the research points
	ASSERT(pRes != NULL, "structureResearchUpgrade: invalid Research pointer");

	pResFunc = (RESEARCH_FUNCTION *)psBuilding->pStructureType->asFuncList[0];
	ASSERT( pResFunc != NULL,
		"structureResearchUpgrade: invalid Function pointer" );

    //current base value depends on whether there are modules attached to the structure
    baseOutput = pResFunc->researchPoints;
    psStat = getModuleStat(psBuilding);
    if (psStat)
    {
        if (pRes->capacity)
        {
            baseOutput += ((RESEARCH_FUNCTION*)psStat->asFuncList[0])->researchPoints;
        }
    }
	pRes->researchPoints = baseOutput + (pResFunc->researchPoints *
		asResearchUpgrade[psBuilding->player].modifier) / 100;
}

void structureReArmUpgrade(STRUCTURE *psBuilding)
{
	REARM_PAD					*pPad = &psBuilding->pFunctionality->rearmPad;
	REARM_FUNCTION				*pPadFunc;

	//upgrade the reArm points
	ASSERT(pPad != NULL, "structureReArmUpgrade: invalid ReArm pointer");

	pPadFunc = (REARM_FUNCTION *)psBuilding->pStructureType->asFuncList[0];
	ASSERT( pPadFunc != NULL,
		"structureReArmUpgrade: invalid Function pointer" );

	pPad->reArmPoints = pPadFunc->reArmPoints + (pPadFunc->reArmPoints *
		asReArmUpgrade[psBuilding->player].modifier) / 100;
}

void structurePowerUpgrade(STRUCTURE *psBuilding)
{
	POWER_GEN		*pPowerGen = &psBuilding->pFunctionality->powerGenerator;
	POWER_GEN_FUNCTION	*pPGFunc = (POWER_GEN_FUNCTION *)psBuilding->pStructureType->asFuncList[0];
	UDWORD			multiplier;
	STRUCTURE_STATS		*psStat;

	ASSERT(pPowerGen != NULL, "Invalid Power Gen pointer");
	ASSERT(pPGFunc != NULL, "Invalid function pointer");

	// Current base value depends on whether there are modules attached to the structure
	multiplier = pPGFunc->powerMultiplier;
	psStat = getModuleStat(psBuilding);
	if (psStat)
	{
		if (pPowerGen->capacity)
		{
			multiplier += ((POWER_GEN_FUNCTION*)psStat->asFuncList[0])->powerMultiplier;
		}
	}
	pPowerGen->multiplier = multiplier + (pPGFunc->powerMultiplier * asPowerUpgrade[psBuilding->player].modifier) / 100;
}

void structureRepairUpgrade(STRUCTURE *psBuilding)
{
	REPAIR_FACILITY			*pRepair = &psBuilding->pFunctionality->repairFacility;
	REPAIR_DROID_FUNCTION	*pRepairFunc;

	//upgrade the research points
	ASSERT(pRepair != NULL, "structureRepairUpgrade: invalid Repair pointer");

	pRepairFunc = (REPAIR_DROID_FUNCTION *)psBuilding->pStructureType->asFuncList[0];
	ASSERT( pRepairFunc != NULL,
		"structureRepairUpgrade: invalid Function pointer" );

	pRepair->power = pRepairFunc->repairPoints + (pRepairFunc->repairPoints *
		asRepairFacUpgrade[psBuilding->player].modifier) / 100;
}

void structureSensorUpgrade(STRUCTURE *psBuilding)
{
	objSensorCache((BASE_OBJECT *)psBuilding, psBuilding->pStructureType->pSensor);
}

void structureECMUpgrade(STRUCTURE *psBuilding)
{
	objEcmCache((BASE_OBJECT *)psBuilding, psBuilding->pStructureType->pECM);
}

void droidSensorUpgrade(DROID *psDroid)
{
	objSensorCache((BASE_OBJECT *)psDroid, asSensorStats + psDroid->asBits[COMP_SENSOR].nStat);
}

void droidECMUpgrade(DROID *psDroid)
{
	objEcmCache((BASE_OBJECT *)psDroid, asECMStats + psDroid->asBits[COMP_ECM].nStat);
}

void droidBodyUpgrade(FUNCTION *pFunction, DROID *psDroid)
{
	UDWORD	increase, prevBaseBody, newBaseBody, base;
    DROID   *psCurr;

	increase = ((DROIDBODY_UPGRADE_FUNCTION*)pFunction)->body;

	prevBaseBody = psDroid->originalBody;
	base = calcDroidBaseBody(psDroid);
	newBaseBody =  base + (base * increase) / 100;

	if (newBaseBody > prevBaseBody)
	{
		psDroid->body = (psDroid->body * newBaseBody) / prevBaseBody;
		psDroid->originalBody = newBaseBody;
	}
    //if a transporter droid then need to upgrade the contents
    if (psDroid->droidType == DROID_TRANSPORTER || psDroid->droidType == DROID_SUPERTRANSPORTER)
    {
        for (psCurr = psDroid->psGroup->psList; psCurr != NULL; psCurr =
            psCurr->psGrpNext)
        {
            if (psCurr != psDroid)
            {
                droidBodyUpgrade(pFunction, psCurr);
            }
        }
    }
}

//upgrade the weapon stats for the correct subclass
void weaponUpgrade(FUNCTION *pFunction, UBYTE player)
{
	WEAPON_UPGRADE_FUNCTION		*pUpgrade;

	pUpgrade = (WEAPON_UPGRADE_FUNCTION *)pFunction;

	//check upgrades increase all values!
	if (asWeaponUpgrade[player][pUpgrade->subClass].firePause < pUpgrade->firePause)
	{
        //make sure don't go less than 100%
        if (pUpgrade->firePause > 100)
        {
            pUpgrade->firePause = 100;
        }
		asWeaponUpgrade[player][pUpgrade->subClass].firePause = pUpgrade->firePause;
	}
	if (asWeaponUpgrade[player][pUpgrade->subClass].shortHit < pUpgrade->shortHit)
	{
		asWeaponUpgrade[player][pUpgrade->subClass].shortHit = pUpgrade->shortHit;
	}
	if (asWeaponUpgrade[player][pUpgrade->subClass].longHit < pUpgrade->longHit)
	{
		asWeaponUpgrade[player][pUpgrade->subClass].longHit = pUpgrade->longHit;
	}
	if (asWeaponUpgrade[player][pUpgrade->subClass].damage < pUpgrade->damage)
	{
		asWeaponUpgrade[player][pUpgrade->subClass].damage = pUpgrade->damage;
	}
	if (asWeaponUpgrade[player][pUpgrade->subClass].radiusDamage < pUpgrade->radiusDamage)
	{
		asWeaponUpgrade[player][pUpgrade->subClass].radiusDamage = pUpgrade->radiusDamage;
	}
	if (asWeaponUpgrade[player][pUpgrade->subClass].incenDamage < pUpgrade->incenDamage)
	{
		asWeaponUpgrade[player][pUpgrade->subClass].incenDamage = pUpgrade->incenDamage;
	}
	if (asWeaponUpgrade[player][pUpgrade->subClass].radiusHit < pUpgrade->radiusHit)
	{
		asWeaponUpgrade[player][pUpgrade->subClass].radiusHit = pUpgrade->radiusHit;
	}
}

//upgrade the sensor stats
void sensorUpgrade(FUNCTION *pFunction, UBYTE player)
{
	DROIDSENSOR_UPGRADE_FUNCTION		*pUpgrade;

	pUpgrade = (DROIDSENSOR_UPGRADE_FUNCTION *)pFunction;

	//check upgrades increase all values!
	if (asSensorUpgrade[player].range < pUpgrade->range)
	{
		asSensorUpgrade[player].range = pUpgrade->range;
	}
	if (asSensorUpgrade[player].power < pUpgrade->upgradePoints)
	{
		asSensorUpgrade[player].power = pUpgrade->upgradePoints;
	}
}

//upgrade the repair stats
void repairUpgrade(FUNCTION *pFunction, UBYTE player)
{
	DROIDREPAIR_UPGRADE_FUNCTION		*pUpgrade;

	pUpgrade = (DROIDREPAIR_UPGRADE_FUNCTION *)pFunction;

	//check upgrades increase all values!
	if (asRepairUpgrade[player].repairPoints < pUpgrade->upgradePoints)
	{
		asRepairUpgrade[player].repairPoints = pUpgrade->upgradePoints;
	}
}

//upgrade the repair stats
void ecmUpgrade(FUNCTION *pFunction, UBYTE player)
{
	DROIDECM_UPGRADE_FUNCTION		*pUpgrade;

	pUpgrade = (DROIDECM_UPGRADE_FUNCTION *)pFunction;

	//check upgrades increase all values!
	if (asECMUpgrade[player].power < pUpgrade->upgradePoints)
	{
		asECMUpgrade[player].power = pUpgrade->upgradePoints;
	}
}

//upgrade the repair stats
void constructorUpgrade(FUNCTION *pFunction, UBYTE player)
{
	DROIDCONSTR_UPGRADE_FUNCTION		*pUpgrade;

	pUpgrade = (DROIDCONSTR_UPGRADE_FUNCTION *)pFunction;

	//check upgrades increase all values!
	if (asConstUpgrade[player].constructPoints < pUpgrade->upgradePoints)
	{
		asConstUpgrade[player].constructPoints = pUpgrade->upgradePoints;
	}
}

//upgrade the body stats
void bodyUpgrade(FUNCTION *pFunction, UBYTE player)
{
	DROIDBODY_UPGRADE_FUNCTION		*pUpgrade;
	UBYTE							inc;

	pUpgrade = (DROIDBODY_UPGRADE_FUNCTION *)pFunction;

	//check upgrades increase all values!
	if (pUpgrade->droid)
	{
		if (asBodyUpgrade[player][DROID_BODY_UPGRADE].powerOutput <
			pUpgrade->upgradePoints)
		{
			asBodyUpgrade[player][DROID_BODY_UPGRADE].powerOutput =
				pUpgrade->upgradePoints;
		}
		if (asBodyUpgrade[player][DROID_BODY_UPGRADE].body <
			pUpgrade->body)
		{
			asBodyUpgrade[player][DROID_BODY_UPGRADE].body =
				pUpgrade->body;
		}
		for (inc=0; inc < WC_NUM_WEAPON_CLASSES; inc++)
		{
			if (asBodyUpgrade[player][DROID_BODY_UPGRADE].armourValue[inc] <
				pUpgrade->armourValue[inc])
			{
				asBodyUpgrade[player][DROID_BODY_UPGRADE].armourValue[inc] =
					pUpgrade->armourValue[inc];
			}
		}
	}
	if (pUpgrade->cyborg)
	{
		if (asBodyUpgrade[player][CYBORG_BODY_UPGRADE].powerOutput <
			pUpgrade->upgradePoints)
		{
			asBodyUpgrade[player][CYBORG_BODY_UPGRADE].powerOutput =
				pUpgrade->upgradePoints;
		}
		if (asBodyUpgrade[player][CYBORG_BODY_UPGRADE].body <
			pUpgrade->body)
		{
			asBodyUpgrade[player][CYBORG_BODY_UPGRADE].body =
				pUpgrade->body;
		}
		for (inc=0; inc < WC_NUM_WEAPON_CLASSES; inc++)
		{
			if (asBodyUpgrade[player][CYBORG_BODY_UPGRADE].armourValue[inc] <
				pUpgrade->armourValue[inc])
			{
				asBodyUpgrade[player][CYBORG_BODY_UPGRADE].armourValue[inc] =
					pUpgrade->armourValue[inc];
			}
		}
	}
}

//upgrade the structure stats for the correct player
void structureUpgrade(FUNCTION *pFunction, UBYTE player)
{
	STRUCTURE_UPGRADE_FUNCTION		*pUpgrade;

	pUpgrade = (STRUCTURE_UPGRADE_FUNCTION *)pFunction;

	//check upgrades increase all values!
	if (asStructureUpgrade[player].armour < pUpgrade->armour)
	{
		asStructureUpgrade[player].armour = pUpgrade->armour;
	}
	if (asStructureUpgrade[player].body < pUpgrade->body)
	{
		asStructureUpgrade[player].body = pUpgrade->body;
	}
	if (asStructureUpgrade[player].resistance < pUpgrade->resistance)
	{
		asStructureUpgrade[player].resistance = pUpgrade->resistance;
	}
}

//upgrade the wall/Defence structure stats for the correct player
void wallDefenceUpgrade(FUNCTION *pFunction, UBYTE player)
{
	WALLDEFENCE_UPGRADE_FUNCTION		*pUpgrade;

	pUpgrade = (WALLDEFENCE_UPGRADE_FUNCTION *)pFunction;

	//check upgrades increase all values!
	if (asWallDefenceUpgrade[player].armour < pUpgrade->armour)
	{
		asWallDefenceUpgrade[player].armour = pUpgrade->armour;
	}
	if (asWallDefenceUpgrade[player].body < pUpgrade->body)
	{
		asWallDefenceUpgrade[player].body = pUpgrade->body;
	}
}

/*upgrades the droids inside a Transporter uwith the appropriate upgrade function*/
void upgradeTransporterDroids(DROID *psTransporter, void(*pUpgradeFunction)(DROID *psDroid))
{
    ASSERT (psTransporter->droidType == DROID_TRANSPORTER || psTransporter->droidType == DROID_SUPERTRANSPORTER, "upgradeTransporterUnits: invalid unit type");

    //loop thru' each unit in the Transporter
    for (DROID *psCurr = psTransporter->psGroup->psList; psCurr != NULL; psCurr = psCurr->psGrpNext)
    {
        if (psCurr != psTransporter)
        {
            //apply upgrade if not the transporter itself
            pUpgradeFunction(psCurr);
        }
    }
}

bool FunctionShutDown(void)
{
	UDWORD		inc;//, player;
	FUNCTION	*pFunction, **pStartList = asFunctions;

	for (inc=0; inc < numFunctions; inc++)
	{
		pFunction = *asFunctions;
		free(pFunction->pName);
		free(pFunction);
		asFunctions++;
	}
	free(pStartList);

	return true;
}

bool loadFunctionStats(const char *pFunctionData, UDWORD bufferSize)
{
	//array of functions pointers for each load function
	static const LoadFunction pLoadFunction[NUMFUNCTIONS] =
	{
		loadProduction,
		loadProductionUpgradeFunction,
		loadResearchFunction,
		loadResearchUpgradeFunction,
		loadPowerGenFunction,
		loadResourceFunction,
		loadRepairDroidFunction,
		loadWeaponUpgradeFunction,
		loadWallFunction,
		loadStructureUpgradeFunction,
		loadWallDefenceUpgradeFunction,
		loadPowerUpgradeFunction,
		loadRepairUpgradeFunction,
		loadDroidRepairUpgradeFunction,
		loadDroidECMUpgradeFunction,
		loadDroidBodyUpgradeFunction,
		loadDroidSensorUpgradeFunction,
		loadDroidConstUpgradeFunction,
		loadReArmFunction,
		loadReArmUpgradeFunction,
	};

	const unsigned int totalFunctions = numCR(pFunctionData, bufferSize);
	UDWORD		i, type;
	char		FunctionType[MAX_STR_LENGTH];
	FUNCTION	**pStartList;

	//allocate storage for the Function pointer array
	asFunctions = (FUNCTION**) malloc(totalFunctions*sizeof(FUNCTION*));
	if (!asFunctions)
	{
		debug( LOG_FATAL, "Out of memory" );
		abort();
		return false;
	}
	pStartList = asFunctions;
	//initialise the storage
	memset(asFunctions, 0, totalFunctions*sizeof(FUNCTION*));
	numFunctions = 0;
	//numProductionUpgrades =	numResearchUpgrades = 0;//numArmourUpgrades =
		//numRepairUpgrades = numResistanceUpgrades = numBodyUpgrades =
		//numWeaponUpgrades = 0;

	for (i=0; i < totalFunctions; i++)
	{
		//read the data into the storage - the data is delimeted using comma's
		FunctionType[0] = '\0';
		sscanf(pFunctionData, "%255[^,'\r\n]", FunctionType);
		type = functionType(FunctionType);
		pFunctionData += (strlen(FunctionType)+1);

		if (!(pLoadFunction[type](pFunctionData)))
		{
			return false;
		}
		//increment the pointer to the start of the next record
		pFunctionData = strchr(pFunctionData,'\n') + 1;
	}
	//set the function list pointer to the start
	asFunctions = pStartList;

	return true;
}
