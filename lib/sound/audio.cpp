/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
#include "lib/framework/wzconfig.h"
#include "lib/framework/object_list_iteration.h"

#include "oggopus.h"
#include "oggvorbis.h"
#include "tracklib.h"
#include "aud.h"
#include "audio.h"
#include "audio_id.h"
#include "openal_error.h"
#include "mixer.h"

#include <list>

// defines
#define NO_SAMPLE				- 2
#define MAX_SAME_SAMPLES		2

// global variables
static std::list<AUDIO_SAMPLE *> g_psSampleList;
static std::list<AUDIO_SAMPLE *> g_psSampleQueue;
static bool			g_bAudioEnabled = false;
static bool			g_bAudioPaused = false;
static AUDIO_SAMPLE g_sPreviousSample;
static int			g_iPreviousSampleTime = 0;

bool loadAudioEffectFileData(WzConfig &ini)
{
	ASSERT(ini.isAtDocumentRoot(), "WzConfig instance is in the middle of traversal");
	std::vector<WzString> list = ini.childGroups();
	for (int i = 0; i < list.size(); ++i)
	{
		ini.beginGroup(list[i]);
		nlohmann::json array = ini.json("data");
		if (array.is_null())
		{
			continue;
		}
		ASSERT(array.is_array(), "data is not an array");
		for(auto &a : array)
		{
			std::string fname = a["fileName"].get<std::string>();
			bool loop = a["loop"].get<bool>();
			unsigned int volume = a["volume"].get<uint32_t>();
			unsigned int range = a["range"].get<uint32_t>();
			audio_SetTrackVals(fname.c_str(), loop, volume, range);
		}
		ini.endGroup();
	}
	return true;
}

/** Counts the number of samples in the SampleQueue
 *  \return the number of samples in the SampleQueue
 */
unsigned int audio_GetSampleQueueCount()
{
	return static_cast<unsigned int>(g_psSampleQueue.size());
}

/** Counts the number of samples in the SampleList
 *  \return the number of samples in the SampleList
 */
unsigned int audio_GetSampleListCount()
{
	return static_cast<unsigned int>(g_psSampleList.size());
}
//*
// =======================================================================================================================
// =======================================================================================================================
//
bool audio_Disabled()
{
	return !g_bAudioEnabled;
}

