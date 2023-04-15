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
/**
 * @file configuration.cpp
 * Saves your favourite options.
 *
 */

#include "lib/framework/wzconfig.h"
#include "lib/framework/input.h"
#include "lib/framework/physfs_ext.h"
#include "lib/netplay/netplay.h"
#include "lib/sound/mixer.h"
#include "lib/sound/sounddefs.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/ivis_opengl/pieclip.h"
#include "lib/ivis_opengl/piestate.h" // for fog

#include "ai.h"
#include "component.h"
#include "configuration.h"
#include "difficulty.h"
#include "ingameop.h"
#include "multiint.h"
#include "multiplay.h"
#include "multistat.h"
#include "radar.h"
#include "seqdisp.h"
#include "texture.h"
#include "warzoneconfig.h"
#include "titleui/titleui.h"
#include "activity.h"
#include "nethelpers.h"
#include "lib/framework/wzapp.h"
#include "display3d.h" // for building animation speed
#include "display.h"
#include "keybind.h" // for MAP_ZOOM_RATE_STEP
#include "loadsave.h" // for autosaveEnabled
#include "clparse.h" // for autoratingUrl

#include <type_traits>

#include "mINI/ini.h"
#define PHYFSPP_IMPL
#include "3rdparty/physfs.hpp"

// ////////////////////////////////////////////////////////////////////////////

#define MASTERSERVERPORT	9990
#define GAMESERVERPORT		2100
#define BASECONFVERSION		1
#define CURRCONFVERSION		2

static const char *fileName = "config";

// ////////////////////////////////////////////////////////////////////////////

// PhysFS implementation of mINI::INIFileStreamGenerator

class PhysFSFileStreamGenerator : public mINI::INIFileStreamGenerator
{
public:
	PhysFSFileStreamGenerator(std::string const& utf8Path)
	: INIFileStreamGenerator(utf8Path)
	{ }
	virtual ~PhysFSFileStreamGenerator() { }
public:
	virtual std::shared_ptr<std::istream> getFileReadStream() const override
	{
		if (utf8Path.empty())
		{
			return nullptr;
		}
		try {
			return std::static_pointer_cast<std::istream>(PhysFS::ifstream::make(utf8Path));
		}
		catch (const std::exception&)
		{
			// file likely does not exist
			return nullptr;
		}
	}
	virtual std::shared_ptr<std::ostream> getFileWriteStream() const override
	{
		if (utf8Path.empty())
		{
			return nullptr;
		}
		try {
			return std::static_pointer_cast<std::ostream>(PhysFS::ofstream::make(utf8Path));
		}
		catch (const std::exception&)
		{
			// file likely does not exist
			return nullptr;
		}
	}
	virtual bool fileExists() const override
	{
		if (utf8Path.empty())
		{
			return false;
		}
		return PHYSFS_exists(utf8Path.c_str());
	}
	optional<uint64_t> fileSize() const
	{
#if defined(WZ_PHYSFS_2_1_OR_GREATER)
		PHYSFS_Stat metaData;
		if (PHYSFS_stat(utf8Path.c_str(), &metaData) == 0)
		{
			return nullopt; // failed to get file info
		}
		if (metaData.filesize < 0)
		{
			// unknown filesize
			return nullopt;
		}
		return static_cast<uint64_t>(metaData.filesize);
#else
		return nullopt; // unknown
#endif
	}
	std::string realPath() const
	{
		std::string fullPath = WZ_PHYSFS_getRealDir_String(utf8Path.c_str());
		if (fullPath.empty()) { return fullPath; }
		fullPath += PHYSFS_getDirSeparator();
		fullPath += utf8Path;
		return fullPath;
	}
};

// ////////////////////////////////////////////////////////////////////////////

typedef mINI::INIMap<std::string> IniSection;

static optional<int> iniSectionGetInteger(const IniSection& iniSection, const std::string& key, optional<int> defaultValue = nullopt)
{
	if (!iniSection.has(key))
	{
		return defaultValue;
	}
	try {
		auto valueStr = iniSection.get(key);
		int valueInt = std::stoi(valueStr);
		return valueInt;
	}
	catch (const std::exception& e)
	{
		debug(LOG_ERROR, "Failed to convert value for key: \"%s\" to integer; error: %s", key.c_str(), e.what());
		return defaultValue;
	}
}

static void iniSectionSetInteger(IniSection& iniSection, const std::string& key, int value)
{
	iniSection[key] = std::to_string(value);
}

