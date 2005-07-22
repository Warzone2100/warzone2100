

/*
	MapDisplay - Renders the world view necessary for the intelligence map
	Alex McLean, Pumpkin Studios, EIDOS Interactive, 1997

	Makes heavy use of the functions available in display3d.c. Could have
	messed about with display3d.c to make to world render dual purpose, but
	it's neater as a separate file, as the intelligence map has special requirements
	and overlays and needs to render to a specified buffer for later use.
*/

/* ----------------------------------------------------------------------------------------- */
/* Included files */
#include <stdio.h>

/* Includes direct access to render library */
#include "ivisdef.h"
#include "piedef.h"
#include "piestate.h"

#include "piemode.h"
#include "pietexture.h"

#include "piematrix.h"
#include "vid.h"

#include "map.h"
#include "mapdisplay.h"
#include "component.h"
#include "disp2d.h"
#include "display3d.h"
#include "hci.h"
#include "intelmap.h"
#include "intimage.h"

//#include "dglide.h"
#include "texture.h"
#include "intdisplay.h"









extern UWORD ButXPos;	// From intDisplay.c
extern UWORD ButYPos;
extern UWORD ButWidth,ButHeight;
extern BOOL		godMode;


#define MAX_MAP_GRID	32
#define ROTATE_ANGLE	5

/* ----------------------------------------------------------------------------------------- */
/* Function prototypes */

/*	Sets up the intelligence map by allocating the necessary memory and assigning world
	variables for the renderer to work with */
//void		setUpIntelMap		(UDWORD width, UDWORD height);
/* Draws the intelligence map to the already setup buffer */
//void		renderIntelMap		(iVector *location, iVector *viewVector, UDWORD elevation);

/* Frees up the memory we've used */
//void		releaseIntelMap		( void );

/* Draw the grid */
//void		drawMapWorld		( iSurface *pSurface );
void		drawMapWorld		( void );

/* Draw a tile on the grid */
//void		drawMapTile				(SDWORD i, SDWORD j);//line draw nolonger used

/* Textured tile draw */
void		drawMapTile2(SDWORD i, SDWORD j);

/* Clears the map buffer prior to drawing in it */
//static	void	clearMapBuffer(iSurface *surface);
//clear text message background with gray fill
//static void clearIntelText(iSurface *pSurface);

/*fills the map buffer with intelColours prior to drawing in it*/
//static	void	fillMapBuffer(iSurface *surface);

//only used in software
/*fills the map buffer with a bitmap prior to drawing in it*/
static void	fillMapBufferWithBitmap(iSurface *surface);


void		tileLayouts(int texture);

//fill the intelColours array with the colours used for the background
//static void setUpIntelColours(void);
/* ----------------------------------------------------------------------------------------- */

static	iTexture texturePage = {6, 64, 64, NULL};
SDWORD	elevation;
iVector mapPos, mapView;
//static	iVector	oldPos, oldView;
static	SDWORD mapGridWidth, mapGridHeight, mapGridMidX, mapGridMidY;
static	SDWORD mapGridX, mapGridZ;
static	SDWORD gridDivX, gridDivZ;
static	iVector tileScreenCoords[MAX_MAP_GRID][MAX_MAP_GRID];

/*Flag to switch code for bucket sorting in renderFeatures etc 
  for the renderMapToBuffer code */
  /*This is no longer used but may be useful for testing so I've left it in - maybe
  get rid of it eventually? - AB 1/4/98*/
BOOL	doBucket = TRUE;

#define MAX_INTEL_SHADES		20

//colours used to 'paint' the background of 3D view
UDWORD	intelColours[MAX_INTEL_SHADES];

