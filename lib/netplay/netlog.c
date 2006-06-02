// ////////////////////////////////////////////////////////////////////////
// Includes
#include "lib/framework/frame.h"
#include "netplay.h"
#include "netlog.h"

#include <time.h>
#include <stdio.h>

// ////////////////////////////////////////////////////////////////////////
// Logging for degug only
// ////////////////////////////////////////////////////////////////////////

static FILE	*pFileHandle;

BOOL NETstartLogging()
{
	time_t aclock;
	struct tm *newtime;

	time( &aclock );                 /* Get time in seconds */
	newtime = localtime( &aclock );  /* Convert time to struct */

	pFileHandle = fopen("netplay.log", "w"); // open the file
	if (!pFileHandle)
	{
		return FALSE;
	}
	fprintf(pFileHandle,"NETPLAY log: %s\n",asctime( newtime ) );
	return TRUE;
}

BOOL NETstopLogging()
{
	if (fclose(pFileHandle) != 0)
	{
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
		fprintf(pFileHandle,"-----------------------------------------------------------\n");
	}

	switch(a)		// replace common msgs with txt descriptions
	{
	case 1:
		fprintf(pFileHandle,"%s \t: NET_DROIDINFO  \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 2:
		fprintf(pFileHandle,"%s \t: NET_DROIDDEST  \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 3:
		fprintf(pFileHandle,"%s \t: NET_DROIDMOVE  \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 4:
		fprintf(pFileHandle,"%s \t: NET_GROUPORDER  \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 8:
		fprintf(pFileHandle,"%s \t: NET_PING \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 9:
		fprintf(pFileHandle,"%s \t: NET_CHECK_DROID \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 10:
		fprintf(pFileHandle,"%s \t: NET_CHECK_STRUCT \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 11:
		fprintf(pFileHandle,"%s \t: NET_CHECK_POWER \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 13:
		fprintf(pFileHandle,"%s \t: NET_BUILD \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 15:
		fprintf(pFileHandle,"%s \t: NET_BUILDFINISHED \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 17:
		fprintf(pFileHandle,"%s \t: NET_TXTMSG \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 18:
		fprintf(pFileHandle,"************************************************************\n");
		fprintf(pFileHandle,"%s \t: NET_LEAVING \t:%d\t\t%s",str,b,asctime( newtime ));
		fprintf(pFileHandle,"************************************************************\n");
		break;
	case 19:
		fprintf(pFileHandle,"%s \t: NET_REQUESTDROID \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 23:
		fprintf(pFileHandle,"%s \t: NET_WHOLEDROID \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 22:
		fprintf(pFileHandle,"%s \t: NET_STRUCT (Whole) \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 25:
		fprintf(pFileHandle,"%s \t: NET_PLAYERRESPONDING \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 26:
		fprintf(pFileHandle,"%s \t: NET_OPTIONS \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 27:
		fprintf(pFileHandle,"%s \t: NET_WAYPOINT \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 28:
		fprintf(pFileHandle,"%s \t: NET_SECONDARY \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 29:
		fprintf(pFileHandle,"%s \t: NET_FIREUP \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 34:
		fprintf(pFileHandle,"%s \t: NET_ARTIFACTS \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 36:
		fprintf(pFileHandle,"%s \t: NET_SCORESUBMIT \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 37:
		fprintf(pFileHandle,"%s \t: NET_DESTROYXTRA \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 38:
		fprintf(pFileHandle,"%s \t: NET_VTOL \t:%d\t\t%s",str,b,asctime( newtime ));
		break;
	case 39:
		fprintf(pFileHandle,"%s \t: NET_VTOLREARM \t:%d\t\t%s",str,b,asctime( newtime ));
		break;

	default:
		fprintf(pFileHandle,"%s \t:%d \t\t\t:%d\t\t%s",str,a,b,asctime( newtime ));
		break;
	}
	fflush(pFileHandle);
	return TRUE;
}

// ////////////////////////////////////////////////////////////////////////
