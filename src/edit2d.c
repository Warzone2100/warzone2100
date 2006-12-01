#ifdef DISP2D
/*
 * Edit2D.c
 *
 * 2D map editing routines.
 */

#include <stdio.h>
#include <string.h>

/* Grab box printf's */
//#define DEBUG_GROUP2
/* Selection flip and rotate printf's */
//#define DEBUG_GROUP3
#include "frame.h"
#include "frameint.h"

#include "gtime.h"
#include "map.h"
#include "ivi.h"
#include "display3d.h"
#include "disp2d.h"
#include "edit2d.h"

/* Whether to show the terrain type on the map */
BOOL			showTerrain = FALSE;

/* Which terrain type to use for painting */
//static TERRAIN_TYPE	currTerrain = TER_SANDYBRUSH;

/* Which tile is to be used for painting */
static UDWORD	currTile=0;

/* The default mapping between terrain type and texture type */
//static TERRAIN_TYPE		*aDefaultType;

/* The possible states of the mouse */
typedef enum _mouse_state
{
	MS_GAME,		// Normal game mode
	MS_GRABSTART,	// LMB is down - possibly the start of a grab
	MS_GRAB,		// Creating a grab box
	MS_GRABBED,		// Grab box created but not copied to paste box
	MS_PASTESTART,	// LMB is down - possibly the start of a move
	MS_PASTE,		// Paste mode
} MOUSE_STATE;
static MOUSE_STATE	mState;

/* The coords of the drag box */
static UDWORD	dragSX,dragSY, dragEX,dragEY;

/* The coords of the selection box selSX,selSY is always at top left */
static UDWORD	selSX,selSY, selEX,selEY;

/* a paste box */
typedef struct _paste_box
{
	UDWORD		x,y, width,height;
	MAPTILE		*psTiles;
} PASTE_BOX;
PASTE_BOX	sPasteBox, sUndoBox;		// Store the undo box as a paste box.

/* Get a past box from the map */
static BOOL getBox(PASTE_BOX *psBox, UDWORD x, UDWORD y, UDWORD width, UDWORD height);
/* Store a paste box into the map */
static void putBox(PASTE_BOX *psBox, UDWORD x, UDWORD y);
/* Flip a paste box on the X axis */
static void flipBoxX(PASTE_BOX *psBox);
/* Flip a paste box on the Y axis */
static void flipBoxY(PASTE_BOX *psBox);
/* Rotate a paste box
 * If the box isn't square, width and height get swapped
 */
static void rotBox(PASTE_BOX *psBox);


/* Load in the default terrain type mapping */
/*static BOOL loadTypeMap(void)
{
	UBYTE	*pFileData;
	UDWORD	fileSize;
	UDWORD	line;
	UDWORD	texNum;
	char	aType[255], *pCurr;

	// Allocate an array to store the type mapping
	aDefaultType = MALLOC(sizeof(TERRAIN_TYPE) * maxTexTile);
	if (!aDefaultType)
	{
		DBERROR(("Out of memory"));
		return FALSE;
	}

	if (!loadFile("typemap.txt", &pFileData, &fileSize))
	{
		DBERROR(("Couldn't open typemap.txt"));
		return FALSE;
	}

	// Go through the file line by line - one mapping per line
	pCurr = (char *)pFileData;
	for (line=0; line < maxTexTile; line++)
	{
		sscanf(pCurr, "%d %s", &texNum, aType);
		if (texNum >= maxTexTile)
		{
			DBERROR(("texture number out of range in typemap.txt"));
			FREE(pFileData);
			return FALSE;
		}
		if (strcmp(aType, "grass") == 0)
		{
			aDefaultType[texNum] = TER_GRASS;
		}
		else if (strcmp(aType, "sand") == 0)
		{
			aDefaultType[texNum] = TER_SAND;
		}
		else if (strcmp(aType, "stone") == 0)
		{
			aDefaultType[texNum] = TER_STONE;
		}
		else if (strcmp(aType, "water") == 0)
		{
			aDefaultType[texNum] = TER_WATER;
		}

		// Skip to the next line
		do
		{
			pCurr ++;
		} while (*pCurr != '\n');

		if (pCurr - (char *)pFileData > (SDWORD)fileSize)
		{
			DBERROR(("Unexpected EOF in typemap.txt"));
			FREE(pFileData);
			return FALSE;
		}
	}

	FREE(pFileData);

	return TRUE;
}*/


