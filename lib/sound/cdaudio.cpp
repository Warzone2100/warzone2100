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

#include "lib/framework/frame.h"
#include "lib/framework/math_ext.h"

#include <string.h>
#include <physfs.h>
#include "lib/framework/physfs_ext.h"
#include "oggopus.h"
#include "oggvorbis.h"
#include "audio.h"
#include "track.h"
#include "tracklib.h"
#include "cdaudio.h"
#include "mixer.h"
#include "playlist.h"

#include <algorithm>

// MARK: - Globals

static float	music_volume = 0.5;
static bool		music_initialized = false;
static bool		stopping = true;
static bool		is_opening_new_track = false;
static bool		queued_play_track_while_loading = false;
static AUDIO_STREAM *cdStream = nullptr;

const char MENU_MUSIC[] = "music/menu.opus";
static SONG_CONTEXT currentSongContext = SONG_FRONTEND;
static std::shared_ptr<const WZ_TRACK> currentTrack;
static std::vector<std::shared_ptr<CDAudioEventSink>> registeredEventSinks;

// MARK: - Helpers

std::string to_string(MusicGameMode mode)
{
	switch (mode)
	{
		case MusicGameMode::MENUS:
			return _("Menu");
		case MusicGameMode::CAMPAIGN:
			return _("Campaign");
		case MusicGameMode::CHALLENGE:
			return _("Challenge");
		case MusicGameMode::SKIRMISH:
			return _("Skirmish");
		case MusicGameMode::MULTIPLAYER:
			return _("Multiplayer");
	}
	return ""; // silence warning
}

// MARK: - CDAudioEventSink

CDAudioEventSink::~CDAudioEventSink() { }
void CDAudioEventSink::startedPlayingTrack(const std::shared_ptr<const WZ_TRACK>& track) { }
void CDAudioEventSink::trackEnded(const std::shared_ptr<const WZ_TRACK>& track) { }
void CDAudioEventSink::musicStopped() { }
void CDAudioEventSink::musicPaused(const std::shared_ptr<const WZ_TRACK>& track) { }
void CDAudioEventSink::musicResumed(const std::shared_ptr<const WZ_TRACK>& track) { }

static inline void EventSinkCleanup()
{
	if (registeredEventSinks.empty()) { return; }
	registeredEventSinks.erase(
		std::remove_if(
			registeredEventSinks.begin(), registeredEventSinks.end(),
			[](const std::shared_ptr<CDAudioEventSink>& a) {
				if (!a) { return true; }
				if (a->unregisterEventSink()) { return true; }
				return false;
			}
		)
	, registeredEventSinks.end());
}

#define NOTIFY_MUSIC_EVENT(event) \
EventSinkCleanup(); \
for (auto sink : registeredEventSinks) \
{ \
	sink->event(); \
}

#define NOTIFY_MUSIC_EVENT_TRACK(event, currenttrack) \
EventSinkCleanup(); \
for (auto sink : registeredEventSinks) \
{ \
	sink->event(currenttrack); \
}

// MARK: - Core cdAudio functions

