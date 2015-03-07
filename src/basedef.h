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
/** \file
 *  Definitions for the base object type.
 */

#ifndef __INCLUDED_BASEDEF_H__
#define __INCLUDED_BASEDEF_H__

#include "lib/framework/vector.h"
#include "animobj.h"
#include "displaydef.h"
#include "statsdef.h"

//the died flag for a droid is set to this when it gets added to the non-current list
#define NOT_CURRENT_LIST 1

enum OBJECT_TYPE
{
	OBJ_DROID,      ///< Droids
	OBJ_STRUCTURE,  ///< All Buildings
	OBJ_FEATURE,    ///< Things like roads, trees, bridges, fires
	OBJ_PROJECTILE, ///< Comes out of guns, stupid :-)
	OBJ_TARGET,     ///< for the camera tracking
	OBJ_NUM_TYPES,  ///< number of object types - MUST BE LAST
};

struct TILEPOS
{
	UBYTE x, y, type;
};

/*
 Coordinate system used for objects in Warzone 2100:
  x - "right"
  y - "forward"
  z - "up"

  For explanation of yaw/pitch/roll look for "flight dynamics" in your encyclopedia.
*/

/// NEXTOBJ is an ugly hack to avoid having to fix all occurences of psNext and psNextFunc. The use of the original NEXTOBJ(pointerType) hack wasn't valid C, so in that sense, it's an improvement.
/// NEXTOBJ is a BASE_OBJECT *, which can automatically be cast to DROID *, STRUCTURE * and FEATURE *...

struct BASE_OBJECT;

struct NEXTOBJ
{
	NEXTOBJ(BASE_OBJECT *ptr_ = NULL) : ptr(ptr_) {}
	NEXTOBJ &operator =(BASE_OBJECT *ptr_) { ptr = ptr_; return *this; }
	template<class T>
	operator T * () const { return static_cast<T *>(ptr); }

	BASE_OBJECT *ptr;
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

#define BASEFLAG_TARGETED  0x01 ///< Whether object is targeted by a selectedPlayer droid sensor (quite the hack)
#define BASEFLAG_DIRTY     0x02 ///< Whether certain recalculations are needed for object on frame update

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
	UWORD               numWatchedTiles;            ///< Number of watched tiles, zero for features
	UDWORD              lastEmission;               ///< When did it last puff out smoke?
	WEAPON_SUBCLASS     lastHitWeapon;              ///< The weapon that last hit it
	UDWORD              timeLastHit;                ///< The time the structure was last attacked
	UDWORD              body;                       ///< Hit points with lame name
	UDWORD              periodicalDamageStart;                  ///< When the object entered the fire
	UDWORD              periodicalDamage;                 ///< How much damage has been done since the object entered the fire
	uint16_t            flags;                      ///< Various flags
	TILEPOS             *watchedTiles;              ///< Variable size array of watched tiles, NULL for features
	ANIM_OBJECT         *psCurAnim;                 ///< Animation frames

	NEXTOBJ             psNext;                     ///< Pointer to the next object in the object list
	NEXTOBJ             psNextFunc;                 ///< Pointer to the next object in the function list
};

/// Space-time coordinate, including orientation.
struct Spacetime
{
	Spacetime() : time(0), pos(0, 0, 0), rot(0, 0, 0) {}
	Spacetime(Position pos_, Rotation rot_, uint32_t time_) : time(time_), pos(pos_), rot(rot_) {}

	uint32_t  time;        ///< Game time

	Position  pos;         ///< Position of the object
	Rotation  rot;         ///< Rotation of the object
};

static inline Spacetime getSpacetime(SIMPLE_OBJECT const *psObj) { return Spacetime(psObj->pos, psObj->rot, psObj->time); }
static inline void setSpacetime(SIMPLE_OBJECT *psObj, Spacetime const &st) { psObj->pos = st.pos; psObj->rot = st.rot; psObj->time = st.time; }

static inline bool isDead(const BASE_OBJECT* psObj)
{
	// See objmem.c for comments on the NOT_CURRENT_LIST hack
	return (psObj->died > NOT_CURRENT_LIST);
}

static inline int objPosDiffSq(Position pos1, Position pos2)
{
	const Vector2i diff = removeZ(pos1 - pos2);
	return diff*diff;
}

static inline int objPosDiffSq(SIMPLE_OBJECT const *pos1, SIMPLE_OBJECT const *pos2)
{
	return objPosDiffSq(pos1->pos, pos2->pos);
}

// True iff object is a droid, structure or feature (not a projectile). Will incorrectly return true if passed a nonsense object of type OBJ_TARGET or OBJ_NUM_TYPES.
static inline bool isBaseObject(SIMPLE_OBJECT const *psObject)                 { return psObject != NULL && psObject->type != OBJ_PROJECTILE; }
// Returns BASE_OBJECT * if base_object or NULL if not.
static inline BASE_OBJECT *castBaseObject(SIMPLE_OBJECT *psObject)             { return isBaseObject(psObject)? (BASE_OBJECT *)psObject : (BASE_OBJECT *)NULL; }
// Returns BASE_OBJECT const * if base_object or NULL if not.
static inline BASE_OBJECT const *castBaseObject(SIMPLE_OBJECT const *psObject) { return isBaseObject(psObject)? (BASE_OBJECT const *)psObject : (BASE_OBJECT const *)NULL; }

// Must be #included __AFTER__ the definition of BASE_OBJECT
#include "baseobject.h"

#endif // __INCLUDED_BASEDEF_H__
