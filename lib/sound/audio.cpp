/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
#include "lib/framework/math_ext.h"
#include "lib/gamelib/gtime.h"
#include "lib/ivis_opengl/pietypes.h"
#include "lib/framework/physfs_ext.h"

#include "tracklib.h"
#include "aud.h"
#include "audio.h"
#include "audio_id.h"
#include "openal_error.h"
#include "mixer.h"
// defines
#define NO_SAMPLE				- 2
#define MAX_SAME_SAMPLES		2

// global variables
static AUDIO_SAMPLE *g_psSampleList = nullptr;
static AUDIO_SAMPLE *g_psSampleQueue = nullptr;
static bool			g_bAudioEnabled = false;
static bool			g_bAudioPaused = false;
static AUDIO_SAMPLE g_sPreviousSample;
static int			g_iPreviousSampleTime = 0;

/** Counts the number of samples in the SampleQueue
 *  \return the number of samples in the SampleQueue
 */
unsigned int audio_GetSampleQueueCount()
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE	*psSample = nullptr;
	unsigned int	count = 0;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// loop through SampleQueue to count how many we have.
	psSample = g_psSampleQueue;
	while (psSample != nullptr)
	{
		count++;
		psSample = psSample->psNext;
	}

	return count;
}

/** Counts the number of samples in the SampleList
 *  \return the number of samples in the SampleList
 */
