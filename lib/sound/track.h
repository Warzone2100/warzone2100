/***************************************************************************/

#ifndef _TRACK_H_
#define _TRACK_H_

/***************************************************************************/
/* defines */

#include "frame.h"

#ifdef WZ_OPENAL_MAC_H
#include <openal/al.h>
#else
#include <AL/al.h>
#endif

#ifndef MAX_STR
	#define	MAX_STR			255
#endif

#define	MIN_PITCH				0
#define	MAX_PITCH				127
#define	CENTER_PITCH			(MAX_PITCH-MIN_PITCH)/2

#define	SAMPLE_NOT_ALLOCATED	-1
#define	NO_MAX_SAME_SAMPLES		-2
#define	SAMPLE_NOT_FOUND		-3
#define	SAMPLE_GROUP_NOT_SET	-4
#define	SAMPLE_COORD_INVALID	-5

#define	AUDIO_PAN_MIN			0L
#define	AUDIO_PAN_MAX			100L
#define AUDIO_PAN_RANGE			(AUDIO_PAN_MAX-AUDIO_PAN_MIN)
#define	AUDIO_VOL_MIN			0L
#define	AUDIO_VOL_MAX			100L
#define	AUDIO_VOL_RANGE			(AUDIO_VOL_MAX-AUDIO_VOL_MIN)

/***************************************************************************/

/***************************************************************************/
/* enums */


/***************************************************************************/
/* forward definitions
 */

struct AUDIO_SAMPLE;

/***************************************************************************/
/* typedefs
 */

typedef BOOL (* SAMPLEVALIDFUNC) ( struct AUDIO_SAMPLE *psSample );
typedef BOOL (* AUDIO_CALLBACK)  ( struct AUDIO_SAMPLE *psSample );

/***************************************************************************/
/* structs */

typedef struct AUDIO_SAMPLE
{
	SDWORD				iTrack;
	ALuint				iSample;
	SDWORD				x, y, z;
	SDWORD				iLoops;
	BOOL				bRemove;
	AUDIO_CALLBACK		pCallback;
	void				*psObj;
	struct AUDIO_SAMPLE	*psPrev;
	struct AUDIO_SAMPLE	*psNext;
}
AUDIO_SAMPLE;

typedef struct TRACK
{
	BOOL		bLoop;
	SDWORD		iVol;
	SDWORD		iPriority;
	SDWORD		iAudibleRadius;
	SDWORD		iTime;					/* duration in milliseconds */
	UDWORD		iTimeLastFinished;		/* time last finished in ms */
	UDWORD		iNumPlaying;
	BOOL		bMemBuffer;				/* memory buffer flag       */
	ALuint		iBufferName;				/* name of the openal buffer */
	char		*pName;					// resource name of the track
	UDWORD		resID;					// hashed name of the WAV

} TRACK;

/***************************************************************************/
/* functions
 */

BOOL	sound_Init( SDWORD iMaxSameSamples );
BOOL	sound_Shutdown(void);

BOOL	sound_LoadTrackFromFile(char szFileName[]);
TRACK *	sound_LoadTrackFromBuffer(char *pBuffer, UDWORD udwSize);
BOOL	sound_SetTrackVals( TRACK *psTrack, BOOL bLoop, SDWORD iTrack,
			SDWORD iVol, SDWORD iPriority, SDWORD iAudibleRadius);
BOOL	sound_ReleaseTrack( TRACK * psTrack );

void	sound_StopTrack( AUDIO_SAMPLE *psSample );
void	sound_PauseTrack( AUDIO_SAMPLE *psSample );
void	sound_UpdateSample( AUDIO_SAMPLE *psSample );
void	sound_CheckSample( AUDIO_SAMPLE *psSample );
void	sound_CheckAllUnloaded( void );

BOOL	sound_CheckTrack( SDWORD iTrack );

SDWORD	sound_GetTrackTime( SDWORD iTrack );
SDWORD	sound_GetTrackPriority( SDWORD iTrack );
SDWORD	sound_GetTrackAudibleRadius( SDWORD iTrack );
SDWORD	sound_GetTrackVolume( SDWORD iTrack );
const char *	sound_GetTrackName( SDWORD iTrack );
UDWORD	sound_GetTrackHashName( SDWORD iTrack );

BOOL	sound_TrackLooped( SDWORD iTrack );
SDWORD	sound_TrackAudibleRadius( SDWORD iTrack );
void	sound_SetCallbackFunction( void * fn );

BOOL	sound_Play2DTrack( AUDIO_SAMPLE *psSample, BOOL bQueued );
BOOL	sound_Play3DTrack( AUDIO_SAMPLE *psSample );
void	sound_PlayWithCallback( AUDIO_SAMPLE *psSample, SDWORD iCurTime,
									AUDIO_CALLBACK pDoneFunc );
void	sound_FinishedCallback( AUDIO_SAMPLE *psSample );

BOOL	sound_GetSystemActive( void );
SDWORD	sound_GetTrackID( TRACK *psTrack );
SDWORD	sound_GetAvailableID( void );
SDWORD	sound_GetNumPlaying( SDWORD iTrack );

SDWORD	sound_GetGlobalVolume( void );
void	sound_SetGlobalVolume( SDWORD iVol );

void	sound_SetStoppedCallback( AUDIO_CALLBACK pStopTrackCallback );

UDWORD	sound_GetTrackTimeLastFinished( SDWORD iTrack );
void	sound_SetTrackTimeLastFinished( SDWORD iTrack, UDWORD iTime );

/***************************************************************************/

#endif	// _TRACK_H_

/***************************************************************************/
