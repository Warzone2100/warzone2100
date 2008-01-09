/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
#include "netplay.h"
#include "netlog.h"

#include <time.h>
#include <stdio.h>
#include <physfs.h>
#include <string.h>

// ////////////////////////////////////////////////////////////////////////
// Logging for degug only
// ////////////////////////////////////////////////////////////////////////

// kluge, since I do not want to include src/multiplay.h here
#define NET_MAX 52

static const char *packetname[NET_MAX] =
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
	"NET_VERSION",
	"NET_BUILD",
	"NET_STRUCTDEST",
	"NET_BUILDFINISHED",
	"NET_RESEARCH",
	"NET_TEXTMSG",
	"NET_LEAVING",
	"NET_REQUESTDROID",
	"NET_PLAYERCOMPLETE",
	"NET_REQUESTPLAYER",
	"NET_STRUCT",
	"NET_WHOLEDROID",
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
	"NET_REQUESTMAP",
	"NET_AITEXTMSG",
	"NET_TEAMS_ON",
	"NET_BEACONMSG",
	"NET_SET_TEAMS",
	"NET_TEAMREQUEST"
};

static PHYSFS_file	*pFileHandle;
static uint32_t		packetcount[2][NET_MAX];
static uint32_t		packetsize[2][NET_MAX];

BOOL NETstartLogging(void)
{
	time_t aclock;
	struct tm *newtime;
	char buf[256];
	char *filename = "netplay.log";
	int i;

	for (i = 0; i < NET_MAX; i++)
	{
		packetcount[0][i] = 0;
		packetsize[0][i] = 0;
		packetcount[1][i] = 0;
		packetsize[1][i] = 0;
	}

	time( &aclock );                 /* Get time in seconds */
	newtime = localtime( &aclock );  /* Convert time to struct */

	pFileHandle = PHYSFS_openWrite( filename ); // open the file
	if (!pFileHandle)
	{
		debug(LOG_ERROR, "Could not create net log %s: %s", filename,
		      PHYSFS_getLastError());
		return FALSE;
	}
	snprintf(buf, sizeof(buf), "NETPLAY log: %s\n", asctime(newtime));
	// Guarantee to nul-terminate
	buf[sizeof(buf) - 1] = '\0';
	PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
	return TRUE;
}

BOOL NETstopLogging(void)
{
	char buf[256];
	int i;

	/* Output stats */
	for (i = 0; i < NET_MAX; i++)
	{
		snprintf(buf, sizeof(buf), "%s: received %u times, %u bytes; sent %u times, %u bytes\n", packetname[i],
			packetcount[0][i], packetsize[0][i], packetcount[1][i], packetsize[1][i]);
		// Guarantee to nul-terminate
		buf[sizeof(buf) - 1] = '\0';
		PHYSFS_write(pFileHandle, buf, strlen(buf), 1);
	}

	if (!PHYSFS_close(pFileHandle))
	{
		debug(LOG_ERROR, "Could not close net log: %s", PHYSFS_getLastError());
		return FALSE;
	}

	return TRUE;
}

void NETlogPacket(NETMSG *msg, BOOL received)
{
	if (msg->type >= NET_MAX)
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

#ifndef MASSIVELOGS
	if(a ==9 || a==10)
	{
		return TRUE;
	}
#endif

	time( &aclock );                 /* Get time in seconds */
	newtime = localtime( &aclock );  /* Convert time to struct */

	// check to see if a new frame.
	if(frame != lastframe)
	{
		static const char dash_line[] = "-----------------------------------------------------------\n";

		lastframe = frame;

		PHYSFS_write(pFileHandle, dash_line, strlen(dash_line), 1);
	}

	if (a <= 51)
		// replace common msgs with txt descriptions
		snprintf(buf, sizeof(buf), "%s \t: %s \t:%d\t\t%s", str, packetname[a], b, asctime(newtime));
	else
		snprintf(buf, sizeof(buf), "%s \t:%d \t\t\t:%d\t\t%s", str, a, b, asctime(newtime));

	// Guarantee to nul-terminate
	buf[sizeof(buf) - 1] = '\0';

	if (a == 18) // NET_LEAVING
		// Write a starry line above NET_LEAVING messages
		PHYSFS_write(pFileHandle, star_line, strlen(star_line), 1);

	PHYSFS_write(pFileHandle, buf, strlen( buf ), 1);

	if (a == 18) // NET_LEAVING
		// Write a starry line below NET_LEAVING messages
		PHYSFS_write(pFileHandle, star_line, strlen(star_line), 1);

	PHYSFS_flush(pFileHandle);
	return TRUE;
}