/* Initialise the 2D editing module */
BOOL ed2dInitialise(void)
{
	/* Set the mouse mode */
	mState = MS_GAME;

	/* Initialise the paste and undo buffers */
	memset(&sPasteBox, 0, sizeof(PASTE_BOX));
	memset(&sUndoBox, 0, sizeof(PASTE_BOX));

/*	if (!loadTypeMap())
	{
		return FALSE;
	}*/

	return TRUE;
}


/* ShutDown the 2D editing module */
void ed2dShutDown(void)
{
	if (sPasteBox.psTiles != NULL)
	{
		FREE(sPasteBox.psTiles);
	}
	if (sUndoBox.psTiles != NULL)
	{
		FREE(sUndoBox.psTiles);
	}

//	FREE(aDefaultType);

}


/* Process input for 2D editing */
BOOL ed2dProcessInput(void)
{
	UDWORD		worldX,worldY;
	UDWORD		tileX,tileY, mx,my;
	PASTE_BOX	sGrabBox;
	BOOL		quitting = FALSE;

	if (keyDown(KEY_RCTRL))
	{
		worldX = worldY * 2;
	}

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

	/* Undo a cut or paste */
	if ((keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL)) &&
		keyPressed(KEY_Z))
	{
		putBox(&sUndoBox, sUndoBox.x,sUndoBox.y);
		debug( LOG_NEVER, "MS_GAME\n");
		mState = MS_GAME;
	}

	/* Copy the grab area */
	if (mState == MS_GRABBED &&
		(keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL)) &&
		keyPressed(KEY_C))
	{
		if (sPasteBox.psTiles != NULL)
		{
			FREE(sPasteBox.psTiles);
		}
		if (getBox(&sPasteBox, selSX,selSY, selEX-selSX+1,selEY-selSY+1))
		{
			debug( LOG_NEVER, "MS_GAME\n");
			mState = MS_GAME;
		}
		debug( LOG_NEVER, "Copy area: (%d,%d) %d x %d\n", sPasteBox.x,sPasteBox.y,
					sPasteBox.width,sPasteBox.height);
	}

	disp2DToWorld(mouseX(), mouseY(), &worldX,&worldY);

	/* Paste the copied area */
	if ((keyDown(KEY_LCTRL) || keyDown(KEY_RCTRL)) &&
		keyPressed(KEY_V) && sPasteBox.psTiles != NULL)
	{
		selSX = (worldX >> TILE_SHIFT) - (sPasteBox.width >> 1);
		selSY = (worldY >> TILE_SHIFT) - (sPasteBox.height >> 1);
		selEX = selSX + sPasteBox.width - 1;
		selEY = selSY + sPasteBox.height - 1;

		debug( LOG_NEVER, "MS_PASTE\n");
		mState = MS_PASTE;

		debug( LOG_NEVER, "Paste area: (%d,%d) %d x %d\n", selSX,selSY,
					sPasteBox.width,sPasteBox.height);
	}

	/* Process mouse clicks, dependant on mouse mode */
	switch (mState)
	{
	case MS_GAME:
	case MS_GRABSTART:
		if (mousePressed(MOUSE_LMB))
		{
			/* LMB is down - might have started a grab box */
			mState = MS_GRABSTART;
			disp2DToWorld(mouseX(), mouseY(), &dragSX,&dragSY);
			dragSX = dragSX >> TILE_SHIFT;
			dragSY = dragSY >> TILE_SHIFT;
			debug( LOG_NEVER, "MS_GRABSTART\n");
		}

		/* Clear the grab start if the mouse button has gone up */
		if ((mouseReleased(MOUSE_LMB) || !mouseDown(MOUSE_LMB)) && mState != MS_GAME)
		{
			debug( LOG_NEVER, "MS_GAME\n");
			mState = MS_GAME;
		}

		/* Start a grab if the mouse has moved enough */
		if (mState == MS_GRABSTART &&
			(worldX>>TILE_SHIFT != dragSX || worldY>>TILE_SHIFT != dragSY))
		{
			mState = MS_GRAB;
			selSX = selEX = dragEX = dragSX;
			selSY = selEY = dragEY = dragSY;
			debug( LOG_NEVER, "MS_GRAB\n");
			break;
		}
		else if (mouseDown(MOUSE_RMB))
		{
			disp2DToWorld(mouseX(), mouseY(), &worldX,&worldY);
			if (showTerrain)
			{
//				mapTile(worldX >> TILE_SHIFT, worldY >> TILE_SHIFT)->type = currTerrain;
			}
			else
			{
				mapTile(worldX >> TILE_SHIFT, worldY >> TILE_SHIFT)->texture = (UWORD)currTile;
//				mapTile(worldX >> TILE_SHIFT, worldY >> TILE_SHIFT)->type = currTerrain;
			}
		}
		break;

	case MS_GRAB:
		dragEX = worldX >> TILE_SHIFT;
		dragEY = worldY >> TILE_SHIFT;

		/* Make sure selSX,selSY is always the top left of the box */
		if (dragSX < dragEX)
		{
			selSX = dragSX;
			selEX = dragEX;
		}
		else
		{
			selSX = dragEX;
			selEX = dragSX;
		}
		if (dragSY < dragEY)
		{
			selSY = dragSY;
			selEY = dragEY;
		}
		else
		{
			selSY = dragEY;
			selEY = dragSY;
		}

		/* If the button's gone up, end the drag */
		if (mouseReleased(MOUSE_LMB))
		{
			mState = MS_GRABBED;
		}
		break;
	case MS_GRABBED:
	case MS_PASTESTART:
		if (!mouseDown(MOUSE_LMB) && mState != MS_GRABBED)
		{
			debug( LOG_NEVER, "MS_GRABBED\n");
			mState = MS_GRABBED;
		}
		tileX = worldX >> TILE_SHIFT;
		tileY = worldY >> TILE_SHIFT;
		if (mousePressed(MOUSE_LMB))
		{
			/* See if the click is outside the selection rectangle */

			if (tileX < selSX || tileX > selEX ||
				tileY < selSY || tileY > selEY)
			{
				/* Clear the selection */
				debug( LOG_NEVER, "MS_GAME\n");
				mState = MS_GAME;
				break;
			}
			debug( LOG_NEVER, "MS_PASTESTART\n");
			mState = MS_PASTESTART;
			dragSX = tileX;
			dragSY = tileY;
		}
		/* See if the selection has moved enough to start a paste */
		if (mouseDown(MOUSE_LMB) && mState == MS_PASTESTART &&
			dragSX != tileX && dragSY != tileY)
		{
			/* Free any old undo and paste buffer */
			if (sPasteBox.psTiles != NULL)
			{
				FREE(sPasteBox.psTiles);
			}
			if (sUndoBox.psTiles != NULL)
			{
				FREE(sUndoBox.psTiles);
			}

			/* Get the paste and undo boxes */
			if (!getBox(&sPasteBox, selSX,selSY, selEX - selSX + 1, selEY - selSY + 1))
			{
				mState = MS_GAME;
				break;
			}
			if (!getBox(&sUndoBox, selSX,selSY, selEX - selSX + 1, selEY - selSY + 1))
			{
				FREE(sPasteBox.psTiles);
				sPasteBox.psTiles = NULL;
				mState = MS_GAME;
				break;
			}

			/* Fill the removed area with the current fill tile */
			for (my = selSY; my <= selEY; my ++)
			{
				for (mx = selSX; mx <= selEX; mx++)
				{
					mapTile(mx,my)->texture = (UWORD)currTile;
//					mapTile(mx,my)->type = currTerrain;
				}
			}

			debug( LOG_NEVER, "MS_PASTE\n");
			mState = MS_PASTE;
		}
		break;
	case MS_PASTE:
		/* See if the selection should get pasted */
		/* See if the click is outside the selection rectangle */
		tileX = worldX >> TILE_SHIFT;
		tileY = worldY >> TILE_SHIFT;
		if (mousePressed(MOUSE_LMB) &&
			(tileX < selSX || tileX > selEX ||
			 tileY < selSY || tileY > selEY))
		{
			/* Free the old undo data */
			if (sUndoBox.psTiles != NULL)
			{
				FREE(sUndoBox.psTiles);
			}
			/* Get the new undo data */
			if (!getBox(&sUndoBox, selSX, selSY, sPasteBox.width,sPasteBox.height))
			{
				debug( LOG_ERROR, "Out of memory" );
				abort();
				break;
			}

			putBox(&sPasteBox, selSX, selSY);
			debug( LOG_NEVER, "MS_GAME\n");
			mState = MS_GAME;
			break;
		}

		/* Calculate the current position for the selection box */
		if (mouseDown(MOUSE_LMB))
		{
			/* Clip to the top and left of the screen */
			if (tileX > sPasteBox.width >> 1)
			{
				selSX = tileX - (sPasteBox.width >> 1);
			}
			else
			{
				selSX = 0;
			}
			if (tileY > sPasteBox.height >> 1)
			{
				selSY = tileY - (sPasteBox.height >> 1);
			}
			else
			{
				selSY = 0;
			}
			/* Clip to the bottom and right of the screen */
			if (selSX + sPasteBox.width >= mapWidth)
			{
				selSX = mapWidth - sPasteBox.width;
				selEX = mapWidth - 1;
			}
			else
			{
				selEX = selSX + sPasteBox.width - 1;
			}
			if (selSY + sPasteBox.height >= mapHeight)
			{
				selSY = mapHeight - sPasteBox.height;
				selEY = mapHeight - 1;
			}
			else
			{
				selEY = selSY + sPasteBox.height - 1;
			}
		}
		break;
	}

	if (keyPressed(KEY_T))
	{
		showTerrain = !showTerrain;
	}

	/* Do rotates, flips and copies on the current grab/paste box */
	if (mState == MS_GRABBED)
	{
		/* Flip on X axis */
		if (keyPressed(KEY_X) &&
			getBox(&sGrabBox, selSX,selSY, selEX-selSX+1,selEY-selSY+1))
		{
			debug( LOG_NEVER, "flip box X: (%d,%d) %d x %d\n", sGrabBox.x,sGrabBox.y,
						sGrabBox.width,sGrabBox.height);
			flipBoxX(&sGrabBox);
			putBox(&sGrabBox, selSX,selSY);
			FREE(sGrabBox.psTiles);
		}
		/* Flip on Y axis */
		if (keyPressed(KEY_Y) &&
			getBox(&sGrabBox, selSX,selSY, selEX-selSX+1,selEY-selSY+1))
		{
			debug( LOG_NEVER, "flip box Y: (%d,%d) %d x %d\n", sGrabBox.x,sGrabBox.y,
						sGrabBox.width,sGrabBox.height);
			flipBoxY(&sGrabBox);
			putBox(&sGrabBox, selSX,selSY);
			FREE(sGrabBox.psTiles);
		}
		/* Rotate */
		if (keyPressed(KEY_Z) &&
			selEX - selSX == selEY - selSY &&
			getBox(&sGrabBox, selSX,selSY, selEX-selSX+1,selEY-selSY+1))
		{
			debug( LOG_NEVER, "rotate box: (%d,%d) %d x %d\n", sGrabBox.x,sGrabBox.y,
						sGrabBox.width,sGrabBox.height);
			rotBox(&sGrabBox);
			putBox(&sGrabBox, selSX,selSY);
			FREE(sGrabBox.psTiles);
		}
	}
	else if (mState == MS_PASTE)
	{
		/* Flip on X axis */
		if (keyPressed(KEY_X))
		{
			flipBoxX(&sPasteBox);
		}
		/* Flip on Y axis */
		if (keyPressed(KEY_Y))
		{
			flipBoxY(&sPasteBox);
		}
		/* Rotate */
		if (keyPressed(KEY_Z))
		{
			rotBox(&sPasteBox);
			selEX = selSX + sPasteBox.width - 1;
			selEY = selSY + sPasteBox.height - 1;
		}
	}
	/* Select the current texture/terrain for painting */
	else if (showTerrain)
	{
/*		if (keyPressed(KEY_MINUS))
		{
			if (currTerrain > TER_GRASS)
			{
				currTerrain = currTerrain >> 1;
			}
			else
			{
				currTerrain = TER_WATER;
			}
		}
		if (keyPressed(KEY_EQUALS))
		{
			if (currTerrain < TER_WATER)
			{
				currTerrain = currTerrain << 1;
			}
			else
			{
				currTerrain = TER_GRASS;
			}
		}*/
	}
	else
	{
		if (keyPressed(KEY_MINUS))
		{
			if (currTile > 0)
			{
				currTile = (currTile & TILE_NUMMASK) - 1;
			}
//			currTerrain = aDefaultType[currTile];
		}
		if (keyPressed(KEY_EQUALS))
		{
			if ((currTile & TILE_NUMMASK) < maxTexTile)
			{
				currTile = (currTile & TILE_NUMMASK) + 1;
			}
//			currTerrain = aDefaultType[currTile];
		}
		if (keyPressed(KEY_X))
		{
			currTile = (currTile & TILE_NUMMASK) | (currTile & ~TILE_NUMMASK) ^ TILE_XFLIP;
		}
		if (keyPressed(KEY_Y))
		{
			currTile = (currTile & TILE_NUMMASK) | (currTile & ~TILE_NUMMASK) ^ TILE_YFLIP;
		}
		if (keyPressed(KEY_Z))
		{
			if ((currTile & TILE_ROTMASK) < (3 << TILE_ROTSHIFT))
			{
				currTile = (currTile & ~TILE_ROTMASK) | ((currTile & TILE_ROTMASK)
														+ (1 << TILE_ROTSHIFT));
			}
			else
			{
				currTile = (currTile & ~TILE_ROTMASK);
			}
		}
	}

	return quitting;
}


