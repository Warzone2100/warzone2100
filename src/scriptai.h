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
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
/** @file
 *  Script functions to support the AI system
 */

#ifndef __INCLUDED_SRC_SCRIPTAI_H__
#define __INCLUDED_SRC_SCRIPTAI_H__

// Add a droid to a group
extern bool scrGroupAddDroid(void);

// Add droids in an area to a group
extern bool scrGroupAddArea(void);

// Add groupless droids in an area to a group
extern bool scrGroupAddAreaNoGroup(void);

// Move the droids from one group to another
extern bool scrGroupAddGroup(void);

// check if a droid is a member of a group
extern bool scrGroupMember(void);

// return number of idle droids in group.
extern bool scrIdleGroup(void);

// initialise iterating a groups members
extern bool scrInitIterateGroup(void);

// iterate through a groups members
extern bool scrIterateGroup(void);

// remove a droid from a group
extern bool scrDroidLeaveGroup(void);

// Give a group an order
extern bool scrOrderGroup(void);

// Give a group an order to a location
extern bool scrOrderGroupLoc(void);

// Give a group an order to an object
extern bool scrOrderGroupObj(void);

// Give a Droid an order
extern bool scrOrderDroid(void);

// Give a Droid an order to a location
extern bool scrOrderDroidLoc(void);

// Give a Droid an order to an object
extern bool scrOrderDroidObj(void);

// Give a Droid an order with a stat
extern bool scrOrderDroidStatsLoc(void);

// set the secondary state for a droid
extern bool scrSetDroidSecondary(void);

// set the secondary state for a droid
extern bool scrSetGroupSecondary(void);

// initialise iterating a cluster
extern bool scrInitIterateCluster(void);

// iterate a cluster
extern bool scrIterateCluster(void);

// add a droid to a commander
extern bool scrCmdDroidAddDroid(void);

// returns max number of droids in a commander group
extern bool scrCmdDroidMaxGroup(void);

// return whether a droid can reach given destination
extern bool scrDroidCanReach(void);

// types for structure targets
enum SCR_STRUCT_TAR
{
	// normal structure types
	SCR_ST_HQ					= 0x00000001,
	SCR_ST_FACTORY				= 0x00000002,
	SCR_ST_POWER_GEN			= 0x00000004,
	SCR_ST_RESOURCE_EXTRACTOR	= 0x00000008,
	SCR_ST_WALL					= 0x00000010,
	SCR_ST_RESEARCH				= 0x00000020,
	SCR_ST_REPAIR_FACILITY		= 0x00000040,
	SCR_ST_COMMAND_CONTROL		= 0x00000080,
	SCR_ST_CYBORG_FACTORY		= 0x00000100,
	SCR_ST_VTOL_FACTORY			= 0x00000200,
	SCR_ST_REARM_PAD			= 0x00000400,
	SCR_ST_SENSOR				= 0x00000800,

	// defensive structure types
	SCR_ST_DEF_GROUND			= 0x00001000,
	SCR_ST_DEF_AIR				= 0x00002000,
	SCR_ST_DEF_IDF				= 0x00004000,
	SCR_ST_DEF_ALL				= 0x00007000,
};


enum SCR_DROID_TAR
{
	// turret types
	SCR_DT_COMMAND				= 0x00000001,
	SCR_DT_SENSOR				= 0x00000002,
	SCR_DT_CONSTRUCT			= 0x00000004,
	SCR_DT_REPAIR				= 0x00000008,
	SCR_DT_WEAP_GROUND			= 0x00000010,
	SCR_DT_WEAP_AIR				= 0x00000020,
	SCR_DT_WEAP_IDF				= 0x00000040,
	SCR_DT_WEAP_ALL				= 0x00000070,

	// body types
	SCR_DT_LIGHT				= 0x00000080,
	SCR_DT_MEDIUM				= 0x00000100,
	SCR_DT_HEAVY				= 0x00000200,
	SCR_DT_SUPER_HEAVY			= 0x00000400,

	// propulsion
	SCR_DT_TRACK				= 0x00000800,
	SCR_DT_HTRACK				= 0x00001000,
	SCR_DT_WHEEL				= 0x00002000,
	SCR_DT_LEGS				= 0x00004000,
	SCR_DT_GROUND				= 0x00007800,
	SCR_DT_VTOL				= 0x00008000,
	SCR_DT_HOVER				= 0x00010000,
	SCR_DT_PROPELLOR			= 0x00020000,
};


// reset the structure preferences
bool scrResetStructTargets(void);
// reset the droid preferences
bool scrResetDroidTargets(void);
// set prefered structure target types
bool scrSetStructTarPref(void);
// set structure target ignore types
bool scrSetStructTarIgnore(void);
// set prefered droid target types
bool scrSetDroidTarPref(void);
// set droid target ignore types
bool scrSetDroidTarIgnore(void);
// get a structure target in an area using the preferences
bool scrStructTargetInArea(void);
// get a structure target on the map using the preferences
bool scrStructTargetOnMap(void);
// get a droid target in an area using the preferences
bool scrDroidTargetInArea(void);
// get a droid target on the map using the preferences
bool scrDroidTargetOnMap(void);
// get a target from a cluster using the preferences
bool scrTargetInCluster(void);

// Skirmish funcs may99

// choose and do research
bool scrSkDoResearch(void);

// find the human players
bool scrSkLocateEnemy(void);

// check a template
bool scrSkCanBuildTemplate(void);

// check for vtol availability
bool scrSkVtolEnableCheck(void);

// check capacity
bool scrSkGetFactoryCapacity(void);

// help/hinder player.
bool scrSkDifficultyModifier(void);

// pick good spots.
bool scrSkDefenseLocation(void);

// line build.
//bool scrSkOrderDroidLineBuild(void);

#endif // __INCLUDED_SRC_SCRIPTAI_H__
