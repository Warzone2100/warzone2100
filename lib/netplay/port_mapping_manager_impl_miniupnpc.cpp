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
/**
 * @file port_mapping_manager_impl_miniupnpc.cpp
 * miniupnpc-based port mapping implementation
 */

#include "lib/framework/wzapp.h"
#include "lib/framework/debug.h"

#include "port_mapping_manager_impl_miniupnpc.h"

#if (defined(__GNUC__) || defined(__clang__)) && !defined(__INTEL_COMPILER)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wpedantic"
#endif
#include <miniupnpc.h>
#if (defined(__GNUC__) || defined(__clang__)) && !defined(__INTEL_COMPILER)
# pragma GCC diagnostic pop
#endif
#include <upnpcommands.h>

// Enforce minimum MINIUPNPC_API_VERSION
#if !defined(MINIUPNPC_API_VERSION) || (MINIUPNPC_API_VERSION < 9)
	#error lib/netplay requires MINIUPNPC_API_VERSION >= 9
#endif

void PortMappingImpl_MappingCallbackSuccess(PortMappingImpl::Type type, int mappingId, const std::string& externalHost, uint16_t externalPort);
void PortMappingImpl_MappingCallbackFailure(PortMappingImpl::Type type, int mappingId);

// MARK: - Minupnpc implementation

void _MiniupnpcLogCallback(int line, code_part part, const char *function, const char* message)
{
	std::string msg = (message) ? message : "";
	wzAsyncExecOnMainThread([line, part, function, msg]() {
		_debug(line, part, function, "%s", msg.c_str());
	});
}
#define MiniupnpcLogCallback(part, msg) do { if (enabled_debug[part]) _MiniupnpcLogCallback(__LINE__, part, __FUNCTION__, msg); } while(0)

PortMappingImpl_Miniupnpc::~PortMappingImpl_Miniupnpc()
{
	try
	{
		shutdown();
	}
	catch(...) // Don't let any exceptions escape the dtor.
	{}
}

PortMappingImpl::Type PortMappingImpl_Miniupnpc::get_impl_type() const
{
	return PortMappingImpl::Type::Miniupnpc;
}

int PortMappingImpl_Miniupnpc::get_discovery_timeout_seconds() const
{
	return 20; // Slightly longer timeout for Miniupnpc
}

const char* WZ_UPNP_GetValidIGD_GetErrStr(int result)
{
	switch (result)
	{
		case -1:
			return "Internal error";
		case 0:
			return "No IGD found";
		case 1:
			return "A valid connected IGD has been found";
#if defined(MINIUPNPC_API_VERSION) && (MINIUPNPC_API_VERSION >= 18)
		case 2:
			return "A valid connected IGD has been found but its IP address is reserved (non routable)";
		case 3:
			return "A valid IGD has been found but it reported as not connected";
		case 4:
			return "A UPnP device has been found but was not recognized as an IGD";
#else
		case 2:
			return "A valid IGD has been found but it reported as not connected";
		case 3:
			return "A UPnP device has been found but was not recognized as an IGD";
#endif
		default:
			return "Unknown error";
	}
}

struct DiscoveryResults
{
	struct UPNPUrls urls;
	struct IGDdatas data;
	char lanaddr[64] = {};
};

