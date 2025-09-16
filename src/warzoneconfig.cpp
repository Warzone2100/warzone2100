/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2024  Warzone 2100 Project

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
 * @file warzoneconfig.c
 *
 * Warzone Global configuration functions.
 */

#include "lib/framework/frame.h"
#include "warzoneconfig.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/sound/sounddefs.h"
#include "lib/netplay/connection_provider_registry.h"
#include "advvis.h"
#include "component.h"
#include "display.h"
#include "keybind.h"
#include "radar.h"
#include "activity.h"

#define MAX_REPLAY_FILES 36
constexpr int MAX_OLD_LOGS = 50;

/***************************************************************************/

#if !defined(__EMSCRIPTEN__)
#define WZ_DEFAULT_FMV_MODE FMV_FULLSCREEN
#else
#define WZ_DEFAULT_FMV_MODE FMV_2X
#endif

struct WARZONE_GLOBALS
{
	FMV_MODE FMVmode = WZ_DEFAULT_FMV_MODE;
	UDWORD width = 1024;
	UDWORD height = 768;
	UDWORD videoBufferDepth = 32;
	int displayScale = 100;
	int screen = 0;
	int8_t SPcolor = 0;
	int MPcolour = -1;
	int antialiasing = 0;
	WINDOW_MODE Fullscreen = WINDOW_MODE::windowed; // Leave this to windowed, some system will fail and they can't see the system popup dialog!
	bool soundEnabled = true;
	TrapCursorMode trapCursor = TrapCursorMode::Automatic;
	int vsync = 1;
	bool pauseOnFocusLoss = false;
	bool ColouredCursor = true;
	bool MusicEnabled = true;
	HRTFMode hrtfMode = HRTFMode::Auto;
	int mapZoom = STARTDISTANCE;
	int mapZoomRate = MAP_ZOOM_RATE_DEFAULT;
	int radarZoom = DEFAULT_RADARZOOM;
	int cameraSpeed = CAMERASPEED_DEFAULT;
	bool radarJump = false;
	video_backend gfxBackend = video_backend::opengl; // the actual default value is determined in loadConfig()
	JS_BACKEND jsBackend = (JS_BACKEND)0;
	bool autoAdjustDisplayScale = true;
	int autoLagKickSeconds = 60;
	int autoLagKickAggressiveness = 3;
	int autoDesyncKickSeconds = 10;
	int autoNotReadyKickSeconds = 0;
	bool disableReplayRecording = false;
	int maxReplaysSaved = MAX_REPLAY_FILES;
	int oldLogsLimit = MAX_OLD_LOGS;
	uint32_t MPinactivityMinutes = 5;
	uint32_t MPgameTimeLimitMinutes = 0; // default to unlimited
	uint8_t MPopenSpectatorSlots = 0;
	PLAYER_LEAVE_MODE MPplayerLeaveMode = PLAYER_LEAVE_MODE_DEFAULT;
	int fogStart = 4000;
	int fogEnd = 8000;
	int lodDistanceBiasPercentage = WZ_LODDISTANCEPERCENTAGE_HIGH; // default to "High" to best match prior version behavior
	int minimizeOnFocusLoss = -1; // see enum class MinimizeOnFocusLossBehavior
	// fullscreen mode settings
	UDWORD fullscreenModeWidth = 0; // current display default
	UDWORD fullscreenModeHeight = 0; // current display default
	int fullscreenModeScreen = -1;
	int toggleFullscreenMode = 0; // 0 = the backend default
	unsigned int cursorScale = 100;
	// shadow mapping settings
	uint32_t shadowFilteringMode = 1;
	uint32_t shadowFilterSize = 5;
	uint32_t shadowMapResolution = 0; // this defaults to 0, which causes the gfx backend to figure out a recommended default based on the system properties
	bool pointLightLighting = false;
	// UI config
	bool groupsMenuEnabled = true;
	uint8_t optionsButtonVisibility = 100;

	// run-time only settings (not persisted to config!)
	bool allowVulkanImplicitLayers = false;

	// Connection provider used for hosting games
	ConnectionProviderType hostProviderType = ConnectionProviderType::TCP_DIRECT;

	// audio cues
	bool playAudioCue_GroupReporting = true;
};

static WARZONE_GLOBALS warGlobs;

/***************************************************************************/

std::string js_backend_names[] =
{
	"quickjs",
	"invalid" // Must be last!
};

static_assert((size_t)JS_BACKEND::num_backends == (sizeof(js_backend_names) / sizeof(std::string)) - 1, "js_backend_names must match JS_BACKEND enum");

