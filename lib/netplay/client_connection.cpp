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

#include "client_connection.h"

#include "lib/netplay/error_categories.h"
#include "lib/netplay/pending_writes_manager.h"
#include "lib/netplay/polling_util.h"
#include "lib/netplay/wz_connection_provider.h"
#include "lib/netplay/wz_compression_provider.h"

IClientConnection::IClientConnection(WzConnectionProvider& connProvider, WzCompressionProvider& compressionProvider, PendingWritesManager& pwm)
	: selfConnList_({ this }),
	connProvider_(connProvider.shared_from_this()),
	compressionProvider_(&compressionProvider),
	pwm_(&pwm),
	readAllDescriptorSet_(connProvider.newDescriptorSet(PollEventType::READABLE))
{}

net::result<ssize_t> IClientConnection::readAll(void* buf, size_t size, unsigned timeout)
{
	ASSERT_OR_RETURN(tl::make_unexpected(make_network_error_code(EINVAL)),
		!isCompressed(), "readAll on compressed sockets not implemented.");

	if (!isValid())
	{
		debug(LOG_ERROR, "IClientConnection::readAll: Invalid socket (%p) (error: EBADF)", static_cast<void*>(this));
		return tl::make_unexpected(make_network_error_code(EBADF));
	}

	size_t received = 0;
	while (received < size)
	{
		// If a timeout is set, wait for that amount of time for data to arrive (or abort)
		if (timeout)
		{
			const auto ret = checkConnectionsReadable(selfConnList_, *readAllDescriptorSet_, std::chrono::milliseconds(timeout));
			if (!ret.has_value())
			{
				const auto msg = ret.error().message();
				debug(LOG_NET, "socket (%p) error: %s.", static_cast<void*>(this), msg.c_str());
				return tl::make_unexpected(ret.error());
			}
			else if (ret.value() == 0)
			{
				debug(LOG_NET, "socket (%p) has timed out.", static_cast<void*>(this));
				return tl::make_unexpected(make_network_error_code(ETIMEDOUT));
			}
			ASSERT(readReady(), "Socket should be ready for reading");
		}
		const auto recvRes = recvImpl(&((char*)buf)[received], size - received);
		setReadReady(false);
		if (!recvRes.has_value())
		{
			const auto ec = recvRes.error();
			if (ec == std::errc::resource_unavailable_try_again || // EWOULDBLOCK
				ec == std::errc::operation_would_block || // EAGAIN
				ec == std::errc::interrupted) // EINTR
			{
				continue;
			}
			return recvRes;
		}
		received += recvRes.value();
	}

	return received;
}

net::result<ssize_t> IClientConnection::readNoInt(void* buf, size_t max_size, size_t* rawByteCount)
{
	if (rawByteCount)
	{
		*rawByteCount = 0;
	}

	if (!isValid())
	{
		debug(LOG_ERROR, "IClientConnection::readNoInt: Invalid socket");
		return tl::make_unexpected(make_network_error_code(EBADF));
	}

	if (isCompressed())
	{
		if (compressionAdapter_->decompressionNeedInput())
		{
			// No input data, read some.
			auto& decompressInBuf = compressionAdapter_->decompressionInBuffer();
			decompressInBuf.resize(max_size + 1000);

			const auto recvRes = recvImpl(reinterpret_cast<char*>(decompressInBuf.data()), decompressInBuf.size());
			if (!recvRes.has_value())
			{
				return recvRes;
			}
			const auto received = recvRes.value();
			compressionAdapter_->resetDecompressionStreamInputSize(received);
			if (rawByteCount)
			{
				*rawByteCount = received;
			}
			compressionAdapter_->setDecompressionNeedInput(false);
		}

		const auto decompressRes = compressionAdapter_->decompress(buf, max_size);
		if (!decompressRes.has_value())
		{
			return tl::make_unexpected(decompressRes.error());
		}

		if (compressionAdapter_->availableSpaceToDecompress() != 0)
		{
			compressionAdapter_->setDecompressionNeedInput(true);
			ASSERT(compressionAdapter_->decompressionStreamConsumedAllInput(), "Compression algorithm impl not consuming all input!");
		}

		return max_size - compressionAdapter_->availableSpaceToDecompress();  // Got some data, return how much.
	}

	const auto recvRes = recvImpl(reinterpret_cast<char*>(buf), max_size);
	if (!recvRes.has_value())
	{
		return recvRes;
	}

	setReadReady(false);
	const auto received = recvRes.value();
	if (rawByteCount)
	{
		*rawByteCount = received;
	}

	return received;
}

