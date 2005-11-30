#include "frame.h"
#include "piedef.h"
#include "rendmode.h"
#include "piematrix.h"
#include "piepalette.h"
#include "piestate.h"
#include "objects.h"
#include "display3d.h"
#include "map.h"
#include "screen.h"
#include "component.h"
#include "radar.h"
#include "mapdisplay.h"
#include "hci.h"
#include "geometry.h"
#include "intimage.h"
#include "loop.h"
#include "warcam.h"
#include "display.h"
#include "gtime.h"
#include "mission.h"



#include "multiplay.h"
#include "piefunc.h"





#define HIT_NOTIFICATION	(GAME_TICKS_PER_SEC*2)



//#define CHECKBUFFER		// Do assertions for buffer overun\underun

#define ALPHABLEND_RADAR	// Define this to do semi transparency on PSX radar.
#define BLEND_RATE	0		// Semi-transparency rate for PSX radar.
#define RADAR_3DFX_TPAGEID	31
#define RADAR_3DFX_TU	0
#define RADAR_3DFX_TV	0
#define RADAR_DRAW_VIEW_BOX		// If defined then draw a box to show the viewing area.
//#define	RADAR_TRIANGLE_TOPLEFT	// If defined then put direction triangle in top left
								// otherwise put in view box and scale it by zoom level.
#ifdef RADAR_TRIANGLE_TOPLEFT
#define RADAR_TRIANGLE_SIZE		16
#define RADAR_TRIANGLE_X	(RADTLX+2+RADAR_TRIANGLE_SIZE/2)
#define RADAR_TRIANGLE_Y	(RADTLY+2+RADAR_TRIANGLE_SIZE/2)
#else
#define RADAR_TRIANGLE_SIZE		8
#endif
#define RADAR_TRIANGLE_HEIGHT	RADAR_TRIANGLE_SIZE
#define RADAR_TRIANGLE_WIDTH	(RADAR_TRIANGLE_SIZE/2)

static UDWORD		sweep;
static UBYTE		colBlack,colWhite,colRadarBorder,colGrey;

// colours for each clan on the radar map.

#define CAMPAIGNS	3
static UDWORD		clanColours[CAMPAIGNS][MAX_PLAYERS] = 
{
{81,243,231,1,182,187,207,195}, 
{81,243,231,1,182,187,207,195}, 
{81,243,231,1,182,187,207,195}, 
};

static UDWORD		flashColours[CAMPAIGNS][MAX_PLAYERS] = 
{
{165,165,165,165,255,165,165,165}, // everything flashes red when attacked except red - it goes white!
{165,165,165,165,255,165,165,165}, 
{165,165,165,165,255,165,165,165}, 
};






static UBYTE		tileColours[NUM_TILES];
static BOOL		radarStrobe;
static UDWORD		radarStrobeX,radarStrobeY,radarStrobeIndex,sweepStrobeIndex;
static UBYTE		*radarBuffer;

static SDWORD RadarScrollX;
static SDWORD RadarScrollY;
static SDWORD RadarWidth;
static SDWORD RadarHeight;
static SDWORD RadVisWidth;
static SDWORD RadVisHeight;
static SDWORD RadarOffsetX;
static SDWORD RadarOffsetY;
static BOOL RadarRedraw;
static UWORD RadarZoom;
static IMAGEDEF RadarImage;
static SDWORD RadarMapOriginX;
static SDWORD RadarMapOriginY;
static SDWORD RadarMapWidth;
static SDWORD RadarMapHeight;

static void CalcRadarPixelSize(UWORD *SizeH,UWORD *SizeV);
static void CalcRadarScroll(UWORD boxSizeH,UWORD boxSizeV);
static void ClearRadar(UBYTE *screen,UDWORD Modulus,UWORD boxSizeH,UWORD boxSizeV);
static void DrawRadarTiles(UBYTE *screen,UDWORD Modulus,UWORD boxSizeH,UWORD boxSizeV);
static void DrawRadarObjects(UBYTE *screen,UDWORD Modulus,UWORD boxSizeH,UWORD boxSizeV);
static void DrawRadarExtras(UWORD boxSizeH,UWORD boxSizeV);
static void UpdateRadar(UWORD boxSizeH,UWORD boxSizeV);


