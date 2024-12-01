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

#pragma once

#include "compression_adapter.h"

#if !defined(ZLIB_CONST)
#  define ZLIB_CONST
#endif
#include <zlib.h>

/// <summary>
/// Implementation of `ICompressionAdapter` interface, which uses the
/// Zlib library to compress/decompress the data.
/// </summary>
class ZlibCompressionAdapter : public ICompressionAdapter
{
public:

	explicit ZlibCompressionAdapter();
	virtual ~ZlibCompressionAdapter() override;

	virtual net::result<void> initialize() override;

	virtual net::result<void> compress(const void* src, size_t size) override;
	virtual net::result<void> flushCompressionStream() override;

	virtual std::vector<uint8_t>& compressionOutBuffer() override
	{
		return deflateOutBuf_;
	}

	virtual const std::vector<uint8_t>& compressionOutBuffer() const override
	{
		return deflateOutBuf_;
	}

	virtual net::result<void> decompress(void* dst, size_t size) override;

	virtual std::vector<uint8_t>& decompressionInBuffer() override
	{
		return inflateInBuf_;
	}

	virtual const std::vector<uint8_t>& decompressionInBuffer() const override
	{
		return inflateInBuf_;
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

	std::vector<uint8_t> deflateOutBuf_;
	std::vector<uint8_t> inflateInBuf_;
	z_stream deflateStream_;
	z_stream inflateStream_;
	bool inflateNeedInput_ = false;
};
