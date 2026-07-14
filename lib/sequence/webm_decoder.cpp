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
 * WebM (Matroska) video decoding: libwebm's mkvparser for demuxing, libvpx for
 * VP8/VP9 video, and lib/sound's packet decoders for Opus/Vorbis audio.
 *
 * The video codec is isolated behind WebmVideoCodec so that a future codec
 * decoder only needs one new class and no demuxer changes.
 */

#include "lib/framework/frame.h"
#include "webm_decoder.h"
#include "lib/sound/codec_packet.h"

#include <mkvparser/mkvparser.h>
#include <vpx/vpx_decoder.h>
#include <vpx/vp8dx.h>

#include <algorithm>
#include <cstring>
#include <deque>
#include <thread>
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

/** The codec-specific part of WebM video decoding */
class WebmVideoCodec
{
public:
	virtual ~WebmVideoCodec() { }
	/** Feed one compressed frame */
	virtual bool decode(const uint8_t *data, size_t len) = 0;
	/** Get the (borrowed) displayable frame from the last decode; false if none
	 * (e.g. an invisible alt-ref frame). Planes are valid until the next decode(). */
	virtual bool getFrame(WZVideoFrameYUV& out) = 0;
};

class VpxVideoCodec final : public WebmVideoCodec
{
public:
	static std::unique_ptr<VpxVideoCodec> create(const char *codecId)
	{
		vpx_codec_iface_t *iface = nullptr;
		if (strcmp(codecId, "V_VP8") == 0)
		{
			iface = vpx_codec_vp8_dx();
		}
		else if (strcmp(codecId, "V_VP9") == 0)
		{
			iface = vpx_codec_vp9_dx();
		}
		if (!iface)
		{
			return nullptr;
		}

		auto codec = std::unique_ptr<VpxVideoCodec>(new VpxVideoCodec());
		vpx_codec_dec_cfg_t cfg;
		memset(&cfg, 0, sizeof(cfg));
		cfg.threads = static_cast<unsigned>(std::max(1u, std::min(4u, std::thread::hardware_concurrency())));
		if (vpx_codec_dec_init(&codec->m_ctx, iface, &cfg, 0) != VPX_CODEC_OK)
		{
			debug(LOG_ERROR, "vpx_codec_dec_init() failed: %s", vpx_codec_error(&codec->m_ctx));
			return nullptr;
		}
		codec->m_initialized = true;
		debug(LOG_VIDEO, "Initialized libvpx %s decoder (%u threads)", codecId, cfg.threads);
		return codec;
	}

	~VpxVideoCodec() override
	{
		if (m_initialized)
		{
			vpx_codec_destroy(&m_ctx);
		}
	}

	bool decode(const uint8_t *data, size_t len) override
	{
		if (vpx_codec_decode(&m_ctx, data, static_cast<unsigned>(len), nullptr, 0) != VPX_CODEC_OK)
		{
			debug(LOG_WARNING, "vpx_codec_decode() failed: %s", vpx_codec_error(&m_ctx));
			return false;
		}
		return true;
	}

	bool getFrame(WZVideoFrameYUV& out) override
	{
		vpx_codec_iter_t iter = nullptr;
		vpx_image_t *img = vpx_codec_get_frame(&m_ctx, &iter);
		if (!img)
		{
			return false;
		}
		if (img->fmt != VPX_IMG_FMT_I420)
		{
			if (!m_formatErrorLogged)
			{
				debug(LOG_ERROR, "Video is not 8-bit YUV420 (vpx format %d); only VP8/VP9 profile 0 is supported", (int)img->fmt);
				m_formatErrorLogged = true;
			}
			return false;
		}
		out.y = img->planes[VPX_PLANE_Y];
		out.u = img->planes[VPX_PLANE_U];
		out.v = img->planes[VPX_PLANE_V];
		out.yStride = img->stride[VPX_PLANE_Y];
		out.uvStride = img->stride[VPX_PLANE_U];
		out.width = img->d_w;
		out.height = img->d_h;
		return true;
	}

private:
	VpxVideoCodec() { }

	vpx_codec_ctx_t m_ctx;
	bool m_initialized = false;
	bool m_formatErrorLogged = false;
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

class WebmVideoDecoder final : public WZVideoDecoder
{
public:
	explicit WebmVideoDecoder(std::shared_ptr<VideoProvider> provider)
	: m_provider(std::move(provider))
	, m_reader(m_provider)
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
	size_t decodeAudio(int16_t *dest, size_t maxSamplesPerChannel, double& firstSamplePts) override;

private:
	bool createAudioDecoderFor(const WZAudioTrackMetadata& trackMeta);
	/** Demux the next block in file order into the per-track packet queues.
	 * \returns false when the input is exhausted (or on parse error) */
	bool demuxNextBlock();

private:
	std::shared_ptr<VideoProvider> m_provider;
	MkvReaderAdapter m_reader;

	std::unique_ptr<mkvparser::Segment, void(*)(mkvparser::Segment*)> m_segment{nullptr, [](mkvparser::Segment *s) { delete s; }};
	const mkvparser::Cluster *m_cluster = nullptr;
	const mkvparser::BlockEntry *m_currentEntry = nullptr;

	std::unique_ptr<WebmVideoCodec> m_videoCodec;
	long long m_videoTrackNumber = -1;
	WZVideoTrackMetadata m_videoMetadata;

	std::vector<WZAudioTrackMetadata> m_audioTracks;
	std::vector<long long> m_audioTrackNumbers;	// mkv track number per m_audioTracks entry
	size_t m_selectedAudioIdx = 0;
	long long m_selectedAudioTrackNumber = -1;
	std::unique_ptr<WZPacketAudioDecoder> m_audioDecoder;
	bool m_audioDecodeStarted = false;

