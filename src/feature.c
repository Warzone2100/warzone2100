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
/*
 * Feature.c
 *
 * Load feature stats
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "lib/framework/strres.h"

#include "feature.h"
#include "map.h"
#include "hci.h"
#include "lib/gamelib/gtime.h"
#include "power.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
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

#include "multiplay.h"
#include "advvis.h"

#include "mapgrid.h"
#include "display3d.h"
#include "gateway.h"


/* The statistics for the features */
FEATURE_STATS	*asFeatureStats;
UDWORD			numFeatureStats;

//Value is stored for easy access to this feature in destroyDroid()/destroyStruct()
UDWORD			oilResFeature;

/* other house droid to add */
#define DROID_TEMPLINDEX	0
#define DROID_X				(TILE_UNITS * 37 + TILE_UNITS/2)
#define DROID_Y				(TILE_UNITS + TILE_UNITS/2)
#define DROID_TARX			37
#define DROID_TARY			42

//specifies how far round (in tiles) a constructor droid sound look for more wreckage
#define WRECK_SEARCH 3

void featureInitVars(void)
{
	asFeatureStats = NULL;
	numFeatureStats = 0;
	oilResFeature = 0;
}

static void featureType(FEATURE_STATS* psFeature, char *pType)
{
	if (!strcmp(pType,"HOVER WRECK"))
	{
		psFeature->subType = FEAT_HOVER;
		return;
	}
	if (!strcmp(pType,"TANK WRECK"))
	{
		psFeature->subType = FEAT_TANK;
		return;
	}
	if (!strcmp(pType,"GENERIC ARTEFACT"))
	{
		psFeature->subType = FEAT_GEN_ARTE;
		return;
	}
	if (!strcmp(pType,"OIL RESOURCE"))
	{
		psFeature->subType = FEAT_OIL_RESOURCE;
		return;
	}
	if (!strcmp(pType,"BOULDER"))
	{
		psFeature->subType = FEAT_BOULDER;
		return;
	}
	if (!strcmp(pType,"VEHICLE"))
	{
		psFeature->subType = FEAT_VEHICLE;
		return;
	}
	if (!strcmp(pType,"DROID WRECK"))
	{
		psFeature->subType = FEAT_DROID;
		return;
	}
	if (!strcmp(pType,"BUILDING WRECK"))
	{
		psFeature->subType = FEAT_BUILD_WRECK;
		return;
	}
	if (!strcmp(pType,"BUILDING"))
	{
		psFeature->subType = FEAT_BUILDING;
		return;
	}

	if (!strcmp(pType,"OIL DRUM"))
	{
		psFeature->subType = FEAT_OIL_DRUM;
		return;
	}

	if (!strcmp(pType,"TREE"))
	{
		psFeature->subType = FEAT_TREE;
		return;
	}
	if (!strcmp(pType,"SKYSCRAPER"))
	{
		psFeature->subType = FEAT_SKYSCRAPER;
		return;
	}
	ASSERT(!"unknown feature type", "Unknown Feature Type");
}

