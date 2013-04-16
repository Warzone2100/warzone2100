/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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

#include "lib/gamelib/animobj.h"
#include "positiondef.h"
#include "basedef.h"
#include "statsdef.h"
#include "weapondef.h"

#include <vector>

#define NUM_FACTORY_MODULES	2
#define NUM_POWER_MODULES	4

#define	REF_ANY	255	// Used to indicate any kind of building when calling intGotoNextStructureType()

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
REF_BLASTDOOR,
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
	Vector3i		coords;							//the world coords of the Position
	UBYTE		factoryInc;						//indicates whether the first, second etc factory
	UBYTE		factoryType;					//indicates whether standard, cyborg or vtol factory
	FLAG_POSITION * psNext;
};


#define STRUCT_MAXWEAPS		4

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

//this structure is used to hold the permenant stats for each type of building
struct STRUCTURE_STATS : public BASE_STATS
{
	STRUCTURE_STATS() : pBaseIMD(NULL), pECM(NULL), pSensor(NULL) {};

	STRUCTURE_TYPE	type;				/* the type of structure */
	STRUCT_STRENGTH	strength;		/* strength against the weapon effects */
	UDWORD		baseWidth;			/*The width of the base in tiles*/
	UDWORD		baseBreadth;		/*The breadth of the base in tiles*/
	UDWORD		buildPoints;		/*The number of build points required to build
									  the structure*/
	UDWORD		height;				/*The height above/below the terrain - negative
									  values denote below the terrain*/
	UDWORD		armourValue;		/*The armour value for the structure - can be
									  upgraded */
	UDWORD		bodyPoints;			/*The structure's body points - A structure goes
									  off-line when 50% of its body points are lost*/
	UDWORD		powerToBuild;		/*How much power the structure requires to build*/
	UDWORD		resistance;			/*The number used to determine whether a
									  structure can resist an enemy takeover -
									  0 = cannot be attacked electrically*/
	std::vector<iIMDShape *> pIMD;          // The IMDs to draw for this structure, for each possible number of modules.
	iIMDShape	*pBaseIMD;	/*The base IMD to draw for this structure */
	struct ECM_STATS	*pECM;		/*Which ECM is standard for the structure -
									  if any*/
	struct SENSOR_STATS *pSensor;	/*Which Sensor is standard for the structure -
									  if any*/
	UDWORD		weaponSlots;		/*Number of weapons that can be attached to the
									  building*/
	UDWORD		numWeaps;			/*Number of weapons for default */

	struct WEAPON_STATS    *psWeapStat[STRUCT_MAXWEAPS];

	SDWORD		defaultFunc;		/*The default function*/
	std::vector<struct FUNCTION *> asFuncList;               ///< List of pointers to allowable functions - unalterable
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
	RESEARCH *      psSubject;                      // The subject the structure is working on.
	RESEARCH *      psSubjectPending;               // The subject the structure is going to work on when the GAME_RESEARCHSTATUS message is received.
	StatusPending   statusPending;                  ///< Pending = not yet synchronised.
	unsigned        pendingCount;                   ///< Number of messages sent but not yet processed.
	UDWORD		researchPoints;			/* Research Points produced per research cycle*/
	RESEARCH *      psBestTopic;                    // The topic with the most research points that was last performed
	UDWORD		timeStartHold;		    /* The time the research facility was put on hold*/
};

struct DROID_TEMPLATE;

struct FACTORY
{
	uint8_t                         productionLoops;        ///< Number of loops to perform. Not synchronised, and only meaningful for selectedPlayer.
	UBYTE				loopsPerformed;		/* how many times the loop has been performed*/
	int				productionOutput;	/* Droid Build Points Produced Per
											   Build Cycle*/
	DROID_TEMPLATE *                psSubject;              ///< The subject the structure is working on.
	DROID_TEMPLATE *                psSubjectPending;       ///< The subject the structure is going to working on. (Pending = not yet synchronised.)
	StatusPending                   statusPending;          ///< Pending = not yet synchronised.
	unsigned                        pendingCount;           ///< Number of messages sent but not yet processed.

	UDWORD				timeStarted;		/* The time the building started on the subject*/
	int                             buildPointsRemaining;   ///< Build points required to finish building the droid.
	UDWORD				timeStartHold;		/* The time the factory was put on hold*/
	FLAG_POSITION		*psAssemblyPoint;	/* Place for the new droids to assemble at */
	struct DROID		*psCommander;	    // command droid to produce droids for (if any)
	uint32_t                        secondaryOrder;         ///< Secondary order state for all units coming out of the factory.
};

struct RES_EXTRACTOR
{
	bool				active;				/*indicates when the extractor is on ie digging up oil*/
	struct STRUCTURE *      psPowerGen;                             ///< owning power generator
};

struct POWER_GEN
{
	UDWORD			multiplier;				///< Factor to multiply output by - percentage
	struct STRUCTURE *      apResExtractors[NUM_POWER_MODULES];     ///< Pointers to associated oil derricks
};

class DROID_GROUP;

