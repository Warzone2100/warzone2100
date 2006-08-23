/*
 * Feature.c
 *
 * Load feature stats
 */

#include <stdio.h>
#include <assert.h>

#include "lib/framework/frame.h"
#include "feature.h"
#include "map.h"
#include "hci.h"
#include "lib/gamelib/gtime.h"
#include "power.h"
#include "lib/sound/audio.h"
#include "audio_id.h"
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

#define	DRIVE_OVER_RUBBLE_TILE		54
#define NO_DRIVE_OVER_RUBBLE_TILE	67

//Value is stored for easy access to this feature in destroyDroid()/destroyStruct()
//UDWORD			droidFeature;
UDWORD			structFeature;
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
	//droidFeature = 0;
	structFeature = 0;
	oilResFeature = 0;
}

void featureType(FEATURE_STATS* psFeature, char *pType)
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
	ASSERT( FALSE, "Unknown Feature Type" );
}

/* Load the feature stats */
BOOL loadFeatureStats(char *pFeatureData, UDWORD bufferSize)
{
	char				*pData;
	FEATURE_STATS		*psFeature;
	UDWORD				i;
	STRING				featureName[MAX_NAME_SIZE], GfxFile[MAX_NAME_SIZE],
						type[MAX_NAME_SIZE];
						//compName[MAX_NAME_SIZE], compType[MAX_NAME_SIZE];

	//keep the start so we release it at the end
	pData = pFeatureData;

	numFeatureStats = numCR(pFeatureData, bufferSize);

	asFeatureStats = (FEATURE_STATS *)MALLOC(sizeof(FEATURE_STATS)*
		numFeatureStats);

	if (asFeatureStats == NULL)
	{
		debug( LOG_ERROR, "Feature Stats - Out of memory" );
		abort();
		return FALSE;
	}

	psFeature = asFeatureStats;
	for (i=0; i < numFeatureStats; i++)
	{
		UDWORD Width,Breadth;

		memset(psFeature, 0, sizeof(FEATURE_STATS));
		featureName[0] = '\0';
		GfxFile[0] = '\0';
		type[0] = '\0';
		//read the data into the storage - the data is delimeted using comma's

/*		sscanf(pFeatureData,"%[^','],%d,%d,%d,%d,%d,%[^','],%[^','],%[^','],%[^','],%d,%d,%d",
			&featureName, &psFeature->baseWidth, &psFeature->baseBreadth,
			&psFeature->damageable, &psFeature->armour, &psFeature->body,
			&GfxFile, &type, &compType, &compName,
			&psFeature->tileDraw, &psFeature->allowLOS, &psFeature->visibleAtStart);*/
		sscanf(pFeatureData,"%[^','],%d,%d,%d,%d,%d,%[^','],%[^','],%d,%d,%d",
			featureName, &Width, &Breadth,
			&psFeature->damageable, &psFeature->armour, &psFeature->body,
			GfxFile, type, &psFeature->tileDraw, &psFeature->allowLOS,
			&psFeature->visibleAtStart);


		// These are now only 16 bits wide - so we need to copy them
		psFeature->baseWidth=(UWORD)Width;
		psFeature->baseBreadth=(UWORD)Breadth;


#ifdef HASH_NAMES
		psFeature->NameHash=HashString(featureName);
#else
		if (!allocateName(&psFeature->pName, featureName))
		{
			return FALSE;
		}
#endif

		//determine the feature type
		featureType(psFeature, type);

		//need to know which is the wrecked droid and wrecked structure for later use
		//the last stat of each type is used
		/*if (psFeature->subType == FEAT_DROID)
		{
			droidFeature = i;
		}
		else */if (psFeature->subType == FEAT_BUILD_WRECK)
		{
			structFeature = i;
		}
		//and the oil resource - assumes only one!
		else if (psFeature->subType == FEAT_OIL_RESOURCE)
		{
			oilResFeature = i;
		}

		//get the IMD for the feature
		psFeature->psImd = (iIMDShape *) resGetData("IMD", GfxFile);
		if (psFeature->psImd == NULL)
		{
#ifdef HASH_NAMES
			debug( LOG_ERROR, "Cannot find the feature PIE for record %s",  strresGetString( NULL, psFeature->NameHash ) );
#else
			debug( LOG_ERROR, "Cannot find the feature PIE for record %s",  getName( psFeature->pName ) );
#endif
			abort();
			return FALSE;
		}

		//sort out the component - if any
		/*if (strcmp(compType, "0"))
		{
			psFeature->compType = componentType(compType);
			psFeature->compIndex = getCompFromName(psFeature->compType, compName);
		}*/

		psFeature->ref = REF_FEATURE_START + i;

		//increment the pointer to the start of the next record
		pFeatureData = strchr(pFeatureData,'\n') + 1;
		//increment the list to the start of the next storage block
		psFeature++;
	}

//	FREE(pData);

	return TRUE;

	/* Allocate the stats Array */
/*	numFeatureStats = 19;
	asFeatureStats = (FEATURE_STATS *)MALLOC(sizeof(FEATURE_STATS) * numFeatureStats);
	if (!asFeatureStats)
	{
		DBERROR(("Out of memory"));
		return FALSE;
	}
	memset(asFeatureStats, 0, sizeof(FEATURE_STATS) * numFeatureStats);

	// Create some simple stats
	ref = REF_FEATURE_START;
	psStats = asFeatureStats;
	psStats->pName = "Mesa Feature";
	psStats->ref = ref ++;
	psStats->subType = FEAT_MESA;
	psStats->baseWidth = 6;
	psStats->baseBreadth = 4;
	psStats->tileDraw = FALSE;
	psStats->allowLOS = FALSE;
	psStats->psImd = resGetData("IMD", "mimesa1.imd");

	psStats++;
	psStats->pName = "Mesa Feature 2";
	psStats->ref = ref ++;
	psStats->subType = FEAT_MESA2;
	psStats->baseWidth = 5;
	psStats->baseBreadth = 4;
	psStats->tileDraw = FALSE;
	psStats->allowLOS = FALSE;
	psStats->psImd = resGetData("IMD", "mimesa2.imd");

	psStats++;
	psStats->pName = "Cliff";
	psStats->ref = ref ++;
	psStats->subType = FEAT_CLIFF;
	psStats->baseWidth = 7;
	psStats->baseBreadth = 7;
	psStats->tileDraw = FALSE;
	psStats->allowLOS = FALSE;
	psStats->psImd = resGetData("IMD", "micliff.imd");

	psStats++;
	psStats->pName = "Stack";
	psStats->ref = ref ++;
	psStats->subType = FEAT_STACK;
	psStats->baseWidth = 2;
	psStats->baseBreadth = 2;
	psStats->tileDraw = FALSE;
	psStats->allowLOS = FALSE;
	psStats->psImd = resGetData("IMD", "mistack.imd");

	psStats++;
	psStats->pName = "Wrecked Building";
	psStats->ref = ref ++;
	psStats->subType = FEAT_BUILD_WRECK1;
	psStats->baseWidth = 1;
	psStats->baseBreadth = 1;
	psStats->tileDraw = FALSE;
	psStats->allowLOS = TRUE;
	psStats->psImd = resGetData("IMD", "miwreck.imd");

	psStats++;
	psStats->pName = "Wrecked Hovercraft";
	psStats->ref = ref ++;
	psStats->subType = FEAT_HOVER;
	psStats->baseWidth = 1;
	psStats->baseBreadth = 1;
	psStats->tileDraw = FALSE;
	psStats->allowLOS = TRUE;
	psStats->psImd = resGetData("IMD", "miarthov.imd");

	// Find the hover component for it
	psStats->compType = COMP_PROPULSION;
	for(comp=0; comp<numPropulsionStats; comp++)
	{
		if (strcmp(asPropulsionStats[comp].pName, "Hover Propulsion") == 0)
		{
			psStats->compIndex = comp;
		}
	}

	psStats++;
	psStats->pName = "Wrecked Tank";
	psStats->ref = ref ++;
	psStats->subType = FEAT_TANK;
	psStats->baseWidth = 1;
	psStats->baseBreadth = 1;
	psStats->tileDraw = FALSE;
	psStats->allowLOS = TRUE;
	psStats->psImd = resGetData("IMD", "miartecm.imd");

	// Find the ecm component for it
	psStats->compType = COMP_ECM;
	for(comp=0; comp<numECMStats; comp++)
	{
		if (strcmp(asECMStats[comp].pName, "Heavy ECM #1") == 0)
		{
			psStats->compIndex = comp;
		}
	}

	psStats++;
	psStats->pName = "Generic Artefact";
	psStats->ref = ref ++;
	psStats->subType = FEAT_GEN_ARTE;
	psStats->baseWidth = 1;
	psStats->baseBreadth = 1;
	psStats->tileDraw = FALSE;
	psStats->allowLOS = FALSE;
	psStats->psImd = resGetData("IMD", "miartgen.imd");

	psStats++;
	psStats->pName = "Oil Resource";
	psStats->ref = ref ++;
	psStats->subType = FEAT_OIL_RESOURCE;
	psStats->baseWidth = 1;
	psStats->baseBreadth = 1;
	psStats->tileDraw = TRUE;
	psStats->allowLOS = TRUE;
	psStats->psImd = resGetData("IMD", "mislick.imd");

	psStats++;
	psStats->pName = "Boulder 1";
	psStats->ref = ref ++;
	psStats->subType = FEAT_BOULDER1;
	psStats->baseWidth = 1;
	psStats->baseBreadth = 1;
	psStats->tileDraw = FALSE;
	psStats->allowLOS = FALSE;
	psStats->psImd = resGetData("IMD", "mibould1.imd");

	psStats++;
	psStats->pName = "Boulder 2";
	psStats->ref = ref ++;
	psStats->subType = FEAT_BOULDER2;
	psStats->baseWidth = 1;
	psStats->baseBreadth = 1;
	psStats->tileDraw = FALSE;
	psStats->allowLOS = FALSE;
	psStats->psImd = resGetData("IMD", "mibould2.imd");

	psStats++;
	psStats->pName = "Boulder 3";
	psStats->ref = ref ++;
	psStats->subType = FEAT_BOULDER3;
	psStats->baseWidth = 1;
	psStats->baseBreadth = 1;
	psStats->tileDraw = FALSE;
	psStats->allowLOS = FALSE;
	psStats->psImd = resGetData("IMD", "mibould3.imd");

	psStats++;
	psStats->pName = "Futuristic Car";
	psStats->ref = ref ++;
	psStats->subType = FEAT_FUTCAR;
	psStats->baseWidth = 1;
	psStats->baseBreadth = 1;
	psStats->tileDraw = FALSE;
	psStats->allowLOS = FALSE;
	psStats->psImd = resGetData("IMD", "mifutcar.imd");

	psStats++;
	psStats->pName = "Fururistic Van";
	psStats->ref = ref ++;
	psStats->subType = FEAT_FUTVAN;
	psStats->baseWidth = 1;
	psStats->baseBreadth = 1;
	psStats->tileDraw = FALSE;
	psStats->allowLOS = FALSE;
	psStats->psImd = resGetData("IMD", "mifutvan.imd");

	psStats++;
	psStats->pName = "Wrecked Droid Hub";
	psStats->ref = ref ++;
	psStats->subType = FEAT_DROID;
	psStats->baseWidth = 1;
	psStats->baseBreadth = 1;
	psStats->tileDraw = TRUE;
	psStats->allowLOS = TRUE;
	psStats->psImd = resGetData("IMD", "drwreck.imd");
	//Value is stored for easy access to this feature in destroyDroid()
	droidFeature = psStats - asFeatureStats;

	psStats++;
	//Value is stored for easy access to this feature in destroyStruct
	structFeature = psStats - asFeatureStats;
	psStats->pName = "Wrecked Building1";
	psStats->ref = ref++;
	psStats->subType = FEAT_BUILD_WRECK1;
	psStats->baseWidth = 1;
	psStats->baseBreadth = 1;
	psStats->tileDraw = FALSE;
	psStats->allowLOS = TRUE;
	psStats->psImd = resGetData("IMD", "miwrek1.imd");

	psStats++;
	psStats->pName = "Wrecked Building2";
	psStats->ref = ref++;
	psStats->subType = FEAT_BUILD_WRECK2;
	psStats->baseWidth = 1;
	psStats->baseBreadth = 1;
	psStats->tileDraw = FALSE;
	psStats->allowLOS = TRUE;
	psStats->psImd = resGetData("IMD", "miwrek2.imd");

	psStats++;
	psStats->pName = "Wrecked Building3";
	psStats->ref = ref++;
	psStats->subType = FEAT_BUILD_WRECK3;
	psStats->baseWidth = 1;
	psStats->baseBreadth = 1;
	psStats->tileDraw = FALSE;
	psStats->allowLOS = TRUE;
	psStats->psImd = resGetData("IMD", "miwrek3.imd");

	psStats++;
	psStats->pName = "Wrecked Building4";
	psStats->ref = ref++;
	psStats->subType = FEAT_BUILD_WRECK4;
	psStats->baseWidth = 1;
	psStats->baseBreadth = 1;
	psStats->tileDraw = FALSE;
	psStats->allowLOS = TRUE;
	psStats->psImd = resGetData("IMD", "miwrek4.imd");

	// These are test features for the LOS code
	psStats++;
	psStats->pName = "Cube 1,1";
	psStats->ref = ref++;
	psStats->subType = FEAT_LOS_OBJ;
	psStats->baseWidth = 1;
	psStats->baseBreadth = 1;
	psStats->tileDraw = TRUE;
	psStats->allowLOS = FALSE;
	psStats->psImd = resGetData("IMD", "cube_1_1.imd");

	psStats++;
	psStats->pName = "Cube 1,3";
	psStats->ref = ref++;
	psStats->subType = FEAT_LOS_OBJ;
	psStats->baseWidth = 1;
	psStats->baseBreadth = 1;
	psStats->tileDraw = TRUE;
	psStats->allowLOS = FALSE;
	psStats->psImd = resGetData("IMD", "cube_1_3.imd");

	psStats++;
	psStats->pName = "Cube 2,1";
	psStats->ref = ref++;
	psStats->subType = FEAT_LOS_OBJ;
	psStats->baseWidth = 2;
	psStats->baseBreadth = 2;
	psStats->tileDraw = TRUE;
	psStats->allowLOS = FALSE;
	psStats->psImd = resGetData("IMD", "cube_2_1.imd");

	psStats++;
	psStats->pName = "Cube 2,3";
	psStats->ref = ref++;
	psStats->subType = FEAT_LOS_OBJ;
	psStats->baseWidth = 2;
	psStats->baseBreadth = 2;
	psStats->tileDraw = TRUE;
	psStats->allowLOS = FALSE;
	psStats->psImd = resGetData("IMD", "cube_2_3.imd");

	psStats++;
	psStats->pName = "Cube 3,1";
	psStats->ref = ref++;
	psStats->subType = FEAT_LOS_OBJ;
	psStats->baseWidth = 3;
	psStats->baseBreadth = 3;
	psStats->tileDraw = TRUE;
	psStats->allowLOS = FALSE;
	psStats->psImd = resGetData("IMD", "cube_3_1.imd");

	psStats++;
	psStats->pName = "Cube 3,3";
	psStats->ref = ref++;
	psStats->subType = FEAT_LOS_OBJ;
	psStats->baseWidth = 3;
	psStats->baseBreadth = 3;
	psStats->tileDraw = TRUE;
	psStats->allowLOS = FALSE;
	psStats->psImd = resGetData("IMD", "cube_3_3.imd");

	psStats++;
	psStats->pName = "Cyl 1,1";
	psStats->ref = ref++;
	psStats->subType = FEAT_LOS_OBJ;
	psStats->baseWidth = 1;
	psStats->baseBreadth = 1;
	psStats->tileDraw = TRUE;
	psStats->allowLOS = FALSE;
	psStats->psImd = resGetData("IMD", "cyl_1_1.imd");

	psStats++;
	psStats->pName = "Cyl 1,3";
	psStats->ref = ref++;
	psStats->subType = FEAT_LOS_OBJ;
	psStats->baseWidth = 1;
	psStats->baseBreadth = 1;
	psStats->tileDraw = TRUE;
	psStats->allowLOS = FALSE;
	psStats->psImd = resGetData("IMD", "cyl_1_3.imd");

	psStats++;
	psStats->pName = "Cyl 2,1";
	psStats->ref = ref++;
	psStats->subType = FEAT_LOS_OBJ;
	psStats->baseWidth = 2;
	psStats->baseBreadth = 2;
	psStats->tileDraw = TRUE;
	psStats->allowLOS = FALSE;
	psStats->psImd = resGetData("IMD", "cyl_2_1.imd");

	psStats++;
	psStats->pName = "Cyl 2,3";
	psStats->ref = ref++;
	psStats->subType = FEAT_LOS_OBJ;
	psStats->baseWidth = 2;
	psStats->baseBreadth = 2;
	psStats->tileDraw = TRUE;
	psStats->allowLOS = FALSE;
	psStats->psImd = resGetData("IMD", "cyl_2_3.imd");

	psStats++;
	psStats->pName = "Cyl 3,1";
	psStats->ref = ref++;
	psStats->subType = FEAT_LOS_OBJ;
	psStats->baseWidth = 3;
	psStats->baseBreadth = 3;
	psStats->tileDraw = TRUE;
	psStats->allowLOS = FALSE;
	psStats->psImd = resGetData("IMD", "cyl_3_1.imd");

	psStats++;
	psStats->pName = "Cyl 3,3";
	psStats->ref = ref++;
	psStats->subType = FEAT_LOS_OBJ;
	psStats->baseWidth = 3;
	psStats->baseBreadth = 3;
	psStats->tileDraw = TRUE;
	psStats->allowLOS = FALSE;
	psStats->psImd = resGetData("IMD", "cyl_3_3.imd");

	psStats++;
	ASSERT( psStats - asFeatureStats == (SDWORD)numFeatureStats,
		"loadFeatureStats: incorrect number of features" );

	return TRUE;*/
}