bool audio_Paused()
{
	return g_bAudioPaused;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
bool audio_Init(AUDIO_CALLBACK pStopTrackCallback, HRTFMode hrtf, bool really_enable)
{
	// init audio system
	g_sPreviousSample.iTrack = NO_SAMPLE;
	g_sPreviousSample.x = SAMPLE_COORD_INVALID;
	g_sPreviousSample.y = SAMPLE_COORD_INVALID;
	g_sPreviousSample.z = SAMPLE_COORD_INVALID;
	g_bAudioEnabled = really_enable;
	if (g_bAudioEnabled)
	{
		g_bAudioEnabled = sound_Init(hrtf);
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
	bool			bOK;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// if audio not enabled return true to carry on game without audio
	if (audio_Disabled())
	{
		return true;
	}

	bOK = sound_Shutdown();

	// empty sample list
	for (AUDIO_SAMPLE* psSample : g_psSampleList)
	{
		free(psSample);
	}

	// empty sample queue
	for (AUDIO_SAMPLE* psSample : g_psSampleQueue)
	{
		free(psSample);
	}

	// free sample heap
	g_psSampleList.clear();
	g_psSampleQueue.clear();

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
static void audio_AddSampleToHead(std::list<AUDIO_SAMPLE *>& ppsSampleList, AUDIO_SAMPLE *psSample)
{
	ppsSampleList.emplace_front(psSample);
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
static void audio_AddSampleToTail(std::list<AUDIO_SAMPLE *>& ppsSampleList, AUDIO_SAMPLE *psSample)
{
	ppsSampleList.emplace_back(psSample);
}

//*
//
// audio_RemoveSample Removes sample from list but doesn't free its memory

//*
// =======================================================================================================================
// =======================================================================================================================
//
static void audio_RemoveSample(std::list<AUDIO_SAMPLE *>& ppsSampleList, AUDIO_SAMPLE *psSample)
{
	if (psSample == nullptr)
	{
		return;
	}

	auto it = std::find(ppsSampleList.begin(), ppsSampleList.end(), psSample);
	ASSERT(it != ppsSampleList.end(), "audio_RemoveSample: sample not found in list");
	ppsSampleList.erase(it);
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
static bool audio_CheckTooManySameQueueTracksPlaying(SDWORD iTrack)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	SDWORD iCount = 0;
	bool isTooMany = false;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// return if audio not enabled
	if (audio_Disabled() || audio_Paused())
	{
		return true;
	}

	// loop through queue sounds and check whether too many already in it
	for (const AUDIO_SAMPLE* psSample : g_psSampleQueue)
	{
		if (psSample->iTrack == iTrack)
		{
			++iCount;
		}

		if (iCount > MAX_SAME_SAMPLES)
		{
			isTooMany = true;
			break;
		}
	}

	return isTooMany;
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
	if (audio_Disabled() || audio_Paused())
	{
		return nullptr;
	}

	ASSERT(sound_CheckTrack(iTrack) == true, "audio_QueueSample: track %i outside limits\n", iTrack);

	// reject track if too many of same ID already in queue
	if (audio_CheckTooManySameQueueTracksPlaying(iTrack))
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
	audio_AddSampleToTail(g_psSampleQueue, psSample);

	return psSample;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void audio_QueueTrack(SDWORD iTrack)
{
	// return if audio not enabled
	if (audio_Disabled() || audio_Paused())
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
	if (audio_Disabled() || audio_Paused())
	{
		return;
	}

	// Determine if at least iMinDelay time has passed since the last time this track was played
	iDelay = sound_GetGameTime() - sound_GetTrackTimeLastFinished(iTrack);
	if (iDelay < iMinDelay)
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
	if (audio_Disabled() || audio_Paused())
	{
		return;
	}

	// Determine if at least iMinDelay time has passed since the last time this track was played
	iDelay = sound_GetGameTime() - sound_GetTrackTimeLastFinished(iTrack);
	if (iDelay < iMinDelay)
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
	if (audio_Disabled() || audio_Paused())
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
	if (audio_Disabled() || audio_Paused())
	{
		return;
	}

	if (sound_QueueSamplePlaying() == true)
	{
		return;
	}

	// check queue for members
	if (g_psSampleQueue.empty())
	{
		return;
	}

	// remove queue head
	psSample = g_psSampleQueue.front();
	audio_RemoveSample(g_psSampleQueue, psSample);

	// add sample to list if able to play
	if (!sound_Play2DTrack(psSample, true))
	{
		debug(LOG_NEVER, "audio_UpdateQueue: couldn't play sample\n");
		free(psSample);
		return;
	}

	audio_AddSampleToHead(g_psSampleList, psSample);

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
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// if audio not enabled return true to carry on game without audio
	if (audio_Disabled())
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
	mutating_list_iterate(g_psSampleList, [](AUDIO_SAMPLE* psSample)
	{
		// remove finished samples from list
		if (psSample->bFinishedPlaying == true)
		{
			audio_RemoveSample(g_psSampleList, psSample);
			free(psSample);
		}
		else // check looping sound callbacks for finished condition
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
		}
		return IterationResult::CONTINUE_ITERATION;
	});

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
	if (audio_Disabled())
	{
		return 1;
	}

	debug(LOG_NEVER, "File: %s, loop: %d, volume: %d, radius: %d", fileName, loop, volume, audibleRadius);
	return sound_SetTrackVals(fileName, loop, volume, audibleRadius);
}

//*
//
//
// * audio_CheckTooManySame3DTracksPlaying Reject samples if too many already playing in
// * same area
//

//*
// =======================================================================================================================
// =======================================================================================================================
//
static bool audio_CheckTooManySame3DTracksPlaying(SDWORD iTrack, SDWORD iX, SDWORD iY, SDWORD iZ)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	SDWORD iCount = 0, iDx = 0, iDy = 0, iDz = 0, iDistSq = 0, iMaxDistSq = 0, iRad = 0;
	bool isTooMany = false;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// return if audio not enabled
	if (audio_Disabled() || audio_Paused())
	{
		return true;
	}

	// loop through 3D sounds and check whether too many already in earshot
	for (const AUDIO_SAMPLE* psSample : g_psSampleList)
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
				++iCount;
			}

			if (iCount > MAX_SAME_SAMPLES)
			{
				isTooMany = true;
				break;
			}
		}
	}

	return isTooMany;
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
	if (audio_Disabled() || audio_Paused())
	{
		return false;
	}

	if (audio_CheckTooManySame3DTracksPlaying(iTrack, iX, iY, iZ))
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
	gain = (1.0f - (distance * ATTENUATION_FACTOR)) ;//* 1.0f * sfx3d_volume
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

	audio_AddSampleToHead(g_psSampleList, psSample);
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
	if (audio_Disabled() || audio_Paused())
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
	if (audio_Disabled() || audio_Paused())
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
	if (audio_Disabled() || audio_Paused())
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
	if (audio_Disabled() || audio_Paused())
	{
		return false;
	}

	audio_GetObjectPos(psObj, &iX, &iY, &iZ);
	return audio_Play3DTrack(iX, iY, iZ, iTrack, psObj, pUserCallback);
}

