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
 *  Object access functions for the script library
 */

#ifndef __INCLUDED_SRC_SCRIPTOBJ_H__
#define __INCLUDED_SRC_SCRIPTOBJ_H__

#include "group.h"

// id's for object variables
enum _objids
{
	OBJID_POSX,			// Position of a base object
	OBJID_POSY,
	OBJID_POSZ,
	OBJID_ID,			// id of a base object
	OBJID_PLAYER,		// player of a base object
	OBJID_TYPE,			// type of a base object
	OBJID_ORDER,		// current droid order
	OBJID_DROIDTYPE,	// what type of droid
	OBJID_CLUSTERID,	// which cluster the object is a member of
	OBJID_HEALTH,		// %age damage level
	OBJID_BODY,			// the body component
	OBJID_PROPULSION,	// the propulsion component
	OBJID_WEAPON,		// the weapon component
	OBJID_STRUCTSTAT,	// the stat of a structure
	OBJID_STRUCTSTATTYPE,//new
	OBJID_ORDERX,		// order coords.106
	OBJID_ORDERY,
	OBJID_ACTION,
	OBJID_SELECTED,		// if droid is selected (humans only)
	OBJID_TARGET,		// added object->psTarget
	OBJID_GROUP,		// group a droid belongs to
	WEAPID_SHORT_RANGE,		// short range of a weapon
	WEAPID_LONG_RANGE,		// short range of a weapon
	WEAPID_SHORT_HIT,		// weapon's chance to hit in the short range
	WEAPID_LONG_HIT,		// weapon's chance to hit in the long range
	WEAPID_FIRE_PAUSE,		// weapon's fire pause
	WEAPID_RELOAD_TIME,		// weapon's reload time
	WEAPID_NUM_ROUNDS,		// num of weapon's rounds (salvo fire)
	WEAPID_DAMAGE,			// weapon's damage
	OBJID_HITPOINTS,			// doid's health left
	OBJID_ORIG_HITPOINTS,		// original health of a droid (when not damaged)
};

// id's for group variables
enum _groupids
{
	GROUPID_POSX,		// average x of a group
	GROUPID_POSY,		// average y of a group
	GROUPID_MEMBERS,	// number of units in a group
	GROUPID_HEALTH,		// average health of a group
	GROUPID_TYPE,		// group type, one of: GT_NORMAL, GT_COMMAND or GT_TRANSPORTER
	GROUPID_CMD,		// commander of the group if group type == GT_COMMAND
};

// Get values from a base object
extern BOOL scrBaseObjGet(UDWORD index);

// Set values from a base object
extern BOOL scrBaseObjSet(UDWORD index);

// convert a base object to a droid if it is the right type
extern BOOL scrObjToDroid(void);

// convert a base object to a structure if it is the right type
extern BOOL scrObjToStructure(void);

// convert a base object to a feature if it is the right type
extern BOOL scrObjToFeature(void);

// Get values from a group
extern BOOL scrGroupObjGet(UDWORD index);

// Get values from a weapon
extern BOOL scrWeaponObjGet(UDWORD index);

// default value save routine
extern BOOL scrValDefSave(INTERP_VAL *psVal, char *pBuffer, UDWORD *pSize);

// default value load routine
extern BOOL scrValDefLoad(SDWORD version, INTERP_VAL *psVal, char *pBuffer, UDWORD size);

extern Vector2i luaWZ_checkWorldCoords(lua_State *L, int param);
extern int luaWZObj_checkstructurestat(lua_State *L, int pos);
extern int luaWZObj_checkfeaturestat(lua_State *L, int pos);
extern BASE_OBJECT *luaWZObj_checkobject(lua_State *L, int pos, int type);
extern BASE_OBJECT *luaWZObj_checkbaseobject(lua_State *L, int pos);
extern unsigned int luaWZObj_toid(lua_State *L, int pos);
extern void luaWZObj_pushstructure(lua_State *L, STRUCTURE* structure);
extern void luaWZObj_pushfeature(lua_State *L, FEATURE* feature);
extern void luaWZObj_pushdroid(lua_State *L, DROID* droid);
extern void luaWZObj_pushweapon(lua_State *L, WEAPON* weapon);
extern void luaWZObj_pushweaponstat(lua_State *L, int index);
extern const char* luaWZObj_checkname(lua_State *L, int pos);
extern DROID_TEMPLATE* luaWZObj_checktemplate(lua_State *L, int pos);
extern void luaWZObj_pushtemplate(lua_State *L, DROID_TEMPLATE *template);

extern void luaWZObj_pushgroup(lua_State *L, DROID_GROUP *group);
extern DROID_GROUP *luaWZObj_checkgroup(lua_State *L, int pos);
/// Push a droid, structure or feature when you don't know what it is
extern void luaWZObj_pushobject(lua_State *L, BASE_OBJECT* baseobject);

#endif // __INCLUDED_SRC_SCRIPTOBJ_H__
