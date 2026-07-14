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
#include "lib/framework/wzapp.h"

#include "lib/ivis_opengl/piedef.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/gfx_api.h"
#include "lib/ivis_opengl/render_graph/cached_render_graph.h"
#include "lib/ivis_opengl/render_graph/topology.h"
#include "piematrix.h"
#include "lib/ivis_opengl/piefunc.h"
#include "lib/ivis_opengl/tex.h"
#include "lib/ivis_opengl/pieclip.h"
#include "screen.h"

#include <algorithm>

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
	if (wz_texture_compression)
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

	pie_InitializeInstancedRenderer();

	return true;
}


void pie_ShutDown()
{
	pie_CleanUp();
}

/***************************************************************************/

static bool renderingFrame = true; // true: realmain() ends one bootstrap frame before mainLoop's first Begin
static gfx_api::CachedRenderGraph g_cachedFrameRenderGraph;

gfx_api::CachedRenderGraph& pie_GetCachedFrameRenderGraph()
{
	return g_cachedFrameRenderGraph;
}

bool pie_IsScreenFrameRendering()
{
	return renderingFrame;
}

void pie_ScreenFrameRenderBegin()
{
	if (renderingFrame)
	{
		debug(LOG_WZ, "Call to pie_ScreenFrameRenderBegin when previous frame render hasn't ended yet");
		return;
	}
	renderingFrame = true;

	pie_ResetInGame3DFrameContextForFrame();

	if (gfx_api::context::isInitialized())
	{
		gfx_api::context::get().beginScreenFrame();
	}

	if (!gfx_api::context::isInitialized() || gfx_api::context::get().consumeScreenGeometryDirty())
	{
		screen_updateGeometry();
	}
}

void pie_ScreenFrameRenderEnd()
{
	if (!renderingFrame)
	{
		debug(LOG_WZ, "Call to pie_ScreenFrameRenderEnd without matching pie_ScreenFrameRenderBegin");
		return;
	}

	const gfx_api::IRenderTopologyQuery& query = gfx_api::getGameRenderTopologyQuery();
	if (!query.headlessOrSkipDrawing())
	{
		if (gfx_api::context::isInitialized())
		{
			gfx_api::context::get().prepareSwapchainForDrawing();
		}
		const gfx_api::RenderTopologySnapshot snapshot = gfx_api::render_topology::snapshot(query);
		gfx_api::CachedRenderGraph& graph = pie_GetCachedFrameRenderGraph();
		graph.ensureBuilt(snapshot);
		graph.execute();
		screenDoDumpToDiskIfRequired();
	}

	if (gfx_api::context::isInitialized())
	{
		gfx_api::context::get().finishScreenFrame();
	}

	renderingFrame = false;
	wzPerfFrame();
}

/***************************************************************************/
UDWORD	pie_GetResScalingFactor()
{
	UDWORD result = 0;
	if (pie_GetVideoBufferWidth() * 4 > pie_GetVideoBufferHeight() * 5)
	{
		result = pie_GetVideoBufferHeight() * 5 / 4 / 6;
	}
	else
	{
		result = pie_GetVideoBufferWidth() / 6;
	}
	return std::max<UDWORD>(result, 1);
}

