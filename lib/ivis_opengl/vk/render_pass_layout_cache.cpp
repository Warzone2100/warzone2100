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
/** @file render_pass_layout_cache.cpp
 * Implementation of Vulkan render pass layout cache get-or-create.
 */

#if defined(WZ_VULKAN_ENABLED)

#include "vk/render_pass_layout_cache.h"

#include "gfx_api_vk.h"
#include "vk/layout_key_builder.h"
#include "vk/layout_sync.h"

#include <array>

namespace
{

bool validatePassLayoutKey(const gfx_api::vk::PassLayoutKey& key)
{
	if (key.resolveFormat.has_value())
	{
		if (key.colorFormats.size() != 1)
		{
			return false;
		}
		if (!key.depthFormat.has_value())
		{
			return false;
		}
	}
	return true;
}

::vk::SubpassDependency crossFrameExternalToSubpass0(
	::vk::PipelineStageFlags srcStage,
	::vk::PipelineStageFlags dstStage,
	::vk::AccessFlags srcAccess,
	::vk::AccessFlags dstAccess)
{
	return ::vk::SubpassDependency()
		.setSrcSubpass(VK_SUBPASS_EXTERNAL)
		.setDstSubpass(0)
		.setSrcStageMask(srcStage)
		.setDstStageMask(dstStage)
		.setSrcAccessMask(srcAccess)
		.setDstAccessMask(dstAccess)
		.setDependencyFlags(::vk::DependencyFlagBits::eByRegion);
}

void appendCrossFrameIncomingDependencies(
	const gfx_api::vk::PassLayoutKey& key,
	std::vector<::vk::SubpassDependency>& dependencies)
{
	const bool hasColor = !key.colorFormats.empty();
	const bool hasDepth = key.depthFormat.has_value();

	// Prior-frame shader read of depth → this pass depth write (depth-only and color+depth).
	if (hasDepth)
	{
		dependencies.push_back(crossFrameExternalToSubpass0(
			::vk::PipelineStageFlagBits::eFragmentShader,
			::vk::PipelineStageFlagBits::eEarlyFragmentTests,
			::vk::AccessFlagBits::eShaderRead,
			::vk::AccessFlagBits::eDepthStencilAttachmentWrite));
	}

	// Prior-frame shader read of color → this pass color write.
	if (hasColor)
	{
		dependencies.push_back(crossFrameExternalToSubpass0(
			::vk::PipelineStageFlagBits::eFragmentShader,
			::vk::PipelineStageFlagBits::eColorAttachmentOutput,
			::vk::AccessFlagBits::eShaderRead,
			::vk::AccessFlagBits::eColorAttachmentWrite));
	}

	// Cross-frame WAW on reused attachment writes (initialLayout is Undefined after Clear).
	if (hasColor)
	{
		dependencies.push_back(crossFrameExternalToSubpass0(
			::vk::PipelineStageFlagBits::eColorAttachmentOutput | ::vk::PipelineStageFlagBits::eLateFragmentTests,
			::vk::PipelineStageFlagBits::eColorAttachmentOutput | ::vk::PipelineStageFlagBits::eEarlyFragmentTests,
			::vk::AccessFlagBits::eColorAttachmentWrite | ::vk::AccessFlagBits::eDepthStencilAttachmentWrite,
			::vk::AccessFlagBits::eColorAttachmentWrite | ::vk::AccessFlagBits::eDepthStencilAttachmentWrite | ::vk::AccessFlagBits::eDepthStencilAttachmentRead));
	}
}

} // anonymous namespace

