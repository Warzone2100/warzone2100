#ifdef WZ_CDA
#include <SDL/SDL.h>
#else
#include <AL/al.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>
#endif
#include "frame.h"
#include "audio.h"
#include "cdaudio.h"

#ifdef WZ_CDA
SDL_CD	*cdAudio_dev;
#else
#define NB_BUFFERS  4
#define BUFFER_SIZE (8192)

FILE*		music_file = NULL;

OggVorbis_File	music_ogg_stream;
vorbis_info*    music_vorbis_info;

ALfloat		music_volume = 0.5;

char		music_data[BUFFER_SIZE];

unsigned int	music_track = 0;
ALuint 		music_buffers[NB_BUFFERS];
ALuint		music_source;
ALenum		music_format;
#endif

//*
//
// cdAudio Subclass procedure

//*
// ======================================================================
// ======================================================================
//
BOOL cdAudio_Open( void )
{
#ifdef WZ_CDA
	if ( !SDL_CDNumDrives() )
	{
		printf( "No CDROM devices available\n" );
		return FALSE;
	}

	cdAudio_dev = SDL_CDOpen( 0 );
	if ( !cdAudio_dev )
	{
		printf( "Couldn't open drive: %s\n", SDL_GetError() );
		return FALSE;
	}

	SDL_CDStatus( cdAudio_dev );

	return TRUE;
#else
	alGenBuffers(NB_BUFFERS, music_buffers);
	alGenSources(1, &music_source);

	alSourcef (music_source, AL_GAIN, music_volume);
	alSource3f(music_source, AL_POSITION, 0.0, 0.0, 0.0);
	alSource3f(music_source, AL_VELOCITY, 0.0, 0.0, 0.0);
	alSourcef (music_source, AL_ROLLOFF_FACTOR, 0.0);
	alSourcei (music_source, AL_SOURCE_RELATIVE, AL_TRUE);

	return TRUE;
#endif
}

//*
// ======================================================================
// ======================================================================
//
BOOL cdAudio_Close( void )
{
#ifdef WZ_CDA
	if ( cdAudio_dev != NULL )
	{
		SDL_CDClose( cdAudio_dev );
	}

	return TRUE;
#else
	return TRUE;
#endif
}

#ifndef WZ_CDA
BOOL cdAudio_OpenTrack(SDWORD t) {
	char filename[16];

	if (music_file != NULL) {
		fclose(music_file);
	}

	sprintf(filename, "music/track%i.ogg", t);

	music_file = fopen(filename, "rb");

	if (music_file == NULL) {
		return FALSE;
	}

	if (ov_open(music_file, &music_ogg_stream, NULL, 0) < 0) {
		fclose(music_file);
		music_file = NULL;
		return FALSE;
	}
    
	music_vorbis_info = ov_info(&music_ogg_stream, -1);
 
	if (music_vorbis_info->channels == 1) {
		music_format = AL_FORMAT_MONO16;
	} else {
		music_format = AL_FORMAT_STEREO16;
	}

	return TRUE;
}

BOOL cdAudio_CloseTrack() {
	if (music_track != 0) {
		int queued, processed, all;

		alSourceStop(music_source);
		alGetSourcei(music_source, AL_BUFFERS_QUEUED, &queued);
		alGetSourcei(music_source, AL_BUFFERS_PROCESSED, &processed);
		all = queued + processed;

		while (all > 0) {
			ALuint buffer;

			alSourceUnqueueBuffers(music_source, 1, &buffer);
			all--;
		}

		fclose(music_file);
		music_file = NULL;
		music_track = 0;
	}

	return TRUE;
}

BOOL cdAudio_FillBuffer(ALuint b) {
	int  size = 0;
	int  section;
	int  result;

	while (size < BUFFER_SIZE) {
		result = ov_read(&music_ogg_stream, music_data+size, BUFFER_SIZE-size, 0, 2, 1, &section);

		if (result > 0) {
			size += result;
		} else {
			cdAudio_OpenTrack(music_track);
		}
	}

	if (size == 0) {
		return FALSE;
	}

	alBufferData(b, music_format, music_data, size, music_vorbis_info->rate);

	return TRUE;
}
#endif

//*
// ======================================================================
// ======================================================================
//
BOOL cdAudio_PlayTrack( SDWORD iTrack )
{
#ifdef WZ_CDA
	if ( cdAudio_dev != NULL )
	{
		SDL_CDPlayTracks( cdAudio_dev, iTrack - 1, 0, 1, 0 );
	}

	return TRUE;
#else
	unsigned int i;

	cdAudio_CloseTrack();

	if (cdAudio_OpenTrack(iTrack)) {
		music_track = iTrack;
	} else {
		music_track = 0;
		return FALSE;
	}

	for (i = 0; i < NB_BUFFERS; ++i) {
		if (!cdAudio_FillBuffer(music_buffers[i])) {
			return FALSE;
		}
	}

	alSourceQueueBuffers(music_source, NB_BUFFERS, music_buffers);
	alSourcePlay(music_source);

	return TRUE;
#endif
}

//*
// ======================================================================
// ======================================================================
//
BOOL cdAudio_Stop( void )
{
#ifdef WZ_CDA
	if ( cdAudio_dev != NULL )
	{
		SDL_CDStop( cdAudio_dev );
	}

	return TRUE;
#else
	cdAudio_CloseTrack();
	return TRUE;
#endif
}

//*
// ======================================================================
// ======================================================================
//
BOOL cdAudio_Pause( void )
{
#ifdef WZ_CDA
	if ( cdAudio_dev != NULL )
	{
		SDL_CDPause( cdAudio_dev );
	}

	return TRUE;
#else
	return TRUE;
#endif
}

//*
// ======================================================================
// ======================================================================
//
BOOL cdAudio_Resume( void )
{
#ifdef WZ_CDA
	if ( cdAudio_dev != NULL )
	{
		SDL_CDResume( cdAudio_dev );
	}

	return TRUE;
#else
	return TRUE;
#endif
}

//*
// ======================================================================
// ======================================================================
//
void cdAudio_SetVolume( SDWORD iVol )
{
#ifdef WZ_CDA
#else
	//alSourcef(music_source, AL_GAIN, 0.01*iVol);
#endif
}

//*
// ======================================================================
// ======================================================================
//
void cdAudio_Update( void )
{
#ifdef WZ_CDA
#else
	if (music_track != 0) {
		int processed = 0;

		alGetSourcei(music_source, AL_BUFFERS_PROCESSED, &processed);

		while (processed > 0) {
			ALuint buffer;

			alSourceUnqueueBuffers(music_source, 1, &buffer);
			cdAudio_FillBuffer(buffer);
			alSourceQueueBuffers(music_source, 1, &buffer);
			processed--;
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
SDWORD mixer_GetCDVolume( void )
{
	return 100*music_volume;
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