static optional<bool> iniSectionGetBool(const IniSection& iniSection, const std::string& key, optional<bool> defaultValue = nullopt)
{
	if (!iniSection.has(key))
	{
		return defaultValue;
	}
	auto valueStr = WzString::fromUtf8(iniSection.get(key)).toLower();
	// first check if it's equal to "true" or "false" (case-insensitive)
	if (valueStr == "true")
	{
		return true;
	}
	else if (valueStr == "false")
	{
		return false;
	}
	else
	{
		// check for 1 or 0
		if (auto valueInt = iniSectionGetInteger(iniSection, key))
		{
			if (valueInt.value() == 1)
			{
				return true;
			}
			else if (valueInt.value() == 0)
			{
				return false;
			}
		}
	}
	return defaultValue;
}

static void iniSectionSetBool(IniSection& iniSection, const std::string& key, bool value)
{
	iniSection[key] = (value) ? "true" : "false";
}

static optional<std::string> iniSectionGetString(const IniSection& iniSection, const std::string& key, optional<std::string> defaultValue = nullopt)
{
	if (!iniSection.has(key))
	{
		return defaultValue;
	}
	std::string result = iniSection.get(key);
	// To support prior INI files written by QSettings, strip surrounding "" if present
	if (!result.empty() && result.front() == '"' && result.back() == '"')
	{
		if (result.size() <= 2)
		{
			return std::string();
		}
		result = result.substr(1, result.size() - 2);
	}
	return result;
}

bool saveIniFile(mINI::INIFile &file, mINI::INIStructure &ini)
{
	// write out ini file changes
	try
	{
		if (!file.write(ini))
		{
			debug(LOG_INFO, "Could not write configuration file \"%s\"", fileName);
			return false;
		}
	}
	catch (const std::exception& e)
	{
		debug(LOG_ERROR, "Ini write failed with exception: %s", e.what());
		return false;
	}
	return true;
}

constexpr uint64_t MAX_CONFIG_FILE_SIZE = 1024 * 1024 * 2; // 2 MB seems like enough...

