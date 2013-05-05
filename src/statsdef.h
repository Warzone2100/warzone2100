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
*  Definitions for the stats system.
*/
#ifndef __INCLUDED_STATSDEF_H__
#define __INCLUDED_STATSDEF_H__

#include "lib/ivis_opengl/ivisdef.h"

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

static inline bool stringToEnumSortFunction(std::pair<char const *, unsigned> const &a, std::pair<char const *, unsigned> const &b) { return strcmp(a.first, b.first) < 0; }
template <typename STATS>
static inline STATS *findStatsByName(std::string const &name, STATS *asStats, unsigned numStats)
{
	for (unsigned inc = 0; inc < numStats; ++inc)  // Could be more efficient, if the stats were sorted by name...
	{
		//compare the names
		if (name == asStats[inc].pName)
		{
			return &asStats[inc];
		}
	}
	return NULL;  // Not found.
}
template <typename STATS>
static inline STATS *findStatsByName(std::string const &name, STATS **asStats, unsigned numStats)
{
	for (unsigned inc = 0; inc < numStats; ++inc)  // Could be more efficient, if the stats were sorted by name...
	{
		//compare the names
		if (name == asStats[inc]->pName)
		{
			return asStats[inc];
		}
	}
	return NULL;  // Not found.
}

template <typename Enum>
struct StringToEnum
{
	operator std::pair<char const *, unsigned>() const { return std::make_pair(string, value); }

	char const *    string;
	Enum            value;
};

template <typename Enum>
struct StringToEnumMap : public std::vector<std::pair<char const *, unsigned> >
{
	typedef std::vector<std::pair<char const *, unsigned> > V;

	template <int N>
	static StringToEnumMap<Enum> const &FromArray(StringToEnum<Enum> const (&map)[N])
	{
		static StringToEnum<Enum> const (&myMap)[N] = map;
		static const StringToEnumMap<Enum> sortedMap(map);
		assert(map == myMap);
		(void)myMap;  // Squelch warning in release mode.
		return sortedMap;
	}

	template <int N>
	StringToEnumMap(StringToEnum<Enum> const (&entries)[N]) : V(entries, entries + N) { std::sort(V::begin(), V::end(), stringToEnumSortFunction); }
};

/// Read-only view of file data in "A,1,2\nB,3,4" format as a 2D array-like object. Note — does not copy the file data.
class TableView
{
public:
	TableView(char const *buffer, unsigned size);

	bool isError() const { return !parseError.isEmpty(); }  ///< If returning true, there was an error parsing the table.
	QString getError() const { return parseError; }         ///< Returns an error message about what went wrong.

	unsigned size() const { return lines.size(); }

private:
	friend class LineView;
	char const *                                buffer;
	std::vector<uint32_t>                       cells;
	std::vector<std::pair<uint32_t, uint32_t> > lines;  // List of pairs of offsets into the cells array and the number of cells in the line.
	QString                                     parseError;
	std::string                                 returnString;
};

/// Read-only view of a line of file data in "A,1,2" format as an array-like object. Note — does not copy the file data.
class LineView
{
public:
	LineView(class TableView &table, unsigned lineNumber) : table(table), cells(&table.cells[table.lines.at(lineNumber).first]), numCells(table.lines.at(lineNumber).second), lineNumber(lineNumber) {}  ///< This LineView is only valid for the lifetime of the TableView.

	bool isError() const { return !table.parseError.isEmpty(); }  ///< If returning true, there was an error parsing the table.
	void setError(unsigned index, char const *error);  ///< Only the first error is saved.

	unsigned size() const { return numCells; }
	unsigned line() const { return lineNumber; }

	/// Function for reading a bool (in the form of "0" or "1") in the line.
	bool b(unsigned index) { return i(index, 0, 1); }

	/// Functions for reading integers in the line.
	int64_t i(unsigned index, int64_t min, int64_t max);
	uint32_t i8(unsigned index)  { return i(index, INT8_MIN,  INT8_MAX);   }
	uint32_t u8(unsigned index)  { return i(index, 0,         UINT8_MAX);  }
	uint32_t i16(unsigned index) { return i(index, INT16_MIN, INT16_MAX);  }
	uint32_t u16(unsigned index) { return i(index, 0,         UINT16_MAX); }
	uint32_t i32(unsigned index) { return i(index, INT32_MIN, INT32_MAX);  }
	uint32_t u32(unsigned index) { return i(index, 0,         UINT32_MAX); }

