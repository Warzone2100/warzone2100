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
#include <stdio.h>

#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/piepalette.h"
#include "lib/ivis_common/rendmode.h"
#include "lib/framework/fractions.h"
#include "screen.h"

static void pie_SetColourDefines(void);

static PIELIGHT	*psGamePal = NULL;
static BOOL	bPaletteInitialised = FALSE;

Uint8	colours[16];
PIELIGHT psPalette[WZCOL_MAX];


//*************************************************************************
//*** add a new palette
//*
//* params	pal = pointer to palette to add
//*
//* returns slot number of added palette or -1 if error
//*
//******

int pal_AddNewPalette(PIELIGHT *pal)
{
	int i;
	PIELIGHT *p;

	bPaletteInitialised = TRUE;
	if (psGamePal == NULL)
	{
		psGamePal = malloc(PALETTE_SIZE * sizeof(PIELIGHT));
		if (psGamePal == NULL)
		{
			debug( LOG_ERROR, "pal_AddNewPalette - Out of memory" );
			abort();
			return FALSE;
		}
	}

	p = psGamePal;

	for (i=0; i<PALETTE_SIZE; i++)
	{
		//set pie palette
		p[i].argb = pal[i].argb;
	}
	pie_SetColourDefines();

	return 0;
}

//*************************************************************************
//*** calculate primary colours for current palette (store in COL_ ..
//*
//* on exit	_iVCOLS[0..15] contain colour values matched
//*			COL_.. below access _iVCOLS[0..15]
//******

void pie_SetColourDefines(void)
{
	COL_BLACK 		= pal_GetNearestColour(  1,  1, 1);
	COL_RED 		= pal_GetNearestColour( 128,  0, 0);
	COL_GREEN 		= pal_GetNearestColour(  0, 128, 0);
 	COL_BLUE 		= pal_GetNearestColour(  0,  0, 128);
	COL_CYAN 		= pal_GetNearestColour(  0, 128, 128);
	COL_MAGENTA 		= pal_GetNearestColour( 128,  0, 128);
	COL_BROWN 		= pal_GetNearestColour( 128, 64,  0);
	COL_DARKGREY 		= pal_GetNearestColour( 32, 32, 32);
	COL_GREY		= pal_GetNearestColour( 128, 128, 128);
	COL_LIGHTRED 		= pal_GetNearestColour( 255,  0,  0);
	COL_LIGHTGREEN 		= pal_GetNearestColour(  0, 255,  0);
	COL_LIGHTBLUE		= pal_GetNearestColour(  0,  0, 255);
	COL_LIGHTCYAN 		= pal_GetNearestColour(  0, 255, 255);
	COL_LIGHTMAGENTA	= pal_GetNearestColour( 255,  0, 255);
	COL_YELLOW	  	= pal_GetNearestColour( 255, 255,  0);
	COL_WHITE 		= pal_GetNearestColour( 255, 255, 255);

// used to print out values from the old palette; remove with the old palette
// #define PRINTCOL(x) debug(LOG_ERROR, "(%hhu, %hhu, %hhu)", psGamePal[x].byte.r, psGamePal[x].byte.g, psGamePal[x].byte.b);

	// TODO: Read these from file so that mod-makers can change them

	WZCOL_WHITE = pal_Colour(UBYTE_MAX, UBYTE_MAX, UBYTE_MAX);
	WZCOL_BLACK = pal_Colour(1, 1, 1);
	WZCOL_GREEN = pal_Colour(0, UBYTE_MAX, 0);
	WZCOL_RED = pal_Colour(UBYTE_MAX, 0, 0);
	WZCOL_YELLOW = pal_Colour(UBYTE_MAX, UBYTE_MAX, 0);

	WZCOL_RELOAD_BAR	= WZCOL_WHITE;
	WZCOL_RELOAD_BACKGROUND	= WZCOL_BLACK;
	WZCOL_HEALTH_HIGH	= WZCOL_GREEN;
	WZCOL_HEALTH_MEDIUM	= WZCOL_YELLOW;
	WZCOL_HEALTH_LOW	= WZCOL_RED;
	WZCOL_CURSOR		= WZCOL_WHITE;

	WZCOL_MENU_BACKGROUND = pal_Colour(0, 1, 97);
	WZCOL_MENU_BORDER = pal_Colour(0, 21, 240);

	WZCOL_MENU_LOAD_BORDER		= WZCOL_BLACK;
	WZCOL_MENU_LOAD_BORDER.byte.r	= 133;

	WZCOL_MENU_SCORES_INTERIOR	= WZCOL_BLACK;
	WZCOL_MENU_SCORES_INTERIOR.byte.b = 33;

	WZCOL_MENU_SEPARATOR = pal_Colour(0x64, 0x64, 0xa0);

	WZCOL_TEXT_BRIGHT = WZCOL_WHITE;
	WZCOL_TEXT_MEDIUM.byte.r = 0.627451f * UBYTE_MAX;
	WZCOL_TEXT_MEDIUM.byte.g = 0.627451f * UBYTE_MAX;
	WZCOL_TEXT_MEDIUM.byte.b = UBYTE_MAX;
	WZCOL_TEXT_MEDIUM.byte.a = UBYTE_MAX;
	WZCOL_TEXT_DARK.byte.r = 0.376471f * UBYTE_MAX;
	WZCOL_TEXT_DARK.byte.g = 0.376471f * UBYTE_MAX;
	WZCOL_TEXT_DARK.byte.b = UBYTE_MAX;
	WZCOL_TEXT_DARK.byte.a = UBYTE_MAX;

	WZCOL_SCORE_BOX_BORDER = WZCOL_BLACK;
	WZCOL_SCORE_BOX = pal_Colour(0, 0, 88);
	WZCOL_SCORE_BOX.byte.a = 128;

	WZCOL_TOOLTIP_TEXT = WZCOL_WHITE;

	WZCOL_UNIT_SELECT_BORDER = pal_Colour(0, 0, 128);
	WZCOL_UNIT_SELECT_BOX = WZCOL_WHITE;
	WZCOL_UNIT_SELECT_BOX.byte.a = 16;

	WZCOL_RADAR_BACKGROUND = WZCOL_MENU_BACKGROUND;

	WZCOL_MAP_OUTLINE_OK = WZCOL_WHITE;
	WZCOL_MAP_OUTLINE_BAD = WZCOL_RED;
}

