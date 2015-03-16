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

#include "lib/framework/frame.h"
#include "lib/netplay/netplay.h"
#include "lib/sound/audio.h"

#include "baseobject.h"
#include "droid.h"
#include "projectile.h"
#include "structure.h"
#include "feature.h"
#include "intdisplay.h"
#include "map.h"


static inline uint16_t interpolateAngle(uint16_t v1, uint16_t v2, uint32_t t1, uint32_t t2, uint32_t t)
{
	const int numer = t - t1, denom = t2 - t1;
	return v1 + angleDelta(v2 - v1) * numer / denom;
}

static Position interpolatePos(Position p1, Position p2, uint32_t t1, uint32_t t2, uint32_t t)
{
	const int numer = t - t1, denom = t2 - t1;
	return p1 + (p2 - p1) * numer / denom;
}

Rotation interpolateRot(Rotation v1, Rotation v2, uint32_t t1, uint32_t t2, uint32_t t)
{
	//return v1 + (v2 - v1) * (t - t1) / (t2 - t1);
	return Rotation(interpolateAngle(v1.direction, v2.direction, t1, t2, t),
	                interpolateAngle(v1.pitch,     v2.pitch,     t1, t2, t),
	                interpolateAngle(v1.roll,      v2.roll,      t1, t2, t)
	               );
}

static Spacetime interpolateSpacetime(Spacetime st1, Spacetime st2, uint32_t t)
{
	// Cyp says this should never happen, #3037 and #3238 say it does though.
	ASSERT_OR_RETURN(st1, st1.time != st2.time, "Spacetime overlap!");
	return Spacetime(interpolatePos(st1.pos, st2.pos, st1.time, st2.time, t), interpolateRot(st1.rot, st2.rot, st1.time, st2.time, t), t);
}

Spacetime interpolateObjectSpacetime(const SIMPLE_OBJECT *obj, uint32_t t)
{
	switch (obj->type)
	{
	default:
		return getSpacetime(obj);
	case OBJ_DROID:
		return interpolateSpacetime(castDroid(obj)->prevSpacetime, getSpacetime(obj), t);
	case OBJ_PROJECTILE:
		return interpolateSpacetime(castProjectile(obj)->prevSpacetime, getSpacetime(obj), t);
	}
}

SIMPLE_OBJECT::SIMPLE_OBJECT(OBJECT_TYPE type, uint32_t id, unsigned player)
	: type(type)
	, id(id)
	, pos(0, 0, 0)
	, rot(0, 0, 0)
	, player(player)
	, born(gameTime)
	, died(0)
	, time(0)
{}

SIMPLE_OBJECT::~SIMPLE_OBJECT()
{
#ifdef DEBUG
	const_cast<OBJECT_TYPE &>(type) = (OBJECT_TYPE)(type + 1000000000);  // Hopefully this will trigger an assert              if someone uses the freed object.
	player += 100;                                                       // Hopefully this will trigger an assert and/or crash if someone uses the freed object.
#endif //DEBUG
}

BASE_OBJECT::BASE_OBJECT(OBJECT_TYPE type, uint32_t id, unsigned player)
	: SIMPLE_OBJECT(type, id, player)
	, selected(false)
	, cluster(0)
	, numWatchedTiles(0)
	, lastEmission(0)
	, lastHitWeapon(WSC_NUM_WEAPON_SUBCLASSES)  // No such weapon.
	, timeLastHit(UDWORD_MAX)
	, body(0)
	, periodicalDamageStart(0)
	, periodicalDamage(0)
	, flags(0)
	, watchedTiles(NULL)
	, psCurAnim(NULL)
{
	memset(visible, 0, sizeof(visible));
	sDisplay.imd = NULL;
	sDisplay.frameNumber = 0;
	sDisplay.screenX = 0;
	sDisplay.screenY = 0;
	sDisplay.screenR = 0;
}

BASE_OBJECT::~BASE_OBJECT()
{
	// Make sure to get rid of some final references in the sound code to this object first
	audio_RemoveObj(this);

	visRemoveVisibility(this);
	free(watchedTiles);

#ifdef DEBUG
	psNext = this;                                                       // Hopefully this will trigger an infinite loop       if someone uses the freed object.
	psNextFunc = this;                                                   // Hopefully this will trigger an infinite loop       if someone uses the freed object.
#endif //DEBUG
}

void checkObject(const SIMPLE_OBJECT *psObject, const char *const location_description, const char *function, const int recurse)
{
	if (recurse < 0)
	{
		return;
	}

	ASSERT(psObject != NULL, "NULL pointer");

	switch (psObject->type)
	{
	case OBJ_DROID:
		checkDroid((const DROID *)psObject, location_description, function, recurse - 1);
		break;

	case OBJ_STRUCTURE:
		checkStructure((const STRUCTURE *)psObject, location_description, function, recurse - 1);
		break;

	case OBJ_PROJECTILE:
		checkProjectile((const PROJECTILE *)psObject, location_description, function, recurse - 1);
		break;

	case OBJ_FEATURE:
	case OBJ_TARGET:
		break;

	default:
		ASSERT_HELPER(!"invalid object type", location_description, function, "CHECK_OBJECT: Invalid object type (type num %u)", (unsigned int)psObject->type);
		break;
	}
}

void _syncDebugObject(const char *function, SIMPLE_OBJECT const *psObject, char ch)
{
	switch (psObject->type)
	{
	case OBJ_DROID:      _syncDebugDroid(function, (const DROID *)     psObject, ch); break;
	case OBJ_STRUCTURE:  _syncDebugStructure(function, (const STRUCTURE *) psObject, ch); break;
	case OBJ_FEATURE:    _syncDebugFeature(function, (const FEATURE *)   psObject, ch); break;
	case OBJ_PROJECTILE: _syncDebugProjectile(function, (const PROJECTILE *)psObject, ch); break;
	default:             _syncDebug(function, "%c unidentified_object%d = p%d;objectType%d", ch, psObject->id, psObject->player, psObject->type);
		ASSERT_HELPER(!"invalid object type", "_syncDebugObject", function, "syncDebug: Invalid object type (type num %u)", (unsigned int)psObject->type);
		break;
	}
}

Vector2i getStatsSize(BASE_STATS const *pType, uint16_t direction)
{
	if (StatIsStructure(pType))
	{
		return getStructureStatsSize(static_cast<STRUCTURE_STATS const *>(pType), direction);
	}
	else if (StatIsFeature(pType))
	{
		return getFeatureStatsSize(static_cast<FEATURE_STATS const *>(pType));
	}
	return Vector2i(1, 1);
}

StructureBounds getStructureBounds(BASE_OBJECT const *object)
{
	STRUCTURE const *psStructure = castStructure(object);
	FEATURE const *psFeature = castFeature(object);

	if (psStructure != NULL)
	{
		return getStructureBounds(psStructure);
	}
	else if (psFeature != NULL)
	{
		return getStructureBounds(psFeature);
	}

	return StructureBounds(Vector2i(32767, 32767), Vector2i(-65535, -65535));  // Default to an invalid area.
}

StructureBounds getStructureBounds(BASE_STATS const *stats, Vector2i pos, uint16_t direction)
{
	if (StatIsStructure(stats))
	{
		return getStructureBounds(static_cast<STRUCTURE_STATS const *>(stats), pos, direction);
	}
	else if (StatIsFeature(stats))
	{
		return getStructureBounds(static_cast<FEATURE_STATS const *>(stats), pos);
	}

	return StructureBounds(map_coord(pos), Vector2i(1, 1));  // Default to a 1×1 tile.
}
