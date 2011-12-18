/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

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
/***************************************************************************/
/*
 * Projectile functions
 *
 */
/***************************************************************************/
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/trig.h"

#include "lib/gamelib/gtime.h"
#include "objects.h"
#include "move.h"
#include "action.h"
#include "combat.h"
#include "effects.h"
#include "map.h"
#include "lib/sound/audio_id.h"
#include "lib/sound/audio.h"
#include "anim_id.h"
#include "projectile.h"
#include "visibility.h"
#include "lib/script/script.h"
#include "scripttabs.h"
#include "scriptcb.h"
#include "group.h"
#include "cmddroid.h"
#include "feature.h"
#include "lib/ivis_opengl/piestate.h"
#include "loop.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/piematrix.h"

#include "scores.h"

#include "display3d.h"
#include "display.h"
#include "multiplay.h"
#include "multistat.h"
#include "mapgrid.h"
#include "random.h"

#include <algorithm>
#include <functional>

#define VTOL_HITBOX_MODIFICATOR 100

struct INTERVAL
{
	int begin, end;  // Time 1 = 0, time 2 = 1024. Or begin >= end if empty.
};

// Watermelon:they are from droid.c
/* The range for neighbouring objects */
#define PROJ_NEIGHBOUR_RANGE (TILE_UNITS*4)
// used to create a specific ID for projectile objects to facilitate tracking them.
static const UDWORD ProjectileTrackerID =	0xdead0000;

/* The list of projectiles in play */
static std::vector<PROJECTILE *> psProjectileList;

/* The next projectile to give out in the proj_First / proj_Next methods */
static ProjectileIterator psProjectileNext;

/***************************************************************************/

// the last unit that did damage - used by script functions
BASE_OBJECT		*g_pProjLastAttacker;

/***************************************************************************/

struct ObjectShape
{
	ObjectShape() {}
	ObjectShape(int radius) : isRectangular(false), size(radius, radius) {}
	ObjectShape(int width, int breadth) : isRectangular(true), size(width, breadth) {}
	ObjectShape(Vector2i widthBreadth) : isRectangular(true), size(widthBreadth) {}
	int radius() const { return size.x; }

	bool     isRectangular;  ///< True if rectangular, false if circular.
	Vector2i size;           ///< x == y if circular.
};

static ObjectShape establishTargetShape(BASE_OBJECT *psTarget);
static void	proj_ImpactFunc( PROJECTILE *psObj );
static void	proj_PostImpactFunc( PROJECTILE *psObj );
static void	proj_checkBurnDamage( BASE_OBJECT *apsList, PROJECTILE *psProj);

static int32_t objectDamage(BASE_OBJECT *psObj, UDWORD damage, WEAPON_CLASS weaponClass, WEAPON_SUBCLASS weaponSubClass);


static inline void setProjectileDestination(PROJECTILE *psProj, BASE_OBJECT *psObj)
{
	aiObjectAddExpectedDamage(psProj->psDest, -psProj->expectedDamageCaused);  // The old target shouldn't be expecting any more damage from this projectile.
	psProj->psDest = psObj;
	aiObjectAddExpectedDamage(psProj->psDest, psProj->expectedDamageCaused);  // Let the new target know to say its prayers.
}


/***************************************************************************/
bool gfxVisible(PROJECTILE *psObj)
{
	// Already know it is visible
	if (psObj->bVisible)
	{
		return true;
	}

	// You fired it
	if (psObj->player == selectedPlayer)
	{
		return true;
	}

	// Someone elses structure firing at something you can't see
	if (psObj->psSource != NULL
	 && !psObj->psSource->died
	 && psObj->psSource->type == OBJ_STRUCTURE
	 && psObj->psSource->player != selectedPlayer
	 && (psObj->psDest == NULL
	  || psObj->psDest->died
	  || !psObj->psDest->visible[selectedPlayer]))
	{
		return false;
	}

	// Something you cannot see firing at a structure that isn't yours
	if (psObj->psDest != NULL
	 && !psObj->psDest->died
	 && psObj->psDest->type == OBJ_STRUCTURE
	 && psObj->psDest->player != selectedPlayer
	 && (psObj->psSource == NULL
	  || !psObj->psSource->visible[selectedPlayer]))
	{
		return false;
	}

	// You can see the source
	if (psObj->psSource != NULL
	 && !psObj->psSource->died
	 && psObj->psSource->visible[selectedPlayer])
	{
		return true;
	}

	// You can see the destination
	if (psObj->psDest != NULL
	 && !psObj->psDest->died
	 && psObj->psDest->visible[selectedPlayer])
	{
		return true;
	}

	return false;
}

/***************************************************************************/

bool
proj_InitSystem( void )
{
	psProjectileList.clear();
	psProjectileNext = psProjectileList.end();

	return true;
}

/***************************************************************************/

// Clean out all projectiles from the system, and properly decrement
// all reference counts.
void
proj_FreeAllProjectiles( void )
{
	psProjectileList.clear();
	psProjectileNext = psProjectileList.end();
}

/***************************************************************************/

bool
proj_Shutdown( void )
{
	proj_FreeAllProjectiles();

	return true;
}

/***************************************************************************/

// Reset the first/next methods, and give out the first projectile in the list.
PROJECTILE *
proj_GetFirst( void )
{
	psProjectileNext = psProjectileList.begin();
	return psProjectileNext != psProjectileList.end()? *psProjectileNext : NULL;
}

/***************************************************************************/

// Get the next projectile
PROJECTILE *
proj_GetNext( void )
{
	++psProjectileNext;
	return psProjectileNext != psProjectileList.end()? *psProjectileNext : NULL;
}

/***************************************************************************/

/*
 * Relates the quality of the attacker to the quality of the victim.
 * The value returned satisfies the following inequality: 0.5 <= ret/65536 <= 2.0
 */
static uint32_t qualityFactor(DROID *psAttacker, DROID *psVictim)
{
	uint32_t powerRatio = (uint64_t)65536 * calcDroidPower(psVictim) / calcDroidPower(psAttacker);
	uint32_t pointsRatio = (uint64_t)65536 * calcDroidPoints(psVictim) / calcDroidPoints(psAttacker);

	CLIP(powerRatio, 65536/2, 65536*2);
	CLIP(pointsRatio, 65536/2, 65536*2);

	return (powerRatio + pointsRatio) / 2;
}

// update the kills after a target is damaged/destroyed
static void proj_UpdateKills(PROJECTILE *psObj, int32_t experienceInc)
{
	DROID	        *psDroid;
	BASE_OBJECT     *psSensor;

	CHECK_PROJECTILE(psObj);

	if (psObj->psSource == NULL || (psObj->psDest && psObj->psDest->type == OBJ_FEATURE)
	    || (psObj->psDest && psObj->psSource->player == psObj->psDest->player))	// no exp for friendly fire
	{
		return;
	}

	// If experienceInc is negative then the target was killed
	if (bMultiPlayer && experienceInc < 0)
	{
		updateMultiStatsKills(psObj->psDest, psObj->psSource->player);
	}

	// Since we are no longer interested if it was killed or not, abs it
	experienceInc = abs(experienceInc);

	if (psObj->psSource->type == OBJ_DROID)			/* update droid kills */
	{
		psDroid = (DROID *) psObj->psSource;

		// If it is 'droid-on-droid' then modify the experience by the Quality factor
		// Only do this in MP so to not un-balance the campaign
		if (psObj->psDest != NULL
		 && psObj->psDest->type == OBJ_DROID
		 && bMultiPlayer)
		{
			// Modify the experience gained by the 'quality factor' of the units
			experienceInc = (uint64_t)experienceInc * qualityFactor(psDroid, (DROID *)psObj->psDest) / 65536;
		}

		ASSERT_OR_RETURN(, 0 <= experienceInc && experienceInc < (int)(2.1*65536), "Experience increase out of range");

		psDroid->experience += experienceInc;
		cmdDroidUpdateKills(psDroid, experienceInc);

		psSensor = orderStateObj(psDroid, DORDER_FIRESUPPORT);
		if (psSensor
		 && psSensor->type == OBJ_DROID)
		{
		    ((DROID *)psSensor)->experience += experienceInc;
		}
	}
	else if (psObj->psSource->type == OBJ_STRUCTURE)
	{
		ASSERT_OR_RETURN(, 0 <= experienceInc && experienceInc < (int)(2.1*65536), "Experience increase out of range");

		// See if there was a command droid designating this target
		psDroid = cmdDroidGetDesignator(psObj->psSource->player);

		if (psDroid != NULL
		 && psDroid->action == DACTION_ATTACK
		 && psDroid->psActionTarget[0] == psObj->psDest)
		{
			psDroid->experience += experienceInc;
		}
	}
}

