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
#include <string.h>
#include <physfs.h>

#include "lib/framework/frame.h"

#ifndef WZ_NOSOUND
# ifdef WZ_OS_MAC
#  include <OpenAL/al.h>
# else
#  include <AL/al.h>
# endif
#endif

#include "audio.h"
#include "cdaudio.h"
#include "mixer.h"
#include "playlist.h"
#include "oggvorbis.h"

extern BOOL		openal_initialized;

#define NB_BUFFERS  16
static const size_t bufferSize = 16 * 1024;

static BOOL		music_initialized;

static PHYSFS_file*		music_file = NULL;

enum {	WZ_NONE,
	WZ_OGG }	music_file_format;

static unsigned int	music_track = 0;

#ifndef WZ_NOSOUND
static ALfloat		music_volume = 0.5;
#endif

#ifndef WZ_NOSOUND
static ALuint 		music_buffers[NB_BUFFERS];
static ALuint		music_source;
#endif

struct OggVorbisDecoderState* decoder = NULL;

static inline unsigned int numProcessedBuffers(void)
{
#ifndef WZ_NOSOUND
	int count;
	alGetSourcei(music_source, AL_BUFFERS_PROCESSED, &count);

	return count;
#else
	return 0;
#endif
}

static inline unsigned int numQueuedBuffers(void)
{
#ifndef WZ_NOSOUND
	int count;
	alGetSourcei(music_source, AL_BUFFERS_QUEUED, &count);

	return count;
#else
	return 0;
#endif
}

//*
// ======================================================================
// ======================================================================
//
BOOL cdAudio_Open( const char* user_musicdir )
{
	if (!openal_initialized) {
		return FALSE;
	}

#ifndef WZ_NOSOUND
	alGenBuffers(NB_BUFFERS, music_buffers);
	alGenSources(1, &music_source);
	alSourcef (music_source, AL_GAIN, music_volume);
	alSource3f(music_source, AL_POSITION, 0.0, 0.0, 0.0);
	alSource3f(music_source, AL_VELOCITY, 0.0, 0.0, 0.0);
	alSourcef (music_source, AL_ROLLOFF_FACTOR, 0.0);
	alSourcei (music_source, AL_SOURCE_RELATIVE, AL_TRUE);
#endif

	PlayList_Init();

	if ((user_musicdir == NULL
	  || !PlayList_Read(user_musicdir))
	 && !PlayList_Read("music"))
	{
		return FALSE;
	}

	music_initialized = TRUE;

	return TRUE;
}

//*
// ======================================================================
// ======================================================================
//
BOOL cdAudio_Close( void )
{
#ifndef WZ_NOSOUND
	alDeleteBuffers(NB_BUFFERS, music_buffers);
	alDeleteSources(1, &music_source);
#endif
	PlayList_Quit();
	return TRUE;
}

static BOOL cdAudio_OpenTrack(const char* filename) {
	if (!music_initialized) {
		return FALSE;
	}

	if (music_file != NULL) {
		PHYSFS_close(music_file);
	}

	music_file_format = WZ_NONE;

#ifndef WZ_NOSOUND
	if (strncasecmp(filename+strlen(filename)-4, ".ogg", 4) == 0)
	{
		music_file = PHYSFS_openRead(filename);

		if (music_file == NULL) {
			debug( LOG_SOUND, "Failed opening %s: %s\n", filename, PHYSFS_getLastError() );
			return FALSE;
		}

		decoder = sound_CreateOggVorbisDecoder(music_file, FALSE);
		if (decoder == NULL)
		{
			debug(LOG_ERROR, "cdAudio_OpenTrack: Failed to construct a decoder object\n");
			PHYSFS_close(music_file);
			music_file = NULL;
			return FALSE;
		}

		music_file_format = WZ_OGG;
		return TRUE;
	}
#endif

	return FALSE; // unhandled
}

static BOOL cdAudio_CloseTrack(void) {
	if (music_track != 0)
	{
#ifndef WZ_NOSOUND
		unsigned int bufferCount;

		alSourceStop(music_source);

		for(bufferCount = numQueuedBuffers() + numProcessedBuffers(); bufferCount != 0; --bufferCount)
		{
			ALuint buffer;

			alSourceUnqueueBuffers(music_source, 1, &buffer);
			alDeleteBuffers(1, &buffer);
		}
#endif

		sound_DestroyOggVorbisDecoder(decoder);
		decoder = NULL;

		PHYSFS_close(music_file);
		music_file = NULL;
		music_track = 0;
	}

	return TRUE;
}

