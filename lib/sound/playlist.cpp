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

#include <nlohmann/json.hpp> // Must come before WZ includes

#include "lib/framework/frame.h"
#include "lib/framework/file.h"
#include "lib/framework/string_ext.h"
#include "lib/framework/stdio_ext.h"
#include "lib/framework/physfs_ext.h"

#include "playlist.h"
#include "cdaudio.h"

#include <algorithm>
#include <unordered_map>
#include <sstream>
#include <limits>

#include <nonstd/optional.hpp>
using nonstd::optional;
using nonstd::nullopt;

// MARK: - WZ_TRACK_SETTINGS

struct WZ_TRACK_SETTINGS
{
	bool music_modes[NUM_MUSICGAMEMODES] = {false};

public:
	void setMusicMode(MusicGameMode mode, bool enabled)
	{
		music_modes[static_cast<std::underlying_type<MusicGameMode>::type>(mode)] = enabled;
	}
};

inline void to_json(nlohmann::json& j, const WZ_TRACK_SETTINGS& p)
{
	j = nlohmann::json::object();
	nlohmann::json music_modes = nlohmann::json::array();
	for (size_t i = 0; i < NUM_MUSICGAMEMODES; i++)
	{
		music_modes[i] = p.music_modes[i];
	}
	j["music_modes"] = music_modes;
}

inline void from_json(const nlohmann::json& j, WZ_TRACK_SETTINGS& p)
{
	if (!j.is_object())
	{
		throw nlohmann::json::type_error::create(302, "type must be an object, but is " + std::string(j.type_name()), j);
	}
	auto it = j.find("music_modes");
	if (it != j.end())
	{
		if (!it.value().is_array())
		{
			throw nlohmann::json::type_error::create(302, "music_modes type must be an array, but is " + std::string(j.type_name()), j);
		}
		size_t i = 0;
		for (const auto& v : it.value())
		{
			if (i >= NUM_MUSICGAMEMODES)
			{
				break;
			}
			p.music_modes[i] = v.get<bool>();
			i++;
		}
	}
}

struct WZ_TRACK_SETTINGS_PAIR
{
	WZ_TRACK_SETTINGS default_settings;
	WZ_TRACK_SETTINGS user_settings;
};

// MARK: - WZ_Playlist_Preferences

class WZ_Playlist_Preferences
{
public:
	WZ_Playlist_Preferences(const std::string &fileName);

	optional<WZ_TRACK_SETTINGS> getTrackSettings(const std::shared_ptr<const WZ_TRACK>& track);
	void setTrackSettings(const std::shared_ptr<const WZ_TRACK>& track, const WZ_TRACK_SETTINGS& settings);

	void clearAllPreferences();
	bool savePreferences();

private:
	std::string keyForTrack(const std::shared_ptr<const WZ_TRACK>& track) const;
	void storeValueForTrack(const std::shared_ptr<const WZ_TRACK>& track, const std::string& key, const nlohmann::json& value);
	optional<nlohmann::json> getJSONValueForTrack(const std::shared_ptr<const WZ_TRACK>& track, const std::string& key) const;

	template<typename T>
	optional<T> getValueForTrack(const std::shared_ptr<const WZ_TRACK>& track, const std::string& key) const
	{
		optional<T> result = nullopt;
		try {
			auto jsonResult = getJSONValueForTrack(track, key);
			if (!jsonResult.has_value())
			{
				return nullopt;
			}
			result = jsonResult.value().get<T>();
		}
		catch (const std::exception &e) {
			debug(LOG_WARNING, "Failed to convert json_variant to %s because of error: %s", typeid(T).name(), e.what());
			return nullopt;
		}
		catch (...) {
			debug(LOG_FATAL, "Unexpected exception encountered: json_variant::toType<%s>", typeid(T).name());
		}
		return result;
	}

private:
	nlohmann::json mRoot;
	std::string mFilename;
};

