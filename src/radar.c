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
#include <string.h>

#include "lib/framework/frame.h"
#include "lib/ivis_common/piedef.h"
#include "lib/ivis_common/rendmode.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_common/piepalette.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/piefunc.h"
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

BOOL bEnemyAllyRadarColor = false;     			/**< Enemy/ally radar color. */
RADAR_DRAW_MODE	radarDrawMode = RADAR_MODE_DEFAULT;	/**< Current mini-map mode. */

static PIELIGHT		colRadarAlly, colRadarMe, colRadarEnemy;
static PIELIGHT		tileColours[MAX_TILES];
static UDWORD		*radarBuffer = NULL;

PIELIGHT clanColours[MAX_PLAYERS]=
{	// see frontend2.png for team color order.
	// [r,g,b,a]
	{{0,255,0,255}},		// green  Player 0
	{{255,210,40,255}},		// orange Player 1
	{{255,255,255,255}},	// grey   Player 2
	{{0,0,0,255}},			// black  Player 3
	{{255,0,0,255}},		// red    Player 4
	{{20,20,255,255}},		// blue   Player 5
	{{255,0,255,255}},		// pink   Player 6
	{{0,255,255,255}},		// cyan   Player 7
};

static PIELIGHT flashColours[MAX_PLAYERS]=
{	//right now the flash color is all bright red
	{{254,37,37,200}},	// Player 0
	{{254,37,37,200}},	// Player 1
	{{254,37,37,200}},	// Player 2
	{{254,37,37,200}},	// Player 3
	{{254,37,37,200}},	// Player 4  (notice, brighter red)
	{{254,37,37,200}},	// Player 5
	{{254,37,37,200}},	// Player 6
	{{254,37,37,200}}		// Player 7
};

static SDWORD radarWidth, radarHeight, radarX, radarY, radarTexWidth, radarTexHeight;
static float RadarZoom;
static UDWORD radarBufferSize = 0;

static void DrawRadarTiles(UDWORD *screen);
static void DrawRadarObjects(UDWORD *screen);
static void DrawRadarExtras(float pixSizeH, float pixSizeV);

static void radarSize(float zoom)
{
	radarWidth = radarTexWidth * zoom;
	radarHeight = radarTexHeight * zoom;
	radarX = pie_GetVideoBufferWidth() - BASE_GAP * 2 - MAX(radarHeight, radarWidth);
	radarY = pie_GetVideoBufferHeight() - BASE_GAP * 2 - MAX(radarWidth, radarHeight);
	debug(LOG_WZ, "radar=(%u,%u) tex=(%u,%u) size=(%u,%u)", radarX, radarY, radarTexWidth, radarTexHeight, radarWidth, radarHeight);
}

void radarInitVars(void)
{
	radarTexWidth = 0;
	radarTexHeight = 0;
	RadarZoom = 1.0f;
	debug(LOG_WZ, "Resetting radar zoom to %f", RadarZoom);
	radarSize(RadarZoom);
}

//called for when a new mission is started
void resetRadarRedraw(void)
{
	// nothing here now
}

BOOL InitRadar(void)
{
	// Ally/enemy/me colors
	colRadarAlly = WZCOL_YELLOW;
	colRadarEnemy = WZCOL_RED;
	colRadarMe = WZCOL_WHITE;

	pie_InitRadar();

	return true;
}

BOOL resizeRadar(void)
{
	if (radarBuffer)
	{
		free(radarBuffer);
	}
	radarTexWidth = scrollMaxX - scrollMinX;
	radarTexHeight = scrollMaxY - scrollMinY;
	radarBufferSize = radarTexWidth * radarTexHeight * sizeof(UDWORD);
	radarBuffer = malloc(radarBufferSize);
	if (radarBuffer == NULL)
	{
		debug(LOG_ERROR, "Out of memory!");
		abort();
		return false;
	}
	memset(radarBuffer, 0, radarBufferSize);
	RadarZoom = (float)MAX(RADWIDTH, RADHEIGHT) / (float)MAX(radarTexWidth, radarTexHeight);
	debug(LOG_WZ, "Setting radar zoom to %f", RadarZoom);
	radarSize(RadarZoom);

	return true;
}

BOOL ShutdownRadar(void)
{
	pie_ShutdownRadar();

	free(radarBuffer);
	radarBuffer = NULL;

	return true;
}

void SetRadarZoom(float ZoomLevel)
{
	if (ZoomLevel <= MAX_RADARZOOM && ZoomLevel >= MIN_RADARZOOM)
	{
		debug(LOG_WZ, "Setting radar zoom to %f from %f", ZoomLevel, RadarZoom);
		RadarZoom = ZoomLevel;
		radarSize(ZoomLevel);
	}
}

