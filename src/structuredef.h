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
 *  Definitions for structures.
 */

#ifndef __INCLUDED_STRUCTUREDEF_H__
#define __INCLUDED_STRUCTUREDEF_H__

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

#include "lib/gamelib/animobj.h"
#include "positiondef.h"
#include "basedef.h"
#include "statsdef.h"
#include "weapondef.h"

#define NUM_FACTORY_MODULES	2
#define NUM_RESEARCH_MODULES 4
#define NUM_POWER_MODULES 4

#define	REF_ANY	255	// Used to indicate any kind of building when calling intGotoNextStructureType()

/* Defines for indexing an appropriate IMD object given a buildings purpose. */
typedef enum _structure_type
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
} STRUCTURE_TYPE;

typedef struct _flag_position
{
	POSITION_OBJ;
	Vector3i		coords;							//the world coords of the Position
	UBYTE		factoryInc;						//indicates whether the first, second etc factory
	UBYTE		factoryType;					//indicates whether standard, cyborg or vtol factory
//	UBYTE		factorySub;						//sub value. needed to order production points.
//	UBYTE		primary;
	struct _flag_position	*psNext;
} FLAG_POSITION;


//only allowed one weapon per structure (more memory for Tim)
//Watermelon:only allowed 4 weapons per structure(sorry Tim...)
#define STRUCT_MAXWEAPS		4

typedef enum _struct_strength
{
	STRENGTH_SOFT,
	STRENGTH_MEDIUM,
	STRENGTH_HARD,
	STRENGTH_BUNKER,

	NUM_STRUCT_STRENGTH,
} STRUCT_STRENGTH;

#define INVALID_STRENGTH	(NUM_STRUCT_STRENGTH + 1)

typedef UWORD STRUCTSTRENGTH_MODIFIER;

#define SAS_OPEN_SPEED		(GAME_TICKS_PER_SEC * 2)
#define SAS_STAY_OPEN_TIME	(GAME_TICKS_PER_SEC * 6)

typedef enum _anim_states
{
	SAS_NORMAL,
	SAS_OPEN,
	SAS_OPENING,
	SAS_CLOSING,
} STRUCT_ANIM_STATES;

//this structure is used to hold the permenant stats for each type of building
typedef struct _structure_stats
{
	STATS_BASE;						/* basic stats */
	STRUCTURE_TYPE	type;				/* the type of structure */
	STRUCT_STRENGTH	strength;		/* strength against the weapon effects */
	UDWORD		terrainType;		/*The type of terrain the structure has to be
									  built next to - may be none*/
	UDWORD		baseWidth;			/*The width of the base in tiles*/
	UDWORD		baseBreadth;		/*The breadth of the base in tiles*/
	UDWORD		foundationType;		/*The type of foundation for the structure*/
	UDWORD		buildPoints;		/*The number of build points required to build
									  the structure*/
	UDWORD		height;				/*The height above/below the terrain - negative
									  values denote below the terrain*/
	UDWORD		armourValue;		/*The armour value for the structure - can be
									  upgraded */
	UDWORD		bodyPoints;			/*The structure's body points - A structure goes
									  off-line when 50% of its body points are lost*/
	UDWORD		repairSystem;		/*The repair system points are added to the body
									  points until fully restored . The points are
									  then added to the Armour Points*/
	UDWORD		powerToBuild;		/*How much power the structure requires to build*/
	UDWORD		resistance;			/*The number used to determine whether a
									  structure can resist an enemy takeover -
									  0 = cannot be attacked electrically*/
	UDWORD		sizeModifier;		/*The larger the target, the easier to hit*/
	iIMDShape	*pIMD;		/*The IMD to draw for this structure */
	iIMDShape	*pBaseIMD;	/*The base IMD to draw for this structure */
	struct ECM_STATS	*pECM;		/*Which ECM is standard for the structure -
									  if any*/
	struct SENSOR_STATS *pSensor;	/*Which Sensor is standard for the structure -
									  if any*/
	UDWORD		weaponSlots;		/*Number of weapons that can be attached to the
									  building*/
	UDWORD		numWeaps;			/*Number of weapons for default */

	struct WEAPON_STATS    *psWeapStat[STRUCT_MAXWEAPS];

	UDWORD		numFuncs;			/*Number of functions for default*/
	SDWORD		defaultFunc;		/*The default function*/
	struct _function	**asFuncList;		/*List of pointers to allowable functions -
									  unalterable*/
} WZ_DECL_MAY_ALIAS STRUCTURE_STATS;