/* Display the editing state */
void ed2dDisplay(void)
{
	UDWORD					worldX,worldY;
	RECT					sSrcRect, sDestRect;
	MAPTILE					*psTile;
	SDWORD					sx0,sy0,sx1,sy1;
	SDWORD					px0,py0,px1,py1;
	SDWORD					x,y;

	/* Display the contents of the paste box */
	if (mState == MS_PASTE)
	{
		/* clip to the screen */
		px0=0;
		py0=0;
		px1 = sPasteBox.width;
		py1 = sPasteBox.height;
		sx0 = selSX;
		sy0 = selSY;
		sx1 = selEX + 1;
		sy1 = selEY + 1;
		if (sx0 < (SDWORD)viewX)
		{
			px0 += (SDWORD)viewX - sx0;
			sx0 = viewX;
		}
		if (sy0 < (SDWORD)viewY)
		{
			py0 += (SDWORD)viewY - sy0;
			sy0 = viewY;
		}
		if (sx1 > (SDWORD)viewX + TILES_ACROSS)
		{
			px1 -= sx1 - (SDWORD)viewX - TILES_ACROSS;
			sx1 = (SDWORD)viewX + TILES_ACROSS;
		}
		if (sy1 > (SDWORD)viewY + TILES_DOWN)
		{
			py1 -= sy1 - (SDWORD)viewY - TILES_DOWN;
			sy1 = (SDWORD)viewY + TILES_DOWN;
		}

		ASSERT( sx0 >= (SDWORD)viewX && sx1 <= ((SDWORD)viewX + TILES_ACROSS) &&
			    sy0 >= (SDWORD)viewY && sy1 <= ((SDWORD)viewY + TILES_DOWN) &&
				sx1 > sx0 && sy1 > sy0,
				"paste Box: clipping failed" );

		/* Display the tiles */
		for(y = py0; y < py1; y ++)
		{
			disp2DFromWorld(sx0 << TILE_SHIFT, (sy0 + y - py0) << TILE_SHIFT,
							(SDWORD *)&sDestRect.left, (SDWORD *)&sDestRect.top);
			sDestRect.right = sDestRect.left + TILE_SIZE2D;
			sDestRect.bottom = sDestRect.top + TILE_SIZE2D;
			psTile = sPasteBox.psTiles + y * sPasteBox.width + px0;
			for(x = px0; x < px1; x++)
			{
				sSrcRect.left = ((psTile->texture & TILE_NUMMASK)
									% tilesPerLine) << SCR_TILE_SHIFT;
				sSrcRect.right = sSrcRect.left + TILE_SIZE2D;
				sSrcRect.top = ((psTile->texture & TILE_NUMMASK)
									/ tilesPerLine) << SCR_TILE_SHIFT;
				sSrcRect.bottom = sSrcRect.top + TILE_SIZE2D;

				/* Draw the current tile */
				blitTile(&sDestRect, &sSrcRect, psTile->texture);

				psTile++;
				sDestRect.left += TILE_SIZE2D;
				sDestRect.right += TILE_SIZE2D;
			}
		}
	}

	/* Display the selection box if there is one */
	if (mState != MS_GAME && mState != MS_GRABSTART)
	{
		/* Draw the bounding box */
		disp2DFromWorld(selSX << TILE_SHIFT,selSY<< TILE_SHIFT, &sx0,&sy0);
		disp2DFromWorld((selEX+1) << TILE_SHIFT, (selEY + 1)<<TILE_SHIFT, &sx1,&sy1);
		screenSetLineColour(0xff,0xff,0xff);
		screenDrawLine(sx0-1,sy0-1, sx1+1,sy0-1);
		screenDrawLine(sx1+1,sy0-1, sx1+1,sy1+1);
		screenDrawLine(sx1+1,sy1+1, sx0-1,sy1+1);
		screenDrawLine(sx0-1,sy1+1, sx0-1,sy0-1);

		screenDrawLine(sx0+1,sy0+1, sx1-1,sy0+1);
		screenDrawLine(sx1-1,sy0+1, sx1-1,sy1-1);
		screenDrawLine(sx1-1,sy1-1, sx0+1,sy1-1);
		screenDrawLine(sx0+1,sy1-1, sx0+1,sy0+1);

		screenSetLineColour(0,0,0);
		screenDrawLine(sx0,sy0, sx1,sy0);
		screenDrawLine(sx1,sy0, sx1,sy1);
		screenDrawLine(sx1,sy1, sx0,sy1);
		screenDrawLine(sx0,sy1, sx0,sy0);
	}

	/* Display the current painting tile */
	screenSetFillColour(0xaa,0xaa,0xaa);
	screenFillRect(2+TILE_SIZE2D, TILE_SIZE2D*2, TILE_SIZE2D*4 - 5, TILE_SIZE2D*6);
/*	if (showTerrain)
	{
		dispTerrain(TILE_SIZE2D*3, 5*TILE_SIZE2D/2, currTerrain);
	}
	else
	{*/
		sSrcRect.left = ((currTile & TILE_NUMMASK) % tilesPerLine) << SCR_TILE_SHIFT;
		sSrcRect.right = sSrcRect.left + TILE_SIZE2D;
		sSrcRect.top = ((currTile & TILE_NUMMASK) / tilesPerLine) << SCR_TILE_SHIFT;
		sSrcRect.bottom = sSrcRect.top + TILE_SIZE2D;
		sDestRect.left = TILE_SIZE2D*2;
		sDestRect.right = 3 * TILE_SIZE2D;
		sDestRect.top = 5 * TILE_SIZE2D / 2;
		sDestRect.bottom = 7 *TILE_SIZE2D / 2;
		blitTile(&sDestRect, &sSrcRect, currTile);
//	}

	screenSetTextColour(0,0,0);
	disp2DToWorld(mouseX(), mouseY(), &worldX,&worldY);
	screenTextOut(40,TILE_SIZE2D*4, "%3d,%3d", worldX>>TILE_SHIFT,worldY>>TILE_SHIFT);
	if (mState != MS_GAME && mState != MS_GRABSTART)
	{
		screenTextOut(40,TILE_SIZE2D*5, "%3d,%3d", selSX,selSY);
		screenTextOut(40,TILE_SIZE2D*5 + 15, "%3dx%3d", selEX - selSX + 1,selEY - selSY + 1);
	}
}


