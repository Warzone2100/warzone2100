#include <stdio.h>
#include <math.h>
#include "lib/ivis_common/ivi.h"
#include "lib/ivis_common/piestate.h"
#include "lib/ivis_common/piepalette.h"
#include "lib/ivis_common/rendmode.h"
#include "lib/ivis_common/bug.h"
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




/*



	PC VERSION !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


*/

iColour*			psGamePal = NULL;
PALETTEENTRY*		psWinPal = NULL;
uint8				palShades[PALETTE_SIZE * PALETTE_SHADE_LEVEL];
BOOL bPaletteInitialised = FALSE;
uint8	 colours[16];

/* Look up table for transparency */
/*	entry[x][y] tells you what colour to poke in when you're writing
	x over y
*/
uint8				transLookup[PALETTE_SIZE][PALETTE_SIZE];

UWORD	palette16Bit[PALETTE_SIZE];	//16 bit version of the present palette

BOOL	pal_Make16BitPalette(void)
{
iColour* psPal;
UDWORD	i;
UWORD	alpha, red,green,blue;
BYTE				ap = 0,	ac = 0, rp = 0,	rc = 0, gp = 0,	gc = 0, bp = 0, bc = 0;
ULONG				mask, amask, rmask, gmask, bmask;

	/*
	// Cannot convert iof not 16bit mode
	*/

	if ( ! screenGetFrontBufferPixelFormatMasks(&amask, &rmask, &gmask, &bmask))
	{
		return FALSE;
	}

//	ASSERT((DDPixelFormat->dwRGBBitCount == 16, "make16BitPalette RGB bit count not 16"));

	psPal =	pie_GetGamePal();

	/*
	// Cannot playback if not 16bit mode
	*/
	if( screenGetFrontBufferBitDepth() == 16 )
	{
		/*
		// Find out the RGB type of the surface and tell the codec...
		*/
		mask = amask;

		if(mask!=0)
		{
			while(!(mask & 1))
			{
				mask>>=1;
				ap++;
			}
		}

		while((mask & 1))
		{
			mask>>=1;
			ac++;
		}

		mask = rmask;

		if(mask!=0)
		{
			while(!(mask & 1))
			{
				mask>>=1;
				rp++;
			}
		}

		while((mask & 1))
		{
			mask>>=1;
			rc++;
		}

		mask = gmask;

		if(mask!=0)
		{
			while(!(mask & 1))
			{
				mask>>=1;
				gp++;
			}
		}

		while((mask & 1))
		{
			mask>>=1;
			gc++;
		}

		mask = bmask;

		if(mask!=0)
		{
			while(!(mask & 1))
			{
				mask>>=1;
				bp++;
			}
		}

		while((mask & 1))
		{
			mask>>=1;
			bc++;
		}
	}
	else
	{
		//if not 16 bit use blue 5 only so we know the problem
		bc = 5;
	}

	alpha = 0;

	for(i=0; i<PALETTE_SIZE; i++)
	{
		//alpha = 0 when i = 0
		red = (UWORD) psPal[i].r;
		green = (UWORD) psPal[i].g;
		blue = (UWORD) psPal[i].b;

		alpha >>= (8-ac);
		red >>= (8-rc);
		blue >>= (8-bc);
		green >>= (8-gc);

		alpha <<= ap;
		red <<= rp;
		blue <<= bp;
		green <<= gp;

		palette16Bit[i] = alpha + red + green + blue;
		alpha = 0xff;//alpha = 0xff when i > 0
	}
	return(TRUE);

}


//*************************************************************************
//*** add a new palette
//*
//* params	pal = pointer to palette to add
//*
//* returns slot number of added palette or -1 if error
//*
//******

BOOL pal_AddNewPalette(iColour *pal)
{
// PSX version dos'nt use palettes as such, SetRGBLookup sets up a global palette instead which is generally
// just used for colour index to RGB conversions.
	int i;
#if defined(RED_GREEN) || defined(GREEN_BLUE)
	int rg;
#endif
	iColour *p;
	PALETTEENTRY *w;

	bPaletteInitialised = TRUE;
	if (psGamePal == NULL)
	{
		psGamePal = (iColour*) MALLOC(PALETTE_SIZE * sizeof(iColour));
		if (psGamePal == NULL)
		{
			DBERROR(("pal_AddNewPalette - Out of memory"));
			return FALSE;
		}
	}
	if (psWinPal == NULL)
	{
		psWinPal = (PALETTEENTRY*) MALLOC(PALETTE_SIZE * sizeof(PALETTEENTRY));
		if (psGamePal == NULL)
		{
			DBERROR(("pal_AddNewPalette - Out of memory"));
			return FALSE;
		}
	}
#ifndef   PIETOOL			// ffs
	/* If we're adding a palette and running on a 3dfx, then bang it down to the card */

#endif
	p = psGamePal;
	w = psWinPal;

	for (i=0; i<PALETTE_SIZE; i++)
	{
#ifdef RED_GREEN
		rg = pal[i].r + pal[i].g;
		rg /= 2;
		//set pie palette
		p[i].r = rg;
		p[i].g = rg;
		p[i].b = pal[i].b;
		//set copy of windows palette
		w[i].peRed   = (uint8) rg;
		w[i].peGreen = (uint8) rg;
		w[i].peBlue  = (uint8) pal[i].b;
		w[i].peFlags = 0;
#elif defined GREEN_BLUE
		rg = pal[i].g + pal[i].b;
		rg /= 2;
		//set pie palette
		p[i].r = pal[i].r;
		p[i].g = rg;
		p[i].b = rg;
		//set copy of windows palette
		w[i].peRed   = (uint8) pal[i].r;
		w[i].peGreen = (uint8) rg;
		w[i].peBlue  = (uint8) rg;
		w[i].peFlags = 0;
#else

		//set pie palette
		p[i].r = pal[i].r;
		p[i].g = pal[i].g;
		p[i].b = pal[i].b;
		//set copy of windows palette
		w[i].peRed   = (uint8) pal[i].r;
		w[i].peGreen = (uint8) pal[i].g;
		w[i].peBlue  = (uint8) pal[i].b;
		w[i].peFlags = 0;
#endif
	}
	//set windows palette
	screenSetPalette(0, PALETTE_SIZE, psWinPal);

	pie_SetColourDefines();
	pal_Make16BitPalette();
	return 0;
}


