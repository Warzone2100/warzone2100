// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2025-2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/

#include "polling_util.h"

#include "lib/framework/frame.h" // for ASSERT, debug
#include "lib/netplay/client_connection.h"
#include "lib/netplay/descriptor_set.h"

namespace
{

void resetDescriptorSet(const std::vector<IClientConnection*>& conns, IDescriptorSet& dset)
{
	dset.clear();
	for (const auto& conn : conns)
	{
		ASSERT(dset.add(conn), "Failed to add connection to descriptor set");
	}
}

} // anonymous namespace

net::result<int> checkConnectionsReadable(const std::vector<IClientConnection*>& conns,
	IDescriptorSet& readableSet, std::chrono::milliseconds timeout)
{
	if (conns.empty())
	{
		return 0;
	}

	bool compressedReady = false;
	for (const auto& conn : conns)
	{
		ASSERT(conn->isValid(), "Invalid connection!");

		if (conn->isCompressed() && !conn->compressionAdapter().decompressionNeedInput())
		{
			compressedReady = true;
			break;
		}
	}

	if (compressedReady)
	{
		// A socket already has some data ready. Don't really poll the sockets.
		for (auto& conn : conns)
		{
			conn->setReadReady(conn->isCompressed() && !conn->compressionAdapter().decompressionNeedInput());
		}
		return conns.size();
	}

	resetDescriptorSet(conns, readableSet);
	const auto pollRes = readableSet.poll(timeout);
	if (!pollRes.has_value())
	{
		const auto msg = pollRes.error().message();
		debug(LOG_ERROR, "poll failed: %s", msg.c_str());
		return pollRes;
	}

	if (pollRes.value() == 0)
	{
		debug(LOG_WARNING, "poll timed out after waiting for %u milliseconds", static_cast<unsigned int>(timeout.count()));
		return 0;
	}

	for (auto& conn : conns)
	{
		conn->setReadReady(readableSet.isSet(conn).value_or(false));
	}
	return pollRes.value();
}