/* ----------------------------------------------------------------------------------------- */
/* Functions */
iSurface*	setUpMapSurface(UDWORD width, UDWORD height) 
{
void		*bufSpace;
iSurface	*pMapSurface;


	/*	Release the old buffer if necessary - we may use many different intel maps
		before resetting the game back to init/close */
	//releaseIntelMap();

	/* Get the required memory for the render surface */
	bufSpace = MALLOC(width*height);

	//initialise the buffer
	memset(bufSpace, 0, (width*height));

	/* Exit if we can't get it! */
	ASSERT((bufSpace!=NULL,"Can't get the memory for the map buffer"));

	/* Build our new surface */
	pMapSurface = iV_SurfaceCreate(REND_SURFACE_USR, width, height, 10, 10,bufSpace);

	/* Exit if we can't get it! */
	ASSERT((pMapSurface!=NULL,"Whoa - can't make surface for map"));

	//set up the intel colours
	//setUpIntelColours();

	/*	Return a pointer to our surface - from this they can get the rendered buffer
		as well as info about width and height etc. */
	return(pMapSurface);
}

void	releaseMapSurface(iSurface *pSurface)
{
	/* Free up old alloaction if necessary */
	if(pSurface!=NULL)
	{

		/* Free up old buffer if necessary */
		if(pSurface->buffer!=NULL)
		{
			FREE(pSurface->buffer);
		}

		FREE(pSurface);
	}
}




/* Draws the world into the current surface - set using 
   iV_RenderAssign(iV_MODE_SURFACE,pSurface) */
//void	drawMapWorld(iSurface *pSurface)
void	drawMapWorld(void)
{
	SDWORD			i,j;
	MAPTILE			*psTile;
	iVector			tileCoords;
	static UDWORD	angle = 0;

	/* How many tiles to draw on grid - calculate */
	mapGridWidth	= BUFFER_GRIDX;
	mapGridHeight	= BUFFER_GRIDY;

	/* Mid point tiles? */
	mapGridMidX		= (mapGridWidth>>1);
	mapGridMidY		= (mapGridHeight>>1);

	/* Where are we positioned? */
	mapGridX = mapPos.x>>TILE_SHIFT;
	mapGridZ = mapPos.z>>TILE_SHIFT;

	/* Pixel position inside tile */
	gridDivX = mapPos.x & (TILE_UNITS-1);
	gridDivZ = mapPos.z & (TILE_UNITS-1);

	/* Set up context */
	pie_MatBegin();

	/* Translate for the camera position */
	pie_MATTRANS(0,0,elevation);
	
	/* Rotate for the view angle */
	pie_MatRotZ(mapView.z);
	pie_MatRotX(mapView.x);
	pie_MatRotY(mapView.y);

	/* Translate to our location */
	pie_TRANSLATE(-gridDivX,-mapPos.y,gridDivZ);

	/* Rotate round */
	angle += ROTATE_ANGLE;
	if (angle > 360)
	{
		angle -= 360;
	}
	pie_MatRotY(DEG(angle) + mapPos.y);

	/* Now we're in camera and viewer context */

	for(i=0; i<mapGridWidth+1; i++)
	{
		for (j=0; j<mapGridHeight+1; j++)
		{
			psTile = mapTile(mapGridX+j,mapGridZ+i);
			tileCoords.x	= ((j - mapGridMidX)<<TILE_SHIFT);
			tileCoords.y	= psTile->height;
			tileCoords.z	= ((mapGridMidY-i)<<TILE_SHIFT);
			/* Rotate and project the tile to get its screen coords and distance away */
			tileScreenCoords[i][j].z = pie_RotProj(&tileCoords,(iPoint *)&tileScreenCoords[i][j]);
		}
	}
	
	for(i=0; i<mapGridWidth; i++)
	{
		for (j=0; j<mapGridHeight; j++)
		{
			drawMapTile2(i,j);
		}
	}

	doBucket = FALSE;
	displayFeatures();
	displayStaticObjects();
	displayDynamicObjects();
	//don't show proximity messages in this view
	//don't show Delivery Points in this view
	doBucket = TRUE;

	/* Close matrix context */
	pie_MatEnd();
}

