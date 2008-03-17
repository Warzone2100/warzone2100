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
#include "texture.h"

#define HIT_NOTIFICATION	(GAME_TICKS_PER_SEC*2)

#define RADAR_DRAW_VIEW_BOX		// If defined then draw a box to show the viewing area.
#define RADAR_TRIANGLE_SIZE		8
#define RADAR_TRIANGLE_HEIGHT	RADAR_TRIANGLE_SIZE
#define RADAR_TRIANGLE_WIDTH	(RADAR_TRIANGLE_SIZE/2)

#define RADAR_FRAME_SKIP 10

static PIELIGHT	colRadarAlly, colRadarMe, colRadarEnemy;

BOOL bEnemyAllyRadarColor = FALSE;     //enemy/ally radar color

//current mini-map mode
RADAR_DRAW_MODE	radarDrawMode = RADAR_MODE_DEFAULT;

// colours for each clan on the radar map.

static PIELIGHT		tileColours[MAX_TILES];
static UDWORD		*radarBuffer;

static PIELIGHT clanColours[MAX_PLAYERS]=
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

static SDWORD RadarScrollX;
static SDWORD RadarScrollY;
static SDWORD RadarWidth;
static SDWORD RadarHeight;
static SDWORD RadVisWidth;
static SDWORD RadVisHeight;
static SDWORD RadarOffsetX;
static SDWORD RadarOffsetY;
static UWORD RadarZoom;
static SDWORD RadarMapOriginX;
static SDWORD RadarMapOriginY;
static SDWORD RadarMapWidth;
static SDWORD RadarMapHeight;

static void CalcRadarPixelSize(UWORD *SizeH,UWORD *SizeV);
static void CalcRadarScroll(UWORD boxSizeH,UWORD boxSizeV);
static void ClearRadar(UDWORD *screen);
static void DrawRadarTiles(UDWORD *screen,UDWORD Modulus,UWORD boxSizeH,UWORD boxSizeV);
static void DrawRadarObjects(UDWORD *screen,UDWORD Modulus,UWORD boxSizeH,UWORD boxSizeV);
static void DrawRadarExtras(UWORD boxSizeH,UWORD boxSizeV);


void radarInitVars(void)
{
	RadarScrollX = 0;
	RadarScrollY = 0;
	RadarWidth = RADWIDTH;
	RadarHeight = RADHEIGHT;
	RadarOffsetX = 0;
	RadarOffsetY = 0;
	RadarZoom = 0;
}

//called for when a new mission is started
void resetRadarRedraw(void)
{
	// nothing here now
}

BOOL InitRadar(void)
{
	radarBuffer = malloc(RADWIDTH * RADHEIGHT * sizeof(UDWORD));
	if (radarBuffer == NULL)
	{
		return FALSE;
	}
	memset(radarBuffer, 0, RADWIDTH * RADHEIGHT * sizeof(UDWORD));

	// Ally/enemy/me colors
	colRadarAlly = WZCOL_YELLOW;
	colRadarEnemy = WZCOL_RED;
	colRadarMe = WZCOL_WHITE;

	pie_InitRadar();

	return TRUE;
}


BOOL ShutdownRadar(void)
{
	pie_ShutdownRadar();

	free(radarBuffer);
	radarBuffer = NULL;

	return TRUE;
}


void SetRadarZoom(UWORD ZoomLevel)
{
	ASSERT( ZoomLevel <= MAX_RADARZOOM,"SetRadarZoom: Max radar zoom exceeded" );

	if (ZoomLevel != RadarZoom)
	{
		RadarZoom = ZoomLevel;
	}
}

UDWORD GetRadarZoom(void)
{
	return RadarZoom;
}