bool js_backend_from_str(const char *str, JS_BACKEND &output_backend)
{
	for (size_t i = 0; i < (size_t)JS_BACKEND::num_backends; i++)
	{
		if (strcasecmp(js_backend_names[i].c_str(), str) == 0)
		{
			output_backend = (JS_BACKEND)i;
			return true;
		}
	}
	return false;
}

std::string to_string(JS_BACKEND backend)
{
	return js_backend_names[(size_t)backend];
}


/***************************************************************************/

void war_SetDefaultStates()
{
	warGlobs = WARZONE_GLOBALS();
}

void war_SetSPcolor(int color)
{
	if (color >= 1 && color <= 3)		// only 0,4,5,6,7 are allowed for SP games, AI uses the other colors.
	{
		color = 0;
	}
	warGlobs.SPcolor = color;
	setPlayerColour(0, color);
	ActivityManager::instance().changedSetting("SPcolor", std::to_string(color));
}

int8_t war_GetSPcolor()
{
	return warGlobs.SPcolor;
}

void war_setMPcolour(int colour)
{
	warGlobs.MPcolour = colour;
	ActivityManager::instance().changedSetting("MPcolour", std::to_string(colour));
}

int war_getMPcolour()
{
	return warGlobs.MPcolour;
}

void war_setWindowMode(WINDOW_MODE b)
{
	warGlobs.Fullscreen = b;
}

WINDOW_MODE war_getWindowMode()
{
	return warGlobs.Fullscreen;
}

void war_setAntialiasing(int antialiasing)
{
	if (antialiasing > 16)
	{
		debug(LOG_WARNING, "Antialising set to value > 16, which can cause crashes.");
	}
	warGlobs.antialiasing = antialiasing;
	ActivityManager::instance().changedSetting("antialiasing", std::to_string(antialiasing));
}

int war_getAntialiasing()
{
	return warGlobs.antialiasing;
}

void war_SetTrapCursor(TrapCursorMode v)
{
	warGlobs.trapCursor = v;
	ActivityManager::instance().changedSetting("trapCursor", std::to_string(static_cast<int>(v)));
}

TrapCursorMode war_GetTrapCursor()
{
	return warGlobs.trapCursor;
}

void war_SetVsync(int value)
{
	warGlobs.vsync = value;
}

int war_GetVsync()
{
	return warGlobs.vsync;
}

unsigned int war_GetDisplayScale()
{
	return warGlobs.displayScale;
}

void war_SetDisplayScale(unsigned int scale)
{
	warGlobs.displayScale = scale;
	ActivityManager::instance().changedSetting("displayScale", std::to_string(scale));
}

void war_setCursorScale(unsigned int scale)
{
	warGlobs.cursorScale = std::max<unsigned int>(scale, 100);
}

unsigned int war_getCursorScale()
{
	return warGlobs.cursorScale;
}

void war_SetWidth(UDWORD width)
{
	warGlobs.width = width;
}

UDWORD war_GetWidth()
{
	return warGlobs.width;
}

void war_SetHeight(UDWORD height)
{
	warGlobs.height = height;
}

UDWORD war_GetHeight()
{
	return warGlobs.height;
}

void war_SetVideoBufferDepth(UDWORD videoBufferDepth)
{
	warGlobs.videoBufferDepth = videoBufferDepth;
}

UDWORD war_GetVideoBufferDepth()
{
	return warGlobs.videoBufferDepth;
}

void war_SetScreen(int screen)
{
	warGlobs.screen = screen;
}

int war_GetScreen()
{
	return warGlobs.screen;
}

void war_SetFMVmode(FMV_MODE mode)
{
	warGlobs.FMVmode = (FMV_MODE)(mode % FMV_MAX);
}

FMV_MODE war_GetFMVmode()
{
	return  warGlobs.FMVmode;
}

void war_setScanlineMode(SCANLINE_MODE mode)
{
	debug(LOG_VIDEO, "%d", mode);
	seq_setScanlineMode(mode);
}

SCANLINE_MODE war_getScanlineMode()
{
	debug(LOG_VIDEO, "%d", seq_getScanlineMode());
	return seq_getScanlineMode();
}

void war_SetPauseOnFocusLoss(bool enabled)
{
	warGlobs.pauseOnFocusLoss = enabled;
	ActivityManager::instance().changedSetting("pauseOnFocusLoss", std::to_string(enabled));
}

