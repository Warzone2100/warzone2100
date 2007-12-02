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

#define RED_CHROMATICITY	1
#define GREEN_CHROMATICITY	1
#define BLUE_CHROMATICITY	1

static void pie_SetColourDefines(void);

/*
	This is how far from the end you want the drawn as the artist intended shades
	to appear
*/

#define COLOUR_BALANCE	6		// 3 from the end. (two brighter shades!)

static PIELIGHT	*psGamePal = NULL;
static BOOL	bPaletteInitialised = FALSE;

Uint8	palShades[PALETTE_SIZE * PALETTE_SHADE_LEVEL];
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
	WZCOL_WHITE.byte.a = 255;
	WZCOL_WHITE.byte.r = 255;
	WZCOL_WHITE.byte.g = 255;
	WZCOL_WHITE.byte.b = 255;

	WZCOL_BLACK.byte.a = 255;
	WZCOL_BLACK.byte.r = 1;
	WZCOL_BLACK.byte.g = 1;
	WZCOL_BLACK.byte.b = 1;

	WZCOL_GREEN.byte.a = 255;
	WZCOL_GREEN.byte.r = 0;
	WZCOL_GREEN.byte.g = 255;
	WZCOL_GREEN.byte.b = 0;

	WZCOL_RED.byte.a = 255;
	WZCOL_RED.byte.r = 255;
	WZCOL_RED.byte.g = 0;
	WZCOL_RED.byte.b = 0;

	WZCOL_YELLOW.byte.a = 255;
	WZCOL_YELLOW.byte.r = 255;
	WZCOL_YELLOW.byte.g = 255;
	WZCOL_YELLOW.byte.b = 0;

	WZCOL_RELOAD_BAR	= WZCOL_WHITE;
	WZCOL_RELOAD_BACKGROUND	= WZCOL_BLACK;
	WZCOL_HEALTH_HIGH	= WZCOL_GREEN;
	WZCOL_HEALTH_MEDIUM	= WZCOL_YELLOW;
	WZCOL_HEALTH_LOW	= WZCOL_RED;
	WZCOL_CURSOR		= WZCOL_WHITE;

	WZCOL_MENU_BACKGROUND.byte.a	= 255;
	WZCOL_MENU_BACKGROUND.byte.r	= 0;
	WZCOL_MENU_BACKGROUND.byte.g	= 1;
	WZCOL_MENU_BACKGROUND.byte.b	= 97;

	WZCOL_MENU_BORDER.byte.a	= 255;
	WZCOL_MENU_BORDER.byte.r	= 0;
	WZCOL_MENU_BORDER.byte.g	= 21;
	WZCOL_MENU_BORDER.byte.b	= 240;

	WZCOL_MENU_LOAD_BORDER		= WZCOL_BLACK;
	WZCOL_MENU_LOAD_BORDER.byte.r	= 133;

	WZCOL_MENU_SCORES_INTERIOR	= WZCOL_BLACK;
	WZCOL_MENU_SCORES_INTERIOR.byte.b = 33;

	WZCOL_MENU_SEPARATOR.byte.a = UBYTE_MAX;
	WZCOL_MENU_SEPARATOR.byte.r = 0x64;
	WZCOL_MENU_SEPARATOR.byte.g = 0x64;
	WZCOL_MENU_SEPARATOR.byte.b = 0xa0;
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

void pal_BuildAdjustedShadeTable( void )
{
float	redFraction, greenFraction, blueFraction;
int		seekRed, seekGreen,seekBlue;
int		numColours;
int		numShades;

	ASSERT( bPaletteInitialised,"pal_BuildAdjustedShadeTable, palette not initialised." );

	for(numColours = 0; numColours<255; numColours++)
	{
		redFraction =	(float)psGamePal[numColours].byte.r / (float) 16;
		greenFraction = (float)psGamePal[numColours].byte.g / (float) 16;
		blueFraction =	(float)psGamePal[numColours].byte.b / (float) 16;

		for(numShades = COLOUR_BALANCE; numShades < 16+COLOUR_BALANCE; numShades++)
		{
			seekRed =	(int)((float)numShades * redFraction);
			seekGreen = (int)((float)numShades * greenFraction);
			seekBlue =	(int)((float)numShades * blueFraction);

			if(seekRed >255) seekRed = 255;
			if(seekGreen >255) seekGreen = 255;
			if(seekBlue >255) seekBlue = 255;

			palShades[(numColours * PALETTE_SHADE_LEVEL) + (numShades-COLOUR_BALANCE)] =
				pal_GetNearestColour((Uint8) seekRed, (Uint8) seekGreen, (Uint8) seekBlue);
		}
	}
}

PIELIGHT *pie_GetGamePal(void)
{
	ASSERT( bPaletteInitialised,"pie_GetGamePal, palette not initialised" );
	return 	psGamePal;
}