WZ_Playlist_Preferences::WZ_Playlist_Preferences(const std::string &fileName)
: mFilename(fileName)
{
	if (PHYSFS_exists(fileName.c_str()))
	{
		UDWORD size;
		char *data;
		if (loadFile(fileName.c_str(), &data, &size))
		{
			try {
				mRoot = nlohmann::json::parse(data, data + size);
			}
			catch (const std::exception &e) {
				ASSERT(false, "JSON document from %s is invalid: %s", fileName.c_str(), e.what());
			}
			catch (...) {
				debug(LOG_ERROR, "Unexpected exception parsing JSON %s", fileName.c_str());
			}
			ASSERT(!mRoot.is_null(), "JSON document from %s is null", fileName.c_str());
			ASSERT(mRoot.is_object(), "JSON document from %s is not an object. Read: \n%s", fileName.c_str(), data);
			free(data);
		}
		else
		{
			debug(LOG_ERROR, "Could not open \"%s\"", fileName.c_str());
			// treat as if no preferences exist yet
			mRoot = nlohmann::json::object();
		}
	}
	else
	{
		// no preferences exist yet
		mRoot = nlohmann::json::object();
	}

	// always ensure there's a "notifications" dictionary in the root object
	auto notificationsObject = mRoot.find("tracks");
	if (notificationsObject == mRoot.end() || !notificationsObject->is_object())
	{
		// create a dictionary object
		mRoot["tracks"] = nlohmann::json::object();
	}
}

std::string WZ_Playlist_Preferences::keyForTrack(const std::shared_ptr<const WZ_TRACK>& track) const
{
	// remove the "music/albums/" prefix
	std::string result = track->filename;
	if (result.rfind("music/albums/", 0) == 0)
	{
		result = result.substr(13);
	}
	return result;
}

void WZ_Playlist_Preferences::storeValueForTrack(const std::shared_ptr<const WZ_TRACK>& track, const std::string& key, const nlohmann::json& value)
{
	nlohmann::json& trackSettingsObj = mRoot["tracks"];
	std::string trackKey = keyForTrack(track);
	auto trackData = trackSettingsObj.find(trackKey);
	if (trackData == trackSettingsObj.end() || !trackData->is_object())
	{
		trackSettingsObj[trackKey] = nlohmann::json::object();
		trackData = trackSettingsObj.find(trackKey);
	}
	(*trackData)[key] = value;
}

optional<nlohmann::json> WZ_Playlist_Preferences::getJSONValueForTrack(const std::shared_ptr<const WZ_TRACK>& track, const std::string& key) const
{
	try {
		return mRoot.at("tracks").at(keyForTrack(track)).at(key);
	}
	catch (nlohmann::json::out_of_range&)
	{
		// some part of the path doesn't exist yet
		return nullopt;
	}
}

optional<WZ_TRACK_SETTINGS> WZ_Playlist_Preferences::getTrackSettings(const std::shared_ptr<const WZ_TRACK>& track)
{
	return getValueForTrack<WZ_TRACK_SETTINGS>(track, "settings");
}

void WZ_Playlist_Preferences::setTrackSettings(const std::shared_ptr<const WZ_TRACK>& track, const WZ_TRACK_SETTINGS& settings)
{
	storeValueForTrack(track, "settings", settings);
}

void WZ_Playlist_Preferences::clearAllPreferences()
{
	mRoot["tracks"] = nlohmann::json::object();
}

bool WZ_Playlist_Preferences::savePreferences()
{
	std::ostringstream stream;
	stream << mRoot.dump(4) << std::endl;
	std::string jsonString = stream.str();
#if SIZE_MAX >= UDWORD_MAX
	ASSERT_OR_RETURN(false, jsonString.size() <= static_cast<size_t>(std::numeric_limits<UDWORD>::max()), "jsonString.size (%zu) exceeds UDWORD::max", jsonString.size());
#endif
	saveFile(mFilename.c_str(), jsonString.c_str(), static_cast<UDWORD>(jsonString.size()));
	return true;
}

// MARK: - Globals

static std::vector<std::shared_ptr<WZ_ALBUM>> albumList;
static std::vector<std::shared_ptr<const WZ_TRACK>> fullTrackList;
typedef std::unordered_map<std::shared_ptr<const WZ_TRACK>, WZ_TRACK_SETTINGS_PAIR> TRACK_SETTINGS_MAP;
static TRACK_SETTINGS_MAP trackToSettingsMap;
static optional<size_t> currentSong = nullopt;
static MusicGameMode lastFilteredMode = MusicGameMode::MENUS;
static std::function<bool (const std::shared_ptr<const WZ_TRACK>& track)> currentFilterFunc;
static WZ_Playlist_Preferences* playlistPrefs = nullptr;

// MARK: - Core PlayList Functions

static void PlayList_ClearAll()
{
	albumList.clear();
	fullTrackList.clear();
	trackToSettingsMap.clear();
	currentFilterFunc = nullptr;
	currentSong = nullopt;
	if (playlistPrefs)
	{
		delete playlistPrefs;
		playlistPrefs = nullptr;
	}
}