// This function is run in its own thread! Do not call any non-threadsafe functions!
static PortMappingImpl_Miniupnpc::DiscoveryStatus upnp_discover(DiscoveryResults& output)
{
	char buf[512] = {'\0'};
	struct UPNPDev *devlist;
	int result = 0;
	memset(&output.urls, 0, sizeof(struct UPNPUrls));
	memset(&output.data, 0, sizeof(struct IGDdatas));

	MiniupnpcLogCallback(LOG_NET, "Searching for UPnP devices for automatic port forwarding...");
#if defined(MINIUPNPC_API_VERSION) && (MINIUPNPC_API_VERSION >= 14)
	devlist = upnpDiscover(3000, nullptr, nullptr, 0, 0, 2, &result);
#else
	devlist = upnpDiscover(3000, nullptr, nullptr, 0, 0, &result);
#endif
	MiniupnpcLogCallback(LOG_NET, "UPnP device search finished");

	if (!devlist)
	{
		return PortMappingImpl_Miniupnpc::DiscoveryStatus::UPNP_ERROR_DEVICE_NOT_FOUND;
	}

#if defined(MINIUPNPC_API_VERSION) && (MINIUPNPC_API_VERSION >= 18)
	char wanaddr[64] = {};
	int validIGDResult = UPNP_GetValidIGD(devlist, &output.urls, &output.data, output.lanaddr, sizeof(output.lanaddr), wanaddr, sizeof(wanaddr));
#else
	int validIGDResult = UPNP_GetValidIGD(devlist, &output.urls, &output.data, output.lanaddr, sizeof(output.lanaddr));
#endif
	freeUPNPDevlist(devlist);

	if (validIGDResult == 1)
	{
		ssprintf(buf, "UPnP IGD device found: %s (LAN address %s)", output.urls.controlURL, output.lanaddr);
		MiniupnpcLogCallback(LOG_NET, buf);
	}
#if defined(MINIUPNPC_API_VERSION) && (MINIUPNPC_API_VERSION >= 18)
	else if (validIGDResult == 2)
	{
		ssprintf(buf, "UPnP found an IGD with a reserved IP address (%s): %s\n", wanaddr, output.urls.controlURL);
		MiniupnpcLogCallback(LOG_NET, buf);
	}
#endif
	else
	{
		ssprintf(buf, "UPNP_GetValidIGD failed: (%d): %s", validIGDResult, WZ_UPNP_GetValidIGD_GetErrStr(validIGDResult));
		MiniupnpcLogCallback(LOG_NET, buf);
		return PortMappingImpl_Miniupnpc::DiscoveryStatus::UPNP_ERROR_DEVICE_NOT_FOUND;
	}

	if (!output.urls.controlURL || output.urls.controlURL[0] == '\0')
	{
		return PortMappingImpl_Miniupnpc::DiscoveryStatus::UPNP_ERROR_CONTROL_NOT_AVAILABLE;
	}
	else
	{
		return PortMappingImpl_Miniupnpc::DiscoveryStatus::UPNP_SUCCESS;
	}
}

static bool upnp_remove_redirect(int mappingId, const DiscoveryResults& discovery, uint16_t port)
{
	char port_str[16];
	char buf[512] = {'\0'};

	ssprintf(buf, "upnp_remove_redirect(%" PRIu16 ")", port);
	MiniupnpcLogCallback(LOG_NET, buf);

	ssprintf(port_str, "%" PRIu16, port);
	auto result = UPNP_DeletePortMapping(discovery.urls.controlURL, discovery.data.first.servicetype, port_str, "TCP", nullptr);
	if (result != 0)
	{
		ssprintf(buf, "upnp_remove_redirect(%" PRIu16 ") failed with error: %d", port, result);
		MiniupnpcLogCallback(LOG_NET, buf);
	}
	return result == 0;
}

struct upnp_map_output
{
	bool success = false;
	std::string externalHost;
	uint16_t externalPort = 0;
};

static upnp_map_output upnp_add_redirect(int mappingId, const DiscoveryResults& discovery, uint16_t port)
{
	char port_str[16];
	char buf[512] = {'\0'};
	int r;
	char externalIPAddress[40] = {};

	ssprintf(buf, "upnp_add_redirect(%" PRIu16 ")", port);
	MiniupnpcLogCallback(LOG_NET, buf);
	ssprintf(port_str, "%" PRIu16, port);
	r = UPNP_AddPortMapping(discovery.urls.controlURL, discovery.data.first.servicetype,
							port_str, port_str, discovery.lanaddr, "Warzone 2100", "TCP", nullptr, "0");	// "0" = lease time unlimited
	if (r != UPNPCOMMAND_SUCCESS)
	{
		ssprintf(buf, "Could not open required port (%s) on (%s)", port_str, discovery.lanaddr);
		MiniupnpcLogCallback(LOG_NET, buf);

		// Failure
		upnp_map_output result;
		result.success = false;
		return result;
	}

	r = UPNP_GetExternalIPAddress(discovery.urls.controlURL, discovery.data.first.servicetype, externalIPAddress);
	if (r != UPNPCOMMAND_SUCCESS)
	{
		// Non-fatal error
		ssprintf(buf, "Opened required port (%s), but failed externalIPAddress discovery - proceeding anyway", port_str);
		MiniupnpcLogCallback(LOG_NET, buf);
		// Zero-out externalIpAddress
		externalIPAddress[0] = '\0';
	}

	upnp_map_output result;
	result.success = true;
	result.externalHost = externalIPAddress;
	result.externalPort = port;
	return result;
}