typedef enum _struct_states
{
	SS_BEING_BUILT,
	SS_BUILT,
	SS_BEING_DEMOLISHED,
	SS_BLUEPRINT_VALID,
	SS_BLUEPRINT_INVALID,
	SS_BLUEPRINT_PLANNED,
} STRUCT_STATES;

typedef struct _research_facility
{
	struct BASE_STATS *     psSubject;              // The subject the structure is working on.
	struct BASE_STATS *     psSubjectPending;       // The subject the structure is going to work on when the GAME_RESEARCHSTATUS message is received.
	UDWORD		capacity;				/* Number of upgrade modules added*/
	UDWORD		timeStarted;			/* The time the building started on the subject*/
	UDWORD		researchPoints;			/* Research Points produced per research cycle*/
	UDWORD		timeToResearch;			/* Time taken to research the topic*/
	struct BASE_STATS	*psBestTopic;	/* The topic with the most research points
										   that was last performed*/
	UDWORD		powerAccrued;			/* used to keep track of power before
										   researching a topic*/
	UDWORD		timeStartHold;		    /* The time the research facility was put on hold*/

} RESEARCH_FACILITY;

typedef enum
{
	FACTORY_NOTHING_PENDING = 0,
	FACTORY_START_PENDING,
	FACTORY_HOLD_PENDING,
	FACTORY_CANCEL_PENDING
} FACTORY_STATUS_PENDING;

typedef struct _factory
{
	UBYTE				capacity;			/* The max size of body the factory
											   can produce*/
	uint8_t                         productionLoops;        ///< Number of loops to perform. Not synchronised, and only meaningful for selectedPlayer.
	UBYTE				loopsPerformed;		/* how many times the loop has been performed*/
	UBYTE				productionOutput;	/* Droid Build Points Produced Per
											   Build Cycle*/
	UDWORD				powerAccrued;		/* used to keep track of power before building a droid*/
	BASE_STATS *                    psSubject;              ///< The subject the structure is working on.
	BASE_STATS *                    psSubjectPending;       ///< The subject the structure is going to working on. (Pending = not yet synchronised.)
	FACTORY_STATUS_PENDING          statusPending;          ///< Pending = not yet synchronised.
	unsigned                        pendingCount;           ///< Number of messages sent but not yet processed.

	UDWORD				timeStarted;		/* The time the building started on the subject*/
	UDWORD				timeToBuild;		/* Time taken to build one droid */
	UDWORD				timeStartHold;		/* The time the factory was put on hold*/
	FLAG_POSITION		*psAssemblyPoint;	/* Place for the new droids to assemble at */
	struct DROID		*psCommander;	    // command droid to produce droids for (if any)
	uint32_t                        secondaryOrder;         ///< Secondary order state for all units coming out of the factory.
                                            // added AB 22/04/99
} FACTORY;

typedef struct _res_extractor
{
	UDWORD				timeLastUpdated;	/*time the Res Extr last got points*/
	BOOL				active;				/*indicates when the extractor is on ie digging up oil*/
	struct _structure	*psPowerGen;		/*owning power generator*/
} RES_EXTRACTOR;

typedef struct _power_gen
{
	UDWORD			multiplier;				///< Factor to multiply output by - percentage
	UDWORD			capacity;				///< Number of upgrade modules added
	struct _structure	*apResExtractors[NUM_POWER_MODULES];	///< Pointers to associated oil derricks
} POWER_GEN;

