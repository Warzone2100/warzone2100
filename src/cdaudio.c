/***************************************************************************/

#include "frame.h"
#include "audio.h"
#include "cdaudio.h"

/***************************************************************************/

#define	CD_MAX_VOL				0xFFFF
#define	CD_ID_NOT_FOUND			-1
#define	CD_LEFT_CHANNEL_MASK	0xFFFF

#define	CD_NO_TRACK_SELECTED	-1

/***************************************************************************/

static UINT		g_wDeviceID;
static SDWORD	g_iCurTrack    = CD_NO_TRACK_SELECTED;
static DWORD	g_iCurPlayFrom = 0;
static DWORD	g_iCurPlayTo   = 0;
static DWORD	g_dwPlayFlags;

static WNDPROC	fnOldWinProc = NULL;

/***************************************************************************/
/* cdAudio Subclass procedure */

LRESULT APIENTRY cdAudio_CheckTrackFinished( HWND hWnd, UINT uMsg, 
											 WPARAM wParam, LPARAM lParam )
{ 
	if ( uMsg == MM_MCINOTIFY && wParam == (WPARAM) MCI_NOTIFY_SUCCESSFUL )
	{
		cdAudio_PlayTrack( g_iCurTrack );
	}
 
    return CallWindowProc( fnOldWinProc, hWnd, uMsg, wParam, lParam ); 
} 

/***************************************************************************/

BOOL
cdAudio_Open( void )
{ 
#ifndef WIN32
	return TRUE;
#else
	DWORD				dwReturn;
	MCI_OPEN_PARMS		mciOpenParms;
	MCI_SET_PARMS		mciSetParms;
	MCI_GENERIC_PARMS	mciParams;

	// Open the CD audio device by specifying the device name.
	mciOpenParms.lpstrDeviceType = "cdaudio";
	if ( (dwReturn = mciSendCommand( 0, MCI_OPEN,
			MCI_OPEN_TYPE, (DWORD)(LPVOID) &mciOpenParms) ) == 0 )
	{
		// The device opened successfully; get the device ID.
		g_wDeviceID = mciOpenParms.wDeviceID;

		// Set the time format to track/minute/second/frame (TMSF).
		mciSetParms.dwTimeFormat = MCI_FORMAT_TMSF;
	    if ( (dwReturn = mciSendCommand(g_wDeviceID, MCI_SET, 
				MCI_SET_TIME_FORMAT, (DWORD)(LPVOID) &mciSetParms)) != 0 )
		{
			DBPRINTF( ("cdAudio_Open: set time format failed\n ") );
			mciSendCommand( g_wDeviceID, MCI_CLOSE, 0, (DWORD) &mciParams );
			return FALSE;
		}
	}

	/* subclass window proc so can examine for track finished messages */
	fnOldWinProc = (WNDPROC) SetWindowLong( GetActiveWindow(), GWL_WNDPROC,
								(LPARAM) cdAudio_CheckTrackFinished );
	
	return TRUE;
#endif
} 

/***************************************************************************/

BOOL
cdAudio_Close( void )
{ 
#ifdef WIN32
	MCI_GENERIC_PARMS	mciParams;

	mciSendCommand( g_wDeviceID, MCI_CLOSE, 0, (DWORD) &mciParams );

	/* unsubclass window */
	SetWindowLong( GetActiveWindow(), GWL_WNDPROC, (LPARAM) fnOldWinProc );
#endif

	return TRUE;
} 

/***************************************************************************/

