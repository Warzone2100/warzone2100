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
#include <memory>
#include <string>

#include "lib/netplay/wz_connection_provider.h"

namespace tcp
{

class TCPConnectionProvider final : public WzConnectionProvider
{
public:

	virtual void initialize() override;
	virtual void shutdown() override;

	virtual net::result<std::unique_ptr<IConnectionAddress>> resolveHost(const char* host, uint16_t port) override;

	virtual net::result<IListenSocket*> openListenSocket(uint16_t port) override;

	virtual net::result<IClientConnection*> openClientConnectionAny(const IConnectionAddress& addr, unsigned timeout) override;

	virtual IConnectionPollGroup* newConnectionPollGroup() override;

	virtual std::unique_ptr<IDescriptorSet> newDescriptorSet(PollEventType eventType) override;
};

} // namespace tcp
