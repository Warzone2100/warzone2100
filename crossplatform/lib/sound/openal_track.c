//*
//
// Sound library-specific functions
//*
//
#include <AL/al.h>
#include <AL/alc.h>
#include <ALut/alut.h>
#include "frame.h"
#include "tracklib.h"
#include "audio.h"
#define ATTENUATION_FACTOR	0.0003
#ifndef M_PI
	#define M_PI	3.1415926535897932385
#endif // win32 doesn't define that...
int current_queue_sample = -1;
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

BOOL openal_initialized = FALSE;

BOOL		cdAudio_Update( void );

static void PrintOpenALVersion()
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
	int contextAttributes[] = { 0 };
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	device = alcOpenDevice(0);
	if(device == 0) {
		PrintOpenALVersion();
		debug(LOG_ERROR, "Couldn't open audio device.");
		return FALSE;
	}

	context = alcCreateContext(device, contextAttributes);
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
	if(context != 0) {
		alcMakeContextCurrent(NULL);		//this should work now -Q
		alcDestroyContext(context); // this gives a long delay on some impl.
		context = 0;
	}
	if(device != 0) {
		alcCloseDevice(device);
		device = 0;
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
			ALenum	state;
			//~~~~~~~~~~

			alGetSourcei( i->curr->iSample, AL_SOURCE_STATE, &state );
			switch ( state )
			{
			case AL_PLAYING:
			case AL_LOOPING:
				//
				// * sound_SetObjectPosition(i->curr->iSample, i->curr->x, i->curr->y, i->curr->z);
				//
				tmp = &( i->next );
				break;

			default:
				sound_FinishedCallback( i->curr );
				alDeleteSources( 1, &(i->curr->iSample) );
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
		printf("Error while processing audio context: %s\n",
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
			alDeleteSources( 1, &current_queue_sample );
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
	// save data pointer in track
	psTrack->pMem = (void *) buffer;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_ReadTrackFromFile( TRACK *psTrack, signed char szFileName[] )
{
	//~~~~~~~~~~~~~~~~~~~
	ALuint		buffer;
	ALsizei		size, freq;
	ALenum		format;
	ALvoid		*data;
	ALboolean	loop;
	//~~~~~~~~~~~~~~~~~~~

	alGenBuffers( 1, &buffer );
	alutLoadWAVFile( szFileName, &format, &data, &size, &freq, &loop );
	alBufferData( buffer, format, data, size, freq );
	alutUnloadWAV( format, data, size, freq );
	sound_SaveTrackData( psTrack, buffer );
	return TRUE;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_ReadTrackFromBuffer( TRACK *psTrack, void *pBuffer, UDWORD udwSize )
{
	//~~~~~~~~~~~~~~~~~~~
	ALuint		buffer;
	ALsizei		size, freq;
	ALenum		format;
	ALvoid		*data;
	ALboolean	loop;
	//~~~~~~~~~~~~~~~~~~~

	alGenBuffers( 1, &buffer );
	alutLoadWAVMemory( pBuffer, &format, &data, &size, &freq, &loop );
	alBufferData( buffer, format, data, size, freq );
	alutUnloadWAV( format, data, size, freq );
	sound_SaveTrackData( psTrack, buffer );
	return TRUE;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_FreeTrack( TRACK *psTrack )
{
	ALuint buffer = (ALuint)(psTrack->pMem);

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
void sound_AddActiveSample( AUDIO_SAMPLE *psSample )
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
	alSourcei( psSample->iSample, AL_BUFFER, (ALuint) (psTrack->pMem) );
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
	alSourcei( psSample->iSample, AL_BUFFER, (ALuint) (psTrack->pMem) );
	sound_SetupChannel( psSample, &iLoops );
	alSourcei( psSample->iSample, AL_LOOPING, (iLoops) ? AL_TRUE : AL_FALSE );
	alSourcePlay( psSample->iSample );
	return TRUE;
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
BOOL sound_PlayStream( AUDIO_SAMPLE *psSample, char szFileName[], SDWORD iVol )
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
	alDeleteSources( 1, &iSample );
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
void sound_SetSamplePan( AUDIO_SAMPLE *psSample, int iPan )
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
		alDeleteSources( 1, &(psSample->iSample) );
		return TRUE;
	}
}

//*
// =======================================================================================================================
// =======================================================================================================================
//
LPDIRECTSOUND sound_GetDirectSoundObj( void )
{
	return NULL;
}

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



//*
//
