/*
	This file is part of Warzone 2100.
	Copyright (C) 2024  Warzone 2100 Project

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

#include <string>
#include <stddef.h>

#include "lib/framework/types.h" // bring in `ssize_t` for MSVC
#include "lib/netplay/net_result.h"

/// <summary>
/// Basic abstraction over client connection sockets.
///
/// These are capable of reading (`readAll` and `readNoInt`) and
/// writing data (via `writeAll()` + `flush()` combination).
///
/// The internal implementation may also implement advanced compression mechanisms
/// on top of these connections by providing non-trivial `enableCompression()` overload.
///
///	In this case, `writeAll()` should somehow accumulate the data into a write queue,
/// compressing the outcoming data on-the-fly; and `flush()` should empty the write queue
/// and actually post a message to the transmission queue, which, in turn, will be emptied
/// by the internal connection interface in a timely manner, when there are enough messages
/// to be sent over the network.
/// </summary>
class IClientConnection
{
public:

	virtual ~IClientConnection() = default;

	/// <summary>
	/// Read exactly `size` bytes into `buf` buffer.
	/// Supports setting a timeout value in milliseconds.
	/// </summary>
	/// <param name="buf">Destination buffer to read the data into.</param>
	/// <param name="size">The size of data to be read in bytes.</param>
	/// <param name="timeout">Timeout value in milliseconds.</param>
	/// <returns>On success, returns the number of bytes read;
	/// On failure, returns an `std::error_code` (having `GenericSystemErrorCategory` error category)
	/// describing the actual error.</returns>
	virtual net::result<ssize_t> readAll(void* buf, size_t size, unsigned timeout) = 0;
	/// <summary>
	/// Reads at most `max_size` bytes into `buf` buffer.
	/// Raw count of bytes (after compression) is returned in `rawByteCount`.
	/// </summary>
	/// <param name="buf">Destination buffer to read the data into.</param>
	/// <param name="max_size">The maximum number of bytes to read from the client socket.</param>
	/// <param name="rawByteCount">Output parameter: Raw count of bytes (after compression).</param>
	/// <returns>On success, returns the number of bytes read;
	/// On failure, returns an `std::error_code` (having `GenericSystemErrorCategory` error category)
	/// describing the actual error.</returns>
	virtual net::result<ssize_t> readNoInt(void* buf, size_t max_size, size_t* rawByteCount) = 0;
	/// <summary>
	/// Nonblocking write of `size` bytes to the socket. The data will be written to a
	/// separate write queue in asynchronous manner, possibly by a separate thread.
	/// Raw count of bytes (after compression) will be returned in `rawByteCount`, which
	/// will often be 0 until the socket is flushed.
	///
	/// The reason for this method to be async is that in some cases we want
	/// client connections to have compression mechanism enabled. This naturally
	/// introduces the 2-phase write process, which involves a write queue (accumulating
	/// the data for compression on-the-fly) and a submission (transmission)
	/// queue (for transmitting of compressed and assembled messages),
	/// which is managed by the network backend implementation.
	/// </summary>
	/// <param name="buf">Source buffer to read the data from.</param>
	/// <param name="size">The number of bytes to write to the socket.</param>
	/// <param name="rawByteCount">Output parameter: raw count of bytes (after compression) written.</param>
	/// <returns>The total number of bytes written.</returns>
	virtual net::result<ssize_t> writeAll(const void* buf, size_t size, size_t* rawByteCount) = 0;
	/// <summary>
	/// This method indicates whether the socket has some data ready to be read (i.e.
	/// whether the next `readAll/readNoInt` operation will execute without blocking or not).
	/// </summary>
	virtual bool readReady() const = 0;
	/// <summary>
	/// Actually sends the data written with `writeAll()`. Only useful with sockets
	/// which have compression enabled.
	/// Note that flushing too often makes compression less effective.
	/// Raw count of bytes (after compression) is returned in `rawByteCount`.
	/// </summary>
	/// <param name="rawByteCount">Raw count of bytes (after compression) as written
	/// to the submission queue by the flush operation.</param>
	virtual void flush(size_t* rawByteCount) = 0;
	/// <summary>
	/// Enables compression for the current socket.
	///
	/// This makes all subsequent write operations asynchronous, plus
	/// the written data will need to be flushed explicitly at some point.
	/// </summary>
	virtual void enableCompression() = 0;
	/// <summary>
	/// Enables or disables the use of Nagle algorithm for the socket.
	///
	/// For direct TCP connections this is equivalent to setting `TCP_NODELAY` to the
	/// appropriate value (i.e.:
	/// `enable == true`  <=> `TCP_NODELAY == false`;
	/// `enable == false` <=> `TCP_NODELAY == true`).
	/// </summary>
	virtual void useNagleAlgorithm(bool enable) = 0;
	/// <summary>
	/// Returns textual representation of the socket's connection address.
	/// </summary>
	virtual std::string textAddress() const = 0;
};
