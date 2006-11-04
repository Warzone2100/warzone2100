//*
//
#ifdef WIN32
#include <windows.h>
#endif
#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "tracklib.h"
#include "lib/gamelib/priority.h"

//*
//
// defines
#define MAX_TRACKS	( 600 )

//*
//
// static global variables
// array of pointers to sound effects
static TRACK			**g_apTrack;

// number of tracks loaded
static SDWORD			g_iCurTracks = 0;
static SDWORD			g_iSamples = 0;

//
// static SDWORD g_iMaxSamples;
//
static SDWORD			g_iMaxSameSamples;

// flag set when system is active (for callbacks etc)
static BOOL				g_bSystemActive = FALSE;
static BOOL				g_bDevVolume = FALSE;
static AUDIO_CALLBACK	g_pStopTrackCallback = NULL;

//*
// =======================================================================================================================
// =======================================================================================================================
//
static BOOL sound_CheckDevice( void )
{
	//
	// * // Bah, not needed! --Qamly. #ifdef WIN32MM WAVEOUTCAPS waveCaps;
	// * MMRESULT mmRes;
	// * // check wave out device(s) present if ( waveOutGetNumDevs() == 0 ) { DBPRINTF(
	// * ("sound_CheckDevice: error in waveOutGetNumDevs\n") );
	// * return FALSE;
	// * } // default to using first device: check volume control caps mmRes =
	// * waveOutGetDevCaps( 0, (LPWAVEOUTCAPS) &waveCaps, sizeof(WAVEOUTCAPS) );
	// * if ( mmRes != MMSYSERR_NOERROR ) { DBPRINTF( ("sound_CheckDevice: error in
	// * waveOutGetDevCaps\n") );
	// * return FALSE;
	// * } // verify device supports volume changes // if ( waveCaps.dwSupport &
	// * WAVECAPS_VOLUME ) { return TRUE;
	// * } else { DBPRINTF( ("sound_CheckDevice: wave out device doesn't support volume
	// * changes\n") );
	// * return FALSE;
	// * } #endif Checking if needed
	//
	return TRUE;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_Init( SDWORD iMaxSameSamples )
{
	//~~~~~~~~~~~~~
#ifdef USE_COMPRESSED_SPEECH
	void *	lpMsgBuf;
#endif
	SDWORD	i;
	//~~~~~~~~~~~~~

	//
	// hWnd;
	//
	g_iMaxSameSamples = iMaxSameSamples;
	g_iCurTracks = 0;
	g_bDevVolume = sound_CheckDevice();
#ifdef USE_COMPRESSED_SPEECH
	if ( !LoadLibrary("MSACM32.DLL") )
	{
		FormatMessage
		(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) & lpMsgBuf,
			0,
			NULL
		);
		debug( LOG_NEVER, "sound_Init: couldn't load compression manager MSACM32.DLL\n" );
	}

	if ( !LoadLibrary("MSADP32.ACM") )
	{
		FormatMessage
		(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR) & lpMsgBuf,
			0,
			NULL
		);
		debug( LOG_NEVER, "sound_Init: couldn't load ADPCM codec MSADP32.ACM\n" );
	}