// ////////////////////////////////////////////////////////////////////////////
bool loadConfig()
{
	// first, create a file instance
	auto fileStreamGenerator = std::make_shared<PhysFSFileStreamGenerator>(fileName);
	mINI::INIFile file(fileStreamGenerator);

	// next, create a structure that will hold data
	mINI::INIStructure ini;
	bool createdConfigFile = false;

	// now we can read the file
	try
	{
		uint64_t fileSize = fileStreamGenerator->fileSize().value_or(0);
		if (fileSize > MAX_CONFIG_FILE_SIZE)
		{
			createdConfigFile = true;
			debug(LOG_ERROR, "Could not read existing configuration file \"%s\"; filesize (%" PRIu64 ") exceeds max", fileName, fileSize);
			// will just proceed with an empty ini structure
		}
		if (!createdConfigFile && !file.read(ini))
		{
			createdConfigFile = true;
			debug(LOG_WZ, "Could not read existing configuration file \"%s\"", fileName);
			// will just proceed with an empty ini structure
		}
	}
	catch (const std::exception& e)
	{
		createdConfigFile = true;
		debug(LOG_ERROR, "Ini read failed with exception: %s", e.what());
		ini.clear();
		// will just proceed with an empty ini structure
	}

	auto& iniGeneral = ini["General"];

	auto iniGetInteger = [&iniGeneral](const std::string& key, optional<int> defaultValue) -> optional<int> {
		return iniSectionGetInteger(iniGeneral, key, defaultValue);
	};

	auto iniGetIntegerOpt = [&iniGeneral](const std::string& key) -> optional<int> {
		return iniSectionGetInteger(iniGeneral, key);
	};

	auto iniGetBool = [&iniGeneral](const std::string& key, optional<bool> defaultValue) -> optional<bool> {
		return iniSectionGetBool(iniGeneral, key, defaultValue);
	};

	auto iniGetBoolOpt = [&iniGeneral](const std::string& key) -> optional<bool> {
		return iniSectionGetBool(iniGeneral, key);
	};

	auto iniGetString = [&iniGeneral](const std::string& key, optional<std::string> defaultValue) -> optional<std::string> {
		return iniSectionGetString(iniGeneral, key, defaultValue);
	};

	ActivityManager::instance().beginLoadingSettings();

	debug(LOG_WZ, "Reading configuration from: %s", fileStreamGenerator->realPath().c_str());
	if (auto value = iniGetIntegerOpt("voicevol"))
	{
		sound_SetUIVolume(static_cast<float>(value.value()) / 100.0f);
	}
	if (auto value = iniGetIntegerOpt("fxvol"))
	{
		sound_SetEffectsVolume(static_cast<float>(value.value()) / 100.0f);
	}
	if (auto value = iniGetIntegerOpt("cdvol"))
	{
		sound_SetMusicVolume(static_cast<float>(value.value()) / 100.0f);
	}
	if (auto value = iniGetBoolOpt("music_enabled"))
	{
		war_SetMusicEnabled(value.value());
	}
	if (auto value = iniGetIntegerOpt("hrtf"))
	{
		int hrtfmode_int = value.value();
		if (hrtfmode_int >= static_cast<int>(MIN_VALID_HRTFMode) && hrtfmode_int <= static_cast<int>(MAX_VALID_HRTFMode))
		{
			war_SetHRTFMode(static_cast<HRTFMode>(hrtfmode_int));
		}
	}
	if (auto value = iniGetIntegerOpt("mapZoom"))
	{
		int v = value.value();
		war_SetMapZoom((v % MAP_ZOOM_CONFIG_STEP != 0) ? STARTDISTANCE : v);
	}
	if (auto value = iniGetIntegerOpt("mapZoomRate"))
	{
		int v = value.value();
		war_SetMapZoomRate((v % MAP_ZOOM_RATE_STEP != 0) ? MAP_ZOOM_RATE_DEFAULT : v);
	}
	if (auto value = iniGetIntegerOpt("radarZoom"))
	{
		int v = value.value();
		war_SetRadarZoom((v % RADARZOOM_STEP != 0) ? DEFAULT_RADARZOOM : v);
	}
	if (iniGeneral.has("language"))
	{
		setLanguage(iniGeneral.get("language").c_str());
	}
	if (auto value = iniGetBoolOpt("nomousewarp"))
	{
		setMouseWarp(value.value());
	}
	wz_texture_compression = iniGetBool("textureCompression", true).value();
	showFPS = iniGetBool("showFPS", false).value();
	showUNITCOUNT = iniGetBool("showUNITCOUNT", false).value();
	if (auto value = iniGetIntegerOpt("cameraSpeed"))
	{
		int v = value.value();
		war_SetCameraSpeed((v % CAMERASPEED_STEP != 0) ? CAMERASPEED_DEFAULT : v);
	}
	setShakeStatus(iniGetBool("shake", false).value());
	setCameraAccel(iniGetBool("cameraAccel", true).value());
	setDrawShadows(iniGetBool("shadows", true).value());
	war_setSoundEnabled(iniGetBool("sound", true).value());
	setInvertMouseStatus(iniGetBool("mouseflip", true).value());
	setRightClickOrders(iniGetBool("RightClickOrders", false).value());
	setMiddleClickRotate(iniGetBool("MiddleClickRotate", false).value());
	if (auto value = iniGetIntegerOpt("cursorScale"))
	{
		war_setCursorScale(value.value());
	}
	if (auto value = iniGetBoolOpt("radarJump"))
	{
		war_SetRadarJump(value.value());
	}
	if (auto value = iniGetIntegerOpt("lodDistanceBias"))
	{
		war_setLODDistanceBiasPercentage(value.value());
	}
	rotateRadar = iniGetBool("rotateRadar", true).value();
	radarRotationArrow = iniGetBool("radarRotationArrow", true).value();
	hostQuitConfirmation = iniGetBool("hostQuitConfirmation", true).value();
	war_SetPauseOnFocusLoss(iniGetBool("PauseOnFocusLoss", false).value());
	setAutoratingUrl(iniGetString("autoratingUrlV2", WZ_DEFAULT_PUBLIC_RATING_LOOKUP_SERVICE_URL).value());
	setAutoratingEnable(iniGetBool("autorating", false).value());
	NETsetMasterserverName(iniGetString("masterserver_name", "lobby.wz2100.net").value().c_str());
	mpSetServerName(iniGetString("server_name", "").value().c_str());
//	iV_font(ini.value("fontname", "DejaVu Sans").toString().toUtf8().constData(),
//	        ini.value("fontface", "Book").toString().toUtf8().constData(),
//	        ini.value("fontfacebold", "Bold").toString().toUtf8().constData());
	NETsetMasterserverPort(iniGetInteger("masterserver_port", MASTERSERVERPORT).value());
	if(!netGameserverPortOverride)  // do not load the config port setting if there's a command-line override
	{
		NETsetGameserverPort(iniGetInteger("gameserver_port", GAMESERVERPORT).value());
	}
	NETsetJoinPreferenceIPv6(iniGetBool("prefer_ipv6", true).value());
	setPublicIPv4LookupService(iniGetString("publicIPv4LookupService_Url", WZ_DEFAULT_PUBLIC_IPv4_LOOKUP_SERVICE_URL).value(), iniGetString("publicIPv4LookupService_JSONKey", WZ_DEFAULT_PUBLIC_IPv4_LOOKUP_SERVICE_JSONKEY).value());
	setPublicIPv6LookupService(iniGetString("publicIPv6LookupService_Url", WZ_DEFAULT_PUBLIC_IPv6_LOOKUP_SERVICE_URL).value(), iniGetString("publicIPv6LookupService_JSONKey", WZ_DEFAULT_PUBLIC_IPv6_LOOKUP_SERVICE_JSONKEY).value());
	war_SetFMVmode((FMV_MODE)iniGetInteger("FMVmode", FMV_FULLSCREEN).value());
	war_setScanlineMode((SCANLINE_MODE)iniGetInteger("scanlines", SCANLINES_OFF).value());
	seq_SetSubtitles(iniGetBool("subtitles", true).value());
	setDifficultyLevel((DIFFICULTY_LEVEL)iniGetInteger("difficulty", DL_NORMAL).value());
	if (!createdConfigFile && iniGetInteger("configVersion", BASECONFVERSION).value() < CURRCONFVERSION)
	{
		int level = (int)getDifficultyLevel() + 1;
		if (level > static_cast<int>(AIDifficulty::INSANE))
		{
			level = static_cast<int>(AIDifficulty::INSANE);
		}
		else if (level < static_cast<int>(AIDifficulty::SUPEREASY))
		{
			level = static_cast<int>(AIDifficulty::SUPEREASY);
		}
		setDifficultyLevel(static_cast<DIFFICULTY_LEVEL>(level));
	}
	war_SetSPcolor(iniGetInteger("colour", 0).value());	// default is green (0)
	war_setMPcolour(iniGetInteger("colourMP", -1).value());  // default is random (-1)
	sstrcpy(game.name, iniGetString("gameName", _("My Game")).value().c_str());
	sstrcpy(sPlayer, iniGetString("playerName", _("Player")).value().c_str());

	// Set a default map to prevent hosting games without a map.
	sstrcpy(game.map, DEFAULTSKIRMISHMAP);
	game.hash.setZero();
	game.maxPlayers = DEFAULTSKIRMISHMAPMAXPLAYERS;

	game.techLevel = 1;

	game.power = iniGetInteger("powerLevel", LEV_MED).value();
	game.base = iniGetInteger("base", CAMP_BASE).value();
	game.alliance = iniGetInteger("alliance", NO_ALLIANCES).value();
	game.scavengers = iniGetInteger("newScavengers", SCAVENGERS).value();
	war_setMPInactivityMinutes(iniGetInteger("inactivityMinutesMP", war_getMPInactivityMinutes()).value());
	game.inactivityMinutes = war_getMPInactivityMinutes();
	bEnemyAllyRadarColor = iniGetBool("radarObjectMode", false).value();
	radarDrawMode = (RADAR_DRAW_MODE)iniGetInteger("radarTerrainMode", RADAR_MODE_DEFAULT).value();
	radarDrawMode = (RADAR_DRAW_MODE)MIN(NUM_RADAR_MODES - 1, radarDrawMode); // restrict to allowed values
	if (auto value = iniGetIntegerOpt("textureSize"))
	{
		setTextureSize(value.value());
	}
	NetPlay.isUPNP = iniGetBool("UPnP", true).value();
	if (auto value = iniGetIntegerOpt("antialiasing"))
	{
		war_setAntialiasing(value.value());
	}
	if (auto value = iniGetIntegerOpt("fullscreen"))
	{
		int fullscreenmode_int = value.value();
		if (fullscreenmode_int >= static_cast<int>(MIN_VALID_WINDOW_MODE) && fullscreenmode_int <= static_cast<int>(MAX_VALID_WINDOW_MODE))
		{
			war_setWindowMode(static_cast<WINDOW_MODE>(fullscreenmode_int));
		}
	}
	war_SetTrapCursor(iniGetBool("trapCursor", false).value());
	war_SetColouredCursor(iniGetBool("coloredCursor", true).value());
	// this should be enabled on all systems by default
	war_SetVsync(iniGetInteger("vsync", 1).value());
	// the default (and minimum) display scale is 100 (%)
	int displayScale = iniGetInteger("displayScale", war_GetDisplayScale()).value();
	if (displayScale < 100)
	{
		displayScale = 100;
	}
	if (displayScale > 500) // Maximum supported display scale of 500%. (Further testing needed for anything greater.)
	{
		displayScale = 500;
	}
	war_SetDisplayScale(static_cast<unsigned int>(displayScale));
	war_setAutoAdjustDisplayScale(iniGetBool("autoAdjustDisplayScale", true).value());
	// 640x480 is minimum that we will support, but default to something more sensible
	int width = iniGetInteger("width", war_GetWidth()).value();
	int height = iniGetInteger("height", war_GetHeight()).value();
	int screen = iniGetInteger("screen", 0).value();
	if (width < 640 || height < 480)	// sanity check
	{
		width = 640;
		height = 480;
	}
	pie_SetVideoBufferWidth(width);
	pie_SetVideoBufferHeight(height);
	war_SetWidth(width);
	war_SetHeight(height);
	war_SetScreen(screen);

	int fullscreenWidth = iniGetInteger("fullscreenWidth", war_GetFullscreenModeWidth()).value();
	int fullscreenHeight = iniGetInteger("fullscreenHeight", war_GetFullscreenModeHeight()).value();
	int fullscreenScreen = iniGetInteger("fullscreenScreen", 0).value();
	if ((fullscreenWidth != 0 && fullscreenWidth < 640) || (fullscreenHeight != 0 && fullscreenHeight < 480))	// sanity check
	{
		// set to special value (0x0) that reverts to the default for this display
		fullscreenWidth = 0;
		fullscreenHeight = 0;
	}
	war_SetFullscreenModeWidth(fullscreenWidth);
	war_SetFullscreenModeHeight(fullscreenHeight);
	war_SetFullscreenModeScreen(fullscreenScreen);

	if (auto value = iniGetIntegerOpt("bpp"))
	{
		war_SetVideoBufferDepth(value.value());
	}

	video_backend gfxBackend;
	if (iniGeneral.has("gfxbackend"))
	{
		std::string gfxbackendStr = iniGetString("gfxbackend", "").value();
		if (!video_backend_from_str(gfxbackendStr.c_str(), gfxBackend))
		{
			gfxBackend = wzGetDefaultGfxBackendForCurrentSystem();
			debug(LOG_WARNING, "Unsupported / invalid gfxbackend value: %s; defaulting to: %s", gfxbackendStr.c_str(), to_string(gfxBackend).c_str());
		}
	}
	else
	{
		gfxBackend = wzGetDefaultGfxBackendForCurrentSystem();
	}
	war_setGfxBackend(gfxBackend);
	if (auto value = iniGetIntegerOpt("minimizeOnFocusLoss"))
	{
		war_setMinimizeOnFocusLoss(value.value());
	}
	if (auto value = iniGetIntegerOpt("altEnterToggleMode"))
	{
		war_setToggleFullscreenMode(value.value());
	}
	auto js_backend = war_getJSBackend();
	if (iniGeneral.has("jsbackend"))
	{
		std::string jsbackendStr = iniGetString("jsbackend", "").value();
		if (!js_backend_from_str(jsbackendStr.c_str(), js_backend))
		{
			js_backend = (JS_BACKEND)0; // use the first available option, whatever it is
			debug(LOG_WARNING, "Unsupported / invalid jsbackend value: %s; defaulting to: %s", jsbackendStr.c_str(), to_string(js_backend).c_str());
		}
	}
	BlueprintTrackAnimationSpeed = iniGetInteger("BlueprintTrackAnimationSpeed", 20).value();
	lockCameraScrollWhileRotating = iniGetBool("lockCameraScrollWhileRotating", false).value();
	autosaveEnabled = iniGetBool("autosaveEnabled", true).value();
	bool fogEnabled = iniGetBool("fog", false).value();
	if (fogEnabled)
	{
		pie_EnableFog(true);
	}
	else
	{
		pie_SetFogStatus(false);
		pie_EnableFog(false);
	}
	war_setAutoLagKickSeconds(iniGetInteger("hostAutoLagKickSeconds", war_getAutoLagKickSeconds()).value());
	war_setDisableReplayRecording(iniGetBool("disableReplayRecord", war_getDisableReplayRecording()).value());
	war_setMaxReplaysSaved(iniGetInteger("maxReplaysSaved", war_getMaxReplaysSaved()).value());
	war_setOldLogsLimit(iniGetInteger("oldLogsLimit", war_getOldLogsLimit()).value());
	int openSpecSlotsIntValue = iniGetInteger("openSpectatorSlotsMP", war_getMPopenSpectatorSlots()).value();
	war_setMPopenSpectatorSlots(static_cast<uint16_t>(std::max<int>(0, std::min<int>(openSpecSlotsIntValue, MAX_SPECTATOR_SLOTS))));
	war_setFogEnd(iniGetInteger("fogEnd", 8000).value());
	war_setFogStart(iniGetInteger("fogStart", 4000).value());
	ActivityManager::instance().endLoadingSettings();
	return true;
}

