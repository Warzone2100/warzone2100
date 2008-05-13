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

#include <string.h>
#include <physfs.h>

#include "lib/framework/frame.h"

#include "audio.h"
#include "track.h"
#include "cdaudio.h"
#include "mixer.h"
#include "playlist.h"

static const size_t bufferSize = 16 * 1024;
static const unsigned int buffer_count = 32;
static bool		music_initialized;
static unsigned int	music_track = 0;
static float		music_volume = 0.5;

static AUDIO_STREAM* cdStream = NULL;

BOOL cdAudio_Open(const char* user_musicdir)
{
	PlayList_Init();

	if (user_musicdir == NULL
	 || !PlayList_Read(user_musicdir))
	{
		return false;
	}

	music_initialized = true;

	return true;
}

static void cdAudio_CloseTrack(void)
{
	cdAudio_Stop();

	if (music_track != 0)
	{
		music_track = 0;
	}
}

void cdAudio_Close(void)
{
	cdAudio_CloseTrack();
	PlayList_Quit();
	music_initialized = false;
}

static void cdAudio_TrackFinished(void*);

static bool cdAudio_OpenTrack(const char* filename)
{
	if (!music_initialized)
	{
		return false;
	}

	cdAudio_CloseTrack();

#ifndef WZ_NOSOUND
	if (strncasecmp(filename+strlen(filename)-4, ".ogg", 4) == 0)
	{
		PHYSFS_file* music_file = PHYSFS_openRead(filename);

		if (music_file == NULL)
		{
			debug(LOG_ERROR, "cdAudio_OpenTrack: Failed opening file %s, with error %s", filename, PHYSFS_getLastError());
			return false;
		}

		cdStream = sound_PlayStreamWithBuf(music_file, music_volume, cdAudio_TrackFinished, (char*)filename, bufferSize, buffer_count);
		if (cdStream == NULL)
		{
			PHYSFS_close(music_file);
			debug(LOG_ERROR, "Failed creating audio stream for %s", filename);
			return false;
		}

		return true;
	}
#endif

	return false; // unhandled
}

static void cdAudio_TrackFinished(void* user_data)
{
	const char* filename;

	// HACK: bail out when a song switch has taken place
	if (user_data != PlayList_CurrentSong())
	{
		return;
	}

	filename = PlayList_NextSong();
	
	// This pointer is now officially invalidated; so set it to NULL
	cdStream = NULL;

	if (filename == 0)
	{
		music_track = 0;
		return;
	}

	if (cdAudio_OpenTrack(filename))
	{
		debug(LOG_SOUND, "Now playing %s", filename);
	}
}

BOOL cdAudio_PlayTrack(SDWORD iTrack)
{
	cdAudio_CloseTrack();

	PlayList_SetTrack(iTrack);

	{
		const char* filename = PlayList_CurrentSong();

		for (;;)
		{
			if (filename == NULL)
			{
				music_track = 0;
				return false;
			}
			if (cdAudio_OpenTrack(filename))
			{
				music_track = iTrack;
				break;
			}
			else
			{
				return false; // break out to avoid infinite loops
			}

			filename = PlayList_NextSong();
		}
	}

	return true;
}

void cdAudio_Stop()
{
	if (cdStream)
	{
		sound_StopStream(cdStream);
		cdStream = NULL;
	}
}

void cdAudio_Pause()
{
	if (cdStream)
	{
		sound_PauseStream(cdStream);
	}
}

void cdAudio_Resume()
{
	if (cdStream)
	{
		sound_ResumeStream(cdStream);
	}
}

float sound_GetMusicVolume()
{
	return music_volume;
}

void sound_SetMusicVolume(float volume)
{
	music_volume = volume;

	// Keep volume in the range of 0.0 - 1.0
	if (music_volume < 0.0)
	{
		music_volume = 0.0;
	}
	else if (music_volume > 1.0)
	{
		music_volume = 1.0;
	}

	// Change the volume of the current stream as well (if any)
	if (cdStream)
	{
		sound_SetStreamVolume(cdStream, music_volume);
	}
}
