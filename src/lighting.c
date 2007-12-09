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
/* Lighting.c - Alex McLean, Pumpkin Studios, EIDOS Interactive. */
/* Calculates the shading values for the terrain world. */
/* The terrain intensity values are calculated at map load/creation time. */

#include "lib/framework/frame.h"
#include <stdio.h>
#include <stdlib.h>
#include "lib/ivis_common/ivisdef.h" //ivis matrix code
#include "lib/ivis_common/piestate.h" //ivis matrix code
#include "lib/ivis_common/piefunc.h" //ivis matrix code
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_common/piepalette.h"
#include "map.h"
#include "lighting.h"
#include "display3d.h"
#include "effects.h"
#include "atmos.h"
#include "environ.h"
#include "lib/gamelib/gtime.h"
#include "console.h"

// These values determine the fog when fully zoomed in
// Determine these when fully zoomed in
#define FOG_END 3500
#define FOG_DEPTH 800

// These values are multiplied by the camera distance
// to obtain the optimal settings when fully zoomed out
// Determine these when fully zoomed out
#define FOG_BEGIN_SCALE 0.3
#define FOG_END_SCALE 0.6

/*	The vector that holds the sun's lighting direction - planar */
Vector3f	theSun;
UDWORD	fogStatus = 0;

/*	Module function Prototypes */
static void colourTile(SDWORD xIndex, SDWORD yIndex, LIGHT_COLOUR colour, UBYTE percent);
static void normalsOnTile(UDWORD tileX, UDWORD tileY, UDWORD quadrant);
static UDWORD calcDistToTile(UDWORD tileX, UDWORD tileY, Vector3i *pos);

static UDWORD	numNormals;		// How many normals have we got?
static Vector3i normals[8];		// Maximum 8 possible normals

extern void	draw3dLine(Vector3i *src, Vector3i *dest, UBYTE col);


/*****************************************************************************/
/*
 * SOURCE
 */
/*****************************************************************************/

//By passing in params - it means that if the scroll limits are changed mid-mission
//we can re-do over the area that hasn't been seen
void initLighting(UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2)
{
	UDWORD       i, j;
	MAPTILE	    *psTile;

	// quick check not trying to go off the map - don't need to check for < 0 since UWORD's!!
	if (x1 > mapWidth || x2 > mapWidth || y1 > mapHeight || y2 > mapHeight)
	{
		ASSERT( FALSE, "initLighting: coords off edge of map" );
		return;
	}

	for (i = x1; i < x2; i++)
	{
		for(j = y1; j < y2; j++)
		{
			psTile = mapTile(i, j);
			// always make the edge tiles dark
			if (i==0 || j==0 || i >= mapWidth-1 || j >= mapHeight-1)
			{
				psTile->illumination = 16;

				// give water tiles at edge of map a border
				if (terrainType(psTile) == TER_WATER)
				{
					psTile->texture = 0;
				}
			}
			else
			{
				calcTileIllum(i,j);
			}
			// Basically darkens down the tiles that are outside the scroll
			// limits - thereby emphasising the cannot-go-there-ness of them
			if ((SDWORD)i < scrollMinX + 4 || (SDWORD)i > scrollMaxX - 4
			    || (SDWORD)j < scrollMinY + 4 || (SDWORD)j > scrollMaxY - 4)
			{
				psTile->illumination/=3;
			}
		}
	}
}


void	calcTileIllum(UDWORD tileX, UDWORD tileY)
{
	Vector3i finalVector;
	SDWORD	dotProduct;
	UDWORD	i;
	UDWORD	val;

	numNormals = 0;
	/* Quadrants look like:-

				  *
				  *
			0	  *    1
				  *
				  *
		**********V**********
				  *
				  *
			3	  *	   2
				  *
				  *
	*/

	/* Do quadrant 0 - tile that's above and left*/
	normalsOnTile(tileX-1, tileY-1,0);

	/* Do quadrant 1 - tile that's above and right*/
	normalsOnTile(tileX,tileY-1,1);

	/* Do quadrant 2 - tile that's down and right*/
	normalsOnTile(tileX,tileY,2);

	/* Do quadrant 3 - tile that's down and left*/
	normalsOnTile(tileX-1,tileY,3);

	/* The number or normals that we got is in numNormals*/
	finalVector.x = finalVector.y = finalVector.z = 0;

	for(i=0; i<numNormals; i++)
	{
		finalVector.x += normals[i].x;
		finalVector.y += normals[i].y;
		finalVector.z += normals[i].z;
	}
	pie_VectorNormalise3iv(&finalVector);
	pie_VectorNormalise3fv(&theSun);

	dotProduct =	(finalVector.x * theSun.x +
			 finalVector.y * theSun.y +
			 finalVector.z * theSun.z) / FP12_MULTIPLIER;

	val = ((abs(dotProduct)) / 16);
	if (val == 0) val = 1;
	if(val > 254) val = 254;
	mapTile(tileX, tileY)->illumination = val;
}

