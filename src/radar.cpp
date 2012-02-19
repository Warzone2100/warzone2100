/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2012  Warzone 2100 Project

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
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/framework/fixedpoint.h"
#include "lib/ivis_opengl/pieblitfunc.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piefunc.h"
#include "lib/gamelib/gtime.h"
#include "advvis.h"
#include "objects.h"
#include "display3d.h"
#include "map.h"
#include "component.h"
#include "radar.h"
#include "mapdisplay.h"
#include "hci.h"
#include "geometry.h"
#include "intimage.h"
#include "loop.h"
#include "warcam.h"
#include "display.h"
#include "mission.h"
#include "multiplay.h"
#include "intdisplay.h"
#include "texture.h"

#define HIT_NOTIFICATION	(GAME_TICKS_PER_SEC * 2)
#define RADAR_FRAME_SKIP	10

bool bEnemyAllyRadarColor = false;     			/**< Enemy/ally radar color. */
RADAR_DRAW_MODE	radarDrawMode = RADAR_MODE_DEFAULT;	/**< Current mini-map mode. */
bool rotateRadar; ///< Rotate the radar?

static PIELIGHT		colRadarAlly, colRadarMe, colRadarEnemy;
static PIELIGHT		tileColours[MAX_TILES];
static UDWORD		*radarBuffer = NULL;

PIELIGHT clanColours[]=
{	// see frontend2.png for team color order.
	// [r,g,b,a]
	{{0,255,0,255}},		// green  Player 0
	{{255,210,40,255}},		// orange Player 1
	{{255,255,255,255}},	// grey   Player 2
	{{0,0,0,255}},			// black  Player 3
	{{255,0,0,255}},		// red    Player 4
	{{20,20,255,255}},		// blue   Player 5
	{{255,0,255,255}},		// pink   Player 6 (called purple in palette.txt)
	{{0,255,255,255}},		// cyan   Player 7
	{{255,255,0,255}},              // yellow Player 8
	{{192,0,255,255}},              // pink   Player 9
	{{200,255,255,255}},            // white  Player A (Should be brighter than grey, but grey is already maximum.)
	{{128,128,255,255}},            // bright blue Player B
	{{128,255,128,255}},            // neon green  Player C
	{{128,0,0,255}},                // infrared    Player D
	{{64,0,128,255}},               // ultraviolet Player E
	{{128,128,0,255}},              // brown       Player F
};

static PIELIGHT flashColours[]=
{	//right now the flash color is all bright red
	{{254,37,37,200}},	// Player 0
	{{254,37,37,200}},	// Player 1
	{{254,37,37,200}},	// Player 2
	{{254,37,37,200}},	// Player 3
	{{254,37,37,200}},	// Player 4  (notice, brighter red)
	{{254,37,37,200}},	// Player 5
	{{254,37,37,200}},	// Player 6
	{{254,37,37,200}},      // Player 7
	{{254,37,37,200}},      // Player 8
	{{254,37,37,200}},      // Player 9
	{{254,37,37,200}},      // Player A
	{{254,37,37,200}},      // Player B
	{{254,37,37,200}},      // Player C
	{{254,37,37,200}},      // Player D
	{{254,37,37,200}},      // Player E
	{{254,37,37,200}},      // Player F
};

static SDWORD radarWidth, radarHeight, radarCenterX, radarCenterY, radarTexWidth, radarTexHeight;
static uint8_t RadarZoom;
static float RadarZoomMultiplier = 1.0f;
static UDWORD radarBufferSize = 0;
static int frameSkip = 0;

static void DrawRadarTiles(void);
static void DrawRadarObjects(void);
static void DrawRadarExtras(float radarX, float radarY, float pixSizeH, float pixSizeV);
static void DrawNorth(void);

