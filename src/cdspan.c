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

/** Plays the given audio file as a background sound at the given volume.
 *  \param fileName the (OggVorbis) audio file to play from
 *  \param volume the volume to play at in a range of 0.0 to 1.0
 */
void cdspan_PlayInGameAudio(const char* fileName, float volume)
{
	BOOL	bPlaying;

	audio_StopAll();

	bPlaying = audio_PlayStream(fileName, volume, NULL, NULL);

	/* try playing from hard disk */
	if (bPlaying == FALSE)
	{
		const char* streamFileName;
		sasprintf((char**)&streamFileName, "audio/%s", fileName);

		audio_PlayStream(streamFileName, volume, NULL, NULL);
	}
}
