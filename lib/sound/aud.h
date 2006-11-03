/***************************************************************************/
/*
 * Aud.h
 *
 * Audio wrapper functions
 *
 * Gareth Jones 16/12/97
 */
/***************************************************************************/

#ifndef _AUD_H_
#define _AUD_H_

/***************************************************************************/

#include "audio.h"

/***************************************************************************/

void	audio_GetObjectPos( void *psObj, SDWORD *piX, SDWORD *piY,
								SDWORD *piZ );
void	audio_GetStaticPos( SDWORD iWorldX, SDWORD iWorldY,
								SDWORD *piX, SDWORD *piY, SDWORD *piZ );
BOOL	audio_ObjectDead( void * psObj );
BOOL	audio_GetClusterCentre( void *psGroup, SDWORD *piX, SDWORD *piY,
								SDWORD *piZ );
BOOL	audio_GetNewClusterObject( void **psClusterObj, SDWORD iClusterID );
BOOL	audio_ClusterEmpty( void * psGroup );
SDWORD	audio_GetClusterIDFromObj( void *psClusterObj );
void	audio_Get2DPlayerPos( SDWORD *piX, SDWORD *piY, SDWORD *piZ );
void	audio_Get3DPlayerPos( SDWORD *piX, SDWORD *piY, SDWORD *piZ );
void	audio_Get2DPlayerRotAboutVerticalAxis( SDWORD *piA );
void	audio_Get3DPlayerRotAboutVerticalAxis( SDWORD *piA );
BOOL	audio_Display3D( void );
UWORD	audio_GetScreenWidth( void );
BOOL	audio_GetIDFromStr( char *pWavStr, SDWORD *piID );

/***************************************************************************/

#endif		/* _AUD_H_ */

/***************************************************************************/
