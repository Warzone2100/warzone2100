/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project

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

#pragma once

#include <string>

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

#include "lib/framework/wzglobal.h" // for WZ_GNS_NETWORK_BACKEND_ENABLED

#include <cstdint>

struct JoinConnectionDescription
{
public:
	JoinConnectionDescription()
	{ }
public:
	enum class JoinConnectionType
	{
		TCP_DIRECT,
#ifdef WZ_GNS_NETWORK_BACKEND_ENABLED
		GNS_DIRECT,
#endif
	};
public:
	JoinConnectionDescription(const std::string& host, uint32_t port)
	: host(host)
	, port(port)
	, type(JoinConnectionType::TCP_DIRECT)
	{ }
	JoinConnectionDescription(JoinConnectionType t, const std::string& host, uint32_t port)
	: host(host)
	, port(port)
	, type(t)
	{ }
public:
	static std::string connectiontype_to_string(JoinConnectionType type);
	static optional<JoinConnectionType> connectiontype_from_string(const std::string& str);
public:
	std::string host;
	uint32_t port = 0;
	JoinConnectionType type = JoinConnectionType::TCP_DIRECT;
};
