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

#include <stdint.h>
#include <type_traits>

class IClientConnection;
class PendingWritesManager;
class WzConnectionProvider;
class WzCompressionProvider;

/// <summary>
/// Server-side listen socket abstraction.
/// </summary>
class IListenSocket
{
public:

	virtual ~IListenSocket() = default;

	enum class IPVersions : uint8_t
	{
		IPV4 = 0b00000001,
		IPV6 = 0b00000010
	};
	using IPVersionsMask = std::underlying_type_t<IPVersions>;

	/// <summary>
	/// Accept an incoming client connection on the current server-side listen socket.
	/// </summary>
	virtual IClientConnection* accept() = 0;
	virtual IPVersionsMask supportedIpVersions() const = 0;

protected:

	IListenSocket(WzConnectionProvider& connProvider, WzCompressionProvider& compressionProvider, PendingWritesManager& pwm);

	WzConnectionProvider* connProvider_ = nullptr;
	WzCompressionProvider* compressionProvider_ = nullptr;
	PendingWritesManager* pwm_ = nullptr;
};
