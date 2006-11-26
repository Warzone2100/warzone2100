/*
 * Disp2D.c
 *
 * 2D display routines.
 *
 */
#ifdef DISP2D

/*
 * In order to reintroduce the 2D map editor, we will have to rewrite the
 * graphics code almost completely. It used a previous iteration of their
 * graphics interface. Although portions of it may still be in commented
 * out portions of the original source release, it is of doubtful value.
 * Better to port it to the new interface and OpenGL. - Per
 */

/* Blit a tile (rectangle) from the surface
 * to the back buffer at the given location.
 * The tile is specified by it's size and number, numbering
 * across from top left to bottom right.
 * The blit is clipped to the screen size.
 */
/*
extern void screenBlitTile(SDWORD destX, SDWORD destY,  // The location on screen
                                LPDIRECTDRAWSURFACE4 psSurf,            // The surface to blit from
                                UDWORD  width, UDWORD height,   // The size of the tile
                                UDWORD  tile);
*/

/* Blit the source rectangle of the surface
 * to the back buffer at the given location.
 * The blit is clipped to the screen size.
 */
/*
extern void screenBlit(SDWORD destX, SDWORD destY,              // The location on screen
                                LPDIRECTDRAWSURFACE4 psSurf,            // The surface to blit from
                                UDWORD  srcX, UDWORD srcY,
                                UDWORD  width, UDWORD height);  // The source rectangle from the surface
*/

#include <windows.h>
#include <string.h>
#include <stdio.h>

/* blitTile printfs */
//#define DEBUG_GROUP1
/* Surf recreate bitmap */
//#define DEBUG_GROUP2
#include "frame.h"
#include "frameint.h"
#include "map.h"
#include "disp2d.h"
#include "objects.h"
#include "display.h"
#include "loop.h"
#include "gtime.h"
#include "findpath.h"
#include "display3d.h"
#include "edit2d.h"
#include "hci.h"
#include "player.h"
#include "order.h"
#include "geometry.h"
#include "component.h"
#include "projectile.h"

//#include "netplay.h"
//#include "multiplay.h"
//#include "multistat.h"

// hack to display the collsion box
//extern 	QUAD	sBox;

/* The drag start threshold distance (in squared world coordinates) */
#define DRAG_THRESHOLD ((4*5)*(4*5))

/* The size and number of tiles for the explosion bitmap */
#define EXP_SIZE 112
#define EXP_TILES 15

/* The size of the muzzle flash tiles */
#define FLASH_SIZE 64

/* The size and number of the flame tiles */
#define FLAME_SIZE 32
#define FLAME_TILES 16

/* The size and number of bullet tiles */
#define BULLET_SIZE 8
#define BULLET_TILES 8

LPDIRECTDRAWSURFACE4		psTiles, psVidTiles;
LPDIRECTDRAWSURFACE4		psDroids, psVidDroids;
LPDIRECTDRAWSURFACE4		psExp, psVidExp;
LPDIRECTDRAWSURFACE4		psFlash, psVidFlash;
LPDIRECTDRAWSURFACE4		psFlame, psVidFlame;
LPDIRECTDRAWSURFACE4		psBullet, psVidBullet;
LPDIRECTDRAWSURFACE4		psStructures, psVidStructures;

/* Map Position of top right hand corner of the screen */
UDWORD				viewX,viewY;

/* Number of tiles across the tile texture page */
UDWORD				tilesPerLine;

/* Number of explosions across the explosion texture page */
static UDWORD		expPerLine;

/* The maximum tile number */
UDWORD				maxTexTile=0;

static BOOL			quitting = FALSE;
static BOOL			rangeCircles = FALSE;
static BOOL			visibleInfo = FALSE;
static BOOL			showStats = FALSE;
static UDWORD		godModeKeys[]={KEY_G,KEY_O,KEY_D};
static UDWORD		godModeIndex=0;
//static DROID		*psSelected = NULL;
//static STRUCTURE	*psBuilding = NULL;
//static FEATURE		*psFeature = NULL;

void showDroidRange(DROID *psDroid);
void	showGameStats( void );

/* Data for positioning a structure */
static enum _struct_posmode
{
	SPM_NONE,		// Not positioning a structure
	SPM_POS,		// Positioning a structure
	SPM_FINISHED,	// Finished positioning a structure
	SPM_VALID,		// Valid Position found
} structPosMode;

/* The current structure position */
static struct _struct_pos
{
	UDWORD			x,y;
	UDWORD			width,height;
	BASE_STATS		*psStats;
} sStructPos;

/* Mouse selection mode */
static enum _mouse_sel_mode
{
	MSM_NORMAL,		// Normal single click selection mode
	MSM_DRAGSTART,	// Might be starting a drag operation
	MSM_DRAG,		// Drag selection mode
} mouseSelMode;
static BOOL noDrag;		// Set to true when the mouse is pressed
						// and released in one frame

/* Object selection mode */
static enum _obj_sel_mode
{
	OSEL_NONE,		// Nothing selected
	OSEL_SINGLE,	// Single unit selected
	OSEL_MULTI,		// Multiple units selected
} objSelMode;
static BASE_OBJECT	*psSelected;		// The object selected if in single selection mode

/* Current drag box */
static UDWORD		dragSX,dragSY;		// Start point of drag
static UDWORD		dragCX,dragCY;		// Current drag cursor position

/* Current selection box coords */
static UDWORD		selX0,selY0, selX1,selY1;

/* Initialise the display system */
BOOL disp2DInitialise(void)
{
	UDWORD	width, height;

	screenSetTextColour(0xff,0xff,0xff);

	/* Load in the map tiles */
	if (!surfCreateFromBMP("tiles32.bmp", &psTiles, &width, &height))
	{
		return FALSE;
	}

	if (!surfCreate(&psVidTiles, width, height, DDSCAPS_OFFSCREENPLAIN,
					NULL, FALSE))
	{
		return FALSE;
	}

	if (!surfLoadFromSurface(psVidTiles, psTiles))
	{
		return FALSE;
	}

	/* Calculate how many tiles there are across the surface */
	tilesPerLine = width / TILE_SIZE2D;
	maxTexTile = (width / TILE_SIZE2D) * (height / TILE_SIZE2D);

	/* Load in the Droid Tiles */
	if (!surfCreateFromBMP("Droids4.bmp", &psDroids, &width, &height))
	{
		return FALSE;
	}

	if (!surfCreate(&psVidDroids, width, height, DDSCAPS_OFFSCREENPLAIN,
					NULL, FALSE))
	{
		return FALSE;
	}

	if (!surfLoadFromSurface(psVidDroids, psDroids))
	{
		return FALSE;
	}

	/* Load in the Explosion Tiles */
	if (!surfCreateFromBMP("explosion.bmp", &psExp, &width, &height))
	{
		return FALSE;
	}

	if (!surfCreate(&psVidExp, width, height, DDSCAPS_OFFSCREENPLAIN,
					NULL, FALSE))
	{
		return FALSE;
	}

	if (!surfLoadFromSurface(psVidExp, psExp))
	{
		return FALSE;
	}

	/* Calculate how many frames there are across the surface */
	expPerLine = width / EXP_SIZE;

	/* Load in the Muzzle Flash Tiles */
	if (!surfCreateFromBMP("flash.bmp", &psFlash, &width, &height))
	{
		return FALSE;
	}

	if (!surfCreate(&psVidFlash, width, height, DDSCAPS_OFFSCREENPLAIN,
					NULL, FALSE))
	{
		return FALSE;
	}

	if (!surfLoadFromSurface(psVidFlash, psFlash))
	{
		return FALSE;
	}

	/* Load in the Flame Tiles */
	if (!surfCreateFromBMP("flame.bmp", &psFlame, &width, &height))
	{
		return FALSE;
	}

	if (!surfCreate(&psVidFlame, width, height, DDSCAPS_OFFSCREENPLAIN,
					NULL, FALSE))
	{
		return FALSE;
	}

	if (!surfLoadFromSurface(psVidFlame, psFlame))
	{
		return FALSE;
	}

	/* Load in the Flame Tiles */
	if (!surfCreateFromBMP("missile1.bmp", &psBullet, &width, &height))
	{
		return FALSE;
	}

	if (!surfCreate(&psVidBullet, width, height, DDSCAPS_OFFSCREENPLAIN,
					NULL, FALSE))
	{
		return FALSE;
	}

	if (!surfLoadFromSurface(psVidBullet, psBullet))
	{
		return FALSE;
	}

	/* Load in the Structure Tiles */
	if (!surfCreateFromBMP("building2D.bmp", &psStructures, &width, &height))
	{
		return FALSE;
	}

	if (!surfCreate(&psVidStructures, width, height, DDSCAPS_OFFSCREENPLAIN,
					NULL, FALSE))
	{
		return FALSE;
	}

	if (!surfLoadFromSurface(psVidStructures, psStructures))
	{
		return FALSE;
	}

	structPosMode = SPM_NONE;
	mouseSelMode = MSM_NORMAL;
	noDrag = FALSE;
	objSelMode = OSEL_NONE;

	return TRUE;
}

