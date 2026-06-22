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
/** @file gfx_api_legacy_pass_compat.cpp
 * Implementation of legacy pass description builders shared by Vulkan and OpenGL.
 */


#include "gfx_api_legacy_pass_compat.h"

#include "gfx_api.h"
#include "render_graph/pipeline_surfaces.h"

#include "lib/framework/wzapp.h"

namespace gfx_api
{
namespace legacy_pass
{

namespace
{

AttachmentDesc pipelineDepthAttachment(PipelineSurfaceId id, AttachmentLoadOp loadOp,
	AttachmentStoreOp storeOp, uint32_t arrayLayer = 0)
{
	AttachmentDesc attachment = makePipelineSurfaceAttachment(id, loadOp, ClearValue::depthStencilClear());
	attachment.storeOp = storeOp;
	attachment.arrayLayer = arrayLayer;
	return attachment;
}

AttachmentDesc pipelineColorAttachment(PipelineSurfaceId id, AttachmentLoadOp loadOp,
	AttachmentStoreOp storeOp)
{
	AttachmentDesc attachment = makePipelineSurfaceAttachment(id, loadOp, ClearValue::colorClear());
	attachment.storeOp = storeOp;
	return attachment;
}

void setViewportFromSurface(RenderPassDesc& pass, PipelineSurfaceId id)
{
	auto& ctx = gfx_api::context::get();
	if (abstract_texture* surface = ctx.getPipelineSurface(id))
	{
		if (const auto dims = ctx.getRenderTargetDimensions(surface))
		{
			pass.viewportSize = *dims;
			return;
		}
	}

	const auto drawable = ctx.getDrawableDimensions();
	pass.viewportSize = drawable;
}

} // anonymous namespace

RenderPassDesc buildShadowCascadePassDesc(size_t cascadeIndex)
{
	RenderPassDesc pass;
	pass.debugName = "LegacyShadowCascade";
	pass.depthAttachment = pipelineDepthAttachment(PipelineSurfaceId::ShadowMap,
		AttachmentLoadOp::Clear, AttachmentStoreOp::Store, static_cast<uint32_t>(cascadeIndex));
	setViewportFromSurface(pass, PipelineSurfaceId::ShadowMap);
	return pass;
}

RenderPassDesc buildScenePassDesc()
{
	auto& ctx = gfx_api::context::get();
	RenderPassDesc pass;
	pass.debugName = "LegacyScenePass";

	if (ctx.isSceneMSAAEnabled())
	{
		pass.colorAttachments.push_back(pipelineColorAttachment(PipelineSurfaceId::SceneMSAAColor,
			AttachmentLoadOp::Clear, AttachmentStoreOp::DontCare));
		pass.resolveAttachment = pipelineColorAttachment(PipelineSurfaceId::SceneColor,
			AttachmentLoadOp::DontCare, AttachmentStoreOp::Store);
	}
	else
	{
		pass.colorAttachments.push_back(pipelineColorAttachment(PipelineSurfaceId::SceneColor,
			AttachmentLoadOp::Clear, AttachmentStoreOp::Store));
	}

	pass.depthAttachment = pipelineDepthAttachment(PipelineSurfaceId::SceneDepth,
		AttachmentLoadOp::Clear, AttachmentStoreOp::Invalidate);
	setViewportFromSurface(pass, PipelineSurfaceId::SceneColor);
	return pass;
}

RenderPassDesc buildSwapchainPassDesc(AttachmentLoadOp colorLoad, AttachmentLoadOp depthLoad)
{
	auto& ctx = gfx_api::context::get();
	RenderPassDesc pass;
	pass.debugName = "LegacySwapchain";

	if (ctx.isSwapchainMSAAEnabled())
	{
		pass.colorAttachments.push_back(pipelineColorAttachment(PipelineSurfaceId::SwapchainMSAAColor,
			colorLoad, AttachmentStoreOp::DontCare));
		pass.resolveAttachment = pipelineColorAttachment(PipelineSurfaceId::SwapchainColor,
			AttachmentLoadOp::DontCare, AttachmentStoreOp::Store);
	}
	else
	{
		pass.colorAttachments.push_back(pipelineColorAttachment(PipelineSurfaceId::SwapchainColor,
			colorLoad, AttachmentStoreOp::Store));
	}

	pass.depthAttachment = pipelineDepthAttachment(PipelineSurfaceId::SwapchainDepth,
		depthLoad, AttachmentStoreOp::DontCare);
	pass.viewportSize = ctx.getDrawableDimensions();
	return pass;
}

} // namespace legacy_pass

} // namespace gfx_api