#endif
	if ( sound_InitLibrary() == FALSE )
	{
		debug( LOG_NEVER, "Cannot init sound library\n" );
		return FALSE;
	}

	// init audio array
	g_apTrack = (TRACK **) MALLOC( sizeof(TRACK* ) * MAX_TRACKS);
	for ( i = 0; i < MAX_TRACKS; i++ )
	{
		g_apTrack[i] = NULL;
	}

	// set system active flag for callbacks
	g_bSystemActive = TRUE;
	return TRUE;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_Shutdown( void )
{
	FREE( g_apTrack );

	// set inactive flag to prevent callbacks coming after shutdown
	g_bSystemActive = FALSE;
	sound_ShutdownLibrary();
	return TRUE;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_GetSystemActive( void )
{
	return g_bSystemActive;
}

//*
//
// Vag ID is just used on PSX szFileName just used on PC

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_SetTrackVals
	(
		TRACK	*psTrack,
		BOOL	bLoop,
		SDWORD	iTrack,
		SDWORD	iVol,
		SDWORD	iPriority,
		SDWORD	iAudibleRadius,
		SDWORD	VagID
	)
{
	ASSERT( iPriority >= LOW_PRIORITY && iPriority <= HIGH_PRIORITY, "sound_CreateTrack: priority %i out of bounds\n", iPriority );

	// add to sound array
	if ( iTrack < MAX_TRACKS )
	{
		if ( g_apTrack[iTrack] != NULL )
		{
			debug( LOG_ERROR, "sound_SetTrackVals: track %i already set\n", iTrack );
			abort();
			return FALSE;
		}

		// set track members
		psTrack->bLoop = bLoop;
		psTrack->iVol = iVol;
		psTrack->iPriority = iPriority;
		psTrack->iAudibleRadius = iAudibleRadius;
		psTrack->iTime =0;			//added, since they really should init all the values. -Q
		psTrack->iTimeLastFinished = 0;
		psTrack->iNumPlaying = 0;
		psTrack->bCompressed =0;	//added  this was the bugger that caused grief for .net.  It was never defined. -Q

		// I didn't comment the below value out, so I guess NOT needed. -Q
		//
		// VagID;
		//
		// set global
		g_apTrack[iTrack] = psTrack;

		// increment current sound
		g_iCurTracks++;
		return TRUE;
	}

	return FALSE;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
static BOOL sound_AddTrack( TRACK *pTrack )
{
	// add to sound array
	if ( g_iCurTracks < MAX_TRACKS )
	{
		// set pointer in table
		g_apTrack[g_iCurTracks] = pTrack;

		// increment current sound
		g_iCurTracks++;
		return TRUE;
	}
	else
	{
		debug( LOG_ERROR, "sound_AddTrack: all tracks used: increase MAX_TRACKS\n" );
		abort();
		return FALSE;
	}
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
TRACK *sound_LoadTrackFromBuffer(char *pBuffer, UDWORD udwSize)
{
	//~~~~~~~~~~~~
	TRACK	*pTrack;
	//~~~~~~~~~~~~

	// allocate track
	pTrack = (TRACK *) MALLOC( sizeof(TRACK) );
	memset(pTrack, 0, sizeof(TRACK));
	if ( pTrack == NULL )
	{
		debug( LOG_ERROR, "sound_LoadTrackFromBuffer: couldn't allocate memory\n" );
		abort();
		return NULL;
	}
	else
	{
		pTrack->bMemBuffer = TRUE;
		pTrack->pName = (char*)MALLOC( strlen(GetLastResourceFilename()) + 1 );
		if ( pTrack->pName == NULL )
		{
			debug( LOG_ERROR, "sound_LoadTrackFromBuffer: couldn't allocate memory\n" );
			abort();
			FREE( pTrack );
			return NULL;
		}

		strcpy( pTrack->pName, GetLastResourceFilename() );
		pTrack->resID = GetLastHashName();
		if ( sound_ReadTrackFromBuffer(pTrack, pBuffer, udwSize) == FALSE )
		{
			return NULL;
		}
		else
		{
#ifdef USE_COMPRESSED_SPEECH
			// flag compressed audio load
			if ( pTrack->bCompressed == TRUE )
			{
				debug( LOG_NEVER, "sound_LoadTrackFromBuffer: %s is compressed!\n", pTrack->pName );
			}
#endif
			return pTrack;
		}
	}
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_LoadTrackFromFile(char szFileName[])
{
	//~~~~~~~~~~~~
	TRACK	*pTrack;
	//~~~~~~~~~~~~

	// allocate track
	pTrack = (TRACK *) MALLOC( sizeof(TRACK) );
	if ( pTrack != NULL )
	{
		pTrack->bMemBuffer = FALSE;
		pTrack->pName = (char*)MALLOC( strlen((char*) szFileName) + 1 );
		if ( pTrack->pName == NULL )
		{
			debug( LOG_ERROR, "sound_LoadTrackFromFile: Out of memory" );
			abort();
			return FALSE;
		}

		strcpy( pTrack->pName, (char*) szFileName );
		pTrack->resID = HashStringIgnoreCase( (char*) szFileName );
		if ( sound_ReadTrackFromFile(pTrack, szFileName) == FALSE )
		{
			return FALSE;
		}

		return sound_AddTrack( pTrack );
	}

	return FALSE;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_ReleaseTrack( TRACK *psTrack )
{
	//~~~~~~~~~~~
	SDWORD	iTrack;
	//~~~~~~~~~~~

	if ( psTrack->pName != NULL )
	{
		FREE( psTrack->pName );
	}

	for ( iTrack = 0; iTrack < g_iCurTracks; iTrack++ )
	{
		if ( g_apTrack[iTrack] == psTrack )
		{
			g_apTrack[iTrack] = NULL;
		}
	}

	sound_FreeTrack( psTrack );
	return TRUE;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_CheckAllUnloaded( void )
{
	//~~~~~~~~~~~
	SDWORD	iTrack;
	//~~~~~~~~~~~

	for ( iTrack = 0; iTrack < MAX_TRACKS; iTrack++ )
	{
		ASSERT( g_apTrack[iTrack] == NULL, "sound_CheckAllUnloaded: check audio.cfg for duplicate IDs\n" );
	}
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_TrackLooped( SDWORD iTrack )
{
	sound_CheckTrack( iTrack );
	return g_apTrack[iTrack]->bLoop;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
SDWORD sound_TrackAudibleRadius( SDWORD iTrack )
{
	sound_CheckTrack( iTrack );
	return g_apTrack[iTrack]->iAudibleRadius;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
SDWORD sound_GetNumPlaying( SDWORD iTrack )
{
	sound_CheckTrack( iTrack );
	return g_apTrack[iTrack]->iNumPlaying;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_CheckSample( AUDIO_SAMPLE *psSample )
{
	ASSERT( PTRVALID(psSample, sizeof(AUDIO_SAMPLE)), "sound_CheckSample: sample pointer invalid\n" );
	ASSERT( psSample->iSample >= 0 || psSample->iSample == SAMPLE_NOT_ALLOCATED, "sound_CheckSample: sample %i out of range\n", psSample->iSample );

	//
	// psSample;
	//
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_CheckTrack( SDWORD iTrack )
{
	if ( iTrack < 0 || iTrack > g_iCurTracks - 1 )
	{
		debug( LOG_SOUND, "sound_CheckTrack: track number %i outside max %i\n", iTrack, g_iCurTracks );
		return FALSE;
	}

	if ( g_apTrack[iTrack] == NULL )
	{
		debug( LOG_SOUND, "sound_CheckTrack: track %i NULL\n", iTrack );
		return FALSE;
	}

	return TRUE;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
SDWORD sound_GetTrackTime( SDWORD iTrack )
{
	sound_CheckTrack( iTrack );
	return g_apTrack[iTrack]->iTime;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
SDWORD sound_GetTrackPriority( SDWORD iTrack )
{
	sound_CheckTrack( iTrack );
	return g_apTrack[iTrack]->iPriority;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
SDWORD sound_GetTrackVolume( SDWORD iTrack )
{
	sound_CheckTrack( iTrack );
	return g_apTrack[iTrack]->iVol;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
SDWORD sound_GetTrackAudibleRadius( SDWORD iTrack )
{
	sound_CheckTrack( iTrack );
	return g_apTrack[iTrack]->iAudibleRadius;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
const char *sound_GetTrackName( SDWORD iTrack )
{
	if ( iTrack == SAMPLE_NOT_FOUND ) return NULL;
	ASSERT( g_apTrack[iTrack] != NULL, "sound_GetTrackName: unallocated track" );
	return g_apTrack[iTrack] ? g_apTrack[iTrack]->pName : "unallocated";
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
UDWORD sound_GetTrackHashName( SDWORD iTrack )
{
	if (iTrack == 0 || iTrack == SAMPLE_NOT_FOUND)
		return 0;
	ASSERT( g_apTrack[iTrack] != NULL, "sound_GetTrackName: unallocated track" );
	return g_apTrack[iTrack] ? g_apTrack[iTrack]->resID : 0;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_Play2DTrack( AUDIO_SAMPLE *psSample, BOOL bQueued )
{
	//~~~~~~~~~~~~~
	TRACK	*psTrack;
	//~~~~~~~~~~~~~

	if (!sound_CheckTrack(psSample->iTrack))
	  return FALSE;

	psTrack = g_apTrack[psSample->iTrack];

	if (psTrack == NULL) return FALSE;

/*	// check only playing compressed audio on queue channel
#ifdef USE_COMPRESSED_SPEECH
	if ( bQueued && !psTrack->bCompressed )
	{
		DBPRINTF( ("sound_PlayTrack: trying to play uncompressed speech %s!\n", psTrack->pName) );
		return FALSE;
	}

	if ( !bQueued && psTrack->bCompressed )
	{
		DBPRINTF( ("sound_PlayTrack: trying to play compressed audio %s!\n", psTrack->pName) );
		return FALSE;
	}

#else
	if ( psTrack->bCompressed )
	{
		DBPRINTF( ("sound_PlayTrack: trying to play compressed speech %s!\n", psTrack->pName) );
		return FALSE;
	}
#endif
*/
	return sound_Play2DSample( psTrack, psSample, bQueued );
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_Play3DTrack( AUDIO_SAMPLE *psSample )
{
	//~~~~~~~~~~~~~
	TRACK	*psTrack;
	//~~~~~~~~~~~~~

	if (!sound_CheckTrack(psSample->iTrack))
	  return FALSE;

	psTrack = g_apTrack[psSample->iTrack];
/*	if ( psTrack->bCompressed )
	{
		DBPRINTF( ("sound_PlayTrack: trying to play compressed audio %s!\n", psTrack->pName) );
		return FALSE;
	}
*/
	return sound_Play3DSample( psTrack, psSample );
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_StopTrack( AUDIO_SAMPLE *psSample )
{
	sound_CheckSample( psSample );
	if ( psSample->iSample != SAMPLE_NOT_ALLOCATED )
	{
		sound_StopSample( psSample->iSample );
	}

	// do stopped callback
	if ( g_pStopTrackCallback != NULL && psSample->psObj != NULL )
	{
		( g_pStopTrackCallback ) ( psSample );
	}

	// update number of samples playing
	g_iSamples--;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_PauseTrack( AUDIO_SAMPLE *psSample )
{
	if ( psSample->iSample != SAMPLE_NOT_ALLOCATED )
	{
		sound_StopSample( psSample->iSample );
	}
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_FinishedCallback( AUDIO_SAMPLE *psSample )
{
	sound_CheckSample( psSample );
	if ( g_apTrack[psSample->iTrack] != NULL )
	{
		g_apTrack[psSample->iTrack]->iTimeLastFinished = sound_GetGameTime();
	}

	// call user callback if specified
	if ( psSample->pCallback != NULL )
	{
		( psSample->pCallback ) ( psSample );
	}

	// set remove flag
	psSample->bRemove = TRUE;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
SDWORD sound_GetTrackID( TRACK *psTrack )
{
	//~~~~~~~~~~
	SDWORD	i = 0;
	//~~~~~~~~~~

	// find matching track
	for ( i = 0; i < MAX_TRACKS; i++ )
	{
		if ( (g_apTrack[i] != NULL) && (g_apTrack[i] == psTrack) )
		{
			break;
		}
	}

	// if matching track found return it else find empty track
	if ( i < MAX_TRACKS )
	{
		return i;
	}
	else
	{
		return SAMPLE_NOT_FOUND;
	}
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
SDWORD sound_GetAvailableID( void )
{
	//~~~~~~
	SDWORD	i;
	//~~~~~~

	for ( i = 0; i < MAX_TRACKS; i++ )
	{
		if ( g_apTrack[i] == NULL )
		{
			break;
		}
	}

	ASSERT( i < MAX_TRACKS, "sound_GetTrackID: unused track not found!\n" );
	if ( i < MAX_TRACKS )
	{
		return i;
	}
	else
	{
		return SAMPLE_NOT_ALLOCATED;
	}
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_SetStoppedCallback( AUDIO_CALLBACK pStopTrackCallback )
{
	g_pStopTrackCallback = pStopTrackCallback;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
UDWORD sound_GetTrackTimeLastFinished( SDWORD iTrack )
{
	sound_CheckTrack( iTrack );
	return g_apTrack[iTrack]->iTimeLastFinished;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_SetTrackTimeLastFinished( SDWORD iTrack, UDWORD iTime )
{
	sound_CheckTrack( iTrack );
	g_apTrack[iTrack]->iTimeLastFinished = iTime;
}

//*
//
