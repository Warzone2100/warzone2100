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
/** \file
 *  Definitions for structures.
 */

#ifndef __INCLUDED_STRUCTUREDEF_H__
#define __INCLUDED_STRUCTUREDEF_H__

#include "positiondef.h"
#include "basedef.h"
#include "statsdef.h"
#include "weapondef.h"

#include <vector>

#define NUM_FACTORY_MODULES 2
#define NUM_POWER_MODULES 4
#define	REF_ANY 255		// Used to indicate any kind of building when calling intGotoNextStructureType()

/* Defines for indexing an appropriate IMD object given a buildings purpose. */
enum STRUCTURE_TYPE
{
	REF_HQ,
	REF_FACTORY,
	REF_FACTORY_MODULE,//draw as factory 2
	REF_POWER_GEN,
	REF_POWER_MODULE,
	REF_RESOURCE_EXTRACTOR,
	REF_DEFENSE,
	REF_WALL,
	REF_WALLCORNER,				//corner wall - no gun
	REF_GENERIC,
	REF_RESEARCH,
	REF_RESEARCH_MODULE,
	REF_REPAIR_FACILITY,
	REF_COMMAND_CONTROL,		//control centre for command droids
	REF_BRIDGE,			//NOT USED, but removing it would change savegames
	REF_DEMOLISH,			//the demolish structure type - should only be one stat with this type
	REF_CYBORG_FACTORY,
	REF_VTOL_FACTORY,
	REF_LAB,
	REF_REARM_PAD,
	REF_MISSILE_SILO,
	REF_SAT_UPLINK,         //added for updates - AB 8/6/99
	REF_GATE,
	NUM_DIFF_BUILDINGS,		//need to keep a count of how many types for IMD loading
};

struct FLAG_POSITION : public OBJECT_POSITION
{
	Vector3i coords = Vector3i(0, 0, 0); //the world coords of the Position
	UBYTE    factoryInc;           //indicates whether the first, second etc factory
	UBYTE    factoryType;          //indicates whether standard, cyborg or vtol factory
	FLAG_POSITION *psNext;
};

enum STRUCT_STRENGTH
{
	STRENGTH_SOFT,
	STRENGTH_MEDIUM,
	STRENGTH_HARD,
	STRENGTH_BUNKER,

	NUM_STRUCT_STRENGTH,
};

typedef UWORD STRUCTSTRENGTH_MODIFIER;

#define SAS_OPEN_SPEED		(GAME_TICKS_PER_SEC)
#define SAS_STAY_OPEN_TIME	(GAME_TICKS_PER_SEC * 6)

enum STRUCT_ANIM_STATES
{
	SAS_NORMAL,
	SAS_OPEN,
	SAS_OPENING,
	SAS_CLOSING,
};

#define STRUCTURE_CONNECTED 0x0001 ///< This structure must be built side by side with another of the same player

//this structure is used to hold the permanent stats for each type of building
struct STRUCTURE_STATS : public BASE_STATS
{
	STRUCTURE_STATS() : pBaseIMD(nullptr), pECM(nullptr), pSensor(nullptr)
	{
		memset(curCount, 0, sizeof(curCount));
		memset(upgrade, 0, sizeof(upgrade));
	}

	STRUCTURE_TYPE type;            /* the type of structure */
	STRUCT_STRENGTH strength;       /* strength against the weapon effects */
	UDWORD baseWidth;               /*The width of the base in tiles*/
	UDWORD baseBreadth;             /*The breadth of the base in tiles*/
	UDWORD buildPoints;             /*The number of build points required to build the structure*/
	UDWORD height;                  /*The height above/below the terrain - negative values denote below the terrain*/
	UDWORD powerToBuild;            /*How much power the structure requires to build*/
	std::vector<iIMDShape *> pIMD;  // The IMDs to draw for this structure, for each possible number of modules.
	iIMDShape *pBaseIMD;            /*The base IMD to draw for this structure */
	struct ECM_STATS *pECM;         /*Which ECM is standard for the structure -if any*/
	struct SENSOR_STATS *pSensor;   /*Which Sensor is standard for the structure -if any*/
	UDWORD weaponSlots;             /*Number of weapons that can be attached to the building*/
	UDWORD numWeaps;                /*Number of weapons for default */
	struct WEAPON_STATS *psWeapStat[MAX_WEAPONS];
	uint64_t flags;

