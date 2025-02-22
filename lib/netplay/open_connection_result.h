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

#pragma once

#include <string>
#include <system_error>
#include <memory>
#include <functional>

#include "lib/netplay/client_connection.h"

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

// Higher-level functions for opening a connection / socket
struct OpenConnectionResult
{
public:
	OpenConnectionResult(std::error_code ec, std::string errorString)
		: errorCode(ec)
		, errorString(std::move(errorString))
	{ }

	OpenConnectionResult(IClientConnection* open_socket)
		: open_socket(open_socket)
	{ }

	bool hasError() const { return errorCode.has_value(); }

	OpenConnectionResult(const OpenConnectionResult& other) = delete; // non construction-copyable
	OpenConnectionResult& operator=(const OpenConnectionResult&) = delete; // non copyable
	OpenConnectionResult(OpenConnectionResult&&) = default;
	OpenConnectionResult& operator=(OpenConnectionResult&&) = default;

	struct ConnectionCloser
	{
		void operator() (IClientConnection* conn)
		{
			if (conn)
			{
				conn->close();
			}
		}
	};

	std::unique_ptr<IClientConnection, ConnectionCloser> open_socket;
	optional<std::error_code> errorCode = nullopt;
	std::string errorString;
};

typedef std::function<void(OpenConnectionResult&& result)> OpenConnectionToHostResultCallback;
