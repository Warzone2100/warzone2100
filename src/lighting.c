/* Lighting.c - Alex McLean, Pumpkin Studios, EIDOS Interactive. */
/* Calculates the shading values for the terrain world. */
/* The terrain intensity values are calculated at map load/creation time. */

#include "lib/framework/frame.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lib/ivis_common/ivisdef.h" //ivis matrix code
#include "lib/ivis_common/piestate.h" //ivis matrix code
#include "lib/ivis_common/piefunc.h" //ivis matrix code
#include "lib/ivis_common/geo.h" //ivis matrix code
#include "map.h"
#include "lighting.h"
#include "display3d.h"
#include "effects.h"
#include "atmos.h"
#include "environ.h"
#include "lib/gamelib/gtime.h"
#include "console.h"
#include "arrow.h"

/*	The vector that holds the sun's lighting direction - planar */
iVector	theSun;
UDWORD	fogStatus = 0;
/*	Module function Prototypes */

UDWORD	lightDoFogAndIllumination(UBYTE brightness, SDWORD dx, SDWORD dz, UDWORD* pSpecular);
void	doBuildingLights( void );
void	processLight(LIGHT *psLight);
UDWORD	calcDistToTile(UDWORD tileX, UDWORD tileY, iVector *pos);
void	colourTile(SDWORD xIndex, SDWORD yIndex, LIGHT_COLOUR colour, UBYTE percent);
//void	initLighting( void );
void	calcTileIllum(UDWORD tileX, UDWORD tileY);
void	normalsOnTile(UDWORD tileX, UDWORD tileY, UDWORD quadrant);
UDWORD	numNormals;		// How many normals have we got?
iVector normals[8];		// Maximum 8 possible normals
extern void	draw3dLine(iVector *src, iVector *dest, UBYTE col);




/*****************************************************************************/
/*
 * SOURCE
 */
/*****************************************************************************/

/*Rewrote the function so it takes parameters and also doesn't loop thru'
the map 4 times!*/
/*void initLighting( void )
{
UDWORD	i,j;
MAPTILE	*psTile;

	for(i=0; i<mapWidth; i++)
	{
		for(j=0; j<mapHeight; j++)
		{
			mapTile(i,j)->illumination = 16;
		}
	}

	//for(i=2; i<mapHeight-2; i++)
	//{
	//	for(j=2; j<mapWidth-2; j++)
	//	{
	//		calcTileIllum(j,i);
	//	}
	//}

	for(i=1; i<mapHeight-1; i++)
	{
		for(j=1; j<mapWidth-1; j++)
		{
			calcTileIllum(j,i);
		}
	}

	for(i=0; i<mapWidth; i++)
	{
		for(j=0; j<mapHeight; j++)
		{
			if(i==0 OR j==0 OR i>=mapWidth-1 OR j>=mapHeight-1)
			{
//				mapTile(i,j)->height = 0;
				psTile = mapTile(i,j);
				if(TERRAIN_TYPE(psTile) == TER_WATER)
				{
					psTile->texture = 0;
				}
			}
		}
	}

	// Cheers to paul for this idea - works on PC too
	//	Basically darkens down the tiles that are outside the scroll
	//	limits - thereby emphasising the cannot-go-there-ness of them
	for(i=0; i<mapWidth; i++)
	{
		for(j=0; j<mapHeight; j++)
		{
			if(i<(scrollMinX+4) OR i>(scrollMaxX-4) OR j<(scrollMinY+4) OR j>(scrollMaxY-4))
			{
				mapTile(i,j)->illumination/=3;
			}
		}
	}
}*/

//should do the same as above except cuts down on the loop count!
//By passing in params - it means that if the scroll limits are changed mid-mission
//we can re-do over the area that hasn't been seen
void initLighting(UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2)
{
    UDWORD       i, j;
    MAPTILE	    *psTile;

    //quick check not trying to go off the map - don't need to check for < 0 since UWORD's!!
    if (x1 > mapWidth OR x2 > mapWidth OR y1 > mapHeight OR y2 > mapHeight)
    {
        ASSERT((FALSE, "initLighting: coords off edge of map"));
        return;
    }

    for(i = x1; i < x2; i++)
	{
		for(j = y1; j < y2; j++)
		{
            psTile = mapTile(i, j);
            //always make the edge tiles dark
            if (i==0 OR j==0 OR i >= mapWidth-1 OR j >= mapHeight-1)
            {
			    psTile->illumination = 16;

                //give water tiles at edge of map a border
				if(TERRAIN_TYPE(psTile) == TER_WATER)
				{
					psTile->texture = 0;
				}
            }
            else
            {
			    calcTileIllum(i,j);
            }
        	// Cheers to paul for this idea - works on PC too
    	    //	Basically darkens down the tiles that are outside the scroll
	        //	limits - thereby emphasising the cannot-go-there-ness of them
			if((SDWORD)i < (scrollMinX+4) OR (SDWORD)i > (scrollMaxX-4) OR
                (SDWORD)j < (scrollMinY+4) OR (SDWORD)j > (scrollMaxY-4))
			{
				psTile->illumination/=3;
			}
		}
	}
}


