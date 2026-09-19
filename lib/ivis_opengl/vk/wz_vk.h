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
/** @file wz_vk.h
 * `WZ_vk` namespace: Vulkan-Hpp dispatch loader alias and unique-handle typedefs.
 */

#pragma once

#if defined(WZ_VULKAN_ENABLED)

#include "vk/vulkan_hpp_include.h"

namespace WZ_vk
{
#if VK_HEADER_VERSION >= 301
using DispatchLoaderDynamic = vk::detail::DispatchLoaderDynamic;
#else
using DispatchLoaderDynamic = vk::DispatchLoaderDynamic;
#endif
using UniqueBuffer = vk::UniqueHandle<vk::Buffer, WZ_vk::DispatchLoaderDynamic>;
using UniqueDeviceMemory = vk::UniqueHandle<vk::DeviceMemory, WZ_vk::DispatchLoaderDynamic>;
using UniqueImage = vk::UniqueHandle<vk::Image, WZ_vk::DispatchLoaderDynamic>;
using UniqueImageView = vk::UniqueHandle<vk::ImageView, WZ_vk::DispatchLoaderDynamic>;
using UniqueSemaphore = vk::UniqueHandle<vk::Semaphore, WZ_vk::DispatchLoaderDynamic>;
} // namespace WZ_vk

#endif // defined(WZ_VULKAN_ENABLED)
