/***************************************************************************/
/*
 * QSound library-specific functions
 */
/***************************************************************************/

#include <windows.h>
#include <mmreg.h>

#include "frame.h"
#include "tracklib.h"
#include "audio.h"

#define	PI	 3.14159265359

#define QMIXER	1

#if QMIXER
#include "qmixer.h"
#else
#include "qmdx.h"
#endif

/***************************************************************************/

#if QMIXER
	#define QSOUND(name) QSWaveMix ## name
#else
	#define QSOUND(name) QMDX_ ## name
#endif

#define	QS_SAMPLINGRATE			KHZ22

#define	QS_CHANNELS				200
#define QS_QSOUND_CHANNELS		20

#define	QS_2D_CHANNELS			3
#define	QS_CHANNEL_QUEUE		0
#define	QS_CHANNEL_STREAM		1
#define	QS_CHANNEL_FX			2

#define	QS_MIN_DISTANCE			(300.0f)
#define	QS_SCALE				(1.5f)

#define	DYNAMIC_SHEDULING		1

/***************************************************************************/

static HQMIXER		g_hQMixer;
static QMIXCONFIG	g_qMixConfig;

static char			g_szErrMsg[MAX_STR];
static UINT			g_uiRet;
static SDWORD		g_iError;

static UDWORD		g_iDynamicChannel = QS_2D_CHANNELS;

typedef struct RIFFDATA
{
	WAVEFORMATEX	*pWaveFormat;
	UBYTE			*pubData;
	LPMIXWAVE		psMixWave;
}
RIFFDATA;

/***************************************************************************/

#define	MONO	 1
#define	STEREO	 2
#define	BIT8	 8
#define	BIT16	16

#define	MONO_16BIT_BLOCKALIGN	MONO*BIT16/8
#define	STEREO_16BIT_BLOCKALIGN	STEREO*BIT16/8

#define	AVERAGEBYTERATE_MONO16		QS_SAMPLINGRATE*MONO_16BIT_BLOCKALIGN			
#define	AVERAGEBYTERATE_STEREO16	QS_SAMPLINGRATE*STEREO_16BIT_BLOCKALIGN			

/***************************************************************************/