// Given a position within the radar, return a world coordinate.
//
void CalcRadarPosition(UDWORD mX,UDWORD mY,UDWORD *PosX,UDWORD *PosY)
{
	UWORD	boxSizeH,boxSizeV;
	SDWORD	sPosX, sPosY;
	SDWORD Xoffset,Yoffset;

	CalcRadarPixelSize(&boxSizeH,&boxSizeV);
	CalcRadarScroll(boxSizeH,boxSizeV);

	// Calculate where on the radar we clicked
	Xoffset=mX-RADTLX-RadarOffsetX;
	// we need to check for negative values (previously this meant that sPosX/Y were becoming huge)
	if (Xoffset<0) Xoffset=0;

	Yoffset=mY-RADTLY-RadarOffsetY;
	if (Yoffset<0) Yoffset=0;

	sPosX = ((Xoffset)/boxSizeH)+RadarScrollX+RadarMapOriginX;
	sPosY = ((Yoffset)/boxSizeV)+RadarScrollY+RadarMapOriginY;

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
	*PosX = (UDWORD)sPosX;
	*PosY = (UDWORD)sPosY;
}


// Calculate the radar pixel sizes.
//
static void CalcRadarPixelSize(UWORD *SizeH,UWORD *SizeV)
{
	UWORD Size = (UWORD)(1<<RadarZoom);

	*SizeH = Size;
	*SizeV = Size;
}


// Calculate the radar scroll positions from the current player position.
//
static void CalcRadarScroll(UWORD boxSizeH,UWORD boxSizeV)
{
	SDWORD viewX,viewY;
	SDWORD BorderX;
	SDWORD BorderY;

	RadarMapOriginX = scrollMinX;
	RadarMapOriginY = scrollMinY;
	RadarMapWidth = scrollMaxX- scrollMinX;
	RadarMapHeight = scrollMaxY - scrollMinY;

	RadVisWidth = RadarWidth;
	RadVisHeight = RadarHeight;

	if(RadarMapWidth < RadVisWidth/boxSizeH) {
		RadVisWidth = RadarMapWidth*boxSizeH;
		RadarOffsetX = (RadarWidth-RadVisWidth)/2;
	} else {
		RadarOffsetX = 0;
	}

	if(RadarMapHeight < RadVisHeight/boxSizeV) {
		RadVisHeight = RadarMapHeight*boxSizeV;
		RadarOffsetY = (RadarHeight-RadVisHeight)/2;
	} else {
		RadarOffsetY = 0;
	}

	BorderX = (RadVisWidth - visibleTiles.x*boxSizeH) / 2;
	BorderY = (RadVisHeight - visibleTiles.y*boxSizeV) / 2;
	BorderX /= boxSizeH;
	BorderY /= boxSizeV;

	if(BorderX > 16) {
		BorderX = 16;
	} else if(BorderX < 0) {
		BorderX = 0;
	}

	if(BorderY > 16) {
		BorderY = 16;
	} else if(BorderY < 0) {
		BorderY = 0;
	}

	viewX = ((player.p.x/TILE_UNITS)-RadarScrollX-RadarMapOriginX);
	viewY = ((player.p.z/TILE_UNITS)-RadarScrollY-RadarMapOriginY);

	if(viewX < BorderX) {
		RadarScrollX += viewX-BorderX;
	}

	viewX += visibleTiles.x;
	if(viewX > (RadVisWidth/boxSizeH)-BorderX) {
		RadarScrollX += viewX-(RadVisWidth/boxSizeH)+BorderX;
	}

	if(viewY < BorderY) {
		RadarScrollY += viewY-BorderY;
	}

	viewY += visibleTiles.y;
	if(viewY > (RadVisHeight/boxSizeV)-BorderY) {
		RadarScrollY += viewY-(RadVisHeight/boxSizeV)+BorderY;
	}

	if(RadarScrollX < 0) {
		RadarScrollX = 0;
	} else if(RadarScrollX > RadarMapWidth-(RadVisWidth/boxSizeH)) {
		RadarScrollX = RadarMapWidth-(RadVisWidth/boxSizeH);
	}

	if(RadarScrollY < 0) {
		RadarScrollY = 0;
	} else if(RadarScrollY > RadarMapHeight-(RadVisHeight/boxSizeV)) {
		RadarScrollY = RadarMapHeight-(RadVisHeight/boxSizeV);
	}
}