/* Load the feature stats */
BOOL loadFeatureStats(const char *pFeatureData, UDWORD bufferSize)
{
	FEATURE_STATS		*psFeature;
	unsigned int		i;
	char				featureName[MAX_NAME_SIZE], GfxFile[MAX_NAME_SIZE],
						type[MAX_NAME_SIZE];

	numFeatureStats = numCR(pFeatureData, bufferSize);

	asFeatureStats = (FEATURE_STATS*)malloc(sizeof(FEATURE_STATS) * numFeatureStats);

	if (asFeatureStats == NULL)
	{
		debug( LOG_ERROR, "Feature Stats - Out of memory" );
		abort();
		return FALSE;
	}

	psFeature = asFeatureStats;
	for (i = 0; i < numFeatureStats; i++)
	{
		UDWORD Width, Breadth;

		memset(psFeature, 0, sizeof(FEATURE_STATS));

		featureName[0] = '\0';
		GfxFile[0] = '\0';
		type[0] = '\0';

		//read the data into the storage - the data is delimeted using comma's
		sscanf(pFeatureData,"%[^','],%d,%d,%d,%d,%d,%[^','],%[^','],%d,%d,%d",
			featureName, &Width, &Breadth,
			&psFeature->damageable, &psFeature->armourValue, &psFeature->body,
			GfxFile, type, &psFeature->tileDraw, &psFeature->allowLOS,
			&psFeature->visibleAtStart);

		// These are now only 16 bits wide - so we need to copy them
		psFeature->baseWidth = (UWORD)Width;
		psFeature->baseBreadth = (UWORD)Breadth;

		if (!allocateName(&psFeature->pName, featureName))
		{
			return FALSE;
		}

		//determine the feature type
		featureType(psFeature, type);

		//and the oil resource - assumes only one!
		if (psFeature->subType == FEAT_OIL_RESOURCE)
		{
			oilResFeature = i;
		}

		//get the IMD for the feature
		psFeature->psImd = (iIMDShape *) resGetData("IMD", GfxFile);
		if (psFeature->psImd == NULL)
		{
			debug( LOG_ERROR, "Cannot find the feature PIE for record %s",  getName( psFeature->pName ) );
			abort();
			return FALSE;
		}

		psFeature->ref = REF_FEATURE_START + i;

		//increment the pointer to the start of the next record
		pFeatureData = strchr(pFeatureData,'\n') + 1;
		//increment the list to the start of the next storage block
		psFeature++;
	}

	return TRUE;
}

/* Release the feature stats memory */
void featureStatsShutDown(void)
{
#if !defined(RESOURCE_NAMES) && !defined(STORE_RESOURCE_ID)
	FEATURE_STATS	*psFeature = asFeatureStats;
	UDWORD			inc;

	for(inc=0; inc < numFeatureStats; inc++, psFeature++)
	{
		free(psFeature->pName);
	}
#endif

	if(numFeatureStats)
	{
		free(asFeatureStats);
		asFeatureStats = NULL;
	}
}

/* Deals with damage to a feature
 * \param psFeature feature to deal damage to
 * \param damage amount of damage to deal
 * \param weaponSubClass the subclass of the weapon that deals the damage
 * \return TRUE when the dealt damage destroys the feature, FALSE when the feature survives
 */
SDWORD featureDamage(FEATURE *psFeature, UDWORD damage, UDWORD weaponClass, UDWORD weaponSubClass, HIT_SIDE impactSide)
{
	// Do at least one point of damage
	unsigned int actualDamage = 1;
	float		body = (float) psFeature->body;
	float		originalBody = (float) psFeature->psStats->body;

	ASSERT( psFeature != NULL,
		"featureDamage: Invalid feature pointer" );

	debug( LOG_ATTACK, "featureDamage(%d): body %d armour %d damage: %d\n",
		psFeature->id, psFeature->body, psFeature->armour[impactSide][weaponClass], damage);

	// EMP cannons do not work on Features
	if (weaponSubClass == WSC_EMP)
	{
		return 0;
	}

	if (damage > psFeature->armour[impactSide][weaponClass])
	{
		// Damage has penetrated - reduce body points
		actualDamage = damage - psFeature->armour[impactSide][weaponClass];
		debug( LOG_ATTACK, "        penetrated: %d\n", actualDamage);
	}

	// If the shell did sufficient damage to destroy the feature
	if (actualDamage >= psFeature->body)
	{
		destroyFeature(psFeature);
		return (SDWORD) (body / originalBody * -100);
	}

	// Substract the dealt damage from the feature's remaining body points
	psFeature->body -= actualDamage;

	// Set last hit-time to <now>
	psFeature->timeLastHit = gameTime;

	return (SDWORD) ((float) actualDamage / originalBody * 100);
}