typedef struct REPAIR_FACILITY
{
	UDWORD				power;				/* Power used in repairing */
	UDWORD				timeStarted;		/* Time repair started on current object */
	BASE_OBJECT			*psObj;				/* Object being repaired */
	UDWORD				powerAccrued;		/* used to keep track of power before
											   repairing a droid */
	FLAG_POSITION		*psDeliveryPoint;	/* Place for the repaired droids to assemble
                                               at */
    UDWORD              currentPtsAdded;    /* stores the amount of body points added to the unit
                                               that is being worked on */

	// The group the droids to be repaired by this facility belong to
	struct _droid_group		*psGroup;
	struct DROID			*psGrpNext;
	int				droidQueue;		///< Last count of droid queue for this facility
} REPAIR_FACILITY;

typedef struct _rearm_pad
{
	UDWORD				reArmPoints;		/* rearm points per cycle				 */
	UDWORD				timeStarted;		/* Time reArm started on current object	 */
	BASE_OBJECT			*psObj;				/* Object being rearmed		             */
    UDWORD              timeLastUpdated;    /* Time rearm was last updated */
} REARM_PAD;

typedef union
{
	RESEARCH_FACILITY researchFacility;
	FACTORY           factory;
	RES_EXTRACTOR     resourceExtractor;
	POWER_GEN         powerGenerator;
	REPAIR_FACILITY   repairFacility;
	REARM_PAD         rearmPad;
} FUNCTIONALITY;

//this structure is used whenever an instance of a building is required in game
typedef struct _structure
{
	/* The common structure elements for all objects */
	BASE_ELEMENTS(struct _structure);

	STRUCTURE_STATS*	pStructureType;		/* pointer to the structure stats for this
											   type of building */
	UBYTE		status;						/* defines whether the structure is being
											   built, doing nothing or performing a function*/
    SWORD		currentBuildPts;			/* the build points currently assigned to this
											   structure */
    SWORD       currentPowerAccrued;        /* the power accrued for building this structure*/
	SWORD		resistance;					/* current resistance points
											   0 = cannot be attacked electrically*/
	UDWORD		lastResistance;				/* time the resistance was last increased*/

	/* The other structure data.  These are all derived from the functions
	 * but stored here for easy access - will need to add more for variable stuff!
	 */

	FUNCTIONALITY	*pFunctionality;		/* pointer to structure that contains fields
											   necessary for functionality */
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

	STRUCT_ANIM_STATES	state;
	UDWORD			lastStateTime;
} WZ_DECL_MAY_ALIAS STRUCTURE;

#define LOTS_OF	255						/*highest number the limit can be set to */
typedef struct _structure_limits
{
	UBYTE		limit;				/* the number allowed to be built */
	UBYTE		currentQuantity;	/* the number of the type currently
												   built per player*/

	UBYTE		globalLimit;		// multiplayer only. sets the max value selectable (limits changed by player)

} STRUCTURE_LIMITS;


//the three different types of factory (currently) - FACTORY, CYBORG_FACTORY, VTOL_FACTORY
// added repair facilities as they need an assebly point as well
#define NUM_FACTORY_TYPES	3
#define FACTORY_FLAG		0
#define CYBORG_FLAG			1
#define VTOL_FLAG			2
#define REPAIR_FLAG			3
//seperate the numfactory from numflag
#define NUM_FLAG_TYPES      4

//this is used for module graphics - factory and vtol factory
#define NUM_FACMOD_TYPES	2

typedef struct _production_run
{
	UBYTE						quantity;			//number to build
	UBYTE						built;				//number built on current run
	struct _droid_template		*psTemplate;		//template to build
} PRODUCTION_RUN;

/* structure stats which can be upgraded by research*/
typedef struct _structure_upgrade
{
	UWORD			armour;
	UWORD			body;
	UWORD			resistance;
} STRUCTURE_UPGRADE;

/* wall/Defence structure stats which can be upgraded by research*/
typedef struct _wallDefence_upgrade
{
	UWORD			armour;
	UWORD			body;
} WALLDEFENCE_UPGRADE;

typedef struct _upgrade
{
	UWORD		modifier;		//% to increase the stat by
} UPGRADE;

typedef UPGRADE		RESEARCH_UPGRADE;
typedef UPGRADE		PRODUCTION_UPGRADE;
typedef UPGRADE		REPAIR_FACILITY_UPGRADE;
typedef UPGRADE		POWER_UPGRADE;
typedef UPGRADE		REARM_UPGRADE;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __INCLUDED_STRUCTUREDEF_H__
