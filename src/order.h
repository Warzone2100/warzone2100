/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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
 *  Function prototypes for giving droids orders
 */

#ifndef __INCLUDED_SRC_ORDER_H__
#define __INCLUDED_SRC_ORDER_H__

#include "orderdef.h"
#include "structuredef.h"

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

//turn off the build queue availability until desired release date!
//#define DISABLE_BUILD_QUEUE

// The droid orders
typedef enum _droid_order
{
	DORDER_NONE,				// no order set

	DORDER_STOP,				// stop the current order
	DORDER_MOVE,				// 2 - move to a location
	DORDER_ATTACK,				// attack an enemy
	DORDER_BUILD,				// 4 - build a structure
	DORDER_HELPBUILD,			// help to build a structure
	DORDER_LINEBUILD,			// 6 - build a number of structures in a row (walls + bridges)
	DORDER_DEMOLISH,			// demolish a structure
	DORDER_REPAIR,				// 8 - repair a structure
	DORDER_OBSERVE,				// keep a target in sensor view
	DORDER_FIRESUPPORT,			// 10 - attack whatever the linked sensor droid attacks
	DORDER_RETREAT,				// return to the players retreat position
	DORDER_DESTRUCT,			// 12 - self destruct
	DORDER_RTB,					// return to base
	DORDER_RTR,					// 14 - return to repair at any repair facility
	DORDER_RUN,					// run away after moral failure
	DORDER_EMBARK,				// 16 - board a transporter
	DORDER_DISEMBARK,			// get off a transporter
	DORDER_ATTACKTARGET,		// 18 - a suggestion to attack something
								// i.e. the target was chosen because the droid could see it
	DORDER_COMMAND,				// a command droid issuing orders to it's group
	DORDER_BUILDMODULE,			// 20 - build a module (power, research or factory)
	DORDER_RECYCLE,				// return to factory to be recycled
	DORDER_TRANSPORTOUT,		// 22 - offworld transporter order
	DORDER_TRANSPORTIN,			// onworld transporter order
	DORDER_TRANSPORTRETURN,		// 24 - transporter return after unloading
	DORDER_GUARD,				// guard a structure
	DORDER_DROIDREPAIR,			// 26 - repair a droid
	DORDER_RESTORE,				// restore resistance points for a structure
	DORDER_SCOUT,				// 28 - same as move, but stop if an enemy is seen
	DORDER_RUNBURN,				// run away on fire
	DORDER_CLEARWRECK,			// 30 - constructor droid to clear up building wreckage
	DORDER_PATROL,				// move between two way points
	DORDER_REARM,				// 32 - order a vtol to rearming pad
	DORDER_MOVE_ATTACKWALL,		// move to a location taking out a blocking wall on the way
	DORDER_SCOUT_ATTACKWALL,	// 34 - scout to a location taking out a blocking wall on the way
	DORDER_RECOVER,				// pick up an artifact
	DORDER_LEAVEMAP,			// 36 - vtol flying off the map
	DORDER_RTR_SPECIFIED,		// return to repair at a specified repair center
	DORDER_CIRCLE = 40,				// circles target location and engage
	DORDER_TEMP_HOLD,		// hold position until given next order
} DROID_ORDER;

// secondary orders for droids
typedef enum _secondary_order
{
	DSO_ATTACK_RANGE,
	DSO_REPAIR_LEVEL,
	DSO_ATTACK_LEVEL,
	DSO_ASSIGN_PRODUCTION,		// assign production to a command droid - state is the factory number
	DSO_ASSIGN_CYBORG_PRODUCTION,
	DSO_CLEAR_PRODUCTION,		// remove production from a command droid
	DSO_RECYCLE,
	DSO_PATROL,					// patrol between current pos and next move target
	DSO_HALTTYPE,				// what to do when stopped
	DSO_RETURN_TO_LOC,			// return to various locations
	DSO_FIRE_DESIGNATOR,		// command droid controlling IDF structures
	DSO_ASSIGN_VTOL_PRODUCTION,
	DSO_CIRCLE,					// circling target position and engage
} SECONDARY_ORDER;