// ////////////////////////////////////////////////////////////////////////////
bool saveConfig()
{
	// first, create a file instance
	auto fileStreamGenerator = std::make_shared<PhysFSFileStreamGenerator>(fileName);
	mINI::INIFile file(fileStreamGenerator);

	// next, create a structure that will hold data
	mINI::INIStructure ini;

	// read in the current file
	try
	{
		uint64_t fileSize = fileStreamGenerator->fileSize().value_or(0);
		bool skipLoadExisting = false;
		if (fileSize > MAX_CONFIG_FILE_SIZE)
		{
			skipLoadExisting = true;
			debug(LOG_ERROR, "Could not read existing configuration file \"%s\"; filesize (%" PRIu64 ") exceeds max", fileName, fileSize);
			// will just proceed with an empty ini structure
		}
		if (!skipLoadExisting && !file.read(ini))
		{
			debug(LOG_WZ, "Could not read existing configuration file \"%s\"", fileName);
			// will just proceed with an empty ini structure
		}
	}
	catch (const std::exception& e)
	{
		debug(LOG_ERROR, "Ini read failed with exception: %s", e.what());
		ini.clear();
		// will just proceed with an empty ini structure
	}

	std::string fullConfigFilePath;
	if (PHYSFS_getWriteDir())
	{
		fullConfigFilePath += PHYSFS_getWriteDir();
		fullConfigFilePath += "/";
	}
	fullConfigFilePath += fileName;
	debug(LOG_WZ, "Writing configuration to: \"%s\"", fullConfigFilePath.c_str());

	auto& iniGeneral = ini["General"];

	auto iniSetInteger = [&iniGeneral](const std::string& key, int value) {
		iniSectionSetInteger(iniGeneral, key, value);
	};
	auto iniSetBool = [&iniGeneral](const std::string& key, bool value) {
		iniSectionSetBool(iniGeneral, key, value);
	};
	auto iniSetString = [&iniGeneral](const std::string& key, const std::string& value) {
		iniGeneral[key] = value;
	};

	// //////////////////////////
	// voicevol, fxvol and cdvol
	iniSetInteger("voicevol", (int)(sound_GetUIVolume() * 100.0));
	iniSetInteger("fxvol", (int)(sound_GetEffectsVolume() * 100.0));
	iniSetInteger("cdvol", (int)(sound_GetMusicVolume() * 100.0));
	iniSetBool("music_enabled", war_GetMusicEnabled());
	iniSetInteger("hrtf", static_cast<typename std::underlying_type<HRTFMode>::type>(war_GetHRTFMode()));
	iniSetInteger("mapZoom", war_GetMapZoom());
	iniSetInteger("mapZoomRate", war_GetMapZoomRate());
	iniSetInteger("radarZoom", war_GetRadarZoom());
	iniSetInteger("width", war_GetWidth());
	iniSetInteger("height", war_GetHeight());
	iniSetInteger("screen", war_GetScreen());
	iniSetInteger("fullscreenWidth", war_GetFullscreenModeWidth());
	iniSetInteger("fullscreenHeight", war_GetFullscreenModeHeight());
	iniSetInteger("fullscreenScreen", war_GetFullscreenModeScreen());
	iniSetInteger("bpp", war_GetVideoBufferDepth());
	iniSetInteger("fullscreen", static_cast<typename std::underlying_type<WINDOW_MODE>::type>(war_getWindowMode()));
	iniSetString("language", getLanguage());
	iniSetInteger("difficulty", getDifficultyLevel());		// level
	iniSetInteger("cameraSpeed", war_GetCameraSpeed());	// camera speed
	iniSetBool("radarJump", war_GetRadarJump());		// radar jump
	iniSetInteger("lodDistanceBias", war_getLODDistanceBiasPercentage());
	iniSetBool("cameraAccel", getCameraAccel());		// camera acceleration
	iniSetInteger("shake", (int)getShakeStatus());		// screenshake
	iniSetInteger("mouseflip", (int)(getInvertMouseStatus()));	// flipmouse
	iniSetInteger("nomousewarp", (int)getMouseWarp());		// mouse warp
	iniSetInteger("coloredCursor", (int)war_GetColouredCursor());
	iniSetInteger("RightClickOrders", (int)(getRightClickOrders()));
	iniSetInteger("MiddleClickRotate", (int)(getMiddleClickRotate()));
	iniSetInteger("cursorScale", (int)war_getCursorScale());
	iniSetInteger("textureCompression", (wz_texture_compression) ? 1 : 0);
	iniSetInteger("showFPS", (int)showFPS);
	iniSetInteger("showUNITCOUNT", (int)showUNITCOUNT);
	iniSetInteger("shadows", (int)(getDrawShadows()));	// shadows
	iniSetInteger("sound", (int)war_getSoundEnabled());
	iniSetInteger("FMVmode", (int)(war_GetFMVmode()));		// sequences
	iniSetInteger("scanlines", (int)war_getScanlineMode());
	iniSetInteger("subtitles", (int)(seq_GetSubtitles()));		// subtitles
	iniSetInteger("radarObjectMode", (int)bEnemyAllyRadarColor);   // enemy/allies radar view
	iniSetInteger("radarTerrainMode", (int)radarDrawMode);
	iniSetBool("trapCursor", war_GetTrapCursor());
	iniSetInteger("vsync", war_GetVsync());
	iniSetInteger("displayScale", war_GetDisplayScale());
	iniSetBool("autoAdjustDisplayScale", war_getAutoAdjustDisplayScale());
	iniSetInteger("textureSize", getTextureSize());
	iniSetInteger("antialiasing", war_getAntialiasing());
	iniSetInteger("UPnP", (int)NetPlay.isUPNP);
	iniSetBool("rotateRadar", rotateRadar);
	iniSetBool("radarRotationArrow", radarRotationArrow);
	iniSetBool("hostQuitConfirmation", hostQuitConfirmation);
	iniSetBool("PauseOnFocusLoss", war_GetPauseOnFocusLoss());
	iniSetString("autoratingUrlV2", getAutoratingUrl());
	iniSetBool("autorating", getAutoratingEnable());
	iniSetString("masterserver_name", NETgetMasterserverName());
	iniSetInteger("masterserver_port", (int)NETgetMasterserverPort());
	iniSetString("server_name", mpGetServerName());
	if (!netGameserverPortOverride) // do not save the config port setting if there's a command-line override
	{
		iniSetInteger("gameserver_port", (int)NETgetGameserverPort());
	}
	iniSetBool("prefer_ipv6", NETgetJoinPreferenceIPv6());
	iniSetString("publicIPv4LookupService_Url", getPublicIPv4LookupServiceUrl());
	iniSetString("publicIPv4LookupService_JSONKey", getPublicIPv4LookupServiceJSONKey());
	iniSetString("publicIPv6LookupService_Url", getPublicIPv6LookupServiceUrl());
	iniSetString("publicIPv6LookupService_JSONKey", getPublicIPv6LookupServiceJSONKey());
	if (!bMultiPlayer)
	{
		iniSetInteger("colour", (int)war_GetSPcolor());			// favourite colour.
	}
	else
	{
		if (NetPlay.isHost && ingame.localJoiningInProgress)
		{
			if (bMultiPlayer && NetPlay.bComms)
			{
				iniSetString("gameName", game.name);			//  last hosted game
				war_setMPInactivityMinutes(game.inactivityMinutes);

				// remember number of spectator slots in MP games
				auto currentSpectatorSlotInfo = SpectatorInfo::currentNetPlayState();
				war_setMPopenSpectatorSlots(currentSpectatorSlotInfo.totalSpectatorSlots);
			}
			iniSetString("mapName", game.map);				//  map name
			iniSetString("mapHash", game.hash.toString());          //  map hash
			iniSetInteger("maxPlayers", (int)game.maxPlayers);		// maxPlayers
			iniSetInteger("powerLevel", game.power);				// power
			iniSetInteger("base", game.base);				// size of base
			iniSetInteger("alliance", (int)game.alliance);		// allow alliances
			iniSetInteger("newScavengers", game.scavengers);
		}
		iniSetString("playerName", (char *)sPlayer);		// player name
	}
	iniSetInteger("colourMP", war_getMPcolour());
	iniSetInteger("inactivityMinutesMP", war_getMPInactivityMinutes());
	iniSetInteger("openSpectatorSlotsMP", war_getMPopenSpectatorSlots());
	iniSetString("gfxbackend", to_string(war_getGfxBackend()));
	iniSetInteger("minimizeOnFocusLoss", war_getMinimizeOnFocusLoss());
	iniSetInteger("altEnterToggleMode", war_getToggleFullscreenMode());
	iniSetString("jsbackend", to_string(war_getJSBackend()));
	iniSetInteger("BlueprintTrackAnimationSpeed", BlueprintTrackAnimationSpeed);
	iniSetBool("lockCameraScrollWhileRotating", lockCameraScrollWhileRotating);
	iniSetBool("autosaveEnabled", autosaveEnabled);
	iniSetBool("fog", pie_GetFogEnabled());
	iniSetInteger("hostAutoLagKickSeconds", war_getAutoLagKickSeconds());
	iniSetBool("disableReplayRecord", war_getDisableReplayRecording());
	iniSetInteger("maxReplaysSaved", war_getMaxReplaysSaved());
	iniSetInteger("oldLogsLimit", war_getOldLogsLimit());
	iniSetInteger("fogEnd", war_getFogEnd());
	iniSetInteger("fogStart", war_getFogStart());
	iniSetInteger("configVersion", CURRCONFVERSION);

	// write out ini file changes
	bool result = saveIniFile(file, ini);
	return result;
}

