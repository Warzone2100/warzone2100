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

#include "lib/framework/frame.h"

#include "lib/framework/frameresource.h"
#include "tracklib.h"

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

//
// static SDWORD g_iMaxSamples;
//
static SDWORD			g_iMaxSameSamples;

// flag set when system is active (for callbacks etc)
static BOOL				g_bSystemActive = FALSE;
static AUDIO_CALLBACK	g_pStopTrackCallback = NULL;

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_Init( SDWORD iMaxSameSamples )
{
	g_iMaxSameSamples = iMaxSameSamples;
	g_iCurTracks = 0;
	if (!sound_InitLibrary())
	{
		debug(LOG_ERROR, "Cannot init sound library");
		return FALSE;
	}

	// init audio array (with NULL pointers; which calloc ensures by setting all allocated memory to zero)
	g_apTrack = calloc(MAX_TRACKS, sizeof(TRACK*));
	if (!g_apTrack)
	{
		debug(LOG_ERROR, "sound_Init: Out of memory");
		abort();
		return FALSE;
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
	free( g_apTrack );

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
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_SetTrackVals
	(
		TRACK	*psTrack,
		BOOL	bLoop,
		SDWORD	iTrack,
		SDWORD	iVol,
		SDWORD	iAudibleRadius
	)
{
	if ( !(iTrack < MAX_TRACKS) )
	{
		return FALSE;
	}

	if ( g_apTrack[iTrack] != NULL )
	{
		debug( LOG_ERROR, "sound_SetTrackVals: track %i already set\n", iTrack );
		abort();
		return FALSE;
	}

	// set track members
	psTrack->bLoop = bLoop;
	psTrack->iVol = iVol;
	psTrack->iAudibleRadius = iAudibleRadius;
	psTrack->iTime =0;			//added, since they really should init all the values. -Q
	psTrack->iTimeLastFinished = 0;
	psTrack->iNumPlaying = 0;

	// set global
	g_apTrack[iTrack] = psTrack;

	// increment current sound
	g_iCurTracks++;

	return TRUE;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_ReleaseTrack( TRACK *psTrack )
{
	TRACK** currTrack;

	// This is here to save CPU wasted by the loop below;
	// Calling this function with psTrack = NULL is perfectly legal,
	// with or without this check
	if (!psTrack)
		return;

	// Run through the list of tracks and set the pointer to the track which is to be released to NULL
	for (currTrack = &g_apTrack[0]; currTrack != &g_apTrack[MAX_TRACKS]; ++currTrack)
	{
		if (*currTrack == psTrack)
		{
			*currTrack = NULL;
		}
	}

	sound_FreeTrack( psTrack );
	free( psTrack );
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_CheckAllUnloaded( void )
{
	TRACK** currTrack;

	for (currTrack = &g_apTrack[0]; currTrack != &g_apTrack[MAX_TRACKS]; ++currTrack)
	{
		ASSERT(*currTrack == NULL, "sound_CheckAllUnloaded: a track is not unloaded yet (%s); check audio.cfg for duplicate IDs", (*currTrack)->pName);
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
	if ( iTrack == SAMPLE_NOT_FOUND )
	{
		return NULL;
	}

	ASSERT( g_apTrack[iTrack] != NULL, "sound_GetTrackName: unallocated track" );
	return g_apTrack[iTrack] ? g_apTrack[iTrack]->pName : "unallocated";
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_Play2DTrack( AUDIO_SAMPLE *psSample, BOOL bQueued )
{
	TRACK	*psTrack;

	if (!sound_CheckTrack(psSample->iTrack))
	{
		return FALSE;
	}

	psTrack = g_apTrack[psSample->iTrack];

	if (psTrack == NULL)
	{
		return FALSE;
	}

	return sound_Play2DSample( psTrack, psSample, bQueued );
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_Play3DTrack( AUDIO_SAMPLE *psSample )
{
	TRACK	*psTrack;

	if (!sound_CheckTrack(psSample->iTrack))
	{
		return FALSE;
	}

	psTrack = g_apTrack[psSample->iTrack];
	return sound_Play3DSample( psTrack, psSample );
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_StopTrack( AUDIO_SAMPLE *psSample )
{
	ASSERT( psSample != NULL, "sound_StopTrack: sample pointer invalid\n" );

#ifndef WZ_NOSOUND
	if ( psSample->iSample != (ALuint)SAMPLE_NOT_ALLOCATED )
	{
		sound_StopSample( psSample->iSample );
	}
	else
	{
		debug( LOG_SOUND, "sound_StopTrack: sample %u out of range, we probably have run out of available OpenAL sources\n", psSample->iSample );
	}
#endif

	// do stopped callback
	if ( g_pStopTrackCallback != NULL && psSample->psObj != NULL )
	{
		g_pStopTrackCallback( psSample->psObj );
	}
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_PauseTrack( AUDIO_SAMPLE *psSample )
{
#ifndef WZ_NOSOUND
	if ( psSample->iSample != (ALuint)SAMPLE_NOT_ALLOCATED )
	{
		sound_StopSample( psSample->iSample );
	}
#endif
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_FinishedCallback( AUDIO_SAMPLE *psSample )
{
	ASSERT( psSample != NULL, "sound_FinishedCallback: sample pointer invalid\n" );

	if ( g_apTrack[psSample->iTrack] != NULL )
	{
		g_apTrack[psSample->iTrack]->iTimeLastFinished = sound_GetGameTime();
	}

	// call user callback if specified
	if ( psSample->pCallback != NULL )
	{
		psSample->pCallback(psSample->psObj);
	}

	// set finished flag
	psSample->bFinishedPlaying = TRUE;
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
	if ( i >= MAX_TRACKS )
	{
		return SAMPLE_NOT_FOUND;
	}

	return i;
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
	if ( i >= MAX_TRACKS )
	{
		return SAMPLE_NOT_ALLOCATED;
	}

	return i;
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
