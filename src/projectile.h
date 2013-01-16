/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_PROJECTILE_H__
#define __INCLUDED_SRC_PROJECTILE_H__

#include "projectiledef.h"
#include "weapondef.h"

/**
 *	@file projectile.h
 *	Projectile types and function headers
 *	@defgroup Projectile Projectile system
 *	@{
 */

/***************************************************************************/

extern	BASE_OBJECT	*g_pProjLastAttacker;	///< The last unit that did damage - used by script functions

#define PROJ_MAX_PITCH  45
#define PROJ_ULTIMATE_PITCH  80

#define BURN_TIME	10000	///< How long an object burns for after leaving a fire.
#define BURN_DAMAGE	15	///< How much damaga a second an object takes when it is burning.
#define ACC_GRAVITY	1000	///< Downward force against projectiles.

/** How long to display a single electronic warfare shimmmer. */
#define ELEC_DAMAGE_DURATION    (GAME_TICKS_PER_SEC/5)

bool	proj_InitSystem(void);	///< Initialize projectiles subsystem.
void	proj_UpdateAll(void);	///< Frame update for projectiles.
bool	proj_Shutdown(void);	///< Shut down projectile subsystem.

PROJECTILE *proj_GetFirst(void);	///< Get first projectile in the list.
PROJECTILE *proj_GetNext(void);		///< Get next projectile in the list.

void	proj_FreeAllProjectiles(void);	///< Free all projectiles in the list.

/// Calculate the initial velocities of an indirect projectile. Returns the flight time.
int32_t projCalcIndirectVelocities(const int32_t dx, const int32_t dz, int32_t v, int32_t *vx, int32_t *vz, int min_angle);

/** Send a single projectile against the given target. */
bool proj_SendProjectile(WEAPON *psWeap, SIMPLE_OBJECT *psAttacker, int player, Vector3i target, BASE_OBJECT *psTarget, bool bVisible, int weapon_slot);

/** Send a single projectile against the given target
 * with a minimum shot angle. */
bool proj_SendProjectileAngled(WEAPON *psWeap, SIMPLE_OBJECT *psAttacker, int player, Vector3i target, BASE_OBJECT *psTarget, bool bVisible, int weapon_slot, int min_angle, unsigned fireTime);

/** Return whether a weapon is direct or indirect. */
bool proj_Direct(const WEAPON_STATS* psStats);

/** Return the maximum range for a weapon. */
SDWORD	proj_GetLongRange(const WEAPON_STATS* psStats);

extern UDWORD calcDamage(UDWORD baseDamage, WEAPON_EFFECT weaponEffect, BASE_OBJECT *psTarget);
extern bool gfxVisible(PROJECTILE *psObj);

/***************************************************************************/

extern void	objectShimmy	( BASE_OBJECT *psObj );

static inline void setProjectileSource(PROJECTILE *psProj, SIMPLE_OBJECT *psObj)
{
	// use the source of the source of psProj if psAttacker is a projectile
	psProj->psSource = NULL;
	if (psObj == NULL)
	{
	}
	else if (isProjectile(psObj))
	{
		PROJECTILE *psPrevProj = castProjectile(psObj);

		if (psPrevProj->psSource && !psPrevProj->psSource->died)
		{
			psProj->psSource = psPrevProj->psSource;
		}
	}
	else
	{
		psProj->psSource = castBaseObject(psObj);
	}
}

int establishTargetHeight(BASE_OBJECT const *psTarget);

/* @} */

void checkProjectile(const PROJECTILE* psProjectile, const char * const location_description, const char * function, const int recurse);

/* assert if projectile is bad */
#define CHECK_PROJECTILE(object) checkProjectile((object), AT_MACRO, __FUNCTION__, max_check_object_recursion)

#define syncDebugProjectile(psProj, ch) _syncDebugProjectile(__FUNCTION__, psProj, ch)
void _syncDebugProjectile(const char *function, PROJECTILE const *psProj, char ch);

#endif // __INCLUDED_SRC_PROJECTILE_H__