static void radarSize(int ZoomLevel)
{
	float zoom = (float)ZoomLevel * RadarZoomMultiplier / 16.0;
	radarWidth = radarTexWidth * zoom;
	radarHeight = radarTexHeight * zoom;
	if (rotateRadar)
	{
		radarCenterX = pie_GetVideoBufferWidth() - BASE_GAP * 4 - MAX(radarHeight, radarWidth) / 2;
		radarCenterY = pie_GetVideoBufferHeight() - BASE_GAP * 4 - MAX(radarWidth, radarHeight) / 2;
	}
	else
	{
		radarCenterX = pie_GetVideoBufferWidth() - BASE_GAP * 4 - radarWidth / 2;
		radarCenterY = pie_GetVideoBufferHeight() - BASE_GAP * 4 - radarHeight / 2;
	}
	debug(LOG_WZ, "radar=(%u,%u) tex=(%u,%u) size=(%u,%u)", radarCenterX, radarCenterY, radarTexWidth, radarTexHeight, radarWidth, radarHeight);
}

void radarInitVars(void)
{
	radarTexWidth = 0;
	radarTexHeight = 0;
	RadarZoom = DEFAULT_RADARZOOM;
	debug(LOG_WZ, "Resetting radar zoom to %u", RadarZoom);
	radarSize(RadarZoom);
}

bool InitRadar(void)
{
	// Ally/enemy/me colors
	colRadarAlly = WZCOL_YELLOW;
	colRadarEnemy = WZCOL_RED;
	colRadarMe = WZCOL_WHITE;
	if (mapWidth < 150)	// too small!
	{
		RadarZoom = pie_GetVideoBufferWidth() <= 640 ? 14 : DEFAULT_RADARZOOM * 2;
	}
	pie_InitRadar();

	return true;
}

bool resizeRadar(void)
{
	if (radarBuffer)
	{
		free(radarBuffer);
	}
	radarTexWidth = scrollMaxX - scrollMinX;
	radarTexHeight = scrollMaxY - scrollMinY;
	radarBufferSize = radarTexWidth * radarTexHeight * sizeof(UDWORD);
	radarBuffer = (uint32_t *)malloc(radarBufferSize);
	if (radarBuffer == NULL)
	{
		debug(LOG_FATAL, "Out of memory!");
		abort();
		return false;
	}
	memset(radarBuffer, 0, radarBufferSize);
        if (rotateRadar)
	{
		RadarZoomMultiplier = (float)MAX(RADWIDTH, RADHEIGHT) / (float)MAX(radarTexWidth, radarTexHeight);
	}
	else
	{
		RadarZoomMultiplier = 1.0f;
	}
	debug(LOG_WZ, "Setting radar zoom to %u", RadarZoom);
	radarSize(RadarZoom);

	return true;
}

bool ShutdownRadar(void)
{
	pie_ShutdownRadar();

	free(radarBuffer);
	radarBuffer = NULL;

	return true;
}

void SetRadarZoom(uint8_t ZoomLevel)
{
	if (ZoomLevel < 4) // old savegame format didn't save zoom levels very well
	{
		ZoomLevel = DEFAULT_RADARZOOM;
	}
	if (ZoomLevel > MAX_RADARZOOM)
	{
		ZoomLevel = MAX_RADARZOOM;
	}
	if (ZoomLevel < MIN_RADARZOOM)
	{
		ZoomLevel = MIN_RADARZOOM;
	}
	debug(LOG_WZ, "Setting radar zoom to %u from %u", ZoomLevel, RadarZoom);
	RadarZoom = ZoomLevel;
	radarSize(RadarZoom);
	frameSkip = 0;
}

uint8_t GetRadarZoom(void)
{
	return RadarZoom;
}

/** Calculate the radar pixel sizes. Returns pixels per tile. */
static void CalcRadarPixelSize(float *SizeH, float *SizeV)
{
	*SizeH = (float)radarHeight / (float)radarTexHeight;
	*SizeV = (float)radarWidth / (float)radarTexWidth;
}

