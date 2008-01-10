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
	Display3D.c - draws the 3D terrain view. Both the 3D and pseudo-3D components:-
	textured tiles.

	-------------------------------------------------------------------
	-	Alex McLean & Jeremy Sallis, Pumpkin Studios, EIDOS INTERACTIVE -
	-------------------------------------------------------------------
*/
/* Generic includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* Includes direct access to render library */
#include "lib/ivis_common/piedef.h"
#include "lib/ivis_common/tex.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/piepalette.h"
// FIXME Direct iVis implementation include!
#include "lib/ivis_opengl/piematrix.h"
#include "lib/ivis_common/piemode.h"
#include "lib/ivis_common/piefunc.h"
#include "lib/ivis_common/rendmode.h"
#include "e3demo.h"
#include "loop.h"
#include "atmos.h"
#include "raycast.h"
#include "levels.h"
#include "lib/framework/frame.h"
#include "map.h"
#include "move.h"
#include "visibility.h"
#include "geometry.h"
#include "lib/gamelib/gtime.h"
#include "resource.h"
#include "messagedef.h"
#include "miscimd.h"
#include "effects.h"
#include "edit3d.h"
#include "feature.h"
#include "hci.h"
#include "display.h"
#include "intdisplay.h"
#include "radar.h"
#include "display3d.h"
#include "lib/framework/fractions.h"
#include "lighting.h"
#include "console.h"
#include "lib/gamelib/animobj.h"
#include "projectile.h"
#include "bucket3d.h"
#include "intelmap.h"
#include "mapdisplay.h"
#include "message.h"
#include "component.h"
#include "warcam.h"
#include "lib/script/script.h"
#include "scripttabs.h"
#include "scriptextern.h"
#include "scriptcb.h"
#include "target.h"
#include "keymap.h"
#include "drive.h"
#include "fpath.h"
#include "gateway.h"
#include "transporter.h"
#include "warzoneconfig.h"
#include "lib/sound/audio.h"
#include "lib/sound/audio_id.h"
#include "action.h"
#include "keybind.h"
#include "combat.h"
#include "order.h"
#include "scores.h"
#include "multiplay.h"
#include "environ.h"
#include "advvis.h"
#include "texture.h"
#include "anim_id.h"
#include "cmddroid.h"

// HACK to be able to use static shadows for walls
// We just store a separate IMD for each direction
static iIMDShape otherDirections[3];
static BOOL directionSet[3] = {FALSE, FALSE, FALSE};

#define WATER_ALPHA_LEVEL 255 //was 164	// Amount to alpha blend water.
#define WATER_ZOFFSET 32		// Sorting offset for main water tile.
#define WATER_EDGE_ZOFFSET 64	// Sorting offset for water edge tiles.
#define WATER_DEPTH	127			// Amount to push terrain below water.

/********************  Prototypes  ********************/

static UDWORD	getTargettingGfx(void);
static void	drawDroidGroupNumber(DROID *psDroid);
static void	trackHeight(float desiredHeight);
static void	renderSurroundings(void);
static void	locateMouse(void);
static void preprocessTiles(void);
static BOOL	renderWallSection(STRUCTURE *psStructure);
static void	drawDragBox(void);
static void	calcFlagPosScreenCoords(SDWORD *pX, SDWORD *pY, SDWORD *pR);
static void	flipsAndRots(unsigned int tileNumber, unsigned int i, unsigned int j);
static void	displayTerrain(void);
static iIMDShape	*flattenImd(iIMDShape *imd, UDWORD structX, UDWORD structY, UDWORD direction);
static void	drawTiles(iView *camera, iView *player);
static void	display3DProjectiles(void);
static void	drawDroidSelections(void);
static void	drawStructureSelections(void);
static void	displayAnimation(ANIM_OBJECT * psAnimObj, BOOL bHoldOnFirstFrame);
static void	processSensorTarget(void);
static void	processDestinationTarget(void);
static BOOL	eitherSelected(DROID *psDroid);
static void	structureEffects(void);
static void	showDroidSensorRanges(void);
static void	showSensorRange2(BASE_OBJECT *psObj);
static void	drawRangeAtPos(SDWORD centerX, SDWORD centerY, SDWORD radius);
static void	addConstructionLine(DROID *psDroid, STRUCTURE *psStructure);
static void	doConstructionLines(void);
static void	drawDroidCmndNo(DROID *psDroid);
static void	drawDroidRank(DROID *psDroid);
static void	drawDroidSensorLock(DROID *psDroid);
static void	calcAverageTerrainHeight(iView *player);
BOOL	doWeDrawRadarBlips(void);
BOOL	doWeDrawProximitys(void);

static void drawTerrainTile(UDWORD i, UDWORD j, BOOL onWaterEdge);
static void drawTerrainWaterTile(UDWORD i, UDWORD j);

/********************  Variables  ********************/
// Should be cleaned up properly and be put in structures.

BOOL	bRender3DOnly;
static BOOL	bRangeDisplay = FALSE;
static SDWORD	rangeCenterX,rangeCenterY,rangeRadius;
static BOOL	bDrawBlips=TRUE;
static BOOL	bDrawProximitys=TRUE;
BOOL	godMode;

static float waterRealValue = 0.0f;
#define WAVE_SPEED 0.015f

// When to display HP bars
UWORD barMode;

/* Have we made a selection by clicking the mouse - used for dragging etc */
BOOL	selectAttempt = FALSE;

/* Vectors that hold the player and camera directions and positions */
iView	player;
static iView	camera;

/* Temporary rotation vectors to store rotations for droids etc */
static Vector3i	imdRot,imdRot2;

/* How far away are we from the terrain */
UDWORD		distance = START_DISTANCE;//(DISTANCE - (DISTANCE/6));

/* Stores the screen coordinates of the transformed terrain tiles */
static TERRAIN_VERTEX tileScreenInfo[LAND_YGRD][LAND_XGRD];

/* Records the present X and Y values for the current mouse tile (in tiles */
SDWORD mouseTileX, mouseTileY;

/* Do we want the radar to be rendered */
BOOL	radarOnScreen=FALSE;

/* Show unit/building gun/sensor range*/
bool rangeOnScreen = false;  // For now, most likely will change later!  -Q 5-10-05   A very nice effect - Per

/* Temporary values for the terrain render - top left corner of grid to be rendered */
static int playerXTile, playerZTile;

/* Have we located the mouse? */
static BOOL	mouseLocated = TRUE;

/* The box used for multiple selection - present screen coordinates */
UDWORD currentGameFrame;
static UDWORD numTiles = 0;
static SDWORD tileZ = 8000;
static QUAD dragQuad;

/* temporary buffer used for flattening IMDs */
static Vector3f alteredPoints[iV_IMD_MAX_POINTS];

//number of tiles visible
// FIXME This should become dynamic! (A function of resolution, angle and zoom maybe.)
const Vector2i visibleTiles = { VISIBLE_XTILES, VISIBLE_YTILES };

UDWORD	terrainMidX;
UDWORD	terrainMidY;

static unsigned int underwaterTile = WATER_TILE;
static unsigned int rubbleTile = BLOCKING_RUBBLE_TILE;

bool showFPS = false;       // default OFF, turn ON via console command 'showfps'
bool showSAMPLES = false;   // default OFF, turn ON via console command 'showsamples'
UDWORD geoOffset;

static int averageCentreTerrainHeight;

static UDWORD	lastTargetAssignation = 0;
static UDWORD	lastDestAssignation = 0;
static BOOL	bSensorTargetting = FALSE;
static BOOL	bDestTargetting = FALSE;
static BASE_OBJECT *psSensorObj = NULL;
static UDWORD	destTargetX,destTargetY;
static UDWORD	destTileX=0,destTileY=0;

#define	TARGET_TO_SENSOR_TIME	((4*(GAME_TICKS_PER_SEC))/5)
#define	DEST_TARGET_TIME	(GAME_TICKS_PER_SEC/4)
#define STRUCTURE_ANIM_RATE 4

/* Colour strobe values for the strobing drag selection box */
#define BOX_PULSE_SIZE  10

/********************  Functions  ********************/

static void displayMultiChat(void)
{
	UDWORD	pixelLength;
	UDWORD	pixelHeight;

	pixelLength = iV_GetTextWidth(sTextToSend);
	pixelHeight = iV_GetTextLineSize();

	if((gameTime2 % 500) < 250)
	{
		// implement blinking cursor in multiplayer chat
		pie_BoxFill(RET_X + pixelLength + 3, 474 + E_H - (pixelHeight/4), RET_X + pixelLength + 10, 473 + E_H, WZCOL_CURSOR);
	}

	/* FIXME: GET RID OF THE MAGIC NUMBERS BELOW */
	iV_TransBoxFill(RET_X + 1, 474 + E_H - pixelHeight, RET_X + 1 + pixelLength + 2, 473 + E_H);

	iV_DrawText(sTextToSend, RET_X + 3, 469 + E_H);
}

// Optimisation to stop it being calculated every frame
static SDWORD	gridCentreX,gridCentreZ,gridVarCalls;
SDWORD	getCentreX( void )
{
	gridVarCalls++;
	return(gridCentreX);
}

SDWORD	getCentreZ( void )
{
	return(gridCentreZ);
}

/* Render the 3D world */
void draw3DScene( void )
{
	BOOL bPlayerHasHQ = FALSE;

	// the world centre - used for decaying lighting etc
	gridCentreX = player.p.x + world_coord(visibleTiles.x / 2);
	gridCentreZ = player.p.z + world_coord(visibleTiles.y / 2);

	camera.p.z = distance;
	camera.p.y = 0;
	camera.p.x = 0;

	/* What frame number are we on? */
	currentGameFrame = frameGetFrameNumber();

	/* Build the drag quad */
	if(dragBox3D.status == DRAG_RELEASED)
	{
		dragQuad.coords[0].x = dragBox3D.x1; // TOP LEFT
		dragQuad.coords[0].y = dragBox3D.y1;

		dragQuad.coords[1].x = dragBox3D.x2; // TOP RIGHT
		dragQuad.coords[1].y = dragBox3D.y1;

		dragQuad.coords[2].x = dragBox3D.x2; // BOTTOM RIGHT
		dragQuad.coords[2].y = dragBox3D.y2;

		dragQuad.coords[3].x = dragBox3D.x1; // BOTTOM LEFT
		dragQuad.coords[3].y = dragBox3D.y2;
	}

	/* Calculate the position of the sun */
//	findSunVector();

	pie_Begin3DScene();
	/* Set 3D world origins */
	pie_SetGeometricOffset((rendSurface.width >> 1), geoOffset);

	// draw sky and fogbox
	renderSurroundings();

	// draw terrain
	displayTerrain();

	pie_BeginInterface();
	updateLightLevels();
	drawDroidSelections();

	drawStructureSelections();

	bPlayerHasHQ = getHQExists(selectedPlayer);

	if(radarOnScreen && bPlayerHasHQ)
	{
		pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
		pie_SetFogStatus(FALSE);
		drawRadar();
		if(doWeDrawRadarBlips())
		{
#ifndef RADAR_ROT
			drawRadarBlips();
#endif
		}
		pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);
		pie_SetFogStatus(TRUE);
	}

	if(!bRender3DOnly)
	{
		/* Ensure that any text messages are displayed at bottom of screen */
		pie_SetFogStatus(FALSE);
		displayConsoleMessages();
	}
	pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_OFF);
	pie_SetFogStatus(FALSE);
	iV_SetTextColour(WZCOL_TEXT_BRIGHT);

	/* Dont remove this folks!!!! */
	if(!bAllowOtherKeyPresses)
	{
		displayMultiChat();
	}
	if (showSAMPLES)		//Displays the number of sound samples we currently have
	{
		unsigned int width, height;
		const char *Qbuf, *Lbuf, *Abuf;

		sasprintf((char**)&Qbuf,"Que: %04u",audio_GetSampleQueueCount());
		sasprintf((char**)&Lbuf,"Lst: %04u",audio_GetSampleListCount());
		sasprintf((char**)&Abuf,"Act: %04u",sound_GetActiveSamplesCount());
		width = iV_GetTextWidth(Qbuf) + 11;
		height = iV_GetTextHeight(Qbuf);

		iV_DrawText(Qbuf, pie_GetVideoBufferWidth() - width, height + 2);
		iV_DrawText(Lbuf, pie_GetVideoBufferWidth() - width, height + 48);
		iV_DrawText(Abuf, pie_GetVideoBufferWidth() - width, height + 59);
	}
	if (showFPS)
	{
		unsigned int width, height;
		const char* fps;
		sasprintf((char**)&fps, "FPS: %u", frameGetAverageRate());
		width = iV_GetTextWidth(fps) + 10;
		height = iV_GetTextHeight(fps);

		iV_DrawText(fps, pie_GetVideoBufferWidth() - width, pie_GetVideoBufferHeight() - height);
	}
	if(getDebugMappingStatus() && !demoGetStatus() && !gamePaused())
	{
		iV_DrawText( "DEBUG ", RET_X + 134, 440 + E_H );
	}
	else
	{
#ifdef DEBUG
		if(!gamePaused())
		{
			char buildInfo[255];
			iV_DrawText( getLevelName(), RET_X + 134, 420 + E_H );
			getAsciiTime(buildInfo,gameTime);
			iV_DrawText( buildInfo, RET_X + 134, 434 + E_H );
		}
#endif
	}

	while(player.r.y>DEG(360))
	{
		player.r.y-=DEG(360);
	}

	/* If we don't have an active camera track, then track terrain height! */
	if(!getWarCamStatus())
	{
		/* Move the autonomous camera if necessary */
		calcAverageTerrainHeight(&player);
		trackHeight(averageCentreTerrainHeight+CAMERA_PIVOT_HEIGHT);
	}
	else
	{
		processWarCam();
	}

	if(demoGetStatus())
	{
		flushConsoleMessages();
		setConsolePermanence(TRUE, TRUE);
		permitNewConsoleMessages(TRUE);
		addConsoleMessage("Warzone 2100 : Pumpkin Studios ", RIGHT_JUSTIFY);
		permitNewConsoleMessages(FALSE);
	}

#ifdef ALEXM
	iV_DrawTextF(100, 200, "Skipped effects : %d", getNumSkippedEffects());
	iV_DrawTextF(100, 220, "Miss Count : %d", getMissCount());
	iV_DrawTextF(100, 240, "Even effects : %d", getNumEvenEffects());
#endif

	processDemoCam();
	processSensorTarget();
	processDestinationTarget();

	structureEffects(); // add fancy effects to structures

	showDroidSensorRanges(); //shows sensor data for units/droids/whatever...-Q 5-10-05

	//visualize radius if needed
	if (bRangeDisplay)
	{
		drawRangeAtPos(rangeCenterX,rangeCenterY,rangeRadius);
	}
}


/* Draws the 3D textured terrain */
static void displayTerrain(void)
{
	tileZ = 8000;

	/* We haven't yet located which tile mouse is over */
	mouseLocated = FALSE;

	numTiles = 0;

	pie_PerspectiveBegin();

	/* Setup tiles */
	preprocessTiles();

	/* Now, draw the terrain */
	drawTiles(&camera, &player);

	pie_PerspectiveEnd();

	/* Show the drag Box if necessary */
	drawDragBox();

	/* Have we released the drag box? */
	if(dragBox3D.status == DRAG_RELEASED)
	{
		dragBox3D.status = DRAG_INACTIVE;
	}
}

/***************************************************************************/
BOOL	doWeDrawRadarBlips( void )
{
	return(bDrawBlips);
}
/***************************************************************************/
BOOL	doWeDrawProximitys( void )
{
	return(bDrawProximitys);
}
/***************************************************************************/
void	setBlipDraw(BOOL val)
{
	bDrawBlips = val;
}
/***************************************************************************/
void	setProximityDraw(BOOL val)
{
	bDrawProximitys = val;
}
/***************************************************************************/

static void calcAverageTerrainHeight(iView *player)
{
	int numTilesAveraged = 0, i, j;

	/*	We track the height here - so make sure we get the average heights
		of the tiles directly underneath us
	*/
	averageCentreTerrainHeight = 0;
	for (i = VISIBLE_YTILES / 2 - 4; i < VISIBLE_YTILES / 2 + 4; i++)
	{
		for (j = VISIBLE_XTILES / 2 - 4; j < VISIBLE_XTILES / 2 + 4; j++)
		{
			if (tileOnMap(playerXTile + j, playerZTile + i))
			{
				/* Get a pointer to the tile at this location */
				MAPTILE *psTile = mapTile(playerXTile + j, playerZTile + i);

				averageCentreTerrainHeight += psTile->height * ELEVATION_SCALE;
				numTilesAveraged++;
			}
		}
	}
	/* Work out the average height. We use this information to keep the player camera
	 * above the terrain. */
	if (numTilesAveraged) // might not be if off map
	{
		MAPTILE *psTile = mapTile(playerXTile + VISIBLE_XTILES / 2, playerZTile + VISIBLE_YTILES / 2);

		averageCentreTerrainHeight /= numTilesAveraged;
		if (averageCentreTerrainHeight < psTile->height * ELEVATION_SCALE)
		{
			averageCentreTerrainHeight = psTile->height * ELEVATION_SCALE;
		}
	}
	else
	{
		averageCentreTerrainHeight = ELEVATION_SCALE * TILE_UNITS;
	}
}

PIELIGHT getTileColour(int x, int y)
{
	return mapTile(x, y)->colour;
}

void setTileColour(int x, int y, PIELIGHT colour)
{
	MAPTILE *psTile = mapTile(x, y);

	psTile->colour = colour;
}

