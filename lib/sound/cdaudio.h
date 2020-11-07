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

#ifndef __INCLUDED_LIB_SOUND_CDAUDIO_H__
#define __INCLUDED_LIB_SOUND_CDAUDIO_H__

#include <string>
#include <memory>
#include <vector>
#include <functional>

enum SONG_CONTEXT
{
	SONG_FRONTEND,
	SONG_INGAME,
};

enum class MusicGameMode
{
	MENUS = -1,
	CAMPAIGN = 0,
	CHALLENGE,
	SKIRMISH,
	MULTIPLAYER
};
#define NUM_MUSICGAMEMODES 4
std::string to_string(MusicGameMode mode);

struct WZ_ALBUM; // forward-declare

struct WZ_TRACK
{
	std::string filename;
	std::string title;
	std::string author;
	unsigned int base_volume = 100;
	unsigned int bpm = 80;
	std::weak_ptr<WZ_ALBUM> album;
};

struct WZ_ALBUM
{
	std::string title;
	std::string author;
	std::string date;
	std::string description;
	std::string album_cover_filename;
	std::vector<std::shared_ptr<WZ_TRACK>> tracks;
};

class CDAudioEventSink
{
public:
	virtual ~CDAudioEventSink();
	virtual void startedPlayingTrack(const std::shared_ptr<const WZ_TRACK>& track);
	virtual void trackEnded(const std::shared_ptr<const WZ_TRACK>& track);
	virtual void musicStopped();
	virtual void musicPaused(const std::shared_ptr<const WZ_TRACK>& track);
	virtual void musicResumed(const std::shared_ptr<const WZ_TRACK>& track);
	virtual bool unregisterEventSink() const = 0;
};

bool cdAudio_Open(const char *user_musicdir);
void cdAudio_Close();
bool cdAudio_PlayTrack(SONG_CONTEXT context);
bool cdAudio_PlaySpecificTrack(const std::shared_ptr<const WZ_TRACK>& track);
SONG_CONTEXT cdAudio_CurrentSongContext();
void cdAudio_SetGameMode(MusicGameMode mode);
void cdAudio_NextTrack();
void cdAudio_Stop();
void cdAudio_Pause();
void cdAudio_Resume();

std::shared_ptr<const WZ_TRACK> cdAudio_GetCurrentTrack();
void cdAudio_RegisterForEvents(std::shared_ptr<CDAudioEventSink> musicEventSink);

#endif // __INCLUDED_LIB_SOUND_CDAUDIO_H__
