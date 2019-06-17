//
// VkhRenderPassCompat
// Version: 0.9.1
//
// Copyright (c) 2019 past-due
//
// https://github.com/past-due/vulkan-helpers
//
// Distributed under the MIT License.
// See accompanying file LICENSE or copy at https://opensource.org/licenses/MIT
//

#define VULKAN_HPP_TYPESAFE_CONVERSION 1
#include "vkh_renderpasscompat.hpp"

#include <algorithm>

VkhRenderPassCompat::VkhRenderPassCompat(const vk::RenderPassCreateInfo& createInfo)
{
	// Make a deep copy of a vk::RenderPassCreateInfo

	if (createInfo.pNext)
	{
		// TODO: VkhRenderPassCompat does not currently handle the pNext member of VkRenderPassCreateInfo
		//       - if either structure's has_pNext == true, they are currently treated as incompatible
		has_pNext = true;
	}

	// flags
	flags = createInfo.flags;

	// attachments
	attachments.reserve(createInfo.attachmentCount);
	for (uint32_t i = 0; i < createInfo.attachmentCount; ++i)
	{
		attachments.push_back(createInfo.pAttachments[i]);
	}

	// subpasses
	subpasses.reserve(createInfo.subpassCount);
	for (uint32_t i = 0; i < createInfo.subpassCount; ++i)
	{
		const auto& currSubpass = createInfo.pSubpasses[i];

		auto subpassDescDeepCopy = vk::SubpassDescription{};
		subpassDescDeepCopy.flags = currSubpass.flags;
		subpassDescDeepCopy.pipelineBindPoint = currSubpass.pipelineBindPoint;
		subpassDescDeepCopy.inputAttachmentCount = currSubpass.inputAttachmentCount;
		subpassDescDeepCopy.pInputAttachments = deepCopyAttachmentReferences(currSubpass.pInputAttachments, currSubpass.inputAttachmentCount);
		subpassDescDeepCopy.colorAttachmentCount = currSubpass.colorAttachmentCount;
		subpassDescDeepCopy.pColorAttachments = deepCopyAttachmentReferences(currSubpass.pColorAttachments, currSubpass.colorAttachmentCount);
		subpassDescDeepCopy.pResolveAttachments = deepCopyAttachmentReferences(currSubpass.pResolveAttachments, currSubpass.colorAttachmentCount);
		subpassDescDeepCopy.pDepthStencilAttachment = deepCopyAttachmentReference(currSubpass.pDepthStencilAttachment);
		subpassDescDeepCopy.preserveAttachmentCount = currSubpass.preserveAttachmentCount;
		subpassDescDeepCopy.pPreserveAttachments = deepCopy(currSubpass.pPreserveAttachments, currSubpass.preserveAttachmentCount, reserveAttachmentsCopies);
		subpasses.push_back(subpassDescDeepCopy);
	}

	// dependencies
	dependencies.reserve(createInfo.dependencyCount);
	for (uint32_t i = 0; i < createInfo.dependencyCount; ++i)
	{
		dependencies.push_back(createInfo.pDependencies[i]);
	}
}

bool VkhRenderPassCompat::isCompatibleWith(const VkhRenderPassCompat& other) const
{
	//		Two render passes are compatible if their corresponding
	//		color, input, resolve, and depth/stencil attachment references are compatible and if they are otherwise identical except for:
	//
	//		-	Initial and final image layout in attachment descriptions
	//		-	Load and store operations in attachment descriptions
	//		-	Image layout in attachment references

	// TODO: VkhRenderPassCompat does not currently (fully) handle pNext - if either structure's
	//       has_pNext == true, they are currently treated as incompatible
	if (has_pNext || other.has_pNext) return false; // TODO

	if (flags != other.flags) return false;

	if (subpasses.size() != other.subpasses.size())
	{
		return false;
	}

	for (size_t i = 0; i < subpasses.size(); ++i)
	{
		if (!checkSubpassCompatibility(subpasses[i], other.subpasses[i], subpasses.size() == 1)) return false;
	}

	if (dependencies.size() != other.dependencies.size())
	{
		return false;
	}

	for (size_t i = 0; i < dependencies.size(); ++i)
	{
		if (dependencies[i] != other.dependencies[i]) return false;
	}

	return true;
}