static void drawTiles(iView *camera, iView *player)
{
	UDWORD i, j;
	SDWORD rx, rz;

	if (bDisplaySensorRange)
	{
		updateSensorDisplay();
	}

	// Animate the water texture, just cycles the V coordinate through half the tiles height.
	if(!gamePaused())
	{
		waterRealValue += timeAdjustedIncrement(WAVE_SPEED, FALSE);
		if (waterRealValue >= (1.0f / TILES_IN_PAGE_ROW) / 2)
		{
			waterRealValue = 0.0f;
		}
	}

	/* ---------------------------------------------------------------- */
	/* Do boundary and extent checking                                  */
	/* ---------------------------------------------------------------- */
	/* Get the mid point of the grid */
	terrainMidX = visibleTiles.x/2;
	terrainMidY = visibleTiles.y/2;

	/* Find our position in tile coordinates */
	playerXTile = map_coord(player->p.x);
	playerZTile = map_coord(player->p.z);

	/* Get the x,z translation components */
	rx = (player->p.x) & (TILE_UNITS-1);
	rz = (player->p.z) & (TILE_UNITS-1);

	/* ---------------------------------------------------------------- */
	/* Set up the geometry                                              */
	/* ---------------------------------------------------------------- */

	/* ---------------------------------------------------------------- */
	/* Push identity matrix onto stack */
	pie_MatBegin();

	// Now, scale the world according to what resolution we're running in
	pie_MatScale(pie_GetResScalingFactor());

	/* Set the camera position */
	pie_MATTRANS(camera->p.x, camera->p.y, camera->p.z);

	/* Rotate for the player */
	pie_MatRotZ(player->r.z);
	pie_MatRotX(player->r.x);
	pie_MatRotY(player->r.y);

	/* Translate */
	pie_TRANSLATE(-rx, -player->p.y, rz);

	if (getDrawShadows())
	{
		// this also detemines the length of the shadows
		pie_BeginLighting(&theSun);
	}

	/* ---------------------------------------------------------------- */
	/* Rotate and project all the tiles within the grid                 */
	/* ---------------------------------------------------------------- */
	for (i = 0; i < visibleTiles.y+1; i++)
	{
		/* Go through the x's */
		for (j = 0; j < (SDWORD)visibleTiles.x+1; j++)
		{
			Vector2i screen;
			PIELIGHT TileIllum = WZCOL_BLACK;
			int shiftVal = 0;

			tileScreenInfo[i][j].pos.x = world_coord(j - terrainMidX);
			tileScreenInfo[i][j].pos.z = world_coord(terrainMidY - i);
			tileScreenInfo[i][j].pos.y = 0;
			tileScreenInfo[i][j].bWater = FALSE;

			if( playerXTile+j < 0 ||
				playerZTile+i < 0 ||
				playerXTile+j > (SDWORD)(mapWidth-1) ||
				playerZTile+i > (SDWORD)(mapHeight-1) )
			{
				// Special past-edge-of-map tiles
				tileScreenInfo[i][j].u = 0;
				tileScreenInfo[i][j].v = 0;
			}
			else
			{
				BOOL bEdgeTile = FALSE;
				MAPTILE *psTile = mapTile(playerXTile + j, playerZTile + i);
				BOOL pushedDown = FALSE;

				tileScreenInfo[i][j].pos.y = map_TileHeight(playerXTile + j, playerZTile + i);

				if (getRevealStatus() && !godMode)
				{
					TileIllum = pal_SetBrightness(psTile->level == UBYTE_MAX ? 1 : psTile->level);
				}
				else
				{
					TileIllum = pal_SetBrightness(psTile->illumination);
				}

				// Real fog of war - darken where we cannot see enemy units moving around
				if (bDisplaySensorRange && !psTile->activeSensor)
				{
					TileIllum.byte.r = TileIllum.byte.r / 2;
					TileIllum.byte.g = TileIllum.byte.g / 2;
					TileIllum.byte.b = TileIllum.byte.b / 2;
				}

				if ( playerXTile+j <= 1 ||
					 playerZTile+i <= 1 ||
					 playerXTile+j >= mapWidth-2 ||
					 playerZTile+i >= mapHeight-2 )
				{
					bEdgeTile = TRUE;
				}

				// If it's the main water tile (has water texture) then..
				if (TileNumber_tile(psTile->texture) == WATER_TILE && !bEdgeTile)
				{
					// Push the terrain down for the river bed.
					shiftVal = WATER_DEPTH + environGetData(playerXTile+j, playerZTile+i) * 1.5f;
					tileScreenInfo[i][j].pos.y -= shiftVal;
					// And darken it.
					TileIllum.byte.r = (TileIllum.byte.r * 2) / 3;
					TileIllum.byte.g = (TileIllum.byte.g * 2) / 3;
					TileIllum.byte.b = (TileIllum.byte.b * 2) / 3;
					pushedDown = TRUE;
				}

				// If it's any water tile..
				if (terrainType(psTile) == TER_WATER)
				{
					// If it's the main water tile then bring it back up because it was pushed down for the river bed calc.
					int tmp_y = tileScreenInfo[i][j].pos.y;

					if (pushedDown)
					{
						tileScreenInfo[i][j].pos.y += shiftVal;
					}

					tileScreenInfo[i][j].bWater = TRUE;
					tileScreenInfo[i][j].water_height = tileScreenInfo[i][j].pos.y;
					tileScreenInfo[i][j].pos.y = tmp_y;
				}
				else
				{
					// If it wasnt a water tile then need to ensure water.xyz are valid because
					// a water tile might be sharing verticies with it.
					tileScreenInfo[i][j].water_height = tileScreenInfo[i][j].pos.y;
				}
				setTileColour(playerXTile + j, playerZTile + i, TileIllum);
			}
			// hack since tileScreenInfo[i][j].screen is Vector3i and pie_RotateProject takes Vector2i as 2nd param
			screen.x = tileScreenInfo[i][j].screen.x;
			screen.y = tileScreenInfo[i][j].screen.y;
			tileScreenInfo[i][j].screen.z = pie_RotateProject(&tileScreenInfo[i][j].pos, &screen);
			tileScreenInfo[i][j].screen.x = screen.x;
			tileScreenInfo[i][j].screen.y = screen.y;
		}
	}

	/* This is done here as effects can light the terrain - pause mode problems though */
	processEffects();
	atmosUpdateSystem();

	if(getRevealStatus())
	{
		avUpdateTiles();
	}

	// Draw all the normal tiles
	pie_SetAlphaTest(FALSE);
	pie_SetFogStatus(TRUE);
	pie_SetTexturePage(terrainPage);
	for (i = 0; i < MIN(visibleTiles.y, mapHeight); i++)
	{
		for (j = 0; j < MIN(visibleTiles.x, mapWidth); j++)
		{
			//get distance of furthest corner
			int zMax = MAX(tileScreenInfo[i][j].screen.z, tileScreenInfo[i+1][j].screen.z);

			zMax = MAX(zMax, tileScreenInfo[i + 1][j + 1].screen.z);
			zMax = MAX(zMax, tileScreenInfo[i][j + 1].screen.z);

			if (zMax < 0)
			{
				// clipped
				continue;
			}

			drawTerrainTile(i, j, FALSE);
		}
	}
	pie_DrawTerrainDone(MIN(visibleTiles.x, mapWidth),  MIN(visibleTiles.y, mapHeight));

	// Draw water edges
	pie_SetDepthOffset(-2.0);
	pie_SetAlphaTest(TRUE);
	for (i = 0; i < MIN(visibleTiles.y, mapHeight); i++)
	{
		for (j = 0; j < MIN(visibleTiles.x, mapWidth); j++)
		{
			// check if we need to draw a water edge
			if (tileScreenInfo[i][j].bWater
			    && TileNumber_tile(mapTile(playerXTile + j, playerZTile + i)->texture) != WATER_TILE)
			{
				//get distance of furthest corner
				int zMax = MAX(tileScreenInfo[i][j].screen.z, tileScreenInfo[i + 1][j].screen.z);
				zMax = MAX(zMax, tileScreenInfo[i + 1][j + 1].screen.z);
				zMax = MAX(zMax, tileScreenInfo[i][j + 1].screen.z);

				if (zMax < 0)
				{
					// clipped
					continue;
				}

				// the edge is in front of the water (which is drawn at z-index -1)
				drawTerrainTile(i, j, TRUE);
			}
		}
	}

	// Now draw the water tiles in a second pass to get alpha sort order correct
	pie_TranslateTextureBegin(Vector2f_New(0.0f, waterRealValue));
	pie_SetRendMode(REND_ALPHA_TEX);
	pie_SetAlphaTest(FALSE);
	pie_SetDepthOffset(-1.0f);
	for (i = 0; i < MIN(visibleTiles.y, mapHeight); i++)
	{
		for (j = 0; j < MIN(visibleTiles.x, mapWidth); j++)
		{
			if (tileScreenInfo[i][j].bWater)
			{
				//get distance of furthest corner
				int zMax = MAX(tileScreenInfo[i][j].screen.z, tileScreenInfo[i + 1][j].screen.z);
				zMax = MAX(zMax, tileScreenInfo[i + 1][j + 1].screen.z);
				zMax = MAX(zMax, tileScreenInfo[i][j + 1].screen.z);

				if (zMax < 0)
				{
					// clipped
					continue;
				}

				drawTerrainWaterTile(i, j);
			}
		}
	}
	pie_SetDepthOffset(0.0f);
	pie_SetRendMode(REND_GOURAUD_TEX);
	pie_SetAlphaTest(TRUE);
	pie_TranslateTextureEnd();

	targetOpenList((BASE_OBJECT*)driveGetDriven());

	/* ---------------------------------------------------------------- */
	/* Now display all the static objects                               */
	/* ---------------------------------------------------------------- */
	displayStaticObjects(); // bucket render implemented
	displayFeatures(); // bucket render implemented
	displayDynamicObjects(); //bucket render implemented
	if(doWeDrawProximitys())
	{
		displayProximityMsgs(); // bucket render implemented
	}
	displayDelivPoints(); // bucket render implemented
	display3DProjectiles(); // bucket render implemented

	atmosDrawParticles();

	bucketRenderCurrentList();
	pie_RemainingPasses();
	pie_EndLighting();

#ifdef ARROWS
	arrowDrawAll();
#endif

	targetCloseList();

	if(driveModeActive()) {
		// If were in driving mode then mark the current target.
		if(targetGetCurrent() != NULL) {
			targetMarkCurrent();
		}
	}
	if(!gamePaused())
	{
		doConstructionLines();
	}

	/* Clear the matrix stack */
	iV_MatrixEnd();
	locateMouse();
}


BOOL init3DView(void)
{
	// the world centre - used for decaying lighting etc
	gridCentreX = player.p.x + world_coord(visibleTiles.x / 2);
	gridCentreZ = player.p.z + world_coord(visibleTiles.y / 2);

	/* Base Level */
	geoOffset = 192;

	/* There are no drag boxes */
	dragBox3D.status = DRAG_INACTIVE;

	/* Arbitrary choice - from direct read! */
	theSun.x = 225.0f;
	theSun.y = -600.0f;
	theSun.z = 450.0f;

	/* Make sure and change these to comply with map.c */
	imdRot.x = -35;

	/* Get all the init stuff out of here? */
	initWarCam();

	/* Init the game messaging system */
	initConsoleMessages();

	atmosInitSystem();

	// Set the initial fog distance
	UpdateFogDistance(distance);

	initDemoCamera();

	/* Set up the sine table for the bullets */
	initBulletTable();

	/* No initial rotations */
	imdRot2.x = 0;
	imdRot.y = 0;
	imdRot2.z = 0;

	bRender3DOnly = FALSE;

	targetInitialise();

	memset(directionSet, FALSE, sizeof(directionSet));

	return TRUE;
}


// set the view position from save game
void disp3d_setView(iView *newView)
{
	memcpy(&player,newView,sizeof(iView));
}

// get the view position for save game
void disp3d_getView(iView *newView)
{
	memcpy(newView,&player,sizeof(iView));
}

/* John's routine - deals with flipping around the vertex ordering for source textures
when flips and rotations are being done */
static void flipsAndRots(unsigned int tileNumber, unsigned int i, unsigned int j)
{
	/* unmask proper values from compressed data */
	const unsigned short texture = TileNumber_texture(tileNumber);
	const unsigned short tile = TileNumber_tile(tileNumber);

	/* Used to calculate texture coordinates */
	const float xMult = 1.0f / TILES_IN_PAGE_COLUMN;
	const float yMult = 1.0f / TILES_IN_PAGE_ROW;
	const float one = 1.0f / (TILES_IN_PAGE_COLUMN * (float)getTextureSize());

	/*
	 * Points for flipping the texture around if the tile is flipped or rotated
	 * Store the source rect as four points
	 */
	Vector2f
		sP1 = { one, one },
		sP2 = { xMult - one, one },
		sP3 = { xMult - one, yMult - one },
		sP4 = { one, yMult - one },
		sPTemp;

	if (texture & TILE_XFLIP)
	{
		sPTemp = sP1;
		sP1 = sP2;
		sP2 = sPTemp;

		sPTemp = sP3;
		sP3 = sP4;
		sP4 = sPTemp;
	}
	if (texture & TILE_YFLIP)
	{
		sPTemp = sP1;
		sP1 = sP4;
		sP4 = sPTemp;
		sPTemp = sP2;
		sP2 = sP3;
		sP3 = sPTemp;
	}

	switch ((texture & TILE_ROTMASK) >> TILE_ROTSHIFT)
	{
		case 1:
			sPTemp = sP1;
			sP1 = sP4;
			sP4 = sP3;
			sP3 = sP2;
			sP2 = sPTemp;
			break;
		case 2:
			sPTemp = sP1;
			sP1 = sP3;
			sP3 = sPTemp;
			sPTemp = sP4;
			sP4 = sP2;
			sP2 = sPTemp;
			break;
		case 3:
			sPTemp = sP1;
			sP1 = sP2;
			sP2 = sP3;
			sP3 = sP4;
			sP4 = sPTemp;
			break;
	}

	tileScreenInfo[i + 0][j + 0].u = tileTexInfo[tile].uOffset + sP1.x;
	tileScreenInfo[i + 0][j + 0].v = tileTexInfo[tile].vOffset + sP1.y;

	tileScreenInfo[i + 0][j + 1].u = tileTexInfo[tile].uOffset + sP2.x;
	tileScreenInfo[i + 0][j + 1].v = tileTexInfo[tile].vOffset + sP2.y;

	tileScreenInfo[i + 1][j + 1].u = tileTexInfo[tile].uOffset + sP3.x;
	tileScreenInfo[i + 1][j + 1].v = tileTexInfo[tile].vOffset + sP3.y;

	tileScreenInfo[i + 1][j + 0].u = tileTexInfo[tile].uOffset + sP4.x;
	tileScreenInfo[i + 1][j + 0].v = tileTexInfo[tile].vOffset + sP4.y;
}


/* Clips anything - not necessarily a droid */
BOOL clipXY(SDWORD x, SDWORD y)
{
	if (x > (SDWORD)player.p.x &&  x < (SDWORD)(player.p.x+(visibleTiles.x*
		TILE_UNITS)) &&
		y > (SDWORD)player.p.z && y < (SDWORD)(player.p.z+(visibleTiles.y*TILE_UNITS)))
		return(TRUE);
	else
		return(FALSE);
}


/*	Get the onscreen corrdinates of a Object Position so we can draw a 'button' in
the Intelligence screen.  VERY similar to above function*/
static void	calcFlagPosScreenCoords(SDWORD *pX, SDWORD *pY, SDWORD *pR)
{
	/* Get it's absolute dimensions */
	Vector3i center3d = {0, 0, 0};
	Vector2i center2d = {0, 0};
	/* How big a box do we want - will ultimately be calculated using xmax, ymax, zmax etc */
	UDWORD	radius = 22;

	/* Pop matrices and get the screen coordinates for last point*/
	pie_RotateProject( &center3d, &center2d );

	/*store the coords*/
	*pX = center2d.x;
	*pY = center2d.y;
	*pR = radius;
}


/* Renders the bullets and their effects in 3D */
static void display3DProjectiles( void )
{
	PROJECTILE		*psObj;

	psObj = proj_GetFirst();

	while ( psObj != NULL )
	{
		switch(psObj->state)
		{
		case PROJ_INFLIGHT:
			// if source or destination is visible
			if(gfxVisible(psObj))
			{
				/* don't display first frame of trajectory (projectile on firing object) */
				if ( gameTime != psObj->born )
				{
					/* Draw a bullet at psObj->pos.x for X coord
										psObj->pos.y for Z coord
										whatever for Y (height) coord - arcing ?
					*/
					/* these guys get drawn last */
					if(psObj->psWStats->weaponSubClass == WSC_ROCKET ||
						psObj->psWStats->weaponSubClass == WSC_MISSILE ||
						psObj->psWStats->weaponSubClass == WSC_COMMAND ||
						psObj->psWStats->weaponSubClass == WSC_SLOWMISSILE ||
						psObj->psWStats->weaponSubClass == WSC_SLOWROCKET ||
						psObj->psWStats->weaponSubClass == WSC_ENERGY ||
						psObj->psWStats->weaponSubClass == WSC_EMP)
					{
						bucketAddTypeToList(RENDER_PROJECTILE, psObj);
					}
					else
					{
						renderProjectile(psObj);
					}
				}
			}
			break;

		case PROJ_IMPACT:
			break;

		case PROJ_POSTIMPACT:
			break;

		default:
			break;
		}	/* end switch */
		psObj = proj_GetNext();
	}
}	/* end of function display3DProjectiles */


void	renderProjectile(PROJECTILE *psCurr)
{
	WEAPON_STATS	*psStats;
	Vector3i			dv;
	iIMDShape		*pIMD;
	SDWORD			rx, rz;

	psStats = psCurr->psWStats;
	/* Reject flame or command since they have interim drawn fx */
	if(psStats->weaponSubClass == WSC_FLAME ||
		psStats->weaponSubClass == WSC_COMMAND || // || psStats->weaponSubClass == WSC_ENERGY)
		psStats->weaponSubClass == WSC_ELECTRONIC ||
		psStats->weaponSubClass == WSC_EMP ||
		(bMultiPlayer && psStats->weaponSubClass == WSC_LAS_SAT))
	{
		/* We don't do projectiles from these guys, cos there's an effect instead */
		return;
	}


	//the weapon stats holds the reference to which graphic to use
	/*Need to draw the graphic depending on what the projectile is doing - hitting target,
	missing target, in flight etc - JUST DO IN FLIGHT FOR NOW! */
	pIMD = psStats->pInFlightGraphic;

	if (clipXY(psCurr->pos.x,psCurr->pos.y))
	{
		/* Get bullet's x coord */
		dv.x = (psCurr->pos.x - player.p.x) - terrainMidX*TILE_UNITS;

		/* Get it's y coord (z coord in the 3d world */
		dv.z = terrainMidY*TILE_UNITS - (psCurr->pos.y - player.p.z);

		/* What's the present height of the bullet? */
		dv.y = psCurr->pos.z;
		/* Set up the matrix */
		iV_MatrixBegin();

		/* Translate to the correct position */
		iV_TRANSLATE(dv.x,dv.y,dv.z);
		/* Get the x,z translation components */
		rx = player.p.x & (TILE_UNITS-1);
		rz = player.p.z & (TILE_UNITS-1);

		/* Translate */
		iV_TRANSLATE(rx,0,-rz);

		/* Rotate it to the direction it's facing */
		imdRot2.y = DEG( (int)psCurr->direction );
		iV_MatrixRotateY(-imdRot2.y);

		/* pitch it */
		imdRot2.x = DEG(psCurr->pitch);
		iV_MatrixRotateX(imdRot2.x);

		if (psStats->weaponSubClass == WSC_ROCKET || psStats->weaponSubClass == WSC_MISSILE
		    || psStats->weaponSubClass == WSC_SLOWROCKET || psStats->weaponSubClass == WSC_SLOWMISSILE)
		{
			pie_Draw3DShape(pIMD, 0, 0, WZCOL_WHITE, WZCOL_BLACK, pie_ADDITIVE, 164);
		}
		else
		{
			pie_Draw3DShape(pIMD, 0, 0, WZCOL_WHITE, WZCOL_BLACK, pie_NO_BILINEAR, 0);
		}

		iV_MatrixEnd();
	}
	/* Flush matrices */
}