void PortMappingImpl_Miniupnpc::miniupnpc_background_thread(std::shared_ptr<PortMappingImpl> pInstanceBase)
{
	auto pInstance = std::dynamic_pointer_cast<PortMappingImpl_Miniupnpc>(pInstanceBase);

	auto doDiscovery = [pInstance](DiscoveryResults& output) -> DiscoveryStatus {
		DiscoveryStatus status = upnp_discover(output);

		// Only once that's done do we handle mutex-protected data
		switch (status)
		{
			case DiscoveryStatus::UPNP_ERROR_CONTROL_NOT_AVAILABLE:
			case DiscoveryStatus::UPNP_ERROR_DEVICE_NOT_FOUND:
			{
				if (status == DiscoveryStatus::UPNP_ERROR_DEVICE_NOT_FOUND)
				{
					MiniupnpcLogCallback(LOG_NET, "UPnP device not found");
				}
				else if (status == DiscoveryStatus::UPNP_ERROR_CONTROL_NOT_AVAILABLE)
				{
					MiniupnpcLogCallback(LOG_NET, "controlURL not available, UPnP disabled");
				}
				std::vector<int> failedMappingIds;
				{
					const std::lock_guard<std::mutex> guard {pInstance->mappings_mtx_};
					// Store failure status
					pInstance->status_ = status;
					// Fail any pending requests
					for (size_t i = 0; i < pInstance->mappings_.size(); ++i)
					{
						if (pInstance->mappings_[i].state == WzMiniupnpc_State::Destroyed)
						{
							continue;
						}
						pInstance->mappings_[i].state = WzMiniupnpc_State::Destroyed;
						failedMappingIds.push_back(i);
					}
				}
				for (auto mappingId : failedMappingIds)
				{
					PortMappingImpl_MappingCallbackFailure(PortMappingImpl::Type::Miniupnpc, mappingId);
				}
				// exit background thread
				return status;
			}
			case DiscoveryStatus::UPNP_SUCCESS:
				// continue to process pending and new mapping requests
				break;
		}

		// Store discovery success status
		{
			const std::lock_guard<std::mutex> guard {pInstance->mappings_mtx_};
			pInstance->status_ = status;
		}

		return status;
	};

	// Call upnp_discover
	std::shared_ptr<DiscoveryResults> discovery = std::make_shared<DiscoveryResults>();
	DiscoveryStatus status = doDiscovery(*discovery);
	if (status != DiscoveryStatus::UPNP_SUCCESS)
	{
		// exit background thread
		return;
	}

	bool shouldUpdateDiscovery = false;
	while (true)
	{
		wzSemaphoreWait(pInstance->threadSemaphore); // wait until main thread sends tasks to do

		if (shouldUpdateDiscovery)
		{
			status = doDiscovery(*discovery);
			if (status != DiscoveryStatus::UPNP_SUCCESS)
			{
				// exit background thread
				return;
			}
			shouldUpdateDiscovery = false;
		}

		size_t i = 0;
		size_t anyMappings = false;
		while (true)
		{
			auto mappingId = i;
			MappingInfo mapInfo;

			{
				const std::lock_guard<std::mutex> guard {pInstance->mappings_mtx_};

				if (pInstance->stopRequested_)
				{
					return;
				}

				if (i >= pInstance->mappings_.size())
				{
					break;
				}

				if (pInstance->mappings_[i].state == WzMiniupnpc_State::Destroyed)
				{
					++i;
					continue;
				}

				mapInfo = pInstance->mappings_[i];
			}

			switch (mapInfo.state)
			{
				case WzMiniupnpc_State::Pending:
				{
					anyMappings = true;
					auto result = upnp_add_redirect(mappingId, *discovery, mapInfo.port);
					bool doCallbacks = false;
					{
						const std::lock_guard<std::mutex> guard {pInstance->mappings_mtx_};
						if (pInstance->mappings_[i].state != WzMiniupnpc_State::Destroying)
						{
							if (result.success)
							{
								pInstance->mappings_[i].state = WzMiniupnpc_State::Success;
							}
							else
							{
								pInstance->mappings_[i].state = WzMiniupnpc_State::Failure;
							}
							doCallbacks = true;
						}
					}
					if (doCallbacks)
					{
						if (result.success)
						{
							PortMappingImpl_MappingCallbackSuccess(PortMappingImpl::Type::Miniupnpc, mappingId, result.externalHost, result.externalPort);
						}
						else
						{
							PortMappingImpl_MappingCallbackFailure(PortMappingImpl::Type::Miniupnpc, mappingId);
						}
					}
					break;
				}
				case WzMiniupnpc_State::Destroying:
					upnp_remove_redirect(mappingId, *discovery, mapInfo.port);
					{
						const std::lock_guard<std::mutex> guard {pInstance->mappings_mtx_};
						pInstance->mappings_[i].reset(); // sets state to WzMiniupnpc_State::Destroyed
					}
					break;
				case WzMiniupnpc_State::Success:
				case WzMiniupnpc_State::Failure:
					// do nothing
					anyMappings = true;
					break;
				default:
					// no-op
					break;
			}

			++i;
		}

		shouldUpdateDiscovery = !anyMappings; // queue a re-discovery next time a mapping is requested, if all existing mappings have been destroyed
	}
}

