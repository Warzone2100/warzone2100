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

#include "net_result.h"

#include <stddef.h>
#include <stdint.h>
#include <vector>

/// <summary>
/// Generic facade for integration of various compression algorithms into WZ's
/// networking code.
///
/// Provides very basic facilities to initialize compression/decompression streams
/// and perform compression/decompression tasks.
/// </summary>
class ICompressionAdapter
{
public:

	virtual ~ICompressionAdapter() = default;

	/// <summary>
	/// The first thing that should be called on an `ICompressionAdapter` instance
	/// before doing anything else with it.
	///
	/// Initializes states of internal buffers, streams, etc.
	/// </summary>
	/// <returns>
	/// In case of failure, returns an error code describing the error.
	/// </returns>
	virtual net::result<void> initialize() = 0;

	/// <summary>
	/// Executes the compression routine against `src` buffer of a given size.
	/// The result (compressed buffer) can be later accessed via `compressionOutBuffer()` function.
	///
	/// This function should be used to incrementally compress incoming data.
	///
	/// Note, that it won't necessarily flush all the data to the output buffer,
	/// so one would then need to call `flushCompressionStream()` to flush all
	/// internal streams to the output buffer.
	/// </summary>
	/// <param name="src">Source buffer containing uncompressed data</param>
	/// <param name="size">Size of the source buffer in bytes</param>
	/// <returns>
	/// In case of failure, returns an error code describing the error.
	/// </returns>
	virtual net::result<void> compress(const void* src, size_t size) = 0;
	/// <summary>
	/// Flushes the internal compression stream to the output buffer.
	/// The resulting data can be accessed via `compressionOutBuffer()`
	/// accessor function.
	/// </summary>
	/// <returns>
	/// In case of failure, returns an error code describing the error.
	/// </returns>
	virtual net::result<void> flushCompressionStream() = 0;
	/// <summary>
	/// Accessor function (non-const) for compression output buffer (this will be the
	/// implicit destination to `compress()` and `flushCompressionStream()` functions.
	/// </summary>
	virtual std::vector<uint8_t>& compressionOutBuffer() = 0;
	/// <summary>
	/// Accessor function (const) for compression output buffer (this will be the
	/// implicit destination to `compress()` and `flushCompressionStream()` functions.
	/// </summary>
	virtual const std::vector<uint8_t>& compressionOutBuffer() const = 0;

	/// <summary>
	/// Decompress the data from the compressed buffer into `dst` output buffer
	/// of a given size.
	///
	/// The caller is responsible for ensuring that `dst` is long enough to
	/// hold the decompression result of `size` length in bytes.
	///
	/// One would also need to call `resetDecompressionStreamInput()` function
	/// first to specify the input buffer (containing compressed data) before
	/// calling this function.
	///
	/// </summary>
	/// <param name="dst">Destination buffer, where the decompressed data will go</param>
	/// <param name="size">Size of the destination buffer</param>
	/// <returns>
	/// In case of failure, returns an error code describing the error.
	/// </returns>
	virtual net::result<void> decompress(void* dst, size_t size) = 0;
	/// <summary>
	/// Accessor function (non-const) for decompression input buffer (this will be
	/// the implicit source for `decompress()` function).
	/// </summary>
	virtual std::vector<uint8_t>& decompressionInBuffer() = 0;
	/// <summary>
	/// Accessor function (const) for decompression input buffer (this will be
	/// the implicit source for `decompress()` function).
	/// </summary>
	virtual const std::vector<uint8_t>& decompressionInBuffer() const = 0;
	/// <summary>
	/// Remaining free space (in bytes) at the decompression output buffer.
	/// </summary>
	virtual size_t availableSpaceToDecompress() const = 0;
	/// <summary>
	/// Returns `true` if the internal decompression stream doesn't have any more
	/// available bytes in the input buffer (all input was consumed by a prior
	/// `decompress()` function call).
	/// </summary>
	virtual bool decompressionStreamConsumedAllInput() const = 0;
	/// <summary>
	/// Returns `true` if the decompression algorithm needs to process more input (i.e.
	/// there's more data available to process).
	/// </summary>
	virtual bool decompressionNeedInput() const = 0;
	/// <summary>
	/// Update the decompression algorithm on whether it needs more input or not.
	/// </summary>
	virtual void setDecompressionNeedInput(bool needInput) = 0;
	/// <summary>
	/// Updates the available input bytes count at the decompression input stream.
	/// </summary>
	/// <param name="size">New size for the decompression input stream</param>
	virtual void resetDecompressionStreamInputSize(size_t size) = 0;
};
