/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2019  Warzone 2100 Project

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
 *  Definitions for projectiles.
 */

#ifndef __INCLUDED_PROJECTILEDEF_H__
#define __INCLUDED_PROJECTILEDEF_H__

#include "basedef.h"
#include "lib/gamelib/gtime.h"

#include <vector>


enum PROJ_STATE
{
	PROJ_INFLIGHT,
	PROJ_IMPACT,
	PROJ_POSTIMPACT,
	PROJ_INACTIVE,
};

struct PROJECTILE : public SIMPLE_OBJECT
{
	PROJECTILE(uint32_t id, unsigned player) : SIMPLE_OBJECT(OBJ_PROJECTILE, id, player) {}

	void            update();
	bool            deleteIfDead()
	{
		if (died == 0 || died >= gameTime - deltaGameTime)
		{
			return false;
		}
		delete this;
		return true;
	}

	UBYTE           state;                  ///< current projectile state
	UBYTE           bVisible;               ///< whether the selected player should see the projectile
	WEAPON_STATS   *psWStats;               ///< firing weapon stats
	BASE_OBJECT    *psSource;               ///< what fired the projectile
	BASE_OBJECT    *psDest;                 ///< target of this projectile
	std::vector<BASE_OBJECT *> psDamaged;   ///< the targets that have already been dealt damage to (don't damage the same target twice)

	Vector3i        src = Vector3i(0, 0, 0); ///< Where projectile started
	Vector3i        dst = Vector3i(0, 0, 0); ///< The target coordinates
	SDWORD          vXY, vZ;                ///< axis velocities
	Spacetime       prevSpacetime;          ///< Location of projectile in previous tick.
	UDWORD          expectedDamageCaused;   ///< Expected damage that this projectile will cause to the target.
	int             partVisible;            ///< how much of target was visible on shooting (important for homing)
};

typedef std::vector<PROJECTILE *>::const_iterator ProjectileIterator;


/// True iff object is a projectile.
static inline bool isProjectile(SIMPLE_OBJECT const *psObject)
{
	return psObject != nullptr && psObject->type == OBJ_PROJECTILE;
}
/// Returns PROJECTILE * if projectile or NULL if not.
static inline PROJECTILE *castProjectile(SIMPLE_OBJECT *psObject)
{
	return isProjectile(psObject) ? (PROJECTILE *)psObject : (PROJECTILE *)nullptr;
}
/// Returns PROJECTILE const * if projectile or NULL if not.
static inline PROJECTILE const *castProjectile(SIMPLE_OBJECT const *psObject)
{
	return isProjectile(psObject) ? (PROJECTILE const *)psObject : (PROJECTILE const *)nullptr;
}

#endif // __INCLUDED_PROJECTILEDEF_H__