void
renderAnimComponent( const COMPONENT_OBJECT *psObj )
{
	BASE_OBJECT *psParentObj = (BASE_OBJECT*)psObj->psParent;
	const SDWORD posX = psParentObj->pos.x + psObj->position.x,
		posY = psParentObj->pos.y + psObj->position.y;
	SWORD rx, rz;

	ASSERT( psParentObj != NULL, "renderAnimComponent: invalid parent object pointer" );

	/* only draw visible bits */
	if( (psParentObj->type == OBJ_DROID) && !godMode && !demoGetStatus() &&
		((DROID*)psParentObj)->visible[selectedPlayer] != UBYTE_MAX )
	{
		return;
	}

	/* render */
	if( clipXY( posX, posY ) )
	{
		/* get parent object translation */
		const Vector3i dv = {
			(psParentObj->pos.x - player.p.x) - terrainMidX * TILE_UNITS,
			psParentObj->pos.z,
			terrainMidY * TILE_UNITS - (psParentObj->pos.y - player.p.z)
		};
		SDWORD iPlayer;
		PIELIGHT brightness;

		psParentObj->sDisplay.frameNumber = currentGameFrame;

		/* Push the indentity matrix */
		iV_MatrixBegin();

		/* parent object translation */
		iV_TRANSLATE(dv.x, dv.y, dv.z);

		/* Get the x,z translation components */
		rx = player.p.x & (TILE_UNITS-1);
		rz = player.p.z & (TILE_UNITS-1);

		/* Translate */
		iV_TRANSLATE(rx, 0, -rz);

		/* parent object rotations */
		imdRot2.y = DEG( (int)psParentObj->direction );
		iV_MatrixRotateY(-imdRot2.y);
		imdRot2.x = DEG(psParentObj->pitch);
		iV_MatrixRotateX(imdRot2.x);

		/* object (animation) translations - ivis z and y flipped */
		iV_TRANSLATE( psObj->position.x, psObj->position.z, psObj->position.y );

		/* object (animation) rotations */
		iV_MatrixRotateY( -psObj->orientation.z );
		iV_MatrixRotateZ( -psObj->orientation.y );
		iV_MatrixRotateX( -psObj->orientation.x );

		/* Set frame numbers - look into this later?? FIXME!!!!!!!! */
		if( psParentObj->type == OBJ_DROID )
		{
			DROID *psDroid = (DROID*)psParentObj;
			if ( psDroid->droidType == DROID_PERSON )
			{
				iPlayer = psParentObj->player - 6;
				pie_MatScale(75);
			}
			else
			{
				iPlayer = getPlayerColour(psParentObj->player);
			}

			/* Get the onscreen coordinates so we can draw a bounding box */
			calcScreenCoords( psDroid );
			targetAdd((BASE_OBJECT*)psDroid);
		}
		else
		{
			iPlayer = getPlayerColour(psParentObj->player);
		}

		//brightness and fog calculation
		if (psParentObj->type == OBJ_STRUCTURE)
		{
			const Vector3i zero = {0, 0, 0};
			Vector2i s = {0, 0};
			STRUCTURE *psStructure = (STRUCTURE*)psParentObj;

			brightness = pal_SetBrightness(200 - (100 - PERCENT(psStructure->body, structureBody(psStructure))));

			pie_RotateProject( &zero, &s );
			psStructure->sDisplay.screenX = s.x;
			psStructure->sDisplay.screenY = s.y;

			targetAdd((BASE_OBJECT*)psStructure);
		}
		else
		{
			brightness = pal_SetBrightness(UBYTE_MAX);
		}

		if(getRevealStatus() && !godMode)
		{
			brightness = pal_SetBrightness(avGetObjLightLevel((BASE_OBJECT*)psParentObj, brightness.byte.r));
		}

		pie_Draw3DShape(psObj->psShape, 0, iPlayer, brightness, WZCOL_BLACK, pie_NO_BILINEAR|pie_STATIC_SHADOW, 0);

		/* clear stack */
		iV_MatrixEnd();
	}
}


/* Draw the buildings */
void displayStaticObjects( void )
{
	STRUCTURE	*psStructure;
	UDWORD		clan;
	UDWORD		test = 0;
	ANIM_OBJECT	*psAnimObj;

	// to solve the flickering edges of baseplates
	pie_SetDepthOffset(-1.0f);

	/* Go through all the players */
	for (clan = 0; clan < MAX_PLAYERS; clan++)
	{
		/* Now go all buildings for that player */
		for(psStructure = apsStructLists[clan]; psStructure != NULL;
			psStructure = psStructure->psNext)
		{
			test++;
			/* Worth rendering the structure? */
			if(clipXY(psStructure->pos.x,psStructure->pos.y))
			{
				if ( psStructure->pStructureType->type == REF_RESOURCE_EXTRACTOR &&
					psStructure->psCurAnim == NULL &&
					(psStructure->currentBuildPts > (SDWORD)psStructure->pStructureType->buildPoints) )
				{
					psStructure->psCurAnim = animObj_Add( psStructure, ID_ANIM_DERIK, 0, 0 );
				}

				if ( psStructure->psCurAnim == NULL ||
						psStructure->psCurAnim->bVisible == FALSE ||
						(psAnimObj = animObj_Find( psStructure,
						psStructure->psCurAnim->uwID )) == NULL )
				{
					renderStructure(psStructure);
				}
				else
				{
					if ( psStructure->visible[selectedPlayer] || godMode )
					{
						//check not a resource extractors
						if (psStructure->pStructureType->type !=
							REF_RESOURCE_EXTRACTOR)
						{
							displayAnimation( psAnimObj, FALSE );
						}
						//check that a power gen exists before animationg res extrac
						//else if (getPowerGenExists(psStructure->player))
						/*check the building is active*/
						else if (psStructure->pFunctionality->resourceExtractor.active)
						{
							displayAnimation( psAnimObj, FALSE );
							if(selectedPlayer == psStructure->player)
							{
								audio_PlayObjStaticTrack( (void *) psStructure, ID_SOUND_OIL_PUMP_2 );
							}
						}
						else
						{
							/* hold anim on first frame */
							displayAnimation( psAnimObj, TRUE );
							audio_StopObjTrack( (void *) psStructure, ID_SOUND_OIL_PUMP_2 );
						}

					}
				}
			}
		}
	}
	pie_SetDepthOffset(0.0f);
}

//draw Factory Delivery Points
void displayDelivPoints(void)
{
	FLAG_POSITION	*psDelivPoint;

	//only do the selected players'
	/* go through all DPs for that player */
	for(psDelivPoint = apsFlagPosLists[selectedPlayer]; psDelivPoint != NULL;
		psDelivPoint = psDelivPoint->psNext)
	{
		if (clipXY(psDelivPoint->coords.x, psDelivPoint->coords.y))
		{
			renderDeliveryPoint(psDelivPoint);
		}
	}
}

/* Draw the features */
void displayFeatures( void )
{
FEATURE	*psFeature;
UDWORD		clan;

		/* player can only be 0 for the features */
		clan = 0;

		/* Go through all the features */
		for(psFeature = apsFeatureLists[clan]; psFeature != NULL;
			psFeature = psFeature->psNext)
		{
			/* Is the feature worth rendering? */
			if(clipXY(psFeature->pos.x,psFeature->pos.y))
			{
				renderFeature(psFeature);
			}
		}
}

/* Draw the Proximity messages for the **SELECTED PLAYER ONLY***/
void displayProximityMsgs( void )
{
	PROXIMITY_DISPLAY	*psProxDisp;
	VIEW_PROXIMITY		*pViewProximity;
	UDWORD				x, y;

	/* Go through all the proximity Displays*/
	for (psProxDisp = apsProxDisp[selectedPlayer]; psProxDisp != NULL;
		psProxDisp = psProxDisp->psNext)
	{
		if(!(psProxDisp->psMessage->read))
		{
			if (psProxDisp->type == POS_PROXDATA)
			{
				pViewProximity = (VIEW_PROXIMITY*)((VIEWDATA *)psProxDisp->psMessage->
					pViewData)->pData;
				x = pViewProximity->x;
				y = pViewProximity->y;
			}
			else
			{
				x = ((BASE_OBJECT *)psProxDisp->psMessage->pViewData)->pos.x;
				y = ((BASE_OBJECT *)psProxDisp->psMessage->pViewData)->pos.y;
			}
			/* Is the Message worth rendering? */
			if(clipXY(x,y))
			{
				renderProximityMsg(psProxDisp);
			}
		}
	}
}

static void displayAnimation( ANIM_OBJECT * psAnimObj, BOOL bHoldOnFirstFrame )
{
	UWORD i, uwFrame;
	Vector3i vecPos, vecRot, vecScale;
	COMPONENT_OBJECT *psComp;

	for ( i = 0; i < psAnimObj->psAnim->uwObj; i++ )
	{
		if ( bHoldOnFirstFrame == TRUE )
		{
			uwFrame = 0;
			vecPos.x = vecPos.y = vecPos.z = 0;
			vecRot.x = vecRot.y = vecRot.z = 0;
			vecScale.x = vecScale.y = vecScale.z = 0;
		}
		else
		{
			uwFrame = anim_GetFrame3D( psAnimObj->psAnim, i, gameTime, psAnimObj->udwStartTime, psAnimObj->udwStartDelay, &vecPos, &vecRot, &vecScale );
		}

		if ( uwFrame != ANIM_DELAYED )
		{
			if ( psAnimObj->psAnim->animType == ANIM_3D_TRANS )
			{
				psComp = &psAnimObj->apComponents[i];
			}
			else
			{
				psComp = &psAnimObj->apComponents[uwFrame];
			}

			psComp->position.x = vecPos.x;
			psComp->position.y = vecPos.y;
			psComp->position.z = vecPos.z;

			psComp->orientation.x = vecRot.x;
			psComp->orientation.y = vecRot.y;
			psComp->orientation.z = vecRot.z;

			bucketAddTypeToList( RENDER_ANIMATION, psComp );
		}
	}
}

/* Draw the droids */
void displayDynamicObjects( void )
{
	DROID		*psDroid;
	ANIM_OBJECT	*psAnimObj;
	UDWORD		clan;

	/* Need to go through all the droid lists */
	for(clan = 0; clan < MAX_PLAYERS; clan++)
	{
		for(psDroid = apsDroidLists[clan]; psDroid != NULL;
			psDroid = psDroid->psNext)
		{
			/* Find out whether the droid is worth rendering */
				if(clipXY(psDroid->pos.x,psDroid->pos.y))
				{
					/* No point in adding it if you can't see it? */
					if(psDroid->visible[selectedPlayer] || godMode || demoGetStatus())
					{
						psDroid->sDisplay.frameNumber = currentGameFrame;
						renderDroid( (DROID *) psDroid);
						/* draw anim if visible */
						if ( psDroid->psCurAnim != NULL &&
							psDroid->psCurAnim->bVisible == TRUE &&
							(psAnimObj = animObj_Find( psDroid,
							psDroid->psCurAnim->uwID )) != NULL )
						{
							displayAnimation( psAnimObj, FALSE );
						}
					}
				} // end clipDroid
		} // end for
	} // end for clan
} // end Fn

/* Sets the player's position and view angle - defaults player rotations as well */
void setViewPos( UDWORD x, UDWORD y, WZ_DECL_UNUSED BOOL Pan )
{
	SDWORD midX,midY;

	/* Find centre of grid thats actually DRAWN */
	midX = x-(visibleTiles.x/2);
	midY = y-(visibleTiles.y/2);

	player.p.x = midX*TILE_UNITS;
	player.p.z = midY*TILE_UNITS;
	player.r.z = 0;

	if(getWarCamStatus())
	{
		camToggleStatus();
	}

	scroll();
}

void getPlayerPos(SDWORD *px, SDWORD *py)
{
	*px = player.p.x + (visibleTiles.x/2)*TILE_UNITS;
	*py = player.p.z + (visibleTiles.y/2)*TILE_UNITS;
}

void setPlayerPos(SDWORD x, SDWORD y)
{
	SDWORD midX,midY;


	ASSERT( (x >= 0) && (x < (SDWORD)(mapWidth*TILE_UNITS)) &&
			(y >= 0) && (y < (SDWORD)(mapHeight*TILE_UNITS)),
		"setPlayerPos: position off map" );

	// Find centre of grid thats actually DRAWN
	midX = map_coord(x) - visibleTiles.x / 2;
	midY = map_coord(y) - visibleTiles.y / 2;

	player.p.x = midX*TILE_UNITS;
	player.p.z = midY*TILE_UNITS;
	player.r.z = 0;
}


void	setViewAngle(SDWORD angle)
{
	player.r.x = DEG(360 + angle);
}


UDWORD getViewDistance(void)
{
	return distance;
}

void	setViewDistance(UDWORD dist)
{
	dist = distance;
}


void	renderFeature(FEATURE *psFeature)
{
	UDWORD		featX,featY;
	SDWORD		rotation, rx, rz;
	PIELIGHT	brightness;
	Vector3i dv;
	Vector3f *vecTemp;
	BOOL bForceDraw = ( !getRevealStatus() && psFeature->psStats->visibleAtStart);
	int shadowFlags = 0;

	if (psFeature->visible[selectedPlayer] || godMode || demoGetStatus() || bForceDraw)
	{
		psFeature->sDisplay.frameNumber = currentGameFrame;
		/* Get it's x and y coordinates so we don't have to deref. struct later */
		featX = psFeature->pos.x;
		featY = psFeature->pos.y;
		/* Daft hack to get around the oild derrick issue */
		if (!TILE_HAS_FEATURE(mapTile(map_coord(featX), map_coord(featY))))
		{
			return;
		}
		dv.x = (featX - player.p.x) - terrainMidX*TILE_UNITS;
		dv.z = terrainMidY*TILE_UNITS - (featY - player.p.z);

		/* features sits at the height of the tile it's centre is on */
		dv.y = psFeature->pos.z;

		/* Push the indentity matrix */
		iV_MatrixBegin();

		/* Translate the feature  - N.B. We can also do rotations here should we require
		buildings to face different ways - Don't know if this is necessary - should be IMO */
		iV_TRANSLATE(dv.x,dv.y,dv.z);
		/* Get the x,z translation components */
		rx = player.p.x & (TILE_UNITS-1);
		rz = player.p.z & (TILE_UNITS-1);

		/* Translate */
		iV_TRANSLATE(rx,0,-rz);
		rotation = DEG( (int)psFeature->direction );

		iV_MatrixRotateY(-rotation);

		brightness = pal_SetBrightness(200); //? HUH?

		if(psFeature->psStats->subType == FEAT_SKYSCRAPER)
		{
			objectShimmy((BASE_OBJECT*)psFeature);
		}

		if(godMode || demoGetStatus() || bForceDraw)
		{
			brightness = pal_SetBrightness(200);
		}
		else if(getRevealStatus())
		{
			brightness = pal_SetBrightness(avGetObjLightLevel((BASE_OBJECT*)psFeature, brightness.byte.r));
		}

		if (psFeature->psStats->subType == FEAT_BUILDING || psFeature->psStats->subType == FEAT_SKYSCRAPER
		    || psFeature->psStats->subType == FEAT_OIL_DRUM)
		{
			// these cast a shadow
			shadowFlags = pie_STATIC_SHADOW;
		}

		if(psFeature->psStats->subType == FEAT_OIL_RESOURCE)
		{
			vecTemp = psFeature->sDisplay.imd->points;
			flattenImd(psFeature->sDisplay.imd, psFeature->pos.x, psFeature->pos.y, 0);
			/* currentGameFrame/2 set anim running - GJ hack */
			pie_Draw3DShape(psFeature->sDisplay.imd, currentGameFrame/2, 0, brightness, WZCOL_BLACK, 0, 0);
			psFeature->sDisplay.imd->points = vecTemp;
		}
		else
		{
			pie_Draw3DShape(psFeature->sDisplay.imd, 0, 0, brightness, WZCOL_BLACK, shadowFlags,0);
		}

		{
			Vector3i zero = {0, 0, 0};
			Vector2i s = {0, 0};

			pie_RotateProject( &zero, &s );
			psFeature->sDisplay.screenX = s.x;
			psFeature->sDisplay.screenY = s.y;

			targetAdd((BASE_OBJECT*)psFeature);
		}

		iV_MatrixEnd();
	}
}

void renderProximityMsg(PROXIMITY_DISPLAY *psProxDisp)
{
	UDWORD			msgX = 0, msgY = 0;
	Vector3i			dv = { 0, 0, 0 };
	VIEW_PROXIMITY	*pViewProximity = NULL;
	SDWORD			x, y, r, rx, rz;
	iIMDShape		*proxImd = NULL;

	//store the frame number for when deciding what has been clicked on
	psProxDisp->frameNumber = currentGameFrame;

	/* Get it's x and y coordinates so we don't have to deref. struct later */
	if (psProxDisp->type == POS_PROXDATA)
	{
		pViewProximity = (VIEW_PROXIMITY*)((VIEWDATA *)psProxDisp->psMessage->
			pViewData)->pData;
		if (pViewProximity)
		{
			msgX = pViewProximity->x;
			msgY = pViewProximity->y;
			/* message sits at the height specified at input*/
			dv.y = pViewProximity->z + 64;

			/* in case of a beacon message put above objects */
			if(((VIEWDATA *)psProxDisp->psMessage->pViewData)->type == VIEW_HELP)
			{
				if(TILE_OCCUPIED(mapTile(msgX / TILE_UNITS,msgY / TILE_UNITS)))
					dv.y = pViewProximity->z + 150;
			}

		}
	}
	else if (psProxDisp->type == POS_PROXOBJ)
	{
		msgX = ((BASE_OBJECT *)psProxDisp->psMessage->pViewData)->pos.x;
		msgY = ((BASE_OBJECT *)psProxDisp->psMessage->pViewData)->pos.y;
		/* message sits at the height specified at input*/
		dv.y = ((BASE_OBJECT *)psProxDisp->psMessage->pViewData)->pos.z + 64;
	}
	else
	{
		ASSERT(!"unknown proximity display message type", "Buggered proximity message type");
	}

	dv.x = (msgX - player.p.x) - terrainMidX*TILE_UNITS;
	dv.z = terrainMidY*TILE_UNITS - (msgY - player.p.z);

	/* Push the indentity matrix */
	iV_MatrixBegin();

	/* Translate the message */
	iV_TRANSLATE(dv.x,dv.y,dv.z);
	/* Get the x,z translation components */
	rx = player.p.x & (TILE_UNITS-1);
	rz = player.p.z & (TILE_UNITS-1);

	/* Translate */
	iV_TRANSLATE(rx,0,-rz);
	//get the appropriate IMD
	if (pViewProximity)
	{
		switch(pViewProximity->proxType)
		{
		case PROX_ENEMY:
			proxImd = getImdFromIndex(MI_BLIP_ENEMY);
			break;
		case PROX_RESOURCE:
			proxImd = getImdFromIndex(MI_BLIP_RESOURCE);
			break;
		case PROX_ARTEFACT:
			proxImd = getImdFromIndex(MI_BLIP_ARTEFACT);
			break;
		default:
			ASSERT(!"unknown proximity display message type", "Buggered proximity message type");
			break;
		}
	}
	else
	{
		//object Proximity displays are for oil resources and artefacts
		ASSERT( ((BASE_OBJECT *)psProxDisp->psMessage->pViewData)->type ==
			OBJ_FEATURE, "renderProximityMsg: invalid feature" );

		if (((FEATURE *)psProxDisp->psMessage->pViewData)->psStats->subType ==
			FEAT_OIL_RESOURCE)
		{
			//resource
			proxImd = getImdFromIndex(MI_BLIP_RESOURCE);
		}
		else
		{
			//artefact
			proxImd = getImdFromIndex(MI_BLIP_ARTEFACT);
		}
	}

	iV_MatrixRotateY(-player.r.y);
	iV_MatrixRotateX(-player.r.x);

	if(!gamePaused())
	{
		pie_Draw3DShape(proxImd, getTimeValueRange(1000,4), 0, WZCOL_WHITE, WZCOL_BLACK, pie_ADDITIVE, 192);
	}
	else
	{
		pie_Draw3DShape(proxImd, 0, 0, WZCOL_WHITE, WZCOL_BLACK, pie_ADDITIVE, 192);
	}

	//get the screen coords for determining when clicked on
	calcFlagPosScreenCoords(&x, &y, &r);
	psProxDisp->screenX = x;
	psProxDisp->screenY = y;
	psProxDisp->screenR = r;
	//storeProximityScreenCoords(psMessage, x, y);

	iV_MatrixEnd();
}

