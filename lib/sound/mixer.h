/***************************************************************************/

#ifndef _MIXER_H_
#define _MIXER_H_

/***************************************************************************/

#include "lib/framework/frame.h"
#include "audio.h"

/***************************************************************************/

WZ_DECL_DEPRECATED BOOL	mixer_Open( void );
WZ_DECL_DEPRECATED void	mixer_Close( void );
SDWORD	mixer_GetCDVolume( void );
void	mixer_SetCDVolume( SDWORD iVol );
SDWORD	mixer_GetWavVolume( void );
void	mixer_SetWavVolume( SDWORD iVol );
SDWORD	mixer_Get3dWavVolume( void );
void	mixer_Set3dWavVolume( SDWORD iVol );
WZ_DECL_DEPRECATED void	mixer_SaveWinVols(void);
WZ_DECL_DEPRECATED void	mixer_RestoreWinVols(void);
WZ_DECL_DEPRECATED void	mixer_SaveIngameVols(void);
WZ_DECL_DEPRECATED void	mixer_RestoreIngameVols(void);

/***************************************************************************/

#endif		// #ifndef _MIXER_H_

/***************************************************************************/