net::result<ssize_t> IClientConnection::writeAll(const void* buf, size_t size, size_t* rawByteCount)
{
	if (!isValid())
	{
		debug(LOG_ERROR, "IClientConnection::writeAll: Invalid socket (EBADF)");
		return tl::make_unexpected(make_network_error_code(EBADF));
	}

	if (writeErrorCode().has_value())
	{
		return tl::make_unexpected(writeErrorCode().value());
	}

	if (rawByteCount)
	{
		*rawByteCount = 0;
	}

	if (size == 0)
	{
		return 0;
	}

	if (!isCompressed())
	{
		pwm_->append(this, [buf, size](std::vector<uint8_t>& writeQueue)
		{
			writeQueue.insert(writeQueue.end(), static_cast<char const*>(buf), static_cast<char const*>(buf) + size);
		});
		if (rawByteCount)
		{
			*rawByteCount = size;
		}
	}
	else
	{
		auto compressRes = compressionAdapter_->compress(buf, size);
		if (!compressRes.has_value())
		{
			// compress failed?
			debug(LOG_ERROR, "IClientConnection::writeAll: compress failed?");
			return tl::make_unexpected(compressRes.error());
		}
	}

	return size;
}

net::result<void> IClientConnection::flush(size_t* rawByteCount)
{
	if (!isValid())
	{
		debug(LOG_ERROR, "IClientConnection::flush: Invalid socket (EBADF)");
		return tl::make_unexpected(make_network_error_code(EBADF));
	}

	if (rawByteCount)
	{
		*rawByteCount = 0;
	}
	if (!isCompressed())
	{
		return {};  // Not compressed, so don't mess with compression.
	}

	if (writeErrorCode().has_value())
	{
		const auto errMsg = writeErrorCode().value().message();
		debug(LOG_ERROR, "Socket write error encountered in flush: %s", errMsg.c_str());
		return tl::make_unexpected(writeErrorCode().value());
	}

	auto flushCompressionRes = compressionAdapter_->flushCompressionStream();
	if (!flushCompressionRes.has_value())
	{
		// flushCompressionStream failed
		debug(LOG_ERROR, "Socket write error encountered flushing compression stream");
		return tl::make_unexpected(flushCompressionRes.error());
	}

	auto& compressionBuf = compressionAdapter_->compressionOutBuffer();
	if (compressionBuf.empty())
	{
		return {};  // No data to flush out.
	}

	pwm_->append(this, [&compressionBuf] (PendingWritesManager::ConnectionWriteQueue& writeQueue)
	{
		writeQueue.reserve(writeQueue.size() + compressionBuf.size());
		writeQueue.insert(writeQueue.end(), compressionBuf.begin(), compressionBuf.end());
	});
	// Data sent, don't send again.
	if (rawByteCount)
	{
		*rawByteCount = compressionBuf.size();
	}
	compressionBuf.clear();
	return {};
}

void IClientConnection::enableCompression()
{
	if (isCompressed_)
	{
		return;  // Nothing to do.
	}

	ASSERT_OR_RETURN(, compressionProvider_ != nullptr, "Invalid compression provider");

	pwm_->executeUnderLock([this]
	{
		compressionAdapter_ = compressionProvider_->newCompressionAdapter();
		const auto initRes = compressionAdapter_->initialize();
		if (!initRes.has_value())
		{
			const auto errMsg = initRes.error().message();
			debug(LOG_NET, "Failed to initialize compression algorithms. Sockets won't work properly! Detailed error message: %s", errMsg.c_str());
			return;
		}
		isCompressed_ = true;
	});
}

void IClientConnection::close()
{
	pwm_->safeDispose(this);
}
