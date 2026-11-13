// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2025-2026  Warzone 2100 Project (https://github.com/Warzone2100)

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

#include "wz_compression_provider.h"

#include "lib/netplay/zlib_compression_adapter.h"

WzCompressionProvider& WzCompressionProvider::Instance()
{
	static WzCompressionProvider instance;
	return instance;
}

std::unique_ptr<ICompressionAdapter> WzCompressionProvider::newCompressionAdapter()
{
	// Only support Zlib for the time being.
	// TODO: in the future, we might want to support more compression algorithms, e.g. Zstd.
	// In this case, we would need to somehow configure `WzCompressionAdapter` from
	// the global WZ config to specify compression algorithm to use.
	return std::make_unique<ZlibCompressionAdapter>();
}