void radarInitVars(void)
{
	sweep = 0;
	radarStrobe = TRUE;
	RadarScrollX = 0;
	RadarScrollY = 0;
	RadarWidth = RADWIDTH;
	RadarHeight = RADHEIGHT;
	RadarRedraw = TRUE;
	RadarOffsetX = 0;
	RadarOffsetY = 0;
	RadarZoom = 0;
}

//called for when a new mission is started
void resetRadarRedraw(void)
{
	RadarRedraw = TRUE;
}




BOOL InitRadar(void)
{

	radarBuffer = MALLOC(RADWIDTH*RADHEIGHT);
	if(radarBuffer==NULL) return FALSE;
	memset(radarBuffer,0,RADWIDTH*RADHEIGHT);


// Set up an image structure for the radar bitmap so we can draw
// it useing iV_DrawImageDef().

	RadarImage.TPageID = RADAR_3DFX_TPAGEID;	// 3dfx only,radar is hard coded to texture page 31 - sort this out?
	RadarImage.Tu = 0;
	RadarImage.Tv = 0;
	RadarImage.Width = (UWORD)RadarWidth;
	RadarImage.Height = (UWORD)RadarHeight;
	RadarImage.XOffset = 0;
	RadarImage.YOffset = 0;
	
	colRadarBorder	= COL_GREY;
	colBlack = 0;
	colGrey = COL_DARKGREY;
	colWhite = COL_WHITE;


//	clanColours[0] = (UDWORD)iV_PaletteNearestColour(255,255,0);
//	clanColours[1] = (UDWORD)iV_PaletteNearestColour(255,255,0);
//	clanColours[2] = (UDWORD)iV_PaletteNearestColour(255,255,0);
//	clanColours[3] = (UDWORD)iV_PaletteNearestColour(255,255,0);
//	clanColours[4] = (UDWORD)iV_PaletteNearestColour(255,255,0);
//	clanColours[5] = (UDWORD)iV_PaletteNearestColour(255,255,0);
//	clanColours[6] = (UDWORD)iV_PaletteNearestColour(255,255,0);
//	clanColours[7] = (UDWORD)iV_PaletteNearestColour(255,255,0);



	pie_InitRadar();

	return TRUE;
}


BOOL ShutdownRadar(void)
{

	pie_ShutdownRadar();

	FREE(radarBuffer);

	return TRUE;
}


