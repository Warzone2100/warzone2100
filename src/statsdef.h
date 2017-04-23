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

/** \file
*  Definitions for the stats system.
*/
#ifndef __INCLUDED_STATSDEF_H__
#define __INCLUDED_STATSDEF_H__

struct iIMDShape;

#include <vector>
#include <algorithm>

/* The different types of droid */
// NOTE, if you add to, or change this list then you'll need
// to update the DroidSelectionWeights lookup table in Display.c
enum DROID_TYPE
{
	DROID_WEAPON,           ///< Weapon droid
	DROID_SENSOR,           ///< Sensor droid
	DROID_ECM,              ///< ECM droid
	DROID_CONSTRUCT,        ///< Constructor droid
	DROID_PERSON,           ///< person
	DROID_CYBORG,           ///< cyborg-type thang
	DROID_TRANSPORTER,      ///< guess what this is!
	DROID_COMMAND,          ///< Command droid
	DROID_REPAIR,           ///< Repair droid
	DROID_DEFAULT,          ///< Default droid
	DROID_CYBORG_CONSTRUCT, ///< cyborg constructor droid - new for update 28/5/99
	DROID_CYBORG_REPAIR,    ///< cyborg repair droid - new for update 28/5/99
	DROID_CYBORG_SUPER,     ///< cyborg repair droid - new for update 7/6/99
	DROID_SUPERTRANSPORTER,	///< SuperTransport (MP)
	DROID_ANY,              ///< Any droid. Used as a parameter for various stuff.
};

static inline bool stringToEnumSortFunction(std::pair<char const *, unsigned> const &a, std::pair<char const *, unsigned> const &b)
{
	return strcmp(a.first, b.first) < 0;
}

template <typename Enum>
struct StringToEnum
{
	operator std::pair<char const *, unsigned>() const
	{
		return std::make_pair(string, value);
	}

	char const     *string;
	Enum            value;
};

template <typename Enum>
struct StringToEnumMap : public std::vector<std::pair<char const *, unsigned> >
{
	typedef std::vector<std::pair<char const *, unsigned> > V;

	template <int N>
	static StringToEnumMap<Enum> const &FromArray(StringToEnum<Enum> const(&map)[N])
	{
		static StringToEnum<Enum> const(&myMap)[N] = map;
		static const StringToEnumMap<Enum> sortedMap(map);
		assert(map == myMap);
		(void)myMap;  // Squelch warning in release mode.
		return sortedMap;
	}

	template <int N>
	StringToEnumMap(StringToEnum<Enum> const(&entries)[N]) : V(entries, entries + N)
	{
		std::sort(V::begin(), V::end(), stringToEnumSortFunction);
	}
};

enum COMPONENT_TYPE
{
	COMP_BODY,
	COMP_BRAIN,
	COMP_PROPULSION,
	COMP_REPAIRUNIT,
	COMP_ECM,
	COMP_SENSOR,
	COMP_CONSTRUCT,
	COMP_WEAPON,
	COMP_NUMCOMPONENTS,			/** The number of enumerators in this enum.	 */
};

/**
 * LOC used for holding locations for Sensors and ECM's
 */
enum LOC
{
	LOC_DEFAULT,
	LOC_TURRET,
};

/**
 * SIZE used for specifying body size
 */
enum BODY_SIZE
{
	SIZE_LIGHT,
	SIZE_MEDIUM,
	SIZE_HEAVY,
	SIZE_SUPER_HEAVY,
	SIZE_NUM
};

/**
 * SIZE used for specifying weapon size
 */
enum WEAPON_SIZE
{
	WEAPON_SIZE_LIGHT,
	WEAPON_SIZE_HEAVY,
	WEAPON_SIZE_ANY,
	WEAPON_SIZE_NUM
};

/**
 * Basic weapon type
 */
enum WEAPON_CLASS
{
	WC_KINETIC,					///< bullets etc
	WC_HEAT,					///< laser etc
	WC_NUM_WEAPON_CLASSES		/** The number of enumerators in this enum.	 */
};

/**
 * weapon subclasses used to define which weapons are affected by weapon upgrade
 * functions
 */
enum WEAPON_SUBCLASS
{
	WSC_MGUN,
	WSC_CANNON,
	WSC_MORTARS,
	WSC_MISSILE,
	WSC_ROCKET,
	WSC_ENERGY,
	WSC_GAUSS,
	WSC_FLAME,
	//WSC_CLOSECOMBAT,
	WSC_HOWITZERS,
	WSC_ELECTRONIC,
	WSC_AAGUN,
	WSC_SLOWMISSILE,
	WSC_SLOWROCKET,
	WSC_LAS_SAT,
	WSC_BOMB,
	WSC_COMMAND,
	WSC_EMP,
	WSC_NUM_WEAPON_SUBCLASSES,	/** The number of enumerators in this enum.	 */
};

