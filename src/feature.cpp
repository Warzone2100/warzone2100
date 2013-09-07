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
/*
 * Feature.c
 *
 * Load feature stats
 */
#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/strres.h"
#include "lib/framework/wzconfig.h"

#include "lib/gamelib/gtime.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "lib/netplay/netplay.h"

#include "feature.h"
#include "map.h"
#include "hci.h"
#include "power.h"
#include "objects.h"
#include "display.h"
#include "console.h"
#include "order.h"
#include "structure.h"
#include "miscimd.h"
#include "anim_id.h"
#include "visibility.h"
#include "text.h"
#include "effects.h"
#include "geometry.h"
#include "scores.h"
#include "combat.h"
#include "multiplay.h"
#include "advvis.h"

#include "mapgrid.h"
#include "display3d.h"
#include "random.h"

/* The statistics for the features */
FEATURE_STATS	*asFeatureStats;
UDWORD			numFeatureStats;

//Value is stored for easy access to this feature in destroyDroid()/destroyStruct()
FEATURE_STATS* oilResFeature = NULL;

void featureInitVars(void)
{
	asFeatureStats = NULL;
	numFeatureStats = 0;
	oilResFeature = NULL;
}

/* Load the feature stats */
bool loadFeatureStats(const char *pFileName)
{
	WzConfig ini(pFileName, WzConfig::ReadOnlyAndRequired);
	QStringList list = ini.childGroups();
	asFeatureStats = new FEATURE_STATS[list.size()];
	numFeatureStats = list.size();
	for (int i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		asFeatureStats[i] = FEATURE_STATS(REF_FEATURE_START + i);
		FEATURE_STATS *p = &asFeatureStats[i];
		p->name = ini.value("name").toString();
		p->id = list[i];
		QString subType = ini.value("type").toString();
		if (subType == "TANK WRECK") p->subType = FEAT_TANK;
		else if (subType == "GENERIC ARTEFACT") p->subType = FEAT_GEN_ARTE;
		else if (subType == "OIL RESOURCE") p->subType = FEAT_OIL_RESOURCE;
		else if (subType == "BOULDER") p->subType = FEAT_BOULDER;
		else if (subType == "VEHICLE") p->subType = FEAT_VEHICLE;
		else if (subType == "BUILDING") p->subType = FEAT_BUILDING;
		else if (subType == "OIL DRUM") p->subType = FEAT_OIL_DRUM;
		else if (subType == "TREE") p->subType = FEAT_TREE;
		else if (subType == "SKYSCRAPER") p->subType = FEAT_SKYSCRAPER;
		else ASSERT(false, "Unknown feature type: %s", subType.toUtf8().constData());
		p->psImd = modelGet(ini.value("model").toString());
		p->baseWidth = ini.value("width", 1).toInt();
		p->baseBreadth = ini.value("breadth", 1).toInt();
		p->tileDraw = ini.value("tiledraw", 1).toInt();
		p->allowLOS = ini.value("lineOfSight", 1).toInt();
		p->visibleAtStart = ini.value("startVisible", 1).toInt();
		p->damageable = ini.value("damageable", 1).toInt();
		p->body = ini.value("hitpoints", 1).toInt();
		p->armourValue = ini.value("armour", 1).toInt();

		//and the oil resource - assumes only one!
		if (asFeatureStats[i].subType == FEAT_OIL_RESOURCE)
		{
			oilResFeature = &asFeatureStats[i];
		}
		ini.endGroup();
	}

	return true;
}

/* Release the feature stats memory */
void featureStatsShutDown(void)
{
	delete[] asFeatureStats;
	asFeatureStats = NULL;
	numFeatureStats = 0;
}

/** Deals with damage to a feature
 *  \param psFeature feature to deal damage to
 *  \param damage amount of damage to deal
 *  \param weaponClass,weaponSubClass the class and subclass of the weapon that deals the damage
 *  \return < 0 never, >= 0 always
 */
int32_t featureDamage(FEATURE *psFeature, unsigned damage, WEAPON_CLASS weaponClass, WEAPON_SUBCLASS weaponSubClass, unsigned impactTime, bool isDamagePerSecond, int minDamage)
{
	int32_t relativeDamage;

	ASSERT_OR_RETURN(0, psFeature != NULL, "Invalid feature pointer");

	debug(LOG_ATTACK, "feature (id %d): body %d armour %d damage: %d",
	      psFeature->id, psFeature->body, psFeature->psStats->armourValue, damage);

	relativeDamage = objDamage(psFeature, damage, psFeature->psStats->body, weaponClass, weaponSubClass, isDamagePerSecond, minDamage);

	// If the shell did sufficient damage to destroy the feature
	if (relativeDamage < 0)
	{
		debug(LOG_ATTACK, "feature (id %d) DESTROYED", psFeature->id);
		destroyFeature(psFeature, impactTime);
		return relativeDamage * -1;
	}
	else
	{
		return relativeDamage;
	}
}