BOOL
sound_InitLibrary( void )
{
	static WAVEFORMATEX
		wFormatMonoPCM16   = { WAVE_FORMAT_PCM, 1, QS_SAMPLINGRATE, AVERAGEBYTERATE_MONO16,
							MONO_16BIT_BLOCKALIGN, BIT16, 0 },
		wFormatStereoPCM16 = { WAVE_FORMAT_PCM, 2, QS_SAMPLINGRATE, AVERAGEBYTERATE_STEREO16,
							STEREO_16BIT_BLOCKALIGN, BIT16, 0 },

		wFormatMonoADPCM11 = { WAVE_FORMAT_ADPCM, 1, KHZ11, 5644,  256, 4, 32 },
		wFormatMonoADPCM22 = { WAVE_FORMAT_ADPCM, 1, KHZ22, 11155, 512, 4, 512 };


	/* specify number of dynamic channels to be as many hardware as possible
	 * and QS_QSOUND_CHANNELS software channels
	 */
	QMIX_CHANNELTYPES	channelTypes[] = {
		{ QMIX_CHANNELTYPE_QSOUND    | QMIX_CHANNELTYPE_2D, QS_2D_CHANNELS },
//		{ QMIX_CHANNELTYPE_STREAMING | QMIX_CHANNELTYPE_3D, -1 },
		{ QMIX_CHANNELTYPE_QSOUND    | QMIX_CHANNELTYPE_3D, QS_QSOUND_CHANNELS },
		{ 0, 0 } };

	/* init config struct */
	memset( &g_qMixConfig, 0, sizeof( g_qMixConfig ) );
	g_qMixConfig.dwSize         = sizeof( g_qMixConfig );
	g_qMixConfig.dwSamplingRate = QS_SAMPLINGRATE;
	g_qMixConfig.iChannels      = QS_CHANNELS;

	g_hQMixer = QSOUND(InitEx( &g_qMixConfig ));
	if ( !g_hQMixer )
	{
		goto initError;
	}

	/* open all channels */
	g_uiRet = QSOUND(OpenChannel( g_hQMixer, 0, QMIX_OPENALL ));
	if ( g_uiRet == -1 )
	{
		goto initError;
	}

# if USE_COMPRESSED_SPEECH
	/* configure fixed 2D speech queue channel */
	g_uiRet = QSOUND(ConfigureChannel( g_hQMixer, QS_CHANNEL_QUEUE,
		 QMIX_CHANNELTYPE_2D, &wFormatMonoADPCM11, 0 ));
	if ( g_uiRet < 0 )
	{
		goto initError;
	}
#else
	/* configure fixed 2D speech queue channel */
	g_uiRet = QSOUND(ConfigureChannel( g_hQMixer, QS_CHANNEL_QUEUE,
		 QMIX_CHANNELTYPE_2D, &wFormatMonoPCM16, 0 ));
	if ( g_uiRet < 0 )
	{
		goto initError;
	}
#endif

	/* configure fixed 2D streaming channel */
	g_uiRet = QSOUND(ConfigureChannel( g_hQMixer, QS_CHANNEL_STREAM,
		QMIX_CHANNELTYPE_2D, &wFormatStereoPCM16, 0 ));
	if ( g_uiRet < 0 )
	{
		goto initError;
	}

	/* configure fixed 2D fx channel */
	g_uiRet = QSOUND(ConfigureChannel( g_hQMixer, QS_CHANNEL_FX,
		QMIX_CHANNELTYPE_2D, &wFormatMonoPCM16, 0 ));
	if ( g_uiRet < 0 )
	{
		goto initError;
	}

#if DYNAMIC_SHEDULING
	/* configure dynamic allocation channels */
	g_uiRet = QSOUND(PrioritizeChannels( g_hQMixer, channelTypes,
				QMIX_FINDCHANNEL_DISTANCE, 0 ));
	if ( g_uiRet < 0 )
	{
		goto initError;
	}
#endif

	/* start system */
	g_uiRet = QSOUND(Activate( g_hQMixer, TRUE ));
	if ( g_uiRet < 0 )
	{
		goto initError;
	}

	return TRUE;

initError:

	g_iError = QSOUND(GetLastError());
	QSOUND(GetErrorText( g_iError, g_szErrMsg, MAX_STR ));
	DBPRINTF( ("sound_InitLibrary: %s", g_szErrMsg) );
	return FALSE;
}

/***************************************************************************/

void
sound_ShutdownLibrary( void )
{
	if ( QSOUND(CloseChannel( g_hQMixer, -1, QMIX_ALL )) != 0 ||
		 QSOUND(CloseSession( g_hQMixer )) != 0 )
	{
		g_iError = QSOUND(GetLastError());
		QSOUND(GetErrorText( g_iError, g_szErrMsg, MAX_STR ));
		DBPRINTF( ("sound_ShutdownLibrary: %s", g_szErrMsg) );
	}
}

/***************************************************************************/

void
sound_Update( void )
{
#if 0
	QSOUND(Pump());
#endif
}

/***************************************************************************/

BOOL
sound_QueueSamplePlaying( void )
{
	return !QSOUND(IsChannelDone( g_hQMixer, QS_CHANNEL_QUEUE ));
}

/***************************************************************************/

static void
sound_SaveTrackData( TRACK * psTrack, QMIXWAVEPARAMS *psQMixParams,
						LPMIXWAVE psMixWave )
{
	RIFFDATA		*psRiffData;

	psTrack->iTime = psMixWave->wh.dwBufferLength * 1000 /
							g_qMixConfig.dwSamplingRate + 1;

	/* add to riff data list */
	psRiffData = MALLOC( sizeof(RIFFDATA) );
	psRiffData->pWaveFormat = psQMixParams->Resident.Format;
	psRiffData->pubData     = psQMixParams->Resident.Data;
	psRiffData->psMixWave   = psMixWave;

	/* save data pointer in track */
	psTrack->pMem = psRiffData;
}

/***************************************************************************/

