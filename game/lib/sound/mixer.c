/***************************************************************************/

#include "mixer.h"

/***************************************************************************/

static HMIXER		g_hMixer;
static SDWORD		g_iMixerLineCD,
					g_iMixerLineWav;
static DWORD		g_dwCDControlID,
					g_dwWavControlID,
					g_iVolRangeCD,
					g_iVolRangeWav,
					g_iWinVolWav,
					g_iWinVolCD,
					g_iInGameVolWav,
					g_iInGameVolCD;
static BOOL			bMixerOK = FALSE;

/***************************************************************************/

static BOOL
mixer_GetVolumeControlID( DWORD dwComponentType, DWORD *pdwControlID,
							DWORD *piVolRange )
{
#ifndef WIN32
	return TRUE;
#else
	MMRESULT			mmRes;
	UINT				i;
	MIXERLINE			mixerLine;
	MIXERLINECONTROLS	mixerLineControls;
	MIXERCONTROL		*aMixerControl = NULL;
	BOOL				bControlFound = FALSE;

	/* get mixer line */
	memset( &mixerLine, 0 , sizeof(MIXERLINE) );
	mixerLine.cbStruct        = sizeof(MIXERLINE);
	mixerLine.dwComponentType = dwComponentType;
	if ( (mmRes = mixerGetLineInfo( g_hMixer, &mixerLine,
				MIXER_GETLINEINFOF_COMPONENTTYPE )) != MMSYSERR_NOERROR )
	{
		DBPRINTF( ("mixer_GetVolumeControlID: mixerGetLineInfo failed\n ") );
		return FALSE;
	}
	else
	{
		/* allocate control space */
		aMixerControl = MALLOC( mixerLine.cControls * sizeof(MIXERCONTROL) );
		if ( aMixerControl == NULL )
		{
			DBPRINTF( ("mixer_GetVolumeControlID: malloc failed\n ") );
			return FALSE;
		}

		/* get control details */
		memset( &mixerLineControls, 0, sizeof(MIXERLINECONTROLS) );
		mixerLineControls.cbStruct      = sizeof(MIXERLINECONTROLS);
		mixerLineControls.dwLineID      = mixerLine.dwLineID;
		mixerLineControls.cControls     = mixerLine.cControls;
		mixerLineControls.cbmxctrl      = sizeof(MIXERCONTROL);
		mixerLineControls.pamxctrl      = aMixerControl;
		if ( (mmRes = mixerGetLineControls( g_hMixer, &mixerLineControls,
						MIXER_GETLINECONTROLSF_ALL )) != MMSYSERR_NOERROR )
		{
			DBPRINTF( ("mixer_GetVolumeControlID: mixerGetLineControls failed\n ") );
			return FALSE;
		}
		else
		{
			/* find volume channel */
			for ( i=0; i<mixerLine.cControls; i++ )
			{
				if ( aMixerControl[i].dwControlType ==
						MIXERCONTROL_CONTROLTYPE_VOLUME )
				{
					*pdwControlID = aMixerControl[i].dwControlID;
					bControlFound = TRUE;
					break;
				}
			}

			/* save control volume range */
			if ( bControlFound == TRUE )
			{
				*piVolRange = aMixerControl[i].Bounds.dwMaximum -
							  aMixerControl[i].Bounds.dwMinimum;
			}

			FREE( aMixerControl );
		}
	}

	return bControlFound;
#endif
}

/***************************************************************************/

void
mixer_SaveWinVols()
{
#ifdef WIN32
	g_iWinVolWav = mixer_GetWavVolume();
	g_iWinVolCD  = mixer_GetCDVolume();
#endif
}

/***************************************************************************/

void
mixer_RestoreWinVols()
{
#ifdef WIN32
	/* restore Windows volumes */
	mixer_SetWavVolume( g_iWinVolWav );
	mixer_SetCDVolume(  g_iWinVolCD );
#endif
}

/***************************************************************************/

void
mixer_SaveIngameVols()
{
#ifdef WIN32
	g_iInGameVolWav = mixer_GetWavVolume();
	g_iInGameVolCD  = mixer_GetCDVolume();
#endif
}

/***************************************************************************/

void
mixer_RestoreIngameVols()
{
#ifdef WIN32
	mixer_SetWavVolume( g_iInGameVolWav );
	mixer_SetCDVolume(  g_iInGameVolCD );
#endif
}

/***************************************************************************/