float GetRadarZoom(void)
{
	return RadarZoom;
}

// Calculate the radar pixel sizes. Returns pixels per tile.
//
static void CalcRadarPixelSize(float *SizeH, float *SizeV)
{
	*SizeH = (float)radarHeight / (float)radarTexHeight;
	*SizeV = (float)radarWidth / (float)radarTexWidth;
}

// Given a position within the radar, return a world coordinate.
//
void CalcRadarPosition(UDWORD mX, UDWORD mY, UDWORD *PosX, UDWORD *PosY)
{
	const int	posX = mX - radarX;		// pixel position within radar
	const int	posY = mY - radarY;
	int		sPosX, sPosY;
	float		pixSizeH, pixSizeV;

	if (mX < radarX || mY < radarY)
	{
		debug(LOG_ERROR, "clicked outside radar minimap (%u, %u)", mX, mY);
		*PosX = 0;
		*PosY = 0;
		return;
	}
	CalcRadarPixelSize(&pixSizeH, &pixSizeV);
	sPosX = (float)posX / pixSizeH;	// adjust for pixel size
	sPosY = (float)posY / pixSizeV;
	sPosX -= scrollMinX;		// adjust for scroll limits
	sPosY -= scrollMinY;

#if REALLY_DEBUG_RADAR
	debug(LOG_ERROR, "m=(%u,%u) radar=(%u,%u) pos(%u,%u), scroll=(%u-%u,%u-%u) sPos=(%u,%u), pixSize=(%f,%f)",
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
	static int frameSkip = 0;
	const int largerSize = MAX(radarWidth, radarHeight);
	const int offX = radarX - (largerSize - radarWidth) / 2;
	const int offY = radarY - (largerSize - radarHeight) / 2;

	ASSERT(radarBuffer, "No radar buffer allocated");
	if (!radarBuffer)
	{
		return;
	}

	CalcRadarPixelSize(&pixSizeH, &pixSizeV);

	if (frameSkip <= 0)
	{
		DrawRadarTiles(radarBuffer);
		DrawRadarObjects(radarBuffer);
		pie_DownLoadRadar(radarBuffer, radarTexWidth, radarTexHeight);
		frameSkip = RADAR_FRAME_SKIP;
	}
	frameSkip--;

	iV_TransBoxFill(offX, offY, offX + largerSize, offY + largerSize);
	pie_ClipBegin(radarX, radarY, radarX + radarWidth, radarY + radarHeight);
	pie_RenderRadar(radarX, radarY, radarWidth, radarHeight);
	DrawRadarExtras(pixSizeH, pixSizeV);
	drawRadarBlips(radarX, radarY, pixSizeH, pixSizeV);
	pie_ClipEnd();
}

static PIELIGHT appliedRadarColour(RADAR_DRAW_MODE radarDrawMode, MAPTILE *WTile)
{
	PIELIGHT WScr = WZCOL_BLACK;	// squelch warning

	switch(radarDrawMode)
	{
		case RADAR_MODE_TERRAIN:
		{
			// draw radar terrain on/off feature
			PIELIGHT col = tileColours[TileNumber_tile(WTile->texture)];

			col.byte.r = sqrtf(col.byte.r * WTile->illumination);
			col.byte.b = sqrtf(col.byte.b * WTile->illumination);
			col.byte.g = sqrtf(col.byte.g * WTile->illumination);
			WScr = col;
		}
		break;
		case RADAR_MODE_COMBINED:
		{
			// draw radar terrain on/off feature
			PIELIGHT col = tileColours[TileNumber_tile(WTile->texture)];

			col.byte.r = sqrtf(col.byte.r * (WTile->illumination + WTile->height) / 2);
			col.byte.b = sqrtf(col.byte.b * (WTile->illumination + WTile->height) / 2);
			col.byte.g = sqrtf(col.byte.g * (WTile->illumination + WTile->height) / 2);
			WScr = col;
		}
		break;
		case RADAR_MODE_HEIGHT_MAP:
		{
			// draw radar terrain on/off feature
			WScr.byte.r = WScr.byte.g = WScr.byte.b = WTile->height;
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

// Draw the map tiles on the radar.
//
static void DrawRadarTiles(UDWORD *screen)
{
	SDWORD	x, y;

	for (x = scrollMinX; x < scrollMaxX; x++)
	{
		for (y = scrollMinY; y < scrollMaxY; y++)
		{
			UDWORD	*pScr = screen + radarTexWidth * y + x;
			MAPTILE	*psTile = mapTile(x, y);

			if (!getRevealStatus() || TEST_TILE_VISIBLE(selectedPlayer, psTile) || godMode)
			{
				*pScr = appliedRadarColour(radarDrawMode, psTile).rgba;
			} else {
				*pScr = WZCOL_RADAR_BACKGROUND.rgba;
			}
		}
	}
}

// Draw the droids and structure positions on the radar.
//
static void DrawRadarObjects(UDWORD *screen)
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
			playerCol = clanColours[getPlayerColour(clan)];
		}

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
			    || godMode
			    || (bMultiPlayer && game.alliance == ALLIANCES_TEAMS
			        && aiCheckAlliances(selectedPlayer,psDroid->player)))
			{
				int	x = psDroid->pos.x / TILE_UNITS;
   				int	y = psDroid->pos.y / TILE_UNITS;
				UDWORD	*Ptr = screen + x + y * radarTexWidth;

				if (clan == selectedPlayer && gameTime-psDroid->timeLastHit < HIT_NOTIFICATION)
				{
					*Ptr = flashCol.rgba;
				}
				else
				{
					*Ptr = playerCol.rgba;
				}
			}
   		}
   	}

   	/* Do the same for structures */
	for (x = scrollMinX; x < scrollMaxX; x++)
	{
		for (y = scrollMinY; y < scrollMaxY; y++)
		{
			UDWORD		*pScr = screen + y * radarTexWidth + x;
			MAPTILE		*psTile = mapTile(x, y);
			STRUCTURE	*psStruct = (STRUCTURE *)psTile->psObject;

			if (!TileHasStructure(psTile))
			{
				continue;
			}
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
			    || godMode
			    || (bMultiPlayer && game.alliance == ALLIANCES_TEAMS
			        && aiCheckAlliances(selectedPlayer, psStruct->player)))
			{
				if (clan == selectedPlayer && gameTime - psStruct->timeLastHit < HIT_NOTIFICATION)
				{
					*pScr = flashCol.rgba;
				}
				else
				{
					*pScr = playerCol.rgba;
				}
			}
   		}
   	}
}

