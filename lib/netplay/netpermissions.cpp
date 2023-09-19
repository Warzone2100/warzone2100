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

#include "netpermissions.h"
#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/string_ext.h"
#include "lib/framework/physfs_ext.h"

#include "netplay.h"

#include <deque>
#include <unordered_map>
#include <re2/re2.h>

#include "src/stdinreader.h"

// ////////////////////////////////////////////////////////////////////////
// Variables

std::deque<PLAYER_IP> IPlist;
bool loadedUserConfigBanList = false;

struct IdentityPermissions
{
	optional<ConnectPermissions> connectPermissions;

	bool hasAnyPermissionsSet();
};
std::unordered_map<std::string, IdentityPermissions> identityPermissions;

// ////////////////////////////////////////////////////////////////////////

static void NETloadUserConfigBanList();

// ////////////////////////////////////////////////////////////////////////

void NETpermissionsInit()
{
	NETloadUserConfigBanList();
}

void NETpermissionsShutdown()
{
	IPlist.clear();
}

// ////////////////////////////////////////////////////////////////////////

static void NETloadUserConfigBanList()
{
	loadedUserConfigBanList = true;
	char BanListPath[4096] = {0};
	strncpy(BanListPath, PHYSFS_getWriteDir(), 4095);
	size_t BanListAppendFname = strlen(BanListPath);
	strncpy(BanListPath+BanListAppendFname, "/banlist.txt", 4095-BanListAppendFname);
	FILE* f = fopen(BanListPath, "r");
	if(f == NULL) {
		return;
	}
	debug(LOG_INFO, "Reading banlist file: [%s]\n", BanListPath);
	char BanStringBuf[2048] = {0};
	char ToBanIP[256] = {0};
	char ToBanName[256] = {0};
	while(fgets(BanStringBuf, sizeof(BanStringBuf)-1, f)) {
		if(sscanf(BanStringBuf, "%255s %255[^\n]", ToBanIP, ToBanName) != 2) {
			if(strlen(BanStringBuf) > 2) {
				debug(LOG_ERROR, "Error reading banlist file!\n");
			}
		} else {
			addIPToBanList(ToBanIP, ToBanName);
		}
	}
}

bool isLoopbackIP(const char *ip)
{
	if (!ip) return false;
	if (strncmp(ip, "127.", 4) == 0)
	{
		return true;
	}
	if (strcmp(ip, "::1") == 0)
	{
		return true;
	}
	if (strcmp(ip, "0000:0000:0000:0000:0000:0000:0000:0001") == 0)
	{
		return true;
	}
	return false;
}

/**
 * Check if ip is on the banned list.
 * \param ip IP address converted to text
 */
bool onBanList(const char *ip)
{
	if (IPlist.empty())
	{
		return false;    //if no bans are added, then don't check.
	}
	if (isLoopbackIP(ip))
	{
		return false;	// ignore loopback IPs
	}
	for (const auto& entry : IPlist)
	{
		if (RE2::FullMatch(ip, entry.IPAddress))
		{
			return true;
		}
	}
	wz_command_interface_output("WZEVENT: bancheck: %s\n", ip);
	return false;
}

/**
 * Create the banned list.
 * \param ip IP address in text format
 * \param name Name of the player we are banning
 */
void addIPToBanList(const char *ip, const char *name)
{
	if (isLoopbackIP(ip))
	{
		return;
	}
	PLAYER_IP newIP;
	sstrcpy(newIP.IPAddress, ip);
	sstrcpy(newIP.pname, name);
	IPlist.push_back(newIP);
	sync_counter.banned++;
	if (IPlist.size() > MAX_BANS)
	{
		debug(LOG_INFO, "We have exceeded %d bans, clearing oldest", MAX_BANS);
		IPlist.pop_front();
	}
}

bool removeIPFromBanList(const char *ip)
{
	auto it = std::find_if(IPlist.begin(), IPlist.end(), [ip](const PLAYER_IP& ipInfo) -> bool {
		return strcmp(ipInfo.IPAddress, ip) == 0;
	});
	if (it == IPlist.end())
	{
		return false;
	}
	IPlist.erase(it);
	return true;
}

std::vector<PLAYER_IP> NETgetIPBanList()
{
	std::vector<PLAYER_IP> vecCopy;
	vecCopy.reserve(IPlist.size());
	vecCopy.assign(IPlist.begin(), IPlist.end());
	return vecCopy;
}

// ////////////////////////////////////////////////////////////////////////

bool IdentityPermissions::hasAnyPermissionsSet()
{
	return connectPermissions.has_value();
}

// identityStr: Either the the (b64) public key (preferred) or the public hash
void netPermissionsSet_Connect(const std::string& identityStr, ConnectPermissions perm)
{
	if (identityStr.empty()) { return; }
	identityPermissions[identityStr].connectPermissions = perm;
}

bool netPermissionsUnset_Connect(const std::string& identityStr)
{
	auto it = identityPermissions.find(identityStr);
	if (it == identityPermissions.end())
	{
		return false;
	}
	if (!it->second.connectPermissions.has_value())
	{
		return false;
	}
	it->second.connectPermissions.reset();
	if (!it->second.hasAnyPermissionsSet())
	{
		identityPermissions.erase(it);
	}
	return true;
}

bool netPermissionsRemoveAll(const std::string& identityStr)
{
	return identityPermissions.erase(identityStr) > 0;
}

optional<ConnectPermissions> netPermissionsCheck_Connect(const std::string& identityStr)
{
	auto it = identityPermissions.find(identityStr);
	if (it == identityPermissions.end())
	{
		return nullopt;
	}
	return it->second.connectPermissions;
}

optional<ConnectPermissions> netPermissionsCheck_Connect(const EcKey& identity)
{
	// Check both the (b64) public key and the public hash
	// (b64 public key always takes precedence)
	std::string checkPublicKeyB64 = base64Encode(identity.toBytes(EcKey::Public));
	auto result = netPermissionsCheck_Connect(checkPublicKeyB64);
	if (result.has_value())
	{
		return result;
	}
	std::string checkIdentityHash = identity.publicHashString();
	result = netPermissionsCheck_Connect(checkIdentityHash);
	return result;
}
