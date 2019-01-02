/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2019  Warzone 2100 Project

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
/** @file
 *  Object information printing routines
 */

#include "lib/framework/frame.h"
#include "objects.h"
#include "projectile.h"
#include "console.h"
#include "oprint.h"
#include "visibility.h"

/** print out information about a base object
 *  \param psObj the object to print the info for
 */
static void printBaseObjInfo(const BASE_OBJECT *psObj)
{
	const char *pType;
	switch (psObj->type)
	{
	case OBJ_DROID:
		pType = "UNIT";
		break;
	case OBJ_STRUCTURE:
		pType = "STRUCT";
		break;
	case OBJ_FEATURE:
		pType = "FEAT";
		break;
	default:
		pType = "UNKNOWN TYPE";
		break;
	}

	CONPRINTF("%s id %d at (%d,%d,%d) dpr (%hu,%hu,%hu)\n", pType, psObj->id, psObj->pos.x, psObj->pos.y, psObj->pos.z, psObj->rot.direction, psObj->rot.pitch, psObj->rot.roll);
}

/** print out information about a general component
 *  \param psStats the component to print the info for
 */
static void printComponentInfo(const COMPONENT_STATS *psStats)
{
	CONPRINTF("%s ref %d\n"
	                          "   bPwr %d bPnts %d wt %d bdy %d imd %p\n",
	                          getName(psStats), psStats->ref, psStats->buildPower,
	                          psStats->buildPoints, psStats->weight, psStats->pBase->hitpoints,
	                          static_cast<void *>(psStats->pIMD));
}

/** print out weapon information
 *  \param psStats the weapon to print the info for
 */
static void printWeaponInfo(const WEAPON_STATS *psStats)
{
	const char *pWC, *pWSC, *pMM;

	switch (psStats->weaponClass)
	{
	case WC_KINETIC:		//bullets etc
		pWC = "WC_KINETIC";
		break;
	case WC_HEAT:		//laser etc
		pWC = "WC_HEAT";
		break;
	default:
		pWC = "UNKNOWN CLASS";
		break;
	}
	switch (psStats->weaponSubClass)
	{
	case WSC_MGUN:
		pWSC = "WSC_MGUN";
		break;
	case WSC_CANNON:
		pWSC = "WSC_CANNON";
		break;
	case WSC_MORTARS:
		pWSC = "WSC_MORTARS";
		break;
	case WSC_MISSILE:
		pWSC = "WSC_MISSILE";
		break;
	case WSC_ROCKET:
		pWSC = "WSC_ROCKET";
		break;
	case WSC_ENERGY:
		pWSC = "WSC_ENERGY";
		break;
	case WSC_GAUSS:
		pWSC = "WSC_GAUSS";
		break;
	case WSC_FLAME:
		pWSC = "WSC_FLAME";
		break;
	case WSC_HOWITZERS:
		pWSC = "WSC_HOWITZERS";
		break;
	case WSC_ELECTRONIC:
		pWSC = "WSC_ELECTRONIC";
		break;
	case WSC_AAGUN:
		pWSC = "WSC_AAGUN";
		break;
	case WSC_SLOWMISSILE:
		pWSC = "WSC_SLOWMISSILE";
		break;
	case WSC_SLOWROCKET:
		pWSC = "WSC_SLOWROCKET";
		break;
	case WSC_LAS_SAT:
		pWSC = "WSC_LAS_SAT";
		break;
	case WSC_BOMB:
		pWSC = "WSC_BOMB";
		break;
	case WSC_COMMAND:
		pWSC = "WSC_COMMAND";
		break;
	case WSC_EMP:
		pWSC = "WSC_EMP";
		break;
	default:
		pWSC = "UNKNOWN SUB CLASS";
		break;
	}
	switch (psStats->movementModel)
	{
	case MM_DIRECT:
		pMM = "MM_DIRECT";
		break;
	case MM_INDIRECT:
		pMM = "MM_INDIRECT";
		break;
	case MM_HOMINGDIRECT:
		pMM = "MM_HOMINGDIRECT";
		break;
	case MM_HOMINGINDIRECT:
		pMM = "MM_HOMINGINDIRECT";
		break;
	default:
		pMM = "UNKNOWN MOVE MODEL";
		break;
	}

	CONPRINTF("%s", "Weapon: ");
	printComponentInfo((const COMPONENT_STATS *)psStats);
	CONPRINTF("   lRng %d mRng %d %s\n"
	                          "   lHt %d pause %d dam %d\n",
	                          proj_GetLongRange(psStats, selectedPlayer), psStats->upgrade[selectedPlayer].minRange,
	                          proj_Direct(psStats) ? "direct" : "indirect",
	                          weaponLongHit(psStats, selectedPlayer), weaponFirePause(psStats, selectedPlayer),
	                          weaponDamage(psStats, selectedPlayer));
	CONPRINTF("   rad %d radDam %d\n"
	                          "   inTime %d inDam %d inRad %d\n",
	                          psStats->upgrade[selectedPlayer].radius, psStats->upgrade[selectedPlayer].radiusDamage,
	                          psStats->upgrade[selectedPlayer].periodicalDamageTime, psStats->upgrade[selectedPlayer].periodicalDamage, psStats->upgrade[selectedPlayer].periodicalDamageRadius);
	CONPRINTF("   flSpd %d %s\n",
	                          psStats->flightSpeed, psStats->fireOnMove ? "fireOnMove" : "not fireOnMove");
	CONPRINTF("   %s %s %s\n", pWC, pWSC, pMM);
	CONPRINTF("   %srotate recoil %d\n"
	                          "   radLife %d\n",
	                          psStats->rotate ? "" : "not ",
	                          psStats->recoilValue, psStats->radiusLife);
}

