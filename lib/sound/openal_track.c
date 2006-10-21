//*
//
// Sound library-specific functions
//*
//

// this has to be first
#include "lib/framework/frame.h"

#ifdef WZ_OPENAL_MAC_H
#include <openal/al.h>
#include <openal/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#ifndef WZ_NOOGG
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>
#include <vorbis/vorbisfile.h>
#endif

#include <physfs.h>

#include "tracklib.h"
#include "audio.h"
#define ATTENUATION_FACTOR	0.0003f
#ifndef M_PI
	#define M_PI	3.1415926535897932385
#endif // win32 doesn't define that...

#ifdef __BIG_ENDIAN__
#define OGG_ENDIAN 1
#else
#define OGG_ENDIAN 0
#endif

ALuint current_queue_sample = -1;

typedef struct	SAMPLE_LIST
{
	struct AUDIO_SAMPLE *curr;
	struct SAMPLE_LIST	*next;
} SAMPLE_LIST;

static SAMPLE_LIST *active_samples = NULL;

static ALfloat		listenerPos[3] = { 0.0, 0.0, 0.0 };

static ALfloat		sfx_volume = 1.0;
static ALfloat		sfx3d_volume = 1.0;

static ALCdevice* device = 0;
static ALCcontext* context = 0;

static ALvoid	*data = NULL; // Needed for ReadTrackFromBuffer, must be global, so it can be free'd on shutdown

BOOL openal_initialized = FALSE;

BOOL		cdAudio_Update( void );

