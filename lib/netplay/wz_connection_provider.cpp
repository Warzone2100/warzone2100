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

#include "wz_connection_provider.h"

#include "lib/framework/wzapp.h"

namespace
{

OpenConnectionResult openClientConnectionSyncImpl(const char* host, uint32_t port, std::chrono::milliseconds timeout, std::shared_ptr<WzConnectionProvider> connProvider)
{
	if (!connProvider)
	{
		return OpenConnectionResult(std::make_error_code(std::errc::connection_aborted), "Connection provider no longer valid");
	}
	auto addrResult = connProvider->resolveHost(host, port);
	if (!addrResult.has_value())
	{
		const auto hostsErr = addrResult.error();
		const auto hostsErrMsg = hostsErr.message();
		return OpenConnectionResult(hostsErr, astringf("Cannot resolve host \"%s\": [%d]: %s", host, hostsErr.value(), hostsErrMsg.c_str()));
	}
	auto connRes = connProvider->openClientConnectionAny(*addrResult.value(), timeout.count());
	if (!connRes.has_value())
	{
		const auto connErr = connRes.error();
		const auto connErrMsg = connErr.message();
		return OpenConnectionResult(connErr, astringf("Cannot resolve host \"%s\": [%d]: %s", host, connErr.value(), connErrMsg.c_str()));
	}
	return OpenConnectionResult(connRes.value());
}

struct OpenConnectionRequest
{
	std::string host;
	uint32_t port = 0;
	std::chrono::milliseconds timeout{ 15000 };
	OpenConnectionToHostResultCallback callback;
	std::weak_ptr<WzConnectionProvider> connProvider;
};

int openDirectConnectionAsyncImpl(void* data)
{
	OpenConnectionRequest* pRequestInfo = (OpenConnectionRequest*)data;
	if (!pRequestInfo)
	{
		return 1;
	}
	auto strongConnProvider = pRequestInfo->connProvider.lock();
	pRequestInfo->callback(openClientConnectionSyncImpl(
		pRequestInfo->host.c_str(),
		pRequestInfo->port,
		pRequestInfo->timeout,
		strongConnProvider));
	delete pRequestInfo;
	return 0;
}

} // anonymous namespace

bool WzConnectionProvider::openClientConnectionAsync(const std::string& host, uint32_t port, std::chrono::milliseconds timeout, OpenConnectionToHostResultCallback callback)
{
	// spawn background thread to handle this
	auto pRequest = new OpenConnectionRequest();
	pRequest->host = host;
	pRequest->port = port;
	pRequest->timeout = timeout;
	pRequest->callback = callback;
	pRequest->connProvider = shared_from_this();
	WZ_THREAD* pOpenConnectionThread = wzThreadCreate(openDirectConnectionAsyncImpl, pRequest);
	if (pOpenConnectionThread == nullptr)
	{
		debug(LOG_ERROR, "Failed to create thread for opening connection");
		delete pRequest;
		return false;
	}
	wzThreadDetach(pOpenConnectionThread);
	// the thread handles deleting pRequest
	pOpenConnectionThread = nullptr;
	return true;
}
