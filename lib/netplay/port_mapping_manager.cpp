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
 * @file port_mapping_manager.cpp
 * Port Mapping Manager singleton for communicating with port mapping library implementation.
 */

#include "lib/framework/wzapp.h"
#include "lib/framework/debug.h"

#include "port_mapping_manager.h"

#include <array>

PortMappingImpl::~PortMappingImpl()
{
	// no-op
}

// MARK: - Libplum implementation

#include <plum/plum.h>

void PlumMappingCallbackSuccess(int mappingId, const std::string& externalHost, uint16_t externalPort)
{
	PortMappingManager::instance().resolve_success_fromimpl(PortMappingImpl::Type::Libplum, mappingId, externalHost, externalPort);
}

void PlumMappingCallbackFailure(int mappingId)
{
	PortMappingManager::instance().resolve_failure_fromimpl(PortMappingImpl::Type::Libplum, mappingId, PortMappingDiscoveryStatus::FAILURE);
}

namespace
{

// This function is run in its own thread managed by LibPLum! Do not call any non-threadsafe functions!
void PlumMappingCallback(int mappingId, plum_state_t state, const plum_mapping_t* mapping)
{
	debug(LOG_NET, "LibPlum cb: Port mapping %d: state=%d\n", mappingId, (int)state);
	switch (state) {
	case PLUM_STATE_SUCCESS:
		debug(LOG_NET, "Mapping %d: success, internal=%hu, external=%s:%hu\n", mappingId, mapping->internal_port,
			mapping->external_host, mapping->external_port);
		PlumMappingCallbackSuccess(mappingId, mapping->external_host, mapping->external_port);
		break;

	case PLUM_STATE_FAILURE:
		debug(LOG_NET, "Mapping %d: failed\n", mappingId);
		PlumMappingCallbackFailure(mappingId);
		break;

	default:
		break;
	}
}

void PlumLogCallback(plum_log_level_t level, const char* message)
{
	code_part wz_log_level = LOG_NET;
	switch (level)
	{
	case PLUM_LOG_LEVEL_FATAL:
	case PLUM_LOG_LEVEL_ERROR:
	case PLUM_LOG_LEVEL_WARN:
		wz_log_level = LOG_INFO;
		break;
	default:
		break;
	}
	if (enabled_debug[wz_log_level])
	{
		std::string msg = (message) ? message : "";
		wzAsyncExecOnMainThread([wz_log_level, msg]() {
			_debug(__LINE__, wz_log_level, "PlumLogCallback", "%s", msg.c_str());
		});
	}
}

} // anonymous namespace

class PortMappingImpl_LibPlum : public PortMappingImpl
{
public:
	virtual ~PortMappingImpl_LibPlum();
	virtual bool init() override;
	virtual bool shutdown() override;
	virtual int create_port_mapping(uint16_t port, PortMappingInternetProtocol protocol) override;
	virtual bool destroy_port_mapping(int mappingId) override;
	virtual PortMappingImpl::Type get_impl_type() const override;
	virtual int get_discovery_timeout_seconds() const override;
private:
	bool m_isInit = false;
};

PortMappingImpl_LibPlum::~PortMappingImpl_LibPlum()
{
	try
	{
		shutdown();
	}
	catch(...) // Don't let any exceptions escape the dtor.
	{}
}

PortMappingImpl::Type PortMappingImpl_LibPlum::get_impl_type() const
{
	return PortMappingImpl::Type::Libplum;
}

int PortMappingImpl_LibPlum::get_discovery_timeout_seconds() const
{
	return 10; // 10 seconds for Libplum, to ensure we don't wait too long before falling-back to Miniupnpc
}

bool PortMappingImpl_LibPlum::init()
{
	plum_config_t config;
	memset(&config, 0, sizeof(config));
	config.log_level = PLUM_LOG_LEVEL_INFO;
	config.log_callback = &PlumLogCallback;
	auto res = plum_init(&config);
	m_isInit = (res == PLUM_ERR_SUCCESS);
	return res == PLUM_ERR_SUCCESS;
}

