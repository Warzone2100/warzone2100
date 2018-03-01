/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
#include "lib/framework/frame.h"
#include "lib/framework/file.h"
#include "lib/framework/string_ext.h"
#include "lib/framework/stdio_ext.h"
#include "lib/framework/physfs_ext.h"

#include "playlist.h"
#include "cdaudio.h"

#define BUFFER_SIZE 2048

struct WZ_TRACK
{
	char		path[PATH_MAX];
	WZ_TRACK       *next;
};

static WZ_TRACK *currentSong = nullptr;
static int numSongs = 0;
static WZ_TRACK *songList = nullptr;

void PlayList_Init()
{
	songList = nullptr;
	currentSong = nullptr;
	numSongs = 0;
}

void PlayList_Quit()
{
	WZ_TRACK *list = songList;

	while (list)
	{
		WZ_TRACK *next = list->next;

		free(list);
		list = next;
	}

	PlayList_Init();
}

bool PlayList_Read(const char *path)
{
	WZ_TRACK **last = &songList;
	PHYSFS_file *fileHandle;
	char listName[PATH_MAX];

	// Construct file name
	ssprintf(listName, "%s/music.wpl", path);

	// Attempt to open the playlist file
	fileHandle = PHYSFS_openRead(listName);
	debug(LOG_WZ, "Reading...[directory: %s] %s", PHYSFS_getRealDir(listName), listName);
	if (fileHandle == nullptr)
	{
		debug(LOG_INFO, "PHYSFS_openRead(\"%s\") failed with error: %s\n", listName, WZ_PHYSFS_getLastError());
		return false;
	}

	// Find the end of the songList
	for (; *last; last = &(*last)->next) {}

	while (!PHYSFS_eof(fileHandle))
	{
		WZ_TRACK *song;
		char filename[BUFFER_SIZE];
		size_t buf_pos = 0;

		// Read a single line
		while (buf_pos < sizeof(filename) - 1
		       && WZ_PHYSFS_readBytes(fileHandle, &filename[buf_pos], 1)
		       && filename[buf_pos] != '\n'
		       && filename[buf_pos] != '\r')
		{
			++buf_pos;
		}

		// Nul-terminate string, and trim line endings ('\n' and '\r')
		filename[buf_pos] = '\0';

		// Don't add empty filenames to the playlist
		if (filename[0] == '\0' /* strlen(filename) == 0 */)
		{
			continue;
		}

		song = (WZ_TRACK *)malloc(sizeof(*songList));
		if (song == nullptr)
		{
			debug(LOG_FATAL, "Out of memory!");
			PHYSFS_close(fileHandle);
			abort();
			return false;
		}

		sstrcpy(song->path, path);
		sstrcat(song->path, "/");
		sstrcat(song->path, filename);
		song->next = nullptr;

		// Append this song to the list
		*last = song;
		last = &song->next;

		numSongs++;
		debug(LOG_SOUND, "Added song %s to playlist", filename);
	}
	PHYSFS_close(fileHandle);
	currentSong = songList;

	return true;
}

const char *PlayList_CurrentSong()
{
	if (currentSong)
	{
		return currentSong->path;
	}
	else
	{
		return nullptr;
	}
}

const char *PlayList_NextSong()
{
	// If there's a next song in the playlist select it
	if (currentSong
	    && currentSong->next)
	{
		currentSong = currentSong->next;
	}
	// Otherwise jump to the start of the playlist
	else
	{
		currentSong = songList;
	}

	return PlayList_CurrentSong();
}
