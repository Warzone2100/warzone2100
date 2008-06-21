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
#include "lib/framework/frame.h"
#include "lib/framework/file.h"

#include "playlist.h"
#include "cdaudio.h"

#define BUFFER_SIZE 2048

typedef struct _wzTrack
{
	char		path[PATH_MAX];
	struct _wzTrack	*next;
} WZ_TRACK;

static WZ_TRACK *currentSong = NULL;
static int numSongs = 0;
static WZ_TRACK *songList = NULL;

void PlayList_Init()
{
	songList = NULL;
	currentSong = NULL;
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

bool PlayList_Read(const char* path)
{
	PHYSFS_file* fileHandle;
	char listName[PATH_MAX];

	// Construct file name
	ssprintf(listName, "%s/music.wpl", path);

	// Attempt to open the playlist file
	fileHandle = PHYSFS_openRead(listName);
	if (fileHandle == NULL)
	{
		debug(LOG_ERROR, "PHYSFS_openRead(\"%s\") failed with error: %s\n", listName, PHYSFS_getLastError());
		return false;
	}

	while (!PHYSFS_eof(fileHandle))
	{
		char line_buf[BUFFER_SIZE];
		size_t buf_pos = 0;
		char* filename;

		while (buf_pos < sizeof(line_buf) - 1
		    && PHYSFS_read(fileHandle, &line_buf[buf_pos], 1, 1)
		    && line_buf[buf_pos] != '\n'
		    && line_buf[buf_pos] != '\r')
		{
			++buf_pos;
		}

		// Nul-terminate string
		line_buf[buf_pos] = '\0';
		buf_pos = 0;

		if (line_buf[0] != '\0'
		    && (filename = strtok(line_buf, "\n")) != NULL
		    && strlen(filename) != 0)
		{
			WZ_TRACK *song = malloc(sizeof(*songList));
			WZ_TRACK *last = songList;

			sstrcpy(song->path, path);
			sstrcat(song->path, "/");
			sstrcat(song->path, filename);
			song->next = NULL;

			// find last
			for (; last && last->next; last = last->next);

			if (last)
			{
				last->next = song;	// add last in list
			}
			else
			{
				songList = song;	// create list
			}

			numSongs++;
			debug(LOG_SOUND, "Added song %s to playlist", filename);
		}
	}
	PHYSFS_close(fileHandle);
	currentSong = songList;

	return true;
}

const char* PlayList_CurrentSong()
{
	if (currentSong)
	{
		return currentSong->path;
	}
	else
	{
		return NULL;
	}
}

const char* PlayList_NextSong()
{
	if (currentSong)
	{
		currentSong = currentSong->next;
	}
	if (!currentSong)
	{
		currentSong = songList;
	}
	return PlayList_CurrentSong();
}

void playListTest()
{
	int i;

	for (i = 0; i < 10; i++)
	{
		PlayList_Quit();
		PlayList_Init();
		PlayList_Read("music");
		if (numSongs != 2)
		{
			debug(LOG_ERROR, "Use the default playlist for selftest!");
		}
		assert(songList);
		assert(numSongs == 2);
	}
}
