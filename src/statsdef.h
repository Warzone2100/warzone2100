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
#include <bitset>

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

enum WEAPON_FLAGS
{
	WEAPON_FLAG_NO_FRIENDLY_FIRE,
	WEAPON_FLAG_COUNT
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
	BASE_STATS(unsigned ref = 0) : ref(ref) {}

	QString id;     ///< Text id (i.e. short language-independent name)
	QString name;   ///< Full / real name of the item
	unsigned ref;   ///< Unique ID of the item
	int index = 0;  ///< Index into containing array
};

#define getName(_psStats) ((_psStats)->name.isEmpty()? "" : gettext((_psStats)->name.toUtf8().constData()))
#define getID(_psStats) (_psStats)->id.toUtf8().constData()

/* Stats common to all droid components */
struct COMPONENT_STATS : public BASE_STATS
{
	COMPONENT_STATS() {}

	struct UPGRADE
	{
		/// Number of upgradeable hitpoints
		unsigned hitpoints = 0;
		/// Adjust final droid hitpoints by this percentage amount
		int hitpointPct = 100;
	};

	UPGRADE *pBase = nullptr;
	UPGRADE *pUpgrade[MAX_PLAYERS] = { nullptr };
	iIMDShape *pIMD = nullptr;				/**< The IMD to draw for this component */
	unsigned buildPower = 0;			/**< Power required to build the component */
	unsigned buildPoints = 0;		/**< Time required to build the component */
	unsigned weight = 0;				/**< Component's weight */
	COMPONENT_TYPE compType = COMP_NUMCOMPONENTS;
	DROID_TYPE droidTypeOverride = DROID_ANY;
	bool designable = false;		///< Flag to indicate whether this component can be used in the design screen
};

struct PROPULSION_STATS : public COMPONENT_STATS
{
	PROPULSION_STATS()
	{
		pBase = &base;
		for (int i = 0; i < MAX_PLAYERS; i++) pUpgrade[i] = &upgrade[i];
	}

	PROPULSION_TYPE propulsionType = PROPULSION_TYPE_NUM;
	unsigned maxSpeed = 0;		///< Max speed for the droid
	unsigned turnSpeed = 0;
	unsigned spinSpeed = 0;
	unsigned spinAngle = 0;
	unsigned skidDeceleration = 0;
	unsigned deceleration = 0;
	unsigned acceleration = 0;

	struct : UPGRADE
	{
		/// Increase hitpoints by this percentage of the body's hitpoints
		int hitpointPctOfBody = 0;
	} upgrade[MAX_PLAYERS], base;
};

struct SENSOR_STATS : public COMPONENT_STATS
{
	SENSOR_STATS()
	{
		pBase = &base;
		for (int i = 0; i < MAX_PLAYERS; i++) pUpgrade[i] = &upgrade[i];
	}

	iIMDShape *pMountGraphic = nullptr;     ///< The turret mount to use
	unsigned location = 0;                  ///< specifies whether the Sensor is default or for the Turret
	SENSOR_TYPE type = STANDARD_SENSOR;     ///< used for combat

	struct : UPGRADE
	{
		unsigned range = 0;
	} upgrade[MAX_PLAYERS], base;
};

struct ECM_STATS : public COMPONENT_STATS
{
	ECM_STATS()
	{
		pBase = &base;
		for (int i = 0; i < MAX_PLAYERS; i++) pUpgrade[i] = &upgrade[i];
	}

	iIMDShape *pMountGraphic = nullptr;   ///< The turret mount to use
	unsigned location = 0;                ///< Specifies whether the ECM is default or for the Turret

	struct : UPGRADE
	{
		unsigned range = 0;
	} upgrade[MAX_PLAYERS], base;
};

struct REPAIR_STATS : public COMPONENT_STATS
{
	REPAIR_STATS()
	{
		pBase = &base;
		for (int i = 0; i < MAX_PLAYERS; i++) pUpgrade[i] = &upgrade[i];
	}

	iIMDShape *pMountGraphic = nullptr;	///< The turret mount to use
	unsigned location = 0;			///< Specifies whether the Repair is default or for the Turret
	unsigned time = 0;			///< Time delay for repair cycle

