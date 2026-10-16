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

#include "lib/framework/frame.h"

#include "cameratrace.h"

#if defined(WZ_PERF_COUNTERS)

#include "lib/gamelib/gtime.h"

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

namespace camtrace
{
bool g_enabled = false;
bool g_autoPan = false;

using Clock = std::chrono::steady_clock;

static constexpr int64_t kUnset = std::numeric_limits<int64_t>::min();
static constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

FILE *outFile = nullptr;
Clock::time_point origin;

struct FrameRecord
{
	uint64_t frame = 0;
	int64_t tLoop = kUnset;

	unsigned ticks = 0;
	int64_t tickNs = 0;
	int64_t tTickEnd = kUnset;

	int64_t tScroll = kUnset;
	int scrollUpdated = 0;
	double scrollDt = kNaN;
	double stepLeftRight = kNaN, stepUpDown = kNaN;
	double speedLeftRight = kNaN, speedUpDown = kNaN;
	double xExact = kNaN, zExact = kNaN;
	int64_t xDif = kUnset, zDif = kUnset;
	int64_t autoPanLeftRight = kUnset, autoPanUpDown = kUnset;
	int limitHit = 0;

	int64_t averageHeight = kUnset, desiredHeight = kUnset;
	int heightUpdated = 0;
	double heightDtMs = kNaN;
	double separationBefore = kNaN, separationAfter = kNaN, heightSpeed = kNaN;

	int64_t tDraw = kUnset;
	int64_t camX = kUnset, camY = kUnset, camZ = kUnset, camRotX = kUnset, camRotY = kUnset;
	double viewDistance = kNaN;
	int64_t warCam = kUnset;

	int radarRedraw = 0;

	int64_t tPresent = kUnset;
	int64_t graphicsTimeMs = kUnset, deltaGraphicsTimeMs = kUnset, gameTimeMs = kUnset;

	int64_t tRealTime = kUnset;
	int64_t realTimeMs = kUnset, deltaRealTimeMs = kUnset;

	int64_t tRenderBegin = kUnset, tAudioEnd = kUnset, tSceneEnd = kUnset, tRenderEnd = kUnset;
};

static FrameRecord cur;
static bool frameOpen = false;
static Clock::time_point tickStart;

static int64_t sinceOrigin(Clock::time_point t)
{
	return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(t - origin).count());
}

static int64_t nowNs()
{
	return sinceOrigin(Clock::now());
}

static void putInt(int64_t v)
{
	if (v == kUnset)
	{
		fputc(',', outFile);
	}
	else
	{
		fprintf(outFile, ",%" PRId64, v);
	}
}

static void putDouble(double v)
{
	if (std::isnan(v))
	{
		fputc(',', outFile);
	}
	else
	{
		fprintf(outFile, ",%.9g", v);
	}
}

static const char *columns =
	"frame,t_loop"
	",ticks,tick_ns,t_tick_end"
	",t_scroll,scroll_upd,scroll_dt,step_lr,step_ud,speed_lr,speed_ud,x_exact,z_exact,x_dif,z_dif,autopan_lr,autopan_ud,limit_hit"
	",avg_h,desired_h,h_upd,h_dt_ms,h_sep_before,h_sep_after,h_speed"
	",t_draw,cam_x,cam_y,cam_z,cam_rx,cam_ry,view_dist,warcam"
	",radar"
	",t_present,graphics_time,delta_graphics_time,game_time"
	",t_realtime,real_time,delta_real_time"
	",t_render_begin,t_audio_end,t_scene_end,t_render_end";

static void writeRow(const FrameRecord &r)
{
	fprintf(outFile, "%" PRIu64, r.frame);
	putInt(r.tLoop);
	putInt(r.ticks); putInt(r.tickNs); putInt(r.tTickEnd);
	putInt(r.tScroll); putInt(r.scrollUpdated); putDouble(r.scrollDt);
	putDouble(r.stepLeftRight); putDouble(r.stepUpDown); putDouble(r.speedLeftRight); putDouble(r.speedUpDown);
	putDouble(r.xExact); putDouble(r.zExact); putInt(r.xDif); putInt(r.zDif);
	putInt(r.autoPanLeftRight); putInt(r.autoPanUpDown); putInt(r.limitHit);
	putInt(r.averageHeight); putInt(r.desiredHeight); putInt(r.heightUpdated); putDouble(r.heightDtMs);
	putDouble(r.separationBefore); putDouble(r.separationAfter); putDouble(r.heightSpeed);
	putInt(r.tDraw); putInt(r.camX); putInt(r.camY); putInt(r.camZ); putInt(r.camRotX); putInt(r.camRotY);
	putDouble(r.viewDistance); putInt(r.warCam);
	putInt(r.radarRedraw);
	putInt(r.tPresent); putInt(r.graphicsTimeMs); putInt(r.deltaGraphicsTimeMs); putInt(r.gameTimeMs);
	putInt(r.tRealTime); putInt(r.realTimeMs); putInt(r.deltaRealTimeMs);
	putInt(r.tRenderBegin); putInt(r.tAudioEnd); putInt(r.tSceneEnd); putInt(r.tRenderEnd);
	fputc('\n', outFile);
}

