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
/** \file
 *  Sound library-specific functions
 */

// this has to be first
#include "lib/framework/frame.h"
#include "lib/framework/math_ext.h"
#include "lib/framework/frameresource.h"
#include "lib/exceptionhandler/dumpinfo.h"

#include <AL/al.h>
#include <AL/alc.h>
#if defined(HAVE_OPENAL_ALEXT_H)
# include <AL/alext.h>
#endif

#include <physfs.h>
#include "lib/framework/physfs_ext.h"
#include <string.h>
#include <math.h>
#include <limits>

#include "tracklib.h"
#include "audio.h"
#include "cdaudio.h"
#include "oggvorbis.h"
#include "openal_error.h"
#include "mixer.h"
#include "openal_info.h"
#include "oggopus.h"
#include "oggvorbis.h"
static ALuint current_queue_sample = -1;

static bool openal_initialized = false;
static const size_t bufferSize = 16 * 1024;
static const unsigned int buffer_count = 32;

/** Source of music */
struct AUDIO_STREAM
{
	ALuint                  source = -1;        // OpenAL name of the sound source
	WZDecoder       		*decoder = nullptr;
	PHYSFS_file				*fileHandle = nullptr;
	float                   volume = 0.f;
	bool					queuedStop = false;	// when sound_StopStream has been called on the stream

	// Callbacks
	std::function<void (const AUDIO_STREAM *, const void *)> onFinished;
	const void              *user_data = nullptr;

	// Linked list pointer
	AUDIO_STREAM           *next = nullptr;
};

struct SAMPLE_LIST
{
	AUDIO_SAMPLE   *curr;
	SAMPLE_LIST    *next;
};

static SAMPLE_LIST *active_samples = nullptr;

/* actives openAL-Sources */
static AUDIO_STREAM *active_streams = nullptr;

static ALfloat		sfx_volume = 1.0;
static ALfloat		sfx3d_volume = 1.0;

static ALCdevice *device = nullptr;
static ALCcontext *context = nullptr;

#if defined(ALC_SOFT_HRTF)
static LPALCGETSTRINGISOFT alcGetStringiSOFT = nullptr;
static LPALCRESETDEVICESOFT alcResetDeviceSOFT = nullptr;
#endif


/** Removes the given sample from the "active_samples" linked list
 *  \param previous either NULL (if \c to_remove is the first item in the
 *                  list) or the item occurring just before \c to_remove in
 *                  the list
 *  \param to_remove the item to actually remove from the list
 */
static void sound_RemoveSample(SAMPLE_LIST *previous, SAMPLE_LIST *to_remove)
{
	if (previous != nullptr && previous != to_remove)
	{
		// Verify that the given two samples actually follow eachother in the list
		ASSERT(previous->next == to_remove, "Sound samples don't follow eachother in the list, we're probably removing the wrong item.");

		// Remove the item to remove from the linked list by skipping
		// it in the pointer sequence.
		previous->next = to_remove->next;
	}
	else
	{
		// Apparently we're removing the first item from the list. So
		// make the next one the list's head.
		active_samples = to_remove->next;
	}
}