bool PortMappingImpl_LibPlum::shutdown()
{
	if (!m_isInit)
	{
		return false;
	}
	plum_cleanup();
	m_isInit = false;
	return true;
}

int PortMappingImpl_LibPlum::create_port_mapping(uint16_t port, PortMappingInternetProtocol protocol)
{
	plum_mapping_t m;
	memset(&m, 0, sizeof(m));
	m.protocol = PLUM_IP_PROTOCOL_TCP;
	m.internal_port = port;
	m.external_port = port; // suggest an external port the same as the internal port (the router may decide otherwise)

	auto mappingId = plum_create_mapping(&m, PlumMappingCallback);
	return mappingId;
}

bool PortMappingImpl_LibPlum::destroy_port_mapping(int mappingId)
{
	const auto result = plum_destroy_mapping(mappingId);
	return result == PLUM_ERR_SUCCESS;
}

// MARK: - Minupnpc implementation

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

void _MiniupnpcLogCallback(int line, code_part part, const char *function, const char* message)
{
	std::string msg = (message) ? message : "";
	wzAsyncExecOnMainThread([line, part, function, msg]() {
		_debug(line, part, function, "%s", msg.c_str());
	});
}
#define MiniupnpcLogCallback(part, msg) do { if (enabled_debug[part]) _MiniupnpcLogCallback(__LINE__, part, __FUNCTION__, msg); } while(0)

class PortMappingImpl_Miniupnpc : public PortMappingImpl
{
public:
	virtual ~PortMappingImpl_Miniupnpc();
	virtual bool init() override;
	virtual bool shutdown() override;
	virtual int create_port_mapping(uint16_t port, PortMappingInternetProtocol protocol) override;
	virtual bool destroy_port_mapping(int mappingId) override;
	virtual PortMappingImpl::Type get_impl_type() const override;
	virtual int get_discovery_timeout_seconds() const override;

private:
	enum class DiscoveryStatus
	{
		UPNP_SUCCESS,
		UPNP_ERROR_DEVICE_NOT_FOUND,
		UPNP_ERROR_CONTROL_NOT_AVAILABLE
	};
	struct DiscoveryResults
	{
		struct UPNPUrls urls;
		struct IGDdatas data;
		char lanaddr[64] = {};
	};
	static DiscoveryStatus upnp_discover(DiscoveryResults& output);

	struct upnp_map_output
	{
		bool success = false;
		std::string externalHost;
		uint16_t externalPort = 0;
	};
	static upnp_map_output upnp_add_redirect(int mappingId, const DiscoveryResults& discovery, uint16_t port);
	static bool upnp_remove_redirect(int mappingId, const DiscoveryResults& discovery, uint16_t port);
	static void miniupnpc_background_thread(std::shared_ptr<PortMappingImpl> pInstance);
private:
	bool m_isInit = false;

	enum class WzMiniupnpc_State
	{
		Destroyed = 0,
		Pending = 1,
		Success = 2,
		Failure = 3,
		Destroying = 4
	};

	struct MappingInfo
	{
		uint16_t port = 0;
		PortMappingInternetProtocol protocol = PortMappingInternetProtocol::IPV4;
		WzMiniupnpc_State state = WzMiniupnpc_State::Destroyed;

		void reset()
		{
			*this = MappingInfo();
		}
	};

	std::thread thread_;
	WZ_SEMAPHORE *threadSemaphore = nullptr;

	// mutex protected data
	std::mutex mappings_mtx_;
	bool stopRequested_ = false;
	optional<DiscoveryStatus> status_ = nullopt;
	std::array<MappingInfo, 16> mappings_;
};

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

// This function is run in its own thread! Do not call any non-threadsafe functions!
PortMappingImpl_Miniupnpc::DiscoveryStatus PortMappingImpl_Miniupnpc::upnp_discover(DiscoveryResults& output)
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
		return DiscoveryStatus::UPNP_ERROR_DEVICE_NOT_FOUND;
	}

	char wanaddr[64] = {};