// Auto-pan: move for legSeconds, coast (no input) for idleSeconds, reverse, repeat.
// Hitting the scroll limits ends the current leg early.
struct AutoPan
{
	int leftRight = 0;
	int upDown = 0;
	double legSeconds = 6.0;
	double idleSeconds = 1.0;

	bool started = false;
	bool moving = true;
	int sign = 1;
	Clock::time_point phaseStart;
};
static AutoPan autoPan;

} // anonymous namespace

bool open(const char *path)
{
	outFile = fopen(path, "w");
	if (outFile == nullptr)
	{
		debug(LOG_ERROR, "Failed to open camera trace file for writing: %s", path);
		return false;
	}
	setvbuf(outFile, nullptr, _IOFBF, 1 << 20);
	fprintf(outFile, "%s\n", columns);
	origin = Clock::now();
	cur = FrameRecord();
	frameOpen = false;
	g_enabled = true;
	debug(LOG_INFO, "Writing per-frame camera trace to: %s", path);
	return true;
}

void close()
{
	if (outFile != nullptr)
	{
		if (frameOpen)
		{
			writeRow(cur);
		}
		fclose(outFile);
		outFile = nullptr;
	}
	frameOpen = false;
	g_enabled = false;
}

bool setAutoPan(const char *spec)
{
	int leftRight = 0, upDown = 0;
	double leg = autoPan.legSeconds, idle = autoPan.idleSeconds;
	const int n = sscanf(spec, "%d,%d,%lf,%lf", &leftRight, &upDown, &leg, &idle);
	if (n < 2 || leftRight < -1 || leftRight > 1 || upDown < -1 || upDown > 1 || (leftRight == 0 && upDown == 0) || leg <= 0.0 || idle < 0.0)
	{
		debug(LOG_ERROR, "Bad --cameraautopan value \"%s\": expected <lr>,<ud>[,<legSeconds>[,<idleSeconds>]] with lr, ud in {-1, 0, 1}, not both 0", spec);
		return false;
	}
	autoPan = AutoPan();
	autoPan.leftRight = leftRight;
	autoPan.upDown = upDown;
	autoPan.legSeconds = leg;
	autoPan.idleSeconds = idle;
	g_autoPan = true;
	debug(LOG_INFO, "Camera auto-pan: direction (%d, %d), %.2fs legs, %.2fs coast", leftRight, upDown, leg, idle);
	return true;
}

void frameBegin()
{
	if (!g_enabled)
	{
		return;
	}
	const uint64_t nextFrame = cur.frame + 1;
	if (frameOpen)
	{
		writeRow(cur);
	}
	cur = FrameRecord();
	cur.frame = nextFrame;
	cur.tLoop = nowNs();
	frameOpen = true;
}

void markRealTimeUpdate(uint32_t realTimeMs, uint32_t deltaRealTimeMs)
{
	if (!g_enabled)
	{
		return;
	}
	cur.tRealTime = nowNs();
	cur.realTimeMs = realTimeMs;
	cur.deltaRealTimeMs = deltaRealTimeMs;
}

void tickBegin()
{
	if (!g_enabled)
	{
		return;
	}
	tickStart = Clock::now();
}

void tickEnd()
{
	if (!g_enabled)
	{
		return;
	}
	const Clock::time_point end = Clock::now();
	cur.ticks += 1;
	cur.tickNs += static_cast<int64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(end - tickStart).count());
	cur.tTickEnd = sinceOrigin(end);
}

void markPresentEnd()
{
	if (!g_enabled)
	{
		return;
	}
	cur.tPresent = nowNs();
	cur.graphicsTimeMs = graphicsTime;
	cur.deltaGraphicsTimeMs = deltaGraphicsTime;
	cur.gameTimeMs = gameTime;
}

void markScrollSample(bool updated)
{
	if (!g_enabled)
	{
		return;
	}
	cur.tScroll = nowNs();
	cur.scrollUpdated = updated ? 1 : 0;
}

