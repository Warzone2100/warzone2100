/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/
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
#if 0
BOOL	audio_GetClusterCentre( void *psGroup, SDWORD *piX, SDWORD *piY,
								SDWORD *piZ );
BOOL	audio_GetNewClusterObject( void **psClusterObj, SDWORD iClusterID );
BOOL	audio_ClusterEmpty( void * psGroup );
SDWORD	audio_GetClusterIDFromObj( void *psClusterObj );
#endif
void	audio_Get2DPlayerPos( SDWORD *piX, SDWORD *piY, SDWORD *piZ );
void	audio_Get3DPlayerPos( SDWORD *piX, SDWORD *piY, SDWORD *piZ );
void	audio_Get2DPlayerRotAboutVerticalAxis( SDWORD *piA );
void	audio_Get3DPlayerRotAboutVerticalAxis( SDWORD *piA );
BOOL	audio_Display3D( void );

/***************************************************************************/

#endif		/* _AUD_H_ */

/***************************************************************************/
