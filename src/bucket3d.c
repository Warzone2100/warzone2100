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
/**
 * @file bucket3D.c
 *
 * Stores object render calls in a linked list renders after bucket sorting objects.
 */

/* Includes direct access to matrix code */
#include "lib/ivis_common/piedef.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_opengl/piematrix.h"
/* Includes from PUMPKIN stuff */
#include "lib/framework/frame.h"
#include "objectdef.h"
#include "map.h"
#include "display3d.h"
#include "miscimd.h"
#include "effects.h"
#include "component.h"
#include "bucket3d.h"
#include "message.h"
#include "console.h"
#include "atmos.h"

#define NUM_BUCKETS		1000
#define NUM_OBJECTS		4000
#define BUCKET_OFFSET	0
#define BUCKET_RANGE	32000

#define CLIP_LEFT	((SDWORD)0)
#define CLIP_RIGHT	((SDWORD)pie_GetVideoBufferWidth())
#define CLIP_TOP	((SDWORD)0)
#define CLIP_BOTTOM ((SDWORD)pie_GetVideoBufferHeight())
//scale depth = 1<<FP12_SHIFT>>STRETCHED_Z_SHIFT<<xpshift
// Gerard - HACK Multiplied by 7 to fix clipping
// someone needs to take a good look at the radius calculation
#define SCALE_DEPTH (FP12_MULTIPLIER*7)

typedef struct _tile_bucket
{
	UDWORD	i;
	UDWORD	j;
	SDWORD	depth;
}
TILE_BUCKET;

typedef struct _bucket_tag
{
	struct _bucket_tag* psNextTag;
	RENDER_TYPE			objectType; //type of object held
	void*				pObject; //pointer to the object
	SDWORD				actualZ;
} BUCKET_TAG;

static BUCKET_TAG tagResource[NUM_OBJECTS];
static BUCKET_TAG* bucketArray[NUM_BUCKETS];
static UDWORD resourceCounter;
static SDWORD zMax;
static SDWORD zMin;
static SDWORD worldMax,worldMin;

/* function prototypes */
static SDWORD bucketCalculateZ(RENDER_TYPE objectType, void* pObject);
static SDWORD bucketCalculateState(RENDER_TYPE objectType, void* pObject);

/* reset object list */
BOOL bucketSetupList(void)
{
	UDWORD i;

	zMax = SDWORD_MIN;
	zMin = SDWORD_MAX;
	worldMax = SDWORD_MIN;
	worldMin = SDWORD_MAX;
	//reset resource
	resourceCounter = 0;
	//reset buckets
	for (i = 0; i < NUM_BUCKETS; i++)
	{
		bucketArray[i] = NULL;
	}
	return true;
}

