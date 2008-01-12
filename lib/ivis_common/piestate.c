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
#include "piestate.h"

unsigned int pieStateCount = 0; // Used in pie_GetResetCounts
RENDER_STATE rendStates;

void pie_SetColourCombine(COLOUR_MODE colCombMode);
void pie_SetTranslucencyMode(TRANSLUCENCY_MODE transMode);

void pie_SetDefaultStates(void)//Sets all states
{
	PIELIGHT black;

	//fog off
	rendStates.fogEnabled = FALSE;// enable fog before renderer
	rendStates.fog = FALSE;//to force reset to false
	pie_SetFogStatus(FALSE);
	black.argb = 0;
	black.byte.a = 255;
	pie_SetFogColour(black);//nicks colour

	//depth Buffer on
	pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);

	rendStates.colourCombine = COLOUR_FLAT_CONSTANT;//to force reset to GOURAUD_TEX
	pie_SetColourCombine(COLOUR_TEX_ITERATED);
	rendStates.transMode = TRANS_ALPHA;//to force reset to DECAL
	pie_SetTranslucencyMode(TRANS_DECAL);

	//chroma keying on black
	rendStates.keyingOn = FALSE;//to force reset to true
	pie_SetAlphaTest(TRUE);
}


//***************************************************************************
//
// pie_EnableFog(BOOL val)
//
// Global enable/disable fog to allow fog to be turned of ingame
//
//***************************************************************************

void pie_EnableFog(BOOL val)
{
	if (rendStates.fogEnabled != val)
	{
		debug(LOG_FOG, "pie_EnableFog: Setting fog to %s", val ? "ON" : "OFF");
		rendStates.fogEnabled = val;
		if (val == TRUE)
		{
			PIELIGHT nickscolour;

			nickscolour.byte.r = 0xB0;
			nickscolour.byte.g = 0x08;
			nickscolour.byte.b = 0x5f;
			nickscolour.byte.a = 0xff;
			pie_SetFogColour(nickscolour); // nicks colour
		}
		else
		{
			PIELIGHT black;

			black.argb = 0;
			black.byte.a = 255;
			pie_SetFogColour(black); // clear background to black
		}
	}
}

BOOL pie_GetFogEnabled(void)
{
	return rendStates.fogEnabled;
}

//***************************************************************************
//
// pie_SetFogStatus(BOOL val)
//
// Toggle fog on and off for rendering objects inside or outside the 3D world
//
//***************************************************************************

BOOL pie_GetFogStatus(void)
{
	return rendStates.fog;
}

void pie_SetFogColour(PIELIGHT colour)
{
	rendStates.fogColour = colour;
}

PIELIGHT pie_GetFogColour(void)
{
	return rendStates.fogColour;
}

void pie_SetRendMode(REND_MODE rendMode)
{
	if (rendMode != rendStates.rendMode)
	{
		rendStates.rendMode = rendMode;
		switch (rendMode)
		{
			case REND_GOURAUD_TEX:
				pie_SetColourCombine(COLOUR_TEX_ITERATED);
				pie_SetTranslucencyMode(TRANS_DECAL);
				break;
			case REND_ALPHA_TEX:
				pie_SetColourCombine(COLOUR_TEX_ITERATED);
				pie_SetTranslucencyMode(TRANS_ALPHA);
				break;
			case REND_ADDITIVE_TEX:
				pie_SetColourCombine(COLOUR_TEX_ITERATED);
				pie_SetTranslucencyMode(TRANS_ADDITIVE);
				break;
			case REND_TEXT:
				pie_SetColourCombine(COLOUR_TEX_CONSTANT);
				pie_SetTranslucencyMode(TRANS_DECAL);
				break;
			case REND_ALPHA_TEXT:
				pie_SetColourCombine(COLOUR_TEX_CONSTANT);
				pie_SetTranslucencyMode(TRANS_ALPHA);
				break;
			case REND_ALPHA_FLAT:
				pie_SetColourCombine(COLOUR_FLAT_CONSTANT);
				pie_SetTranslucencyMode(TRANS_ALPHA);
				break;
			case REND_ALPHA_ITERATED:
				pie_SetColourCombine(COLOUR_FLAT_ITERATED);
				pie_SetTranslucencyMode(TRANS_ADDITIVE);
				break;
			case REND_FILTER_FLAT:
				pie_SetColourCombine(COLOUR_FLAT_CONSTANT);
				pie_SetTranslucencyMode(TRANS_FILTER);
				break;
			case REND_FILTER_ITERATED:
				pie_SetColourCombine(COLOUR_FLAT_CONSTANT);
				pie_SetTranslucencyMode(TRANS_ALPHA);
				break;
			case REND_FLAT:
				pie_SetColourCombine(COLOUR_FLAT_CONSTANT);
				pie_SetTranslucencyMode(TRANS_DECAL);
			default:
				break;
		}
	}
	return;
}
