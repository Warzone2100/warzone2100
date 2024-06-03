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

namespace
{

// This function is run in its own thread managed by LibPLum! Do not call any non-threadsafe functions!
void PlumMappingCallback(int mappingId, plum_state_t state, const plum_mapping_t* mapping)
{
	debug(LOG_NET, "Port mapping %d: state=%d\n", mappingId, (int)state);
	switch (state) {
	case PLUM_STATE_SUCCESS:
		debug(LOG_NET, "Mapping %d: success, internal=%hu, external=%s:%hu\n", mappingId, mapping->internal_port,
			mapping->external_host, mapping->external_port);
		PortMappingManager::instance().set_discovery_success(mapping->external_host, mapping->external_port);
		break;

	case PLUM_STATE_FAILURE:
		debug(LOG_NET, "Mapping %d: failed\n", mappingId);
		PortMappingManager::instance().set_discovery_failure(PortMappingManager::DiscoveryStatus::FAILURE);
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

PortMappingManager& PortMappingManager::instance()
{
	static PortMappingManager inst;
	return inst;
}

PortMappingManager::PortMappingManager()
	: mapping_id_(PLUM_ERR_INVALID)
{}

void PortMappingManager::init()
{
	plum_config_t config;
	memset(&config, 0, sizeof(config));
	config.log_level = PLUM_LOG_LEVEL_INFO;
	config.log_callback = &PlumLogCallback;
	plum_init(&config);
}

void PortMappingManager::shutdown()
{
	plum_cleanup();
	status_ = DiscoveryStatus::NOT_STARTED;
	// Request the monitoring thread to stop and wait for it to join.
	stop_requested_.store(true, std::memory_order_release);
	if (monitoring_thread_.joinable())
	{
		monitoring_thread_.join();
	}
}

bool PortMappingManager::create_port_mapping(uint16_t port)
{
	plum_mapping_t m;
	memset(&m, 0, sizeof(m));
	m.protocol = PLUM_IP_PROTOCOL_TCP;
	m.internal_port = port;

	mapping_id_ = plum_create_mapping(&m, PlumMappingCallback);
	const bool status = mapping_id_ == PLUM_ERR_SUCCESS;
	if (!status)
	{
		return false;
	}
	on_discovery_initiated();
	return true;
}

bool PortMappingManager::destroy_active_mapping()
{
	return plum_destroy_mapping(mapping_id_) == PLUM_ERR_SUCCESS;
}

PortMappingManager::DiscoveryStatus PortMappingManager::status() const
{
	const std::lock_guard<std::mutex> guard {mtx_};
	return status_;
}

void PortMappingManager::on_discovery_initiated()
{
	deadline_ = ClockType::now() + std::chrono::seconds(DISCOVERY_TIMEOUT_SECONDS);
	status_ = DiscoveryStatus::IN_PROGRESS;
	stop_requested_.store(false, std::memory_order_release);
	monitoring_thread_ = std::thread([this] { thread_monitor_function(); });
}

void PortMappingManager::schedule_callback(SuccessCallback successCb, FailureCallback failureCb)
{
	DiscoveryStatus statusCopy;
	{
		const std::lock_guard<std::mutex> guard {mtx_};
		statusCopy = status_;
		if (statusCopy == DiscoveryStatus::IN_PROGRESS)
		{
			callbacks_.emplace_back(std::move(successCb), std::move(failureCb));
			return;
		}
	}
	// If the discovery has already finished with either success or failure, we can schedule
	// the callback right away, because the monitoring thread may already have finished executing.
	if (statusCopy == DiscoveryStatus::SUCCESS)
	{
		wzAsyncExecOnMainThread([successCb, ip = discoveredIPaddress_, port = discoveredPort_]()
		{
			successCb(ip, port);
		});
		return;
	}
	if (statusCopy == DiscoveryStatus::FAILURE || statusCopy == DiscoveryStatus::TIMEOUT)
	{
		wzAsyncExecOnMainThread([failureCb, statusCopy]()
		{
			failureCb(statusCopy);
		});
		return;
	}
}

void PortMappingManager::thread_monitor_function()
{
	using namespace std::chrono_literals;

	while (!stop_requested_.load(std::memory_order_acquire))
	{
		if (status_ != DiscoveryStatus::IN_PROGRESS)
		{
			break;
		}
		// Check if we're past the deadline.
		auto currentTime = ClockType::now();
		if (currentTime >= deadline_) {
			// If so, exit the loop and notify the system that the timeout has occurred.
			set_discovery_failure(DiscoveryStatus::TIMEOUT);
		}
		// Sleep for another 50ms.
		std::this_thread::sleep_for(50ms);
	}
	// Someone has requested to stop the monitoring thread. So, the discovery process
	// has finished either with success or failure.
	//
	// Also, the status should already be set by the time we get there.
	// The only case in which the status may still be `IN_PROGRESS` is forcefully being shut down
	// by the higher level code via `shutdown()` method call.

	CallbackVector callbacks;
	{
		std::lock_guard<std::mutex> guard {mtx_};
		callbacks = callbacks_;
	}
	if (callbacks.empty())
	{
		return;
	}
	// Schedule callbacks in a single batch for better efficiency.
	switch (status_)
	{
	case DiscoveryStatus::SUCCESS:
		wzAsyncExecOnMainThread([callbacks, externalHost = discoveredIPaddress_, externalPort = discoveredPort_]()
		{
			for (auto& cb : callbacks)
			{
				cb.first(externalHost, externalPort);
			}
		});
		break;
	case DiscoveryStatus::FAILURE:
	case DiscoveryStatus::TIMEOUT:
		wzAsyncExecOnMainThread([callbacks, status = status_]()
		{
			for (auto& cb : callbacks)
			{
				cb.second(status);
			}
		});
		break;
	case DiscoveryStatus::IN_PROGRESS:
		// We may be terminated via `shutdown()` - don't attempt to do anything.
		break;
	default:
		ASSERT(false, "Should be unreachable");
	}
}

void PortMappingManager::set_discovery_success(std::string externalIp, uint16_t externalPort)
{
	{
		const std::lock_guard<std::mutex> guard {mtx_};
		if (status_ != DiscoveryStatus::IN_PROGRESS)
		{
			// Someone has already gotten ahead of us - nothing to do there.
			return;
		}
		status_ = DiscoveryStatus::SUCCESS;
		deadline_ = TimePoint::max();
		discoveredIPaddress_ = std::move(externalIp);
		discoveredPort_ = externalPort;
	}
	stop_requested_.store(true, std::memory_order_release);
}

void PortMappingManager::set_discovery_failure(DiscoveryStatus status)
{
	ASSERT(status == DiscoveryStatus::FAILURE || status == DiscoveryStatus::TIMEOUT, "Status should either be failure or timeout");
	{
		const std::lock_guard<std::mutex> guard {mtx_};
		if (status_ != DiscoveryStatus::IN_PROGRESS)
		{
			// Someone has already gotten ahead of us - nothing to do there.
			return;
		}
		status_ = status;
		deadline_ = TimePoint::max();
	}
	stop_requested_.store(true, std::memory_order_release);
}

constexpr int PortMappingManager::DISCOVERY_TIMEOUT_SECONDS;
