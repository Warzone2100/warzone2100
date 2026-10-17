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
/** \file
 *  The replay timeline (see replay_controller.h): the replay's length as a track, the part played,
 *  a mark where each ring snapshot is (seeking back to there is quick), and the playhead.
 *  Click or drag to seek, and hover for the time.
 */

#pragma once

#include "lib/widget/widget.h"

#include <memory>
#include <nonstd/optional.hpp>

// The track is a short strip, and the widget is taller so the square playhead can overhang it
constexpr int REPLAY_TIMELINE_STRIP_HEIGHT = 8;
constexpr int REPLAY_PLAYHEAD_SIZE = 12;
constexpr float REPLAY_TIMELINE_CORNER_RADIUS = 1.f;

class ReplayTimelineWidget : public WIDGET
{
public:
	static std::shared_ptr<ReplayTimelineWidget> make();

	void display(int xOffset, int yOffset) override;
	void clicked(W_CONTEXT *psContext, WIDGET_KEY key) override;
	void released(W_CONTEXT *psContext, WIDGET_KEY key) override;
	bool capturesMouseDrag(WIDGET_KEY) override;
	bool isGamepadCursorMagnetTarget() const override;

protected:
	void mouseDragged(WIDGET_KEY key, W_CONTEXT *start, W_CONTEXT *current) override;

private:
	int trackInset() const;
	int trackWidth() const;
	int msToX(int x0, uint32_t duration, uint32_t timeMs) const;
	uint32_t xToMs(int localX) const;
	uint32_t seekMsFromContext(W_CONTEXT *psContext) const;
	void drawPlayhead(float centerX, float centerY, PIELIGHT colour) const;
	void drawCursorTimeTip(uint32_t timeMs) const;

	nonstd::optional<uint32_t> ghostMs_;
	bool dragging_ = false;
};