	/// Function for reading a float in the line. (Should not use the result to affect game state.)
	float f(unsigned index, float min = -1.e30f, float max = 1.e30f);

	/// Function for reading the raw contents of the cell as a string.
	std::string const &s(unsigned index);

	// Function for reading enum values. The StringToEnum map should give a list of strings and corresponding enum values.
	template <typename Enum>
	Enum e(unsigned index, StringToEnumMap<Enum> const &map) { return (Enum)eu(index, map); }
	template <typename Enum, int N>
	Enum e(unsigned index, StringToEnum<Enum> const (&map)[N]) { return e(index, StringToEnumMap<Enum>::FromArray(map)); }

	/// Returns the .pie file data referenced by the cell. May return NULL without error if the cell is "0" and accept0AsNULL is true.
	iIMDShape *imdShape(unsigned index, bool accept0AsNULL = false);
	/// Returns the .pie files data referenced by the cell, separated by '@' characters.
	std::vector<iIMDShape *> imdShapes(unsigned index);

	/// Returns the STATS * in the given list with the same name as this cell. May return NULL without error if the cell is "0" and accept0AsNULL is true.
	template <typename STATS>
	inline STATS *stats(unsigned index, STATS *asStats, unsigned numStats, bool accept0AsNULL = false)
	{
		std::string const &name = s(index);
		if (accept0AsNULL && name == "0")
		{
			return NULL;
		}
		STATS *ret = findStatsByName(name, asStats, numStats);
		if (ret == NULL)
		{
			setError(index, "Couldn't find stats.");
		}
		return ret;
	}
	/// Returns the STATS * in the given list with the same name as this cell. May return NULL without error if the cell is "0" and accept0AsNULL is true.
	template <typename STATS>
	inline STATS *stats(unsigned index, STATS **asStats, unsigned numStats, bool accept0AsNULL = false)
	{
		std::string const &name = s(index);
		if (accept0AsNULL && name == "0")
		{
			return NULL;
		}
		STATS *ret = findStatsByName(name, asStats, numStats);
		if (ret == NULL)
		{
			setError(index, "Couldn't find stats.");
		}
		return ret;
	}


private:
	unsigned eu(unsigned index, std::vector<std::pair<char const *, unsigned> > const &map);
	bool checkRange(unsigned index);
	class TableView &       table;
	unsigned const *        cells;
	unsigned                numCells;
	unsigned                lineNumber;
};

/*
if any types are added BEFORE 'COMP_BODY' - then Save/Load Game will have to be
altered since it loops through the components from 1->MAX_COMP
*/
enum COMPONENT_TYPE
{
	COMP_UNKNOWN,
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
	PROPULSION_TYPE_SKI,
	PROPULSION_TYPE_LIFT,
	PROPULSION_TYPE_PROPELLOR,
	PROPULSION_TYPE_HALF_TRACKED,
	PROPULSION_TYPE_JUMP,
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
	BASE_STATS(unsigned ref = 0) : ref(ref), pName(NULL) {}   ///< Only initialised here when using new/delete! TODO Use new/delete only, not malloc()/free().
	BASE_STATS(unsigned ref, std::string const &str);   ///< Only initialised here when using new/delete! TODO Use new/delete only, not malloc()/free(). TODO Then pName could be a QString...
	//Gah, too soon to add destructors to BASE_STATS, thanks to local temporaries that are copied with memcpy()... --- virtual ~BASE_STATS() { free(pName); }  ///< pName is only freed here when using new/delete! TODO Use new/delete only, not malloc()/free().
	//So this one isn't needed for now, maybe not ever. --- BASE_STATS(BASE_STATS const &stats) : ref(stats.ref), pName(strdup(stats.pName)) {}  // TODO Not needed when pName is a QString...
	//So this one isn't needed for now, maybe not ever. --- BASE_STATS const &operator =(BASE_STATS const &stats) { ref = stats.ref; free(pName); pName = strdup(stats.pName); return *this; }  // TODO Not needed when pName is a QString...