/* add an object to the current render list */
extern BOOL bucketAddTypeToList(RENDER_TYPE objectType, void* pObject)
{
	BUCKET_TAG* newTag;
	SDWORD z;

	//get next Tag
	newTag = &tagResource[resourceCounter];
	if(resourceCounter>=NUM_OBJECTS)
	{
		debug(LOG_NEVER, "bucket sort too many objects");
		/* Just get out if there's too much to render already...! */
		return(true);
	}
	resourceCounter++;
	ASSERT( resourceCounter <= NUM_OBJECTS, "bucketAddTypeToList: too many objects" );

	//put the object data into the tag
	newTag->objectType = objectType;
	newTag->pObject = pObject;
	{
		if ((objectType == RENDER_EFFECT) && ((((EFFECT*)pObject)->group == EFFECT_EXPLOSION) ||
			(((EFFECT*)pObject)->group == EFFECT_CONSTRUCTION) ||
			(((EFFECT*)pObject)->group == EFFECT_SMOKE) ||
			(((EFFECT*)pObject)->group == EFFECT_FIREWORK)))
		{

			z = bucketCalculateZ(objectType, pObject);
		}
		else if(objectType == RENDER_SHADOW)
		{
			z = bucketCalculateZ(objectType, pObject);
		}
		else if(objectType == RENDER_PROJECTILE)
		{
			z = bucketCalculateZ(objectType, pObject);
		}
		else if(objectType == RENDER_PROXMSG)
		{
			z = bucketCalculateZ(objectType, pObject);
		}
		else
		{
			z = bucketCalculateState(objectType, pObject);
		}
	}

	if (z < 0)
	{
		/* Object will not be render - has been clipped! */
		if(objectType == RENDER_DROID)
		{
			/* Won't draw selection boxes */
			DROID *psDroid = (DROID*)pObject;

			psDroid->sDisplay.frameNumber = 0;
		}
		else if(objectType == RENDER_STRUCTURE)
		{
			/* Won't draw selection boxes */
			STRUCTURE *psStructure = (STRUCTURE*)pObject;

			psStructure->sDisplay.frameNumber = 0;
		}

		return true;
	}

	/* Maintain biggest*/
	if(z>worldMax)
	{
		worldMax = z;
	}
	else if(z<worldMin)
	{
		worldMin = z;
	}

	/* get min and max */
	if (z > zMax)
	{
		zMax = z;
	}
	else if (z < zMin)
	{
		zMin = z;
	}

	z = (z * NUM_BUCKETS)/BUCKET_RANGE;

	if (z >= NUM_BUCKETS)
	{
		z = NUM_BUCKETS - 1;
	}

	//add tag to bucketArray
	newTag->psNextTag = bucketArray[z];
	newTag->actualZ = z;
	bucketArray[z] = newTag;

	return true;
}


/* render Objects in list */
extern BOOL bucketRenderCurrentList(void)
{
	SDWORD z;
	BUCKET_TAG* thisTag;

	for (z = NUM_BUCKETS - 1; z >= 0; z--) // render from back to front
	{
		thisTag = bucketArray[z];
		while(thisTag != NULL)
		{
			switch(thisTag->objectType)
			{
				case RENDER_PARTICLE:
	  				renderParticle((ATPART*)thisTag->pObject);
				break;
				case RENDER_EFFECT:
					renderEffect((EFFECT*)thisTag->pObject);
					break;
				case RENDER_DROID:
					renderDroid((DROID*)thisTag->pObject);
				break;
				case RENDER_SHADOW:
					renderShadow((DROID*)thisTag->pObject,getImdFromIndex(MI_SHADOW));
				break;
				case RENDER_STRUCTURE:
					renderStructure((STRUCTURE*)thisTag->pObject);
				break;
				case RENDER_FEATURE:
					renderFeature((FEATURE*)thisTag->pObject);
				break;
				case RENDER_PROXMSG:
					renderProximityMsg((PROXIMITY_DISPLAY*)thisTag->pObject);
				break;
				case RENDER_PROJECTILE:
					renderProjectile((PROJECTILE*)thisTag->pObject);
				break;
				case RENDER_ANIMATION:
					renderAnimComponent((COMPONENT_OBJECT*)thisTag->pObject);
				break;
				case RENDER_DELIVPOINT:
					renderDeliveryPoint((FLAG_POSITION*)thisTag->pObject);
				break;
			}
			thisTag = thisTag->psNextTag;
		}
		//reset the bucket array as we go
		bucketArray[z] = NULL;
	}

	//reset the tag array
	resourceCounter = 0;
	zMax = SDWORD_MIN;
	zMin = SDWORD_MAX;
	worldMax = SDWORD_MIN;
	worldMin = SDWORD_MAX;
	return true;
}

