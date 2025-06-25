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
 * @file port_mapping_manager_impl_miniupnpc.h
 */
#pragma once

#include "port_mapping_manager.h"

#include <array>

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

	enum class DiscoveryStatus
	{
		UPNP_SUCCESS,
		UPNP_ERROR_DEVICE_NOT_FOUND,
		UPNP_ERROR_CONTROL_NOT_AVAILABLE
	};
private:
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
		PortMappingInternetProtocol protocol = PortMappingInternetProtocol::TCP_IPV4;
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