namespace gfx_api::vk
{

RenderPassLayoutCache::RenderPassLayoutCache(VkRoot& root)
	: _root(root)
{
}

void RenderPassLayoutCache::clear()
{
	_keys.clear();
}

const PassLayoutKey& RenderPassLayoutCache::keyAt(size_t layoutId) const
{
	ASSERT_OR_RETURN(PassLayoutKey::invalid(), layoutId < _keys.size(), "Render pass layout id out of range");
	return _keys[layoutId];
}

size_t RenderPassLayoutCache::getOrCreate(const PassLayoutKey& key)
{
	for (size_t i = 0; i < _keys.size(); ++i)
	{
		if (_keys[i] == key)
		{
			return i;
		}
	}

	ASSERT_OR_RETURN(0, validatePassLayoutKey(key), "Invalid PassLayoutKey for render pass creation");

	const size_t renderPassId = _root.renderPasses.size();
	_root.renderPasses.push_back(VkRoot::RenderPassDetails(renderPassId));

	const bool hasMsaaResolve = key.resolveFormat.has_value();
	const ::vk::SampleCountFlagBits sampleCount = (hasMsaaResolve && !key.colorSamples.empty())
		? key.colorSamples[0]
		: ::vk::SampleCountFlagBits::e1;

	std::vector<::vk::AttachmentDescription> attachments;
	if (hasMsaaResolve)
	{
		ASSERT_OR_RETURN(0, key.colorFormats.size() == 1, "MSAA resolve pass expects one color attachment");
		ASSERT_OR_RETURN(0, key.depthFormat.has_value(), "MSAA resolve pass requires depth");

		const gfx_api::AttachmentStoreOp msaaColorStoreOp = (key.colorStoreOps.size() > 0)
			? key.colorStoreOps[0]
			: gfx_api::AttachmentStoreOp::DontCare;
		const ::vk::AttachmentLoadOp vkColorLoadOp = toVkAttachmentLoadOp(key.colorLoadOps[0]);
		attachments.push_back(
			::vk::AttachmentDescription()
				.setFormat(key.colorFormats[0])
				.setSamples(sampleCount)
				.setLoadOp(vkColorLoadOp)
				.setStoreOp(toVkAttachmentStoreOp(msaaColorStoreOp))
				.setStencilLoadOp(::vk::AttachmentLoadOp::eDontCare)
				.setStencilStoreOp(::vk::AttachmentStoreOp::eDontCare)
				.setInitialLayout(resolveColorInitialLayout(key, 0))
				.setFinalLayout((!key.colorFinalLayouts.empty())
					? key.colorFinalLayouts[0]
					: colorFinalLayoutFromStoreOp(msaaColorStoreOp))
		);

		const ::vk::AttachmentLoadOp vkDepthLoadOp = toVkAttachmentLoadOp(key.depthLoadOp);
		const ::vk::AttachmentStoreOp vkDepthStoreOp = toVkAttachmentStoreOp(key.depthStoreOp);
		attachments.push_back(
			::vk::AttachmentDescription()
				.setFormat(key.depthFormat.value())
				.setSamples(sampleCount)
				.setLoadOp(vkDepthLoadOp)
				.setStoreOp(vkDepthStoreOp)
				.setStencilLoadOp(vkDepthLoadOp)
				.setStencilStoreOp(vkDepthStoreOp)
				.setInitialLayout(resolveDepthInitialLayout(key))
				.setFinalLayout(key.depthFinalLayout)
		);

		const ::vk::AttachmentLoadOp vkResolveLoadOp = toVkAttachmentLoadOp(key.resolveLoadOp);
		attachments.push_back(
			::vk::AttachmentDescription()
				.setFormat(key.resolveFormat.value())
				.setSamples(::vk::SampleCountFlagBits::e1)
				.setLoadOp(vkResolveLoadOp)
				.setStoreOp(toVkAttachmentStoreOp(key.resolveStoreOp))
				.setStencilLoadOp(::vk::AttachmentLoadOp::eDontCare)
				.setStencilStoreOp(::vk::AttachmentStoreOp::eDontCare)
				.setInitialLayout(resolveResolveInitialLayout(key))
				.setFinalLayout(key.resolveFinalLayout.has_value()
					? key.resolveFinalLayout.value()
					: colorFinalLayoutFromStoreOp(key.resolveStoreOp))
		);
	}
	else
	{
		attachments.reserve(key.colorFormats.size() + (key.depthFormat.has_value() ? 1 : 0));
		for (size_t i = 0; i < key.colorFormats.size(); ++i)
		{
			const ::vk::SampleCountFlagBits colorSamples = (i < key.colorSamples.size())
				? key.colorSamples[i]
				: ::vk::SampleCountFlagBits::e1;
			const gfx_api::AttachmentStoreOp colorStoreOp = (i < key.colorStoreOps.size())
				? key.colorStoreOps[i]
				: gfx_api::AttachmentStoreOp::Store;
			const ::vk::AttachmentLoadOp vkLoadOp = toVkAttachmentLoadOp(key.colorLoadOps[i]);
			attachments.push_back(
				::vk::AttachmentDescription()
					.setFormat(key.colorFormats[i])
					.setSamples(colorSamples)
					.setLoadOp(vkLoadOp)
					.setStoreOp(toVkAttachmentStoreOp(colorStoreOp))
					.setStencilLoadOp(::vk::AttachmentLoadOp::eDontCare)
					.setStencilStoreOp(::vk::AttachmentStoreOp::eDontCare)
					.setInitialLayout(resolveColorInitialLayout(key, i))
					.setFinalLayout((i < key.colorFinalLayouts.size())
						? key.colorFinalLayouts[i]
						: colorFinalLayoutFromStoreOp(colorStoreOp))
			);
		}

		if (key.depthFormat.has_value())
		{
			const ::vk::AttachmentLoadOp vkDepthLoadOp = toVkAttachmentLoadOp(key.depthLoadOp);
			attachments.push_back(
				::vk::AttachmentDescription()
					.setFormat(key.depthFormat.value())
					.setSamples(::vk::SampleCountFlagBits::e1)
					.setLoadOp(vkDepthLoadOp)
					.setStoreOp(toVkAttachmentStoreOp(key.depthStoreOp))
					.setStencilLoadOp(::vk::AttachmentLoadOp::eDontCare)
					.setStencilStoreOp(::vk::AttachmentStoreOp::eDontCare)
					.setInitialLayout(resolveDepthInitialLayout(key))
					.setFinalLayout(key.depthFinalLayout)
			);
		}
	}

	optional<::vk::AttachmentReference> depthStencilAttachmentRef;
	optional<::vk::AttachmentReference> colorAttachmentResolveRef;
	if (hasMsaaResolve)
	{
		depthStencilAttachmentRef = ::vk::AttachmentReference()
			.setAttachment(1)
			.setLayout(::vk::ImageLayout::eDepthStencilAttachmentOptimal);
		colorAttachmentResolveRef = ::vk::AttachmentReference()
			.setAttachment(2)
			.setLayout(::vk::ImageLayout::eColorAttachmentOptimal);
	}
	else if (key.depthFormat.has_value())
	{
		const uint32_t depthAttachmentIndex = static_cast<uint32_t>(attachments.size()) - 1;
		depthStencilAttachmentRef = ::vk::AttachmentReference()
			.setAttachment(depthAttachmentIndex)
			.setLayout(::vk::ImageLayout::eDepthStencilAttachmentOptimal);
	}

	std::vector<::vk::AttachmentReference> colorAttachmentRefs;
	if (hasMsaaResolve)
	{
		colorAttachmentRefs.push_back(
			::vk::AttachmentReference()
				.setAttachment(0)
				.setLayout(::vk::ImageLayout::eColorAttachmentOptimal)
		);
	}
	else
	{
		colorAttachmentRefs.reserve(key.colorFormats.size());
		for (uint32_t i = 0; i < static_cast<uint32_t>(key.colorFormats.size()); ++i)
		{
			colorAttachmentRefs.push_back(
				::vk::AttachmentReference()
					.setAttachment(i)
					.setLayout(::vk::ImageLayout::eColorAttachmentOptimal)
			);
		}
	}

	const auto subpasses = std::array<::vk::SubpassDescription, 1> {
		::vk::SubpassDescription()
			.setPipelineBindPoint(::vk::PipelineBindPoint::eGraphics)
			.setColorAttachmentCount(static_cast<uint32_t>(colorAttachmentRefs.size()))
			.setPColorAttachments(colorAttachmentRefs.data())
			.setPDepthStencilAttachment(depthStencilAttachmentRef.has_value() ? &depthStencilAttachmentRef.value() : nullptr)
			.setPResolveAttachments(colorAttachmentResolveRef.has_value() ? &colorAttachmentResolveRef.value() : nullptr)
	};

	// Outgoing producer synchronization, derived from the layout table: an attachment that ends
	// the pass in a read-only layout will be sampled by a later pass and thus needs a
	// 0->EXTERNAL dependency. The source scope is how this subpass produced it (its in-subpass
	// attachment layout); the destination scope is how the later pass consumes it (its read-only
	// final layout). One unioned dependency covers all such attachments and works identically for
	// the graph and legacy paths, with no surface/read-list special cases.
	::vk::PipelineStageFlags outSrcStage {};
	::vk::PipelineStageFlags outDstStage {};
	::vk::AccessFlags outSrcAccess {};
	::vk::AccessFlags outDstAccess {};
	bool anyReadOnlyFinal = false;
	const auto addReadOnlyFinal = [&](::vk::ImageLayout subpassLayout, ::vk::ImageLayout finalLayout)
	{
		const LayoutSync producer = layoutProducerSync(subpassLayout);
		const LayoutSync consumer = layoutConsumerSync(finalLayout);
		outSrcStage |= producer.stage;
		outSrcAccess |= producer.access;
		outDstStage |= consumer.stage;
		outDstAccess |= consumer.access;
		anyReadOnlyFinal = true;
	};
	for (const ::vk::ImageLayout colorFinalLayout : key.colorFinalLayouts)
	{
		if (colorFinalLayout == ::vk::ImageLayout::eShaderReadOnlyOptimal)
		{
			addReadOnlyFinal(::vk::ImageLayout::eColorAttachmentOptimal, colorFinalLayout);
		}
	}
	if (key.resolveFinalLayout.has_value()
		&& key.resolveFinalLayout.value() == ::vk::ImageLayout::eShaderReadOnlyOptimal)
	{
		addReadOnlyFinal(::vk::ImageLayout::eColorAttachmentOptimal, key.resolveFinalLayout.value());
	}
	if (key.depthFormat.has_value()
		&& key.depthFinalLayout == ::vk::ImageLayout::eDepthStencilReadOnlyOptimal)
	{
		addReadOnlyFinal(::vk::ImageLayout::eDepthStencilAttachmentOptimal, key.depthFinalLayout);
	}

	// Incoming consumer synchronization stays conservative (not table-derived). Reused render
	// targets are Cleared each frame, so their initialLayout is Undefined and cannot express the
	// cross-frame WAR/WAW hazard against the previous frame's access to the same image; these
	// templates encode that worst-case prior access (prior sample and/or prior attachment write).
	std::vector<::vk::SubpassDependency> dependencies;
	appendCrossFrameIncomingDependencies(key, dependencies);

	if (anyReadOnlyFinal)
	{
		dependencies.push_back(
			::vk::SubpassDependency()
				.setSrcSubpass(0)
				.setDstSubpass(VK_SUBPASS_EXTERNAL)
				.setSrcStageMask(outSrcStage)
				.setDstStageMask(outDstStage)
				.setSrcAccessMask(outSrcAccess)
				.setDstAccessMask(outDstAccess)
				.setDependencyFlags(::vk::DependencyFlagBits::eByRegion)
		);
	}

	auto createInfo = ::vk::RenderPassCreateInfo()
		.setAttachmentCount(static_cast<uint32_t>(attachments.size()))
		.setPAttachments(attachments.data())
		.setSubpassCount(static_cast<uint32_t>(subpasses.size()))
		.setPSubpasses(subpasses.data())
		.setDependencyCount(static_cast<uint32_t>(dependencies.size()))
		.setPDependencies(dependencies.data());

	auto& renderPassDetails = _root.renderPasses[renderPassId];
	renderPassDetails.rp_compat_info = std::make_shared<VkhRenderPassCompat>(createInfo);
	renderPassDetails.rp = _root.dev.createRenderPass(createInfo, nullptr, _root.vkDynLoader);
	renderPassDetails.msaaSamples = hasMsaaResolve ? _root.msaaSamples : ::vk::SampleCountFlagBits::e1;

	_keys.push_back(key);
	_root.ensureRenderPassPSOCapacity(_root.renderPasses.size());
	return renderPassId;
}

} // namespace gfx_api::vk

#endif // defined(WZ_VULKAN_ENABLED)