/* Create a feature on the map */
FEATURE * buildFeature(FEATURE_STATS *psStats, UDWORD x, UDWORD y,bool FromSave)
{
	//try and create the Feature
	FEATURE *psFeature = new FEATURE(generateSynchronisedObjectId(), psStats);

	if (psFeature == NULL)
	{
		debug(LOG_WARNING, "Feature couldn't be built.");
		return NULL;
	}
	// features are not in the cluster system
	// this will cause an assert when they still end up there
	psFeature->cluster = ~0;
	//add the feature to the list - this enables it to be drawn whilst being built
	addFeature(psFeature);

	// snap the coords to a tile
	if (!FromSave)
	{
		x = (x & ~TILE_MASK) + psStats->baseWidth  %2 * TILE_UNITS/2;
		y = (y & ~TILE_MASK) + psStats->baseBreadth%2 * TILE_UNITS/2;
	}
	else
	{
		if ((x & TILE_MASK) != psStats->baseWidth  %2 * TILE_UNITS/2 ||
		    (y & TILE_MASK) != psStats->baseBreadth%2 * TILE_UNITS/2)
		{
			debug(LOG_WARNING, "Feature not aligned. position (%d,%d), size (%d,%d)", x, y, psStats->baseWidth, psStats->baseBreadth);
		}
	}

	psFeature->pos.x = x;
	psFeature->pos.y = y;

	StructureBounds b = getStructureBounds(psFeature);

	// get the terrain average height
	int foundationMin = INT32_MAX;
	int foundationMax = INT32_MIN;
	for (int breadth = 0; breadth <= b.size.y; ++breadth)
	{
		for (int width = 0; width <= b.size.x; ++width)
		{
			int h = map_TileHeight(b.map.x + width, b.map.y + breadth);
			foundationMin = std::min(foundationMin, h);
			foundationMax = std::max(foundationMax, h);
		}
	}
	//return the average of max/min height
	int height = (foundationMin + foundationMax) / 2;

	if (psStats->subType == FEAT_TREE)
	{
		psFeature->rot.direction = gameRand(DEG_360);
	}
	else
	{
		psFeature->rot.direction = 0;
	}
	psFeature->body = psStats->body;
	psFeature->periodicalDamageStart = 0;
	psFeature->periodicalDamage = 0;

	// it has never been drawn
	psFeature->sDisplay.frameNumber = 0;

	memset(psFeature->seenThisTick, 0, sizeof(psFeature->seenThisTick));
	memset(psFeature->visible, 0, sizeof(psFeature->visible));

	// set up the imd for the feature
	psFeature->sDisplay.imd = psStats->psImd;

	ASSERT_OR_RETURN(NULL, psFeature->sDisplay.imd, "No IMD for feature");		// make sure we have an imd.

	for (int breadth = 0; breadth < b.size.y; ++breadth)
	{
		for (int width = 0; width < b.size.x; ++width)
		{
			MAPTILE *psTile = mapTile(b.map.x + width, b.map.y + breadth);

			//check not outside of map - for load save game
			ASSERT_OR_RETURN(NULL, b.map.x + width < mapWidth, "x coord bigger than map width - %s, id = %d", getName(psFeature->psStats), psFeature->id);
			ASSERT_OR_RETURN(NULL, b.map.y + breadth < mapHeight, "y coord bigger than map height - %s, id = %d", getName(psFeature->psStats), psFeature->id);

			if (width != psStats->baseWidth && breadth != psStats->baseBreadth)
			{
				if (TileHasFeature(psTile))
				{
					FEATURE *psBlock = (FEATURE *)psTile->psObject;

					debug(LOG_ERROR, "%s(%d) already placed at (%d+%d, %d+%d) when trying to place %s(%d) at (%d+%d, %d+%d) - removing it",
					      getName(psBlock->psStats), psBlock->id, map_coord(psBlock->pos.x), psBlock->psStats->baseWidth, map_coord(psBlock->pos.y),
					      psBlock->psStats->baseBreadth, getName(psFeature->psStats), psFeature->id, b.map.x, b.size.x, b.map.y, b.size.y);

					removeFeature(psBlock);
				}

				psTile->psObject = (BASE_OBJECT*)psFeature;

				// if it's a tall feature then flag it in the map.
				if (psFeature->sDisplay.imd->max.y > TALLOBJECT_YMAX)
				{
					auxSetBlocking(b.map.x + width, b.map.y + breadth, AIR_BLOCKED);
				}

				if (psStats->subType != FEAT_GEN_ARTE && psStats->subType != FEAT_OIL_DRUM)
				{
					auxSetBlocking(b.map.x + width, b.map.y + breadth, FEATURE_BLOCKED);
				}
			}

			if( (!psStats->tileDraw) && (FromSave == false) )
			{
				psTile->height = height;
			}
		}
	}
	psFeature->pos.z = map_TileHeight(b.map.x, b.map.y);//jps 18july97

	return psFeature;
}


