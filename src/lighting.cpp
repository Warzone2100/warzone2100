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
 * @file lighting.c
 * Calculates the shading values for the terrain world.
 * The terrain intensity values are calculated at map load/creation time.
 * - Alex McLean, Pumpkin Studios, EIDOS Interactive.
 */

#include "lib/framework/frame.h"
#include "lib/framework/math_ext.h"

#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_opengl/pienormalize.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/framework/fixedpoint.h"

#include "lib/gamelib/gtime.h"

#include "map.h"
#include "lighting.h"
#include "display3d.h"
#include "terrain.h"
#include "warzoneconfig.h"

// These magic values determine the fog
#define FOG_ALTITUDE_COEFFICIENT 1.3f

/*	The vector that holds the sun's lighting direction - planar */
static Vector3f theSun(0.f, 0.f, 0.f);
static Vector3f theSun_ForTileIllumination(0.f, 0.f, 0.f);

/*	Module function Prototypes */
static UDWORD calcDistToTile(UDWORD tileX, UDWORD tileY, Vector3i *pos);
static void calcTileIllum(UDWORD tileX, UDWORD tileY);

void setTheSun(Vector3f newSun)
{
	Vector3f oldSun = theSun;
	theSun = normalise(newSun) * float(FP12_MULTIPLIER);
	theSun_ForTileIllumination = Vector3f(-theSun.x, -theSun.y, theSun.z);
	if(oldSun != theSun)
	{
		// The sun has changed - must relcalulate lighting
		initLighting(0, 0, mapWidth, mapHeight);
	}
}

Vector3f getTheSun()
{
	return theSun;
}

/*****************************************************************************/
/*
 * SOURCE
 */
/*****************************************************************************/

//By passing in params - it means that if the scroll limits are changed mid-mission
//we can re-do over the area that hasn't been seen
void initLighting(UDWORD x1, UDWORD y1, UDWORD x2, UDWORD y2)
{
	// quick check not trying to go off the map - don't need to check for < 0 since UWORD's!!
	if (x1 > mapWidth || x2 > mapWidth || y1 > mapHeight || y2 > mapHeight)
	{
		ASSERT(false, "initLighting: coords off edge of map");
		return;
	}

	for (unsigned i = x1; i < x2; i++)
	{
		for (unsigned j = y1; j < y2; j++)
		{
			MAPTILE	*psTile = mapTile(i, j);

			// always make the edge tiles dark
			if (i == 0 || j == 0 || i >= mapWidth - 1 || j >= mapHeight - 1)
			{
				psTile->illumination = 16;
			}
			else
			{
				calcTileIllum(i, j);
			}
			// Basically darkens down the tiles that are outside the scroll
			// limits - thereby emphasising the cannot-go-there-ness of them
			if ((SDWORD)i < scrollMinX + 4 || (SDWORD)i > scrollMaxX - 4
			    || (SDWORD)j < scrollMinY + 4 || (SDWORD)j > scrollMaxY - 4)
			{
				psTile->illumination /= 3;
			}
		}
	}
}


static void normalsOnTile(unsigned int tileX, unsigned int tileY, unsigned int quadrant, unsigned int *numNormals, Vector3f normals[])
{
	Vector2i tiles[2][2];
	MAPTILE *psTiles[2][2];
	Vector3f corners[2][2];

	for (unsigned j = 0; j < 2; ++j)
		for (unsigned i = 0; i < 2; ++i)
		{
			tiles[i][j] = Vector2i(tileX + i, tileY + j);
			/* Get a pointer to our tile */
			/* And to the ones to the east, south and southeast of it */
			psTiles[i][j] = mapTile(tiles[i][j]);
			corners[i][j] = Vector3f(world_coord(tiles[i][j]), psTiles[i][j]->height);
		}

	int flipped = TRI_FLIPPED(psTiles[0][0]) ? 10 : 0;

	switch (quadrant + flipped)
	{
	// Not flipped.
	case 0:
	case 2:
		/* Otherwise, it's not flipped and so two triangles*/
		normals[(*numNormals)++] = pie_SurfaceNormal3fv(corners[0][0], corners[1][0], corners[1][1]);
		normals[(*numNormals)++] = pie_SurfaceNormal3fv(corners[0][0], corners[1][1], corners[0][1]);
		break;
	case 1:
		normals[(*numNormals)++] = pie_SurfaceNormal3fv(corners[0][0], corners[1][1], corners[0][1]);
		break;
	case 3:
		normals[(*numNormals)++] = pie_SurfaceNormal3fv(corners[0][0], corners[1][0], corners[1][1]);
		break;
	// Flipped.
	case 10:
		/* Is it flipped? In this case one triangle  */
		normals[(*numNormals)++] = pie_SurfaceNormal3fv(corners[1][0], corners[1][1], corners[0][1]);
		break;
	case 12:
		normals[(*numNormals)++] = pie_SurfaceNormal3fv(corners[0][0], corners[1][0], corners[0][1]);
		break;
	case 11:
	case 13:
		/* Is it flipped? In this case two triangles  */
		normals[(*numNormals)++] = pie_SurfaceNormal3fv(corners[0][0], corners[1][0], corners[0][1]);
		normals[(*numNormals)++] = pie_SurfaceNormal3fv(corners[1][0], corners[1][1], corners[0][1]);
		break;
	default:
		ASSERT(false, "Invalid quadrant in lighting code");
	} // end switch
}


