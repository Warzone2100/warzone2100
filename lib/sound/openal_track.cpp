/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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

#ifdef WZ_OS_MAC
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#include <physfs.h>
#include <string.h>
#include <math.h>

#include "tracklib.h"
#include "audio.h"
#include "cdaudio.h"
#include "oggvorbis.h"
#include "openal_error.h"
#include "mixer.h"

static ALuint current_queue_sample = -1;

static bool openal_initialized = false;

struct AUDIO_STREAM
{
	ALuint                  source;        // OpenAL name of the sound source
	struct OggVorbisDecoderState *decoder;
	PHYSFS_file *fileHandle;
	float                   volume;

	// Callbacks
	void (*onFinished)(void *);
	void                    *user_data;

	size_t                  bufferSize;

	// Linked list pointer
	AUDIO_STREAM           *next;
};

struct SAMPLE_LIST
{
	AUDIO_SAMPLE   *curr;
	SAMPLE_LIST    *next;
};

static SAMPLE_LIST *active_samples = NULL;

static AUDIO_STREAM *active_streams = NULL;

static ALfloat		sfx_volume = 1.0;
static ALfloat		sfx3d_volume = 1.0;

static ALCdevice *device = NULL;
static ALCcontext *context = NULL;


/** Removes the given sample from the "active_samples" linked list
 *  \param previous either NULL (if \c to_remove is the first item in the
 *                  list) or the item occurring just before \c to_remove in
 *                  the list
 *  \param to_remove the item to actually remove from the list
 */
