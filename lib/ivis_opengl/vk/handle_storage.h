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
/** @file handle_storage.h
 * Portable encode/decode of Vulkan opaque handles into uint64_t cache storage.
 */

#pragma once

#if defined(WZ_VULKAN_ENABLED)

#include <cstdint>
#include <cstring>

namespace gfx_api::vk
{

template<typename Handle>
uint64_t encodeHandle(Handle handle)
{
	static_assert(sizeof(Handle) <= sizeof(uint64_t), "Vulkan handle too large for uint64_t storage");
	uint64_t encoded = 0;
	std::memcpy(&encoded, &handle, sizeof(handle));
	return encoded;
}

template<typename Handle>
Handle decodeHandle(uint64_t encoded)
{
	static_assert(sizeof(Handle) <= sizeof(uint64_t), "Vulkan handle too large for uint64_t storage");
	Handle handle {};
	std::memcpy(&handle, &encoded, sizeof(handle));
	return handle;
}

} // namespace gfx_api::vk

#endif // defined(WZ_VULKAN_ENABLED)