/* unused
void	drawMapTile(SDWORD i, SDWORD j)
{
#ifdef PSX
		iV_SetOTIndex_PSX(OT2D_EXTREMEBACK);
		DBPRINTF(("drawMapTile called\n");
#endif

		 iV_Line(tileScreenCoords[i+0][j+0].x,tileScreenCoords[i+0][j+0].y,
    	 		tileScreenCoords[i+0][j+1].x,tileScreenCoords[i+0][j+1].y,255);
    	 iV_Line(tileScreenCoords[i+0][j+1].x,tileScreenCoords[i+0][j+1].y,
		 		tileScreenCoords[i+1][j+1].x,tileScreenCoords[i+1][j+1].y,255);
    	 iV_Line(tileScreenCoords[i+1][j+1].x,tileScreenCoords[i+1][j+1].y,
    	 		tileScreenCoords[i+1][j+0].x,tileScreenCoords[i+1][j+0].y,255);
    	 iV_Line(tileScreenCoords[i+1][j+0].x,tileScreenCoords[i+1][j+0].y,
    	 		tileScreenCoords[i+0][j+0].x,tileScreenCoords[i+0][j+0].y,255); 
}
*/


/* Clears the map buffer prior to drawing in it */
/*void	clearMapBuffer(iSurface *surface)
{
	UDWORD		surfaceWidth, extraWidth, height, width;
	UDWORD		*toClear;
#ifndef PSX
	toClear = (UDWORD *)surface->buffer;
	//make sure width is multiple of 4
	surfaceWidth = surface->width & 0xfffc;
	if (surfaceWidth < (UDWORD) surface->width)
	{
		surfaceWidth += 4;
	}
	extraWidth = (MSG_BUFFER_WIDTH - surfaceWidth)/4;

	for (height = 0; height < (UDWORD)(surface->height); height++)
	{
		for (width=0; width < surfaceWidth/4; width++)
		{
			*toClear++ = (UDWORD)0;
		}
		toClear += extraWidth;
	}

#endif
}
*/
/*fills the map buffer with intelColours prior to drawing in it*/
/*void	fillMapBuffer(iSurface *surface)
{
#ifdef PSX
	DBPRINTF(("fillMapBuffer not defined on psx\n");
#else
	UBYTE		*toFill;
	UDWORD		width, height, extraWidth;

	toFill = surface->buffer;
	extraWidth = MSG_BUFFER_WIDTH - surface->width;
	for (height = 0; height < (UDWORD)(surface->height); height++)
	{
		for (width=0; width < (UDWORD)(surface->width); width++)
		{
			*toFill++ = (UBYTE)intelColours[(MAX_INTEL_SHADES-1) * 
				height/surface->height];
		}
		toFill += extraWidth;
	}
#endif
}*/

//only used in software
/*fills the map buffer with a bitmap*/
void	fillMapBufferWithBitmap(iSurface *surface)
{

	UBYTE		*toFill;
	UDWORD		x, y, extraWidth, surfaceWidth, surfaceHeight, 
				bitmapWidth, bitmapHeight, xSource, ySource,
				x0, y0;
	iBitmap		*pBitmapBuffer;
	IMAGEDEF	*pImageDef;
	UDWORD		Modulus;

	
	toFill = surface->buffer;
	extraWidth = MSG_BUFFER_WIDTH - surface->width;

	pImageDef = &IntImages->ImageDefs[IMAGE_BUT0_UP];
	Modulus = IntImages->TexturePages[pImageDef->TPageID].width;

	pBitmapBuffer = IntImages->TexturePages[pImageDef->TPageID].bmp;
	x0 = (UDWORD)pImageDef->Tu + 5;
	y0 = (UDWORD)pImageDef->Tv + 5;
	//pBitmapBuffer += (x0 + y0 * Modulus);

	bitmapWidth = pImageDef->Width - 10;
	bitmapHeight = pImageDef->Height - 10;
	surfaceWidth = (UDWORD)surface->width;
	surfaceHeight = (UDWORD)surface->height;

	for (y=0; y < surfaceHeight; y++)
	{
		for (x=0; x < surfaceWidth; x++)
		{
			//get the source x/y for this destination
			xSource = x * bitmapWidth/surfaceWidth;
			ySource = y * bitmapHeight/surfaceHeight;

			*toFill++ = pBitmapBuffer[x0+xSource + (y0+ySource)*Modulus];
		}
		toFill += extraWidth;
	}

}

