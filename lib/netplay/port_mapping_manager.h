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

#include <vector>
#include <functional>
#include <chrono>
#include <thread>
#include <string>

#include <stdint.h>

/// <summary>
/// Singleton object responsible for
/// 1. Communicating with an underlying Port Mapping library implementation (currently, libplum);
/// 2. Executing user callbacks in response to the port mapping discovery outcome;
/// 3. Automatically detecting if the port mapping discovery times out, so that it can
///    schedule timeout callbacks in response to that.
///
/// The current value of timeout is equal to 10 seconds.
/// </summary>
class PortMappingManager
{
public:

	enum class DiscoveryStatus
	{
		NOT_STARTED,
		IN_PROGRESS,
		SUCCESS,
		FAILURE,
		TIMEOUT
	};

	using SuccessCallback = std::function<void(std::string, uint16_t)>;
	// DiscoveryStatus can only be `FAILURE` or `TIMEOUT` in this case.
	using FailureCallback = std::function<void(DiscoveryStatus)>;

	static constexpr int DISCOVERY_TIMEOUT_SECONDS = 10;

	static PortMappingManager& instance();

	void init();
	void shutdown();

	bool create_port_mapping(uint16_t port);
	bool destroy_active_mapping();

	DiscoveryStatus status() const;

	// In case the discovery is still in progress, add a pair of `<success, failure>` callbacks
	// to be scheduled later on the main WZ thread.
	// The actual scheduling will happen by the time the monitoring thread exits.
	//
	// If the method is called when the discovery has already finished, the appropriate callback
	// is scheduled on the main WZ thread right away.
	void schedule_callback(SuccessCallback successCb, FailureCallback failureCb);

	// Set discovery status to `SUCCESS` and request the monitoring thread to return.
	// The method saves the discovered IP address + port combination inside the
	// `PortMappingManager` instance so that subsequent `schedule_callback()`
	// invocations can use this data.
	//
	// The method shall only be called from the LibPlum's discovery callback.
	void set_discovery_success(std::string externalIp, uint16_t externalPort);
	// Forcefully set discovery status to `status` and request the monitoring thread to return.
	// The method may either be called from LibPlum's discovery callback in case the discovery
	// process has failed for any reason as reported directly by LibPlum, or by
	// `PortMappingManager` itself (by the monitoring thread), if the discovery process reaches timeout.
	//
	// The `status` may only take `FAILURE` or `TIMEOUT` values.
	void set_discovery_failure(DiscoveryStatus status);

private:

	PortMappingManager();

	using ClockType = std::chrono::steady_clock;
	using TimePoint = ClockType::time_point;
	using PortMappingId = int;
	using CallbackVector = std::vector<std::pair<SuccessCallback, FailureCallback>>;

	// Main loop function for the monitoring thread.
	// It's main purpose is to detect when the discovery process
	// times out and execute delayed callbacks once the discovery
	// process finishes by any means (success or failure/timeout).
	void thread_monitor_function();

	// Calculates the timeout deadline and spins up a separate thread, which monitors
	// the state of port mapping discovery process with respect to the deadline.
	void on_discovery_initiated();

	PortMappingId mapping_id_;
	CallbackVector callbacks_;
	// Use `TimePoint::max()` as a special sentinel value to indicate that the discovery isn't started yet.
	// Once the discovery process commences, this will be set to the value of `now() + 10s`.
	TimePoint deadline_ = TimePoint::max();
	bool isInit_ = false;
	DiscoveryStatus status_ = DiscoveryStatus::NOT_STARTED;
	std::string discoveredIPaddress_;
	uint16_t discoveredPort_ = 0;

	std::thread monitoring_thread_;
	mutable std::mutex mtx_;
	std::atomic_bool stop_requested_;
};
