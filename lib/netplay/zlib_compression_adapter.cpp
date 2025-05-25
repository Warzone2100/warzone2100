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

#include "zlib_compression_adapter.h"
#include "error_categories.h"

#include "lib/framework/frame.h" // for `ASSERT`

#include <cstring>

ZlibCompressionAdapter::ZlibCompressionAdapter()
{
	std::memset(&deflateStream_, 0, sizeof(deflateStream_));
	std::memset(&inflateStream_, 0, sizeof(inflateStream_));
}

ZlibCompressionAdapter::~ZlibCompressionAdapter()
{
	deflateEnd(&deflateStream_);
	deflateEnd(&inflateStream_);
}

net::result<void> ZlibCompressionAdapter::initialize()
{
	// Init deflate stream
	deflateStream_.zalloc = Z_NULL;
	deflateStream_.zfree = Z_NULL;
	deflateStream_.opaque = Z_NULL;
	int ret = deflateInit(&deflateStream_, 6);
	ASSERT(ret == Z_OK, "deflateInit failed! Sockets won't work.");
	if (ret != Z_OK)
	{
		return tl::make_unexpected(make_zlib_error_code(ret));
	}

	// Init inflate stream
	inflateStream_.zalloc = Z_NULL;
	inflateStream_.zfree = Z_NULL;
	inflateStream_.opaque = Z_NULL;
	inflateStream_.avail_in = 0;
	inflateStream_.next_in = Z_NULL;
	ret = inflateInit(&inflateStream_);
	ASSERT(ret == Z_OK, "deflateInit failed! Sockets won't work.");
	if (ret != Z_OK)
	{
		return tl::make_unexpected(make_zlib_error_code(ret));
	}

	inflateNeedInput_ = true;

	return {};
}

void ZlibCompressionAdapter::resetCompressionStreamInput(const void* src, size_t size)
{
#if ZLIB_VERNUM < 0x1252
	// zlib < 1.2.5.2 does not support `#define ZLIB_CONST`
	// Unfortunately, some OSes (ex. OpenBSD) ship with zlib < 1.2.5.2
	// Workaround: cast away the const of the input, and disable the resulting -Wcast-qual warning
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wcast-qual"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wcast-qual"
#endif

	// cast away the const for earlier zlib versions
	deflateStream_.next_in = (Bytef*)src; // -Wcast-qual

#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#endif
#else
	// zlib >= 1.2.5.2 supports ZLIB_CONST
	deflateStream_.next_in = (const Bytef*)src;
#endif

	deflateStream_.avail_in = size;
}

net::result<void> ZlibCompressionAdapter::compress(const void* src, size_t size)
{
	resetCompressionStreamInput(src, size);
	do
	{
		const size_t alreadyHave = deflateOutBuf_.size();
		deflateOutBuf_.resize(alreadyHave + size + 20);  // A bit more than size should be enough to always do everything in one go.
		deflateStream_.next_out = (Bytef*)&deflateOutBuf_[alreadyHave];
		deflateStream_.avail_out = deflateOutBuf_.size() - alreadyHave;

		int ret = deflate(&deflateStream_, Z_NO_FLUSH);
		ASSERT(ret != Z_STREAM_ERROR, "zlib compression failed!");

		// Remove unused part of buffer.
		deflateOutBuf_.resize(deflateOutBuf_.size() - deflateStream_.avail_out);
	} while (deflateStream_.avail_out == 0);

	ASSERT(deflateStream_.avail_in == 0, "zlib didn't compress everything!");

	return {};
}

net::result<void> ZlibCompressionAdapter::flushCompressionStream()
{
	// Flush data out of zlib compression state.
	do
	{
		deflateStream_.next_in = (Bytef*)nullptr;
		deflateStream_.avail_in = 0;
		const size_t alreadyHave = deflateOutBuf_.size();
		deflateOutBuf_.resize(alreadyHave + 1000);  // 100 bytes would probably be enough to flush the rest in one go.
		deflateStream_.next_out = (Bytef*)&deflateOutBuf_[alreadyHave];
		deflateStream_.avail_out = deflateOutBuf_.size() - alreadyHave;

		int ret = deflate(&deflateStream_, Z_PARTIAL_FLUSH);
		ASSERT(ret != Z_STREAM_ERROR, "zlib compression failed!");

		// Remove unused part of buffer.
		deflateOutBuf_.resize(deflateOutBuf_.size() - deflateStream_.avail_out);
	} while (deflateStream_.avail_out == 0);

	return {};
}

net::result<void> ZlibCompressionAdapter::decompress(void* dst, size_t size)
{
	resetDecompressionStreamOutput(dst, size);

	int ret = inflate(&inflateStream_, Z_NO_FLUSH);
	ASSERT(ret != Z_STREAM_ERROR, "zlib inflate not working!");
	char const* err = nullptr;
	switch (ret)
	{
	case Z_NEED_DICT:  err = "Z_NEED_DICT";  break;
	case Z_DATA_ERROR: err = "Z_DATA_ERROR"; break;
	case Z_MEM_ERROR:  err = "Z_MEM_ERROR";  break;
	}
	if (ret == Z_STREAM_ERROR)
	{
		if (err)
		{
			debug(LOG_ERROR, "Couldn't decompress data from socket. zlib error %s", err);
		}
		else
		{
			debug(LOG_ERROR, "Couldn't decompress data from socket. Unknown zlib error");
		}
		return tl::make_unexpected(make_zlib_error_code(ret));
	}
	return {};
}

size_t ZlibCompressionAdapter::availableSpaceToDecompress() const
{
	return inflateStream_.avail_out;
}

bool ZlibCompressionAdapter::decompressionStreamConsumedAllInput() const
{
	return inflateStream_.avail_in == 0;
}

void ZlibCompressionAdapter::resetDecompressionStreamInputSize(size_t size)
{
	inflateStream_.next_in = inflateInBuf_.data();
	inflateStream_.avail_in = size;
}

void ZlibCompressionAdapter::resetDecompressionStreamOutput(void* buf, size_t size)
{
	inflateStream_.next_out = (Bytef*)buf;
	inflateStream_.avail_out = size;
}