/* Shutdown the display system */
BOOL disp2DShutdown(void)
{
	surfRelease(psVidTiles);
	surfRelease(psTiles);
	surfRelease(psVidDroids);
	surfRelease(psDroids);
	surfRelease(psVidExp);
	surfRelease(psExp);
	surfRelease(psVidFlash);
	surfRelease(psFlash);
	surfRelease(psVidFlame);
	surfRelease(psFlame);
	surfRelease(psBullet);
	surfRelease(psVidBullet);
	surfRelease(psVidStructures);
	surfRelease(psStructures);

	tilesPerLine = 0;
	maxTexTile = 0;
	expPerLine = 0;

	return TRUE;
}

/* Tidy up after a mode change */
BOOL disp2DModeChange()
{
	DBP2(("VidTiles\n"));
	if (!surfRecreate(&psVidTiles))
	{
		return FALSE;
	}
	if (!surfLoadFromSurface(psVidTiles, psTiles))
	{
		return FALSE;
	}
	DBP2(("VidDroids\n"));
	if (!surfRecreate(&psVidDroids))
	{
		return FALSE;
	}
	if (!surfLoadFromSurface(psVidDroids, psDroids))
	{
		return FALSE;
	}
	DBP2(("VidExp\n"));
	if (!surfRecreate(&psVidExp))
	{
		return FALSE;
	}
	if (!surfLoadFromSurface(psVidExp, psExp))
	{
		return FALSE;
	}
	DBP2(("VidFlash\n"));
	if (!surfRecreate(&psVidFlash))
	{
		return FALSE;
	}
	if (!surfLoadFromSurface(psVidFlash, psFlash))
	{
		return FALSE;
	}
	DBP2(("VidFlame\n"));
	if (!surfRecreate(&psVidFlame))
	{
		return FALSE;
	}
	if (!surfLoadFromSurface(psVidFlame, psFlame))
	{
		return FALSE;
	}
	DBP2(("VidBullet\n"));
	if (!surfRecreate(&psVidBullet))
	{
		return FALSE;
	}
	if (!surfLoadFromSurface(psVidBullet, psBullet))
	{
		return FALSE;
	}
	DBP2(("VidStructures\n"));
	if (!surfRecreate(&psVidStructures))
	{
		return FALSE;
	}
	if (!surfLoadFromSurface(psVidStructures, psStructures))
	{
		return FALSE;
	}
	return TRUE;
}

/* Start looking for a structure location */
void disp2DStartStructPosition(BASE_STATS *psStats)
{
	UDWORD	worldX, worldY;

	/* find the current mouse position */
	disp2DToWorld(mouseX(),mouseY(), &worldX,&worldY);

	/* Set up the structure position */
	structPosMode = SPM_POS;
	if (psStats->ref >= REF_STRUCTURE_START &&
		psStats->ref < (REF_STRUCTURE_START + REF_RANGE))
	{
		sStructPos.x = (worldX >> TILE_SHIFT) - ((STRUCTURE_STATS *)psStats)->baseWidth/2;
		sStructPos.y = (worldY >> TILE_SHIFT) - ((STRUCTURE_STATS *)psStats)->baseBreadth/2;
		sStructPos.width = ((STRUCTURE_STATS *)psStats)->baseWidth;
		sStructPos.height = ((STRUCTURE_STATS *)psStats)->baseBreadth;
		sStructPos.psStats = psStats;
	}
	else if (psStats->ref >= REF_FEATURE_START &&
			 psStats->ref < (REF_FEATURE_START + REF_RANGE))
	{
		sStructPos.x = (worldX >> TILE_SHIFT) - ((FEATURE_STATS *)psStats)->baseWidth/2;
		sStructPos.y = (worldY >> TILE_SHIFT) - ((FEATURE_STATS *)psStats)->baseBreadth/2;
		sStructPos.width = ((FEATURE_STATS *)psStats)->baseWidth;
		sStructPos.height = ((FEATURE_STATS *)psStats)->baseBreadth;
		sStructPos.psStats = psStats;
	}
	else if (psStats->ref >= REF_TEMPLATE_START &&
			 psStats->ref < (REF_TEMPLATE_START + REF_RANGE))
	{
		sStructPos.x = (worldX >> TILE_SHIFT) - 1/2;
		sStructPos.y = (worldY >> TILE_SHIFT) - 1/2;
		sStructPos.width = 1;
		sStructPos.height = 1;
		sStructPos.psStats = psStats;
	}
}


/* Stop looking for a structure location */
void disp2DStopStructPosition(void)
{
	structPosMode = SPM_NONE;
}


/* See if a structure location has been found */
BOOL disp2DGetStructPosition(UDWORD *pX, UDWORD *pY)
{
	if (structPosMode != SPM_FINISHED)
	{
		return FALSE;
	}

	*pX = sStructPos.x;
	*pY = sStructPos.y;

	structPosMode = SPM_NONE;

	return TRUE;
}




static BASE_OBJECT *getMouseObject(void)
{
	BASE_OBJECT *psMObj;
	UDWORD		i, worldX,worldY, width,breadth;
	DROID		*psCurr;
	STRUCTURE	*psCurrStruct;
	FEATURE		*psCurrFeat;

	disp2DToWorld(mouseX(),mouseY(), &worldX,&worldY);

	/* See if a droid has been clicked on */
	psMObj = NULL;
	for(i=0; i<MAX_PLAYERS; i++)
	{
		for(psCurr = apsDroidLists[i]; psCurr; psCurr = psCurr->psNext)
		{
			if ((worldX > psCurr->x - TILE_UNITS/2) &&
				(worldX < psCurr->x + TILE_UNITS/2) &&
				(worldY > psCurr->y - TILE_UNITS/2) &&
				(worldY < psCurr->y + TILE_UNITS/2))
			{
				psMObj = (BASE_OBJECT *)psCurr;
				goto found_mdroid;
			}
		}
	}
	/* Jump to here if a droid is found */
found_mdroid:
	//now check for Structure
	if (psMObj == NULL)
	{
		/* see if there is a Structure under the mouse */
	   	for(i=0; i<MAX_PLAYERS; i++)
		{
			for(psCurrStruct = apsStructLists[i]; psCurrStruct;
				psCurrStruct = psCurrStruct->psNext)
			{
				width = psCurrStruct->pStructureType->baseWidth * TILE_UNITS/2;
				breadth = psCurrStruct->pStructureType->baseBreadth * TILE_UNITS/2;
				if ((worldX > psCurrStruct->x - width) &&
					(worldX < psCurrStruct->x + width) &&
					(worldY > psCurrStruct->y - breadth) &&
					(worldY < psCurrStruct->y + breadth))
				{
					psMObj = (BASE_OBJECT *)psCurrStruct;
					goto found_mstruct;
				}
			}
		}
	}
	/* Jump to here if a struct is found */
found_mstruct:
	//if haven't clicked on a droid or a structure - try a feature!
	if (psMObj == NULL)
	{
		/* see if there is a Feature under the mouse */
		for(psCurrFeat = apsFeatureLists[0]; psCurrFeat;
			psCurrFeat = psCurrFeat->psNext)
		{
			width = psCurrFeat->psStats->baseWidth * TILE_UNITS/2;
			breadth = psCurrFeat->psStats->baseBreadth * TILE_UNITS/2;
			if ((worldX > psCurrFeat->x - width) &&
				(worldX < psCurrFeat->x + width) &&
				(worldY > psCurrFeat->y - breadth) &&
				(worldY < psCurrFeat->y + breadth))
			{
				psMObj = (BASE_OBJECT *)psCurrFeat;
				goto found_mfeat;
			}
		}
	}
	/* Jump to here if a feature is found */
found_mfeat:

	return psMObj;
}

