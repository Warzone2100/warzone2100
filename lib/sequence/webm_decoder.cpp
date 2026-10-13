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

/** @file webm_decoder.cpp
 *
 * WebM (Matroska) video decoding: libwebm's mkvparser for demuxing, a
 * WZWebmVideoCodec backend for VP8/VP9 video, and lib/sound's packet decoders
 * for Opus/Vorbis audio.
 */

#include "lib/framework/frame.h"
#include "webm_decoder.h"
#include "webm_video_codec.h"
#include "lib/sound/codec_packet.h"

#include <mkvparser/mkvparser.h>

#include <algorithm>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

namespace
{

/** mkvparser random-access reader over a VideoProvider */
class MkvReaderAdapter : public mkvparser::IMkvReader
{
public:
	explicit MkvReaderAdapter(std::shared_ptr<VideoProvider> provider)
	: m_provider(std::move(provider))
	{ }

	int Read(long long pos, long len, unsigned char *buf) override
	{
		if (pos < 0 || len < 0)
		{
			return -1;
		}
		if (len == 0)
		{
			return 0;
		}
		if (!m_provider->seek(pos))
		{
			return -1;
		}
		uint64_t remaining = static_cast<uint64_t>(len);
		unsigned char *dest = buf;
		while (remaining > 0)
		{
			int64_t bytes = m_provider->read(dest, remaining);
			if (bytes <= 0)
			{
				return -1;	// short read = out-of-bounds request or I/O error
			}
			dest += bytes;
			remaining -= static_cast<uint64_t>(bytes);
		}
		return 0;
	}

	int Length(long long *total, long long *available) override
	{
		auto len = m_provider->length();
		if (!len.has_value())
		{
			return -1;
		}
		if (total)
		{
			*total = len.value();
		}
		if (available)
		{
			*available = len.value();
		}
		return 0;
	}

private:
	std::shared_ptr<VideoProvider> m_provider;
};

/** Parse a Vorbis-in-Matroska CodecPrivate blob: the three Vorbis header
 * packets stored with Xiph lacing. */
bool parseVorbisCodecPrivate(const uint8_t *data, size_t len,
                             std::array<std::pair<const uint8_t*, size_t>, 3>& out)
{
	if (!data || len < 3 || data[0] != 2)	// packet count - 1 must be 2
	{
		return false;
	}
	size_t pos = 1;
	size_t sizes[2] = {0, 0};
	for (int i = 0; i < 2; ++i)	// lengths of the first two packets, 255-run coded
	{
		for (;;)
		{
			if (pos >= len)
			{
				return false;
			}
			sizes[i] += data[pos];
			if (data[pos++] != 255)
			{
				break;
			}
		}
	}
	if (pos + sizes[0] + sizes[1] > len)
	{
		return false;
	}
	out[0] = { data + pos, sizes[0] };
	out[1] = { data + pos + sizes[0], sizes[1] };
	out[2] = { data + pos + sizes[0] + sizes[1], len - pos - sizes[0] - sizes[1] };
	return true;
}

struct DemuxedPacket
{
	std::vector<uint8_t> data;
	double pts;	// seconds
};

/** A read position in the file for one track: video and audio are demuxed independently,
 * so the video can seek while the audio plays on */
struct TrackCursor
{
	long long trackNumber = -1;
	const mkvparser::Cluster *cluster = nullptr;
	const mkvparser::BlockEntry *entry = nullptr;	// the last block read (nullptr = none yet in this cluster)
	bool resumeAtEntry = false;						// the next read returns entry itself (after a seek)
	std::deque<DemuxedPacket> queue;
};

class WebmVideoDecoder final : public WZVideoDecoder
{
public:
	WebmVideoDecoder(std::shared_ptr<VideoProvider> provider, WZWebmCodecBackend backendPreference)
	: m_provider(std::move(provider))
	, m_reader(m_provider)
	, m_backendPreference(backendPreference)
	{ }

	bool open();

	bool hasVideo() const override
	{
		return m_videoCodec != nullptr;
	}