#if defined(MINIUPNPC_API_VERSION) && (MINIUPNPC_API_VERSION >= 18)
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
		return DiscoveryStatus::UPNP_ERROR_DEVICE_NOT_FOUND;
	}

	if (!output.urls.controlURL || output.urls.controlURL[0] == '\0')
	{
		return DiscoveryStatus::UPNP_ERROR_CONTROL_NOT_AVAILABLE;
	}
	else
	{
		return DiscoveryStatus::UPNP_SUCCESS;
	}
}

bool PortMappingImpl_Miniupnpc::upnp_remove_redirect(int mappingId, const DiscoveryResults& discovery, uint16_t port)
{
	char port_str[16];
	char buf[512] = {'\0'};

	ssprintf(buf, "upnp_remove_redirect(%" PRIu16 ")", port);
	MiniupnpcLogCallback(LOG_NET, buf);

	sprintf(port_str, "%" PRIu16, port);
	auto result = UPNP_DeletePortMapping(discovery.urls.controlURL, discovery.data.first.servicetype, port_str, "TCP", nullptr);
	if (result != 0)
	{
		ssprintf(buf, "upnp_remove_redirect(%" PRIu16 ") failed with error: %d", port, result);
		MiniupnpcLogCallback(LOG_NET, buf);
	}
	return result == 0;
}

void MiniupnpcMappingCallbackSuccess(int mappingId, const std::string& externalHost, uint16_t externalPort)
{
	PortMappingManager::instance().resolve_success_fromimpl(PortMappingImpl::Type::Miniupnpc, mappingId, externalHost, externalPort);
}

void MiniupnpcMappingCallbackFailure(int mappingId)
{
	PortMappingManager::instance().resolve_failure_fromimpl(PortMappingImpl::Type::Miniupnpc, mappingId, PortMappingDiscoveryStatus::FAILURE);
}