/* Process the key presses for the 2D display */
BOOL process2DInput(void)
{
	DROID		*psCurr, *psTarget = NULL;
	BASE_OBJECT	*psMObj;
	UDWORD		worldX,worldY, numSel;

	quitting = FALSE;

	if(!keyDown(KEY_LCTRL) AND !keyDown(KEY_RCTRL))
	{
		/* Move the map around with the cursor keys */
		if (keyDown(KEY_LEFTARROW) && (mapX > 0))
		{
			mapX -= 1;
		}
		if (keyDown(KEY_RIGHTARROW) && (mapX < mapWidth-1))
		{
			mapX += 1;
		}
		if (keyDown(KEY_UPARROW) && (mapY > 0))
		{
			mapY -= 1;
		}
		if (keyDown(KEY_DOWNARROW) && (mapY < mapHeight-1))
		{
			mapY += 1;
		}
	}

	/* If the mouse is at the edge, scroll the map */
	if (mapX > 0 && mouseX() >= 0 && mouseX() <= TILE_SIZE2D/2)
	{
		mapX --;
	}
	else if (mapX < mapWidth -1 && mouseX() >= DISP_WIDTH - TILE_SIZE2D/2 &&
			 mouseX() <= DISP_WIDTH)
	{
		mapX ++;
	}
	if (mapY > 0 && mouseY() <= TILE_SIZE2D/2)
	{
		mapY --;
	}
	else if (mapY < mapWidth -1 && mouseY() >= DISP_HEIGHT - TILE_SIZE2D/2 &&
			 mouseY() <= DISP_HEIGHT)
	{
		mapY ++;
	}

/*
removed cos types are different - am
#if 0
	if (keyPressed(KEY_H))
	{
		psTile = psMapTiles;
		for(i=mapWidth*mapHeight; i>0; i--)
		{
//			psTile->type = TER_ROAD;
			psTile++;
		}
	}
#endif
*/
	/* Process mouse clicks, dependant on mouse mode */
	disp2DToWorld(mouseX(), mouseY(), &worldX,&worldY);

	if (objSelMode == OSEL_SINGLE && psSelected->died)
	{
		objSelMode = OSEL_NONE;
	}

	switch (mouseSelMode)
	{
	case MSM_NORMAL:
		/* See if we are doing a structure position */
		if (structPosMode == SPM_POS OR structPosMode == SPM_VALID)
		{
			/* Update the position of structure cursor */
			if ((sStructPos.width % 2) == 0)
			{
				sStructPos.x = ((worldX + TILE_UNITS/2) >> TILE_SHIFT) - sStructPos.width/2;
			}
			else
			{
				sStructPos.x = (worldX >> TILE_SHIFT) - sStructPos.width/2;
			}
			if ((sStructPos.height % 2) == 0)
			{
				sStructPos.y = ((worldY + TILE_UNITS/2) >> TILE_SHIFT) - sStructPos.height/2;
			}
			else
			{
				sStructPos.y = (worldY >> TILE_SHIFT) - sStructPos.height/2;
			}
			//check for valid location
			if (validLocation(sStructPos.psStats, worldX >> TILE_SHIFT, worldY >> TILE_SHIFT))
			{
				structPosMode = SPM_VALID;
			}
			else
			{
				structPosMode = SPM_POS;
			}
			//if valid position is found
			if (structPosMode == SPM_VALID)
			{
				if (mousePressed(MOUSE_LMB))
				{
					/* Finish the structure positioning */
					structPosMode = SPM_FINISHED;
				}
			}
		}
		/* Check for a mouse click to select or move a droid or to select a Structure*/
		else if (mousePressed(MOUSE_LMB))
		{
			/* Note the position of a possible drag */
			dragSX=dragCX = worldX;
			dragSY=dragCY = worldY;
			mouseSelMode = MSM_DRAGSTART;

			/* Check for a click and release in one frame */
			if (mouseReleased(MOUSE_LMB))
			{
				noDrag = TRUE;
			}
		}
		else if (mousePressed(MOUSE_RMB))
		{
			psMObj = getMouseObject();
			if (psMObj)
			{
				intObjectSelected(psMObj);
			}
		}
		break;
	case MSM_DRAGSTART:
		/* A drag might be starting */
		dragCX = worldX;
		dragCY = worldY;
		if (mouseReleased(MOUSE_LMB) || noDrag)
		{
			/* Not dragging - go back to normal mouse mode */
			mouseSelMode = MSM_NORMAL;
			noDrag = FALSE;

			psMObj = getMouseObject();

			/* Deal with what has been clicked */
			if (psMObj)
			{
				if (objSelMode == OSEL_NONE)
				{
					if (psMObj->player == selectedPlayer ||
						psMObj->type == OBJ_FEATURE)
					{
						// set the selection to this object
						psSelected = psMObj;
						psMObj->selected = TRUE;
						objSelMode = OSEL_SINGLE;
					}
				}
				else
				{
					// got selected objects
					if (psMObj->type != OBJ_STRUCTURE &&
						psMObj->player == selectedPlayer)
					{
						// change the selection to this object
						clearSelection();
						psSelected = psMObj;
						psMObj->selected = TRUE;
						objSelMode = OSEL_SINGLE;
					}
					else
					{
						orderSelectedObj(selectedPlayer, psMObj);
					}
				}
			}
			else if (objSelMode != OSEL_NONE)
			{
				/* clicked on nothing - move to it */
				orderSelectedLoc(selectedPlayer, worldX,worldY);
			}

		}
		else if (((dragCX-dragSX)*(dragCX-dragSX) +
				  (dragCY-dragSY)*(dragCY-dragSY)) >
				DRAG_THRESHOLD)
		{
			/* A drag has started */
			mouseSelMode = MSM_DRAG;
			selX0=selX1=dragSX;
			selY0=selY1=dragSY;
		}
		break;
	case MSM_DRAG:
		dragCX = worldX;
		dragCY = worldY;

		/* Copy the drag coords into the selection coords */
		if (dragSX < dragCX)
		{
			selX0 = dragSX;
			selX1 = dragCX;
		}
		else
		{
			selX0 = dragCX;
			selX1 = dragSX;
		}
		if (dragSY < dragCY)
		{
			selY0 = dragSY;
			selY1 = dragCY;
		}
		else
		{
			selY0 = dragCY;
			selY1 = dragSY;
		}

		if (mouseReleased(MOUSE_LMB))
		{
			/* Drag has finished - find all the droids in the box */
			mouseSelMode = MSM_NORMAL;
			numSel = 0;
			for(psCurr=apsDroidLists[selectedPlayer]; psCurr; psCurr=psCurr->psNext)
			{
				if (psCurr->x >= selX0 && psCurr->x <= selX1 &&
					psCurr->y >= selY0 && psCurr->y <= selY1)
				{
					psCurr->selected = TRUE;
					psSelected = (BASE_OBJECT *)psCurr;
					numSel++;
				}
			}
			if (numSel > 1)
			{
				objSelMode = OSEL_MULTI;
			}
			else if (numSel == 1)
			{
				objSelMode = OSEL_SINGLE;
			}
			else
			{
				objSelMode = OSEL_NONE;
			}
		}
		break;
	}
	/* Clear the selection with a right mouse click */
	if (mousePressed(MOUSE_RMB))
	{
		clearSelection();
		objSelMode = OSEL_NONE;
	}

/*	if (keyPressed(KEY_1))
	{
		playerStartManufacture(1);
	}
	if (keyPressed(KEY_2))
	{
		playerStartManufacture(2);
	}
	if (keyPressed(KEY_7))
	{
		playerStartManufacture(7);
	}
*/
	if (keyPressed(godModeKeys[godModeIndex]))
	{
	 godModeIndex++;
	 if (godModeIndex==3)
	 {
	  godModeIndex=0;
	  godMode =!godMode;
	 }
	}

	if (keyPressed(KEY_R))
	{
	 rangeCircles = !rangeCircles;
	}

	if (keyPressed(KEY_V))
	{
	 visibleInfo = !visibleInfo;
	}

	if (keyPressed(KEY_S))
	{
	 showStats = !showStats;
	}


	//check for deleting an object - 2D only
	if (keyPressed(KEY_DELETE) && psSelected != NULL)
	{
		switch (psSelected->type)
		{
		case OBJ_DROID:
			destroyDroid((DROID *)psSelected);
			break;
		case OBJ_STRUCTURE:
			destroyStruct((STRUCTURE *)psSelected);
			break;
		case OBJ_FEATURE:
			destroyFeature((FEATURE *)psSelected);
			break;
		}
		psSelected = NULL;
		objSelMode = OSEL_NONE;
	}

	/* Check for toggling between 2D and 3D */
	if (keyPressed(KEY_TAB))
	{
		display3D = TRUE;
		intSetMapPos(mapX << TILE_SHIFT, mapY << TILE_SHIFT);
	}

	return quitting;
}


