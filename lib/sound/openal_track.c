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
/** \file
 *  Sound library-specific functions
 */

// this has to be first
#include "lib/framework/frame.h"
#include "lib/framework/math-help.h"
#include "lib/framework/frameresource.h"

#ifndef WZ_NOSOUND
# ifdef WZ_OS_MAC
#  include <OpenAL/al.h>
#  include <OpenAL/alc.h>
# else
#  include <AL/al.h>
#  include <AL/alc.h>
# endif
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

#define ATTENUATION_FACTOR	0.0003f

#ifndef WZ_NOSOUND
ALuint current_queue_sample = -1;
#endif

struct __audio_stream
{
#ifndef WZ_NOSOUND
	ALuint                  source;        // OpenAL name of the sound source
#endif
	struct OggVorbisDecoderState* decoder;
	PHYSFS_file* fileHandle;
	float                   volume;

	// Callbacks
	void                    (*onFinished)(void*);
	void                    *user_data;

	size_t                  bufferSize;

	// Linked list pointer
	struct __audio_stream   *next;
};

typedef struct	SAMPLE_LIST
{
	struct AUDIO_SAMPLE *curr;
	struct SAMPLE_LIST	*next;
} SAMPLE_LIST;

static SAMPLE_LIST *active_samples = NULL;
static AUDIO_STREAM* active_streams = NULL;

#ifndef WZ_NOSOUND
static ALfloat		sfx_volume = 1.0;
static ALfloat		sfx3d_volume = 1.0;

static ALCdevice* device = 0;
static ALCcontext* context = 0;
#endif

BOOL openal_initialized = false;

/** Removes the given sample from the "active_samples" linked list
 *  \param previous either NULL (if \c to_remove is the first item in the
 *                  list) or the item occurring just before \c to_remove in
 *                  the list
 *  \param to_remove the item to actually remove from the list
 */
static void sound_RemoveSample(SAMPLE_LIST* previous, SAMPLE_LIST* to_remove)
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

