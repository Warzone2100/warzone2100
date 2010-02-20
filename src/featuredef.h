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
/** \file
 *  Definitions for features.
 */

#ifndef __INCLUDED_FEATUREDEF_H__
#define __INCLUDED_FEATUREDEF_H__

#include "basedef.h"
#include "statsdef.h"

#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

typedef enum _feature_type
{
	FEAT_BUILD_WRECK,
	FEAT_HOVER,
	FEAT_TANK,
	FEAT_GEN_ARTE,
	FEAT_OIL_RESOURCE,
	FEAT_BOULDER,
	FEAT_VEHICLE,
	FEAT_BUILDING,
	FEAT_DROID,
	FEAT_LOS_OBJ,
	FEAT_OIL_DRUM,
	FEAT_TREE,
	FEAT_SKYSCRAPER,
	//FEAT_MESA, // no longer used
	//FEAT_MESA2,
	//FEAT_CLIFF,
	//FEAT_STACK,
	//FEAT_BUILD_WRECK1,
	//FEAT_BUILD_WRECK2,
	//FEAT_BUILD_WRECK3,
	//FEAT_BUILD_WRECK4,
	//FEAT_BOULDER1,
	//FEAT_BOULDER2,
	//FEAT_BOULDER3,
	//FEAT_FUTCAR,
	//FEAT_FUTVAN,
} FEATURE_TYPE;

/* Stats for a feature */
typedef struct _feature_stats
{
	STATS_BASE;

	FEATURE_TYPE    subType;                ///< type of feature

	iIMDShape*      psImd;                  ///< Graphic for the feature
	UWORD           baseWidth;              ///< The width of the base in tiles
	UWORD           baseBreadth;            ///< The breadth of the base in tiles

	BOOL            tileDraw;               ///< Whether the tile needs to be drawn
	BOOL            allowLOS;               ///< Whether the feature allows the LOS. true = can see through the feature
	BOOL            visibleAtStart;         ///< Whether the feature is visible at the start of the mission
	BOOL            damageable;             ///< Whether the feature can be destroyed
	UDWORD		body;			///< Number of body points
	UDWORD          armourValue;            ///< Feature armour
} WZ_DECL_MAY_ALIAS FEATURE_STATS;

typedef struct _feature
{
	/* The common structure elements for all objects */
	BASE_ELEMENTS(struct _feature);

	FEATURE_STATS*  psStats;
} WZ_DECL_MAY_ALIAS FEATURE;

#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __INCLUDED_FEATUREDEF_H__
