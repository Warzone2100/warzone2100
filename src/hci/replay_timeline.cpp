/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project

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

#include "replay_timeline.h"
#include "replay_controller.h"

#include "lib/framework/frame.h"
#include "lib/framework/geometry.h"
#include "lib/framework/input.h"
#include "lib/framework/math_ext.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/piepalette.h"
#include "lib/ivis_opengl/pietypes.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/ivis_opengl/textdraw.h"

#include <algorithm>

std::shared_ptr<ReplayTimelineWidget> ReplayTimelineWidget::make()
{
	return std::make_shared<ReplayTimelineWidget>();
}

int ReplayTimelineWidget::trackInset() const
{
	return REPLAY_PLAYHEAD_SIZE / 2;
}

int ReplayTimelineWidget::trackWidth() const
{
	return std::max(1, width() - REPLAY_PLAYHEAD_SIZE);
}

uint32_t ReplayTimelineWidget::xToMs(int localX) const
{
	const uint32_t duration = replayplayback::durationMs();
	const int trackW = trackWidth();
	if (duration == 0 || trackW <= 0)
	{
		return 0;
	}
	const int clamped = std::clamp(localX - trackInset(), 0, trackW);
	return static_cast<uint32_t>((static_cast<uint64_t>(clamped) * duration) / static_cast<uint32_t>(trackW));
}

int ReplayTimelineWidget::msToX(int x0, uint32_t duration, uint32_t timeMs) const
{
	const uint32_t clamped = std::min(timeMs, duration);
	return x0 + trackInset() + static_cast<int>((static_cast<uint64_t>(clamped) * static_cast<uint32_t>(trackWidth())) / duration);
}

uint32_t ReplayTimelineWidget::seekMsFromContext(W_CONTEXT *psContext) const
{
	const uint32_t t = xToMs(psContext->mx - x());
	return (t / REPLAY_SEEK_QUANTUM_MS) * REPLAY_SEEK_QUANTUM_MS;
}

bool ReplayTimelineWidget::capturesMouseDrag(WIDGET_KEY)
{
	return !replayplayback::isBusy();
}

bool ReplayTimelineWidget::isGamepadCursorMagnetTarget() const
{
	return !replayplayback::isBusy();
}

void ReplayTimelineWidget::clicked(W_CONTEXT *psContext, WIDGET_KEY)
{
	if (replayplayback::isBusy())
	{
		return;
	}
	dragging_ = true;
	ghostMs_ = seekMsFromContext(psContext);
}

void ReplayTimelineWidget::mouseDragged(WIDGET_KEY, W_CONTEXT *, W_CONTEXT *current)
{
	if (dragging_)
	{
		ghostMs_ = seekMsFromContext(current);
	}
}

void ReplayTimelineWidget::released(W_CONTEXT *psContext, WIDGET_KEY)
{
	if (dragging_)
	{
		const uint32_t target = seekMsFromContext(psContext);
		replayplayback::seekTo(target);
	}
	dragging_ = false;
	ghostMs_.reset();
}

void ReplayTimelineWidget::drawPlayhead(float centerX, float centerY, PIELIGHT colour) const
{
	const float half = static_cast<float>(REPLAY_PLAYHEAD_SIZE) * 0.5f;
	pie_DrawRoundedBox(centerX - half, centerY - half, centerX + half, centerY + half, colour, REPLAY_TIMELINE_CORNER_RADIUS);
}

