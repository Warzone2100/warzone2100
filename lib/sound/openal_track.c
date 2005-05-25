/***************************************************************************/
/*
 * QSound library-specific functions
 */
/***************************************************************************/

#include <AL/al.h>
#include <AL/alut.h>

#include "frame.h"
#include "tracklib.h"
#include "audio.h"

#define ATTENUATION_FACTOR	0.0003

int current_queue_sample = -1;

typedef struct SAMPLE_LIST {
	struct AUDIO_SAMPLE	*curr;
	struct SAMPLE_LIST	*next;
} SAMPLE_LIST;

SAMPLE_LIST* active_samples = NULL;

ALfloat listenerPos[3]={0.0,0.0,0.0};
BOOL listenerMoved = FALSE;

BOOL cdAudio_Update();

/***************************************************************************/

BOOL
sound_InitLibrary( void ) {
	int nbargs = 0;
	ALfloat listenerVel[3]={0.0,0.0,0.0};
	ALfloat listenerOri[6]={0.0,0.0,1.0, 0.0,1.0,0.0};

	alutInit(&nbargs, NULL);
	alListenerfv(AL_POSITION,listenerPos);
	alListenerfv(AL_VELOCITY,listenerVel);
	alListenerfv(AL_ORIENTATION,listenerOri);
	alDistanceModel(AL_NONE);

	return TRUE;
}

/***************************************************************************/

void
sound_ShutdownLibrary( void ) {
}

/***************************************************************************/

void
sound_Update( void ) {
	{
		SAMPLE_LIST** tmp = &active_samples;
		SAMPLE_LIST* i;

		for (tmp = &active_samples, i = *tmp; i != NULL; i = *tmp) {
			ALenum state;

			alGetSourcei(i->curr->iSample, AL_SOURCE_STATE, &state);

			switch (state) {
			case AL_PLAYING:
			case AL_LOOPING:
				/*
				sound_SetObjectPosition(i->curr->iSample,
							i->curr->x,
							i->curr->y,
							i->curr->z);
				*/
				tmp = &(i->next);
				break;
			default:
				sound_FinishedCallback( i->curr );
				alDeleteSources(1, &(i->curr->iSample));
				*tmp = i->next;
				free(i);
				break;
			}
		}
	}

	cdAudio_Update();
}

/***************************************************************************/

BOOL
sound_QueueSamplePlaying( void ) {
	if (current_queue_sample == -1) {
        	return FALSE;
	} else {
		ALenum state;

		alGetSourcei(current_queue_sample, AL_SOURCE_STATE, &state);

		if (state == AL_PLAYING) {
			return TRUE;
		} else {
			alDeleteSources(1, &current_queue_sample);
			current_queue_sample = -1;
			return FALSE;
		}
	}
}

/***************************************************************************/

static void
sound_SaveTrackData( TRACK *psTrack, ALuint buffer) {
	/* save data pointer in track */
	psTrack->pMem = (void*) buffer;
}

/***************************************************************************/

BOOL
sound_ReadTrackFromFile( TRACK * psTrack, char szFileName[] ) {
	ALuint buffer;
	ALsizei size,freq;
	ALenum  format;
	ALvoid  *data;
	ALboolean loop;

	alGenBuffers(1, &buffer);
	alutLoadWAVFile(szFileName, &format, &data, &size, &freq, &loop);
	alBufferData(buffer, format, data, size, freq);
	alutUnloadWAV(format, data, size, freq);

	sound_SaveTrackData( psTrack, buffer );

        return TRUE;
}

/***************************************************************************/

BOOL
sound_ReadTrackFromBuffer( TRACK * psTrack, void *pBuffer, UDWORD udwSize ) {
	ALuint buffer;
	ALsizei size,freq;
	ALenum  format;
	ALvoid  *data;
	ALboolean loop;

	alGenBuffers(1, &buffer);
	alutLoadWAVMemory(pBuffer, &format, &data, &size, &freq, &loop);
	alBufferData(buffer, format, data, size, freq);
	alutUnloadWAV(format, data, size, freq);

	sound_SaveTrackData( psTrack, buffer );

        return TRUE;
}

/***************************************************************************/

void
sound_FreeTrack( TRACK * psTrack ) {
	alDeleteBuffers(1, &(psTrack->pMem));
}

/***************************************************************************/

int
sound_GetMaxVolume( void ) {
	return 32767;
}

/***************************************************************************/

void
sound_AddActiveSample( AUDIO_SAMPLE * psSample ) {
	SAMPLE_LIST *tmp = (SAMPLE_LIST*)malloc(sizeof(SAMPLE_LIST));

	tmp->curr = psSample;
	tmp->next = active_samples;

	active_samples = tmp;
}

/***************************************************************************/

static void
sound_SetupChannel( AUDIO_SAMPLE * psSample, SDWORD *piLoops ) {
	if ( sound_TrackLooped( psSample->iTrack ) == TRUE ) {
		*piLoops = -1;
	} else {
		*piLoops = 0;
	}

	sound_AddActiveSample(psSample);
}

/***************************************************************************/