FEATURE::FEATURE(uint32_t id, FEATURE_STATS const *psStats)
	: BASE_OBJECT(OBJ_FEATURE, id, PLAYER_FEATURE)  // Set the default player out of range to avoid targeting confusions
	, psStats(psStats)
{}

/* Release the resources associated with a feature */
FEATURE::~FEATURE()
{
}

void _syncDebugFeature(const char *function, FEATURE const *psFeature, char ch)
{
	int list[] =
	{
		ch,

		(int)psFeature->id,

		psFeature->player,
		psFeature->pos.x, psFeature->pos.y, psFeature->pos.z,
		(int)psFeature->psStats->subType,
		psFeature->psStats->damageable,
		(int)psFeature->body,
	};
	_syncDebugIntList(function, "%c feature%d = p%d;pos(%d,%d,%d),subtype%d,damageable%d,body%d", list, ARRAY_SIZE(list));
}

/* Update routine for features */
void featureUpdate(FEATURE *psFeat)
{
	syncDebugFeature(psFeat, '<');

	/* Update the periodical damage data */
	if (psFeat->periodicalDamageStart != 0 && psFeat->periodicalDamageStart != gameTime - deltaGameTime)  // -deltaGameTime, since projectiles are updated after features.
	{
		// The periodicalDamageStart has been set, but is not from the previous tick, so we must be out of the periodical damage.
		psFeat->periodicalDamage = 0;  // Reset periodical damage done this tick.
		// Finished periodical damaging
		psFeat->periodicalDamageStart = 0;
	}

	syncDebugFeature(psFeat, '>');
}


// free up a feature with no visual effects
bool removeFeature(FEATURE *psDel)
{
	MESSAGE		*psMessage;
	Vector3i	pos;

	ASSERT_OR_RETURN(false, psDel != NULL, "Invalid feature pointer");
	ASSERT_OR_RETURN(false, !psDel->died, "Feature already dead");

	//remove from the map data
	StructureBounds b = getStructureBounds(psDel);
	for (int breadth = 0; breadth < b.size.y; ++breadth)
	{
		for (int width = 0; width < b.size.x; ++width)
		{
			if (tileOnMap(b.map.x + width, b.map.y + breadth))
			{
				MAPTILE *psTile = mapTile(b.map.x + width, b.map.y + breadth);
	 
				if (psTile->psObject == psDel)
				{
					psTile->psObject = NULL;
					auxClearBlocking(b.map.x + width, b.map.y + breadth, FEATURE_BLOCKED | AIR_BLOCKED);
				}
			}
		}
	}

	if (psDel->psStats->subType == FEAT_GEN_ARTE || psDel->psStats->subType == FEAT_OIL_DRUM)
	{
		pos.x = psDel->pos.x;
		pos.z = psDel->pos.y;
		pos.y = map_Height(pos.x, pos.z) + 30;
		addEffect(&pos, EFFECT_EXPLOSION, EXPLOSION_TYPE_DISCOVERY, false, NULL, 0, gameTime - deltaGameTime + 1);
		if (psDel->psStats->subType == FEAT_GEN_ARTE)
		{
			scoreUpdateVar(WD_ARTEFACTS_FOUND);
			intRefreshScreen();
		}
	}

	if (psDel->psStats->subType == FEAT_GEN_ARTE || psDel->psStats->subType == FEAT_OIL_RESOURCE)
	{
		for (unsigned player = 0; player < MAX_PLAYERS; ++player)
		{
			psMessage = findMessage((MSG_VIEWDATA *)psDel, MSG_PROXIMITY, player);
			while (psMessage)
			{
				removeMessage(psMessage, player);
				psMessage = findMessage((MSG_VIEWDATA *)psDel, MSG_PROXIMITY, player);
			}
		}
	}

	debug(LOG_DEATH, "Killing off feature %s id %d (%p)", objInfo(psDel), psDel->id, psDel);
	killFeature(psDel);

	return true;
}