/***************************************************************************/

void _syncDebugProjectile(const char *function, PROJECTILE const *psProj, char ch)
{
	_syncDebug(function, "%c projectile = p%d;pos(%d,%d,%d),rot(%d,%d,%d),state%d,edc%d,nd%lu", ch,
	          psProj->player,
	          psProj->pos.x, psProj->pos.y, psProj->pos.z,
	          psProj->rot.direction, psProj->rot.pitch, psProj->rot.roll,
	          psProj->state,
	          psProj->expectedDamageCaused,
	          (unsigned long)psProj->psDamaged.size());
}

static int32_t randomVariation(int32_t val)
{
	// Up to ±5% random variation.
	return (int64_t)val*(95000 + gameRand(10001))/100000;
}

int32_t projCalcIndirectVelocities(const int32_t dx, const int32_t dz, int32_t v, int32_t *vx, int32_t *vz, int min_angle)
{
	// Find values of vx and vz, which solve the equations:
	// dz = -1/2 g t² + vz t
	// dx = vx t
	// v² = vx² + vz²
	// Increases v, if needed for there to be a solution. Decreases v, if needed for vz > 0.
	// Randomly changes v by up to 2.5%, so the shots don't all follow the same path.

	const int32_t g = ACC_GRAVITY;                         // In units/s².
	int32_t a = randomVariation(v*v) - dz*g;               // In units²/s².
	uint64_t b = g*g*((uint64_t)dx*dx + (uint64_t)dz*dz);  // In units⁴/s⁴. Casting to uint64_t does sign extend the int32_t.
	int64_t c = (uint64_t)a*a - b;                         // In units⁴/s⁴.
	if (c < 0)
	{
		// Must increase velocity, target too high. Find the smallest possible a (which corresponds to the smallest possible velocity).

		a = i64Sqrt(b) + 1;                            // Still in units²/s². Adding +1, since i64Sqrt rounds down.
		c = (uint64_t)a*a - b;                         // Still in units⁴/s⁴. Should be 0, plus possible rounding errors.
	}

	int32_t t = MAX(1, iSqrt(2*(a - i64Sqrt(c)))*(GAME_TICKS_PER_SEC/g));   // In ticks. Note that a - √c ≥ 0, since c ≤ a². Try changing the - to +, and watch the mini-rockets.
	*vx = dx*GAME_TICKS_PER_SEC/t;                                          // In units/sec.
	*vz = dz*GAME_TICKS_PER_SEC/t + g*t/(2*GAME_TICKS_PER_SEC);             // In units/sec.

	STATIC_ASSERT(GAME_TICKS_PER_SEC/ACC_GRAVITY*ACC_GRAVITY == GAME_TICKS_PER_SEC);  // On line that calculates t, must cast iSqrt to uint64_t, and remove brackets around TICKS_PER_SEC/g, if changing ACC_GRAVITY.

	if (*vz < 0)
	{
		// Don't want to shoot downwards, reduce velocity and let gravity take over.
		t = MAX(1, i64Sqrt(-2*dz*(uint64_t)GAME_TICKS_PER_SEC*GAME_TICKS_PER_SEC/g));  // Still in ticks.
		*vx = dx*GAME_TICKS_PER_SEC/t;             // Still in units/sec.
		*vz = 0;                                   // Still in units/sec. (Wouldn't really matter if it was pigeons/inch, since it's 0 anyway.)
	}

	/* CorvusCorax: Check against min_angle */
	if (iAtan2(*vz, *vx) < min_angle)
	{
		/* set pitch to pass terrain */
		// tan(min_angle)=mytan/65536
		int64_t mytan = ((int64_t)iSin(min_angle)*65536)/iCos(min_angle);
		t = MAX(1, i64Sqrt(2*((int64_t)dx*mytan - dz*65536)*(int64_t)GAME_TICKS_PER_SEC*GAME_TICKS_PER_SEC/(int64_t)(g*65536)));  // Still in ticks.
		*vx = dx*GAME_TICKS_PER_SEC/t;
		// mytan=65536*vz/vx
		*vz = (mytan*(*vx))/65536;
	}

	return t;
}

bool proj_SendProjectile(WEAPON *psWeap, SIMPLE_OBJECT *psAttacker, int player, Vector3i target, BASE_OBJECT *psTarget, bool bVisible, int weapon_slot)
{
	return proj_SendProjectileAngled(psWeap, psAttacker, player, target, psTarget, bVisible, weapon_slot, 0);
}

