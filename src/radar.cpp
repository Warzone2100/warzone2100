/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
#include "lib/framework/math_ext.h"
#include "lib/ivis_opengl/pieblitfunc.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piefunc.h"
#include "lib/ivis_opengl/bitimage.h"
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
#include <glm/gtx/transform.hpp>

#define HIT_NOTIFICATION	(GAME_TICKS_PER_SEC * 2)
#define RADAR_FRAME_SKIP	10

bool bEnemyAllyRadarColor = false;     			/**< Enemy/ally radar color. */
RADAR_DRAW_MODE	radarDrawMode = RADAR_MODE_DEFAULT;	/**< Current mini-map mode. */
bool rotateRadar; ///< Rotate the radar?

static PIELIGHT		colRadarAlly, colRadarMe, colRadarEnemy;
static PIELIGHT		tileColours[MAX_TILES];
static UDWORD		*radarBuffer = NULL;
static Vector3i		playerpos;

PIELIGHT clanColours[] =
{
	// see frontend2.png for team color order.
	// [r,g,b,a]
	{{0, 255, 0, 255}},		// green  Player 0
	{{255, 192, 40, 255}},          // orange Player 1
	{{255, 255, 255, 255}},	// grey   Player 2
	{{0, 0, 0, 255}},			// black  Player 3
	{{255, 0, 0, 255}},		// red    Player 4
	{{20, 20, 255, 255}},		// blue   Player 5
	{{255, 0, 192, 255}},           // pink   Player 6
	{{0, 255, 255, 255}},		// cyan   Player 7
	{{255, 255, 0, 255}},           // yellow Player 8
	{{144, 0, 255, 255}},           // purple Player 9
	{{200, 255, 255, 255}},         // white  Player A (Should be brighter than grey, but grey is already maximum.)
	{{128, 128, 255, 255}},         // bright blue Player B
	{{128, 255, 128, 255}},         // neon green  Player C
	{{128, 0, 0, 255}},             // infrared    Player D
	{{64, 0, 128, 255}},            // ultraviolet Player E
	{{128, 128, 0, 255}},           // brown       Player F
};

static PIELIGHT flashColours[] =
{
	//right now the flash color is all bright red
	{{254, 37, 37, 200}},	// Player 0
	{{254, 37, 37, 200}},	// Player 1
	{{254, 37, 37, 200}},	// Player 2
	{{254, 37, 37, 200}},	// Player 3
	{{254, 37, 37, 200}},	// Player 4  (notice, brighter red)
	{{254, 37, 37, 200}},	// Player 5
	{{254, 37, 37, 200}},	// Player 6
	{{254, 37, 37, 200}},   // Player 7
	{{254, 37, 37, 200}},   // Player 8
	{{254, 37, 37, 200}},   // Player 9
	{{254, 37, 37, 200}},   // Player A
	{{254, 37, 37, 200}},   // Player B
	{{254, 37, 37, 200}},   // Player C
	{{254, 37, 37, 200}},   // Player D
	{{254, 37, 37, 200}},   // Player E
	{{254, 37, 37, 200}},   // Player F
};

static SDWORD radarWidth, radarHeight, radarCenterX, radarCenterY, radarTexWidth, radarTexHeight;
static uint8_t RadarZoom;
static float RadarZoomMultiplier = 1.0f;
static UDWORD radarBufferSize = 0;
static int frameSkip = 0;

static void DrawRadarTiles();
static void DrawRadarObjects();
static void DrawRadarExtras(const glm::mat4 &modelViewProjectionMatrix);
static void DrawNorth(const glm::mat4 &modelViewProjectionMatrix);
static void setViewingWindow();

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

void radarInitVars()
{
	radarTexWidth = 0;
	radarTexHeight = 0;
	RadarZoom = DEFAULT_RADARZOOM;
	debug(LOG_WZ, "Resetting radar zoom to %u", RadarZoom);
	radarSize(RadarZoom);
	playerpos = Vector3i(-1, -1, -1);
}

bool InitRadar()
{
	// Ally/enemy/me colors
	colRadarAlly = WZCOL_YELLOW;
	colRadarEnemy = WZCOL_RED;
	colRadarMe = WZCOL_WHITE;
	if (mapWidth < 150)	// too small!
	{
		RadarZoom = pie_GetVideoBufferWidth() <= 640 ? 14 : DEFAULT_RADARZOOM * 2;
	}
	return true;
}

