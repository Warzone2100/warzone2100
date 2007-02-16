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

#include "lib/ivis_common/ivi.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/piepalette.h"
#include "lib/ivis_common/rendmode.h"
#include "lib/framework/fractions.h"
#include "screen.h"

#define RED_CHROMATICITY	1
#define GREEN_CHROMATICITY	1
#define BLUE_CHROMATICITY	1


uint8 pal_GetNearestColour(uint8 r, uint8 g, uint8 b);
void pie_SetColourDefines(void);
/*
	This is how far from the end you want the drawn as the artist intended shades
	to appear
*/

#define COLOUR_BALANCE	6		// 3 from the end. (two brighter shades!)

iColour*			psGamePal = NULL;
uint8	palShades[PALETTE_SIZE * PALETTE_SHADE_LEVEL];
BOOL	bPaletteInitialised = FALSE;
uint8	colours[16];


//*************************************************************************
//*** add a new palette
//*
//* params	pal = pointer to palette to add
//*
//* returns slot number of added palette or -1 if error
//*
//******

int pal_AddNewPalette(iColour *pal)
{
	int i;
	iColour *p;

	bPaletteInitialised = TRUE;
	if (psGamePal == NULL)
	{
		psGamePal = (iColour*) MALLOC(PALETTE_SIZE * sizeof(iColour));
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
		p[i].r = pal[i].r;
		p[i].g = pal[i].g;
		p[i].b = pal[i].b;
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
}

void pal_ShutDown(void)
{
	if (bPaletteInitialised)
	{
		bPaletteInitialised = FALSE;
		FREE(psGamePal);
	}
}

uint8 pal_GetNearestColour(uint8 r, uint8 g, uint8 b)
{
	int c ;
	int32 distance_r, distance_g, distance_b, squared_distance;
	int32 best_colour = 0, best_squared_distance;

	ASSERT( bPaletteInitialised,"pal_GetNearestColour, palette not initialised." );

	best_squared_distance = 0x10000;

	for (c = 0; c < PALETTE_SIZE; c++) {

		distance_r = r -  psGamePal[c].r;
		distance_g = g -  psGamePal[c].g;
		distance_b = b -  psGamePal[c].b;

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
	return ((uint8) best_colour);
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
			redFraction =	(float)psGamePal[numColours].r /	(float) 16;
			greenFraction = (float)psGamePal[numColours].g /	(float) 16;
			blueFraction =	(float)psGamePal[numColours].b /	(float) 16;

		for(numShades = COLOUR_BALANCE; numShades < 16+COLOUR_BALANCE; numShades++)
		{
			seekRed =	(int)((float)numShades * redFraction);
			seekGreen = (int)((float)numShades * greenFraction);
			seekBlue =	(int)((float)numShades * blueFraction);

			if(seekRed >255) seekRed = 255;
			if(seekGreen >255) seekGreen = 255;
			if(seekBlue >255) seekBlue = 255;

			palShades[(numColours * PALETTE_SHADE_LEVEL) + (numShades-COLOUR_BALANCE)] =
				pal_GetNearestColour((uint8) seekRed, (uint8) seekGreen, (uint8) seekBlue);
		}
	}
}

iColour*	pie_GetGamePal(void)
{
	ASSERT( bPaletteInitialised,"pie_GetGamePal, palette not initialised" );
	return 	psGamePal;
}