bool proj_SendProjectileAngled(WEAPON *psWeap, SIMPLE_OBJECT *psAttacker, int player, Vector3i target, BASE_OBJECT *psTarget, bool bVisible, int weapon_slot, int min_angle)
{
	WEAPON_STATS *psStats = &asWeaponStats[psWeap->nStat];

	ASSERT_OR_RETURN( false, psWeap->nStat < numWeaponStats, "Invalid range referenced for numWeaponStats, %d > %d", psWeap->nStat, numWeaponStats);
	ASSERT_OR_RETURN( false, psStats != NULL, "Invalid weapon stats" );
	ASSERT_OR_RETURN( false, psTarget == NULL || !psTarget->died, "Aiming at dead target!" );

	PROJECTILE *psProj = new PROJECTILE(ProjectileTrackerID | (realTime >> 4), player);

	/* get muzzle offset */
	if (psAttacker == NULL)
	{
		// if there isn't an attacker just start at the target position
		// NB this is for the script function to fire the las sats
		psProj->src = target;
	}
	else if (psAttacker->type == OBJ_DROID && weapon_slot >= 0)
	{
		calcDroidMuzzleLocation((DROID *)psAttacker, &psProj->src, weapon_slot);
		/*update attack runs for VTOL droid's each time a shot is fired*/
		updateVtolAttackRun((DROID *)psAttacker, weapon_slot);
	}
	else if (psAttacker->type == OBJ_STRUCTURE && weapon_slot >= 0)
	{
		calcStructureMuzzleLocation((STRUCTURE *)psAttacker, &psProj->src, weapon_slot);
	}
	else // incase anything wants a projectile
	{
		psProj->src = psAttacker->pos;
	}

	/* Initialise the structure */
	psProj->psWStats		= psStats;

	psProj->pos             = psProj->src;
	psProj->dst             = target;

	psProj->bVisible = false;

	psProj->died = 0;

	// Must set ->psDest and ->expectedDamageCaused before first call to setProjectileDestination().
	psProj->psDest = NULL;
	psProj->expectedDamageCaused = objGuessFutureDamage(psStats, player, psTarget);
	setProjectileDestination(psProj, psTarget);  // Updates expected damage of psProj->psDest, using psProj->expectedDamageCaused.

	/*
	When we have been created by penetration (spawned from another projectile),
	we shall live no longer than the original projectile may have lived
	*/
	if (psAttacker && psAttacker->type == OBJ_PROJECTILE)
	{
		PROJECTILE * psOldProjectile = (PROJECTILE*)psAttacker;
		psProj->born = psOldProjectile->born;

		psProj->prevSpacetime.time = psOldProjectile->time;  // Have partially ticked already.
		psProj->time = gameTime;
		psProj->prevSpacetime.time -=  psProj->prevSpacetime.time == psProj->time;  // Times should not be equal, for interpolation.

		setProjectileSource(psProj, psOldProjectile->psSource);
		psProj->psDamaged = psOldProjectile->psDamaged;

		// TODO Should finish the tick, when penetrating.
	}
	else
	{
		psProj->born = gameTime;

		psProj->prevSpacetime.time = gameTime - deltaGameTime;  // Haven't ticked yet.
		psProj->time = psProj->prevSpacetime.time;

		setProjectileSource(psProj, psAttacker);
	}

	if (psTarget)
	{
		int maxHeight = establishTargetHeight(psTarget);
		int minHeight = std::min(std::max(maxHeight + 2 * LINE_OF_FIRE_MINIMUM - areaOfFire(psAttacker, psTarget, weapon_slot, true), 0), maxHeight);
		scoreUpdateVar(WD_SHOTS_ON_TARGET);

		psProj->dst.z = psTarget->pos.z + minHeight + gameRand(std::max(maxHeight - minHeight, 1));
		/* store visible part (LOCK ON this part for homing :) */
		psProj->partVisible = maxHeight - minHeight;
	}
	else
	{
		psProj->dst.z = target.z + LINE_OF_FIRE_MINIMUM;
		scoreUpdateVar(WD_SHOTS_OFF_TARGET);
	}

	Vector3i deltaPos = psProj->dst - psProj->src;

	/* roll never set */
	psProj->rot.roll = 0;

	psProj->rot.direction = iAtan2(removeZ(deltaPos));


	// Get target distance, horizontal distance only.
	uint32_t dist = iHypot(removeZ(deltaPos));

	if (proj_Direct(psStats) || (!proj_Direct(psStats) && dist <= psStats->minRange))
	{
		psProj->rot.pitch = iAtan2(deltaPos.z, dist);
		psProj->state = PROJ_INFLIGHTDIRECT;
	}
	else
	{
		/* indirect */
		projCalcIndirectVelocities(dist, deltaPos.z, psStats->flightSpeed, &psProj->vXY, &psProj->vZ, min_angle);
		psProj->rot.pitch = iAtan2(psProj->vZ, psProj->vXY);
		psProj->state = PROJ_INFLIGHTINDIRECT;
	}

	// If droid or structure, set muzzle pitch.
	if (psAttacker != NULL && weapon_slot >= 0)
	{
		if (psAttacker->type == OBJ_DROID)
		{
			((DROID *)psAttacker)->asWeaps[weapon_slot].rot.pitch = psProj->rot.pitch;
		}
		else if (psAttacker->type == OBJ_STRUCTURE)
		{
			((STRUCTURE *)psAttacker)->asWeaps[weapon_slot].rot.pitch = psProj->rot.pitch;
		}
	}

	/* put the projectile object in the global list */
	psProjectileList.push_back(psProj);

	/* play firing audio */
	// only play if either object is visible, i know it's a bit of a hack, but it avoids the problem
	// of having to calculate real visibility values for each projectile.
	if ( bVisible || gfxVisible(psProj) )
	{
		// note that the projectile is visible
		psProj->bVisible = true;

		if ( psStats->iAudioFireID != NO_SOUND )
		{

			if ( psProj->psSource )
			{
				/* firing sound emitted from source */
				audio_PlayObjDynamicTrack(psProj->psSource, psStats->iAudioFireID, NULL);
				/* GJ HACK: move howitzer sound with shell */
				if ( psStats->weaponSubClass == WSC_HOWITZERS )
				{
					audio_PlayObjDynamicTrack(psProj, ID_SOUND_HOWITZ_FLIGHT, NULL);
				}
			}
			//don't play the sound for a LasSat in multiPlayer
			else if (!(bMultiPlayer && psStats->weaponSubClass == WSC_LAS_SAT))
			{
					audio_PlayObjStaticTrack(psProj, psStats->iAudioFireID);
			}
		}
	}

	if (psAttacker != NULL && !proj_Direct(psStats))
	{
		//check for Counter Battery Sensor in range of target
		counterBatteryFire(castBaseObject(psAttacker), psTarget);
	}

	syncDebugProjectile(psProj, '*');

	CHECK_PROJECTILE(psProj);

	return true;
}

/***************************************************************************/

static INTERVAL intervalIntersection(INTERVAL i1, INTERVAL i2)
{
	INTERVAL ret = {MAX(i1.begin, i2.begin), MIN(i1.end, i2.end)};
	return ret;
}

static bool intervalEmpty(INTERVAL i)
{
	return i.begin >= i.end;
}

static INTERVAL collisionZ(int32_t z1, int32_t z2, int32_t height)
{
	INTERVAL ret = {-1, -1};
	if (z1 > z2)
	{
		z1 *= -1;
		z2 *= -1;
	}

	if (z1 > height || z2 < -height)
		return ret;  // No collision between time 1 and time 2.

	if (z1 == z2)
	{
		if (z1 >= -height && z1 <= height)
		{
			ret.begin = 0;
			ret.end = 1024;
		}
		return ret;
	}

	ret.begin = 1024*(-height - z1)/(z2 - z1);
	ret.end   = 1024*( height - z1)/(z2 - z1);
	return ret;
}

static INTERVAL collisionXY(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t radius)
{
	// Solve (1 - t)v1 + t v2 = r.
	int32_t dx = x2 - x1, dy = y2 - y1;
	int64_t a = (int64_t)dx*dx + (int64_t)dy*dy;                           // a = (v2 - v1)²
	int64_t b = (int64_t)x1*dx + (int64_t)y1*dy;                           // b = v1(v2 - v1)
	int64_t c = (int64_t)x1*x1 + (int64_t)y1*y1 - (int64_t)radius*radius;  // c = v1² - r²
	// Equation to solve is now a t^2 + 2 b t + c = 0.
	int64_t d = b*b - a*c;                                                 // d = b² - a c
	// Solution is (-b ± √d)/a.
	INTERVAL empty = {-1, -1};
	INTERVAL full = {0, 1024};
	INTERVAL ret;
	if (d < 0)
	{
		return empty;  // Missed.
	}
	if (a == 0)
	{
		return c < 0 ? full : empty;  // Not moving. See if inside the target.
	}

	int32_t sd = i64Sqrt(d);
	ret.begin = MAX(   0, 1024*(-b - sd)/a);
	ret.end   = MIN(1024, 1024*(-b + sd)/a);
	return ret;
}

static int32_t collisionXYZ(Vector3i v1, Vector3i v2, ObjectShape shape, int32_t height)
{
	INTERVAL i = collisionZ(v1.z, v2.z, height);
	if (!intervalEmpty(i))  // Don't bother checking x and y unless z passes.
	{
		if (shape.isRectangular)
		{
			i = intervalIntersection(i, collisionZ(v1.x, v2.x, shape.size.x));
			if (!intervalEmpty(i))  // Don't bother checking y unless x and z pass.
			{
				i = intervalIntersection(i, collisionZ(v1.y, v2.y, shape.size.y));
			}
		}
		else  // Else is circular.
		{
			i = intervalIntersection(i, collisionXY(v1.x, v1.y, v2.x, v2.y, shape.radius()));
		}

		if (!intervalEmpty(i))
		{
			return MAX(0, i.begin);
		}
	}
	return -1;
}

