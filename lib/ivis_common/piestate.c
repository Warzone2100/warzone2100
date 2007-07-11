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
static void pie_SetTexCombine(TEX_MODE texCombMode);
static void pie_SetAlphaCombine(ALPHA_MODE alphaCombMode);

void pie_SetDefaultStates(void)//Sets all states
{
//		pie_SetFogColour(0x00B08f5f);//nicks colour
	//fog off
	rendStates.fogEnabled = FALSE;// enable fog before renderer
	rendStates.fog = FALSE;//to force reset to false
	pie_SetFogStatus(FALSE);
	pie_SetFogColour(0x00000000);//nicks colour

	//depth Buffer on
	pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);

	//basic gouraud textured rendering
	rendStates.texCombine = TEX_NONE;//to force reset to GOURAUD_TEX
	pie_SetTexCombine(TEX_LOCAL);
	rendStates.colourCombine = COLOUR_FLAT_CONSTANT;//to force reset to GOURAUD_TEX
	pie_SetColourCombine(COLOUR_TEX_ITERATED);
	rendStates.alphaCombine = ALPHA_ITERATED;//to force reset to GOURAUD_TEX
	pie_SetAlphaCombine(ALPHA_CONSTANT);
	rendStates.transMode = TRANS_ALPHA;//to force reset to DECAL
	pie_SetTranslucencyMode(TRANS_DECAL);

	//chroma keying on black
	rendStates.keyingOn = FALSE;//to force reset to true
	pie_SetColourKeyedBlack(TRUE);

	//bilinear filtering
	rendStates.bilinearOn = FALSE;//to force reset to true
	pie_SetBilinear(TRUE);
}


//***************************************************************************
//
// pie_SetCaps(BOOL val);
//
// HIGHEST LEVEL enable/disable modes
//
//***************************************************************************

void pie_SetFogCap(FOG_CAP val)
{
	rendStates.fogCap = val;
}

FOG_CAP pie_GetFogCap(void)
{
	return rendStates.fogCap;
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
	if (rendStates.fogCap == FOG_CAP_NO)
	{
		debug(LOG_FOG, "pie_EnableFog: Trying to fog set fog to %s, but global fog disabled", val ? "ON" : "OFF");
		val = FALSE;
	}
	if (rendStates.fogEnabled != val)
	{
		debug(LOG_FOG, "pie_EnableFog: Setting fog to %s", val ? "ON" : "OFF");
		rendStates.fogEnabled = val;
		if (val == TRUE)
		{
//			pie_SetFogColour(0x0078684f);//(nicks colour + 404040)/2
			pie_SetFogColour(0x00B08f5f);//nicks colour
		}
		else
		{
			pie_SetFogColour(0x00000000);//clear background to black
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

void pie_SetFogColour(UDWORD colour)
{
	UDWORD grey;
	if (rendStates.fogCap == FOG_CAP_GREY)
	{
		grey = colour & 0xff;
		colour >>= 8;
		grey += (colour & 0xff);
		colour >>= 8;
		grey += (colour & 0xff);
		grey /= 3;
		grey &= 0xff;//check only
		colour = grey + (grey<<8) + (grey<<16);
		rendStates.fogColour = colour;
	}
	else if (rendStates.fogCap == FOG_CAP_NO)
	{
		rendStates.fogColour = 0;
	}
	else
	{
		rendStates.fogColour = colour;
	}
}

UDWORD pie_GetFogColour(void)
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
				pie_SetTexCombine(TEX_LOCAL);
				pie_SetAlphaCombine(ALPHA_CONSTANT);
				pie_SetTranslucencyMode(TRANS_DECAL);
				break;
			case REND_ALPHA_TEX:
				pie_SetColourCombine(COLOUR_TEX_ITERATED);
				pie_SetTexCombine(TEX_LOCAL);
				pie_SetAlphaCombine(ALPHA_ITERATED);
				pie_SetTranslucencyMode(TRANS_ALPHA);
				break;
			case REND_ADDITIVE_TEX:
				pie_SetColourCombine(COLOUR_TEX_ITERATED);
				pie_SetTexCombine(TEX_LOCAL);
				pie_SetAlphaCombine(ALPHA_ITERATED);
				pie_SetTranslucencyMode(TRANS_ADDITIVE);
				break;
			case REND_TEXT:
				pie_SetColourCombine(COLOUR_TEX_CONSTANT);
				pie_SetTexCombine(TEX_LOCAL);
				pie_SetAlphaCombine(ALPHA_CONSTANT);
				pie_SetTranslucencyMode(TRANS_DECAL);
				break;
			case REND_ALPHA_TEXT:
				pie_SetColourCombine(COLOUR_TEX_CONSTANT);
				pie_SetTexCombine(TEX_LOCAL);
				pie_SetAlphaCombine(ALPHA_CONSTANT);
				pie_SetTranslucencyMode(TRANS_ALPHA);
				break;
			case REND_ALPHA_FLAT:
				pie_SetColourCombine(COLOUR_FLAT_CONSTANT);
				pie_SetTexCombine(TEX_LOCAL);
				pie_SetAlphaCombine(ALPHA_CONSTANT);
				pie_SetTranslucencyMode(TRANS_ALPHA);
				break;
			case REND_ALPHA_ITERATED:
				pie_SetColourCombine(COLOUR_FLAT_ITERATED);
				pie_SetTexCombine(TEX_LOCAL);
				pie_SetAlphaCombine(ALPHA_ITERATED);
				pie_SetTranslucencyMode(TRANS_ADDITIVE);
				break;
			case REND_FILTER_FLAT:
				pie_SetColourCombine(COLOUR_FLAT_CONSTANT);
				pie_SetTexCombine(TEX_LOCAL);
				pie_SetAlphaCombine(ALPHA_CONSTANT);
				pie_SetTranslucencyMode(TRANS_FILTER);
				break;
			case REND_FILTER_ITERATED:
				pie_SetColourCombine(COLOUR_FLAT_CONSTANT);
				pie_SetTexCombine(TEX_LOCAL);
				pie_SetAlphaCombine(ALPHA_ITERATED);
				pie_SetTranslucencyMode(TRANS_ALPHA);
				break;
			case REND_FLAT:
				pie_SetColourCombine(COLOUR_FLAT_CONSTANT);
				pie_SetTexCombine(TEX_LOCAL);
				pie_SetAlphaCombine(ALPHA_CONSTANT);
				pie_SetTranslucencyMode(TRANS_DECAL);
			default:
				break;
		}
	}
	return;
}

void pie_SetBilinear(BOOL bilinearOn)
{
	if (bilinearOn != rendStates.bilinearOn)
	{
		rendStates.bilinearOn = bilinearOn;
		pieStateCount++;
	}
}

BOOL pie_GetBilinear(void)
{
	return rendStates.bilinearOn;
}

static void pie_SetTexCombine(TEX_MODE texCombMode)
{
	if (texCombMode != rendStates.texCombine)
	{
		rendStates.texCombine = texCombMode;
		pieStateCount++;
	}
}

static void pie_SetAlphaCombine(ALPHA_MODE alphaCombMode)
{
	if (alphaCombMode != rendStates.alphaCombine)
	{
		rendStates.alphaCombine = alphaCombMode;
		pieStateCount++;
	}
}

/***************************************************************************/
// get the constant colour used in text and flat render modes
/***************************************************************************/
UDWORD pie_GetColour(void)
{
	return	rendStates.colour;
}