/* Release the feature stats memory */
void featureStatsShutDown(void)
{
#if !defined(RESOURCE_NAMES) && !defined(STORE_RESOURCE_ID)
	FEATURE_STATS	*psFeature = asFeatureStats;
	UDWORD			inc;

	for(inc=0; inc < numFeatureStats; inc++, psFeature++)
	{
		FREE(psFeature->pName);
	}
#endif

	if(numFeatureStats)
	{
		FREE(asFeatureStats);
	}
}

/* Deal with damage to a feature */
BOOL featureDamage(FEATURE *psFeature, UDWORD damage, UDWORD weaponClass,
                   UDWORD weaponSubClass)
{
	UDWORD		penDamage;

	/* this is ignored for features */
	//(void)weaponClass;

	ASSERT( PTRVALID(psFeature, sizeof(FEATURE)),
		"featureDamage: Invalid feature pointer" );

	DBP1(("featureDamage(%d): body %d armour %d damage: %d\n",
		psFeature->id, psFeature->body, psFeature->psStats->armour, damage));

    //EMP cannons do not work on Features
    if (weaponSubClass == WSC_EMP)
    {
        return FALSE;
    }

	if (damage > psFeature->psStats->armour)
	{
		/* Damage has penetrated - reduce body points */
		penDamage = damage - psFeature->psStats->armour;
		DBP1(("        penetrated: %d\n", penDamage));
		if (penDamage >= psFeature->body)
		{
			/* feature destroyed */
			DBP1(("        DESTROYED\n"));
			destroyFeature(psFeature);
			return TRUE;
		}
		else
		{
			psFeature->body -= penDamage;
		}
	}
	else
	{
		/* Do one point of damage to body */
		DBP1(("        not penetrated - 1 point damage\n"));
		if (psFeature->body == 1)
		{
			destroyFeature(psFeature);
			DBP1(("        DESTROYED\n"));
			return TRUE;
		}
		else
		{
			psFeature->body -= 1;
		}
	}

//	if(psFeature->sDisplay.imd->ymax > 300)
//	{
		psFeature->timeLastHit = gameTime;
//	}

	return FALSE;

}


