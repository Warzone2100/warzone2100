/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
/** \file
 *  Definitions for functions.
 */

#ifndef __INCLUDED_FUNCTIONDEF_H__
#define __INCLUDED_FUNCTIONDEF_H__

#include "statsdef.h"

enum FUNCTION_TYPES
{
	PRODUCTION_TYPE,
	PRODUCTION_UPGRADE_TYPE,
	RESEARCH_TYPE,
	RESEARCH_UPGRADE_TYPE,
	POWER_GEN_TYPE,
	RESOURCE_TYPE,
	REPAIR_DROID_TYPE,
	WEAPON_UPGRADE_TYPE,
	WALL_TYPE,
	STRUCTURE_UPGRADE_TYPE,
	WALLDEFENCE_UPGRADE_TYPE,
	POWER_UPGRADE_TYPE,
	REPAIR_UPGRADE_TYPE,
	DROIDREPAIR_UPGRADE_TYPE,
	DROIDECM_UPGRADE_TYPE,
	DROIDBODY_UPGRADE_TYPE,
	DROIDSENSOR_UPGRADE_TYPE,
	DROIDCONST_UPGRADE_TYPE,
	REARM_TYPE,
	REARM_UPGRADE_TYPE,
	//DEFENSIVE_STRUCTURE_TYPE,
	//RADAR_MAP_TYPE,
	//POWER_REG_TYPE,
	//POWER_RELAY_TYPE,
	//ARMOUR_UPGRADE_TYPE,
	//REPAIR_UPGRADE_TYPE,
	//RESISTANCE_UPGRADE_TYPE,
	//DROID_DESIGN_TYPE,
	//MAP_MARKER_TYPE,
	//SKY_DOME_MAP_TYPE,
	//BODY_UPGRADE_TYPE,
	//HQ_TYPE,

	/* The number of function types */
	NUMFUNCTIONS,
};

/*Common stats for all Structure Functions*/

/*Common struct for all functions*/
struct FUNCTION : public BASE_STATS
{
	UBYTE           type;                   ///< The type of Function
};


/*To repair droids that enter the repair facility*/
struct REPAIR_DROID_FUNCTION : public FUNCTION
{
	UDWORD			repairPoints;	/*The number of repair points used to reduce
									  damage to the droid. These repair points can
									  restore even destroyed droid components*/
};

/*To generate and supply power to other structures*/
struct POWER_GEN_FUNCTION : public FUNCTION
{
	UDWORD		powerOutput;		/*How much power is generated per power cycle*/
	UDWORD		powerMultiplier;	/*Multiplies the output - upgradeable*/
	UDWORD		criticalMassChance;	/*The % chance of an explosion when the power
									  generator has taken damage*/
	UDWORD		criticalMassRadius;	/*The primary blast radius*/
	UDWORD		criticalMassDamage;	/*The base amount of damage applied to targets
									  within the primary blast area*/
	UDWORD		radiationDecayTime;	/*How long the radiation lasts n time cycles*/
};

/*function used by walls to define which corner to use*/
struct WALL_FUNCTION : public FUNCTION
{
	char						*pStructName;		//storage space for the name so can work out
													//which stat when structs are loaded in
	struct STRUCTURE_STATS *                        pCornerStat;            ///< pointer to which stat to use as a corner wall
};

/*function used by Resource Extractor to indicate how much resource is available*/
struct RESOURCE_FUNCTION : public FUNCTION
{
	UDWORD		maxPower;			/*The max amount output from the resource*/
};

/*To increase a production facilities output*/
struct PRODUCTION_UPGRADE_FUNCTION : public FUNCTION
{
	UBYTE		outputModifier;		/*The amount added to a facility's Output*/

	UBYTE		factory;			/*flag to indicate upgrades standard factories*/
	UBYTE		cyborgFactory;		/*flag to indicate upgrades cyborg factories*/
	UBYTE		vtolFactory;		/*flag to indicate upgrades vtol factories*/

};

