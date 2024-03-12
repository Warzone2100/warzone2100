/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2023  Warzone 2100 Project

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

#ifndef _netpermissions_h
#define _netpermissions_h

#include <string>
#include <vector>

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

class EcKey;

struct PLAYER_IP
{
	char	pname[40] = {};
	char	IPAddress[40] = {};
};
#define MAX_BANS 1024

void NETpermissionsInit();
void NETpermissionsShutdown();

void addIPToBanList(const char *ip, const char *name);
bool removeIPFromBanList(const char *ip);
bool onBanList(const char *ip);
bool isLoopbackIP(const char *ip);
std::vector<PLAYER_IP> NETgetIPBanList();

enum class ConnectPermissions
{
	Blocked,
	Allowed
};

// identityStr: Either the the (b64) public key (preferred) or the public hash
void netPermissionsSet_Connect(const std::string& identityStr, ConnectPermissions perm);
bool netPermissionsUnset_Connect(const std::string& identityStr);
bool netPermissionsRemoveAll(const std::string& identityStr);

optional<ConnectPermissions> netPermissionsCheck_Connect(const std::string& identityStr);
optional<ConnectPermissions> netPermissionsCheck_Connect(const EcKey& identity);

#endif // _netpermissions_h