/* Display a texture tile in 2D */
void blitTile(RECT *psDestRect, RECT *psSrcRect, UDWORD texture)
{
	LPDIRECTDRAWSURFACE4		psBack;
	DDSURFACEDESC2			sDDSDDest, sDDSDSrc;
	HRESULT					ddrval;
	POINT					sP1,sP2,sP3,sP4;
	POINT					*psP1,*psP2,*psP3,*psP4,*psPTemp;
	UBYTE					*p8Src, *p8Dest;
	UWORD					*p16Src, *p16Dest;
	SDWORD					x,y, xDir,yDir, srcInc,destInc;

	psBack = screenGetSurface();
//	surfCreate(&psBack, 640,480, DDSCAPS_SYSTEMMEMORY, NULL);

	/* Store the source rect as four points */
	sP1.x = psSrcRect->left;
	sP1.y = psSrcRect->top;
	sP2.x = psSrcRect->right;
	sP2.y = psSrcRect->top;
	sP3.x = psSrcRect->right;
	sP3.y = psSrcRect->bottom;
	sP4.x = psSrcRect->left;
	sP4.y = psSrcRect->bottom;

	/* Store pointers to the points */
	psP1 = &sP1;
	psP2 = &sP2;
	psP3 = &sP3;
	psP4 = &sP4;

	/* Now flip and rotate the source rectangle using the point pointers */
	if (texture & TILE_XFLIP)
	{
		psPTemp = psP1;
		psP1 = psP2;
		psP2 = psPTemp;
		psPTemp = psP3;
		psP3 = psP4;
		psP4 = psPTemp;
	}
	if (texture & TILE_YFLIP)
	{
		psPTemp = psP1;
		psP1 = psP4;
		psP4 = psPTemp;
		psPTemp = psP2;
		psP2 = psP3;
		psP3 = psPTemp;
	}
	DBP1(("Rotation : %d\n", (texture & TILE_ROTMASK) >> TILE_ROTSHIFT));
	switch ((texture & TILE_ROTMASK) >> TILE_ROTSHIFT)
	{
	case 1:
		psPTemp = psP1;
		psP1 = psP4;
		psP4 = psP3;
		psP3 = psP2;
		psP2 = psPTemp;
		break;
	case 2:
		psPTemp = psP1;
		psP1 = psP3;
		psP3 = psPTemp;
		psPTemp = psP4;
		psP4 = psP2;
		psP2 = psPTemp;
		break;
	case 3:
		psPTemp = psP1;
		psP1 = psP2;
		psP2 = psP3;
		psP3 = psP4;
		psP4 = psPTemp;
		break;
	}

	/* Lock the tile surface to read the info */
	sDDSDSrc.dwSize = sizeof(DDSURFACEDESC2);
//	ddrval = psVidTiles->lpVtbl->Lock(
//					psVidTiles,
//					NULL, &sDDSDSrc, DDLOCK_WAIT, NULL);
	ddrval = psTiles->lpVtbl->Lock(
					psTiles,
					NULL, &sDDSDSrc, DDLOCK_WAIT, NULL);
	if (ddrval != DD_OK)
	{
		debug( LOG_ERROR, "Lock failed for tile blit:\n%s", DDErrorToString(ddrval) );
		abort();
		return;
	}

	/* Lock the back buffer to do the blit */
	sDDSDDest.dwSize = sizeof(DDSURFACEDESC2);
	ddrval = psBack->lpVtbl->Lock(
					psBack,
					NULL, &sDDSDDest, DDLOCK_WAIT, NULL);
	if (ddrval != DD_OK)
	{
		debug( LOG_ERROR, "Lock failed for tile blit:\n%s", DDErrorToString(ddrval) );
		abort();
		return;
	}

	switch (sDDSDDest.ddpfPixelFormat.dwRGBBitCount)
	{
	case 8:
		/* See if P1 -> P2 is horizontal or vertical */
		if (psP1->y == psP2->y)
		{
			/* P1 -> P2 is horizontal */
			xDir = (psP1->x > psP2->x) ? -1 : 1;
			yDir = (psP1->y > psP4->y) ? -1 : 1;
			p8Dest = (UBYTE *)sDDSDDest.lpSurface +
					 sDDSDDest.lPitch * psDestRect->top +
					 psDestRect->left;
			destInc = sDDSDDest.lPitch - (psDestRect->right - psDestRect->left);
			p8Src = (UBYTE *)sDDSDSrc.lpSurface +
					sDDSDSrc.lPitch * psP1->y +
					psP1->x;
			if (psP1->y < psP4->y)
			{
				srcInc = sDDSDSrc.lPitch - (psP2->x - psP1->x);
			}
			else
			{
				srcInc = - sDDSDSrc.lPitch - (psP2->x - psP1->x);
			}
			/* Have to adjust start point if xDir or yDir are negative */
			if (xDir < 0)
			{
				p8Src--;
			}
			if (yDir < 0)
			{
				p8Src = p8Src - sDDSDSrc.lPitch;
			}
			for(y = psP1->y; y != psP4->y; y += yDir)
			{
				for(x = psP1->x; x != psP2->x; x += xDir)
				{
					*p8Dest++ = *p8Src;
					p8Src += xDir;
				}
				p8Dest = p8Dest + destInc;
				p8Src = p8Src + srcInc;
			}
		}
		else
		{
			/* P1 -> P2 is vertical */
			xDir = (psP1->x > psP4->x) ? -1 : 1;
			yDir = (psP1->y > psP2->y) ? -1 : 1;
			p8Dest = (UBYTE *)sDDSDDest.lpSurface +
					 sDDSDDest.lPitch * psDestRect->top +
					 psDestRect->left;
			destInc = sDDSDDest.lPitch - (psDestRect->right - psDestRect->left);
			for(x = psP1->x; x != psP4->x; x += xDir)
			{
				p8Src = (UBYTE *)sDDSDSrc.lpSurface +
						sDDSDSrc.lPitch * psP1->y +
						x;
				if (xDir < 0)
				{
					p8Src --;
				}
				if (yDir < 0)
				{
					p8Src = p8Src - sDDSDSrc.lPitch;
				}
				for(y = psP1->y; y != psP2->y; y += yDir)
				{
					*p8Dest++ = *p8Src;
					p8Src = p8Src + sDDSDSrc.lPitch * yDir;
				}
				p8Dest = p8Dest + destInc;
			}
		}
		break;
	case 16:
		/* See if P1 -> P2 is horizontal or vertical */
		if (psP1->y == psP2->y)
		{
			/* P1 -> P2 is horizontal */
			xDir = (psP1->x > psP2->x) ? -1 : 1;
			yDir = (psP1->y > psP4->y) ? -1 : 1;
			p16Dest = (UWORD *)((UBYTE *)sDDSDDest.lpSurface +
								sDDSDDest.lPitch * psDestRect->top +
								(psDestRect->left << 1));
			destInc = sDDSDDest.lPitch - ((psDestRect->right - psDestRect->left) << 1);
			p16Src = (UWORD *)((UBYTE *)sDDSDSrc.lpSurface +
							   sDDSDSrc.lPitch * psP1->y +
							   (psP1->x << 1));
			if (psP1->y < psP4->y)
			{
				srcInc = sDDSDSrc.lPitch - ((psP2->x - psP1->x) << 1);
			}
			else
			{
				srcInc = - sDDSDSrc.lPitch - ((psP2->x - psP1->x) << 1);
			}
			/* Have to adjust start point if xDir or yDir are negative */
			if (xDir < 0)
			{
				p16Src--;
			}
			if (yDir < 0)
			{
				p16Src = (UWORD *)((UBYTE *)p16Src - sDDSDSrc.lPitch);
			}
			for(y = psP1->y; y != psP4->y; y += yDir)
			{
				for(x = psP1->x; x != psP2->x; x += xDir)
				{
					*p16Dest++ = *p16Src;
					p16Src += xDir;
				}
				p16Dest = (UWORD *)((UBYTE *)p16Dest + destInc);
				p16Src = (UWORD *)((UBYTE *)p16Src + srcInc);
			}
		}
		else
		{
			/* P1 -> P2 is vertical */
			xDir = (psP1->x > psP4->x) ? -1 : 1;
			yDir = (psP1->y > psP2->y) ? -1 : 1;
			p16Dest = (UWORD *)((UBYTE *)sDDSDDest.lpSurface +
								sDDSDDest.lPitch * psDestRect->top +
								(psDestRect->left << 1));
			destInc = sDDSDDest.lPitch - ((psDestRect->right - psDestRect->left) << 1);
			for(x = psP1->x; x != psP4->x; x += xDir)
			{
				p16Src = (UWORD *)((UBYTE *)sDDSDSrc.lpSurface +
								   sDDSDSrc.lPitch * psP1->y +
								   (x << 1));
				if (xDir < 0)
				{
					p16Src --;
				}
				if (yDir < 0)
				{
					p16Src = (UWORD *)((UBYTE *)p16Src - sDDSDSrc.lPitch);
				}
				for(y = psP1->y; y != psP2->y; y += yDir)
				{
					*p16Dest++ = *p16Src;
					p16Src = (UWORD *)((UBYTE *)p16Src + sDDSDSrc.lPitch * yDir);
				}
				p16Dest = (UWORD *)((UBYTE *)p16Dest + destInc);
			}
		}
		break;
	case 24:
		break;
	case 32:
		break;
	default:
		debug( LOG_ERROR, "blitTile: Unknown pixel format" );
		abort();
	}

	ddrval = psBack->lpVtbl->Unlock(psBack, NULL);
	if (ddrval != DD_OK)
	{
		debug( LOG_ERROR, "Unlock failed for tileBlit:\n%s", DDErrorToString(ddrval) );
		abort();
	}

	ddrval = psVidTiles->lpVtbl->Unlock(psTiles, NULL);
	if (ddrval != DD_OK)
	{
		debug( LOG_ERROR, "Unlock failed for tileBlit:\n%s", DDErrorToString(ddrval) );
		abort();
	}
}