/** print out information about a droid and it's components
 *  \param psDroid the droid to print the info for
 */
void printDroidInfo(const DROID *psDroid)
{
	BODY_STATS			*psBdyStats;
	PROPULSION_STATS	*psPropStats;
	ECM_STATS			*psECMStats;
	SENSOR_STATS		*psSensStats;
	CONSTRUCT_STATS		*psConstStats;
	REPAIR_STATS		*psRepairStats;

	printBaseObjInfo((const BASE_OBJECT *)psDroid);

	CONPRINTF("   wt %d bSpeed %d sRng %d ECM %d bdy %d\n",
	                          psDroid->weight, psDroid->baseSpeed, droidSensorRange(psDroid), objJammerPower(psDroid), psDroid->body);

	if (psDroid->asWeaps[0].nStat > 0)
	{
		printWeaponInfo(asWeaponStats + psDroid->asWeaps[0].nStat);
	}

	for (int i = 0; i < COMP_NUMCOMPONENTS; ++i)
	{
		switch (i)
		{
		case COMP_BODY:
			if (psDroid->asBits[i] > 0)
			{
				CONPRINTF("%s", "Body: ");
				psBdyStats = asBodyStats + psDroid->asBits[i];
				printComponentInfo((COMPONENT_STATS *)psBdyStats);
			}
			else
			{
				CONPRINTF("%s", "ZNULL BODY\n");
			}
			break;
		case COMP_BRAIN:
			break;
		case COMP_PROPULSION:
			if (psDroid->asBits[i] > 0)
			{
				CONPRINTF("%s", "Prop: ");
				psPropStats = asPropulsionStats + psDroid->asBits[i];
				printComponentInfo((COMPONENT_STATS *)psPropStats);
			}
			else
			{
				CONPRINTF("%s", "ZNULL PROPULSION\n");
			}
			break;
		case COMP_ECM:
			if (psDroid->asBits[i] > 0)
			{
				CONPRINTF("%s", "ECM: ");
				psECMStats = asECMStats + psDroid->asBits[i];
				printComponentInfo((COMPONENT_STATS *)psECMStats);
				CONPRINTF("   range %d loc %d imd %p\n",
				                          ecmRange(psECMStats, psDroid->player), psECMStats->location,
				                          static_cast<void *>(psECMStats->pMountGraphic));
			}
			else
			{
				CONPRINTF("%s", "ZNULL ECM\n");
			}
			break;
		case COMP_SENSOR:
			if (psDroid->asBits[i] > 0)
			{
				CONPRINTF("%s", "Sensor: ");
				psSensStats = asSensorStats + psDroid->asBits[i];
				printComponentInfo((COMPONENT_STATS *)psSensStats);
				CONPRINTF("   rng %d loc %d imd %p\n",
				                          sensorRange(psSensStats, psDroid->player),
				                          psSensStats->location, static_cast<void *>(psSensStats->pMountGraphic));
			}
			else
			{
				CONPRINTF("%s", "ZNULL SENSOR\n");
			}
			break;
		case COMP_CONSTRUCT:
			if (psDroid->asBits[i] > 0)
			{
				CONPRINTF("%s", "Construct: ");
				psConstStats = asConstructStats + psDroid->asBits[i];
				printComponentInfo((COMPONENT_STATS *)psConstStats);
				CONPRINTF("   cPnts %d imd %p\n",
				                          constructorPoints(psConstStats, psDroid->player),
				                          static_cast<void *>(psConstStats->pMountGraphic));
			}
			break;
		case COMP_REPAIRUNIT:
			if (psDroid->asBits[i] > 0)
			{
				CONPRINTF("%s", "Repair: ");
				psRepairStats = asRepairStats + psDroid->asBits[i];
				printComponentInfo((COMPONENT_STATS *)psRepairStats);
				CONPRINTF("   repPnts %d loc %d imd %p\n",
				                          repairPoints(psRepairStats, psDroid->player),
				                          psRepairStats->location,
				                          static_cast<void *>(psRepairStats->pMountGraphic));
			}
			break;
		case COMP_WEAPON:
		default:
			break;
		}
	}
}