	UDWORD	ref;	/**< Unique ID of the item */
	char	*pName; /**< pointer to the text id name (i.e. short language-independant name) */
};

/* Stats common to all droid components */
struct COMPONENT_STATS : public BASE_STATS
{
	UDWORD		buildPower;			/**< Power required to build the component */
	UDWORD		buildPoints;		/**< Time required to build the component */
	UDWORD		weight;				/**< Component's weight */
	UDWORD		body;				/**< Component's body points */
	bool		designable;			/**< flag to indicate whether this component can be used in the design screen */
	iIMDShape	*pIMD;				/**< The IMD to draw for this component */
};

struct PROPULSION_STATS : public COMPONENT_STATS
{
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
	UDWORD		location;		///< specifies whether the Sensor is default or for the Turret
	SENSOR_TYPE type;			///< used for combat
	UDWORD		time;			///< time delay before associated weapon droids 'know' where the attack is from
	iIMDShape	*pMountGraphic; ///< The turret mount to use

	struct
	{
		int range;
	} upgrade[MAX_PLAYERS], base;
};

struct ECM_STATS : public COMPONENT_STATS
{
	UDWORD		location;		///< specifies whether the ECM is default or for the Turret
	iIMDShape	*pMountGraphic; ///< The turret mount to use

	struct
	{
		int range;
	} upgrade[MAX_PLAYERS], base;
};

struct REPAIR_STATS : public COMPONENT_STATS
{
	UDWORD		location;		///< specifies whether the Repair is default or for the Turret
	UDWORD		time;			///< time delay for repair cycle
	iIMDShape	*pMountGraphic; ///< The turret mount to use

	struct
	{
		short repairPoints;		///< The number of points contributed each cycle
	} upgrade[MAX_PLAYERS], base;
};

struct WEAPON_STATS : public COMPONENT_STATS
{
	struct
	{
		short maxRange;
		short minRange;
		short hitChance;
		short firePause;     ///< Pause between each shot
		short numRounds;     ///< The number of rounds per salvo
		short reloadTime;    ///< Time to reload the round of ammo
		short damage;
		short radius;        ///< Basic blast radius of weapon
		short radiusDamage;  ///< "Splash damage"
		short periodicalDamage; ///< Repeat damage each second after hit
		short periodicalDamageRadius; ///< Repeat damage radius
		short periodicalDamageTime; ///< How long the round keeps damaging
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
	iIMDShape	*pMountGraphic;		///< The turret mount to use

	struct
	{
		short constructPoints;		///< The number of points contributed each cycle
	} upgrade[MAX_PLAYERS], base;
};

struct BRAIN_STATS : public COMPONENT_STATS
{
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
	BODY_SIZE	size;			///< How big the body is - affects how hit
	UDWORD		weaponSlots;	///< The number of weapon slots on the body
	DROID_TYPE	droidTypeOverride; // if not DROID_ANY, sets droid type

	// A measure of how much energy the power plant outputs
	iIMDShape	**ppIMDList;	///< list of IMDs to use for propulsion unit - up to numPropulsionStats
	iIMDShape	*pFlameIMD;		///< pointer to which flame graphic to use - for VTOLs only at the moment
	char		bodyClass[40]; // TBD make into QString once this struct is class-safe

	struct
	{
		int power;
		int body;
		int armour;
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

struct TERRAIN_TABLE
{
	UDWORD	speedFactor;	///< factor to multiply the speed by depending on the method of propulsion and the terrain type - to be divided by 100 before use
};

typedef UWORD	WEAPON_MODIFIER;

/* weapon stats which can be upgraded by research*/
struct WEAPON_UPGRADE
{
	UBYTE	firePause;
	UWORD	shortHit;
	UWORD	longHit;
	UWORD	damage;
	UWORD	radiusDamage;
	UWORD	periodicalDamage;
	UWORD	radiusHit;
};

#endif // __INCLUDED_STATSDEF_H__