BOOL
sound_ReadTrackFromFile( TRACK * psTrack, char szFileName[] )
{
	QMIXWAVEPARAMS	sQMixParams;
	LPMIXWAVE		psMixWave;

	memset( &sQMixParams, 0, sizeof(QMIXWAVEPARAMS) );
	sQMixParams.FileName = szFileName;
	psMixWave = QSOUND(OpenWaveEx( g_hQMixer, &sQMixParams, QMIX_FILE ));

	if ( psMixWave )
	{
		sound_SaveTrackData( psTrack, &sQMixParams, psMixWave );
		return TRUE;
	}
	else
	{
		g_iError = QSOUND(GetLastError());
		QSOUND(GetErrorText( g_iError, g_szErrMsg, MAX_STR ));
		DBERROR( ("sound_LoadTrackFromFile: %s", g_szErrMsg) );
		return FALSE;
	}
}

/***************************************************************************/

static BOOL
sound_ReadRiffMemResFile( QMIXWAVEPARAMS *pQMixParams, void *pBuffer,
							UDWORD udwSize, BOOL *pbCompressed )
{
	MMIOINFO	mmioInfo;
    MMCKINFO	waveChunk, factChunk, formatChunk, dataChunk;
	HMMIO		hmmio;

	memset( pQMixParams, 0, sizeof(QMIXWAVEPARAMS) );

	memset( &mmioInfo, 0, sizeof(MMIOINFO) );
	mmioInfo.fccIOProc = FOURCC_MEM;
	mmioInfo.cchBuffer = udwSize;
	mmioInfo.pchBuffer = pBuffer;

	hmmio = mmioOpen( NULL, &mmioInfo, MMIO_READ );
    if ( !hmmio )
	{
		return FALSE;
	}

    waveChunk.fccType = mmioFOURCC('W', 'A', 'V', 'E');
    if ( mmioDescend(hmmio, &waveChunk, 0, MMIO_FINDRIFF) )
	{
		return FALSE;
	}

    /* Look for 'fact' chunk in case we have a compressed file
	 * and set flag accordingly
	 */
    factChunk.ckid = mmioFOURCC('f', 'a', 'c', 't');
    if ( mmioDescend(hmmio, &factChunk, &waveChunk, MMIO_FINDCHUNK) == 0 &&
		factChunk.cksize >= sizeof(DWORD) )
    {
        mmioRead(hmmio, (char*)&pQMixParams->Resident.Samples, sizeof(DWORD));
		*pbCompressed = TRUE;
    }
	else
	{
		*pbCompressed = FALSE;
	}

    mmioSeek(hmmio, 0, SEEK_SET);
    mmioDescend(hmmio, &waveChunk, 0, MMIO_FINDRIFF);

    //
    // Read wave format.
    //
    formatChunk.ckid = mmioFOURCC('f', 'm', 't', ' ');
    if ( mmioDescend(hmmio, &formatChunk, &waveChunk, MMIO_FINDCHUNK) )
	{
		return FALSE;
	}

    pQMixParams->Resident.Format = (WAVEFORMATEX*) MALLOC( formatChunk.cksize );
    if ( mmioRead(hmmio, (char *) pQMixParams->Resident.Format,
			formatChunk.cksize) != (LONG)formatChunk.cksize )
	{
		return FALSE;
	}

    mmioAscend(hmmio, &formatChunk, 0);

    //
    // Read wave data.
    //
    dataChunk.ckid = mmioFOURCC('d', 'a', 't', 'a');
    if ( mmioDescend(hmmio, &dataChunk, &waveChunk, MMIO_FINDCHUNK) )
	{
		return FALSE;
	}

	pQMixParams->Resident.Bytes = dataChunk.cksize;
    pQMixParams->Resident.Data  = MALLOC( dataChunk.cksize );

    if ( mmioRead( hmmio, (char *) pQMixParams->Resident.Data,
					dataChunk.cksize) != (LONG)dataChunk.cksize )
	{
		return FALSE;
	}
    
	mmioAscend(hmmio, &dataChunk, 0);

    mmioClose(hmmio, 0);

	return TRUE;
}

