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
/** @file layout_key_builder.cpp
 * Implementation of pass layout key population for graph and legacy passes.
 */

#if defined(WZ_VULKAN_ENABLED)

#include "vk/layout_key_builder.h"

#include "gfx_api_vk.h"
#include "render_graph/layout_subresource.h"
#include "render_graph/pass_resolve.h"
#include "vk/layout_translation.h"

namespace gfx_api::vk
{

namespace
{

void clearPassLayoutKey(PassLayoutKey& layoutKey)
{
	layoutKey.colorFormats.clear();
	layoutKey.colorLoadOps.clear();
	layoutKey.colorStoreOps.clear();
	layoutKey.colorSamples.clear();
	layoutKey.colorInitialLayouts.clear();
	layoutKey.colorFinalLayouts.clear();
	layoutKey.resolveFormat.reset();
	layoutKey.resolveLoadOp = AttachmentLoadOp::DontCare;
	layoutKey.resolveStoreOp = AttachmentStoreOp::Store;
	layoutKey.resolveInitialLayout.reset();
	layoutKey.resolveFinalLayout.reset();
	layoutKey.depthFormat.reset();
	layoutKey.depthLoadOp = AttachmentLoadOp::DontCare;
	layoutKey.depthStoreOp = AttachmentStoreOp::DontCare;
	layoutKey.depthInitialLayout.reset();
	layoutKey.depthFinalLayout = ::vk::ImageLayout::eDepthStencilAttachmentOptimal;
}

::vk::ImageLayout initialColorAttachmentLayout(AttachmentLoadOp loadOp)
{
	return (loadOp == AttachmentLoadOp::Clear)
		? ::vk::ImageLayout::eUndefined
		: ::vk::ImageLayout::eColorAttachmentOptimal;
}

::vk::ImageLayout initialResolveAttachmentLayout(AttachmentLoadOp loadOp)
{
	return (loadOp == AttachmentLoadOp::Clear || loadOp == AttachmentLoadOp::DontCare)
		? ::vk::ImageLayout::eUndefined
		: ::vk::ImageLayout::eColorAttachmentOptimal;
}

::vk::ImageLayout initialDepthAttachmentLayout(AttachmentLoadOp loadOp)
{
	return (loadOp == AttachmentLoadOp::Clear)
		? ::vk::ImageLayout::eUndefined
		: ::vk::ImageLayout::eDepthStencilAttachmentOptimal;
}

::vk::ImageLayout depthFinalLayoutFromStoreOp(AttachmentStoreOp storeOp)
{
	return (storeOp == AttachmentStoreOp::Store)
		? ::vk::ImageLayout::eDepthStencilReadOnlyOptimal
		: ::vk::ImageLayout::eDepthStencilAttachmentOptimal;
}

void applyCompiledRenderPassLayoutsToKey(PassLayoutKey& layoutKey, const gfx_api::CompiledPass& compiledPass,
	const gfx_api::RenderPassDesc& pass)
{
	const gfx_api::CompiledPassLayoutMetadata& metadata = compiledPass.renderPassLayouts;

	layoutKey.colorInitialLayouts.clear();
	layoutKey.colorInitialLayouts.reserve(metadata.colorInitialLayouts.size());
	for (const gfx_api::CompileImageLayout colorInitialLayout : metadata.colorInitialLayouts)
	{
		layoutKey.colorInitialLayouts.push_back(toVkImageLayout(colorInitialLayout));
	}
	if (metadata.resolveInitialLayout.has_value())
	{
		layoutKey.resolveInitialLayout = toVkImageLayout(metadata.resolveInitialLayout.value());
	}
	else
	{
		layoutKey.resolveInitialLayout.reset();
	}
	if (pass.depthAttachment.has_value() && pass.depthAttachment->texture != nullptr)
	{
		layoutKey.depthInitialLayout = toVkImageLayout(metadata.depthInitialLayout);
	}

	layoutKey.colorFinalLayouts.clear();
	layoutKey.colorFinalLayouts.reserve(metadata.colorFinalLayouts.size());
	for (const gfx_api::CompileImageLayout colorFinalLayout : metadata.colorFinalLayouts)
	{
		layoutKey.colorFinalLayouts.push_back(toVkImageLayout(colorFinalLayout));
	}
	if (metadata.resolveFinalLayout.has_value())
	{
		layoutKey.resolveFinalLayout = toVkImageLayout(metadata.resolveFinalLayout.value());
	}
	else
	{
		layoutKey.resolveFinalLayout.reset();
	}
	if (pass.depthAttachment.has_value() && pass.depthAttachment->texture != nullptr)
	{
		layoutKey.depthFinalLayout = toVkImageLayout(metadata.depthFinalLayout);
	}
}

void ensurePassLayoutFinalLayouts(PassLayoutKey& layoutKey, const gfx_api::RenderPassDesc& pass)
{
	if (!layoutKey.colorFinalLayouts.empty())
	{
		return;
	}

	// Legacy (out-of-graph / ReadProducerScope::External) fallback: derive final layouts from store ops.
	// The swapchain is left in ColorAttachmentOptimal (the layout it is rendered in) so the only
	// swapchain transition is the single PresentSrcKHR barrier emitted once per frame in submitFrame().
	layoutKey.colorFinalLayouts.reserve(layoutKey.colorFormats.size());
	for (size_t i = 0; i < layoutKey.colorFormats.size(); ++i)
	{
		const AttachmentStoreOp colorStoreOp = (i < layoutKey.colorStoreOps.size())
			? layoutKey.colorStoreOps[i]
			: AttachmentStoreOp::Store;
		if (i < pass.colorAttachments.size()
			&& gfx_api::isSwapchainPresentableColorSurface(pass.colorAttachments[i].texture))
		{
			layoutKey.colorFinalLayouts.push_back(::vk::ImageLayout::eColorAttachmentOptimal);
		}
		else
		{
			layoutKey.colorFinalLayouts.push_back(colorFinalLayoutFromStoreOp(colorStoreOp));
		}
	}

	if (layoutKey.resolveFormat.has_value() && !layoutKey.resolveFinalLayout.has_value())
	{
		if (pass.resolveAttachment.has_value()
			&& gfx_api::isSwapchainPresentableColorSurface(pass.resolveAttachment->texture))
		{
			layoutKey.resolveFinalLayout = ::vk::ImageLayout::eColorAttachmentOptimal;
		}
		else
		{
			layoutKey.resolveFinalLayout = colorFinalLayoutFromStoreOp(layoutKey.resolveStoreOp);
		}
	}
}

} // anonymous namespace

::vk::AttachmentLoadOp toVkAttachmentLoadOp(AttachmentLoadOp loadOp)
{
	switch (loadOp)
	{
	case AttachmentLoadOp::Clear:
		return ::vk::AttachmentLoadOp::eClear;
	case AttachmentLoadOp::Load:
		return ::vk::AttachmentLoadOp::eLoad;
	case AttachmentLoadOp::DontCare:
		return ::vk::AttachmentLoadOp::eDontCare;
	}
	return ::vk::AttachmentLoadOp::eDontCare;
}

::vk::AttachmentStoreOp toVkAttachmentStoreOp(AttachmentStoreOp storeOp)
{
	switch (storeOp)
	{
	case AttachmentStoreOp::Store:
		return ::vk::AttachmentStoreOp::eStore;
	case AttachmentStoreOp::DontCare:
	case AttachmentStoreOp::Invalidate:
	case AttachmentStoreOp::Resolve:
		return ::vk::AttachmentStoreOp::eDontCare;
	}
	return ::vk::AttachmentStoreOp::eDontCare;
}

::vk::ImageLayout colorFinalLayoutFromStoreOp(AttachmentStoreOp storeOp)
{
	return (storeOp == AttachmentStoreOp::Store)
		? ::vk::ImageLayout::eShaderReadOnlyOptimal
		: ::vk::ImageLayout::eColorAttachmentOptimal;
}

::vk::ImageLayout resolveColorInitialLayout(const PassLayoutKey& key, size_t colorIndex)
{
	if (colorIndex < key.colorInitialLayouts.size())
	{
		return key.colorInitialLayouts[colorIndex];
	}
	return initialColorAttachmentLayout(key.colorLoadOps[colorIndex]);
}

::vk::ImageLayout resolveResolveInitialLayout(const PassLayoutKey& key)
{
	if (key.resolveInitialLayout.has_value())
	{
		return key.resolveInitialLayout.value();
	}
	return initialResolveAttachmentLayout(key.resolveLoadOp);
}

::vk::ImageLayout resolveDepthInitialLayout(const PassLayoutKey& key)
{
	if (key.depthInitialLayout.has_value())
	{
		return key.depthInitialLayout.value();
	}
	return initialDepthAttachmentLayout(key.depthLoadOp);
}

bool populatePassLayoutKeyFormats(PassLayoutKey& key, const gfx_api::RenderPassDesc& pass, const VkRoot& root)
{
	for (const auto& colorAttachment : pass.colorAttachments)
	{
		ASSERT_OR_RETURN(false, colorAttachment.texture != nullptr, "Unresolved color attachment in dynamic pass");
		key.colorFormats.push_back(root.getAttachmentVkFormat(colorAttachment.texture));
		key.colorLoadOps.push_back(colorAttachment.loadOp);
		key.colorStoreOps.push_back(gfx_api::attachmentStoreOpOr(colorAttachment));
		key.colorSamples.push_back(root.getAttachmentVkSamples(colorAttachment.texture));
	}
	if (gfx_api::passNeedsMsaaResolve(pass))
	{
		key.resolveFormat = root.getAttachmentVkFormat(pass.resolveAttachment->texture);
		key.resolveLoadOp = pass.resolveAttachment->loadOp;
		key.resolveStoreOp = gfx_api::attachmentStoreOpOr(pass.resolveAttachment.value());
	}
	if (pass.depthAttachment.has_value() && pass.depthAttachment->texture != nullptr)
	{
		key.depthFormat = root.getAttachmentVkFormat(pass.depthAttachment->texture);
		key.depthLoadOp = pass.depthAttachment->loadOp;
		key.depthStoreOp = gfx_api::attachmentStoreOpOr(pass.depthAttachment.value());
	}
	return true;
}

bool populatePassLayoutKeyLayouts(PassLayoutKey& key, const gfx_api::RenderPassDesc& pass,
	const LayoutKeyBuildContext& ctx)
{
	if (ctx.compiledPass != nullptr)
	{
		applyCompiledRenderPassLayoutsToKey(key, *ctx.compiledPass, pass);
		return true;
	}

	ASSERT_OR_RETURN(false, ctx.runtimeLayoutSource != nullptr, "Legacy pass layout key requires runtimeLayoutSource");

	const auto legacyInitialLayout = [&](const gfx_api::AttachmentDesc& attachment) -> ::vk::ImageLayout {
		if (attachment.texture == nullptr || attachment.loadOp != AttachmentLoadOp::Load)
		{
			return ::vk::ImageLayout::eUndefined;
		}
		return ctx.runtimeLayoutSource->getImageLayout(
			gfx_api::layoutSubresourceKey(attachment));
	};

	key.colorInitialLayouts.clear();
	key.colorInitialLayouts.reserve(pass.colorAttachments.size());
	for (const auto& colorAttachment : pass.colorAttachments)
	{
		key.colorInitialLayouts.push_back(legacyInitialLayout(colorAttachment));
	}
	if (gfx_api::passNeedsMsaaResolve(pass))
	{
		key.resolveInitialLayout = legacyInitialLayout(pass.resolveAttachment.value());
	}
	if (pass.depthAttachment.has_value() && pass.depthAttachment->texture != nullptr)
	{
		key.depthInitialLayout = legacyInitialLayout(pass.depthAttachment.value());
		key.depthFinalLayout = depthFinalLayoutFromStoreOp(key.depthStoreOp);
	}
	return true;
}

bool buildPassLayoutKey(PassLayoutKey& out, const gfx_api::RenderPassDesc& pass,
	const LayoutKeyBuildContext& ctx, const VkRoot& root)
{
	ASSERT_OR_RETURN(false, pass.viewportSize.has_value(), "Pass requires resolved viewportSize");

	clearPassLayoutKey(out);

	if (!populatePassLayoutKeyFormats(out, pass, root)
		|| !populatePassLayoutKeyLayouts(out, pass, ctx))
	{
		clearPassLayoutKey(out);
		return false;
	}
	ensurePassLayoutFinalLayouts(out, pass);
	return true;
}

} // namespace gfx_api::vk

#endif // defined(WZ_VULKAN_ENABLED)