static void sound_RemoveSample(SAMPLE_LIST *previous, SAMPLE_LIST *to_remove)
{
	if (previous != NULL && previous != to_remove)
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

//*
// =======================================================================================================================
// =======================================================================================================================
//
bool sound_InitLibrary(void)
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

#ifdef WZ_OS_WIN
	/* HACK: Select the "software" OpenAL device on Windows because it
	 *       provides 256 sound sources (unlike most Creative's default
	 *       which provides only 16), causing our lack of source-management
	 *       to be significantly less noticeable.
	 */
	device = alcOpenDevice("Generic Software");

	// If the software device isn't available, fall back to default
	if (!device)
#endif
	{
		// Open default device
		device = alcOpenDevice(NULL);
	}

	if (!device)
	{
		debug(LOG_ERROR, "Couldn't open audio device.");
		return false;
	}

	// Print current device name and add it to dump info
	deviceName = alcGetString(device, ALC_DEVICE_SPECIFIER);
	debug(LOG_SOUND, "Current audio device: %s", deviceName);
	ssprintf(buf, "OpenAL Device Name: %s", deviceName);
	addDumpInfo(buf);

	context = alcCreateContext(device, NULL);		//NULL was contextAttributes
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

static void sound_UpdateStreams(void);

void sound_ShutdownLibrary(void)
{
	AUDIO_STREAM *stream;
	SAMPLE_LIST *aSample = active_samples, * tmpSample = NULL;

	if (!openal_initialized)
	{
		return;
	}
	debug(LOG_SOUND, "starting shutdown");

	// Stop all streams, sound_UpdateStreams() will deallocate all stopped streams
	for (stream = active_streams; stream != NULL; stream = stream->next)
	{
		sound_StopStream(stream);
	}
	sound_UpdateStreams();

	alcGetError(device);	// clear error codes

	/* On Linux since this caused some versions of OpenAL to hang on exit. - Per */
	debug(LOG_SOUND, "make default context NULL");
	alcMakeContextCurrent(NULL);
	sound_GetContextError(device);

	debug(LOG_SOUND, "destroy previous context");
	alcDestroyContext(context); // this gives a long delay on some impl.
	sound_GetContextError(device);

	debug(LOG_SOUND, "close device");
	if (alcCloseDevice(device) == ALC_FALSE)
	{
		debug(LOG_SOUND, "OpenAl could not close the audio device.");
	}

	while (aSample)
	{
		tmpSample = aSample->next;
		free(aSample);
		aSample = tmpSample;
	}
	active_samples = NULL;
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
	*sample = (*previous != NULL) ? (*previous)->next : active_samples;
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

void sound_Update()
{
	SAMPLE_LIST *node = active_samples;
	SAMPLE_LIST *previous = NULL;
	ALCenum err;
	ALfloat gain;

	if (!openal_initialized)
	{
		return;
	}

	// Update all streaming audio
	sound_UpdateStreams();

	while (node != NULL)
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

	err = sound_GetContextError(device);
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
		SAMPLE_LIST *previous = NULL;

		// We need to remove it from the queue of actively played samples
		while (node != NULL)
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

/** Decodes an opened OggVorbis file into an OpenAL buffer
 *  \param psTrack pointer to object which will contain the final buffer
 *  \param PHYSFS_fileHandle file handle given by PhysicsFS to the opened file
 *  \return on success the psTrack pointer, otherwise it will be free'd and a NULL pointer is returned instead
 */
static inline TRACK *sound_DecodeOggVorbisTrack(TRACK *psTrack, PHYSFS_file *PHYSFS_fileHandle)
{
	ALenum		format;
	ALuint		buffer;
	struct OggVorbisDecoderState *decoder;
	soundDataBuffer	*soundBuffer;

	if (!openal_initialized)
	{
		return NULL;
	}

	decoder = sound_CreateOggVorbisDecoder(PHYSFS_fileHandle, true);
	if (decoder == NULL)
	{
		debug(LOG_WARNING, "Failed to open audio file for decoding");
		free(psTrack);
		return NULL;
	}

	soundBuffer = sound_DecodeOggVorbis(decoder, 0);
	sound_DestroyOggVorbisDecoder(decoder);

	if (soundBuffer == NULL)
	{
		free(psTrack);
		return NULL;
	}

	if (soundBuffer->size == 0)
	{
		debug(LOG_WARNING, "sound_DecodeOggVorbisTrack: OggVorbis track is entirely empty after decoding");
// NOTE: I'm not entirely sure if a track that's empty after decoding should be
//       considered an error condition. Therefore I'll only error out on DEBUG
//       builds. (Returning NULL here __will__ result in a program termination.)
#ifdef DEBUG
		free(soundBuffer);
		free(psTrack);
		return NULL;
#endif
	}

	// Determine PCM data format
	format = (soundBuffer->channelCount == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

	// Create an OpenAL buffer and fill it with the decoded data
	alGenBuffers(1, &buffer);
	sound_GetError();
	alBufferData(buffer, format, soundBuffer->data, soundBuffer->size, soundBuffer->frequency);
	sound_GetError();

	free(soundBuffer);

	// save buffer name in track
	psTrack->iBufferName = buffer;

	return psTrack;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
TRACK *sound_LoadTrackFromFile(const char *fileName)
{
	TRACK *pTrack;
	PHYSFS_file *fileHandle;
	size_t filename_size;
	char *track_name;

	// Use PhysicsFS to open the file
	fileHandle = PHYSFS_openRead(fileName);
	debug(LOG_NEVER, "Reading...[directory: %s] %s", PHYSFS_getRealDir(fileName), fileName);
	if (fileHandle == NULL)
	{
		debug(LOG_ERROR, "sound_LoadTrackFromFile: PHYSFS_openRead(\"%s\") failed with error: %s\n", fileName, PHYSFS_getLastError());
		return NULL;
	}

	if (GetLastResourceFilename() == NULL)
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
	if (pTrack == NULL)
	{
		debug(LOG_FATAL, "sound_ConstructTrack: couldn't allocate memory\n");
		abort();
		return NULL;
	}

	// Initialize everyting (except for the filename) to zero
	memset(pTrack, 0, sizeof(TRACK));

	// Set filename pointer; if the filename (as returned by
	// GetLastResourceFilename()) is a NULL pointer, then this will be a
	// NULL pointer as well.
	track_name = filename_size ? (char *)(pTrack + 1) : NULL;

	// Copy the filename into the struct, if we don't have a NULL pointer
	if (filename_size != 0)
	{
		strcpy(track_name, GetLastResourceFilename());
	}
	pTrack->fileName = track_name;

	// Now use sound_ReadTrackFromBuffer to decode the file's contents
	pTrack = sound_DecodeOggVorbisTrack(pTrack, fileHandle);

	PHYSFS_close(fileHandle);
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
	SAMPLE_LIST *previous = NULL;

	while (node != NULL)
	{
		if (node->curr->psObj == psSample->psObj)
		{
			debug(LOG_MEMORY, "Removing object 0x%p from active_samples list 0x%p\n", psSample->psObj, node);

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

	// HACK: this is a workaround for a bug in the 64bit implementation of OpenAL on GNU/Linux
	// The AL_PITCH value really should be 1.0.
	alSourcef(psSample->iSample, AL_PITCH, 1.001f);

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

/** Plays the audio data from the given file
 *  \param fileHandle PhysicsFS file handle to stream the audio from
 *  \param volume the volume to play the audio at (in a range of 0.0 to 1.0)
 *  \param onFinished callback to invoke when we're finished playing
 *  \param user_data user-data pointer to pass to the \c onFinished callback
 *  \return a pointer to the currently playing stream when playing started
 *          successfully, NULL otherwise.
 *  \post When a non-NULL pointer is returned the audio stream system will
 *        close the PhysicsFS file handle. Otherwise (when false is returned)
 *        this is left to the user.
 *  \note The returned pointer will become invalid/dangling immediately after
 *        the \c onFinished callback is invoked.
 *  \note You must _never_ manually free() the memory used by the returned
 *        pointer.
 */
AUDIO_STREAM *sound_PlayStream(PHYSFS_file *fileHandle, float volume, void (*onFinished)(void *), void *user_data)
{
	// Default buffer size
	static const size_t streamBufferSize = 16 * 1024;
	// Default buffer count
	static const unsigned int buffer_count = 2;

	return sound_PlayStreamWithBuf(fileHandle, volume, onFinished, user_data, streamBufferSize, buffer_count);
}

/** Plays the audio data from the given file
 *  \param fileHandle,volume,onFinished,user_data see sound_PlayStream()
 *  \param streamBufferSize the size to use for the decoded audio buffers
 *  \param buffer_count the amount of audio buffers to use
 *  \see sound_PlayStream() for details about the rest of the function
 *       parameters and other details.
 */
AUDIO_STREAM *sound_PlayStreamWithBuf(PHYSFS_file *fileHandle, float volume, void (*onFinished)(void *), void *user_data, size_t streamBufferSize, unsigned int buffer_count)
{
	AUDIO_STREAM *stream;
	ALuint       *buffers = (ALuint *)alloca(sizeof(ALuint) * buffer_count);
	ALint error;
	unsigned int i;

	if (!openal_initialized)
	{
		debug(LOG_WARNING, "OpenAL isn't initialized, not creating an audio stream");
		return NULL;
	}

	stream = (AUDIO_STREAM *)malloc(sizeof(AUDIO_STREAM));
	if (stream == NULL)
	{
		debug(LOG_FATAL, "sound_PlayStream: Out of memory");
		abort();
		return NULL;
	}

	// Clear error codes
	alGetError();

	// Retrieve an OpenAL sound source
	alGenSources(1, &(stream->source));

	error = sound_GetError();
	if (error != AL_NO_ERROR)
	{
		// Failed to create OpenAL sound source, so bail out...
		debug(LOG_SOUND, "alGenSources failed, most likely out of sound sources");
		free(stream);
		return NULL;
	}

	stream->fileHandle = fileHandle;

	stream->decoder = sound_CreateOggVorbisDecoder(stream->fileHandle, false);
	if (stream->decoder == NULL)
	{
		debug(LOG_ERROR, "sound_PlayStream: Failed to open audio file for decoding");
		free(stream);
		return NULL;
	}

	stream->volume = volume;
	stream->bufferSize = streamBufferSize;

	alSourcef(stream->source, AL_GAIN, stream->volume);

	// HACK: this is a workaround for a bug in the 64bit implementation of OpenAL on GNU/Linux
	// The AL_PITCH value really should be 1.0.
	alSourcef(stream->source, AL_PITCH, 1.001f);

	// Create some OpenAL buffers to store the decoded data in
	alGenBuffers(buffer_count, buffers);
	sound_GetError();

	// Fill some buffers with audio data
	for (i = 0; i < buffer_count; ++i)
	{
		// Decode some audio data
		soundDataBuffer *soundBuffer = sound_DecodeOggVorbis(stream->decoder, stream->bufferSize);

		// If we actually decoded some data
		if (soundBuffer && soundBuffer->size > 0)
		{
			// Determine PCM data format
			ALenum format = (soundBuffer->channelCount == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

			// Copy the audio data into one of OpenAL's own buffers
			alBufferData(buffers[i], format, soundBuffer->data, soundBuffer->size, soundBuffer->frequency);
			sound_GetError();

			// Clean up our memory
			free(soundBuffer);
		}
		else
		{
			// If no data has been decoded we're probably at the end of our
			// stream. So cleanup the excess stuff here.

			// First remove the data buffer itself
			free(soundBuffer);

			// Then remove OpenAL's buffers
			alDeleteBuffers(buffer_count - i, &buffers[i]);
			sound_GetError();

			break;
		}
	}

	// Bail out if we didn't fill any buffers
	if (i == 0)
	{
		debug(LOG_ERROR, "Failed to fill buffers with decoded audio data!");

		// Destroy the decoder
		sound_DestroyOggVorbisDecoder(stream->decoder);

		// Destroy the OpenAL source
		alDeleteSources(1, &stream->source);

		// Free allocated memory
		free(stream);

		return NULL;
	}

	// Attach the OpenAL buffers to our OpenAL source
	// (i = the amount of buffers we worked on in the above for-loop)
	alSourceQueueBuffers(stream->source, i, buffers);
	sound_GetError();

	// Start playing the source
	alSourcePlay(stream->source);

	sound_GetError();

	// Set callback info
	stream->onFinished = onFinished;
	stream->user_data = user_data;

	// Prepend this stream to the linked list
	stream->next = active_streams;
	active_streams = stream;

	return stream;
}

/** Checks if the stream is playing.
 *  \param stream the stream to check
 *  \post true if playing, false otherwise.
 *
 */
bool sound_isStreamPlaying(AUDIO_STREAM *stream)
{
	ALint state;

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
	assert(stream != NULL);

	alGetError();	// clear error codes
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

	// To be sure we won't go mutilating this OpenAL source, check wether
	// it's playing first.
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

	// To be sure we won't go mutilating this OpenAL source, check wether
	// it's paused first.
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
	stream->volume = volume;
	alSourcef(stream->source, AL_GAIN, stream->volume);
	sound_GetError();
}

/** Update the given stream by making sure its buffers remain full
 *  \param stream the stream to update
 *  \return true when the stream is still playing, false when it has stopped
 */
static bool sound_UpdateStream(AUDIO_STREAM *stream)
{
	ALint state, buffer_count;

	alGetSourcei(stream->source, AL_SOURCE_STATE, &state);
	sound_GetError();

	if (state != AL_PLAYING && state != AL_PAUSED)
	{
		return false;
	}

	// Retrieve the amount of buffers which were processed and need refilling
	alGetSourcei(stream->source, AL_BUFFERS_PROCESSED, &buffer_count);
	sound_GetError();

	// Refill and reattach all buffers
	for (; buffer_count != 0; --buffer_count)
	{
		soundDataBuffer *soundBuffer;
		ALuint buffer;

		// Retrieve the buffer to work on
		alSourceUnqueueBuffers(stream->source, 1, &buffer);
		sound_GetError();

		// Decode some data to stuff in our buffer
		soundBuffer = sound_DecodeOggVorbis(stream->decoder, stream->bufferSize);

		// If we actually decoded some data
		if (soundBuffer && soundBuffer->size > 0)
		{
			// Determine PCM data format
			ALenum format = (soundBuffer->channelCount == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

			// Insert the data into the buffer
			alBufferData(buffer, format, soundBuffer->data, soundBuffer->size, soundBuffer->frequency);
			sound_GetError();

			// Reattach the buffer to the source
			alSourceQueueBuffers(stream->source, 1, &buffer);
			sound_GetError();
		}
		else
		{
			// If no data has been decoded we're probably at the end of our
			// stream. So cleanup this buffer.

			// Then remove OpenAL's buffer
			alDeleteBuffers(1, &buffer);
			sound_GetError();
		}

		// Now remove the data buffer itself
		free(soundBuffer);
	}

	return true;
}

/** Destroy the given stream and release its associated resources. This function
 *  also handles calling of the \c onFinished callback function and closing of
 *  the PhysicsFS file handle.
 *  \param stream the stream to destroy
 */
static void sound_DestroyStream(AUDIO_STREAM *stream)
{
	ALint buffer_count;
	ALuint *buffers;
	ALint error;

	// Stop the OpenAL source from playing
	alSourceStop(stream->source);
	error = sound_GetError();

	if (error != AL_NO_ERROR)
	{
		// FIXME: We should really handle these errors.
	}

	// Retrieve the amount of buffers which were processed
	alGetSourcei(stream->source, AL_BUFFERS_PROCESSED, &buffer_count);
	error = sound_GetError();
	if (error != AL_NO_ERROR)
	{
		/* FIXME: We're leaking memory and resources here when bailing
		 * out. But not doing so could cause stack overflows as a
		 * result of the below alloca() call (due to buffer_count not
		 * being properly initialised.
		 */
		debug(LOG_SOUND, "alGetSourcei(AL_BUFFERS_PROCESSED) failed; bailing out...");
		return;
	}

	// Detach all buffers and retrieve their ID numbers
	buffers = (ALuint *)alloca(buffer_count * sizeof(ALuint));
	alSourceUnqueueBuffers(stream->source, buffer_count, buffers);
	sound_GetError();

	// Destroy all of these buffers
	alDeleteBuffers(buffer_count, buffers);
	sound_GetError();

	// Destroy the OpenAL source
	alDeleteSources(1, &stream->source);
	sound_GetError();

	// Destroy the sound decoder
	sound_DestroyOggVorbisDecoder(stream->decoder);

	// Now close the file
	PHYSFS_close(stream->fileHandle);

	// Now call the finished callback
	if (stream->onFinished)
	{
		stream->onFinished(stream->user_data);
	}

	// Free the memory used by this stream
	free(stream);
}

/** Update all currently running streams and destroy them when they're finished.
 */
static void sound_UpdateStreams()
{
	AUDIO_STREAM *stream = active_streams, *previous = NULL, *next = NULL;

	while (stream != NULL)
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
	alSourcePause(psSample->iSample);
	sound_GetError();
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_ResumeSample(AUDIO_SAMPLE *psSample)
{
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
