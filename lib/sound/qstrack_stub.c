/***************************************************************************/
/*
 * QSound library-specific functions
 */
/***************************************************************************/

#include "frame.h"
#include "tracklib.h"
#include "audio.h"

/***************************************************************************/

BOOL
sound_InitLibrary( void )
{
	return FALSE;
}

/***************************************************************************/

void
sound_ShutdownLibrary( void )
{
}

/***************************************************************************/

void
sound_Update( void )
{
}

/***************************************************************************/

BOOL
sound_QueueSamplePlaying( void )
{
        return FALSE;
}

/***************************************************************************/

BOOL
sound_ReadTrackFromFile( TRACK * psTrack, char szFileName[] )
{
        return TRUE;
}

/***************************************************************************/

BOOL
sound_ReadTrackFromBuffer( TRACK * psTrack, void *pBuffer, UDWORD udwSize )
{
        return TRUE;
}

/***************************************************************************/

void
sound_FreeTrack( TRACK * psTrack )
{
}

/***************************************************************************/

int
sound_GetMaxVolume( void )
{
	return 32767;
}

/***************************************************************************/

BOOL
sound_Play2DSample( TRACK * psTrack, AUDIO_SAMPLE * psSample, BOOL bQueued )
{
	return TRUE;
}

/***************************************************************************/

BOOL
sound_Play3DSample( TRACK * psTrack, AUDIO_SAMPLE * psSample )
{
	return TRUE;
}

/***************************************************************************/

BOOL
sound_PlayStream( AUDIO_SAMPLE *psSample, char szFileName[], SDWORD iVol )
{
	return TRUE;
}

/***************************************************************************/

void
sound_StopSample( SDWORD iSample )
{
}

/***************************************************************************/

void
sound_SetSamplePan( AUDIO_SAMPLE * psSample, int iPan )
{
}

/***************************************************************************/

void
sound_SetSampleVolAll( int iVol )
{
}

/***************************************************************************/

void
sound_SetPlayerPos( SDWORD iX, SDWORD iY, SDWORD iZ )
{
}

/***************************************************************************/
/*
 * sound_SetPlayerOrientation
 *
 * Orientation given as angles in degrees: QSound expects position vector
 */
/***************************************************************************/

void
sound_SetPlayerOrientation( SDWORD iX, SDWORD iY, SDWORD iZ )
{
}

/***************************************************************************/

void
sound_SetObjectPosition( SDWORD iSample, SDWORD iX, SDWORD iY, SDWORD iZ )
{
}

/***************************************************************************/

void
sound_PauseSample( AUDIO_SAMPLE * psSample )
{
}

/***************************************************************************/

void
sound_ResumeSample( AUDIO_SAMPLE * psSample )
{
}

/***************************************************************************/


void
sound_PauseAll( void )
{
}

/***************************************************************************/

void
sound_ResumeAll( void )
{
}

/***************************************************************************/

void
sound_StopAll( void )
{
}

/***************************************************************************/

BOOL
sound_SampleIsFinished( AUDIO_SAMPLE * psSample )
{
	return TRUE;
}

/***************************************************************************/

LPDIRECTSOUND
sound_GetDirectSoundObj( void )
{
	return NULL;
}

/***************************************************************************/
