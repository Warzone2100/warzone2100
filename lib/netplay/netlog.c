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
	sprintf( buf, "NETPLAY log: %s\n", asctime( newtime ) );
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
		sprintf(buf, "%s: received %u times, %u bytes; sent %u times, %u bytes\n", packetname[i],
			packetcount[0][i], packetsize[0][i], packetcount[1][i], packetsize[1][i]);
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
		lastframe = frame;
		sprintf( buf, "-----------------------------------------------------------\n" );
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
	}

	switch(a)		// replace common msgs with txt descriptions
	{
	case 1:
		sprintf( buf, "%s \t: NET_DROIDINFO  \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	case 2:
		sprintf( buf, "%s \t: NET_DROIDDEST  \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	case 3:
		sprintf( buf, "%s \t: NET_DROIDMOVE  \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	case 4:
		sprintf( buf, "%s \t: NET_GROUPORDER  \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	case 8:
		sprintf( buf, "%s \t: NET_PING \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	case 9:
		sprintf( buf, "%s \t: NET_CHECK_DROID \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	case 10:
		sprintf( buf, "%s \t: NET_CHECK_STRUCT \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	case 11:
		sprintf( buf, "%s \t: NET_CHECK_POWER \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	case 13:
		sprintf( buf, "%s \t: NET_BUILD \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	case 15:
		sprintf( buf, "%s \t: NET_BUILDFINISHED \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	case 17:
		sprintf( buf, "%s \t: NET_TXTMSG \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	case 18:
		sprintf( buf, "************************************************************\n");
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		sprintf( buf, "%s \t: NET_LEAVING \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		sprintf( buf, "************************************************************\n");
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	case 19:
		sprintf( buf, "%s \t: NET_REQUESTDROID \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	case 23:
		sprintf( buf, "%s \t: NET_WHOLEDROID \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	case 22:
		sprintf( buf, "%s \t: NET_STRUCT (Whole) \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	case 25:
		sprintf( buf, "%s \t: NET_PLAYERRESPONDING \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	case 26:
		sprintf( buf, "%s \t: NET_OPTIONS \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	case 27:
		sprintf( buf, "%s \t: NET_WAYPOINT \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	case 28:
		sprintf( buf, "%s \t: NET_SECONDARY \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	case 29:
		sprintf( buf, "%s \t: NET_FIREUP \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	case 34:
		sprintf( buf, "%s \t: NET_ARTIFACTS \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	case 36:
		sprintf( buf, "%s \t: NET_SCORESUBMIT \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	case 37:
		sprintf( buf, "%s \t: NET_DESTROYXTRA \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	case 38:
		sprintf( buf, "%s \t: NET_VTOL \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	case 39:
		sprintf( buf, "%s \t: NET_VTOLREARM \t:%d\t\t%s",str,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;

	default:
		sprintf( buf, "%s \t:%d \t\t\t:%d\t\t%s",str,a,b,asctime( newtime ));
		PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
		break;
	}
	PHYSFS_flush(pFileHandle);
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
