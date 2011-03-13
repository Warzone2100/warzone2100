/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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

enum FEATURE_TYPE
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
};

/* Stats for a feature */
struct FEATURE_STATS : public BASE_STATS
{
	FEATURE_STATS() {}
	FEATURE_STATS(LineView line);

	FEATURE_TYPE    subType;                ///< type of feature

	iIMDShape*      psImd;                  ///< Graphic for the feature
	UWORD           baseWidth;              ///< The width of the base in tiles
	UWORD           baseBreadth;            ///< The breadth of the base in tiles

	bool            tileDraw;               ///< Whether the tile needs to be drawn
	bool            allowLOS;               ///< Whether the feature allows the LOS. true = can see through the feature
	bool            visibleAtStart;         ///< Whether the feature is visible at the start of the mission
	bool            damageable;             ///< Whether the feature can be destroyed
	UDWORD		body;			///< Number of body points
	UDWORD          armourValue;            ///< Feature armour
};

struct FEATURE : public BASE_OBJECT
{
	FEATURE(uint32_t id, FEATURE_STATS const *psStats);
	~FEATURE();

	FEATURE_STATS const *psStats;
};

#endif // __INCLUDED_FEATUREDEF_H__