/* Get a past box from the map */
static BOOL getBox(PASTE_BOX *psBox, UDWORD x, UDWORD y, UDWORD width, UDWORD height)
{
	UDWORD	mx,my;
	MAPTILE	*psCurr;

	ASSERT( x+width <= mapWidth, "getBox: box off map" );
	ASSERT( y+height <= mapHeight, "getBox: box off map" );

	/* Allocate the box */
	psBox->x = x;
	psBox->y = y;
	psBox->width = width;
	psBox->height = height;
	psBox->psTiles = (MAPTILE *)MALLOC(sizeof(MAPTILE)*
							psBox->width * psBox->height);
	if (psBox->psTiles == NULL)
	{
		debug( LOG_ERROR, "Out of memory" );
		abort();
		return FALSE;
	}

	/* Copy the map data in */
	psCurr = psBox->psTiles;
	for(my=y; my < y + height; my++)
	{
		for(mx=x; mx < x + width; mx++)
		{
			memcpy(psCurr, mapTile(mx,my), sizeof(MAPTILE));
			psCurr++;
		}
	}

	return TRUE;
}

/* Store a paste box into the map */
static void putBox(PASTE_BOX *psBox, UDWORD x, UDWORD y)
{
	UDWORD	mx,my;
	MAPTILE	*psCurr;

	ASSERT( x+psBox->width <= mapWidth, "putBox: box off map" );
	ASSERT( y+psBox->height <= mapHeight, "putBox: box off map" );

	/* Store the terrain type and texture info into the map */
	psCurr = psBox->psTiles;
	for(my = y; my < y + psBox->height; my ++)
	{
		for(mx = x; mx < x + psBox->width; mx ++)
		{
			mapTile(mx,my)->texture = psCurr->texture;
//			mapTile(mx,my)->type = psCurr->type;
			psCurr ++;
		}
	}
}

