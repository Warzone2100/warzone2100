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
#include "video_provider.h"

#include <cstdint>
#include <memory>
#include <vector>

/** One decoded video frame, as YUV 4:2:0 planes.
 *
 * The plane pointers are *borrowed* from the decoder: they remain valid only
 * until the next call to WZVideoDecoder::nextVideoFrame().
 */
struct WZVideoFrameYUV
{
	const uint8_t *y = nullptr;
	const uint8_t *u = nullptr;
	const uint8_t *v = nullptr;
	int yStride = 0;
	int uvStride = 0;
	unsigned width = 0;			// visible frame dimensions
	unsigned height = 0;
	double pts = 0.0;			// presentation time, in seconds
};

struct WZVideoTrackMetadata
{
	unsigned width = 0;
	unsigned height = 0;
	double fps = 0.0;			// informational; frame scheduling should use per-frame pts
};

struct WZAudioTrackMetadata
{
	size_t index = 0;			// decoder-local track index (for selectAudioTrack())
	WzString languageCode;		// ISO 639-2 language ("eng", ...); "und" when the container doesn't say
	WzString name;				// optional human-readable track name
	bool isDefault = false;		// container default-track flag
	unsigned channels = 0;
	unsigned sampleRate = 0;
};

/** A decode session for one video: container demuxing + video & audio decoding.
 *
 * Pull-based: the playback engine asks for decoded video frames and PCM audio,
 * and implementations internally read + demux more input as needed, queueing
 * demuxed packets per selected track. Only the selected audio track is decoded.
 */
class WZVideoDecoder
{
public:
	virtual ~WZVideoDecoder();

	virtual bool hasVideo() const = 0;
	virtual const WZVideoTrackMetadata& videoMetadata() const = 0;

	virtual const std::vector<WZAudioTrackMetadata>& audioTracks() const = 0;
	bool hasAudio() const { return !audioTracks().empty(); }
	/** The audioTracks() index of the currently selected audio track */
	virtual size_t selectedAudioTrack() const = 0;
	/** Select which audio track to decode. Must be called before the first
	 * decodeAudio() call; the default is the container's default track. */
	virtual bool selectAudioTrack(size_t index) = 0;

	/** Decode the next video frame (in presentation order).
	 * \returns false when the video stream is exhausted (or on unrecoverable error) */
	virtual bool nextVideoFrame(WZVideoFrameYUV& out) = 0;

	/** Decode more audio from the selected track: writes up to
	 * maxSamplesPerChannel interleaved int16 samples to dest.
	 * \param firstSamplePts set to the presentation time (seconds) of the first sample written
	 * \returns samples (per channel) written; 0 when the audio stream is exhausted */
	virtual size_t decodeAudio(int16_t *dest, size_t maxSamplesPerChannel, double& firstSamplePts) = 0;
};

/** Open a video: sniffs the container format from the provider's first bytes
 * and returns the appropriate decoder, or nullptr (with a logged error). */
std::unique_ptr<WZVideoDecoder> videoDecoderOpen(std::shared_ptr<VideoProvider> provider);

/** Whether this build includes WebM (VP8/VP9) support (WZ_ENABLE_WEBM) */
bool videoDecoderWebmSupported();

/** Choose which audio track to play, by language:
 *  1. the track matching preferredLanguage (if non-empty),
 *  2. else the track tagged English (untagged tracks count as English),
 *  3. else the container default / first track.
 * preferredLanguage is a WZ locale code ("de", "pt_BR", ...); track languages
 * are ISO 639-2 (B or T form) or BCP-47 codes; matching handles all of these,
 * case-insensitively, on the primary language subtag.
 * \returns an index into tracks (0 if tracks is empty) */
size_t videoDecoderChooseAudioTrack(const std::vector<WZAudioTrackMetadata>& tracks, const WzString& preferredLanguage);
