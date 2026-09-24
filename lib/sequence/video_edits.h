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

#include "lib/framework/wzstring.h"

#include <cstddef>
#include <memory>
#include <vector>

struct WZVideoTrackMetadata;

/** Edit lists ("<name>.edits.json", next to a video).
 *
 * An edit list tells the sequence player how to present a video's picture for a given audio language, so that
 * a language whose audio runs longer or shorter than the video stays in sync.
 * The audio track itself plays straight through.
 * Each language entry may also name a separate video to play instead of the shared one.
 *
 * Example:
 *   {
 *    format": "wz2100-sequence-edits",
 *    "version": 1,
 *    "video": {"duration": s, "frames": n, "fps": f},
 *    "tracks": {"<language>": {"video": "<file>", "video_duration": s, "video_frames": n, timeline": [ops], "duration": s}}
 *   }
 *
 * "video", "video_duration" and "video_frames" are optional (either all three or none), as is "timeline".
 *
 * Ops (times in seconds, on the timeline of the video they apply to):
 *   {"op": "play", "from": a, "to": b}      play [a, b), seeking first when a isn't where the previous op ended
 *   {"op": "loop", "from": a, "to": b, "duration": d}   play [a, b) repeatedly for d (may end mid-cycle)
 *   {"op": "hold", "duration": d}           keep showing the last frame
 *   {"op": "black", "duration": d}          show black
 *   {"op": "hold_fade", "duration": d}      keep showing the last frame while it fades linearly to black
 */

struct WZVideoTimelineOp
{
	enum class Type
	{
		Play,
		Loop,
		Hold,
		Black,
		HoldFade,
	};
	Type type = Type::Play;
	double from = 0.0;			///< Play, Loop: the first source time
	double to = 0.0;			///< Play, Loop: the end source time (exclusive)
	double duration = 0.0;		///< Loop, Hold, Black, HoldFade: the output length
};

struct WZVideoEditTrack
{
	WzString languageCode;						///< as written in the edit list (ex. "fre")
	WzString videoName;							///< this language's own video, next to the edit list (empty = the shared one)
	double videoDuration = 0.0;					///< videoName's expected duration
	size_t videoFrames = 0;						///< videoName's expected frame count
	std::vector<WZVideoTimelineOp> timeline;	///< empty = play the video straight through
	double duration = 0.0;						///< the timeline's total length
};

struct WZVideoEditList
{
	double videoDuration = 0.0;		///< the shared video's expected duration
	size_t videoFrames = 0;			///< the shared video's expected frame count
	double fps = 0.0;				///< the shared video's expected frame rate
	std::vector<WZVideoEditTrack> tracks;

	/** The entry for a language (matched like audio track languages), or nullptr */
	const WZVideoEditTrack *findTrack(const WzString& languageCode) const;
};

/** Parse an edit list. Logs a warning and returns nullptr if it is malformed or an unknown version. */
std::shared_ptr<const WZVideoEditList> videoEditListParse(const char *data, size_t size, const WzString& sourceName);

/** Whether a decoded video matches an edit list's expectations (within half a frame) */
bool videoEditListVideoMatches(double expectedDuration, size_t expectedFrames, double expectedFps, double actualDuration, double actualFps);

/** The timeline to apply to a video:
 * - the entry for audioLanguage when playing the shared video
 * - or the entry for videoLanguage when playing that language's own video
 *
 * Empty (play straight through) when there is no such entry, it has no timeline, or the video doesn't match it (which logs a warning).
 * \param timelineDuration set to the timeline's total length when one is returned */
std::vector<WZVideoTimelineOp> videoEditListChooseTimeline(const WZVideoEditList& edits, const WzString& videoLanguage,
                                                           const WzString& audioLanguage, const WZVideoTrackMetadata& vmeta,
                                                           const WzString& videoName, double& timelineDuration);