/* Flip a paste box on the X axis */
static void flipBoxX(PASTE_BOX *psBox)
{
	MAPTILE	*psNew, *psSrc, *psDest;
	UDWORD	x,y;
	UDWORD	tex1,tex2;

	/* Allocate a buffer for the flipped version */
	psNew = (MAPTILE *)MALLOC(sizeof(MAPTILE) * psBox->width * psBox->height);
	if (psNew == NULL)
	{
		debug( LOG_ERROR, "Out of memory, couldn't do flip\n" );
		abort();
		return;
	}

	psSrc = psBox->psTiles;
	for(y=0; y < psBox->height; y++)
	{
		psDest = psNew + (y+1) * psBox->width - 1;
		for(x=0; x < psBox->width; x++)
		{
			memcpy(psDest, psSrc, sizeof(MAPTILE));
			/* Now we've got to flip the texture info as well
			 * If it's got an odd rotation flip Y otherwise flip X
			 */
			if (((psDest->texture & TILE_ROTMASK) >> TILE_ROTSHIFT) % 2 == 0)
			{
				/* flip X */
				tex1 = psDest->texture & (~TILE_XFLIP);
				tex2 = ((~(psDest->texture & TILE_XFLIP)) & TILE_XFLIP);
				psDest->texture = (UWORD)(tex1 | tex2);
			}
			else
			{
				/* flip Y */
				tex1 = psDest->texture & (~TILE_YFLIP);
				tex2 = ((~(psDest->texture & TILE_YFLIP)) & TILE_YFLIP);
				psDest->texture = (UWORD)(tex1 | tex2);
			}
			psSrc ++;
			psDest --;
		}
	}

	FREE(psBox->psTiles);
	psBox->psTiles = psNew;
}


