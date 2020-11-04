/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
/** \file
 *  Renderer setup and state control routines for 3D rendering.
 */
#include "lib/framework/frame.h"

#include <physfs.h>
#include "lib/framework/physfs_ext.h"

#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piedef.h"
#include "lib/ivis_opengl/tex.h"
#include "lib/ivis_opengl/piepalette.h"
#include "screen.h"
#include "pieclip.h"
#include <glm/gtc/type_ptr.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
	#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/transform.hpp>

#include <algorithm>

/*
 *	Global Variables
 */

static gfx_api::gfxFloat shaderStretch = 0;
gfx_api::buffer* pie_internal::rectBuffer = nullptr;
static RENDER_STATE rendStates;
static int32_t ecmState = 0;
static gfx_api::gfxFloat timeState = 0.0f;

void rendStatesRendModeHack()
{
	rendStates.rendMode = REND_ALPHA;
}

/*
 *	Source
 */

void pie_SetDefaultStates()//Sets all states
{
	PIELIGHT black;

	//fog off
	rendStates.fogEnabled = false;// enable fog before renderer
	rendStates.fog = false;//to force reset to false
	pie_SetFogStatus(false);
	black.rgba = 0;
	black.byte.a = 255;
	pie_SetFogColour(black);

	rendStates.rendMode = REND_ALPHA;	// to force reset to REND_OPAQUE
}

//***************************************************************************
//
// pie_EnableFog(bool val)
//
// Global enable/disable fog to allow fog to be turned of ingame
//
//***************************************************************************
void pie_EnableFog(bool val)
{
	if (rendStates.fogEnabled != val)
	{
		debug(LOG_FOG, "pie_EnableFog: Setting fog to %s", val ? "ON" : "OFF");
		rendStates.fogEnabled = val;
		if (val)
		{
			pie_SetFogColour(WZCOL_FOG);
		}
		else
		{
			pie_SetFogColour(WZCOL_BLACK); // clear background to black
		}
	}
}

bool pie_GetFogEnabled()
{
	return rendStates.fogEnabled;
}

bool pie_GetFogStatus()
{
	return rendStates.fog;
}

void pie_SetFogColour(PIELIGHT colour)
{
	rendStates.fogColour = colour;
}

PIELIGHT pie_GetFogColour()
{
	return rendStates.fogColour;
}

void pie_FreeShaders()
{
	// no-op, currently
}

//static float fogBegin;
//static float fogEnd;

// Run from screen.c on init.
bool pie_LoadShaders()
{
	// note: actual loading of shaders now occurs in gfx_api

	gfx_api::gfxUByte rect[] {
		0, 255, 0, 255,
		0, 0, 0, 255,
		255, 255, 0, 255,
		255, 0, 0, 255
	};
	if (!pie_internal::rectBuffer)
		pie_internal::rectBuffer = gfx_api::context::get().create_buffer_object(gfx_api::buffer::usage::vertex_buffer);
	pie_internal::rectBuffer->upload(16 * sizeof(gfx_api::gfxUByte), rect);

	return true;
}

void pie_SetShaderTime(uint32_t shaderTime)
{
	uint32_t base = shaderTime % 1000;
	if (base > 500)
	{
		base = 1000 - base;	// cycle
	}
	timeState = (gfx_api::gfxFloat)base / 1000.0f;
}

float pie_GetShaderTime()
{
	return timeState;
}

void pie_SetShaderEcmEffect(bool value)
{
	ecmState = (int)value;
}

int pie_GetShaderEcmEffect()
{
	return ecmState;
}

void pie_SetShaderStretchDepth(float stretch)
{
	shaderStretch = stretch;
}

float pie_GetShaderStretchDepth()
{
	return shaderStretch;
}

/// Set the OpenGL fog start and end
void pie_UpdateFogDistance(float begin, float end)
{
	rendStates.fogBegin = begin;
	rendStates.fogEnd = end;
}

void pie_SetFogStatus(bool val)
{
	if (rendStates.fogEnabled)
	{
		//fog enabled so toggle if required
		rendStates.fog = val;
	}
	else
	{
		rendStates.fog = false;
	}
}

RENDER_STATE getCurrentRenderState()
{
	return rendStates;
}

int pie_GetMaxAntialiasing()
{
	int32_t maxSamples = gfx_api::context::get().get_context_value(gfx_api::context::context_value::MAX_SAMPLES);
	return maxSamples;
}
