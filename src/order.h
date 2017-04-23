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
	Foundation, Inc. , 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/**
 *
 * @file
 * Function prototypes for giving droids orders.
 *
 */

#ifndef __INCLUDED_SRC_ORDER_H__
#define __INCLUDED_SRC_ORDER_H__

#include "orderdef.h"

/** Retreat positions for the players. This is a global instance of RUN_DATA.*/
extern RUN_DATA asRunData[MAX_PLAYERS];

/** \brief Gives the droid an order. */
extern void orderDroidBase(DROID *psDroid, DROID_ORDER_DATA *psOrder);

/** \brief Initializes the global instance of RUN_DATA. */
extern void initRunData(void);

/** \brief Checks targets of droid's order list. */
void orderCheckList(DROID *psDroid);

/** \brief Updates a droids order state. */
extern void orderUpdateDroid(DROID *psDroid);

/** \brief Sends an order to a droid. */
extern void orderDroid(DROID *psDroid, DROID_ORDER order, QUEUE_MODE mode);

/** \brief Compares droid's order with order. */
extern bool orderState(DROID *psDroid, DROID_ORDER order);

/** \brief Checks if an order is valid for a location. */
bool validOrderForLoc(DROID_ORDER order);

/** \brief Checks if an order is valid for an object. */
bool validOrderForObj(DROID_ORDER order);

/** \brief Sends an order with a location to a droid. */
void orderDroidLoc(DROID *psDroid, DROID_ORDER order, UDWORD x, UDWORD y, QUEUE_MODE mode);

/** \brief Gets the state of a droid order with a location. */
extern bool orderStateLoc(DROID *psDroid, DROID_ORDER order, UDWORD *pX, UDWORD *pY);

/** \brief Sends an order with an object target to a droid. */
void orderDroidObj(DROID *psDroid, DROID_ORDER order, BASE_OBJECT *psObj, QUEUE_MODE mode);

/** \brief Gets the state of a droid's order with an object. */
extern BASE_OBJECT *orderStateObj(DROID *psDroid, DROID_ORDER order);

/** \brief Sends an order with a location and a stat to a droid. */
void orderDroidStatsLocDir(DROID *psDroid, DROID_ORDER order, STRUCTURE_STATS *psStats, UDWORD x, UDWORD y, uint16_t direction, QUEUE_MODE mode);

/** \brief Gets the state of a droid order with a location and a stat. */
extern bool orderStateStatsLoc(DROID *psDroid, DROID_ORDER order, BASE_STATS **ppsStats, UDWORD *pX, UDWORD *pY);

/** \brief Sends an order with a location and a stat to a droid. */
void orderDroidStatsTwoLocDir(DROID *psDroid, DROID_ORDER order, STRUCTURE_STATS *psStats, UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2, uint16_t direction, QUEUE_MODE mode);

/** \brief Sends an order with two locations and a stat to a droid. */
void orderDroidStatsTwoLocDirAdd(DROID *psDroid, DROID_ORDER order, STRUCTURE_STATS *psStats, UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2, uint16_t direction);

/** \brief Sends an order with a location target to all selected droids. add = true queues the order. */
extern void orderSelectedLoc(uint32_t player, uint32_t x, uint32_t y, bool add);

/** \brief Sends an order with an object target to all selected droids. add = true queues the order. */
extern void orderSelectedObj(UDWORD player, BASE_OBJECT *psObj);
extern void orderSelectedObjAdd(UDWORD player, BASE_OBJECT *psObj, bool add);

/** \brief Adds an order to a droids order list. */
void orderDroidAdd(DROID *psDroid, DROID_ORDER_DATA *psOrder);

/** \brief Adds a pending order to a droids order list. */
void orderDroidAddPending(DROID *psDroid, DROID_ORDER_DATA *psOrder);

/** \brief Sends the next order from a droids order list to the droid. */
extern bool orderDroidList(DROID *psDroid);

/** \brief Sends an order with a location and a stat to all selected droids. add = true queues the order. */
void orderSelectedStatsLocDir(UDWORD player, DROID_ORDER order, STRUCTURE_STATS *psStats, UDWORD x, UDWORD y, uint16_t direction, bool add);

/** \brief Sends an order with a location and a stat to all selected droids. add = true queues the order. */
void orderDroidStatsLocDirAdd(DROID *psDroid, DROID_ORDER order, STRUCTURE_STATS *psStats, UDWORD x, UDWORD y, uint16_t direction, bool add = true);

/** \brief Sends an order with two a locations and a stat to all selected droids. add = true queues the order. */
void orderSelectedStatsTwoLocDir(UDWORD player, DROID_ORDER order, STRUCTURE_STATS *psStats, UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2, uint16_t direction, bool add);

/** \brief Sees if a droid supports a given secondary order. */
extern bool secondarySupported(DROID *psDroid, SECONDARY_ORDER sec);

/** \brief Gets the state of a secondary order, return false if unsupported. */
SECONDARY_STATE secondaryGetState(DROID *psDroid, SECONDARY_ORDER sec, QUEUE_MODE mode = ModeImmediate);

/** \brief Sets the state of a secondary order, return false if failed. */
extern bool secondarySetState(DROID *psDroid, SECONDARY_ORDER sec, SECONDARY_STATE State, QUEUE_MODE mode = ModeQueue);

/** \brief Checks the damage level of a droid against it's secondary state. */
extern void secondaryCheckDamageLevel(DROID *psDroid);

/** \brief Makes all the members of a numeric group to have the same secondary states. */
extern void secondarySetAverageGroupState(UDWORD player, UDWORD group);

/** \brief Does a moral check for a player. */
extern void orderMoralCheck(UDWORD player);

/** \brief Does a moral check for a group. */
extern void orderGroupMoralCheck(DROID_GROUP *psGroup);

/** \brief Gets the name of an order. */
extern const char *getDroidOrderName(DROID_ORDER order);

/** \brief Gets a player's transporter. */
DROID *FindATransporter(DROID const *embarkee);

/** \brief Does a health check on a droid. */
extern void orderHealthCheck(DROID *psDroid);

/** \brief Sets the state of a secondary order for a factory. */
extern bool setFactoryState(STRUCTURE *psStruct, SECONDARY_ORDER sec, SECONDARY_STATE State);

/** \brief Gets the state of a secondary order for a Factory. */
extern bool getFactoryState(STRUCTURE *psStruct, SECONDARY_ORDER sec, SECONDARY_STATE *pState);

/** \brief lasSat structure can select a target. */
extern void orderStructureObj(UDWORD player, BASE_OBJECT *psObj);

/** \brief Pops orders (including pending orders) from the order list. */
void orderDroidListEraseRange(DROID *psDroid, unsigned indexBegin, unsigned indexEnd);

/** \brief Clears all orders for the given target (including pending orders) from the order list. */
void orderClearTargetFromDroidList(DROID *psDroid, BASE_OBJECT *psTarget);

/** \brief Chooses an order from a location. */
extern DROID_ORDER chooseOrderLoc(DROID *psDroid, UDWORD x, UDWORD y, bool altOrder);

/** \brief Chooses an order from an object. */
DroidOrder chooseOrderObj(DROID *psDroid, BASE_OBJECT *psObj, bool altOrder);

#endif // __INCLUDED_SRC_ORDER_H__