static void calcTileIllum(UDWORD tileX, UDWORD tileY)
{
	unsigned int numNormals = 0; // How many normals have we got?
	Vector3f normals[8]; // Maximum 8 possible normals

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
	normalsOnTile(tileX - 1, tileY - 1, 0, &numNormals, normals);

	/* Do quadrant 1 - tile that's above and right*/
	normalsOnTile(tileX, tileY - 1, 1, &numNormals, normals);

	/* Do quadrant 2 - tile that's down and right*/
	normalsOnTile(tileX, tileY, 2, &numNormals, normals);

	/* Do quadrant 3 - tile that's down and left*/
	normalsOnTile(tileX - 1, tileY, 3, &numNormals, normals);

	// The number or normals that we got is in numNormals
	Vector3f finalVector(0.0f, 0.0f, 0.0f);
	for (unsigned i = 0; i < numNormals; i++)
	{
		finalVector += normals[i];
	}

	float dotProduct = glm::dot(normalise(finalVector), theSun_ForTileIllumination)/16;

	// Primitive ambient occlusion calculation.
	float ao = 0;
	const int cx = world_coord(tileX), cy = world_coord(tileY), maxX = world_coord(mapWidth), maxY = world_coord(mapHeight);
	float height = map_Height(clip<int>(cx, 0, maxX), clip<int>(cy, 0, maxY));
	constexpr float I = 100;
	constexpr float H = I*0.70710678118654752440f;  // √½
	constexpr int Dirs = 8;
	constexpr float dx[Dirs] = {0, H, I,  H,  0, -H, -I, -H};  // I sin(2π dir/Dirs)
	constexpr float dy[Dirs] = {I, H, 0, -H, -I, -H,  0,  H};  // I cos(2π dir/Dirs)

	for (int dir = 0; dir < Dirs; ++dir) {
		float maxTangent = 0;
		for (int dist = 1; dist < 9; ++dist) {
			float tangent = (map_Height(clip<int>(static_cast<int>(cx + dx[dir]*dist), 0, maxX), clip<int>(static_cast<int>(cy + dy[dir]*dist), 0, maxY)) - height)/(I*dist);
			maxTangent = std::max(maxTangent, tangent);
		}
		// Ambient light in this direction is proportional to the integral from tan(φ) = tangent to tan(φ) = ∞ of dφ cos(φ).
		// Indefinite integral is sin(φ), so definite integral is 1 - sin(atan(tangent)) = 1 - tangent/√(tangent² + 1).
		ao += 1 - maxTangent/sqrtf(maxTangent*maxTangent + 1);
	}
	ao *= 1.f/Dirs;

	mapTile(tileX, tileY)->illumination = static_cast<uint8_t>(clip<int>(static_cast<int>(abs(dotProduct*ao)), 1, 254));
}

static void colourTile(SDWORD xIndex, SDWORD yIndex, PIELIGHT light_colour, double fraction)
{
	PIELIGHT colour = getTileColour(xIndex, yIndex);
	colour.byte.r = static_cast<uint8_t>(MIN(255, colour.byte.r + light_colour.byte.r * fraction));
	colour.byte.g = static_cast<uint8_t>(MIN(255, colour.byte.g + light_colour.byte.g * fraction));
	colour.byte.b = static_cast<uint8_t>(MIN(255, colour.byte.b + light_colour.byte.b * fraction));
	setTileColour(xIndex, yIndex, colour);
}

void processLight(LIGHT *psLight)
{
	/* Firstly - there's no point processing lights that are off the grid */
	if (clipXY(psLight->position.x, psLight->position.z) == false)
	{
		return;
	}

	const int tileX = psLight->position.x / TILE_UNITS;
	const int tileY = psLight->position.z / TILE_UNITS;
	const int rangeSkip = static_cast<int>(sqrtf(psLight->range * psLight->range * 2) / TILE_UNITS + 1);

	/* Rough guess? */
	int startX = tileX - rangeSkip;
	int endX = tileX + rangeSkip;
	int startY = tileY - rangeSkip;
	int endY = tileY + rangeSkip;

	/* Clip to grid limits */
	startX = MAX(startX, 0);
	endX = MAX(endX, 0);
	endX = MIN(endX, mapWidth - 1);
	startX = MIN(startX, endX);
	startY = MAX(startY, 0);
	endY = MAX(endY, 0);
	endY = MIN(endY, mapHeight - 1);
	startY = MIN(startY, endY);

	for (int i = startX; i <= endX; i++)
	{
		for (int j = startY; j <= endY; j++)
		{
			int distToCorner = calcDistToTile(i, j, &psLight->position);

			/* If we're inside the range of the light */
			if (distToCorner < (SDWORD)psLight->range)
			{
				/* Find how close we are to it */
				double ratio = (100.0 - PERCENT(distToCorner, psLight->range)) / 100.0;
				colourTile(i, j, psLight->colour, ratio);
			}
		}
	}
}