/* Create a feature on the map */
FEATURE * buildFeature(FEATURE_STATS *psStats, UDWORD x, UDWORD y,BOOL FromSave)
{
	UDWORD		mapX, mapY;
	UDWORD		width,breadth, foundationMin,foundationMax, height;
	UDWORD		startX,startY,max,min;
	SDWORD		i;
	UBYTE		vis;

	//try and create the Feature
	FEATURE* psFeature = createFeature();
	if (psFeature == NULL)
	{
		return NULL;
	}
	// features are not in the cluster system
	// this will cause an assert when they still end up there
	psFeature->cluster = ~0;
	//add the feature to the list - this enables it to be drawn whilst being built
	addFeature(psFeature);

	// get the terrain average height
	startX = map_coord(x);
	startY = map_coord(y);
	foundationMin = TILE_MAX_HEIGHT;
	foundationMax = TILE_MIN_HEIGHT;
	for (breadth = 0; breadth < psStats->baseBreadth; breadth++)
	{
		for (width = 0; width < psStats->baseWidth; width++)
		{
			getTileMaxMin(startX + width, startY + breadth, &max, &min);
			if (foundationMin > min)
			{
				foundationMin = min;
			}
			if (foundationMax < max)
			{
				foundationMax = max;
			}
		}
	}
	//return the average of max/min height
	height = (foundationMin + foundationMax) / 2;

	// check you can reach an oil resource
	if ((psStats->subType == FEAT_OIL_RESOURCE) &&
		!gwZoneReachable(gwGetZone(startX,startY)))
	{
		debug( LOG_NEVER, "Oil resource at (%d,%d) is unreachable", startX, startY );
	}

	if(FromSave == TRUE) {
		psFeature->x = (UWORD)x;
		psFeature->y = (UWORD)y;
	} else {
		psFeature->x = (UWORD)((x & (~TILE_MASK)) + psStats->baseWidth * TILE_UNITS / 2);
		psFeature->y = (UWORD)((y & (~TILE_MASK)) + psStats->baseBreadth * TILE_UNITS / 2);
	}

	/* Dump down the building wrecks at random angles - still looks shit though */
	if(psStats->subType == FEAT_BUILD_WRECK)
	{
		psFeature->direction = rand() % 360;
	}
	else if(psStats->subType == FEAT_TREE)
	{
		psFeature->direction = rand() % 360;
	}
	else
	{
		psFeature->direction = 0;
	}
	//psFeature->damage = featureDamage;
	psFeature->selected = FALSE;
	psFeature->psStats = psStats;
	//psFeature->subType = psStats->subType;
	psFeature->body = psStats->body;
	psFeature->player = MAX_PLAYERS+1;	//set the player out of range to avoid targeting confusions

	psFeature->bTargetted = FALSE;
	psFeature->timeLastHit = 0;

	// it has never been drawn
	psFeature->sDisplay.frameNumber = 0;

	if(getRevealStatus())
	{
		vis = 0;
	}
	else
	{
		if(psStats->visibleAtStart)
		{
  			vis = UBYTE_MAX;
		}
		else
		{
			vis = 0;
		}
	}

	// note that the advanced armour system current unused for features
	for (i = 0; i < NUM_HIT_SIDES; i++)
	{
		int j;

		for (j = 0; j < NUM_WEAPON_CLASS; j++)
		{
			psFeature->armour[i][j] = psFeature->psStats->armourValue;
		}
	}

	for(i=0; i<MAX_PLAYERS; i++)
	{
		psFeature->visible[i] = 0;//vis;
	}
	//load into the map data

	if(FromSave) {
		mapX = map_coord(x) - psStats->baseWidth / 2;
		mapY = map_coord(y) - psStats->baseBreadth / 2;
	} else {
		mapX = map_coord(x);
		mapY = map_coord(y);
	}

	// set up the imd for the feature
	if(psFeature->psStats->subType==FEAT_BUILD_WRECK)
	{

		psFeature->sDisplay.imd = getRandomWreckageImd();

	}
	else
	{
		psFeature->sDisplay.imd = psStats->psImd;
  	}


	assert(psFeature->sDisplay.imd);		// make sure we have an imd.

	for (width = 0; width <= psStats->baseWidth; width++)
	{
		for (breadth = 0; breadth <= psStats->baseBreadth; breadth++)
		{
			MAPTILE *psTile = mapTile(mapX + width, mapY + breadth);

			//check not outside of map - for load save game
			ASSERT( (mapX+width) < mapWidth,
				"x coord bigger than map width - %s, id = %d",
				getName(psFeature->psStats->pName), psFeature->id );
			ASSERT( (mapY+breadth) < mapHeight,
				"y coord bigger than map height - %s, id = %d",
				getName(psFeature->psStats->pName), psFeature->id );

			if (width != psStats->baseWidth && breadth != psStats->baseBreadth)
			{
				ASSERT( !(TILE_HAS_FEATURE(psTile)),
					"buildFeature - feature- %d already found at %d, %d",
					psFeature->id, mapX+width,mapY+breadth );

				psTile->psObject = (BASE_OBJECT*)psFeature;

				if (psStats->subType == FEAT_GEN_ARTE || psStats->subType == FEAT_OIL_DRUM || psStats->subType == FEAT_BUILD_WRECK)// they're there - just can see me
				{
					SET_TILE_NOTBLOCKING(psTile);
				}
			}

			if( (!psStats->tileDraw) && (FromSave == FALSE) )
			{
				psTile->height = (UBYTE)(height / ELEVATION_SCALE);
			}
		}
	}
	psFeature->z = map_TileHeight(mapX,mapY);//jps 18july97

	//store the time it was built for removing wrecked droids/structures
	psFeature->startTime = gameTime;

//	// set up the imd for the feature
//	if(psFeature->psStats->subType==FEAT_BUILD_WRECK)
//	{
//		psFeature->sDisplay.imd = wreckageImds[rand()%MAX_WRECKAGE];
//	}
//	else
//	{
//		psFeature->sDisplay.imd = psStats->psImd;
// 	}

	// add the feature to the grid
	gridAddObject((BASE_OBJECT *)psFeature);

	return psFeature;
}