void PlayList_Init()
{
	PlayList_ClearAll();
	playlistPrefs = new WZ_Playlist_Preferences("music.json");
}

void PlayList_Quit()
{
	if (playlistPrefs)
	{
		playlistPrefs->savePreferences();
	}
	PlayList_ClearAll();
}

bool PlayList_IsTrackEnabledForMusicMode(const std::shared_ptr<const WZ_TRACK>& track, MusicGameMode mode)
{
	auto it = trackToSettingsMap.find(track);
	if (it == trackToSettingsMap.end())
	{
		return false;
	}
	return it->second.user_settings.music_modes[static_cast<std::underlying_type<MusicGameMode>::type>(mode)];
}

void PlayList_SetTrackMusicMode(const std::shared_ptr<const WZ_TRACK>& track, MusicGameMode mode, bool enabled)
{
	auto it = trackToSettingsMap.find(track);
	if (it == trackToSettingsMap.end())
	{
		return;
	}
	bool oldValue = it->second.user_settings.music_modes[static_cast<std::underlying_type<MusicGameMode>::type>(mode)];
	if (oldValue != enabled)
	{
		it->second.user_settings.music_modes[static_cast<std::underlying_type<MusicGameMode>::type>(mode)] = enabled;
		if (playlistPrefs)
		{
			playlistPrefs->setTrackSettings(track, it->second.user_settings);
		}
	}
}

static void PlayList_SetTrackDefaultSettings(const std::shared_ptr<const WZ_TRACK>& track, const WZ_TRACK_SETTINGS& default_settings)
{
	auto it = trackToSettingsMap.find(track);
	if (it == trackToSettingsMap.end())
	{
		auto result = trackToSettingsMap.insert(TRACK_SETTINGS_MAP::value_type(track, WZ_TRACK_SETTINGS_PAIR()));
		it = result.first;
	}
	it->second.default_settings = default_settings;
	auto user_settings = playlistPrefs->getTrackSettings(track);
	if (user_settings.has_value())
	{
		// load the saved user settings
		it->second.user_settings = user_settings.value();
	}
	else
	{
		// initialize the user settings to the defaults
		it->second.user_settings = default_settings;
		playlistPrefs->setTrackSettings(track, it->second.user_settings);
	}
}

const std::vector<std::shared_ptr<const WZ_TRACK>>& PlayList_GetFullTrackList()
{
	return fullTrackList;
}

size_t PlayList_SetTrackFilter(const std::function<bool (const std::shared_ptr<const WZ_TRACK>& track)>& filterFunc)
{
	size_t tracksEnabled = 0;
	for (const auto& album : albumList)
	{
		for (const auto& track : album->tracks)
		{
			if (filterFunc(track))
			{
				++tracksEnabled;
			}
		}
	}
	currentFilterFunc = filterFunc;
	return tracksEnabled;
}

size_t PlayList_FilterByMusicMode(MusicGameMode currentMode)
{
	size_t result = 0;
	if (currentMode != MusicGameMode::MENUS)
	{
		result = PlayList_SetTrackFilter([currentMode](const std::shared_ptr<const WZ_TRACK>& track) -> bool {
			return PlayList_IsTrackEnabledForMusicMode(track, currentMode);
		});
	}
	else
	{
		currentFilterFunc = nullptr;
	}
	if (lastFilteredMode != currentMode)
	{
		currentSong = nullopt;
	}
	lastFilteredMode = currentMode;
	return result;
}

MusicGameMode PlayList_GetCurrentMusicMode()
{
	return lastFilteredMode;
}