void	calcTileIllum(UDWORD tileX, UDWORD tileY)
{
iVector	finalVector;
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
	pie_VectorNormalise(&finalVector);
	pie_VectorNormalise(&theSun);

//	iV_NumberOut(theSun.x,100,100,255);
//	iV_NumberOut(theSun.y,100,110,255);
//	iV_NumberOut(theSun.z,100,120,255);

//	iV_NumberOut(numNormals,100,140,255);

	dotProduct =	(finalVector.x * theSun.x +
					finalVector.y * theSun.y +
					finalVector.z * theSun.z)>>FP12_SHIFT;

   /* iV_NumberOut(dotProduct,100,150,255);*/
	val = ((abs(dotProduct)) / 16);
	if (val == 0) val = 1;
	if(val > 254) val = 254;
	mapTile(tileX, tileY)->illumination = val;
}

void normalsOnTile(UDWORD tileX, UDWORD tileY, UDWORD quadrant)
{
iVector	corner1,corner2,corner3;
MAPTILE	*psTile, *tileRight, *tileDownRight, *tileDown;
SDWORD	rMod,drMod,dMod,nMod;

	/* Get a pointer to our tile */
	psTile			= mapTile(tileX,tileY);

	/* And to the ones to the east, south and southeast of it */
	tileRight		= mapTile(tileX+1,tileY);
	tileDownRight	= mapTile(tileX+1,tileY+1);
	tileDown		= mapTile(tileX,tileY+1);

	if(TERRAIN_TYPE(psTile) == TER_WATER)
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
//		if(psTile->triangleFlip)
		if(TRI_FLIPPED(psTile))
		{
			if(quadrant==0)
			{
			 	corner1.x = (tileX+1)<<TILE_SHIFT;
				corner1.y = tileY<<TILE_SHIFT;
				corner1.z = tileRight->height - rMod;

				corner2.x = (tileX+1)<<TILE_SHIFT;
				corner2.y = (tileY+1)<<TILE_SHIFT;
				corner2.z = tileDownRight->height - drMod;

				corner3.x = tileX<<TILE_SHIFT;
				corner3.y = (tileY+1)<<TILE_SHIFT;
				corner3.z = tileDown->height - dMod;
				pie_SurfaceNormal(&corner1,&corner2,&corner3,&normals[numNormals++]);
			}
			else
			{
				corner1.x = tileX<<TILE_SHIFT;
				corner1.y = tileY<<TILE_SHIFT;
				corner1.z = psTile->height - nMod;

				corner2.x = (tileX+1)<<TILE_SHIFT;
				corner2.y = tileY<<TILE_SHIFT;
				corner2.z = tileRight->height - rMod;

				corner3.x = tileX<<TILE_SHIFT;
				corner3.y = (tileY+1)<<TILE_SHIFT;
				corner3.z = tileDown->height - dMod;
				pie_SurfaceNormal(&corner1,&corner2,&corner3,&normals[numNormals++]);
			}
		}
		else
		{
			/* Otherwise, it's not flipped and so two triangles*/
			corner1.x = tileX<<TILE_SHIFT;
			corner1.y = tileY<<TILE_SHIFT;
			corner1.z = psTile->height - nMod;

			corner2.x = (tileX+1)<<TILE_SHIFT;
			corner2.y = tileY<<TILE_SHIFT;
			corner2.z = tileRight->height - rMod;

			corner3.x = (tileX+1)<<TILE_SHIFT;
			corner3.y = (tileY+1)<<TILE_SHIFT;
			corner3.z = tileDownRight->height - drMod;
			pie_SurfaceNormal(&corner1,&corner2,&corner3,&normals[numNormals++]);

			corner1.x = tileX<<TILE_SHIFT;
			corner1.y = tileY<<TILE_SHIFT;
			corner1.z = psTile->height - nMod;

			corner2.x = (tileX+1)<<TILE_SHIFT;
			corner2.y = (tileY+1)<<TILE_SHIFT;
			corner2.z = tileDownRight->height - drMod;

			corner3.x = tileX<<TILE_SHIFT;
			corner3.y = (tileY+1)<<TILE_SHIFT;
			corner3.z = tileDown->height - dMod;
			pie_SurfaceNormal(&corner1,&corner2,&corner3,&normals[numNormals++]);
		}
		break;
	case 1:
	case 3:
		/* Is it flipped? In this case two triangles  */
//		if(psTile->triangleFlip)
		if(TRI_FLIPPED(psTile))
		{
	   	 	corner1.x = tileX<<TILE_SHIFT;
	   		corner1.y = tileY<<TILE_SHIFT;
	   		corner1.z = psTile->height - nMod;

	   		corner2.x = (tileX+1)<<TILE_SHIFT;
	   		corner2.y = tileY<<TILE_SHIFT;
	   		corner2.z = tileRight->height - rMod;

	   		corner3.x = tileX<<TILE_SHIFT;
	   		corner3.y = (tileY+1)<<TILE_SHIFT;
	   		corner3.z = tileDown->height - dMod;
			pie_SurfaceNormal(&corner1,&corner2,&corner3,&normals[numNormals++]);

			corner1.x = (tileX+1)<<TILE_SHIFT;
	   		corner1.y = tileY<<TILE_SHIFT;
	   		corner1.z = tileRight->height - rMod;

			corner2.x = (tileX+1)<<TILE_SHIFT;
	   		corner2.y = (tileY+1)<<TILE_SHIFT;
	   		corner2.z = tileDownRight->height - drMod;

	   		corner3.x = tileX<<TILE_SHIFT;
	   		corner3.y = (tileY+1)<<TILE_SHIFT;
	   		corner3.z = tileDown->height - dMod;
	   		pie_SurfaceNormal(&corner1,&corner2,&corner3,&normals[numNormals++]);
		}
		else
		{
			if(quadrant==1)
			{
			 	corner1.x = tileX<<TILE_SHIFT;
				corner1.y = tileY<<TILE_SHIFT;
				corner1.z = psTile->height - nMod;

				corner2.x = (tileX+1)<<TILE_SHIFT;
				corner2.y = (tileY+1)<<TILE_SHIFT;
				corner2.z = tileDownRight->height - drMod;

				corner3.x = tileX<<TILE_SHIFT;
				corner3.y = (tileY+1)<<TILE_SHIFT;
				corner3.z = tileDown->height - dMod;
				pie_SurfaceNormal(&corner1,&corner2,&corner3,&normals[numNormals++]);
			}
			else
			{
				corner1.x = tileX<<TILE_SHIFT;
				corner1.y = tileY<<TILE_SHIFT;
				corner1.z = psTile->height - nMod;

				corner2.x = (tileX+1)<<TILE_SHIFT;
				corner2.y = tileY<<TILE_SHIFT;
				corner2.z = tileRight->height - rMod;

				corner3.x = (tileX+1)<<TILE_SHIFT;
				corner3.y = (tileY+1)<<TILE_SHIFT;
				corner3.z = tileDownRight->height - drMod;
				pie_SurfaceNormal(&corner1,&corner2,&corner3,&normals[numNormals++]);
			}
		}
		break;
	default:
		ASSERT((FALSE,"Invalid quadrant in lighting code"));
	} // end switch
}