struct REPAIR_FACILITY
{
	UDWORD                          power;                  // Repair rate. Nothing to do with power.
	BASE_OBJECT                     *psObj;                 /* Object being repaired */
	FLAG_POSITION                   *psDeliveryPoint;       /* Place for the repaired droids to assemble at */

	// The group the droids to be repaired by this facility belong to
	DROID_GROUP *                   psGroup;
	int                             droidQueue;              ///< Last count of droid queue for this facility
};

struct REARM_PAD
{
	UDWORD                          reArmPoints;            /* rearm points per cycle */
	UDWORD                          timeStarted;            /* Time reArm started on current object */
	BASE_OBJECT                     *psObj;                 /* Object being rearmed */
	UDWORD                          timeLastUpdated;        /* Time rearm was last updated */
};

struct WALL
{
	unsigned                        type;                   // Type of wall, 0 = ─, 1 = ┼, 2 = ┴, 3 = ┘.
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
	int32_t             currentBuildPts;            /* the build points currently assigned to this structure */
	SWORD               resistance;                 /* current resistance points, 0 = cannot be attacked electrically */
	UDWORD              lastResistance;             /* time the resistance was last increased*/
	FUNCTIONALITY       *pFunctionality;            /* pointer to structure that contains fields necessary for functionality */
	int                 buildRate;                  ///< Rate that this structure is being built, calculated each tick. Only meaningful if status == SS_BEING_BUILT. If construction hasn't started and build rate is 0, remove the structure.
	int                 lastBuildRate;              ///< Needed if wanting the buildRate between buildRate being reset to 0 each tick and the trucks calculating it.

	/* The weapons on the structure */
	UWORD		numWeaps;
	WEAPON		asWeaps[STRUCT_MAXWEAPS];
	BASE_OBJECT	*psTarget[STRUCT_MAXWEAPS];
	UWORD		targetOrigin[STRUCT_MAXWEAPS];

#ifdef DEBUG
	// these are to help tracking down dangling pointers
	char				targetFunc[STRUCT_MAXWEAPS][MAX_EVENT_NAME_LEN];
	int				targetLine[STRUCT_MAXWEAPS];
#endif

	UDWORD          expectedDamage;                 ///< Expected damage to be caused by all currently incoming projectiles. This info is shared between all players,
	                                                ///< but shouldn't make a difference unless 3 mutual enemies happen to be fighting each other at the same time.

	uint32_t        prevTime;                       ///< Time of structure's previous tick.

	/* anim data */
	ANIM_OBJECT	*psCurAnim;

	float		foundationDepth;		///< Depth of structure's foundation
	uint8_t		capacity;			///< Number of module upgrades

	STRUCT_ANIM_STATES	state;
	UDWORD			lastStateTime;

	iIMDShape *         prebuiltImd;
};

#define LOTS_OF 0xFFFFFFFF  // highest number the limit can be set to
struct STRUCTURE_LIMITS
{
	uint32_t        limit;                  // the number allowed to be built
	uint32_t        currentQuantity;        // the number of the type currently built per player

	uint32_t        globalLimit;            // multiplayer only. sets the max value selectable (limits changed by player)
};


//the three different types of factory (currently) - FACTORY, CYBORG_FACTORY, VTOL_FACTORY
// added repair facilities as they need an assebly point as well
enum FLAG_TYPE
{
	FACTORY_FLAG,
	CYBORG_FLAG,
	VTOL_FLAG,
	REPAIR_FLAG,
//seperate the numfactory from numflag
	NUM_FLAG_TYPES,
	NUM_FACTORY_TYPES = REPAIR_FLAG,
};

//this is used for module graphics - factory and vtol factory
static const int NUM_FACMOD_TYPES = 2;

struct ProductionRunEntry
{
	ProductionRunEntry() : quantity(0), built(0), psTemplate(NULL) {}
	void restart() { built = 0; }
	void removeComplete() { quantity -= built; built = 0; }
	int numRemaining() const { return quantity - built; }
	bool isComplete() const { return numRemaining() <= 0; }
	bool isValid() const { return psTemplate != NULL && quantity > 0 && built <= quantity; }
	bool operator ==(DROID_TEMPLATE *t) const;

	int                             quantity;             //number to build
	int                             built;                //number built on current run
	DROID_TEMPLATE *                psTemplate;           //template to build
};
typedef std::vector<ProductionRunEntry> ProductionRun;

/* structure stats which can be upgraded by research*/
struct STRUCTURE_UPGRADE
{
	UWORD			armour;
	UWORD			body;
	UWORD			resistance;
};

/* wall/Defence structure stats which can be upgraded by research*/
struct WALLDEFENCE_UPGRADE
{
	UWORD			armour;
	UWORD			body;
};

struct UPGRADE
{
	UWORD		modifier;		//% to increase the stat by
};

typedef UPGRADE		RESEARCH_UPGRADE;
typedef UPGRADE		PRODUCTION_UPGRADE;
typedef UPGRADE		REPAIR_FACILITY_UPGRADE;
typedef UPGRADE		POWER_UPGRADE;
typedef UPGRADE		REARM_UPGRADE;

#endif // __INCLUDED_STRUCTUREDEF_H__