/* Display the terrain type over the normal tiles */
void dispTerrain(UDWORD x, UDWORD y, TYPE_OF_TERRAIN type)
{
//	HRESULT					ddrval;
	LPDIRECTDRAWSURFACE4		psBack;
//	DDBLTFX					sDDBltFX;
	RECT					sDestRect;

	psBack = screenGetSurface();

	sDestRect.left = x + TILE_SIZE2D/4;
	sDestRect.top = y + TILE_SIZE2D/4;
	sDestRect.right = x + 3*TILE_SIZE2D/4;
	sDestRect.bottom = y + 3*TILE_SIZE2D/4;
/*	memset(&sDDBltFX, 0, sizeof(DDBLTFX));
	sDDBltFX.dwSize = sizeof(DDBLTFX);
#if DISP_BITDEPTH == 8 // DISP_BITDEPTH is pie_GetVideoBufferDepth() now!
	switch (type)
	{
	case TER_SANDYBRUSH:
		sDDBltFX.dwFillColor = screenGetPalEntry(0x00,0xff,0x00);
		break;
	case TER_SAND:
		sDDBltFX.dwFillColor = screenGetPalEntry(0xff,0xff,0x00);
		break;
	case TER_ROAD:
		sDDBltFX.dwFillColor = screenGetPalEntry(0x77,0x77,0x77);
		break;
	case TER_WATER:
		sDDBltFX.dwFillColor = screenGetPalEntry(0x00,0x00,0xFF);
		break;
	default:
		sDDBltFX.dwFillColor = screenGetPalEntry(0x00,0x00,0x00);
		break;
	}
#endif
	ddrval = psBack->lpVtbl->Blt(psBack, &sDestRect, NULL,NULL,
								DDBLT_COLORFILL | DDBLT_WAIT, &sDDBltFX);
	if (ddrval != DD_OK)
	{
		DBERROR(("Couldn't do terrain blit\n%s", DDErrorToString(ddrval)));
	}*/
	screenTextOut(sDestRect.left, sDestRect.top, "%d", type);
}