#if 0
/*	Processes a light into the tileScreenInfo structure - this needs
	to be optimised and profiled as it's costly to perform
*/
void	processLight(LIGHT *psLight)
{
SDWORD	i,j;
UDWORD	tileX,tileY;
UDWORD	offset;
UDWORD	distToCorner;
UDWORD	percent;
UDWORD	xIndex,yIndex;
SDWORD	xUpper,yUpper,xLower,yLower;
SDWORD	gridMinX,gridMinY,gridMaxX,gridMaxY;

	/* Firstly - there's no point processing lights that are off the grid */
	if(clipXY(psLight->position.x,psLight->position.z) == FALSE)
	{
		return;
	}

	tileX = (psLight->position.x/TILE_UNITS);
	tileY = (psLight->position.z/TILE_UNITS);
	offset = psLight->range/TILE_UNITS;

	if(player.p.x>=0)
	{
		gridMinX = player.p.x/TILE_UNITS;
	}
	else
	{
		gridMinX = 0;
	}
	if(player.p.z >=0)
	{
		gridMinY = player.p.z/TILE_UNITS;
	}
	else
	{
		gridMinY = 0;
	}
	gridMaxX = gridMinX + visibleXTiles;
	gridMaxY = gridMinY + visibleYTiles;

	xLower = tileX - offset;
	xUpper = tileX + offset;
	yLower = tileY - offset;
	yUpper = tileY + offset;

	for(i=xLower; i<xUpper; i++)
	{
		for(j=yLower; j<yUpper; j++)
		{
			/*
				We must make sure that we don't attempt to colour a tile that isn't actually
				on our grid - say when a light is on the periphery of the grid.
			*/
			if(i>gridMinX AND i<gridMaxX AND j>gridMinY AND j<gridMaxY)
			{
		 		distToCorner = calcDistToTile(i,j,&psLight->position);
				/* If we're inside the range of the light */
				if(distToCorner<psLight->range)
				{
					/* Find how close we are to it */
					percent = 100 - PERCENT(distToCorner,psLight->range);

					xIndex = i - playerXTile;
					yIndex = j - playerZTile;
					colourTile(xIndex,yIndex,psLight->colour, (UBYTE)percent);
				}
			}
		}
	}
}

