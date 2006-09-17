#include "piestate.h"

SDWORD		pieStateCount = 0; // FIXME Is this really used somewhere? Or is it just a dummy?
RENDER_STATE	rendStates;

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

	//set render mode
	pie_SetTranslucent(TRUE);
	pie_SetAdditive(TRUE);

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
// pie_SetTranslucent(BOOL val);
//
// Global enable/disable Translucent effects
//
//***************************************************************************

void pie_SetTranslucent(BOOL val)
{
	rendStates.translucent = val;
}

BOOL pie_Translucent(void)
{
	return rendStates.translucent;
}

//***************************************************************************
//
// pie_SetAdditive(BOOL val);
//
// Global enable/disable Additive effects
//
//***************************************************************************

void pie_SetAdditive(BOOL val)
{
	rendStates.additive = val;
}

BOOL pie_Additive(void)
{
	return rendStates.additive;
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
		val = FALSE;
	}
	if (rendStates.fogEnabled != val)
	{
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
#ifndef PIETOOL
	if (bilinearOn != rendStates.bilinearOn)
{
	rendStates.bilinearOn = bilinearOn;
	pieStateCount++;
}
#endif
}

BOOL pie_GetBilinear(void)
{
#ifndef PIETOOL
	return rendStates.bilinearOn;
#else
	return FALSE;
#endif
}

static void pie_SetTexCombine(TEX_MODE texCombMode)
{
#ifndef PIETOOL	//ffs
	if (texCombMode != rendStates.texCombine)
{
	rendStates.texCombine = texCombMode;
	pieStateCount++;
}
#endif
}

static void pie_SetAlphaCombine(ALPHA_MODE alphaCombMode)
{
#ifndef PIETOOL	//ffs
	if (alphaCombMode != rendStates.alphaCombine)
{
	rendStates.alphaCombine = alphaCombMode;
	pieStateCount++;
}
#endif
}

/***************************************************************************/
// get the constant colour used in text and flat render modes
/***************************************************************************/
UDWORD pie_GetColour(void)
{
	return	rendStates.colour;
}

void pie_SetMouse(IMAGEFILE *psImageFile,UWORD ImageID) // FIXME Remove if unused
{
}

void pie_SetSwirlyBoxes( BOOL val ) // FIXME Remove if unused
{
}