static std::shared_ptr<WZ_ALBUM> PlayList_LoadAlbum(const nlohmann::json& json, const std::string& sourcePath, const std::string& albumDir)
{
	ASSERT(!json.is_null(), "JSON album from %s is null", sourcePath.c_str());
	ASSERT(json.is_object(), "JSON album from %s is not an object", sourcePath.c_str());

	std::shared_ptr<WZ_ALBUM> album = std::make_shared<WZ_ALBUM>();

#define GET_REQUIRED_ALBUM_KEY_STRVAL(KEY) \
	if (!json.contains(#KEY)) \
	{ \
		debug(LOG_ERROR, "%s: Missing required `" #KEY "` key", sourcePath.c_str()); \
		return nullptr; \
	} \
	try { \
		album->KEY = json[#KEY].get<std::string>(); \
	} catch (const std::exception &e) { \
		debug(LOG_ERROR, "%s: Failed to get `" #KEY "` key, error: %s", sourcePath.c_str(), e.what()); \
		return nullptr; \
	}

	GET_REQUIRED_ALBUM_KEY_STRVAL(title);
	GET_REQUIRED_ALBUM_KEY_STRVAL(author);
	GET_REQUIRED_ALBUM_KEY_STRVAL(date);
	GET_REQUIRED_ALBUM_KEY_STRVAL(description);
	GET_REQUIRED_ALBUM_KEY_STRVAL(album_cover_filename);
	if (!album->album_cover_filename.empty())
	{
		album->album_cover_filename = albumDir + "/" + album->album_cover_filename;
	}

	if (!json.contains("tracks"))
	{
		debug(LOG_ERROR, "Missing required `tracks` key");
		return nullptr;
	}
	if (!json["tracks"].is_array())
	{
		debug(LOG_ERROR, "Required key `tracks` should be an array");
		return nullptr;
	}

#define GET_REQUIRED_TRACK_KEY(KEY, TYPE) \
	if (!trackJson.contains(#KEY)) \
	{ \
		debug(LOG_ERROR, "%s: Missing required track key: `" #KEY "`", sourcePath.c_str()); \
		return nullptr; \
	} \
	try { \
		track->KEY = trackJson[#KEY].get<TYPE>(); \
	} catch (const std::exception &e) { \
		debug(LOG_ERROR, "%s: Failed to get track key `" #KEY "`, error: %s", sourcePath.c_str(), e.what()); \
		return nullptr; \
	}

	for (auto& trackJson : json["tracks"])
	{
		std::shared_ptr<WZ_TRACK> track = std::make_shared<WZ_TRACK>();
		GET_REQUIRED_TRACK_KEY(filename, std::string);
		if (track->filename.empty())
		{
			// Track must have a filename
			debug(LOG_ERROR, "%s: Empty or invalid filename for track", sourcePath.c_str());
			return nullptr;
		}
		track->filename = albumDir + "/" + track->filename;
		GET_REQUIRED_TRACK_KEY(title, std::string);
		GET_REQUIRED_TRACK_KEY(author, std::string);
		GET_REQUIRED_TRACK_KEY(base_volume, unsigned int);
		GET_REQUIRED_TRACK_KEY(bpm, unsigned int);
		track->album = std::weak_ptr<WZ_ALBUM>(album);

		WZ_TRACK_SETTINGS default_settings;
		// default_music_modes
		if (!trackJson.contains("default_music_modes"))
		{
			debug(LOG_ERROR, "%s: Missing required track key: `default_music_modes`", sourcePath.c_str());
			return nullptr;
		}
		if (!trackJson["default_music_modes"].is_array())
		{
			debug(LOG_ERROR, "%s: Required track key `default_music_modes` should be an array", sourcePath.c_str());
			return nullptr;
		}
		for (const auto& musicModeJSON : trackJson["default_music_modes"])
		{
			if (!musicModeJSON.is_string())
			{
				debug(LOG_ERROR, "%s: `default_music_modes` array item should be a string", sourcePath.c_str());
				continue;
			}
			std::string musicMode = musicModeJSON.get<std::string>();
			if (musicMode == "campaign")
			{
				default_settings.setMusicMode(MusicGameMode::CAMPAIGN, true);
			}
			else if (musicMode == "challenge")
			{
				default_settings.setMusicMode(MusicGameMode::CHALLENGE, true);
			}
			else if (musicMode == "skirmish")
			{
				default_settings.setMusicMode(MusicGameMode::SKIRMISH, true);
			}
			else if (musicMode == "multiplayer")
			{
				default_settings.setMusicMode(MusicGameMode::MULTIPLAYER, true);
			}
			else
			{
				debug(LOG_WARNING, "%s: `default_music_modes` array item is unknown value: %s", sourcePath.c_str(), musicMode.c_str());
				continue;
			}
		}
		PlayList_SetTrackDefaultSettings(track, default_settings);
		album->tracks.push_back(std::move(track));
	}

	return album;
}

bool PlayList_Read(const char *path)
{
	UDWORD size = 0;
	char *data = nullptr;
	std::string albumsPath = astringf("%s/albums", path);

	WZ_PHYSFS_enumerateFiles(albumsPath.c_str(), [&](const char *i) -> bool {
		std::string albumDir = albumsPath + "/" + i;
		std::string str = albumDir + "/album.json";
		if (!PHYSFS_exists(str.c_str()))
		{
			return true; // continue;
		}
		if (!loadFile(str.c_str(), &data, &size))
		{
			debug(LOG_ERROR, "album JSON file \"%s\" could not be opened!", str.c_str());
			return true; // continue;
		}
		nlohmann::json tmpJson;
		try {
			tmpJson = nlohmann::json::parse(data, data + size);
		}
		catch (const std::exception &e) {
			debug(LOG_ERROR, "album JSON file %s is invalid: %s", str.c_str(), e.what());
			return true; // continue;
		}
		catch (...) {
			debug(LOG_FATAL, "Unexpected exception parsing album JSON from %s", str.c_str());
		}
		ASSERT(!tmpJson.is_null(), "JSON album from %s is null", str.c_str());
		ASSERT(tmpJson.is_object(), "JSON album from %s is not an object. Read: \n%s", str.c_str(), data);
		free(data);

		// load album data
		auto album = PlayList_LoadAlbum(tmpJson, str, albumDir);
		if (!album)
		{
			debug(LOG_ERROR, "Failed to load album JSON: %s", str.c_str());
			return true; // continue;
		}
		albumList.push_back(std::move(album));
		return true; // continue
	});

	// Ensure that "Warzone 2100 OST" is always the first album in the list
	std::stable_sort(albumList.begin(), albumList.end(), [](const std::shared_ptr<WZ_ALBUM>& a, const std::shared_ptr<WZ_ALBUM>& b) -> bool {
		if (a == b) { return false; }
		if (a->title == "Warzone 2100 OST") { return true; }
		return false;
	});

	// generate full track list
	fullTrackList.clear();
	for (const auto& album : albumList)
	{
		for (const auto& track : album->tracks)
		{
			fullTrackList.push_back(track);
		}
	}
	currentSong = nullopt;

	return !albumList.empty();
}

static optional<size_t> PlayList_FindNextMatchingTrack(size_t startingSongIdx)
{
	if (!currentFilterFunc) { return nullopt; }
	if (fullTrackList.empty()) { return nullopt; }
	size_t idx = (startingSongIdx < (fullTrackList.size() - 1)) ? (startingSongIdx + 1) : 0;
	while (idx != startingSongIdx)
	{
		if (currentFilterFunc(fullTrackList[idx]))
		{
			return idx;
		}
		idx = (idx < (fullTrackList.size() - 1)) ? (idx + 1) : 0;
	}
	return nullopt;
}

std::shared_ptr<const WZ_TRACK> PlayList_CurrentSong()
{
	if (fullTrackList.empty())
	{
		return nullptr;
	}
	if (!currentSong.has_value() || (currentSong.value() >= fullTrackList.size()))
	{
		currentSong = PlayList_FindNextMatchingTrack(fullTrackList.size() - 1);
		if (!currentSong.has_value())
		{
			return nullptr;
		}
	}
	return fullTrackList[currentSong.value()];
}

std::shared_ptr<const WZ_TRACK> PlayList_NextSong()
{
	size_t currentSongIdx = currentSong.has_value() ? currentSong.value() : (fullTrackList.size() - 1);
	currentSong = PlayList_FindNextMatchingTrack(currentSongIdx);
	return PlayList_CurrentSong();
}

std::shared_ptr<const WZ_TRACK> PlayList_RandomizeCurrentSong()
{
	if (fullTrackList.empty())
	{
		return nullptr;
	}
	size_t incrementByNum = ((size_t)rand() % fullTrackList.size());
	size_t currentSongIdx = currentSong.has_value() ? currentSong.value() : (fullTrackList.size() - 1);
	for (size_t i = 0; i < incrementByNum; i++)
	{
		currentSong = PlayList_FindNextMatchingTrack(currentSongIdx);
		if (!currentSong.has_value())
		{
			break;
		}
		currentSongIdx = currentSong.value();
	}
	return PlayList_CurrentSong();
}

bool PlayList_SetCurrentSong(const std::shared_ptr<const WZ_TRACK>& track)
{
	if (!track) { return false; }
	optional<size_t> trackIdx = nullopt;
	for (size_t i = 0; i < fullTrackList.size(); i++)
	{
		if (track == fullTrackList[i])
		{
			trackIdx = i;
			break;
		}
	}
	currentSong = trackIdx;
	return trackIdx.has_value();
}