#endif

void	processLight(LIGHT *psLight)
{
SDWORD	tileX,tileY;
SDWORD	startX,endX;
SDWORD	startY,endY;
SDWORD	rangeSkip;
SDWORD	i,j;
SDWORD	distToCorner;
SDWORD	xIndex,yIndex;
UDWORD	percent;

 	/* Firstly - there's no point processing lights that are off the grid */
	if(clipXY(psLight->position.x,psLight->position.z) == FALSE)
	{
		return;
	}

	tileX = psLight->position.x/TILE_UNITS;
	tileY = psLight->position.z/TILE_UNITS;

	rangeSkip = (psLight->range*psLight->range);
	rangeSkip *=2;
	rangeSkip = (UDWORD)sqrt(rangeSkip);
	rangeSkip/=TILE_UNITS;
	rangeSkip+=1;
	/* Rough guess? */
	startX = tileX - rangeSkip;
	endX = tileX + rangeSkip;
	startY = tileY - rangeSkip;
	endY = tileY + rangeSkip;

	/* Clip to grid limits */
	if(startX < 0)
    {
        startX = 0;
    }
	else if(startX > (SDWORD)(mapWidth-1))
    {
        startX = mapWidth-1;
    }
	if(endX < 0)
    {
        endX = 0;
    }
	else if(endX > (SDWORD)(mapWidth-1))
    {
        endX = mapWidth-1;
    }

	/* Clip to grid limits */
	if(startY < 0)
    {
        startY = 0;
    }
    else if(startY > (SDWORD)(mapHeight-1))
    {
        startY = mapHeight-1;
    }
	if(endY < 0)
    {
        endY = 0;
    }
	else if(endY > (SDWORD)(mapHeight-1))
    {
        endY = mapHeight-1;
    }


	for(i=startX;i<=endX; i++)
	{
		for(j=startY; j<=endY; j++)
		{
				distToCorner = calcDistToTile(i,j,&psLight->position);
				/* If we're inside the range of the light */
			 	if(distToCorner<(SDWORD)psLight->range)
				{
					/* Find how close we are to it */
					percent = 100 - PERCENT(distToCorner,psLight->range);
					xIndex = i - playerXTile;
					yIndex = j - playerZTile;
					// Might go off the grid for light ranges > one tile
//					if(i<visibleXTiles AND j<visibleYTiles AND i>=0 AND j>=0)
					if(xIndex >= 0 AND yIndex >= 0 AND
                        xIndex < (SDWORD)visibleXTiles AND
                        yIndex < (SDWORD)visibleYTiles)
					{
						colourTile(xIndex,yIndex,psLight->colour, (UBYTE)(2*percent));
					}
				}


		}
	}
}