// Draw the structures
void	renderStructure(STRUCTURE *psStructure)
{
	SDWORD			structX, structY, rx, rz;
	SDWORD			rotation;
	SDWORD			frame;
	SDWORD			playerFrame;
	SDWORD			animFrame;
	UDWORD			nWeaponStat;
	PIELIGHT		buildingBrightness;
	Vector3i		dv;
	SDWORD			i;
	Vector3f		*temp = NULL;
	BOOL			bHitByElectronic = FALSE;
	BOOL			defensive = FALSE;
	iIMDShape		*strImd = psStructure->sDisplay.imd;

	if (psStructure->pStructureType->type == REF_WALL || psStructure->pStructureType->type == REF_WALLCORNER)
	{
		renderWallSection(psStructure);
		return;
	}
	else if (!psStructure->visible[selectedPlayer] && !godMode && !demoGetStatus())
	{
		return;
	}
	else if (psStructure->pStructureType->type == REF_DEFENSE)
	{
		defensive = TRUE;
	}

	playerFrame = getPlayerColour(psStructure->player);

	/* Power stations and factories have pulsing lights  */
	if (!defensive && psStructure->sDisplay.imd->numFrames > 0)
	{
		/*OK, so we've got a hack for a new structure - its a 2x2 wall but
		we've called it a BLAST_DOOR cos we don't want it to use the wallDrag code
		So its got clan colour trim and not really an anim - these HACKS just keep
		coming back to haunt us hey? - AB 02/09/99*/
		if (bMultiPlayer && psStructure->pStructureType->type == REF_BLASTDOOR)
		{
			animFrame = getPlayerColour( psStructure->player );
		}
		else
		{
			//calculate an animation frame
			animFrame = (gameTime % (STRUCTURE_ANIM_RATE * GAME_TICKS_PER_SEC)) / GAME_TICKS_PER_SEC;
		}
	}
	else if (!defensive)
	{
		animFrame = 0;
	} else {
		animFrame = playerFrame;
	}

	// -------------------------------------------------------------------------------

	/* Mark it as having been drawn */
	psStructure->sDisplay.frameNumber = currentGameFrame;

	/* Get it's x and y coordinates so we don't have to deref. struct later */
	structX = psStructure->pos.x;
	structY = psStructure->pos.y;

	if (defensive && strImd != NULL)
	{
		// Play with the imd so its flattened
		SDWORD strHeight;

		// Get a copy of the points
		memcpy(alteredPoints, strImd->points, strImd->npoints * sizeof(Vector3f));

		// Get the height of the centre point for reference
		strHeight = psStructure->pos.z;

		// Now we got through the shape looking for vertices on the edge
		for (i = 0; i < strImd->npoints; i++)
		{
			if (alteredPoints[i].y <= 0)
			{
				SDWORD pointHeight, shift;

				pointHeight = map_Height(structX + alteredPoints[i].x, structY - alteredPoints[i].z);
				shift = strHeight - pointHeight;
				alteredPoints[i].y -= shift;
			}
		}
	}

	dv.x = (structX - player.p.x) - terrainMidX * TILE_UNITS;
	dv.z = terrainMidY * TILE_UNITS - (structY - player.p.z);
	if (defensive)
	{
		dv.y = psStructure->pos.z;
	} else {
		dv.y = map_TileHeight(map_coord(structX), map_coord(structY));
	}
	/* Push the indentity matrix */
	iV_MatrixBegin();

	/* Translate the building  - N.B. We can also do rotations here should we require
	buildings to face different ways - Don't know if this is necessary - should be IMO */
	iV_TRANSLATE(dv.x,dv.y,dv.z);

	/* Get the x,z translation components */
	rx = player.p.x & (TILE_UNITS-1);
	rz = player.p.z & (TILE_UNITS-1);

	/* Translate */
	iV_TRANSLATE(rx, 0, -rz);

	/* OK - here is where we establish which IMD to draw for the building - luckily static objects,
	* buildings in other words are NOT made up of components - much quicker! */

	rotation = DEG((int)psStructure->direction);
	iV_MatrixRotateY(-rotation);
	if (!defensive
	    && gameTime2-psStructure->timeLastHit < ELEC_DAMAGE_DURATION
	    && psStructure->lastHitWeapon == WSC_ELECTRONIC )
	{
		bHitByElectronic = TRUE;
	}

	buildingBrightness = pal_SetBrightness(200 - (100 - PERCENT(psStructure->body, structureBody(psStructure))));

	/* If it's selected, then it's brighter */
	if (psStructure->selected)
	{
		SDWORD brightVar;

		if (!gamePaused())
		{
			brightVar = getStaticTimeValueRange(990, 110);
			if (brightVar > 55)
			{
				brightVar = 110 - brightVar;
			}
		}
		else
		{
			brightVar = 55;
		}
		buildingBrightness = pal_SetBrightness(200 + brightVar);
	}
	if (!godMode && !demoGetStatus() && getRevealStatus())
	{
		buildingBrightness = pal_SetBrightness(avGetObjLightLevel((BASE_OBJECT*)psStructure, buildingBrightness.byte.r));
	}

	if (!defensive)
	{
		/* Draw the building's base first */
		if (psStructure->pStructureType->pBaseIMD != NULL)
		{
			pie_Draw3DShape(psStructure->pStructureType->pBaseIMD, 0, 0, buildingBrightness, WZCOL_BLACK, 0,0);
		}

		// override
		if(bHitByElectronic)
		{
			buildingBrightness = pal_SetBrightness(150);
		}
	}

	if (bHitByElectronic)
	{
		objectShimmy((BASE_OBJECT *)psStructure);
	}

	//first check if partially built - ANOTHER HACK!
	if (psStructure->status == SS_BEING_BUILT
	    || psStructure->status == SS_BEING_DEMOLISHED
	    || (psStructure->status == SS_BEING_BUILT && psStructure->pStructureType->type == REF_RESOURCE_EXTRACTOR))
	{
		if (defensive)
		{
			temp = strImd->points;
			strImd->points = alteredPoints;
		}
		pie_Draw3DShape(strImd, 0, playerFrame, buildingBrightness, WZCOL_BLACK, pie_HEIGHT_SCALED | pie_SHADOW,
		                (SDWORD)(structHeightScale(psStructure) * pie_RAISE_SCALE));
		if (defensive)
		{
			strImd->points = temp;
		}
	}
	else if (psStructure->status == SS_BUILT)
	{
		if (defensive)
		{
			temp = strImd->points;
			strImd->points = alteredPoints;
		}
		pie_Draw3DShape(strImd, animFrame, 0, buildingBrightness, WZCOL_BLACK, pie_STATIC_SHADOW, 0);
		if (defensive)
		{
			strImd->points = temp;
		}

		// It might have weapons on it
		if (psStructure->sDisplay.imd->nconnectors > 0)
		{
			iIMDShape	*mountImd[STRUCT_MAXWEAPS];
			iIMDShape	*weaponImd[STRUCT_MAXWEAPS];
			iIMDShape	*flashImd[STRUCT_MAXWEAPS];

			for (i = 0; i < STRUCT_MAXWEAPS; i++)
			{
				weaponImd[i] = NULL;//weapon is gun ecm or sensor
				mountImd[i] = NULL;
				flashImd[i] = NULL;
			}
			//get an imd to draw on the connector priority is weapon, ECM, sensor
			//check for weapon
			if (psStructure->numWeaps > 0)
			{
				for (i = 0; i < psStructure->numWeaps; i++)
				{
					if (psStructure->asWeaps[i].nStat > 0)
					{
						nWeaponStat = psStructure->asWeaps[i].nStat;
						weaponImd[i] =  asWeaponStats[nWeaponStat].pIMD;
						mountImd[i] =  asWeaponStats[nWeaponStat].pMountGraphic;
						flashImd[i] =  asWeaponStats[nWeaponStat].pMuzzleGraphic;
					}
				}
			}
			else
			{
				if (psStructure->asWeaps[0].nStat > 0)
				{
					nWeaponStat = psStructure->asWeaps[0].nStat;
					weaponImd[0] =  asWeaponStats[nWeaponStat].pIMD;
					mountImd[0] =  asWeaponStats[nWeaponStat].pMountGraphic;
					flashImd[0] =  asWeaponStats[nWeaponStat].pMuzzleGraphic;
				}
			}

			if (weaponImd[0] == NULL)
			{
				//check for ECM
				if (psStructure->pStructureType->pECM != NULL)
				{
					weaponImd[0] =  psStructure->pStructureType->pECM->pIMD;
					mountImd[0] =  psStructure->pStructureType->pECM->pMountGraphic;
					flashImd[0] = NULL;
				}
			}

			if (weaponImd[0] == NULL)
			{
				//check for sensor
				if (psStructure->pStructureType->pSensor != NULL)
				{
					weaponImd[0] =  psStructure->pStructureType->pSensor->pIMD;
					/* No recoil for sensors */
					psStructure->asWeaps[0].recoilValue = 0;
					mountImd[0]  =  psStructure->pStructureType->pSensor->pMountGraphic;
					flashImd[0] = NULL;
				}
			}

			// draw Weapon / ECM / Sensor for structure
			for (i = 0; i < psStructure->numWeaps || i == 0; i++)
			{
				if (weaponImd[i] != NULL)
				{
					iV_MatrixBegin();
					iV_TRANSLATE(strImd->connectors[i].x, strImd->connectors[i].z, strImd->connectors[i].y);
					pie_MatRotY(DEG(-((SDWORD)psStructure->turretRotation[i])));
					if (mountImd[i] != NULL)
					{
						pie_TRANSLATE(0, 0, psStructure->asWeaps[i].recoilValue / 3);

						pie_Draw3DShape(mountImd[i], animFrame, 0, buildingBrightness, WZCOL_BLACK, pie_SHADOW, 0);
						if(mountImd[i]->nconnectors)
						{
							iV_TRANSLATE(mountImd[i]->connectors->x, mountImd[i]->connectors->z, mountImd[i]->connectors->y);
						}
					}
					iV_MatrixRotateX(DEG(psStructure->turretPitch[i]));
					pie_TRANSLATE(0, 0, psStructure->asWeaps[i].recoilValue);

					pie_Draw3DShape(weaponImd[i], playerFrame, 0, buildingBrightness, WZCOL_BLACK, pie_SHADOW,0);
					if (psStructure->pStructureType->type == REF_REPAIR_FACILITY)
					{
						REPAIR_FACILITY* psRepairFac = &psStructure->pFunctionality->repairFacility;
						// draw repair flash if the Repair Facility has a target which it has started work on
						if (weaponImd[i]->nconnectors && psRepairFac->psObj != NULL
						    && psRepairFac->psObj->type == OBJ_DROID
						    && ((DROID *)psRepairFac->psObj)->action == DACTION_WAITDURINGREPAIR )
						{
							iIMDShape	*pRepImd;

							iV_TRANSLATE(weaponImd[i]->connectors->x,weaponImd[i]->connectors->z-12,weaponImd[i]->connectors->y);
							pRepImd = getImdFromIndex(MI_FLAME);

							pie_MatRotY(DEG((SDWORD)psStructure->turretRotation[i]));

							iV_MatrixRotateY(-player.r.y);
							iV_MatrixRotateX(-player.r.x);
							pie_Draw3DShape(pRepImd, getStaticTimeValueRange(100,pRepImd->numFrames), 0, buildingBrightness, WZCOL_BLACK, pie_ADDITIVE, 192);

							iV_MatrixRotateX(player.r.x);
							iV_MatrixRotateY(player.r.y);
							pie_MatRotY(DEG((SDWORD)psStructure->turretRotation[i]));
						}
					}
					// we have a droid weapon so do we draw a muzzle flash
					else if( weaponImd[i]->nconnectors && psStructure->visible[selectedPlayer]>(UBYTE_MAX/2))
					{
						/* Now we need to move to the end of the barrel */
						pie_TRANSLATE(weaponImd[i]->connectors[i].x, weaponImd[i]->connectors[i].z, weaponImd[i]->connectors[i].y );
						// and draw the muzzle flash
						// animate for the duration of the flash only
						if (flashImd[i])
						{
							// assume no clan colours formuzzle effects
							if (flashImd[i]->numFrames == 0 || flashImd[i]->animInterval <= 0)
							{
								// no anim so display one frame for a fixed time
								if (gameTime < (psStructure->asWeaps[i].lastFired + BASE_MUZZLE_FLASH_DURATION))
								{
									pie_Draw3DShape(flashImd[i], 0, 0, buildingBrightness, WZCOL_BLACK, pie_ADDITIVE, 128);//muzzle flash
								}
							}
							else
							{
								frame = (gameTime - psStructure->asWeaps[i].lastFired)/flashImd[i]->animInterval;
								if (frame < flashImd[i]->numFrames)
								{
									pie_Draw3DShape(flashImd[i], frame, 0, buildingBrightness, WZCOL_BLACK, pie_ADDITIVE, 20);//muzzle flash
								}
							}
						}
					}
					iV_MatrixEnd();
				}
				// no IMD, its a baba machine gun, bunker, etc.
				else if (psStructure->asWeaps[i].nStat > 0)
				{
					flashImd[i] = NULL;
					// get an imd to draw on the connector priority is weapon, ECM, sensor
					// check for weapon
					nWeaponStat = psStructure->asWeaps[i].nStat;
					flashImd[i] =  asWeaponStats[nWeaponStat].pMuzzleGraphic;
					// draw Weapon/ECM/Sensor for structure
					if (flashImd[i] != NULL)
					{
						iV_MatrixBegin();
						// horrendous hack
						if (strImd->max.y > 80) // babatower
						{
							iV_TRANSLATE(0, 80, 0);
							pie_MatRotY(DEG(-((SDWORD)psStructure->turretRotation[i])));
							iV_TRANSLATE(0, 0, -20);
						}
						else//baba bunker
						{
							iV_TRANSLATE(0, 10, 0);
							pie_MatRotY(DEG(-((SDWORD)psStructure->turretRotation[i])));
							iV_TRANSLATE(0, 0, -40);
						}
						iV_MatrixRotateX(DEG(psStructure->turretPitch[i]));
						// and draw the muzzle flash
						// animate for the duration of the flash only
						// assume no clan colours formuzzle effects
						if (flashImd[i]->numFrames == 0 || flashImd[i]->animInterval <= 0)
						{
							// no anim so display one frame for a fixed time
							if (gameTime < psStructure->asWeaps[i].lastFired + BASE_MUZZLE_FLASH_DURATION)
							{
								pie_Draw3DShape(flashImd[i], 0, 0, buildingBrightness, WZCOL_BLACK, 0, 0); //muzzle flash
							}
						}
						else
						{
							frame = (gameTime - psStructure->asWeaps[i].lastFired) / flashImd[i]->animInterval;
							if (frame < flashImd[i]->numFrames)
							{
								pie_Draw3DShape(flashImd[i], 0, 0, buildingBrightness, WZCOL_BLACK, 0, 0); //muzzle flash
							}
						}
						iV_MatrixEnd();
					}
				}
				// if there is an unused connector, but not the first connector, add a light to it
				else if (psStructure->sDisplay.imd->nconnectors > 1)
				{
					for (i = 0; i < psStructure->sDisplay.imd->nconnectors; i++)
					{
						iIMDShape *lImd;

						iV_MatrixBegin();
						iV_TRANSLATE(psStructure->sDisplay.imd->connectors->x, psStructure->sDisplay.imd->connectors->z,
						             psStructure->sDisplay.imd->connectors->y);
						lImd = getImdFromIndex(MI_LANDING);
						pie_Draw3DShape(lImd, getStaticTimeValueRange(1024, lImd->numFrames), 0, buildingBrightness, WZCOL_BLACK, 0, 0);
						iV_MatrixEnd();
					}
				}
			}
		}
	}

	{
		Vector3i zero = { 0, 0, 0 };
		Vector2i s = { 0, 0 };

		pie_RotateProject(&zero, &s);
		psStructure->sDisplay.screenX = s.x;
		psStructure->sDisplay.screenY = s.y;

		targetAdd((BASE_OBJECT*)psStructure);
	}

	iV_MatrixEnd();
}

/*draw the delivery points */
void	renderDeliveryPoint(FLAG_POSITION *psPosition)
{
	Vector3i dv;
	SDWORD			x, y, r, rx, rz;
	Vector3f *temp = NULL;
	//store the frame number for when deciding what has been clicked on
	psPosition->frameNumber = currentGameFrame;

	dv.x = (psPosition->coords.x - player.p.x) - terrainMidX*TILE_UNITS;
	dv.z = terrainMidY*TILE_UNITS - (psPosition->coords.y - player.p.z);
	dv.y = psPosition->coords.z;

	/* Push the indentity matrix */
	iV_MatrixBegin();

	iV_TRANSLATE(dv.x,dv.y,dv.z);

	/* Get the x,z translation components */
	rx = player.p.x & (TILE_UNITS-1);
	rz = player.p.z & (TILE_UNITS-1);

	/* Translate */
	iV_TRANSLATE(rx,0,-rz);

	//quick check for invalid data
	ASSERT( psPosition->factoryType < NUM_FLAG_TYPES && psPosition->factoryInc < MAX_FACTORY, "Invalid assembly point" );

	if(!psPosition->selected)
	{
		temp = pAssemblyPointIMDs[psPosition->factoryType][psPosition->factoryInc]->points;
		flattenImd(pAssemblyPointIMDs[psPosition->factoryType][psPosition->factoryInc],
			psPosition->coords.x, psPosition->coords.y,0);
	}

	pie_MatScale(50); // they are all big now so make this one smaller too

	pie_Draw3DShape(pAssemblyPointIMDs[psPosition->factoryType][psPosition->factoryInc], 0, 0, WZCOL_WHITE, WZCOL_BLACK, pie_NO_BILINEAR, 0);

	if(!psPosition->selected)
	{
		pAssemblyPointIMDs[psPosition->factoryType][psPosition->factoryInc]->points = temp;
	}

	//get the screen coords for the DP
	calcFlagPosScreenCoords(&x, &y, &r);
	psPosition->screenX = x;
	psPosition->screenY = y;
	psPosition->screenR = r;

	iV_MatrixEnd();
}