	unsigned minLimit;		///< lowest value user can set limit to (currently unused)
	unsigned maxLimit;		///< highest value user can set limit to, LOTS_OF = no limit
	unsigned curCount[MAX_PLAYERS];	///< current number of instances of this type

	struct
	{
		unsigned research;
		unsigned moduleResearch;
		unsigned repair;
		unsigned power;
		unsigned modulePower;
		unsigned production;
		unsigned moduleProduction;
		unsigned rearm;
		unsigned armour;
		unsigned thermal;
		unsigned hitpoints;
		unsigned resistance;	// resist enemy takeover; 0 = immune
		unsigned limit;		// current max limit for this type, LOTS_OF = no limit
	} upgrade[MAX_PLAYERS], base;

	inline Vector2i size(uint16_t direction) const
	{
		Vector2i size(baseWidth, baseBreadth);
		if (((direction + 0x2000) & 0x4000) != 0) // if building is rotated left or right by 90°, swap width and height
		{
			std::swap(size.x, size.y);
		}
		return size;
	}
};

enum STRUCT_STATES
{
	SS_BEING_BUILT,
	SS_BUILT,
	SS_BLUEPRINT_VALID,
	SS_BLUEPRINT_INVALID,
	SS_BLUEPRINT_PLANNED,
	SS_BLUEPRINT_PLANNED_BY_ALLY,
};

enum StatusPending
{
	FACTORY_NOTHING_PENDING = 0,
	FACTORY_START_PENDING,
	FACTORY_HOLD_PENDING,
	FACTORY_CANCEL_PENDING
};

struct RESEARCH;

struct RESEARCH_FACILITY
{
	RESEARCH *psSubject;              // The subject the structure is working on.
	RESEARCH *psSubjectPending;       // The subject the structure is going to work on when the GAME_RESEARCHSTATUS message is received.
	StatusPending statusPending;      ///< Pending = not yet synchronised.
	unsigned pendingCount;            ///< Number of messages sent but not yet processed.
	RESEARCH *psBestTopic;            // The topic with the most research points that was last performed
	UDWORD timeStartHold;             /* The time the research facility was put on hold*/
};

struct DROID_TEMPLATE;

struct FACTORY
{
	uint8_t productionLoops;          ///< Number of loops to perform. Not synchronised, and only meaningful for selectedPlayer.
	UBYTE loopsPerformed;             /* how many times the loop has been performed*/
	DROID_TEMPLATE *psSubject;        ///< The subject the structure is working on.
	DROID_TEMPLATE *psSubjectPending; ///< The subject the structure is going to working on. (Pending = not yet synchronised.)
	StatusPending statusPending;      ///< Pending = not yet synchronised.
	unsigned pendingCount;            ///< Number of messages sent but not yet processed.
	UDWORD timeStarted;               /* The time the building started on the subject*/
	int buildPointsRemaining;         ///< Build points required to finish building the droid.
	UDWORD timeStartHold;             /* The time the factory was put on hold*/
	FLAG_POSITION *psAssemblyPoint;   /* Place for the new droids to assemble at */
	struct DROID *psCommander;        // command droid to produce droids for (if any)
	uint32_t secondaryOrder;          ///< Secondary order state for all units coming out of the factory.
};

struct RES_EXTRACTOR
{
	struct STRUCTURE *psPowerGen;    ///< owning power generator
};

struct POWER_GEN
{
	struct STRUCTURE *apResExtractors[NUM_POWER_MODULES];   ///< Pointers to associated oil derricks
};

class DROID_GROUP;

struct REPAIR_FACILITY
{
	BASE_OBJECT *psObj;                /* Object being repaired */
	FLAG_POSITION *psDeliveryPoint;    /* Place for the repaired droids to assemble at */
	// The group the droids to be repaired by this facility belong to
	DROID_GROUP *psGroup;
	int droidQueue;                    ///< Last count of droid queue for this facility
};

struct REARM_PAD
{
	UDWORD timeStarted;            /* Time reArm started on current object */
	BASE_OBJECT *psObj;            /* Object being rearmed */
	UDWORD timeLastUpdated;        /* Time rearm was last updated */
};