void drawRadar(void)
{
	UWORD	boxSizeH,boxSizeV;
	static int frameSkip = 0;

	CalcRadarPixelSize(&boxSizeH,&boxSizeV);
	CalcRadarScroll(boxSizeH,boxSizeV);

	if(frameSkip<=0)
	{
		if (RadVisWidth != RadarWidth || RadVisHeight != RadarHeight)
		{
			ClearRadar(radarBuffer);
		}
		DrawRadarTiles(radarBuffer, RADWIDTH, boxSizeH, boxSizeV);
		DrawRadarObjects(radarBuffer, RADWIDTH, boxSizeH, boxSizeV);
		pie_DownLoadRadar(radarBuffer, RADWIDTH, RADHEIGHT);
		frameSkip=RADAR_FRAME_SKIP;
	}
	frameSkip--;

	iV_TransBoxFill( RADTLX,RADTLY, RADTLX + RADWIDTH, RADTLY + RADHEIGHT);

	pie_RenderRadar(RADTLX, RADTLY, RADWIDTH, RADHEIGHT);
	DrawRadarExtras(boxSizeH,boxSizeV);
}


// Clear the radar buffer.
//
static void ClearRadar(UDWORD *screen)
{
	SDWORD i, j;
	UDWORD *pScr = screen;

	for (i = 0; i < RadarWidth; i++)
	{
		for (j = 0; j < RadarHeight; j++)
		{
			*pScr++ = WZCOL_RADAR_BACKGROUND.argb;
		}
	}
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
			PIELIGHT col = tileColours[TileNumber_tile(WTile->texture)];

			col.byte.r = (col.byte.r * 3 + WTile->height) / 4;
			col.byte.b = (col.byte.b * 3 + WTile->height) / 4;
			col.byte.g = (col.byte.g * 3 + WTile->height) / 4;
			WScr = col;
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
static void DrawRadarTiles(UDWORD *screen,UDWORD Modulus,UWORD boxSizeH,UWORD boxSizeV)
{
	MAPTILE	*psTile;
	UWORD	SizeH = boxSizeH;
	UWORD	SizeV = boxSizeV;
	SDWORD	VisWidth = RadVisWidth;
	SDWORD	VisHeight = RadVisHeight;
	SDWORD	i, j, EndY = VisHeight;
	SDWORD	OffsetX = RadarOffsetX;
	SDWORD	OffsetY = RadarOffsetY;
	UDWORD	*Scr = screen + OffsetX + OffsetY * Modulus;

	ASSERT( (SizeV!=0) && (SizeV!=0) ,"Zero pixel size" );

	/* Get pointer to very first tile */
	psTile = psMapTiles + RadarScrollX + RadarScrollY*mapWidth;
	psTile += RadarMapOriginX + RadarMapOriginY*mapWidth;

	if(SizeH==1)
	{
		for (i=0; i<EndY; i+=SizeH)
		{
			MAPTILE *WTile = psTile;
			UDWORD *WScr = Scr;

			for (j=0; j<VisWidth; j+=SizeV)
			{
				if (!getRevealStatus() || TEST_TILE_VISIBLE(selectedPlayer, WTile) || godMode)
				{
					*WScr = appliedRadarColour(radarDrawMode, WTile).argb;
				} else {
					*WScr = WZCOL_RADAR_BACKGROUND.argb;
				}
				/* Next pixel, next tile */
				WScr++;
				WTile++;
			}
			Scr += Modulus;
			psTile += mapWidth;
		}
	}
	else
	{
		for (i=0; i<EndY; i+=SizeV)
		{
			MAPTILE *WTile = psTile;

			for (j=0; j<VisWidth; j+=SizeH)
			{
				/* Only draw if discovered or in GOD mode */
				if (!getRevealStatus() || TEST_TILE_VISIBLE(selectedPlayer, WTile) || godMode)
				{
					PIELIGHT col = tileColours[TileNumber_tile(WTile->texture)];
					UDWORD Val, c, d;
					UDWORD *Ptr = Scr + j + i * Modulus;

					col.byte.r = sqrtf(col.byte.r * WTile->illumination);
					col.byte.b = sqrtf(col.byte.b * WTile->illumination);
					col.byte.g = sqrtf(col.byte.g * WTile->illumination);
					Val = col.argb;

   					for(c=0; c<SizeV; c++)
   					{
   						UDWORD *WPtr = Ptr;

   						for(d=0; d<SizeH; d++)
   						{
							*WPtr = appliedRadarColour(radarDrawMode, WTile).argb;
   							WPtr++;
   						}
   						Ptr += Modulus;
   					}
				}
				else
				{
					UDWORD c, d;
   					UDWORD *Ptr = Scr + j + i * Modulus;

   					for(c=0; c<SizeV; c++)
   					{
   						UDWORD *WPtr = Ptr;

   						for(d=0; d<SizeH; d++)
   						{
   							*WPtr = WZCOL_RADAR_BACKGROUND.argb;
   							WPtr++;
   						}
   						Ptr += Modulus;
   					}
				}
				WTile++;
			}
			psTile += mapWidth;
		}
	}
}


// Draw the droids and structure positions on the radar.
//
static void DrawRadarObjects(UDWORD *screen,UDWORD Modulus,UWORD boxSizeH,UWORD boxSizeV)
{
	SDWORD				c,d;
	UBYTE				clan;
	SDWORD				x = 0, y = 0;
	DROID				*psDroid;
	STRUCTURE			*psStruct;
	PROXIMITY_DISPLAY	*psProxDisp;
	VIEW_PROXIMITY		*psViewProx;
	UDWORD				*Ptr,*WPtr;
	SDWORD				SizeH,SizeV;
	SDWORD				bw,bh;
	SDWORD				SSizeH,SSizeV;
	SDWORD				VisWidth;
	SDWORD				VisHeight;
	SDWORD				OffsetX;
	SDWORD				OffsetY;
	PIELIGHT			playerCol, col;
	PIELIGHT			flashCol;

	SizeH = boxSizeH;
	SizeV = boxSizeV;
	VisWidth = RadVisWidth;
	VisHeight = RadVisHeight;
	OffsetX = RadarOffsetX;
	OffsetY = RadarOffsetY;

   	/* Show droids on map - go through all players */
   	for(clan = 0; clan < MAX_PLAYERS; clan++)
	{
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
   		for(psDroid = apsDroidLists[clan]; psDroid != NULL;
   			psDroid = psDroid->psNext)
   		{
			if (psDroid->visible[selectedPlayer]
			    || godMode
			    || (bMultiPlayer && game.alliance == ALLIANCES_TEAMS
			        && aiCheckAlliances(selectedPlayer,psDroid->player)))
			{
   				x=(psDroid->pos.x/TILE_UNITS)-RadarScrollX;
   				y=(psDroid->pos.y/TILE_UNITS)-RadarScrollY;
				x -= RadarMapOriginX;
				y -= RadarMapOriginY;
				x *= boxSizeH;
				y *= boxSizeV;

				{

					if((x < VisWidth) && (y < VisHeight) && (x >= 0) && (y >= 0)) {
   						Ptr = screen + x + y*Modulus + OffsetX + OffsetY*Modulus;

						if((clan == selectedPlayer) && (gameTime-psDroid->timeLastHit < HIT_NOTIFICATION))
						{
								col = flashCol;
						}
						else
						{
								col = playerCol;
						}

   						for(c=0; c<SizeV; c++)
   						{
   							WPtr = Ptr;
   							for(d=0; d<SizeH; d++)
   							{
								*WPtr = col.argb;
   								WPtr++;
   							}
							Ptr += Modulus;
   						}
					}
				}
			}
   		}
   	}

   	/* Do the same for structures */
   	for(clan = 0; clan < MAX_PLAYERS; clan++)
   	{
		//see if have to draw enemy/ally color
		if (bEnemyAllyRadarColor)
		{
			if (clan == selectedPlayer)
			{
				playerCol = colRadarMe;
			}
			else
			{
				playerCol = (aiCheckAlliances(selectedPlayer,clan) ? colRadarAlly: colRadarEnemy);
			}
		}
		else
		{
			//original 8-color mode
			playerCol = clanColours[getPlayerColour(clan)];
		}

		flashCol = flashColours[getPlayerColour(clan)];

		/* Go through all structures */
   		for(psStruct = apsStructLists[clan]; psStruct != NULL;
   			psStruct = psStruct->psNext)
   		{
			if (psStruct->visible[selectedPlayer]
			    || godMode
			    || (bMultiPlayer && game.alliance == ALLIANCES_TEAMS
			        && aiCheckAlliances(selectedPlayer,psStruct->player)))
			{
   				x=(psStruct->pos.x/TILE_UNITS)-RadarScrollX;
   				y=(psStruct->pos.y/TILE_UNITS)-RadarScrollY;
				x -= RadarMapOriginX;
				y -= RadarMapOriginY;
				x *= boxSizeH;
				y *= boxSizeV;

	// Get structures tile size.
				bw = (UWORD)psStruct->pStructureType->baseWidth;
				bh = (UWORD)psStruct->pStructureType->baseBreadth;

	// Need to offset for the structures top left corner.
				x -= bw >> 1;
				y -= bh >> 1;
				x = x&(~(boxSizeH-1));
				y = y&(~(boxSizeV-1));

					SSizeH = (SWORD)boxSizeH*bh;
					SSizeV = (SWORD)boxSizeV*bw;

					// Clip the structure box.
					if(x < 0) {
						SSizeH += x;
						x = 0;
					}

					if(y < 0) {
						SSizeV += y;
						y=0;
					}

					if(x+SSizeH > VisWidth) {
						SSizeH -= (x+SSizeH) - VisWidth;
					}

					if(y+SSizeV > VisHeight) {
						SSizeV -= (y+SSizeV) - VisHeight;
					}

					// And draw it.
					if((SSizeV > 0) && (SSizeH > 0)) {
   						Ptr = screen + x + y*Modulus + OffsetX + OffsetY*Modulus;
											if((clan == selectedPlayer) && (gameTime - psStruct->timeLastHit < HIT_NOTIFICATION))
						{
								col = flashCol;
						}
						else
						{
								col = playerCol;
						}

   						for(c=0; c<SSizeV; c++)
   						{
   							WPtr = Ptr;
   							for(d=0; d<SSizeH; d++)
   							{
   								*WPtr = col.argb;
   								WPtr++;
   							}
							Ptr += Modulus;
   						}
					}
			}
   		}
   	}

	//now set up coords for Proximity Messages - but only for selectedPlayer
   	for(psProxDisp = apsProxDisp[selectedPlayer]; psProxDisp != NULL;
   		psProxDisp = psProxDisp->psNext)
   	{
		if (psProxDisp->type == POS_PROXDATA)
		{
			psViewProx = (VIEW_PROXIMITY *)((VIEWDATA *)psProxDisp->psMessage->
				pViewData)->pData;
   			x = (psViewProx->x/TILE_UNITS)-RadarScrollX;
   			y = (psViewProx->y/TILE_UNITS)-RadarScrollY;
		}
		else if (psProxDisp->type == POS_PROXOBJ)
		{
			x = (((BASE_OBJECT *)psProxDisp->psMessage->pViewData)->pos.x /
				TILE_UNITS) - RadarScrollX;
			y = (((BASE_OBJECT *)psProxDisp->psMessage->pViewData)->pos.y /
				TILE_UNITS) - RadarScrollY;
		}
		x -= RadarMapOriginX;
		y -= RadarMapOriginY;
		x *= boxSizeH;
		y *= boxSizeV;

			psProxDisp->radarX = 0;
			psProxDisp->radarY = 0;
			if((x < VisWidth) && (y < VisHeight) && (x >= 0) && (y >= 0))
			{
				//store the coords
				psProxDisp->radarX = x + OffsetX;
				psProxDisp->radarY = y + OffsetY;
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
UDWORD	origDistance;
SDWORD	dif;

	origDistance = MAXDISTANCE;
	dif = origDistance-distance;
	if(dif <0 ) dif = 0;
	dif/=100;
	return(dif);
}

static SDWORD getLengthAdjust( void )
{
SDWORD	pitch;
UDWORD	lookingDown,lookingFar;
SDWORD	dif;

	pitch = 360 - (player.r.x/DEG_1);

	// Max at
	lookingDown = (0-MIN_PLAYER_X_ANGLE);
	lookingFar = (0-MAX_PLAYER_X_ANGLE);
	dif = pitch-lookingFar;
	if(dif <0) dif = 0;
	if(dif>(lookingDown-lookingFar)) dif = (lookingDown-lookingFar);

	return(dif/2);
}


/* Draws a Myth/FF7 style viewing window */
static void drawViewingWindow( UDWORD x, UDWORD y, UDWORD boxSizeH, UDWORD boxSizeV )
{
	Vector3i v[4], tv[4], centre;
	int	shortX, longX, yDrop, yDropVar;
	int	dif = getDistanceAdjust();
	int	dif2 = getLengthAdjust();
	PIELIGHT colour;

	shortX = ((visibleTiles.x/4)-(dif/6)) * boxSizeH;
	longX = ((visibleTiles.x/2)-(dif/4)) * boxSizeH;
	yDropVar = ((visibleTiles.y/2)-(dif2/3)) * boxSizeV;
	yDrop = ((visibleTiles.y/2)-dif2/3) * boxSizeV;

 	v[0].x = longX;
	v[0].y = -yDropVar;

	v[1].x = -longX;
	v[1].y = -yDropVar;

	v[2].x = shortX;
	v[2].y = yDrop;

	v[3].x = -shortX;
	v[3].y = yDrop;

	centre.x = RADTLX+x+(visibleTiles.x*boxSizeH)/2;
	centre.y = RADTLY+y+(visibleTiles.y*boxSizeV)/2;

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
		colour.argb = 0;
		colour.byte.a = 0x3f;
		break;

	}

	/* Send the four points to the draw routine and the clip box params */
	pie_DrawViewingWindow(tv,RADTLX,RADTLY,RADTLX+RADWIDTH,RADTLY+RADHEIGHT,colour);
}

static void DrawRadarExtras(UWORD boxSizeH,UWORD boxSizeV)
{
	SDWORD	viewX,viewY;

	viewX = ((player.p.x/TILE_UNITS)-RadarScrollX-RadarMapOriginX)*boxSizeH;
	viewY = ((player.p.z/TILE_UNITS)-RadarScrollY-RadarMapOriginY)*boxSizeV;
	viewX += RadarOffsetX;
	viewY += RadarOffsetY;

	viewX = viewX&(~(boxSizeH-1));
	viewY = viewY&(~(boxSizeV-1));

	drawViewingWindow(viewX,viewY,boxSizeH,boxSizeV);
	RenderWindowFrame(FRAME_RADAR, RADTLX - 1, RADTLY - 1, RADWIDTH + 2, RADHEIGHT + 2);
}

// Does a screen coordinate lie within the radar area?
//
BOOL CoordInRadar(int x,int y)
{
	if( (x >=RADTLX -1 ) &&
		(x < RADTLX +RADWIDTH+1) &&
		(y >=RADTLY -1) &&
		(y < RADTLY +RADHEIGHT+1) ) {

		return TRUE;
	}

	return FALSE;
}

void radarColour(UDWORD tileNumber, uint8_t r, uint8_t g, uint8_t b)
{
	tileColours[tileNumber].byte.r = r;
	tileColours[tileNumber].byte.g = g;
	tileColours[tileNumber].byte.b = b;
	tileColours[tileNumber].byte.a = 255;
}