/***************************************************************************/

BOOL
sound_ReadTrackFromBuffer( TRACK * psTrack, void *pBuffer, UDWORD udwSize )
{
	QMIXWAVEPARAMS	sQMixParams;
	LPMIXWAVE		psMixWave;

	if ( !sound_ReadRiffMemResFile( &sQMixParams, pBuffer, udwSize,
									&psTrack->bCompressed ) )
	{
		return FALSE;
	}

	psMixWave = QSOUND(OpenWaveEx( g_hQMixer, &sQMixParams, QMIX_RESIDENT ));

	if ( psMixWave )
	{
		sound_SaveTrackData( psTrack, &sQMixParams, psMixWave );
		return TRUE;
	}
	else
	{
		g_iError = QSOUND(GetLastError());
		QSOUND(GetErrorText( g_iError, g_szErrMsg, MAX_STR ));
		DBERROR( ("sound_ReadTrackFromBuffer: %s", g_szErrMsg) );
		return FALSE;
	}
}

/***************************************************************************/

void
sound_FreeTrack( TRACK * psTrack )
{
	RIFFDATA	*psRiffData = (RIFFDATA *) psTrack->pMem;

	if ( !PTRVALID(psRiffData, sizeof(RIFFDATA)) )
	{
		DBPRINTF( ("sound_FreeTrack: invalid data pointer") );
		return;
	}

	g_uiRet = QSOUND(FreeWave( g_hQMixer, psRiffData->psMixWave ));
	if ( g_uiRet != 0 )
	{
		g_iError = QSOUND(GetLastError());
		QSOUND(GetErrorText( g_iError, g_szErrMsg, MAX_STR ));
		DBPRINTF( ("sound_FreeTrack: %s", g_szErrMsg) );
	}

	if ( psRiffData->pWaveFormat != NULL )
	{
		FREE( psRiffData->pWaveFormat );
	}

	if ( psRiffData->pubData != NULL )
	{
		FREE( psRiffData->pubData );
	}

	FREE( psRiffData );
}

/***************************************************************************/

int
sound_GetMaxVolume( void )
{
	return 32767;
}

/***************************************************************************/

void CALLBACK
sound_QSoundFinishedCallback(int iChannel, LPMIXWAVE lpWave, DWORD dwUser )
{
	if ( sound_GetSystemActive() == TRUE )
	{
		iChannel;
		lpWave;
		sound_FinishedCallback( (AUDIO_SAMPLE *) dwUser );
	}
}

/***************************************************************************/

static void
sound_SetupChannel( AUDIO_SAMPLE * psSample, QMIXPLAYPARAMS *pParams,
					SDWORD *piLoops )
{
	if ( sound_TrackLooped( psSample->iTrack ) == TRUE )
	{
		*piLoops = -1;
	}
	else
	{
		*piLoops = 0;
	}

	/* set up callback */
	memset( pParams, 0, sizeof(QMIXPLAYPARAMS));
	pParams->dwSize   = sizeof(QMIXPLAYPARAMS);
	pParams->callback = sound_QSoundFinishedCallback;
	pParams->dwUser   = (DWORD) psSample;
}

/***************************************************************************/

BOOL
sound_Play2DSample( TRACK * psTrack, AUDIO_SAMPLE * psSample, BOOL bQueued )
{
	SDWORD			iLoops, iPan;
	QMIXPLAYPARAMS	params;
	RIFFDATA		*psRiffData = (RIFFDATA *) psTrack->pMem;

	sound_SetupChannel( psSample, &params, &iLoops );

	if ( bQueued == TRUE )
	{
		psSample->iSample = QS_CHANNEL_QUEUE;
	}
	else
	{
		psSample->iSample = QS_CHANNEL_FX;
	}

	sound_SetSampleVol( psSample, AUDIO_VOL_MAX, FALSE );

	iPan = (AUDIO_PAN_MAX-AUDIO_PAN_MIN)/2;
	sound_SetSamplePan( psSample, iPan );

	g_uiRet = QSOUND(PlayEx( g_hQMixer, psSample->iSample,
						QMIX_QUEUEWAVE | QMIX_PLAY_NOTIFYSTOP,
						psRiffData->psMixWave, iLoops, &params ));
	if ( g_uiRet != 0 )
	{
		goto playError;
	}

	return TRUE;

playError:

	g_iError = QSOUND(GetLastError());
	QSOUND(GetErrorText( g_iError, g_szErrMsg, MAX_STR ));
	DBPRINTF( ("sound_Play2DSample: %s", g_szErrMsg) );
	return FALSE;
}