static SDWORD bucketCalculateZ(RENDER_TYPE objectType, void* pObject)
{
	SDWORD				z = 0, radius;
	SDWORD				px, pz;
	Vector2i				pixel;
	Vector3i				position;
	UDWORD				droidSize;
	DROID				*psDroid;
	BODY_STATS			*psBStats;
	SIMPLE_OBJECT		*psSimpObj;
	COMPONENT_OBJECT	*psCompObj;
	iIMDShape			*pImd;

   	iV_MatrixBegin();

	switch(objectType)
	{
		case RENDER_PARTICLE:
	   		px = player.p.x & (TILE_UNITS-1);
	   		pz = player.p.z & (TILE_UNITS-1);

	   		/* Translate */
   			iV_TRANSLATE(px,0,-pz);

			position.x = ((ATPART*)pObject)->position.x;
			position.y = ((ATPART*)pObject)->position.y;
			position.z = ((ATPART*)pObject)->position.z;

   			position.x = (SDWORD)(position.x - player.p.x) - terrainMidX*TILE_UNITS;
   			position.z = (SDWORD)(terrainMidY*TILE_UNITS - (position.z - player.p.z));
 			position.y = (SDWORD)position.y;

			/* 16 below is HACK!!! */
			z = pie_RotateProject(&position,&pixel) - 16;
			if (z > 0)
			{
				//particle use the image radius
				radius = ((ATPART*)pObject)->imd->radius;
				radius *= SCALE_DEPTH;
				radius /= z;
				if ((pixel.x + radius < CLIP_LEFT) || (pixel.x - radius > CLIP_RIGHT)
					|| (pixel.y + radius < CLIP_TOP) || (pixel.y - radius > CLIP_BOTTOM))
				{
					z = -1;
				}
			}
			break;
		case RENDER_PROJECTILE:
			if(((PROJECTILE*)pObject)->psWStats->weaponSubClass == WSC_FLAME ||
                ((PROJECTILE*)pObject)->psWStats->weaponSubClass == WSC_COMMAND ||
                ((PROJECTILE*)pObject)->psWStats->weaponSubClass == WSC_EMP)
			{
				/* We don't do projectiles from these guys, cos there's an effect instead */
				z = -1;
			}
			else
			{

				//the weapon stats holds the reference to which graphic to use
				pImd = ((PROJECTILE*)pObject)->psWStats->pInFlightGraphic;

	   			px = player.p.x & (TILE_UNITS-1);
	   			pz = player.p.z & (TILE_UNITS-1);

	   			/* Translate */
   				iV_TRANSLATE(px,0,-pz);

				psSimpObj = (SIMPLE_OBJECT*) pObject;
   				position.x = (psSimpObj->pos.x - player.p.x) - terrainMidX*TILE_UNITS;
   				position.z = terrainMidY*TILE_UNITS - (psSimpObj->pos.y - player.p.z);

				position.y = psSimpObj->pos.z;

				z = pie_RotateProject(&position,&pixel);

				if (z > 0)
				{
					//particle use the image radius
					radius = pImd->radius;
					radius *= SCALE_DEPTH;
					radius /= z;
					if ((pixel.x + radius < CLIP_LEFT) || (pixel.x - radius > CLIP_RIGHT)
						|| (pixel.y + radius < CLIP_TOP) || (pixel.y - radius > CLIP_BOTTOM))
					{
						z = -1;
					}
				}
			}
			break;
		case RENDER_STRUCTURE://not depth sorted
	   		px = player.p.x & (TILE_UNITS-1);
	   		pz = player.p.z & (TILE_UNITS-1);

	   		/* Translate */
   			iV_TRANSLATE(px,0,-pz);

			psSimpObj = (SIMPLE_OBJECT*) pObject;
   			position.x = (psSimpObj->pos.x - player.p.x) - terrainMidX*TILE_UNITS;
   			position.z = terrainMidY*TILE_UNITS - (psSimpObj->pos.y - player.p.z);

			//if((objectType == RENDER_STRUCTURE) && (((STRUCTURE*)pObject)->
			//	pStructureType->type >= REF_DEFENSE) &&
			//	(((STRUCTURE*)pObject)->pStructureType->type<=REF_TOWER4))
			if((objectType == RENDER_STRUCTURE) &&
				((((STRUCTURE*)pObject)->pStructureType->type == REF_DEFENSE) ||
				 (((STRUCTURE*)pObject)->pStructureType->type == REF_WALL) ||
				 (((STRUCTURE*)pObject)->pStructureType->type == REF_WALLCORNER)))
			{
				position.y = psSimpObj->pos.z + 64;
				radius = ((STRUCTURE*)pObject)->sDisplay.imd->radius;//walls guntowers and tank traps clip tightly
			}
			else
			{
				position.y = psSimpObj->pos.z;
				radius = (((STRUCTURE*)pObject)->sDisplay.imd->radius);
			}

			z = pie_RotateProject(&position,&pixel);

			if (z > 0)
			{
				//particle use the image radius
				radius *= SCALE_DEPTH;
				radius /= z;
				if ((pixel.x + radius < CLIP_LEFT) || (pixel.x - radius > CLIP_RIGHT)
					|| (pixel.y + radius < CLIP_TOP) || (pixel.y - radius > CLIP_BOTTOM))
				{
					z = -1;
				}
			}
			break;
		case RENDER_FEATURE://not depth sorted
	   		px = player.p.x & (TILE_UNITS-1);
	   		pz = player.p.z & (TILE_UNITS-1);

	   		/* Translate */
   			iV_TRANSLATE(px,0,-pz);

			psSimpObj = (SIMPLE_OBJECT*) pObject;
   			position.x = (psSimpObj->pos.x - player.p.x) - terrainMidX*TILE_UNITS;
   			position.z = terrainMidY*TILE_UNITS - (psSimpObj->pos.y - player.p.z);

			position.y = psSimpObj->pos.z+2;

			z = pie_RotateProject(&position,&pixel);

			if (z > 0)
			{
				//particle use the image radius
				radius = ((FEATURE*)pObject)->sDisplay.imd->radius;
				radius *= SCALE_DEPTH;
				radius /= z;
				if ((pixel.x + radius < CLIP_LEFT) || (pixel.x - radius > CLIP_RIGHT)
					|| (pixel.y + radius < CLIP_TOP) || (pixel.y - radius > CLIP_BOTTOM))
				{
					z = -1;
				}
			}
			break;
		case RENDER_ANIMATION://not depth sorted
	   		px = player.p.x & (TILE_UNITS-1);
	   		pz = player.p.z & (TILE_UNITS-1);

	   		/* Translate */
   			iV_TRANSLATE(px,0,-pz);

			psCompObj = (COMPONENT_OBJECT *) pObject;
			psSimpObj = (SIMPLE_OBJECT *) psCompObj->psParent;
			position.x = (psSimpObj->pos.x - player.p.x) - terrainMidX*TILE_UNITS;
			position.z = terrainMidY*TILE_UNITS - (psSimpObj->pos.y - player.p.z);
			position.y = psSimpObj->pos.z;

			/* object offset translation */
			position.x += psCompObj->psShape->ocen.x;
			position.y += psCompObj->psShape->ocen.z;
			position.z -= psCompObj->psShape->ocen.y;

			/* object (animation) translations - ivis z and y flipped */
			iV_TRANSLATE( psCompObj->position.x, psCompObj->position.z,
							psCompObj->position.y );

			/* object (animation) rotations */
			iV_MatrixRotateY( -psCompObj->orientation.z );
			iV_MatrixRotateZ( -psCompObj->orientation.y );
			iV_MatrixRotateX( -psCompObj->orientation.x );

			z = pie_RotateProject(&position,&pixel);

			break;
		case RENDER_DROID:
		case RENDER_SHADOW:
			psDroid = (DROID*) pObject;
	   		px = player.p.x & (TILE_UNITS-1);
	   		pz = player.p.z & (TILE_UNITS-1);

	   		/* Translate */
   			iV_TRANSLATE(px,0,-pz);

			psSimpObj = (SIMPLE_OBJECT*) pObject;
   			position.x = (psSimpObj->pos.x - player.p.x) - terrainMidX*TILE_UNITS;
   			position.z = terrainMidY*TILE_UNITS - (psSimpObj->pos.y - player.p.z);
 			position.y = psSimpObj->pos.z;
			if(objectType == RENDER_SHADOW)
			{
				position.y+=4;
			}

			psBStats = asBodyStats + psDroid->asBits[COMP_BODY].nStat;
			droidSize = psBStats->pIMD->radius;
			z = pie_RotateProject(&position,&pixel) - (droidSize*2);

			if (z > 0)
			{
				//particle use the image radius
				radius = droidSize;
				radius *= SCALE_DEPTH;
				radius /= z;
				if ((pixel.x + radius < CLIP_LEFT) || (pixel.x - radius > CLIP_RIGHT)
					|| (pixel.y + radius < CLIP_TOP) || (pixel.y - radius > CLIP_BOTTOM))
				{
					z = -1;
				}
			}
			break;
		case RENDER_PROXMSG:
	   		px = player.p.x & (TILE_UNITS-1);
	   		pz = player.p.z & (TILE_UNITS-1);

	   		/* Translate */
   			iV_TRANSLATE(px,0,-pz);
			if (((PROXIMITY_DISPLAY *)pObject)->type == POS_PROXDATA)
			{
				position.x = (((VIEW_PROXIMITY *)((VIEWDATA *)((PROXIMITY_DISPLAY *)
					pObject)->psMessage->pViewData)->pData)->x - player.p.x) -
					terrainMidX * TILE_UNITS;
   				position.z = terrainMidY * TILE_UNITS - (((VIEW_PROXIMITY *)((VIEWDATA *)
					((PROXIMITY_DISPLAY *)pObject)->psMessage->pViewData)->pData)->y -
					player.p.z);
 				position.y = ((VIEW_PROXIMITY *)((VIEWDATA *)((PROXIMITY_DISPLAY *)pObject)->
					psMessage->pViewData)->pData)->z;
			}
			else if (((PROXIMITY_DISPLAY *)pObject)->type == POS_PROXOBJ)
			{
				position.x = (((BASE_OBJECT *)((PROXIMITY_DISPLAY *)pObject)->
					psMessage->pViewData)->pos.x - player.p.x) - terrainMidX *
					TILE_UNITS;
   				position.z = terrainMidY * TILE_UNITS - (((BASE_OBJECT *)((
					PROXIMITY_DISPLAY *)pObject)->psMessage->pViewData)->pos.y -
					player.p.z);
 				position.y = ((BASE_OBJECT *)((PROXIMITY_DISPLAY *)pObject)->
					psMessage->pViewData)->pos.z;
			}
			z = pie_RotateProject(&position,&pixel);

			if (z > 0)
			{
				//particle use the image radius
				pImd = getImdFromIndex(MI_BLIP_ENEMY);//use MI_BLIP_ENEMY as all are same radius
				radius = pImd->radius;
				radius *= SCALE_DEPTH;
				radius /= z;
				if ((pixel.x + radius < CLIP_LEFT) || (pixel.x - radius > CLIP_RIGHT)
					|| (pixel.y + radius < CLIP_TOP) || (pixel.y - radius > CLIP_BOTTOM))
				{
					z = -1;
				}
			}
			break;
		case RENDER_EFFECT:
	   		px = player.p.x & (TILE_UNITS-1);
	   		pz = player.p.z & (TILE_UNITS-1);

	   		/* Translate */
   			iV_TRANSLATE(px,0,-pz);

   			position.x = (SDWORD)(((EFFECT*)pObject)->position.x - player.p.x) - terrainMidX*TILE_UNITS;
   			position.z = (SDWORD)(terrainMidY*TILE_UNITS - (((EFFECT*)pObject)->position.z - player.p.z));
 			position.y = (SDWORD)((EFFECT*)pObject)->position.y;

			/* 16 below is HACK!!! */
			z = pie_RotateProject(&position,&pixel) - 16;

			if (z > 0)
			{
				//particle use the image radius
				pImd = ((EFFECT*)pObject)->imd;
				if (pImd != NULL)
				{
					radius = pImd->radius;
					radius *= SCALE_DEPTH;
					radius /= z;
					if ((pixel.x + radius < CLIP_LEFT) || (pixel.x - radius > CLIP_RIGHT)
						|| (pixel.y + radius < CLIP_TOP) || (pixel.y - radius > CLIP_BOTTOM))
					{
						z = -1;
					}
				}
			}

			break;

		case RENDER_DELIVPOINT:
	   		px = player.p.x & (TILE_UNITS-1);
	   		pz = player.p.z & (TILE_UNITS-1);

	   		/* Translate */
   			iV_TRANSLATE(px,0,-pz);
			position.x = (((FLAG_POSITION *)pObject)->coords.x - player.p.x) -
				terrainMidX * TILE_UNITS;
   			position.z = terrainMidY*TILE_UNITS - (((FLAG_POSITION*)pObject)->
				coords.y - player.p.z);
 			position.y = ((FLAG_POSITION*)pObject)->coords.z;

			z = pie_RotateProject(&position,&pixel);

			if (z > 0)
			{
				//particle use the image radius
				radius = pAssemblyPointIMDs[((FLAG_POSITION*)pObject)->factoryType][((FLAG_POSITION*)pObject)->factoryInc]->radius;
				radius *= SCALE_DEPTH;
				radius /= z;
				if ((pixel.x + radius < CLIP_LEFT) || (pixel.x - radius > CLIP_RIGHT)
					|| (pixel.y + radius < CLIP_TOP) || (pixel.y - radius > CLIP_BOTTOM))
				{
					z = -1;
				}
			}

			break;

		default:
		break;
	}

   	pie_MatEnd();

	return z;
}

