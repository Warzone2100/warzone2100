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
#include "video_edits.h"
#include "video_decoder.h"

#include <nlohmann/json.hpp>

#include <cmath>
#include <string>

static const char *EDIT_LIST_FORMAT = "wz2100-sequence-edits";
static const int EDIT_LIST_VERSION = 1;

const WZVideoEditTrack *WZVideoEditList::findTrack(const WzString& languageCode) const
{
	for (const auto& track : tracks)
	{
		if (videoDecoderLanguageMatches(track.languageCode, languageCode))
		{
			return &track;
		}
	}
	return nullptr;
}

static bool readSeconds(const nlohmann::json& obj, const char *key, double& out)
{
	auto it = obj.find(key);
	if (it == obj.end() || !it->is_number())
	{
		return false;
	}
	out = it->get<double>();
	return std::isfinite(out) && out >= 0.0;
}

static bool readCount(const nlohmann::json& obj, const char *key, size_t& out)
{
	auto it = obj.find(key);
	if (it == obj.end() || !it->is_number_unsigned())
	{
		return false;
	}
	out = it->get<size_t>();
	return true;
}

static bool parseOp(const nlohmann::json& obj, WZVideoTimelineOp& op)
{
	if (!obj.is_object())
	{
		return false;
	}
	auto it = obj.find("op");
	if (it == obj.end() || !it->is_string())
	{
		return false;
	}
	const std::string type = it->get<std::string>();
	if (type == "play" || type == "loop")
	{
		op.type = (type == "play") ? WZVideoTimelineOp::Type::Play : WZVideoTimelineOp::Type::Loop;
		if (!readSeconds(obj, "from", op.from) || !readSeconds(obj, "to", op.to) || op.to <= op.from)
		{
			return false;
		}
		return op.type == WZVideoTimelineOp::Type::Play || readSeconds(obj, "duration", op.duration);
	}
	if (type == "hold")
	{
		op.type = WZVideoTimelineOp::Type::Hold;
	}
	else if (type == "black")
	{
		op.type = WZVideoTimelineOp::Type::Black;
	}
	else if (type == "hold_fade")
	{
		op.type = WZVideoTimelineOp::Type::HoldFade;
	}
	else
	{
		return false;
	}
	return readSeconds(obj, "duration", op.duration);
}

static bool parseTrack(const std::string& language, const nlohmann::json& obj, WZVideoEditTrack& track)
{
	if (!obj.is_object() || language.empty())
	{
		return false;
	}
	track.languageCode = WzString::fromUtf8(language);

	auto video = obj.find("video");
	if (video != obj.end())
	{
		if (!video->is_string() || video->get<std::string>().empty()
		    || !readSeconds(obj, "video_duration", track.videoDuration) || !readCount(obj, "video_frames", track.videoFrames))
		{
			return false;
		}
		track.videoName = WzString::fromUtf8(video->get<std::string>());
	}

	auto timeline = obj.find("timeline");
	if (timeline != obj.end())
	{
		if (!timeline->is_array() || timeline->empty())
		{
			return false;
		}
		for (const auto& item : *timeline)
		{
			WZVideoTimelineOp op;
			if (!parseOp(item, op))
			{
				return false;
			}
			track.timeline.push_back(op);
		}
	}
	return readSeconds(obj, "duration", track.duration);
}

std::shared_ptr<const WZVideoEditList> videoEditListParse(const char *data, size_t size, const WzString& sourceName)
{
	const nlohmann::json root = nlohmann::json::parse(data, data + size, nullptr, false);
	if (root.is_discarded() || !root.is_object())
	{
		debug(LOG_WARNING, "Ignoring edit list %s: not valid JSON", sourceName.toUtf8().c_str());
		return nullptr;
	}

	auto format = root.find("format");
	auto version = root.find("version");
	if (format == root.end() || !format->is_string() || format->get<std::string>() != EDIT_LIST_FORMAT
	    || version == root.end() || !version->is_number_integer())
	{
		debug(LOG_WARNING, "Ignoring edit list %s: not a sequence edit list", sourceName.toUtf8().c_str());
		return nullptr;
	}
	if (version->get<int>() != EDIT_LIST_VERSION)
	{
		debug(LOG_WARNING, "Ignoring edit list %s: unsupported version %d", sourceName.toUtf8().c_str(), version->get<int>());
		return nullptr;
	}

	auto result = std::make_shared<WZVideoEditList>();
	auto video = root.find("video");
	auto tracks = root.find("tracks");
	if (video == root.end() || !video->is_object()
	    || !readSeconds(*video, "duration", result->videoDuration) || !readCount(*video, "frames", result->videoFrames)
	    || !readSeconds(*video, "fps", result->fps) || result->fps <= 0.0
	    || tracks == root.end() || !tracks->is_object())
	{
		debug(LOG_WARNING, "Ignoring edit list %s: malformed header", sourceName.toUtf8().c_str());
		return nullptr;
	}

	for (const auto& entry : tracks->items())
	{
		WZVideoEditTrack track;
		if (!parseTrack(entry.key(), entry.value(), track))
		{
			debug(LOG_WARNING, "Ignoring edit list %s: malformed entry for \"%s\"", sourceName.toUtf8().c_str(), entry.key().c_str());
			return nullptr;
		}
		result->tracks.push_back(std::move(track));
	}
	return result;
}

bool videoEditListVideoMatches(double expectedDuration, size_t expectedFrames, double expectedFps, double actualDuration, double actualFps)
{
	if (expectedFps <= 0.0 || actualFps <= 0.0 || actualDuration <= 0.0)
	{
		return false;
	}
	const double halfFrame = 0.5 / expectedFps;
	return std::abs(actualFps - expectedFps) < 0.01
		&& std::abs(actualDuration - expectedDuration) < halfFrame
		&& static_cast<size_t>(std::lround(actualDuration * expectedFps)) == expectedFrames;
}

std::vector<WZVideoTimelineOp> videoEditListChooseTimeline(const WZVideoEditList& edits, const WzString& videoLanguage,
                                                           const WzString& audioLanguage, const WZVideoTrackMetadata& vmeta,
                                                           const WzString& videoName, double& timelineDuration)
{
	const WZVideoEditTrack *track = nullptr;
	bool videoMatches = false;
	if (!videoLanguage.isEmpty())
	{
		// a language's own video
		track = edits.findTrack(videoLanguage);
		videoMatches = track && track->videoDuration > 0.0
			&& videoEditListVideoMatches(track->videoDuration, track->videoFrames,
			                             static_cast<double>(track->videoFrames) / track->videoDuration, vmeta.duration, vmeta.fps);
	}
	else
	{
		track = edits.findTrack(audioLanguage);
		if (track && !track->videoName.isEmpty())
		{
			track = nullptr;	// its timeline belongs to its own video
		}
		videoMatches = videoEditListVideoMatches(edits.videoDuration, edits.videoFrames, edits.fps, vmeta.duration, vmeta.fps);
	}
	if (!track || track->timeline.empty())
	{
		return {};
	}
	if (!videoMatches)
	{
		debug(LOG_WARNING, "Ignoring the \"%s\" edit list of %s: the video doesn't match it (%.3fs at %.3f fps)",
		      track->languageCode.toUtf8().c_str(), videoName.toUtf8().c_str(), vmeta.duration, vmeta.fps);
		return {};
	}
	timelineDuration = track->duration;
	return track->timeline;
}