#ifndef WZ_NOSOUND
static BOOL cdAudio_FillBuffer(ALuint buffer)
{
	ALenum format;
	soundDataBuffer* pcm;

	if (decoder == NULL)
	{
		return FALSE;
	}

	pcm = sound_DecodeOggVorbis(decoder, bufferSize);
	if (pcm == NULL)
	{
		return FALSE;
	}

	if (pcm->size < bufferSize)
	{
		const char* filename = PlayList_NextSong();
		cdAudio_CloseTrack();

		if (filename == NULL)
		{
			music_track = 0;
		}
		if (cdAudio_OpenTrack(filename))
		{
			debug( LOG_SOUND, "Now playing %s\n", filename );
		}
	}

	if (pcm->size == 0)
	{
		free(pcm);
		return FALSE;
	}

	// Determine PCM data format
	format = (pcm->channelCount == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

	// fill the OpenAL buffer with the decoded data
	alBufferData(buffer, format, pcm->data, pcm->size, pcm->frequency);

	free(pcm);
	return TRUE;
}
#endif

//*
// ======================================================================
// ======================================================================
//
BOOL cdAudio_PlayTrack( SDWORD iTrack )
{
#ifndef WZ_NOSOUND
	unsigned int i;
#endif

	cdAudio_CloseTrack();

	PlayList_SetTrack(iTrack);

	{
		const char* filename = PlayList_CurrentSong();

		for (;;) {
			if (filename == NULL) {
				music_track = 0;
				return FALSE;
			}
			if (cdAudio_OpenTrack(filename)) {
				music_track = iTrack;
				break;
			} else {
				return FALSE; // break out to avoid infinite loops
			}

			filename = PlayList_NextSong();
		}
	}

#ifndef WZ_NOSOUND
	for (i = 0; i < NB_BUFFERS; ++i) {
		if (!cdAudio_FillBuffer(music_buffers[i])) {
			return FALSE;
		}
	}

	alSourceQueueBuffers(music_source, NB_BUFFERS, music_buffers);
	alSourcePlay(music_source);
#endif

	return TRUE;
}

//*
// ======================================================================
// ======================================================================
//
BOOL cdAudio_Stop( void )
{
	cdAudio_CloseTrack();
	return TRUE;
}

//*
// ======================================================================
// ======================================================================
//
BOOL cdAudio_Pause( void )
{
	return TRUE;
}

//*
// ======================================================================
// ======================================================================
//
BOOL cdAudio_Resume( void )
{
	return TRUE;
}

//*
// ======================================================================
// ======================================================================
//
void cdAudio_Update( void )
{
#ifndef WZ_NOSOUND
	if (music_track != 0 && music_volume != 0.0)
	{
		unsigned int update;

		for (update = numProcessedBuffers(); update != 0; --update)
		{
			ALuint buffer;

			alSourceUnqueueBuffers(music_source, 1, &buffer);
			cdAudio_FillBuffer(buffer);
			alSourceQueueBuffers(music_source, 1, &buffer);
		}

		{
			ALenum state;

			alGetSourcei(music_source, AL_SOURCE_STATE, &state);
			if (state != AL_PLAYING) {
				alSourcePlay(music_source);
			}
		}
	}
#endif
}

//*
// ======================================================================
// ======================================================================
//
float sound_GetMusicVolume()
{
#ifndef WZ_NOSOUND
	return music_volume;
#else
	return 0;
#endif
}

//*
// ======================================================================
// ======================================================================
//
void sound_SetMusicVolume(float volume)
{
#ifndef WZ_NOSOUND
	music_volume = volume;

    // Keep volume in the range of 0.0 - 1.0
	if (music_volume < 0.0)
	{
		music_volume = 0.0;
	}
	else if (music_volume > 1.0)
	{
		music_volume = 1.0;
	}

	// Set the volume on the OpenAL source playing our music stream
	alSourcef(music_source, AL_GAIN, music_volume);
#endif
}