	const WZVideoTrackMetadata& videoMetadata() const override
	{
		return m_videoMetadata;
	}

	const std::vector<WZAudioTrackMetadata>& audioTracks() const override
	{
		return m_audioTracks;
	}

	size_t selectedAudioTrack() const override
	{
		return m_selectedAudioIdx;
	}

	bool selectAudioTrack(size_t index) override;

	bool nextVideoFrame(WZVideoFrameYUV& out) override;
	bool seekVideo(double pts) override;
	size_t decodeAudio(int16_t *dest, size_t maxSamplesPerChannel, double& firstSamplePts) override;

private:
	bool createAudioDecoderFor(const WZAudioTrackMetadata& trackMeta);
	/** Demux the cursor's next block (of its own track, in file order) into its packet queue.
	 * \returns false when the input is exhausted (or on parse error) */
	bool demuxNextBlock(TrackCursor& cursor);
	/** The end time of the last video frame, found from the last cue point (or a full scan without cues) */
	double findVideoDuration() const;
	/** Switch to another backend, once per decoder. \returns false when there is none */
	bool replaceVideoCodec();

private:
	std::shared_ptr<VideoProvider> m_provider;
	MkvReaderAdapter m_reader;
	WZWebmCodecBackend m_backendPreference = WZWebmCodecBackend::Auto;

	std::unique_ptr<mkvparser::Segment, void(*)(mkvparser::Segment*)> m_segment{nullptr, [](mkvparser::Segment *s) { delete s; }};
	TrackCursor m_video;
	TrackCursor m_audio;

	std::unique_ptr<WZWebmVideoCodec> m_videoCodec;
	std::string m_videoCodecId;
	bool m_videoCodecReplaced = false;
	long long m_videoTrackNumber = -1;
	WZVideoTrackMetadata m_videoMetadata;

	std::vector<WZAudioTrackMetadata> m_audioTracks;
	std::vector<long long> m_audioTrackNumbers;	// mkv track number per m_audioTracks entry
	size_t m_selectedAudioIdx = 0;
	long long m_selectedAudioTrackNumber = -1;
	std::unique_ptr<WZPacketAudioDecoder> m_audioDecoder;
	bool m_audioDecodeStarted = false;

	unsigned m_consecutiveNoOutput = 0;
	double m_lastVideoPacketPts = 0.0;
	bool m_videoDraining = false;
	double m_videoDecodeStartPts = 0.0;		// where decoding last (re)started
	double m_lastVideoFramePts = -1.0;		// the last frame returned since then (-1 = none)
	double m_videoResumeAfterPts = -1.0;	// frames up to here were returned by a replaced codec