/* Flip a paste box on the Y axis */
static void flipBoxY(PASTE_BOX *psBox)
{
	MAPTILE	*psNew, *psSrc, *psDest;
	UDWORD	x,y;
	UDWORD	tex1,tex2;

	/* Allocate a buffer for the flipped version */
	psNew = (MAPTILE *)MALLOC(sizeof(MAPTILE) * psBox->width * psBox->height);
	if (psNew == NULL)
	{
		debug( LOG_ERROR, "Out of memory, couldn't do flip\n" );
		abort();
		return;
	}

	psSrc = psBox->psTiles;
	for(y=0; y < psBox->height; y++)
	{
		psDest = psNew + (psBox->height - y - 1) * psBox->width;
		for(x=0; x < psBox->width; x++)
		{
			memcpy(psDest, psSrc, sizeof(MAPTILE));
			/* Now we've got to flip the texture info as well
			 * If it's got an odd rotation flip X otherwise flip Y
			 */
			if (((psDest->texture & TILE_ROTMASK) >> TILE_ROTSHIFT) % 2 == 1)
			{
				/* flip X */
				tex1 = psDest->texture & (~TILE_XFLIP);
				tex2 = ((~(psDest->texture & TILE_XFLIP)) & TILE_XFLIP);
				psDest->texture = (UWORD)(tex1 | tex2);
			}
			else
			{
				/* flip Y */
				tex1 = psDest->texture & (~TILE_YFLIP);
				tex2 = ((~(psDest->texture & TILE_YFLIP)) & TILE_YFLIP);
				psDest->texture = (UWORD)(tex1 | tex2);
			}
			psSrc ++;
			psDest ++;
		}
	}

	FREE(psBox->psTiles);
	psBox->psTiles = psNew;
}