static BOOL	renderWallSection(STRUCTURE *psStructure)
{
	SDWORD			structX, structY, rx, rz;
	PIELIGHT		brightness, specular = WZCOL_BLACK, buildingBrightness;
	iIMDShape		*imd;
	SDWORD			rotation;
	Vector3i			dv;
	UDWORD			i;
	Vector3f			*temp;
	iIMDShape *originalDirection = NULL;

	if(psStructure->visible[selectedPlayer] || godMode || demoGetStatus())
	{
		psStructure->sDisplay.frameNumber = currentGameFrame;
		/* Get it's x and y coordinates so we don't have to deref. struct later */
		structX = psStructure->pos.x;
		structY = psStructure->pos.y;
		buildingBrightness = pal_SetBrightness(200 - (100 - PERCENT(psStructure->body, structureBody(psStructure))));

		if(psStructure->selected)
		{
			SDWORD brightVar;

			if(!gamePaused())
			{
				brightVar = getStaticTimeValueRange(990,110);
				if(brightVar>55) brightVar = 110-brightVar;
			}
			else
			{
				brightVar = 55;
			}


			buildingBrightness = pal_SetBrightness(200 + brightVar);
		}

		if(godMode || demoGetStatus())
		{
			/* NOP */
		}
		else if(getRevealStatus())
		{
			buildingBrightness = pal_SetBrightness(avGetObjLightLevel((BASE_OBJECT*)psStructure, buildingBrightness.byte.r));
		}

		brightness = buildingBrightness;

		/*
		Right, now the tricky bit, we need to bugger about with the coordinates of the imd to make it
		fit tightly to the ground and to neighbours.
		*/
		imd = psStructure->pStructureType->pBaseIMD;
		if(imd != NULL)
		{
			UDWORD centreHeight;

			// Get a copy of the points
			memcpy(alteredPoints,imd->points,imd->npoints*sizeof(Vector3i));
			// Get the height of the centre point for reference
			centreHeight = map_Height(structX,structY);
			// Now we got through the shape looking for vertices on the edge
			for(i=0; i<(UDWORD)imd->npoints; i++)
			{
				UDWORD pointHeight;
				SDWORD shift;

				pointHeight = map_Height(structX+alteredPoints[i].x,structY-alteredPoints[i].z);
				shift = centreHeight - pointHeight;
				alteredPoints[i].y -= shift;
			}
		}
		/* Establish where it is in the world */
		dv.x = (structX - player.p.x) - terrainMidX*TILE_UNITS;
		dv.z = terrainMidY*TILE_UNITS - (structY - player.p.z);
		dv.y = map_Height(structX, structY);

		/* Push the indentity matrix */
		iV_MatrixBegin();

		/* Translate */
		iV_TRANSLATE(dv.x,dv.y,dv.z);

		/* Get the x,z translation components */
		rx = player.p.x & (TILE_UNITS-1);
		rz = player.p.z & (TILE_UNITS-1);

		/* Translate */
		iV_TRANSLATE(rx, 0, -rz);

		rotation = DEG( (int)psStructure->direction );
		iV_MatrixRotateY(-rotation);

		if(imd != NULL)
		{
			// Make the imd pointer to the vertex list point to ours
			temp = imd->points;
			imd->points = alteredPoints;
			// Actually render it
			pie_Draw3DShape(imd, 0, getPlayerColour(psStructure->player), brightness, specular, 0, 0);
			imd->points = temp;
		}

		imd = psStructure->sDisplay.imd;
		temp = imd->points;

		// now check if we need to apply the wall hack
		if ( psStructure->direction > 0 && psStructure->pStructureType->type == REF_WALL )
		{
			// switch them
			originalDirection = imd;
			imd = &otherDirections[(int)psStructure->direction / 90 - 1];
			if(!directionSet[(int)psStructure->direction / 90 - 1])
			{
				// not yet initialised, so do that now
				*imd = *originalDirection;
				imd->shadowEdgeList = NULL;
				directionSet[(int)psStructure->direction / 90 - 1] = TRUE;
			}
		}

		flattenImd(imd, structX, structY, psStructure->direction );

		/* Actually render it */
		if ( (psStructure->status == SS_BEING_BUILT ) ||
			(psStructure->status == SS_BEING_DEMOLISHED ) ||
			(psStructure->status == SS_BEING_BUILT && psStructure->pStructureType->type == REF_RESOURCE_EXTRACTOR) )
		{
			pie_Draw3DShape(psStructure->sDisplay.imd, 0, getPlayerColour(psStructure->player),
							brightness, specular, pie_HEIGHT_SCALED|pie_SHADOW,
							(SDWORD)(structHeightScale(psStructure) * pie_RAISE_SCALE) );
		}
		else if(psStructure->status == SS_BUILT)
		{
			pie_Draw3DShape(imd, 0, getPlayerColour(psStructure->player), brightness, specular, pie_STATIC_SHADOW, 0);
		}
		imd->points = temp;

		if ( psStructure->direction > 0 && psStructure->pStructureType->type == REF_WALL )
		{
			// switch back
			imd = originalDirection;
		}

		{
			Vector3i zero = {0, 0, 0};
			Vector2i s = {0, 0};

			pie_RotateProject( &zero, &s );
			psStructure->sDisplay.screenX = s.x;
			psStructure->sDisplay.screenY = s.y;
		}

		iV_MatrixEnd();

		return(TRUE);
	}
	return FALSE;
}

/* renderShadow: draws shadow under droid */
void renderShadow( DROID *psDroid, iIMDShape *psShadowIMD )
{
	Vector3i			dv;
	Vector3f			*pVecTemp;
	SDWORD			shadowScale, rx, rz;

	dv.x = (psDroid->pos.x - player.p.x) - terrainMidX*TILE_UNITS;
	if(psDroid->droidType == DROID_TRANSPORTER)
	{
		dv.x -= bobTransporterHeight()/2;
	}
	dv.z = terrainMidY*TILE_UNITS - (psDroid->pos.y - player.p.z);
	dv.y = map_Height(psDroid->pos.x, psDroid->pos.y);

	/* Push the indentity matrix */
	iV_MatrixBegin();

	iV_TRANSLATE(dv.x,dv.y,dv.z);

	/* Get the x,z translation components */
	rx = player.p.x & (TILE_UNITS-1);
	rz = player.p.z & (TILE_UNITS-1);

	/* Translate */
	iV_TRANSLATE(rx,0,-rz);

	if(psDroid->droidType == DROID_TRANSPORTER)
	{
		iV_MatrixRotateY( DEG( -psDroid->direction ) );
	}

	pVecTemp = psShadowIMD->points;
	if(psDroid->droidType == DROID_TRANSPORTER)
	{
		flattenImd( psShadowIMD, psDroid->pos.x, psDroid->pos.y, 0);
		shadowScale = 100-(psDroid->pos.z/100);
		if(shadowScale < 50) shadowScale = 50;
	}
	else
	{
		pie_MatRotY( DEG(-psDroid->direction ) );
		pie_MatRotX( DEG( psDroid->pitch ) );
		pie_MatRotZ( DEG( psDroid->roll ) );
	}

	pie_Draw3DShape(psShadowIMD, 0, 0, WZCOL_WHITE, WZCOL_BLACK, pie_TRANSLUCENT, 128);
	psShadowIMD->points = pVecTemp;

	iV_MatrixEnd();
}

/* Draw the droids */
void renderDroid( DROID *psDroid )
{
	displayComponentObject( (BASE_OBJECT *) psDroid);
	targetAdd((BASE_OBJECT*)psDroid);
}


/* Draws the strobing 3D drag box that is used for multiple selection */
static void	drawDragBox( void )
{
	int minX, maxX;		// SHURCOOL: These 4 ints will hold the corners of the selection box
	int minY, maxY;

	if(dragBox3D.status == DRAG_DRAGGING && buildState == BUILD3D_NONE)
	{
		if(gameTime - dragBox3D.lastTime > BOX_PULSE_SPEED)
		{
			dragBox3D.pulse++;
			if (dragBox3D.pulse >= BOX_PULSE_SIZE)
			{
				dragBox3D.pulse = 0;
			}
			dragBox3D.lastTime = gameTime;
		}

		// SHURCOOL: Determine the 4 corners of the selection box, and use them for consistent selection box rendering
		minX = MIN(dragBox3D.x1, mouseXPos);
		maxX = MAX(dragBox3D.x1, mouseXPos);
		minY = MIN(dragBox3D.y1, mouseYPos);
		maxY = MAX(dragBox3D.y1, mouseYPos);

		// SHURCOOL: Reduce the box in size to produce a (consistent) pulsing inward effect
		minX += dragBox3D.pulse / 2;
		maxX -= dragBox3D.pulse / 2;
		minY += dragBox3D.pulse / 2;
		maxY -= dragBox3D.pulse / 2;

		pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_OFF);
		iV_Box(minX, minY, maxX, maxY, WZCOL_UNIT_SELECT_BORDER);
		pie_UniTransBoxFill(minX + 1, minY + 1, maxX - 1, maxY - 1, WZCOL_UNIT_SELECT_BOX);
		pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);
	}
}


// display reload bars for structures and droids
// Watermelon:make it to accept additional int value weapon_slot
// this should fix the overlapped reloadbar problem
static void drawWeaponReloadBar(BASE_OBJECT *psObj, WEAPON *psWeap, int weapon_slot)
{
	WEAPON_STATS		*psStats;
	BOOL			bSalvo;
	UDWORD			firingStage, interval, damLevel;
	SDWORD			scrX,scrY, scrR, scale;
	STRUCTURE		*psStruct;
	float			mulH;	// display unit resistance instead of reload!
	DROID			*psDroid;

	if (ctrlShiftDown() && (psObj->type == OBJ_DROID))
	{
		psDroid = (DROID*)psObj;
		scrX = psObj->sDisplay.screenX;
		scrY = psObj->sDisplay.screenY;
		scrR = psObj->sDisplay.screenR;
		scrY += scrR + 2;

		if (psDroid->resistance)
		{
			mulH = (float)psDroid->resistance / (float)droidResistance(psDroid);
		}
		else
		{
			mulH = 100.f;
		}
		firingStage = mulH;
		firingStage = ((((2*scrR)*10000)/100)*firingStage)/10000;
		if(firingStage >= (UDWORD)(2*scrR))
		{
			firingStage = (2*scrR) - 1;
		}
		pie_BoxFill(scrX - scrR-1, 6+scrY + 0 + (weapon_slot * 5), scrX - scrR +(2*scrR),    6+scrY+3 + (weapon_slot * 5), WZCOL_RELOAD_BACKGROUND);
		pie_BoxFill(scrX - scrR,   6+scrY + 1 + (weapon_slot * 5), scrX - scrR +firingStage, 6+scrY+2 + (weapon_slot * 5), WZCOL_RELOAD_BAR);
		return;
	}

	if (psWeap->nStat == 0)
	{
		// no weapon
		return;
	}

	psStats = asWeaponStats + psWeap->nStat;

	/* Justifiable only when greater than a one second reload or intra salvo time  */
	bSalvo = FALSE;
	if(psStats->numRounds > 1)
	{
		bSalvo = TRUE;
	}
	if( (bSalvo && (psStats->reloadTime > GAME_TICKS_PER_SEC)) ||
		(psStats->firePause > GAME_TICKS_PER_SEC) ||
		((psObj->type == OBJ_DROID) && vtolDroid((DROID *)psObj)) )
	{
		if (psObj->type == OBJ_DROID && vtolDroid((DROID *)psObj))
		{
			//deal with VTOLs
			firingStage = getNumAttackRuns((DROID *)psObj, weapon_slot) - ((DROID *)psObj)->sMove.iAttackRuns[weapon_slot];

			//compare with max value
			interval = getNumAttackRuns((DROID *)psObj, weapon_slot);
		}
		else
		{
			firingStage = gameTime - psWeap->lastFired;
			if (bSalvo)
			{
				interval = weaponReloadTime(psStats, psObj->player);
			}
			else
			{
				interval = weaponFirePause(psStats, psObj->player);
			}
		}

		scrX = psObj->sDisplay.screenX;
		scrY = psObj->sDisplay.screenY;
		scrR = psObj->sDisplay.screenR;
		switch (psObj->type)
		{
		case OBJ_DROID:
			damLevel = PERCENT(((DROID *)psObj)->body, ((DROID *)psObj)->originalBody);
			scrY += scrR + 2;
			break;
		case OBJ_STRUCTURE:
			psStruct = (STRUCTURE *)psObj;
			damLevel = PERCENT(psStruct->body, structureBody(psStruct));
			scale = MAX(psStruct->pStructureType->baseWidth, psStruct->pStructureType->baseBreadth);
			scrY += scale * 10 - 1;
			scrR = scale * 20;
			break;
		default:
			ASSERT(!"invalid object type", "drawWeaponReloadBars: invalid object type");
			damLevel = 100;
			break;
		}

		//now we know what it is!!
		if (damLevel < HEAVY_DAMAGE_LEVEL)
		{
			interval += interval;
		}

		if(firingStage < interval)
		{
			/* Get a percentage */
			firingStage = PERCENT(firingStage,interval);

			/* Scale it into an appropriate range */
			firingStage = ((((2*scrR)*10000)/100)*firingStage)/10000;
			if(firingStage >= (UDWORD)(2*scrR))
			{
				firingStage = (2*scrR) - 1;
			}
			/* Power bars */
			pie_BoxFill(scrX - scrR-1, 6+scrY + 0 + (weapon_slot * 5), scrX - scrR +(2*scrR),    6+scrY+3 + (weapon_slot * 5), WZCOL_RELOAD_BACKGROUND);
			pie_BoxFill(scrX - scrR,   6+scrY + 1 + (weapon_slot * 5), scrX - scrR +firingStage, 6+scrY+2 + (weapon_slot * 5), WZCOL_RELOAD_BAR);
		}
	}
}


static void	drawStructureSelections( void )
{
	STRUCTURE	*psStruct;
	SDWORD		scrX,scrY,scrR;
	PIELIGHT	powerCol = WZCOL_BLACK;
	UDWORD		health,width;
	UDWORD		scale;
	UDWORD		i;
	BASE_OBJECT	*psClickedOn;
	BOOL		bMouseOverStructure = FALSE;
	BOOL		bMouseOverOwnStructure = FALSE;
	float		mulH;

	psClickedOn = mouseTarget();
	if(psClickedOn!=NULL && psClickedOn->type == OBJ_STRUCTURE)
	{
		bMouseOverStructure = TRUE;
		if(psClickedOn->player == selectedPlayer)
		{
			bMouseOverOwnStructure = TRUE;
		}
	}
	pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
	pie_SetFogStatus(FALSE);

	/* Go thru' all the buildings */
	for(psStruct = apsStructLists[selectedPlayer]; psStruct; psStruct = psStruct->psNext)
	{
		if(clipXY(psStruct->pos.x,psStruct->pos.y))
		{
			/* If it's selected */
			if (psStruct->selected
			    || (barMode == BAR_DROIDS_AND_STRUCTURES
			        && (psStruct->pStructureType->type != REF_WALL && psStruct->pStructureType->type != REF_WALLCORNER))
			    || (bMouseOverOwnStructure
			        && psStruct == (STRUCTURE *) psClickedOn
			        && ((STRUCTURE * )psClickedOn)->status == SS_BUILT
			            && psStruct->sDisplay.frameNumber == currentGameFrame))
			{
				scale = MAX(psStruct->pStructureType->baseWidth, psStruct->pStructureType->baseBreadth);
				width = scale*20;
				scrX = psStruct->sDisplay.screenX;
				scrY = psStruct->sDisplay.screenY + (scale*10);
				scrR = width;
				//health = PERCENT(psStruct->body, psStruct->baseBodyPoints);
				if (ctrlShiftDown())
				{
					//show resistance values if CTRL/SHIFT depressed
					UDWORD  resistance = structureResistance(psStruct->pStructureType, psStruct->player);

					if (resistance)
					{
						health = PERCENT(psStruct->resistance, resistance);
					}
					else
					{
						health = 100;
					}
				}
				else
				{
					//show body points
					health = PERCENT(psStruct->body, structureBody(psStruct));
				}
				if(health>100) health =100;

				if (health > REPAIRLEV_HIGH) {
					powerCol = WZCOL_HEALTH_HIGH;
				} else if (health >= REPAIRLEV_LOW) {
					powerCol = WZCOL_HEALTH_MEDIUM;
				} else {
					powerCol = WZCOL_HEALTH_LOW;
				}
				mulH = (float)health / 100.f;
				mulH *= (float)width;
				health = mulH;
				if(health>width) health = width;
				health*=2;
				pie_BoxFill(scrX-scrR - 1, scrY - 1, scrX + scrR + 1, scrY + 2, WZCOL_RELOAD_BACKGROUND);
				pie_BoxFill(scrX-scrR, scrY, scrX - scrR + health, scrY + 1, powerCol);

				for (i = 0; i < psStruct->numWeaps; i++)
				{
					drawWeaponReloadBar((BASE_OBJECT *)psStruct, &psStruct->asWeaps[i], i);
				}
			}
			else
			{
				if(psStruct->status == SS_BEING_BUILT && psStruct->sDisplay.frameNumber == currentGameFrame)
				{
					scale = MAX(psStruct->pStructureType->baseWidth, psStruct->pStructureType->baseBreadth);
					width = scale*20;
					scrX = psStruct->sDisplay.screenX;
					scrY = psStruct->sDisplay.screenY + (scale*10);
					scrR = width;
					health =  PERCENT(psStruct->currentBuildPts ,
					psStruct->pStructureType->buildPoints);
					if (health >= 100)
					{
						health = 100;	// belt and braces
					}
					powerCol = WZCOL_YELLOW;
					mulH = (float)health / 100.f;
					mulH *= (float)width;
					health = mulH;
					if (health > width)
					{
						health = width;
					}
					health *= 2;
					pie_BoxFill(scrX - scrR - 1, scrY - 1, scrX + scrR + 1, scrY + 2, WZCOL_RELOAD_BACKGROUND);
					pie_BoxFill(scrX - scrR, scrY, scrX - scrR + health, scrY + 1, powerCol);
				}
			}
		}
	}

	for(i=0; i<MAX_PLAYERS; i++)
	{
		/* Go thru' all the buildings */
		for(psStruct = apsStructLists[i]; psStruct; psStruct = psStruct->psNext)
		{
			if(i!=selectedPlayer)		// only see enemy buildings being targetted, not yours!
			{
				if(clipXY(psStruct->pos.x,psStruct->pos.y))
				{
					/* If it's targetted and on-screen */
					if(psStruct->targetted)
					{
						if(psStruct->sDisplay.frameNumber == currentGameFrame)

						{
							psStruct->targetted = 0;
							scrX = psStruct->sDisplay.screenX;
							scrY = psStruct->sDisplay.screenY - (psStruct->sDisplay.imd->max.y / 4);
							iV_DrawImage(IntImages,getTargettingGfx(),scrX,scrY);
						}
					}
				}
			}
		}
	}

	if(bMouseOverStructure && !bMouseOverOwnStructure)
	{
		if(mouseDown(MOUSE_RMB))
		{
			psStruct = (STRUCTURE*)psClickedOn;
			if(psStruct->status==SS_BUILT)
			{
				scale = MAX(psStruct->pStructureType->baseWidth, psStruct->pStructureType->baseBreadth);
				width = scale*20;
				scrX = psStruct->sDisplay.screenX;
				scrY = psStruct->sDisplay.screenY + (scale*10);
				scrR = width;
				//health = PERCENT(psStruct->body, psStruct->baseBodyPoints);
				if (ctrlShiftDown())
				{
					//show resistance values if CTRL/SHIFT depressed
					UDWORD  resistance = structureResistance(
						psStruct->pStructureType, psStruct->player);
					if (resistance)
					{
						health = PERCENT(psStruct->resistance, resistance);
					}
					else
					{
						health = 100;
					}
				}
				else
				{
					//show body points
					health = PERCENT(psStruct->body, structureBody(psStruct));
				}
				if (health > REPAIRLEV_HIGH)
				{
					powerCol = WZCOL_HEALTH_HIGH;
				}
				else if (health > REPAIRLEV_LOW)
				{
					powerCol = WZCOL_HEALTH_MEDIUM;
				}
				else
				{
					powerCol = WZCOL_HEALTH_LOW;
				}
				health = (((width*10000)/100)*health)/10000;
				health*=2;
				pie_BoxFill(scrX-scrR-1, scrY-1, scrX+scrR+1, scrY+2, WZCOL_RELOAD_BACKGROUND);
				pie_BoxFill(scrX-scrR, scrY, scrX-scrR+health, scrY+1, powerCol);
			}
			else if(psStruct->status == SS_BEING_BUILT)
			{
				scale = MAX(psStruct->pStructureType->baseWidth, psStruct->pStructureType->baseBreadth);
				width = scale*20;
				scrX = psStruct->sDisplay.screenX;
				scrY = psStruct->sDisplay.screenY + (scale*10);
				scrR = width;
				health =  PERCENT(psStruct->currentBuildPts , psStruct->pStructureType->buildPoints);
				powerCol = WZCOL_GREEN;
				health = (((width*10000)/100)*health)/10000;
				health*=2;
				pie_BoxFill(scrX - scrR - 1, scrY - 1, scrX + scrR + 1, scrY + 2, WZCOL_RELOAD_BACKGROUND);
				pie_BoxFill(scrX - scrR - 1, scrY, scrX - scrR + health, scrY + 1, powerCol);
			}
		}
	}

	pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);
}

