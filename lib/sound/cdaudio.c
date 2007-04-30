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

#ifdef WZ_OS_MAC
#include <OpenAL/al.h>
#else
#include <AL/al.h>
#endif

#ifndef WZ_NOOGG
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>
#endif

#include "audio.h"
#include "cdaudio.h"
#include "mixer.h"
#include "playlist.h"

extern BOOL		openal_initialized;

#define NB_BUFFERS  16
#define BUFFER_SIZE (16 * 1024)

static BOOL		music_initialized;

static PHYSFS_file*		music_file = NULL;

enum {	WZ_NONE,
	WZ_OGG }	music_file_format;

#ifndef WZ_NOOGG
static OggVorbis_File	ogg_stream;
static vorbis_info*	ogg_info;

static size_t wz_ogg_read( void *ptr, size_t size, size_t nmemb, void *datasource );
static int wz_ogg_seek( void *datasource, ogg_int64_t offset, int whence );
static int wz_ogg_close( void *datasource );

static ov_callbacks		wz_ogg_callbacks = { wz_ogg_read, wz_ogg_seek, wz_ogg_close, NULL };
#endif

static ALfloat		music_volume = 0.5;

static char		music_data[BUFFER_SIZE];

static unsigned int	music_track = 0;
static ALuint 		music_buffers[NB_BUFFERS];
static ALuint		music_source;
static ALenum		music_format;
static unsigned int	music_rate;


static inline unsigned int numProcessedBuffers()
{
	int count;
	alGetSourcei(music_source, AL_BUFFERS_PROCESSED, &count);

	return count;
}

static inline unsigned int numQueuedBuffers()
{
	int count;
	alGetSourcei(music_source, AL_BUFFERS_QUEUED, &count);

	return count;
}


//*
//
// cdAudio Subclass procedure

size_t wz_ogg_read( void *ptr, size_t size, size_t nmemb, void *datasource )
{
	return (PHYSFS_sint64)PHYSFS_read( (PHYSFS_file*)datasource, ptr, (PHYSFS_uint32)size, (PHYSFS_uint32)nmemb );
}
int wz_ogg_seek( void *datasource, ogg_int64_t offset, int whence )
{
	return -1;
}

int wz_ogg_close( void *datasource )
{
	return PHYSFS_close( (PHYSFS_file*)datasource );
}


//*
// ======================================================================
// ======================================================================
//
BOOL cdAudio_Open( char* user_musicdir )
{
	if (!openal_initialized) {
		return FALSE;
	}

	alGenBuffers(NB_BUFFERS, music_buffers);
	alGenSources(1, &music_source);
	alSourcef (music_source, AL_GAIN, music_volume);
	alSource3f(music_source, AL_POSITION, 0.0, 0.0, 0.0);
	alSource3f(music_source, AL_VELOCITY, 0.0, 0.0, 0.0);
	alSourcef (music_source, AL_ROLLOFF_FACTOR, 0.0);
	alSourcei (music_source, AL_SOURCE_RELATIVE, AL_TRUE);

	PlayList_Init();

	if ( ( user_musicdir == NULL
		|| PlayList_Read(user_musicdir) )
	    && PlayList_Read("music") ) {
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
	alDeleteBuffers(NB_BUFFERS, music_buffers);
	alDeleteSources(1, &music_source);
	PlayList_Quit();
	return TRUE;
}

static BOOL cdAudio_OpenTrack(char* filename) {
	if (!music_initialized) {
		return FALSE;
	}

	if (music_file != NULL) {
		PHYSFS_close(music_file);
	}

	music_file_format = WZ_NONE;

#ifndef WZ_NOOGG
	if (strncasecmp(filename+strlen(filename)-4, ".ogg", 4) == 0)
	{
		music_file = PHYSFS_openRead(filename);

		if (music_file == NULL) {
			debug( LOG_SOUND, "Failed opening %s: %s\n", filename, PHYSFS_getLastError() );
			return FALSE;
		}

		if ( ov_open_callbacks( (void*)music_file, &ogg_stream, NULL, 0, wz_ogg_callbacks ) < 0 ) {
			PHYSFS_close(music_file);
			music_file = NULL;
			return FALSE;
		}

		ogg_info = ov_info(&ogg_stream, -1);

		if (ogg_info->channels == 1) {
			music_format = AL_FORMAT_MONO16;
		} else {
			music_format = AL_FORMAT_STEREO16;
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
		unsigned int bufferCount;

		alSourceStop(music_source);

		for(bufferCount = numQueuedBuffers() + numProcessedBuffers(); bufferCount != 0; --bufferCount)
		{
			ALuint buffer;

			alSourceUnqueueBuffers(music_source, 1, &buffer);
			alDeleteBuffers(1, &buffer);
		}

#ifndef WZ_NOOGG
		ov_clear( &ogg_stream );
#endif // WZ_NOOGG

		PHYSFS_close(music_file);
		music_file = NULL;
		music_track = 0;
	}

	return TRUE;
}

static BOOL cdAudio_FillBuffer(ALuint b) {
	int  size = 0;
	int  section;
	int  result = 0;

	while (size < BUFFER_SIZE) {

#ifndef WZ_NOOGG
		if (music_file_format == WZ_OGG) {
			result = ov_read(&ogg_stream, music_data+size,
					 BUFFER_SIZE-size, 0, 2, 1, &section);
			music_rate = ogg_info->rate;
		}
#endif

		if (result > 0) {
			size += result;
		} else {
			char* filename;

			for (;;) {
				filename = PlayList_NextSong();

				if (filename == NULL) {
					music_track = 0;
					break;
				}
				if (cdAudio_OpenTrack(filename)) {
					debug( LOG_SOUND, "Now playing %s\n", filename );
					break;
				} else {
					return FALSE;	// break out to avoid infinite loops
				}
			}
		}
	}

	if (size == 0) {
		return FALSE;
	}

	alBufferData(b, music_format, music_data, size, music_rate);

	return TRUE;
}

//*
// ======================================================================
// ======================================================================
//
BOOL cdAudio_PlayTrack( SDWORD iTrack )
{
	unsigned int i;

	cdAudio_CloseTrack();

	PlayList_SetTrack(iTrack);

	{
		char* filename = PlayList_CurrentSong();

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

	for (i = 0; i < NB_BUFFERS; ++i) {
		if (!cdAudio_FillBuffer(music_buffers[i])) {
			return FALSE;
		}
	}

	alSourceQueueBuffers(music_source, NB_BUFFERS, music_buffers);
	alSourcePlay(music_source);

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
}

//*
// ======================================================================
// ======================================================================
//
SDWORD mixer_GetCDVolume( void )
{
	return (SDWORD)(100*music_volume);
}

//*
// ======================================================================
// ======================================================================
//
void mixer_SetCDVolume( SDWORD iVol )
{
	music_volume = 0.01*iVol;

	if (music_volume < 0.0) {
		music_volume = 0.0;
	} else if (music_volume > 1.0) {
		music_volume = 1.0;
	}
	alSourcef (music_source, AL_GAIN, music_volume);
}