bool saveGfxConfig()
{
	// first, create a file instance
	mINI::INIFile file(std::make_shared<PhysFSFileStreamGenerator>(fileName));

	// next, create a structure that will hold data
	mINI::INIStructure ini;

	// read in the current file
	try
	{
		if (!file.read(ini))
		{
			debug(LOG_WZ, "Could not read existing configuration file \"%s\"", fileName);
			// will just proceed with an empty ini structure
		}
	}
	catch (const std::exception& e)
	{
		debug(LOG_ERROR, "Ini read failed with exception: %s", e.what());
		return false;
	}

	std::string fullConfigFilePath;
	if (PHYSFS_getWriteDir())
	{
		fullConfigFilePath += PHYSFS_getWriteDir();
		fullConfigFilePath += "/";
	}
	fullConfigFilePath += fileName;
	debug(LOG_WZ, "Writing gfx configuration to: \"%s\"", fullConfigFilePath.c_str());

	auto& iniGeneral = ini["General"];

	auto iniSetString = [&iniGeneral](const std::string& key, const std::string& value) {
		iniGeneral[key] = value;
	};

	// only change the gfx entry
	iniSetString("gfxbackend", to_string(war_getGfxBackend()));

	// write out ini file changes
	bool result = saveIniFile(file, ini);
	return result;
}

