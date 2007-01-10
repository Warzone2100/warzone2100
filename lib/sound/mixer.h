/***************************************************************************/

#ifndef _MIXER_H_
#define _MIXER_H_

/***************************************************************************/

#include "lib/framework/frame.h"
#include "audio.h"

/***************************************************************************/

SDWORD	mixer_GetCDVolume( void );
void	mixer_SetCDVolume( SDWORD iVol );
SDWORD	mixer_GetWavVolume( void );
void	mixer_SetWavVolume( SDWORD iVol );
SDWORD	mixer_Get3dWavVolume( void );
void	mixer_Set3dWavVolume( SDWORD iVol );

/***************************************************************************/

#endif		// #ifndef _MIXER_H_

/***************************************************************************/