static void normalsOnTile(UDWORD tileX, UDWORD tileY, UDWORD quadrant)
{
	Vector3i corner1, corner2, corner3;
	MAPTILE	*psTile, *tileRight, *tileDownRight, *tileDown;
	SDWORD	rMod,drMod,dMod,nMod;

	/* Get a pointer to our tile */
	psTile			= mapTile(tileX,tileY);

	/* And to the ones to the east, south and southeast of it */
	tileRight		= mapTile(tileX+1,tileY);
	tileDownRight	= mapTile(tileX+1,tileY+1);
	tileDown		= mapTile(tileX,tileY+1);

	if (terrainType(psTile) == TER_WATER)
	{
 		nMod = 100 + (2*environGetData(tileX,tileY));
		rMod = 100 + (2*environGetData(tileX+1,tileY));
		drMod = 100 + (2*environGetData(tileX+1,tileY+1));
		dMod = 100 + (2*environGetData(tileX,tileY+1));
	}
	else
	{
		nMod = rMod = drMod = dMod = 0;
	}


 	switch(quadrant)
	{

	case 0:
	case 2:
		/* Is it flipped? In this case one triangle  */
		if(TRI_FLIPPED(psTile))
		{
			if(quadrant==0)
			{
			 	corner1.x = world_coord(tileX + 1);
				corner1.y = world_coord(tileY);
				corner1.z = tileRight->height - rMod;

				corner2.x = world_coord(tileX + 1);
				corner2.y = world_coord(tileY + 1);
				corner2.z = tileDownRight->height - drMod;

				corner3.x = world_coord(tileX);
				corner3.y = world_coord(tileY + 1);
				corner3.z = tileDown->height - dMod;
				pie_SurfaceNormal3iv(&corner1,&corner2,&corner3,&normals[numNormals++]);
			}
			else
			{
				corner1.x = world_coord(tileX);
				corner1.y = world_coord(tileY);
				corner1.z = psTile->height - nMod;

				corner2.x = world_coord(tileX + 1);
				corner2.y = world_coord(tileY);
				corner2.z = tileRight->height - rMod;

				corner3.x = world_coord(tileX);
				corner3.y = world_coord(tileY + 1);
				corner3.z = tileDown->height - dMod;
				pie_SurfaceNormal3iv(&corner1,&corner2,&corner3,&normals[numNormals++]);
			}
		}
		else
		{
			/* Otherwise, it's not flipped and so two triangles*/
			corner1.x = world_coord(tileX);
			corner1.y = world_coord(tileY);
			corner1.z = psTile->height - nMod;

			corner2.x = world_coord(tileX + 1);
			corner2.y = world_coord(tileY);
			corner2.z = tileRight->height - rMod;

			corner3.x = world_coord(tileX + 1);
			corner3.y = world_coord(tileY + 1);
			corner3.z = tileDownRight->height - drMod;
			pie_SurfaceNormal3iv(&corner1,&corner2,&corner3,&normals[numNormals++]);

			corner1.x = world_coord(tileX);
			corner1.y = world_coord(tileY);
			corner1.z = psTile->height - nMod;

			corner2.x = world_coord(tileX + 1);
			corner2.y = world_coord(tileY + 1);
			corner2.z = tileDownRight->height - drMod;

			corner3.x = world_coord(tileX);
			corner3.y = world_coord(tileY + 1);
			corner3.z = tileDown->height - dMod;
			pie_SurfaceNormal3iv(&corner1,&corner2,&corner3,&normals[numNormals++]);
		}
		break;
	case 1:
	case 3:
		/* Is it flipped? In this case two triangles  */
		if(TRI_FLIPPED(psTile))
		{
	   	 	corner1.x = world_coord(tileX);
	   		corner1.y = world_coord(tileY);
	   		corner1.z = psTile->height - nMod;

	   		corner2.x = world_coord(tileX + 1);
	   		corner2.y = world_coord(tileY);
	   		corner2.z = tileRight->height - rMod;

	   		corner3.x = world_coord(tileX);
	   		corner3.y = world_coord(tileY + 1);
	   		corner3.z = tileDown->height - dMod;
			pie_SurfaceNormal3iv(&corner1,&corner2,&corner3,&normals[numNormals++]);

			corner1.x = world_coord(tileX + 1);
	   		corner1.y = world_coord(tileY);
	   		corner1.z = tileRight->height - rMod;

			corner2.x = world_coord(tileX + 1);
	   		corner2.y = world_coord(tileY + 1);
	   		corner2.z = tileDownRight->height - drMod;

	   		corner3.x = world_coord(tileX);
	   		corner3.y = world_coord(tileY + 1);
	   		corner3.z = tileDown->height - dMod;
	   		pie_SurfaceNormal3iv(&corner1,&corner2,&corner3,&normals[numNormals++]);
		}
		else
		{
			if(quadrant==1)
			{
			 	corner1.x = world_coord(tileX);
				corner1.y = world_coord(tileY);
				corner1.z = psTile->height - nMod;

				corner2.x = world_coord(tileX + 1);
				corner2.y = world_coord(tileY + 1);
				corner2.z = tileDownRight->height - drMod;

				corner3.x = world_coord(tileX);
				corner3.y = world_coord(tileY + 1);
				corner3.z = tileDown->height - dMod;
				pie_SurfaceNormal3iv(&corner1,&corner2,&corner3,&normals[numNormals++]);
			}
			else
			{
				corner1.x = world_coord(tileX);
				corner1.y = world_coord(tileY);
				corner1.z = psTile->height - nMod;

				corner2.x = world_coord(tileX + 1);
				corner2.y = world_coord(tileY);
				corner2.z = tileRight->height - rMod;

				corner3.x = world_coord(tileX + 1);
				corner3.y = world_coord(tileY + 1);
				corner3.z = tileDownRight->height - drMod;
				pie_SurfaceNormal3iv(&corner1,&corner2,&corner3,&normals[numNormals++]);
			}
		}
		break;
	default:
		ASSERT( FALSE,"Invalid quadrant in lighting code" );
	} // end switch
}

