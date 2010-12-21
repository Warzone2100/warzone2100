/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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
 *  Definitions for missions.
 */

#ifndef __INCLUDED_MISSIONDEF_H__
#define __INCLUDED_MISSIONDEF_H__

#include "map.h"
#include "structuredef.h"
#include "droiddef.h"
#include "featuredef.h"
#include "power.h"

//mission types

//used to set the reinforcement time on hold whilst the Transporter is unable to land
//hopefully they'll never need to set it this high for other reasons!
#define SCR_LZ_COMPROMISED_TIME     999999
//this is used to compare the value passed in from the scripts with which is multiplied by 100
#define LZ_COMPROMISED_TIME         99999900

typedef struct _landing_zone
{
	uint8_t x1;
	uint8_t y1;
	uint8_t x2;
	uint8_t y2;
} LANDING_ZONE;

//storage structure for values that need to be kept between missions
typedef struct _mission
{
	UDWORD				type;							//defines which start and end functions to use - see levels_type in levels.h
	MAPTILE				*psMapTiles;					//the original mapTiles
	UDWORD				mapWidth;						//the original mapWidth
	UDWORD				mapHeight;						//the original mapHeight
	struct _gateway		*psGateways;					//the gateway list
	UDWORD				scrollMinX;						//scroll coords for original map
	UDWORD				scrollMinY;
	UDWORD				scrollMaxX;
	UDWORD				scrollMaxY;
	STRUCTURE			*apsStructLists[MAX_PLAYERS], *apsExtractorLists[MAX_PLAYERS];	//original object lists
	DROID						*apsDroidLists[MAX_PLAYERS];
	FEATURE						*apsFeatureLists[MAX_PLAYERS];
	BASE_OBJECT			*apsSensorList[1];
	FEATURE				*apsOilList[1];
	//struct _proximity_display	*apsProxDisp[MAX_PLAYERS];
	FLAG_POSITION				*apsFlagPosLists[MAX_PLAYERS];
	int32_t                         asCurrentPower[MAX_PLAYERS];

	UDWORD				startTime;			//time the mission started
	SDWORD				time;				//how long the mission can last
											// < 0 = no limit
	SDWORD				ETA;				//time taken for reinforcements to arrive
											// < 0 = none allowed
   	UDWORD				cheatTime;			//time the cheating started (mission time-wise!)

	//LANDING_ZONE		homeLZ;
    UWORD               homeLZ_X;           //selectedPlayer's LZ x and y
    UWORD               homeLZ_Y;
	SDWORD				playerX;			//original view position
	SDWORD				playerY;

	/* transporter entry/exit tiles */
	UWORD				iTranspEntryTileX[MAX_PLAYERS];
	UWORD				iTranspEntryTileY[MAX_PLAYERS];
	UWORD				iTranspExitTileX[MAX_PLAYERS];
	UWORD				iTranspExitTileY[MAX_PLAYERS];

} MISSION;

#endif // __INCLUDED_MISSIONDEF_H__