	uint64_t m_audioSamplesOut = 0;
	double m_audioBasePts = 0.0;
	bool m_haveAudioBasePts = false;
};

bool WebmVideoDecoder::open()
{
	mkvparser::EBMLHeader ebmlHeader;
	long long pos = 0;
	if (ebmlHeader.Parse(&m_reader, pos) != 0)
	{
		debug(LOG_ERROR, "Failed to parse EBML header: %s", m_provider->filename().toUtf8().c_str());
		return false;
	}

	mkvparser::Segment *segment = nullptr;
	if (mkvparser::Segment::CreateInstance(&m_reader, pos, segment) != 0 || !segment)
	{
		debug(LOG_ERROR, "Failed to parse Matroska segment: %s", m_provider->filename().toUtf8().c_str());
		return false;
	}
	m_segment.reset(segment);

	if (m_segment->Load() < 0)
	{
		debug(LOG_ERROR, "Failed to load Matroska segment: %s", m_provider->filename().toUtf8().c_str());
		return false;
	}

	const mkvparser::Tracks *tracks = m_segment->GetTracks();
	if (!tracks)
	{
		debug(LOG_ERROR, "No tracks in WebM file: %s", m_provider->filename().toUtf8().c_str());
		return false;
	}

	for (unsigned long i = 0; i < tracks->GetTracksCount(); ++i)
	{
		const mkvparser::Track *track = tracks->GetTrackByIndex(i);
		if (!track)
		{
			continue;
		}
		const char *codecId = track->GetCodecId();
		if (!codecId)
		{
			continue;
		}

		if (track->GetType() == mkvparser::Track::kVideo && !m_videoCodec)
		{
			const auto *videoTrack = static_cast<const mkvparser::VideoTrack *>(track);
			m_videoTrackNumber = track->GetNumber();
			m_videoMetadata.width = static_cast<unsigned>(videoTrack->GetWidth());
			m_videoMetadata.height = static_cast<unsigned>(videoTrack->GetHeight());
			const unsigned long long defaultDurationNs = track->GetDefaultDuration();
			m_videoMetadata.fps = (defaultDurationNs > 0) ? 1e9 / static_cast<double>(defaultDurationNs) : 0.0;
			debug(LOG_VIDEO, "WebM video track %lld: %s %ux%u %.02f fps",
			      m_videoTrackNumber, codecId, m_videoMetadata.width, m_videoMetadata.height, m_videoMetadata.fps);

			m_videoCodecId = codecId;
			m_videoCodec = wzWebmVideoCodecCreate(codecId, m_videoMetadata.width, m_videoMetadata.height, m_backendPreference);
			if (!m_videoCodec)
			{
				debug(LOG_ERROR, "Unsupported video codec '%s' in %s", codecId, m_provider->filename().toUtf8().c_str());
				return false;
			}
			if (const mkvparser::Colour *colour = videoTrack->GetColour())
			{
				// Matroska MatrixCoefficients (ISO/IEC 23091-4): 1 = BT.709, 5 and 6 = BT.601
				m_videoCodec->containerColorMatrix = (colour->matrix_coefficients == 1) ? WZVideoColorMatrix::BT709 : WZVideoColorMatrix::BT601;
				m_videoCodec->containerFullRange = (colour->range == 2);
			}
		}
		else if (track->GetType() == mkvparser::Track::kAudio)
		{
			const bool isOpus = (strcmp(codecId, "A_OPUS") == 0);
			const bool isVorbis = (strcmp(codecId, "A_VORBIS") == 0);
			if (!isOpus && !isVorbis)
			{
				debug(LOG_WARNING, "Ignoring audio track with unsupported codec '%s' in %s", codecId, m_provider->filename().toUtf8().c_str());
				continue;
			}
			const auto *audioTrack = static_cast<const mkvparser::AudioTrack *>(track);
			WZAudioTrackMetadata meta;
			meta.index = m_audioTracks.size();
			const char *lang = track->GetLanguage();
			meta.languageCode = (lang && lang[0] != '\0') ? lang : "eng";	// Matroska default language is English
			const char *name = track->GetNameAsUTF8();
			meta.name = name ? name : "";
			// mkvparser does not expose FlagDefault; treat the first listed audio track as the default
			meta.isDefault = m_audioTracks.empty();
			meta.channels = static_cast<unsigned>(audioTrack->GetChannels());
			// report the *decode output* rate: Opus always decodes at 48kHz
			meta.sampleRate = isOpus ? 48000 : static_cast<unsigned>(audioTrack->GetSamplingRate());
			debug(LOG_VIDEO, "WebM audio track %ld: %s %u ch %u Hz lang=%s%s",
			      track->GetNumber(), codecId, meta.channels, meta.sampleRate,
			      meta.languageCode.toUtf8().c_str(), meta.isDefault ? " (default)" : "");
			m_audioTracks.push_back(std::move(meta));
			m_audioTrackNumbers.push_back(track->GetNumber());
		}
	}

	if (!m_videoCodec && m_audioTracks.empty())
	{
		debug(LOG_ERROR, "No supported video or audio tracks in %s", m_provider->filename().toUtf8().c_str());
		return false;
	}

	m_video.trackNumber = m_videoTrackNumber;
	m_video.cluster = m_segment->GetFirst();

	// select the default audio track
	if (!m_audioTracks.empty() && !selectAudioTrack(0))
	{
		return false;
	}

	if (m_videoCodec)
	{
		m_videoMetadata.duration = findVideoDuration();
		debug(LOG_VIDEO, "WebM video duration: %.3fs", m_videoMetadata.duration);
	}
	return true;
}

double WebmVideoDecoder::findVideoDuration() const
{
	const mkvparser::Cluster *cluster = m_segment->GetFirst();
	const mkvparser::Cues *cues = m_segment->GetCues();
	const mkvparser::Track *track = m_segment->GetTracks()->GetTrackByNumber(static_cast<long>(m_videoTrackNumber));
	if (cues && track)
	{
		while (!cues->DoneParsing())
		{
			cues->LoadCuePoint();
		}
		const mkvparser::CuePoint *lastCue = cues->GetLast();
		const mkvparser::CuePoint::TrackPosition *position = lastCue ? lastCue->Find(track) : nullptr;
		const mkvparser::BlockEntry *entry = position ? cues->GetBlock(lastCue, position) : nullptr;
		if (entry && !entry->EOS())
		{
			cluster = entry->GetCluster();
		}
	}

	long long lastTimeNs = -1;
	for (; cluster && !cluster->EOS(); cluster = m_segment->GetNext(cluster))
	{
		const mkvparser::BlockEntry *entry = nullptr;
		if (cluster->GetFirst(entry) < 0)
		{
			break;
		}
		while (entry && !entry->EOS())
		{
			const mkvparser::Block *block = entry->GetBlock();
			if (block && block->GetTrackNumber() == m_videoTrackNumber)
			{
				lastTimeNs = std::max(lastTimeNs, block->GetTime(cluster));
			}
			if (cluster->GetNext(entry, entry) < 0)
			{
				break;
			}
		}
	}
	if (lastTimeNs < 0)
	{
		return 0.0;
	}
	const double frameDuration = (m_videoMetadata.fps > 0.0) ? 1.0 / m_videoMetadata.fps : 0.0;
	return static_cast<double>(lastTimeNs) / 1e9 + frameDuration;
}

bool WebmVideoDecoder::createAudioDecoderFor(const WZAudioTrackMetadata& trackMeta)
{
	const mkvparser::Track *track = m_segment->GetTracks()->GetTrackByNumber(static_cast<long>(m_audioTrackNumbers[trackMeta.index]));
	ASSERT_OR_RETURN(false, track != nullptr, "Track disappeared?");
	const char *codecId = track->GetCodecId();

	size_t privSize = 0;
	const unsigned char *priv = track->GetCodecPrivate(privSize);
	if (!priv || privSize == 0)
	{
		debug(LOG_ERROR, "Audio track %zu has no CodecPrivate data", trackMeta.index);
		return false;
	}

	std::unique_ptr<WZPacketAudioDecoder> decoder;
	if (strcmp(codecId, "A_OPUS") == 0)
	{
		// Note: the track's CodecDelay element mirrors OpusHead pre-skip, which
		// the packet decoder applies internally.
		decoder = wzOpusPacketDecoderCreate(priv, privSize);
	}
	else	// A_VORBIS (filtered during open())
	{
		std::array<std::pair<const uint8_t*, size_t>, 3> headerPackets;
		if (!parseVorbisCodecPrivate(priv, privSize, headerPackets))
		{
			debug(LOG_ERROR, "Malformed Vorbis CodecPrivate data on audio track %zu", trackMeta.index);
			return false;
		}
		decoder = wzVorbisPacketDecoderCreate(headerPackets);
	}

	if (!decoder)
	{
		debug(LOG_ERROR, "Failed to initialize audio decoder for track %zu", trackMeta.index);
		return false;
	}
	m_audioDecoder = std::move(decoder);
	return true;
}

bool WebmVideoDecoder::selectAudioTrack(size_t index)
{
	if (index >= m_audioTracks.size())
	{
		return false;
	}
	ASSERT(!m_audioDecodeStarted, "selectAudioTrack() must be called before audio decoding starts");

	if (!createAudioDecoderFor(m_audioTracks[index]))
	{
		return false;
	}
	m_audioTracks[index].channels = m_audioDecoder->channels();
	m_audioTracks[index].sampleRate = m_audioDecoder->sampleRate();
	m_selectedAudioIdx = index;
	m_selectedAudioTrackNumber = m_audioTrackNumbers[index];
	m_audio = TrackCursor();
	m_audio.trackNumber = m_selectedAudioTrackNumber;
	m_audio.cluster = m_segment->GetFirst();
	m_audioSamplesOut = 0;
	m_haveAudioBasePts = false;
	return true;
}

bool WebmVideoDecoder::demuxNextBlock(TrackCursor& cursor)
{
	for (;;)
	{
		if (!cursor.cluster || cursor.cluster->EOS())
		{
			return false;
		}

		const mkvparser::BlockEntry *entry = nullptr;
		if (cursor.resumeAtEntry)
		{
			entry = cursor.entry;
			cursor.resumeAtEntry = false;
		}
		else
		{
			long status = (cursor.entry == nullptr) ? cursor.cluster->GetFirst(entry)
			                                        : cursor.cluster->GetNext(cursor.entry, entry);
			if (status < 0)
			{
				debug(LOG_ERROR, "Error parsing WebM cluster: %s", m_provider->filename().toUtf8().c_str());
				return false;
			}
		}

		if (!entry || entry->EOS())
		{
			cursor.cluster = m_segment->GetNext(cursor.cluster);
			cursor.entry = nullptr;
			continue;
		}
		cursor.entry = entry;

		const mkvparser::Block *block = entry->GetBlock();
		if (!block || block->GetTrackNumber() != cursor.trackNumber)
		{
			continue;	// another track's block: skip without copying
		}

		const double pts = static_cast<double>(block->GetTime(cursor.cluster)) / 1e9;
		for (int i = 0; i < block->GetFrameCount(); ++i)
		{
			const mkvparser::Block::Frame& frame = block->GetFrame(i);
			DemuxedPacket packet;
			packet.data.resize(static_cast<size_t>(frame.len));
			if (frame.Read(&m_reader, packet.data.data()) != 0)
			{
				debug(LOG_ERROR, "Error reading WebM block frame: %s", m_provider->filename().toUtf8().c_str());
				return false;
			}
			packet.pts = pts;
			cursor.queue.push_back(std::move(packet));
		}
		return true;
	}
}

bool WebmVideoDecoder::nextVideoFrame(WZVideoFrameYUV& out)
{
	if (!m_videoCodec)
	{
		return false;
	}

	for (;;)
	{
		// a buffering backend may hold several frames
		if (m_videoCodec->getFrame(out))
		{
			if (!m_videoCodec->providesFramePts())
			{
				out.pts = m_lastVideoPacketPts;
			}
			m_consecutiveNoOutput = 0;
			if (out.pts <= m_videoResumeAfterPts)
			{
				continue;
			}
			m_lastVideoFramePts = out.pts;
			return true;
		}

		if (m_videoCodec->failed() && replaceVideoCodec())
		{
			continue;
		}
		if (m_videoDraining)
		{
			return false;
		}

		if (m_video.queue.empty())
		{
			if (!demuxNextBlock(m_video))
			{
				m_videoCodec->drain();
				m_videoDraining = true;
			}
			continue;
		}

		// a packet without output is skippable (corrupt, an invisible alt-ref frame, a pipeline filling),
		// but a stream that never decodes (ex. an unsupported VP9 profile) must fail promptly
		if (m_consecutiveNoOutput >= 32)
		{
			if (replaceVideoCodec())
			{
				continue;
			}
			debug(LOG_ERROR, "Video stream produced no displayable frames (unsupported profile/format?): %s", m_provider->filename().toUtf8().c_str());
			return false;
		}

		DemuxedPacket packet = std::move(m_video.queue.front());
		m_video.queue.pop_front();
		m_lastVideoPacketPts = packet.pts;
		m_videoCodec->decode(packet.data.data(), packet.data.size(), packet.pts);
		++m_consecutiveNoOutput;
	}
}

bool WebmVideoDecoder::seekVideo(double pts)
{
	const mkvparser::Track *track = m_videoCodec ? m_segment->GetTracks()->GetTrackByNumber(static_cast<long>(m_videoTrackNumber)) : nullptr;
	const mkvparser::BlockEntry *entry = nullptr;
	if (!track || track->Seek(static_cast<long long>(std::max(pts, 0.0) * 1e9 + 0.5), entry) < 0 || !entry || entry->EOS())
	{
		return false;
	}
	m_video.cluster = entry->GetCluster();
	m_video.entry = entry;
	m_video.resumeAtEntry = true;
	m_video.queue.clear();
	m_consecutiveNoOutput = 0;
	m_videoCodec->flush();
	m_videoDraining = false;
	m_videoDecodeStartPts = pts;
	m_lastVideoFramePts = -1.0;
	m_videoResumeAfterPts = -1.0;
	return true;
}

bool WebmVideoDecoder::replaceVideoCodec()
{
	if (m_videoCodecReplaced)
	{
		return false;
	}
	m_videoCodecReplaced = true;
	auto replacement = wzWebmVideoCodecCreateReplacement(m_videoCodecId.c_str(), m_videoMetadata.width, m_videoMetadata.height, *m_videoCodec);
	if (!replacement)
	{
		return false;
	}
	debug(LOG_WARNING, "Video decode backend '%s' stopped producing frames, switching to '%s': %s",
	      m_videoCodec->backendName(), replacement->backendName(), m_provider->filename().toUtf8().c_str());
	replacement->containerColorMatrix = m_videoCodec->containerColorMatrix;
	replacement->containerFullRange = m_videoCodec->containerFullRange;
	m_videoCodec = std::move(replacement);

	// restart at a key frame, skipping the frames already shown
	const double resumeAfterPts = m_lastVideoFramePts;
	const double seekPts = (resumeAfterPts >= 0.0) ? resumeAfterPts : m_videoDecodeStartPts;
	if (!seekVideo(seekPts))
	{
		debug(LOG_WARNING, "Video can't seek to %.3fs, continuing from the next key frame", seekPts);
		m_consecutiveNoOutput = 0;
		m_videoDraining = false;
		return true;
	}
	m_videoResumeAfterPts = resumeAfterPts;
	return true;
}

size_t WebmVideoDecoder::decodeAudio(int16_t *dest, size_t maxSamplesPerChannel, double& firstSamplePts)
{
	if (!m_audioDecoder)
	{
		return 0;
	}
	m_audioDecodeStarted = true;

	for (;;)
	{
		size_t samples = m_audioDecoder->pcmOut(dest, maxSamplesPerChannel);
		if (samples > 0)
		{
			firstSamplePts = m_audioBasePts + static_cast<double>(m_audioSamplesOut) / m_audioDecoder->sampleRate();
			m_audioSamplesOut += samples;
			return samples;
		}

		if (!m_audio.queue.empty())
		{
			DemuxedPacket packet = std::move(m_audio.queue.front());
			m_audio.queue.pop_front();
			if (!m_haveAudioBasePts)
			{
				m_audioBasePts = packet.pts;
				m_haveAudioBasePts = true;
			}
			m_audioDecoder->submitPacket(packet.data.data(), packet.data.size());
			continue;
		}

		if (!demuxNextBlock(m_audio))
		{
			return 0;
		}
	}
}

} // anonymous namespace

std::unique_ptr<WZVideoDecoder> webmVideoDecoderOpen(std::shared_ptr<VideoProvider> provider)
{
	if (!provider)
	{
		return nullptr;
	}
	auto decoder = std::make_unique<WebmVideoDecoder>(std::move(provider), wzWebmPreferredCodecBackend());
	if (!decoder->open())
	{
		return nullptr;
	}
	return decoder;
}