void	processLight(LIGHT *psLight)
{
SDWORD	tileX,tileY;
SDWORD	startX,endX;
SDWORD	startY,endY;
SDWORD	rangeSkip;
SDWORD	i,j;
SDWORD	distToCorner;
UDWORD	percent;

 	/* Firstly - there's no point processing lights that are off the grid */
	if(clipXY(psLight->position.x,psLight->position.z) == FALSE)
	{
		return;
	}

	tileX = psLight->position.x/TILE_UNITS;
	tileY = psLight->position.z/TILE_UNITS;

	rangeSkip = sqrtf(psLight->range * psLight->range * 2) / TILE_UNITS + 1;

	/* Rough guess? */
	startX = tileX - rangeSkip;
	endX = tileX + rangeSkip;
	startY = tileY - rangeSkip;
	endY = tileY + rangeSkip;

	/* Clip to grid limits */
	startX = MAX(startX, 0);
	endX = MAX(endX, 0);
	endX = MIN(endX, mapWidth - 1);
	startX = MIN(startX, endX);
	startY = MAX(startY, 0);
	endY = MAX(endY, 0);
	endY = MIN(endY, mapHeight - 1);
	startY = MIN(startY, endY);

	for(i=startX;i<=endX; i++)
	{
		for(j=startY; j<=endY; j++)
		{
			distToCorner = calcDistToTile(i,j,&psLight->position);

			/* If we're inside the range of the light */
			if (distToCorner<(SDWORD)psLight->range)
			{
				/* Find how close we are to it */
				percent = 100 - PERCENT(distToCorner,psLight->range);
				colourTile(i, j, psLight->colour, 2 * percent);
			}
		}
	}
}