/** Plays the given audio file as a stream and reports back when it has finished
 *  playing.
 *  \param fileName the (OggVorbis|OggOpus) file to play from
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
AUDIO_STREAM *audio_PlayStream(const char *fileName, float volume, const std::function<void (const AUDIO_STREAM *, const void *)>& onFinished, const void *user_data)
{
	// If audio is not enabled return false to indicate that the given callback
	// will not be invoked.
	if (audio_Disabled())
	{
		return nullptr;
	}
	AUDIO_STREAM *stream = sound_PlayStream(fileName, volume, onFinished, user_data);
	return stream;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void audio_StopObjTrack(SIMPLE_OBJECT *psObj, int iTrack)
{
	// return if audio not enabled
	if (audio_Disabled())
	{
		return;
	}

	// find sample
	for (AUDIO_SAMPLE* psSample : g_psSampleList)
	{
		// If track has been found stop it and return
		if (psSample->psObj == psObj && psSample->iTrack == iTrack)
		{
			sound_StopTrack(psSample);
			return;
		}
	}
}
static UDWORD lastTimeBuildFailedPlayed = 0;
#define REPEAT_AUDIO_CUE_THRESHOLD 100
// Play annoying sound only once per tick
void audio_PlayBuildFailedOnce()
{
	if (realTime - lastTimeBuildFailedPlayed >= REPEAT_AUDIO_CUE_THRESHOLD)
	{
		audio_PlayTrack(ID_SOUND_BUILD_FAIL);
		lastTimeBuildFailedPlayed = realTime;
	}
}

//*
//
// audio_PlayTrack Play immediate 2D FX track

//*
// =======================================================================================================================
// =======================================================================================================================
//
void audio_PlayTrack(int iTrack, const bool playIfPaused/*=true*/)
{
	//~~~~~~~~~~~~~~~~~~~~~~
	AUDIO_SAMPLE	*psSample;
	//~~~~~~~~~~~~~~~~~~~~~~

	// return if audio not enabled
	if (audio_Disabled() || (audio_Paused() && !playIfPaused))
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

	audio_AddSampleToHead(g_psSampleList, psSample);
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void audio_setPause(const bool state)
{
	// return if audio not enabled
	if (audio_Disabled())
	{
		return;
	}

	g_bAudioPaused = state;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void audio_StopAll(void)
{
	// return if audio not enabled
	if (audio_Disabled())
	{
		return;
	}

	//
	// * empty list - audio_Update will free samples because callbacks have to come in first
	//
	for (AUDIO_SAMPLE* psSample : g_psSampleList)
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
	for (AUDIO_SAMPLE* psSample : g_psSampleQueue)
	{
		// Destroy the sample
		free(psSample);
	}
	g_psSampleQueue.clear();
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
	if (audio_Disabled())
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
	mutating_list_iterate(g_psSampleQueue, [psObj, &count](AUDIO_SAMPLE* psSample)
	{
		if (psSample->psObj != psObj)
		{
			return IterationResult::CONTINUE_ITERATION;
		}
		// The current audio sample seems to refer to an object
		// that is about to be destroyed. So destroy this
		// sample as well.
		AUDIO_SAMPLE* toRemove = psSample;

		debug(LOG_MEMORY, "audio_RemoveObj: callback %p sample %d\n", reinterpret_cast<void*>(toRemove->pCallback), toRemove->iTrack);
		// Remove sound from global active list
		sound_RemoveActiveSample(toRemove);   //remove from global active list.

		// Perform the actual task of destroying this sample
		audio_RemoveSample(g_psSampleQueue, toRemove);
		free(toRemove);

		// Increment the deletion count
		++count;

		return IterationResult::CONTINUE_ITERATION;
	});

	if (count)
	{
		debug(LOG_MEMORY, "audio_RemoveObj: BASE_OBJECT* 0x%p was found %u times in the audio sample queue", static_cast<const void *>(psObj), count);
	}

	// Reset the deletion count
	count = 0;

	// loop through list of currently playing sounds and check if a sample needs to be removed
	mutating_list_iterate(g_psSampleList, [psObj, &count](AUDIO_SAMPLE* psSample)
	{
		if (psSample->psObj != psObj)
		{
			return IterationResult::CONTINUE_ITERATION;
		}
		// The current audio sample seems to refer to an object
		// that is about to be destroyed. So destroy this
		// sample as well.
		AUDIO_SAMPLE* toRemove = psSample;

		debug(LOG_MEMORY, "audio_RemoveObj: callback %p sample %d\n", reinterpret_cast<void*>(toRemove->pCallback), toRemove->iTrack);
		// Stop this sound sample
		sound_RemoveActiveSample(toRemove);   //remove from global active list.

		// Perform the actual task of destroying this sample
		audio_RemoveSample(g_psSampleList, toRemove);
		free(toRemove);

		// Increment the deletion count
		++count;

		return IterationResult::CONTINUE_ITERATION;
	});

	if (count)
	{
		debug(LOG_MEMORY, "audio_RemoveObj: ***Warning! psOBJ %p was found %u times in the list of playing audio samples", static_cast<const void *>(psObj), count);
	}
}