BOOL
cdAudio_PlayTrack( SDWORD iTrack )
{ 
#ifndef WIN32
	return TRUE;
#else
	DWORD				dwReturn;
	MCI_PLAY_PARMS		mciPlayParms;
	MCI_GENERIC_PARMS	mciParams;
	MCI_STATUS_PARMS	mciStatusParams;

	/* store track in case have to restart */
	g_iCurTrack = iTrack;

	/* increment iTrack because first track is data */
	iTrack++;

	/* set MCI_TO flag only if final track
	 * (MCI_PLAY plays to end if MCI_TO not set)
	 */
	memset( &mciStatusParams, 0, sizeof(MCI_STATUS_PARMS) );
	mciStatusParams.dwItem = MCI_STATUS_NUMBER_OF_TRACKS;
	if ( (dwReturn = mciSendCommand( g_wDeviceID, MCI_STATUS,
				MCI_WAIT | MCI_STATUS_ITEM,
				(DWORD)(LPVOID) &mciStatusParams) ) != 0 )
	{
		DBPRINTF( ("cdAudio_Play: get status failed\n ") );
		goto CDPlayError;
	}
	
	if ( mciStatusParams.dwReturn == (DWORD) iTrack )
	{
		g_dwPlayFlags = MCI_FROM | MCI_NOTIFY;
	}
	else
	{
		g_dwPlayFlags = MCI_FROM | MCI_NOTIFY | MCI_TO;
	}

	/* save track start/end positions */
	g_iCurPlayFrom = MCI_MAKE_TMSF(iTrack  , 0, 0, 0);
	g_iCurPlayTo   = MCI_MAKE_TMSF(iTrack+1, 0, 0, 0);

	/* play it */
	memset( &mciPlayParms, 0, sizeof(MCI_PLAY_PARMS) );
	mciPlayParms.dwFrom     = g_iCurPlayFrom;
	mciPlayParms.dwTo       = g_iCurPlayTo;
	mciPlayParms.dwCallback = MAKELONG(GetActiveWindow(), 0);
	if ( (dwReturn = mciSendCommand( g_wDeviceID, MCI_PLAY,
			g_dwPlayFlags, (DWORD)(LPVOID) &mciPlayParms) ) != 0 )
	{
		DBPRINTF( ("cdAudio_Play: play failed\n ") );
		goto CDPlayError;
	}

	return TRUE;

CDPlayError:

	mciSendCommand( g_wDeviceID, MCI_CLOSE, 0, (DWORD) &mciParams );
	return FALSE;
#endif
} 

/***************************************************************************/

BOOL
cdAudio_Stop( void )
{ 
#ifndef WIN32
	return TRUE;
#else
	DWORD				dwReturn;
	MCI_GENERIC_PARMS	mciParams;

	if ( (dwReturn = mciSendCommand( g_wDeviceID, MCI_STOP, 0,
						(DWORD) &mciParams )) != 0 )
	{
	    mciSendCommand( g_wDeviceID, MCI_CLOSE, 0, (DWORD) &mciParams );
		return FALSE;
	}

	return TRUE;
#endif
} 

/***************************************************************************/

BOOL
cdAudio_Pause( void )
{ 
#ifndef WIN32
	return TRUE;
#else
	DWORD				dwReturn;
	MCI_GENERIC_PARMS	mciParams;
	MCI_STATUS_PARMS	mciStatusParams;

	if ( (dwReturn = mciSendCommand( g_wDeviceID, MCI_PAUSE, 0,
						(DWORD) &mciParams )) != 0 )
	{
	    mciSendCommand( g_wDeviceID, MCI_CLOSE, 0, (DWORD) &mciParams );
		return FALSE;
	}

	/* get current position */
	memset( &mciStatusParams, 0, sizeof(MCI_STATUS_PARMS) );
	mciStatusParams.dwItem = MCI_STATUS_POSITION;
	if ( (dwReturn = mciSendCommand( g_wDeviceID, MCI_STATUS,
				MCI_WAIT | MCI_STATUS_ITEM,
				(DWORD)(LPVOID) &mciStatusParams) ) != 0 )
	{
		DBPRINTF( ("cdAudio_Pause: get position failed\n ") );
		return FALSE;
	}

	/* save current position */
	g_iCurPlayFrom = mciStatusParams.dwReturn;
	
	return TRUE;

#endif
} 

/***************************************************************************/

BOOL
cdAudio_Resume( void )
{ 
#ifndef WIN32
	return TRUE;
#else
	DWORD			dwReturn;
	MCI_PLAY_PARMS	mciPlayParams;

	/* attempt to resume else re-start current track */
	memset( &mciPlayParams, 0, sizeof(MCI_PLAY_PARMS) );
	mciPlayParams.dwFrom = g_iCurPlayFrom;
	mciPlayParams.dwTo   = g_iCurPlayTo;
	dwReturn = mciSendCommand( g_wDeviceID, MCI_PLAY, 0,
									(DWORD) &mciPlayParams );
	if ( dwReturn != 0 )
	{
		DBPRINTF( ("cdAudio_Resume: unable to resume\n ") );
		if ( g_iCurTrack != CD_NO_TRACK_SELECTED )
		{
			cdAudio_PlayTrack( g_iCurTrack );
		}
	}

	return TRUE;
#endif
} 

/***************************************************************************/
