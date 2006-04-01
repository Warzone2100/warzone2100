/***************************************************************************/

#ifndef _AUDIO_H_
#define _AUDIO_H_

/***************************************************************************/



#include "track.h"

/***************************************************************************/

extern BOOL		audio_Init( AUDIO_CALLBACK pStopTrackCallback );
extern BOOL		audio_Update();
extern BOOL		audio_Shutdown();
extern BOOL		audio_Disabled( void );

extern BOOL		audio_LoadTrackFromFile( signed char szFileName[] );
extern void *	audio_LoadTrackFromBuffer( UBYTE *pBuffer, UDWORD udwSize );
extern BOOL		audio_SetTrackVals( char szFileName[], BOOL bLoop, int *piID,
					int iVol, int iPriority, int iAudibleRadius, int VagID );
extern BOOL		audio_SetTrackValsHashName( UDWORD hash, BOOL bLoop, int iTrack, int iVol,
							int iPriority, int iAudibleRadius, int VagID );
extern void		audio_ReleaseTrack( TRACK *psTrack );

extern BOOL		audio_PlayStaticTrack( SDWORD iX, SDWORD iY, int iTrack );
extern BOOL		audio_PlayObjStaticTrack( void * psObj, int iTrack );
extern BOOL		audio_PlayObjStaticTrackCallback( void * psObj, int iTrack,
									AUDIO_CALLBACK pUserCallback );
extern BOOL		audio_PlayObjDynamicTrack( void * psObj, int iTrack,
											AUDIO_CALLBACK pUserCallback );
extern BOOL		audio_PlayClusterDynamicTrack( void * psClusterObj,
								int iTrack, AUDIO_CALLBACK pUserCallback );
extern void		audio_StopObjTrack( void * psObj, int iTrack );
extern void		audio_PlayTrack( int iTrack );
extern void		audio_PlayCallbackTrack( int iTrack,
											AUDIO_CALLBACK pUserCallback );
extern BOOL		audio_PlayStream( char szFileName[], SDWORD iVol,
											AUDIO_CALLBACK pUserCallback );
extern void		audio_QueueTrack( SDWORD iTrack );
extern void		audio_QueueTrackMinDelay( SDWORD iTrack, UDWORD iMinDelay );
extern void		audio_QueueTrackMinDelayPos( SDWORD iTrack, UDWORD iMinDelay,
											SDWORD iX, SDWORD iY, SDWORD iZ);
extern void		audio_QueueTrackGroup( SDWORD iTrack, SDWORD iGroup );
extern void		audio_QueueTrackPos( SDWORD iTrack, SDWORD iX, SDWORD iY,
										SDWORD iZ );
extern void		audio_QueueTrackGroupPos( SDWORD iTrack, SDWORD iGroup,
										SDWORD iX, SDWORD iY, SDWORD iZ );
extern void		audio_PlayPreviousQueueTrack( void );
extern BOOL		audio_GetPreviousQueueTrackPos( SDWORD *iX, SDWORD *iY,
											SDWORD *iZ );
extern void		audio_StopTrack( int iTrack );

extern void		audio_SetTrackPan( int iTrack, int iPan );
extern void		audio_SetTrackVol( int iTrack, int iVol );
extern void		audio_SetTrackFreq( int iTrack, int iFreq );

extern void		audio_PauseAll( void );
extern void		audio_ResumeAll( void );
extern void		audio_StopAll( void );
extern void		audio_CheckAllUnloaded( void );

extern SDWORD	audio_GetTrackID( char szFileName[] );
extern SDWORD	audio_GetTrackIDFromHash( UDWORD hash );
extern SDWORD	audio_GetAvailableID( void );
extern SDWORD	audio_GetMixVol( SDWORD iVol );
extern SDWORD	audio_GetSampleMixVol( AUDIO_SAMPLE * psSample, SDWORD iVol,
										BOOL bScale3D );

extern SDWORD	audio_Get3DVolume( void );
extern void		audio_Set3DVolume( SDWORD iVol );

extern LPDIRECTSOUND	audio_GetDirectSoundObj( void );

/***************************************************************************/

#endif	// _AUDIO_H_

/***************************************************************************/