#ifndef WZ_NOSOUND
static void PrintOpenALVersion(code_part part)
{
	debug(part, "OpenAL Vendor: %s", alGetString(AL_VENDOR));
	debug(part, "OpenAL Version: %s", alGetString(AL_VERSION));
	debug(part, "OpenAL Renderer: %s", alGetString(AL_RENDERER));
	debug(part, "OpenAL Extensions: %s", alGetString(AL_EXTENSIONS));
}
#endif

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_InitLibrary( void )
{
#ifndef WZ_NOSOUND
	int err;
	ALfloat listenerVel[3] = { 0.0, 0.0, 0.0 };
	ALfloat listenerOri[6] = { 0.0, 0.0, 1.0, 0.0, 1.0, 0.0 };

	device = alcOpenDevice(0);
	if(device == 0)
	{
		PrintOpenALVersion(LOG_ERROR);
		debug(LOG_ERROR, "Couldn't open audio device.");
		return false;
	}

	context = alcCreateContext(device, NULL);		//NULL was contextAttributes
	alcMakeContextCurrent(context);

	err = sound_GetDeviceError(device);
	if (err != ALC_NO_ERROR)
	{
		PrintOpenALVersion(LOG_ERROR);
		debug(LOG_ERROR, "Couldn't initialize audio context: %s", alcGetString(device, err));
		return false;
	}
#endif

	openal_initialized = true;

#ifndef WZ_NOSOUND
	// Clear Error Codes
	alGetError();
	alcGetError(device);

	// Check what version of Open AL we are using
	PrintOpenALVersion(LOG_SOUND);


	alListener3f(AL_POSITION, 0.f, 0.f, 0.f);
	alListenerfv( AL_VELOCITY, listenerVel );
	alListenerfv( AL_ORIENTATION, listenerOri );
	alDistanceModel( AL_NONE );
#endif
	return true;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_ShutdownLibrary( void )
{
	SAMPLE_LIST * aSample = active_samples, * tmpSample = NULL;

	debug(LOG_SOUND, "sound_ShutdownLibrary: starting shutdown");
#ifndef WZ_NOSOUND
	if(context != 0) {
#ifdef WIN32
		/* Ifdef'ed out the two lines below on Linux since this caused some versions
		 * of OpenAL to hang on exit. - Per */
		debug(LOG_SOUND, "sound_ShutdownLibrary: make default context NULL");
		alcMakeContextCurrent(NULL);		//this should work now -Q
#endif
		debug(LOG_SOUND, "sound_ShutdownLibrary: destroy previous context");
		alcDestroyContext(context); // this gives a long delay on some impl.
		context = 0;
	}
	debug(LOG_SOUND, "sound_ShutdownLibrary: close device");
	if(device != 0) {
		alcCloseDevice(device);
		device = 0;
	}
#endif

	while( aSample )
	{
		tmpSample = aSample->next;
		free( aSample );
		aSample = tmpSample;
	}
	active_samples = NULL;
}

/** Deletes the given sample and updates the \c previous and \c current iterators
 *  \param previous iterator to the previous sample in the list
 *  \param sample iterator to the current sample in the list which you want to delete
 */
static void sound_DestroyIteratedSample(SAMPLE_LIST** previous, SAMPLE_LIST** sample)
{
#ifndef WZ_NOSOUND
	// If an OpenAL source is associated with this sample, release it
	if ((*sample)->curr->iSample != (ALuint)AL_INVALID)
	{
		alDeleteSources(1, &(*sample)->curr->iSample);
		sound_GetError();
	}
#endif

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
	SAMPLE_LIST* node = active_samples;

	while(node)
	{
		num++;
		node = node->next;
	}
	return num;
}

static void sound_UpdateStreams(void);

void sound_Update()
{
#ifndef WZ_NOSOUND
	SAMPLE_LIST* node = active_samples;
	SAMPLE_LIST* previous = NULL;
	ALCenum err;

	// Update all streaming audio
	sound_UpdateStreams();

	while (node != NULL)
	{
		ALenum state, err;

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

			case AL_STOPPED:
				sound_FinishedCallback(node->curr);

			default:
				sound_DestroyIteratedSample(&previous, &node);
				break;
		}
	}

	// Reset the current error state
	alcGetError(device);

	alcProcessContext(context);

	err = sound_GetDeviceError(device);
	if (err != ALC_NO_ERROR)
	{
		debug(LOG_ERROR, "Error while processing audio context: %s",
		      alGetString(err));
	}
#endif
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_QueueSamplePlaying( void )
{
#ifndef WZ_NOSOUND
	ALenum	state;

	if ( current_queue_sample == (ALuint)AL_INVALID )
	{
		return false;
	}

	alGetSourcei(current_queue_sample, AL_SOURCE_STATE, &state);

	// Check whether an error occurred while retrieving the state.
	// If one did, the state returned is useless. So instead of
	// using it return false.
	if (sound_GetError() != AL_NO_ERROR)
		return false;

	if (state == AL_PLAYING)
	{
		return true;
	}

	if (current_queue_sample != (ALuint)AL_INVALID)
	{
		alDeleteSources(1, &current_queue_sample);
		sound_GetError();
		current_queue_sample = AL_INVALID;
	}
#endif
	return false;
}

/** Decodes an opened OggVorbis file into an OpenAL buffer
 *  \param psTrack pointer to object which will contain the final buffer
 *  \param PHYSFS_fileHandle file handle given by PhysicsFS to the opened file
 *  \return on success the psTrack pointer, otherwise it will be free'd and a NULL pointer is returned instead
 */
static inline TRACK* sound_DecodeOggVorbisTrack(TRACK *psTrack, PHYSFS_file* PHYSFS_fileHandle)
{
#ifndef WZ_NOSOUND
	ALenum		format;
	ALuint		buffer;

	struct OggVorbisDecoderState* decoder = sound_CreateOggVorbisDecoder(PHYSFS_fileHandle, true);
	soundDataBuffer* soundBuffer;

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
#endif

	return psTrack;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
TRACK* sound_LoadTrackFromFile(const char *fileName)
{
	TRACK* pTrack;
	PHYSFS_file* fileHandle;
	size_t filename_size;
	char* track_name;

	// Use PhysicsFS to open the file
	fileHandle = PHYSFS_openRead(fileName);
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
	pTrack = (TRACK*)malloc(sizeof(TRACK) + filename_size);
	if (pTrack == NULL)
	{
		debug( LOG_ERROR, "sound_ConstructTrack: couldn't allocate memory\n" );
		abort();
		return NULL;
	}

	// Initialize everyting (except for the filename) to zero
	memset(pTrack, 0, sizeof(TRACK));

	// Set filename pointer; if the filename (as returned by
	// GetLastResourceFilename()) is a NULL pointer, then this will be a
	// NULL pointer as well.
	track_name = filename_size ? (char*)(pTrack + 1) : NULL;

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

void sound_FreeTrack( TRACK *psTrack )
{
#ifndef WZ_NOSOUND
	alDeleteBuffers(1, &psTrack->iBufferName);
	sound_GetError();
#endif
}

#ifndef WZ_NOSOUND
static void sound_AddActiveSample( AUDIO_SAMPLE *psSample )
{
	SAMPLE_LIST *tmp = (SAMPLE_LIST *) malloc( sizeof(SAMPLE_LIST) );

	// Prepend the given sample to our list of active samples
	tmp->curr = psSample;
	tmp->next = active_samples;
	active_samples = tmp;
}
#endif

/** Routine gets rid of the psObj's sound sample and reference in active_samples.
 */
void sound_RemoveActiveSample( AUDIO_SAMPLE *psSample )
{
	SAMPLE_LIST* node = active_samples;
	SAMPLE_LIST* previous = NULL;

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

#ifndef WZ_NOSOUND
static bool sound_SetupChannel( AUDIO_SAMPLE *psSample )
{
	sound_AddActiveSample( psSample );

	return sound_TrackLooped(psSample->iTrack);
}
#endif

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_Play2DSample( TRACK *psTrack, AUDIO_SAMPLE *psSample, BOOL bQueued )
{
#ifndef WZ_NOSOUND
	ALfloat zero[3] = { 0.0, 0.0, 0.0 };
	ALfloat volume;

	if (sfx_volume == 0.0)
	{
		return false;
	}
	volume = ((float)psTrack->iVol / 100.0f);       // each object can have OWN volume!
	psSample->fVol = volume;                        // save computed volume
	volume *= sfx_volume;                           // and now take into account the Users sound Prefs.
	alGenSources( 1, &(psSample->iSample) );
	sound_GetError();
	alSourcef( psSample->iSample, AL_PITCH, 1.0f );
	alSourcef( psSample->iSample, AL_GAIN,volume );
	alSourcefv( psSample->iSample, AL_POSITION, zero );
	alSourcefv( psSample->iSample, AL_VELOCITY, zero );
	alSourcei( psSample->iSample, AL_BUFFER, psTrack->iBufferName );
	alSourcei( psSample->iSample, AL_SOURCE_RELATIVE, AL_TRUE );
	alSourcei( psSample->iSample, AL_LOOPING, (sound_SetupChannel(psSample)) ? AL_TRUE : AL_FALSE );
	sound_GetError();
	alSourcePlay( psSample->iSample );
	sound_GetError();
	if ( bQueued )
	{
		current_queue_sample = psSample->iSample;
	}
	else if ( current_queue_sample == psSample->iSample )
	{
		current_queue_sample = -1;
	}
#endif

	return true;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_Play3DSample( TRACK *psTrack, AUDIO_SAMPLE *psSample )
{
#ifndef WZ_NOSOUND
	ALfloat zero[3] = { 0.0, 0.0, 0.0 };
	ALfloat volume;

	if (sfx3d_volume == 0.0)
	{
		return false;
	}

	volume = ((float)psTrack->iVol / 100.f);        // max range is 0-100
	psSample->fVol = volume;                        // store results for later
	alGenSources( 1, &(psSample->iSample) );
	sound_GetError();
	// HACK: this is a workaround for a bug in the 64bit implementation of OpenAL on GNU/Linux
	// The AL_PITCH value really should be 1.0.
	alSourcef( psSample->iSample, AL_PITCH, 1.001 );

	sound_SetObjectPosition( psSample );
	alSourcefv( psSample->iSample, AL_VELOCITY, zero );
	alSourcei( psSample->iSample, AL_BUFFER, psTrack->iBufferName );
	alSourcei( psSample->iSample, AL_LOOPING, (sound_SetupChannel(psSample)) ? AL_TRUE : AL_FALSE );
	sound_GetError();
	alSourcePlay( psSample->iSample );
	sound_GetError();
#endif
	return true;
}

/** Plays the audio data from the given file
 *  \param fileHandle PhysicsFS file handle to stream the audio from
 *  \param volume the volume to play the audio at (in a range of 0.0 to 1.0)
 *  \param onFinished callback to invoke when we're finished playing
 *  \param user_data user-data pointer to pass to the \c onFinished callback
 *  \return a pointer to the currently playing stream when playing started
 *          succesfully, NULL otherwise.
 *  \post When a non-NULL pointer is returned the audio stream system will
 *        close the PhysicsFS file handle. Otherwise (when false is returned)
 *        this is left to the user.
 *  \note The returned pointer will become invalid/dangling immediately after
 *        the \c onFinished callback is invoked.
 *  \note You must _never_ manually free() the memory used by the returned
 *        pointer.
 */
AUDIO_STREAM* sound_PlayStream(PHYSFS_file* fileHandle, float volume, void (*onFinished)(void*), void* user_data)
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
AUDIO_STREAM* sound_PlayStreamWithBuf(PHYSFS_file* fileHandle, float volume, void (*onFinished)(void*), void* user_data, size_t streamBufferSize, unsigned int buffer_count)
{
	AUDIO_STREAM* stream;
	ALuint*       buffers = alloca(sizeof(ALuint) * buffer_count);

	unsigned int i;

	stream = malloc(sizeof(AUDIO_STREAM));
	if (stream == NULL)
	{
		debug(LOG_ERROR, "sound_PlayStream: Out of memory");
		abort();
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

	// Retrieve an OpenAL sound source
	alGenSources(1, &(stream->source));
	sound_GetError();

	// HACK: this is a workaround for a bug in the 64bit implementation of OpenAL on GNU/Linux
	// The AL_PITCH value really should be 1.0.
	alSourcef(stream->source, AL_PITCH, 1.001);

	// Create some OpenAL buffers to store the decoded data in
	alGenBuffers(buffer_count, buffers);
	sound_GetError();

	// Fill some buffers with audio data
	for (i = 0; i < buffer_count; ++i)
	{
		// Decode some audio data
		soundDataBuffer* soundBuffer = sound_DecodeOggVorbis(stream->decoder, stream->bufferSize);

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

/** Stops the current stream from playing.
 *  \param stream the stream to stop
 *  \post The stopped stream will be destroyed on the next invocation of
 *        sound_UpdateStreams(). So calling this function will result in the
 *        \c onFinished callback being called and the \c stream pointer becoming
 *        invalid.
 */
void sound_StopStream(AUDIO_STREAM* stream)
{
	assert(stream != NULL);

	// Tell OpenAL to stop playing on the given source
	alSourceStop(stream->source);
	sound_GetError();
}

/** Pauses playing of this stream until playing is resumed with
 *  sound_ResumeStream() or completely stopped with sound_StopStream().
 *  \param stream the stream to pause playing for
 */
void sound_PauseStream(AUDIO_STREAM* stream)
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
void sound_ResumeStream(AUDIO_STREAM* stream)
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

/** Update the given stream by making sure its buffers remain full
 *  \param stream the stream to update
 *  \return true when the stream is still playing, false when it has stopped
 */
static bool sound_UpdateStream(AUDIO_STREAM* stream)
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
		soundDataBuffer* soundBuffer;
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
static void sound_DestroyStream(AUDIO_STREAM* stream)
{
	ALint buffer_count;
	ALuint* buffers;

	// Stop the OpenAL source from playing
	alSourceStop(stream->source);
	sound_GetError();

	// Retrieve the amount of buffers which were processed
	alGetSourcei(stream->source, AL_BUFFERS_PROCESSED, &buffer_count);
	sound_GetError();

	// Detach all buffers and retrieve their ID numbers
	buffers = alloca(buffer_count * sizeof(ALuint));
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
	AUDIO_STREAM *stream = active_streams, *previous = NULL;

	while (stream != NULL)
	{
		// Attempt to update the current stream, if we find that impossible,
		// destroy it instead.
		if (!sound_UpdateStream(stream))
		{
			// First remove our current stream from the linked list
			if (previous)
			{
				// Make the previous item skip over the current to the next item
				previous->next = stream->next;
			}
			else
			{
				// Apparently this is the first item in the list, so make the
				// next item the list-head.
				active_streams = stream->next;
			}

			// Now actually destroy the current stream
			sound_DestroyStream(stream);

			// Make sure the current stream pointer is intact again
			stream = (previous != NULL) ? previous->next : active_streams;

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
void sound_StopSample(AUDIO_SAMPLE* psSample)
{
#ifndef WZ_NOSOUND
	if (psSample->iSample == (ALuint)SAMPLE_NOT_ALLOCATED)
	{
		debug(LOG_SOUND, "sound_StopSample: sample number (%u) out of range, we probably have run out of available OpenAL sources", psSample->iSample);
		return;
	}

	// Tell OpenAL to stop playing the given sample
	alSourceStop(psSample->iSample);
	sound_GetError();
#endif
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_SetPlayerPos( SDWORD iX, SDWORD iY, SDWORD iZ )
{
#ifndef WZ_NOSOUND
	alListener3f( AL_POSITION, iX, iY, iZ );
	sound_GetError();
#endif
}

//
// =======================================================================================================================
// =======================================================================================================================
//
/** sets player's sound orientation
 * \param iX pitch in degree (current function implementation ignores this)
 * \param iY roll in degree (current function implementation ignores this)
 * \param iZ yaw in degree
 */
void sound_SetPlayerOrientation( SDWORD iX, SDWORD iY, SDWORD iZ )
{
#ifndef WZ_NOSOUND
	const float yaw = deg2radf(iZ);
	const float ori[6] = {-sinf( yaw ), cosf( yaw ), 0.0f, 0.0f, 0.0f, 1.0f};

	alListenerfv( AL_ORIENTATION, ori );
	sound_GetError();
#endif
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_SetObjectPosition(AUDIO_SAMPLE *psSample)
{
#ifndef WZ_NOSOUND
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// coordinates
	float	listenerX, listenerY, listenerZ, dX, dY, dZ;

	// calculation results
	float	distance, gain;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// compute distance
	alGetListener3f( AL_POSITION, &listenerX, &listenerY, &listenerZ );
	sound_GetError();
	dX = psSample->x  - listenerX; // distances on all axis
	dY = psSample->y - listenerY;
	dZ = psSample->z - listenerZ;
	distance = sqrtf(dX * dX + dY * dY + dZ * dZ); // Pythagorean theorem

	// compute gain
	gain = (1.0 - (distance * ATTENUATION_FACTOR)) * psSample->fVol * sfx3d_volume;
	if (gain > 1.0)
	{
		gain = 1.0;
	}
	if ( gain < 0.0 )
	{
		gain = 0.0;
	}
	alSourcef( psSample->iSample, AL_GAIN, gain );

	// the alSource3i variant would be better, if it wouldn't provide linker errors however
	alSource3f( psSample->iSample, AL_POSITION, (float)psSample->x,(float)psSample->x,(float)psSample->x );
	sound_GetError();
#endif
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_PauseSample( AUDIO_SAMPLE *psSample )
{
#ifndef WZ_NOSOUND
	alSourcePause( psSample->iSample );
	sound_GetError();
#endif
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_ResumeSample( AUDIO_SAMPLE *psSample )
{
#ifndef WZ_NOSOUND
	alSourcePlay( psSample->iSample );
	sound_GetError();
#endif
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_PauseAll( void )
{
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_ResumeAll( void )
{
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_StopAll( void )
{
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_SampleIsFinished( AUDIO_SAMPLE *psSample )
{
#ifndef WZ_NOSOUND
	//~~~~~~~~~~
	ALenum	state;
	//~~~~~~~~~~

	alGetSourcei( psSample->iSample, AL_SOURCE_STATE, &state );
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
#endif
	return true;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//

float sound_GetUIVolume()
{
#ifndef WZ_NOSOUND
	return sfx_volume;
#else
	return 0;
#endif
}

void sound_SetUIVolume(float volume)
{
#ifndef WZ_NOSOUND
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
#endif
}

float sound_GetEffectsVolume()
{
#ifndef WZ_NOSOUND
	return sfx3d_volume;
#else
	return 0;
#endif
}

void sound_SetEffectsVolume(float volume)
{
#ifndef WZ_NOSOUND
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
#endif
}