	struct : UPGRADE
	{
		unsigned repairPoints = 0;	///< The number of points contributed each cycle
	} upgrade[MAX_PLAYERS], base;
};

struct WEAPON_STATS : public COMPONENT_STATS
{
	WEAPON_STATS() : periodicalDamageWeaponClass(WC_NUM_WEAPON_CLASSES),
	                 periodicalDamageWeaponSubClass(WSC_NUM_WEAPON_SUBCLASSES),
	                 periodicalDamageWeaponEffect(WE_NUMEFFECTS),
	                 weaponClass(WC_NUM_WEAPON_CLASSES),
	                 weaponSubClass(WSC_NUM_WEAPON_SUBCLASSES)
	{
		pBase = &base;
		for (int i = 0; i < MAX_PLAYERS; i++) pUpgrade[i] = &upgrade[i];
	}

	struct : UPGRADE
	{
		unsigned maxRange = 0;               ///< Max distance to target for long range shot
		unsigned minRange = 0;               ///< Min distance to target for shot
		unsigned hitChance = 0;              ///< Chance to hit at
		unsigned firePause = 0;              ///< Pause between each shot
		uint8_t numRounds = 0;               ///< The number of rounds per salvo
		unsigned reloadTime = 0;             ///< Time to reload the round of ammo
		unsigned damage = 0;
		unsigned radius = 0;                 ///< Basic blast radius of weapon
		unsigned radiusDamage = 0;           ///< "Splash damage"
		unsigned periodicalDamage = 0;       ///< Repeat damage each second after hit
		unsigned periodicalDamageRadius = 0; ///< Repeat damage radius
		unsigned periodicalDamageTime = 0;   ///< How long the round keeps damaging
		unsigned minimumDamage = 0;          ///< Minimum amount of damage done, in percentage of damage
	} base, upgrade[MAX_PLAYERS];

	WEAPON_CLASS	periodicalDamageWeaponClass;	///< Periodical damage weapon class by damage type (KINETIC, HEAT)
	WEAPON_SUBCLASS	periodicalDamageWeaponSubClass;	///< Periodical damage weapon subclass (research class)
	WEAPON_EFFECT	periodicalDamageWeaponEffect;	///< Periodical damage weapon effect (propulsion/body damage modifier)

	WEAPON_CLASS weaponClass;			///< the class of weapon  (KINETIC, HEAT)
	WEAPON_SUBCLASS weaponSubClass;			///< the subclass to which the weapon belongs (research class)
	MOVEMENT_MODEL movementModel = MM_DIRECT;	///< which projectile model to use for the bullet
	WEAPON_EFFECT weaponEffect = WE_NUMEFFECTS;	///< which type of warhead is associated with the weapon (propulsion/body damage modifier)
	WEAPON_SIZE weaponSize = WEAPON_SIZE_NUM;	///< eg light weapons can be put on light bodies or as sidearms
	unsigned flightSpeed = 0;			///< speed ammo travels at
	unsigned recoilValue = 0;			///< used to compare with weight to see if recoils or not
	int distanceExtensionFactor = 0;		///< max extra distance a projectile can travel if misses target
	short rotate = 0;				///< amount the weapon(turret) can rotate 0 = none
	short maxElevation = 0;				///< max amount the turret can be elevated up
	short minElevation = 0;				///< min amount the turret can be elevated down
	uint16_t effectSize = 0;			///< size of the effect 100 = normal, 50 = half etc
	short vtolAttackRuns = 0;			///< number of attack runs a VTOL droid can do with this weapon
	UBYTE facePlayer = 0;				///< flag to make the (explosion) effect face the player when drawn
	UBYTE faceInFlight = 0;				///< flag to make the inflight effect face the player when drawn
	UBYTE surfaceToAir = 0;				///< indicates how good in the air - SHOOT_ON_GROUND, SHOOT_IN_AIR or both
	bool lightWorld = false;			///< flag to indicate whether the effect lights up the world
	bool penetrate = false;				///< flag to indicate whether pentrate droid or not
	bool fireOnMove = false;			///< indicates whether the droid has to stop before firing

	std::bitset<WEAPON_FLAG_COUNT> flags;

