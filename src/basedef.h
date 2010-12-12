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
 *  Definitions for the base object type.
 */

#ifndef __INCLUDED_BASEDEF_H__
#define __INCLUDED_BASEDEF_H__

#include "lib/framework/vector.h"
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
	OBJ_NUM_TYPES,  ///< number of object types - MUST BE LAST
} OBJECT_TYPE;

typedef struct _tilePos
{
	UBYTE x, y;
} TILEPOS;

/*
 Coordinate system used for objects in Warzone 2100:
  x - "right"
  y - "forward"
  z - "up"

  For explanation of yaw/pitch/roll look for "flight dynamics" in your encyclopedia.
*/

/// NEXTOBJ is an ugly hack to avoid having to fix all occurences of psNext and psNextFunc. The use of the original NEXTOBJ(pointerType) hack wasn't valid C, so in that sense, it's an improvement.
/// NEXTOBJ is a SIMPLE_OBJECT *, which can automatically be cast to DROID *, STRUCTURE *, FEATURE * and PROJECTILE *...
struct NEXTOBJ
{
	NEXTOBJ(struct SIMPLE_OBJECT *ptr_ = NULL) : ptr(ptr_) {}
	NEXTOBJ &operator =(struct SIMPLE_OBJECT *ptr_) { ptr = ptr_; return *this; }
	template<class T>
	operator T * () const { return static_cast<T *>(ptr); }

	struct SIMPLE_OBJECT *ptr;
};

struct SIMPLE_OBJECT
{
	SIMPLE_OBJECT(OBJECT_TYPE type, uint32_t id, unsigned player);
	virtual ~SIMPLE_OBJECT();

	const OBJECT_TYPE type;                         ///< The type of object
	UDWORD          id;                             ///< ID number of the object
	Position        pos;                            ///< Position of the object
	Rotation        rot;                            ///< Object's yaw +ve rotation around up-axis
	UBYTE           player;                         ///< Which player the object belongs to
	UDWORD          born;                           ///< Time the game object was born
	UDWORD          died;                           ///< When an object was destroyed, if 0 still alive
	uint32_t        time;                           ///< Game time of given space-time position.
};

struct BASE_OBJECT : public SIMPLE_OBJECT
{
	BASE_OBJECT(OBJECT_TYPE type, uint32_t id, unsigned player);
	~BASE_OBJECT();

	SCREEN_DISP_DATA    sDisplay;                   ///< screen coordinate details
	UBYTE               group;                      ///< Which group selection is the droid currently in?
	UBYTE               selected;                   ///< Whether the object is selected (might want this elsewhere)
	UBYTE               cluster;                    ///< Which cluster the object is a member of
	UBYTE               visible[MAX_PLAYERS];       ///< Whether object is visible to specific player
	UBYTE               seenThisTick[MAX_PLAYERS];  ///< Whether object has been seen this tick by the specific player.
	UBYTE               inFire;                     ///< true if the object is in a fire
	UWORD               numWatchedTiles;            ///< Number of watched tiles, zero for features
	UDWORD              lastEmission;               ///< When did it last puff out smoke?
	WEAPON_SUBCLASS     lastHitWeapon;              ///< The weapon that last hit it
	UDWORD              timeLastHit;                ///< The time the structure was last attacked
	UDWORD              body;                       ///< Hit points with lame name
	UDWORD              burnStart;                  ///< When the object entered the fire
	UDWORD              burnDamage;                 ///< How much damage has been done since the object entered the fire
	SDWORD              sensorPower;                ///< Active sensor power
	SDWORD              sensorRange;                ///< Range of sensor
	SDWORD              ECMMod;                     ///< Ability to conceal oneself from sensors
	BOOL                bTargetted;                 ///< Whether object is targetted by a selectedPlayer droid sensor (quite the hack)
	TILEPOS             *watchedTiles;              ///< Variable size array of watched tiles, NULL for features
	UDWORD              armour[NUM_HIT_SIDES][WC_NUM_WEAPON_CLASSES];

	NEXTOBJ             psNext;                     ///< Pointer to the next object in the object list
	NEXTOBJ             psNextFunc;                 ///< Pointer to the next object in the function list
};

/// Space-time coordinate.
struct SpaceTime
{
	SpaceTime() {}
	SpaceTime(Position pos_, Rotation rot_, uint32_t time_) : time(time_), pos(pos_), rot(rot_) {}

	uint32_t  time;        ///< Game time

	Position  pos;         ///< Position of the object
	Rotation  rot;         ///< Rotation of the object
};

typedef SpaceTime SPACETIME;

static inline SpaceTime constructSpacetime(Position pos, Rotation rot, uint32_t time)
{
	return SpaceTime(pos, rot, time);
}

#define GET_SPACETIME(psObj) constructSpacetime(psObj->pos, psObj->rot, psObj->time)
#define SET_SPACETIME(psObj, st) do { psObj->pos = st.pos; psObj->rot = st.rot; psObj->time = st.time; } while(0)

static inline bool isDead(const BASE_OBJECT* psObj)
{
	// See objmem.c for comments on the NOT_CURRENT_LIST hack
	return (psObj->died > NOT_CURRENT_LIST);
}

static inline int objPosDiffSq(Position pos1, Position pos2)
{
	const int xdiff = pos1.x - pos2.x;
	const int ydiff = pos1.y - pos2.y;
	return (xdiff * xdiff + ydiff * ydiff);
}

// Must be #included __AFTER__ the definition of BASE_OBJECT
#include "baseobject.h"

#endif // __INCLUDED_BASEDEF_H__