/*
UDWORD	calcDistToTile(UDWORD tileX, UDWORD tileY, iVector *pos)
{
UDWORD	x1,y1;
UDWORD	x2,y2;
UDWORD	xDif,yDif,zDif;
UDWORD	total;

	x1 = tileX * TILE_UNITS;
	y1 = tileY * TILE_UNITS;

	x2 = pos->x;
	y2 = pos->z;

	xDif = abs(x1-x2);
	zDif = abs(y1-y2);

	total = (xDif*xDif) + (yDif*yDif);
	return((UDWORD)sqrt(total));
}
*/
 UDWORD	calcDistToTile(UDWORD tileX, UDWORD tileY, iVector *pos)
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
	return((UDWORD)sqrt(total));
}


void	colourTile(SDWORD xIndex, SDWORD yIndex, LIGHT_COLOUR colour, UBYTE percent)
{

	ASSERT((xIndex<LAND_XGRD,"X Colour Value out of range (above) for lighting"));
	ASSERT((yIndex<LAND_YGRD,"Y Colour Value out of range (above)for lighting"));
	ASSERT((xIndex>=0,"X Colour Value out of range (below) for lighting"));
	ASSERT((yIndex>=0,"Y Colour Value out of range (below )for lighting"));


	switch(colour)
	{
 		case LIGHT_RED:
			/* And add that to the lighting value */
 			if(tileScreenInfo[yIndex][xIndex].light.byte.r + percent <= 255)
 			{
 			   tileScreenInfo[yIndex][xIndex].light.byte.r += percent;
 			}
 			else
 			{
 			   tileScreenInfo[yIndex][xIndex].light.byte.r = 255;
 			}
		break;
 		case LIGHT_GREEN:
			/* And add that to the lighting value */
 			if(tileScreenInfo[yIndex][xIndex].light.byte.g + percent <= 255)
 			{
 			   tileScreenInfo[yIndex][xIndex].light.byte.g += percent;
 			}
 			else
 			{
 			   tileScreenInfo[yIndex][xIndex].light.byte.g = 255;
 			}
		break;
 		case LIGHT_BLUE:
			/* And add that to the lighting value */
 			if(tileScreenInfo[yIndex][xIndex].light.byte.b + percent <= 255)
 			{
 			   tileScreenInfo[yIndex][xIndex].light.byte.b += percent;
 			}
 			else
 			{
 			   tileScreenInfo[yIndex][xIndex].light.byte.b = 255;
 			}
		break;
		case LIGHT_YELLOW:
			/* And add that to the lighting value */
 			if(tileScreenInfo[yIndex][xIndex].light.byte.r + percent <= 255)
 			{
 			   tileScreenInfo[yIndex][xIndex].light.byte.r += percent;
 			   tileScreenInfo[yIndex][xIndex].light.byte.g += percent;
 			}
 			else
 			{
 			   tileScreenInfo[yIndex][xIndex].light.byte.r = 255;
 			   tileScreenInfo[yIndex][xIndex].light.byte.g = 255;
 			}
		break;
		case LIGHT_WHITE:
			/* And add that to the lighting value */
 			if(tileScreenInfo[yIndex][xIndex].light.byte.r + percent <= 255)
 			{
 			   tileScreenInfo[yIndex][xIndex].light.byte.r += percent;
 			   tileScreenInfo[yIndex][xIndex].light.byte.g += percent;
 			   tileScreenInfo[yIndex][xIndex].light.byte.b += percent;
 			}
 			else
 			{
 			   tileScreenInfo[yIndex][xIndex].light.byte.r = 255;
 			   tileScreenInfo[yIndex][xIndex].light.byte.g = 255;
 			   tileScreenInfo[yIndex][xIndex].light.byte.b = 255;
 			}
		break;
		default:
			ASSERT((FALSE,"Weirdy colour of light attempted"));
			break;
	}
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
	if(tileX<=1 OR tileX>=mapWidth-2 OR tileY<=1 OR tileY>=mapHeight-2)
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

//#define EDGE_FOG


UDWORD	lightDoFogAndIllumination(UBYTE brightness, SDWORD dx, SDWORD dz, UDWORD* pSpecular)
{
SDWORD	umbraRadius;	// Distance to start of light falloff
SDWORD	penumbraRadius; // radius of area of obscurity
SDWORD	umbra;
//SDWORD	edge;
SDWORD	distance;
SDWORD	cosA,sinA;
PIELIGHT lighting, specular, fogColour;
SDWORD	depth = 0;
SDWORD	colour;
SDWORD	fog = 0;
//SDWORD	mist = 0;

	distance = (SDWORD) (sqrt(dx*dx+dz*dz));

	penumbraRadius = (visibleXTiles/2)<<TILE_SHIFT;

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
#ifdef EDGE_FOG
	if(player.p.x < penumbraRadius)//fog lefthand edge
	{
		if (pie_GetFogEnabled())
		{
			if (player.p.x - dx + penumbraRadius < 0)
			{
				edge = 0;
			}
			else if (player.p.x - dx + penumbraRadius > UMBRA_RADIUS)
			{
				edge = 255;
			}
			else
			{
				edge = (((player.p.x - dx + penumbraRadius)*255)/(UMBRA_RADIUS));
			}
		}
		else
		{
			edge = 255;
		}
	}
	else
	{
		edge = 255;
	}
#endif

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

   	/*
	if ((fogStatus & FOG_GROUND) && (pie_GetFogStatus()))
	{
		//add mist
		centreX = ( player.p.x + ((visibleXTiles/2)<<TILE_SHIFT) );
		centreZ = ( player.p.z + ((visibleYTiles/2)<<TILE_SHIFT) );
		mist = map_MistValue(centreX-dx,centreZ-dz);
		if (!(fogStatus & FOG_BACKGROUND))//black penumbra so fade fog effect
		{
			mist = mist - (255 - umbra);
		}
		if (mist < 0)
		{
			mist = 0;
		}
		else if (mist > 255)
		{
			mist = 255;
		}
	}

	fog = fog + mist;
	if (fog > 255)
	{
		fog = 255;
	}
	*/
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
#ifdef EDGE_FOG
		fog = fog + (255 - edge);
		if (fog > 255)
		{
			fog = 255;
		}
		if (fog < 0)
		{
			fog = 0;
		}
#endif
	}
	else
	{
		brightness = (UBYTE)pie_ByteScale((UBYTE)brightness, (UBYTE)umbra);
	}

	if (fog == 0) {
		// (d3d with no fog?)
		*pSpecular = 0;
		lighting.byte.a = UBYTE_MAX;
		lighting.byte.r = brightness;
		lighting.byte.g = brightness;
		lighting.byte.b = brightness;
	}
	else
	{
		fogColour.argb = pie_GetFogColour();
		specular.byte.a = (UBYTE)fog;
		specular.byte.r = pie_ByteScale((UBYTE)fog, fogColour.byte.r);
		specular.byte.g = pie_ByteScale((UBYTE)fog, fogColour.byte.g);
		specular.byte.b = pie_ByteScale((UBYTE)fog, fogColour.byte.b);
		*pSpecular = specular.argb;

		//calculate new brightness
		colour = 256 - fog;
		brightness = (UBYTE)pie_ByteScale((UBYTE)colour, (UBYTE)brightness);
		lighting.byte.a = UBYTE_MAX;
		lighting.byte.r = brightness;
		lighting.byte.g = brightness;
		lighting.byte.b = brightness;
	}
	return lighting.argb;
}