struct WALL
{
	unsigned type;             // Type of wall, 0 = ─, 1 = ┼, 2 = ┴, 3 = ┘.
};

union FUNCTIONALITY
{
	RESEARCH_FACILITY researchFacility;
	FACTORY           factory;
	RES_EXTRACTOR     resourceExtractor;
	POWER_GEN         powerGenerator;
	REPAIR_FACILITY   repairFacility;
	REARM_PAD         rearmPad;
	WALL              wall;
};

//this structure is used whenever an instance of a building is required in game
struct STRUCTURE : public BASE_OBJECT
{
	STRUCTURE(uint32_t id, unsigned player);
	~STRUCTURE();

	STRUCTURE_STATS     *pStructureType;            /* pointer to the structure stats for this type of building */
	STRUCT_STATES       status;                     /* defines whether the structure is being built, doing nothing or performing a function */
	uint32_t            currentBuildPts;            /* the build points currently assigned to this structure */
	int                 resistance;                 /* current resistance points, 0 = cannot be attacked electrically */
	UDWORD              lastResistance;             /* time the resistance was last increased*/
	FUNCTIONALITY       *pFunctionality;            /* pointer to structure that contains fields necessary for functionality */
	int                 buildRate;                  ///< Rate that this structure is being built, calculated each tick. Only meaningful if status == SS_BEING_BUILT. If construction hasn't started and build rate is 0, remove the structure.
	int                 lastBuildRate;              ///< Needed if wanting the buildRate between buildRate being reset to 0 each tick and the trucks calculating it.
	BASE_OBJECT *psTarget[MAX_WEAPONS];
#ifdef DEBUG
	// these are to help tracking down dangling pointers
	char targetFunc[MAX_WEAPONS][MAX_EVENT_NAME_LEN];
	int targetLine[MAX_WEAPONS];
#endif

	UDWORD expectedDamage;           ///< Expected damage to be caused by all currently incoming projectiles. This info is shared between all players,
	///< but shouldn't make a difference unless 3 mutual enemies happen to be fighting each other at the same time.
	uint32_t prevTime;               ///< Time of structure's previous tick.
	float foundationDepth;           ///< Depth of structure's foundation
	uint8_t capacity;                ///< Number of module upgrades
	STRUCT_ANIM_STATES	state;
	UDWORD lastStateTime;
	iIMDShape *prebuiltImd;

	inline Vector2i size() const { return pStructureType->size(rot.direction); }
};

#define LOTS_OF 0xFFFFFFFF  // highest number the limit can be set to

//the three different types of factory (currently) - FACTORY, CYBORG_FACTORY, VTOL_FACTORY
// added repair facilities as they need an assembly point as well
enum FLAG_TYPE
{
	FACTORY_FLAG,
	CYBORG_FLAG,
	VTOL_FLAG,
	REPAIR_FLAG,
//separate the numfactory from numflag
	NUM_FLAG_TYPES,
	NUM_FACTORY_TYPES = REPAIR_FLAG,
};

//this is used for module graphics - factory and vtol factory
static const int NUM_FACMOD_TYPES = 2;

struct ProductionRunEntry
{
	ProductionRunEntry() : quantity(0), built(0), psTemplate(nullptr) {}
	void restart()
	{
		built = 0;
	}
	void removeComplete()
	{
		quantity -= built;
		built = 0;
	}
	int numRemaining() const
	{
		return quantity - built;
	}
	bool isComplete() const
	{
		return numRemaining() <= 0;
	}
	bool isValid() const
	{
		return psTemplate != nullptr && quantity > 0 && built <= quantity;
	}
	bool operator ==(DROID_TEMPLATE *t) const;

	int quantity;                 //number to build
	int built;                    //number built on current run
	DROID_TEMPLATE *psTemplate;   //template to build
};
typedef std::vector<ProductionRunEntry> ProductionRun;

struct UPGRADE_MOD
{
	UWORD modifier;      //% to increase the stat by
};

typedef UPGRADE_MOD REPAIR_FACILITY_UPGRADE;
typedef UPGRADE_MOD POWER_UPGRADE;
typedef UPGRADE_MOD REARM_UPGRADE;

#endif // __INCLUDED_STRUCTUREDEF_H__