static SDWORD bucketCalculateState(RENDER_TYPE objectType, void* pObject)
{
	SDWORD				z = 0;
	iIMDShape*			pie;

	if (bucketCalculateZ(objectType,pObject) < 0)
	{
		return -1;
	}

	switch(objectType)
	{
		case RENDER_EFFECT:
			switch(((EFFECT*)pObject)->group)
			{
				case EFFECT_WAYPOINT:
				case EFFECT_EXPLOSION:
				case EFFECT_CONSTRUCTION:
					pie = ((EFFECT*)pObject)->imd;
					z = NUM_BUCKETS - pie->texpage;
				break;

				case EFFECT_SMOKE:
				case EFFECT_GRAVITON:
				case EFFECT_BLOOD:
				case EFFECT_STRUCTURE:
				case EFFECT_DESTRUCTION:
				default:
					z = NUM_BUCKETS - 42;
				break;
			}
		break;
		case RENDER_DROID:
			pie = BODY_IMD(((DROID*)pObject),0);
			z = NUM_BUCKETS - pie->texpage;
		break;
		case RENDER_STRUCTURE:
			pie = ((STRUCTURE*)pObject)->sDisplay.imd;
			z = NUM_BUCKETS - pie->texpage;
		break;
		case RENDER_FEATURE:
			pie = ((FEATURE*)pObject)->sDisplay.imd;
			z = NUM_BUCKETS - pie->texpage;
		break;
		case RENDER_PROXMSG:
			z = NUM_BUCKETS - 40;
		break;
		case RENDER_PROJECTILE:
			pie = ((PROJECTILE*)pObject)->psWStats->pInFlightGraphic;
			z = NUM_BUCKETS - pie->texpage;
		break;
		case RENDER_ANIMATION:
			pie = ((COMPONENT_OBJECT*)pObject)->psShape;
			z = NUM_BUCKETS - pie->texpage;
		break;
		case RENDER_DELIVPOINT:
			pie = pAssemblyPointIMDs[((FLAG_POSITION*)pObject)->
				factoryType][((FLAG_POSITION*)pObject)->factoryInc];
			z = NUM_BUCKETS - pie->texpage;
		break;
		default:
		break;
	}

	z *= (BUCKET_RANGE/NUM_BUCKETS);//stretch the dummy depth so its right when its compressed into the bucket array
	return z;
}
