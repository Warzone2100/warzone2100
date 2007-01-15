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
/* Handles the two CD issue */
/* Alex McLean */

/* seems to be responsible for playing music in-game,
 * move that to somewhere else
 */

#include "lib/framework/frame.h"

#include "lib/sound/audio.h"
#include "lib/sound/track.h"
#include "cdspan.h"

static char		g_szCurDriveName[MAX_STR] = "";

void
cdspan_PlayInGameAudio( char szFileName[], SDWORD iVol )
{
	char szStream[MAX_STR];//	szDrive[MAX_STR] = "",
	BOOL	bPlaying = FALSE;

	audio_StopAll();

	sprintf( szStream, "%s%s", g_szCurDriveName, szFileName );
	bPlaying = audio_PlayStream( szStream, iVol, NULL );

	/* try playing from hard disk */
	if ( bPlaying == FALSE )
	{
		sprintf( szStream, "audio/%s", szFileName );
		bPlaying = audio_PlayStream( szStream, iVol, NULL );
	}
}