static UDWORD	getTargettingGfx( void )
{
UDWORD	index;

	index = getTimeValueRange(1000,10);

	switch(index)
	{
	case	0:
	case	1:
	case	2:
		return(IMAGE_TARGET1+index);
		break;
	default:
		if(index & 0x01)
		{
			return(IMAGE_TARGET4);
		}
		else
		{
			return(IMAGE_TARGET5);
		}
		break;
	}
}

BOOL	eitherSelected(DROID *psDroid)
{
BOOL			retVal;
BASE_OBJECT		*psObj;

	retVal = FALSE;
	if(psDroid->selected)
	{
		retVal = TRUE;
	}

	if(psDroid->psGroup)
	{
		if(psDroid->psGroup->psCommander)
		{
			if(psDroid->psGroup->psCommander->selected)
			{
				retVal = TRUE;
			}
		}
	}

	if (orderStateObj(psDroid, DORDER_FIRESUPPORT, &psObj))
	{
		if (psObj != NULL && psObj->selected)
		{
			retVal = TRUE;
		}
	}
	return(retVal);
}

static void	drawDroidSelections( void )
{
	UDWORD			scrX,scrY,scrR;
	DROID			*psDroid;
	UDWORD			damage;
	PIELIGHT		powerCol = WZCOL_BLACK;
	PIELIGHT		boxCol;
	BASE_OBJECT		*psClickedOn;
	BOOL			bMouseOverDroid = FALSE;
	BOOL			bMouseOverOwnDroid = FALSE;
	BOOL			bBeingTracked;
	UDWORD			i,index;
	FEATURE			*psFeature;
	float			mulH;

	psClickedOn = mouseTarget();
	if(psClickedOn!=NULL && psClickedOn->type == OBJ_DROID)
	{
		bMouseOverDroid = TRUE;
		if(psClickedOn->player == selectedPlayer && !psClickedOn->selected)
		{
			bMouseOverOwnDroid = TRUE;
		}
	}

	pie_SetDepthBufferStatus(DEPTH_CMP_ALWAYS_WRT_ON);
	pie_SetFogStatus(FALSE);
	for(psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
	{
		bBeingTracked = FALSE;
		/* If it's selected and on screen or it's the one the mouse is over ||*/
		// ABSOLUTELY MAD LOGICAL EXPRESSION!!! :-)
		if ((eitherSelected(psDroid) && psDroid->sDisplay.frameNumber == currentGameFrame)
		 || (bMouseOverOwnDroid && psDroid == (DROID *) psClickedOn)
		 || (droidUnderRepair(psDroid) && psDroid->sDisplay.frameNumber == currentGameFrame)
		 ||  (barMode == BAR_DROIDS || barMode == BAR_DROIDS_AND_STRUCTURES))
		{
			damage = PERCENT(psDroid->body, psDroid->originalBody);

			if (damage > REPAIRLEV_HIGH)
			{
				powerCol = WZCOL_HEALTH_HIGH;
			}
			else if (damage > REPAIRLEV_LOW)
			{
				powerCol = WZCOL_HEALTH_MEDIUM;
			}
			else
			{
				powerCol = WZCOL_HEALTH_LOW;
			}
			mulH = (float)psDroid->body / (float)psDroid->originalBody;
			damage = mulH * (float)psDroid->sDisplay.screenR;// (((psDroid->sDisplay.screenR*10000)/100)*damage)/10000;
			if(damage>psDroid->sDisplay.screenR) damage = psDroid->sDisplay.screenR;

			damage *=2;
			scrX = psDroid->sDisplay.screenX;
			scrY = psDroid->sDisplay.screenY;
			scrR = psDroid->sDisplay.screenR;

			/* Yeah, yeah yeah - hardcoded palette entries - need to change to #defined colour names */
			/* Three DFX clips properly right now - not sure if software does */
//			if((scrX+scrR)>0 && (scrY+scrR)>0 && (scrX-scrR)<DISP_WIDTH
//				&& (scrY-scrR)<DISP_HEIGHT)
			{
				if(!driveModeActive() || driveIsDriven(psDroid)) {
					boxCol = WZCOL_WHITE;
				} else {
					boxCol = WZCOL_GREEN;
				}

				if (psDroid->selected)
				{
					pie_BoxFill(scrX - scrR, scrY + scrR - 7, scrX - scrR+1, scrY + scrR, boxCol);
					pie_BoxFill(scrX - scrR, scrY + scrR, scrX - scrR + 7, scrY + scrR + 1, boxCol);
					pie_BoxFill(scrX + scrR - 7, scrY + scrR, scrX + scrR, scrY + scrR + 1, boxCol);
					pie_BoxFill(scrX + scrR, scrY + scrR - 7, scrX + scrR + 1, scrY + scrR + 1, boxCol);
				}

				/* Power bars */
				pie_BoxFill(scrX - scrR - 1, scrY + scrR+2, scrX + scrR + 1, scrY + scrR + 5, WZCOL_RELOAD_BACKGROUND);
				pie_BoxFill(scrX - scrR, scrY + scrR+3, scrX - scrR + damage, scrY + scrR + 4, powerCol);

				/* Write the droid rank out */
				if((scrX+scrR)>0 && (scrY+scrR)>0 && (scrX-scrR) < pie_GetVideoBufferWidth() && (scrY-scrR) < pie_GetVideoBufferHeight())
				{
					drawDroidRank(psDroid);
					drawDroidSensorLock(psDroid);

					if ((psDroid->droidType == DROID_COMMAND) ||
						(psDroid->psGroup != NULL && psDroid->psGroup->type == GT_COMMAND))
					{
						drawDroidCmndNo(psDroid);
					}
					else if(psDroid->group!=UBYTE_MAX)
					{
						drawDroidGroupNumber(psDroid);
					}
				}
			}

			for (i = 0;i < psDroid->numWeaps;i++)
			{
				drawWeaponReloadBar((BASE_OBJECT *)psDroid, &psDroid->asWeaps[i], i);
			}
		}
	}



	/* Are we over an enemy droid */
	if(bMouseOverDroid && !bMouseOverOwnDroid)
	{
		if(mouseDown(MOUSE_RMB))
		{
			if(psClickedOn->player!=selectedPlayer && psClickedOn->sDisplay.frameNumber == currentGameFrame)
			{
				psDroid = (DROID*)psClickedOn;
				//show resistance values if CTRL/SHIFT depressed
				if (ctrlShiftDown())
				{
					if (psDroid->resistance)
					{
						damage = PERCENT(psDroid->resistance, droidResistance(psDroid));
					}
					else
					{
						damage = 100;
					}
				}
				else
				{
					damage = PERCENT(psDroid->body,psDroid->originalBody);
				}

				if (damage > REPAIRLEV_HIGH)
				{
					powerCol = WZCOL_HEALTH_HIGH;
				}
				else if (damage > REPAIRLEV_LOW)
				{
					powerCol = WZCOL_HEALTH_MEDIUM;
				}
				else
				{
					powerCol = WZCOL_HEALTH_LOW;
				}

				//show resistance values if CTRL/SHIFT depressed
				if (ctrlShiftDown())
				{
					if (psDroid->resistance)
					{
						mulH = (float)psDroid->resistance / (float)droidResistance(psDroid);
					}
					else
					{
						mulH = 100.f;
					}
				}
				else
				{
					mulH = (float)psDroid->body / (float)psDroid->originalBody;
				}
				damage = mulH * (float)psDroid->sDisplay.screenR;// (((psDroid->sDisplay.screenR*10000)/100)*damage)/10000;
				if(damage>psDroid->sDisplay.screenR) damage = psDroid->sDisplay.screenR;
				damage *=2;
				scrX = psDroid->sDisplay.screenX;
				scrY = psDroid->sDisplay.screenY;
				scrR = psDroid->sDisplay.screenR;

				/* Yeah, yeah yeah - hardcoded palette entries - need to change to #defined colour names */
				/* Three DFX clips properly right now - not sure if software does */
				if((scrX+scrR)>0 && (scrY+scrR)>0 && (scrX-scrR) < pie_GetVideoBufferWidth() && (scrY-scrR) < pie_GetVideoBufferHeight())
				{
					if(!driveModeActive() || driveIsDriven(psDroid)) {
						boxCol = WZCOL_WHITE;
					} else {
						boxCol = WZCOL_GREEN;
					}

					//we always want to show the enemy health/resistance as energyBar - AB 18/06/99
					//if(bEnergyBars)
					{
						/* Power bars */
						pie_BoxFill(scrX - scrR - 1, scrY + scrR + 2, scrX + scrR + 1, scrY + scrR + 5, WZCOL_RELOAD_BACKGROUND);
						pie_BoxFill(scrX - scrR, scrY + scrR+3, scrX - scrR + damage, scrY + scrR + 4, powerCol);
					}
				}
			}
		}
	}

	for(i=0; i<MAX_PLAYERS; i++)
	{
		/* Go thru' all the droidss */
		for(psDroid = apsDroidLists[i]; psDroid; psDroid = psDroid->psNext)
		{
			if(i!=selectedPlayer && !psDroid->died && psDroid->sDisplay.frameNumber == currentGameFrame)
			{
				/* If it's selected */
				if(psDroid->bTargetted && (psDroid->visible[selectedPlayer] == UBYTE_MAX))
				{
					psDroid->bTargetted = FALSE;
					scrX = psDroid->sDisplay.screenX;
					scrY = psDroid->sDisplay.screenY - 8;
					index = IMAGE_BLUE1+getTimeValueRange(1020,5);
					iV_DrawImage(IntImages,index,scrX,scrY);
				}
			}
		}
	}

	for(psFeature = apsFeatureLists[0]; psFeature; psFeature = psFeature->psNext)
	{
		if(!psFeature->died && psFeature->sDisplay.frameNumber == currentGameFrame)
		{
			if(psFeature->bTargetted)
			{
				psFeature->bTargetted = FALSE;
				scrX = psFeature->sDisplay.screenX;
				scrY = psFeature->sDisplay.screenY - (psFeature->sDisplay.imd->max.y / 4);
				iV_DrawImage(IntImages,getTargettingGfx(),scrX,scrY);
			}
		}
	}


	pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);
}

/* ---------------------------------------------------------------------------- */
#define GN_X_OFFSET	(28)
#define GN_Y_OFFSET (17)
static void	drawDroidGroupNumber(DROID *psDroid)
{
UWORD	id;
UDWORD	id2;
BOOL	bDraw;
SDWORD	xShift,yShift;

	bDraw = TRUE;

	id = id2 = UDWORD_MAX;

	/* Is the unit in a group? */
	if(psDroid->psGroup && psDroid->psGroup->type == GT_COMMAND)
	{
		id2 = IMAGE_GN_STAR;

	}
	//else
	{
		switch(psDroid->group)
		{
		case 0:
			id = IMAGE_GN_0;
			break;
		case 1:
			id = IMAGE_GN_1;
			break;
		case 2:
			id = IMAGE_GN_2;
			break;
		case 3:
			id = IMAGE_GN_3;
			break;
		case 4:
			id = IMAGE_GN_4;
			break;
		case 5:
			id = IMAGE_GN_5;
			break;
		case 6:
			id = IMAGE_GN_6;
			break;
		case 7:
			id = IMAGE_GN_7;
			break;
		case 8:
			id = IMAGE_GN_8;
			break;
		case 9:
			id = IMAGE_GN_9;
			break;
		default:
			bDraw = FALSE;
			break;
		}
	}
	if(bDraw)
	{
		xShift = GN_X_OFFSET;  // yeah yeah, I know
		yShift = GN_Y_OFFSET;
		xShift = ((xShift*pie_GetResScalingFactor())/100);
		yShift = ((yShift*pie_GetResScalingFactor())/100);
		iV_DrawImage(IntImages,id,psDroid->sDisplay.screenX-xShift,psDroid->sDisplay.screenY+yShift);
		if(id2!=UDWORD_MAX)
		{
			iV_DrawImage(IntImages,id2,psDroid->sDisplay.screenX-xShift,psDroid->sDisplay.screenY+yShift-8);
		}
	}
}

/* ---------------------------------------------------------------------------- */
static void	drawDroidCmndNo(DROID *psDroid)
{
UWORD	id;
UDWORD	id2;
BOOL	bDraw;
SDWORD	xShift,yShift, index;

	bDraw = TRUE;

	id = id2 = UDWORD_MAX;

	id2 = IMAGE_GN_STAR;
	index = SDWORD_MAX;
	if (psDroid->droidType == DROID_COMMAND)
	{
		index = cmdDroidGetIndex(psDroid);
	}
	else if (psDroid->psGroup && psDroid->psGroup->type == GT_COMMAND)
	{
		index = cmdDroidGetIndex(psDroid->psGroup->psCommander);
	}
	switch(index)
	{
	case 1:
		id = IMAGE_GN_1;
		break;
	case 2:
		id = IMAGE_GN_2;
		break;
	case 3:
		id = IMAGE_GN_3;
		break;
	case 4:
		id = IMAGE_GN_4;
		break;
	case 5:
		id = IMAGE_GN_5;
		break;
	case 6:
		id = IMAGE_GN_6;
		break;
	case 7:
		id = IMAGE_GN_7;
		break;
	case 8:
		id = IMAGE_GN_8;
		break;
	case 9:
		id = IMAGE_GN_9;
		break;
	default:
		bDraw = FALSE;
		break;
	}

	if(bDraw)
	{
		xShift = GN_X_OFFSET;  // yeah yeah, I know
		yShift = GN_Y_OFFSET;
		xShift = ((xShift*pie_GetResScalingFactor())/100);
		yShift = ((yShift*pie_GetResScalingFactor())/100);
		iV_DrawImage(IntImages,id2,psDroid->sDisplay.screenX-xShift-6,psDroid->sDisplay.screenY+yShift);
		iV_DrawImage(IntImages,id,psDroid->sDisplay.screenX-xShift,psDroid->sDisplay.screenY+yShift);
	}
}
/* ---------------------------------------------------------------------------- */


/*	Get the onscreen corrdinates of a droid - so we can draw a bounding box - this need to be severely
	speeded up and the accuracy increased to allow variable size bouding boxes */
void calcScreenCoords(DROID *psDroid)
{
	/* Get it's absolute dimensions */
	const Vector3i zero = {0, 0, 0};
	Vector2i center = {0, 0};
	SDWORD cZ;
	UDWORD radius;

	/* How big a box do we want - will ultimately be calculated using xmax, ymax, zmax etc */
	if(psDroid->droidType == DROID_TRANSPORTER)
	{
		radius = 45;
	}
	else
	{
		radius = 22;
	}

	/* Pop matrices and get the screen corrdinates */
	cZ = pie_RotateProject( &zero, &center );

	//Watermelon:added a crash protection hack...
	if (cZ != 0)
	{
		radius = (radius * pie_GetResScalingFactor()) * 80 / cZ;
	}

	/* Deselect all the droids if we've released the drag box */
	if(dragBox3D.status == DRAG_RELEASED)
	{
		if(inQuad(&center, &dragQuad) && psDroid->player == selectedPlayer)
		{
			//don't allow Transporter Droids to be selected here
			//unless we're in multiPlayer mode!!!!
			if (psDroid->droidType != DROID_TRANSPORTER || bMultiPlayer)
			{
				dealWithDroidSelect(psDroid, TRUE);
			}
		}
	}
	center.y -= 4;

	/* Store away the screen coordinates so we can select the droids without doing a trasform */
	psDroid->sDisplay.screenX = center.x;
	psDroid->sDisplay.screenY = center.y;
	psDroid->sDisplay.screenR = radius;
}


static void preprocessTiles(void)
{
	UDWORD i, j;
	UDWORD left,right,up,down, size;
	DROID *psDroid;
	SDWORD order;

	/* Set up the highlights if we're putting down a wall */
	if (wallDrag.status == DRAG_PLACING || wallDrag.status == DRAG_DRAGGING)
	{
		/* Ensure the start point is always shown */
		SET_TILE_HIGHLIGHT(mapTile(wallDrag.x1, wallDrag.y1));
		if( wallDrag.x1 == wallDrag.x2 || wallDrag.y1 == wallDrag.y2 )
		{
			/* First process the ones inside the wall dragging area */
			left = MIN(wallDrag.x1, wallDrag.x2);
			right = MAX(wallDrag.x1, wallDrag.x2) + 1;
			up = MIN(wallDrag.y1, wallDrag.y2);
			down = MAX(wallDrag.y1, wallDrag.y2) + 1;

			for(i = left; i < right; i++)
			{
				for(j = up; j < down; j++)
				{
					SET_TILE_HIGHLIGHT(mapTile(i,j));
				}
			}
		}
	}
	/* Only bother if we're placing a building */
	else if (buildState == BUILD3D_VALID || buildState == BUILD3D_POS)
	{
	/* Now do the ones inside the building highlight */
		left = buildSite.xTL;
		right = buildSite.xBR + 1;
		up = buildSite.yTL;
		down = buildSite.yBR + 1;

		for(i = left; i < right; i++)
		{
			for(j = up; j < down; j++)
			{
				SET_TILE_HIGHLIGHT(mapTile(i,j));
			}
		}
	}

	// HACK don't display until we're releasing this feature in an update!
#ifndef DISABLE_BUILD_QUEUE
	if (intBuildSelectMode())
	{
		//and there may be multiple building sites that need highlighting - AB 26/04/99
		if (ctrlShiftDown())
		{
			//this highlights ALL constructor units' build sites
			for (psDroid = apsDroidLists[selectedPlayer]; psDroid; psDroid = psDroid->psNext)
			{
				//psDroid = (DROID *)psObj;
				if (psDroid->droidType == DROID_CONSTRUCT ||
					psDroid->droidType == DROID_CYBORG_CONSTRUCT)
				{
					//draw the current build site if its a line of structures
					if (psDroid->order == DORDER_LINEBUILD)
					{
							left = map_coord(psDroid->orderX);
							right = map_coord(psDroid->orderX2) + 1;
							if (left > right)
							{
								size = left;
								left = right;
								right = size;
							}
							up = map_coord(psDroid->orderY);
							down = map_coord(psDroid->orderY2) + 1;
							if (up > down)
							{
								size = up;
								up = down;
								down = size;
							}
							//hilight the tiles
							for(i = left; i < right; i++)
							{
								for(j = up; j < down; j++)
								{
									SET_TILE_HIGHLIGHT(mapTile(i,j));
								}
							}
					}
					//now look thru' the list of orders to see if more building sites
					for (order = 0; order < psDroid->listSize; order++)
					{
						if (psDroid->asOrderList[order].order == DORDER_BUILD)
						{
							//set up coords for tiles
							size = ((STRUCTURE_STATS *)psDroid->asOrderList[order].
								psOrderTarget)->baseWidth;
							left = map_coord(psDroid->asOrderList[order].x) - size/2;
							right = left + size;
							size = ((STRUCTURE_STATS *)psDroid->asOrderList[order].
								psOrderTarget)->baseBreadth;
							up = map_coord(psDroid->asOrderList[order].y) - size/2;
							down = up + size;
							//hilight the tiles
							for(i = left; i < right; i++)
							{
								for(j = up; j < down; j++)
								{
									SET_TILE_HIGHLIGHT(mapTile(i,j));
								}
							}
						}
						else if (psDroid->asOrderList[order].order == DORDER_LINEBUILD)
						{
							//need to highlight the length of the wall
							left = map_coord(psDroid->asOrderList[order].x);
							right = map_coord(psDroid->asOrderList[order].x2);
							if (left > right)
							{
								size = left;
								left = right;
								right = size;
							}
							up = map_coord(psDroid->asOrderList[order].y);
							down = map_coord(psDroid->asOrderList[order].y2);
							if (up > down)
							{
								size = up;
								up = down;
								down = size;
							}
							//hilight the tiles
							for(i = left; i <= right; i++)
							{
								for(j = up; j <= down; j++)
								{
									SET_TILE_HIGHLIGHT(mapTile(i, j));
								}
							}
						}
					}
				}
			}
		}
	}
#endif
}


