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

#include <plum/plum.h>

void PlumMappingCallbackSuccess(int mappingId, const std::string& externalHost, uint16_t externalPort)
{
	PortMappingManager::instance().resolve_success_fromlibplum(mappingId, externalHost, externalPort);
}

void PlumMappingCallbackFailure(int mappingId)
{
	PortMappingManager::instance().resolve_failure_fromlibplum(mappingId, PortMappingDiscoveryStatus::FAILURE);
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

void PlumLogCallback(plum_log_level_t /*level*/, const char* message)
{
	debug(LOG_NET, "LibPlum message: %s", message);
}

} // anonymous namespace

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

PortMappingManager& PortMappingManager::instance()
{
	static PortMappingManager inst;
	return inst;
}

void PortMappingManager::init()
{
	if (isInit_)
	{
		return;
	}
	plum_config_t config;
	memset(&config, 0, sizeof(config));
	config.log_level = PLUM_LOG_LEVEL_INFO;
	config.log_callback = &PlumLogCallback;
	if (plum_init(&config) == PLUM_ERR_SUCCESS)
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
	plum_cleanup();
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

		plum_mapping_t m;
		memset(&m, 0, sizeof(m));
		m.protocol = PLUM_IP_PROTOCOL_TCP;
		m.internal_port = port;
		m.external_port = port; // suggest an external port the same as the internal port (the router may decide otherwise)

		auto mappingId = plum_create_mapping(&m, PlumMappingCallback);
		if (mappingId < 0)
		{
			return nullptr;
		}
		h = std::make_shared<PortMappingAsyncRequest>();
		h->deadline = PortMappingAsyncRequest::ClockType::now() + std::chrono::seconds(DISCOVERY_TIMEOUT_SECONDS);
		h->s = PortMappingDiscoveryStatus::IN_PROGRESS;
		h->mappingId = mappingId;
		h->sourcePort = port;
		h->protocol = protocol;

		hasActiveDiscoveries = !activeDiscoveries_.empty();
		activeDiscoveries_.emplace(mappingId, h);
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
		auto it = activeDiscoveries_.find(h->mappingId);
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
	// Destroy the internal mapping.
	const auto result = plum_destroy_mapping(h->mappingId);
	h->mappingId = PLUM_ERR_INVALID;
	h->s = PortMappingDiscoveryStatus::NOT_STARTED;
	return result == PLUM_ERR_SUCCESS;
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
					resolve_failure_internal(req, PortMappingDiscoveryStatus::TIMEOUT);
					takeCallbacksAndRemoveFromActiveList(req);
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
		// Sleep for another 50ms and re-check the status for all active requests.
		std::this_thread::sleep_for(50ms);
	}
}

// Set discovery status to `SUCCESS`.
// The method saves the discovered IP address + port combination inside the
// `PortMappingManager` instance so that subsequent `schedule_callback()`
// invocations can use this data.
//
// The method shall only be called from the LibPlum's discovery callback.
void PortMappingManager::resolve_success_fromlibplum(int mappingId, std::string externalIp, uint16_t externalPort)
{
	const std::lock_guard<std::mutex> guard {mtx_};
	const auto req = active_request_for_id_internal(mappingId);
	if (!req)
	{
		debug(LOG_NET, "Mapping %d: no active request found in PortMappingManager, probably timed out\n", mappingId);
		return;
	}
	resolve_success_internal(req, externalIp, externalPort);
}

// Forcefully set discovery status to `status`.
// The method shall only be called from LibPlum's discovery callback in case the discovery
// process has failed for any reason as reported directly by LibPlum.
//
// The `status` may only take `FAILURE` or `TIMEOUT` values.
void PortMappingManager::resolve_failure_fromlibplum(int mappingId, PortMappingDiscoveryStatus failureStatus)
{
	const std::lock_guard<std::mutex> guard {mtx_};
	const auto req = active_request_for_id_internal(mappingId);
	if (!req)
	{
		debug(LOG_NET, "Mapping %d: no active request found in PortMappingManager, probably timed out\n", mappingId);
		return;
	}
	resolve_failure_internal(req, failureStatus);
}

// MARK: - BEGIN: Unsafe functions that must only be called if mtx_ lock is already held!

// Must only be called if lock is held!
PortMappingManager::PortMappingAsyncRequestPtr PortMappingManager::active_request_for_id_internal(PortMappingAsyncRequest::MappingId id) const
{
	auto it = activeDiscoveries_.find(id);
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
// The method shall only be called from resolve_success_fromlibplum() (which is called by LibPlum's discovery callback).
void PortMappingManager::resolve_success_internal(PortMappingAsyncRequestPtr req, std::string externalIp, uint16_t externalPort)
{
	if (req->s != PortMappingDiscoveryStatus::IN_PROGRESS)
	{
		// Someone has already gotten ahead of us - nothing to do there.
		return;
	}
	req->s = PortMappingDiscoveryStatus::SUCCESS;
	req->deadline = PortMappingAsyncRequest::TimePoint::max();
	req->discoveredIPaddress = std::move(externalIp);
	req->discoveredPort = externalPort;
}

// Forcefully set discovery status to `status`.
// The method may either be called from resolve_failure_fromlibplum() (i.e. from libPlum callbacks), or by
// `PortMappingManager` itself (by the monitoring thread), if the discovery process reaches timeout.
//
// The `status` may only take `FAILURE` or `TIMEOUT` values.
void PortMappingManager::resolve_failure_internal(PortMappingAsyncRequestPtr req, PortMappingDiscoveryStatus failureStatus)
{
	ASSERT(failureStatus == PortMappingDiscoveryStatus::FAILURE || failureStatus == PortMappingDiscoveryStatus::TIMEOUT,
		"Status should either be failure or timeout");
	if (req->s != PortMappingDiscoveryStatus::IN_PROGRESS)
	{
		// Someone has already gotten ahead of us - nothing to do there.
		return;
	}
	req->s = failureStatus;
	req->deadline = PortMappingAsyncRequest::TimePoint::max();
	req->discoveredIPaddress.clear();
	req->discoveredPort = 0;
}

// MARK: - END: Unsafe functions that must only be called if mtx_ lock is already held!

constexpr int PortMappingManager::DISCOVERY_TIMEOUT_SECONDS;