bool resizeRadar()
{
	if (radarBuffer)
	{
		free(radarBuffer);
	}
	radarTexWidth = scrollMaxX - scrollMinX;
	radarTexHeight = scrollMaxY - scrollMinY;
	radarBufferSize = radarTexWidth * radarTexHeight * sizeof(UDWORD);
	radarBuffer = (uint32_t *)malloc(radarBufferSize);
	memset(radarBuffer, 0, radarBufferSize);
	if (rotateRadar)
	{
		RadarZoomMultiplier = (float)std::max(RADWIDTH, RADHEIGHT) / std::max({radarTexWidth, radarTexHeight, 1});
	}
	else
	{
		RadarZoomMultiplier = 1.0f;
	}
	debug(LOG_WZ, "Setting radar zoom to %u", RadarZoom);
	radarSize(RadarZoom);
	pie_SetRadar(-radarWidth / 2.0 - 1, -radarHeight / 2.0 - 1, radarWidth, radarHeight,
	             radarTexWidth, radarTexHeight, rotateRadar || (RadarZoom % 16 != 0));
	setViewingWindow();

	return true;
}

bool ShutdownRadar()
{
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
	resizeRadar();
}

uint8_t GetRadarZoom()
{
	return RadarZoom;
}

/** Calculate the radar pixel sizes. Returns pixels per tile. */
static void CalcRadarPixelSize(float *SizeH, float *SizeV)
{
	*SizeH = (float)radarHeight / std::max(radarTexHeight, 1);
	*SizeV = (float)radarWidth / std::max(radarTexWidth, 1);
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
	pos.x += radarWidth / 2.0;
	pos.y += radarHeight / 2.0;

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
	sPosX = clip(sPosX, scrollMinX, scrollMaxX);
	sPosY = clip(sPosY, scrollMinY, scrollMaxY);

	*PosX = sPosX;
	*PosY = sPosY;
}

void drawRadar()
{
	float	pixSizeH, pixSizeV;

	CalcRadarPixelSize(&pixSizeH, &pixSizeV);

	ASSERT_OR_RETURN(, radarBuffer, "No radar buffer allocated");

	// Do not recalculate frustum window coordinates if position or zoom does not change
	if (playerpos.x != player.p.x || playerpos.y != player.p.y || playerpos.z != player.p.z)
	{
		setViewingWindow();
	}
	playerpos = player.p; // cache position

	if (frameSkip <= 0)
	{
		DrawRadarTiles();
		DrawRadarObjects();
		pie_DownLoadRadar(radarBuffer);
		frameSkip = RADAR_FRAME_SKIP;
	}
	frameSkip--;
	pie_SetRendMode(REND_ALPHA);
	glm::mat4 radarMatrix = glm::translate(radarCenterX, radarCenterY, 0);
	glm::mat4 orthoMatrix = glm::ortho(0.f, static_cast<float>(pie_GetVideoBufferWidth()), static_cast<float>(pie_GetVideoBufferHeight()), 0.f);
	if (rotateRadar)
	{
		// rotate the map
		radarMatrix *= glm::rotate(UNDEG(player.r.y), glm::vec3(0.f, 0.f, 1.f));
		DrawNorth(orthoMatrix * radarMatrix);
	}

	pie_RenderRadar(orthoMatrix * radarMatrix);
	DrawRadarExtras(orthoMatrix * radarMatrix * glm::translate(-radarWidth / 2.f - 1.f, -radarHeight / 2.f - 1.f, 0.f));
	drawRadarBlips(-radarWidth / 2.0 - 1, -radarHeight / 2.0 - 1, pixSizeH, pixSizeV, orthoMatrix * radarMatrix);
}

static void DrawNorth(const glm::mat4 &modelViewProjectionMatrix)
{
	iV_DrawImage(IntImages, RADAR_NORTH, -((radarWidth / 2.0) + iV_GetImageWidth(IntImages, RADAR_NORTH) + 1), -(radarHeight / 2.0), modelViewProjectionMatrix);
}

