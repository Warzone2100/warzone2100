/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2010  Warzone 2100 Project

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

void pie_SetDefaultStates(void)//Sets all states
{
	PIELIGHT black;

	//fog off
	rendStates.fogEnabled = false;// enable fog before renderer
	rendStates.fog = false;//to force reset to false
	pie_SetFogStatus(false);
	black.rgba = 0;
	black.byte.a = 255;
	pie_SetFogColour(black);//nicks colour

	//depth Buffer on
	pie_SetDepthBufferStatus(DEPTH_CMP_LEQ_WRT_ON);

	rendStates.transMode = TRANS_ALPHA;//to force reset to DECAL
	pie_SetTranslucencyMode(TRANS_DECAL);

	//chroma keying on black
	rendStates.keyingOn = false;//to force reset to true
	pie_SetAlphaTest(true);
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
		if (val == true)
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

			black.rgba = 0;
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
			case REND_OPAQUE:
				pie_SetTranslucencyMode(TRANS_DECAL);
				break;

			case REND_ALPHA:
				pie_SetTranslucencyMode(TRANS_ALPHA);
				break;

			case REND_ADDITIVE:
				pie_SetTranslucencyMode(TRANS_ADDITIVE);
				break;

			case REND_MULTIPLICATIVE:
				pie_SetTranslucencyMode(TRANS_MULTIPLICATIVE);
				break;

			default:
				break;
		}
	}
	return;
}