bool war_GetPauseOnFocusLoss()
{
	return warGlobs.pauseOnFocusLoss;
}

void war_SetColouredCursor(bool enabled)
{
	warGlobs.ColouredCursor = enabled;
	ActivityManager::instance().changedSetting("ColouredCursor", std::to_string(enabled));
}

bool war_GetColouredCursor()
{
	return warGlobs.ColouredCursor;
}

void war_setSoundEnabled(bool soundEnabled)
{
	warGlobs.soundEnabled = soundEnabled;
	ActivityManager::instance().changedSetting("soundEnabled", std::to_string(soundEnabled));
}

bool war_getSoundEnabled()
{
	return warGlobs.soundEnabled;
}

bool war_GetMusicEnabled()
{
	return warGlobs.MusicEnabled;
}

void war_SetMusicEnabled(bool enabled)
{
	warGlobs.MusicEnabled = enabled;
	ActivityManager::instance().changedSetting("musicEnabled", std::to_string(enabled));
}

HRTFMode war_GetHRTFMode()
{
	return warGlobs.hrtfMode;
}

void war_SetHRTFMode(HRTFMode mode)
{
	warGlobs.hrtfMode = mode;
	ActivityManager::instance().changedSetting("hrtfMode", std::to_string(static_cast<typename std::underlying_type<HRTFMode>::type>(mode)));
}

int war_GetMapZoom()
{
	return warGlobs.mapZoom;
}

void war_SetMapZoom(int mapZoom)
{
	if (mapZoom % MAP_ZOOM_CONFIG_STEP == 0 && ! (mapZoom < MINDISTANCE_CONFIG || mapZoom > MAXDISTANCE))
	{
		warGlobs.mapZoom = mapZoom;
		ActivityManager::instance().changedSetting("mapZoom", std::to_string(mapZoom));
	}
}

int war_GetMapZoomRate()
{
	return warGlobs.mapZoomRate;
}

void war_SetMapZoomRate(int mapZoomRate)
{
	if (mapZoomRate % MAP_ZOOM_RATE_STEP == 0 && ! (mapZoomRate < MAP_ZOOM_RATE_MIN || mapZoomRate > MAP_ZOOM_RATE_MAX))
	{
		warGlobs.mapZoomRate = mapZoomRate;
		ActivityManager::instance().changedSetting("mapZoomRate", std::to_string(mapZoomRate));
	}
}

int war_GetRadarZoom()
{
	return warGlobs.radarZoom;
}

void war_SetRadarZoom(int radarZoom)
{
	if (radarZoom % RADARZOOM_STEP == 0 && ! (radarZoom < MIN_RADARZOOM || radarZoom > MAX_RADARZOOM))
	{
		warGlobs.radarZoom = radarZoom;
		ActivityManager::instance().changedSetting("radarZoom", std::to_string(radarZoom));
	}
}

int war_GetCameraSpeed()
{
	return warGlobs.cameraSpeed;
}

void war_SetCameraSpeed(int cameraSpeed)
{
	if (cameraSpeed % CAMERASPEED_STEP == 0 && ! (cameraSpeed < CAMERASPEED_MIN || cameraSpeed > CAMERASPEED_MAX))
	{
		warGlobs.cameraSpeed = cameraSpeed;
		ActivityManager::instance().changedSetting("cameraSpeed", std::to_string(cameraSpeed));
	}
}

bool war_GetRadarJump()
{
	return warGlobs.radarJump;
}

void war_SetRadarJump(bool radarJump)
{
	warGlobs.radarJump = radarJump;
	ActivityManager::instance().changedSetting("radarJump", std::to_string(radarJump));
}

int war_getLODDistanceBiasPercentage()
{
	return warGlobs.lodDistanceBiasPercentage;
}

void war_setLODDistanceBiasPercentage(int bias)
{
	if (bias > 200) { bias = 200; }
	if (bias < -200) { bias = -200; }
	warGlobs.lodDistanceBiasPercentage = bias;
}

int war_getMinimizeOnFocusLoss()
{
	return warGlobs.minimizeOnFocusLoss;
}

void war_setMinimizeOnFocusLoss(int val)
{
	warGlobs.minimizeOnFocusLoss = val;
}

video_backend war_getGfxBackend()
{
	return warGlobs.gfxBackend;
}

void war_setGfxBackend(video_backend backend)
{
	warGlobs.gfxBackend = backend;
}

JS_BACKEND war_getJSBackend()
{
	return warGlobs.jsBackend;
}

void war_setJSBackend(JS_BACKEND backend)
{
	warGlobs.jsBackend = backend;
}