BOOL
sound_Play2DSample( TRACK * psTrack, AUDIO_SAMPLE * psSample, BOOL bQueued ) {
	SDWORD iLoops;
	ALfloat zero[3]={0.0,0.0,0.0};

    	alGenSources(1, &(psSample->iSample));
	alSourcef(psSample->iSample, AL_PITCH, 1.0f);
    	alSourcef(psSample->iSample, AL_GAIN, 1.0f);
    	alSourcefv(psSample->iSample, AL_POSITION, zero);
    	alSourcefv(psSample->iSample, AL_VELOCITY, zero);
    	alSourcei(psSample->iSample, AL_BUFFER, (ALuint)(psTrack->pMem));
	alSourcei(psSample->iSample, AL_SOURCE_RELATIVE, AL_TRUE );
	sound_SetupChannel( psSample, &iLoops );
    	alSourcei(psSample->iSample, AL_LOOPING, (iLoops) ? AL_TRUE : AL_FALSE);
	alSourcePlay(psSample->iSample);

	if (bQueued) {
		current_queue_sample = psSample->iSample;
	} else if (current_queue_sample == psSample->iSample) {
		current_queue_sample = -1;
	}

	return TRUE;
}

/***************************************************************************/

BOOL
sound_Play3DSample( TRACK * psTrack, AUDIO_SAMPLE * psSample ) {
	SDWORD iLoops;
	ALfloat zero[3]={0.0,0.0,0.0};

    	alGenSources(1, &(psSample->iSample));
	alSourcef(psSample->iSample, AL_PITCH, 1.0f);
	sound_SetObjectPosition(psSample->iSample, psSample->x,
				psSample->y, psSample->z);
    	alSourcefv(psSample->iSample, AL_VELOCITY, zero);
    	alSourcei(psSample->iSample, AL_BUFFER, (ALuint)(psTrack->pMem));

	sound_SetupChannel( psSample, &iLoops );
    	alSourcei(psSample->iSample, AL_LOOPING, (iLoops) ? AL_TRUE : AL_FALSE);
	alSourcePlay(psSample->iSample);

	return TRUE;
}

/***************************************************************************/

BOOL
sound_PlayStream( AUDIO_SAMPLE *psSample, char szFileName[], SDWORD iVol ) {
	return TRUE;
}

/***************************************************************************/

void
sound_StopSample( SDWORD iSample ) {
	alSourceStop(iSample);
	alDeleteSources(1, &iSample);
}

/***************************************************************************/

void
sound_SetSamplePan( AUDIO_SAMPLE * psSample, int iPan ) {
}

/***************************************************************************/

void
sound_SetSampleVolAll( int iVol ) {
}

/***************************************************************************/

void
sound_SetPlayerPos( SDWORD iX, SDWORD iY, SDWORD iZ ) {
	listenerPos[0] = iX;
	listenerPos[1] = iY;
	listenerPos[2] = iZ;
	
	alListenerfv(AL_POSITION, listenerPos);
}

void
sound_SetPlayerOrientation( SDWORD iX, SDWORD iY, SDWORD iZ ) {
	float ori[6];

	ori[0] = -sin(((float)iZ)*M_PI/180.0f);
	ori[1] = cos(((float)iZ)*M_PI/180.0f);
	ori[2] = 0;
	ori[3] = 0;
	ori[4] = 0;
	ori[5] = 1;
	
	alListenerfv(AL_ORIENTATION, ori);
}

/***************************************************************************/

void
sound_SetObjectPosition( SDWORD iSample, SDWORD iX, SDWORD iY, SDWORD iZ ) {
	float pos[3];
	float vx = iX - listenerPos[0];
	float vy = iY - listenerPos[1];
	float vz = iZ - listenerPos[2];
	float l2 = vx*vx+vy*vy+vz*vz;
 	float gain = 1-sqrt(l2)*ATTENUATION_FACTOR;

	if (gain < 0.0) {
		gain = 0.0;
	}

   	alSourcef(iSample, AL_GAIN, gain);
   	//alSourcef(iSample, AL_MIN_GAIN, gain);
   	//alSourcef(iSample, AL_MAX_GAIN, gain);

	pos[0] = iX;
	pos[1] = iY;
	pos[2] = iZ;

   	alSourcefv(iSample, AL_POSITION, pos);
}

/***************************************************************************/

void
sound_PauseSample( AUDIO_SAMPLE * psSample ) {
	alSourcePause(psSample->iSample);
}

/***************************************************************************/

void
sound_ResumeSample( AUDIO_SAMPLE * psSample ) {
	alSourcePlay(psSample->iSample);
}

/***************************************************************************/


void
sound_PauseAll( void ) {
}

/***************************************************************************/

void
sound_ResumeAll( void ) {
}

/***************************************************************************/

void
sound_StopAll( void ) {
}

/***************************************************************************/

BOOL
sound_SampleIsFinished( AUDIO_SAMPLE * psSample ) {
	ALenum state;

	alGetSourcei(psSample->iSample, AL_SOURCE_STATE, &state);

	if (state == AL_PLAYING) {
		return FALSE;
	} else {
		alDeleteSources(1, &(psSample->iSample));
		return TRUE;
	}
}

/***************************************************************************/

LPDIRECTSOUND
sound_GetDirectSoundObj( void ) {
	return NULL;
}

/***************************************************************************/
