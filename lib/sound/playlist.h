/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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

#ifndef __INCLUDED_LIB_SOUND_PLAYLIST_H__
#define __INCLUDED_LIB_SOUND_PLAYLIST_H__

#include "lib/framework/types.h"
#include <memory>
#include "cdaudio.h"

void PlayList_Init();
void PlayList_Quit();
bool PlayList_Read(const char *path);
size_t PlayList_FilterByMusicMode(MusicGameMode currentMode);
MusicGameMode PlayList_GetCurrentMusicMode();

std::shared_ptr<const WZ_TRACK> PlayList_CurrentSong();
std::shared_ptr<const WZ_TRACK> PlayList_NextSong();
std::shared_ptr<const WZ_TRACK> PlayList_RandomizeCurrentSong();
bool PlayList_SetCurrentSong(const std::shared_ptr<const WZ_TRACK>& track);

const std::vector<std::shared_ptr<const WZ_TRACK>>& PlayList_GetFullTrackList();

bool PlayList_IsTrackEnabledForMusicMode(const std::shared_ptr<const WZ_TRACK>& track, MusicGameMode mode);
void PlayList_SetTrackMusicMode(const std::shared_ptr<const WZ_TRACK>& track, MusicGameMode mode, bool enabled);

#endif // __INCLUDED_LIB_SOUND_PLAYLIST_H__