/***************************************************************************/

BOOL
sound_Play3DSample( TRACK * psTrack, AUDIO_SAMPLE * psSample )
{
	SDWORD			iLoops;
	QMIXPLAYPARAMS	params;
	QMIX_DISTANCES	sDistances;
	RIFFDATA		*psRiffData = (RIFFDATA *) psTrack->pMem;

	sound_SetupChannel( psSample, &params, &iLoops );

	psSample->iSample = QSOUND(FindChannel( g_hQMixer, QMIX_FINDCHANNEL_DISTANCE, 0 ));
	if ( psSample->iSample < 0 )
	{
		goto playError;
	}

	/* set distance mapping for 3D */
	sDistances.cbSize = sizeof(QMIX_DISTANCES);
	sDistances.minDistance = QS_MIN_DISTANCE;
	sDistances.maxDistance = (float) sound_TrackAudibleRadius( psSample->iTrack );
	sDistances.scale = QS_SCALE;
	
	g_uiRet = QSOUND(SetDistanceMapping( g_hQMixer, psSample->iSample,
											0, &sDistances ));    
	
	sound_SetObjectPosition( psSample->iSample,
								psSample->x, psSample->y, psSample->z );

#if 0
DBPRINTF( ("sound_Play3DSample: playing channel %i\n", psSample->iSample ) );
#endif

	sound_SetSampleVol( psSample, AUDIO_VOL_MAX, TRUE );

	/* clear queue on channel and play sound */
	g_uiRet = QSOUND(PlayEx( g_hQMixer, psSample->iSample,
						QMIX_CLEARQUEUE | QMIX_PLAY_NOTIFYSTOP,
						psRiffData->psMixWave, iLoops, &params ));
	if ( g_uiRet < 0 )
	{
		goto playError;
	}

	return TRUE;

playError:

	g_iError = QSOUND(GetLastError());
	QSOUND(GetErrorText( g_iError, g_szErrMsg, MAX_STR ));
//	DBPRINTF( ("sound_Play3DSample: %s\n", g_szErrMsg) );
	return FALSE;
}

/***************************************************************************/

BOOL
sound_PlayStream( AUDIO_SAMPLE *psSample, char szFileName[], SDWORD iVol )
{
	QMIXPLAYPARAMS	params;
	LPMIXWAVE		psMixWave;
	SDWORD			iPan;

	psMixWave = QSOUND(OpenWave( g_hQMixer, szFileName, 0, QMIX_FILESTREAM ));
	if ( psMixWave == NULL )
	{
		goto streamError;
	}

	memset(&params, 0, sizeof(params));
	params.dwSize   = sizeof(params);
	params.callback = sound_QSoundFinishedCallback;
	params.dwUser   = (DWORD) psSample;

	/* play on stream channel */
	psSample->iSample  = QS_CHANNEL_STREAM;

	sound_SetSampleVol( psSample, AUDIO_VOL_MAX, FALSE );
	iPan = (AUDIO_PAN_MAX-AUDIO_PAN_MIN)/2;
	sound_SetSamplePan( psSample, iPan );

	if ( g_uiRet != 0 )
	{
		goto streamError;
	}

	g_uiRet = QSOUND(PlayEx( g_hQMixer, psSample->iSample,
								QMIX_CLEARQUEUE, psMixWave, 0, &params ));
	if ( g_uiRet != 0 )
	{
		goto streamError;
	}

	return TRUE;

streamError:

	g_iError = QSOUND(GetLastError());
	QSOUND(GetErrorText( g_iError, g_szErrMsg, MAX_STR ));
	DBPRINTF( ("sound_PlayStream: %s", g_szErrMsg) );
	return FALSE;
}

