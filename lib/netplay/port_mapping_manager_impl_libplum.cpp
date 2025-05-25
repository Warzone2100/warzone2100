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
 * @file port_mapping_manager_impl_libplum.cpp
 * libplum-based port mapping implementation
 */

#include "lib/framework/wzapp.h"
#include "lib/framework/debug.h"

#include "port_mapping_manager_impl_libplum.h"

#include <plum/plum.h>

void PortMappingImpl_MappingCallbackSuccess(PortMappingImpl::Type type, int mappingId, const std::string& externalHost, uint16_t externalPort);
void PortMappingImpl_MappingCallbackFailure(PortMappingImpl::Type type, int mappingId);

// MARK: - Libplum implementation

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
		PortMappingImpl_MappingCallbackSuccess(PortMappingImpl::Type::Libplum, mappingId, mapping->external_host, mapping->external_port);
		break;

	case PLUM_STATE_FAILURE:
		debug(LOG_NET, "Mapping %d: failed\n", mappingId);
		PortMappingImpl_MappingCallbackFailure(PortMappingImpl::Type::Libplum, mappingId);
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
	if (protocol == PortMappingInternetProtocol::TCP_IPV4 || protocol == PortMappingInternetProtocol::TCP_IPV6)
	{
		m.protocol = PLUM_IP_PROTOCOL_TCP;
	}
	else // UDP_IPV4 / UDP_IPV6
	{
		m.protocol = PLUM_IP_PROTOCOL_UDP;
	}
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