/* Display the map with the tile x,y at the middle of the viewport */
static void display2DMap(void)
{
	LPDIRECTDRAWSURFACE4		psBack;		// The back buffer;
	SDWORD					scrX, scrY;
	RECT					sSrcRect, sDestRect;
	DROID					*psDroid;
	STRUCTURE				*psStructure;
	FEATURE					*psFeature;
	UDWORD					i,j,k, width,breadth;
	MAPTILE					*psTile;
	DDBLTFX					sDDBltFx;
	SDWORD					x,y;
	SDWORD					sx0,sy0, sx1,sy1;
	SDWORD					bmpDir;

	x = (SDWORD)mapX;
	y = (SDWORD)mapY;

	ASSERT( x < (SDWORD)mapWidth,
		"displayMap: x coord off map" );
	ASSERT( y < (SDWORD)mapHeight,
		"displayMap: y coord off map" );

	psBack = screenGetSurface();

	memset(&sDDBltFx, 0, sizeof(DDBLTFX));
	sDDBltFx.dwSize = sizeof(DDBLTFX);

	/* The coordinates give the centre point of the view - shift it to
	   the top left and clip to the map size */
	if (x < (UDWORD)(TILES_ACROSS/2))
	{
		x = 0;
	}
	else
	{
		x -= (TILES_ACROSS/2);
		if (x > (SDWORD)mapWidth - TILES_ACROSS)
		{
			x = mapWidth - TILES_ACROSS;
		}
	}
	if (y < (UDWORD)(TILES_DOWN/2))
	{
		y = 0;
	}
	else
	{
		y -= TILES_DOWN/2;
		if (y > (SDWORD)mapHeight - TILES_DOWN)
		{
			y = mapHeight - TILES_DOWN;
		}
	}
	/* Note the origin of the visible screen */
	viewX = x;
	viewY = y;

	/* Now zip through the tiles that are visible and blit them to the screen */
	screenSetTextColour(0xff,0xff,0xff);
	sDestRect.top = 0;
	sDestRect.left = 0;
	sDestRect.bottom = TILE_SIZE2D;
	sDestRect.right = TILE_SIZE2D;
	sSrcRect.top = 0;
	sSrcRect.bottom = TILE_SIZE2D;
	for(scrX = 0; scrX < DISP_WIDTH; scrX += TILE_SIZE2D)
	{
		for(scrY = 0; scrY < DISP_HEIGHT; scrY += TILE_SIZE2D)
		{
			psTile = mapTile(x,y);
//			if (psTile->tileVisible[selectedPlayer] OR godMode)
//			if  ( (psTile->tileVisBits & (1<<selectedPlayer) OR godMode))
			if ( TEST_TILE_VISIBLE(selectedPlayer,psTile) OR godMode)
			{
				sSrcRect.left = ((psTile->texture & TILE_NUMMASK)
									% tilesPerLine) << SCR_TILE_SHIFT;
				sSrcRect.right = sSrcRect.left + TILE_SIZE2D;
				sSrcRect.top = ((psTile->texture & TILE_NUMMASK)
									/ tilesPerLine) << SCR_TILE_SHIFT;
				sSrcRect.bottom = sSrcRect.top + TILE_SIZE2D;
				blitTile(&sDestRect, &sSrcRect, psTile->texture);

				/* Show the terrain type if necessary */
				if (showTerrain)
				{
					dispTerrain(scrX, scrY, TERRAIN_TYPE(psTile));
				}
			}
			sDestRect.top += TILE_SIZE2D;
			sDestRect.bottom += TILE_SIZE2D;
			y++;
		}
		sDestRect.top = 0;
		sDestRect.left += TILE_SIZE2D;
		sDestRect.bottom = TILE_SIZE2D;
		sDestRect.right += TILE_SIZE2D;
		x ++;
		y = viewY;
	}

	if (showStats)
	{
		showGameStats();
	}

	/* Display the structure positioning box */
	if (structPosMode == SPM_VALID OR structPosMode == SPM_POS)
	{
		disp2DFromWorld(sStructPos.x << TILE_SHIFT, sStructPos.y << TILE_SHIFT, &sx0,&sy0);
		sx1 = sx0 + (sStructPos.width * TILE_SIZE2D);
		sy1 = sy0 + (sStructPos.height * TILE_SIZE2D);

		screenSetLineCacheColour(outlineColour);
		//screenSetLineColour(0xff,0xff,0xff);
		screenDrawLine(sx0,sy0, sx1,sy0);
		screenDrawLine(sx0,sy0, sx0,sy1);
		screenDrawLine(sx0,sy1, sx1,sy1);
		screenDrawLine(sx1,sy1, sx1,sy0);
		screenDrawLine(sx0,sy0, sx1,sy1);
		screenDrawLine(sx0,sy1, sx1,sy0);
	}

	/* Now display the droids */
	screenSetLineColour(0xff,0xff,0xff);
	for(i=0; i<MAX_PLAYERS; i++)
	{
		for(psDroid = apsDroidLists[i]; psDroid != NULL; psDroid = psDroid->psNext)
		{
			if ((psDroid->visible[selectedPlayer] OR godMode) &&
				(((psDroid->x - TILE_SIZE2D/2) >> TILE_SHIFT) >= (viewX)) &&
				(((psDroid->x + TILE_SIZE2D/2) >> TILE_SHIFT) < (viewX + TILES_ACROSS)) &&
				(((psDroid->y - TILE_SIZE2D/2) >> TILE_SHIFT) >= (viewY)) &&
				(((psDroid->y + TILE_SIZE2D/2) >> TILE_SHIFT) < (viewY + TILES_DOWN)))

			{
				if (psDroid->sMove.Status != MOVEINACTIVE)
				{
/*					scrX = (((psDroid->sMove.targetX - (viewX << TILE_SHIFT)) * TILE_SIZE2D)
							>> TILE_SHIFT) - TILE_SIZE2D/2;
					scrY = (((psDroid->sMove.targetY - (viewY << TILE_SHIFT)) * TILE_SIZE2D)
							>> TILE_SHIFT) - TILE_SIZE2D/2;
					screenDrawLine(scrX,scrY,
						scrX+psDroid->sMove.boundX,scrY+psDroid->sMove.boundY);*/
					for(j=psDroid->sMove.Position; psDroid->sMove.MovementList[j].XCoordinate!= -1; j++)
					{
/*						scrX = ((((psDroid->sMove.MovementList[j].XCoordinate << TILE_SHIFT) + TILE_UNITS/2
								- (viewX << TILE_SHIFT)) * TILE_SIZE2D)
								>> TILE_SHIFT) - TILE_SIZE2D/2;
						scrY = ((((psDroid->sMove.MovementList[j].YCoordinate << TILE_SHIFT) + TILE_UNITS/2
								- (viewY << TILE_SHIFT)) * TILE_SIZE2D)
								>> TILE_SHIFT) - TILE_SIZE2D/2;*/
						scrX = (((psDroid->sMove.MovementList[j].XCoordinate - (viewX << TILE_SHIFT)) * TILE_SIZE2D)
								>> TILE_SHIFT) - TILE_SIZE2D/2;
						scrY = (((psDroid->sMove.MovementList[j].YCoordinate
								- (viewY << TILE_SHIFT)) * TILE_SIZE2D)
								>> TILE_SHIFT) - TILE_SIZE2D/2;
						screenDrawLine(scrX-10,scrY, scrX+10,scrY);
						screenDrawLine(scrX,scrY-10, scrX,scrY+10);
					}
				}

				scrX = (((psDroid->x - (viewX << TILE_SHIFT)) * TILE_SIZE2D)
						>> TILE_SHIFT) - TILE_SIZE2D/2;
				scrY = (((psDroid->y - (viewY << TILE_SHIFT)) * TILE_SIZE2D)
						>> TILE_SHIFT) - TILE_SIZE2D/2;

				if (visibleInfo)
				{
					screenTextOut(scrX-30,scrY-40,"0 1 2 3 4 5 6 7");
					screenTextOut(scrX-30,scrY-20,"%d %d %d %d %d %d %d %d",
						psDroid->visible[0],psDroid->visible[1],
						psDroid->visible[2],psDroid->visible[3],
						psDroid->visible[4],psDroid->visible[5],
						psDroid->visible[6],psDroid->visible[7]);
				}

				bmpDir = psDroid->direction + 180;
				bmpDir = bmpDir >= 360 ? bmpDir - 360 : bmpDir;
				bmpDir = (360 - bmpDir)/45;
				screenBlitTile(scrX, scrY, psVidDroids,
							32,32, bmpDir);
/*				switch (psDroid->action)
				{
				case DACTION_MOVE:
					screenTextOut(scrX,scrY, "Mv");
					break;
				case DACTION_MOVEFIRE:
					screenTextOut(scrX,scrY, "MvFr");
					break;
				case DACTION_ATTACK:
					screenTextOut(scrX,scrY, "Att");
					break;
				case DACTION_MOVETOATTACK:
					screenTextOut(scrX,scrY, "MvAtt");
					break;
				case DACTION_BUILD:
					screenTextOut(scrX,scrY, "Bld");
					break;
				case DACTION_BUILDWANDER:
					screenTextOut(scrX,scrY, "BldWnd");
					break;
				case DACTION_MOVETOBUILD:
					screenTextOut(scrX,scrY, "MvBld");
					break;
				default:
					if (psDroid->selected)
					{
						screenTextOut(scrX,scrY, "%s", psDroid->pName);
					}
					else
					{
						screenTextOut(scrX,scrY, "%d", psDroid->player);
					}
					break;
				}*/

				switch (psDroid->sMove.Status)
				{
				case MOVEINACTIVE:
					if (psDroid->selected)
					{
						screenTextOut(scrX,scrY, psDroid->pName);
					}
					else
					{
						screenTextOut(scrX,scrY, "%d", psDroid->player);
					}
					break;
				case MOVENAVIGATE:
					screenTextOut(scrX,scrY, "Nv");
					break;
				case MOVETURN:
					screenTextOut(scrX,scrY, "Trn");
					break;
				case MOVEPAUSE:
					screenTextOut(scrX,scrY, "Ps");
					break;
				case MOVEPOINTTOPOINT:
					screenTextOut(scrX,scrY, "Mv");
					break;
				}

				if (rangeCircles)
				{
					showDroidRange(psDroid);
				}
			}
		}
		//now display the structures
		for(psStructure = apsStructLists[i]; psStructure != NULL; psStructure = psStructure->psNext)
		{
	   		width = psStructure->pStructureType->baseWidth;
	   		breadth = psStructure->pStructureType->baseBreadth;
			if ((psStructure->visible[selectedPlayer] OR godMode) &&
				(((psStructure->x - width * TILE_SIZE2D/2) >> TILE_SHIFT)
								>= (viewX)) &&
				(((psStructure->x + width *  TILE_SIZE2D/2) >> TILE_SHIFT)
								< (viewX + TILES_ACROSS)) &&
				(((psStructure->y - breadth * TILE_SIZE2D/2) >> TILE_SHIFT)
								>= (viewY)) &&
				(((psStructure->y + breadth * TILE_SIZE2D/2) >> TILE_SHIFT)
								< (viewY + TILES_DOWN)))

			{
				scrX = (((psStructure->x - (viewX << TILE_SHIFT)) * TILE_SIZE2D)
						>> TILE_SHIFT) - TILE_SIZE2D/2;
				scrY = (((psStructure->y - (viewY << TILE_SHIFT)) * TILE_SIZE2D)
						>> TILE_SHIFT) - TILE_SIZE2D/2;

				if (visibleInfo)
				{
					screenTextOut(scrX-30,scrY-40,"0 1 2 3 4 5 6 7");
					screenTextOut(scrX-30,scrY-20,"%d %d %d %d %d %d %d %d",
						psStructure->visible[0],psStructure->visible[1],
						psStructure->visible[2],psStructure->visible[3],
						psStructure->visible[4],psStructure->visible[5],
						psStructure->visible[6],psStructure->visible[7]);
				}

				disp2DFromWorld(psStructure->x - (width * TILE_UNITS/2),
								psStructure->y - (breadth * TILE_UNITS/2),
								&scrX, &scrY);
				for (j = 0; j < breadth; j++)
				{
					for (k = 0; k < width; k++)
					{
						screenBlitTile(scrX, scrY, psVidStructures,
							32,32, psStructure->status);
						scrX += TILE_SIZE2D;
					}
					scrX -= width * TILE_SIZE2D;
					scrY += TILE_SIZE2D;
				}
				scrY -= breadth * TILE_SIZE2D;

				if (psStructure->selected)
				{
					screenTextOut(scrX-15,scrY-15, psStructure->pStructureType->pName);
				}
				else
				{
					screenTextOut(scrX-15,scrY-15, "%d", psStructure->player);
				}
			}
		}
	}
	//now display the features
	for(psFeature = apsFeatureLists[0]; psFeature != NULL; psFeature = psFeature->psNext)
	{
	   	width = psFeature->psStats->baseWidth;
	   	breadth = psFeature->psStats->baseBreadth;
		if ((psFeature->visible[selectedPlayer] OR godMode) &&
			(((psFeature->x - width * TILE_SIZE2D/2) >> TILE_SHIFT)
							>= (viewX)) &&
			(((psFeature->x + width *  TILE_SIZE2D/2) >> TILE_SHIFT)
							< (viewX + TILES_ACROSS)) &&
			(((psFeature->y - breadth * TILE_SIZE2D/2) >> TILE_SHIFT)
							>= (viewY)) &&
			(((psFeature->y + breadth * TILE_SIZE2D/2) >> TILE_SHIFT)
							< (viewY + TILES_DOWN)))

		{
			scrX = (((psFeature->x - (viewX << TILE_SHIFT)) * TILE_SIZE2D)
					>> TILE_SHIFT) - TILE_SIZE2D/2;
			scrY = (((psFeature->y - (viewY << TILE_SHIFT)) * TILE_SIZE2D)
					>> TILE_SHIFT) - TILE_SIZE2D/2;

			if (visibleInfo)
			{
				screenTextOut(scrX-30,scrY-40,"0 1 2 3 4 5 6 7");
				screenTextOut(scrX-30,scrY-20,"%d %d %d %d %d %d %d %d",
					psFeature->visible[0],psFeature->visible[1],
					psFeature->visible[2],psFeature->visible[3],
					psFeature->visible[4],psFeature->visible[5],
					psFeature->visible[6],psFeature->visible[7]);
			}

			disp2DFromWorld(psFeature->x - (width * TILE_UNITS/2),
							psFeature->y - (breadth * TILE_UNITS/2),
							&scrX, &scrY);
			for (j = 0; j < breadth; j++)
			{
				for (k = 0; k < width; k++)
				{
					screenBlitTile(scrX, scrY, psVidStructures,
						32,32, 3);
					scrX += TILE_SIZE2D;
				}
				scrX -= width * TILE_SIZE2D;
				scrY += TILE_SIZE2D;
			}
			scrY -= breadth * TILE_SIZE2D;

			if (psFeature->selected)
			{
				screenTextOut(scrX-15,scrY-15, psFeature->psStats->pName);
			}
		}
	}

	/* Draw the selection box */
	if (mouseSelMode == MSM_DRAG)
	{
		disp2DFromWorld(selX0,selY0, &sx0,&sy0);
		disp2DFromWorld(selX1,selY1, &sx1,&sy1);
		screenSetLineColour(0xff,0,0);
		screenDrawLine(sx0,sy0, sx1,sy0);
		screenDrawLine(sx1,sy0, sx1,sy1);
		screenDrawLine(sx0,sy0, sx0,sy1);
		screenDrawLine(sx0,sy1, sx1,sy1);
	}

	// hack to display the collision box
/*	screenSetLineColour(0xff,0,0);
	for(i=0; i<4; i++)
	{
		disp2DFromWorld(sBox.coords[i].x, sBox.coords[i].y, &sx0,&sy0);
		if (i!= 3)
		{
			disp2DFromWorld(sBox.coords[i+1].x, sBox.coords[i+1].y, &sx1,&sy1);
		}
		else
		{
			disp2DFromWorld(sBox.coords[0].x, sBox.coords[0].y, &sx1,&sy1);
		}

		screenDrawLine(sx0,sy0, sx1,sy1);
	}*/
}