/*To manufacture droids designed previously*/
struct PRODUCTION_FUNCTION : public FUNCTION
{
	UWORD					capacity;			/*The max size of body the factory
												  can produce*/
	UWORD					productionOutput;	/*Droid Build Points Produced Per
												  Build Cycle*/
	//struct _propulsion_types*		propulsionType;
	//UBYTE					propulsionType;		/*The type of propulsion the facility
	//											  can produce*/
};

/*To research topics available*/
struct RESEARCH_FUNCTION : public FUNCTION
{
	UDWORD			researchPoints;	/*The number of research points added per
									  research cycle*/
};

/*To rearm VTOLs*/
struct REARM_FUNCTION : public FUNCTION
{
	UDWORD			reArmPoints;	/*The number of reArm points added per cycle*/
};

struct UPGRADE_FUNCTION : public FUNCTION
{
	uint16_t                upgradePoints;  ///< The % to add to the action points
};

typedef UPGRADE_FUNCTION	RESEARCH_UPGRADE_FUNCTION;
typedef UPGRADE_FUNCTION	REPAIR_UPGRADE_FUNCTION;
typedef UPGRADE_FUNCTION	POWER_UPGRADE_FUNCTION;
typedef UPGRADE_FUNCTION	REARM_UPGRADE_FUNCTION;

/*Upgrade the weapon ROF and accuracy for the weapons of a particular class*/
struct WEAPON_UPGRADE_FUNCTION : public FUNCTION
{
// GNU C complains about this...
//	WEAPON_CLASS            subClass;		/*which weapons are affected */
// So need to do it this way...
	WEAPON_SUBCLASS		subClass;			/*which weapons are affected */
	UBYTE			firePause;			/*The % to decrease the fire pause or reload time */
	UWORD			shortHit;			/*The % to increase the  short range accuracy */
	UWORD			longHit;			/*The % to increase the long range accuracy */
	UWORD			damage;				/*The % to increase the damage*/
	UWORD			radiusDamage;		/*The % to increase the radius damage*/
	UWORD			incenDamage;		/*The % to increase the incendiary damage*/
	UWORD			radiusHit;			/*The % to increase the chance to hit in blast radius*/

};

/*Upgrade the structure stats for all non wall and defence structures*/
struct STRUCTURE_UPGRADE_FUNCTION : public FUNCTION
{
	UWORD			armour;			/*The % to increase the armour value*/
	UWORD			body;			/*The % to increase the body points*/
	UWORD			resistance;		/*The % to increase the resistance*/
};

/*Upgrade the structure stats for all wall and defence structures*/
struct WALLDEFENCE_UPGRADE_FUNCTION : public FUNCTION
{
	UWORD			armour;			/*The % to increase the armour value*/
	UWORD			body;			/*The % to increase the body points*/
};

typedef UPGRADE_FUNCTION	DROIDREPAIR_UPGRADE_FUNCTION;
typedef UPGRADE_FUNCTION	DROIDECM_UPGRADE_FUNCTION;
typedef UPGRADE_FUNCTION	DROIDCONSTR_UPGRADE_FUNCTION;

struct DROIDBODY_UPGRADE_FUNCTION : public UPGRADE_FUNCTION
{
	UWORD					body;		//The % to increase the whole vehicle body points by*/
	UWORD					armourValue[WC_NUM_WEAPON_CLASSES];
	UBYTE					cyborg;		//flag to specify the upgrade is valid for cyborgs
	UBYTE					droid;		/*flag to specify the upgrade is valid
										  for droids (non cyborgs!)*/
};

struct DROIDSENSOR_UPGRADE_FUNCTION : public UPGRADE_FUNCTION
{
	UWORD					range;		// % to increase range by
};