static UDWORD calcDistToTile(UDWORD tileX, UDWORD tileY, Vector3i *pos)
{
	UDWORD	x1,y1,z1;
	UDWORD	x2,y2,z2;
	UDWORD	xDif,yDif,zDif;
	UDWORD	total;

	/* The coordinates of the tile corner */
	x1 = tileX * TILE_UNITS;
	y1 = map_TileHeight(tileX,tileY);
	z1 = tileY * TILE_UNITS;

	/* The coordinates of the position */
	x2 = pos->x;
	y2 = pos->y;
	z2 = pos->z;

	xDif = abs(x1-x2);
	yDif = abs(y1-y2);
	zDif = abs(z1-z2);

	total = (xDif*xDif) + (yDif*yDif) + (zDif*zDif);
	return (UDWORD)sqrtf(total);
}

// FIXME: Is the percent variable misnamed here, or is the code wrong? Because we do
// not use it as a percentage!
static void colourTile(SDWORD xIndex, SDWORD yIndex, LIGHT_COLOUR colour, UBYTE percent)
{
	MAPTILE *psTile = mapTile(xIndex, yIndex);

	switch(colour)
	{
 		case LIGHT_RED:
			/* And add that to the lighting value */
			psTile->colour.byte.r = MIN(255, psTile->colour.byte.r + percent);
		break;
 		case LIGHT_GREEN:
			/* And add that to the lighting value */
			psTile->colour.byte.g = MIN(255, psTile->colour.byte.g + percent);
		break;
 		case LIGHT_BLUE:
			/* And add that to the lighting value */
			psTile->colour.byte.b = MIN(255, psTile->colour.byte.b + percent);
		break;
		case LIGHT_YELLOW:
			/* And add that to the lighting value */
			psTile->colour.byte.r = MIN(255, psTile->colour.byte.r + percent);
			psTile->colour.byte.g = MIN(255, psTile->colour.byte.g + percent);
		break;
		case LIGHT_WHITE:
			psTile->colour.byte.r = MIN(255, psTile->colour.byte.r + percent);
			psTile->colour.byte.g = MIN(255, psTile->colour.byte.g + percent);
			psTile->colour.byte.b = MIN(255, psTile->colour.byte.b + percent);
		break;
		default:
			ASSERT( FALSE,"Weirdy colour of light attempted" );
			break;
	}
}

/// Sets the begin and end distance for the distance fog (mist)
/// It should provide maximum visiblitiy and minimum
/// "popping" tiles
void UpdateFogDistance(float distance)
{
	pie_UpdateFogDistance(FOG_END-FOG_DEPTH+distance*FOG_BEGIN_SCALE, FOG_END+distance*FOG_END_SCALE);
}


//three fog modes, background fog, distance fog, ground mist


#define UMBRA_RADIUS 384
#define FOG_RADIUS 384   //256 too abrupt at edges
#define FOG_START 512
#define FOG_RATE 10

#define MIN_DROID_LIGHT_LEVEL	96
#define	DROID_SEEK_LIGHT_SPEED	2

void	calcDroidIllumination(DROID *psDroid)
{
UDWORD	lightVal;	// sum of light vals
UDWORD	presVal;
UDWORD	tileX,tileY;
UDWORD	retVal;
float	fraction,adjust;

 /* Establish how long the last game frame took */
	fraction = MAKEFRACT(frameTime)/GAME_TICKS_PER_SEC;

	/* See if the droid's at the edge of the map */
	tileX = psDroid->x/TILE_UNITS;
	tileY = psDroid->y/TILE_UNITS;
	/* Are we at the edge */
	if(tileX<=1 || tileX>=mapWidth-2 || tileY<=1 || tileY>=mapHeight-2)
	{
		lightVal = mapTile(tileX,tileY)->illumination;
		lightVal += MIN_DROID_LIGHT_LEVEL;
	}
	else
	{
		lightVal = mapTile(tileX,tileY)->illumination +		 //
				   mapTile(tileX-1,tileY)->illumination +	 //		 *
				   mapTile(tileX,tileY-1)->illumination +	 //		***		pattern
				   mapTile(tileX+1,tileY)->illumination +	 //		 *
				   mapTile(tileX+1,tileY+1)->illumination;	 //
		lightVal/=5;
		lightVal += MIN_DROID_LIGHT_LEVEL;
	}

	/* Saturation */
	if(lightVal>255) lightVal = 255;
	presVal = psDroid->illumination;
	adjust = MAKEFRACT(lightVal) - MAKEFRACT(presVal);
	adjust *= (fraction*DROID_SEEK_LIGHT_SPEED) ;
	retVal = presVal + MAKEINT(adjust);
	if(retVal > 255) retVal = 255;
	psDroid->illumination = (UBYTE)retVal;
}