/* Display the currently active bullets */
static void display2DBullets(void)
{
	PROJ_OBJECT			*psCurr;
	WEAPON_STATS	*psStats;
	SDWORD			scrX1,scrY1;
	SDWORD			i,j, radSquared;
	SDWORD			xDiff,yDiff;
	SDWORD			tileX,tileY, tileRad;
	UDWORD			flameFrame, expTime, expTile;
	UDWORD			flashFrame;
	DROID			*psDroid;
	SDWORD			bmpDir;

	/* All bullets are red lines for now */
	screenSetLineColour(0xff,0,0);

	/* Zip through the list and display em */
	for(psCurr = psActiveBullets; psCurr != NULL; psCurr = psCurr->psNext)
	{
		psStats = psCurr->psWStats;

		switch (psCurr->state)
		{
		case PROJ_INFLIGHT:
			/* The bullet is still on it's way to the target */
			if (proj_Direct(psStats))
			{
				if (psCurr->psSource)
				{
					psDroid = (DROID *)psCurr->psSource;
					disp2DFromWorld(psDroid->x, psDroid->y, &scrX1, &scrY1);
					bmpDir = psCurr->psSource->direction + 180;
					bmpDir = bmpDir >= 360 ? bmpDir - 360 : bmpDir;
					bmpDir = (360 - bmpDir)/45;
					flashFrame = ((gameTime >> 7) & 1) +
						(bmpDir << 1);
					screenBlitTile(scrX1 - FLASH_SIZE/2, scrY1 - FLASH_SIZE/2,
									psVidFlash, FLASH_SIZE,FLASH_SIZE,
									flashFrame);
				}
//				disp2DFromWorld(psCurr->tarX, psCurr->tarY, &scrX2, &scrY2);
//				screenDrawLine(scrX1,scrY1, scrX2,scrY2);
			}
			else
			{
				/* display the bullet as it moves */
				disp2DFromWorld(psCurr->x, psCurr->y, &scrX1,&scrY1);
				bmpDir = psCurr->direction + 180;
				bmpDir = bmpDir >= 360 ? bmpDir - 360 : bmpDir;
				bmpDir = (360 - bmpDir)/45;
				screenBlitTile(scrX1 - BULLET_SIZE/2, scrY1 - BULLET_SIZE/2,
								psVidBullet, BULLET_SIZE,BULLET_SIZE,
								bmpDir);
			}
			break;
		case PROJ_IMPACT:
			/* Blast radius weapon - display the explosion */
			expTime = psStats->radiusLife / EXP_TILES;
			expTile = (gameTime - psCurr->born) / expTime;
			disp2DFromWorld(psCurr->x, psCurr->y, &scrX1,&scrY1);
			if (expTile < EXP_TILES)
			{
				screenBlitTile(scrX1 - EXP_SIZE/2, scrY1 - EXP_SIZE/2,
								psVidExp, EXP_SIZE,EXP_SIZE, expTile);
			}
			break;
		case PROJ_POSTIMPACT:
			/* Incendiary weapon */
			tileRad = ((SDWORD)psStats->incenRadius >> TILE_SHIFT) + 1;
			radSquared = tileRad * tileRad;
			tileX = (SDWORD)psCurr->x >> TILE_SHIFT;
			tileY = (SDWORD)psCurr->y >> TILE_SHIFT;
			for(i=tileX - tileRad; i < tileX + tileRad; i++)
			{
				for(j = tileY - tileRad; j < tileY + tileRad; j++)
				{
					if (tileOnMap(i,j) &&
						(i >= (SDWORD)viewX) &&
						(i < (SDWORD)viewX + TILES_ACROSS) &&
						(j >= (SDWORD)viewY) &&
						(j < (SDWORD)viewY + TILES_DOWN))
					{
						xDiff = i - tileX;
						yDiff = j - tileY;
//						if ((mapTile(i,j)->tileVisible[selectedPlayer] || godMode) &&
						if ( ((mapTile(i,j)->tileVisBits & 1<<selectedPlayer) OR godMode) AND
						    ((xDiff*xDiff + yDiff*yDiff) < radSquared))
						{
							/* We've got us a burning tile - burn baby burn !! */
							flameFrame = ((gameTime >> 5) + i + j) % FLAME_TILES;
							screenBlitTile((i - viewX) << SCR_TILE_SHIFT,
								           (j - viewY) << SCR_TILE_SHIFT,
										   psVidFlame,
										   FLAME_SIZE,FLAME_SIZE,
										   flameFrame);
						}
					}
				}
			}
			break;
		} /* end switch */
	} /* end loop */
}