/** Given a position within the radar, return a world coordinate. */
void CalcRadarPosition(int mX, int mY, int *PosX, int *PosY)
{
	int		sPosX, sPosY;
	float		pixSizeH, pixSizeV;
	
	Vector2f pos;
	pos.x = mX - radarCenterX;
	pos.y = mY - radarCenterY;
	if (rotateRadar)
	{
		pos = Vector2f_Rotate2f(pos, -player.r.y);
	}
	pos.x += radarWidth/2.0;
	pos.y += radarHeight/2.0;

	CalcRadarPixelSize(&pixSizeH, &pixSizeV);
	sPosX = pos.x / pixSizeH;	// adjust for pixel size
	sPosY = pos.y / pixSizeV;
	sPosX += scrollMinX;		// adjust for scroll limits
	sPosY += scrollMinY;

#if REALLY_DEBUG_RADAR
	debug(LOG_ERROR, "m=(%d,%d) radar=(%d,%d) pos(%d,%d), scroll=(%u-%u,%u-%u) sPos=(%d,%d), pixSize=(%f,%f)",
	      mX, mY, radarX, radarY, posX, posY, scrollMinX, scrollMaxX, scrollMinY, scrollMaxY, sPosX, sPosY, pixSizeH, pixSizeV);
#endif

	// old safety code -- still necessary?
	if (sPosX < scrollMinX)
	{
		sPosX = scrollMinX;
	}
	else if (sPosX > scrollMaxX)
	{
		sPosX = scrollMaxX;
	}
	if (sPosY < scrollMinY)
	{
		sPosY = scrollMinY;
	}
	else if (sPosY > scrollMaxY)
	{
		sPosY = scrollMaxY;
	}

	*PosX = sPosX;
	*PosY = sPosY;
}

void drawRadar(void)
{
	float	pixSizeH, pixSizeV;

	ASSERT(radarBuffer, "No radar buffer allocated");
	if (!radarBuffer)
	{
		return;
	}

	CalcRadarPixelSize(&pixSizeH, &pixSizeV);

	if (frameSkip <= 0)
	{
		bool filter = true;
		if (!rotateRadar)
		{
			filter = RadarZoom % 16 != 0;
		}
		DrawRadarTiles();
		DrawRadarObjects();
		pie_DownLoadRadar(radarBuffer, radarTexWidth, radarTexHeight, filter);
		frameSkip = RADAR_FRAME_SKIP;
	}
	frameSkip--;
	pie_SetRendMode(REND_ALPHA);
	pie_MatBegin();
		pie_TRANSLATE(radarCenterX, radarCenterY, 0);
		if (rotateRadar)
		{
			// rotate the map
			pie_MatRotZ(player.r.y);
			DrawNorth();
		}
		// draw the box at the dimensions of the map
		iV_TransBoxFill(-radarWidth/2.0 - 1,
						-radarHeight/2.0 - 1,
						 radarWidth/2.0,
						 radarHeight/2.0);
		pie_RenderRadar(-radarWidth/2.0 - 1,
						-radarHeight/2.0 - 1,
						 radarWidth,
						 radarHeight);
        pie_MatBegin();
            pie_TRANSLATE(-radarWidth/2 - 1, -radarHeight/2 - 1, 0);
            DrawRadarExtras(0, 0, pixSizeH, pixSizeV);
        pie_MatEnd();
		drawRadarBlips(-radarWidth/2.0 - 1, -radarHeight/2.0 - 1, pixSizeH, pixSizeV);
	pie_MatEnd();
}

static void DrawNorth(void)
{
	iV_DrawImage(IntImages, RADAR_NORTH, -((radarWidth / 2.0) + (IntImages->ImageDefs[RADAR_NORTH].Width) + 1) , -(radarHeight / 2.0));
}