static PIELIGHT appliedRadarColour(RADAR_DRAW_MODE radarDrawMode, MAPTILE *WTile)
{
	PIELIGHT WScr = WZCOL_BLACK;	// squelch warning

	// draw radar on/off feature
	if (!getRevealStatus() && !TEST_TILE_VISIBLE(selectedPlayer, WTile))
	{
		return WZCOL_RADAR_BACKGROUND;
	}

	switch (radarDrawMode)
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
static void DrawRadarTiles()
{
	SDWORD	x, y;

	for (x = scrollMinX; x < scrollMaxX; x++)
	{
		for (y = scrollMinY; y < scrollMaxY; y++)
		{
			MAPTILE	*psTile = mapTile(x, y);
			size_t pos = radarTexWidth * (y - scrollMinY) + (x - scrollMinX);

			ASSERT(pos * sizeof(*radarBuffer) < radarBufferSize, "Buffer overrun");
			if (y == scrollMinY || x == scrollMinX || y == scrollMaxY - 1 || x == scrollMaxX - 1)
			{
				radarBuffer[pos] = WZCOL_BLACK.rgba;
				continue;
			}
			radarBuffer[pos] = appliedRadarColour(radarDrawMode, psTile).rgba;
		}
	}
}

/** Draw the droids and structure positions on the radar. */
static void DrawRadarObjects()
{
	UBYTE				clan;
	PIELIGHT			playerCol;
	PIELIGHT			flashCol;
	int				x, y;

	/* Show droids on map - go through all players */
	for (clan = 0; clan < MAX_PLAYERS; clan++)
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
		for (psDroid = apsDroidLists[clan]; psDroid != NULL; psDroid = psDroid->psNext)
		{
			if (psDroid->pos.x < world_coord(scrollMinX) || psDroid->pos.y < world_coord(scrollMinY)
			    || psDroid->pos.x >= world_coord(scrollMaxX) || psDroid->pos.y >= world_coord(scrollMaxY))
			{
				continue;
			}
			if (psDroid->visible[selectedPlayer]
			    || (bMultiPlayer && alliancesSharedVision(game.alliance)
			        && aiCheckAlliances(selectedPlayer, psDroid->player)))
			{
				int	x = psDroid->pos.x / TILE_UNITS;
				int	y = psDroid->pos.y / TILE_UNITS;
				size_t	pos = (x - scrollMinX) + (y - scrollMinY) * radarTexWidth;

				ASSERT(pos * sizeof(*radarBuffer) < radarBufferSize, "Buffer overrun");
				if (clan == selectedPlayer && gameTime - psDroid->timeLastHit < HIT_NOTIFICATION)
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
					playerCol = (aiCheckAlliances(selectedPlayer, clan) ? colRadarAlly : colRadarEnemy);
				}
			}
			else
			{
				//original 8-color mode
				playerCol = clanColours[getPlayerColour(clan)];
			}
			flashCol = flashColours[getPlayerColour(clan)];

			if (psStruct->visible[selectedPlayer]
			    || (bMultiPlayer && alliancesSharedVision(game.alliance)
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
		TVec->x = ((Vec->x * Cos + Vec->y * Sin) >> 16) + ox;
		TVec->y = ((Vec->y * Cos - Vec->x * Sin) >> 16) + oy;
		Vec++;
		TVec++;
	}
}

static SDWORD getDistanceAdjust(void)
{
	int dif = std::max<int>(MAXDISTANCE - getViewDistance(), 0);

	return dif / 100;
}

static SDWORD getLengthAdjust(void)
{
	const int pitch = 360 - (player.r.x / DEG_1);

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
static void setViewingWindow()
{
	float pixSizeH, pixSizeV;
	Vector3i v[4], tv[4], centre;
	int	shortX, longX, yDrop, yDropVar;
	int	dif = getDistanceAdjust();
	int	dif2 = getLengthAdjust();
	PIELIGHT colour;
	CalcRadarPixelSize(&pixSizeH, &pixSizeV);
	int x = player.p.x * pixSizeH / TILE_UNITS;
	int y = player.p.z * pixSizeV / TILE_UNITS;

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

	centre.x = x - scrollMinX * pixSizeH;
	centre.y = y - scrollMinY * pixSizeV;

	RotateVector2D(v, tv, &centre, player.r.y, 4);

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
		colour.byte.g = UBYTE_MAX;
		colour.byte.b = 0x3f;
		colour.byte.a = 0x3f;
		break;
	default:
		// black
		colour.rgba = 0;
		colour.byte.a = 0x3f;
		break;
	}

	/* Send the four points to the draw routine and the clip box params */
	pie_SetViewingWindow(tv, colour);
}

static void DrawRadarExtras(const glm::mat4 &modelViewProjectionMatrix)
{
	pie_DrawViewingWindow(modelViewProjectionMatrix);
	RenderWindowFrame(FRAME_RADAR, -1, -1, radarWidth + 2, radarHeight + 2, modelViewProjectionMatrix);
}

/** Does a screen coordinate lie within the radar area? */
bool CoordInRadar(int x, int y)
{
	Vector2f pos;
	pos.x = x - radarCenterX;
	pos.y = y - radarCenterY;
	if (rotateRadar)
	{
		pos = Vector2f_Rotate2f(pos, -player.r.y);
	}
	pos.x += radarWidth / 2.0;
	pos.y += radarHeight / 2.0;

	if (pos.x < 0 || pos.y < 0 || pos.x >= radarWidth || pos.y >= radarHeight)
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