/* Release the resources associated with a feature */
void featureRelease(FEATURE *psFeature)
{
	gridRemoveObject((BASE_OBJECT *)psFeature);
}


/* Update routine for features */
void featureUpdate(FEATURE *psFeat)
{
   //	if(getRevealStatus())
   //	{
		// update the visibility for the feature
		processVisibility((BASE_OBJECT *)psFeat);
   //	}

	switch (psFeat->psStats->subType)
	{
	case FEAT_DROID:
	case FEAT_BUILD_WRECK:
//		//kill off wrecked droids and structures after 'so' long
//		if ((gameTime - psFeat->startTime) > WRECK_LIFETIME)
//		{
			destroyFeature(psFeat); // get rid of the now!!!
//		}
		break;
	default:
		break;
	}
}


// free up a feature with no visual effects
void removeFeature(FEATURE *psDel)
{
	UDWORD		mapX, mapY, width,breadth, player;
	MESSAGE		*psMessage;
	Vector3i pos;

	ASSERT( psDel != NULL,
		"removeFeature: invalid feature pointer\n" );

	if (psDel->died)
	{
		// feature has already been killed, quit
		ASSERT( FALSE,
			"removeFeature: feature already dead" );
		return;
	}


	if(bMultiPlayer && !ingame.localJoiningInProgress)
	{
		SendDestroyFeature(psDel);	// inform other players of destruction
	}


	//remove from the map data
	mapX = map_coord(psDel->x - psDel->psStats->baseWidth * TILE_UNITS / 2);
	mapY = map_coord(psDel->y - psDel->psStats->baseBreadth * TILE_UNITS / 2);
	for (width = 0; width < psDel->psStats->baseWidth; width++)
	{
		for (breadth = 0; breadth < psDel->psStats->baseBreadth; breadth++)
		{
			MAPTILE *psTile = mapTile(mapX + width, mapY + breadth);

			psTile->psObject = NULL;

			CLEAR_TILE_NOTBLOCKING(psTile);
		}
	}

	if(psDel->psStats->subType == FEAT_GEN_ARTE)
	{
		pos.x = psDel->x;
		pos.z = psDel->y;
		pos.y = map_Height(pos.x,pos.z);
		addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_DISCOVERY,FALSE,NULL,0);
		scoreUpdateVar(WD_ARTEFACTS_FOUND);
		intRefreshScreen();
	}

	if (  (psDel->psStats->subType == FEAT_GEN_ARTE)
		||(psDel->psStats->subType == FEAT_OIL_RESOURCE))
	{
		// have to check all players cos if you cheat you'll get em.
		for (player=0; player<MAX_PLAYERS; player ++)
		{
			//see if there is a proximity message FOR THE SELECTED PLAYER at this location
			psMessage = findMessage((MSG_VIEWDATA *)psDel, MSG_PROXIMITY, selectedPlayer);
			while (psMessage)
			{
				removeMessage(psMessage, selectedPlayer);
				psMessage = findMessage((MSG_VIEWDATA *)psDel, MSG_PROXIMITY, player);
			}
		}
	}

	// remove the feature from the grid
	gridRemoveObject((BASE_OBJECT *)psDel);

	killFeature(psDel);
}

