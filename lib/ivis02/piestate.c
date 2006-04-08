/***************************************************************************/
/*
 * pieState.c
 *
 * renderer setup and state control routines for 3D rendering
 *
 */
/***************************************************************************/

#include "piestate.h"

/***************************************************************************/
/*
 *	Global Variables
 */
/***************************************************************************/

extern RENDER_STATE	rendStates;

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/

void pie_SetDepthBufferStatus(DEPTH_MODE depthMode)
{
#ifndef PIETOOL
	if (rendStates.depthBuffer != depthMode)
	{
		rendStates.depthBuffer = depthMode;
	}
#endif
}

//***************************************************************************
//
// pie_SetFogStatus(BOOL val)
//
// Toggle fog on and off for rendering objects inside or outside the 3D world
//
//***************************************************************************

void pie_SetFogStatus(BOOL val)
{
	if (rendStates.fogEnabled)
	{
		//fog enabled so toggle if required
		if (rendStates.fog != val)
		{
			rendStates.fog = val;

		}
	}
	else
	{
		//fog disabled so turn it off if not off already
		if (rendStates.fog != FALSE)
		{
			rendStates.fog = FALSE;

		}
	}
}

WZ_DEPRECATED void pie_SetTexturePage(SDWORD num) // FIXME Remove if unused
{
}

/***************************************************************************/

void pie_SetColourKeyedBlack(BOOL keyingOn)
{
#ifndef PIETOOL
	if (keyingOn != rendStates.keyingOn)
	{
		rendStates.keyingOn = keyingOn;
		pieStateCount++;
	}
#endif
}

/***************************************************************************/
void pie_SetColourCombine(COLOUR_MODE colCombMode)
{
#ifndef PIETOOL	//ffs

	if (colCombMode != rendStates.colourCombine) {
		rendStates.colourCombine = colCombMode;
		pieStateCount++;
	}
#endif
}

/***************************************************************************/
void pie_SetTranslucencyMode(TRANSLUCENCY_MODE transMode)
{
#ifndef PIETOOL
	if (transMode != rendStates.transMode)
	{
		rendStates.transMode = transMode;
		pieStateCount++;
	}
#endif
}

/***************************************************************************/
// set the constant colour used in text and flat render modes
/***************************************************************************/
void pie_SetColour(UDWORD colour)
{
	if (colour != rendStates.colour)
	{
		rendStates.colour = colour;
		pieStateCount++;

	}
}

/***************************************************************************/
void pie_SetGammaValue(float val)
{
	pieStateCount++;
}