bool war_getAutoAdjustDisplayScale()
{
	return warGlobs.autoAdjustDisplayScale;
}

void war_setAutoAdjustDisplayScale(bool autoAdjustDisplayScale)
{
	warGlobs.autoAdjustDisplayScale = autoAdjustDisplayScale;
}

int war_getAutoLagKickSeconds()
{
	return warGlobs.autoLagKickSeconds;
}

void war_setAutoLagKickSeconds(int seconds)
{
	seconds = std::max(seconds, 0);
	if (seconds > 0)
	{
		seconds = std::max(seconds, 15);
	}
	warGlobs.autoLagKickSeconds = seconds;
}

int war_getAutoLagKickAggressiveness()
{
	return warGlobs.autoLagKickAggressiveness;
}

void war_setAutoLagKickAggressiveness(int aggressiveness)
{
	aggressiveness = std::max(aggressiveness, 1);
	aggressiveness = std::min(aggressiveness, 10);
	warGlobs.autoLagKickAggressiveness = aggressiveness;
}

int war_getAutoDesyncKickSeconds()
{
	return warGlobs.autoDesyncKickSeconds;
}

void war_setAutoDesyncKickSeconds(int seconds)
{
	seconds = std::max(seconds, 0);
	if (seconds > 0)
	{
		seconds = std::max(seconds, 10);
	}
	warGlobs.autoDesyncKickSeconds = seconds;
}

int war_getAutoNotReadyKickSeconds()
{
	return warGlobs.autoNotReadyKickSeconds;
}

void war_setAutoNotReadyKickSeconds(int seconds)
{
	seconds = std::max(seconds, 0);
	if (seconds > 0)
	{
		seconds = std::max(seconds, 15);
	}
	warGlobs.autoNotReadyKickSeconds = seconds;
}

bool war_getDisableReplayRecording()
{
	return warGlobs.disableReplayRecording;
}

void war_setDisableReplayRecording(bool disable)
{
	warGlobs.disableReplayRecording = disable;
}

int war_getMaxReplaysSaved()
{
	return warGlobs.maxReplaysSaved;
}

void war_setMaxReplaysSaved(int maxReplaysSaved)
{
	warGlobs.maxReplaysSaved = maxReplaysSaved;
}

int war_getOldLogsLimit()
{
	return warGlobs.oldLogsLimit;
}

void war_setOldLogsLimit(int oldLogsLimit)
{
	warGlobs.oldLogsLimit = oldLogsLimit;
}

uint32_t war_getMPInactivityMinutes()
{
	return warGlobs.MPinactivityMinutes;
}

void war_setMPInactivityMinutes(uint32_t minutes)
{
	if (minutes > 0 && minutes < MIN_MPINACTIVITY_MINUTES)
	{
		minutes = MIN_MPINACTIVITY_MINUTES;
	}
	warGlobs.MPinactivityMinutes = minutes;
}

uint32_t war_getMPGameTimeLimitMinutes()
{
	return warGlobs.MPgameTimeLimitMinutes;
}

void war_setMPGameTimeLimitMinutes(uint32_t minutes)
{
	if (minutes > 0 && minutes < MIN_MPGAMETIMELIMIT_MINUTES)
	{
		minutes = MIN_MPGAMETIMELIMIT_MINUTES;
	}
	warGlobs.MPgameTimeLimitMinutes = minutes;
}

uint16_t war_getMPopenSpectatorSlots()
{
	return warGlobs.MPopenSpectatorSlots;
}

void war_setMPopenSpectatorSlots(uint16_t spectatorSlots)
{
	spectatorSlots = std::min<uint16_t>(spectatorSlots, MAX_SPECTATOR_SLOTS);
	warGlobs.MPopenSpectatorSlots = spectatorSlots;
}

PLAYER_LEAVE_MODE war_getMPPlayerLeaveMode()
{
	return warGlobs.MPplayerLeaveMode;
}

void war_setMPPlayerLeaveMode(PLAYER_LEAVE_MODE mode)
{
	warGlobs.MPplayerLeaveMode = mode;
}

int war_getFogEnd()
{
	return warGlobs.fogEnd;
}

int war_getFogStart()
{
	return warGlobs.fogStart;
}

void war_setFogEnd(int end)
{
	 warGlobs.fogEnd = end;
}

void war_setFogStart(int start)
{
	 warGlobs.fogStart = start;
}

void war_SetFullscreenModeWidth(UDWORD width)
{
	warGlobs.fullscreenModeWidth = width;
}

