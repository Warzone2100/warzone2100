/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
 *  Definitions for the base object type.
 */

#ifndef __INCLUDED_BASEDEF_H__
#define __INCLUDED_BASEDEF_H__

#include "lib/ivis_common/pievector.h"
#include "displaydef.h"
#include "statsdef.h"

//the died flag for a droid is set to this when it gets added to the non-current list
#define NOT_CURRENT_LIST 1

typedef enum _object_type
{
	OBJ_DROID,      ///< Droids
	OBJ_STRUCTURE,  ///< All Buildings
	OBJ_FEATURE,    ///< Things like roads, trees, bridges, fires
	OBJ_PROJECTILE, ///< Comes out of guns, stupid :-)
	OBJ_TARGET,     ///< for the camera tracking
} OBJECT_TYPE;

/*
 Coordinate system used for objects in Warzone 2100:
  x - "right"
  y - "forward"
  z - "up"

  For explanation of yaw/pitch/roll look for "flight dynamics" in your encyclopedia.
*/

#define BASE_ELEMENTS1(pointerType) \
	OBJECT_TYPE     type;                           /**< The type of object */ \
	UDWORD          id;                             /**< ID number of the object */ \
	Vector3uw       pos;                            /**< Position of the object */ \
	float           direction;                      /**< Object's yaw +ve rotation around up-axis */ \
	SWORD           pitch;                          /**< Object's pitch +ve rotation around right-axis (nose up/down) */ \
	SWORD           roll                            /**< Object's roll +ve rotation around forward-axis (left wing up/down) */

#define BASE_ELEMENTS2(pointerType) \
	SCREEN_DISP_DATA    sDisplay;                   /**< screen coordinate details */ \
	UBYTE               player;                     /**< Which player the object belongs to */ \
	UBYTE               group;                      /**< Which group selection is the droid currently in? */ \
	UBYTE               selected;                   /**< Whether the object is selected (might want this elsewhere) */ \
	UBYTE               cluster;                    /**< Which cluster the object is a member of */ \
	UBYTE               visible[MAX_PLAYERS];       /**< Whether object is visible to specific player */ \
	UDWORD              died;                       /**< When an object was destroyed, if 0 still alive */ \
	UDWORD              lastEmission;               /**< When did it last puff out smoke? */ \
	UDWORD              lastHitWeapon;		/**< The weapon that last hit it */ \
	UDWORD              timeLastHit;		/**< The time the structure was last attacked */ \
	UDWORD              body;			/**< Hit points with lame name */ \
	BOOL                inFire;                     /**< true if the object is in a fire */ \
	UDWORD              burnStart;                  /**< When the object entered the fire */ \
	UDWORD              burnDamage;                 /**< How much damage has been done since the object entered the fire */ \
	SDWORD              sensorPower;		/**< Active sensor power */ \
	SDWORD              sensorRange;		/**< Range of sensor */ \
	SDWORD              ECMMod;			/**< Ability to conceal oneself from sensors */ \
	UDWORD              armour[NUM_HIT_SIDES][NUM_WEAPON_CLASS]

#define NEXTOBJ(pointerType) \
	pointerType     *psNext                         /**< Pointer to the next object in the list */

#define SIMPLE_ELEMENTS(pointerType) \
	BASE_ELEMENTS1(pointerType); \
	NEXTOBJ(pointerType)

#define BASE_ELEMENTS(pointerType) \
	SIMPLE_ELEMENTS(pointerType); \
	BASE_ELEMENTS2(pointerType)

typedef struct _base_object
{
	BASE_ELEMENTS( struct _base_object );
} WZ_DECL_MAY_ALIAS BASE_OBJECT;

typedef struct SIMPLE_OBJECT
{
	SIMPLE_ELEMENTS( struct SIMPLE_OBJECT );
} SIMPLE_OBJECT;

static inline bool isDead(BASE_OBJECT *psObj)
{
	// See objmem.c for comments on the NOT_CURRENT_LIST hack
	return (psObj->died > NOT_CURRENT_LIST);
}

/* assert if object is bad */
#define CHECK_OBJECT(object) \
do \
{ \
	assert(object != NULL); \
\
	assert(object->type == OBJ_DROID \
	    || object->type == OBJ_STRUCTURE \
	    || object->type == OBJ_FEATURE \
	    || object->type == OBJ_PROJECTILE \
	    || object->type == OBJ_TARGET \
	    || !"invalid object type"); \
\
	assert(object->type == OBJ_FEATURE \
	    || object->type == OBJ_TARGET \
	    || object->player < MAX_PLAYERS \
	    || !"invalid owning player"); \
\
	assert(object->direction <= 360.0f \
	    && object->direction >= 0.0f \
	    && "out of range direction"); \
} while (0)

#endif // __INCLUDED_BASEDEF_H__