ALCint HRTFModeToALCint(HRTFMode mode)
{
#if defined(ALC_SOFT_HRTF)
	switch (mode)
	{
	case HRTFMode::Unsupported:
		// should never be called with unsupported, log it and fall through to disabled
		debug(LOG_ERROR, "HRTFModeToALCint called with HRTFMode::Unsupported");
		// fallthrough
	case HRTFMode::Disabled:
		return ALC_FALSE;
	case HRTFMode::Enabled:
		return ALC_TRUE;
	case HRTFMode::Auto:
		return ALC_DONT_CARE_SOFT;
	}
#endif
	return ALC_FALSE;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
bool sound_InitLibrary(HRTFMode hrtf)
{
	int err;
	const ALfloat listenerVel[3] = { 0.0, 0.0, 0.0 };
	const ALfloat listenerOri[6] = { 0.0, 0.0, 1.0, 0.0, 1.0, 0.0 };
	char buf[512];
	const ALCchar *deviceName;

#if 0
	// This code is disabled because enumerating devices apparently crashes PulseAudio on Fedora12

	/* Get the available devices and print them.
	 * Devices are separated by NUL chars ('\0') and the list of devices is
	 * terminated by two NUL chars.
	 */
	deviceName = alcGetString(NULL, ALC_DEVICE_SPECIFIER);
	while (deviceName != NULL && *deviceName != '\0')
	{
		debug(LOG_SOUND, "available OpenAL device(s) are: %s", deviceName);
		deviceName += strlen(deviceName) + 1;
	}
#endif

	if (enabled_debug[LOG_SOUND])
	{
		OpenALInfo::Output_PlaybackDevices([](const std::string &output) {
			addDumpInfo(output.c_str());
			if (enabled_debug[LOG_SOUND])
			{
				_debug_multiline(0, LOG_SOUND, "sound", output);
			}
		});
	}

	// Open default device
	device = alcOpenDevice(nullptr);

	if (!device)
	{
		debug(LOG_ERROR, "Couldn't open audio device.");
		return false;
	}

	if (enabled_debug[LOG_SOUND])
	{
		OpenALInfo::Output_ALCInfo(device, [](const std::string &output) {
			// addDumpInfo(output.c_str());
			//if (enabled_debug[LOG_SOUND])
			//{
				_debug_multiline(0, LOG_SOUND, "sound", output);
			//}
		});
	}

#if defined(ALC_SOFT_HRTF)
	// Load some extensions from OpenAL-Soft (if available)
	alcGetStringiSOFT = (LPALCGETSTRINGISOFT)alcGetProcAddress(device, "alcGetStringiSOFT");
	alcResetDeviceSOFT = (LPALCRESETDEVICESOFT)alcGetProcAddress(device, "alcResetDeviceSOFT");
#endif


	// Print current device name and add it to dump info
	deviceName = alcGetString(device, ALC_DEVICE_SPECIFIER);
	debug(LOG_SOUND, "Current audio device: %s", deviceName);
	ssprintf(buf, "OpenAL Device Name: %s", deviceName);
	addDumpInfo(buf);

	context = alcCreateContext(device, nullptr);		//NULL was contextAttributes
	if (!context)
	{
		debug(LOG_ERROR, "Couldn't open audio context.");
		return false;
	}

	alcMakeContextCurrent(context);

	err = sound_GetContextError(device);
	if (err != ALC_NO_ERROR)
	{
		debug(LOG_ERROR, "Couldn't initialize audio context: %s", alcGetString(device, err));
		return false;
	}

	// Dump Open AL device info (depends on context)
	// to the crash handler for the dump file and debug log
	ssprintf(buf, "OpenAL Vendor: %s", alGetString(AL_VENDOR));
	addDumpInfo(buf);
	debug(LOG_SOUND, "%s", buf);

	ssprintf(buf, "OpenAL Version: %s", alGetString(AL_VERSION));
	addDumpInfo(buf);
	debug(LOG_SOUND, "%s", buf);

	ssprintf(buf, "OpenAL Renderer: %s", alGetString(AL_RENDERER));
	addDumpInfo(buf);
	debug(LOG_SOUND, "%s", buf);

	ssprintf(buf, "OpenAL Extensions: %s", alGetString(AL_EXTENSIONS));
	addDumpInfo(buf);
	debug(LOG_SOUND, "%s", buf);

	openal_initialized = true;

#if defined(ALC_SOFT_HRTF)
	if(alcIsExtensionPresent(device, "ALC_SOFT_HRTF"))
	{
		// Set desired HRTF mode
		sound_SetHRTFMode(hrtf);

		// Get current HRTF status
		ALCint hrtfStatus;
		alcGetIntegerv(device, ALC_HRTF_STATUS_SOFT, 1, &hrtfStatus);

		const char *hrtfStatusString = nullptr;
		switch (hrtfStatus)
		{
		case ALC_HRTF_DISABLED_SOFT:
			hrtfStatusString = "ALC_HRTF_DISABLED_SOFT: HRTF is disabled";
			break;
		case ALC_HRTF_ENABLED_SOFT:
			hrtfStatusString = "ALC_HRTF_ENABLED_SOFT: HRTF is enabled";
			break;
		case ALC_HRTF_DENIED_SOFT:
			// This may be caused by invalid resource permissions, or other user configuration that disallows HRTF.
			hrtfStatusString = "ALC_HRTF_DENIED_SOFT: HRTF is disabled because it's not allowed on the device.";
			break;
		case ALC_HRTF_REQUIRED_SOFT:
			// This may be caused by a device that can only use HRTF, or other user configuration that forces HRTF to be used.
			hrtfStatusString = "ALC_HRTF_REQUIRED_SOFT: HRTF is enabled because it must be used on the device.";
			break;
		case ALC_HRTF_HEADPHONES_DETECTED_SOFT:
			hrtfStatusString = "ALC_HRTF_HEADPHONES_DETECTED_SOFT: HRTF is enabled automatically because the device reported itself as headphones.";
			break;
		case ALC_HRTF_UNSUPPORTED_FORMAT_SOFT:
			// HRTF is disabled because the device does not support it with the current format.
			// Typically this is caused by non-stereo output or an incompatible output frequency.
			hrtfStatusString = "ALC_HRTF_UNSUPPORTED_FORMAT_SOFT: HRTF is disabled because the device does not support it with the current format.";
			break;
		default:
			hrtfStatusString = nullptr;
			break;
		}

		if (hrtfStatusString)
		{
			debug(LOG_SOUND, "%s", hrtfStatusString);
		}
		else
		{
			debug(LOG_SOUND, "OpenAL-Soft returned an unknown ALC_HRTF_STATUS_SOFT result: %d", hrtfStatus);
		}
	}
	else
	{
		debug(LOG_SOUND, "alcIsExtensionPresent(..., \"ALC_SOFT_HRTF\") returned false");
	}
#else
	debug(LOG_SOUND, "ALC_SOFT_HRTF not defined");
#endif

	// Clear Error Codes
	alGetError();
	alcGetError(device);

	alListener3f(AL_POSITION, 0.f, 0.f, 0.f);
	alListenerfv(AL_VELOCITY, listenerVel);
	alListenerfv(AL_ORIENTATION, listenerOri);
	alDistanceModel(AL_NONE);
	sound_GetError();

	return true;
}

HRTFMode sound_GetHRTFMode()
{
#if defined(ALC_SOFT_HRTF)
	if(alcIsExtensionPresent(device, "ALC_SOFT_HRTF"))
	{
		ALCint hrtfStatus;
		alcGetIntegerv(device, ALC_HRTF_SOFT, 1, &hrtfStatus);
		switch (hrtfStatus)
		{
		case ALC_TRUE:
			return HRTFMode::Enabled;
		case ALC_FALSE:
			return HRTFMode::Disabled;
		default:
			debug(LOG_SOUND, "OpenAL-Soft returned an unexpected ALC_HRTF_SOFT result: %d", hrtfStatus);
		}
	}
#endif
	return HRTFMode::Unsupported;
}

bool sound_SetHRTFMode(HRTFMode mode)
{
#if defined(ALC_SOFT_HRTF)
	if(alcIsExtensionPresent(device, "ALC_SOFT_HRTF"))
	{
		ALCint hrtfSetting = HRTFModeToALCint(mode);

		ALCint attrs[] = {
			ALC_HRTF_SOFT, hrtfSetting, /* configure HRTF */
			0 /* end of list */
		};
		if (alcResetDeviceSOFT)
		{
			ASSERT(device, "device is null");
			if (!alcResetDeviceSOFT(device, attrs))
			{
				debug(LOG_ERROR, "Failed to reset device: %s\n", alcGetString(device, alcGetError(device)));
				return false;
			}
			return true;
		}
		else
		{
			debug(LOG_ERROR, "ALC_SOFT_HRTF extension is available, but alcResetDeviceSOFT is null");
		}
	}
#endif
	return false;
}

static void sound_UpdateStreams(void);

void sound_ShutdownLibrary(void)
{
	AUDIO_STREAM *stream;
	SAMPLE_LIST *aSample = active_samples, * tmpSample = nullptr;

	if (!openal_initialized)
	{
		return;
	}
	debug(LOG_SOUND, "starting shutdown");

	// Stop all streams, sound_UpdateStreams() will deallocate all stopped streams
	for (stream = active_streams; stream != nullptr; stream = stream->next)
	{
		sound_StopStream(stream);
	}
	sound_UpdateStreams();

	alcGetError(device);	// clear error codes

	/* On Linux since this caused some versions of OpenAL to hang on exit. - Per */
	debug(LOG_SOUND, "make default context NULL");
	alcMakeContextCurrent(nullptr);
	sound_GetContextError(device);

	debug(LOG_SOUND, "destroy previous context");
	alcDestroyContext(context); // this gives a long delay on some impl.
	sound_GetContextError(device);

	debug(LOG_SOUND, "close device");
	if (alcCloseDevice(device) == ALC_FALSE)
	{
		debug(LOG_SOUND, "OpenAl could not close the audio device.");
	}
	device = nullptr;

	while (aSample)
	{
		tmpSample = aSample->next;
		free(aSample);
		aSample = tmpSample;
	}
	active_samples = nullptr;
}

/** Deletes the given sample and updates the \c previous and \c current iterators
 *  \param previous iterator to the previous sample in the list
 *  \param sample iterator to the current sample in the list which you want to delete
 */
static void sound_DestroyIteratedSample(SAMPLE_LIST **previous, SAMPLE_LIST **sample)
{
	// If an OpenAL source is associated with this sample, release it
	if ((*sample)->curr->iSample != (ALuint)AL_INVALID)
	{
		alDeleteSources(1, &(*sample)->curr->iSample);
		sound_GetError();
	}

	// Do the cleanup of this sample
	sound_FinishedCallback((*sample)->curr);

	// Remove the sample from the list
	sound_RemoveSample(*previous, *sample);
	// Free it
	free(*sample);

	// Get a pointer to the next node, the previous pointer doesn't change
	*sample = (*previous != nullptr) ? (*previous)->next : active_samples;
}

/** Counts the number of samples in active_samples
 *  \return the number of actively playing sound samples
 */
unsigned int sound_GetActiveSamplesCount()
{
	unsigned int  num = 0;
	SAMPLE_LIST *node = active_samples;

	while (node)
	{
		num++;
		node = node->next;
	}
	return num;
}

/* gets called in audio.cpp: audio_update(), which gets called in renderLoop() */
void sound_Update()
{
	SAMPLE_LIST *node = active_samples;
	SAMPLE_LIST *previous = nullptr;
	ALfloat gain;

	if (!openal_initialized)
	{
		return;
	}

	// Update all streaming audio
	sound_UpdateStreams();

	while (node != nullptr)
	{
		ALenum state, err;

		// query what the gain is for this sample
		alGetSourcef(node->curr->iSample, AL_GAIN, &gain);
		err = sound_GetError();

		// if gain is 0, then we can't hear it, so we kill it.
		if (gain == 0.0f)
		{
			sound_DestroyIteratedSample(&previous, &node);
			continue;
		}

		//ASSERT(alIsSource(node->curr->iSample), "Not a valid source!");
		alGetSourcei(node->curr->iSample, AL_SOURCE_STATE, &state);

		// Check whether an error occurred while retrieving the state.
		// If one did, the state returned is useless. So instead of
		// using it continue with the next sample.
		err = sound_GetError();
		if (err != AL_NO_ERROR)
		{
			// Make sure to invoke the "finished" callback
			sound_FinishedCallback(node->curr);

			// Destroy this object and move to the next object
			sound_DestroyIteratedSample(&previous, &node);
			continue;
		}

		switch (state)
		{
		case AL_PLAYING:
		case AL_PAUSED:
			// If we haven't finished playing yet, just
			// continue with the next item in the list.

			// sound_SetObjectPosition(i->curr->iSample, i->curr->x, i->curr->y, i->curr->z);

			// Move to the next object
			previous = node;
			node = node->next;
			break;

		// NOTE: if it isn't playing | paused, then it is most likely either done
		// or a error.  In either case, we want to kill the sample in question.

		default:
			sound_DestroyIteratedSample(&previous, &node);
			break;
		}
	}

	// Reset the current error state
	alcGetError(device);

	alcProcessContext(context);

	ALCenum err = sound_GetContextError(device);
	if (err != ALC_NO_ERROR)
	{
		debug(LOG_ERROR, "Error while processing audio context: %s", alGetString(err));
	}
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
bool sound_QueueSamplePlaying(void)
{
	ALenum	state;

	if (!openal_initialized)
	{
		return false;
	}
	if (current_queue_sample == (ALuint)AL_INVALID)
	{
		return false;
	}

	alGetSourcei(current_queue_sample, AL_SOURCE_STATE, &state);

	// Check whether an error occurred while retrieving the state.
	// If one did, the state returned is useless. So instead of
	// using it return false.
	if (sound_GetError() != AL_NO_ERROR)
	{
		return false;
	}

	if (state == AL_PLAYING)
	{
		return true;
	}

	if (current_queue_sample != (ALuint)AL_INVALID)
	{
		SAMPLE_LIST *node = active_samples;
		SAMPLE_LIST *previous = nullptr;

		// We need to remove it from the queue of actively played samples
		while (node != nullptr)
		{
			if (node->curr->iSample == current_queue_sample)
			{
				sound_DestroyIteratedSample(&previous, &node);
				current_queue_sample = AL_INVALID;
				return false;
			}
			previous = node;
			if (node)
			{
				node = node->next;
			}
		}
		debug(LOG_ERROR, "Sample %u not deleted because it wasn't in the active queue!", current_queue_sample);
		current_queue_sample = AL_INVALID;
	}
	return false;
}

/** Decodes *entirely* an opened OggVorbis file into an OpenAL buffer.
 *  This is used to play sound effects, not "music". Assumes .ogg file.
 *  
 *  \param psTrack pointer to object which will contain the final buffer
 *  \param PHYSFS_fileHandle file handle given by PhysicsFS to the opened file
 *  \return true on success
 */
static inline bool sound_DecodeOggVorbisTrack(TRACK *psTrack, const char* fileName)
{
	if (!openal_initialized)
	{
		return false;
	}

	WZVorbisDecoder* decoder = WZVorbisDecoder::fromFilename(fileName);
	if (!decoder)
	{
		debug(LOG_ERROR, "couldn't allocate decoder for %s", fileName);
		return false;
	}
	const unsigned estimate = decoder->totalSamples() * decoder->channels() * 2;
	uint8_t* buffer = (uint8_t*) malloc(estimate);
	if (buffer == nullptr)
	{
		debug(LOG_ERROR, "couldn't allocate temp buffer to load track %s", fileName);
		delete decoder;
		return false;
	}
	memset(buffer, 0, estimate);
	auto res = decoder->decode(buffer, estimate);
	if (!res.has_value())
	{
		debug(LOG_ERROR, "failed decoding %s", fileName);
		free(buffer);
		delete decoder;
		return false;
	}

	// Determine PCM data format
	ALenum format = (decoder->channels() == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	ALuint alBuffer;
	// Create an OpenAL buffer and fill it with the decoded data
	alGenBuffers(1, &alBuffer);
	sound_GetError();
	ASSERT(estimate <= static_cast<size_t>(std::numeric_limits<ALsizei>::max()), "soundBuffer->size (%u) exceeds ALsizei::max", estimate);
	ASSERT(decoder->frequency() <= static_cast<size_t>(std::numeric_limits<ALsizei>::max()), "decoder->frequency() (%zu) exceeds ALsizei::max ??", decoder->frequency());
	alBufferData(alBuffer, format, buffer, static_cast<ALsizei>(estimate), static_cast<ALsizei>(decoder->frequency()));
	sound_GetError();

	// save buffer name in track
	psTrack->iBufferName = alBuffer;
	free(buffer);
	delete decoder;
	return true;
}

/** This is used to play sound effets (not "music"). Assumes .ogg file.
 * \param [in] fileName: <soundeffect>.ogg
 * \returns Track pointer, or nullptr on failure
*/
TRACK *sound_LoadTrackFromFile(const char *fileName)
{
	TRACK *pTrack;
	size_t filename_size;
	char *track_name;

	if (GetLastResourceFilename() == nullptr)
	{
		// This is a non fatal error.  We just can't find filename for some reason.
		debug(LOG_WARNING, "sound_LoadTrackFromFile: missing resource filename?");
		filename_size = 0;
	}
	else
	{
		filename_size = strlen(GetLastResourceFilename()) + 1;
	}

	// allocate track, plus the memory required to contain the filename
	// one malloc call ensures only one free call is required
	pTrack = (TRACK *)malloc(sizeof(TRACK) + filename_size);
	if (pTrack == nullptr)
	{
		debug(LOG_FATAL, "sound_ConstructTrack: couldn't allocate memory\n");
		abort();
		return nullptr;
	}

	// Initialize everything (except for the filename) to zero
	memset(pTrack, 0, sizeof(TRACK));

	// Set filename pointer; if the filename (as returned by
	// GetLastResourceFilename()) is a NULL pointer, then this will be a
	// NULL pointer as well.
	track_name = filename_size ? (char *)(pTrack + 1) : nullptr;

	// Copy the filename into the struct, if we don't have a NULL pointer
	if (filename_size != 0)
	{
		strcpy(track_name, GetLastResourceFilename());
	}
	pTrack->fileName = track_name;

	// Now use sound_ReadTrackFromBuffer to decode the file's contents
	if (!sound_DecodeOggVorbisTrack(pTrack, fileName))
	{
		free(pTrack);
		return nullptr;
	}

	return pTrack;
}

void sound_FreeTrack(TRACK *psTrack)
{
	alDeleteBuffers(1, &psTrack->iBufferName);
	sound_GetError();
}

static void sound_AddActiveSample(AUDIO_SAMPLE *psSample)
{
	SAMPLE_LIST *tmp = (SAMPLE_LIST *) malloc(sizeof(SAMPLE_LIST));

	// Prepend the given sample to our list of active samples
	tmp->curr = psSample;
	tmp->next = active_samples;
	active_samples = tmp;
}

/** Routine gets rid of the psObj's sound sample and reference in active_samples.
 */
void sound_RemoveActiveSample(AUDIO_SAMPLE *psSample)
{
	SAMPLE_LIST *node = active_samples;
	SAMPLE_LIST *previous = nullptr;

	while (node != nullptr)
	{
		if (node->curr->psObj == psSample->psObj)
		{
			debug(LOG_MEMORY, "Removing object 0x%p from active_samples list 0x%p\n", static_cast<void *>(psSample->psObj), static_cast<void *>(node));

			// Buginator: should we wait for it to finish, or just stop it?
			sound_StopSample(node->curr);

			sound_FinishedCallback(node->curr);	//tell the callback it is finished.

			sound_DestroyIteratedSample(&previous, &node);
		}
		else
		{
			// Move to the next sample object
			previous = node;
			node = node->next;
		}
	}
}

static bool sound_SetupChannel(AUDIO_SAMPLE *psSample)
{
	sound_AddActiveSample(psSample);

	return sound_TrackLooped(psSample->iTrack);
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
bool sound_Play2DSample(TRACK *psTrack, AUDIO_SAMPLE *psSample, bool bQueued)
{
	ALfloat zero[3] = { 0.0, 0.0, 0.0 };
	ALfloat volume;
	ALint error;

	if (sfx_volume == 0.0)
	{
		return false;
	}
	volume = ((float)psTrack->iVol / 100.0f);		// each object can have OWN volume!
	psSample->fVol = volume;						// save computed volume
	volume *= sfx_volume;							// and now take into account the Users sound Prefs.

	// We can't hear it, so don't bother creating it.
	if (volume == 0.0f)
	{
		return false;
	}

	// Clear error codes
	alGetError();

	alGenSources(1, &(psSample->iSample));

	error = sound_GetError();
	if (error != AL_NO_ERROR)
	{
		/* FIXME: We run out of OpenAL sources very quickly, so we
		 * should handle the case where we've ran out of them.
		 * Currently we don't do this, causing some unpleasant side
		 * effects, e.g. crashing...
		 */
	}

	alSourcef(psSample->iSample, AL_PITCH, 1.0f);
	alSourcef(psSample->iSample, AL_GAIN, volume);
	alSourcefv(psSample->iSample, AL_POSITION, zero);
	alSourcefv(psSample->iSample, AL_VELOCITY, zero);
	alSourcei(psSample->iSample, AL_BUFFER, psTrack->iBufferName);
	alSourcei(psSample->iSample, AL_SOURCE_RELATIVE, AL_TRUE);
	alSourcei(psSample->iSample, AL_LOOPING, (sound_SetupChannel(psSample)) ? AL_TRUE : AL_FALSE);

	// NOTE: this is only useful for debugging.
#ifdef DEBUG
	psSample->is3d = false;
	psSample->isLooping = sound_TrackLooped(psSample->iTrack) ? AL_TRUE : AL_FALSE;
	memcpy(psSample->filename, psTrack->fileName, strlen(psTrack->fileName));
	psSample->filename[strlen(psTrack->fileName)] = '\0';
#endif
	// Clear error codes
	alGetError();

	alSourcePlay(psSample->iSample);
	sound_GetError();

	if (bQueued)
	{
		current_queue_sample = psSample->iSample;
	}
	else if (current_queue_sample == psSample->iSample)
	{
		current_queue_sample = -1;
	}

	return true;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
bool sound_Play3DSample(TRACK *psTrack, AUDIO_SAMPLE *psSample)
{
	ALfloat zero[3] = { 0.0, 0.0, 0.0 };
	ALfloat volume;
	ALint error;

	if (sfx3d_volume == 0.0)
	{
		return false;
	}

	volume = ((float)psTrack->iVol / 100.f);		// max range is 0-100
	psSample->fVol = volume;						// store results for later

	// If we can't hear it, then don't bother playing it.
	if (volume == 0.0f)
	{
		return false;
	}
	// Clear error codes
	alGetError();

	alGenSources(1, &(psSample->iSample));

	error = sound_GetError();
	if (error != AL_NO_ERROR)
	{
		/* FIXME: We run out of OpenAL sources very quickly, so we
		 * should handle the case where we've ran out of them.
		 * Currently we don't do this, causing some unpleasant side
		 * effects, e.g. crashing...
		 */
	}

#if defined(WZ_OS_UNIX) && !defined(WZ_OS_MAC)
	// HACK: this is a workaround for a bug in the 64bit implementation of OpenAL on GNU/Linux
	// The AL_PITCH value really should be 1.0.
	alSourcef(psSample->iSample, AL_PITCH, 1.001f);
#else
	alSourcef(psSample->iSample, AL_PITCH, 1.0f);
#endif

	sound_SetObjectPosition(psSample);
	alSourcefv(psSample->iSample, AL_VELOCITY, zero);
	alSourcei(psSample->iSample, AL_BUFFER, psTrack->iBufferName);
	alSourcei(psSample->iSample, AL_LOOPING, (sound_SetupChannel(psSample)) ? AL_TRUE : AL_FALSE);

	// NOTE: this is only useful for debugging.
#ifdef DEBUG
	psSample->is3d = true;
	psSample->isLooping = sound_TrackLooped(psSample->iTrack) ? AL_TRUE : AL_FALSE;
	memcpy(psSample->filename, psTrack->fileName, strlen(psTrack->fileName));
	psSample->filename[strlen(psTrack->fileName)] = '\0';
#endif

	// Clear error codes
	alGetError();

	alSourcePlay(psSample->iSample);
	sound_GetError();

	return true;
}

/** Fills N buffers, each of buffSize
 *  Only frees whatever was allocated by itself.
 * \returns nb of buffers copied (but not enqueued yet) to openAL
*/
static int sound_fillNBuffers(ALuint* alBuffersIds, WZDecoder* decoder, size_t n, size_t buffSize)
{
	ASSERT_OR_RETURN(-1, n <= static_cast<size_t>(std::numeric_limits<int>::max()), "number of buffers (%zu) exceeds int::max", n);
	static uint8_t *pcm = (uint8_t*) malloc(buffSize);
	if (!pcm)
	{
		debug(LOG_ERROR, "can't allocate buff of size %zu", buffSize);
		return -1;
	}
	// Determine PCM data format
	ALenum format = (decoder->channels() == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
	int i = 0;
	const int i_max = static_cast<int>(n);
	for (; i < i_max; ++i)
	{
		memset(pcm, 0, buffSize);
		// Decode some audio data
		auto res = decoder->decode(pcm, buffSize);

		if (!res.has_value())
		{
			//free(pcm);
			sound_GetError();
			return -1;
		}

		// If we actually decoded some data
		if (res.value() > 0)
		{
			ASSERT(res.value() <= static_cast<size_t>(std::numeric_limits<ALint>::max()), "read size (%zu) exceeds ALint::max", res.value());
			alBufferData(alBuffersIds[i], format, pcm, static_cast<ALint>(res.value()), static_cast<ALsizei>(decoder->frequency()));
			sound_GetError();
		}
		else // if (res.value() == 0)
		{
			// If no data has been decoded we're probably at the end of our
			// stream. So cleanup the excess stuff here.
			// First remove the data buffer itself
			//free(pcm);
			sound_GetError();
			break;
		}
	}
	// all good: return how many buffers were actually (at least partially) filled
	return i;
}


/** Plays the audio data from the given file
 *  \param volume the volume to play the audio at (in a range of 0.0 to 1.0)
 *  \param onFinished callback to invoke when we're finished playing
 *  \param user_data user-data pointer to pass to the \c onFinished callback
 *  \return a pointer to the currently playing stream when playing started
 *          successfully, NULL otherwise.
 *  \note The returned pointer will become invalid/dangling immediately after
 *        the \c onFinished callback is invoked.
 *  \note You must _never_ manually free() the memory used by the returned
 *        pointer.
 */
AUDIO_STREAM *sound_PlayStream(const char* fileName, 
																			float volume, 
																			const std::function<void (const AUDIO_STREAM *, const void *)>& onFinished,
																			const void *user_data)
{
	if (!openal_initialized)
	{
		debug(LOG_WARNING, "OpenAL isn't initialized, not creating an audio stream");
		return nullptr;
	}
		// Clean errors
	alGetError();
	WZDecoder *decoder = nullptr;
	const size_t len = strlen(fileName);
	if (len > 4 && (strncasecmp(fileName + len - 4, ".ogg", 4) == 0))
	{
		decoder = WZVorbisDecoder::fromFilename(fileName);
	}
	else if (len > 5 && (strncasecmp(fileName + len - 5, ".opus", 5) == 0))
	{
		decoder = WZOpusDecoder::fromFilename(fileName);
	}
	if (!decoder)
	{
		debug(LOG_ERROR, "couldn't allocate decoder for %s", fileName);
		return nullptr;
	}

	AUDIO_STREAM *stream = new AUDIO_STREAM();
	if (stream == nullptr)
	{
		debug(LOG_FATAL, "sound_PlayStream: Out of memory");
		abort();
		return nullptr;
	}
	stream->decoder = decoder;
	// Retrieve an OpenAL sound source
	alGenSources(1, &(stream->source));
	if (sound_GetError() != AL_NO_ERROR)
	{
		// Failed to create OpenAL sound source, so bail out...
		debug(LOG_SOUND, "alGenSources failed, most likely out of sound sources");
		delete stream;
		delete decoder;
		return nullptr;
	}

	stream->volume = volume;
	alSourcef(stream->source, AL_GAIN, stream->volume);

#if defined(WZ_OS_UNIX) && !defined(WZ_OS_MAC)
	// HACK: this is a workaround for a bug in the 64bit implementation of OpenAL on GNU/Linux
	// The AL_PITCH value really should be 1.0.
	alSourcef(stream->source, AL_PITCH, 1.001f);
#else
	alSourcef(stream->source, AL_PITCH, 1.0f);
#endif
	if (sound_GetError() != AL_NO_ERROR)
	{
		delete decoder;
		delete stream;
		return nullptr;
	}
	int res =0;
	// Create some OpenAL buffers to store the decoded data in
	static ALuint       *alBuffersIds = (ALuint *) malloc(buffer_count * sizeof(ALuint));
	memset(alBuffersIds, 0, buffer_count * sizeof(ALuint));
	alGenBuffers(buffer_count, alBuffersIds);
	if (sound_GetError() != AL_NO_ERROR) { goto _error; }
	// Copy the audio data into one of OpenAL's own buffers
	ASSERT(bufferSize <= static_cast<size_t>(std::numeric_limits<ALsizei>::max()), "soundBuffer->size (%zu) exceeds ALsizei::max", bufferSize);
	
	res = sound_fillNBuffers(alBuffersIds, decoder, buffer_count, bufferSize);
	// Bail out if we didn't fill any buffers
	if (res <= 0)
	{
		debug(LOG_ERROR, "Failed to fill buffers with decoded audio data!");
		goto _error_with_albuffers;
	}
	// Attach the OpenAL buffers to our OpenAL source
	alGetError();
	if (res < buffer_count)
	{
		// free unused buffers
		debug(LOG_INFO, "freeing unused %i buffers", buffer_count - res);
		alDeleteBuffers(buffer_count - res, alBuffersIds + res);
		if (sound_GetError() != AL_NO_ERROR) {	goto _error_with_albuffers; }
	}

	alSourceQueueBuffers(stream->source, res, alBuffersIds);
	if (sound_GetError() != AL_NO_ERROR) {	goto _error_with_albuffers; }

	// Start playing the source
	alGetError();
	alSourcePlay(stream->source);
	if (sound_GetError() != AL_NO_ERROR) {	goto _error_with_albuffers; }

	// Set callback info
	stream->onFinished = onFinished;
	stream->user_data = user_data;

	// Prepend this stream to the linked list
	stream->next = active_streams;
	active_streams = stream;

	return stream;

	_error_with_albuffers:
		alDeleteBuffers(buffer_count, alBuffersIds);

	_error:
		delete stream;
		delete decoder;
		alDeleteSources(1, &stream->source);
		return nullptr;
}

/** Checks if the stream is playing.
 *  \param stream the stream to check
 *  \post true if playing, false otherwise.
 *
 */
bool sound_isStreamPlaying(AUDIO_STREAM *stream)
{
	ALint state;
	alGetError();
	if (stream)
	{
		alGetSourcei(stream->source, AL_SOURCE_STATE, &state);
		sound_GetError();
		if (state == AL_PLAYING)
		{
			return true;
		}
	}
	return false;
}

/** Stops the current stream from playing.
 *  \param stream the stream to stop
 *  \post The stopped stream will be destroyed on the next invocation of
 *        sound_UpdateStreams(). So calling this function will result in the
 *        \c onFinished callback being called and the \c stream pointer becoming
 *        invalid.
 */
void sound_StopStream(AUDIO_STREAM *stream)
{
	assert(stream != nullptr);

	alGetError();	// clear error codes
	stream->queuedStop = true;
	// Tell OpenAL to stop playing on the given source
	alSourceStop(stream->source);
	sound_GetError();
}

/** Pauses playing of this stream until playing is resumed with
 *  sound_ResumeStream() or completely stopped with sound_StopStream().
 *  \param stream the stream to pause playing for
 */
void sound_PauseStream(AUDIO_STREAM *stream)
{
	ALint state;

	// To be sure we won't go mutilating this OpenAL source, check whether
	// it's playing first.
	alGetError();
	alGetSourcei(stream->source, AL_SOURCE_STATE, &state);
	sound_GetError();

	if (state != AL_PLAYING)
	{
		return;
	}

	// Pause playing of this OpenAL source
	alSourcePause(stream->source);
	sound_GetError();
}

/** Resumes playing of a stream that's paused by means of sound_PauseStream().
 *  \param stream the stream to resume playing for
 */
void sound_ResumeStream(AUDIO_STREAM *stream)
{
	ALint state;

	// To be sure we won't go mutilating this OpenAL source, check whether
	// it's paused first.
	alGetError();
	alGetSourcei(stream->source, AL_SOURCE_STATE, &state);
	sound_GetError();

	if (state != AL_PAUSED)
	{
		return;
	}

	// Resume playing of this OpenAL source
	alSourcePlay(stream->source);
	sound_GetError();
}

/** Retrieve the playing volume of the given stream.
 *
 *  @param stream the stream to retrieve the volume for.
 *
 *  @return a floating point value between 0.f and 1.f, representing this
 *          stream's volume.
 */
float sound_GetStreamVolume(const AUDIO_STREAM *stream)
{
	ALfloat volume;
	alGetError();
	alGetSourcef(stream->source, AL_GAIN, &volume);
	sound_GetError();

	return volume;
}

/** Set the playing volume of the given stream.
 *
 *  @param stream the stream to change the volume for.
 *  @param volume a floating point value between 0.f and 1.f, to use as this
 *                @c stream's volume.
 */
void sound_SetStreamVolume(AUDIO_STREAM *stream, float volume)
{
	alGetError();
	stream->volume = volume;
	alSourcef(stream->source, AL_GAIN, stream->volume);
	sound_GetError();
}

double sound_GetStreamTotalTime(AUDIO_STREAM *stream)
{
	return stream->decoder->totalTime();
}

/** Update the given stream (="alSource" in openAL parlance) by making sure its buffers remain full
 *  \param stream the stream to update
 *  \return true when the stream is still playing, false when it has stopped
 */
static bool sound_UpdateStream(AUDIO_STREAM *stream)
{
	ALint state, buffers_processed_count;
	alGetError();
	alGetSourcei(stream->source, AL_SOURCE_STATE, &state);
	sound_GetError();

	if (state != AL_PLAYING && state != AL_PAUSED && (state != AL_STOPPED || stream->queuedStop))
	{
		return false;
	}

	// Retrieve the amount of buffers which were processed and need refilling
	alGetSourcei(stream->source, AL_BUFFERS_PROCESSED, &buffers_processed_count);
	if (sound_GetError() != AL_NO_ERROR) { return false; }
	if (buffers_processed_count == 0) { return true; }

	// Determine PCM data format
	ALuint *alBuffersIds = (ALuint *) malloc(buffers_processed_count * sizeof(ALuint));
	alSourceUnqueueBuffers(stream->source, buffers_processed_count, alBuffersIds);
	if (sound_GetError() != AL_NO_ERROR) { return false; }

	auto freeUnusedALBuffers = [alBuffersIds, buffers_processed_count](ALint startingIdx) {
		if (startingIdx < buffers_processed_count)
		{
			alDeleteBuffers(buffers_processed_count - startingIdx, &alBuffersIds[startingIdx]);
		}
	};

	const auto res = sound_fillNBuffers(alBuffersIds, stream->decoder, buffers_processed_count, bufferSize);
	if (res == 0)
	{
		// nothing more to read and queue - will be deleted with sound_DestroyStream (later, when done playing)
		sound_GetError();
		freeUnusedALBuffers(0);
		free(alBuffersIds);
		if (state != AL_STOPPED)
		{
			return true; // must return true here - don't shortcut playing the remaining buffers!
		}
		else
		{
			// no more buffers to read, and the existing audio has stopped
			return false;
		}
	}
	if (res < 0)
	{
		debug(LOG_ERROR, "bailing out");
		freeUnusedALBuffers(0);
		free(alBuffersIds);
		return false;
	}
	// Reattach the filled buffers to the source
	alSourceQueueBuffers(stream->source, res, alBuffersIds);
	// Delete any unused unqueued buffers
	freeUnusedALBuffers(res);
	sound_GetError();
	free(alBuffersIds);

	if (state == AL_STOPPED && !stream->queuedStop)
	{
		// Resume playing of this OpenAL source
		debug(LOG_SOUND, "Auto-resuming play of stream");
		alSourcePlay(stream->source);
		if (sound_GetError() != AL_NO_ERROR) { return false; }
	}

	return true;
}

/** Destroy the given stream (="alSource" in openAL parlance) and release its associated resources. This function
 *  also handles calling of the \c onFinished callback function and closing of
 *  the PhysicsFS file handle.
 *  \param stream the stream to destroy
 *  \note  we are not exiting on errors, we don't want to leak memory...
 */
static void sound_DestroyStream(AUDIO_STREAM *stream)
{
	ALint buffers_processed_count = 0;
	ALuint *buffers = nullptr;

	// Stop the OpenAL source from playing
	alGetError();
	alSourceStop(stream->source);

	sound_GetError();

	// Retrieve the amount of buffers which were processed
	alGetSourcei(stream->source, AL_BUFFERS_PROCESSED, &buffers_processed_count);
	if(sound_GetError() != AL_NO_ERROR)
	{
		buffers_processed_count = 0;
		// proceed as if nothing happened
	}

	// Detach all buffers and retrieve their ID numbers
	if (buffers_processed_count > 0)
	{
		const size_t buffer_count_sizet = static_cast<size_t>(buffers_processed_count);
		buffers = (ALuint *)malloc(buffer_count_sizet * sizeof(ALuint));
		memset(buffers, 0, buffer_count_sizet * sizeof(ALuint));
		alSourceUnqueueBuffers(stream->source, buffers_processed_count, buffers);
		sound_GetError();

		// Destroy all of these buffers
		alDeleteBuffers(buffers_processed_count, buffers);
		sound_GetError();
		free(buffers);
	}
	else
	{
		debug(LOG_SOUND, "alGetSourcei(AL_BUFFERS_PROCESSED) returned count: %d", buffers_processed_count);
	}

	// Destroy the OpenAL source
	alDeleteSources(1, &stream->source);
	sound_GetError();

	// Destroy the sound decoder
	delete stream->decoder;

	// Now call the finished callback
	if (stream->onFinished)
	{
		ASSERT(stream->user_data == nullptr, "user_data was not null!");
		stream->onFinished(stream, stream->user_data);
	}

	// Free the memory used by this stream
	delete stream;
}

/** Update all currently running streams(="alSource"s) and destroy them when they're finished.
 */
static void sound_UpdateStreams()
{
	AUDIO_STREAM *stream = active_streams, *previous = nullptr, *next = nullptr;

	while (stream != nullptr)
	{
		next = stream->next;

		// Attempt to update the current stream, if we find that impossible,
		// destroy it instead.
		if (!sound_UpdateStream(stream))
		{
			// First remove our current stream from the linked list
			if (previous)
			{
				// Make the previous item skip over the current to the next item
				previous->next = next;
			}
			else
			{
				// Apparently this is the first item in the list, so make the
				// next item the list-head.
				active_streams = next;
			}

			// Now actually destroy the current stream
			sound_DestroyStream(stream);

			// Make sure the current stream pointer is intact again
			stream = next;

			// Skip regular style iterator incrementing
			continue;
		}

		// Increment our iterator pair
		previous = stream;
		stream = stream->next;
	}
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_StopSample(AUDIO_SAMPLE *psSample)
{
	if (psSample->iSample == (ALuint)SAMPLE_NOT_ALLOCATED)
	{
		debug(LOG_SOUND, "sound_StopSample: sample number (%u) out of range, we probably have run out of available OpenAL sources", psSample->iSample);
		return;
	}
	alGetError();	// clear error codes
	// Tell OpenAL to stop playing the given sample
	alSourceStop(psSample->iSample);
	sound_GetError();
}

void sound_SetPlayerPos(Vector3f pos)
{
	alListener3f(AL_POSITION, pos.x, pos.y, pos.z);
	sound_GetError();
}

/**
 * Sets the player's orientation to use for sound
 * \param angle the angle in radians
 @NOTE the up vector is swapped because of qsound idiosyncrasies
 @FIXME we don't use qsound, but it still is in qsound 'format'...
*/
void sound_SetPlayerOrientation(float angle)
{
	const ALfloat ori[6] =
	{
		-sinf(angle), cosf(angle), 0.0f,	// forward (at) vector
		0.0f, 0.0f, 1.0f,					// up vector
	};
	alGetError();
	alListenerfv(AL_ORIENTATION, ori);
	sound_GetError();
}

/**
 * Sets the player's orientation to use for sound
 * \param forward forward pointing vector
 * \param up      upward pointing vector
 */
void sound_SetPlayerOrientationVector(Vector3f forward, Vector3f up)
{
	const ALfloat ori[6] =
	{
		forward.x, forward.y, forward.z,
		up.x,      up.y,      up.z,
	};
	alGetError();
	alListenerfv(AL_ORIENTATION, ori);
	sound_GetError();
}

//*
// =======================================================================================================================
// Compute the sample's volume relative to AL_POSITION.
// =======================================================================================================================
//
void sound_SetObjectPosition(AUDIO_SAMPLE *psSample)
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// coordinates
	float	listenerX, listenerY, listenerZ, dX, dY, dZ;

	// calculation results
	float	distance, gain;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// only set it when we have a valid sample
	if (!psSample)
	{
		return;
	}
	alGetError();
	// compute distance
	alGetListener3f(AL_POSITION, &listenerX, &listenerY, &listenerZ);
	sound_GetError();
	dX = psSample->x  - listenerX; // distances on all axis
	dY = psSample->y - listenerY;
	dZ = psSample->z - listenerZ;
	distance = sqrtf(dX * dX + dY * dY + dZ * dZ); // Pythagorean theorem

	// compute gain
	gain = (1.0f - (distance * ATTENUATION_FACTOR)) * psSample->fVol * sfx3d_volume;
	// max volume
	if (gain > 1.0f)
	{
		gain = 1.0f;
	}
	if (gain < 0.0f)
	{
		// this sample can't be heard right now
		gain = 0.0f;
	}
	alSourcef(psSample->iSample, AL_GAIN, gain);

	// the alSource3i variant would be better, if it wouldn't provide linker errors however
	alSource3f(psSample->iSample, AL_POSITION, (float)psSample->x, (float)psSample->y, (float)psSample->z);
	sound_GetError();

	return;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_PauseSample(AUDIO_SAMPLE *psSample)
{
	alGetError();
	alSourcePause(psSample->iSample);
	sound_GetError();
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_ResumeSample(AUDIO_SAMPLE *psSample)
{
	alGetError();
	alSourcePlay(psSample->iSample);
	sound_GetError();
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_PauseAll(void)
{
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_ResumeAll(void)
{
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_StopAll(void)
{
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
bool sound_SampleIsFinished(AUDIO_SAMPLE *psSample)
{
	ALenum	state;
	alGetError();
	alGetSourcei(psSample->iSample, AL_SOURCE_STATE, &state);
	sound_GetError(); // check for an error and clear the error state for later on in this function
	if (state == AL_PLAYING || state == AL_PAUSED)
	{
		return false;
	}

	if (psSample->iSample != (ALuint)AL_INVALID)
	{
		alDeleteSources(1, &(psSample->iSample));
		sound_GetError();
		psSample->iSample = AL_INVALID;
	}

	return true;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//

float sound_GetUIVolume()
{
	return sfx_volume;
}

void sound_SetUIVolume(float volume)
{
	sfx_volume = volume;

	// Keep volume in the range of 0.0 - 1.0
	if (sfx_volume < 0.0)
	{
		sfx_volume = 0.0;
	}
	else if (sfx_volume > 1.0)
	{
		sfx_volume = 1.0;
	}
}

float sound_GetEffectsVolume()
{
	return sfx3d_volume;
}

void sound_SetEffectsVolume(float volume)
{
	sfx3d_volume = volume;

	// Keep volume in the range of 0.0 - 1.0
	if (sfx3d_volume < 0.0)
	{
		sfx3d_volume = 0.0;
	}
	else if (sfx3d_volume > 1.0)
	{
		sfx3d_volume = 1.0;
	}
}