/**
 * Used to define which projectile model to use for the weapon.
 */
enum MOVEMENT_MODEL
{
	MM_DIRECT,
	MM_INDIRECT,
	MM_HOMINGDIRECT,
	MM_HOMINGINDIRECT
};

/**
 * Used to modify the damage to a propuslion type (or structure) based on
 * weapon.
 */
enum WEAPON_EFFECT
{
	WE_ANTI_PERSONNEL,
	WE_ANTI_TANK,
	WE_BUNKER_BUSTER,
	WE_ARTILLERY_ROUND,
	WE_FLAMER,
	WE_ANTI_AIRCRAFT,
	WE_NUMEFFECTS,			/**  The number of enumerators in this enum. */
};

/**
 * Defines the left and right sides for propulsion IMDs
 */
enum PROP_SIDE
{
	LEFT_PROP,
	RIGHT_PROP,
	NUM_PROP_SIDES,			/**  The number of enumerators in this enum. */
};

enum PROPULSION_TYPE
{
	PROPULSION_TYPE_WHEELED,
	PROPULSION_TYPE_TRACKED,
	PROPULSION_TYPE_LEGGED,
	PROPULSION_TYPE_HOVER,
	PROPULSION_TYPE_LIFT,
	PROPULSION_TYPE_PROPELLOR,
	PROPULSION_TYPE_HALF_TRACKED,
	PROPULSION_TYPE_NUM,	/**  The number of enumerators in this enum. */
};

enum SENSOR_TYPE
{
	STANDARD_SENSOR,
	INDIRECT_CB_SENSOR,
	VTOL_CB_SENSOR,
	VTOL_INTERCEPT_SENSOR,
	SUPER_SENSOR,			///< works as all of the above together! - new for updates
	RADAR_DETECTOR_SENSOR,
};

enum TRAVEL_MEDIUM
{
	GROUND,
	AIR,
};

/*
* Stats structures type definitions
*/

/* Elements common to all stats structures */

/* Stats common to all stats structs */
struct BASE_STATS
{
	BASE_STATS(unsigned ref = 0) : ref(ref), index(0) {}

	UDWORD	ref;    /**< Unique ID of the item */
	QString id;     /**< Text id (i.e. short language-independant name) */
	QString name;   /**< Full / real name of the item */
	int	index;	///< Index into containing array
};

#define getName(_psStats) ((_psStats)->name.isEmpty()? "" : gettext((_psStats)->name.toUtf8().constData()))
#define getID(_psStats) (_psStats)->id.toUtf8().constData()

/* Stats common to all droid components */
struct COMPONENT_STATS : public BASE_STATS
{
	COMPONENT_STATS() : buildPower(0), buildPoints(0), weight(0), body(0), designable(false), pIMD(NULL),
		compType(COMP_NUMCOMPONENTS) {}

	UDWORD		buildPower;			/**< Power required to build the component */
	UDWORD		buildPoints;		/**< Time required to build the component */
	UDWORD		weight;				/**< Component's weight */
	UDWORD		body;				/**< Component's body points */
	bool		designable;			/**< flag to indicate whether this component can be used in the design screen */
	iIMDShape	*pIMD;				/**< The IMD to draw for this component */
	COMPONENT_TYPE	compType;
};

struct PROPULSION_STATS : public COMPONENT_STATS
{
	PROPULSION_STATS() : maxSpeed(0), propulsionType(PROPULSION_TYPE_NUM), turnSpeed(0), spinSpeed(0),
		spinAngle(0), skidDeceleration(0), deceleration(0), acceleration(0) {}

	UDWORD			maxSpeed;		///< Max speed for the droid
	PROPULSION_TYPE propulsionType; ///< Type of propulsion used - index into PropulsionTable
	UDWORD		turnSpeed;
	UDWORD		spinSpeed;
	UDWORD		spinAngle;
	UDWORD		skidDeceleration;
	UDWORD		deceleration;
	UDWORD		acceleration;
};

struct SENSOR_STATS : public COMPONENT_STATS
{
	SENSOR_STATS() : location(0), type(STANDARD_SENSOR), pMountGraphic(NULL)
	{
		memset(&upgrade, 0, sizeof(upgrade));
		memset(&base, 0, sizeof(base));
	}