/* Rotate a paste box
 * If the box isn't square, width and height get swapped
 */
static void rotBox(PASTE_BOX *psBox)
{
	UDWORD		x,y;
	UDWORD		rot;
	MAPTILE		*psNew, *psSrc, *psDest;

	/* Allocate the new tile buffer */
	psNew = (MAPTILE *)MALLOC(sizeof(MAPTILE) * psBox->width * psBox->height);
	if (psNew == NULL)
	{
		debug( LOG_ERROR, "Out of memory" );
		abort();
		return;
	}

	psSrc = psBox->psTiles;
	for(y=0; y<psBox->height; y++)
	{
		psDest = psNew + (psBox->height - y - 1);
		for(x=0; x<psBox->width; x++)
		{
			memcpy(psDest, psSrc, sizeof(MAPTILE));
			rot = (psDest->texture & TILE_ROTMASK) + (1 << TILE_ROTSHIFT);
			rot = rot % (4 << TILE_ROTSHIFT);
			psDest->texture = (UWORD)((psDest->texture & ~TILE_ROTMASK) | rot);
			psSrc ++;
			psDest += psBox->height;
		}
	}

	/* Store the new tiles into the box */
	FREE(psBox->psTiles);
	psBox->psTiles = psNew;
	x = psBox->width;
	psBox->width = psBox->height;
	psBox->height = x;
}


