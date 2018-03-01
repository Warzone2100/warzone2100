/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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

#include "netlog.h"
#include "netplay.h"

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
	struct tm *newtime;
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
	newtime = localtime(&aclock);    /* Convert time to struct */

	snprintf(filename, sizeof(filename), "logs/netplay-%04d%02d%02d_%02d%02d%02d.log", newtime->tm_year + 1900, newtime->tm_mon + 1, newtime->tm_mday, newtime->tm_hour, newtime->tm_min, newtime->tm_sec);
	pFileHandle = PHYSFS_openWrite(filename);   // open the file
	if (!pFileHandle)
	{
		debug(LOG_ERROR, "Could not create net log %s: %s", filename,
		      WZ_PHYSFS_getLastError());
		return false;
	}
	snprintf(buf, sizeof(buf), "NETPLAY log: %s\n", asctime(newtime));
	PHYSFS_write(pFileHandle, buf, strlen(buf), 1);
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
		PHYSFS_write(pFileHandle, buf, strlen(buf), 1);
		totalBytessent += packetsize[0][i];
		totalBytesrecv += packetsize[1][i];
		totalPacketsent += packetcount[0][i];
		totalPacketrecv += packetcount[1][i];
	}
	snprintf(buf, sizeof(buf), "== Total bytes sent %u, Total bytes received %u ==\n", totalBytessent, totalBytesrecv);
	PHYSFS_write(pFileHandle, buf, strlen(buf), 1);
	snprintf(buf, sizeof(buf), "== Total packets sent %u, recv %u ==\n", totalPacketsent, totalPacketrecv);
	PHYSFS_write(pFileHandle, buf, strlen(buf), 1);
	snprintf(buf, sizeof(buf), "\n-Sync statistics -\n");
	PHYSFS_write(pFileHandle, buf, strlen(buf), 1);
	PHYSFS_write(pFileHandle, dash_line, strlen(dash_line), 1);
	snprintf(buf, sizeof(buf), "joins: %u, kicks: %u, drops: %u, left %u\n", sync_counter.joins, sync_counter.kicks, sync_counter.drops, sync_counter.left);
	PHYSFS_write(pFileHandle, buf, strlen(buf), 1);
	snprintf(buf, sizeof(buf), "banned: %u, cantjoin: %u, rejected: %u\n", sync_counter.banned, sync_counter.cantjoin, sync_counter.rejected);
	PHYSFS_write(pFileHandle, buf, strlen(buf), 1);
	if (sync_counter.banned && IPlist)
	{
		snprintf(buf, sizeof(buf), "Banned list:\n");
		PHYSFS_write(pFileHandle, buf, strlen(buf), 1);
		for (i = 0; i < MAX_BANS; i++)
		{
			if (IPlist[i].IPAddress[0] != '\0')
			{
				snprintf(buf, sizeof(buf), "player %s, IP: %s\n", IPlist[i].pname, IPlist[i].IPAddress);
				PHYSFS_write(pFileHandle, buf, strlen(buf), 1);
			}
		}

	}
	PHYSFS_write(pFileHandle, dash_line, strlen(dash_line), 1);
	PHYSFS_write(pFileHandle, dash_line, strlen(dash_line), 1);

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
	struct tm *newtime;
	char buf[256];

	if (!pFileHandle)
	{
		return false;
	}

	time(&aclock);					/* Get time in seconds */
	newtime = localtime(&aclock);		/* Convert time to struct */

	if (!newtime || !pFileHandle)
	{
		debug(LOG_ERROR, "Fatal error averted in NETlog");
		return false;
	}

	// check to see if a new frame.
	if (frame != lastframe)
	{
		static const char dash_line[] = "-----------------------------------------------------------\n";

		lastframe = frame;
		PHYSFS_write(pFileHandle, dash_line, strlen(dash_line), 1);
	}

	if (a < NUM_GAME_PACKETS)
		// replace common msgs with txt descriptions
	{
		snprintf(buf, sizeof(buf), "%s \t: %s \t:%d\t\t%s", str, messageTypeToString(a), b, asctime(newtime));
	}
	else if (a == SYNC_FLAG)
	{
		snprintf(buf, sizeof(buf), "%s \t: %d \t(Sync) \t%s", str, b, asctime(newtime));
	}
	else
	{
		snprintf(buf, sizeof(buf), "%s \t:%d \t\t\t:%d\t\t%s", str, a, b, asctime(newtime));
	}

	if (a == NET_PLAYER_LEAVING || a == NET_PLAYER_DROPPED)
	{
		// Write a starry line above NET_LEAVING messages
		PHYSFS_write(pFileHandle, star_line, strlen(star_line), 1);
		PHYSFS_write(pFileHandle, buf, strlen(buf), 1);
		PHYSFS_write(pFileHandle, star_line, strlen(star_line), 1);
	}
	else
	{
		PHYSFS_write(pFileHandle, buf, strlen(buf), 1);
	}

	PHYSFS_flush(pFileHandle);
	return true;
}