static PIELIGHT appliedRadarColour(RADAR_DRAW_MODE radarDrawMode, MAPTILE *WTile)
{
	PIELIGHT WScr = WZCOL_BLACK;	// squelch warning

	// draw radar on/off feature
	if (!getRevealStatus() && !TEST_TILE_VISIBLE(selectedPlayer, WTile))
	{
		return WZCOL_RADAR_BACKGROUND;
	}

	switch(radarDrawMode)
	{
		case RADAR_MODE_TERRAIN:
		{
			// draw radar terrain on/off feature
			PIELIGHT col = tileColours[TileNumber_tile(WTile->texture)];

			col.byte.r = sqrtf(col.byte.r * WTile->illumination);
			col.byte.b = sqrtf(col.byte.b * WTile->illumination);
			col.byte.g = sqrtf(col.byte.g * WTile->illumination);
			if (terrainType(WTile) == TER_CLIFFFACE)
			{
				col.byte.r /= 2;
				col.byte.g /= 2;
				col.byte.b /= 2;
			}
			if (!hasSensorOnTile(WTile, selectedPlayer))
			{
				col.byte.r = col.byte.r * 2 / 3;
				col.byte.g = col.byte.g * 2 / 3;
				col.byte.b = col.byte.b * 2 / 3;
			}
			if (!TEST_TILE_VISIBLE(selectedPlayer, WTile))
			{
				col.byte.r /= 2;
				col.byte.g /= 2;
				col.byte.b /= 2;
			}
			WScr = col;
		}
		break;
		case RADAR_MODE_COMBINED:
		{
			// draw radar terrain on/off feature
			PIELIGHT col = tileColours[TileNumber_tile(WTile->texture)];

			col.byte.r = sqrtf(col.byte.r * (WTile->illumination + WTile->height / ELEVATION_SCALE) / 2);
			col.byte.b = sqrtf(col.byte.b * (WTile->illumination + WTile->height / ELEVATION_SCALE) / 2);
			col.byte.g = sqrtf(col.byte.g * (WTile->illumination + WTile->height / ELEVATION_SCALE) / 2);
			if (terrainType(WTile) == TER_CLIFFFACE)
			{
				col.byte.r /= 2;
				col.byte.g /= 2;
				col.byte.b /= 2;
			}
			if (!hasSensorOnTile(WTile, selectedPlayer))
			{
				col.byte.r = col.byte.r * 2 / 3;
				col.byte.g = col.byte.g * 2 / 3;
				col.byte.b = col.byte.b * 2 / 3;
			}
			if (!TEST_TILE_VISIBLE(selectedPlayer, WTile))
			{
				col.byte.r /= 2;
				col.byte.g /= 2;
				col.byte.b /= 2;
			}
			WScr = col;
		}
		break;
		case RADAR_MODE_HEIGHT_MAP:
		{
			WScr.byte.r = WScr.byte.g = WScr.byte.b = WTile->height / ELEVATION_SCALE;
		}
		break;
		case RADAR_MODE_NO_TERRAIN:
		{
			WScr = WZCOL_RADAR_BACKGROUND;
		}
		break;
		case NUM_RADAR_MODES:
		{
			assert(false);
		}
		break;
	}
	return WScr;
}

/** Draw the map tiles on the radar. */
static void DrawRadarTiles(void)
{
	SDWORD	x, y;

	for (x = scrollMinX; x < scrollMaxX; x++)
	{
		for (y = scrollMinY; y < scrollMaxY; y++)
		{
			MAPTILE	*psTile = mapTile(x, y);
			size_t pos = radarTexWidth * (y - scrollMinY) + (x - scrollMinX);

			ASSERT(pos * sizeof(*radarBuffer) < radarBufferSize, "Buffer overrun");
			radarBuffer[pos] = appliedRadarColour(radarDrawMode, psTile).rgba;
		}
	}
}

