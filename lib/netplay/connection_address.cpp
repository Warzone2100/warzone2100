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

#include "lib/netplay/connection_address.h"
#include "lib/netplay/tcp/netsocket.h" // for `resolveHost`

#include "lib/framework/frame.h" // for `ASSERT`

struct ConnectionAddress::Impl final
{
	explicit Impl(SocketAddress* addr)
		: mAddr_(addr)
	{}

	~Impl()
	{
		ASSERT(mAddr_ != nullptr, "Invalid addrinfo stored in the connection address");
		freeaddrinfo(mAddr_);
	}

	SocketAddress* mAddr_;
};

ConnectionAddress::ConnectionAddress() = default;
ConnectionAddress::ConnectionAddress(ConnectionAddress&&) = default;
ConnectionAddress::~ConnectionAddress() = default;

const SocketAddress* ConnectionAddress::asRawSocketAddress() const
{
	return mPimpl_->mAddr_;
}


net::result<ConnectionAddress> ConnectionAddress::parse(const char* hostname, uint16_t port)
{
	ConnectionAddress res;
	const auto addr = tcp::resolveHost(hostname, port);
	if (!addr.has_value())
	{
		return tl::make_unexpected(addr.error());
	}
	res.mPimpl_ = std::make_unique<Impl>(addr.value());
	return net::result<ConnectionAddress>{std::move(res)};
}

net::result<ConnectionAddress> ConnectionAddress::parse(const std::string& hostname, uint16_t port)
{
	return parse(hostname.c_str(), port);
}