/* Remove a Feature and free it's memory */
void destroyFeature(FEATURE *psDel)
{
	UDWORD			widthScatter,breadthScatter,heightScatter, i;
	EFFECT_TYPE		explosionSize;
	Vector3i pos;
	UDWORD			width,breadth;
	UDWORD			mapX,mapY;
	UDWORD			texture;

	ASSERT( psDel != NULL,
		"destroyFeature: invalid feature pointer\n" );

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
			pos.x = psDel->x + widthScatter - rand()%(2*widthScatter);
			pos.z = psDel->y + breadthScatter - rand()%(2*breadthScatter);
			pos.y = psDel->z + 32 + rand()%heightScatter;
			addEffect(&pos,EFFECT_EXPLOSION,explosionSize,FALSE,NULL,0);
		}

//	  	if(psDel->sDisplay.imd->ymax>300)	// WARNING - STATS CHANGE NEEDED!!!!!!!!!!!
		if(psDel->psStats->subType == FEAT_SKYSCRAPER)
		{
			pos.x = psDel->x;
			pos.z = psDel->y;
			pos.y = psDel->z;
			addEffect(&pos,EFFECT_DESTRUCTION,DESTRUCTION_TYPE_SKYSCRAPER,TRUE,psDel->sDisplay.imd,0);
			initPerimeterSmoke(psDel->sDisplay.imd,pos.x,pos.y,pos.z);

			// ----- Flip all the tiles under the skyscraper to a rubble tile
			// smoke effect should disguise this happening
			mapX = map_coord(psDel->x - psDel->psStats->baseWidth * TILE_UNITS / 2);
			mapY = map_coord(psDel->y - psDel->psStats->baseBreadth * TILE_UNITS / 2);
//			if(psDel->sDisplay.imd->ymax>300)
			if (psDel->psStats->subType == FEAT_SKYSCRAPER)
			{
				for (width = 0; width < psDel->psStats->baseWidth; width++)
				{
					for (breadth = 0; breadth < psDel->psStats->baseBreadth; breadth++)
					{
						MAPTILE *psTile = mapTile(mapX+width,mapY+breadth);
						// stops water texture chnaging for underwateer festures
					 	if (terrainType(psTile) != TER_WATER)
						{
							if (terrainType(psTile) != TER_CLIFFFACE)
							{
						   		/* Clear feature bits */
								psTile->psObject = NULL;
								texture = TileNumber_texture(psTile->texture) | RUBBLE_TILE;
								psTile->texture = (UWORD)texture;
							}
							else
							{
							   /* This remains a blocking tile */
								psTile->psObject = NULL;
								texture = TileNumber_texture(psTile->texture) | BLOCKING_RUBBLE_TILE;
								psTile->texture = (UWORD)texture;

							}
						}
					}
				}
			// -------
			shakeStart();
			}
		}

		/* Then a sequence of effects */
		pos.x = psDel->x;
		pos.z = psDel->y;
		pos.y = map_Height(pos.x,pos.z);
		addEffect(&pos,EFFECT_DESTRUCTION,DESTRUCTION_TYPE_FEATURE,FALSE,NULL,0);

		//play sound
		// ffs gj
		if(psDel->psStats->subType == FEAT_SKYSCRAPER)
		{
			audio_PlayStaticTrack( psDel->x, psDel->y, ID_SOUND_BUILDING_FALL );
		}
		else

		{
			audio_PlayStaticTrack( psDel->x, psDel->y, ID_SOUND_EXPLOSION );
		}
	}
