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
#include "video_timeline.h"

#include <algorithm>
#include <cmath>
#include <limits>

static constexpr double EPSILON = 1e-4;	// seconds, below the container's 1 ms timestamp resolution

TimelineVideoSource::TimelineVideoSource(WZVideoDecoder& decoder, std::vector<WZVideoTimelineOp> timeline, double frameDuration)
: m_decoder(decoder)
, m_ops(std::move(timeline))
, m_frameDuration(frameDuration)
{
	if (m_ops.empty())
	{
		WZVideoTimelineOp straight;
		straight.type = WZVideoTimelineOp::Type::Play;
		straight.from = 0.0;
		straight.to = std::numeric_limits<double>::infinity();
		m_ops.push_back(straight);
	}
}

bool TimelineVideoSource::next(Item& out)
{
	while (m_opIndex < m_ops.size())
	{
		const WZVideoTimelineOp& op = m_ops[m_opIndex];
		switch (op.type)
		{
		case WZVideoTimelineOp::Type::Play:
		case WZVideoTimelineOp::Type::Loop:
			if (nextSourceFrame(op, out))
			{
				return true;
			}
			break;
		case WZVideoTimelineOp::Type::Hold:
			finishOp(m_opStart + op.duration);
			break;
		case WZVideoTimelineOp::Type::Black:
			if (!m_opStarted)
			{
				m_opStarted = true;
				out = Item();
				out.kind = Item::Kind::Black;
				out.time = m_opStart;
				return true;
			}
			finishOp(m_opStart + op.duration);
			break;
		case WZVideoTimelineOp::Type::HoldFade:
		{
			const size_t steps = std::max<size_t>(1, static_cast<size_t>(std::lround(op.duration / m_frameDuration)));
			if (m_fadeStep < steps)
			{
				out = Item();
				out.kind = Item::Kind::Fade;
				out.time = m_opStart + static_cast<double>(m_fadeStep) * m_frameDuration;
				out.fadeStep = m_fadeStep;
				out.fadeLevel = 1.f - static_cast<float>(m_fadeStep + 1) / static_cast<float>(steps);
				++m_fadeStep;
				return true;
			}
			finishOp(m_opStart + op.duration);
			break;
		}
		}
	}
	return false;
}

bool TimelineVideoSource::nextSourceFrame(const WZVideoTimelineOp& op, Item& out)
{
	const bool isLoop = (op.type == WZVideoTimelineOp::Type::Loop);
	const double opEnd = m_opStart + (isLoop ? op.duration : op.to - op.from);
	if (!m_opStarted)
	{
		m_opStarted = true;
		m_cycleStart = m_opStart;
		m_cycleFrames = 0;
		positionAt(op.from);
	}

	for (;;)
	{
		WZVideoFrameYUV frame;
		bool haveFrame = false;
		if (m_haveLookahead)
		{
			frame = m_lookahead;
			m_haveLookahead = false;
			haveFrame = true;
		}
		else
		{
			haveFrame = m_decoder.nextVideoFrame(frame);
			if (haveFrame)
			{
				m_nextSourcePts = frame.pts + m_frameDuration;
			}
		}

		const bool pastRange = !haveFrame || frame.pts >= op.to - EPSILON;
		if (haveFrame && pastRange)
		{
			// keep it: the next op may continue from here
			m_lookahead = frame;
			m_haveLookahead = true;
		}
		if (pastRange)
		{
			if (isLoop && m_cycleFrames > 0 && m_cycleStart + (op.to - op.from) < opEnd - EPSILON)
			{
				// start the next cycle
				m_cycleStart += op.to - op.from;
				m_cycleFrames = 0;
				positionAt(op.from);
				continue;
			}
			finishOp(opEnd);
			return false;
		}
		if (frame.pts < op.from - EPSILON)
		{
			continue;	// decoding forward from the keyframe before op.from
		}

		const double time = m_cycleStart + (frame.pts - op.from);
		if (time >= opEnd - EPSILON)
		{
			// a loop ending mid-cycle
			m_lookahead = frame;
			m_haveLookahead = true;
			finishOp(opEnd);
			return false;
		}
		++m_cycleFrames;
		out = Item();
		out.kind = Item::Kind::Frame;
		out.time = time;
		out.sourcePts = frame.pts;
		out.droppable = time + m_frameDuration < opEnd - EPSILON;
		out.frame = frame;
		return true;
	}
}

void TimelineVideoSource::positionAt(double pts)
{
	if (m_haveLookahead && std::abs(m_lookahead.pts - pts) < EPSILON)
	{
		return;
	}
	if (!m_haveLookahead && std::abs(m_nextSourcePts - pts) < EPSILON)
	{
		return;
	}
	if (!m_decoder.seekVideo(pts))
	{
		if (!m_seekFailureLogged)
		{
			debug(LOG_WARNING, "Video can't seek to %.3fs; continuing without the edit list's jumps", pts);
			m_seekFailureLogged = true;
		}
		return;
	}
	m_haveLookahead = false;
	m_nextSourcePts = -1.0;
}

void TimelineVideoSource::finishOp(double end)
{
	m_opStart = end;
	++m_opIndex;
	m_opStarted = false;
	m_fadeStep = 0;
}