//clear text message background with gray fill
/*void clearIntelText(iSurface *surface)
{
#ifdef PSX
	DBPRINTF(("clearIntelText not defined on psx\n");
#else
	UBYTE		*toFill;
	UDWORD		width, height, extraWidth;

	toFill = surface->buffer;
	toFill += (MSG_BUFFER_WIDTH * surface->height);
	extraWidth = MSG_BUFFER_WIDTH - surface->width;

	for (height = 0; height < INTMAP_TEXTWINDOWHEIGHT; height++)
	{
		for (width=0; width < (UDWORD)(surface->width); width++)
		{
			*toFill++ = 224;
		}
		toFill += extraWidth;
	}
#endif
}
*/
/* This draws the tile regardless of whether the tile should be VISIBLE */
void	drawMapTile2(SDWORD i, SDWORD j)
{

UDWORD	renderFlag;
UDWORD	realX, realY;
UDWORD	tileNumber;
UDWORD	topL,botL,topR,botR;
//UDWORD	n;
iVertex p[4];
//iVertex clip[iV_POLY_MAX_POINTS];
MAPTILE	*psTile;
iPoint	offset;


	/* Get the actual tile to render */
	realX = mapGridX+j;
	realY = mapGridZ+i;

	topL = mapTile(realX,realY)->illumination+2;
	botL = mapTile(realX,realY+1)->illumination+2;
	botR = mapTile(realX+1,realY+1)->illumination+2;
	topR = mapTile(realX+1,realY)->illumination+2;
	
	/* Get a pointer to the tile we're going to render */
	psTile = mapTile(realX,realY);

	/* Draw ALL the tiles - don't check for visible - for Intelligence Screen 3D View*/
	//if ( TEST_TILE_VISIBLE(selectedPlayer, psTile) OR godMode)
 		/* get the appropriate tile texture */
 		tileNumber = psTile->texture; 
		if (pie_GetRenderEngine() == ENGINE_GLIDE)
		{
			pie_SetTexturePage(tileTexInfo[tileNumber & TILE_NUMMASK].texPage);
		}
		else
		{
			texturePage.bmp = tilesRAW[tileNumber & TILE_NUMMASK];
		}
 		  
		/* Check for flipped and rotated tiles */
		tileLayouts(tileNumber & ~TILE_NUMMASK);

		if(TRI_FLIPPED(psTile))
		{
		 	/* Get the screen coordinates to render into for the texturer */
	   		p[0].x = tileScreenCoords[i+0][j+0].x; p[0].y = tileScreenCoords[i+0][j+0].y; p[0].z = tileScreenCoords[i+0][j+0].z;
	   		p[1].x = tileScreenCoords[i+0][j+1].x; p[1].y = tileScreenCoords[i+0][j+1].y; p[1].z = tileScreenCoords[i+0][j+1].z;
	   		p[2].x = tileScreenCoords[i+1][j+0].x; p[2].y = tileScreenCoords[i+1][j+0].y; p[2].z = tileScreenCoords[i+1][j+0].z;
		   
			/* Get the U,V values for the indexing into the texture */
			p[0].u = psP1->x; p[0].v=psP1->y;
			p[1].u = psP2->x; p[1].v=psP2->y;
			p[2].u = psP4->x; p[2].v=psP4->y;

			/* Get the intensity values	for shading */
	 		p[0].g = (UBYTE)topL;
	 		p[1].g = (UBYTE)topR;
			p[2].g = (UBYTE)botL;
		}
		else
		{
			/* Get the screen coordinates to render into for the texturer */
	   		p[0].x = tileScreenCoords[i+0][j+0].x; p[0].y = tileScreenCoords[i+0][j+0].y; p[0].z = tileScreenCoords[i+0][j+0].z;
	   		p[1].x = tileScreenCoords[i+0][j+1].x; p[1].y = tileScreenCoords[i+0][j+1].y; p[1].z = tileScreenCoords[i+0][j+1].z;
	   		p[2].x = tileScreenCoords[i+1][j+1].x; p[2].y = tileScreenCoords[i+1][j+1].y; p[2].z = tileScreenCoords[i+1][j+1].z;
		   
			/* Get the U,V values for the indexing into the texture */
			p[0].u = psP1->x; p[0].v=psP1->y;
			p[1].u = psP2->x; p[1].v=psP2->y;
			p[2].u = psP3->x; p[2].v=psP3->y;

			/* Get the intensity values	for shading */
	 		p[0].g = (UBYTE)topL;
	 		p[1].g = (UBYTE)topR;
			p[2].g = (UBYTE)botR;

		}

		renderFlag = 0;
		if (pie_GetRenderEngine() == ENGINE_GLIDE)
		{
			offset.x = (tileTexInfo[tileNumber & TILE_NUMMASK].xOffset * 64); 
			offset.y = (tileTexInfo[tileNumber & TILE_NUMMASK].yOffset * 64); 
		}
		pie_DrawTriangle(p, &texturePage, renderFlag, &offset);	
		// Clip the polygon and establish how many sides it has. 
		// This routines also now clips shading and U,V values - Alex.
		if(TRI_FLIPPED(psTile))
		{
		 	/* Set up the texel coordinates */
			p[0].x = tileScreenCoords[i+0][j+1].x; p[0].y = tileScreenCoords[i+0][j+1].y; p[0].z = tileScreenCoords[i+0][j+1].z;
			p[1].x = tileScreenCoords[i+1][j+1].x; p[1].y = tileScreenCoords[i+1][j+1].y; p[1].z = tileScreenCoords[i+1][j+1].z;
			p[2].x = tileScreenCoords[i+1][j+0].x; p[2].y = tileScreenCoords[i+1][j+0].y; p[2].z = tileScreenCoords[i+1][j+0].z;
			
			/* Set up U,V */
			p[0].u = psP2->x; p[0].v=psP2->y;
   			p[1].u = psP3->x; p[1].v=psP3->y;
   			p[2].u = psP4->x; p[2].v=psP4->y;

			/* Set up shading vars */
   			p[0].g = (UBYTE)topR;
			p[1].g = (UBYTE)botR;
		  	p[2].g = (UBYTE)botL;
		}
		else
		{
			/* Set up the texel coordinates */
			p[0].x = tileScreenCoords[i+0][j+0].x; p[0].y = tileScreenCoords[i+0][j+0].y; p[0].z = tileScreenCoords[i+0][j+0].z;
   			p[1].x = tileScreenCoords[i+1][j+1].x; p[1].y = tileScreenCoords[i+1][j+1].y; p[1].z = tileScreenCoords[i+1][j+1].z;
   			p[2].x = tileScreenCoords[i+1][j+0].x; p[2].y = tileScreenCoords[i+1][j+0].y; p[2].z = tileScreenCoords[i+1][j+0].z;
			
			/* Set up U,V */
			p[0].u = psP1->x; p[0].v=psP1->y;
   			p[1].u = psP3->x; p[1].v=psP3->y;
   			p[2].u = psP4->x; p[2].v=psP4->y;

			/* Set up shading vars */
   			p[0].g = (UBYTE)topL;
			p[1].g = (UBYTE)botR;
		  	p[2].g = (UBYTE)botL;

		}
		pie_DrawTriangle(p, &texturePage, renderFlag, &offset);	

}