void ReplayTimelineWidget::drawCursorTimeTip(uint32_t timeMs) const
{
	const uint32_t duration = replayplayback::durationMs();
	if (duration == 0)
	{
		return;
	}
	constexpr int hGap = 6;
	constexpr int vGap = 3;
	constexpr int cursorGap = 8;
	const bool useHours = duration >= 3600u * 1000u;
	const std::string label = replayplayback::formatTime(timeMs, useHours);
	const int textW = iV_GetTextWidth(label.c_str(), font_regular);
	const int textH = iV_GetTextLineSize(font_regular);
	const int boxW = textW + hGap * 2;
	const int boxH = vGap * 2 + textH;
	const int boxX = clip<int>(mouseX() - boxW / 2, 0, std::max(0, static_cast<int>(screenWidth) - boxW));
	int boxY = mouseY() - boxH - cursorGap;
	if (boxY < 0)
	{
		boxY = std::min(mouseY() + cursorGap, std::max(0, static_cast<int>(screenHeight) - boxH));
	}
	iV_ShadowBox(boxX, boxY, boxX + boxW, boxY + boxH, 1, WZCOL_FORM_LIGHT, WZCOL_FORM_DARK, WZCOL_FORM_TIP_BACKGROUND);
	iV_SetTextColour(WZCOL_WHITE);
	iV_DrawText(label.c_str(), boxX + hGap, boxY + vGap - iV_GetTextAboveBase(font_regular), font_regular);
}

void ReplayTimelineWidget::display(int xOffset, int yOffset)
{
	const int x0 = x() + xOffset;
	const int y0 = y() + yOffset;
	const float trackX0 = static_cast<float>(x0 + trackInset());
	const float trackX1 = trackX0 + static_cast<float>(trackWidth());
	const float trackY0 = static_cast<float>(y0) + static_cast<float>(height() - REPLAY_TIMELINE_STRIP_HEIGHT) * 0.5f;
	const float trackY1 = trackY0 + static_cast<float>(REPLAY_TIMELINE_STRIP_HEIGHT);
	const float radius = REPLAY_TIMELINE_CORNER_RADIUS;
	const PIELIGHT noBorder = pal_RGBA(0, 0, 0, 0);

	pie_DrawRoundedBox(trackX0, trackY0, trackX1, trackY1, pal_RGBA(255, 255, 255, 35), radius);

	const uint32_t duration = replayplayback::durationMs();
	if (duration == 0 || trackWidth() <= 0)
	{
		return;
	}

	const auto fillTrackRange = [&](uint32_t fromMs, uint32_t toMs, PIELIGHT colour) {
		if (toMs <= fromMs)
		{
			return;
		}
		const int xStart = msToX(x0, duration, fromMs);
		const int xEnd = msToX(x0, duration, toMs);
		if (xEnd <= xStart)
		{
			return;
		}
		const WzRect clipRect(xStart, static_cast<int>(trackY0), xEnd - xStart, REPLAY_TIMELINE_STRIP_HEIGHT);
		pie_DrawRoundedBoxClipped(trackX0, trackY0, trackX1, trackY1, colour, radius, noBorder, 0.f, clipRect);
	};

	const uint32_t playheadMs = replayplayback::playheadMs();
	fillTrackRange(0, playheadMs, pal_RGBA(218, 207, 255, 160));

	// The snapshots: seeking back to (just after) one of them is quick
	for (uint32_t t : replayplayback::snapshotTimes())
	{
		const float mx = static_cast<float>(msToX(x0, duration, t));
		pie_UniTransBoxFill(mx - 0.5f, trackY1, mx + 0.5f, trackY1 + 3.f, pal_RGBA(255, 255, 255, 120));
	}

	const float centerY = static_cast<float>(y0) + static_cast<float>(height()) * 0.5f;
	if (replayplayback::isBusy())
	{
		// Where the seek is going
		drawPlayhead(static_cast<float>(msToX(x0, duration, replayplayback::seekTargetMs())), centerY, pal_RGBA(255, 255, 255, 110));
	}
	else if (ghostMs_.has_value())
	{
		drawPlayhead(static_cast<float>(msToX(x0, duration, *ghostMs_)), centerY, pal_RGBA(255, 255, 255, 160));
	}
	drawPlayhead(static_cast<float>(msToX(x0, duration, playheadMs)), centerY, pal_RGBA(255, 255, 255, 230));
	if (ghostMs_.has_value())
	{
		drawCursorTimeTip(*ghostMs_);
	}
	else if (isMouseOverWidget() && !replayplayback::isBusy())
	{
		const uint32_t t = xToMs(mouseX() - screenPosX());
		drawCursorTimeTip((t / REPLAY_SEEK_QUANTUM_MS) * REPLAY_SEEK_QUANTUM_MS);
	}
}