/** Draw the droids and structure positions on the radar. */
static void DrawRadarObjects(void)
{
	UBYTE				clan;
	PIELIGHT			playerCol;
	PIELIGHT			flashCol;
	int				x, y;

   	/* Show droids on map - go through all players */
   	for(clan = 0; clan < MAX_PLAYERS; clan++)
	{
		DROID		*psDroid;

		//see if have to draw enemy/ally color
		if (bEnemyAllyRadarColor)
		{
			if (clan == selectedPlayer)
			{
				playerCol = colRadarMe;
			}
			else
			{
				playerCol = (aiCheckAlliances(selectedPlayer, clan) ? colRadarAlly : colRadarEnemy);
			}
		}
		else
		{
			//original 8-color mode
			STATIC_ASSERT(MAX_PLAYERS <= ARRAY_SIZE(clanColours));
			playerCol = clanColours[getPlayerColour(clan)];
		}

		STATIC_ASSERT(MAX_PLAYERS <= ARRAY_SIZE(flashColours));
		flashCol = flashColours[getPlayerColour(clan)];

   		/* Go through all droids */
   		for(psDroid = apsDroidLists[clan]; psDroid != NULL; psDroid = psDroid->psNext)
   		{
			if (psDroid->pos.x < world_coord(scrollMinX) || psDroid->pos.y < world_coord(scrollMinY)
			    || psDroid->pos.x >= world_coord(scrollMaxX) || psDroid->pos.y >= world_coord(scrollMaxY))
			{
				continue;
			}
			if (psDroid->visible[selectedPlayer]
			    || (bMultiPlayer && game.alliance == ALLIANCES_TEAMS
			        && aiCheckAlliances(selectedPlayer,psDroid->player)))
			{
				int	x = psDroid->pos.x / TILE_UNITS;
   				int	y = psDroid->pos.y / TILE_UNITS;
				size_t	pos = (x - scrollMinX) + (y - scrollMinY) * radarTexWidth;

				ASSERT(pos * sizeof(*radarBuffer) < radarBufferSize, "Buffer overrun");
				if (clan == selectedPlayer && gameTime-psDroid->timeLastHit < HIT_NOTIFICATION)
				{
					radarBuffer[pos] = flashCol.rgba;
				}
				else
				{
					radarBuffer[pos] = playerCol.rgba;
				}
			}
   		}
   	}

   	/* Do the same for structures */
	for (x = scrollMinX; x < scrollMaxX; x++)
	{
		for (y = scrollMinY; y < scrollMaxY; y++)
		{
			MAPTILE		*psTile = mapTile(x, y);
			STRUCTURE	*psStruct;
			size_t		pos = (x - scrollMinX) + (y - scrollMinY) * radarTexWidth;

			ASSERT(pos * sizeof(*radarBuffer) < radarBufferSize, "Buffer overrun");
			if (!TileHasStructure(psTile))
			{
				continue;
			}
			psStruct = (STRUCTURE *)psTile->psObject;
			clan = psStruct->player;

			//see if have to draw enemy/ally color
			if (bEnemyAllyRadarColor)
			{
				if (clan == selectedPlayer)
				{
					playerCol = colRadarMe;
				}
				else
				{
					playerCol = (aiCheckAlliances(selectedPlayer, clan) ? colRadarAlly: colRadarEnemy);
				}
			} 
			else 
			{
				//original 8-color mode
				playerCol = clanColours[getPlayerColour(clan)];
			}
			flashCol = flashColours[getPlayerColour(clan)];

			if (psStruct->visible[selectedPlayer]
			    || (bMultiPlayer && game.alliance == ALLIANCES_TEAMS
			        && aiCheckAlliances(selectedPlayer, psStruct->player)))
			{
				if (clan == selectedPlayer && gameTime - psStruct->timeLastHit < HIT_NOTIFICATION)
				{
					radarBuffer[pos] = flashCol.rgba;
				}
				else
				{
					radarBuffer[pos] = playerCol.rgba;
				}
			}
   		}
   	}
}

/** Rotate an array of 2d vectors about a given angle, also translates them after rotating. */
static void RotateVector2D(Vector3i *Vector, Vector3i *TVector, Vector3i *Pos, int Angle, int Count)
{
	int64_t Cos = iCos(Angle);
	int64_t Sin = iSin(Angle);
	int ox = 0;
	int oy = 0;
	int i;
	Vector3i *Vec = Vector;
	Vector3i *TVec = TVector;

	if (Pos)
	{
		ox = Pos->x;
		oy = Pos->y;
	}

	for (i = 0; i < Count; i++)
	{
		TVec->x = ((Vec->x*Cos + Vec->y*Sin) >> 16) + ox;
		TVec->y = ((Vec->y*Cos - Vec->x*Sin) >> 16) + oy;
		Vec++;
		TVec++;
	}
}

