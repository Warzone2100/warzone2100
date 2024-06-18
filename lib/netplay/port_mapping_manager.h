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
 * @file port_mapping_manager.h
 * Port Mapping Manager singleton for communicating with port mapping library implementation.
 */
#pragma once

#include <atomic>
#include <vector>
#include <functional>
#include <chrono>
#include <thread>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <memory>

#include <stdint.h>

enum class PortMappingDiscoveryStatus
{
	NOT_STARTED,
	IN_PROGRESS,
	SUCCESS,
	FAILURE,
	TIMEOUT
};

using SuccessCallback = std::function<void(std::string, uint16_t)>;
// DiscoveryStatus can only be `FAILURE` or `TIMEOUT` in this case.
using FailureCallback = std::function<void(PortMappingDiscoveryStatus)>;

enum class PortMappingInternetProtocol
{
	IPV4,
	IPV6
};

/// <summary>
/// An opaque base class to represent asynchronous port mapping discovery request.
///
/// These are supposed to be created only by `PortMappingManager` instance and used
/// as opaque handles (PortMappingAsyncRequestHandle) by the higher-level code.
/// </summary>
class PortMappingAsyncRequestOpaqueBase
{
protected:
	PortMappingAsyncRequestOpaqueBase();
public:
	virtual ~PortMappingAsyncRequestOpaqueBase();
public:
	PortMappingAsyncRequestOpaqueBase( const PortMappingAsyncRequestOpaqueBase& ) = delete;
	PortMappingAsyncRequestOpaqueBase& operator=( const PortMappingAsyncRequestOpaqueBase& ) = delete;
};

using PortMappingAsyncRequestHandle = std::shared_ptr<PortMappingAsyncRequestOpaqueBase>;
using PortWithProtocol = std::pair<uint16_t, PortMappingInternetProtocol>;

// Specialize `std::hash` for `PortWithProtocol` pair to be able to use it as the key in associative containers.
template <>
struct std::hash<PortWithProtocol>
{
	size_t operator ()(const PortWithProtocol& p) const
	{
		std::hash<int> h;
		return h(static_cast<int>(p.first)) ^ h(static_cast<int>(p.second));
	}
};

/// <summary>
/// Singleton object responsible for
/// 1. Communicating with an underlying Port Mapping library implementation (currently, libplum);
/// 2. Executing user callbacks in response to the port mapping discovery outcome;
/// 3. Automatically detecting if the port mapping discovery times out, so that it can
///    schedule timeout callbacks in response to that.
///
/// The current value of timeout is equal to `DISCOVERY_TIMEOUT_SECONDS`, which is 10 seconds.
/// </summary>
class PortMappingManager
{
public:

	static constexpr int DISCOVERY_TIMEOUT_SECONDS = 10;

	static PortMappingManager& instance();

	void init();
	void shutdown();

	// If previous attempt was successful, returns already-resolved request ptr.
	// otherwise, creates a new request.
	PortMappingAsyncRequestHandle create_port_mapping(uint16_t port, PortMappingInternetProtocol protocol);
	bool destroy_port_mapping(const PortMappingAsyncRequestHandle& h);

	PortMappingDiscoveryStatus get_status(const PortMappingAsyncRequestHandle& h) const;

	// In case the discovery is still in progress, add a pair of `<success, failure>` callbacks
	// to be scheduled later on the main WZ thread.
	// The actual scheduling will happen by the time the monitoring thread exits.
	//
	// If the method is called when the discovery has already finished, the appropriate callback
	// is scheduled on the main WZ thread right away.
	void attach_callback(const PortMappingAsyncRequestHandle& h, SuccessCallback successCb, FailureCallback failureCb);

private:
	/// <summary>
	/// An internal class to represent asynchronous port mapping discovery request.
	///
	/// These are supposed to be created only by `PortMappingManager` instance and used
	/// as opaque handles by the higher-level code.
	/// </summary>
	class PortMappingAsyncRequest : public PortMappingAsyncRequestOpaqueBase
	{
	public:
		using MappingId = int;
		using ClockType = std::chrono::steady_clock;
		using TimePoint = ClockType::time_point;
		using CallbackVector = std::vector<std::pair<SuccessCallback, FailureCallback>>;

		PortMappingAsyncRequest();

		PortMappingAsyncRequest( const PortMappingAsyncRequest& ) = delete;
		PortMappingAsyncRequest& operator=( const PortMappingAsyncRequest& ) = delete;

		PortMappingDiscoveryStatus status() const;
		bool in_progress() const;
		bool failed() const;
		bool succeeded() const;
		bool timed_out(TimePoint t) const;

		// Use `TimePoint::max()` as a special sentinel value to indicate that the discovery isn't started yet.
		// Once the discovery process commences, this will be set to the value of `now() + 10s`.
		TimePoint deadline = TimePoint::max();
		MappingId mappingId;
		CallbackVector callbacks;
		PortMappingDiscoveryStatus s = PortMappingDiscoveryStatus::NOT_STARTED;
		std::string discoveredIPaddress;
		uint16_t discoveredPort = 0;
		uint16_t sourcePort = 0;
		PortMappingInternetProtocol protocol = PortMappingInternetProtocol::IPV4;
	};
	using PortMappingAsyncRequestPtr = std::shared_ptr<PortMappingAsyncRequest>;

protected:
	// Only intended to be called by plum callback handlers:
	friend void PlumMappingCallbackSuccess(int mappingId, const std::string& externalHost, uint16_t externalPort);
	friend void PlumMappingCallbackFailure(int mappingId);

	// Set discovery status to `SUCCESS`.
	// The method saves the discovered IP address + port combination inside the
	// `PortMappingManager` instance so that subsequent `schedule_callback()`
	// invocations can use this data.
	//
	// The method shall only be called from the LibPlum's discovery callback.
	void resolve_success_fromlibplum(int mappingId, std::string externalIp, uint16_t externalPort);

	// Forcefully set discovery status to `status`.
	// The method shall only be called from LibPlum's discovery callback in case the discovery
	// process has failed for any reason as reported directly by LibPlum.
	//
	// The `status` may only take `FAILURE` or `TIMEOUT` values.
	void resolve_failure_fromlibplum(int mappingId, PortMappingDiscoveryStatus failureStatus);

private:

	~PortMappingManager();

	// Main loop function for the monitoring thread.
	// It's main purpose is to:
	// 1. Detect when the discovery process reaches some terminal point of
	//    its execution (success/failure/timeout)
	// 2. Schedule delayed callbacks to execute on the main thread once the
	//    discovery process finishes by any means (success or failure/timeout).
	void thread_monitor_function();

	using CallbacksPerRequest = std::unordered_map<PortMappingAsyncRequestPtr, PortMappingAsyncRequest::CallbackVector>;
	static void scheduleCallbacksOnMainThread(CallbacksPerRequest cbReqMap);

	// Must only be called internally, and if mtx_ lock is held!
	PortMappingAsyncRequestPtr active_request_for_id_internal(int id) const;
	static void resolve_success_internal(PortMappingAsyncRequestPtr req, std::string externalIp, uint16_t externalPort);
	static void resolve_failure_internal(PortMappingAsyncRequestPtr req, PortMappingDiscoveryStatus failureStatus);


	std::unordered_map<PortMappingAsyncRequest::MappingId, PortMappingAsyncRequestPtr> activeDiscoveries_;
	std::unordered_map<PortWithProtocol, PortMappingAsyncRequestPtr> successfulRequests_;

	bool isInit_ = false;

	std::thread monitoringThread_;
	mutable std::mutex mtx_;
	std::atomic_bool stopRequested_;
};
