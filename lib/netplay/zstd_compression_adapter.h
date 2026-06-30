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

#pragma once

#include "lib/framework/wzglobal.h" // for WZ_ZSTD_COMPRESSION_ADAPTER_ENABLED

#ifdef WZ_ZSTD_COMPRESSION_ADAPTER_ENABLED

#include "compression_adapter.h"

#include "lib/netplay/net_result.h"

#include <stdint.h>
#include <vector>

#include <zstd.h>

/// <summary>
/// Implementation of `ICompressionAdapter` interface, which uses the
/// Zstd library to compress/decompress the data.
/// </summary>
class ZstdCompressionAdapter : public ICompressionAdapter
{
public:

	explicit ZstdCompressionAdapter();
	virtual ~ZstdCompressionAdapter() override;

	virtual net::result<void> initialize() override;

	virtual net::result<void> compress(const void* src, size_t size) override;
	virtual net::result<void> flushCompressionStream() override;

	virtual std::vector<uint8_t>& compressionOutBuffer() override
	{
		return compressOutBufVec_;
	}

	virtual const std::vector<uint8_t>& compressionOutBuffer() const override
	{
		return compressOutBufVec_;
	}

	virtual net::result<void> decompress(void* dst, size_t size) override;

	virtual std::vector<uint8_t>& decompressionInBuffer() override
	{
		return decompressInBufVec_;
	}

	virtual const std::vector<uint8_t>& decompressionInBuffer() const override
	{
		return decompressInBufVec_;
	}

	virtual size_t availableSpaceToDecompress() const override;
	virtual bool decompressionStreamConsumedAllInput() const override;
	virtual bool decompressionNeedInput() const override
	{
		return inflateNeedInput_;
	}
	virtual void setDecompressionNeedInput(bool needInput) override
	{
		inflateNeedInput_ = needInput;
	}

	virtual void resetDecompressionStreamInputSize(size_t size) override;

private:

	void resetCompressionStreamInput(const void* src, size_t size);
	void resetDecompressionStreamOutput(void* dst, size_t size);

	std::vector<uint8_t> compressOutBufVec_;
	std::vector<uint8_t> decompressInBufVec_;

	ZSTD_inBuffer compressInBuf_;
	ZSTD_outBuffer compressOutBuf_;

	ZSTD_inBuffer decompressInBuf_;
	ZSTD_outBuffer decompressOutBuf_;

	ZSTD_CCtx* compressCtx_ = nullptr;
	ZSTD_DCtx* decompressCtx_ = nullptr;

	bool inflateNeedInput_ = false;
};

#endif // WZ_ZSTD_COMPRESSION_ADAPTER_ENABLED
