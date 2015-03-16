/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
 *  Form droids and structures into clusters
 */

#ifndef __INCLUDED_SRC_CLUSTER_H__
#define __INCLUDED_SRC_CLUSTER_H__

#include "droiddef.h"
#include "structuredef.h"

// maximum number of clusters in a game
#define CLUSTER_MAX		UBYTE_MAX

// cluster information flags
#define CLUSTER_PLAYER_MASK		0x07
#define CLUSTER_DROID			0x08
#define CLUSTER_STRUCTURE		0x10

// Indirect the cluster ID to an actual cluster number
extern UBYTE	aClusterMap[CLUSTER_MAX];

// information about the cluster
extern UBYTE	aClusterInfo[CLUSTER_MAX];

// initialise the cluster system
void clustInitialise(void);

// update routine for the cluster system
void clusterUpdate(void);

// tell the cluster system about a new droid
void clustNewDroid(DROID *psDroid);

// tell the cluster system about a new structure
void clustNewStruct(STRUCTURE *psStruct);

// update the cluster information for an object
void clustUpdateObject(BASE_OBJECT *psObj);

// update all objects from a list belonging to a specific cluster
void clustUpdateCluster(BASE_OBJECT *psList, SDWORD cluster);

// remove an object from the cluster system
void clustRemoveObject(BASE_OBJECT *psObj);

// tell the cluster system that an objects visibility has changed
void clustObjectSeen(BASE_OBJECT *psObj, BASE_OBJECT *psViewer);

// tell the cluster system that an object has been attacked
void clustObjectAttacked(BASE_OBJECT *psObj);

// get the cluster ID for an object
SDWORD clustGetClusterID(BASE_OBJECT *psObj);

// reset the visibility for all clusters for a particular player
void clustResetVisibility(SDWORD player);

#endif // __INCLUDED_SRC_CLUSTER_H__