void	tileLayouts(int texture)
{
	
	/* Store the source rect as four points */
	sP1.x = 0;
	sP1.y = 0;
	sP2.x = 63;
	sP2.y = 0;
	sP3.x = 63; sP3.y = 63; sP4.x = 0; 	sP4.y = 63;

	/* Store pointers to the points */
	psP1 = &sP1; psP2 = &sP2; psP3 = &sP3; psP4 = &sP4;
	
	if (texture & TILE_XFLIP)
	{
		psPTemp = psP1; psP1 = psP2; psP2 = psPTemp; psPTemp = psP3; psP3 = psP4; psP4 = psPTemp;
	}
	if (texture & TILE_YFLIP)
	{
 		psPTemp = psP1;	psP1 = psP4; psP4 = psPTemp; psPTemp = psP2; psP2 = psP3; psP3 = psPTemp;
	}
	
	switch ((texture & TILE_ROTMASK) >> TILE_ROTSHIFT)
	{
	case 1:
 		psPTemp = psP1; psP1 = psP4; psP4 = psP3; psP3 = psP2; 	psP2 = psPTemp;
		break;
	case 2:
		psPTemp = psP1; psP1 = psP3; psP3 = psPTemp; psPTemp = psP4; psP4 = psP2; psP2 = psPTemp;
		break;
	case 3:
		psPTemp = psP1; psP1 = psP2; psP2 = psP3; psP3 = psP4; psP4 = psPTemp;
		break;
	}
}