void markScrollStep(double dtSeconds, double stepLeftRight, double stepUpDown, double speedLeftRight, double speedUpDown,
                    double xExact, double zExact, int xDif, int zDif)
{
	if (!g_enabled)
	{
		return;
	}
	cur.scrollDt = dtSeconds;
	cur.stepLeftRight = stepLeftRight;
	cur.stepUpDown = stepUpDown;
	cur.speedLeftRight = speedLeftRight;
	cur.speedUpDown = speedUpDown;
	cur.xExact = xExact;
	cur.zExact = zExact;
	cur.xDif = xDif;
	cur.zDif = zDif;
}

void applyAutoPan(int &scrollDirLeftRight, int &scrollDirUpDown)
{
	if (!g_autoPan)
	{
		return;
	}
	const Clock::time_point now = Clock::now();
	if (!autoPan.started)
	{
		autoPan.started = true;
		autoPan.moving = true;
		autoPan.sign = 1;
		autoPan.phaseStart = now;
	}
	const double elapsed = std::chrono::duration<double>(now - autoPan.phaseStart).count();
	if (autoPan.moving && elapsed >= autoPan.legSeconds)
	{
		autoPan.moving = false;
		autoPan.phaseStart = now;
	}
	else if (!autoPan.moving && elapsed >= autoPan.idleSeconds)
	{
		autoPan.moving = true;
		autoPan.sign = -autoPan.sign;
		autoPan.phaseStart = now;
	}
	scrollDirLeftRight = autoPan.moving ? autoPan.sign * autoPan.leftRight : 0;
	scrollDirUpDown = autoPan.moving ? autoPan.sign * autoPan.upDown : 0;
	if (g_enabled)
	{
		cur.autoPanLeftRight = scrollDirLeftRight;
		cur.autoPanUpDown = scrollDirUpDown;
	}
}

void notifyScrollLimitHit()
{
	if (g_enabled)
	{
		cur.limitHit = 1;
	}
	if (g_autoPan && autoPan.started && autoPan.moving)
	{
		// End the leg early (the coast phase then reverses direction), after a grace period.
		// NOTE: CheckScrollLimits() also reports a hit while the camera sits on the limit,
		// and a leg that starts there takes several frames to move off it.
		const Clock::time_point now = Clock::now();
		const double grace = std::min(0.5, autoPan.legSeconds / 2.0);
		if (std::chrono::duration<double>(now - autoPan.phaseStart).count() >= grace)
		{
			autoPan.moving = false;
			autoPan.phaseStart = now;
		}
	}
}

void markHeightTarget(int averageTerrainHeight, int desiredHeight)
{
	if (!g_enabled)
	{
		return;
	}
	cur.averageHeight = averageTerrainHeight;
	cur.desiredHeight = desiredHeight;
}

void markHeightUpdate(double dtMs, float separationBefore, float separationAfter, float heightSpeed, bool resting)
{
	if (!g_enabled)
	{
		return;
	}
	cur.heightUpdated = resting ? 2 : 1;
	cur.heightDtMs = dtMs;
	cur.separationBefore = separationBefore;
	cur.separationAfter = separationAfter;
	cur.heightSpeed = heightSpeed;
}

void markDraw(int x, int y, int z, int rotX, int rotY, float viewDistance, bool warCamActive)
{
	if (!g_enabled)
	{
		return;
	}
	cur.tDraw = nowNs();
	cur.camX = x;
	cur.camY = y;
	cur.camZ = z;
	cur.camRotX = rotX;
	cur.camRotY = rotY;
	cur.viewDistance = viewDistance;
	cur.warCam = warCamActive ? 1 : 0;
}

void markRadarRedraw()
{
	if (g_enabled)
	{
		cur.radarRedraw = 1;
	}
}

void markStage(Stage stage)
{
	if (!g_enabled)
	{
		return;
	}
	const int64_t t = nowNs();
	switch (stage)
	{
	case Stage::RenderBegin: cur.tRenderBegin = t; break;
	case Stage::AudioEnd:    cur.tAudioEnd = t; break;
	case Stage::SceneEnd:    cur.tSceneEnd = t; break;
	case Stage::RenderEnd:   cur.tRenderEnd = t; break;
	}
}

} // namespace camtrace

#else // !defined(WZ_PERF_COUNTERS)

namespace camtrace
{
bool open(const char *)
{
	debug(LOG_ERROR, "This build has no camera trace. Configure with -DWZ_PERF_COUNTERS=ON.");
	return false;
}

bool setAutoPan(const char *)
{
	debug(LOG_ERROR, "This build has no camera auto-pan. Configure with -DWZ_PERF_COUNTERS=ON.");
	return false;
}
}

#endif // defined(WZ_PERF_COUNTERS)