void SetRadarZoom(UWORD ZoomLevel)
{
	ASSERT((ZoomLevel <= MAX_RADARZOOM,"SetRadarZoom: Max radar zoom exceeded"));


	if(ZoomLevel != RadarZoom) {
		RadarZoom = ZoomLevel;
		RadarRedraw = TRUE;
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

	//*PosX = ((mX-RADTLX-RadarOffsetX)/boxSizeH)+RadarScrollX+RadarMapOriginX;
	//*PosY = ((mY-RADTLY-RadarOffsetY)/boxSizeV)+RadarScrollY+RadarMapOriginY;
	

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

//given a world pos, return a radar pos..
// ajl did this, so don't blame paul when it barfs...
void worldPosToRadarPos(UDWORD wX,UDWORD wY,SDWORD *rX, SDWORD *rY)
{	
	SDWORD x,y;
	UWORD	boxSizeH,boxSizeV;

	CalcRadarPixelSize(&boxSizeH,&boxSizeV);
	CalcRadarScroll(boxSizeH,boxSizeV);

	x= ((wX-RadarScrollX-RadarMapOriginX) *boxSizeH)+RadarOffsetX;
	y= ((wY-RadarScrollY-RadarMapOriginY) *boxSizeV)+RadarOffsetY;

	*rX = x;
	*rY = y;
}

// Kick of a new radar strobe.
//
void SetRadarStrobe(UDWORD x,UDWORD y)
{
	radarStrobe = TRUE;
	radarStrobeX = x;
	radarStrobeY = y;
	radarStrobeIndex = 0;
}


// Calculate the radar pixel sizes.
//
static void CalcRadarPixelSize(UWORD *SizeH,UWORD *SizeV)
{

	UWORD Size = (UWORD)(1<<RadarZoom);

	*SizeH = Size;
	*SizeV = Size;

//#ifdef FORCEPIXELSIZE
//#ifndef PSX
//	*SizeH = 2;
//	*SizeV = 2;
//#else
//	*SizeH = 2;
//	*SizeV = 2;
//#endif
//#else
//	UWORD boxSizeH,boxSizeV;
//
//	boxSizeH = (UWORD)(RadarWidth/mapWidth);
//	boxSizeV = (UWORD)(RadarHeight/mapHeight);
//
//// Ensure boxSizeH and V are always 1 or greater and equal to each other.
//	if((boxSizeH == 0) || (boxSizeV ==0)) {
//		boxSizeH = boxSizeV = 1;
//	} else if(boxSizeH > boxSizeV) {
//		boxSizeV = boxSizeH;
//	} else {
//		boxSizeH = boxSizeV;
//	}
//
//	*SizeH = boxSizeH;
//	*SizeV = boxSizeV;
//#endif
}


// Calculate the radar scroll positions from the current player position.
//
static void CalcRadarScroll(UWORD boxSizeH,UWORD boxSizeV)
{
	SDWORD viewX,viewY;
	SDWORD PrevRadarOffsetX = RadarOffsetX;
	SDWORD PrevRadarOffsetY = RadarOffsetY;
	SDWORD PrevRadarScrollX = RadarScrollX;
	SDWORD PrevRadarScrollY = RadarScrollY;
	SDWORD PrevRadarMapOriginX = RadarMapOriginX;
	SDWORD PrevRadarMapOriginY = RadarMapOriginY;
	SDWORD PrevRadarMapWidth = RadarMapWidth; 
	SDWORD PrevRadarMapHeight = RadarMapHeight;

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


	BorderX = (RadVisWidth - visibleXTiles*boxSizeH) / 2;
	BorderY = (RadVisHeight - visibleYTiles*boxSizeV) / 2;
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

	viewX += visibleXTiles;
	if(viewX > (RadVisWidth/boxSizeH)-BorderX) {
		RadarScrollX += viewX-(RadVisWidth/boxSizeH)+BorderX;
	}

	if(viewY < BorderY) {
		RadarScrollY += viewY-BorderY;
	}

	viewY += visibleYTiles;
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

	if( (PrevRadarOffsetX != RadarOffsetX) ||
		(PrevRadarOffsetY != RadarOffsetY) ||
		(PrevRadarScrollX != RadarScrollX) ||
		(PrevRadarScrollY != RadarScrollY) ||
		(PrevRadarMapOriginX != RadarMapOriginX) ||
		(PrevRadarMapOriginY != RadarMapOriginY) ||
		(PrevRadarMapWidth != RadarMapWidth) ||
		(PrevRadarMapHeight != RadarMapHeight) ) {

		RadarRedraw = TRUE;
	}

}


void drawRadar(void)
{
	UWORD	boxSizeH,boxSizeV;

#ifdef TESTRADAR
	godMode = TRUE;
	scrollMinX = 0;
	scrollMinY = 0;
	scrollMaxX = scrollMinX+191;
	scrollMaxY = scrollMinY+127;
#endif



	CalcRadarPixelSize(&boxSizeH,&boxSizeV);
	CalcRadarScroll(boxSizeH,boxSizeV);

	if(RadarRedraw) {
		if((RadVisWidth != RadarWidth) || (RadVisHeight != RadarHeight)) {
			ClearRadar(radarBuffer,RADWIDTH,boxSizeH,boxSizeV);
		}
	}
	DrawRadarTiles(radarBuffer,RADWIDTH,boxSizeH,boxSizeV);
	DrawRadarObjects(radarBuffer,RADWIDTH,boxSizeH,boxSizeV);

	pie_DownLoadRadar(radarBuffer,RADAR_3DFX_TPAGEID);

	iV_TransBoxFill( RADTLX,RADTLY, RADTLX + RADWIDTH, RADTLY + RADHEIGHT);

	//iV_DrawSemiTransImageDef(&RadarImage,radarBuffer,RadarWidth,RADTLX,RADTLY,192);

	pie_RenderRadar(&RadarImage,radarBuffer,RadarWidth,RADTLX,RADTLY);


	DrawRadarExtras(boxSizeH,boxSizeV);

	UpdateRadar(boxSizeH,boxSizeV);

	RadarRedraw = FALSE;

}

void	downloadAtStartOfFrame( void )
{
	pie_DownLoadRadar(radarBuffer,RADAR_3DFX_TPAGEID);
}

static void UpdateRadar(UWORD boxSizeH,UWORD boxSizeV)
{
	if(!gamePaused())
	{
		sweep += boxSizeV;
	}

 	if(sweep >= (UDWORD)RadarHeight) {
		sweep = 0;
	}
 
	if(!gamePaused())
	{
		if(sweepStrobeIndex++>=BOX_PULSE_SIZE)
		{
			sweepStrobeIndex = 0;
		}
	}
}


// Clear the radar buffer.
//
static void ClearRadar(UBYTE *screen,UDWORD Modulus,UWORD boxSizeH,UWORD boxSizeV)
{
	SDWORD i,j;
	UBYTE *Scr,*WScr;
	SDWORD RadWidth,RadHeight;

	RadWidth = RadarWidth;
	RadHeight = RadarHeight;


	Scr = screen;
	for(i=0; i<RadWidth; i++) {
		WScr = Scr;
		for(j=0; j<RadHeight; j++) {
			*WScr = colBlack;
			WScr++;
		}
		Scr += Modulus;
	}
}

	
// Draw the map tiles on the radar.
//
static void DrawRadarTiles(UBYTE *screen,UDWORD Modulus,UWORD boxSizeH,UWORD boxSizeV)
{
	SDWORD	i,j,EndY;
	MAPTILE	*psTile;
	MAPTILE	*WTile;
	UBYTE	*Scr;
	UBYTE	*WScr;
	UDWORD	c,d;
	UBYTE *Ptr,*WPtr;
	UWORD SizeH,SizeV;
	SDWORD SweepPos;
	SDWORD VisWidth;
	SDWORD VisHeight;
	SDWORD OffsetX;
	SDWORD OffsetY;
	UBYTE ShadeDiv = 0;


	SizeH = boxSizeH;
	SizeV = boxSizeV;
	VisWidth = RadVisWidth;
	VisHeight = RadVisHeight;
	SweepPos = sweep - RadarOffsetY;
	OffsetX = RadarOffsetX;
	OffsetY = RadarOffsetY;


	ASSERT(( (SizeV!=0) && (SizeV!=0) ,"Zero pixel size" ));

	SweepPos = SweepPos&(~(SizeV-1));

	/* Get pointer to very first tile */
	psTile = psMapTiles + RadarScrollX + RadarScrollY*mapWidth;
	psTile += RadarMapOriginX + RadarMapOriginY*mapWidth;

	Scr = screen + OffsetX + OffsetY*Modulus;

	if (pie_Hardware()) {
		ShadeDiv = 4;
	}

	if(RadarRedraw) {
		EndY = VisHeight;
	} else {
		if( (SweepPos < 0) || (SweepPos >= VisHeight) ){
			return;
		}

		EndY = 1;
		Scr +=SweepPos*Modulus;
		psTile += (SweepPos/SizeV)*mapWidth;
	}

	if(SizeH==1)
	{
		for (i=0; i<EndY; i+=SizeH)
		{
			WScr = Scr;
			WTile = psTile;
			for (j=0; j<VisWidth; j+=SizeV)
			{
#ifdef CHECKBUFFER
				ASSERT(( ((UDWORD)WScr) >= radarBuffer , "WScr Onderflow"));
				ASSERT(( ((UDWORD)WScr) < ((UDWORD)radarBuffer)+RADWIDTH*RADHEIGHT , "WScr Overrun"));
#endif
				if ( TEST_TILE_VISIBLE(selectedPlayer,WTile) OR godMode)
					{
					   	*WScr = iV_SHADE_TABLE[(tileColours[(WTile->texture & TILE_NUMMASK)] * iV_PALETTE_SHADE_LEVEL+(WTile->illumination >> ShadeDiv))];

					}
				else
					{
						*WScr = colBlack;//colGrey;
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
			WTile = psTile;

			for (j=0; j<VisWidth; j+=SizeH)
			{
				/* Only draw if discovered or in GOD mode */
				if ( TEST_TILE_VISIBLE(selectedPlayer,WTile) OR godMode)
				{
					UBYTE Val = tileColours[(WTile->texture & TILE_NUMMASK)];
					Val = iV_SHADE_TABLE[(Val * iV_PALETTE_SHADE_LEVEL+
											(WTile->illumination >> ShadeDiv))];

					Ptr = Scr + j + i*Modulus;
   					for(c=0; c<SizeV; c++)
   					{
   						WPtr = Ptr;
   						for(d=0; d<SizeH; d++)
   						{
#ifdef CHECKBUFFER
							ASSERT(( ((UDWORD)WPtr) >= (UDWORD)radarBuffer , "WPtr Onderflow"));
							ASSERT(( ((UDWORD)WPtr) < ((UDWORD)radarBuffer)+RADWIDTH*RADHEIGHT , "WPtr Overrun"));
#endif

   							*WPtr = Val;
   							WPtr++;
   						}
   						Ptr += Modulus;
   					}
				}
				else
				{
   					Ptr = Scr + j + i*Modulus;
   					for(c=0; c<SizeV; c++)
   					{
   						WPtr = Ptr;
   						for(d=0; d<SizeH; d++)
   						{
#ifdef CHECKBUFFER
							ASSERT(( ((UDWORD)WPtr) >= (UDWORD)radarBuffer , "WPtr Onderflow"));
							ASSERT(( ((UDWORD)WPtr) < ((UDWORD)radarBuffer)+RADWIDTH*RADHEIGHT , "WPtr Overrun"));
#endif
   							*WPtr = colBlack;//colGrey;
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
static void DrawRadarObjects(UBYTE *screen,UDWORD Modulus,UWORD boxSizeH,UWORD boxSizeV)
{
	SDWORD				c,d;
	UBYTE				clan;
	SDWORD				x = 0, y = 0;
	DROID				*psDroid;
	STRUCTURE			*psStruct;
	PROXIMITY_DISPLAY	*psProxDisp;
	VIEW_PROXIMITY		*psViewProx;
	UBYTE				*Ptr,*WPtr;
	SDWORD				SizeH,SizeV;
	SDWORD				bw,bh;
	SDWORD				SSizeH,SSizeV;
	SDWORD				SweepPos;
	SDWORD				VisWidth;
	SDWORD				VisHeight;
	SDWORD				OffsetX;
	SDWORD				OffsetY;
	UBYTE				playerCol,col;
	UBYTE				flashCol;
	UBYTE				camNum;
	


	SizeH = boxSizeH;
	SizeV = boxSizeV;
	VisWidth = RadVisWidth;
	VisHeight = RadVisHeight;
//	SweepPos = sweep - RadarOffsetY;
	OffsetX = RadarOffsetX;
	OffsetY = RadarOffsetY;


	SweepPos = sweep - RadarOffsetY;

	if( (SweepPos < 0) || (SweepPos >= RadVisHeight) ){
		return;
	}



	camNum = getCampaignNumber()-1;


   	/* Show droids on map - go through all players */
   	for(clan = 0; clan < MAX_PLAYERS; clan++)
   	{
		playerCol = clanColours[camNum][getPlayerColour(clan)];
		flashCol = flashColours[camNum][getPlayerColour(clan)];

   		/* Go through all droids */
   		for(psDroid = apsDroidLists[clan]; psDroid != NULL;
   			psDroid = psDroid->psNext)
   		{
			if(psDroid->visible[selectedPlayer] 
				OR godMode 
				OR (bMultiPlayer && game.type == TEAMPLAY && aiCheckAlliances(selectedPlayer,psDroid->player))
				)
			{
   				x=(psDroid->x/TILE_UNITS)-RadarScrollX;
   				y=(psDroid->y/TILE_UNITS)-RadarScrollY;
				x -= RadarMapOriginX;
				y -= RadarMapOriginY;
				x *= boxSizeH;
				y *= boxSizeV;

				if(TRUE || (RadarRedraw)) {

					if((x < VisWidth) && (y < VisHeight) && (x >= 0) && (y >= 0)) {
   						Ptr = screen + x + y*Modulus + OffsetX + OffsetY*Modulus;
	// 	
						if((clan == selectedPlayer) AND (gameTime-psDroid->timeLastHit < HIT_NOTIFICATION))
						{
						   	/*
							if(gameTime%250<125)
							{
								col = playerCol;
							}
							else
							{
								col = 165;//COL_RED;
							}
							*/
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
#ifdef CHECKBUFFER
								ASSERT(( ((UDWORD)WPtr) >= (UDWORD)radarBuffer , "WPtr Onderflow"));
								ASSERT(( ((UDWORD)WPtr) < ((UDWORD)radarBuffer)+RADWIDTH*RADHEIGHT , "WPtr Overrun"));
#endif							
								*WPtr = col;	
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
		playerCol = clanColours[camNum][getPlayerColour(clan)];
		flashCol = flashColours[camNum][getPlayerColour(clan)];
   		/* Go through all structures */
   		for(psStruct = apsStructLists[clan]; psStruct != NULL;
   			psStruct = psStruct->psNext)
   		{
			if(psStruct->visible[selectedPlayer] 
				OR godMode 
				OR (bMultiPlayer && game.type == TEAMPLAY && aiCheckAlliances(selectedPlayer,psStruct->player))
				)
			{
   				x=(psStruct->x/TILE_UNITS)-RadarScrollX;
   				y=(psStruct->y/TILE_UNITS)-RadarScrollY;
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

//				if( ((y >= sweep) && (y <= sweep+(bh*boxSizeV))) || (RadarRedraw) ) {

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
											if((clan == selectedPlayer) AND (gameTime - psStruct->timeLastHit < HIT_NOTIFICATION))
						{
							/*
							if(gameTime%250<125)
							{
								col = playerCol;
							}
							else
							{
								col = 165;//COL_RED;
						   	}
							*/
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
#ifdef CHECKBUFFER
								ASSERT(( ((UDWORD)WPtr) >= (UDWORD)radarBuffer , "WPtr Onderflow"));
								ASSERT(( ((UDWORD)WPtr) < ((UDWORD)radarBuffer)+RADWIDTH*RADHEIGHT , "WPtr Overrun"));
#endif
   								*WPtr = (UBYTE)col;
   								WPtr++;
   							}
							Ptr += Modulus;
   						}
						//store the radar coords
						psStruct->radarX = (UWORD)(x + OffsetX);
						psStruct->radarY = (UWORD)(y + OffsetY);
					}
//				}
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
			x = (((BASE_OBJECT *)psProxDisp->psMessage->pViewData)->x /
				TILE_UNITS) - RadarScrollX;
			y = (((BASE_OBJECT *)psProxDisp->psMessage->pViewData)->y /
				TILE_UNITS) - RadarScrollY;
		}
		x -= RadarMapOriginX;
		y -= RadarMapOriginY;
		x *= boxSizeH;
		y *= boxSizeV;

		if(TRUE) {

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
}


// Rotate an array of 2d vectors about a given angle, also translates them after rotating.
//
void RotateVector2D(iVector *Vector,iVector *TVector,iVector *Pos,int Angle,int Count)
{
	int Cos = COS(Angle);
	int Sin = SIN(Angle);
	int ox = 0;
	int oy = 0;
	int i;
	iVector *Vec = Vector;
	iVector *TVec = TVector;

	if(Pos) {
		ox = Pos->x;
		oy = Pos->y;
	}

	for(i=0; i<Count; i++) {
		TVec->x = ( (Vec->x*Cos + Vec->y*Sin) >> FP12_SHIFT ) + ox;
		TVec->y = ( (Vec->y*Cos - Vec->x*Sin) >> FP12_SHIFT ) + oy;
		Vec++;
		TVec++;
	}
}


// Returns the world position which corresponds to the center of the radar view rectangle.
//
void GetRadarPlayerPos(UDWORD *XPos,UDWORD *YPos)
{
	*XPos = player.p.x + (visibleXTiles/2)*TILE_UNITS;
	*YPos = player.p.z + (visibleYTiles/2)*TILE_UNITS;
}

SDWORD	getDistanceAdjust( void )
{
UDWORD	origDistance;
SDWORD	dif;

	origDistance = MAXDISTANCE;
	dif = origDistance-distance;
	if(dif <0 ) dif = 0;
	dif/=100;
	return(dif);
}

SDWORD	getLengthAdjust( void )
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
void	drawViewingWindow( UDWORD x, UDWORD y, UDWORD boxSizeH,UDWORD boxSizeV )
{
iVector	v[4],tv[4],centre;
UDWORD	shortX,longX,yDrop,yDropVar;
SDWORD	dif = getDistanceAdjust();
SDWORD	dif2 = getLengthAdjust();
UDWORD	colour = 0;
UDWORD	camNumber;

	shortX = ((visibleXTiles/4)-(dif/3)) * boxSizeH;
	longX = ((visibleXTiles/2)-(dif/2)) * boxSizeH;
	yDropVar = ((visibleYTiles/2)-(dif2/3)) * boxSizeV;
	yDrop = ((visibleYTiles/2)-dif2/3) * boxSizeV;
   

 	v[0].x = -longX;
	v[0].y = -yDropVar;

	v[1].x = longX;
	v[1].y = -yDropVar;

	v[2].x = -shortX;
	v[2].y = yDrop;

	v[3].x = shortX;
	v[3].y = yDrop;

	centre.x = RADTLX+x+(visibleXTiles*boxSizeH)/2;       
	centre.y = RADTLY+y+(visibleYTiles*boxSizeV)/2;       
   	
	RotateVector2D(v,tv,&centre,player.r.y,4);
  //	iV_Line(tv[0].x,tv[0].y,tv[1].x,tv[1].y,colWhite);
  //	iV_Line(tv[1].x,tv[1].y,tv[3].x,tv[3].y,colWhite);
  //	iV_Line(tv[3].x,tv[3].y,tv[2].x,tv[2].y,colWhite);
  //	iV_Line(tv[2].x,tv[2].y,tv[0].x,tv[0].y,colWhite);

	camNumber = getCampaignNumber();
	switch(camNumber)
	{
	case 1:
		colour = 0x3fffffff;	//white
		break;
	case 2:
		colour = 0x3fffffff;	//white
		break;
	case 3:
		colour = 0x3f3fff3f;	//green?
	default:
		break;

	}

	/* Send the four points to the draw routine and the clip box params */
	pie_DrawViewingWindow(tv,RADTLX,RADTLY,RADTLX+RADWIDTH,RADTLY+RADHEIGHT,colour);
}



static void DrawRadarExtras(UWORD boxSizeH,UWORD boxSizeV)
{
//	UDWORD	viewX,viewY;
	SDWORD	viewX,viewY;
	SDWORD	offsetX,offsetY;

	offsetX = 
	offsetY = 
	viewX = ((player.p.x/TILE_UNITS)-RadarScrollX-RadarMapOriginX)*boxSizeH;
	viewY = ((player.p.z/TILE_UNITS)-RadarScrollY-RadarMapOriginY)*boxSizeV;
	viewX += RadarOffsetX;
	viewY += RadarOffsetY;

	viewX = viewX&(~(boxSizeH-1));
	viewY = viewY&(~(boxSizeV-1));

	//don't update the strobe whilst the game is paused
	if (!gameUpdatePaused())
	{
		/*
		if(radarStrobe)
		{
			iV_Line(RADTLX+radarStrobeX,RADTLY,RADTLX+radarStrobeX,RADBRY,boxPulseColours[radarStrobeIndex]);
			iV_Line(RADTLX,RADTLY+radarStrobeY,RADBRX,RADTLY+radarStrobeY,boxPulseColours[radarStrobeIndex++]);

			sweep = viewY;
			if(radarStrobeIndex >= BOX_PULSE_SIZE)
			{
				radarStrobe = FALSE;
				radarStrobeIndex = 0;
			}	
		}
		*/

	}





  		drawViewingWindow(viewX,viewY,boxSizeH,boxSizeV);
		DrawEnableLocks(FALSE);
		RenderWindowFrame(&FrameRadar,RADTLX-1,RADTLY-1,RADWIDTH+2,RADHEIGHT+2);
		DrawEnableLocks(TRUE);


		// Draw the radar border.
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



void	calcRadarColour(UBYTE *tileBitmap, UDWORD tileNumber)
{
	UDWORD	i, j;
	UBYTE	penNumber;
	UBYTE	fRed,fGreen,fBlue;
	UBYTE	red,green,blue;
	UDWORD	tRed,tGreen,tBlue;
	iColour*	psPalette;
	tRed = tGreen = tBlue = 0;
#define SAMPLES 8
#if 0
	/* Got through every pixel */
	//what all 4096 of them
	for(i=0; i<TILE_SIZE; i++)
	{
		/* Get pixel colour index */
		penNumber = (UBYTE) tileBitmap[i];
		/* Get the r,g,b components */
		red		=	_iVPALETTE[penNumber].r;			  
		green	=	_iVPALETTE[penNumber].g;
		blue	=	_iVPALETTE[penNumber].b;
		/* Add them to totals */
		tRed	+=	red;
		tGreen	+=	green;
		tBlue	+=	blue;
	}
	/* Get average of each component */
	fRed	=	(UBYTE) (tRed/TILE_SIZE);
	fGreen	=	(UBYTE) (tGreen/TILE_SIZE);
	fBlue	=	(UBYTE) (tBlue/TILE_SIZE);
#else
	//this routine only checks 64 pixels
	//offset half a step at the start
	tileBitmap += ((TILE_HEIGHT/(2*SAMPLES)) * TILE_WIDTH);
	tileBitmap += (TILE_WIDTH/(2*SAMPLES));
	psPalette = pie_GetGamePal();
	for(i=0; i<SAMPLES; i++)
	{
		for(j=0; j<SAMPLES; j++)
		{
		/* Get pixel colour index */
		penNumber = (UBYTE) tileBitmap[j*(TILE_WIDTH/SAMPLES)];//stepping across a few steps
		/* Get the r,g,b components */
		red		=	psPalette[penNumber].r;
		green	=	psPalette[penNumber].g;
		blue	=	psPalette[penNumber].b;
		/* Add them to totals */
		tRed	+=	red;
		tGreen	+=	green;
		tBlue	+=	blue;
		}
		//step down a few lines
		tileBitmap += ((TILE_HEIGHT/(SAMPLES)) * TILE_WIDTH);
	}
	/* Get average of each component */
	fRed	=	(UBYTE) (tRed/(SAMPLES*SAMPLES));
	fGreen	=	(UBYTE) (tGreen/(SAMPLES*SAMPLES));
	fBlue	=	(UBYTE) (tBlue/(SAMPLES*SAMPLES));
#endif
	tileColours[tileNumber] = (UBYTE)iV_PaletteNearestColour(fRed,fGreen,fBlue);
}