// Render a Map Surface to display memory.
void renderMapSurface(iSurface *pSurface, UDWORD x, UDWORD y, UDWORD width, UDWORD height)
{

	if (!pie_Hardware())
	{
		pie_LocalRenderBegin();

		iV_ppBitmap((iBitmap*)pSurface->buffer, x, y, width, height,pSurface->width);

		pie_LocalRenderEnd();
	}

}


/* renders up to two IMDs into the surface - used by message display in Intelligence Map 
THIS HAS BEEN REPLACED BY renderResearchToBuffer()*/
/*void renderIMDToBuffer(iSurface *pSurface, iIMDShape *pIMD, iIMDShape *pIMD2,
					   UDWORD WindowX,UDWORD WindowY,UDWORD OriginX,UDWORD OriginY)
{
	static UDWORD angle = 0;

	if(!pie_Hardware())
	{
		 //Ensure all rendering is done to our bitmap and not to back or primary buffer
   		iV_RenderAssign(iV_MODE_SURFACE,pSurface);
	}

	// Empty the buffer 
	//clearMapBuffer(pSurface);
	//fill with the intelColours set up at the beginning
	//fillMapBuffer(pSurface);
	//fill with IMAGE_BUT0 graphic
	if (!pie_Hardware())
	{
		fillMapBufferWithBitmap(pSurface);
	}

	// Set identity (present) context
	pie_MatBegin();

	if (pie_Hardware())
	{
		pie_SetGeometricOffset(OriginX+10,OriginY+10);
	}
	else
	{
		pie_SetGeometricOffset(pSurface->width/2,pSurface->height/2);
	}

	// shift back
	pie_TRANSLATE(0,0,BUTTON_DEPTH);
//	pie_TRANSLATE(0,0,pIMD->sradius*8);
	scaleMatrix(RESEARCH_COMPONENT_SCALE);

	// Pitch down a bit 
	pie_MatRotX(DEG(-30));

	// Rotate round
	angle += ROTATE_ANGLE;
	if (angle > 360)
	{
		angle -= 360;
	}
	pie_MatRotY(DEG(angle));

	//draw the imds
	if (pIMD2)
	{
		pie_Draw3DShape(pIMD2, 0, 0, pie_MAX_BRIGHT_LEVEL, 0, pie_BUTTON, 0);

	}
	pie_Draw3DShape(pIMD, 0, 0, pie_MAX_BRIGHT_LEVEL, 0, pie_BUTTON, 0);

	// close matrix context
	pie_MatEnd();


	if (!pie_Hardware())
	{
		// Tell renderer we're back to back buffer 
		iV_RenderAssign(iV_MODE_4101,&rendSurface);
	}
}*/