	UDWORD		location;		///< specifies whether the Sensor is default or for the Turret
	SENSOR_TYPE type;			///< used for combat
	iIMDShape	*pMountGraphic; ///< The turret mount to use

	struct
	{
		unsigned range;
	} upgrade[MAX_PLAYERS], base;
};

struct ECM_STATS : public COMPONENT_STATS
{
	ECM_STATS() : location(0), pMountGraphic(NULL)
	{
		memset(&upgrade, 0, sizeof(upgrade));
		memset(&base, 0, sizeof(base));
	}

	UDWORD location;          ///< specifies whether the ECM is default or for the Turret
	iIMDShape *pMountGraphic; ///< The turret mount to use

	struct
	{
		unsigned range;
	} upgrade[MAX_PLAYERS], base;
};

struct REPAIR_STATS : public COMPONENT_STATS
{
	REPAIR_STATS() : location(0), time(0), pMountGraphic(NULL)
	{
		memset(&upgrade, 0, sizeof(upgrade));
		memset(&base, 0, sizeof(base));
	}

	UDWORD		location;		///< specifies whether the Repair is default or for the Turret
	UDWORD		time;			///< time delay for repair cycle
	iIMDShape	*pMountGraphic; ///< The turret mount to use

	struct
	{
		unsigned repairPoints;		///< The number of points contributed each cycle
	} upgrade[MAX_PLAYERS], base;
};

struct WEAPON_STATS : public COMPONENT_STATS
{
	WEAPON_STATS() : pMountGraphic(NULL), pMuzzleGraphic(NULL), pInFlightGraphic(NULL), pTargetHitGraphic(NULL),
		pTargetMissGraphic(NULL), pWaterHitGraphic(NULL), pTrailGraphic(NULL), iAudioFireID(0),
		iAudioImpactID(0)
	{
		memset(&upgrade, 0, sizeof(upgrade));
		memset(&base, 0, sizeof(base));
	}

	struct
	{
		unsigned maxRange;               ///< Max distance to target for long range shot
		unsigned minRange;               ///< Min distance to target for shot
		unsigned hitChance;              ///< Chance to hit at
		unsigned firePause;              ///< Pause between each shot
		uint8_t numRounds;               ///< The number of rounds per salvo
		unsigned reloadTime;             ///< Time to reload the round of ammo
		unsigned damage;
		unsigned radius;                 ///< Basic blast radius of weapon
		unsigned radiusDamage;           ///< "Splash damage"
		unsigned periodicalDamage;       ///< Repeat damage each second after hit
		unsigned periodicalDamageRadius; ///< Repeat damage radius
		unsigned periodicalDamageTime;   ///< How long the round keeps damaging
		unsigned minimumDamage;          ///< Minimum amount of damage done, in percentage of damage
	} base, upgrade[MAX_PLAYERS];

	WEAPON_CLASS	periodicalDamageWeaponClass;	///< Periodical damage weapon class by damage type (KINETIC, HEAT)
	WEAPON_SUBCLASS	periodicalDamageWeaponSubClass;	///< Periodical damage weapon subclass (research class)
	WEAPON_EFFECT	periodicalDamageWeaponEffect;	///< Periodical damage weapon effect (propulsion/body damage modifier)

	UDWORD			flightSpeed;			///< speed ammo travels at
	bool			fireOnMove;				///< indicates whether the droid has to stop before firing
	WEAPON_CLASS	weaponClass;			///< the class of weapon  (KINETIC, HEAT)
	WEAPON_SUBCLASS weaponSubClass;			///< the subclass to which the weapon belongs (research class)
	MOVEMENT_MODEL	movementModel;			///< which projectile model to use for the bullet
	WEAPON_EFFECT	weaponEffect;			///< which type of warhead is associated with the weapon (propulsion/body damage modifier)
	WEAPON_SIZE		weaponSize;				///< eg light weapons can be put on light bodies or as sidearms
	UDWORD			recoilValue;			///< used to compare with weight to see if recoils or not
	short			rotate;					///< amount the weapon(turret) can rotate 0	= none
	short			maxElevation;			///< max amount the	turret can be elevated up
	short			minElevation;			///< min amount the	turret can be elevated down
	UBYTE			facePlayer;				///< flag to make the (explosion) effect face the	player when	drawn
	UBYTE			faceInFlight;			///< flag to make the inflight effect	face the player when drawn
	uint16_t		effectSize;				///< size of the effect 100 = normal,	50 = half etc
	bool			lightWorld;				///< flag to indicate whether the effect lights up the world
	UBYTE			surfaceToAir;			///< indicates how good in the air - SHOOT_ON_GROUND, SHOOT_IN_AIR or both
	short			vtolAttackRuns;			///< number of attack runs a VTOL droid can	do with this weapon
	bool			penetrate;				///< flag to indicate whether pentrate droid or not
	int			distanceExtensionFactor;	///< max extra distance a projectile can travel if misses target

