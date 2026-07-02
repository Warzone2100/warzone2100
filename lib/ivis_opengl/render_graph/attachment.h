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
/** @file attachment.h
 * Attachment load/store operations and `AttachmentDesc` for render pass inputs and outputs.
 */

#pragma once

#include <array>
#include <cstdint>

#include <optional>

namespace gfx_api
{

struct abstract_texture;

/// <summary>
/// Load operation for a render pass attachment
/// </summary>
enum class AttachmentLoadOp
{
	Clear,
	Load,
	DontCare
};

/// <summary>
/// Store operation for a render pass attachment (post-pass contract).
/// </summary>
enum class AttachmentStoreOp : uint8_t
{
	Store,      // keep contents (shadow depth, resolve output)
	DontCare,   // contents not needed after pass (MSAA intermediate color)
	Resolve,    // MSAA color → resolve attachment (VK subpass; GL blit target)
	Invalidate, // tile-friendly discard (scene depth/stencil after scene pass)
};

/// <summary>
/// Clear values for color and depth/stencil attachments.
/// </summary>
struct ClearValue
{
	std::array<float, 4> color = {0.f, 0.f, 0.f, 1.f};
	float depth = 1.f;
	uint32_t stencil = 0;

	static ClearValue colorClear(float r = 0.f, float g = 0.f, float b = 0.f, float a = 1.f)
	{
		ClearValue v;
		v.color = {r, g, b, a};
		return v;
	}

	static ClearValue depthStencilClear(float depthValue = 1.f, uint32_t stencilValue = 0)
	{
		ClearValue v;
		v.depth = depthValue;
		v.stencil = stencilValue;
		return v;
	}
};

/// <summary>
/// Describes a color, depth, or resolve attachment for a render pass.
/// </summary>
struct AttachmentDesc
{
	abstract_texture* texture = nullptr;
	AttachmentLoadOp loadOp = AttachmentLoadOp::Clear;
	/// When unset, resolved during pass resolution.
	std::optional<AttachmentStoreOp> storeOp;
	ClearValue clearValue;
	uint32_t mipLevel = 0;
	uint32_t arrayLayer = 0;

	bool shouldClear() const
	{
		return loadOp == AttachmentLoadOp::Clear;
	}

	static AttachmentDesc color(abstract_texture* tex, AttachmentLoadOp op = AttachmentLoadOp::Clear,
		ClearValue clear = ClearValue::colorClear())
	{
		AttachmentDesc desc;
		desc.texture = tex;
		desc.loadOp = op;
		desc.clearValue = clear;
		return desc;
	}

	static AttachmentDesc depth(abstract_texture* tex, AttachmentLoadOp op = AttachmentLoadOp::Clear,
		ClearValue clear = ClearValue::depthStencilClear())
	{
		AttachmentDesc desc;
		desc.texture = tex;
		desc.loadOp = op;
		desc.clearValue = clear;
		return desc;
	}
};

} // namespace gfx_api