bool PortMappingImpl_Miniupnpc::init()
{
	if (m_isInit)
	{
		return true;
	}
	stopRequested_ = false;
	threadSemaphore = wzSemaphoreCreate(0);
	thread_ = std::thread(PortMappingImpl_Miniupnpc::miniupnpc_background_thread, shared_from_this());
	m_isInit = true;
	return true;
}

bool PortMappingImpl_Miniupnpc::shutdown()
{
	if (!m_isInit)
	{
		return false;
	}

	{
		const std::lock_guard<std::mutex> guard {mappings_mtx_};
		stopRequested_ = true;
	}
	wzSemaphorePost(threadSemaphore);  // Wake up the thread, so it can quit.
	if (thread_.joinable())
	{
		thread_.join();
	}
	wzSemaphoreDestroy(threadSemaphore);
	threadSemaphore = nullptr;
	m_isInit = false;
	return true;
}

int PortMappingImpl_Miniupnpc::create_port_mapping(uint16_t port, PortMappingInternetProtocol protocol)
{
	if (protocol != PortMappingInternetProtocol::TCP_IPV4)
	{
		// currently only supports IPv4
		return -1;
	}

	size_t mappingId = 0;
	{
		const std::lock_guard<std::mutex> guard {mappings_mtx_};

		if (status_.has_value() && status_.value() != DiscoveryStatus::UPNP_SUCCESS)
		{
			// UPnP not supported
			return -1;
		}

		// Find first available mapping id
		for (size_t i = 0; i <= mappings_.size(); ++i)
		{
			if (i == mappings_.size())
			{
				// no available mapping id
				return -1;
			}
			if (mappings_[i].state == WzMiniupnpc_State::Destroyed)
			{
				mappingId = i;
				break;
			}
		}

		mappings_[mappingId].protocol = protocol;
		mappings_[mappingId].port = port;
		mappings_[mappingId].state = WzMiniupnpc_State::Pending;
	}
	wzSemaphorePost(threadSemaphore);

	return static_cast<int>(mappingId);
}

bool PortMappingImpl_Miniupnpc::destroy_port_mapping(int mappingId)
{
	{
		const std::lock_guard<std::mutex> guard {mappings_mtx_};
		if (mappingId < 0 || static_cast<size_t>(mappingId) >= mappings_.size())
		{
			return false;
		}

		if (mappings_[mappingId].state == WzMiniupnpc_State::Destroyed)
		{
			return true;
		}

		mappings_[mappingId].state = WzMiniupnpc_State::Destroying;
	}
	wzSemaphorePost(threadSemaphore);
	return true;
}
