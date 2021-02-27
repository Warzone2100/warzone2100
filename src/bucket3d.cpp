/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
 * @file bucket3d.c
 *
 * Stores object render calls in a linked list renders after bucket sorting objects.
 */

#include "lib/framework/frame.h"
#include "lib/framework/vector.h"
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_opengl/pieclip.h"

#include "atmos.h"
#include "bucket3d.h"
#include "component.h"
#include "display3d.h"
#include "effects.h"
#include "miscimd.h"

#include <algorithm>

#define CLIP_LEFT	((SDWORD)0)
#define CLIP_RIGHT	((SDWORD)pie_GetVideoBufferWidth())
#define CLIP_TOP	((SDWORD)0)
#define CLIP_BOTTOM ((SDWORD)pie_GetVideoBufferHeight())
// Gerard - HACK Multiplied by 7 to fix clipping
// someone needs to take a good look at the radius calculation
#define SCALE_DEPTH (FP12_MULTIPLIER*7)

struct BUCKET_TAG
{
	bool operator <(BUCKET_TAG const &b) const
	{
		return actualZ > b.actualZ;    // Sort in reverse z order.
	}

	RENDER_TYPE     objectType; //type of object held
	void           *pObject;    //pointer to the object
	int32_t         actualZ;
};

static std::vector<BUCKET_TAG> bucketArray;