// the state of secondary orders
typedef enum _secondary_state
{
	DSS_NONE						= 0x000000,
	DSS_ARANGE_SHORT		= 0x000001,
	DSS_ARANGE_LONG			= 0x000002,
	DSS_ARANGE_DEFAULT		= 0x000003,
	DSS_REPLEV_LOW			= 0x000004,
	DSS_REPLEV_HIGH			= 0x000008,
	DSS_REPLEV_NEVER		= 0x00000c,
	DSS_ALEV_ALWAYS			= 0x000010,
	DSS_ALEV_ATTACKED		= 0x000020,
	DSS_ALEV_NEVER			= 0x000030,
	DSS_HALT_HOLD			= 0x000040,
	DSS_HALT_GUARD			= 0x000080,
	DSS_HALT_PURSUE			= 0x0000c0,
	DSS_RECYCLE_SET			= 0x000100,
	DSS_ASSPROD_START		= 0x000200,
	DSS_ASSPROD_MID			= 0x002000,
	DSS_ASSPROD_END			= 0x040000,
	DSS_RTL_REPAIR			= 0x080000,
	DSS_RTL_BASE			= 0x100000,
	DSS_RTL_TRANSPORT		= 0x200000,
	DSS_PATROL_SET			= 0x400000,
	DSS_CIRCLE_SET			= 0x400100,
	DSS_FIREDES_SET			= 0x800000,
} SECONDARY_STATE;

// masks for the secondary order state
#define DSS_ARANGE_MASK		0x000003
#define DSS_REPLEV_MASK		0x00000c
#define DSS_ALEV_MASK		0x000030
#define DSS_HALT_MASK		0x0000c0
#define DSS_RECYCLE_MASK	0x000100
#define DSS_ASSPROD_MASK	0x1f07fe00
#define DSS_ASSPROD_FACT_MASK		0x003e00
#define DSS_ASSPROD_CYB_MASK		0x07c000
#define DSS_ASSPROD_VTOL_MASK		0x1f000000
#define DSS_ASSPROD_SHIFT			9
#define DSS_ASSPROD_CYBORG_SHIFT	(DSS_ASSPROD_SHIFT + 5)
#define DSS_ASSPROD_VTOL_SHIFT		24
#define DSS_RTL_MASK		0x380000
#define DSS_PATROL_MASK		0x400000
#define DSS_FIREDES_MASK	0x800000
#define DSS_CIRCLE_MASK		0x400100

//call this *AFTER* every mission so it gets reset
extern void initRunData(void);

/* Update a droids order state */
extern void orderUpdateDroid(DROID *psDroid);

/* Give a droid an order */
extern void orderDroid(DROID *psDroid, DROID_ORDER order);

/* Check the order state of a droid */
extern BOOL orderState(DROID *psDroid, DROID_ORDER order);

/** Check if an order is valid for a location. */
BOOL validOrderForLoc(DROID_ORDER order);

/** Check if an order is valid for an object. */
BOOL validOrderForObj(DROID_ORDER order);

/* Give a droid an order with a location target */
extern void orderDroidLoc(DROID *psDroid, DROID_ORDER order, UDWORD x, UDWORD y);

/* Get the state of a droid order with a location */
extern BOOL orderStateLoc(DROID *psDroid, DROID_ORDER order, UDWORD *pX, UDWORD *pY);

/* Give a droid an order with an object target */
extern void orderDroidObj(DROID *psDroid, DROID_ORDER order, BASE_OBJECT *psObj);

/* Get the state of a droid order with an object */
extern BASE_OBJECT* orderStateObj(DROID *psDroid, DROID_ORDER order);

/* Give a droid an order with a location and a stat */
extern void orderDroidStatsLoc(DROID *psDroid, DROID_ORDER order,
						BASE_STATS *psStats, UDWORD x, UDWORD y);

/* Get the state of a droid order with a location and a stat */
extern BOOL orderStateStatsLoc(DROID *psDroid, DROID_ORDER order,
						BASE_STATS **ppsStats, UDWORD *pX, UDWORD *pY);

