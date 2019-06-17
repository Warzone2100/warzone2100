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

#pragma once

#if defined( _MSC_VER )
#pragma warning( push )
#pragma warning( disable : 4191 ) // warning C4191: '<function-style-cast>': unsafe conversion from 'PFN_vkVoidFunction' to 'PFN_vk<...>'
#endif
#include <vulkan/vulkan.hpp>
#if defined( _MSC_VER )
#pragma warning( pop )
#endif

#include <vector>
#include <memory>

struct VkhRenderPassCompat
{
public:
	VkhRenderPassCompat(const vk::RenderPassCreateInfo& createInfo);
	bool isCompatibleWith(const VkhRenderPassCompat& other) const;

	// non-copyable
	VkhRenderPassCompat(const VkhRenderPassCompat& other) = delete;
	VkhRenderPassCompat& operator=(const VkhRenderPassCompat&) = delete;

private:

	// deep copies attachment references to internal storage
	const vk::AttachmentReference* deepCopyAttachmentReference(const vk::AttachmentReference* pAttachment);
	const vk::AttachmentReference* deepCopyAttachmentReferences(const vk::AttachmentReference* pAttachments, uint32_t attachmentCount);

	template<typename A>
	const A* deepCopy(const A* pItems, uint32_t itemCount, std::vector<std::unique_ptr<const std::vector<A>>>& output)
	{
		if (itemCount == 0 || pItems == nullptr)
		{
			return nullptr;
		}

		using vectorA = std::vector<A>;
		auto pItemsCopy = new vectorA();
		pItemsCopy->reserve(itemCount);
		std::copy(pItems, pItems + itemCount, std::back_inserter(*pItemsCopy));
		output.push_back(std::unique_ptr<const vectorA>(pItemsCopy));
		return output.back()->data();
	}

	bool checkAttachmentRefCompatibility(const vk::AttachmentReference* pRefA, const vk::AttachmentReference* pRefB) const;

	bool checkArraysOfAttachmentRefsCompatibility(const vk::AttachmentReference* pAttachmentsA, uint32_t attachmentCountA, const vk::AttachmentReference* pAttachmentsB, uint32_t attachmentCountB) const;

	bool checkSubpassCompatibility(const vk::SubpassDescription& a, const vk::SubpassDescription& b, bool singleSubpassSpecialCase) const;

private:

	bool has_pNext = false;
	vk::RenderPassCreateFlags flags;
	std::vector<vk::AttachmentDescription> attachments;
	std::vector<vk::SubpassDescription> subpasses;
	std::vector<vk::SubpassDependency> dependencies;
	typedef std::vector<vk::AttachmentReference> attachmentRefVector;
	typedef std::unique_ptr<const attachmentRefVector> attachmentRefVectorPt;
	std::vector<attachmentRefVectorPt> attachmentRefCopies;
	typedef std::vector<uint32_t> reserveAttachmentsVector;
	typedef std::unique_ptr<const reserveAttachmentsVector> reserveAttachmentsVectorPt;
	std::vector<reserveAttachmentsVectorPt> reserveAttachmentsCopies;
};
