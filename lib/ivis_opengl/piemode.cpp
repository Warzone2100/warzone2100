/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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
#include "lib/ivis_opengl/ivi.h"
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

bool pie_Initialise(void)
{
	pie_SetUp();
	pie_TexInit();

	/* Find texture compression extension */
	if (GLEW_ARB_texture_compression && wz_texture_compression != GL_RGBA)
	{
		debug(LOG_TEXTURE, "Texture compression: Yes");
		wz_texture_compression = GL_COMPRESSED_RGBA_ARB;
	}
	else
	{
		debug(LOG_TEXTURE, "Texture compression: No");
		wz_texture_compression = GL_RGBA;
	}

	pie_MatInit();
	_TEX_INDEX = 0;

	rendSurface.width	= pie_GetVideoBufferWidth();
	rendSurface.height	= pie_GetVideoBufferHeight();
	rendSurface.xcentre	= pie_GetVideoBufferWidth() / 2;
	rendSurface.ycentre	= pie_GetVideoBufferHeight() / 2;
	rendSurface.clip.left	= 0;
	rendSurface.clip.top	= 0;
	rendSurface.clip.right	= pie_GetVideoBufferWidth();
	rendSurface.clip.bottom	= pie_GetVideoBufferHeight();

	pie_SetDefaultStates();
	debug(LOG_3D, "xcentre %d; ycentre %d", rendSurface.xcentre, rendSurface.ycentre);

	return true;
}


void pie_ShutDown(void)
{
	pie_CleanUp();
}

/***************************************************************************/

void pie_ScreenFlip(int clearMode)
{
	GLbitfield clearFlags = 0;

	screenDoDumpToDiskIfRequired();
	wzScreenFlip();
	if (clearMode & CLEAR_OFF_AND_NO_BUFFER_DOWNLOAD)
	{
		return;
	}

	glDepthMask(GL_TRUE);
	clearFlags = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
	if (clearMode & CLEAR_SHADOW)
	{
		clearFlags |= GL_STENCIL_BUFFER_BIT;
	}
	glClear(clearFlags);

	if (screen_GetBackDrop())
	{
		screen_Upload(NULL, screen_getMapPreview());
	}
}

/***************************************************************************/
UDWORD	pie_GetResScalingFactor(void)
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