void pal_SelectPalette(int n)
{
}


//*************************************************************************
//***
//*
//******

void pal_SetPalette(void)

{
}

//*************************************************************************
//*** calculate primary colours for current palette (store in COL_ ..
//*
//* on exit	_iVCOLS[0..15] contain colour values matched
//*			COL_.. below access _iVCOLS[0..15]
//******

void pie_SetColourDefines(void)
{
	COL_BLACK 			= pal_GetNearestColour(  1,  1, 1);
	COL_RED 			= pal_GetNearestColour( 128,  0, 0);
	COL_GREEN 			= pal_GetNearestColour(  0, 128, 0);
 	COL_BLUE 			= pal_GetNearestColour(  0,  0, 128);
	COL_CYAN 			= pal_GetNearestColour(  0, 128, 128);
	COL_MAGENTA 		= pal_GetNearestColour( 128,  0, 128);
	COL_BROWN 			= pal_GetNearestColour( 128, 64,  0);
	COL_DARKGREY 		= pal_GetNearestColour( 32, 32, 32);
	COL_GREY			= pal_GetNearestColour( 128, 128, 128);
	COL_LIGHTRED 		= pal_GetNearestColour( 255,  0,  0);
	COL_LIGHTGREEN 		= pal_GetNearestColour(  0, 255,  0);
	COL_LIGHTBLUE		= pal_GetNearestColour(  0,  0, 255);
	COL_LIGHTCYAN 		= pal_GetNearestColour(  0, 255, 255);
	COL_LIGHTMAGENTA	= pal_GetNearestColour( 255,  0, 255);
	COL_YELLOW	  		= pal_GetNearestColour( 255, 255,  0);
	COL_WHITE 			= pal_GetNearestColour( 255, 255, 255);
}

//*************************************************************************
//*** init palette (sets default palette and calc primary colours)
//*
//* on exit	psCurrentPalette = pointer to default palette (palette 0)
//******

void pal_Init(void)
{
}

void pal_ShutDown(void)
{
	if (bPaletteInitialised)
	{
		bPaletteInitialised = FALSE;
		FREE(psGamePal);
		FREE(psWinPal);
	}
}

uint8 pal_GetNearestColour(uint8 r, uint8 g, uint8 b)
{
	int c ;
	int32 distance_r, distance_g, distance_b, squared_distance;
	int32 best_colour = 0, best_squared_distance;

	ASSERT((bPaletteInitialised,"pal_GetNearestColour, palette not initialised."));

	best_squared_distance = 0x10000;

	for (c = 0; c < PALETTE_SIZE; c++) {

#if(0)
		distance_r = r - (int32) ((float) psGamePal[c].r * RED_CHROMATICITY);
		distance_g = g - (int32) ((float) psGamePal[c].g * GREEN_CHROMATICITY);
		distance_b = b - (int32) ((float) psGamePal[c].b * BLUE_CHROMATICITY);
#else
		distance_r = r -  psGamePal[c].r;
		distance_g = g -  psGamePal[c].g;
		distance_b = b -  psGamePal[c].b;
#endif

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


void	pie_BuildSoftwareTransparency( void )
{
int	i,j;
int	red,green,blue;

	for(i=0; i<PALETTE_SIZE; i++)
	{
		for(j=0; j<PALETTE_SIZE; j++)
		{
			red = (psGamePal[i].r + psGamePal[j].r)	/ 2;
			green = (psGamePal[i].g + psGamePal[j].g)	/ 2;
			blue = (psGamePal[i].b + psGamePal[j].b)	/ 2;
			transLookup[i][j] = pal_GetNearestColour(red,green,blue);
		}
	}
}


void pal_BuildAdjustedShadeTable( void )
{
float	redFraction, greenFraction, blueFraction;
int		seekRed, seekGreen,seekBlue;
int		numColours;
int		numShades;

	ASSERT((bPaletteInitialised,"pal_BuildAdjustedShadeTable, palette not initialised."));

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
	ASSERT((bPaletteInitialised,"pie_GetGamePal, palette not initialised"));
	return 	psGamePal;
}

PALETTEENTRY*	pie_GetWinPal(void)
{
	ASSERT((bPaletteInitialised,"pie_GetWinPal, palette not initialised"));
	return 	psWinPal;
}


/*





	End of PC Version






*/