PortMappingImpl_Miniupnpc::upnp_map_output PortMappingImpl_Miniupnpc::upnp_add_redirect(int mappingId, const DiscoveryResults& discovery, uint16_t port)
{
	char port_str[16];
	char buf[512] = {'\0'};
	int r;
	char externalIPAddress[40] = {};

	ssprintf(buf, "upnp_add_redirect(%" PRIu16 ")", port);
	MiniupnpcLogCallback(LOG_NET, buf);
	sprintf(port_str, "%" PRIu16, port);
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
					MiniupnpcMappingCallbackFailure(mappingId);
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
							MiniupnpcMappingCallbackSuccess(mappingId, result.externalHost, result.externalPort);
						}
						else
						{
							MiniupnpcMappingCallbackFailure(mappingId);
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
	if (protocol != PortMappingInternetProtocol::IPV4)
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

// MARK: - PortMappingManager

PortMappingAsyncRequestOpaqueBase::PortMappingAsyncRequestOpaqueBase()
{ }

PortMappingAsyncRequestOpaqueBase::~PortMappingAsyncRequestOpaqueBase()
{ }

PortMappingManager::PortMappingAsyncRequest::PortMappingAsyncRequest()
	: mappingId(PLUM_ERR_INVALID)
{}

PortMappingDiscoveryStatus PortMappingManager::PortMappingAsyncRequest::status() const
{
	return s;
}

bool PortMappingManager::PortMappingAsyncRequest::in_progress() const
{
	return s == PortMappingDiscoveryStatus::IN_PROGRESS;
}

bool PortMappingManager::PortMappingAsyncRequest::failed() const
{
	return s == PortMappingDiscoveryStatus::FAILURE || s == PortMappingDiscoveryStatus::TIMEOUT;
}

bool PortMappingManager::PortMappingAsyncRequest::succeeded() const
{
	return s == PortMappingDiscoveryStatus::SUCCESS;
}

bool PortMappingManager::PortMappingAsyncRequest::timed_out(TimePoint t) const
{
	return t >= deadline;
}

void PortMappingManager::PortMappingAsyncRequest::set_port_mapping_in_progress(TimePoint deadline_, MappingId mappingId_, const std::shared_ptr<PortMappingImpl>& impl_)
{
	ASSERT(s == PortMappingDiscoveryStatus::NOT_STARTED || s == PortMappingDiscoveryStatus::IN_PROGRESS, "Invalid state");
	deadline = deadline_;
	s = PortMappingDiscoveryStatus::IN_PROGRESS;
	mappingId = mappingId_;
	impl = impl_;
}

bool PortMappingManager::PortMappingAsyncRequest::destroy_mapping()
{
	// Destroy the internal mapping.
	if (mappingId != PLUM_ERR_INVALID)
	{
		const auto result = impl->destroy_port_mapping(mappingId);
		mappingId = PLUM_ERR_INVALID;
		s = PortMappingDiscoveryStatus::NOT_STARTED;
		return result;
	}
	else
	{
		return false;
	}
}

PortMappingManager& PortMappingManager::instance()
{
	static PortMappingManager inst;
	return inst;
}

std::shared_ptr<PortMappingImpl> PortMappingManager::getImpl(size_t idx)
{
	switch (idx)
	{
		case 0:
			return std::make_shared<PortMappingImpl_LibPlum>();
		case 1:
			return std::make_shared<PortMappingImpl_Miniupnpc>();
	}
	return nullptr;
}

bool PortMappingManager::getNextImpl(size_t startingIdx)
{
	std::shared_ptr<PortMappingImpl> newImpl;
	while ((newImpl = getImpl(startingIdx)))
	{
		if (newImpl->init())
		{
			currImplIdx_ = startingIdx;
			currentImpl_ = newImpl;
			loadedImpls.push_back(newImpl);
			return true;
		}
		++startingIdx;
	}
	return false;
}

void PortMappingManager::init()
{
	if (isInit_)
	{
		return;
	}

	if (getNextImpl(currImplIdx_))
	{
		isInit_ = true;
	}
}

void PortMappingManager::shutdown()
{
	if (!isInit_)
	{
		return;
	}

	currentImpl_->shutdown();
	for (auto impl : loadedImpls)
	{
		impl->shutdown();
	}
	loadedImpls.clear();
	currentImpl_ = nullptr;

	{
		const std::lock_guard<std::mutex> guard {mtx_};
		activeDiscoveries_.clear();
		successfulRequests_.clear();
	}

	// Request the monitoring thread to stop and wait for it to join.
	stopRequested_.store(true, std::memory_order_relaxed);
	if (monitoringThread_.joinable())
	{
		monitoringThread_.join();
	}
	isInit_ = false;
}

PortMappingManager::~PortMappingManager()
{
	try
	{
		shutdown();
	}
	catch(...) // Don't let any exceptions escape the dtor.
	{}
}

PortMappingAsyncRequestHandle PortMappingManager::create_port_mapping(uint16_t port, PortMappingInternetProtocol protocol)
{
	if (!isInit_)
	{
		return nullptr;
	}
	PortMappingAsyncRequestPtr h = nullptr;
	bool hasActiveDiscoveries = false;
	{
		const std::lock_guard<std::mutex> guard {mtx_};
		// First, check if there's a request for a given <port, protocol> pair
		// that has been successfully executed before. If so, just reuse it.
		auto it = successfulRequests_.find({ port, protocol });
		if (it != successfulRequests_.end())
		{
			return it->second;
		}

		auto mappingId = currentImpl_->create_port_mapping(port, protocol);
		if (mappingId < 0)
		{
			// TODO: Could try other backends as well
			return nullptr;
		}
		h = std::make_shared<PortMappingAsyncRequest>();
		h->sourcePort = port;
		h->protocol = protocol;
		h->set_port_mapping_in_progress(PortMappingAsyncRequest::ClockType::now() + std::chrono::seconds(currentImpl_->get_discovery_timeout_seconds()), mappingId, currentImpl_);

		hasActiveDiscoveries = !activeDiscoveries_.empty();
		activeDiscoveries_.emplace(ImplMappingId{h->impl->get_impl_type(), mappingId}, h);
	}
	if (!hasActiveDiscoveries)
	{
		// The monitoring thread should aready have finished processing.
		// So, need to restart it.
		if (monitoringThread_.joinable())
		{
			monitoringThread_.join();
		}
		stopRequested_.store(false, std::memory_order_relaxed);
		monitoringThread_ = std::thread([this] { thread_monitor_function(); });
	}
	return h;
}

bool PortMappingManager::destroy_port_mapping(const PortMappingAsyncRequestHandle& p)
{
	ASSERT_OR_RETURN(false, isInit_, "PortMappingManager is not initialized");
	auto h = std::dynamic_pointer_cast<PortMappingAsyncRequest>(p);
	ASSERT_OR_RETURN(false, h != nullptr, "Invalid request handle");
	const std::lock_guard<std::mutex> guard {mtx_};
	if (h->mappingId == PLUM_ERR_INVALID)
	{
		return true;
	}
	// Remove this request from active/successful requests.
	if (h->in_progress())
	{
		auto it = activeDiscoveries_.find({h->impl->get_impl_type(), h->mappingId});
		if (it != activeDiscoveries_.end())
		{
			activeDiscoveries_.erase(it);
		}
	}
	else if (h->succeeded())
	{
		auto it = successfulRequests_.find(PortWithProtocol{ h->sourcePort, h->protocol });
		if (it != successfulRequests_.end())
		{
			successfulRequests_.erase(it);
		}
	}
	auto wasFailedMapping = h->failed();
	// Destroy the internal mapping.
	auto result = h->destroy_mapping();
	h->mappingId = PLUM_ERR_INVALID;
	return (wasFailedMapping) ? true : result;
}

PortMappingDiscoveryStatus PortMappingManager::get_status(const PortMappingAsyncRequestHandle& p) const
{
	auto h = std::dynamic_pointer_cast<PortMappingAsyncRequest>(p);
	ASSERT_OR_RETURN(PortMappingDiscoveryStatus::FAILURE, h != nullptr, "Invalid request handle");
	const std::lock_guard<std::mutex> guard {mtx_};
	return h->status();
}

void PortMappingManager::attach_callback(const PortMappingAsyncRequestHandle& p, SuccessCallback successCb, FailureCallback failureCb)
{
	auto h = std::dynamic_pointer_cast<PortMappingAsyncRequest>(p);
	ASSERT_OR_RETURN(, h != nullptr, "Invalid request handle");
	PortMappingDiscoveryStatus s;
	{
		const std::lock_guard<std::mutex> guard {mtx_};
		s = h->s;
		if (s == PortMappingDiscoveryStatus::IN_PROGRESS)
		{
			h->callbacks.emplace_back(std::move(successCb), std::move(failureCb));
			return;
		}
	}
	// If the discovery has already finished with either success or failure, we can schedule
	// the callback right away, because the monitoring thread may already have finished executing.
	if (s == PortMappingDiscoveryStatus::SUCCESS)
	{
		wzAsyncExecOnMainThread([successCb, ip = h->discoveredIPaddress, port = h->discoveredPort]()
		{
			successCb(ip, port);
		});
		return;
	}
	if (s == PortMappingDiscoveryStatus::FAILURE || s == PortMappingDiscoveryStatus::TIMEOUT)
	{
		wzAsyncExecOnMainThread([failureCb, s]()
		{
			failureCb(s);
		});
		return;
	}
}

void PortMappingManager::scheduleCallbacksOnMainThread(CallbacksPerRequest cbReqMap)
{
	if (cbReqMap.empty())
	{
		return;
	}
	for (auto& cb : cbReqMap)
	{
		if (cb.first->succeeded() && !cb.second.empty())
		{
			wzAsyncExecOnMainThread([callbacks = cb.second, externalHost = cb.first->discoveredIPaddress, externalPort = cb.first->discoveredPort]()
			{
				for (auto& cb : callbacks)
				{
					cb.first(externalHost, externalPort);
				}
			});
		}
		else if (cb.first->failed() && !cb.second.empty()) // Failed for any reason or timed out
		{
			wzAsyncExecOnMainThread([callbacks = cb.second, status = cb.first->s]()
			{
				for (auto& cb : callbacks)
				{
					cb.second(status);
				}
			});
		}
		else
		{
			ASSERT(false, "Should be unreachable");
		}
	}
}

void PortMappingManager::thread_monitor_function()
{
	using namespace std::chrono_literals;

	CallbacksPerRequest callbacksPerRequest;
	std::vector<PortMappingAsyncRequestPtr> requestsSwitchingImpl;
	while (!stopRequested_.load(std::memory_order_relaxed))
	{
		{
			const std::lock_guard<std::mutex> lock {mtx_};
			// If all inflight requests have already finished, stop monitoring changes.
			if (activeDiscoveries_.empty())
			{
				break;
			}
			// Go through active discovery requests and collect callbacks to be scheduled
			// for execution at the end of the current loop iteration.
			for (auto it = activeDiscoveries_.begin(); it != activeDiscoveries_.end();)
			{
				auto& req = it->second;
				auto takeCallbacksAndRemoveFromActiveList = [this, &callbacksPerRequest, &it](auto& req)
				{
					// Move callbacks out of the finished request and remove from active list.
					callbacksPerRequest[req] = std::move(req->callbacks);
					req->callbacks.clear();
					it = activeDiscoveries_.erase(it);
				};
				if (req->succeeded())
				{
					successfulRequests_.emplace(PortWithProtocol{ req->sourcePort, req->protocol }, req);
					takeCallbacksAndRemoveFromActiveList(req);
				}
				else if (req->timed_out(PortMappingAsyncRequest::ClockType::now()))
				{
					if (resolve_failure_internal(req, PortMappingDiscoveryStatus::TIMEOUT))
					{
						// resolve_failure_internal dispatched to a different implementation and reset timeout - wait to see what it returns
						// queue updating its key in the active list
						requestsSwitchingImpl.push_back(req);
						it = activeDiscoveries_.erase(it);
					}
					else
					{
						// failure is final
						takeCallbacksAndRemoveFromActiveList(req);
					}
				}
				else if (req->failed())
				{
					// Already resolved, but need to move out the callbacks and remove from active list.
					takeCallbacksAndRemoveFromActiveList(req);
				}
				else
				{
					ASSERT(req->in_progress(), "Unexpected request state: %d", static_cast<int>(req->s));
					++it;
				}
			}
		}
		// Schedule execution of registered callbacks for all requests, which have already
		// transitioned to some terminal (either success or failure) state.
		scheduleCallbacksOnMainThread(std::move(callbacksPerRequest));
		callbacksPerRequest.clear();
		// Move any requestsSwitchingImpl to the appropriate mapping entry in the activeDiscoveriesMap_
		for (const auto& req : requestsSwitchingImpl)
		{
			activeDiscoveries_.emplace(ImplMappingId{req->impl->get_impl_type(), req->mappingId}, req);
		}
		requestsSwitchingImpl.clear();

		// Sleep for another 50ms and re-check the status for all active requests.
		std::this_thread::sleep_for(50ms);
	}
}

// Set discovery status to `SUCCESS`.
// The method saves the discovered IP address + port combination inside the
// `PortMappingManager` instance so that subsequent `schedule_callback()`
// invocations can use this data.
//
// The method shall only be called from the impl's discovery callback.
void PortMappingManager::resolve_success_fromimpl(PortMappingImpl::Type type, int mappingId, std::string externalIp, uint16_t externalPort)
{
	const std::lock_guard<std::mutex> guard {mtx_};
	const auto req = active_request_for_id_internal(type, mappingId);
	if (!req)
	{
		debug(LOG_NET, "Mapping %d: no active request found in PortMappingManager, probably timed out\n", mappingId);
		return;
	}
	if (!resolve_success_internal(req, externalIp, externalPort))
	{
		if (req->mappingId != PLUM_ERR_INVALID)
		{
			req->destroy_mapping();
			req->mappingId = PLUM_ERR_INVALID;
		}
	}
}

// Forcefully set discovery status to `status`.
// The method shall only be called from impl's discovery callback in case the discovery
// process has failed for any reason as reported directly by the impl.
//
// The `status` may only take `FAILURE` or `TIMEOUT` values.
void PortMappingManager::resolve_failure_fromimpl(PortMappingImpl::Type type, int mappingId, PortMappingDiscoveryStatus failureStatus)
{
	const std::lock_guard<std::mutex> guard {mtx_};
	auto req = active_request_for_id_internal(type, mappingId);
	if (!req)
	{
		debug(LOG_NET, "Mapping %d: no active request found in PortMappingManager, probably timed out\n", mappingId);
		return;
	}
	if (resolve_failure_internal(req, failureStatus))
	{
		// update its key in the active list
		auto it = activeDiscoveries_.find({type, mappingId});
		if (it != activeDiscoveries_.end())
		{
			activeDiscoveries_.erase(it);
		}
		activeDiscoveries_.emplace(ImplMappingId{req->impl->get_impl_type(), req->mappingId}, req);
	}
}

// MARK: - BEGIN: Unsafe functions that must only be called if mtx_ lock is already held!

// Must only be called if lock is held!
PortMappingManager::PortMappingAsyncRequestPtr PortMappingManager::active_request_for_id_internal(PortMappingImpl::Type type, PortMappingAsyncRequest::MappingId id) const
{
	auto it = activeDiscoveries_.find({type, id});
	if (it != activeDiscoveries_.end())
	{
		return it->second;
	}
	return nullptr;
}

// Set discovery status to `SUCCESS`.
// The method saves the discovered IP address + port combination inside the
// `PortMappingManager` instance so that subsequent `schedule_callback()`
// invocations can use this data.
//
// The method shall only be called from resolve_success_fromimpl() (which is called by LibPlum's discovery callback).
bool PortMappingManager::resolve_success_internal(PortMappingAsyncRequestPtr req, std::string externalIp, uint16_t externalPort)
{
	if (req->s != PortMappingDiscoveryStatus::IN_PROGRESS)
	{
		// Someone has already gotten ahead of us
		if (req->s == PortMappingDiscoveryStatus::SUCCESS)
		{
			// already resolved to success ?
			return true;
		}
		else
		{
			// failed to resolve to success
			// parent will handle cleanup as needed
			return false;
		}
	}
	req->s = PortMappingDiscoveryStatus::SUCCESS;
	req->deadline = PortMappingAsyncRequest::TimePoint::max();
	req->discoveredIPaddress = std::move(externalIp);
	req->discoveredPort = externalPort;
	return true;
}

// Forcefully set discovery status to `status`.
// The method may either be called from resolve_failure_fromimpl() (i.e. from libPlum callbacks), or by
// `PortMappingManager` itself (by the monitoring thread), if the discovery process reaches timeout.
//
// The `status` may only take `FAILURE` or `TIMEOUT` values.
bool PortMappingManager::resolve_failure_internal(PortMappingAsyncRequestPtr req, PortMappingDiscoveryStatus failureStatus)
{
	ASSERT(failureStatus == PortMappingDiscoveryStatus::FAILURE || failureStatus == PortMappingDiscoveryStatus::TIMEOUT,
		"Status should either be failure or timeout");
	if (req->s != PortMappingDiscoveryStatus::IN_PROGRESS)
	{
		// Someone has already gotten ahead of us - nothing to do there.
		return false;
	}

	bool finalFailure = false;

	// Check for another implementation
	if (req->impl != currentImpl_ || getNextImpl(currImplIdx_ + 1))
	{
		// Another implementation is available - fallback to it and try again
		do {
			auto newMappingId = currentImpl_->create_port_mapping(req->sourcePort, req->protocol);
			if (newMappingId >= 0)
			{
				// new impl is attempting request
				req->set_port_mapping_in_progress(PortMappingAsyncRequest::ClockType::now() + std::chrono::seconds(currentImpl_->get_discovery_timeout_seconds()), newMappingId, currentImpl_);
				return true; // attempting with different implementation
			}
		} while (getNextImpl(currImplIdx_ + 1));

		// new impl(s) failed
		finalFailure = true;
	}
	else
	{
		// no additional impl available - fail out
		finalFailure = true;
	}

	if (finalFailure)
	{
		req->s = failureStatus;
		req->deadline = PortMappingAsyncRequest::TimePoint::max();
		req->discoveredIPaddress.clear();
		req->discoveredPort = 0;
	}
	return false;
}

// MARK: - END: Unsafe functions that must only be called if mtx_ lock is already held!