bool cdAudio_Open(const char *user_musicdir)
{
	PlayList_Init();

	if (user_musicdir == nullptr
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

static void cdAudio_Stop_Internal(bool suppressEvent = false)
{
	stopping = true;
	debug(LOG_SOUND, "called, cdStream=%p", static_cast<void *>(cdStream));

	if (cdStream)
	{
		sound_StopStream(cdStream);
		cdStream = nullptr;
		currentTrack = nullptr;
		sound_Update();
		if (!suppressEvent)
		{
			NOTIFY_MUSIC_EVENT(musicStopped);
		}
	}
}

static std::shared_ptr<WZ_TRACK> cdAudio_GetMenuTrack()
{
	std::shared_ptr<WZ_TRACK> pTrack = std::make_shared<WZ_TRACK>();
	pTrack->filename = MENU_MUSIC;
	pTrack->title = _("Menu Music");
	pTrack->author = "Martin Severn";
	pTrack->base_volume = 100;
	pTrack->bpm = 80;
	return pTrack;
}

static void cdAudio_TrackFinished(const std::shared_ptr<const WZ_TRACK>& track); // forward-declare

static float cdAudio_CalculateTrackVolume(const std::shared_ptr<const WZ_TRACK>& track)
{
	if (!track) { return music_volume; }
	if (track->base_volume == 100)
	{
		return music_volume;
	}
	float track_volume = music_volume * (float(track->base_volume) / float(100));
	// Keep volume in the range of 0.0 - 1.0
	track_volume = clipf(track_volume, 0.0f, 1.0f);
	return track_volume;
}

static bool cdAudio_OpenTrack(std::shared_ptr<const WZ_TRACK> track)
{
	const std::string& filename = track->filename;
	if (!music_initialized)
	{
		return false;
	}

	debug(LOG_SOUND, "called(%s)", filename.c_str());
	is_opening_new_track = true;
	cdAudio_Stop_Internal(true);
	is_opening_new_track = false;

	PlayList_SetCurrentSong(track);

	cdStream = sound_PlayStream(filename.c_str(), cdAudio_CalculateTrackVolume(track), 
						[track](const void*) { cdAudio_TrackFinished(track);}, nullptr);
	if (cdStream == nullptr)
	{
		debug(LOG_ERROR, "Failed creating audio stream for %s", filename.c_str());
		NOTIFY_MUSIC_EVENT(musicStopped);
		return false;
	}
	currentTrack = track;
	NOTIFY_MUSIC_EVENT_TRACK(startedPlayingTrack, track);

	debug(LOG_SOUND, "successful(%s)", filename.c_str());
	stopping = false;
	return true;
}

static std::shared_ptr<const WZ_TRACK> cdAudio_GetNextTrack()
{
	std::shared_ptr<const WZ_TRACK> nextTrack;
	switch (currentSongContext)
	{
	case SONG_FRONTEND:
		nextTrack = cdAudio_GetMenuTrack();
		break;

	case SONG_INGAME:
		nextTrack = PlayList_NextSong();
		break;
	}
	return nextTrack;
}

static void cdAudio_TrackFinished(const std::shared_ptr<const WZ_TRACK> &track)
{
	NOTIFY_MUSIC_EVENT_TRACK(trackEnded, track);

	// This pointer is now officially invalidated; so set it to NULL
	cdStream = nullptr;
	currentTrack = nullptr;

	if (is_opening_new_track)
	{
		return; // shortcut further processing
	}
	std::shared_ptr<const WZ_TRACK> nextTrack = cdAudio_GetNextTrack();

	if (!nextTrack)
	{
		if (!stopping)
		{
			debug(LOG_ERROR, "Out of playlist?! was playing %s", track->filename.c_str());
			NOTIFY_MUSIC_EVENT(musicStopped);
		}
		return;
	}

	if (!stopping && cdAudio_OpenTrack(nextTrack))
	{
		debug(LOG_SOUND, "Now playing %s (was playing %s)", nextTrack->filename.c_str(), track->filename.c_str());
	}
}

bool cdAudio_PlayTrack(SONG_CONTEXT context)
{
	debug(LOG_SOUND, "called(%d)", (int)context);
	currentSongContext = context;

	switch (context)
	{
	case SONG_FRONTEND:
		return cdAudio_OpenTrack(cdAudio_GetMenuTrack());

	case SONG_INGAME:
		{
			if (PlayList_GetCurrentMusicMode() == MusicGameMode::MENUS)
			{
				// Likely caused by loading a saved game, but we don't (yet) have the full game mode
				// As a workaround, queue playing the track
				queued_play_track_while_loading = true;
				return true;
			}

			auto nextTrack = PlayList_RandomizeCurrentSong();

			if (!nextTrack)
			{
				cdAudio_Stop();
				return false;
			}

			return cdAudio_OpenTrack(nextTrack);
		}
	}

	ASSERT(!"Invalid songcontext", "Invalid song context specified for playing: %u", (unsigned int)context);

	return false;
}

bool cdAudio_PlaySpecificTrack(const std::shared_ptr<const WZ_TRACK>& track)
{
	if (!track) { return false; }
	return cdAudio_OpenTrack(track);
}

SONG_CONTEXT cdAudio_CurrentSongContext()
{
	return currentSongContext;
}

void cdAudio_SetGameMode(MusicGameMode mode)
{
	auto oldMode = PlayList_GetCurrentMusicMode();
	if (oldMode != mode)
	{
		size_t numTracks = PlayList_FilterByMusicMode(mode);
		if ((numTracks == 0) && (mode != MusicGameMode::MENUS))
		{
			// no music configured for this game mode...
			debug(LOG_WARNING, "No music configured for current game mode");
		}
		if (queued_play_track_while_loading)
		{
			// Workaround for lack of proper game type before savegame is fully loaded
			queued_play_track_while_loading = false;
			cdAudio_PlayTrack(currentSongContext);
		}
	}
}

void cdAudio_NextTrack()
{
	cdAudio_OpenTrack(cdAudio_GetNextTrack());
}

void cdAudio_Stop()
{
	cdAudio_Stop_Internal();
}

void cdAudio_Pause()
{
	if (cdStream)
	{
		sound_PauseStream(cdStream);
		NOTIFY_MUSIC_EVENT_TRACK(musicPaused, currentTrack);
	}
}

void cdAudio_Resume()
{
	if (cdStream)
	{
		sound_ResumeStream(cdStream);
		NOTIFY_MUSIC_EVENT_TRACK(musicResumed, currentTrack);
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
		sound_SetStreamVolume(cdStream, cdAudio_CalculateTrackVolume(currentTrack));
	}
}

std::shared_ptr<const WZ_TRACK> cdAudio_GetCurrentTrack()
{
	return currentTrack;
}

void cdAudio_RegisterForEvents(std::shared_ptr<CDAudioEventSink> musicEventSink)
{
	if (!musicEventSink) { return; }
	if (musicEventSink->unregisterEventSink()) { return; }
	registeredEventSinks.push_back(musicEventSink);
}