UDWORD war_GetFullscreenModeWidth()
{
	return warGlobs.fullscreenModeWidth;
}

void war_SetFullscreenModeHeight(UDWORD height)
{
	warGlobs.fullscreenModeHeight = height;
}

UDWORD war_GetFullscreenModeHeight()
{
	return warGlobs.fullscreenModeHeight;
}
void war_SetFullscreenModeScreen(int screen)
{
	warGlobs.fullscreenModeScreen = screen;
}

int war_GetFullscreenModeScreen()
{
	return warGlobs.fullscreenModeScreen;
}

void war_setToggleFullscreenMode(int mode)
{
	warGlobs.toggleFullscreenMode = mode;
}

int war_getToggleFullscreenMode()
{
	return warGlobs.toggleFullscreenMode;
}

uint32_t war_getShadowFilterSize()
{
	return warGlobs.shadowFilterSize;
}

void war_setShadowFilterSize(uint32_t filterSize)
{
	if (filterSize > 7)
	{
		debug(LOG_INFO, "Shadow filter size %" PRIu32 " may not have the desired effect", filterSize);
	}
	warGlobs.shadowFilterSize = filterSize;
}

uint32_t war_getShadowMapResolution()
{
	return warGlobs.shadowMapResolution;
}

void war_setShadowMapResolution(uint32_t resolution)
{
	if (resolution > 0 && ((resolution & (resolution - 1)) != 0))
	{
		debug(LOG_ERROR, "Shadow map resolution %" PRIu32 " is unsupported: must be a power of 2", resolution);
		return;
	}
	if (resolution > 0 && (resolution < 2048 || resolution > 4096)) // 0 is a special case that maps to "figure out a decent default for this system"
	{
		debug(LOG_INFO, "Shadow map resolution %" PRIu32 " may not have the desired effect", resolution);
	}
	warGlobs.shadowMapResolution = resolution;
}

bool war_getPointLightPerPixelLighting()
{
	return warGlobs.pointLightLighting;
}

void war_setPointLightPerPixelLighting(bool perPixelEnabled)
{
	warGlobs.pointLightLighting = perPixelEnabled;
}

bool war_getGroupsMenuEnabled()
{
	return warGlobs.groupsMenuEnabled;
}

void war_setGroupsMenuEnabled(bool enabled)
{
	warGlobs.groupsMenuEnabled = enabled;
}

void war_runtimeOnlySetAllowVulkanImplicitLayers(bool allowed) // not persisted to config
{
	warGlobs.allowVulkanImplicitLayers = allowed;
}

uint8_t war_getOptionsButtonVisibility()
{
	return warGlobs.optionsButtonVisibility;
}

void war_setOptionsButtonVisibility(uint8_t val)
{
	val = std::min<uint8_t>(val, 100);
	if (val > 0)
	{
		val = std::max<uint8_t>(val, 50);
	}
	warGlobs.optionsButtonVisibility = val;
}

bool war_getAllowVulkanImplicitLayers()
{
	return warGlobs.allowVulkanImplicitLayers;
}

void war_setHostConnectionProvider(ConnectionProviderType pt)
{
	warGlobs.hostProviderType = pt;
}

ConnectionProviderType war_getHostConnectionProvider()
{
	return warGlobs.hostProviderType;
}

bool net_backend_from_str(const char* str, ConnectionProviderType& pt)
{
	if (strcasecmp(str, "tcp") == 0)
	{
		pt = ConnectionProviderType::TCP_DIRECT;
		return true;
	}
#ifdef WZ_GNS_NETWORK_BACKEND_ENABLED
	if (strcasecmp(str, "gns") == 0)
	{
		pt = ConnectionProviderType::GNS_DIRECT;
		return true;
	}
#endif
	return false;
}

std::string to_string(ConnectionProviderType pt)
{
	switch (pt)
	{
	case ConnectionProviderType::TCP_DIRECT:
		return "tcp";
#ifdef WZ_GNS_NETWORK_BACKEND_ENABLED
	case ConnectionProviderType::GNS_DIRECT:
		return "gns";
#endif
	}
	ASSERT(false, "Invalid connection provider type enumeration value: %d", static_cast<int>(pt)); // silence GCC warning
	return {};
}

bool war_getPlayAudioCue_GroupReporting()
{
	return warGlobs.playAudioCue_GroupReporting;
}

void war_setPlayAudioCue_GroupReporting(bool val)
{
	warGlobs.playAudioCue_GroupReporting = val;
}
