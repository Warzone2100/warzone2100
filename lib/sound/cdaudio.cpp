/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2015  Warzone 2100 Project

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
#include "tracklib.h"
#include "cdaudio.h"
#include "mixer.h"
#include "playlist.h"

static float		music_volume = 0.5;

static const size_t bufferSize = 16 * 1024;
static const unsigned int buffer_count = 32;
static bool		music_initialized = false;
static bool		stopping = true;
static AUDIO_STREAM *cdStream = NULL;

bool cdAudio_Open(const char *user_musicdir)
{
	PlayList_Init();

	if (user_musicdir == NULL
	    || !PlayList_Read(user_musicdir))
	{
		return false;
	}

	debug(LOG_SOUND, "called(%s)", user_musicdir);

	music_initialized = true;
	stopping = true;

	return true;
}

void cdAudio_Close(void)
{
	debug(LOG_SOUND, "called");
	cdAudio_Stop();
	PlayList_Quit();

	music_initialized = false;
	stopping = true;
}

static void cdAudio_TrackFinished(void *);

static bool cdAudio_OpenTrack(const char *filename)
{
	if (!music_initialized)
	{
		return false;
	}

	debug(LOG_SOUND, "called(%s)", filename);
	cdAudio_Stop();

	if (strncasecmp(filename + strlen(filename) - 4, ".ogg", 4) == 0)
	{
		PHYSFS_file *music_file = PHYSFS_openRead(filename);

		debug(LOG_WZ, "Reading...[directory: %s] %s", PHYSFS_getRealDir(filename), filename);
		if (music_file == NULL)
		{
			debug(LOG_ERROR, "Failed opening file [directory: %s] %s, with error %s", PHYSFS_getRealDir(filename), filename, PHYSFS_getLastError());
			return false;
		}

		cdStream = sound_PlayStreamWithBuf(music_file, music_volume, cdAudio_TrackFinished, (char *)filename, bufferSize, buffer_count);
		if (cdStream == NULL)
		{
			PHYSFS_close(music_file);
			debug(LOG_ERROR, "Failed creating audio stream for %s", filename);
			return false;
		}

		debug(LOG_SOUND, "successful(%s)", filename);
		stopping = false;
		return true;
	}

	return false; // unhandled
}

static void cdAudio_TrackFinished(void *user_data)
{
	const char *filename = PlayList_NextSong();

	// This pointer is now officially invalidated; so set it to NULL
	cdStream = NULL;

	if (filename == NULL)
	{
		debug(LOG_ERROR, "Out of playlist?! was playing %s", (char *)user_data);
		return;
	}

	if (!stopping && cdAudio_OpenTrack(filename))
	{
		debug(LOG_SOUND, "Now playing %s (was playing %s)", filename, (char *)user_data);
	}
}

bool cdAudio_PlayTrack(SONG_CONTEXT context)
{
	debug(LOG_SOUND, "called(%d)", (int)context);

	switch (context)
	{
	case SONG_FRONTEND:
		return cdAudio_OpenTrack("music/menu.ogg");

	case SONG_INGAME:
		{
			const char *filename = PlayList_CurrentSong();

			if (filename == NULL)
			{
				return false;
			}

			return cdAudio_OpenTrack(filename);
		}
	}

	ASSERT(!"Invalid songcontext", "Invalid song context specified for playing: %u", (unsigned int)context);

	return false;
}

void cdAudio_Stop()
{
	stopping = true;
	debug(LOG_SOUND, "called, cdStream=%p", cdStream);

	if (cdStream)
	{
		sound_StopStream(cdStream);
		cdStream = NULL;
		sound_Update();
	}
}

void cdAudio_Pause()
{
	debug(LOG_SOUND, "called");
	if (cdStream)
	{
		sound_PauseStream(cdStream);
	}
}

void cdAudio_Resume()
{
	debug(LOG_SOUND, "called");
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
	// Keep volume in the range of 0.0 - 1.0
	music_volume = clipf(volume, 0.0f, 1.0f);

	// Change the volume of the current stream as well (if any)
	if (cdStream)
	{
		sound_SetStreamVolume(cdStream, music_volume);
	}
}
