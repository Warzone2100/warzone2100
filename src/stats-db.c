/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2008  Warzone Resurrection Project
	Copyright (C) 2008  Giel van Schijndel

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
 *  Store/Load common stats for weapons, components, brains, etc.
 */

#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "stats-db.h"
#include "stats.h"
#include "main.h"
#include "projectile.h"
#include "lib/sound/audio_id.h"
#include "lib/sqlite3/sqlite3.h"

/** Load the weapon stats from the given SQLite database file
 *  \param filename name of the database file to load the weapon stats from.
 */
bool loadWeaponStatsFromDB(const char* filename)
{
	sqlite3* db;
	sqlite3_stmt* stmt;

	int rc = sqlite3_open_v2(filename, &db, SQLITE_OPEN_READONLY, "physfs");
	if (rc != SQLITE_OK)
	{
		debug(LOG_ERROR, "loadWeaponStatsFromDB: Can't open database (%s): %s", filename, sqlite3_errmsg(db));
		goto in_db_err;
	}

	// Prepare this SQL statement for execution
	rc = sqlite3_prepare_v2(db, "SELECT MAX(id) FROM `weapons`;", -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		debug(LOG_ERROR, "loadWeaponStatsFromDB: SQL error: %s", sqlite3_errmsg(db));
		goto in_db_err;
	}

	/* Execute and process the results of the above SQL statement to
	 * determine the amount of weapons we're about to fetch. Then make sure
	 * to allocate memory for that amount of weapons. */
	rc = sqlite3_step(stmt);
	if (rc != SQLITE_ROW
	 || sqlite3_data_count(stmt) != 1
	 || !statsAllocWeapons(sqlite3_column_int(stmt, 0)))
	{
		goto in_statement_err;
	}

	sqlite3_finalize(stmt);
	rc = sqlite3_prepare_v2(db, "SELECT `id`,"
	                                   "`name`,"
	                                   "`techlevel`,"
	                                   "`buildPower`,"
	                                   "`buildPoints`,"
	                                   "`weight`,"
	                                   "`hitpoints`,"
	                                   "`systempoints`,"
	                                   "`body`,"
	                                   "`GfxFile`,"
	                                   "`mountGfx`,"
	                                   "`muzzleGfx`,"
	                                   "`flightGfx`,"
	                                   "`hitGfx`,"
	                                   "`missGfx`,"
	                                   "`waterGfx`,"
	                                   "`trailGfx`,"
	                                   "`short_range`,"
	                                   "`long_range`,"
	                                   "`short_range_accuracy`,"
	                                   "`long_range_accuracy`,"
	                                   "`firePause`,"
	                                   "`numExplosions`,"
	                                   "`rounds_per_salvo`,"
	                                   "`reload_time_per_salvo`,"
	                                   "`damage`,"
	                                   "`radius`,"
	                                   "`radiusHit`,"
	                                   "`radiusDamage`,"
	                                   "`incenTime`,"
	                                   "`incenDamage`,"
	                                   "`incenRadius`,"
	                                   "`directLife`,"
	                                   "`radiusLife`,"
	                                   "`flightSpeed`,"
	                                   "`indirectHeight`,"
	                                   "`fireOnMove`,"
	                                   "`weaponClass`,"
	                                   "`weaponSubClass`,"
	                                   "`movement`,"
	                                   "`weaponEffect`,"
	                                   "`rotate`,"
	                                   "`maxElevation`,"
	                                   "`minElevation`,"
	                                   "`facePlayer`,"
	                                   "`faceInFlight`,"
	                                   "`recoilValue`,"
	                                   "`minRange`,"
	                                   "`lightWorld`,"
	                                   "`effectSize`,"
	                                   "`surfaceToAir`,"
	                                   "`numAttackRuns`,"
	                                   "`designable`,"
	                                   "`penetrate` "
	                            "FROM `weapons`;", -1, &stmt, NULL);
	if (rc != SQLITE_OK)
	{
		debug(LOG_ERROR, "loadWeaponStatsFromDB: SQL error: %s", sqlite3_errmsg(db));
		goto in_db_err;
	}

	while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
	{
		WEAPON_STATS sStats, * const stats = &sStats;
		unsigned int colnum = 0;
		const unsigned int weapon_id = sqlite3_column_int(stmt, colnum++);
		const char* str;
		unsigned int longRange;

		memset(stats, 0, sizeof(*stats));

		stats->ref = REF_WEAPON_START + weapon_id - 1;

		// name                  TEXT    NOT NULL, -- Text id name (short language independant name)
		if (!allocateStatName((BASE_STATS *)stats, (const char*)sqlite3_column_text(stmt, colnum++)))
		{
			goto in_statement_err;
		}

		// techlevel             TEXT    NOT NULL, -- Technology level of this component
		if (!setTechLevel((BASE_STATS *)stats, (const char*)sqlite3_column_text(stmt, colnum++)))
		{
			goto in_statement_err;
		}

		// buildPower            NUMERIC NOT NULL, -- Power required to build this component
		stats->buildPower = sqlite3_column_double(stmt, colnum++);
		// buildPoints           NUMERIC NOT NULL, -- Time required to build this component
		stats->buildPoints = sqlite3_column_double(stmt, colnum++);
		// weight                NUMERIC NOT NULL, -- Component's weight (mass?)
		stats->weight = sqlite3_column_double(stmt, colnum++);
		// hitpoints             NUMERIC NOT NULL, -- Component's hitpoints - SEEMS TO BE UNUSED
		stats->hitPoints = sqlite3_column_double(stmt, colnum++);
		// systempoints          NUMERIC NOT NULL, -- Space the component takes in the droid - SEEMS TO BE UNUSED
		stats->systemPoints = sqlite3_column_double(stmt, colnum++);
		// body                  NUMERIC NOT NULL, -- Component's body points
		stats->body = sqlite3_column_double(stmt, colnum++);

		// Get the IMD for the component
		// GfxFile               TEXT,             -- The IMD to draw for this component
		if (sqlite3_column_type(stmt, colnum) != SQLITE_NULL)
		{
			stats->pIMD = (iIMDShape *) resGetData("IMD", (const char*)sqlite3_column_text(stmt, colnum++));
			if (stats->pIMD == NULL)
			{
				debug(LOG_ERROR, "Cannot find the weapon PIE for record %s", getStatName(stats));
				abort();
				goto in_statement_err;
			}
		}
		else
		{
			stats->pIMD = NULL;
			++colnum;
		}

		// Get the rest of the IMDs
		// mountGfx              TEXT,             -- The turret mount to use
		if (sqlite3_column_type(stmt, colnum) != SQLITE_NULL)
		{
			stats->pMountGraphic = (iIMDShape *) resGetData("IMD", (const char*)sqlite3_column_text(stmt, colnum++));
			if (stats->pMountGraphic == NULL)
			{
				debug(LOG_ERROR, "Cannot find the mount PIE for record %s", getStatName(stats));
				abort();
				goto in_statement_err;
			}
		}
		else
		{
			stats->pMountGraphic = NULL;
			++colnum;
		}

		if(GetGameMode() == GS_NORMAL)
		{
			// muzzleGfx             TEXT,             -- The muzzle flash
			stats->pMuzzleGraphic = (iIMDShape *) resGetData("IMD", (const char*)sqlite3_column_text(stmt, colnum++));
			if (stats->pMuzzleGraphic == NULL)
			{
				debug(LOG_ERROR, "Cannot find the muzzle PIE for record %s", getStatName(stats));
				abort();
				goto in_statement_err;
			}

			// flightGfx             TEXT,             -- The ammo in flight
			stats->pInFlightGraphic = (iIMDShape *) resGetData("IMD", (const char*)sqlite3_column_text(stmt, colnum++));
			if (stats->pInFlightGraphic == NULL)
			{
				debug(LOG_ERROR, "Cannot find the flight PIE for record %s", getStatName(stats));
				abort();
				goto in_statement_err;
			}

			// hitGfx                TEXT,             -- The ammo hitting a target
			stats->pTargetHitGraphic = (iIMDShape *) resGetData("IMD", (const char*)sqlite3_column_text(stmt, colnum++));
			if (stats->pTargetHitGraphic == NULL)
			{
				debug(LOG_ERROR, "Cannot find the target hit PIE for record %s", getStatName(stats));
				abort();
				goto in_statement_err;
			}

			// missGfx               TEXT,             -- The ammo missing a target
			stats->pTargetMissGraphic = (iIMDShape *) resGetData("IMD", (const char*)sqlite3_column_text(stmt, colnum++));
			if (stats->pTargetMissGraphic == NULL)
			{
				debug(LOG_ERROR, "Cannot find the target miss PIE for record %s", getStatName(stats));
				abort();
				goto in_statement_err;
			}

			// waterGfx              TEXT,             -- The ammo hitting water
			stats->pWaterHitGraphic = (iIMDShape *) resGetData("IMD", (const char*)sqlite3_column_text(stmt, colnum++));
			if (stats->pWaterHitGraphic == NULL)
			{
				debug(LOG_ERROR, "Cannot find the water hit PIE for record %s", getStatName(stats));
				abort();
				goto in_statement_err;
			}

			// trailGfx              TEXT,             -- The trail used for in flight
			// Trail graphic can be null
			if (sqlite3_column_type(stmt, colnum) != SQLITE_NULL)
			{
				stats->pTrailGraphic = (iIMDShape *) resGetData("IMD", (const char*)sqlite3_column_text(stmt, colnum++));
				if (stats->pTrailGraphic == NULL)
				{
					debug( LOG_ERROR, "Cannot find the trail PIE for record %s", getStatName(stats) );
					abort();
					goto in_statement_err;
				}
			}
			else
			{
				stats->pTrailGraphic = NULL;
				++colnum;
			}
		}
		else
		{
			colnum += 6;
		}

		// short_range           NUMERIC NOT NULL, -- Max distance to target for short range shot
		stats->shortRange = sqlite3_column_double(stmt, colnum++);
		// long_range            NUMERIC NOT NULL, -- Max distance to target for long range shot
		stats->longRange = sqlite3_column_double(stmt, colnum++);
		// short_range_accuracy  NUMERIC NOT NULL, -- Chance to hit at short range
		stats->shortHit = sqlite3_column_double(stmt, colnum++);
		// long_range_accuracy   NUMERIC NOT NULL, -- Chance to hit at long range
		stats->longHit = sqlite3_column_double(stmt, colnum++);
		// firePause             NUMERIC NOT NULL, -- Time between each weapon fire
		stats->firePause = sqlite3_column_double(stmt, colnum++);

		// numExplosions         INTEGER NOT NULL, -- The number of explosions per shot
		stats->numExplosions = sqlite3_column_int(stmt, colnum++);
		// rounds_per_salvo      INTEGER NOT NULL, -- The number of rounds per salvo(magazine)
		stats->numRounds = sqlite3_column_int(stmt, colnum++);

		// reload_time_per_salvo NUMERIC NOT NULL, -- Time to reload the round of ammo (salvo fire)
		stats->reloadTime = sqlite3_column_double(stmt, colnum++);
		// damage                NUMERIC NOT NULL, -- How much damage the weapon causes
		stats->damage = sqlite3_column_double(stmt, colnum++);
		// radius                NUMERIC NOT NULL, -- Basic blast radius of weapon
		stats->radius = sqlite3_column_double(stmt, colnum++);
		// radiusHit             NUMERIC NOT NULL, -- Chance to hit in the blast radius
		stats->radiusHit = sqlite3_column_double(stmt, colnum++);
		// radiusDamage          NUMERIC NOT NULL, -- Damage done in the blast radius
		stats->radiusDamage = sqlite3_column_double(stmt, colnum++);
		// incenTime             NUMERIC NOT NULL, -- How long the round burns
		stats->incenTime = sqlite3_column_double(stmt, colnum++);
		// incenDamage           NUMERIC NOT NULL, -- Damage done each burn cycle
		stats->incenDamage = sqlite3_column_double(stmt, colnum++);
		// incenRadius           NUMERIC NOT NULL, -- Burn radius of the round
		stats->incenRadius = sqlite3_column_double(stmt, colnum++);
		// directLife            NUMERIC NOT NULL, -- How long a direct fire weapon is visible. Measured in 1/100 sec.
		stats->directLife = sqlite3_column_double(stmt, colnum++);
		// radiusLife            NUMERIC NOT NULL, -- How long a blast radius is visible
		stats->radiusLife = sqlite3_column_double(stmt, colnum++);

		// flightSpeed           NUMERIC NOT NULL, -- speed ammo travels at
		stats->flightSpeed = sqlite3_column_double(stmt, colnum++);
//#ifdef DEBUG
// Hack to get the current stats working... a zero flight speed value will cause an assert in projectile.c line 957
//  I'm not sure if this should be on debug only...
//    ... the last thing we want is for a zero value to get through on release (with no asserts!)
//
// Anyway if anyone has a problem with this, take it up with Tim ... we have a frank and open discussion about it.
#define DEFAULT_FLIGHTSPEED (500)
		if (stats->flightSpeed == 0)
		{
			debug(LOG_NEVER, "STATS: Zero Flightspeed for %s - using default of %d\n", getStatName(stats), DEFAULT_FLIGHTSPEED);
			stats->flightSpeed = DEFAULT_FLIGHTSPEED;
		}

		// indirectHeight        NUMERIC NOT NULL, -- how high the ammo travels for indirect fire
		stats->indirectHeight = sqlite3_column_double(stmt, colnum++);

		// fireOnMove            TEXT    NOT NULL, -- indicates whether the droid has to stop before firing
		str = (const char*)sqlite3_column_text(stmt, colnum++);
		if (!strcmp(str, "NO"))
		{
			stats->fireOnMove = FOM_NO;
		}
		else if (!strcmp(str,"PARTIAL"))
		{
			stats->fireOnMove = FOM_PARTIAL;
		}
		else if (!strcmp(str,"YES"))
		{
			stats->fireOnMove = FOM_YES;
		}
		else
		{
			debug(LOG_ERROR, "Invalid fire on move flag for weapon %s", getStatName(stats));
			abort();
			goto in_statement_err;
		}

		// weaponClass           TEXT    NOT NULL, -- the class of weapon
		str = (const char*)sqlite3_column_text(stmt, colnum++);
		if (!strcmp(str, "KINETIC"))
		{
			stats->weaponClass = WC_KINETIC;
		}
		else if (!strcmp(str,"EXPLOSIVE"))
		{
			//stats->weaponClass = WC_EXPLOSIVE;
			stats->weaponClass = WC_KINETIC; 	// explosives were removed from release version of Warzone
		}
		else if (!strcmp(str,"HEAT"))
		{
			stats->weaponClass = WC_HEAT;
		}
		else if (!strcmp(str,"MISC"))
		{
			//stats->weaponClass = WC_MISC;
			stats->weaponClass = WC_HEAT;		// removed from release version of Warzone
		}
		else
		{
			debug(LOG_ERROR, "Invalid weapon class for weapon %s", getStatName(stats));
			abort();
			goto in_statement_err;
		}

		// weaponSubClass        TEXT    NOT NULL, -- the subclass to which the weapon belongs
		stats->weaponSubClass = getWeaponSubClass((const char*)sqlite3_column_text(stmt, colnum++));
		if (stats->weaponSubClass == INVALID_SUBCLASS)
		{
			goto in_statement_err;
		}

		// movement              TEXT    NOT NULL, -- which projectile model to use for the bullet
		stats->movementModel = getMovementModel((const char*)sqlite3_column_text(stmt, colnum++));
		if (stats->movementModel == INVALID_MOVEMENT)
		{
			goto in_statement_err;
		}

		// weaponEffect          TEXT    NOT NULL, -- which type of warhead is associated with the weapon
		stats->weaponEffect = getWeaponEffect((const char*)sqlite3_column_text(stmt, colnum++));
		if (stats->weaponEffect == INVALID_WEAPON_EFFECT)
		{
			debug(LOG_ERROR, "loadWepaonStats: Invalid weapon effect for weapon %s", getStatName(stats));
			abort();
			goto in_statement_err;
		}

		// rotate                NUMERIC NOT NULL, -- amount the weapon(turret) can rotate 0 = none
		if (sqlite3_column_int(stmt, colnum) > UBYTE_MAX)
		{
			ASSERT(FALSE, "loadWeaponStats: rotate is greater than %u for weapon %s", (unsigned int)UBYTE_MAX, getStatName(stats));
			goto in_statement_err;
		}
		stats->rotate = sqlite3_column_double(stmt, colnum++);

		// maxElevation          NUMERIC NOT NULL, -- max amount the turret can be elevated up
		if (sqlite3_column_int(stmt, colnum) > UBYTE_MAX)
		{
			ASSERT(FALSE, "loadWeaponStats: maxElevation is greater than %u for weapon %s", (unsigned int)UBYTE_MAX, getStatName(stats));
			goto in_statement_err;
		}
		stats->maxElevation = sqlite3_column_double(stmt, colnum++);

		// minElevation          NUMERIC NOT NULL, -- min amount the turret can be elevated down
		if (sqlite3_column_int(stmt, colnum) > SBYTE_MAX
		 || sqlite3_column_int(stmt, colnum) < SBYTE_MIN)
		{
			ASSERT(FALSE, "loadWeaponStats: minElevation is outside of limits for weapon %s", getStatName(stats));
			return false;
		}
		stats->minElevation = sqlite3_column_double(stmt, colnum++);

		// facePlayer            TEXT    NOT NULL, -- flag to make the (explosion) effect face the player when drawn
		stats->facePlayer = compareYes((const char*)sqlite3_column_text(stmt, colnum++), getStatName(stats)) ? TRUE : FALSE;
		// faceInFlight          TEXT    NOT NULL, -- flag to make the inflight effect face the player when drawn
		stats->faceInFlight = compareYes((const char*)sqlite3_column_text(stmt, colnum++), getStatName(stats)) ? TRUE : FALSE;

		// recoilValue           NUMERIC NOT NULL, -- used to compare with weight to see if recoils or not
		stats->recoilValue = sqlite3_column_double(stmt, colnum++);
		// minRange              NUMERIC NOT NULL, -- Min distance to target for shot
		stats->minRange = sqlite3_column_double(stmt, colnum++);

		// lightWorld            TEXT    NOT NULL, -- flag to indicate whether the effect lights up the world
		stats->lightWorld = compareYes((const char*)sqlite3_column_text(stmt, colnum++), getStatName(stats)) ? TRUE : FALSE;

		// effectSize            NUMERIC NOT NULL, -- size of the effect 100 = normal, 50 = half etc
		if (sqlite3_column_int(stmt, colnum) > UBYTE_MAX)
		{
			ASSERT(FALSE, "loadWeaponStats: effectSize is greater than %u for weapon %s", (unsigned int)UBYTE_MAX, getStatName(stats));
			goto in_statement_err;
		}
		stats->rotate = sqlite3_column_double(stmt, colnum++);

		// surfaceToAir          NUMERIC NOT NULL, -- indicates how good in the air - SHOOT_ON_GROUND, SHOOT_IN_AIR or both
		if (sqlite3_column_int(stmt, colnum) > UBYTE_MAX)
		{
			ASSERT(FALSE, "loadWeaponStats: surfaceToAir is greater than %u for weapon %s", (unsigned int)UBYTE_MAX, getStatName(stats));
			goto in_statement_err;
		}
		stats->surfaceToAir = sqlite3_column_double(stmt, colnum++);
		if (stats->surfaceToAir == 0)
		{
			stats->surfaceToAir = (UBYTE)SHOOT_ON_GROUND;
		}
		else if (stats->surfaceToAir <= 50)
		{
			stats->surfaceToAir = (UBYTE)SHOOT_IN_AIR;
		}
		else
		{
			stats->surfaceToAir = (UBYTE)(SHOOT_ON_GROUND | SHOOT_IN_AIR);
		}

		// numAttackRuns         NUMERIC NOT NULL, -- number of attack runs a VTOL droid can do with this weapon
		if (sqlite3_column_int(stmt, colnum) > UBYTE_MAX)
		{
			ASSERT(FALSE, "loadWeaponStats: numAttackRuns is greater than %u for weapon %s", (unsigned int)UBYTE_MAX, getStatName(stats));
			goto in_statement_err;
		}
		stats->vtolAttackRuns = sqlite3_column_double(stmt, colnum++);

		// designable            INTEGER NOT NULL, -- flag to indicate whether this component can be used in the design screen
		stats->design = sqlite3_column_int(stmt, colnum++) ? TRUE : FALSE;

		// penetrate             NUMERIC NOT NULL  -- flag to indicate whether pentrate droid or not
		stats->penetrate = sqlite3_column_int(stmt, colnum++) ? TRUE : FALSE;

		// error check the ranges
		if (stats->flightSpeed > 0
		 && !proj_Direct(stats))
		{
			longRange = proj_GetLongRange(stats);
		}
		else
		{
			longRange = UINT_MAX;
		}

		if (stats->shortRange > longRange)
		{
			debug(LOG_NEVER, "%s, flight speed is too low to reach short range (max range %d)\n", getStatName(stats), longRange);
		}
		else if (stats->longRange > longRange)
		{
			debug(LOG_NEVER, "%s, flight speed is too low to reach long range (max range %d)\n", getStatName(stats), longRange);
		}

		// Set the max stat values for the design screen
		if (stats->design)
		{
			setMaxWeaponRange(stats->longRange);
			setMaxWeaponDamage(stats->damage);
			setMaxWeaponROF(weaponROF(stats, -1));
			setMaxComponentWeight(stats->weight);
		}

		//multiply time stats
		stats->firePause *= WEAPON_TIME;
		stats->incenTime *= WEAPON_TIME;
		stats->directLife *= WEAPON_TIME;
		stats->radiusLife *= WEAPON_TIME;
		stats->reloadTime *= WEAPON_TIME;

		//set the weapon sounds to default value
		stats->iAudioFireID = NO_SOUND;
		stats->iAudioImpactID = NO_SOUND;

		//save the stats
		statsSetWeapon(stats, weapon_id - 1);
	}

	sqlite3_finalize(stmt);
	sqlite3_close(db);
	return true;

in_statement_err:
	sqlite3_finalize(stmt);
in_db_err:
	sqlite3_close(db);
	return false;
}