bool VkhRenderPassCompat::checkSubpassCompatibility(const vk::SubpassDescription& a, const vk::SubpassDescription& b, bool singleSubpassSpecialCase) const
{
	// TODO: Handle singleSubpassSpecialCase (optimization)
	// "As an additional special case, if two render passes have a single subpass, they are compatible even if they have different resolve attachment references or depth/stencil resolve modes but satisfy the other compatibility conditions."

	if (a.flags != b.flags) return false;
	if (a.pipelineBindPoint != b.pipelineBindPoint) return false;
	if (!checkArraysOfAttachmentRefsCompatibility(a.pInputAttachments, a.inputAttachmentCount,
												  b.pInputAttachments, b.inputAttachmentCount)) return false;
	if (!checkArraysOfAttachmentRefsCompatibility(a.pColorAttachments, a.colorAttachmentCount,
												  b.pColorAttachments, b.colorAttachmentCount)) return false;
	if (!checkArraysOfAttachmentRefsCompatibility(a.pResolveAttachments, a.colorAttachmentCount,
												  b.pResolveAttachments, b.colorAttachmentCount)) return false;
	if (!checkAttachmentRefCompatibility(a.pDepthStencilAttachment, b.pDepthStencilAttachment)) return false;
	if (a.preserveAttachmentCount != b.preserveAttachmentCount) return false;
	for (uint32_t i = 0; i < a.preserveAttachmentCount; ++i)
	{
		if (a.pPreserveAttachments[i] != b.pPreserveAttachments[i]) return false;
	}

	return true;
}

bool VkhRenderPassCompat::checkArraysOfAttachmentRefsCompatibility(const vk::AttachmentReference* pAttachmentsA, uint32_t attachmentCountA, const vk::AttachmentReference* pAttachmentsB, uint32_t attachmentCountB) const
{
	// Two arrays of attachment references are compatible if all corresponding pairs of attachments are compatible. If the arrays are of different lengths, attachment references not present in the smaller array are treated as VK_ATTACHMENT_UNUSED.

	size_t maxAttachmentsCount = std::max(attachmentCountA, attachmentCountB);
	for (size_t i = 0; i < maxAttachmentsCount; ++i)
	{
		const vk::AttachmentReference* pRefA = (i < attachmentCountA) ? &pAttachmentsA[i] : nullptr;
		const vk::AttachmentReference* pRefB = (i < attachmentCountB) ? &pAttachmentsB[i] : nullptr;

		if (!checkAttachmentRefCompatibility(pRefA, pRefB)) return false;
	}
	return true;
}

bool VkhRenderPassCompat::checkAttachmentRefCompatibility(const vk::AttachmentReference* pRefA, const vk::AttachmentReference* pRefB) const
{
	// Two attachment references are compatible if they have matching format and sample count, or are both VK_ATTACHMENT_UNUSED or the pointer that would contain the reference is NULL.

	if (pRefA == nullptr || pRefB == nullptr)
	{
		return pRefA == pRefB;
	}
	if(!((pRefA->attachment < attachments.size()) && (pRefB->attachment < attachments.size())))
	{
		throw;
	}

	const auto& attachmentA = attachments[static_cast<size_t>(pRefA->attachment)];
	const auto& attachmentB = attachments[static_cast<size_t>(pRefB->attachment)];

	return attachmentA.format == attachmentB.format
	&& attachmentA.samples == attachmentB.samples;
}

const vk::AttachmentReference* VkhRenderPassCompat::deepCopyAttachmentReference(const vk::AttachmentReference* pAttachment)
{
	if (pAttachment == nullptr)
	{
		return nullptr;
	}
	return deepCopyAttachmentReferences(pAttachment, 1);
}

// deep copies attachment references to internal storage
const vk::AttachmentReference* VkhRenderPassCompat::deepCopyAttachmentReferences(const vk::AttachmentReference* pAttachments, uint32_t attachmentCount)
{
	return deepCopy(pAttachments, attachmentCount, attachmentRefCopies);
}


