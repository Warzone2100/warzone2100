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
/***************************************************************************/
/*
 * pieMode.h
 *
 * renderer control for pumpkin library functions.
 *
 */
/***************************************************************************/

#include "lib/framework/frame.h"
#include "lib/framework/opengl.h"
#include "lib/framework/wzapp.h"

#include "lib/ivis_opengl/piedef.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piemode.h"
#include "piematrix.h"
#include "lib/ivis_opengl/piefunc.h"
#include "lib/ivis_opengl/tex.h"
#include "lib/ivis_opengl/pieclip.h"
#include "screen.h"

/***************************************************************************/
/*
 *	Source
 */
/***************************************************************************/

iSurface rendSurface;

void pie_UpdateSurfaceGeometry()
{
	rendSurface.width	= pie_GetVideoBufferWidth();
	rendSurface.height	= pie_GetVideoBufferHeight();
	rendSurface.xcentre	= pie_GetVideoBufferWidth() / 2;
	rendSurface.ycentre	= pie_GetVideoBufferHeight() / 2;
	rendSurface.clip.left	= 0;
	rendSurface.clip.top	= 0;
	rendSurface.clip.right	= pie_GetVideoBufferWidth();
	rendSurface.clip.bottom	= pie_GetVideoBufferHeight();
}

bool pie_Initialise()
{
	pie_TexInit();

	/* Find texture compression extension */
	if (GLAD_GL_ARB_texture_compression && wz_texture_compression)
	{
		debug(LOG_TEXTURE, "Texture compression: Yes");
	}
	else
	{
		debug(LOG_TEXTURE, "Texture compression: No");
	}

	pie_UpdateSurfaceGeometry();

	pie_SetDefaultStates();
	debug(LOG_3D, "xcentre %d; ycentre %d", rendSurface.xcentre, rendSurface.ycentre);

	return true;
}


void pie_ShutDown()
{
	pie_CleanUp();
}

/***************************************************************************/

void pie_ScreenFlip(int clearMode)
{
	screenDoDumpToDiskIfRequired();
	gfx_api::context::get().flip(clearMode);
	wzPerfFrame();

	if (screen_GetBackDrop())
	{
		screen_Display();
	}
}

/***************************************************************************/
UDWORD	pie_GetResScalingFactor()
{
	if (pie_GetVideoBufferWidth() * 4 > pie_GetVideoBufferHeight() * 5)
	{
		return pie_GetVideoBufferHeight() * 5 / 4 / 6;
	}
	else
	{
		return pie_GetVideoBufferWidth() / 6;
	}
}

