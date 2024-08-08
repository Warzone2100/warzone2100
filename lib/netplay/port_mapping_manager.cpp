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

#include "port_mapping_manager_impl_libplum.h"
#include "port_mapping_manager_impl_miniupnpc.h"

#define WZ_PORT_MAPPING_ID_INVALID -1

PortMappingImpl::~PortMappingImpl()
{
	// no-op
}

// MARK: - General callback helpers

void PortMappingImpl_MappingCallbackSuccess(PortMappingImpl::Type type, int mappingId, const std::string& externalHost, uint16_t externalPort)
{
	PortMappingManager::instance().resolve_success_fromimpl(type, mappingId, externalHost, externalPort);
}

void PortMappingImpl_MappingCallbackFailure(PortMappingImpl::Type type, int mappingId)
{
	PortMappingManager::instance().resolve_failure_fromimpl(type, mappingId, PortMappingDiscoveryStatus::FAILURE);
}

// MARK: - PortMappingManager

PortMappingAsyncRequestOpaqueBase::PortMappingAsyncRequestOpaqueBase()
{ }

PortMappingAsyncRequestOpaqueBase::~PortMappingAsyncRequestOpaqueBase()
{ }

PortMappingManager::PortMappingAsyncRequest::PortMappingAsyncRequest()
	: mappingId(WZ_PORT_MAPPING_ID_INVALID)
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
	ASSERT(impl_ != nullptr, "Missing impl?");
	deadline = deadline_;
	s = PortMappingDiscoveryStatus::IN_PROGRESS;
	mappingId = mappingId_;
	impl = impl_;
}

bool PortMappingManager::PortMappingAsyncRequest::destroy_mapping()
{
	// Destroy the internal mapping.
	if (mappingId != WZ_PORT_MAPPING_ID_INVALID)
	{
		ASSERT_OR_RETURN(false, impl != nullptr, "Missing impl?");
		const auto result = impl->destroy_port_mapping(mappingId);
		mappingId = WZ_PORT_MAPPING_ID_INVALID;
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

constexpr PortMappingImpl::Type nextImplType(PortMappingImpl::Type t)
{
	ASSERT(static_cast<std::underlying_type_t<PortMappingImpl::Type>>(t) < static_cast<std::underlying_type_t<PortMappingImpl::Type>>(PortMappingImpl::Type::End), "Unexpected PortMappingImpl type");
	return static_cast<PortMappingImpl::Type>(static_cast<std::underlying_type_t<PortMappingImpl::Type>>(t) + 1);
}

std::shared_ptr<PortMappingImpl> PortMappingManager::makeImpl(PortMappingImpl::Type t)
{
	switch (t)
	{
		case PortMappingImpl::Type::Libplum:
			return std::make_shared<PortMappingImpl_LibPlum>();
		case PortMappingImpl::Type::Miniupnpc:
			return std::make_shared<PortMappingImpl_Miniupnpc>();
		case PortMappingImpl::Type::End:
			return nullptr;
	}
	return nullptr;
}

bool PortMappingManager::switchToNextWorkingImpl(PortMappingImpl::Type startingTypeIdx)
{
	std::shared_ptr<PortMappingImpl> newImpl;
	while ((newImpl = makeImpl(startingTypeIdx)))
	{
		if (newImpl->init())
		{
			currImplTypeIdx_ = startingTypeIdx;
			currentImpl_ = newImpl;
			loadedImpls_.push_back(newImpl);
			return true;
		}
		startingTypeIdx = nextImplType(startingTypeIdx);
	}
	return false;
}

void PortMappingManager::init()
{
	if (isInit_)
	{
		return;
	}

	if (switchToNextWorkingImpl(currImplTypeIdx_))
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

	if (currentImpl_)
	{
		currentImpl_->shutdown();
	}
	for (auto impl : loadedImpls_)
	{
		impl->shutdown();
	}
	loadedImpls_.clear();
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
	if (h->mappingId == WZ_PORT_MAPPING_ID_INVALID)
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
	h->mappingId = WZ_PORT_MAPPING_ID_INVALID;
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
		if (req->mappingId != WZ_PORT_MAPPING_ID_INVALID)
		{
			req->destroy_mapping();
			req->mappingId = WZ_PORT_MAPPING_ID_INVALID;
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
	if (req->impl != currentImpl_ || switchToNextWorkingImpl(nextImplType(currImplTypeIdx_)))
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
		} while (switchToNextWorkingImpl(nextImplType(currImplTypeIdx_)));

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
