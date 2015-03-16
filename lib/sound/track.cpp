/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2013  Warzone 2100 Project

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

#include "tracklib.h"
#include "lib/framework/frame.h"
#include "lib/framework/frameresource.h"
#include "audio_id.h"
#include "src/droid.h"

//*
//
// defines
#define MAX_TRACKS	( 600 )

//*
//
// static global variables
// array of pointers to sound effects
static TRACK			*g_apTrack[MAX_TRACKS];

// number of tracks loaded
static SDWORD			g_iCurTracks = 0;

// flag set when system is active (for callbacks etc)
static bool				g_bSystemActive = false;
static AUDIO_CALLBACK	g_pStopTrackCallback = NULL;

//*
// =======================================================================================================================
// =======================================================================================================================
//
bool sound_Init()
{
	g_iCurTracks = 0;
	if (!sound_InitLibrary())
	{
		debug(LOG_ERROR, "Cannot init sound library");
		return false;
	}

	// init audio array (with NULL pointers; which calloc ensures by setting all allocated memory to zero)
	memset(g_apTrack, 0, sizeof(g_apTrack));

	// set system active flag for callbacks
	g_bSystemActive = true;
	return true;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
bool sound_Shutdown(void)
{
	// set inactive flag to prevent callbacks coming after shutdown
	g_bSystemActive = false;
	sound_ShutdownLibrary();
	return true;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
bool sound_GetSystemActive(void)
{
	return g_bSystemActive;
}

/** Retrieves loaded audio files and retrieves a TRACK from them on which some values are set and returns their respective ID numbers
 *  \param fileName the filename of the track
 *  \param loop whether the track should be looped until explicitly stopped
 *  \param volume the volume this track should be played on (range is 0-100)
 *  \param audibleRadius the radius from the source of sound where it can be heard
 *  \return a non-zero value representing the track id number when successful, zero when the file is not found or no more tracks can be loaded (i.e. the limit is reached)
 */
unsigned int sound_SetTrackVals(const char *fileName, bool loop, unsigned int volume, unsigned int audibleRadius)
{
	unsigned int trackID;
	TRACK *psTrack;

	if (fileName == NULL || strlen(fileName) == 0) // check for empty filename.  This is a non fatal error.
	{
		debug(LOG_WARNING, "fileName is %s", (fileName == NULL) ? "a NULL pointer" : "empty");
		return 0;
	}

	psTrack = (TRACK *)resGetData("WAV", fileName);
	if (psTrack == NULL)
	{
		debug(LOG_WARNING, "track %s resource not found", fileName);
		return 0;
	}

	// get pre-assigned ID or produce one
	trackID = audio_GetIDFromStr(fileName);
	if (trackID == NO_SOUND)
	{
		// No pre-assigned ID available, produce one
		trackID = sound_GetAvailableID();
	}

	if (g_apTrack[trackID] != NULL)
	{
		debug(LOG_ERROR, "sound_SetTrackVals: track %i already set (filename: \"%s\"\n", trackID, g_apTrack[trackID]->fileName);
		return 0;
	}

	// set track members
	psTrack->bLoop = loop;
	psTrack->iVol = volume;
	psTrack->iAudibleRadius = audibleRadius;

	// RAII: Make sure to initialize all values (we don't want undefined values)!
	psTrack->iTime = 0;
	psTrack->iTimeLastFinished = 0;
	psTrack->iNumPlaying = 0;

	// set global
	g_apTrack[trackID] = psTrack;

	// increment counter for amount of loaded tracks
	++g_iCurTracks;

	return trackID;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_ReleaseTrack(TRACK *psTrack)
{
	TRACK **currTrack;

	// This is here to save CPU wasted by the loop below;
	// Calling this function with psTrack = NULL is perfectly legal,
	// with or without this check
	if (!psTrack)
	{
		return;
	}

	// Run through the list of tracks and set the pointer to the track which is to be released to NULL
	for (currTrack = &g_apTrack[0]; currTrack != &g_apTrack[MAX_TRACKS]; ++currTrack)
	{
		if (*currTrack == psTrack)
		{
			*currTrack = NULL;
		}
	}

	sound_FreeTrack(psTrack);
	free(psTrack);
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_CheckAllUnloaded(void)
{
	TRACK **currTrack;

	for (currTrack = &g_apTrack[0]; currTrack != &g_apTrack[MAX_TRACKS]; ++currTrack)
	{
		ASSERT(*currTrack == NULL, "A track is not unloaded yet (%s); check audio.cfg for duplicate IDs", (*currTrack)->fileName);
	}
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
bool sound_TrackLooped(SDWORD iTrack)
{
	sound_CheckTrack(iTrack);
	return g_apTrack[iTrack]->bLoop;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
SDWORD sound_GetNumPlaying(SDWORD iTrack)
{
	sound_CheckTrack(iTrack);
	return g_apTrack[iTrack]->iNumPlaying;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
bool sound_CheckTrack(SDWORD iTrack)
{
	if (iTrack < 0 || iTrack > g_iCurTracks - 1)
	{
		debug(LOG_SOUND, "Track number %i outside max %i\n", iTrack, g_iCurTracks);
		return false;
	}

	if (g_apTrack[iTrack] == NULL)
	{
		debug(LOG_SOUND, "Track %i NULL\n", iTrack);
		return false;
	}

	return true;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
SDWORD sound_GetTrackTime(SDWORD iTrack)
{
	sound_CheckTrack(iTrack);
	return g_apTrack[iTrack]->iTime;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
SDWORD sound_GetTrackVolume(SDWORD iTrack)
{
	sound_CheckTrack(iTrack);
	return g_apTrack[iTrack]->iVol;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
SDWORD sound_GetTrackAudibleRadius(SDWORD iTrack)
{
	sound_CheckTrack(iTrack);
	return g_apTrack[iTrack]->iAudibleRadius;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
const char *sound_GetTrackName(SDWORD iTrack)
{
	// If we get an invalid track ID or there are
	// currently no tracks loaded then return NULL
	if (iTrack <= 0
	    || iTrack >= MAX_TRACKS
	    || iTrack == SAMPLE_NOT_FOUND
	    || g_apTrack == NULL
	    || g_apTrack[iTrack] == NULL)
	{
		return NULL;
	}

	return g_apTrack[iTrack]->fileName;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
bool sound_Play2DTrack(AUDIO_SAMPLE *psSample, bool bQueued)
{
	TRACK	*psTrack;

	// Check to make sure the requested track is loaded
	if (!sound_CheckTrack(psSample->iTrack))
	{
		return false;
	}

	psTrack = g_apTrack[psSample->iTrack];
	return sound_Play2DSample(psTrack, psSample, bQueued);
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
bool sound_Play3DTrack(AUDIO_SAMPLE *psSample)
{
	TRACK	*psTrack;

	// Check to make sure the requested track is loaded
	if (!sound_CheckTrack(psSample->iTrack))
	{
		return false;
	}

	psTrack = g_apTrack[psSample->iTrack];
	return sound_Play3DSample(psTrack, psSample);
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_StopTrack(AUDIO_SAMPLE *psSample)
{
	ASSERT_OR_RETURN(, psSample != NULL, "Sample pointer invalid");

#ifndef WZ_NOSOUND
	sound_StopSample(psSample);
#endif

	// do stopped callback
	if (g_pStopTrackCallback != NULL && psSample->psObj != NULL)
	{
		g_pStopTrackCallback(psSample->psObj);
	}
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_PauseTrack(AUDIO_SAMPLE *psSample)
{
#ifndef WZ_NOSOUND
	sound_StopSample(psSample);
#endif
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_FinishedCallback(AUDIO_SAMPLE *psSample)
{
	ASSERT(psSample != NULL, "sound_FinishedCallback: sample pointer invalid\n");

	if (g_apTrack[psSample->iTrack] != NULL)
	{
		g_apTrack[psSample->iTrack]->iTimeLastFinished = sound_GetGameTime();
	}

	// call user callback if specified
	if (psSample->pCallback != NULL)
	{
		psSample->pCallback(psSample->psObj);
		// NOTE: we must invalidate the iAudioID (iTrack) so the game knows to add the
		// sample back into the queue.  This is a bit of a hacky way to handle "looped" samples,
		// since currently, the game don't have a decent way to handle this.
		if (psSample->psObj)
		{
			droidAudioTrackStopped((void *)psSample->psObj);
		}
	}

	// set finished flag
	psSample->bFinishedPlaying = true;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
SDWORD sound_GetTrackID(TRACK *psTrack)
{
	unsigned int i;

	if (psTrack == NULL)
	{
		return SAMPLE_NOT_FOUND;
	}

	// find matching track
	for (i = 0; i < MAX_TRACKS; ++i)
	{
		if (g_apTrack[i] == psTrack)
		{
			break;
		}
	}

	// if matching track found return it else find empty track
	if (i >= MAX_TRACKS)
	{
		return SAMPLE_NOT_FOUND;
	}

	return i;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
SDWORD sound_GetAvailableID()
{
	unsigned int i;

	// Iterate through the list of tracks until we find an unused ID slot
	for (i = ID_SOUND_NEXT; i < MAX_TRACKS; ++i)
	{
		if (g_apTrack[i] == NULL)
		{
			break;
		}
	}

	ASSERT(i < MAX_TRACKS, "sound_GetTrackID: unused track not found!");
	if (i >= MAX_TRACKS)
	{
		return SAMPLE_NOT_ALLOCATED;
	}

	return i;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_SetStoppedCallback(AUDIO_CALLBACK pStopTrackCallback)
{
	g_pStopTrackCallback = pStopTrackCallback;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
UDWORD sound_GetTrackTimeLastFinished(SDWORD iTrack)
{
	sound_CheckTrack(iTrack);
	return g_apTrack[iTrack]->iTimeLastFinished;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_SetTrackTimeLastFinished(SDWORD iTrack, UDWORD iTime)
{
	sound_CheckTrack(iTrack);
	g_apTrack[iTrack]->iTimeLastFinished = iTime;
}
