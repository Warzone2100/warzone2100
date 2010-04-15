/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2009  Warzone Resurrection Project

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

#include "netlog.h"
#include "netplay.h"

// ////////////////////////////////////////////////////////////////////////
// Logging for degug only
// ////////////////////////////////////////////////////////////////////////

static const char *packetname[NUM_GAME_PACKETS] =
{
	"NET_DROID",
	"NET_DROIDINFO",
	"NET_DROIDDEST",
	"NET_DROIDMOVE",
	"NET_GROUPORDER",
	"NET_TEMPLATE",
	"NET_TEMPLATEDEST",
	"NET_FEATUREDEST",
	"NET_PING",
	"NET_CHECK_DROID",
	"NET_CHECK_STRUCT",
	"NET_CHECK_POWER",
	"NET_PLAYER_STATS",
	"NET_BUILD",
	"NET_STRUCTDEST",
	"NET_BUILDFINISHED",
	"NET_RESEARCH",
	"NET_TEXTMSG",
	"NET_UNUSED_18",
	"NET_UNUSED_19",
	"NET_PLAYERCOMPLETE",
	"NET_UNUSED_21",
	"NET_STRUCT",
	"NET_UNUSED_23",
	"NET_FEATURES",
	"NET_PLAYERRESPONDING",
	"NET_OPTIONS",
	"NET_KICK",
	"NET_SECONDARY",
	"NET_FIREUP",
	"NET_ALLIANCE",
	"NET_GIFT",
	"NET_DEMOLISH",
	"NET_COLOURREQUEST",
	"NET_ARTIFACTS",
	"NET_DMATCHWIN",
	"NET_SCORESUBMIT",
	"NET_DESTROYXTRA",
	"NET_VTOL",
	"NET_UNUSED_39",
	"NET_WHITEBOARD",
	"NET_SECONDARY_ALL",
	"NET_DROIDEMBARK",
	"NET_DROIDDISEMBARK",
	"NET_RESEARCHSTATUS",
	"NET_LASSAT",
	"NET_UNUSED_46",
	"NET_AITEXTMSG",
	"NET_TEAMS_ON",
	"NET_BEACONMSG",
	"NET_SET_TEAMS",
	"NET_TEAMREQUEST",
	"NET_JOIN",
	"NET_ACCEPTED",
	"NET_PLAYER_INFO",
	"NET_PLAYER_JOINED",
	"NET_PLAYER_LEAVING",
	"NET_PLAYER_DROPPED",
	"NET_GAME_FLAGS",
	"NET_READY_REQUEST",
	"NET_NEVERUSE",
	"NET_REJECTED",
	"NET_UNUSED_62",
	"NET_UNUSED_63",
	"NET_UNUSED_64",
	"NET_POSITIONREQUEST",
	"NET_DATA_CHECK",
	"NET_HOST_DROPPED",
	"NET_FUTURE1",
	"NET_FUTURE2",
	"NET_FUTURE3",
	"NET_FILE_REQUESTED",
	"NET_FILE_CANCELLED",
	"NET_FILE_PAYLOAD"
};

static PHYSFS_file	*pFileHandle = NULL;
static uint32_t		packetcount[2][NUM_GAME_PACKETS];
static uint32_t		packetsize[2][NUM_GAME_PACKETS];

BOOL NETstartLogging(void)
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

	time( &aclock );                 /* Get time in seconds */
	newtime = localtime( &aclock );  /* Convert time to struct */

	snprintf(filename, sizeof(filename), "logs/netplay-%02d%02d_%02d%02d%02d.log", newtime->tm_mon, newtime->tm_mday, newtime->tm_hour, newtime->tm_min, newtime->tm_sec );
	pFileHandle = PHYSFS_openWrite( filename ); // open the file
	if (!pFileHandle)
	{
		debug(LOG_ERROR, "Could not create net log %s: %s", filename,
		      PHYSFS_getLastError());
		return false;
	}
	snprintf(buf, sizeof(buf), "NETPLAY log: %s\n", asctime(newtime));
	PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
	return true;
}