	std::deque<DemuxedPacket> m_videoQueue;
	std::deque<DemuxedPacket> m_audioQueue;
	unsigned m_consecutiveNoOutput = 0;

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
			m_videoCodec = VpxVideoCodec::create(codecId);
			if (!m_videoCodec)
			{
				debug(LOG_ERROR, "Unsupported video codec '%s' in %s", codecId, m_provider->filename().toUtf8().c_str());
				return false;
			}
			const auto *videoTrack = static_cast<const mkvparser::VideoTrack *>(track);
			m_videoTrackNumber = track->GetNumber();
			m_videoMetadata.width = static_cast<unsigned>(videoTrack->GetWidth());
			m_videoMetadata.height = static_cast<unsigned>(videoTrack->GetHeight());
			const unsigned long long defaultDurationNs = track->GetDefaultDuration();
			m_videoMetadata.fps = (defaultDurationNs > 0) ? 1e9 / static_cast<double>(defaultDurationNs) : 0.0;
			debug(LOG_VIDEO, "WebM video track %lld: %s %ux%u %.02f fps",
			      m_videoTrackNumber, codecId, m_videoMetadata.width, m_videoMetadata.height, m_videoMetadata.fps);
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

	// select the default audio track
	if (!m_audioTracks.empty() && !selectAudioTrack(0))
	{
		return false;
	}

	m_cluster = m_segment->GetFirst();
	return true;
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

	if (strcmp(codecId, "A_OPUS") == 0)
	{
		// Note: the track's CodecDelay element mirrors OpusHead pre-skip, which
		// the packet decoder applies internally.
		m_audioDecoder = wzOpusPacketDecoderCreate(priv, privSize);
	}
	else	// A_VORBIS (filtered during open())
	{
		std::array<std::pair<const uint8_t*, size_t>, 3> headerPackets;
		if (!parseVorbisCodecPrivate(priv, privSize, headerPackets))
		{
			debug(LOG_ERROR, "Malformed Vorbis CodecPrivate data on audio track %zu", trackMeta.index);
			return false;
		}
		m_audioDecoder = wzVorbisPacketDecoderCreate(headerPackets);
	}

	if (!m_audioDecoder)
	{
		debug(LOG_ERROR, "Failed to initialize audio decoder for track %zu", trackMeta.index);
		return false;
	}
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
	m_selectedAudioIdx = index;
	m_selectedAudioTrackNumber = m_audioTrackNumbers[index];
	m_audioQueue.clear();
	m_audioSamplesOut = 0;
	m_haveAudioBasePts = false;
	return true;
}

bool WebmVideoDecoder::demuxNextBlock()
{
	for (;;)
	{
		if (!m_cluster || m_cluster->EOS())
		{
			return false;
		}

		const mkvparser::BlockEntry *entry = nullptr;
		long status = (m_currentEntry == nullptr) ? m_cluster->GetFirst(entry)
		                                          : m_cluster->GetNext(m_currentEntry, entry);
		if (status < 0)
		{
			debug(LOG_ERROR, "Error parsing WebM cluster: %s", m_provider->filename().toUtf8().c_str());
			return false;
		}

		if (!entry || entry->EOS())
		{
			m_cluster = m_segment->GetNext(m_cluster);
			m_currentEntry = nullptr;
			continue;
		}
		m_currentEntry = entry;

		const mkvparser::Block *block = entry->GetBlock();
		if (!block)
		{
			continue;
		}

		const long long trackNumber = block->GetTrackNumber();
		const bool isVideo = (trackNumber == m_videoTrackNumber);
		const bool isSelectedAudio = (trackNumber == m_selectedAudioTrackNumber);
		if (!isVideo && !isSelectedAudio)
		{
			continue;	// unselected/unknown track: skip without copying
		}

		const double pts = static_cast<double>(block->GetTime(m_cluster)) / 1e9;
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
			(isVideo ? m_videoQueue : m_audioQueue).push_back(std::move(packet));
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
		if (!m_videoQueue.empty())
		{
			DemuxedPacket packet = std::move(m_videoQueue.front());
			m_videoQueue.pop_front();
			// a failed decode (corrupt packet) or missing output (invisible
			// alt-ref frame) is skippable - but a stream where *every* packet
			// produces nothing (e.g. an unsupported VP9 profile) must fail
			// loudly and promptly, not play black / grind through the file
			if (!m_videoCodec->decode(packet.data.data(), packet.data.size())
			    || !m_videoCodec->getFrame(out))
			{
				if (++m_consecutiveNoOutput >= 16)
				{
					debug(LOG_ERROR, "Video stream produced no displayable frames (unsupported profile/format?): %s", m_provider->filename().toUtf8().c_str());
					return false;
				}
				continue;
			}
			m_consecutiveNoOutput = 0;
			out.pts = packet.pts;
			return true;
		}

		if (!demuxNextBlock())
		{
			return false;
		}
	}
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

		if (!m_audioQueue.empty())
		{
			DemuxedPacket packet = std::move(m_audioQueue.front());
			m_audioQueue.pop_front();
			if (!m_haveAudioBasePts)
			{
				m_audioBasePts = packet.pts;
				m_haveAudioBasePts = true;
			}
			m_audioDecoder->submitPacket(packet.data.data(), packet.data.size());
			continue;
		}

		if (!demuxNextBlock())
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
	auto decoder = std::make_unique<WebmVideoDecoder>(std::move(provider));
	if (!decoder->open())
	{
		return nullptr;
	}
	return decoder;
}