// Set the tile no draw flags for a structure
void setFeatTileDraw(FEATURE *psFeat)
{
	FEATURE_STATS	*psStats = psFeat->psStats;
	UDWORD			width,breadth, mapX,mapY;

	mapX = (psFeat->x >> TILE_SHIFT) - (psStats->baseWidth/2);
	mapY = (psFeat->y >> TILE_SHIFT) - (psStats->baseBreadth/2);
	if (!psStats->tileDraw)
	{
		for (width = 0; width < psStats->baseWidth; width++)
		{
			for (breadth = 0; breadth < psStats->baseBreadth; breadth++)
			{
				SET_TILE_NODRAW(mapTile(mapX+width,mapY+breadth));
			}
		}
	}
}

/* Create a feature on the map */
FEATURE * buildFeature(FEATURE_STATS *psStats, UDWORD x, UDWORD y,BOOL FromSave)
{
	FEATURE		*psFeature;
	UDWORD		mapX, mapY;
	UDWORD		width,breadth, foundationMin,foundationMax, height;
	MAPTILE		*psTile;
	UDWORD		startX,startY,max,min;
	SDWORD		i;
	UBYTE		vis;

	//try and create the Feature
	if (!createFeature(&psFeature))
	{
		return NULL;
	}
	//add the feature to the list - this enables it to be drawn whilst being built
	addFeature(psFeature);

	// get the terrain average height
	startX = x >> TILE_SHIFT;
	startY = y >> TILE_SHIFT;
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
		psFeature->direction = (UWORD)(rand()%360);
		psFeature->gfxScaling = (UWORD)(80 + (10 - rand()%20)); // put into define
	}
	else if(psStats->subType == FEAT_TREE)
	{
		psFeature->direction = (UWORD)(rand()%360);
		psFeature->gfxScaling = (UWORD) (100 + (14-rand()%28));
	}
	else
	{
		psFeature->direction = 0;
   		psFeature->gfxScaling = 100;	// but irrelevant anyway, cos it's not scaled
	}
	//psFeature->damage = featureDamage;
	psFeature->selected = FALSE;
	psFeature->psStats = psStats;
	//psFeature->subType = psStats->subType;
	psFeature->body = psStats->body;
	psFeature->player = MAX_PLAYERS+1;	//set the player out of range to avoid targeting confusions

	psFeature->bTargetted = FALSE;
	psFeature->timeLastHit = 0;




	if(getRevealStatus())
	{
		vis = 0;
	}
	else
	{
		if(psStats->visibleAtStart)
		{
  			vis = UBYTE_MAX;
  			setFeatTileDraw(psFeature);
		}
		else
		{
			vis = 0;
		}
	}

	for(i=0; i<MAX_PLAYERS; i++)
	{
		psFeature->visible[i] = 0;//vis;
	}
	//load into the map data

	if(FromSave) {
		mapX = (x >> TILE_SHIFT) - (psStats->baseWidth/2);
		mapY = (y >> TILE_SHIFT) - (psStats->baseBreadth/2);
	} else {
		mapX = x >> TILE_SHIFT;
		mapY = y >> TILE_SHIFT;
	}

	// set up the imd for the feature
	if(psFeature->psStats->subType==FEAT_BUILD_WRECK)
	{

//		psFeature->sDisplay.imd = wreckageImds[rand()%MAX_WRECKAGE];
		psFeature->sDisplay.imd = getRandomWreckageImd();

	}
	else
	{
		psFeature->sDisplay.imd = psStats->psImd;
//DBPRINTF(("%d %d\n",psStats->psImd->ymin,psStats->psImd->ymax);
  	}


	assert(psFeature->sDisplay.imd);		// make sure we have an imd.

	for (width = 0; width <= psStats->baseWidth; width++)
	{
		for (breadth = 0; breadth <= psStats->baseBreadth; breadth++)
		{
			//check not outside of map - for load save game

			ASSERT( (mapX+width) < mapWidth,
				"x coord bigger than map width - %s, id = %d",
				getName(psFeature->psStats->pName), psFeature->id );
			ASSERT( (mapY+breadth) < mapHeight,
				"y coord bigger than map height - %s, id = %d",
				getName(psFeature->psStats->pName), psFeature->id );

			psTile = mapTile(mapX+width, mapY+breadth);
			if (width != psStats->baseWidth && breadth != psStats->baseBreadth)
			{
				ASSERT( !(TILE_HAS_FEATURE(mapTile(mapX+width,mapY+breadth))),
					"buildFeature - feature- %d already found at %d, %d",
					psFeature->id, mapX+width,mapY+breadth );

				SET_TILE_FEATURE(psTile);
				// if it's a tall feature then flag it in the map.
				if(psFeature->sDisplay.imd->ymax > TALLOBJECT_YMAX) {
//#ifdef PSX
//DBPRINTF(("Tall feature %d, (%d x %d)\n",psFeature->sDisplay.imd->ymax,psStats->baseWidth,psStats->baseBreadth));
//#endif
					SET_TILE_TALLSTRUCTURE(psTile);
				}

				if (psStats->subType == FEAT_GEN_ARTE OR psStats->subType == FEAT_OIL_DRUM OR psStats->subType == FEAT_BUILD_WRECK)// they're there - just can see me
				{
					SET_TILE_NOTBLOCKING(psTile);
				}
			}

			if( (!psStats->tileDraw) && (FromSave == FALSE) )
			{
				psTile->height = (UBYTE)(height / ELEVATION_SCALE);
				// This sets the gourad shading to give 'as the artist drew it' levels

//				psTile->illumination = ILLUMINATION_NONE;		// set the tile so that there is no illumination connecting to feature ... this value works on both psx & pc
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
//DBPRINTF(("%d %d\n",psStats->psImd->ymin,psStats->psImd->ymax);
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
	MAPTILE		*psTile;
	MESSAGE		*psMessage;
	iVector		pos;

//	iVector		dv;
//	UWORD		uwFlameCycles, uwFlameAnims, i;
//	UDWORD		x, y, udwFlameDelay;

	ASSERT( PTRVALID(psDel, sizeof(FEATURE)),
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
	mapX = (psDel->x - psDel->psStats->baseWidth * TILE_UNITS / 2) >> TILE_SHIFT;
	mapY = (psDel->y - psDel->psStats->baseBreadth * TILE_UNITS / 2) >> TILE_SHIFT;
	for (width = 0; width < psDel->psStats->baseWidth; width++)
	{
		for (breadth = 0; breadth < psDel->psStats->baseBreadth; breadth++)
		{
	 //psor		mapTile(mapX+width, mapY+breadth)->psObject = NULL;
			psTile = mapTile(mapX+width, mapY+breadth);
		  //	psTile->tileInfoBits = (UBYTE)(psTile->tileInfoBits & BITS_STRUCTURE_MASK);
			/* Don't need to worry about clearing structure bits - they should not be there! */
			SET_TILE_EMPTY(psTile);
			CLEAR_TILE_NODRAW(psTile);
			CLEAR_TILE_TALLSTRUCTURE(psTile);
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


	//once a feature of type FEAT_BUILDING is destroyed - it leaves a wrecked
	//struct FEATURE in its place - ?!
	if (psDel->psStats->subType == FEAT_BUILDING)
	{
		mapX = (psDel->x - psDel->psStats->baseWidth * TILE_UNITS / 2) >> TILE_SHIFT;
		mapY = (psDel->y - psDel->psStats->baseBreadth * TILE_UNITS / 2) >> TILE_SHIFT;
		/*for (width = 0; width < psDel->psStats->baseWidth; width++)
		{
			for (breadth = 0; breadth < psDel->psStats->baseBreadth; breadth++)
			{
				buildFeature((asFeatureStats + structFeature),
						(mapX+width) << TILE_SHIFT, (mapY+breadth) << TILE_SHIFT, FALSE);
			}
		}*/

//		buildFeature((asFeatureStats + structFeature), mapX << TILE_SHIFT,
//			mapY << TILE_SHIFT, FALSE);
	}

}

/* Remove a Feature and free it's memory */
void destroyFeature(FEATURE *psDel)
{
	UDWORD			widthScatter,breadthScatter,heightScatter, i, explosionSize;
	iVector			pos;
	UDWORD			width,breadth;
	UDWORD			mapX,mapY;
	MAPTILE			*psTile;
	UDWORD			texture;


	ASSERT( PTRVALID(psDel, sizeof(FEATURE)),
		"destroyFeature: invalid feature pointer\n" );

//---------------------------------------------------------------------------------------
 	/* Only add if visible and damageable*/
	if(psDel->visible[selectedPlayer] AND psDel->psStats->damageable)
	{
		/* Set off a destruction effect */
		/* First Explosions */
		widthScatter = TILE_UNITS/2;
		breadthScatter = TILE_UNITS/2;
		heightScatter = TILE_UNITS/4;
		//set which explosion to use based on size of feature
		if (psDel->psStats->baseWidth < 2 AND psDel->psStats->baseBreadth < 2)
		{
			explosionSize = EXPLOSION_TYPE_SMALL;
		}
		else if (psDel->psStats->baseWidth < 3 AND psDel->psStats->baseBreadth < 3)
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
			mapX = (psDel->x - psDel->psStats->baseWidth * TILE_UNITS / 2) >> TILE_SHIFT;
			mapY = (psDel->y - psDel->psStats->baseBreadth * TILE_UNITS / 2) >> TILE_SHIFT;
//			if(psDel->sDisplay.imd->ymax>300)
		if(psDel->psStats->subType == FEAT_SKYSCRAPER)
			{
				for (width = 0; width < psDel->psStats->baseWidth; width++)
				{
					for (breadth = 0; breadth < psDel->psStats->baseBreadth; breadth++)
					{
						psTile = mapTile(mapX+width,mapY+breadth);
						// stops water texture chnaging for underwateer festures
					 	if(TERRAIN_TYPE(psTile) != TER_WATER)
						{
							if(TERRAIN_TYPE(psTile) != TER_CLIFFFACE)
							{
						   		/* Clear feature bits */
								SET_TILE_EMPTY(mapTile(mapX+width,mapY+breadth));
								texture = (psTile->texture & (~TILE_NUMMASK));
								texture |= DRIVE_OVER_RUBBLE_TILE;//getRubbleTileNum();
								psTile->texture = (UWORD)texture;
							}
							else
							{
							   /* This remains a blocking tile */
								SET_TILE_EMPTY(mapTile(mapX+width,mapY+breadth));
								texture = (psTile->texture & (~TILE_NUMMASK));
								texture |= NO_DRIVE_OVER_RUBBLE_TILE;//getRubbleTileNum();
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
#if defined(PSX) && defined(LIBPAD)
		if(EnableVibration) {
			SetVibro1(0,120,512);
		}
#endif
	}
//---------------------------------------------------------------------------------------

	removeFeature(psDel);
}

SDWORD getFeatureStatFromName( STRING *pName )
{
	UDWORD			inc;
	FEATURE_STATS	*psStat;

#ifdef HASH_NAMES
	UDWORD		HashedName=HashString(pName);
#endif

#ifdef RESOURCE_NAMES

	if (!getResourceName(pName))
	{
		return -1;
	}

#endif

	for (inc = 0; inc < numFeatureStats; inc++)
	{
		psStat = &asFeatureStats[inc];
#ifdef HASH_NAMES
		if (psStat->NameHash==HashedName)
#else
		if (!strcmp(psStat->pName, pName))
#endif
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

	startX = psDroid->x >> TILE_SHIFT;
	startY = psDroid->y >> TILE_SHIFT;

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
				if(psFeature AND psFeature->psStats->subType == FEAT_BUILD_WRECK)
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
				if(psFeature AND psFeature->psStats->subType == FEAT_BUILD_WRECK)
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
				if(psFeature AND psFeature->psStats->subType == FEAT_BUILD_WRECK)
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
				if(psFeature AND psFeature->psStats->subType == FEAT_BUILD_WRECK)
				{
					return psFeature;
				}
			}
		}
	}
	return NULL;
}
