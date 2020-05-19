/*
	This file is part of Warzone 2100.
	Copyright (C) 2020  Warzone 2100 Project

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

#ifndef __INCLUDED_WZ_NET_HELPERS_H__
#define __INCLUDED_WZ_NET_HELPERS_H__

#include <functional>
#include <string>
#include <vector>

#define WZ_DEFAULT_PUBLIC_IPv4_LOOKUP_SERVICE_URL "https://api.ipify.org?format=json"
#define WZ_DEFAULT_PUBLIC_IPv4_LOOKUP_SERVICE_JSONKEY "ip"
#define WZ_DEFAULT_PUBLIC_IPv6_LOOKUP_SERVICE_URL "https://api6.ipify.org?format=json"
#define WZ_DEFAULT_PUBLIC_IPv6_LOOKUP_SERVICE_JSONKEY "ip"

typedef std::function<void (const std::string& ipv4Address, const std::string& ipv6Address, const std::vector<std::string>& errors)> IPAddressResultCallbackFunc;

// Asynchronously query the current device's public IP address(es), using a webservice
// The `resultCallback` may be called on any thread
void getPublicIPAddress(const IPAddressResultCallbackFunc& resultCallback, bool ipv4 = true, bool ipv6 = true);

void setPublicIPv4LookupService(const std::string& publicIPv4LookupServiceUrl, const std::string& jsonKey);
const std::string& getPublicIPv4LookupServiceUrl();
const std::string& getPublicIPv4LookupServiceJSONKey();

void setPublicIPv6LookupService(const std::string& publicIPv6LookupServiceUrl, const std::string& jsonKey);
const std::string& getPublicIPv6LookupServiceUrl();
const std::string& getPublicIPv6LookupServiceJSONKey();

#endif // __INCLUDED_WZ_NET_HELPERS_H__
