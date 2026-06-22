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
/** @file layout_subresource.cpp
 * Implementation of layout subresource key construction from attachments and reads.
 */

#include "layout_subresource.h"

#include "attachment.h"
#include "render_pass.h"

#include "lib/framework/hash_combine.h"

namespace gfx_api
{

namespace
{

bool attachmentMatchesSubresource(const AttachmentDesc& attachment, const LayoutSubresourceKey& subresource)
{
	return attachment.texture == subresource.texture
		&& attachment.arrayLayer == subresource.arrayLayer
		&& attachment.mipLevel == subresource.mipLevel;
}

} // anonymous namespace

size_t LayoutSubresourceKeyHash::operator()(const LayoutSubresourceKey& key) const noexcept
{
	size_t h = 0;
	hash_combine(h, key.texture, key.arrayLayer, key.mipLevel);
	return h;
}

LayoutSubresourceKey layoutSubresourceKey(abstract_texture* texture, uint32_t arrayLayer, uint32_t mipLevel)
{
	return LayoutSubresourceKey {texture, arrayLayer, mipLevel};
}

LayoutSubresourceKey layoutSubresourceKey(const AttachmentDesc& attachment)
{
	return LayoutSubresourceKey {attachment.texture, attachment.arrayLayer, attachment.mipLevel};
}

LayoutSubresourceKey layoutSubresourceKey(const ResolvedRead& read)
{
	return LayoutSubresourceKey {read.texture, read.arrayLayer, read.mipLevel};
}

bool isPassAttachmentSubresource(const RenderPassDesc& pass, const LayoutSubresourceKey& subresource)
{
	if (subresource.texture == nullptr)
	{
		return false;
	}

	for (const AttachmentDesc& colorAttachment : pass.colorAttachments)
	{
		if (attachmentMatchesSubresource(colorAttachment, subresource))
		{
			return true;
		}
	}

	if (pass.depthAttachment.has_value()
		&& attachmentMatchesSubresource(pass.depthAttachment.value(), subresource))
	{
		return true;
	}

	if (pass.resolveAttachment.has_value()
		&& attachmentMatchesSubresource(pass.resolveAttachment.value(), subresource))
	{
		return true;
	}

	return false;
}

} // namespace gfx_api
