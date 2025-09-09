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
/** @file
 *  Warzone Global configuration functions.
 */

#ifndef __INCLUDED_SRC_WARZONECONFIG_H__
#define __INCLUDED_SRC_WARZONECONFIG_H__

#include "lib/framework/frame.h"
#include "lib/sequence/sequence.h"
#include "lib/sound/sounddefs.h"
#include "multiplaydefs.h"
#include <string>
#include <stdint.h>

#define	CAMERASPEED_MAX		(5000)
#define	CAMERASPEED_MIN		(100)
#define	CAMERASPEED_DEFAULT	(2500)
#define	CAMERASPEED_STEP	(100)

#define MIN_MPINACTIVITY_MINUTES 4
#define MIN_MPGAMETIMELIMIT_MINUTES 30

#define WZ_LODDISTANCEPERCENTAGE_HIGH -50

/***************************************************************************/
/*
 *	Global Definitions
 */
/***************************************************************************/
enum FMV_MODE
{
	FMV_FULLSCREEN,
	FMV_1X,
	FMV_2X,
	FMV_MAX
};

enum class JS_BACKEND
{
	quickjs,
	num_backends // Must be last!
};

bool js_backend_from_str(const char *str, JS_BACKEND &output_backend);
std::string to_string(JS_BACKEND backend);

enum class TrapCursorMode : uint8_t
{
	Disabled = 0,
	Enabled,
	Automatic
};

/***************************************************************************/
/*
 *	Global ProtoTypes
 */
/***************************************************************************/
void war_SetDefaultStates();
void war_SetFMVmode(FMV_MODE mode);
FMV_MODE war_GetFMVmode();
void war_SetAllowSubtitles(bool);
bool war_GetAllowSubtitles();
void war_setWindowMode(WINDOW_MODE);
WINDOW_MODE war_getWindowMode();
void war_setAntialiasing(int);
int war_getAntialiasing();
void war_SetTrapCursor(TrapCursorMode v);
TrapCursorMode war_GetTrapCursor();
bool war_GetColouredCursor();
void war_SetColouredCursor(bool enabled);
void war_SetVsync(int value);
int war_GetVsync();
void war_SetDisplayScale(unsigned int scale);
unsigned int war_GetDisplayScale();
// non-fullscreen window sizes / screen
void war_SetWidth(UDWORD width);
UDWORD war_GetWidth();
void war_SetScreen(int screen);
int war_GetScreen();
void war_SetHeight(UDWORD height);
UDWORD war_GetHeight();
// fullscreen display mode + screen
void war_SetFullscreenModeWidth(UDWORD width);
UDWORD war_GetFullscreenModeWidth();
void war_SetFullscreenModeScreen(int screen);
int war_GetFullscreenModeScreen();
void war_SetFullscreenModeHeight(UDWORD height);
UDWORD war_GetFullscreenModeHeight();
void war_setToggleFullscreenMode(int mode);
int war_getToggleFullscreenMode();
void war_SetVideoBufferDepth(UDWORD videoBufferDepth);
UDWORD war_GetVideoBufferDepth();
void war_SetPauseOnFocusLoss(bool enabled);
bool war_GetPauseOnFocusLoss();
bool war_GetMusicEnabled();
void war_SetMusicEnabled(bool enabled);
HRTFMode war_GetHRTFMode();
void war_SetHRTFMode(HRTFMode mode);
int war_GetMapZoom();
void war_SetMapZoom(int mapZoom);
int war_GetMapZoomRate();
void war_SetMapZoomRate(int mapZoomRate);
int war_GetRadarZoom();
void war_SetRadarZoom(int radarZoom);
bool war_GetRadarJump();
void war_SetRadarJump(bool radarJump);
int war_GetCameraSpeed();
void war_SetCameraSpeed(int cameraSpeed);
int8_t war_GetSPcolor();
void war_SetSPcolor(int color);
void war_setMPcolour(int colour);
int war_getMPcolour();
void war_setScanlineMode(SCANLINE_MODE mode);
SCANLINE_MODE war_getScanlineMode();
video_backend war_getGfxBackend();
void war_setGfxBackend(video_backend backend);
JS_BACKEND war_getJSBackend();
void war_setJSBackend(JS_BACKEND backend);
bool war_getAutoAdjustDisplayScale();
void war_setAutoAdjustDisplayScale(bool autoAdjustDisplayScale);
int war_getAutoLagKickSeconds();
void war_setAutoLagKickSeconds(int seconds);
int war_getAutoLagKickAggressiveness();
void war_setAutoLagKickAggressiveness(int aggressiveness);
int war_getAutoDesyncKickSeconds();
void war_setAutoDesyncKickSeconds(int seconds);
int war_getAutoNotReadyKickSeconds();
void war_setAutoNotReadyKickSeconds(int seconds);
bool war_getDisableReplayRecording();
void war_setDisableReplayRecording(bool disable);
int war_getMaxReplaysSaved();
void war_setMaxReplaysSaved(int maxReplaysSaved);
int war_getOldLogsLimit();
void war_setOldLogsLimit(int oldLogsLimit);
uint32_t war_getMPInactivityMinutes();
void war_setMPInactivityMinutes(uint32_t minutes);
uint32_t war_getMPGameTimeLimitMinutes();
void war_setMPGameTimeLimitMinutes(uint32_t minutes);
uint16_t war_getMPopenSpectatorSlots();
void war_setMPopenSpectatorSlots(uint16_t spectatorSlots);
PLAYER_LEAVE_MODE war_getMPPlayerLeaveMode();
void war_setMPPlayerLeaveMode(PLAYER_LEAVE_MODE);
int war_getFogEnd();
int war_getFogStart();
void war_setFogEnd(int end);
void war_setFogStart(int start);
int war_getLODDistanceBiasPercentage();
void war_setLODDistanceBiasPercentage(int bias);
int war_getMinimizeOnFocusLoss();
void war_setMinimizeOnFocusLoss(int val);
void war_setCursorScale(unsigned int scale);
unsigned int war_getCursorScale();

uint32_t war_getShadowFilterSize();
void war_setShadowFilterSize(uint32_t filterSize);
uint32_t war_getShadowMapResolution();
void war_setShadowMapResolution(uint32_t resolution);

bool war_getPointLightPerPixelLighting();
void war_setPointLightPerPixelLighting(bool perPixelEnabled);

bool war_getGroupsMenuEnabled();
void war_setGroupsMenuEnabled(bool enabled);
uint8_t war_getOptionsButtonVisibility();
void war_setOptionsButtonVisibility(uint8_t val);

void war_runtimeOnlySetAllowVulkanImplicitLayers(bool allowed); // not persisted to config
bool war_getAllowVulkanImplicitLayers();

bool war_getPlayAudioCue_GroupReporting();
void war_setPlayAudioCue_GroupReporting(bool val);

enum class ConnectionProviderType : uint8_t;

void war_setHostConnectionProvider(ConnectionProviderType pt);
ConnectionProviderType war_getHostConnectionProvider();

bool net_backend_from_str(const char* str, ConnectionProviderType& pt);
std::string to_string(ConnectionProviderType pt);

/**
 * Enable or disable sound initialization
 *
 * \param	soundEnabled	enable sound (or not)
 */
void war_setSoundEnabled(bool soundEnabled);

/**
 * Whether we should initialize sound or not
 *
 * \return	Enable sound (or not)
 */
bool war_getSoundEnabled();

#endif // __INCLUDED_SRC_WARZONECONFIG_H__