/* renders the Research IMDs into the surface - used by message display in 
Intelligence Map */
void renderResearchToBuffer(iSurface *pSurface, RESEARCH *psResearch, 
                            UDWORD OriginX, UDWORD OriginY)
{
	static UDWORD   angle = 0;
    
    BASE_STATS      *psResGraphic;
    UDWORD          compID, IMDType;
	iVector         Rotation,Position;
	UDWORD          basePlateSize, Radius;
    SDWORD          scale = 0;
	
	if(!pie_Hardware())
	{
		 //Ensure all rendering is done to our bitmap and not to back or primary buffer
   		iV_RenderAssign(iV_MODE_SURFACE,pSurface);
    	//fill with IMAGE_BUT0 graphic
        fillMapBufferWithBitmap(pSurface);
	}

	// Set identity (present) context
	pie_MatBegin();

	if (pie_Hardware())
	{
		pie_SetGeometricOffset(OriginX+10,OriginY+10);
	}
	else
	{
		pie_SetGeometricOffset(pSurface->width/2,pSurface->height/2);
	}

	// Pitch down a bit 
	//pie_MatRotX(DEG(-30));

    // Rotate round
	angle += ROTATE_ANGLE;
	if (angle > 360)
	{
		angle -= 360;
	}
	
    Position.x = 0;
	Position.y = 0;
	Position.z = BUTTON_DEPTH;

    // Rotate round
	Rotation.x = -30;
	Rotation.y = angle;
	Rotation.z = 0;

    //draw the IMD for the research
    if (psResearch->psStat)
    {
        //we have a Stat associated with this research topic
        if  (StatIsStructure(psResearch->psStat))
        {
            //this defines how the button is drawn
			IMDType = IMDTYPE_STRUCTURESTAT;
            psResGraphic = psResearch->psStat;
            //set up the scale
			basePlateSize= getStructureStatSize((STRUCTURE_STATS*)psResearch->psStat);
			if(basePlateSize == 1)
			{
				scale = RESEARCH_COMPONENT_SCALE / 2;
                /*HACK HACK HACK! 
                if its a 'tall thin (ie tower)' structure stat with something on 
                the top - offset the position to show the object on top*/
                if (((STRUCTURE_STATS*)psResearch->psStat)->pIMD->nconnectors AND 
                    getStructureStatHeight((STRUCTURE_STATS*)psResearch->psStat) > TOWER_HEIGHT)
                {
                    Position.y -= 30;
                }
			}
			else if(basePlateSize == 2)
			{
				scale = RESEARCH_COMPONENT_SCALE / 4;
			}
			else
			{
				scale = RESEARCH_COMPONENT_SCALE / 5;
			}
        }
        else
        {
            compID = StatIsComponent(psResearch->psStat);
			if (compID != COMP_UNKNOWN)
			{
                //this defines how the button is drawn
	    		IMDType = IMDTYPE_COMPONENT;
                psResGraphic = psResearch->psStat;
		    	scale = RESEARCH_COMPONENT_SCALE;
		    }
            else
            {
                ASSERT((FALSE, "intDisplayMessageButton: invalid stat"));
                IMDType = IMDTYPE_RESEARCH;
                psResGraphic = (BASE_STATS *)psResearch;
            }
        }
    }
    else
    {
        //no Stat for this research topic so use the research topic to define what is drawn
        psResGraphic = (BASE_STATS *)psResearch;
        IMDType = IMDTYPE_RESEARCH;
    }

    //scale the research according to size of IMD
    if (IMDType == IMDTYPE_RESEARCH)
    {
       	Radius = getResearchRadius((BASE_STATS*)psResGraphic);
		if(Radius <= 100)
		{
			scale = RESEARCH_COMPONENT_SCALE / 2;
		}
		else if(Radius <= 128)
		{
			scale = RESEARCH_COMPONENT_SCALE / 3;
		}
		else if(Radius <= 256)
		{
			scale = RESEARCH_COMPONENT_SCALE / 4;
		}
		else
		{
			scale = RESEARCH_COMPONENT_SCALE / 5;
		}
    }


	/* display the IMDs */
	if(IMDType == IMDTYPE_COMPONENT) {
		displayComponentButton(psResGraphic,&Rotation,&Position,TRUE, scale);
	} else if(IMDType == IMDTYPE_RESEARCH) {
		displayResearchButton(psResGraphic,&Rotation,&Position,TRUE, scale);
	} else if(IMDType == IMDTYPE_STRUCTURESTAT) {
		displayStructureStatButton((STRUCTURE_STATS *)psResGraphic,selectedPlayer,&Rotation,
            &Position,TRUE, scale);
	} else {
		ASSERT((FALSE, "renderResearchToBuffer: Unknown PIEType"));
	}


	// close matrix context
	pie_MatEnd();

	if (!pie_Hardware())
	{
		// Tell renderer we're back to back buffer 
		iV_RenderAssign(iV_MODE_4101,&rendSurface);
	}
}














