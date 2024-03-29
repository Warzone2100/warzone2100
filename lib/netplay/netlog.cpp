/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
// ////////////////////////////////////////////////////////////////////////
// Includes
#include "lib/framework/frame.h"

#include <time.h>
#include <physfs.h>
#include "lib/framework/physfs_ext.h"
#include "lib/framework/wztime.h"

#include "netlog.h"
#include "netplay.h"
#include "netpermissions.h"

// ////////////////////////////////////////////////////////////////////////
// Logging for debug only
// ////////////////////////////////////////////////////////////////////////

#define NUM_GAME_PACKETS 256

static PHYSFS_file	*pFileHandle = nullptr;
static uint32_t		packetcount[2][NUM_GAME_PACKETS];
static uint32_t		packetsize[2][NUM_GAME_PACKETS];

bool NETstartLogging(void)
{
	time_t aclock;
	char buf[256];
	static char filename[256] = {'\0'};
	int i;

	for (i = 0; i < NUM_GAME_PACKETS; i++)
	{
		packetcount[0][i] = 0;
		packetsize[0][i] = 0;
		packetcount[1][i] = 0;
		packetsize[1][i] = 0;
	}

	time(&aclock);                   /* Get time in seconds */
	auto newtime = getLocalTime(aclock);    /* Convert time to struct */

	snprintf(filename, sizeof(filename), "logs/netplay-%04d%02d%02d_%02d%02d%02d.log", newtime.tm_year + 1900, newtime.tm_mon + 1, newtime.tm_mday, newtime.tm_hour, newtime.tm_min, newtime.tm_sec);
	pFileHandle = PHYSFS_openWrite(filename);   // open the file
	if (!pFileHandle)
	{
		debug(LOG_ERROR, "Could not create net log %s: %s", filename,
		      WZ_PHYSFS_getLastError());
		return false;
	}
	snprintf(buf, sizeof(buf), "NETPLAY log: %s\n", getAscTime(newtime).c_str());
	WZ_PHYSFS_writeBytes(pFileHandle, buf, static_cast<PHYSFS_uint32>(strlen(buf)));
	return true;
}

bool NETstopLogging(void)
{
	static const char dash_line[] = "-----------------------------------------------------------\n";
	char buf[256];
	int i;
	UDWORD totalBytessent = 0, totalBytesrecv = 0, totalPacketsent = 0, totalPacketrecv = 0;

	if (!pFileHandle)
	{
		return false;
	}

	/* Output stats */
	for (i = 0; i < NUM_GAME_PACKETS; i++)
	{
		if (!strcmp(messageTypeToString(i), "(UNUSED)"))
		{
			continue;
		}
		else
		{
			snprintf(buf, sizeof(buf), "%-24s:\t received %u times, %u bytes; sent %u times, %u bytes\n", messageTypeToString(i),
			         packetcount[1][i], packetsize[1][i], packetcount[0][i], packetsize[0][i]);
		}
		WZ_PHYSFS_writeBytes(pFileHandle, buf, static_cast<PHYSFS_uint32>(strlen(buf)));
		totalBytessent += packetsize[0][i];
		totalBytesrecv += packetsize[1][i];
		totalPacketsent += packetcount[0][i];
		totalPacketrecv += packetcount[1][i];
	}
	snprintf(buf, sizeof(buf), "== Total bytes sent %u, Total bytes received %u ==\n", totalBytessent, totalBytesrecv);
	WZ_PHYSFS_writeBytes(pFileHandle, buf, static_cast<PHYSFS_uint32>(strlen(buf)));
	snprintf(buf, sizeof(buf), "== Total packets sent %u, recv %u ==\n", totalPacketsent, totalPacketrecv);
	WZ_PHYSFS_writeBytes(pFileHandle, buf, static_cast<PHYSFS_uint32>(strlen(buf)));
	snprintf(buf, sizeof(buf), "\n-Sync statistics -\n");
	WZ_PHYSFS_writeBytes(pFileHandle, buf, static_cast<PHYSFS_uint32>(strlen(buf)));
	WZ_PHYSFS_writeBytes(pFileHandle, dash_line, static_cast<PHYSFS_uint32>(strlen(dash_line)));
	snprintf(buf, sizeof(buf), "joins: %u, kicks: %u, drops: %u, left %u\n", sync_counter.joins, sync_counter.kicks, sync_counter.drops, sync_counter.left);
	WZ_PHYSFS_writeBytes(pFileHandle, buf, static_cast<PHYSFS_uint32>(strlen(buf)));
	snprintf(buf, sizeof(buf), "banned: %u, cantjoin: %u, rejected: %u\n", sync_counter.banned, sync_counter.cantjoin, sync_counter.rejected);
	WZ_PHYSFS_writeBytes(pFileHandle, buf, static_cast<PHYSFS_uint32>(strlen(buf)));
	if (sync_counter.banned)
	{
		auto banList = NETgetIPBanList();
		if (!banList.empty())
		{
			snprintf(buf, sizeof(buf), "Banned list:\n");
			WZ_PHYSFS_writeBytes(pFileHandle, buf, static_cast<PHYSFS_uint32>(strlen(buf)));
			for (const auto& banDetails : banList)
			{
				if (banDetails.IPAddress[0] != '\0')
				{
					snprintf(buf, sizeof(buf), "player %s, IP: %s\n", banDetails.pname, banDetails.IPAddress);
					WZ_PHYSFS_writeBytes(pFileHandle, buf, static_cast<PHYSFS_uint32>(strlen(buf)));
				}
			}
		}

	}
	WZ_PHYSFS_writeBytes(pFileHandle, dash_line, static_cast<PHYSFS_uint32>(strlen(dash_line)));
	WZ_PHYSFS_writeBytes(pFileHandle, dash_line, static_cast<PHYSFS_uint32>(strlen(dash_line)));

	if (!PHYSFS_close(pFileHandle))
	{
		debug(LOG_ERROR, "Could not close net log: %s", WZ_PHYSFS_getLastError());
		return false;
	}
	pFileHandle = nullptr;

	return true;
}