/* Load in a new map */
BOOL ed2dLoadMapFile(void)
{
	char			aFileName[256];
	OPENFILENAME	sOFN;
	UBYTE			*pFileData=NULL;
	UDWORD			fileSize;

	/* Stop the game clock */
	gameTimeStop();

	memset(&sOFN, 0, sizeof(OPENFILENAME));
	sOFN.lStructSize = sizeof(OPENFILENAME);
	sOFN.lpstrFilter = "Map File (*.MAP)\0*.MAP\0";
	sOFN.lpstrFile = aFileName;
	sOFN.nMaxFile = 256;
	aFileName[0] = '\0';

	/* Make sure we're on the gdi screen */
	screenFlipToGDI();
	if (!GetOpenFileName(&sOFN))
	{
		goto error;
	}

	/* Load in the chosen file data */
	if (!loadFile(aFileName, &pFileData, &fileSize))
	{
		goto error;
	}

	/* Load the data into the map -
	   don't check the return code as we do the same thing either way */
	(void)mapLoad(pFileData, fileSize);
	FREE(pFileData);

	/* Start the game clock */
	gameTimeStart();
	return TRUE;

error:
	/* Start the game clock */
	gameTimeStart();
	return FALSE;
}


/* Save the current map */
BOOL ed2dSaveMapFile(void)
{
	char			aFileName[256];
	OPENFILENAME	sOFN;

	/* Stop the game clock */
	gameTimeStop();

	memset(&sOFN, 0, sizeof(OPENFILENAME));
	sOFN.lStructSize = sizeof(OPENFILENAME);
	sOFN.lpstrFilter = "Map File (*.MAP)\0*.MAP\0";
	sOFN.lpstrFile = aFileName;
	sOFN.nMaxFile = 256;
	aFileName[0] = '\0';

	/* make sure we're on the gdi screen */
	screenFlipToGDI();
	if (!GetSaveFileName(&sOFN))
	{
		goto error;
	}

	/* Write the data to the file */
	if (!writeMapFile(aFileName))
	{
		goto error;
	}

	/* Start the game clock */
	gameTimeStart();
	return TRUE;

error:
	/* Start the game clock */
	gameTimeStart();
	return FALSE;
}



#endif