unsigned int audio_GetSampleListCount()
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE *psSample = nullptr;
	unsigned int count = 0;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// loop through SampleList to count how many we have.
	psSample = g_psSampleList;
	while (psSample != nullptr)
	{
		++count;
		psSample = psSample->psNext;
	}

	return count;
}
//*
// =======================================================================================================================
// =======================================================================================================================
//
bool audio_Disabled(void)
{
	return !g_bAudioEnabled;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
bool audio_Init(AUDIO_CALLBACK pStopTrackCallback, bool really_enable)
{
	// init audio system
	g_sPreviousSample.iTrack = NO_SAMPLE;
	g_sPreviousSample.x = SAMPLE_COORD_INVALID;
	g_sPreviousSample.y = SAMPLE_COORD_INVALID;
	g_sPreviousSample.z = SAMPLE_COORD_INVALID;
	g_bAudioEnabled = really_enable;
	if (g_bAudioEnabled)
	{
		g_bAudioEnabled = sound_Init();
	}
	if (g_bAudioEnabled)
	{
		sound_SetStoppedCallback(pStopTrackCallback);
	}
	return g_bAudioEnabled;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
bool audio_Shutdown(void)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE	*psSample = nullptr, *psSampleTemp = nullptr;
	bool			bOK;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// if audio not enabled return true to carry on game without audio
	if (g_bAudioEnabled == false)
	{
		return true;
	}

	sound_StopAll();
	bOK = sound_Shutdown();

	// empty sample list
	psSample = g_psSampleList;
	while (psSample != nullptr)
	{
		psSampleTemp = psSample->psNext;
		free(psSample);
		psSample = psSampleTemp;
	}

	// empty sample queue
	psSample = g_psSampleQueue;
	while (psSample != nullptr)
	{
		psSampleTemp = psSample->psNext;
		free(psSample);
		psSample = psSampleTemp;
	}

	// free sample heap
	g_psSampleList = nullptr;
	g_psSampleQueue = nullptr;

	return bOK;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
bool audio_GetPreviousQueueTrackPos(SDWORD *iX, SDWORD *iY, SDWORD *iZ)
{
	if (g_sPreviousSample.x == SAMPLE_COORD_INVALID
	    || g_sPreviousSample.y == SAMPLE_COORD_INVALID
	    || g_sPreviousSample.z == SAMPLE_COORD_INVALID)
	{
		return false;
	}

	*iX = g_sPreviousSample.x;
	*iY = g_sPreviousSample.y;
	*iZ = g_sPreviousSample.z;

	return true;
}

bool audio_GetPreviousQueueTrackRadarBlipPos(SDWORD *iX, SDWORD *iY)
{
	if (g_sPreviousSample.x == SAMPLE_COORD_INVALID
	    || g_sPreviousSample.y == SAMPLE_COORD_INVALID)
	{
		return false;
	}

	if (g_sPreviousSample.iTrack != ID_SOUND_STRUCTURE_UNDER_ATTACK && g_sPreviousSample.iTrack != ID_SOUND_UNIT_UNDER_ATTACK &&
	    g_sPreviousSample.iTrack != ID_SOUND_LASER_SATELLITE_FIRING && g_sPreviousSample.iTrack != ID_SOUND_INCOMING_LASER_SAT_STRIKE)
	{
		return false;
	}

	if (realTime > g_iPreviousSampleTime + 5 * GAME_TICKS_PER_SEC)
	{
		return false;
	}

	*iX = g_sPreviousSample.x;
	*iY = g_sPreviousSample.y;

	return true;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
static void audio_AddSampleToHead(AUDIO_SAMPLE **ppsSampleList, AUDIO_SAMPLE *psSample)
{
	psSample->psNext = (*ppsSampleList);
	psSample->psPrev = nullptr;
	if ((*ppsSampleList) != nullptr)
	{
		(*ppsSampleList)->psPrev = psSample;
	}
	(*ppsSampleList) = psSample;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
static void audio_AddSampleToTail(AUDIO_SAMPLE **ppsSampleList, AUDIO_SAMPLE *psSample)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE	*psSampleTail = nullptr;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if ((*ppsSampleList) == nullptr)
	{
		(*ppsSampleList) = psSample;
		return;
	}

	psSampleTail = (*ppsSampleList);
	while (psSampleTail->psNext != nullptr)
	{
		psSampleTail = psSampleTail->psNext;
	}

	psSampleTail->psNext = psSample;
	psSample->psPrev = psSampleTail;
	psSample->psNext = nullptr;
}

//*
//
// audio_RemoveSample Removes sample from list but doesn't free its memory

//*
// =======================================================================================================================
// =======================================================================================================================
//
static void audio_RemoveSample(AUDIO_SAMPLE **ppsSampleList, AUDIO_SAMPLE *psSample)
{
	if (psSample == nullptr)
	{
		return;
	}

	if (psSample == (*ppsSampleList))
	{
		// first sample in list
		(*ppsSampleList) = psSample->psNext;
	}

	if (psSample->psPrev != nullptr)
	{
		psSample->psPrev->psNext = psSample->psNext;
	}

	if (psSample->psNext != nullptr)
	{
		psSample->psNext->psPrev = psSample->psPrev;
	}

	// set sample pointers NULL for safety
	psSample->psPrev = nullptr;
	psSample->psNext = nullptr;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
static bool audio_CheckSameQueueTracksPlaying(SDWORD iTrack)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	SDWORD			iCount;
	AUDIO_SAMPLE	*psSample = nullptr;
	bool			bOK = true;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// return if audio not enabled
	if (g_bAudioEnabled == false || g_bAudioPaused == true)
	{
		return true;
	}

	iCount = 0;

	// loop through queue sounds and check whether too many already in it
	psSample = g_psSampleQueue;
	while (psSample != nullptr)
	{
		if (psSample->iTrack == iTrack)
		{
			iCount++;
		}

		if (iCount > MAX_SAME_SAMPLES)
		{
			bOK = false;
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
static AUDIO_SAMPLE *audio_QueueSample(SDWORD iTrack)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE	*psSample = nullptr;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// return if audio not enabled
	if (g_bAudioEnabled == false || g_bAudioPaused == true)
	{
		return nullptr;
	}

	ASSERT(sound_CheckTrack(iTrack) == true, "audio_QueueSample: track %i outside limits\n", iTrack);

	// reject track if too many of same ID already in queue
	if (audio_CheckSameQueueTracksPlaying(iTrack) == false)
	{
		return nullptr;
	}

	psSample = (AUDIO_SAMPLE *)calloc(1, sizeof(AUDIO_SAMPLE));
	if (psSample == nullptr)
	{
		debug(LOG_ERROR, "audio_QueueSample: Out of memory");
		return nullptr;
	}

	psSample->iTrack = iTrack;
	psSample->x = SAMPLE_COORD_INVALID;
	psSample->y = SAMPLE_COORD_INVALID;
	psSample->z = SAMPLE_COORD_INVALID;
	psSample->bFinishedPlaying = false;

	// add to queue
	audio_AddSampleToTail(&g_psSampleQueue, psSample);

	return psSample;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void audio_QueueTrack(SDWORD iTrack)
{
	// return if audio not enabled
	if (g_bAudioEnabled == false || g_bAudioPaused == true)
	{
		return;
	}

	audio_QueueSample(iTrack);
	return;
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
void audio_QueueTrackMinDelay(SDWORD iTrack, UDWORD iMinDelay)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE	*psSample;
	UDWORD			iDelay;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// return if audio not enabled
	if (g_bAudioEnabled == false || g_bAudioPaused == true)
	{
		return;
	}

	// Determine if at least iMinDelay time has passed since the last time this track was played
	iDelay = sound_GetGameTime() - sound_GetTrackTimeLastFinished(iTrack);
	if (!(iDelay > iMinDelay))
	{
		return;
	}

	// Construct an audio sample from requested track
	psSample = audio_QueueSample(iTrack);
	if (psSample == nullptr)
	{
		return;
	}

	// Set last finished tracktime to current time to prevent parallel playing of this track
	sound_SetTrackTimeLastFinished(iTrack, sound_GetGameTime());
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void audio_QueueTrackMinDelayPos(SDWORD iTrack, UDWORD iMinDelay, SDWORD iX, SDWORD iY, SDWORD iZ)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE	*psSample;
	UDWORD			iDelay;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// return if audio not enabled
	if (g_bAudioEnabled == false || g_bAudioPaused == true)
	{
		return;
	}

	// Determine if at least iMinDelay time has passed since the last time this track was played
	iDelay = sound_GetGameTime() - sound_GetTrackTimeLastFinished(iTrack);
	if (iDelay > iMinDelay)
	{
		return;
	}

	// Construct an audio sample from requested track
	psSample = audio_QueueSample(iTrack);
	if (psSample == nullptr)
	{
		return;
	}

	psSample->x = iX;
	psSample->y = iY;
	psSample->z = iZ;

	// Set last finished tracktime to current time to prevent parallel playing of this track
	sound_SetTrackTimeLastFinished(iTrack, sound_GetGameTime());
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void audio_QueueTrackPos(SDWORD iTrack, SDWORD iX, SDWORD iY, SDWORD iZ)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE	*psSample;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// return if audio not enabled
	if (g_bAudioEnabled == false || g_bAudioPaused == true)
	{
		return;
	}

	// Construct an audio sample from requested track
	psSample = audio_QueueSample(iTrack);
	if (psSample == nullptr)
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
static void audio_UpdateQueue(void)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE	*psSample;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// return if audio not enabled
	if (g_bAudioEnabled == false || g_bAudioPaused == true)
	{
		return;
	}

	if (sound_QueueSamplePlaying() == true)
	{
		return;
	}

	// check queue for members
	if (g_psSampleQueue == nullptr)
	{
		return;
	}

	// remove queue head
	psSample = g_psSampleQueue;
	audio_RemoveSample(&g_psSampleQueue, psSample);

	// add sample to list if able to play
	if (!sound_Play2DTrack(psSample, true))
	{
		debug(LOG_NEVER, "audio_UpdateQueue: couldn't play sample\n");
		free(psSample);
		return;
	}

	audio_AddSampleToHead(&g_psSampleList, psSample);

	// update last queue sound coords
	if (psSample->x != SAMPLE_COORD_INVALID && psSample->y != SAMPLE_COORD_INVALID
	    && psSample->z != SAMPLE_COORD_INVALID)
	{
		g_sPreviousSample.x = psSample->x;
		g_sPreviousSample.y = psSample->y;
		g_sPreviousSample.z = psSample->z;
		g_sPreviousSample.iTrack = psSample->iTrack;
		g_iPreviousSampleTime = realTime;
	}
}

void audio_Update()
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Vector3f playerPos;
	float angle;
	AUDIO_SAMPLE	*psSample, *psSampleTemp;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// if audio not enabled return true to carry on game without audio
	if (g_bAudioEnabled == false)
	{
		return;
	}

	alGetError();	// clear error codes
	audio_UpdateQueue();
	alGetError();	// clear error codes

	// get player position
	playerPos = audio_GetPlayerPos();
	audio_Get3DPlayerRotAboutVerticalAxis(&angle);
	sound_SetPlayerPos(playerPos);
	sound_SetPlayerOrientation(angle);

	// loop through 3D sounds and remove if finished or update position
	psSample = g_psSampleList;
	while (psSample != nullptr)
	{
		// remove finished samples from list
		if (psSample->bFinishedPlaying == true)
		{
			psSampleTemp = psSample->psNext;
			audio_RemoveSample(&g_psSampleList, psSample);
			free(psSample);
			psSample = psSampleTemp;
		}

		// check looping sound callbacks for finished condition
		else
		{
			if (psSample->psObj != nullptr)
			{
				if (audio_ObjectDead(psSample->psObj)
				    || (psSample->pCallback != nullptr && psSample->pCallback(psSample->psObj) == false))
				{
					sound_StopTrack(psSample);
					psSample->psObj = nullptr;
				}
				else	// update sample position
				{
					audio_GetObjectPos(psSample->psObj, &psSample->x, &psSample->y, &psSample->z);
					sound_SetObjectPosition(psSample);
				}
			}
			// next sample
			psSample = psSample->psNext;
		}
	}

	sound_Update();
	return;
}


/** Retrieves loaded audio files and retrieves a TRACK from them on which some values are set and returns their respective ID numbers
 *  \param fileName the filename of the track
 *  \param loop whether the track should be looped until explicitly stopped
 *  \param volume the volume this track should be played on (range is 0-100)
 *  \param audibleRadius the radius from the source of sound where it can be heard
 *  \return a non-zero value when successful or audio is disabled, zero when the file is not found or no more tracks can be loaded (i.e. the limit is reached)
 */
unsigned int audio_SetTrackVals(const char *fileName, bool loop, unsigned int volume, unsigned int audibleRadius)
{
	// if audio not enabled return a random non-zero value to carry on game without audio
	if (g_bAudioEnabled == false)
	{
		return 1;
	}

	return sound_SetTrackVals(fileName, loop, volume, audibleRadius);
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
static bool audio_CheckSame3DTracksPlaying(SDWORD iTrack, SDWORD iX, SDWORD iY, SDWORD iZ)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	SDWORD			iCount, iDx, iDy, iDz, iDistSq, iMaxDistSq, iRad;
	AUDIO_SAMPLE	*psSample = nullptr;
	bool			bOK = true;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// return if audio not enabled
	if (g_bAudioEnabled == false || g_bAudioPaused == true)
	{
		return true;
	}

	iCount = 0;

	// loop through 3D sounds and check whether too many already in earshot
	psSample = g_psSampleList;
	while (psSample != nullptr)
	{
		if (psSample->iTrack == iTrack)
		{
			iDx = iX - psSample->x;
			iDy = iY - psSample->y;
			iDz = iZ - psSample->z;
			iDistSq = (iDx * iDx) + (iDy * iDy) + (iDz * iDz);
			iRad = sound_GetTrackAudibleRadius(iTrack);
			iMaxDistSq = iRad * iRad;
			if (iDistSq < iMaxDistSq)
			{
				iCount++;
			}

			if (iCount > MAX_SAME_SAMPLES)
			{
				bOK = false;
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
static bool audio_Play3DTrack(SDWORD iX, SDWORD iY, SDWORD iZ, int iTrack, SIMPLE_OBJECT *psObj, AUDIO_CALLBACK pUserCallback)
{
	AUDIO_SAMPLE	*psSample;
	// coordinates
	float	listenerX = .0f, listenerY = .0f, listenerZ = .0f, dX, dY, dZ;
	// calculation results
	float	distance, gain;
	ALenum err;

	// if audio not enabled return true to carry on game without audio
	if (g_bAudioEnabled == false || g_bAudioPaused == true)
	{
		return false;
	}

	if (audio_CheckSame3DTracksPlaying(iTrack, iX, iY, iZ) == false)
	{
		return false;
	}

	// compute distance
	// NOTE, if this call fails, expect garbage
	alGetListener3f(AL_POSITION, &listenerX, &listenerY, &listenerZ);
	err = sound_GetError();
	if (err != AL_NO_ERROR)
	{
		return false;
	}
	dX = (float)iX - listenerX; // distances on all axis
	dY = (float)iY - listenerY;
	dZ = (float)iZ - listenerZ;
	distance = sqrtf(dX * dX + dY * dY + dZ * dZ); // Pythagorean theorem

	// compute gain
	gain = (1.0 - (distance * ATTENUATION_FACTOR)) ;//* 1.0f * sfx3d_volume
	if (gain > 1.0f)
	{
		gain = 1.0f;
	}
	if (gain < 0.0f)
	{
		gain = 0.0f;
	}
	// don't bother adding samples that we can't hear
	if (gain == 0.0f)
	{
		return false;
	}

	psSample = (AUDIO_SAMPLE *)malloc(sizeof(AUDIO_SAMPLE));
	if (psSample == nullptr)
	{
		debug(LOG_ERROR, "audio_Play3DTrack: Out of memory");
		return false;
	}

	// setup sample
	memset(psSample, 0, sizeof(AUDIO_SAMPLE));						// [check] -Q
	psSample->iTrack = iTrack;
	psSample->x = iX;
	psSample->y = iY;
	psSample->z = iZ;
	psSample->bFinishedPlaying = false;
	psSample->psObj = psObj;
	psSample->pCallback = pUserCallback;

	// add sample to list if able to play
	if (!sound_Play3DTrack(psSample))
	{
		debug(LOG_NEVER, "audio_Play3DTrack: couldn't play sample\n");
		free(psSample);
		return false;
	}

	audio_AddSampleToHead(&g_psSampleList, psSample);
	return true;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
bool audio_PlayStaticTrack(SDWORD iMapX, SDWORD iMapY, int iTrack)
{
	//~~~~~~~~~~~~~~~
	SDWORD	iX, iY, iZ;
	//~~~~~~~~~~~~~~~

	// if audio not enabled return true to carry on game without audio
	if (g_bAudioEnabled == false)
	{
		return false;
	}

	audio_GetStaticPos(iMapX, iMapY, &iX, &iY, &iZ);
	return audio_Play3DTrack(iX, iY, iZ, iTrack, nullptr, nullptr);
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
bool audio_PlayObjStaticTrack(SIMPLE_OBJECT *psObj, int iTrack)
{
	//~~~~~~~~~~~~~~~
	SDWORD	iX, iY, iZ;
	//~~~~~~~~~~~~~~~

	// if audio not enabled return true to carry on game without audio
	if (g_bAudioEnabled == false)
	{
		return false;
	}

	audio_GetObjectPos(psObj, &iX, &iY, &iZ);
	return audio_Play3DTrack(iX, iY, iZ, iTrack, psObj, nullptr);
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
bool audio_PlayObjStaticTrackCallback(SIMPLE_OBJECT *psObj, int iTrack, AUDIO_CALLBACK pUserCallback)
{
	//~~~~~~~~~~~~~~~
	SDWORD	iX, iY, iZ;
	//~~~~~~~~~~~~~~~

	// if audio not enabled return true to carry on game without audio
	if (g_bAudioEnabled == false)
	{
		return false;
	}

	audio_GetObjectPos(psObj, &iX, &iY, &iZ);
	return audio_Play3DTrack(iX, iY, iZ, iTrack, psObj, pUserCallback);
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
bool audio_PlayObjDynamicTrack(SIMPLE_OBJECT *psObj, int iTrack, AUDIO_CALLBACK pUserCallback)
{
	//~~~~~~~~~~~~~~~
	SDWORD	iX, iY, iZ;
	//~~~~~~~~~~~~~~~

	// if audio not enabled return true to carry on game without audio
	if (g_bAudioEnabled == false)
	{
		return false;
	}

	audio_GetObjectPos(psObj, &iX, &iY, &iZ);
	return audio_Play3DTrack(iX, iY, iZ, iTrack, psObj, pUserCallback);
}

/** Plays the given audio file as a stream and reports back when it has finished
 *  playing.
 *  \param fileName the (OggVorbis) file to play from
 *  \param volume the volume to use while playing this file (in the range of
 *         0.0 - 1.0)
 *  \param onFinished a callback function to invoke when playing of this stream
 *         has been finished. You can use NULL to specifiy no callback function.
 *  \param user_data a pointer to contain some user data to pass along to the
 *         finished callback.
 *  \return a pointer to the currently playing stream when the stream is playing
 *          (and as such the callback will be invoked some time in the future),
 *          NULL when the stream didn't start playing (and the callback won't be
 *          invoked).
 *  \note The returned pointer will become invalid/dangling immediately after
 *        the \c onFinished callback is invoked.
 *  \note You must _never_ manually free() the memory used by the returned
 *        pointer.
 */
AUDIO_STREAM *audio_PlayStream(const char *fileName, float volume, void (*onFinished)(const void *), const void *user_data)
{
	PHYSFS_file *fileHandle;
	AUDIO_STREAM *stream;

	// If audio is not enabled return false to indicate that the given callback
	// will not be invoked.
	if (g_bAudioEnabled == false)
	{
		return nullptr;
	}

	// Open up the file
	fileHandle = PHYSFS_openRead(fileName);
	debug(LOG_WZ, "Reading...[directory: %s] %s", PHYSFS_getRealDir(fileName), fileName);
	if (fileHandle == nullptr)
	{
		debug(LOG_ERROR, "sound_LoadTrackFromFile: PHYSFS_openRead(\"%s\") failed with error: %s\n", fileName, WZ_PHYSFS_getLastError());
		return nullptr;
	}

	stream = sound_PlayStream(fileHandle, volume, onFinished, user_data);
	if (stream == nullptr)
	{
		PHYSFS_close(fileHandle);
		return nullptr;
	}

	return stream;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void audio_StopObjTrack(SIMPLE_OBJECT *psObj, int iTrack)
{
	//~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE	*psSample;
	//~~~~~~~~~~~~~~~~~~~~~~

	// return if audio not enabled
	if (g_bAudioEnabled == false)
	{
		return;
	}

	// find sample
	psSample = g_psSampleList;
	while (psSample != nullptr)
	{
		// If track has been found stop it and return
		if (psSample->psObj == psObj && psSample->iTrack == iTrack)
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
void audio_PlayTrack(int iTrack)
{
	//~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE	*psSample;
	//~~~~~~~~~~~~~~~~~~~~~~

	// return if audio not enabled
	if (g_bAudioEnabled == false || g_bAudioPaused == true)
	{
		return;
	}

	// Allocate a sample
	psSample = (AUDIO_SAMPLE *)malloc(sizeof(AUDIO_SAMPLE));
	if (psSample == nullptr)
	{
		debug(LOG_ERROR, "audio_PlayTrack: Out of memory");
		return;
	}

	// setup/initialize sample
	psSample->iTrack = iTrack;
	psSample->bFinishedPlaying = false;

	// Zero callback stuff since we don't need/want it
	psSample->pCallback = nullptr;
	psSample->psObj = nullptr;

	/* iSample, psPrev, and psNext will be initialized by the
	 * following functions, and x, y and z will be completely
	 * ignored so we don't need to bother about it
	 */

	// add sample to list if able to play
	if (!sound_Play2DTrack(psSample, false))
	{
		debug(LOG_NEVER, "audio_PlayTrack: couldn't play sample\n");
		free(psSample);
		return;
	}

	audio_AddSampleToHead(&g_psSampleList, psSample);
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void audio_PauseAll(void)
{
	// return if audio not enabled
	if (g_bAudioEnabled == false)
	{
		return;
	}

	g_bAudioPaused = true;
	sound_PauseAll();
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void audio_ResumeAll(void)
{
	// return if audio not enabled
	if (g_bAudioEnabled == false)
	{
		return;
	}

	g_bAudioPaused = false;
	sound_ResumeAll();
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void audio_StopAll(void)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE *psSample;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// return if audio not enabled
	if (g_bAudioEnabled == false)
	{
		return;
	}

	//
	// * empty list - audio_Update will free samples because callbacks have to come in first
	//
	for (psSample = g_psSampleList; psSample != nullptr; psSample = psSample->psNext)
	{
		// Stop this sound sample
		sound_StopTrack(psSample);

		// HACK:
		// Make sure to set psObj to NULL, since sometimes it becomes
		// invalidated, i.e. dangling, before audio_Update() gets the
		// chance to clean it up.
		//
		// Meaning at this place in the code audio_ObjectDead(psObj)
		// would indicate that psObj is still valid and will remain
		// such. While in reality, before audio_Update() can get the
		// chance to check again, this pointer will have become
		// dangling.
		//
		// NOTE: This would always happen when audio_StopAll() had been
		// invoked by stageThreeShutDown().
		psSample->psObj = nullptr;
	}

	// empty sample queue
	psSample = g_psSampleQueue;
	while (psSample != nullptr)
	{
		AUDIO_SAMPLE *psSampleTemp = psSample;

		// Advance the sample iterator (before invalidating it)
		psSample = psSample->psNext;

		// Destroy the sample
		free(psSampleTemp);
	}

	g_psSampleQueue = nullptr;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
SDWORD audio_GetTrackID(const char *fileName)
{
	//~~~~~~~~~~~~~
	TRACK	*psTrack;
	//~~~~~~~~~~~~~

	// return if audio not enabled
	if (g_bAudioEnabled == false)
	{
		return SAMPLE_NOT_FOUND;
	}

	if (fileName == nullptr || strlen(fileName) == 0)
	{
		debug(LOG_WARNING, "fileName is %s", (fileName == nullptr) ? "a NULL pointer" : "empty");
		return SAMPLE_NOT_FOUND;
	}

	psTrack = (TRACK *)resGetData("WAV", fileName);
	if (psTrack == nullptr)
	{
		return SAMPLE_NOT_FOUND;
	}

	return sound_GetTrackID(psTrack);
}

/** Loop through the list of playing and queued audio samples, and destroy any
 *  of them that refer to the given object.
 *  \param psObj pointer to the object for which we must destroy all of its
 *               outstanding audio samples.
 */
void audio_RemoveObj(SIMPLE_OBJECT const *psObj)
{
	unsigned int count = 0;

	// loop through queued sounds and check if a sample needs to be removed
	AUDIO_SAMPLE *psSample = g_psSampleQueue;
	while (psSample != nullptr)
	{
		if (psSample->psObj == psObj)
		{
			// The current audio sample seems to refer to an object
			// that is about to be destroyed. So destroy this
			// sample as well.
			AUDIO_SAMPLE *toRemove = psSample;

			// Make sure to keep our linked list iterator valid
			psSample = psSample->psNext;

			debug(LOG_MEMORY, "audio_RemoveObj: callback %p sample %d\n", reinterpret_cast<void *>(toRemove->pCallback), toRemove->iTrack);
			// Remove sound from global active list
			sound_RemoveActiveSample(toRemove);   //remove from global active list.

			// Perform the actual task of destroying this sample
			audio_RemoveSample(&g_psSampleQueue, toRemove);
			free(toRemove);

			// Increment the deletion count
			++count;
		}
		else
		{
			psSample = psSample->psNext;
		}
	}

	if (count)
	{
		debug(LOG_MEMORY, "audio_RemoveObj: BASE_OBJECT* 0x%p was found %u times in the audio sample queue", static_cast<const void *>(psObj), count);
	}

	// Reset the deletion count
	count = 0;

	// loop through list of currently playing sounds and check if a sample needs to be removed
	psSample = g_psSampleList;
	while (psSample != nullptr)
	{
		if (psSample->psObj == psObj)
		{
			// The current audio sample seems to refer to an object
			// that is about to be destroyed. So destroy this
			// sample as well.
			AUDIO_SAMPLE *toRemove = psSample;

			// Make sure to keep our linked list iterator valid
			psSample = psSample->psNext;

			debug(LOG_MEMORY, "audio_RemoveObj: callback %p sample %d\n", reinterpret_cast<void *>(toRemove->pCallback), toRemove->iTrack);
			// Stop this sound sample
			sound_RemoveActiveSample(toRemove);   //remove from global active list.

			// Perform the actual task of destroying this sample
			audio_RemoveSample(&g_psSampleList, toRemove);
			free(toRemove);

			// Increment the deletion count
			++count;
		}
		else
		{
			psSample = psSample->psNext;
		}
	}

	if (count)
	{
		debug(LOG_MEMORY, "audio_RemoveObj: ***Warning! psOBJ %p was found %u times in the list of playing audio samples", static_cast<const void *>(psObj), count);
	}
}