//---------------------------------------------------------------------------------------

	removeFeature(psDel);
}


SDWORD getFeatureStatFromName( const char *pName )
{
	unsigned int inc;
	FEATURE_STATS *psStat;

#ifdef RESOURCE_NAMES
	if (!getResourceName(pName))
	{
		return -1;
	}
#endif

	for (inc = 0; inc < numFeatureStats; inc++)
	{
		psStat = &asFeatureStats[inc];
		if (!strcmp(psStat->pName, pName))
		{
			return inc;
		}
	}
	return -1;
}


/*looks around the given droid to see if there is any building
wreckage to clear*/
FEATURE	* checkForWreckage(DROID *psDroid)
{
	FEATURE		*psFeature;
	UDWORD		startX, startY, incX, incY;
	SDWORD		x=0, y=0;

	startX = map_coord(psDroid->x);
	startY = map_coord(psDroid->y);

	//look around the droid - max 2 tiles distance
	for (incX = 1, incY = 1; incX < WRECK_SEARCH; incX++, incY++)
	{
		/* across the top */
		y = startY - incY;
		for(x = startX - incX; x < (SDWORD)(startX + incX); x++)
		{
			if(TILE_HAS_FEATURE(mapTile(x,y)))
			{
				psFeature = getTileFeature(x, y);
				if(psFeature && psFeature->psStats->subType == FEAT_BUILD_WRECK)
				{
					return psFeature;
				}
			}
		}
		/* the right */
		x = startX + incX;
		for(y = startY - incY; y < (SDWORD)(startY + incY); y++)
		{
			if(TILE_HAS_FEATURE(mapTile(x,y)))
			{
				psFeature = getTileFeature(x, y);
				if(psFeature && psFeature->psStats->subType == FEAT_BUILD_WRECK)
				{
					return psFeature;
				}
			}
		}
		/* across the bottom*/
		y = startY + incY;
		for(x = startX + incX; x > (SDWORD)(startX - incX); x--)
		{
			if(TILE_HAS_FEATURE(mapTile(x,y)))
			{
				psFeature = getTileFeature(x, y);
				if(psFeature && psFeature->psStats->subType == FEAT_BUILD_WRECK)
				{
					return psFeature;
				}
			}
		}

		/* the left */
		x = startX - incX;
		for(y = startY + incY; y > (SDWORD)(startY - incY); y--)
		{
			if(TILE_HAS_FEATURE(mapTile(x,y)))
			{
				psFeature = getTileFeature(x, y);
				if(psFeature && psFeature->psStats->subType == FEAT_BUILD_WRECK)
				{
					return psFeature;
				}
			}
		}
	}
	return NULL;
}