static void proj_InFlightFunc(PROJECTILE *psProj, bool bIndirect)
{
	/* we want a delay between Las-Sats firing and actually hitting in multiPlayer
	magic number but that's how long the audio countdown message lasts! */
	const unsigned int LAS_SAT_DELAY = 4;
	int timeSoFar;
	int distancePercent; /* How far we are 0..100 */
	int distanceExtensionFactor; /* Extended lifespan */
	Vector3i move;
	// Projectile is missile:
	bool bMissile = false;
	WEAPON_STATS *psStats;
	Vector3i nextPos;
	int32_t targetDistance, currentDistance;
	BASE_OBJECT *psTempObj, *closestCollisionObject = NULL;
	Spacetime closestCollisionSpacetime;
	memset(&closestCollisionSpacetime, 0, sizeof(Spacetime));  // Squelch uninitialised warning.

	CHECK_PROJECTILE(psProj);

	timeSoFar = gameTime - psProj->born;

	psProj->time = gameTime;

	psStats = psProj->psWStats;
	ASSERT_OR_RETURN( , psStats != NULL, "Invalid weapon stats pointer");

	/* we want a delay between Las-Sats firing and actually hitting in multiPlayer
	magic number but that's how long the audio countdown message lasts! */
	if (bMultiPlayer && psStats->weaponSubClass == WSC_LAS_SAT &&
	    (unsigned)timeSoFar < LAS_SAT_DELAY * GAME_TICKS_PER_SEC)
	{
		return;
	}

	/* Calculate extended lifespan where appropriate */
	switch (psStats->weaponSubClass)
	{
		case WSC_MGUN:
		case WSC_COMMAND:
			distanceExtensionFactor = 120;
			break;
		case WSC_CANNON:
		case WSC_BOMB:
		case WSC_ELECTRONIC:
		case WSC_EMP:
		case WSC_FLAME:
		case WSC_ENERGY:
		case WSC_GAUSS:
			distanceExtensionFactor = 150;
			break;
		case WSC_AAGUN: // No extended distance
			distanceExtensionFactor = 100;
			break;
		case WSC_ROCKET:
		case WSC_MISSILE:
		case WSC_SLOWROCKET:
		case WSC_SLOWMISSILE:
			bMissile = true; // Take the same extended targetDistance as artillery
		case WSC_COUNTER:
		case WSC_MORTARS:
		case WSC_HOWITZERS:
		case WSC_LAS_SAT:
			distanceExtensionFactor = 150;
			break;
		default:
			// WSC_NUM_WEAPON_SUBCLASSES
			/* Uninitialized "marker", this can be used as a
			 * condition to assert on (i.e. it shouldn't occur).
			 */
			distanceExtensionFactor = 0;
			break;
	}

	/* Calculate movement vector: */
	if (psStats->movementModel == MM_HOMINGDIRECT && psProj->psDest)
	{
		/* If it's homing and it has a target (not a miss)... */
		/* Home at the center of the part that was visible when firing */
		move = psProj->psDest->pos - psProj->src + Vector3i(0, 0, establishTargetHeight(psProj->psDest) - psProj->partVisible/2);
	}
	else
	{
		move = psProj->dst - psProj->src;
		// LASSAT doesn't have a z
		if(psStats->weaponSubClass == WSC_LAS_SAT)
		{
			move.z = 0;
		}
		else if (bIndirect)
		{
			move.z = (psProj->vZ - (timeSoFar * ACC_GRAVITY / (GAME_TICKS_PER_SEC * 2))) * timeSoFar / GAME_TICKS_PER_SEC; // '2' because we reach our highest point in the mid of flight, when "vZ is 0".
		}
	}

	targetDistance = iHypot(move.x, move.y);
	if (!bIndirect)
	{
		currentDistance = timeSoFar * psStats->flightSpeed / GAME_TICKS_PER_SEC;
	}
	else
	{
		currentDistance = timeSoFar * psProj->vXY / GAME_TICKS_PER_SEC;
	}

	// Prevent div by zero:
	if (targetDistance == 0)
	{
		targetDistance = 1;
	}

	distancePercent = PERCENT(currentDistance, targetDistance);

	/* Calculate next position */
	nextPos.x = psProj->src.x + move.x * currentDistance/targetDistance;
	nextPos.y = psProj->src.y + move.y * currentDistance/targetDistance;
	if (!bIndirect)
	{
		nextPos.z = psProj->src.z + move.z * currentDistance/targetDistance;
	}
	else
	{
		nextPos.z = psProj->src.z + move.z;
	}

	/* impact if about to go off map else update coordinates */
	if (!worldOnMap(nextPos.x, nextPos.y))
	{
		psProj->state = PROJ_IMPACT;
		psProj->dst.x = psProj->pos.x;
		psProj->dst.y = psProj->pos.y;
		setProjectileDestination(psProj, NULL);
		debug(LOG_NEVER, "**** projectile(%i) off map - removed ****\n", psProj->id);
		return;
	}

	/* Update position */
	psProj->pos = nextPos;

	if (bIndirect)
	{
		/* Update pitch */
		psProj->rot.pitch = iAtan2(psProj->vZ - (timeSoFar * ACC_GRAVITY / GAME_TICKS_PER_SEC), psProj->vXY);
	}

	closestCollisionSpacetime.time = 0xFFFFFFFF;

	/* Check nearby objects for possible collisions */
	gridStartIterate(psProj->pos.x, psProj->pos.y, PROJ_NEIGHBOUR_RANGE);
	for (psTempObj = gridIterate(); psTempObj != NULL; psTempObj = gridIterate())
	{
		CHECK_OBJECT(psTempObj);

		if (std::find(psProj->psDamaged.begin(), psProj->psDamaged.end(), psTempObj) != psProj->psDamaged.end())
		{
			// Dont damage one target twice
			continue;
		}

		if (psTempObj->died)
		{
			// Do not damage dead objects further
			continue;
		}

		if (psTempObj->type == OBJ_PROJECTILE &&
			!(bMissile || ((PROJECTILE*)psTempObj)->psWStats->weaponSubClass == WSC_COUNTER))
		{
			// A projectile should not collide with another projectile unless it's a counter-missile weapon
			continue;
		}

		if (psTempObj->type == OBJ_FEATURE &&
			!((FEATURE*)psTempObj)->psStats->damageable)
		{
			// Ignore oil resources, artifacts and other pickups
			continue;
		}

		if (aiCheckAlliances(psTempObj->player, psProj->player)
			&& psTempObj != psProj->psDest)
		{
			// No friendly fire unless intentional
			continue;
		}
		
		if (!(psStats->surfaceToAir & SHOOT_ON_GROUND) &&
			(psTempObj->type == OBJ_STRUCTURE ||
				psTempObj->type == OBJ_FEATURE ||
				(psTempObj->type == OBJ_DROID && !isFlying((DROID *)psTempObj))
			))
		{
			// AA weapons should not hit buildings and non-vtol droids
			continue;
		}

		Vector3i psTempObjPrevPos = isDroid(psTempObj)? castDroid(psTempObj)->prevSpacetime.pos : psTempObj->pos;

		const Vector3i diff = psProj->pos - psTempObj->pos;
		const Vector3i prevDiff = psProj->prevSpacetime.pos - psTempObjPrevPos;
		const unsigned int targetHeight = establishTargetHeight(psTempObj);
		const ObjectShape targetShape = establishTargetShape(psTempObj);
		const int32_t collision = collisionXYZ(prevDiff, diff, targetShape, targetHeight);
		const uint32_t collisionTime = psProj->prevSpacetime.time + (psProj->time - psProj->prevSpacetime.time)*collision/1024;

		if (collision >= 0 && collisionTime < closestCollisionSpacetime.time)
		{
			// We hit!
			closestCollisionSpacetime = interpolateObjectSpacetime(psProj, collisionTime);
			closestCollisionObject = psTempObj;

			// Keep testing for more collisions, in case there was a closer target.
		}
	}

	if (closestCollisionObject != NULL)
	{
		// We hit!
		setSpacetime(psProj, closestCollisionSpacetime);
		if(psProj->time == psProj->prevSpacetime.time)
		{
			--psProj->prevSpacetime.time;
		}
		setProjectileDestination(psProj, closestCollisionObject);  // We hit something.

		/* Buildings cannot be penetrated and we need a penetrating weapon */
		if (closestCollisionObject->type == OBJ_DROID && psStats->penetrate)
		{
			WEAPON asWeap;
			// Determine position to fire a missile at
			Vector3i newDest = psProj->src + move * distanceExtensionFactor/100;
			asWeap.nStat = psStats - asWeaponStats;

			ASSERT(distanceExtensionFactor != 0, "Unitialized variable used! distanceExtensionFactor is not initialized.");

			// Assume we damaged the chosen target
			psProj->psDamaged.push_back(closestCollisionObject);

			proj_SendProjectile(&asWeap, psProj, psProj->player, newDest, NULL, true, -1);
		}

		psProj->state = PROJ_IMPACT;

		return;
	}

	ASSERT(distanceExtensionFactor != 0, "Unitialized variable used! distanceExtensionFactor is not initialized.");

	if (distancePercent >= distanceExtensionFactor || /* We've traveled our maximum range */
		!mapObjIsAboveGround(psProj)) /* trying to travel through terrain */
	{
		/* Miss due to range or height */
		psProj->state = PROJ_IMPACT;
		setProjectileDestination(psProj, NULL); /* miss registered if NULL target */
		return;
	}

	/* Paint effects if visible */
	if (gfxVisible(psProj))
	{
		uint32_t effectTime;
		// TODO Should probably give effectTime as an extra parameter to addEffect, or make an effectGiveAuxTime parameter, with yet another 'this is naughty' comment.
		for (effectTime = ((psProj->prevSpacetime.time + 31) & ~31); effectTime < psProj->time; effectTime += 32)
		{
			Spacetime st = interpolateObjectSpacetime(psProj, effectTime);
			Vector3i posFlip = swapYZ(st.pos);
			switch (psStats->weaponSubClass)
			{
				case WSC_FLAME:
					posFlip.z -= 8;  // Why?
					effectGiveAuxVar(distancePercent);
					addEffect(&posFlip, EFFECT_EXPLOSION, EXPLOSION_TYPE_FLAMETHROWER, false, NULL, 0);
				break;
				case WSC_COMMAND:
				case WSC_ELECTRONIC:
				case WSC_EMP:
					posFlip.z -= 8;  // Why?
					effectGiveAuxVar(distancePercent/2);
					addEffect(&posFlip, EFFECT_EXPLOSION, EXPLOSION_TYPE_LASER, false, NULL, 0);
				break;
				case WSC_ROCKET:
				case WSC_MISSILE:
				case WSC_SLOWROCKET:
				case WSC_SLOWMISSILE:
					posFlip.z += 8;  // Why?
					addEffect(&posFlip, EFFECT_SMOKE, SMOKE_TYPE_TRAIL, false, NULL, 0);
				break;
				default:
					// Add smoke trail to indirect weapons, even if firing directly.
					if (!proj_Direct(psStats))
					{
						posFlip.z += 4;  // Why?
						addEffect(&posFlip, EFFECT_SMOKE, SMOKE_TYPE_TRAIL, false, NULL, 0);
					}
					// Otherwise no effect.
					break;
			}
		}
	}
}

