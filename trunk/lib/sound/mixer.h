/***************************************************************************/

#ifndef _MIXER_H_
#define _MIXER_H_

/***************************************************************************/

#include "lib/framework/frame.h"
#include "audio.h"

/***************************************************************************/

WZ_DEPRECATED BOOL	mixer_Open( void );
WZ_DEPRECATED void	mixer_Close( void );
SDWORD	mixer_GetCDVolume( void );
void	mixer_SetCDVolume( SDWORD iVol );
SDWORD	mixer_GetWavVolume( void );
void	mixer_SetWavVolume( SDWORD iVol );
SDWORD	mixer_Get3dWavVolume( void );
void	mixer_Set3dWavVolume( SDWORD iVol );
WZ_DEPRECATED void	mixer_SaveWinVols();
WZ_DEPRECATED void	mixer_RestoreWinVols();
WZ_DEPRECATED void	mixer_SaveIngameVols();
WZ_DEPRECATED void	mixer_RestoreIngameVols();

/***************************************************************************/

#endif		// #ifndef _MIXER_H_

/***************************************************************************/