/* Display the world */
void display2DWorld(void)
{
	display2DMap();
	display2DBullets();
}

/* Return the world coords of a screen coordinate */
void disp2DToWorld(UDWORD scrX, UDWORD scrY, UDWORD *pWorldX, UDWORD *pWorldY)
{
	*pWorldX = (viewX << TILE_SHIFT) + (scrX << (TILE_SHIFT - SCR_TILE_SHIFT));
	*pWorldY = (viewY << TILE_SHIFT) + (scrY << (TILE_SHIFT - SCR_TILE_SHIFT));
}

/* Return the world coords of a screen coordinate */
void disp2DFromWorld(UDWORD worldX, UDWORD worldY, SDWORD *pScrX, SDWORD *pScrY)
{
	*pScrX = ((SDWORD)worldX - ((SDWORD)viewX << TILE_SHIFT)) >> (TILE_SHIFT - SCR_TILE_SHIFT);
	*pScrY = ((SDWORD)worldY - ((SDWORD)viewY << TILE_SHIFT)) >> (TILE_SHIFT - SCR_TILE_SHIFT);
}





void	showGameStats( void )
{
int			i;
DROID		*psCurr;
STRUCTURE	*psStruct;
UDWORD		Count;


	screenTextOut(0,0,"Current Player: %d",selectedPlayer);
	screenTextOut(200,0,"Current Frame Rate: %d",frameGetFrameRate());

	Count = 0;

	for (i=0; i<MAX_PLAYERS; i++)
	{
		if (apsDroidLists[i]!=NULL)
		{
			Count++;
		}
	}

	screenTextOut(0,16,"Number of Players: %d",Count);

	psCurr = apsDroidLists[selectedPlayer];
	Count = 0;
	while(psCurr!=NULL)
	{
		Count++;
		psCurr = psCurr->psNext;
	}

	screenTextOut(200,16,"Player has %d droids",Count);

	psStruct = apsStructLists[selectedPlayer];
	Count = 0;
	while(psStruct != NULL)
	{
		Count++;
		psStruct = psStruct->psNext;
	}
	screenTextOut(400, 16, "Player has %d Structures",Count);
	screenTextOut(400,0,"Game Ticks: %d",gameTime);

//	if(apsDroidLists[0])
//	{
//		screenTextOut(400,48,"Droid Direction : %d",apsDroidLists[0]->direction);
//		screenTextOut(400,64,"View Angle : %d",(UDWORD) ((UDWORD)player.r.y/DEG_1)%360);
//		screenTextOut(400,80,"Difference : %d",(UDWORD) ((UDWORD)player.r.y/DEG_1)%360 - apsDroidLists[0]->direction);
//		screenTextOut(400,96,"Left First : %d",leftFirst);
//	}

}

void showDroidRange(DROID *psDroid)
{
UDWORD	radius;
SDWORD	x0,y0,x1,y1;

	/* Get the droid's sensor range */
	radius = psDroid->sensorRange;

	/* Convert droid's coordinates to screen coords */
	disp2DFromWorld(psDroid->x-radius, psDroid->y-radius, &x0, &y0);
	disp2DFromWorld(psDroid->x+radius, psDroid->y+radius, &x1, &y1);

	/* Draw a circle at these coordinates of size range */
	screenDrawEllipse(x0,y0,x1,y1);
}

#endif