/***************************************************************************/

static void proj_ImpactFunc( PROJECTILE *psObj )
{
	WEAPON_STATS    *psStats;
	SDWORD          i, iAudioImpactID;
	int32_t         relativeDamage;
	Vector3i        position, scatter;
	iIMDShape       *imd;
	BASE_OBJECT     *temp;

	CHECK_PROJECTILE(psObj);

	psStats = psObj->psWStats;
	ASSERT(psStats != NULL, "proj_ImpactFunc: Invalid weapon stats pointer");

	// note the attacker if any
	g_pProjLastAttacker = psObj->psSource;

	/* play impact audio */
	if (gfxVisible(psObj))
	{
		if (psStats->iAudioImpactID == NO_SOUND)
		{
			/* play richochet if MG */
			if (psObj->psDest != NULL && psObj->psWStats->weaponSubClass == WSC_MGUN
			 && ONEINTHREE)
			{
				iAudioImpactID = ID_SOUND_RICOCHET_1 + (rand() % 3);
				audio_PlayStaticTrack(psObj->psDest->pos.x, psObj->psDest->pos.y, iAudioImpactID);
			}
		}
		else
		{
			audio_PlayStaticTrack(psObj->pos.x, psObj->pos.y, psStats->iAudioImpactID);
		}

		/* Shouldn't need to do this check but the stats aren't all at a value yet... */ // FIXME
		if (psStats->incenRadius && psStats->incenTime)
		{
			position.x = psObj->pos.x;
			position.z = psObj->pos.y; // z = y [sic] intentional
			position.y = map_Height(position.x, position.z);
			effectGiveAuxVar(psStats->incenRadius);
			effectGiveAuxVarSec(psStats->incenTime);
			addEffect(&position, EFFECT_FIRE, FIRE_TYPE_LOCALISED, false, NULL, 0);
		}

		// may want to add both a fire effect and the las sat effect
		if (psStats->weaponSubClass == WSC_LAS_SAT)
		{
			position.x = psObj->pos.x;
			position.z = psObj->pos.y;  // z = y [sic] intentional
			position.y = map_Height(position.x, position.z);
			addEffect(&position, EFFECT_SAT_LASER, SAT_LASER_STANDARD, false, NULL, 0);
			if (clipXY(psObj->pos.x, psObj->pos.y))
			{
				shakeStart();
			}
		}
	}

	if (psStats->incenRadius != 0 && psStats->incenTime != 0)
	{
		tileSetFire(psObj->pos.x, psObj->pos.y, psStats->incenTime);
	}

	// Set the effects position and radius
	position.x = psObj->pos.x;
	position.z = psObj->pos.y; // z = y [sic] intentional
	position.y = psObj->pos.z; // y = z [sic] intentional
	scatter.x = psStats->radius;
	scatter.y = 0;
	scatter.z = psStats->radius;

	// If the projectile missed its target (or the target died)
	if (psObj->psDest == NULL)
	{
		if (gfxVisible(psObj))
		{
			// Get if we are facing or not
			EFFECT_TYPE facing = (psStats->facePlayer ? EXPLOSION_TYPE_SPECIFIED : EXPLOSION_TYPE_NOT_FACING);

			// The graphic to show depends on if we hit water or not
			if (terrainType(mapTile(map_coord(psObj->pos.x), map_coord(psObj->pos.y))) == TER_WATER)
			{
				imd = psStats->pWaterHitGraphic;
			}
			// We did not hit water, the regular miss graphic will do the trick
			else
			{
				imd = psStats->pTargetMissGraphic;
			}

			addMultiEffect(&position, &scatter, EFFECT_EXPLOSION, facing, true, imd, psStats->numExplosions, psStats->lightWorld, psStats->effectSize);

			// If the target was a VTOL hit in the air add smoke
			if ((psStats->surfaceToAir & SHOOT_IN_AIR)
			 && !(psStats->surfaceToAir & SHOOT_ON_GROUND))
			{
				addMultiEffect(&position, &scatter, EFFECT_SMOKE, SMOKE_TYPE_DRIFTING, false, NULL, 3, 0, 0);
			}
		}
	}
	// The projectile hit its intended target
	else
	{
		CHECK_OBJECT(psObj->psDest);

		if (psObj->psDest->type == OBJ_FEATURE
		 && ((FEATURE *)psObj->psDest)->psStats->damageable == 0)
		{
			debug(LOG_NEVER, "proj_ImpactFunc: trying to damage non-damageable target,projectile removed");
			psObj->died = gameTime;
			return;
		}

		if (gfxVisible(psObj))
		{
			// Get if we are facing or not
			EFFECT_TYPE facing = (psStats->facePlayer ? EXPLOSION_TYPE_SPECIFIED : EXPLOSION_TYPE_NOT_FACING);

			// If we hit a VTOL with an AA gun use the miss graphic and add some smoke
			if ((psStats->surfaceToAir & SHOOT_IN_AIR)
			 && !(psStats->surfaceToAir & SHOOT_ON_GROUND)
			 && psStats->weaponSubClass == WSC_AAGUN)
			{
				imd = psStats->pTargetMissGraphic;
				addMultiEffect(&position, &scatter, EFFECT_SMOKE, SMOKE_TYPE_DRIFTING, false, NULL, 3, 0, 0);
			}
			// Otherwise we just hit it plain and simple
			else
			{
				imd = psStats->pTargetHitGraphic;
			}

			addMultiEffect(&position, &scatter, EFFECT_EXPLOSION, facing, true, imd, psStats->numExplosions, psStats->lightWorld, psStats->effectSize);
		}

		// Check for electronic warfare damage where we know the subclass and source
		if (proj_Direct(psStats)
		 && psStats->weaponSubClass == WSC_ELECTRONIC
		 && psObj->psSource)
		{
			// If we did enough `damage' to capture the target
			if (electronicDamage(psObj->psDest,
			                     calcDamage(weaponDamage(psStats, psObj->player), psStats->weaponEffect, psObj->psDest),
			                     psObj->player))
			{
				switch (psObj->psSource->type)
				{
					case OBJ_DROID:
						((DROID *) psObj->psSource)->order = DORDER_NONE;
						actionDroid((DROID *) (psObj->psSource), DACTION_NONE);
						break;

					case OBJ_STRUCTURE:
						((STRUCTURE *) psObj->psSource)->psTarget[0] = NULL;
						break;

					// This is only here to prevent the compiler from producing
					// warnings for unhandled enumeration values
					default:
						break;
				}
			}
		}
		// Else it is just a regular weapon (direct or indirect)
		else
		{
			// Calculate the damage the weapon does to its target
			unsigned int damage = calcDamage(weaponDamage(psStats, psObj->player), psStats->weaponEffect, psObj->psDest);

			// If we are in a multi-player game and the attacker is our responsibility
			if (bMultiPlayer && psObj->psSource)
			{
				updateMultiStatsDamage(psObj->psSource->player, psObj->psDest->player, damage);
			}

			debug(LOG_NEVER, "Damage to object %d, player %d\n",
			      psObj->psDest->id, psObj->psDest->player);

			// Damage the object
			relativeDamage = objectDamage(psObj->psDest, damage, psStats->weaponClass,psStats->weaponSubClass);

			proj_UpdateKills(psObj, relativeDamage);

			if (relativeDamage >= 0)	// So long as the target wasn't killed
			{
				psObj->psDamaged.push_back(psObj->psDest);
			}
		}
	}

	temp = psObj->psDest;
	setProjectileDestination(psObj, NULL);
	// The damage has been done, no more damage expected from this projectile. (Ignore burning.)
	psObj->expectedDamageCaused = 0;
	setProjectileDestination(psObj, temp);

	// If the projectile does no splash damage and does not set fire to things
	if ((psStats->radius == 0) && (psStats->incenTime == 0) )
	{
		psObj->died = gameTime;
		return;
	}

	if (psStats->radius != 0)
	{
		FEATURE *psCurrF, *psNextF;

		/* An area effect bullet */
		psObj->state = PROJ_POSTIMPACT;

		/* Note when it exploded for the explosion effect */
		psObj->born = gameTime;

		for (i = 0; i < MAX_PLAYERS; i++)
		{
			DROID *psCurrD, *psNextD;

			for (psCurrD = apsDroidLists[i]; psCurrD; psCurrD = psNextD)
			{
				/* have to store the next pointer as psCurrD could be destroyed */
				psNextD = psCurrD->psNext;

				/* see if psCurrD is hit (don't hit main target twice) */
				if ((BASE_OBJECT *)psCurrD != psObj->psDest)
				{
					bool bTargetInAir = (asPropulsionTypes[asPropulsionStats[psCurrD->asBits[COMP_PROPULSION].nStat].propulsionType].travel == AIR && ((DROID *)psCurrD)->sMove.Status != MOVEINACTIVE);

					// Check whether we can hit it and it is in hit radius
					if ((((psStats->surfaceToAir & SHOOT_IN_AIR) && bTargetInAir) || ((psStats->surfaceToAir & SHOOT_ON_GROUND) && !bTargetInAir))
					    && Vector3i_InSphere(psCurrD->pos, psObj->pos, psStats->radius))
					{
						unsigned dice = gameRand(100);
						if (dice < weaponRadiusHit(psStats, psObj->player))
						{
							unsigned int damage = calcDamage(
										weaponRadDamage(psStats, psObj->player),
										psStats->weaponEffect, (BASE_OBJECT *)psCurrD);

							debug(LOG_NEVER, "Damage to object %d, player %d\n",
									psCurrD->id, psCurrD->player);

							if (bMultiPlayer)
							{
								if (psObj->psSource)
								{
									updateMultiStatsDamage(psObj->psSource->player, psCurrD->player, damage);
								}
								turnOffMultiMsg(true);
							}

							relativeDamage = droidDamage(psCurrD, damage, psStats->weaponClass, psStats->weaponSubClass);

							turnOffMultiMsg(false);	// multiplay msgs back on.

							proj_UpdateKills(psObj, relativeDamage);
						}
					}
				}
			}

			// FIXME Check whether we hit above maximum structure height, to skip unnecessary calculations!
			if (psStats->surfaceToAir & SHOOT_ON_GROUND)
			{
				STRUCTURE *psCurrS, *psNextS;

				for (psCurrS = apsStructLists[i]; psCurrS; psCurrS = psNextS)
				{
					/* have to store the next pointer as psCurrD could be destroyed */
					psNextS = psCurrS->psNext;

					/* see if psCurrS is hit (don't hit main target twice) */
					if ((BASE_OBJECT *)psCurrS != psObj->psDest)
					{
						// Check whether it is in hit radius
						if (Vector3i_InCircle(psCurrS->pos, psObj->pos, psStats->radius))
						{
							unsigned dice = gameRand(100);
							if (dice < weaponRadiusHit(psStats, psObj->player))
							{
								unsigned int damage = calcDamage(weaponRadDamage(psStats, psObj->player), psStats->weaponEffect, (BASE_OBJECT *)psCurrS);

								if (bMultiPlayer)
								{
									if (psObj->psSource)
									{
										updateMultiStatsDamage(psObj->psSource->player,	psCurrS->player,damage);
									}
								}

								relativeDamage = structureDamage(psCurrS, damage, psStats->weaponClass, psStats->weaponSubClass);
								proj_UpdateKills(psObj, relativeDamage);
							}
						}
					}
				}
			}
		}

		for (psCurrF = apsFeatureLists[0]; psCurrF; psCurrF = psNextF)
		{
			/* have to store the next pointer as psCurrD could be destroyed */
			psNextF = psCurrF->psNext;

			//ignore features that are not damageable
			if(!psCurrF->psStats->damageable)
			{
				continue;
			}
			/* see if psCurrS is hit (don't hit main target twice) */
			if ((BASE_OBJECT *)psCurrF != psObj->psDest)
			{
				// Check whether it is in hit radius
				if (Vector3i_InCircle(psCurrF->pos, psObj->pos, psStats->radius))
				{
					unsigned dice = gameRand(100);
					if (dice < weaponRadiusHit(psStats, psObj->player))
					{
						debug(LOG_NEVER, "Damage to object %d, player %d\n",
								psCurrF->id, psCurrF->player);

						relativeDamage = featureDamage(psCurrF,
						                              calcDamage(weaponRadDamage(psStats, psObj->player),
						                                         psStats->weaponEffect,
						                                         (BASE_OBJECT *)psCurrF),
						                              psStats->weaponClass,
						                              psStats->weaponSubClass);
						proj_UpdateKills(psObj, relativeDamage);
					}
				}
			}
		}
	}

	if (psStats->incenTime != 0)
	{
		/* Incendiary round */
		/* Incendiary damage gets done in the bullet update routine */
		/* Just note when the incendiary started burning            */
		psObj->state = PROJ_POSTIMPACT;
		psObj->born = gameTime;
	}
	/* Something was blown up */
}

