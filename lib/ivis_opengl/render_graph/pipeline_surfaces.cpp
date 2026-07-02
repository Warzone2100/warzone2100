// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
/** @file pipeline_surfaces.cpp
 * Implementation of pipeline surface metadata and registry lookup helpers.
 */

#include "pipeline_surfaces.h"

#include "gfx_api.h"

#include "lib/framework/wzapp.h"

namespace gfx_api
{

PipelineSurfaceMeta getPipelineSurfaceMeta(PipelineSurfaceId id)
{
	PipelineSurfaceMeta meta;
	switch (id)
	{
	case PipelineSurfaceId::SceneColor:
		meta.usage = PipelineSurfaceUsage::ColorResolve;
		meta.format = pixel_format::FORMAT_RGBA8_UNORM_PACK8;
		meta.samples = 1;
		break;
	case PipelineSurfaceId::SceneMSAAColor:
		meta.usage = PipelineSurfaceUsage::ColorMSAA;
		meta.format = pixel_format::FORMAT_RGBA8_UNORM_PACK8;
		meta.samples = 1;
		break;
	case PipelineSurfaceId::SceneDepth:
		meta.usage = PipelineSurfaceUsage::DepthStencil;
		meta.format = pixel_format::FORMAT_D24_UNORM_S8;
		meta.samples = 1;
		break;
	case PipelineSurfaceId::ShadowMap:
		meta.usage = PipelineSurfaceUsage::DepthOnly;
		meta.format = pixel_format::FORMAT_D24_UNORM_S8;
		meta.samples = 1;
		break;
	case PipelineSurfaceId::SwapchainColor:
		meta.usage = PipelineSurfaceUsage::SwapchainPresent;
		meta.format = pixel_format::FORMAT_RGBA8_UNORM_PACK8;
		meta.samples = 1;
		break;
	case PipelineSurfaceId::SwapchainMSAAColor:
		meta.usage = PipelineSurfaceUsage::ColorMSAA;
		meta.format = pixel_format::FORMAT_RGBA8_UNORM_PACK8;
		meta.samples = 1;
		break;
	case PipelineSurfaceId::SwapchainDepth:
		meta.usage = PipelineSurfaceUsage::DepthStencil;
		meta.format = pixel_format::FORMAT_D24_UNORM_S8;
		meta.samples = 1;
		break;
	case PipelineSurfaceId::Count:
		ASSERT(false, "Invalid pipeline surface ID");
		break;
	}
	return meta;
}

void PipelineSurfaceRegistry::registerSurface(PipelineSurfaceId id, abstract_texture* surface)
{
	_surfaces[static_cast<size_t>(id)] = surface;
}

void PipelineSurfaceRegistry::invalidateSurface(PipelineSurfaceId id)
{
	_surfaces[static_cast<size_t>(id)] = nullptr;
	_samplesOverride[static_cast<size_t>(id)] = 0;
}

abstract_texture* PipelineSurfaceRegistry::get(PipelineSurfaceId id) const
{
	return _surfaces[static_cast<size_t>(id)];
}

void PipelineSurfaceRegistry::setSurfaceSamples(PipelineSurfaceId id, uint32_t samples)
{
	_samplesOverride[static_cast<size_t>(id)] = samples;
}

PipelineSurfaceMeta PipelineSurfaceRegistry::meta(PipelineSurfaceId id) const
{
	PipelineSurfaceMeta result = getPipelineSurfaceMeta(id);
	const uint32_t overrideSamples = _samplesOverride[static_cast<size_t>(id)];
	if (overrideSamples > 0)
	{
		result.samples = overrideSamples;
	}
	return result;
}

nonstd::optional<PipelineSurfaceId> PipelineSurfaceRegistry::findSurfaceId(abstract_texture* texture) const
{
	if (texture == nullptr)
	{
		return nonstd::nullopt;
	}
	for (size_t i = 0; i < static_cast<size_t>(PipelineSurfaceId::Count); ++i)
	{
		if (_surfaces[i] == texture)
		{
			return static_cast<PipelineSurfaceId>(i);
		}
	}
	return nonstd::nullopt;
}

AttachmentDesc makePipelineSurfaceAttachment(PipelineSurfaceId id, AttachmentLoadOp op, ClearValue clear)
{
	AttachmentDesc desc;
	desc.texture = context::get().getPipelineSurface(id);
	ASSERT(desc.texture != nullptr, "Pipeline surface %u is not registered", static_cast<unsigned>(id));
	desc.loadOp = op;
	desc.clearValue = clear;
	return desc;
}

} // namespace gfx_api
