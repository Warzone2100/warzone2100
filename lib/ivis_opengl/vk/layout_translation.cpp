// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/
/** @file layout_translation.cpp
 * Implementation of compile-image-layout ↔ Vulkan layout translation.
 */

#if defined(WZ_VULKAN_ENABLED)

#include "vk/layout_translation.h"

namespace gfx_api::vk
{

::vk::ImageLayout toVkImageLayout(CompileImageLayout layout)
{
	switch (layout)
	{
	case CompileImageLayout::Undefined:
		return ::vk::ImageLayout::eUndefined;
	case CompileImageLayout::ShaderReadOnly:
		return ::vk::ImageLayout::eShaderReadOnlyOptimal;
	case CompileImageLayout::DepthReadOnly:
		return ::vk::ImageLayout::eDepthStencilReadOnlyOptimal;
	case CompileImageLayout::ColorAttachment:
		return ::vk::ImageLayout::eColorAttachmentOptimal;
	case CompileImageLayout::DepthAttachment:
		return ::vk::ImageLayout::eDepthStencilAttachmentOptimal;
	case CompileImageLayout::Present:
		return ::vk::ImageLayout::ePresentSrcKHR;
	case CompileImageLayout::Storage:
		return ::vk::ImageLayout::eGeneral;
	}
	return ::vk::ImageLayout::eUndefined;
}

CompileImageLayout fromVkImageLayout(::vk::ImageLayout layout)
{
	switch (layout)
	{
	case ::vk::ImageLayout::eShaderReadOnlyOptimal:
		return CompileImageLayout::ShaderReadOnly;
	case ::vk::ImageLayout::eDepthStencilReadOnlyOptimal:
		return CompileImageLayout::DepthReadOnly;
	case ::vk::ImageLayout::eColorAttachmentOptimal:
		return CompileImageLayout::ColorAttachment;
	case ::vk::ImageLayout::eDepthStencilAttachmentOptimal:
		return CompileImageLayout::DepthAttachment;
	case ::vk::ImageLayout::ePresentSrcKHR:
		return CompileImageLayout::Present;
	case ::vk::ImageLayout::eGeneral:
		return CompileImageLayout::Storage;
	default:
		return CompileImageLayout::Undefined;
	}
}

} // namespace gfx_api::vk

#endif // defined(WZ_VULKAN_ENABLED)