/***************************************************************************/

static void proj_PostImpactFunc( PROJECTILE *psObj )
{
	WEAPON_STATS	*psStats;
	SDWORD			i, age;

	CHECK_PROJECTILE(psObj);

	psStats = psObj->psWStats;
	ASSERT( psStats != NULL,
		"proj_PostImpactFunc: Invalid weapon stats pointer" );

	age = (SDWORD)gameTime - (SDWORD)psObj->born;

	/* Time to finish postimpact effect? */
	if (age > (SDWORD)psStats->radiusLife && age > (SDWORD)psStats->incenTime)
	{
		psObj->died = gameTime;
		return;
	}

	/* Burning effect */
	if (psStats->incenTime > 0)
	{
		/* See if anything is in the fire and burn it */
		for (i=0; i<MAX_PLAYERS; i++)
		{
			/* Don't damage your own droids - unrealistic, but better */
			if(i!=psObj->player)
			{
				proj_checkBurnDamage((BASE_OBJECT*)apsDroidLists[i], psObj);
				proj_checkBurnDamage((BASE_OBJECT*)apsStructLists[i], psObj);
			}
		}
	}
}

/***************************************************************************/

void PROJECTILE::update()
{
	PROJECTILE *psObj = this;

	CHECK_PROJECTILE(psObj);

	syncDebugProjectile(psObj, '<');

	psObj->prevSpacetime = getSpacetime(psObj);

	/* See if any of the stored objects have died
	 * since the projectile was created
	 */
	if (psObj->psSource && psObj->psSource->died)
	{
		setProjectileSource(psObj, NULL);
	}
	if (psObj->psDest && psObj->psDest->died)
	{
		setProjectileDestination(psObj, NULL);
	}
	// Remove dead objects from psDamaged.
	psDamaged.erase(std::remove_if(psDamaged.begin(), psDamaged.end(), std::ptr_fun(&::isDead)), psDamaged.end());

	// This extra check fixes a crash in cam2, mission1
	if (worldOnMap(psObj->pos.x, psObj->pos.y) == false)
	{
		psObj->died = true;
		return;
	}

	switch (psObj->state)
	{
		case PROJ_INFLIGHTDIRECT:
			proj_InFlightFunc(psObj, false);
			break;

		case PROJ_INFLIGHTINDIRECT:
			proj_InFlightFunc(psObj, true);
			break;

		case PROJ_IMPACT:
			proj_ImpactFunc( psObj );
			break;

		case PROJ_POSTIMPACT:
			proj_PostImpactFunc( psObj );
			break;
	}

	syncDebugProjectile(psObj, '>');
}