/*!
 * Find the tile the mouse is currently over
 */
/* TODO This is slow - speed it up */
static void locateMouse(void)
{
	const Vector2i pt = {mouseXPos, mouseYPos};
	unsigned int i;
	int nearestZ = INT_MAX;

	for(i = 0; i < visibleTiles.x; ++i)
	{
		unsigned int j;
		for(j = 0; j < visibleTiles.y; ++j)
		{
			int tileZ = tileScreenInfo[i][j].screen.z;

			if(tileZ <= nearestZ)
			{
				QUAD quad;

				quad.coords[0].x = tileScreenInfo[i+0][j+0].screen.x;
				quad.coords[0].y = tileScreenInfo[i+0][j+0].screen.y;

				quad.coords[1].x = tileScreenInfo[i+0][j+1].screen.x;
				quad.coords[1].y = tileScreenInfo[i+0][j+1].screen.y;

				quad.coords[2].x = tileScreenInfo[i+1][j+1].screen.x;
				quad.coords[2].y = tileScreenInfo[i+1][j+1].screen.y;

				quad.coords[3].x = tileScreenInfo[i+1][j+0].screen.x;
				quad.coords[3].y = tileScreenInfo[i+1][j+0].screen.y;

				/* We've got a match for our mouse coords */
				if (inQuad(&pt, &quad))
				{
					mouseTileX = playerXTile + j;
					mouseTileY = playerZTile + i;
					if (mouseTileX < 0)
						mouseTileX = 0;
					else if (mouseTileX > mapWidth-1)
						mouseTileX = mapWidth - 1;
					if (mouseTileY < 0)
						mouseTileY = 0;
					else if (mouseTileY > mapHeight-1)
						mouseTileY = mapHeight - 1;

					/* Store away z value */
					nearestZ = tileZ;
				}
			}
		}
	}
}


// Render the sky and surroundings
static void renderSurroundings(void)
{
	static float wind = 0.0f;
	const float skybox_scale = 10000.0f;
	const float height = 20.0f * TILE_UNITS;
	const float wider  = 2.0f * (visibleTiles.x * TILE_UNITS);
	int left, right, front, back, rx, rz;

	// set up matrices and textures
	pie_PerspectiveBegin();

	// Push identity matrix onto stack
	pie_MatBegin();

	// Now, scale the world according to what resolution we're running in
	pie_MatScale(pie_GetResScalingFactor());

	// Set the camera position
	pie_MATTRANS(camera.p.x, camera.p.y, camera.p.z);

	// Rotate for the player and for the wind
	pie_MatRotZ(player.r.z);
	pie_MatRotX(player.r.x);
	pie_MatRotY(player.r.y);

	// Fogbox //
	rx = (player.p.x) & (TILE_UNITS-1);
	rz = (player.p.z) & (TILE_UNITS-1);
	pie_TRANSLATE(-rx, -player.p.y, rz);

	left  = TILE_UNITS * MIN(visibleTiles.x/2, playerXTile+visibleTiles.x/2+1);
	right = TILE_UNITS * MIN(visibleTiles.x/2, mapWidth-playerXTile-visibleTiles.x/2);
	front = TILE_UNITS * MIN(visibleTiles.y/2, playerZTile+visibleTiles.y/2+1);
	back  = TILE_UNITS * MIN(visibleTiles.y/2, mapHeight-playerZTile-visibleTiles.y/2);

	pie_DrawFogBox(left, right, front, back, height, wider);

	// undo the translation
	pie_TRANSLATE(rx,player.p.y,-rz);

	// Skybox //
	// rotate it
	pie_MatRotY(DEG(1) * wind);

	// move it somewhat below ground level for the blending effect
	pie_TRANSLATE(0, -skybox_scale/8, 0);

	// Set the texture page
	pie_SetTexturePage( iV_GetTexture(SKY_TEXPAGE) );

	if(!gamePaused())
	{
		wind += timeAdjustedIncrement(0.5f, FALSE);
		if(wind >= 360.0f)
		{
			wind = 0.0f;
		}
	}
	pie_DrawSkybox(skybox_scale, 0, 128, 256, 128);

	// Load Saved State
	pie_MatEnd();
	pie_PerspectiveEnd();
}

/* Flattens an imd to the landscape and handles 4 different rotations */
static iIMDShape	*flattenImd(iIMDShape *imd, UDWORD structX, UDWORD structY, UDWORD direction)
{
	UDWORD i, centreHeight;

	ASSERT( imd->npoints < iV_IMD_MAX_POINTS, "flattenImd: too many points in the PIE to flatten it" );

	/* Get a copy of the points */
	memcpy(alteredPoints, imd->points, imd->npoints * sizeof(Vector3f));

	/* Get the height of the centre point for reference */
	centreHeight = map_Height(structX,structY);

	/* Now we go through the shape looking for vertices on the edge */
	/* Flip reference coords if we're on a vertical wall */

	/* Little hack below 'cos sometimes they're not exactly 90 degree alligned. */
	direction /= 90;
	direction *= 90;

	switch(direction)
	{
	case 0:
		for(i = 0; i < (UDWORD)imd->npoints; i++)
		{
			if (abs(alteredPoints[i].x) >= 63 || abs(alteredPoints[i].z) >= 63)
			{
				UDWORD tempX = MIN(structX + alteredPoints[i].x, world_coord(mapWidth - 1));
				UDWORD tempY = MAX(structY - alteredPoints[i].z, 0);
				SDWORD shift = centreHeight - map_Height(tempX, tempY);

				alteredPoints[i].y -= (shift - 4);
			}
		}
		break;
	case 90:
		for(i=0; i<(UDWORD)imd->npoints; i++)
		{
			if (abs(alteredPoints[i].x) >= 63 || abs(alteredPoints[i].z) >= 63)
			{
				UDWORD tempX = MAX(structX - alteredPoints[i].z, 0);
				UDWORD tempY = MAX(structY - alteredPoints[i].x, 0);
				SDWORD shift = centreHeight - map_Height(tempX, tempY);

				alteredPoints[i].y -= (shift - 4);
			}
		}
		break;
	case 180:
		for(i=0; i<(UDWORD)imd->npoints; i++)
		{
			if (abs(alteredPoints[i].x) >= 63 || abs(alteredPoints[i].z) >= 63)
			{
				UDWORD tempX = MAX(structX - alteredPoints[i].x, 0);
				UDWORD tempY = MIN(structY + alteredPoints[i].z, world_coord(mapHeight - 1));
				SDWORD shift = centreHeight - map_Height(tempX, tempY);

				alteredPoints[i].y -= (shift - 4);
			}
		}
		break;
	case 270:
		for(i=0; i<(UDWORD)imd->npoints; i++)
		{
			if(abs(alteredPoints[i].x) >= 63 || abs(alteredPoints[i].z)>=63)
			{
				UDWORD tempX = MIN(structX + alteredPoints[i].z, world_coord(mapWidth - 1));
				UDWORD tempY = MIN(structY + alteredPoints[i].x, world_coord(mapHeight - 1));
				SDWORD shift = centreHeight - map_Height(tempX, tempY);

				alteredPoints[i].y -= (shift - 4);
			}
		}
		break;
	default:
		debug(LOG_ERROR, "Weird direction (%u) for a structure in flattenImd", direction);
		abort();
		break;
	}

	imd->points = alteredPoints;
	return imd;
}

//#define SHOW_ZONES
//#define SHOW_GATEWAYS

// -------------------------------------------------------------------------------------
/* New improved (and much faster) tile drawer */
// -------------------------------------------------------------------------------------
static void drawTerrainTile(UDWORD i, UDWORD j, BOOL onWaterEdge)
{
	/* Get the correct tile index for the x/y coordinates */
	SDWORD actualX = playerXTile + j, actualY = playerZTile + i;
	MAPTILE *psTile = NULL;
	BOOL bOutlined = FALSE;
	UDWORD tileNumber = 0;
	TERRAIN_VERTEX vertices[3];
#if defined(SHOW_ZONES) || defined(SHOW_GATEWAYS)
	SDWORD zone = 0;
#endif
	PIELIGHT colour[2][2];

	colour[0][0] = WZCOL_BLACK;
	colour[1][0] = WZCOL_BLACK;
	colour[0][1] = WZCOL_BLACK;
	colour[1][1] = WZCOL_BLACK;

	/* Let's just get out now if we're not supposed to draw it */
	if( (actualX < 0) ||
		(actualY < 0) ||
		(actualX > mapWidth-1) ||
		(actualY > mapHeight-1) )
	{
		tileNumber = 0;
	}
	else
	{
		psTile = mapTile(actualX, actualY);

		colour[0][0] = psTile->colour;
		if (actualY + 1 < mapHeight - 1)
		{
			colour[1][0] = mapTile(actualX, actualY + 1)->colour;
		}
		if (actualX + 1 < mapWidth - 1)
		{
			colour[0][1] = mapTile(actualX + 1, actualY)->colour;
		}
		if (actualX + 1 < mapWidth - 1 && actualY + 1 < mapHeight - 1)
		{
			colour[1][1] = mapTile(actualX + 1, actualY + 1)->colour;
		}
#if defined(SHOW_ZONES)
		if (!fpathBlockingTile(actualX, actualY) ||
			terrainType(psTile) == TER_WATER)
		{
			zone = gwGetZone(actualX, actualY);
		}
#elif defined(SHOW_GATEWAYS)
		if (psTile->tileInfoBits & BITS_GATEWAY)
		{
			zone = gwGetZone(actualX, actualY);
		}
#endif
		if ( terrainType(psTile) != TER_WATER || onWaterEdge )
		{
			// what tile texture number is it?
			tileNumber = psTile->texture;
		}
		else
		{
			// If it's a water tile then force it to be the river bed texture.
			tileNumber = RIVERBED_TILE;
		}
	}

#if defined(SHOW_ZONES)
	if (zone != 0)
	{
		tileNumber = zone;
	}
#elif defined(SHOW_GATEWAYS)
	if (psTile && psTile->tileInfoBits & BITS_GATEWAY)
	{
		tileNumber = 55;//zone;
	}
#endif

	/* Is the tile highlighted? Perhaps because there's a building foundation on it */
	if (psTile && !onWaterEdge && TILE_HIGHLIGHT(psTile))
	{
		/* Clear it for next time round */
		CLEAR_TILE_HIGHLIGHT(psTile);
		bOutlined = TRUE;
		if ( i < (LAND_XGRD-1) && j < (LAND_YGRD-1) ) // FIXME
		{
			if (outlineTile)
			{
				colour[0][0] = pal_SetBrightness(255);
				colour[1][0] = pal_SetBrightness(255);
				colour[0][1] = pal_SetBrightness(255);
				colour[1][1] = pal_SetBrightness(255);
			}
			else
			{
				colour[0][0].byte.r = 255;
				colour[1][0].byte.r = 255;
				colour[0][1].byte.r = 255;
				colour[1][1].byte.r = 255;
			}
		}
	}

	/* Check for rotations and flips - this sets up the coordinates for texturing */
	flipsAndRots(tileNumber, i, j);

	/* The first triangle */
	vertices[0] = tileScreenInfo[i + 0][j + 0];
	vertices[1] = tileScreenInfo[i + 0][j + 1];
	vertices[0].light = colour[0][0];
	vertices[1].light = colour[0][1];
	if (onWaterEdge)
	{
		vertices[0].pos.y = tileScreenInfo[i + 0][j + 0].water_height;
		vertices[1].pos.y = tileScreenInfo[i + 0][j + 1].water_height;
	}

	if (psTile && TRI_FLIPPED(psTile))
	{
		vertices[2] = tileScreenInfo[i + 1][j + 0];
		vertices[2].light = colour[1][0];
		if (onWaterEdge)
		{
			vertices[2].pos.y = tileScreenInfo[i + 1][j + 0].water_height;
		}
	}
	else
	{
		vertices[2] = tileScreenInfo[i + 1][j + 1];
		vertices[2].light = colour[1][1];
		if (onWaterEdge)
		{
			vertices[2].pos.y = tileScreenInfo[i + 1][j + 1].water_height;
		}
	}

	if (onWaterEdge)
	{
		pie_DrawWaterTriangle(vertices);
	}
	else
	{
		pie_DrawTerrainTriangle(i * 2 + j * VISIBLE_XTILES * 2, vertices);
	}

	/* The second triangle */
	if (psTile && TRI_FLIPPED(psTile))
	{
		vertices[0] = tileScreenInfo[i + 0][j + 1];
		vertices[0].light = colour[0][1];
		if (onWaterEdge)
		{
			vertices[0].pos.y = tileScreenInfo[i + 0][j + 1].water_height;
		}
	}
	else
	{
		vertices[0] = tileScreenInfo[i + 0][j + 0];
		vertices[0].light = colour[0][0];
		if (onWaterEdge)
		{
			vertices[0].pos.y = tileScreenInfo[i + 0][j + 0].water_height;
		}
	}

	vertices[1] = tileScreenInfo[i + 1][j + 1];
	vertices[2] = tileScreenInfo[i + 1][j + 0];
	vertices[1].light = colour[1][1];
	vertices[2].light = colour[1][0];
	if ( onWaterEdge )
	{
		vertices[1].pos.y = tileScreenInfo[i + 1][j + 1].water_height;
		vertices[2].pos.y = tileScreenInfo[i + 1][j + 0].water_height;
	}

	if (onWaterEdge)
	{
		pie_DrawWaterTriangle(vertices);
	}
	else
	{
		pie_DrawTerrainTriangle(i * 2 + j * VISIBLE_XTILES * 2 + 1, vertices);
	}
}


// Render a water tile.
//
static void drawTerrainWaterTile(UDWORD i, UDWORD j)
{
	/* Get the correct tile index for the x/y coordinates */
	const unsigned int actualX = playerXTile + j, actualY = playerZTile + i;
	PIELIGHT colour[2][2];
	MAPTILE *psTile;

	/* Let's just get out now if we're not supposed to draw it */
	if ( actualX > mapWidth - 1 || actualY > mapHeight - 1 )
	{
		return;
	}

	psTile = mapTile(actualX, actualY);
	colour[0][0] = psTile->colour;
	colour[1][0] = WZCOL_BLACK;
	colour[0][1] = WZCOL_BLACK;
	colour[1][1] = WZCOL_BLACK;

	if (actualY + 1 < mapHeight - 1)
	{
		colour[1][0] = mapTile(actualX, actualY + 1)->colour;
	}
	if (actualX + 1 < mapWidth - 1)
	{
		colour[0][1] = mapTile(actualX + 1, actualY)->colour;
	}
	if (actualX + 1 < mapWidth - 1 && actualY + 1 < mapHeight - 1)
	{
		colour[1][1] = mapTile(actualX + 1, actualY + 1)->colour;
	}

	// If it's a water tile then draw the water
	if (terrainType(psTile) == TER_WATER)
	{
		/* Used to calculate texture coordinates */
		const float xMult = 1.0f / TILES_IN_PAGE_COLUMN;
		const float yMult = 1.0f / (2.0f * TILES_IN_PAGE_ROW);
		const float one = 1.0f / (TILES_IN_PAGE_COLUMN * (float)getTextureSize());
		const unsigned int tileNumber = getWaterTileNum();
		TERRAIN_VERTEX vertices[3];

		tileScreenInfo[i+0][j+0].u = tileTexInfo[TileNumber_tile(tileNumber)].uOffset + one;
		tileScreenInfo[i+0][j+0].v = tileTexInfo[TileNumber_tile(tileNumber)].vOffset;

		tileScreenInfo[i+0][j+1].u = tileTexInfo[TileNumber_tile(tileNumber)].uOffset + (xMult - one);
		tileScreenInfo[i+0][j+1].v = tileTexInfo[TileNumber_tile(tileNumber)].vOffset;

		tileScreenInfo[i+1][j+1].u = tileTexInfo[TileNumber_tile(tileNumber)].uOffset + (xMult - one);
		tileScreenInfo[i+1][j+1].v = tileTexInfo[TileNumber_tile(tileNumber)].vOffset + (yMult - one);

		tileScreenInfo[i+1][j+0].u = tileTexInfo[TileNumber_tile(tileNumber)].uOffset + one;
		tileScreenInfo[i+1][j+0].v = tileTexInfo[TileNumber_tile(tileNumber)].vOffset + (yMult - one);

		vertices[0] = tileScreenInfo[i + 0][j + 0];
		vertices[0].pos.y = tileScreenInfo[i + 0][j + 0].water_height;
		vertices[0].light = colour[0][0];
		vertices[0].light.byte.a = WATER_ALPHA_LEVEL;

		vertices[1] = tileScreenInfo[i + 0][j + 1];
		vertices[1].pos.y = tileScreenInfo[i + 0][j + 1].water_height;
		vertices[1].light = colour[0][1];
		vertices[1].light.byte.a = WATER_ALPHA_LEVEL;

		vertices[2] = tileScreenInfo[i + 1][j + 1];
		vertices[2].pos.y = tileScreenInfo[i + 1][j + 1].water_height;
		vertices[2].light = colour[1][1];
		vertices[2].light.byte.a = WATER_ALPHA_LEVEL;

		pie_DrawWaterTriangle(vertices);

		vertices[1] = vertices[2];
		vertices[2] = tileScreenInfo[i + 1][j + 0];
		vertices[2].pos.y = tileScreenInfo[i + 1][j + 0].water_height;
		vertices[2].light = colour[1][0];
		vertices[2].light.byte.a = WATER_ALPHA_LEVEL;

		pie_DrawWaterTriangle(vertices);
	}
}


UDWORD	getSuggestedPitch( void )
{
	UDWORD	worldAngle;
	UDWORD	xPos,yPos;
	SDWORD	pitch;

	worldAngle = (UDWORD) ((UDWORD)player.r.y/DEG_1)%360;
	/* Now, we need to track angle too - to avoid near z clip! */

	xPos = player.p.x + world_coord(visibleTiles.x/2);
	yPos = player.p.z + world_coord(visibleTiles.y/2);
// 	getBestPitchToEdgeOfGrid(xPos,yPos,360-worldAngle,&pitch);
	getPitchToHighestPoint(xPos, yPos, 360-worldAngle, 0, &pitch);

	if (pitch < abs(MAX_PLAYER_X_ANGLE))
		pitch = abs(MAX_PLAYER_X_ANGLE);
	if (pitch > abs(MIN_PLAYER_X_ANGLE))
		pitch = abs(MIN_PLAYER_X_ANGLE);

	return(pitch);
}


// -------------------------------------------------------------------------------------
static void trackHeight( float desiredHeight )
{
	static float heightSpeed = 0.0f;
	float separation = desiredHeight - player.p.y;	// How far are we from desired height?
	float acceleration = ACCEL_CONSTANT * separation - VELOCITY_CONSTANT * heightSpeed; // Work out accelertion

	/* ...and now speed */
	heightSpeed += timeAdjustedIncrement(acceleration, FALSE);

	/* Adjust the height accordingly */
	player.p.y += timeAdjustedIncrement(heightSpeed, FALSE);
}