PIELIGHT pal_SetBrightness(UBYTE brightness)
{
	PIELIGHT c;
	c.byte.r = brightness;
	c.byte.g = brightness;
	c.byte.b = brightness;
	c.byte.a = UBYTE_MAX;
	return c;
}

void pal_ShutDown(void)
{
	if (bPaletteInitialised)
	{
		bPaletteInitialised = FALSE;
		free(psGamePal);
		psGamePal = NULL;
	}
}

Uint8 pal_GetNearestColour(Uint8 r, Uint8 g, Uint8 b)
{
	int c ;
	Sint32 distance_r, distance_g, distance_b, squared_distance;
	Sint32 best_colour = 0, best_squared_distance;

	ASSERT( bPaletteInitialised,"pal_GetNearestColour, palette not initialised." );

	best_squared_distance = 0x10000;

	for (c = 0; c < PALETTE_SIZE; c++) {

		distance_r = r -  psGamePal[c].byte.r;
		distance_g = g -  psGamePal[c].byte.g;
		distance_b = b -  psGamePal[c].byte.b;

		squared_distance =  distance_r * distance_r + distance_g * distance_g + distance_b * distance_b;

		if (squared_distance < best_squared_distance)
		{
			best_squared_distance = squared_distance;
			best_colour = c;
		}
	}
	if (best_colour == 0)
	{
		best_colour = 1;
	}
	return ((Uint8) best_colour);
}

PIELIGHT *pie_GetGamePal(void)
{
	ASSERT( bPaletteInitialised,"pie_GetGamePal, palette not initialised" );
	return 	psGamePal;
}