/*
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
*/
#ifdef ALEXM
void	findSunVector( void )
{
SDWORD	val,val2,val3;
UDWORD	i;

	val = getStaticTimeValueRange(16384,8192);
	val = 4096 - val;
	val2 = getStaticTimeValueRange(16384,4096);
	val2 = 0-val2;
	val3 = getStaticTimeValueRange(49152,8192);
	val3 = 4096 - val3;

	theSun.x = val;
	theSun.y = val2;
	theSun.z = val3;
	flushConsoleMessages();
	DBCONPRINTF(ConsoleString,(ConsoleString,"Sun X Vector : %d",theSun.x));
	DBCONPRINTF(ConsoleString,(ConsoleString,"Sun Y Vector : %d",theSun.y));
	DBCONPRINTF(ConsoleString,(ConsoleString,"Sun Z Vector : %d",theSun.z));
   }

void	showSunOnTile(UDWORD x, UDWORD y)
{
iVector	a,b;


	{
		a.x = (x<<TILE_SHIFT)+(TILE_UNITS/2);
		a.z = (y<<TILE_SHIFT)+(TILE_UNITS/2);
		a.y = map_Height(a.x,a.z);

		b.x = a.x + theSun.x/64;
		b.y = a.y + theSun.y/64;
		b.z = a.z + theSun.z/64;

		pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
		pie_SetFogStatus(FALSE);
		draw3dLine(&a,&b,mapTile(x,y)->illumination);
  		pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);
	}
}
#endif
