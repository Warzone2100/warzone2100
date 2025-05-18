// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2025  Warzone 2100 Project

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

#include "wz_compression_provider.h"

#include "lib/framework/frame.h" // for ASSERT

#include "lib/netplay/zlib_compression_adapter.h"
#ifdef WZ_ZSTD_COMPRESSION_ADAPTER_ENABLED
# include "lib/netplay/zstd_compression_adapter.h"
#endif

WzCompressionProvider& WzCompressionProvider::Instance()
{
	static WzCompressionProvider instance;
	return instance;
}

std::unique_ptr<ICompressionAdapter> WzCompressionProvider::newCompressionAdapter(CompressionAdapterType t)
{
	switch (t)
	{
		case CompressionAdapterType::Zlib:
			return std::make_unique<ZlibCompressionAdapter>();
#ifdef WZ_ZSTD_COMPRESSION_ADAPTER_ENABLED
		case CompressionAdapterType::Zstd:
			return std::make_unique<ZstdCompressionAdapter>();
#endif
		default:
			ASSERT(false, "Invalid compression adapter type %d", static_cast<int>(t));
	}
	return nullptr;
}
