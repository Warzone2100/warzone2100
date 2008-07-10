/* This file is generated automatically, do not edit, change the source (stats-db2.tpl) instead. */

#ifndef __INCLUDED_DB_TEMPLATE_SCHEMA_STRUCTDEF_STATS_DB2_H_H__
#define __INCLUDED_DB_TEMPLATE_SCHEMA_STRUCTDEF_STATS_DB2_H_H__

#line 1 "stats-db2.tpl.struct.h"
/* Dummy file, will be filled later on, but needs to exist for code generation
 * of stats-db2.h to be succesfull.
 */
#line 11 "stats-db2.h"

/**
 * if any types are added BEFORE 'COMP_BODY' - then Save/Load Game will have to
 * be  altered since it loops through the components from 1->MAX_COMP
 */
typedef enum COMPONENT_TYPE
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

	/**
	 * The number of enumerators in this enum.
	 */
	COMP_NUMCOMPONENTS,
} COMPONENT_TYPE;

/**
 * LOC used for holding locations for Sensors and ECM's
 */
typedef enum LOC
{
	LOC_DEFAULT,

	LOC_TURRET,
} LOC;

/**
 * SIZE used for specifying body size
 */
typedef enum BODY_SIZE
{
	SIZE_LIGHT,

	SIZE_MEDIUM,

	SIZE_HEAVY,

	SIZE_SUPER_HEAVY,
} BODY_SIZE;

typedef enum PROPULSION_TYPE
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

	/**
	 * The number of enumerators in this enum.
	 */
	PROPULSION_TYPE_NUM,
} PROPULSION_TYPE;

#endif // __INCLUDED_DB_TEMPLATE_SCHEMA_STRUCTDEF_STATS_DB2_TPL_H__