/* Give a droid an order with a location and a stat */
extern void orderDroidStatsTwoLoc(DROID *psDroid, DROID_ORDER order,
						BASE_STATS *psStats, UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2);

extern void orderDroidStatsTwoLocAdd(DROID *psDroid, DROID_ORDER order, BASE_STATS *psStats, UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2);

/* Give selected droids an order with a location target */
// Only called from UI.
extern void orderSelectedLoc(uint32_t player, uint32_t x, uint32_t y, bool add);

/* Give selected droids an order with an object target */
extern void orderSelectedObj(UDWORD player, BASE_OBJECT *psObj);
extern void orderSelectedObjAdd(UDWORD player, BASE_OBJECT *psObj, BOOL add);

// add an order to a droids order list
extern void orderDroidAdd(DROID *psDroid, struct _droid_order_data *psOrder);
// do the next order from a droids order list
extern BOOL orderDroidList(DROID *psDroid);

/* order all selected droids with a location and a stat */
void orderSelectedStatsLoc(UDWORD player, DROID_ORDER order,
						   BASE_STATS *psStats, UDWORD x, UDWORD y, BOOL add);

/* add an order with a location and a stat to the droids order list*/
extern void orderDroidStatsLocAdd(DROID *psDroid, DROID_ORDER order,
						BASE_STATS *psStats, UDWORD x, UDWORD y);

/* order all selected droids with two a locations and a stat */
void orderSelectedStatsTwoLoc(UDWORD player, DROID_ORDER order,
        BASE_STATS *psStats, UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2, BOOL add);

// see if a droid supports a secondary order
extern BOOL secondarySupported(DROID *psDroid, SECONDARY_ORDER sec);

// get the state of a secondary order, return false if unsupported
extern SECONDARY_STATE secondaryGetState(DROID *psDroid, SECONDARY_ORDER sec);

// set the state of a secondary order, return false if failed.
extern BOOL secondarySetState(DROID *psDroid, SECONDARY_ORDER sec, SECONDARY_STATE State);

// check the damage level of a droid against it's secondary state
extern void secondaryCheckDamageLevel(DROID *psDroid);

// make all the members of a numeric group have the same secondary states
extern void secondarySetAverageGroupState(UDWORD player, UDWORD group);

// do a moral check for a player
extern void orderMoralCheck(UDWORD player);

// do a moral check for a group
extern void orderGroupMoralCheck(struct _droid_group *psGroup);

extern const char* getDroidOrderName(DROID_ORDER order);

extern DROID *FindATransporter(void);

/*For a given constructor droid, check if there are any damaged buildings within
a defined range*/
extern BASE_OBJECT * checkForDamagedStruct(DROID *psDroid, STRUCTURE *psTarget);


// do a health check for a droid
extern void orderHealthCheck(DROID *psDroid);

// set the state of a secondary order for a Factory, return false if failed.
extern BOOL setFactoryState(STRUCTURE *psStruct, SECONDARY_ORDER sec, SECONDARY_STATE State);
// get the state of a secondary order for a Factory, return false if unsupported
extern BOOL getFactoryState(STRUCTURE *psStruct, SECONDARY_ORDER sec, SECONDARY_STATE *pState);

//lasSat structure can select a target
extern void orderStructureObj(UDWORD player, BASE_OBJECT *psObj);

static inline void setDroidOrderTarget(DROID *psDroid, void *psNewObject, SDWORD idx)
{
	assert(idx >= 0 && idx < ORDER_LIST_MAX);
	psDroid->asOrderList[idx].psOrderTarget = psNewObject;
}

static inline void removeDroidOrderTarget(DROID *psDroid, SDWORD idx)
{
	assert(idx >= 0 && idx < ORDER_LIST_MAX);
	psDroid->asOrderList[idx].psOrderTarget = NULL;
}

extern DROID_ORDER chooseOrderLoc(DROID *psDroid, UDWORD x,UDWORD y, BOOL altOrder);
extern DROID_ORDER chooseOrderObj(DROID *psDroid, BASE_OBJECT *psObj, BOOL altOrder);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __INCLUDED_SRC_ORDER_H__
