// ////////////////////////////////////////////////////////////////////////
// Includes
#include "lib/framework/frame.h"
#include "netplay.h"
#include "netlog.h"

#include <time.h>
#include <stdio.h>
#include <physfs.h>

// ////////////////////////////////////////////////////////////////////////
// Logging for degug only
// ////////////////////////////////////////////////////////////////////////

static PHYSFS_file	*pFileHandle;

BOOL NETstartLogging()
{
	time_t aclock;
	struct tm *newtime;
	char buf[256];

	time( &aclock );                 /* Get time in seconds */
	newtime = localtime( &aclock );  /* Convert time to struct */

	pFileHandle = PHYSFS_openWrite( "netplay.log" ); // open the file
	if (!pFileHandle)
	{
		return FALSE;
	}
	sprintf( buf, "NETPLAY log: %s\n", asctime( newtime ) );
	PHYSFS_write( pFileHandle, buf, strlen( buf ), 1 );
	return TRUE;
}

BOOL NETstopLogging()
{
	if (!PHYSFS_close(pFileHandle))
	{
		debug(LOG_ERROR, "Could not close net log: %s", PHYSFS_getLastError());
		return FALSE;
	}
	return TRUE;
}


BOOL NETlogEntry(CHAR *str,UDWORD a,UDWORD b)
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