	/* Graphics control stats */
	UDWORD			radiusLife;				///< How long a blast radius is visible
	UDWORD			numExplosions;			///< The number of explosions per shot

	/* Graphics used for the weapon */
	iIMDShape		*pMountGraphic;			///< The turret mount to use
	iIMDShape		*pMuzzleGraphic;		///< The muzzle flash
	iIMDShape		*pInFlightGraphic;		///< The ammo in flight
	iIMDShape		*pTargetHitGraphic;		///< The ammo hitting a target
	iIMDShape		*pTargetMissGraphic;	///< The ammo missing a target
	iIMDShape		*pWaterHitGraphic;		///< The ammo hitting water
	iIMDShape		*pTrailGraphic;			///< The trail used for in flight

	/* Audio */
	SDWORD			iAudioFireID;
	SDWORD			iAudioImpactID;
};

struct CONSTRUCT_STATS : public COMPONENT_STATS
{
	CONSTRUCT_STATS() : pMountGraphic(NULL)
	{
		memset(&upgrade, 0, sizeof(upgrade));
		memset(&base, 0, sizeof(base));
	}

	iIMDShape	*pMountGraphic;              ///< The turret mount to use

	struct
	{
		unsigned constructPoints;        ///< The number of points contributed each cycle
	} upgrade[MAX_PLAYERS], base;
};

struct BRAIN_STATS : public COMPONENT_STATS
{
	BRAIN_STATS() : psWeaponStat(NULL), maxDroids(0), maxDroidsMult(0) {}

	WEAPON_STATS	*psWeaponStat;	///< weapon stats associated with this brain - for Command Droids
	UDWORD          maxDroids;       ///< base maximum number of droids that the commander can control
	UDWORD          maxDroidsMult;   ///< maximum number of controlled droids multiplied by level
};

/*
 * Stats structures type definitions
 */
#define SHOOT_ON_GROUND 0x01
#define SHOOT_IN_AIR	0x02

struct BODY_STATS : public COMPONENT_STATS
{
	BODY_STATS() : size(SIZE_NUM), weaponSlots(0), droidTypeOverride(DROID_ANY)
	{
		memset(&upgrade, 0, sizeof(upgrade));
		memset(&base, 0, sizeof(base));
	}

	BODY_SIZE	size;			///< How big the body is - affects how hit
	UDWORD		weaponSlots;	///< The number of weapon slots on the body
	DROID_TYPE	droidTypeOverride; // if not DROID_ANY, sets droid type

	std::vector<iIMDShape *> ppIMDList;	///< list of IMDs to use for propulsion unit - up to numPropulsionStats
	std::vector<iIMDShape *> ppMoveIMDList;	///< list of IMDs to use when droid is moving - up to numPropulsionStats
	std::vector<iIMDShape *> ppStillIMDList;///< list of IMDs to use when droid is still - up to numPropulsionStats
	QString         bodyClass;		///< rules hint to script about its classification

	struct
	{
		unsigned power;           ///< this is the engine output of the body
		unsigned body;
		unsigned armour;          ///< A measure of how much protection the armour provides
		int thermal;
		int resistance;
	} upgrade[MAX_PLAYERS], base;
};

/************************************************************************************
* Additional stats tables
************************************************************************************/
struct PROPULSION_TYPES
{
	UWORD	powerRatioMult; ///< Multiplier for the calculated power ratio of the droid
	UDWORD	travel;			///< Which medium the propulsion travels in
	SWORD	startID;		///< sound to play when this prop type starts
	SWORD	idleID;			///< sound to play when this prop type is idle
	SWORD	moveOffID;		///< sound to link moveID and idleID
	SWORD	moveID;			///< sound to play when this prop type is moving
	SWORD	hissID;			///< sound to link moveID and idleID
	SWORD	shutDownID;		///< sound to play when this prop type shuts down
};

typedef UWORD	WEAPON_MODIFIER;

#endif // __INCLUDED_STATSDEF_H__