/* Remove a Feature and free it's memory */
bool destroyFeature(FEATURE *psDel, unsigned impactTime)
{
	UDWORD			widthScatter,breadthScatter,heightScatter, i;
	EFFECT_TYPE		explosionSize;
	Vector3i pos;

	ASSERT_OR_RETURN(false, psDel != NULL, "Invalid feature pointer");

 	/* Only add if visible and damageable*/
	if(psDel->visible[selectedPlayer] && psDel->psStats->damageable)
	{
		/* Set off a destruction effect */
		/* First Explosions */
		widthScatter = TILE_UNITS/2;
		breadthScatter = TILE_UNITS/2;
		heightScatter = TILE_UNITS/4;
		//set which explosion to use based on size of feature
		if (psDel->psStats->baseWidth < 2 && psDel->psStats->baseBreadth < 2)
		{
			explosionSize = EXPLOSION_TYPE_SMALL;
		}
		else if (psDel->psStats->baseWidth < 3 && psDel->psStats->baseBreadth < 3)
		{
			explosionSize = EXPLOSION_TYPE_MEDIUM;
		}
		else
		{
			explosionSize = EXPLOSION_TYPE_LARGE;
		}
		for(i=0; i<4; i++)
		{
			pos.x = psDel->pos.x + widthScatter - rand()%(2*widthScatter);
			pos.z = psDel->pos.y + breadthScatter - rand()%(2*breadthScatter);
			pos.y = psDel->pos.z + 32 + rand()%heightScatter;
			addEffect(&pos, EFFECT_EXPLOSION, explosionSize, false, NULL, 0, impactTime);
		}

		if(psDel->psStats->subType == FEAT_SKYSCRAPER)
		{
			pos.x = psDel->pos.x;
			pos.z = psDel->pos.y;
			pos.y = psDel->pos.z;
			addEffect(&pos, EFFECT_DESTRUCTION, DESTRUCTION_TYPE_SKYSCRAPER, true, psDel->sDisplay.imd, 0, impactTime);
			initPerimeterSmoke(psDel->sDisplay.imd, pos);

			shakeStart(250);	// small shake
		}

		/* Then a sequence of effects */
		pos.x = psDel->pos.x;
		pos.z = psDel->pos.y;
		pos.y = map_Height(pos.x,pos.z);
		addEffect(&pos, EFFECT_DESTRUCTION, DESTRUCTION_TYPE_FEATURE, false, NULL, 0, impactTime);

		//play sound
		// ffs gj
		if(psDel->psStats->subType == FEAT_SKYSCRAPER)
		{
			audio_PlayStaticTrack( psDel->pos.x, psDel->pos.y, ID_SOUND_BUILDING_FALL );
		}
		else
		{
			audio_PlayStaticTrack( psDel->pos.x, psDel->pos.y, ID_SOUND_EXPLOSION );
		}
	}

	if (psDel->psStats->subType == FEAT_SKYSCRAPER)
	{
		// ----- Flip all the tiles under the skyscraper to a rubble tile
		// smoke effect should disguise this happening
		StructureBounds b = getStructureBounds(psDel);
		for (int breadth = 0; breadth < b.size.y; ++breadth)
		{
			for (int width = 0; width < b.size.x; ++width)
			{
				MAPTILE *psTile = mapTile(b.map.x + width, b.map.y + breadth);
				// stops water texture changing for underwater features
				if (terrainType(psTile) != TER_WATER)
				{
					if (terrainType(psTile) != TER_CLIFFFACE)
					{
						/* Clear feature bits */
						psTile->texture = TileNumber_texture(psTile->texture) | RUBBLE_TILE;
						auxClearBlocking(b.map.x + width, b.map.y + breadth, AUXBITS_ALL);
					}
					else
					{
						/* This remains a blocking tile */
						psTile->psObject = NULL;
						auxClearBlocking(b.map.x + width, b.map.y + breadth, AIR_BLOCKED);  // Shouldn't remain blocking for air units, however.
						psTile->texture = TileNumber_texture(psTile->texture) | BLOCKING_RUBBLE_TILE;
					}
				}
			}
		}
	}

	removeFeature(psDel);
	psDel->died = impactTime;
	return true;
}


SDWORD getFeatureStatFromName(const char *pName)
{
	FEATURE_STATS *psStat;

	for (int inc = 0; inc < numFeatureStats; inc++)
	{
		psStat = &asFeatureStats[inc];
		if (psStat->id.compare(pName) == 0)
		{
			return inc;
		}
	}
	return -1;
}

Vector2i getFeatureStatsSize(FEATURE_STATS const *pFeatureType)
{
	Vector2i size(pFeatureType->baseWidth, pFeatureType->baseBreadth);

	// Feature has normal orientation (or upsidedown).
	return size;
}

StructureBounds getStructureBounds(FEATURE const *object)
{
	Vector2i size = getFeatureStatsSize(object->psStats);
	Vector2i map = map_coord(removeZ(object->pos)) - size/2;

	return StructureBounds(map, size);
}

StructureBounds getStructureBounds(FEATURE_STATS const *stats, Vector2i pos)
{
	Vector2i size = getFeatureStatsSize(stats);
	Vector2i map = map_coord(pos) - size/2;

	return StructureBounds(map, size);
}