/***************************************************************************/

// iterate through all projectiles and update their status
void proj_UpdateAll()
{
	std::vector<PROJECTILE *> psProjectileListOld = psProjectileList;

	// Update all projectiles. Penetrating projectiles may add to psProjectileList.
	std::for_each(psProjectileListOld.begin(), psProjectileListOld.end(), std::mem_fun(&PROJECTILE::update));

	// Remove and free dead projectiles.
	psProjectileList.erase(std::remove_if(psProjectileList.begin(), psProjectileList.end(), std::mem_fun(&PROJECTILE::deleteIfDead)), psProjectileList.end());
}

/***************************************************************************/

static void proj_checkBurnDamage( BASE_OBJECT *apsList, PROJECTILE *psProj)
{
	BASE_OBJECT		*psCurr, *psNext;
	SDWORD			xDiff,yDiff;
	WEAPON_STATS	*psStats;
	UDWORD			radSquared;
	UDWORD			damageSoFar;
	SDWORD			damageToDo;

	CHECK_PROJECTILE(psProj);

	// note the attacker if any
	g_pProjLastAttacker = psProj->psSource;

	psStats = psProj->psWStats;
	radSquared = psStats->incenRadius * psStats->incenRadius;

	for (psCurr = apsList; psCurr; psCurr = psNext)
	{
		/* have to store the next pointer as psCurr could be destroyed */
		psNext = psCurr->psNext;

		if ((psCurr->type == OBJ_DROID) &&
			isVtolDroid((DROID*)psCurr) &&
			((DROID *)psCurr)->sMove.Status != MOVEINACTIVE)
		{
			// can't set flying vtols on fire
			continue;
		}

		/* Within the bounding box, now check the radius */
		xDiff = psCurr->pos.x - psProj->pos.x;
		yDiff = psCurr->pos.y - psProj->pos.y;
		if ((uint32_t)(xDiff*xDiff + yDiff*yDiff) <= radSquared)
		{
			/* The object is in the fire */
			psCurr->inFire |= IN_FIRE;

			if ( (psCurr->burnStart == 0) ||
				 (psCurr->inFire & BURNING) )
			{
				/* This is the first turn the object is in the fire */
				psCurr->burnStart = gameTime;
				psCurr->burnDamage = 0;
			}
			else
			{
				/* Calculate how much damage should have
				   been done up till now */
				damageSoFar = (gameTime - psCurr->burnStart)
							  //* psStats->incenDamage
							  * weaponIncenDamage(psStats,psProj->player)
							  / GAME_TICKS_PER_SEC;
				damageToDo = (SDWORD)damageSoFar
							 - (SDWORD)psCurr->burnDamage;
				if (damageToDo > 0)
				{
					int32_t relativeDamage;
					debug(LOG_NEVER, "Burn damage of %d to object %d, player %d\n",
							damageToDo, psCurr->id, psCurr->player);

					relativeDamage = objectDamage(psCurr, damageToDo, psStats->weaponClass,psStats->weaponSubClass);
					psCurr->burnDamage += damageToDo;
					proj_UpdateKills(psProj, relativeDamage);
				}
				/* The damage could be negative if the object
				   is being burn't by another fire
				   with a higher burn damage */
			}
		}
	}
}

/***************************************************************************/

// return whether a weapon is direct or indirect
bool proj_Direct(const WEAPON_STATS* psStats)
{
	ASSERT(psStats != NULL, "proj_Direct: called with NULL weapon!");
	if (!psStats)
	{
		return true; // arbitrary value in no-debug case
	}
	ASSERT(psStats->movementModel < NUM_MOVEMENT_MODEL, "proj_Direct: invalid weapon stat");

	switch (psStats->movementModel)
	{
	case MM_DIRECT:
	case MM_HOMINGDIRECT:
	case MM_ERRATICDIRECT:
	case MM_SWEEP:
		return true;
	case MM_INDIRECT:
	case MM_HOMINGINDIRECT:
		return false;
	case NUM_MOVEMENT_MODEL:
		break; // error checking in assert above; this is for no-debug case
	}

	return true; // just to satisfy compiler
}

/***************************************************************************/

// return the maximum range for a weapon
SDWORD proj_GetLongRange(const WEAPON_STATS* psStats)
{
	return psStats->longRange;
}


/***************************************************************************/
static ObjectShape establishTargetShape(BASE_OBJECT *psTarget)
{
	CHECK_OBJECT(psTarget);

	switch (psTarget->type)
	{
		case OBJ_DROID:  // Circular.
			switch (castDroid(psTarget)->droidType)
			{
				case DROID_WEAPON:
				case DROID_SENSOR:
				case DROID_ECM:
				case DROID_CONSTRUCT:
				case DROID_COMMAND:
				case DROID_REPAIR:
				case DROID_PERSON:
				case DROID_CYBORG:
				case DROID_CYBORG_CONSTRUCT:
				case DROID_CYBORG_REPAIR:
				case DROID_CYBORG_SUPER:
					//Watermelon:'hitbox' size is now based on imd size
					return abs(psTarget->sDisplay.imd->radius) * 2;
				case DROID_DEFAULT:
				case DROID_TRANSPORTER:
				default:
					return TILE_UNITS/4;  // how will we arrive at this?
			}
			break;
		case OBJ_STRUCTURE:  // Rectangular.
			return getStructureSize(castStructure(psTarget)) * TILE_UNITS/2;
		case OBJ_FEATURE:  // Rectangular.
			return Vector2i(castFeature(psTarget)->psStats->baseWidth, castFeature(psTarget)->psStats->baseBreadth) * TILE_UNITS/2;
		case OBJ_PROJECTILE:  // Circular, but can't happen since a PROJECTILE isn't a BASE_OBJECT.
			//Watermelon 1/2 radius of a droid?
			return TILE_UNITS/8;
		default:
			break;
	}

	return 0;  // Huh?
}
/***************************************************************************/

/*the damage depends on the weapon effect and the target propulsion type or
structure strength*/
UDWORD	calcDamage(UDWORD baseDamage, WEAPON_EFFECT weaponEffect, BASE_OBJECT *psTarget)
{
	UDWORD	damage;

	if (psTarget->type == OBJ_STRUCTURE)
	{
		damage = baseDamage * asStructStrengthModifier[weaponEffect][((STRUCTURE *)psTarget)->pStructureType->strength] / 100;
	}
	else if (psTarget->type == OBJ_DROID)
	{
		const int propulsion = (asPropulsionStats + ((DROID *)psTarget)->asBits[COMP_PROPULSION].nStat)->propulsionType;
		const int body = (asBodyStats + ((DROID *)psTarget)->asBits[COMP_BODY].nStat)->size;
		damage = baseDamage * (asWeaponModifier[weaponEffect][propulsion] + asWeaponModifierBody[weaponEffect][body]) / 100;
	}
	// Default value
	else
	{
		damage = baseDamage;
	}

	// A little fail safe!
	if (damage == 0 && baseDamage != 0)
	{
		damage = 1;
	}

	return damage;
}

/*
 * A quick explanation about hown this function works:
 *  - It returns an integer between 0 and 100 (see note for exceptions);
 *  - this represents the amount of damage inflicted on the droid by the weapon
 *    in relation to its original health.
 *  - e.g. If 100 points of (*actual*) damage were done to a unit who started
 *    off (when first produced) with 400 points then .25 would be returned.
 *  - If the actual damage done to a unit is greater than its remaining points
 *    then the actual damage is clipped: so if we did 200 actual points of
 *    damage to a cyborg with 150 points left the actual damage would be taken
 *    as 150.
 *  - Should sufficient damage be done to destroy/kill a unit then the value is
 *    multiplied by -1, resulting in a negative number. Killed features do not
 *    result in negative numbers.
 */