// Saves and loads the relevant part of the config files for MP games
// Ensures that others' games don't change our own configuration settings
bool reloadMPConfig()
{
	// first, create a file instance
	mINI::INIFile file(std::make_shared<PhysFSFileStreamGenerator>(fileName));

	// next, create a structure that will hold data
	mINI::INIStructure ini;

	// now we can read the file
	try
	{
		if (!file.read(ini))
		{
			debug(LOG_INFO, "Could not read existing configuration file \"%s\"", fileName);
		}
	}
	catch (const std::exception& e)
	{
		debug(LOG_ERROR, "Ini read failed with exception: %s", e.what());
		return false;
	}

	auto& iniGeneral = ini["General"];

	debug(LOG_WZ, "Reloading prefs prefs to registry");

	// If we're in-game, we already have our own configuration set, so no need to reload it.
	if (NetPlay.isHost && !ingame.localJoiningInProgress)
	{
		if (bMultiPlayer && !NetPlay.bComms)
		{
			// one-player skirmish mode sets game name to "One Player Skirmish", so
			// reset the name
			if (iniGeneral.has("gameName"))
			{
				sstrcpy(game.name, iniSectionGetString(iniGeneral, "gameName", "").value().c_str());
			}
		}
		return true;
	}

	// If we're host and not in-game, we can safely save our settings and return.
	if (NetPlay.isHost && ingame.localJoiningInProgress)
	{
		if (bMultiPlayer && NetPlay.bComms)
		{
			iniGeneral["gameName"] = std::string(game.name);			//  last hosted game
		}
		else
		{
			// One-player skirmish mode sets game name to "One Player Skirmish", so
			// reset the name
			if (iniGeneral.has("gameName"))
			{
				sstrcpy(game.name, iniSectionGetString(iniGeneral, "gameName", "").value().c_str());
			}
		}

		// Set a default map to prevent hosting games without a map.
		sstrcpy(game.map, DEFAULTSKIRMISHMAP);
		game.hash.setZero();
		game.maxPlayers = DEFAULTSKIRMISHMAPMAXPLAYERS;

		iniSectionSetInteger(iniGeneral, "powerLevel", game.power);				// power
		iniSectionSetInteger(iniGeneral, "base", game.base);				// size of base
		iniSectionSetInteger(iniGeneral, "alliance", game.alliance);		// allow alliances

		// write out ini file changes
		bool result = saveIniFile(file, ini);
		return result;
	}

	// We're not host, so let's get rid of the host's game settings and restore our own.

	// game name
	if (iniGeneral.has("gameName"))
	{
		sstrcpy(game.name, iniSectionGetString(iniGeneral, "gameName", "").value().c_str());
	}

	// Set a default map to prevent hosting games without a map.
	sstrcpy(game.map, DEFAULTSKIRMISHMAP);
	game.hash.setZero();
	game.maxPlayers = DEFAULTSKIRMISHMAPMAXPLAYERS;

	game.power = iniSectionGetInteger(iniGeneral, "powerLevel", LEV_MED).value();
	game.base = iniSectionGetInteger(iniGeneral, "base", CAMP_BASE).value();
	game.alliance = iniSectionGetInteger(iniGeneral, "alliance", NO_ALLIANCES).value();
	game.inactivityMinutes = war_getMPInactivityMinutes();

	return true;
}
