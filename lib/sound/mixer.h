/***************************************************************************/

#ifndef _MIXER_H_
#define _MIXER_H_

/***************************************************************************/

#include "lib/framework/frame.h"
#include "audio.h"

/***************************************************************************/

BOOL	mixer_Open( void );
void	mixer_Close( void );
SDWORD	mixer_GetCDVolume( void );
void	mixer_SetCDVolume( SDWORD iVol );
SDWORD	mixer_GetWavVolume( void );
void	mixer_SetWavVolume( SDWORD iVol );
SDWORD	mixer_Get3dWavVolume( void );
void	mixer_Set3dWavVolume( SDWORD iVol );
void	mixer_SaveWinVols();
void	mixer_RestoreWinVols();
void	mixer_SaveIngameVols();
void	mixer_RestoreIngameVols();

/***************************************************************************/

#endif		// #ifndef _MIXER_H_

/***************************************************************************/
