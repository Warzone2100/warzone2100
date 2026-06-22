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
/** @file vulkan_hpp_include.h
 * Guarded include of Vulkan-Hpp for all lib/ivis_opengl/vk headers.
 */

#pragma once

#if defined(WZ_VULKAN_ENABLED)

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4191) // warning C4191: unsafe conversion in vulkan_structs.hpp debug callback PFN casts
#endif
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wnewline-eof"
#endif
#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ >= 9
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-copy" // Ignore warnings caused by vulkan.hpp 148
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wcast-align"
#endif

#define VULKAN_HPP_TYPESAFE_CONVERSION 1
#include <vulkan/vulkan.hpp>

#if !defined(__clang__) && defined(__GNUC__) && __GNUC__ >= 9
#pragma GCC diagnostic pop
#endif
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#endif // defined(WZ_VULKAN_ENABLED)
