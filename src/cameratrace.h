// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

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
/** \file
 *  Per-frame camera trace, compiled in by -DWZ_PERF_COUNTERS=ON and enabled at runtime by --cameratrace=<file>.
 *  Writes one CSV row per main loop iteration with the timestamps of the points in the frame
 *  that matter for camera smoothness (loop start, sim ticks, scroll sample, draw, present, realTime update)
 *  and the camera state each path produced.
 *
 *  --cameraautopan=<lr>,<ud>[,<legSeconds>[,<idleSeconds>]] drives the scroll input
 *  with a repeating move / coast / reverse pattern.
 *  Neither touches game state.
 */
#pragma once

#include "lib/framework/wzglobal.h" // required for config.h

#include <chrono>
#include <cstdint>

#if defined(WZ_PERF_COUNTERS)

namespace camtrace
{
extern bool g_enabled;      ///< --cameratrace is writing
extern bool g_autoPan;      ///< --cameraautopan is driving the scroll input

bool open(const char *path);
void close();
bool setAutoPan(const char *spec);  ///< parses "<lr>,<ud>[,<legSeconds>[,<idleSeconds>]]"

/// Main loop (main.cpp)
void frameBegin();          ///< top of mainLoop(), writes the previous frame's row
void markRealTimeUpdate(uint32_t realTimeMs, uint32_t deltaRealTimeMs);

/// Game loop (loop.cpp)
void tickBegin();
void tickEnd();
void markPresentEnd();      ///< right after pie_ScreenFrameRenderEnd() in gameLoop()

/// Horizontal scroll (display.cpp)
void markScrollSample(bool updated);
void markScrollStep(double dtSeconds, double stepLeftRight, double stepUpDown, double speedLeftRight, double speedUpDown,
                    double xExact, double zExact, int xDif, int zDif);
void applyAutoPan(int &scrollDirLeftRight, int &scrollDirUpDown);
void notifyScrollLimitHit();

/// --cameraheightspring=<stiffness>,<damping> replaces the height follower's spring constants
bool setHeightSpring(const char *spec);
void overrideHeightSpring(double &stiffness, double &damping);

/// Height follow and draw (display3d.cpp)
void markHeightTarget(int averageTerrainHeight, int desiredHeight);
/// resting: the follower sat at the target
void markHeightUpdate(double dtMs, float separationBefore, float separationAfter, float heightSpeed, bool resting = false);
void markDraw(int x, int y, int z, int rotX, int rotY, float viewDistance, bool warCamActive);

/// Radar (radar.cpp)
void markRadarRedraw();

/// Stages of the render half of the frame
enum class Stage
{
	RenderBegin,  ///< renderLoop() entered (loop.cpp)
	AudioEnd,     ///< audio_Update() returned (loop.cpp)
	SceneEnd,     ///< end of draw3DScene() (display3d.cpp)
	RenderEnd,    ///< renderLoop() returned, before pie_ScreenFrameRenderEnd() (loop.cpp)
};
void markStage(Stage stage);
}

#else // !defined(WZ_PERF_COUNTERS)

namespace camtrace
{
constexpr bool g_enabled = false;
constexpr bool g_autoPan = false;

bool open(const char *path);             ///< reports that this build has no camera trace and fails
bool setAutoPan(const char *spec);       ///< reports that this build has no camera trace and fails
bool setHeightSpring(const char *spec);  ///< reports that this build has no camera trace and fails
inline void overrideHeightSpring(double &, double &) {}
inline void close() {}
inline void frameBegin() {}
inline void markRealTimeUpdate(uint32_t, uint32_t) {}
inline void tickBegin() {}
inline void tickEnd() {}
inline void markPresentEnd() {}
inline void markScrollSample(bool) {}
inline void markScrollStep(double, double, double, double, double, double, double, int, int) {}
inline void applyAutoPan(int &, int &) {}
inline void notifyScrollLimitHit() {}
inline void markHeightTarget(int, int) {}
inline void markHeightUpdate(double, float, float, float, bool = false) {}
inline void markDraw(int, int, int, int, int, float, bool) {}
inline void markRadarRedraw() {}
enum class Stage { RenderBegin, AudioEnd, SceneEnd, RenderEnd };
inline void markStage(Stage) {}
}

#endif // defined(WZ_PERF_COUNTERS)