PIELIGHT lightDoFogAndIllumination(PIELIGHT brightness, SDWORD dx, SDWORD dz, PIELIGHT *pSpecular)
{
	SDWORD	umbraRadius;	// Distance to start of light falloff
	SDWORD	penumbraRadius; // radius of area of obscurity
	SDWORD	umbra;
	SDWORD	distance = sqrtf(dx*dx + dz*dz);
	SDWORD	cosA,sinA;
	PIELIGHT lighting, specular, fogColour;
	SDWORD	depth = 0;
	SDWORD	colour;
	SDWORD	fog = 0;

	penumbraRadius = world_coord(visibleTiles.x / 2);

	umbraRadius = penumbraRadius - UMBRA_RADIUS;

	if(distance < umbraRadius)
	{
		umbra = 255;
	}
	else if (distance > penumbraRadius)
	{
		umbra = 0;
	}
	else
	{
			umbra = 255 - (((distance-umbraRadius)*255)/(UMBRA_RADIUS));
	}

	if ((distance) < 32)
	{
		depth = 1;
	}


	if ((fogStatus & FOG_DISTANCE) || (fogStatus & FOG_BACKGROUND))
	{
		//add fog
		if (pie_GetFogEnabled())
		{
			cosA = COS(player.r.y);
			sinA = SIN(player.r.y);
			depth = sinA * dx + cosA * dz;
			depth >>= FP12_SHIFT;
			depth += FOG_START;
			depth /= FOG_RATE;
		}
	}

	if (fogStatus & FOG_DISTANCE)
	{
		//add fog
		if (pie_GetFogEnabled())
		{
			if (!(fogStatus & FOG_BACKGROUND))//black penumbra so fade fog effect
			{
				fog = depth - (255 - umbra);
			}
			else
			{
				fog = depth;
			}
			if (fog < 0)
			{
				fog = 0;
			}
			else if (fog > 255)
			{
				fog = 255;
			}
		}
	}

	if ((fogStatus & FOG_BACKGROUND) && (pie_GetFogEnabled()))
	{
		//fog the umbra but only for distant points
		if (depth > (float)-0.1)
		{
			fog = fog + (255 - umbra);
			if (fog > 255)
			{
				fog = 255;
			}
			if (fog < 0)
			{
				fog = 0;
			}
		}
	}
	else
	{
		brightness = pal_SetBrightness(pie_ByteScale(brightness.byte.r, (UBYTE)umbra));
	}

	if (fog == 0)
	{
		if (pSpecular != NULL)
		{
			*pSpecular = WZCOL_BLACK;
		}
		lighting = brightness;
	}
	else
	{
		if (pSpecular != NULL)
		{
			fogColour = pie_GetFogColour();
			specular.byte.a = fog;
			specular.byte.r = pie_ByteScale(fog, fogColour.byte.r);
			specular.byte.g = pie_ByteScale(fog, fogColour.byte.g);
			specular.byte.b = pie_ByteScale(fog, fogColour.byte.b);
			*pSpecular = specular;
		}

		//calculate new brightness
		colour = 256 - fog;
		brightness = pal_SetBrightness(pie_ByteScale(colour, brightness.byte.r));
		lighting = brightness;
	}

	return lighting;
}

void	doBuildingLights( void )
{
	STRUCTURE	*psStructure;
	UDWORD	i;
	LIGHT	light;

	for(i=0; i<MAX_PLAYERS; i++)
	{
		for(psStructure = apsStructLists[i]; psStructure; psStructure = psStructure->psNext)
		{
			light.range = psStructure->pStructureType->baseWidth * TILE_UNITS;
			light.position.x = psStructure->x;
			light.position.z = psStructure->y;
			light.position.y = map_Height(light.position.x,light.position.z);
			light.range = psStructure->pStructureType->baseWidth * TILE_UNITS;
			light.colour = LIGHT_WHITE;
			processLight(&light);
		}
	}
}

/* Experimental moving shadows code */
void	findSunVector( void )
{
	SDWORD	val,val2,val3;

	val = getStaticTimeValueRange(16384,8192);
	val = 4096 - val;
	val2 = getStaticTimeValueRange(16384,4096);
	val2 = 0-val2;
	val3 = getStaticTimeValueRange(49152,8192);
	val3 = 4096 - val3;

	theSun.x = val;
	theSun.y = val2;
	theSun.z = val3;
}