static int32_t objectDamage(BASE_OBJECT *psObj, UDWORD damage, WEAPON_CLASS weaponClass, WEAPON_SUBCLASS weaponSubClass)
{
	switch (psObj->type)
	{
		case OBJ_DROID:
			return droidDamage((DROID *)psObj, damage, weaponClass,weaponSubClass);
			break;

		case OBJ_STRUCTURE:
			return structureDamage((STRUCTURE *)psObj, damage, weaponClass, weaponSubClass);
			break;

		case OBJ_FEATURE:
			return featureDamage((FEATURE *)psObj, damage, weaponClass, weaponSubClass);
			break;

		case OBJ_PROJECTILE:
			ASSERT(!"invalid object type: bullet", "invalid object type: OBJ_PROJECTILE (id=%d)", psObj->id);
			break;

		case OBJ_TARGET:
			ASSERT(!"invalid object type: target", "invalid object type: OBJ_TARGET (id=%d)", psObj->id);
			break;

		default:
			ASSERT(!"unknown object type", "unknown object type %d, id=%d", psObj->type, psObj->id );
	}
	return 0;
}

/* Returns true if an object has just been hit by an electronic warfare weapon*/
static bool	justBeenHitByEW(BASE_OBJECT *psObj)
{
DROID		*psDroid;
FEATURE		*psFeature;
STRUCTURE	*psStructure;

	if(gamePaused())
	{
		return(false);	// Don't shake when paused...!
	}

	switch(psObj->type)
	{
		case OBJ_DROID:
			psDroid = (DROID*)psObj;
			if ((gameTime - psDroid->timeLastHit) < ELEC_DAMAGE_DURATION
			 && psDroid->lastHitWeapon == WSC_ELECTRONIC)
			{
				return(true);
			}
			break;

		case OBJ_FEATURE:
			psFeature = (FEATURE*)psObj;
			if ((gameTime - psFeature->timeLastHit) < ELEC_DAMAGE_DURATION)
			{
				return(true);
			}
			break;

		case OBJ_STRUCTURE:
			psStructure = (STRUCTURE*)psObj;
			if ((gameTime - psStructure->timeLastHit) < ELEC_DAMAGE_DURATION
			 && psStructure->lastHitWeapon == WSC_ELECTRONIC)
			{
				return true;
			}
			break;

		case OBJ_PROJECTILE:
			ASSERT(!"invalid object type: bullet", "justBeenHitByEW: invalid object type: OBJ_PROJECTILE");
			abort();
			break;

		case OBJ_TARGET:
			ASSERT(!"invalid object type: target", "justBeenHitByEW: invalid object type: OBJ_TARGET");
			abort();
			break;

		default:
			ASSERT(!"unknown object type", "justBeenHitByEW: unknown object type");
			abort();
	}

	return false;
}

void	objectShimmy(BASE_OBJECT *psObj)
{
	if(justBeenHitByEW(psObj))
	{
		pie_MatRotX(SKY_SHIMMY);
		pie_MatRotY(SKY_SHIMMY);
		pie_MatRotZ(SKY_SHIMMY);
		if(psObj->type == OBJ_DROID)
		{
			pie_TRANSLATE(1-rand()%3,0,1-rand()%3);
		}
	}
}


#define BULLET_FLIGHT_HEIGHT 16


int establishTargetHeight(BASE_OBJECT const *psTarget)
{
	if (psTarget == NULL)
	{
		return 0;
	}

	CHECK_OBJECT(psTarget);

	switch(psTarget->type)
	{
		case OBJ_DROID:
		{
			DROID const *psDroid = (DROID const *)psTarget;
			unsigned int height = asBodyStats[psDroid->asBits[COMP_BODY].nStat].pIMD->max.y - asBodyStats[psDroid->asBits[COMP_BODY].nStat].pIMD->min.y;
			unsigned int utilityHeight = 0, yMax = 0, yMin = 0; // Temporaries for addition of utility's height to total height

			// VTOL's don't have pIMD either it seems...
			if (isVtolDroid(psDroid))
			{
				return (height + VTOL_HITBOX_MODIFICATOR);
			}

			switch(psDroid->droidType)
			{
				case DROID_WEAPON:
					if ( psDroid->numWeaps > 0 )
					{
						// Don't do this for Barbarian Propulsions as they don't possess a turret (and thus have pIMD == NULL)
						if ((asWeaponStats[psDroid->asWeaps[0].nStat]).pIMD == NULL)
							return height;

						yMax = (asWeaponStats[psDroid->asWeaps[0].nStat]).pIMD->max.y;
						yMin = (asWeaponStats[psDroid->asWeaps[0].nStat]).pIMD->min.y;
					}
					break;

				case DROID_SENSOR:
					yMax = (asSensorStats[psDroid->asBits[COMP_SENSOR].nStat]).pIMD->max.y;
					yMin = (asSensorStats[psDroid->asBits[COMP_SENSOR].nStat]).pIMD->min.y;
					break;

				case DROID_ECM:
					yMax = (asECMStats[psDroid->asBits[COMP_ECM].nStat]).pIMD->max.y;
					yMin = (asECMStats[psDroid->asBits[COMP_ECM].nStat]).pIMD->min.y;
					break;

				case DROID_CONSTRUCT:
					yMax = (asConstructStats[psDroid->asBits[COMP_CONSTRUCT].nStat]).pIMD->max.y;
					yMin = (asConstructStats[psDroid->asBits[COMP_CONSTRUCT].nStat]).pIMD->min.y;
					break;

				case DROID_REPAIR:
					yMax = (asRepairStats[psDroid->asBits[COMP_REPAIRUNIT].nStat]).pIMD->max.y;
					yMin = (asRepairStats[psDroid->asBits[COMP_REPAIRUNIT].nStat]).pIMD->min.y;
					break;

				case DROID_PERSON:
					//TODO:add person 'state'checks here(stand, knee, crouch, prone etc)
				case DROID_CYBORG:
				case DROID_CYBORG_CONSTRUCT:
				case DROID_CYBORG_REPAIR:
				case DROID_CYBORG_SUPER:
				case DROID_DEFAULT:
				case DROID_TRANSPORTER:
				// Commanders don't have pIMD either
				case DROID_COMMAND:
				case DROID_ANY:
					return height;
			}

			// TODO: check the /2 - does this really make sense? why + ?
			utilityHeight = (yMax + yMin)/2;

			return height + utilityHeight;
		}
		case OBJ_STRUCTURE:
		{
			STRUCTURE_STATS * psStructureStats = ((STRUCTURE const *)psTarget)->pStructureType;
			return psStructureStats->pIMD[0]->max.y + psStructureStats->pIMD[0]->min.y;
		}
		case OBJ_FEATURE:
			// Just use imd ymax+ymin
			return psTarget->sDisplay.imd->max.y + psTarget->sDisplay.imd->min.y;
		case OBJ_PROJECTILE:
			return BULLET_FLIGHT_HEIGHT;
		default:
			return 0;
	}
}

void checkProjectile(const PROJECTILE* psProjectile, const char * const location_description, const char * function, const int recurse)
{
	if (recurse < 0)
		return;

	ASSERT_HELPER(psProjectile != NULL, location_description, function, "CHECK_PROJECTILE: NULL pointer");
	ASSERT_HELPER(psProjectile->psWStats != NULL, location_description, function, "CHECK_PROJECTILE");
	ASSERT_HELPER(psProjectile->type == OBJ_PROJECTILE, location_description, function, "CHECK_PROJECTILE");
	ASSERT_HELPER(psProjectile->player < MAX_PLAYERS, location_description, function, "CHECK_PROJECTILE: Out of bound owning player number (%u)", (unsigned int)psProjectile->player);
	ASSERT_HELPER(psProjectile->state == PROJ_INFLIGHTDIRECT
	    || psProjectile->state == PROJ_INFLIGHTINDIRECT
	    || psProjectile->state == PROJ_IMPACT
	    || psProjectile->state == PROJ_POSTIMPACT, location_description, function, "CHECK_PROJECTILE: invalid projectile state: %u", (unsigned int)psProjectile->state);

	if (psProjectile->psDest)
		checkObject(psProjectile->psDest, location_description, function, recurse - 1);

	if (psProjectile->psSource)
		checkObject(psProjectile->psSource, location_description, function, recurse - 1);

	for (unsigned n = 0; n != psProjectile->psDamaged.size(); ++n)
		checkObject(psProjectile->psDamaged[n], location_description, function, recurse - 1);
}
