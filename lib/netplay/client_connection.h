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

#include <chrono>
#include <memory>
#include <string>
#include <system_error>
#include <vector>
#include <stddef.h>
#include <stdint.h>

#include "lib/framework/types.h" // bring in `ssize_t` for MSVC
#include "lib/netplay/net_result.h"
#include "lib/netplay/compression_adapter.h"

#include <nonstd/optional.hpp>
using nonstd::optional;

class IDescriptorSet;
class PendingWritesManager;
class WzCompressionProvider;
class WzConnectionProvider;

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

	/// <summary>
	/// Read exactly `size` bytes into `buf` buffer.
	/// Supports setting a timeout value in milliseconds.
	///
	/// This function won't be interrupted by signals(EINTR) and will only
	/// return when <em>exactly</em> @c size bytes have been received.
	/// I.e. this function blocks until all data has been received or a timeout occurred.
	/// </summary>
	/// <param name="buf">Destination buffer to read the data into.</param>
	/// <param name="size">The size of data to be read in bytes.</param>
	/// <param name="timeout">Timeout value in milliseconds.</param>
	/// <returns>On success, returns the number of bytes read;
	/// On failure, returns an `std::error_code` (having `GenericSystemErrorCategory` error category)
	/// describing the actual error.</returns>
	net::result<ssize_t> readAll(void* buf, size_t size, unsigned timeout);
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
	net::result<ssize_t> readNoInt(void* buf, size_t max_size, size_t* rawByteCount);
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
	net::result<ssize_t> writeAll(const void* buf, size_t size, size_t* rawByteCount);

	/// <summary>
	/// Low-level implementation method to send raw data (stored in `data`)
	/// via the underlying transport.
	/// </summary>
	/// <param name="data">The data to send over the network.</param>
	/// <returns>Either the number of bytes sent or `std::error_code` describing the error.</returns>
	virtual net::result<ssize_t> sendImpl(const std::vector<uint8_t>& data) = 0;
	/// <summary>
	/// Low-level implementation method to receive the data into `dst` (up to `maxSize` bytes)
	/// via the underlying transport.
	/// </summary>
	/// <param name="dst">Destination buffer to receive the data from the network.</param>
	/// <param name="maxSize">Maximum byte limit for the underlying `recv` implementation.</param>
	/// <returns>Either the number of bytes received or `std::error_code` describing the error.</returns>
	virtual net::result<ssize_t> recvImpl(char* dst, size_t maxSize) = 0;

	/// <summary>
	/// Set the "read ready" state on the underlying socket, which is used to indicate whether
	/// the socket has some data to be ready without blocking. Usually used in conjunction
	/// with polling, e.g.: `checkConnectionsReadable` will set this flag for all affected connections
	/// if they are ready to read anything right away.
	/// </summary>
	/// <param name="ready"></param>
	virtual void setReadReady(bool ready) = 0;
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
	net::result<void> flush(size_t* rawByteCount);
	/// <summary>
	/// Enables compression for the current socket.
	///
	/// This makes all subsequent write operations asynchronous, plus
	/// the written data will need to be flushed explicitly at some point.
	/// </summary>
	void enableCompression();

	bool isCompressed() const
	{
		return isCompressed_;
	}

	ICompressionAdapter& compressionAdapter()
	{
		return *compressionAdapter_;
	}

	const ICompressionAdapter& compressionAdapter() const
	{
		return *compressionAdapter_;
	}

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

	virtual bool isValid() const = 0;
	/// <summary>
	/// Test whether the given connection is active (internal socket has the valid and open connection).
	/// </summary>
	/// <returns>
	/// Empty result when the connection is open, or contains an appropriate `std::error_code`
	/// when it's closed or in an error state.
	/// </returns>
	virtual net::result<void> connectionStatus() const = 0;

	/// <summary>
	/// Close the connection and dispose of it, i.e. schedules the socket
	/// to be closed and destroyed later (after all the pending data has been sent).
	///
	/// WARNING: do not use the connection object after calling this method!
	/// It will eventually be deleted by `PendingWritesManager` after making sure
	/// all outstanding data is sent.
	/// </summary>
	void close();

	bool deleteLaterRequested() const
	{
		return deleteLater_;
	}

	void requestDeleteLater()
	{
		deleteLater_ = true;
	}

	const optional<std::error_code>& writeErrorCode() const
	{
		return writeErrorCode_;
	}

	void setWriteErrorCode(optional<std::error_code> ec)
	{
		writeErrorCode_ = std::move(ec);
	}

	virtual void setConnectedTimeout(std::chrono::milliseconds timeout) = 0;

protected:

	// Allow `PendingWritesManager` to access hidden destructor
	// since this class ultimately does the final cleanup of
	// disposed connections.
	friend class PendingWritesManager;

	IClientConnection(WzConnectionProvider& connProvider, WzCompressionProvider& compressionProvider, PendingWritesManager& pwm);
	// Hide the destructor so that external code cannot accidentally
	// `delete` the connection directly and has to use `close()` method
	// to dispose of the connection object.
	virtual ~IClientConnection() = default;

	// Pre-allocated (in ctor) connection list and descriptor sets, which
	// only contain `this`.
	//
	// Used for some operations which may use polling internally
	// (like `readAll()` and `connectionStatus()`) to avoid extra
	// memory allocations.
	const std::vector<IClientConnection*> selfConnList_;
	// Connection provider used to create internal descriptor sets.
	std::weak_ptr<WzConnectionProvider> connProvider_;
	// Compression provider which is used to initialize compression algorithm in `enableCompression()`.
	WzCompressionProvider* compressionProvider_ = nullptr;
	// Pending writes manager instance, specific to a particular connection provider,
	// which is used to schedule all write operations for this connection.
	PendingWritesManager* pwm_ = nullptr;

private:

	optional<std::error_code> writeErrorCode_;
	std::unique_ptr<ICompressionAdapter> compressionAdapter_;
	std::unique_ptr<IDescriptorSet> readAllDescriptorSet_;
	bool deleteLater_ = false;
	bool isCompressed_ = false;
};
