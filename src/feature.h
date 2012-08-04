/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
 *  Definitions for the feature structures.
 */

#ifndef __INCLUDED_SRC_FEATURE_H__
#define __INCLUDED_SRC_FEATURE_H__

#include "objectdef.h"

/* The statistics for the features */
extern FEATURE_STATS	*asFeatureStats;
extern UDWORD			numFeatureStats;

//Value is stored for easy access to this feature in destroyDroid()/destroyStruct()
extern FEATURE_STATS* oilResFeature;

/* Load the feature stats */
extern bool loadFeatureStats(const char *pFeatureData, UDWORD bufferSize);

/* Release the feature stats memory */
extern void featureStatsShutDown(void);

/* Create a feature on the map */
extern FEATURE * buildFeature(FEATURE_STATS *psStats, UDWORD x, UDWORD y,bool FromSave);

/* Update routine for features */
extern void featureUpdate(FEATURE *psFeat);

// free up a feature with no visual effects
extern bool removeFeature(FEATURE *psDel);

/* Remove a Feature and free it's memory */
bool destroyFeature(FEATURE *psDel, unsigned impactTime);

/* get a feature stat id from its name */
extern SDWORD getFeatureStatFromName(const char *pName);

int32_t featureDamage(FEATURE *psFeature, unsigned damage, WEAPON_CLASS weaponClass, WEAPON_SUBCLASS weaponSubClass, unsigned impactTime, bool isDamagePerSecond);

extern void     featureInitVars(void);

Vector2i getFeatureStatsSize(FEATURE_STATS const *pStructureType);
StructureBounds getStructureBounds(FEATURE const *object);
StructureBounds getStructureBounds(FEATURE_STATS const *stats, Vector2i pos);

#define syncDebugFeature(psFeature, ch) _syncDebugFeature(__FUNCTION__, psFeature, ch)
void _syncDebugFeature(const char *function, FEATURE const *psFeature, char ch);


// True iff object is a feature.
static inline bool isFeature(SIMPLE_OBJECT const *psObject)             { return psObject != NULL && psObject->type == OBJ_FEATURE; }
// Returns FEATURE * if feature or NULL if not.
static inline FEATURE *castFeature(SIMPLE_OBJECT *psObject)             { return isFeature(psObject)? (FEATURE *)psObject : (FEATURE *)NULL; }
// Returns FEATURE const * if feature or NULL if not.
static inline FEATURE const *castFeature(SIMPLE_OBJECT const *psObject) { return isFeature(psObject)? (FEATURE const *)psObject : (FEATURE const *)NULL; }

#endif // __INCLUDED_SRC_FEATURE_H__
