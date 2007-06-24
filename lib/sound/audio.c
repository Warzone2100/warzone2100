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
//*
//
#include <stdio.h>
#include <string.h>
#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "lib/gamelib/gtime.h"
#include "tracklib.h"
#include "aud.h"
#include "audio_id.h"
#include "lib/framework/trig.h"
#include "lib/ivis_common/pietypes.h"

//*
//
// defines
#define NO_SAMPLE				- 2
#define AUDIO_SAMPLE_HEAP_INIT	1000
#define AUDIO_SAMPLE_HEAP_EXT	10
#define MAX_SAME_SAMPLES		2
#define LOWERED_VOL				AUDIO_VOL_MAX / 4

//*
//
// externs
//*
//
// static functions
//*
//
// global variables
static AUDIO_SAMPLE *g_psSampleList = NULL;
static AUDIO_SAMPLE *g_psSampleQueue = NULL;
static BOOL			g_bAudioEnabled = FALSE;
static BOOL			g_bAudioPaused = FALSE;
static BOOL			g_bStopAll = FALSE;
static AUDIO_SAMPLE g_sPreviousSample;
static SDWORD g_i3DVolume = AUDIO_VOL_MAX;


//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL audio_Disabled( void )
{
	return !g_bAudioEnabled;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL audio_Init( AUDIO_CALLBACK pStopTrackCallback )
{
	// init audio system
	g_sPreviousSample.iTrack = NO_SAMPLE;
	g_sPreviousSample.x = SAMPLE_COORD_INVALID;
	g_sPreviousSample.y = SAMPLE_COORD_INVALID;
	g_sPreviousSample.z = SAMPLE_COORD_INVALID;
	g_bAudioEnabled = sound_Init(MAX_SAME_SAMPLES);
	if (g_bAudioEnabled)
	{
		sound_SetStoppedCallback( pStopTrackCallback );
	}
	return g_bAudioEnabled;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL audio_Shutdown( void )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE	*psSample = NULL, *psSampleTemp = NULL;
	BOOL			bOK;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// if audio not enabled return TRUE to carry on game without audio
	if ( g_bAudioEnabled == FALSE )
	{
		return TRUE;
	}

	sound_StopAll();
	bOK = sound_Shutdown();

	// empty sample list
	psSample = g_psSampleList;
	while ( psSample != NULL )
	{
		psSampleTemp = psSample->psNext;
		free(psSample);
		psSample = psSampleTemp;
	}

	// empty sample queue
	psSample = g_psSampleQueue;
	while ( psSample != NULL )
	{
		psSampleTemp = psSample->psNext;
		free(psSample);
		psSample = psSampleTemp;
	}

	// free sample heap
	g_psSampleList = NULL;
	g_psSampleQueue = NULL;

	return bOK;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL audio_GetPreviousQueueTrackPos( SDWORD *iX, SDWORD *iY, SDWORD *iZ )
{
	if (g_sPreviousSample.x == SAMPLE_COORD_INVALID
	 || g_sPreviousSample.y == SAMPLE_COORD_INVALID
	 || g_sPreviousSample.z == SAMPLE_COORD_INVALID)
	{
		return FALSE;
	}

	*iX = g_sPreviousSample.x;
	*iY = g_sPreviousSample.y;
	*iZ = g_sPreviousSample.z;

	return TRUE;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
static void audio_AddSampleToHead( AUDIO_SAMPLE **ppsSampleList, AUDIO_SAMPLE *psSample )
{
	psSample->psNext = ( *ppsSampleList );
	psSample->psPrev = NULL;
	if ( (*ppsSampleList) != NULL )
	{
		( *ppsSampleList )->psPrev = psSample;
	}
	( *ppsSampleList ) = psSample;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
static void audio_AddSampleToTail( AUDIO_SAMPLE **ppsSampleList, AUDIO_SAMPLE *psSample )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE	*psSampleTail = NULL;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if ( (*ppsSampleList) == NULL )
	{
		( *ppsSampleList ) = psSample;
		return;
	}

	psSampleTail = ( *ppsSampleList );
	while ( psSampleTail->psNext != NULL )
	{
		psSampleTail = psSampleTail->psNext;
	}

	psSampleTail->psNext = psSample;
	psSample->psPrev = psSampleTail;
	psSample->psNext = NULL;
}

//*
//
// audio_RemoveSample Removes sample from list but doesn't free its memory

//*
// =======================================================================================================================
// =======================================================================================================================
//
static void audio_RemoveSample( AUDIO_SAMPLE **ppsSampleList, AUDIO_SAMPLE *psSample )
{
	if ( psSample == NULL )
	{
		return;
	}

	if ( psSample == (*ppsSampleList) )
	{
		// first sample in list
		( *ppsSampleList ) = psSample->psNext;
	}

	if ( psSample->psPrev != NULL )
	{
		psSample->psPrev->psNext = psSample->psNext;
	}

	if ( psSample->psNext != NULL )
	{
		psSample->psNext->psPrev = psSample->psPrev;
	}

	// set sample pointers NULL for safety
	psSample->psPrev = NULL;
	psSample->psNext = NULL;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
static BOOL audio_CheckSameQueueTracksPlaying( SDWORD iTrack )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	SDWORD			iCount;
	AUDIO_SAMPLE	*psSample = NULL;
	BOOL			bOK = TRUE;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// return if audio not enabled
	if ( g_bAudioEnabled == FALSE || g_bAudioPaused == TRUE )
	{
		return TRUE;
	}

	iCount = 0;

	// loop through queue sounds and check whether too many already in it
	psSample = g_psSampleQueue;
	while ( psSample != NULL )
	{
		if ( psSample->iTrack == iTrack )
		{
			iCount++;
		}

		if ( iCount > MAX_SAME_SAMPLES )
		{
			bOK = FALSE;
			break;
		}

		psSample = psSample->psNext;
	}

	return bOK;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
static AUDIO_SAMPLE *audio_QueueSample( SDWORD iTrack )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE	*psSample = NULL;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// return if audio not enabled
	if ( g_bAudioEnabled == FALSE || g_bAudioPaused == TRUE || g_bStopAll == TRUE )
	{
		return NULL;
	}

	ASSERT( sound_CheckTrack(iTrack) == TRUE, "audio_QueueSample: track %i outside limits\n", iTrack );

	// reject track if too many of same ID already in queue
	if ( audio_CheckSameQueueTracksPlaying(iTrack) == FALSE )
	{
		return NULL;
	}

	psSample = malloc(sizeof(AUDIO_SAMPLE));
	if ( psSample == NULL )
	{
		debug(LOG_ERROR, "audio_QueueSample: Out of memory");
		return NULL;
	}

	memset( psSample, 0, sizeof(AUDIO_SAMPLE) );		//[check] -Q
	psSample->iTrack = iTrack;
	psSample->x = SAMPLE_COORD_INVALID;
	psSample->y = SAMPLE_COORD_INVALID;
	psSample->z = SAMPLE_COORD_INVALID;
	psSample->bFinishedPlaying = FALSE;

	// add to queue
	audio_AddSampleToTail( &g_psSampleQueue, psSample );

	return psSample;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void audio_QueueTrack( SDWORD iTrack )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE	*psSample = NULL;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// return if audio not enabled
	if ( g_bAudioEnabled == FALSE || g_bAudioPaused == TRUE || g_bStopAll == TRUE )
	{
		return;
	}

	psSample = audio_QueueSample( iTrack );
}

//*
//
//
// * audio_QueueTrackMinDelay Will only play track if iMinDelay has elapsed since
// * track last finished
//

//*
// =======================================================================================================================
// =======================================================================================================================
//
void audio_QueueTrackMinDelay( SDWORD iTrack, UDWORD iMinDelay )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE	*psSample;
	UDWORD			iDelay;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// return if audio not enabled
	if ( g_bAudioEnabled == FALSE || g_bAudioPaused == TRUE )
	{
		return;
	}

	// Determine if at least iMinDelay time has passed since the last time this track was played
	iDelay = sound_GetGameTime() - sound_GetTrackTimeLastFinished( iTrack );
	if ( !(iDelay > iMinDelay) )
	{
		return;
	}

	// Construct an audio sample from requested track
	psSample = audio_QueueSample( iTrack );
	if ( psSample == NULL )
	{
		return;
	}

	// Set last finished tracktime to current time to prevent parallel playing of this track
	sound_SetTrackTimeLastFinished( iTrack, sound_GetGameTime() );
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void audio_QueueTrackMinDelayPos( SDWORD iTrack, UDWORD iMinDelay, SDWORD iX, SDWORD iY, SDWORD iZ )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE	*psSample;
	UDWORD			iDelay;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// return if audio not enabled
	if ( g_bAudioEnabled == FALSE || g_bAudioPaused == TRUE )
	{
		return;
	}

	// Determine if at least iMinDelay time has passed since the last time this track was played
	iDelay = sound_GetGameTime() - sound_GetTrackTimeLastFinished( iTrack );
	if ( iDelay > iMinDelay )
	{
		return;
	}

	// Construct an audio sample from requested track
	psSample = audio_QueueSample( iTrack );
	if ( psSample == NULL )
	{
		return;
	}

	psSample->x = iX;
	psSample->y = iY;
	psSample->z = iZ;

	// Set last finished tracktime to current time to prevent parallel playing of this track
	sound_SetTrackTimeLastFinished( iTrack, sound_GetGameTime() );
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void audio_QueueTrackPos( SDWORD iTrack, SDWORD iX, SDWORD iY, SDWORD iZ )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE	*psSample;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// return if audio not enabled
	if ( g_bAudioEnabled == FALSE || g_bAudioPaused == TRUE )
	{
		return;
	}

	// Construct an audio sample from requested track
	psSample = audio_QueueSample( iTrack );
	if ( psSample == NULL )
	{
		return;
	}

	psSample->x = iX;
	psSample->y = iY;
	psSample->z = iZ;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
static void audio_UpdateQueue( void )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE	*psSample;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// return if audio not enabled
	if ( g_bAudioEnabled == FALSE || g_bAudioPaused == TRUE )
	{
		return;
	}

	if ( sound_QueueSamplePlaying() == TRUE )
	{
		// lower volume whilst playing queue audio
		audio_Set3DVolume( LOWERED_VOL );
		return;
	}

	// set full global volume
	audio_Set3DVolume( AUDIO_VOL_MAX );

	// check queue for members
	if ( g_psSampleQueue == NULL )
	{
		return;
	}

	// remove queue head
	psSample = g_psSampleQueue;
	audio_RemoveSample( &g_psSampleQueue, psSample );

	// add sample to list if able to play
	if ( !sound_Play2DTrack(psSample, TRUE) )
	{
		debug( LOG_NEVER, "audio_UpdateQueue: couldn't play sample\n" );
		free(psSample);
		return;
	}

	audio_AddSampleToHead( &g_psSampleList, psSample );

	// update last queue sound coords
	if ( psSample->x != SAMPLE_COORD_INVALID && psSample->y != SAMPLE_COORD_INVALID
	  && psSample->z != SAMPLE_COORD_INVALID )
	{
		g_sPreviousSample.x = psSample->x;
		g_sPreviousSample.y = psSample->y;
		g_sPreviousSample.z = psSample->z;
	}
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void audio_Update( void )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Vector3i		vecPlayer;
	SDWORD			iA;
	AUDIO_SAMPLE	*psSample, *psSampleTemp;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// if audio not enabled return TRUE to carry on game without audio
	if ( g_bAudioEnabled == FALSE )
	{
		return;
	}

	audio_UpdateQueue();

	// get player position
	audio_Get3DPlayerPos(&vecPlayer.x, &vecPlayer.y, &vecPlayer.z);

	sound_SetPlayerPos( vecPlayer.x, vecPlayer.y, vecPlayer.z );
	audio_Get3DPlayerRotAboutVerticalAxis( &iA );
	sound_SetPlayerOrientation( 0, 0, iA );

	// loop through 3D sounds and remove if finished or update position
	psSample = g_psSampleList;
	while ( psSample != NULL )
	{
		// remove finished samples from list
		if ( psSample->bFinishedPlaying == TRUE )
		{
			audio_RemoveSample( &g_psSampleList, psSample );
			psSampleTemp = psSample->psNext;
			free(psSample);
			psSample = psSampleTemp;
		}

		// check looping sound callbacks for finished condition
		else
		{
			if ( psSample->psObj != NULL )
			{
				if ( audio_ObjectDead(psSample->psObj)
				 || (psSample->pCallback != NULL && psSample->pCallback(psSample->psObj) == FALSE) )
				{
					sound_StopTrack( psSample );
					psSample->psObj = NULL;
				}
				else
				{
					// update sample position
					{
						audio_GetObjectPos( psSample->psObj, &psSample->x, &psSample->y, &psSample->z );
#ifndef WZ_NOSOUND
						sound_SetObjectPosition( psSample->iSample, psSample->x, psSample->y, psSample->z );
#endif
					}
				}
			}

			// next sample
			psSample = psSample->psNext;
		}
	}

	sound_Update();
	return;
}


/** Loads audio files and constructs a TRACK from them and returns their respective ID numbers
 *  \param szFileName the filename of the track
 *  \param bLoop whether the track should be looped until explicitly stopped
 *  \param iTrack[out] the track id number is returned into the variable this pointer points to
 *  \param iVol the volume this track should be played on (range is 0-100)
 *  \param iAudibleRadius the radius from the source of sound where it can be heard
 *  \return TRUE when succesfull or audio is disabled, FALSE when the file is not found or no more tracks can be loaded (i.e. the limit is reached)
 */
BOOL audio_SetTrackVals
	(
		const char	*fileName,
		BOOL	bLoop,
		int		*iTrack,
		int		iVol,
		int		iAudibleRadius
	)
{
	//~~~~~~~~~~~~~
	TRACK	*psTrack;
	//~~~~~~~~~~~~~

	// if audio not enabled return TRUE to carry on game without audio
	if ( g_bAudioEnabled == FALSE )
	{
		return TRUE;
	}

	// get track pointer from resource
	psTrack = (TRACK*)resGetData( "WAV", fileName );		//at this point we have 4 valid entries, and 8 invalid -Q
	if ( psTrack == NULL )
	{
		debug( LOG_NEVER, "audio_SetTrackVals: track %s resource not found\n", fileName );
		return FALSE;
	}

	// get current ID or spare one
	if ( (*iTrack = audio_GetIDFromStr(fileName)) == NO_SOUND )
	{
		*iTrack = sound_GetAvailableID();
	}

	if ( *iTrack == SAMPLE_NOT_ALLOCATED )
	{
		debug( LOG_NEVER, "audio_SetTrackVals: couldn't get spare track ID\n" );
		return FALSE;
	}

	return sound_SetTrackVals( psTrack, bLoop, *iTrack, iVol, iAudibleRadius );	//now psTrack should be fully set. -Q
}

//*
//
//
// * audio_CheckSame3DTracksPlaying Reject samples if too many already playing in
// * same area
//

//*
// =======================================================================================================================
// =======================================================================================================================
//
static BOOL audio_CheckSame3DTracksPlaying( SDWORD iTrack, SDWORD iX, SDWORD iY, SDWORD iZ )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	SDWORD			iCount, iDx, iDy, iDz, iDistSq, iMaxDistSq, iRad;
	AUDIO_SAMPLE	*psSample = NULL;
	BOOL			bOK = TRUE;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// return if audio not enabled
	if ( g_bAudioEnabled == FALSE || g_bAudioPaused == TRUE )
	{
		return TRUE;
	}

	iCount = 0;

	// loop through 3D sounds and check whether too many already in earshot
	psSample = g_psSampleList;
	while ( psSample != NULL )
	{
		if ( psSample->iTrack == iTrack )
		{
			iDx = iX - psSample->x;
			iDy = iY - psSample->y;
			iDz = iZ - psSample->z;
			iDistSq = ( iDx * iDx ) + ( iDy * iDy ) + ( iDz * iDz );
			iRad = sound_GetTrackAudibleRadius( iTrack );
			iMaxDistSq = iRad * iRad;
			if ( iDistSq < iMaxDistSq )
			{
				iCount++;
			}

			if ( iCount > MAX_SAME_SAMPLES )
			{
				bOK = FALSE;
				break;
			}
		}

		psSample = psSample->psNext;
	}

	return bOK;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
static BOOL audio_Play3DTrack( SDWORD iX, SDWORD iY, SDWORD iZ, int iTrack, void *psObj, AUDIO_CALLBACK pUserCallback )
{
	//~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE	*psSample;
	//~~~~~~~~~~~~~~~~~~~~~~

	// if audio not enabled return TRUE to carry on game without audio
	if ( g_bAudioEnabled == FALSE || g_bAudioPaused == TRUE || g_bStopAll == TRUE )
	{
		return FALSE;
	}

	if ( audio_CheckSame3DTracksPlaying(iTrack, iX, iY, iZ) == FALSE )
	{
		return FALSE;
	}

	psSample = malloc(sizeof(AUDIO_SAMPLE));
	if ( psSample == NULL )
	{
		debug(LOG_ERROR, "audio_Play3DTrack: Out of memory");
		return FALSE;
	}

	// setup sample
	memset( psSample, 0, sizeof(AUDIO_SAMPLE) );						// [check] -Q
	psSample->iTrack = iTrack;
	psSample->x = iX;
	psSample->y = iY;
	psSample->z = iZ;
	psSample->bFinishedPlaying = FALSE;
	psSample->psObj = psObj;
	psSample->pCallback = pUserCallback;

	// add sample to list if able to play
	if ( !sound_Play3DTrack(psSample) )
	{
		debug( LOG_NEVER, "audio_Play3DTrack: couldn't play sample\n" );
		free(psSample);
		return FALSE;
	}

	audio_AddSampleToHead( &g_psSampleList, psSample );
	return TRUE;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL audio_PlayStaticTrack( SDWORD iMapX, SDWORD iMapY, int iTrack )
{
	//~~~~~~~~~~~~~~~
	SDWORD	iX, iY, iZ;
	//~~~~~~~~~~~~~~~

	// if audio not enabled return TRUE to carry on game without audio
	if ( g_bAudioEnabled == FALSE )
	{
		return FALSE;
	}

	audio_GetStaticPos( iMapX, iMapY, &iX, &iY, &iZ );
	return audio_Play3DTrack( iX, iY, iZ, iTrack, NULL, NULL );
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL audio_PlayObjStaticTrack( void *psObj, int iTrack )
{
	//~~~~~~~~~~~~~~~
	SDWORD	iX, iY, iZ;
	//~~~~~~~~~~~~~~~

	// if audio not enabled return TRUE to carry on game without audio
	if ( g_bAudioEnabled == FALSE )
	{
		return FALSE;
	}

	audio_GetObjectPos( psObj, &iX, &iY, &iZ );
	return audio_Play3DTrack( iX, iY, iZ, iTrack, psObj, NULL );
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL audio_PlayObjStaticTrackCallback( void *psObj, int iTrack, AUDIO_CALLBACK pUserCallback )
{
	//~~~~~~~~~~~~~~~
	SDWORD	iX, iY, iZ;
	//~~~~~~~~~~~~~~~

	// if audio not enabled return TRUE to carry on game without audio
	if ( g_bAudioEnabled == FALSE )
	{
		return FALSE;
	}

	audio_GetObjectPos( psObj, &iX, &iY, &iZ );
	return audio_Play3DTrack( iX, iY, iZ, iTrack, psObj, pUserCallback );
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL audio_PlayObjDynamicTrack( void *psObj, int iTrack, AUDIO_CALLBACK pUserCallback )
{
	//~~~~~~~~~~~~~~~
	SDWORD	iX, iY, iZ;
	//~~~~~~~~~~~~~~~

	// if audio not enabled return TRUE to carry on game without audio
	if ( g_bAudioEnabled == FALSE )
	{
		return FALSE;
	}

	audio_GetObjectPos( psObj, &iX, &iY, &iZ );
	return audio_Play3DTrack( iX, iY, iZ, iTrack, psObj, pUserCallback );
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL audio_PlayStream( const char szFileName[], SDWORD iVol, AUDIO_CALLBACK pUserCallback )
{
	//~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE	*psSample;
	//~~~~~~~~~~~~~~~~~~~~~~

	// if audio not enabled return TRUE to carry on game without audio
	if ( g_bAudioEnabled == FALSE )
	{
		return FALSE;
	}

	psSample = malloc(sizeof(AUDIO_SAMPLE));
	if ( psSample == NULL )
	{
		debug(LOG_ERROR, "audio_PlayStream: Out of memory");
		return FALSE;
	}

	memset( psSample, 0, sizeof(AUDIO_SAMPLE) );
	psSample->pCallback = pUserCallback;
	psSample->bFinishedPlaying = FALSE;
	audio_Set3DVolume( AUDIO_VOL_MAX );

	if ( !sound_PlayStream(psSample, szFileName, iVol) )
	{
		return FALSE;
	}

	return TRUE;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void audio_StopObjTrack( void *psObj, int iTrack )
{
	//~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE	*psSample;
	//~~~~~~~~~~~~~~~~~~~~~~

	// return if audio not enabled
	if ( g_bAudioEnabled == FALSE || g_bStopAll == TRUE )
	{
		return;
	}

	// find sample
	psSample = g_psSampleList;
	while ( psSample != NULL )
	{
		// If track has been found stop it and return
		if ( psSample->psObj == psObj && psSample->iTrack == iTrack )
		{
			sound_StopTrack(psSample);
			return;
		}

		// get next sample from linked list
		psSample = psSample->psNext;
	}
}

//*
//
// audio_PlayTrack Play immediate 2D FX track

//*
// =======================================================================================================================
// =======================================================================================================================
//
void audio_PlayTrack( int iTrack )
{
	//~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE	*psSample;
	//~~~~~~~~~~~~~~~~~~~~~~

	// return if audio not enabled
	if ( g_bAudioEnabled == FALSE || g_bAudioPaused == TRUE || g_bStopAll == TRUE )
	{
		return;
	}

	// Allocate a sample
	psSample = malloc(sizeof(AUDIO_SAMPLE));
	if ( psSample == NULL )
	{
		debug(LOG_ERROR, "audio_PlayTrack: Out of memory");
		return;
	}

	// setup/initialize sample
	psSample->iTrack = iTrack;
	psSample->bFinishedPlaying = FALSE;

	// Zero callback stuff since we don't need/want it
	psSample->pCallback = NULL;
	psSample->psObj = NULL;

	/* iSample, psPrev, and psNext will be initialized by the
	 * following functions, and x, y and z will be completely
	 * ignored so we don't need to bother about it
	 */

	// add sample to list if able to play
	if ( !sound_Play2DTrack(psSample, FALSE) )
	{
		debug( LOG_NEVER, "audio_PlayTrack: couldn't play sample\n" );
		free(psSample);
		return;
	}

	audio_AddSampleToHead( &g_psSampleList, psSample );
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void audio_PauseAll( void )
{
	// return if audio not enabled
	if ( g_bAudioEnabled == FALSE )
	{
		return;
	}

	g_bAudioPaused = TRUE;
	sound_PauseAll();
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void audio_ResumeAll( void )
{
	// return if audio not enabled
	if ( g_bAudioEnabled == FALSE )
	{
		return;
	}

	g_bAudioPaused = FALSE;
	sound_ResumeAll();
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void audio_StopAll( void )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE	*psSample, *psSampleTemp;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// return if audio not enabled
	if ( g_bAudioEnabled == FALSE )
	{
		return;
	}

	debug( LOG_NEVER, "audio_StopAll called\n" );
	g_bStopAll = TRUE;

	//
	// * empty list - audio_Update will free samples because callbacks have to come in
	// * first
	//
	psSample = g_psSampleList;
	while ( psSample != NULL )
	{
		sound_StopTrack( psSample );
		psSample = psSample->psNext;
	}

	// empty sample queue
	psSample = g_psSampleQueue;
	while ( psSample != NULL )
	{
		psSampleTemp = psSample->psNext;
		free(psSample);
		psSample = psSampleTemp;
	}

	g_psSampleQueue = NULL;
	g_bStopAll = FALSE;
	debug( LOG_NEVER, "audio_StopAll done\n" );
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
SDWORD audio_GetTrackID( const char *fileName )
{
	//~~~~~~~~~~~~~
	TRACK	*psTrack;
	//~~~~~~~~~~~~~

	// return if audio not enabled
	if ( g_bAudioEnabled == FALSE )
	{
		return SAMPLE_NOT_FOUND;
	}

	psTrack = (TRACK*)resGetData( "WAV", fileName );
	if ( psTrack == NULL )
	{
		return SAMPLE_NOT_FOUND;
	}

	return sound_GetTrackID( psTrack );
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
SDWORD audio_Get3DVolume( void )
{
	return g_i3DVolume;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void audio_Set3DVolume( SDWORD iVol )
{
	g_i3DVolume = iVol;
}

//*
//
// audio_GetMixVol iVol and audio_Get3DVolume need to be scaled by AUDIO_VOL_RANGE

//*
// =======================================================================================================================
// =======================================================================================================================
//
SDWORD audio_GetMixVol( SDWORD iVol )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	SDWORD	iMixVol = ( iVol * sound_GetMaxVolume() * audio_Get3DVolume() ) / ( AUDIO_VOL_RANGE * AUDIO_VOL_RANGE );
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	return iMixVol;
}