	/* Graphics control stats */
	unsigned radiusLife = 0;			///< How long a blast radius is visible
	unsigned numExplosions = 0;			///< The number of explosions per shot

	/* Graphics used for the weapon */
	iIMDShape *pMountGraphic = nullptr;		///< The turret mount to use
	iIMDShape *pMuzzleGraphic = nullptr;		///< The muzzle flash
	iIMDShape *pInFlightGraphic = nullptr;		///< The ammo in flight
	iIMDShape *pTargetHitGraphic = nullptr;		///< The ammo hitting a target
	iIMDShape *pTargetMissGraphic = nullptr;	///< The ammo missing a target
	iIMDShape *pWaterHitGraphic = nullptr;		///< The ammo hitting water
	iIMDShape *pTrailGraphic = nullptr;		///< The trail used for in flight

	/* Audio */
	int iAudioFireID = 0;
	int iAudioImpactID = 0;
};

struct CONSTRUCT_STATS : public COMPONENT_STATS
{
	CONSTRUCT_STATS()
	{
		pBase = &base;
		for (int i = 0; i < MAX_PLAYERS; i++) pUpgrade[i] = &upgrade[i];
	}

	iIMDShape *pMountGraphic = nullptr;      ///< The turret mount to use

	struct : UPGRADE
	{
		unsigned constructPoints;        ///< The number of points contributed each cycle
	} upgrade[MAX_PLAYERS], base;
};

struct BRAIN_STATS : public COMPONENT_STATS
{
	BRAIN_STATS()
	{
		pBase = &base;
		for (int i = 0; i < MAX_PLAYERS; i++) pUpgrade[i] = &upgrade[i];
	}

	WEAPON_STATS *psWeaponStat = nullptr;  ///< weapon stats associated with this brain - for Command Droids

	struct : UPGRADE
	{
		std::vector<int> rankThresholds;
		int maxDroids = 0;       ///< base maximum number of droids that the commander can control
		int maxDroidsMult = 0;   ///< maximum number of controlled droids multiplied by level
	} upgrade[MAX_PLAYERS], base;
	std::vector<std::string> rankNames;
};

/*
 * Stats structures type definitions
 */
#define SHOOT_ON_GROUND 0x01
#define SHOOT_IN_AIR	0x02

struct BODY_STATS : public COMPONENT_STATS
{
	BODY_STATS()
	{
		pBase = &base;
		for (int i = 0; i < MAX_PLAYERS; i++) pUpgrade[i] = &upgrade[i];
	}

	BODY_SIZE size = SIZE_NUM;      ///< How big the body is - affects how hit
	unsigned weaponSlots = 0;       ///< The number of weapon slots on the body

	std::vector<iIMDShape *> ppIMDList;	///< list of IMDs to use for propulsion unit - up to numPropulsionStats
	std::vector<iIMDShape *> ppMoveIMDList;	///< list of IMDs to use when droid is moving - up to numPropulsionStats
	std::vector<iIMDShape *> ppStillIMDList;///< list of IMDs to use when droid is still - up to numPropulsionStats
	QString         bodyClass;		///< rules hint to script about its classification

	struct : UPGRADE
	{
		unsigned power = 0;           ///< this is the engine output of the body
		unsigned armour = 0;          ///< A measure of how much protection the armour provides
		int thermal = 0;
		int resistance = 0;
	} upgrade[MAX_PLAYERS], base;
};

/************************************************************************************
* Additional stats tables
************************************************************************************/
struct PROPULSION_TYPES
{
	TRAVEL_MEDIUM travel = GROUND;  ///< Which medium the propulsion travels in
	uint16_t powerRatioMult = 0;    ///< Multiplier for the calculated power ratio of the droid
	int16_t startID = 0;            ///< sound to play when this prop type starts
	int16_t idleID = 0;             ///< sound to play when this prop type is idle
	int16_t moveOffID = 0;          ///< sound to link moveID and idleID
	int16_t moveID = 0;             ///< sound to play when this prop type is moving
	int16_t hissID = 0;             ///< sound to link moveID and idleID
	int16_t shutDownID = 0;         ///< sound to play when this prop type shuts down
};

typedef uint16_t WEAPON_MODIFIER;

#endif // __INCLUDED_STATSDEF_H__