static SDWORD getDistanceAdjust( void )
{
	UDWORD	origDistance = MAXDISTANCE;
	SDWORD	dif = MAX(origDistance - getViewDistance(), 0);

	return dif / 100;
}

static SDWORD getLengthAdjust( void )
{
	const int pitch = 360 - (player.r.x/DEG_1);

	// Max at
	const int lookingDown = (0 - MIN_PLAYER_X_ANGLE);
	const int lookingFar = (0 - MAX_PLAYER_X_ANGLE);

	int dif = MAX(pitch - lookingFar, 0);
	if (dif > (lookingDown - lookingFar)) 
	{
		dif = (lookingDown - lookingFar);
	}

	return dif / 2;
}

/** Draws a Myth/FF7 style viewing window */
static void drawViewingWindow(float radarX, float radarY, int x, int y, float pixSizeH, float pixSizeV)
{
	Vector3i v[4], tv[4], centre;
	int	shortX, longX, yDrop, yDropVar;
	int	dif = getDistanceAdjust();
	int	dif2 = getLengthAdjust();
	PIELIGHT colour;

	shortX = ((visibleTiles.x / 4) - (dif / 6)) * pixSizeH;
	longX = ((visibleTiles.x / 2) - (dif / 4)) * pixSizeH;
	yDropVar = ((visibleTiles.y / 2) - (dif2 / 3)) * pixSizeV;
	yDrop = ((visibleTiles.y / 2) - dif2 / 3) * pixSizeV;

 	v[0].x = longX;
	v[0].y = -yDropVar;

	v[1].x = -longX;
	v[1].y = -yDropVar;

	v[2].x = shortX;
	v[2].y = yDrop;

	v[3].x = -shortX;
	v[3].y = yDrop;

	centre.x = radarX + x - scrollMinX*pixSizeH/2;
	centre.y = radarY + y - scrollMinY*pixSizeV/2;

	RotateVector2D(v,tv,&centre,player.r.y,4);

	switch (getCampaignNumber())
	{
	case 1:
	case 2:
		// white
		colour.byte.r = UBYTE_MAX;
		colour.byte.g = UBYTE_MAX;
		colour.byte.b = UBYTE_MAX;
		colour.byte.a = 0x3f;
		break;
	case 3:
		// greenish
		colour.byte.r = 0x3f;
		colour.byte.a = 0x3f;
		colour.byte.g = UBYTE_MAX;
		colour.byte.b = 0x3f;
	default:
		// black
		colour.rgba = 0;
		colour.byte.a = 0x3f;
		break;

	}

	/* Send the four points to the draw routine and the clip box params */
	pie_DrawViewingWindow(tv, radarX, radarY, radarX + radarWidth, radarY + radarHeight, colour);
}

static void DrawRadarExtras(float radarX, float radarY, float pixSizeH, float pixSizeV)
{
	int	viewX = (player.p.x / TILE_UNITS) * pixSizeH;
	int	viewY = (player.p.z / TILE_UNITS) * pixSizeV;

	drawViewingWindow(radarX, radarY, viewX, viewY, pixSizeH, pixSizeV);
	RenderWindowFrame(FRAME_RADAR, radarX - 1, radarY - 1, radarWidth + 2, radarHeight + 2);
}

/** Does a screen coordinate lie within the radar area? */
bool CoordInRadar(int x,int y)
{
	Vector2f pos;
	pos.x = x - radarCenterX;
	pos.y = y - radarCenterY;
	if (rotateRadar)
	{
		pos = Vector2f_Rotate2f(pos, -player.r.y);
	}
	pos.x += radarWidth/2.0;
	pos.y += radarHeight/2.0;

	if (pos.x<0 || pos.y<0 || pos.x>=radarWidth || pos.y>=radarHeight)
	{
		return false;
	}
	return true;
}

void radarColour(UDWORD tileNumber, uint8_t r, uint8_t g, uint8_t b)
{
	tileColours[tileNumber].byte.r = r;
	tileColours[tileNumber].byte.g = g;
	tileColours[tileNumber].byte.b = b;
	tileColours[tileNumber].byte.a = 255;
}