/** log packet
 *  \param type, uint8_t, the packet's type.
 *  \param size, uint32_t, the packet's size
 *  \param received, bool, true if we are receiving a packet, false if we are sending a packet.
*/
void NETlogPacket(uint8_t type, uint32_t size, bool received)
{
	STATIC_ASSERT((1 << (8 * sizeof(type))) == NUM_GAME_PACKETS); // NUM_GAME_PACKETS must be larger than maximum possible type.
	packetcount[received][type]++;
	packetsize[received][type] += size;
}

bool NETlogEntry(const char *str, UDWORD a, UDWORD b)
{
	static const char star_line[] = "************************************************************\n";
	static UDWORD lastframe = 0;
	UDWORD frame = frameGetFrameNumber();
	time_t aclock;
	char buf[256];

	if (!pFileHandle)
	{
		return false;
	}

	time(&aclock);					/* Get time in seconds */
	auto newtime_result = getLocalTimeOpt(aclock);		/* Convert time to struct */

	if (!newtime_result.has_value() || !pFileHandle)
	{
		debug(LOG_ERROR, "Fatal error averted in NETlog");
		return false;
	}

	struct tm newtime = newtime_result.value();

	// check to see if a new frame.
	if (frame != lastframe)
	{
		static const char dash_line[] = "-----------------------------------------------------------\n";

		lastframe = frame;
		WZ_PHYSFS_writeBytes(pFileHandle, dash_line, static_cast<PHYSFS_uint32>(strlen(dash_line)));
	}

	if (a < NUM_GAME_PACKETS)
		// replace common msgs with txt descriptions
	{
		snprintf(buf, sizeof(buf), "%s # %s \t: %s \t:%d\n", getAscTime(newtime).c_str(), str, messageTypeToString(a), b);
	}
	else if (a == SYNC_FLAG)
	{
		snprintf(buf, sizeof(buf), "%s # %s \t: %d \t(Sync)\n", getAscTime(newtime).c_str(), str, b);
	}
	else
	{
		snprintf(buf, sizeof(buf), "%s # %s \t:%d \t\t\t:%d\n", getAscTime(newtime).c_str(), str, a, b);
	}

	if (a == NET_PLAYER_LEAVING || a == NET_PLAYER_DROPPED)
	{
		// Write a starry line above NET_LEAVING messages
		WZ_PHYSFS_writeBytes(pFileHandle, star_line, static_cast<PHYSFS_uint32>(strlen(star_line)));
		WZ_PHYSFS_writeBytes(pFileHandle, buf, static_cast<PHYSFS_uint32>(strlen(buf)));
		WZ_PHYSFS_writeBytes(pFileHandle, star_line, static_cast<PHYSFS_uint32>(strlen(star_line)));
	}
	else
	{
		WZ_PHYSFS_writeBytes(pFileHandle, buf, static_cast<PHYSFS_uint32>(strlen(buf)));
	}

	PHYSFS_flush(pFileHandle);
	return true;
}
