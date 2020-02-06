/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_COMPONENT_H__
#define __INCLUDED_SRC_COMPONENT_H__

#include "lib/ivis_opengl/pietypes.h"
#include "droiddef.h"
#include "structuredef.h"
#include <glm/fwd.hpp>

/*
	Header file for component.c
	Pumpkin Studios, EIDOS Interactive.
*/

bool setPlayerColour(UDWORD player, UDWORD col);
UBYTE getPlayerColour(UDWORD pl);

UDWORD getComponentDroidRadius(DROID *psDroid);
UDWORD getComponentDroidTemplateRadius(DROID_TEMPLATE *psDroid);
UDWORD getComponentRadius(BASE_STATS *psComponent);
UDWORD getResearchRadius(BASE_STATS *Stat);
UDWORD getStructureSizeMax(STRUCTURE *psStructure);
UDWORD getStructureStatSizeMax(STRUCTURE_STATS *Stats);

#define BLIP_ANIM_DURATION			(200)
#define OBJECT_RADIUS				(128)
#define COMPONENT_RADIUS			(64)
#define DESIGN_DROID_SCALE			(200)
#define DESIGN_COMPONENT_SCALE			(150)
#define RESEARCH_COMPONENT_SCALE		(300)
#define COMP_BUT_SCALE				(100)
#define DROID_BUT_SCALE				(72)
#define SMALL_STRUCT_SCALE			(55)
#define MED_STRUCT_SCALE			(25)  //reduced from 30 to fit command centre in window
#define LARGE_STRUCT_SCALE			(25)
#define ULTRA_SMALL_FEATURE_SCALE		(146)
#define REALLY_SMALL_FEATURE_SCALE		(116)
#define SMALL_FEATURE_SCALE			(55)
#define MED_FEATURE_SCALE			(26)
#define LARGE_FEATURE_SCALE			(16)

#define TOWER_HEIGHT    100
UDWORD getStructureStatHeight(STRUCTURE_STATS *psStat);

void displayIMDButton(iIMDShape *IMDShape, const Vector3i *Rotation, const Vector3i *Position, int scale);
void displayStructureButton(STRUCTURE *psStructure, const Vector3i *Rotation, const Vector3i *Position, int scale);
void displayStructureStatButton(STRUCTURE_STATS *Stats, const Vector3i *Rotation, const Vector3i *Position, int scale);
void displayComponentButton(BASE_STATS *Stat, const Vector3i *Rotation, const Vector3i *Position, int scale);
void displayResearchButton(BASE_STATS *Stat, const Vector3i *Rotation, const Vector3i *Position, int scale);
void displayComponentButtonTemplate(DROID_TEMPLATE *psTemplate, const Vector3i *Rotation, const Vector3i *Position, int scale);
void displayComponentButtonObject(DROID *psDroid, const Vector3i *Rotation, const Vector3i *Position, int scale);
void displayComponentObject(DROID *psDroid, const glm::mat4 &viewMatrix);

void compPersonToBits(DROID *psDroid);

SDWORD rescaleButtonObject(SDWORD radius, SDWORD baseScale, SDWORD baseRadius);
void destroyFXDroid(DROID *psDroid, unsigned impactTime);

void drawMuzzleFlash(WEAPON sWeap, iIMDShape *weaponImd, iIMDShape *flashImd, PIELIGHT buildingBrightness, int pieFlag, int pieFlagData, const glm::mat4 &viewMatrix, UBYTE colour = 0);

/* Pass in the stats you're interested in and the COMPONENT - double reference, but works. NOTE: Unused!*/
#define PART_IMD(STATS,DROID,COMPONENT,PLAYER)	(STATS[DROID->asBits[COMPONENT]].pIMD)

/* Get the chassis imd */
#define BODY_IMD(DROID,PLAYER)	(asBodyStats[DROID->asBits[COMP_BODY]].pIMD)
/* Get the brain imd - NOTE: Unused!*/
#define BRAIN_IMD(DROID,PLAYER)	(asBrainStats[DROID->asBits[COMP_BRAIN]].pIMD)
/* Get the weapon imd */
#define WEAPON_IMD(DROID,WEAPON_NUM)	(asWeaponStats[DROID->asWeaps[WEAPON_NUM].nStat].pIMD)
/* Get the propulsion imd  THIS IS A LITTLE MORE COMPLICATED NOW!*/
//#define PROPULSION_IMD(DROID,PLAYER)	(asPropulsionStats[DROID->asBits[COMP_PROPULSION]].pIMD[PLAYER])
/* Get the sensor imd */
#define SENSOR_IMD(DROID,PLAYER)	(asSensorStats[DROID->asBits[COMP_SENSOR]].pIMD)
/* Get an ECM imd!?! */
#define ECM_IMD(DROID,PLAYER)	(asECMStats[DROID->asBits[COMP_ECM]].pIMD)
/* Get an Repair imd!?! */
#define REPAIR_IMD(DROID,PLAYER)	(asRepairStats[DROID->asBits[COMP_REPAIRUNIT]].pIMD)
/* Get a construct imd */
#define CONSTRUCT_IMD(DROID,PLAYER)	(asConstructStats[DROID->asBits[COMP_CONSTRUCT]].pIMD)
/* Get a weapon mount imd*/
#define WEAPON_MOUNT_IMD(DROID,WEAPON_NUM)	(asWeaponStats[DROID->asWeaps[WEAPON_NUM].nStat].pMountGraphic)
/* Get a sensor mount imd*/
#define SENSOR_MOUNT_IMD(DROID,PLAYER)	(asSensorStats[DROID->asBits[COMP_SENSOR]].pMountGraphic)
/* Get a construct mount imd*/
#define CONSTRUCT_MOUNT_IMD(DROID,PLAYER)	(asConstructStats[DROID->asBits[COMP_CONSTRUCT]].pMountGraphic)
/* Get a ecm mount imd*/
#define ECM_MOUNT_IMD(DROID,PLAYER)	(asECMStats[DROID->asBits[COMP_ECM]].pMountGraphic)
/* Get a repair mount imd*/
#define REPAIR_MOUNT_IMD(DROID,PLAYER)	(asRepairStats[DROID->asBits[COMP_REPAIRUNIT]].pMountGraphic)
/* Get a muzzle flash pie*/
#define MUZZLE_FLASH_PIE(DROID,WEAPON_NUM)	(asWeaponStats[DROID->asWeaps[WEAPON_NUM].nStat].pMuzzleGraphic)

#endif // __INCLUDED_SRC_COMPONENT_H__
