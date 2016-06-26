
/*
 * This file defines generic things that should be defined
 * prior to defining a ruleset. 
 * 
 * NOTE: This file is not included from main.js .
 * 
 */

// weapon path role
const ROLE = {
	AT: 0, 
	AP: 1, 
	AS: 2, 
	AA: 3,
	LENGTH: 4, // number of items in this enum
}

// something dual to the previous enum
const OBJTYPE = {
	TANK: 0,
	BORG: 1,
	DEFS: 2,
	VTOL: 3,
	LENGTH: 4, // number of items in this enum; should be equal to ROLE.LENGTH
}

// this controls body and weapon compatibility.
// A little explanation:
// w \ b | L | M | H  bodies can't be "ultra-", 
//    UL | + | - | -  ultra-light weapons are for light bodies only,
//     L | + | + | -  light weapons are for light or medium bodies,
//     M | + | + | +  medium weapons are for all bodies,
//     H | - | + | +  heavy weapons are for medium or heavy bodies,
//    UH | - | - | +  ultra-heavy weapons are for heavy bodies only.
const WEIGHT = {
	ULTRALIGHT: 0,
	LIGHT: 1, 
	MEDIUM: 2, 
	HEAVY: 3, 
	ULTRAHEAVY: 4,
}

// what to use this defensive structure for
const DEFROLE = {
	STANDALONE: 0,
	GATEWAY: 1,
	ARTY: 2,
	ANTIAIR: 3,
	FORTRESS: 4,
}

// return values of build order calls
const BUILDRET = {
	SUCCESS: 0,
	UNAVAILABLE: 1,
	FAILURE: 2,
}

// should we execute a build call in an unsafe location?
const IMPORTANCE = {
	PEACETIME: 0,
	MANDATORY: 1,
}

// aspects of every research path
const RESASPECTS = {
	WEAPONS: 0,
	DEFENSES: 1,
	VTOLS: 2,
	EXTRAS: 3,
	LENGTH: 4, // number of items in this enum
}

const PROPULSIONUSAGE = {
    GROUND : 1,
    HOVER: 2,
    VTOL: 4
}

// what to use this body for? (bit field)
const BODYUSAGE = {
	GROUND: 1, // for tanks
	AIR: 2, // for VTOLs
	COMBAT: 3, // GROUND | AIR
	TRUCK: 4, // trucks, sensors and repair droids
	UNIVERSAL: 7, // COMBAT | TRUCK
}

// what sort of weapons this body is resistant to?
const BODYCLASS = {
	KINETIC: 1,
	THERMAL: 2,
}

// should this module be prioritized when not having enough power?
const MODULECOST = {
	CHEAP: 0,
	EXPENSIVE: 1,
}

// how to micromanage tanks with this sort of weapon
const MICRO = {
	RANGED: 0, // normal weapons, such as cannons
	MELEE: 1, // weapons that need to get very close to the enemy, such as flamers
	DUMB: 2, // weapons that can't fire while moving
}