/***************************************************************************/

void
sound_StopSample( SDWORD iSample )
{
	g_uiRet = QSOUND(FlushChannel( g_hQMixer, iSample, 0 ));

	if ( g_uiRet != 0 )
	{
		g_iError = QSOUND(GetLastError());
		QSOUND(GetErrorText( g_iError, g_szErrMsg, MAX_STR ));
		DBPRINTF( ("sound_StopSample: %s", g_szErrMsg) );
	}
}

/***************************************************************************/

void
sound_SetSamplePan( AUDIO_SAMPLE * psSample, int iPan )
{
	g_uiRet = QSOUND(SetPan( g_hQMixer, psSample->iSample,
									0, iPan*30/AUDIO_PAN_RANGE ));

	if ( g_uiRet != 0 )
	{
		g_iError = QSOUND(GetLastError());
		QSOUND(GetErrorText( g_iError, g_szErrMsg, MAX_STR ));
		DBPRINTF( ("sound_SetSamplePan: %s", g_szErrMsg) );
	}
}

/***************************************************************************/

static void
sound_SetSampleVol( AUDIO_SAMPLE * psSample, int iVol, BOOL bScale3D )
{
	g_uiRet = QSOUND(SetVolume( g_hQMixer, psSample->iSample,
						0, audio_GetSampleMixVol(psSample,iVol,bScale3D) ));
	if ( g_uiRet != 0 )
	{
		g_iError = QSOUND(GetLastError());
		QSOUND(GetErrorText( g_iError, g_szErrMsg, MAX_STR ));
		DBPRINTF( ("sound_SetSampleVol: sample %i %s\n",
					psSample->iSample, g_szErrMsg) );
	}
}

/***************************************************************************/

void
sound_SetSampleVolAll( int iVol )
{
	g_uiRet = QSOUND(SetVolume( g_hQMixer, 0, QMIX_ALL,
									audio_GetMixVol(iVol) ));
	if ( g_uiRet != 0 )
	{
		g_iError = QSOUND(GetLastError());
		QSOUND(GetErrorText( g_iError, g_szErrMsg, MAX_STR ));
		DBPRINTF( ("sound_SetSampleVolAll: %s", g_szErrMsg) );
	}
}

/***************************************************************************/

void
sound_SetPlayerPos( SDWORD iX, SDWORD iY, SDWORD iZ )
{
	QSVECTOR	vPosition;

	vPosition.x = (float) iX;
	vPosition.y = (float) iY;
	vPosition.z = (float) iZ;

	g_uiRet = QSOUND(SetListenerPosition( g_hQMixer, &vPosition, 0 ));
	if ( g_uiRet != 0 )
	{
		g_iError = QSOUND(GetLastError());
		QSOUND(GetErrorText( g_iError, g_szErrMsg, MAX_STR ));
		DBPRINTF( ("sound_SetPlayerPos: %s", g_szErrMsg) );
	}
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
	QSVECTOR	vUp = { 0.0f, 0.0f, 1.0f }, vDirection;

	vDirection.x = (float) -sin(((float)iZ)*PI/180.0f);
	vDirection.y = (float)  cos(((float)iZ)*PI/180.0f);
	vDirection.z = 0.0f;

	g_uiRet = QSOUND(SetListenerOrientation( g_hQMixer, &vDirection, &vUp, 0 ));
	if ( g_uiRet != 0 )
	{
		g_iError = QSOUND(GetLastError());
		QSOUND(GetErrorText( g_iError, g_szErrMsg, MAX_STR ));
		DBPRINTF( ("sound_SetPlayerOrientation: %s", g_szErrMsg) );
	}
}

/***************************************************************************/

