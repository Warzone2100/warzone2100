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

#pragma once

#include "video_decoder.h"
#include "video_edits.h"

#include <cstddef>
#include <vector>

/** The video frames to display, in presentation order, following an edit-list timeline.
 *
 * Without a timeline the video plays straight through, and output times equal the frames' own times.
 * With one, play and loop ops seek the video and map its frames onto the output timeline,
 * black and hold_fade ops produce synthetic items, and hold ops produce nothing
 * (the last item stays on screen).
 */
class TimelineVideoSource
{
public:
	struct Item
	{
		enum class Kind
		{
			Frame,
			Black,
			Fade,
		};
		Kind kind = Kind::Frame;
		double time = 0.0;			// output time
		double sourcePts = 0.0;		// Frame: the frame's time in the video
		bool droppable = false;		// Frame: a later frame of the same op replaces it (so it may be skipped when late)
		size_t fadeStep = 0;		// Fade: step number (0 = the first)
		float fadeLevel = 1.f;		// Fade: brightness of the held frame (0 = black)
		WZVideoFrameYUV frame;		// Frame: planes borrowed from the decoder
	};

	TimelineVideoSource(WZVideoDecoder& decoder, std::vector<WZVideoTimelineOp> timeline, double frameDuration);

	/** The next item to display.
	 * \returns false when the timeline (or, without one, the video) is exhausted */
	bool next(Item& out);

private:
	/** The next frame of a play or loop op. \returns false (having finished the op) when it has none left */
	bool nextSourceFrame(const WZVideoTimelineOp& op, Item& out);
	/** Make the next decoded frame the one at pts (or the first after it) */
	void positionAt(double pts);
	void finishOp(double end);

private:
	WZVideoDecoder& m_decoder;
	std::vector<WZVideoTimelineOp> m_ops;
	double m_frameDuration;

	size_t m_opIndex = 0;
	double m_opStart = 0.0;			// output time the current op starts at
	bool m_opStarted = false;
	double m_cycleStart = 0.0;		// Loop: output time the current cycle starts at
	size_t m_cycleFrames = 0;		// Loop: frames produced in the current cycle
	size_t m_fadeStep = 0;			// HoldFade: the next step

	WZVideoFrameYUV m_lookahead;	// a decoded frame not yet produced (plane pointers valid until the next decode)
	bool m_haveLookahead = false;
	double m_nextSourcePts = 0.0;	// pts of the frame the decoder produces next, if it continues (-1 = unknown)
	bool m_seekFailureLogged = false;
};