BOOL NETstopLogging(void)
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
		snprintf(buf, sizeof(buf), "%-24s:\t received %u times, %u bytes; sent %u times, %u bytes\n", packetname[i],
			packetcount[0][i], packetsize[0][i], packetcount[1][i], packetsize[1][i]);
		PHYSFS_write(pFileHandle, buf, strlen(buf), 1);
		totalBytessent += packetsize[1][i];
		totalBytesrecv += packetsize[0][i];
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
	snprintf(buf, sizeof(buf), "joins: %hhu, kicks: %hhu, drops: %hhu, left %hhu\n", sync_counter.joins, sync_counter.kicks, sync_counter.drops, sync_counter.left );
	PHYSFS_write(pFileHandle, buf, strlen(buf), 1);
	snprintf(buf, sizeof(buf), "banned: %hhu, cantjoin: %hhu, rejected: %hhu\n", sync_counter.banned, sync_counter.cantjoin, sync_counter.rejected );
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
	snprintf(buf, sizeof(buf), "sent/unsent DroidCheck %"PRIu64" / %"PRIu64"\n", sync_counter.sentDroidCheck, sync_counter.unsentDroidCheck);
	PHYSFS_write(pFileHandle, buf, strlen(buf), 1);
	snprintf(buf, sizeof(buf), "sent/unsent StructureCheck %"PRIu64" / %"PRIu64"\n", sync_counter.sentStructureCheck, sync_counter.unsentStructureCheck);
	PHYSFS_write(pFileHandle, buf, strlen(buf), 1);
	snprintf(buf, sizeof(buf), "sent/unsent PowerCheck %"PRIu64" / %"PRIu64"\n", sync_counter.sentPowerCheck, sync_counter.unsentPowerCheck);
	PHYSFS_write(pFileHandle, buf, strlen(buf), 1);
	snprintf(buf, sizeof(buf), "sent/unsent ScoreCheck %"PRIu64" / %"PRIu64"\n", sync_counter.sentScoreCheck, sync_counter.unsentScoreCheck);
	PHYSFS_write(pFileHandle, buf, strlen(buf), 1);
	snprintf(buf, sizeof(buf), "sent/unsent Ping %"PRIu64" / %"PRIu64"\n", sync_counter.sentPing, sync_counter.unsentPing);
	PHYSFS_write(pFileHandle, buf, strlen(buf), 1);
	snprintf(buf, sizeof(buf), "sent/unsent isMPDirtyBit %"PRIu64" / %"PRIu64"\n", sync_counter.sentisMPDirtyBit, sync_counter.unsentisMPDirtyBit);
	PHYSFS_write(pFileHandle, buf, strlen(buf), 1);
	PHYSFS_write(pFileHandle, dash_line, strlen(dash_line), 1);

	if (!PHYSFS_close(pFileHandle))
	{
		debug(LOG_ERROR, "Could not close net log: %s", PHYSFS_getLastError());
		return false;
	}
	pFileHandle = NULL;

	return true;
}

void NETlogPacket(NETMSG *msg, BOOL received)
{
	if (msg->type >= NUM_GAME_PACKETS)
	{
		return;
	}
	packetcount[received][msg->type]++;
	packetsize[received][msg->type] += msg->size;
}

BOOL NETlogEntry(const char *str,UDWORD a,UDWORD b)
{
	static const char star_line[] = "************************************************************\n";
	static UDWORD lastframe = 0;
	UDWORD frame= frameGetFrameNumber();
	time_t aclock;
	struct tm *newtime;
	char buf[256];

	if (!pFileHandle)
	{
		return false;
	}

	time( &aclock );					/* Get time in seconds */
	newtime = localtime( &aclock );		/* Convert time to struct */

	if (!newtime || !str || !pFileHandle)
	{
		debug(LOG_ERROR, "Fatal error averted in NETlog");
		return false;
	}

	// check to see if a new frame.
	if(frame != lastframe)
	{
		static const char dash_line[] = "-----------------------------------------------------------\n";

		lastframe = frame;
		PHYSFS_write(pFileHandle, dash_line, strlen(dash_line), 1);
	}

	if (a < NUM_GAME_PACKETS)
		// replace common msgs with txt descriptions
		snprintf(buf, sizeof(buf), "%s \t: %s \t:%d\t\t%s", str, packetname[a], b, asctime(newtime));
	else if (a == SYNC_FLAG)
		snprintf(buf, sizeof(buf), "%s \t: %d \t(Sync) \t%s", str, b, asctime(newtime));
	else
		snprintf(buf, sizeof(buf), "%s \t:%d \t\t\t:%d\t\t%s", str, a, b, asctime(newtime));

	if (a == NET_PLAYER_LEAVING || a == NET_PLAYER_DROPPED )
	{
		// Write a starry line above NET_LEAVING messages
		PHYSFS_write(pFileHandle, star_line, strlen(star_line), 1);
		PHYSFS_write(pFileHandle, buf, strlen( buf ), 1);
		PHYSFS_write(pFileHandle, star_line, strlen(star_line), 1);
	}
	else
	{
		PHYSFS_write(pFileHandle, buf, strlen( buf ), 1);
	}

	PHYSFS_flush(pFileHandle);
	return true;
}
