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

#include "zstd_compression_adapter.h"

#ifdef WZ_ZSTD_COMPRESSION_ADAPTER_ENABLED

#if ZSTD_VERSION_NUMBER < 10400
#  error "WZ_ENABLE_ZSTD_COMPRESSION_ADAPTER requires zstd >= 1.4.0"
#endif

#include "lib/framework/frame.h"

#include "lib/netplay/error_categories.h"

#include <cstring>

ZstdCompressionAdapter::ZstdCompressionAdapter()
{
	std::memset(&compressInBuf_, 0, sizeof(compressInBuf_));
	std::memset(&compressOutBuf_, 0, sizeof(compressOutBuf_));

	std::memset(&decompressInBuf_, 0, sizeof(decompressInBuf_));
	std::memset(&decompressOutBuf_, 0, sizeof(decompressOutBuf_));
}

ZstdCompressionAdapter::~ZstdCompressionAdapter()
{
	if (compressCtx_)
	{
		ZSTD_freeCCtx(compressCtx_);
	}
	if (decompressCtx_)
	{
		ZSTD_freeDCtx(decompressCtx_);
	}
}

net::result<void> ZstdCompressionAdapter::initialize()
{
	// zstd's default compression level (currently 3); comparable to zlib's
	// default in throughput/ratio, and should be considerably faster than zlib's level 6.
	constexpr int DEFAULT_COMPRESSION_LEVEL = ZSTD_CLEVEL_DEFAULT;

	compressCtx_ = ZSTD_createCCtx();
	if (!compressCtx_)
	{
		debug(LOG_ERROR, "Failed to create ZSTD compression context");
		return tl::make_unexpected(make_network_error_code(EBADF));
	}
	const size_t setLevelRes = ZSTD_CCtx_setParameter(compressCtx_, ZSTD_c_compressionLevel, DEFAULT_COMPRESSION_LEVEL);
	if (ZSTD_isError(setLevelRes))
	{
		debug(LOG_ERROR, "ZSTD_CCtx_setParameter(compressionLevel) failed: %s", ZSTD_getErrorName(setLevelRes));
		ZSTD_freeCCtx(compressCtx_);
		compressCtx_ = nullptr;
		return tl::make_unexpected(make_zstd_error_code(static_cast<int>(ZSTD_getErrorCode(setLevelRes))));
	}

	decompressCtx_ = ZSTD_createDCtx();
	if (!decompressCtx_)
	{
		ZSTD_freeCCtx(compressCtx_);
		compressCtx_ = nullptr;
		debug(LOG_ERROR, "Failed to create ZSTD decompression context");
		return tl::make_unexpected(make_network_error_code(EBADF));
	}

	inflateNeedInput_ = true;

	return {};
}

void ZstdCompressionAdapter::resetCompressionStreamInput(const void* src, size_t size)
{
	compressInBuf_.src = src;
	compressInBuf_.size = size;
	compressInBuf_.pos = 0;
}

void ZstdCompressionAdapter::resetDecompressionStreamOutput(void* dst, size_t size)
{
	decompressOutBuf_.dst = dst;
	decompressOutBuf_.size = size;
	decompressOutBuf_.pos = 0;
}

net::result<void> ZstdCompressionAdapter::compress(const void* src, size_t size)
{
	resetCompressionStreamInput(src, size);

	// Call ZSTD_compressStream2 in a loop until all input has been consumed.
	// Each iteration grows `compressOutBufVec_` to provide output space, then
	// trims it back to the actual number of bytes written.
	do
	{
		const size_t alreadyHave = compressOutBufVec_.size();
		compressOutBufVec_.resize(alreadyHave + size + 32); // A bit more than `size` should be enough to do everything in one go in most cases.
		compressOutBuf_.dst = compressOutBufVec_.data();
		compressOutBuf_.size = compressOutBufVec_.size();
		compressOutBuf_.pos = alreadyHave;

		const size_t res = ZSTD_compressStream2(compressCtx_, &compressOutBuf_, &compressInBuf_, ZSTD_e_continue);
		if (ZSTD_isError(res))
		{
			debug(LOG_ERROR, "ZSTD_compressStream2 failed: %s", ZSTD_getErrorName(res));
			compressOutBufVec_.resize(compressOutBuf_.pos);
			return tl::make_unexpected(make_zstd_error_code(static_cast<int>(ZSTD_getErrorCode(res))));
		}
		compressOutBufVec_.resize(compressOutBuf_.pos);
	} while (compressInBuf_.pos < compressInBuf_.size);

	return {};
}

net::result<void> ZstdCompressionAdapter::flushCompressionStream()
{
	// No new input is being fed. Just flushing any buffered data
	// out of zstd's internal state into `compressOutBufVec_`.
	compressInBuf_.src = nullptr;
	compressInBuf_.size = 0;
	compressInBuf_.pos = 0;

	// ZSTD_compressStream2 returns the number of bytes still left to flush.
	// Loop until it returns 0 (everything flushed).
	size_t remaining = 0;
	do
	{
		const size_t alreadyHave = compressOutBufVec_.size();
		compressOutBufVec_.resize(alreadyHave + 1024); // 1KB per pass should typically suffice to drain the flush in one go.
		compressOutBuf_.dst = compressOutBufVec_.data();
		compressOutBuf_.size = compressOutBufVec_.size();
		compressOutBuf_.pos = alreadyHave;

		remaining = ZSTD_compressStream2(compressCtx_, &compressOutBuf_, &compressInBuf_, ZSTD_e_flush);
		if (ZSTD_isError(remaining))
		{
			debug(LOG_ERROR, "ZSTD_compressStream2 flush failed: %s", ZSTD_getErrorName(remaining));
			compressOutBufVec_.resize(compressOutBuf_.pos);
			return tl::make_unexpected(make_zstd_error_code(static_cast<int>(ZSTD_getErrorCode(remaining))));
		}
		compressOutBufVec_.resize(compressOutBuf_.pos);
	} while (remaining != 0);

	return {};
}

net::result<void> ZstdCompressionAdapter::decompress(void* dst, size_t size)
{
	resetDecompressionStreamOutput(dst, size);

	const size_t res = ZSTD_decompressStream(decompressCtx_, &decompressOutBuf_, &decompressInBuf_);
	if (ZSTD_isError(res))
	{
		debug(LOG_ERROR, "ZSTD_decompressStream failed: %s", ZSTD_getErrorName(res));
		return tl::make_unexpected(make_zstd_error_code(static_cast<int>(ZSTD_getErrorCode(res))));
	}
	return {};
}

size_t ZstdCompressionAdapter::availableSpaceToDecompress() const
{
	return decompressOutBuf_.size - decompressOutBuf_.pos;
}

bool ZstdCompressionAdapter::decompressionStreamConsumedAllInput() const
{
	return decompressInBuf_.pos == decompressInBuf_.size;
}

void ZstdCompressionAdapter::resetDecompressionStreamInputSize(size_t size)
{
	decompressInBuf_.src = decompressInBufVec_.data();
	decompressInBuf_.size = size;
	decompressInBuf_.pos = 0;
}

#endif // WZ_ZSTD_COMPRESSION_ADAPTER_ENABLED