#if(0)
typedef struct _function_upgrade
{
	UDWORD		functionInc;			/*The index of the function in asFunctions */
	BOOL		available;				/*Flag to indicate whether this Upgrade is available*/
} FUNCTION_UPGRADE;
#endif
/*function used by HQ to input power values*/
//typedef struct _hq_function
//{
//	//common stats
//	FUNCTION_STATS;
//
//	UDWORD			power;				/*The power value of the HQ*/
//} HQ_FUNCTION;


/*upgrade the armour that can be applied to a droid/structure*/
//typedef struct _armour_upgrade_function
//{
//	//common stats
//	FUNCTION_STATS;
//
//	ARMOUR_STATS*	pArmour;		/*The armour to upgrade to*/
//	UDWORD			buildPoints;	/*The number of build points required to upgrade
//									  the structure*/
//	UDWORD			powerRequired;	/*The amount of power required to upgrade*/
//	UDWORD			armourPoints;	/*The percentage to increase the armour points by*/
//} ARMOUR_UPGRADE_FUNCTION;

/*To regulate power flows*/
//typedef struct _power_reg_function
//{
//	//common stats
//	FUNCTION_STATS;
//
//	UDWORD		maxPower;			/*The maximum amount of power output the
//									  regulator can handle*/
//} POWER_REG_FUNCTION;

/*To transfer power across the map*/
//typedef struct _power_relay_function
//{
//	//common stats
//	FUNCTION_STATS;
//
//	UDWORD		powerRelayType;		/*Broadcast=0 cable=1*/
//	UDWORD		powerRelayRange;	/*The range in map distances that the power
//									  can be relayed*/
//} POWER_RELAY_FUNCTION;

/*To display the radar map*/
//typedef struct _radar_map_function
//{
//	//common stats
//	FUNCTION_STATS;
//
//	UDWORD		radarDecayRate;		/*How fast the droids out of LOS decay on the
//									  radar*/
//	UDWORD		radarRadius;		/*How far a radar building can see with 100%
//									  accuracy*/
//} RADAR_MAP_FUNCTION;

/*Upgrade the repair points that can be obtained from a repair facitity*/
//typedef struct _repair_upgrade_function
//{
	//common stats
//	FUNCTION_STATS;

//	struct REPAIR_STATS*	pRepair;		/*The repair unit to be upgraded*/
//	UDWORD			repairPoints;	/*The percentage to increase the repair points by*/
//	UDWORD			buildPoints;	/*The number of build points required to upgrade
//									  the structure*/
//	UDWORD			powerRequired;	/*The amount of power required to upgrade*/
//} REPAIR_UPGRADE_FUNCTION;

/*Upgrade the body points of the structure*/
//typedef struct _body_upgrade_function
//{
//	//common stats
//	FUNCTION_STATS;
//
///	UDWORD			bodyPoints;		/*The percentage to increase the body points by*/
//} BODY_UPGRADE_FUNCTION;

/*Upgrade the resistance*/
//typedef struct _resistance_upgrade_function
//{
//	//common stats
//	FUNCTION_STATS;
//
//	UDWORD			resistanceUpgrade;	/*This is unknown at the moment!!27/02/97*/
//	UDWORD			buildPoints;		/*The number of build points required to
//										  upgrade the structure*/
//	UDWORD			powerRequired;		/*The amount of power required to upgrade*/
//	UDWORD			resistancePoints;	/*The percentage to increase the resistance points by*/
//} RESISTANCE_UPGRADE_FUNCTION;
/*blocks LOS and restricts movement*/
//typedef struct _defensive_structure_function
//{
//	//common stats
//	FUNCTION_STATS;
//
//	struct SENSOR_STATS*	pSensor;		/*The Sensor fitted, if any*/
//	struct ECM_STATS*		pECM;			/*The ECM fitted, if any*/
//	UDWORD			weaponCapacity;	/*The size of weapon in system points that may
//									  be added. 0 = no weapons can be added*/
//} DEFENSIVE_STRUCTURE_FUNCTION;

#endif // __INCLUDED_FUNCTIONDEF_H__