static void PrintOpenALVersion(void)
{
	debug(LOG_ERROR, "OpenAL Vendor: %s\n"
		   "OpenAL Version: %s\n"
		   "OpenAL Renderer: %s\n"
		   "OpenAL Extensions: %s\n",
		   alGetString(AL_VENDOR), alGetString(AL_VERSION),
		   alGetString(AL_RENDERER), alGetString(AL_EXTENSIONS));
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_InitLibrary( void )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	int err=0;
	ALfloat listenerVel[3] = { 0.0, 0.0, 0.0 };
	ALfloat listenerOri[6] = { 0.0, 0.0, 1.0, 0.0, 1.0, 0.0 };
//	int contextAttributes[] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	device = alcOpenDevice(0);
	if(device == 0) {
		PrintOpenALVersion();
		debug(LOG_ERROR, "Couldn't open audio device.");
		return FALSE;
	}

	context = alcCreateContext(device, NULL);		//NULL was contextAttributes
	alcMakeContextCurrent(context);

	err = alcGetError(device);
	if (err != ALC_NO_ERROR) {
		PrintOpenALVersion();
		debug(LOG_ERROR, "Couldn't initialize audio context: %s",
				alcGetString(device, err));
		return FALSE;
	}
	err = alGetError();
	if (err != AL_NO_ERROR) {
		PrintOpenALVersion();
		debug(LOG_ERROR, "Audio error after init: %s", alGetString(err));
		return FALSE;
	}

	openal_initialized = TRUE;

	// Clear Error Codes
	alGetError();
	alcGetError(device);

	// Check what version of Open AL we are using
	debug(LOG_SOUND, "OpenAL Version : %s",alGetString(AL_VERSION));
	debug(LOG_SOUND, "OpenAL Renderer : %s",alGetString(AL_RENDERER));
	debug(LOG_SOUND, "OpenAL Extensions : %s",alGetString(AL_EXTENSIONS));


	alListenerfv( AL_POSITION, listenerPos );
	alListenerfv( AL_VELOCITY, listenerVel );
	alListenerfv( AL_ORIENTATION, listenerOri );
	alDistanceModel( AL_NONE );
	return TRUE;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_ShutdownLibrary( void )
{
	SAMPLE_LIST * aSample = active_samples, * tmpSample = NULL;

	debug(LOG_SOUND, "sound_ShutdownLibrary: starting shutdown");
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

	free( data );

	while( aSample )
	{
		tmpSample = aSample->next;
		free( aSample );
		aSample = tmpSample;
	}
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_Update( void )
{
	int err=0;
//	{			//  <=== whats up with this ??
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		SAMPLE_LIST **tmp = &active_samples;
		SAMPLE_LIST *i;
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

		for ( tmp = &active_samples, i = *tmp; i != NULL; i = *tmp )
		{
			//~~~~~~~~~~
			ALenum	state = AL_STOPPED;
			//~~~~~~~~~~

			alGetSourcei( i->curr->iSample, AL_SOURCE_STATE, &state );
			switch ( state )
			{
			case AL_PLAYING:
			case AL_PAUSED:
				//
				// * sound_SetObjectPosition(i->curr->iSample, i->curr->x, i->curr->y, i->curr->z);
				//
				tmp = &( i->next );
				break;

			default:
				sound_FinishedCallback( i->curr );
				if (i->curr->iSample != AL_INVALID) {
					alDeleteSources( 1, &(i->curr->iSample) );
					i->curr->iSample = AL_INVALID;
				}
				*tmp = i->next;
				free( i );
				break;
			}
		}
//	}//  <=== whats up with this You trying to make those local only ??

	cdAudio_Update();
	alcProcessContext(context);
	err = alcGetError(device);
	if(err != ALC_NO_ERROR) {
		debug(LOG_ERROR, "Error while processing audio context: %s",
				alGetString(err));
	}
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_QueueSamplePlaying( void )
{
	if ( current_queue_sample == -1 )
	{
		return FALSE;
	}
	else
	{
		//~~~~~~~~~~
		ALenum	state;
		//~~~~~~~~~~

		alGetSourcei( current_queue_sample, AL_SOURCE_STATE, &state );
		if ( state == AL_PLAYING )
		{
			return TRUE;
		}
		else
		{
			if (current_queue_sample != AL_INVALID) {
				alDeleteSources( 1, &current_queue_sample );
				current_queue_sample = AL_INVALID;
			}
			current_queue_sample = -1;
			return FALSE;
		}
	}
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
static void sound_SaveTrackData( TRACK *psTrack, ALuint buffer )
{
	// save buffer name in track
	psTrack->iBufferName = buffer;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//

typedef struct {
	void* buffer;
	unsigned int size;
	unsigned int pos;
} ov_buffer_t;

size_t ovbuf_read(void *ptr, size_t size, size_t nmemb, void *datasource) {
	ov_buffer_t* ovbuf = (ov_buffer_t*)datasource;
	unsigned int read_size = size*nmemb;

	if (ovbuf->pos + read_size > ovbuf->size) {
		read_size = ovbuf->size - ovbuf->pos;
	}
	memcpy((char *)ptr,(char *) ovbuf->buffer+ovbuf->pos, read_size);
	ovbuf->pos += read_size;

	return read_size;
}

int ovbuf_seek(void *datasource, ogg_int64_t offset, int whence) {
	ov_buffer_t* ovbuf = (ov_buffer_t*)datasource;
	int new_pos = 0;

	switch (whence) {
		case SEEK_SET:
			new_pos = offset;
			break;
		case SEEK_CUR:
			new_pos = ovbuf->pos+offset;
			break;
		case SEEK_END:
			new_pos = ovbuf->size-offset-1;
			break;
	}

	if (new_pos >= 0 && new_pos < ovbuf->size) {
		ovbuf->pos = new_pos;
		return new_pos;
	} else {
		return -1;
	}
}

int ovbuf_close(void *datasource) {
	return 0;
}

long ovbuf_tell(void *datasource) {
	ov_buffer_t* ovbuf = (ov_buffer_t*)datasource;

	return ovbuf->pos;
}

static ov_callbacks ovbuf_callbacks = {
	ovbuf_read,
	ovbuf_seek,
	ovbuf_close,
	ovbuf_tell
};

BOOL sound_ReadTrackFromBuffer( TRACK *psTrack, void *pBuffer, UDWORD udwSize )
{
	ov_buffer_t	ovbuf;
	OggVorbis_File	ogg_stream;
	vorbis_info*	ogg_info;
	ALuint		buffer;
	ALsizei		size=0, freq;
	ALenum		format;
	static ALuint	data_size;
	int		result, section;

	ovbuf.buffer = pBuffer;
	ovbuf.size = udwSize;
	ovbuf.pos = 0;

	if (ov_open_callbacks(&ovbuf, &ogg_stream, NULL, 0, ovbuf_callbacks) < 0) {
		return FALSE;
	}

	ogg_info = ov_info(&ogg_stream, -1);

	if (ogg_info->channels == 1) {
		format = AL_FORMAT_MONO16;
	} else {
		format = AL_FORMAT_STEREO16;
	}
	freq = ogg_info->rate;

	if (data == NULL) {
		data_size = 8192;
		data = malloc(data_size);
	}

	result = ov_read(&ogg_stream, (char *)data+size, data_size-size, OGG_ENDIAN, 2, 1, &section);
	while( result != 0 ) {
		size += result;
		if (size == data_size) {
			data_size *= 2;
			data = realloc(data, data_size);
		}
		result = ov_read(&ogg_stream, (char *)data+size, data_size-size, OGG_ENDIAN, 2, 1, &section);
	}

	ov_clear(&ogg_stream);
	alGenBuffers(1, &buffer);
	alBufferData(buffer, format, data, size, freq);
	sound_SaveTrackData(psTrack, buffer);
	return TRUE;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_ReadTrackFromFile(TRACK *psTrack, char szFileName[])
{
	PHYSFS_file * f = PHYSFS_openRead(szFileName);
	static char* buffer = NULL;
	static unsigned int buffer_size = 0;
	unsigned int size;
	char seekbuf[1];

	if (f == NULL) return FALSE;

	// FIXME Ugly hack because PhysFS doesn't support reporting the filesize, nor seeking to the end
	while( PHYSFS_read( f, seekbuf, 1, 1 ) );
	size = PHYSFS_tell( f );
	PHYSFS_seek( f, 0 );

	if (size > buffer_size) {
		if (buffer != NULL) free(buffer);
		buffer_size = size*2;
		buffer = (char*)malloc(buffer_size);
	}

	PHYSFS_read(f, buffer, 1, size);

	return sound_ReadTrackFromBuffer(psTrack, buffer, size);
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_FreeTrack( TRACK *psTrack )
{
	ALuint buffer = psTrack->iBufferName;

	alDeleteBuffers( 1, &buffer );
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
int sound_GetMaxVolume( void )
{
	return 32767;		// Why this value? -Q
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
static void sound_AddActiveSample( AUDIO_SAMPLE *psSample )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	SAMPLE_LIST *tmp = (SAMPLE_LIST *) malloc( sizeof(SAMPLE_LIST) );
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	tmp->curr = psSample;
	tmp->next = active_samples;
	active_samples = tmp;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
static void sound_SetupChannel( AUDIO_SAMPLE *psSample, SDWORD *piLoops )
{
	if ( sound_TrackLooped(psSample->iTrack) == TRUE )
	{
		*piLoops = -1;
	}
	else
	{
		*piLoops = 0;
	}

	sound_AddActiveSample( psSample );
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_Play2DSample( TRACK *psTrack, AUDIO_SAMPLE *psSample, BOOL bQueued )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	SDWORD	iLoops;
	ALfloat zero[3] = { 0.0, 0.0, 0.0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (sfx_volume == 0.0) {
		return FALSE;
	}

	alGenSources( 1, &(psSample->iSample) );
	alSourcef( psSample->iSample, AL_PITCH, 1.0f );
	alSourcef( psSample->iSample, AL_GAIN, sfx_volume );
	alSourcefv( psSample->iSample, AL_POSITION, zero );
	alSourcefv( psSample->iSample, AL_VELOCITY, zero );
	alSourcei( psSample->iSample, AL_BUFFER, psTrack->iBufferName );
	alSourcei( psSample->iSample, AL_SOURCE_RELATIVE, AL_TRUE );
	sound_SetupChannel( psSample, &iLoops );
	alSourcei( psSample->iSample, AL_LOOPING, (iLoops) ? AL_TRUE : AL_FALSE );
	alSourcePlay( psSample->iSample );
	if ( bQueued )
	{
		current_queue_sample = psSample->iSample;
	}
	else if ( current_queue_sample == psSample->iSample )
	{
		current_queue_sample = -1;
	}

	return TRUE;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_Play3DSample( TRACK *psTrack, AUDIO_SAMPLE *psSample )
{
	SDWORD	iLoops;
	ALfloat zero[3] = { 0.0, 0.0, 0.0 };

	if (sfx3d_volume == 0.0) {
		return FALSE;
	}

	alGenSources( 1, &(psSample->iSample) );
	alSourcef( psSample->iSample, AL_PITCH, 1.0 );
	alSourcef( psSample->iSample, AL_GAIN, sfx3d_volume );
	sound_SetObjectPosition( psSample->iSample, psSample->x, psSample->y, psSample->z );
	alSourcefv( psSample->iSample, AL_VELOCITY, zero );
	alSourcei( psSample->iSample, AL_BUFFER, psTrack->iBufferName );
	sound_SetupChannel( psSample, &iLoops );
	alSourcei( psSample->iSample, AL_LOOPING, (iLoops) ? AL_TRUE : AL_FALSE );
	alSourcePlay( psSample->iSample );
	return TRUE;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_PlayStream( AUDIO_SAMPLE *psSample, const char szFileName[], SDWORD iVol )
{
	return TRUE;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_StopSample( UDWORD iSample )
{
	alSourceStop( iSample );
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_SetSamplePan( AUDIO_SAMPLE *psSample, SDWORD iPan )
{
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_SetSampleVolAll( int iVol )
{
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_SetPlayerPos( SDWORD iX, SDWORD iY, SDWORD iZ )
{
	listenerPos[0] = iX;
	listenerPos[1] = iY;
	listenerPos[2] = iZ;
	alListenerfv( AL_POSITION, listenerPos );
}

//
// =======================================================================================================================
// =======================================================================================================================
//
void sound_SetPlayerOrientation( SDWORD iX, SDWORD iY, SDWORD iZ )
{
	//~~~~~~~~~~~
	float	ori[6];
	//~~~~~~~~~~~

	ori[0] = -sin( ((float) iZ) * M_PI / 180.0f );
	ori[1] = cos( ((float) iZ) * M_PI / 180.0f );
	ori[2] = 0;
	ori[3] = 0;
	ori[4] = 0;
	ori[5] = 1;
	alListenerfv( AL_ORIENTATION, ori );
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_SetObjectPosition( SDWORD iSample, SDWORD iX, SDWORD iY, SDWORD iZ )
{
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	float	pos[3];
	float	vx = iX - listenerPos[0];
	float	vy = iY - listenerPos[1];
	float	vz = iZ - listenerPos[2];
	float	l2 = vx * vx + vy * vy + vz * vz;
	float	gain = 1 - sqrt( l2 ) * ATTENUATION_FACTOR;
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if ( gain < 0.0 )
	{
		gain = 0.0;
	}

	alSourcef( iSample, AL_GAIN, gain * sfx3d_volume );

	pos[0] = iX;
	pos[1] = iY;
	pos[2] = iZ;
	alSourcefv( iSample, AL_POSITION, pos );
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_PauseSample( AUDIO_SAMPLE *psSample )
{
	alSourcePause( psSample->iSample );
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_ResumeSample( AUDIO_SAMPLE *psSample )
{
	alSourcePlay( psSample->iSample );
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
	//~~~~~~~~~~
	ALenum	state;
	//~~~~~~~~~~

	alGetSourcei( psSample->iSample, AL_SOURCE_STATE, &state );
	if ( state == AL_PLAYING )
	{
		return FALSE;
	}
	else
	{
				if (psSample->iSample != AL_INVALID) {
					alDeleteSources( 1, &(psSample->iSample) );
					psSample->iSample = AL_INVALID;
				}
		return TRUE;
	}
}

//*
// =======================================================================================================================
// =======================================================================================================================
//

SDWORD mixer_GetWavVolume( void )
{
	return 100*sfx_volume;
}

void mixer_SetWavVolume( SDWORD iVol )
{
	sfx_volume = iVol * 0.01;

	if (sfx_volume < 0.0) {
		sfx_volume = 0.0;
	} else if (sfx_volume > 1.0) {
		sfx_volume = 1.0;
	}
}

SDWORD mixer_Get3dWavVolume( void )
{
	return 100*sfx3d_volume;
}

void mixer_Set3dWavVolume( SDWORD iVol )
{
	sfx3d_volume = iVol * 0.01;

	if (sfx3d_volume < 0.0) {
		sfx3d_volume = 0.0;
	} else if (sfx3d_volume > 1.0) {
		sfx3d_volume = 1.0;
	}
}