// Rotate an array of 2d vectors about a given angle, also translates them after rotating.
//
static void RotateVector2D(Vector3i *Vector, Vector3i *TVector, Vector3i *Pos, int Angle, int Count)
{
	int Cos = COS(Angle);
	int Sin = SIN(Angle);
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
		TVec->x = ( (Vec->x*Cos + Vec->y*Sin) >> FP12_SHIFT ) + ox;
		TVec->y = ( (Vec->y*Cos - Vec->x*Sin) >> FP12_SHIFT ) + oy;
		Vec++;
		TVec++;
	}
}

static SDWORD getDistanceAdjust( void )
{
	UDWORD	origDistance = MAXDISTANCE;
	SDWORD	dif = MAX(origDistance - distance, 0);

	return dif / 100;
}

static SDWORD getLengthAdjust( void )
{
	SDWORD	pitch;
	UDWORD	lookingDown,lookingFar;
	SDWORD	dif;

	pitch = 360 - (player.r.x/DEG_1);

	// Max at
	lookingDown = (0 - MIN_PLAYER_X_ANGLE);
	lookingFar = (0 - MAX_PLAYER_X_ANGLE);
	dif = MAX(pitch - lookingFar, 0);
	if (dif > (lookingDown - lookingFar)) 
	{
		dif = (lookingDown - lookingFar);
	}

	return dif / 2;
}

/* Draws a Myth/FF7 style viewing window */
static void drawViewingWindow(UDWORD x, UDWORD y, float pixSizeH, float pixSizeV)
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

	centre.x = radarX + x + (visibleTiles.x * pixSizeH) / 2;
	centre.y = radarY + y + (visibleTiles.y * pixSizeV) / 2;

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

static void DrawRadarExtras(float pixSizeH, float pixSizeV)
{
	int	viewX = (player.p.x / TILE_UNITS) * pixSizeH;
	int	viewY = (player.p.z / TILE_UNITS) * pixSizeV;

	drawViewingWindow(viewX, viewY, pixSizeH, pixSizeV);
	RenderWindowFrame(FRAME_RADAR, radarX - 1, radarY - 1, radarWidth + 2, radarHeight + 2);
}

// Does a screen coordinate lie within the radar area?
//
BOOL CoordInRadar(int x,int y)
{
	if (x >= radarX - 1 && x < radarX + radarWidth + 1 && y >= radarY - 1 && y < radarY + radarHeight + 1)
	{
		return true;
	}

	return false;
}

void radarColour(UDWORD tileNumber, uint8_t r, uint8_t g, uint8_t b)
{
	tileColours[tileNumber].byte.r = r;
	tileColours[tileNumber].byte.g = g;
	tileColours[tileNumber].byte.b = b;
	tileColours[tileNumber].byte.a = 255;
}