BOOL
mixer_Open( void )
{
#ifndef WIN32
	return TRUE;
#else
	MIXERCAPS			mixerCaps;
	MMRESULT			mmRes;

	if ( (mmRes = mixerGetDevCaps( 0, &mixerCaps, sizeof(MIXERCAPS) )) != MMSYSERR_NOERROR )
	{
		DBPRINTF( ("mixer_Open: mixerGetDevCaps failed\n ") );
		return FALSE;
	}

	/* open mixer */
	if ( (mmRes = mixerOpen( &g_hMixer, 0, 0, 0, MIXER_OBJECTF_MIXER )) != MMSYSERR_NOERROR )
	{
		DBPRINTF( ("mixer_Open: mixerOpen failed\n ") );
		return FALSE;
	}
 
	/* get CD and wav volume controls */
	bMixerOK = mixer_GetVolumeControlID(
					MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC,
					&g_dwCDControlID, &g_iVolRangeCD );
	if ( bMixerOK == TRUE )
	{
		bMixerOK = mixer_GetVolumeControlID(
					MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT,
					&g_dwWavControlID, &g_iVolRangeWav );
	}

	/* check volume controls found */
	if ( bMixerOK == FALSE )
	{
		DBPRINTF( ("mixer_Open: couldn't get mixer controls\n ") );
		mixerClose( g_hMixer );
		return FALSE;
	}

	mixer_SaveWinVols();

	return TRUE;
#endif
}

/***************************************************************************/

void
mixer_Close( void )
{
#ifdef WIN32
	mixer_RestoreWinVols();

	if ( bMixerOK == TRUE )
	{
		mixerClose( g_hMixer );
	}
#endif
}

/***************************************************************************/

static SDWORD
mixer_GetVolume( DWORD dwControlID, DWORD iVolRange )
{
#ifndef WIN32
	return 100;
#else
	SDWORD							iVol = 0;
	MMRESULT						mmRes;
	MIXERCONTROLDETAILS				mixerCntlDetails;
	MIXERCONTROLDETAILS_UNSIGNED	mxCntrlDetUnSigned;

	if ( bMixerOK == TRUE )
	{
		memset( &mixerCntlDetails, 0, sizeof(MIXERCONTROLDETAILS) );
		mixerCntlDetails.cbStruct    = sizeof(MIXERCONTROLDETAILS);
		mixerCntlDetails.dwControlID = dwControlID;
		mixerCntlDetails.cChannels   = 1;
		mixerCntlDetails.cbDetails   = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
		mixerCntlDetails.paDetails   = &mxCntrlDetUnSigned;
		if ( (mmRes = mixerGetControlDetails( g_hMixer, &mixerCntlDetails,
				MIXER_GETCONTROLDETAILSF_VALUE )) != MMSYSERR_NOERROR     )
		{
			return 0;
		}

		/* scale audio 0->AUDIO_VOL_MAX-1 */
		iVol = mxCntrlDetUnSigned.dwValue * AUDIO_VOL_MAX / iVolRange;
	}

	return iVol;
#endif
}

/***************************************************************************/

static void
mixer_SetVolume( DWORD dwControlID, DWORD iVolRange, SDWORD iVol )
{
#ifndef WIN32
	return;
#else
	MMRESULT						mmRes;
	MIXERCONTROLDETAILS				mixerCntlDetails;
	MIXERCONTROLDETAILS_UNSIGNED	mxCntrlDetUnSigned;

	if ( bMixerOK == TRUE )
	{
		ASSERT( (iVol>=0 && iVol<=AUDIO_VOL_MAX,
			"mixer_SetVolume: vol %i out of range 0->100", iVol ) );
	
		/* scale audio 0->AUDIO_VOL_MAX-1 */
		mxCntrlDetUnSigned.dwValue = iVol * iVolRange / AUDIO_VOL_MAX;

		memset( &mixerCntlDetails, 0, sizeof(MIXERCONTROLDETAILS) );
		mixerCntlDetails.cbStruct    = sizeof(MIXERCONTROLDETAILS);
		mixerCntlDetails.dwControlID = dwControlID;
		mixerCntlDetails.cChannels   = 1;
		mixerCntlDetails.cbDetails   = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
		mixerCntlDetails.paDetails   = &mxCntrlDetUnSigned;
		if ( (mmRes = mixerSetControlDetails( g_hMixer, &mixerCntlDetails,
				MIXER_GETCONTROLDETAILSF_VALUE )) != MMSYSERR_NOERROR     )
		{
			return;
		}
	}
#endif
}

/***************************************************************************/

SDWORD
mixer_GetCDVolume( void )
{
#ifdef WIN32
	return mixer_GetVolume( g_dwCDControlID, g_iVolRangeCD );
#endif
}

/***************************************************************************/

void
mixer_SetCDVolume( SDWORD iVol )
{
#ifdef WIN32
	mixer_SetVolume( g_dwCDControlID, g_iVolRangeCD, iVol );
#endif
}

/***************************************************************************/

SDWORD
mixer_GetWavVolume( void )
{
#ifdef WIN32
	return mixer_GetVolume( g_dwWavControlID, g_iVolRangeWav );
#endif
}

/***************************************************************************/

void
mixer_SetWavVolume( SDWORD iVol )
{
#ifdef WIN32
	mixer_SetVolume( g_dwWavControlID, g_iVolRangeWav, iVol );
#endif
}

/***************************************************************************/