static SDWORD bucketCalculateZ(RENDER_TYPE objectType, void *pObject, const glm::mat4 &viewMatrix)
{
	SDWORD				z = 0, radius;
	Vector2i				pixel(0, 0);
	Vector3i				position(0, 0, 0);
	UDWORD				droidSize;
	DROID				*psDroid;
	BODY_STATS			*psBStats;
	SIMPLE_OBJECT		*psSimpObj;
	const iIMDShape		*pImd;
	Spacetime               spacetime;

	switch (objectType)
	{
	case RENDER_PARTICLE:
		position.x = ((ATPART *)pObject)->position.x;
		position.y = ((ATPART *)pObject)->position.y;
		position.z = ((ATPART *)pObject)->position.z;

		position.x = position.x - playerPos.p.x;
		position.z = -(position.z - playerPos.p.z);

		/* 16 below is HACK!!! */
		z = pie_RotateProject(&position, viewMatrix, &pixel) - 16;
		if (z > 0)
		{
			//particle use the image radius
			radius = ((ATPART *)pObject)->imd->radius;
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
		if (((PROJECTILE *)pObject)->psWStats->weaponSubClass == WSC_FLAME ||
		    ((PROJECTILE *)pObject)->psWStats->weaponSubClass == WSC_COMMAND ||
		    ((PROJECTILE *)pObject)->psWStats->weaponSubClass == WSC_EMP)
		{
			/* We don't do projectiles from these guys, cos there's an effect instead */
			z = -1;
		}
		else
		{

			//the weapon stats holds the reference to which graphic to use
			pImd = ((PROJECTILE *)pObject)->psWStats->pInFlightGraphic;

			psSimpObj = (SIMPLE_OBJECT *) pObject;
			position.x = psSimpObj->pos.x - playerPos.p.x;
			position.z = -(psSimpObj->pos.y - playerPos.p.z);

			position.y = psSimpObj->pos.z;

			z = pie_RotateProject(&position, viewMatrix, &pixel);

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
		psSimpObj = (SIMPLE_OBJECT *) pObject;
		position.x = psSimpObj->pos.x - playerPos.p.x;
		position.z = -(psSimpObj->pos.y - playerPos.p.z);

		if ((objectType == RENDER_STRUCTURE) &&
		    ((((STRUCTURE *)pObject)->pStructureType->type == REF_DEFENSE) ||
		     (((STRUCTURE *)pObject)->pStructureType->type == REF_WALL) ||
		     (((STRUCTURE *)pObject)->pStructureType->type == REF_WALLCORNER)))
		{
			position.y = psSimpObj->pos.z + 64;
			radius = ((STRUCTURE *)pObject)->sDisplay.imd->radius; //walls guntowers and tank traps clip tightly
		}
		else
		{
			position.y = psSimpObj->pos.z;
			radius = (((STRUCTURE *)pObject)->sDisplay.imd->radius);
		}

		z = pie_RotateProject(&position, viewMatrix, &pixel);

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
		psSimpObj = (SIMPLE_OBJECT *) pObject;
		position.x = psSimpObj->pos.x - playerPos.p.x;
		position.z = -(psSimpObj->pos.y - playerPos.p.z);

		position.y = psSimpObj->pos.z + 2;

		z = pie_RotateProject(&position, viewMatrix, &pixel);

		if (z > 0)
		{
			//particle use the image radius
			radius = ((FEATURE *)pObject)->sDisplay.imd->radius;
			radius *= SCALE_DEPTH;
			radius /= z;
			if ((pixel.x + radius < CLIP_LEFT) || (pixel.x - radius > CLIP_RIGHT)
			    || (pixel.y + radius < CLIP_TOP) || (pixel.y - radius > CLIP_BOTTOM))
			{
				z = -1;
			}
		}
		break;
	case RENDER_DROID:
		psDroid = (DROID *) pObject;

		psSimpObj = (SIMPLE_OBJECT *) pObject;
		position.x = psSimpObj->pos.x - playerPos.p.x;
		position.z = -(psSimpObj->pos.y - playerPos.p.z);
		position.y = psSimpObj->pos.z;

		psBStats = asBodyStats + psDroid->asBits[COMP_BODY];
		droidSize = psBStats->pIMD->radius;
		z = pie_RotateProject(&position, viewMatrix, &pixel) - (droidSize * 2);

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
		if (((PROXIMITY_DISPLAY *)pObject)->type == POS_PROXDATA)
		{
			const PROXIMITY_DISPLAY *ptr = (PROXIMITY_DISPLAY *)pObject;
			position.x = ((VIEW_PROXIMITY *)ptr->psMessage->pViewData->pData)->x - playerPos.p.x;
#if defined( _MSC_VER )
	#pragma warning( push )
	#pragma warning( disable : 4146 ) // warning C4146: unary minus operator applied to unsigned type, result still unsigned
#endif
			position.z = -(((VIEW_PROXIMITY *)ptr->psMessage->pViewData->pData)->y - playerPos.p.z);
#if defined( _MSC_VER )
	#pragma warning( pop )
#endif
			position.y = ((VIEW_PROXIMITY *)ptr->psMessage->pViewData->pData)->z;
		}
		else if (((PROXIMITY_DISPLAY *)pObject)->type == POS_PROXOBJ)
		{
			const PROXIMITY_DISPLAY *ptr = (PROXIMITY_DISPLAY *)pObject;
			position.x = ptr->psMessage->psObj->pos.x - playerPos.p.x;
			position.z = -(ptr->psMessage->psObj->pos.y - playerPos.p.z);
			position.y = ptr->psMessage->psObj->pos.z;
		}
		z = pie_RotateProject(&position, viewMatrix, &pixel);

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
		position.x = ((EFFECT *)pObject)->position.x - playerPos.p.x;
		position.z = -(((EFFECT *)pObject)->position.z - playerPos.p.z);
		position.y = ((EFFECT *)pObject)->position.y;

		/* 16 below is HACK!!! */
		z = pie_RotateProject(&position, viewMatrix, &pixel) - 16;

		if (z > 0)
		{
			//particle use the image radius
			pImd = ((EFFECT *)pObject)->imd;
			if (pImd != nullptr)
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
		position.x = ((FLAG_POSITION *)pObject)->coords.x - playerPos.p.x;
		position.z = -(((FLAG_POSITION *)pObject)->
		               coords.y - playerPos.p.z);
		position.y = ((FLAG_POSITION *)pObject)->coords.z;

		z = pie_RotateProject(&position, viewMatrix, &pixel);

		if (z > 0)
		{
			//particle use the image radius
			radius = pAssemblyPointIMDs[((FLAG_POSITION *)pObject)->factoryType][((FLAG_POSITION *)pObject)->factoryInc]->radius;
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

	return z;
}

/* add an object to the current render list */
void bucketAddTypeToList(RENDER_TYPE objectType, void *pObject, const glm::mat4 &viewMatrix)
{
	const iIMDShape *pie;
	BUCKET_TAG	newTag;
	int32_t		z = bucketCalculateZ(objectType, pObject, viewMatrix);

	if (z < 0)
	{
		/* Object will not be render - has been clipped! */
		if (objectType == RENDER_DROID || objectType == RENDER_STRUCTURE)
		{
			/* Won't draw selection boxes */
			((BASE_OBJECT *)pObject)->sDisplay.frameNumber = 0;
		}

		return;
	}

	switch (objectType)
	{
	case RENDER_EFFECT:
		switch (((EFFECT *)pObject)->group)
		{
		case EFFECT_EXPLOSION:
		case EFFECT_CONSTRUCTION:
		case EFFECT_SMOKE:
		case EFFECT_FIREWORK:
			// Use calculated Z
			break;

		case EFFECT_WAYPOINT:
			pie = ((EFFECT *)pObject)->imd;
			z = INT32_MAX - pie->texpage;
			break;

		default:
			z = INT32_MAX - 42;
			break;
		}
		break;
	case RENDER_DROID:
		pie = BODY_IMD(((DROID *)pObject), 0);
		z = INT32_MAX - pie->texpage;
		break;
	case RENDER_STRUCTURE:
		pie = ((STRUCTURE *)pObject)->sDisplay.imd;
		z = INT32_MAX - pie->texpage;
		break;
	case RENDER_FEATURE:
		pie = ((FEATURE *)pObject)->sDisplay.imd;
		z = INT32_MAX - pie->texpage;
		break;
	case RENDER_DELIVPOINT:
		pie = pAssemblyPointIMDs[((FLAG_POSITION *)pObject)->
		                         factoryType][((FLAG_POSITION *)pObject)->factoryInc];
		z = INT32_MAX - pie->texpage;
		break;
	case RENDER_PARTICLE:
		z = 0;
		break;
	default:
		// Use calculated Z
		break;
	}

	//put the object data into the tag
	newTag.objectType = objectType;
	newTag.pObject = pObject;
	newTag.actualZ = z;

	//add tag to bucketArray
	bucketArray.push_back(newTag);
}

/* render Objects in list */
void bucketRenderCurrentList(const glm::mat4 &viewMatrix)
{
	std::sort(bucketArray.begin(), bucketArray.end());

	for (std::vector<BUCKET_TAG>::const_iterator thisTag = bucketArray.begin(); thisTag != bucketArray.end(); ++thisTag)
	{
		switch (thisTag->objectType)
		{
		case RENDER_PARTICLE:
			renderParticle((ATPART *)thisTag->pObject, viewMatrix);
			break;
		case RENDER_EFFECT:
			renderEffect((EFFECT *)thisTag->pObject, viewMatrix);
			break;
		case RENDER_DROID:
			displayComponentObject((DROID *)thisTag->pObject, viewMatrix);
			break;
		case RENDER_STRUCTURE:
			renderStructure((STRUCTURE *)thisTag->pObject, viewMatrix);
			break;
		case RENDER_FEATURE:
			renderFeature((FEATURE *)thisTag->pObject, viewMatrix);
			break;
		case RENDER_PROXMSG:
			renderProximityMsg((PROXIMITY_DISPLAY *)thisTag->pObject, viewMatrix);
			break;
		case RENDER_PROJECTILE:
			renderProjectile((PROJECTILE *)thisTag->pObject, viewMatrix);
			break;
		case RENDER_DELIVPOINT:
			renderDeliveryPoint((FLAG_POSITION *)thisTag->pObject, false, viewMatrix);
			break;
		}
	}

	//reset the bucket array as we go
	bucketArray.resize(0);
}
