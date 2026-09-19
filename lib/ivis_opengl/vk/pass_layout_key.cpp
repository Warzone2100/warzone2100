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
/** @file pass_layout_key.cpp
 * Implementation of `PassLayoutKey` comparison and hashing.
 */

#if defined(WZ_VULKAN_ENABLED)

#include "vk/pass_layout_key.h"

namespace gfx_api::vk
{

bool PassLayoutKey::operator==(const PassLayoutKey& other) const
{
	return colorFormats == other.colorFormats
		&& colorLoadOps == other.colorLoadOps
		&& colorStoreOps == other.colorStoreOps
		&& colorSamples == other.colorSamples
		&& resolveFormat == other.resolveFormat
		&& resolveLoadOp == other.resolveLoadOp
		&& resolveStoreOp == other.resolveStoreOp
		&& depthFormat == other.depthFormat
		&& depthLoadOp == other.depthLoadOp
		&& depthStoreOp == other.depthStoreOp
		&& colorInitialLayouts == other.colorInitialLayouts
		&& resolveInitialLayout == other.resolveInitialLayout
		&& depthInitialLayout == other.depthInitialLayout
		&& depthFinalLayout == other.depthFinalLayout
		&& colorFinalLayouts == other.colorFinalLayouts
		&& resolveFinalLayout == other.resolveFinalLayout;
}

} // namespace gfx_api::vk

#endif // defined(WZ_VULKAN_ENABLED)