// -------------------------------------------------------------------------------------
void toggleEnergyBars(void)
{
	if (++barMode == BAR_LAST)
	{
		barMode = BAR_SELECTED;
	}
}

// -------------------------------------------------------------------------------------
void assignSensorTarget( BASE_OBJECT *psObj )
{
	bSensorTargetting = TRUE;
	lastTargetAssignation = gameTime2;
	psSensorObj = psObj;
}

// -------------------------------------------------------------------------------------
void	assignDestTarget( void )
{
	bDestTargetting = TRUE;
	lastDestAssignation = gameTime2;
	destTargetX = mouseX();
	destTargetY = mouseY();
	destTileX = mouseTileX;
	destTileY = mouseTileY;
}

// -------------------------------------------------------------------------------------
static void	processSensorTarget( void )
{
	SWORD x,y;
	SWORD offset;
	SWORD x0,y0,x1,y1;
	UDWORD	index;


	if(bSensorTargetting)
	{
		if( (gameTime2 - lastTargetAssignation) < TARGET_TO_SENSOR_TIME)
		{
			if(!psSensorObj->died && psSensorObj->sDisplay.frameNumber == currentGameFrame)
			{
				x = /*mouseX();*/(SWORD)psSensorObj->sDisplay.screenX;
				y = (SWORD)psSensorObj->sDisplay.screenY;
				if(!gamePaused())
				{
					index = IMAGE_BLUE1+getStaticTimeValueRange(1020,5);
				}
				else
				{
					index = IMAGE_BLUE1;
				}
				iV_DrawImage(IntImages,index,x,y);

				offset = (SWORD)(12+ ((TARGET_TO_SENSOR_TIME)-(gameTime2-
					lastTargetAssignation))/2);

				x0 = (SWORD)(x-offset);
				y0 = (SWORD)(y-offset);
				x1 = (SWORD)(x+offset);
				y1 = (SWORD)(y+offset);

				iV_Line(x0, y0, x0 + 8, y0, WZCOL_WHITE);
				iV_Line(x0, y0, x0, y0 + 8, WZCOL_WHITE);

				iV_Line(x1, y0, x1 - 8, y0, WZCOL_WHITE);
				iV_Line(x1, y0, x1, y0 + 8, WZCOL_WHITE);

				iV_Line(x1, y1,x1 - 8, y1, WZCOL_WHITE);
				iV_Line(x1, y1,x1, y1 - 8, WZCOL_WHITE);

				iV_Line(x0, y1, x0 + 8, y1, WZCOL_WHITE);
				iV_Line(x0, y1, x0, y1 - 8, WZCOL_WHITE);
			}
			else
			{
				bSensorTargetting = FALSE;
			}
		}
		else
		{
			bSensorTargetting = FALSE;
		}
	}

}

// -------------------------------------------------------------------------------------
static void	processDestinationTarget( void )
{
	SWORD x,y;
	SWORD offset;
	SWORD x0,y0,x1,y1;


	if(bDestTargetting)
	{
		if( (gameTime2 - lastDestAssignation) < DEST_TARGET_TIME)
		{
				x = (SWORD)destTargetX;
				y = (SWORD)destTargetY;

				offset = (SWORD)(((DEST_TARGET_TIME)-(gameTime2-lastDestAssignation))/2);

				x0 = (SWORD)(x-offset);
				y0 = (SWORD)(y-offset);
				x1 = (SWORD)(x+offset);
				y1 = (SWORD)(y+offset);

				pie_BoxFill(x0, y0, x0 + 2, y0 + 2, WZCOL_WHITE);
				pie_BoxFill(x1 - 2, y0 - 2, x1, y0, WZCOL_WHITE);
				pie_BoxFill(x1 - 2, y1 - 2, x1, y1, WZCOL_WHITE);
				pie_BoxFill(x0, y1, x0 + 2, y1 + 2, WZCOL_WHITE);
		}
		else
		{
			bDestTargetting = FALSE;
		}
	}
}

// -------------------------------------------------------------------------------------
void	setUnderwaterTile(UDWORD num)
{
	underwaterTile = num;
}
// -------------------------------------------------------------------------------------
void	setRubbleTile(UDWORD num)
{
	rubbleTile = num;
}
// -------------------------------------------------------------------------------------
UDWORD	getWaterTileNum( void )
{
	return(underwaterTile);
}
// -------------------------------------------------------------------------------------
UDWORD	getRubbleTileNum( void )
{
	return(rubbleTile);
}
// -------------------------------------------------------------------------------------

static UDWORD	lastSpinVal;

static void structureEffectsPlayer( UDWORD player )
{
	SDWORD	val;
	SDWORD	radius;
	UDWORD	angle;
	STRUCTURE	*psStructure;
	SDWORD	xDif,yDif;
	Vector3i	pos;
	UDWORD	numConnected;
	DROID	*psDroid;
	UDWORD	gameDiv;
	UDWORD	i;
	BASE_OBJECT			*psChosenObj = NULL;
	UWORD	bFXSize;

	for(psStructure = apsStructLists[player]; psStructure; psStructure = psStructure->psNext)
	{
		if(psStructure->status == SS_BUILT)
		{
			if(psStructure->pStructureType->type == REF_POWER_GEN && psStructure->visible[selectedPlayer])
			{
				POWER_GEN* psPowerGen = &psStructure->pFunctionality->powerGenerator;
				numConnected = 0;
				for (i = 0; i < NUM_POWER_MODULES; i++)
				{
					if (psPowerGen->apResExtractors[i])
					{
						numConnected++;
					}
				}
				/* No effect if nothing connected */
				if(!numConnected)
				{
					//keep looking for another!
					continue;
				}

				else switch(numConnected)
				{
				case 1:
				case 2:
					gameDiv = 1440;
					val = 4;
					break;
				case 3:
				case 4:
				default:
					gameDiv = 1080;
					val = 3;	  // really fast!!!
					break;
				}

				angle = gameTime2%gameDiv;
				val = angle/val;

				/* New addition - it shows how many are connected... */
				for(i=0 ;i<numConnected; i++)
				{
					radius = 32 - (i*2);	// around the spire
					xDif = radius * (SIN(DEG(val)));
					yDif = radius * (COS(DEG(val)));

					xDif = xDif/4096;	 // cos it's fixed point
					yDif = yDif/4096;

					pos.x = psStructure->pos.x + xDif;
					pos.z = psStructure->pos.y + yDif;
					pos.y = map_Height(pos.x,pos.z) + 64 + (i*20);	// 64 up to get to base of spire
					effectGiveAuxVar(50);	// half normal plasma size...
					addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_LASER,FALSE,NULL,0);

					pos.x = psStructure->pos.x - xDif;
					pos.z = psStructure->pos.y - yDif;
//					pos.y = map_Height(pos.x,pos.z) + 64 + (i*20);	// 64 up to get to base of spire
					effectGiveAuxVar(50);	// half normal plasma size...

					addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_LASER,FALSE,NULL,0);
				}
			}
			/* Might be a re-arm pad! */
			else if(psStructure->pStructureType->type == REF_REARM_PAD
				&& psStructure->visible[selectedPlayer] )
			{
				REARM_PAD* psReArmPad = &psStructure->pFunctionality->rearmPad;
				psChosenObj = psReArmPad->psObj;
				if(psChosenObj!=NULL)
				{
					if((((DROID*)psChosenObj)->visible[selectedPlayer]))
					{
						bFXSize = 0;
						psDroid = (DROID*) psChosenObj;
						if(!psDroid->died && psDroid->action == DACTION_WAITDURINGREARM )
						{
							bFXSize = 30;

						}
						/* Then it's repairing...? */
						if(!gamePaused())
						{
							val = lastSpinVal = getTimeValueRange(720,360);	// grab an angle - 4 seconds cyclic
						}
						else
						{
							val = lastSpinVal;
						}
						radius = psStructure->sDisplay.imd->radius;
						xDif = radius * (SIN(DEG(val)));
						yDif = radius * (COS(DEG(val)));
						xDif = xDif/4096;	 // cos it's fixed point
						yDif = yDif/4096;
						pos.x = psStructure->pos.x + xDif;
						pos.z = psStructure->pos.y + yDif;
						pos.y = map_Height(pos.x,pos.z) + psStructure->sDisplay.imd->max.y;
						effectGiveAuxVar(30+bFXSize);	// half normal plasma size...
						addEffect(&pos,EFFECT_EXPLOSION, EXPLOSION_TYPE_LASER,FALSE,NULL,0);
						pos.x = psStructure->pos.x - xDif;
						pos.z = psStructure->pos.y - yDif;	// buildings are level!
		//				pos.y = map_Height(pos.x,pos.z) + psStructure->sDisplay->pos.max.y;
						effectGiveAuxVar(30+bFXSize);	// half normal plasma size...
						addEffect(&pos,EFFECT_EXPLOSION, EXPLOSION_TYPE_LASER,FALSE,NULL,0);
					}
				}
			}
		}
	}
}

static void structureEffects()
{
	UDWORD	i;

		/* Only do for player 0 power stations */

		if(bMultiPlayer)
		{
			for(i=0;i<MAX_PLAYERS;i++)
			{
				if(isHumanPlayer(i) && apsStructLists[i] )
				{
					structureEffectsPlayer(i);
				}

			}
		}
		else if(apsStructLists[0])
		{
			structureEffectsPlayer(0);
		}
}


static void	showDroidSensorRanges(void)
{
DROID		*psDroid;
STRUCTURE	*psStruct;

	if(rangeOnScreen)		// note, we still have to decide what to do with multiple units selected, since it will draw it for all of them! -Q 5-10-05
	{
	for(psDroid= apsDroidLists[selectedPlayer]; psDroid; psDroid=psDroid->psNext)
	{
		if(psDroid->selected)
		{
			showSensorRange2((BASE_OBJECT*)psDroid);
		}
	}

	for(psStruct = apsStructLists[selectedPlayer]; psStruct; psStruct = psStruct->psNext)
	{
		if(psStruct->selected)
		{
			showSensorRange2((BASE_OBJECT*)psStruct);
		}
	}
	}//end if we want to display...
}

static void	showSensorRange2(BASE_OBJECT *psObj)
{
	SDWORD	radius;
	SDWORD	xDif,yDif;
	UDWORD	sensorRange;
	Vector3i pos;
	UDWORD	i;
	DROID	*psDroid;
	STRUCTURE	*psStruct;
	BOOL	bBuilding=FALSE;

	for(i=0; i<360; i++)
	{
		if(psObj->type == OBJ_DROID)
		{
			psDroid = (DROID*)psObj;
			sensorRange = asSensorStats[psDroid->asBits[COMP_SENSOR].nStat].range;
		}
		else
		{
			psStruct = (STRUCTURE*)psObj;
			sensorRange = psStruct->sensorRange;
			bBuilding = TRUE;
		}
		radius = sensorRange;
		xDif = radius * (SIN(DEG(i)));
		yDif = radius * (COS(DEG(i)));

		xDif = xDif/4096;	 // cos it's fixed point
		yDif = yDif/4096;
		pos.x = psObj->pos.x - xDif;
		pos.z = psObj->pos.y - yDif;
		pos.y = map_Height(pos.x,pos.z)+ 16;	// 64 up to get to base of spire
		effectGiveAuxVar(80);	// half normal plasma size...
		if(bBuilding)
		{
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SMALL,FALSE,NULL,0);
		}
		else
		{
			addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_LASER,FALSE,NULL,0);
		}
	}
}

static void	drawRangeAtPos(SDWORD centerX, SDWORD centerY, SDWORD radius)
{
	SDWORD	xDif,yDif;
	Vector3i	pos;
	UDWORD	i;

	for(i=0; i<360; i++)
	{
		xDif = radius * (SIN(DEG(i)));
		yDif = radius * (COS(DEG(i)));

		xDif = xDif/4096;	 // cos it's fixed point
		yDif = yDif/4096;
		pos.x = centerX - xDif;
		pos.z = centerY - yDif;
		pos.y = map_Height(pos.x,pos.z)+ 16;	// 64 up to get to base of spire
		effectGiveAuxVar(80);	// half normal plasma size...

		addEffect(&pos,EFFECT_EXPLOSION,EXPLOSION_TYPE_SMALL,FALSE,NULL,0);
	}
}

/* draw some effects at certain position to visualize the radius,
* negative radius turns this off
*/
void showRangeAtPos(SDWORD centerX, SDWORD centerY, SDWORD radius)
{
	rangeCenterX = centerX;
	rangeCenterY = centerY;
	rangeRadius = radius;

	bRangeDisplay = TRUE;

	if(radius <= 0)
		bRangeDisplay = FALSE;
}

/*returns the graphic ID for a droid rank*/
UDWORD  getDroidRankGraphic(DROID *psDroid)
{
	UDWORD gfxId;
	/* Not found yet */
	gfxId = UDWORD_MAX;

	/* Establish the numerical value of the droid's rank */
	switch(getDroidLevel(psDroid))
	{
		case 0:
			break;
		case 1:
			gfxId = IMAGE_LEV_0;
			break;
		case 2:
			gfxId = IMAGE_LEV_1;
			break;
		case 3:
			gfxId = IMAGE_LEV_2;
			break;
		case 4:
			gfxId = IMAGE_LEV_3;
			break;
		case 5:
			gfxId = IMAGE_LEV_4;
			break;
		case 6:
			gfxId = IMAGE_LEV_5;
			break;
		case 7:
			gfxId = IMAGE_LEV_6;
			break;
		case 8:
			gfxId = IMAGE_LEV_7;
			break;
		default:
			ASSERT(!"out of range droid rank", "Weird droid level in drawDroidRank");
		break;
	}

	return gfxId;
}

/*	DOES : Assumes matrix context set and that z-buffer write is force enabled (Always).
	Will render a graphic depiction of the droid's present rank.
	BY : Alex McLean.
*/
static void	drawDroidRank(DROID *psDroid)
{
	UDWORD	gfxId = getDroidRankGraphic(psDroid);

	/* Did we get one? - We should have... */
	if(gfxId!=UDWORD_MAX)
	{
		/* Render the rank graphic at the correct location */ // remove hardcoded numbers?!
		iV_DrawImage(IntImages,(UWORD)gfxId,psDroid->sDisplay.screenX+20,psDroid->sDisplay.screenY+8);
	}
}

/*	DOES : Assumes matrix context set and that z-buffer write is force enabled (Always).
	Will render a graphic depiction of the droid's present rank.
*/
static void	drawDroidSensorLock(DROID *psDroid)
{
	//if on fire support duty - must be locked to a Sensor Droid/Structure
	if (orderState(psDroid, DORDER_FIRESUPPORT))
	{
		/* Render the sensor graphic at the correct location - which is what?!*/
		iV_DrawImage(IntImages,IMAGE_GN_STAR,psDroid->sDisplay.screenX+20,
			psDroid->sDisplay.screenY-20);
	}
}

static	void	doConstructionLines( void )
{
DROID	*psDroid;
UDWORD	i;

	for(i=0; i<MAX_PLAYERS; i++)
	{
		for(psDroid= apsDroidLists[i]; psDroid; psDroid = psDroid->psNext)
		{
			if(clipXY(psDroid->pos.x,psDroid->pos.y))
			{

				if( (psDroid->visible[selectedPlayer]==UBYTE_MAX) &&
					(psDroid->sMove.Status != MOVESHUFFLE) )
				{
					if(psDroid->action == DACTION_BUILD)
					{
						if (psDroid->psTarget)
						{
							if (psDroid->psTarget->type == OBJ_STRUCTURE)
							{
								addConstructionLine(psDroid, (STRUCTURE*)psDroid->psTarget);
							}
						}
					}

					else if ((psDroid->action == DACTION_DEMOLISH) ||
							(psDroid->action == DACTION_REPAIR) ||
							(psDroid->action == DACTION_CLEARWRECK) ||
							(psDroid->action == DACTION_RESTORE))
					{
						if(psDroid->psActionTarget[0])
						{
							if(psDroid->psActionTarget[0]->type == OBJ_STRUCTURE)
							{
								addConstructionLine(psDroid, (STRUCTURE*)psDroid->psActionTarget[0]);
							}
						}
					}
				}

			}
		}
	}
	pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);
}

static void addConstructionLine(DROID *psDroid, STRUCTURE *psStructure)
{
	CLIP_VERTEX pts[3];
	Vector3i each;
	Vector3f *point;
	UDWORD	pointIndex;
	SDWORD	realY;
	Vector3i null, vec;
	SDWORD	rx,rz;
	PIELIGHT colour;

	null.x = null.y = null.z = 0;
	each.x = psDroid->pos.x;
	each.z = psDroid->pos.y;
	each.y = psDroid->pos.z + 24;

	vec.x = (each.x - player.p.x) - terrainMidX*TILE_UNITS;
	vec.z = terrainMidY*TILE_UNITS - (each.z - player.p.z);
	vec.y = each.y;

	rx = player.p.x & (TILE_UNITS-1);
	rz = player.p.z & (TILE_UNITS-1);
	pts[0].pos.x = vec.x + rx;
	pts[0].pos.y = vec.y;
	pts[0].pos.z = vec.z - rz;

	pointIndex = rand()%(psStructure->sDisplay.imd->npoints-1);
	point = &(psStructure->sDisplay.imd->points[pointIndex]);

	each.x = psStructure->pos.x + point->x;
	realY = structHeightScale(psStructure) * point->y;
	each.y = psStructure->pos.z + realY;
	each.z = psStructure->pos.y - point->z;

	if(ONEINEIGHT)
	{
		effectSetSize(30);
		addEffect(&each,EFFECT_EXPLOSION,EXPLOSION_TYPE_SPECIFIED,TRUE,getImdFromIndex(MI_PLASMA),0);
	}

	vec.x = (each.x - player.p.x) - terrainMidX*TILE_UNITS;
	vec.z = terrainMidY*TILE_UNITS - (each.z - player.p.z);
	vec.y = each.y;

	rx = player.p.x & (TILE_UNITS-1);
	rz = player.p.z & (TILE_UNITS-1);
	pts[1].pos.x = vec.x + rx;
	pts[1].pos.y = vec.y;
	pts[1].pos.z = vec.z - rz;

	pointIndex = rand()%(psStructure->sDisplay.imd->npoints-1);
	point = &(psStructure->sDisplay.imd->points[pointIndex]);

	each.x = psStructure->pos.x + point->x;
	realY = structHeightScale(psStructure) * point->y;
	each.y = psStructure->pos.z + realY;
	each.z = psStructure->pos.y - point->z;

	vec.x = (each.x - player.p.x) - terrainMidX*TILE_UNITS;
	vec.z = terrainMidY*TILE_UNITS - (each.z - player.p.z);
	vec.y = each.y;

	rx = player.p.x & (TILE_UNITS-1);
	rz = player.p.z & (TILE_UNITS-1);
	pts[2].pos.x = vec.x + rx;
	pts[2].pos.y = vec.y;
	pts[2].pos.z = vec.z - rz;

	// set the colour
	colour = pal_SetBrightness(UBYTE_MAX);

	if (psDroid->action == DACTION_DEMOLISH || psDroid->action == DACTION_CLEARWRECK)
	{
		colour.byte.g = 0;
		colour.byte.b = 0;
	} else {
		colour.byte.r = 0;
		colour.byte.g = 0;
	}
	pts[0].light.argb = 0xff000000;
	pts[1].light.argb = 0xff000000;
	pts[2].light.argb = 0xff000000;

	pts[0].u = 0;
	pts[0].v = 0;

	pts[1].u = 0;
	pts[1].v = 0;

	pts[2].u = 0;
	pts[2].v = 0;

	pie_TransColouredTriangle(pts, colour);
}