static UDWORD calcDistToTile(UDWORD tileX, UDWORD tileY, Vector3i *pos)
{
	int x1, y1, z1;

	/* The coordinates of the tile corner */
	x1 = tileX * TILE_UNITS;
	y1 = map_TileHeight(tileX, tileY);
	z1 = tileY * TILE_UNITS;

	return iHypot3(x1 - pos->x, y1 - pos->y, z1 - pos->z);
}

/// Sets the begin and end distance for the distance fog (mist)
/// It should provide maximum visibility and minimum
/// "popping" tiles
void updateFogDistance(float distance)
{
	pie_UpdateFogDistance(war_getFogStart() + (distance - war_GetMapZoom()) * FOG_ALTITUDE_COEFFICIENT, war_getFogEnd() + (distance - war_GetMapZoom()) * FOG_ALTITUDE_COEFFICIENT);
}

void setDefaultFogColour()
{
	ASSERT(tilesetDir != nullptr, "Uninitialized tilesetDir");
	switch (currentMapTileset)
	{
		case MAP_TILESET::ARIZONA:
			// Arizona, and default. = b08f5f (or, 0x78684f)
			pie_SetFogColour(WZCOL_FOG_ARIZONA);
			break;
		case MAP_TILESET::URBAN:
			// Urban = 0x101040 (or, 0xc9920f)
			pie_SetFogColour(WZCOL_FOG_URBAN);
			break;
		case MAP_TILESET::ROCKIES:
			// Rockies = 0xb6e1ec
			pie_SetFogColour(WZCOL_FOG_ROCKIE);
			break;
	}
}

#define MIN_DROID_LIGHT_LEVEL	96
#define	DROID_SEEK_LIGHT_SPEED	2

void calcDroidIllumination(DROID *psDroid)
{
	int lightVal, presVal, retVal;
	float adjust;
	const int tileX = map_coord(psDroid->pos.x);
	const int tileY = map_coord(psDroid->pos.y);

	/* Are we at the edge, or even on the map */
	if (!tileOnMap(tileX, tileY))
	{
		psDroid->illumination = UBYTE_MAX;
		return;
	}
	else if (tileX <= 1 || tileX >= mapWidth - 2 || tileY <= 1 || tileY >= mapHeight - 2)
	{
		lightVal = mapTile(tileX, tileY)->illumination;
		lightVal += MIN_DROID_LIGHT_LEVEL;
	}
	else
	{
		lightVal = mapTile(tileX, tileY)->illumination +		 //
		           mapTile(tileX - 1, tileY)->illumination +	 //		 *
		           mapTile(tileX, tileY - 1)->illumination +	 //		***		pattern
		           mapTile(tileX + 1, tileY)->illumination +	 //		 *
		           mapTile(tileX + 1, tileY + 1)->illumination;	 //
		lightVal /= 5;
		lightVal += MIN_DROID_LIGHT_LEVEL;
	}

	/* Saturation */
	if (lightVal > 255)
	{
		lightVal = 255;
	}
	presVal = psDroid->illumination;
	adjust = (float)lightVal - (float)presVal;
	adjust *= graphicsTimeAdjustedIncrement(DROID_SEEK_LIGHT_SPEED);
	retVal = static_cast<int>(presVal + adjust);
	if (retVal > 255)
	{
		retVal = 255;
	}
	psDroid->illumination = retVal;
}

void doBuildingLights()
{
	STRUCTURE	*psStructure;
	UDWORD	i;
	LIGHT	light;

	for (i = 0; i < MAX_PLAYERS; i++)
	{
		for (psStructure = apsStructLists[i]; psStructure; psStructure = psStructure->psNext)
		{
			light.range = psStructure->pStructureType->baseWidth * TILE_UNITS;
			light.position.x = psStructure->pos.x;
			light.position.z = psStructure->pos.y;
			light.position.y = map_Height(light.position.x, light.position.z);
			light.range = psStructure->pStructureType->baseWidth * TILE_UNITS;
			light.colour = pal_Colour(255, 255, 255);
			processLight(&light);
		}
	}
}

#if 0
/* Experimental moving shadows code */
void findSunVector()
{
	Vector3f val(
	    4096 - getModularScaledGraphicsTime(16384, 8192),
	    0 - getModularScaledGraphicsTime(16384, 4096),
	    4096 - getModularScaledGraphicsTime(49152, 8192)
	);

	setTheSun(val);
}
#endif