void
sound_SetObjectPosition( SDWORD iSample, SDWORD iX, SDWORD iY, SDWORD iZ )
{
	QSVECTOR	vPosition;

	vPosition.x = (float) iX;
	vPosition.y = (float) iY;
	vPosition.z = (float) iZ;

	g_uiRet = QSOUND(SetSourcePosition( g_hQMixer, iSample,
											0, &vPosition ));
	if ( g_uiRet != 0 )
	{
		g_iError = QSOUND(GetLastError());
		QSOUND(GetErrorText( g_iError, g_szErrMsg, MAX_STR ));
		DBPRINTF( ("sound_SetObjectPosition: sample %i %s\n",
					iSample, g_szErrMsg) );
	}
}

/***************************************************************************/

void
sound_PauseSample( AUDIO_SAMPLE * psSample )
{
	g_uiRet = QSOUND(PauseChannel( g_hQMixer, 0, psSample->iSample ));

	if ( g_uiRet != 0 )
	{
		g_iError = QSOUND(GetLastError());
		QSOUND(GetErrorText( g_iError, g_szErrMsg, MAX_STR ));
		DBPRINTF( ("sound_PauseSample: %s", g_szErrMsg) );
	}
}

/***************************************************************************/

void
sound_ResumeSample( AUDIO_SAMPLE * psSample )
{
	g_uiRet = QSOUND(RestartChannel( g_hQMixer, 0, psSample->iSample ));

	if ( g_uiRet != 0 )
	{
		g_iError = QSOUND(GetLastError());
		QSOUND(GetErrorText( g_iError, g_szErrMsg, MAX_STR ));
		DBPRINTF( ("sound_ResumeSample: %s", g_szErrMsg) );
	}
}

/***************************************************************************/


void
sound_PauseAll( void )
{
	g_uiRet = QSOUND(PauseChannel( g_hQMixer, 0, QMIX_ALL ));
	if ( g_uiRet != 0 )
	{
		g_iError = QSOUND(GetLastError());
		QSOUND(GetErrorText( g_iError, g_szErrMsg, MAX_STR ));
		DBPRINTF( ("sound_PauseAll: %s", g_szErrMsg) );
	}

	/* don't pause streaming channel */
	g_uiRet = QSOUND(RestartChannel( g_hQMixer, 0, QS_CHANNEL_STREAM ));
	if ( g_uiRet != 0 )
	{
		g_iError = QSOUND(GetLastError());
		QSOUND(GetErrorText( g_iError, g_szErrMsg, MAX_STR ));
		DBPRINTF( ("sound_PauseAll: %s", g_szErrMsg) );
	}
}

/***************************************************************************/

void
sound_ResumeAll( void )
{
	g_uiRet = QSOUND(RestartChannel( g_hQMixer, 0, QMIX_ALL ));
	if ( g_uiRet != 0 )
	{
		g_iError = QSOUND(GetLastError());
		QSOUND(GetErrorText( g_iError, g_szErrMsg, MAX_STR ));
		DBPRINTF( ("sound_ResumeAll: %s", g_szErrMsg) );
	}
}

/***************************************************************************/

void
sound_StopAll( void )
{
	g_uiRet = QSOUND(FlushChannel( g_hQMixer, 0, QMIX_ALL ));

	if ( g_uiRet != 0 )
	{
		g_iError = QSOUND(GetLastError());
		QSOUND(GetErrorText( g_iError, g_szErrMsg, MAX_STR ));
		DBPRINTF( ("sound_StopAll: %s", g_szErrMsg) );
	}
}

/***************************************************************************/

BOOL
sound_SampleIsFinished( AUDIO_SAMPLE * psSample )
{
	if ( QSOUND(IsChannelDone( g_hQMixer, psSample->iSample )) )
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/***************************************************************************/

#ifdef WIN32
LPDIRECTSOUND
sound_GetDirectSoundObj( void )
{
	LPDIRECTSOUND	pDirectSound;

	g_uiRet = QSOUND(GetDirectSound( g_hQMixer, &pDirectSound ));

	if ( g_uiRet != 0 )
	{
		g_iError = QSOUND(GetLastError());
		QSOUND(GetErrorText( g_iError, g_szErrMsg, MAX_STR ));
		DBPRINTF( ("sound_GetDirectSoundObj: %s", g_szErrMsg) );
		return NULL;
	}
	else
	{
		return pDirectSound;
	}
}
#endif

/***************************************************************************/
