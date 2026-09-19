// SPDX-License-Identifier: GPL-2.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2008-2026  Warzone 2100 Project (https://github.com/Warzone2100)

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

/* The Ogg demuxing and Theora header handling in this file is derived from the
 * SDL player example as found in the OggTheora software codec source code. In
 * particular this is examples/player_example.c as found in OggTheora 1.0beta3.
 *
 * The copyright to this file was originally owned by and licensed as follows.
 * Please note, however, that *this* file, i.e. the one you are currently
 * reading is not licensed as such anymore.
 *
 * Copyright (C) 2002-2007 Xiph.org Foundation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 *
 * - Neither the name of the Xiph.org Foundation nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "lib/framework/frame.h"
#include "ogg_decoder.h"
#include "lib/sound/codec_packet.h"

#include <theora/theora.h>
#include <ogg/ogg.h>

#include <array>
#include <cstring>

namespace
{

bool isVorbisIdPacket(const ogg_packet& op)
{
	return op.bytes >= 7 && op.packet[0] == 0x01 && memcmp(&op.packet[1], "vorbis", 6) == 0;
}

class OggTheoraDecoder final : public WZVideoDecoder
{
public:
	OggTheoraDecoder(std::shared_ptr<VideoProvider> provider)
	: m_provider(std::move(provider))
	{
		ogg_sync_init(&m_oggSync);
		theora_comment_init(&m_theoraComment);
		theora_info_init(&m_theoraInfo);
	}

	~OggTheoraDecoder() override
	{
		if (m_hasVorbis)
		{
			ogg_stream_clear(&m_vorbisStream);
		}
		if (m_hasTheora)
		{
			ogg_stream_clear(&m_theoraStream);
		}
		if (m_theoraStateInitialized)
		{
			theora_clear(&m_theoraState);
		}
		theora_comment_clear(&m_theoraComment);
		theora_info_clear(&m_theoraInfo);
		ogg_sync_clear(&m_oggSync);
	}

	bool open();	// parse headers, initialize decoders

	bool hasVideo() const override
	{
		return m_theoraStateInitialized;
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
		return 0;
	}

	bool selectAudioTrack(size_t index) override
	{
		// ogg FMVs only ever have the single (already-selected) vorbis track
		return index == 0 && m_audioDecoder != nullptr;
	}

	bool nextVideoFrame(WZVideoFrameYUV& out) override;
	size_t decodeAudio(int16_t *dest, size_t maxSamplesPerChannel, double& firstSamplePts) override;

private:
	/** Read a chunk of input into the ogg sync layer. \returns bytes read (0 at end-of-data) */
	int bufferData()
	{
		const int size = 262144;	// read in 256K chunks
		char *buffer = ogg_sync_buffer(&m_oggSync, size);
		int64_t bytes = m_provider->read(buffer, size);
		if (bytes < 0)
		{
			if (!m_readErrorLogged)
			{
				debug(LOG_ERROR, "Error reading video data: %s", m_provider->filename().toUtf8().c_str());
				m_readErrorLogged = true;
			}
			bytes = 0;
		}
		ogg_sync_wrote(&m_oggSync, static_cast<long>(bytes));
		return static_cast<int>(bytes);
	}

	/** Push a demuxed page into the stream it belongs to */
	void queuePage(ogg_page *page)
	{
		int serialno = ogg_page_serialno(page);
		if (m_hasTheora && serialno == m_theoraSerial)
		{
			ogg_stream_pagein(&m_theoraStream, page);
		}
		else if (m_hasVorbis && serialno == m_vorbisSerial)
		{
			ogg_stream_pagein(&m_vorbisStream, page);
		}
	}

	/** Demux more input into the per-stream packet queues.
	 * \returns false when the input is exhausted and no page was queued */
	bool pumpInput()
	{
		bool queuedAny = false;
		int bytes = bufferData();
		ogg_page og;
		while (ogg_sync_pageout(&m_oggSync, &og) > 0)
		{
			queuePage(&og);
			queuedAny = true;
		}
		return queuedAny || bytes > 0;
	}

private:
	std::shared_ptr<VideoProvider> m_provider;

	ogg_sync_state m_oggSync;
	ogg_stream_state m_theoraStream;
	ogg_stream_state m_vorbisStream;
	int m_theoraSerial = 0;
	int m_vorbisSerial = 0;
	bool m_hasTheora = false;
	bool m_hasVorbis = false;

	theora_info m_theoraInfo;
	theora_comment m_theoraComment;
	theora_state m_theoraState;
	bool m_theoraStateInitialized = false;

	std::array<std::vector<uint8_t>, 3> m_vorbisHeaderData;
	std::unique_ptr<WZPacketAudioDecoder> m_audioDecoder;
	uint64_t m_audioSamplesOut = 0;

	WZVideoTrackMetadata m_videoMetadata;
	std::vector<WZAudioTrackMetadata> m_audioTracks;

	bool m_readErrorLogged = false;
};

bool OggTheoraDecoder::open()
{
	ogg_packet op;
	ogg_page og;
	int theoraHeaderCount = 0;
	int vorbisHeaderCount = 0;

	/* Parse the header pages: all BOS pages come first. Only interested in
	   the first Theora and the first Vorbis stream. */
	bool bosDone = false;
	while (!bosDone)
	{
		if (bufferData() == 0)
		{
			break;
		}

		while (ogg_sync_pageout(&m_oggSync, &og) > 0)
		{
			if (!ogg_page_bos(&og))
			{
				/* first non-BOS page: don't leak it; get it into the appropriate stream */
				queuePage(&og);
				bosDone = true;
				break;
			}

			ogg_stream_state test;
			ogg_stream_init(&test, ogg_page_serialno(&og));
			ogg_stream_pagein(&test, &og);
			ogg_stream_packetout(&test, &op);

			/* identify the codec */
			if (!m_hasTheora && theora_decode_header(&m_theoraInfo, &m_theoraComment, &op) >= 0)
			{
				memcpy(&m_theoraStream, &test, sizeof(test));
				m_theoraSerial = ogg_page_serialno(&og);
				m_hasTheora = true;
				theoraHeaderCount = 1;
			}
			else if (!m_hasVorbis && isVorbisIdPacket(op))
			{
				memcpy(&m_vorbisStream, &test, sizeof(test));
				m_vorbisSerial = ogg_page_serialno(&og);
				m_hasVorbis = true;
				m_vorbisHeaderData[0].assign(op.packet, op.packet + op.bytes);
				vorbisHeaderCount = 1;
			}
			else
			{
				/* whatever it is, we don't care about it */
				ogg_stream_clear(&test);
			}
		}
	}

	if (!m_hasTheora && !m_hasVorbis)
	{
		debug(LOG_ERROR, "No Theora or Vorbis streams found: %s", m_provider->filename().toUtf8().c_str());
		return false;
	}

	/* we're expecting more header packets */
	while ((m_hasTheora && theoraHeaderCount < 3) || (m_hasVorbis && vorbisHeaderCount < 3))
	{
		int ret;

		while (m_hasTheora && theoraHeaderCount < 3 && (ret = ogg_stream_packetout(&m_theoraStream, &op)))
		{
			if (ret < 0 || theora_decode_header(&m_theoraInfo, &m_theoraComment, &op))
			{
				debug(LOG_ERROR, "Error parsing Theora stream headers; corrupt stream? %s", m_provider->filename().toUtf8().c_str());
				return false;
			}
			theoraHeaderCount++;
		}

		while (m_hasVorbis && vorbisHeaderCount < 3 && (ret = ogg_stream_packetout(&m_vorbisStream, &op)))
		{
			if (ret < 0)
			{
				debug(LOG_ERROR, "Error parsing Vorbis stream headers; corrupt stream? %s", m_provider->filename().toUtf8().c_str());
				return false;
			}
			m_vorbisHeaderData[vorbisHeaderCount].assign(op.packet, op.packet + op.bytes);
			vorbisHeaderCount++;
		}

		/* The header pages/packets will arrive before anything else we
		   care about, or the stream is not obeying spec */
		if (ogg_sync_pageout(&m_oggSync, &og) > 0)
		{
			queuePage(&og);	/* demux into the appropriate stream */
		}
		else if (bufferData() == 0)
		{
			debug(LOG_ERROR, "End of file while searching for codec headers: %s", m_provider->filename().toUtf8().c_str());
			return false;
		}
	}

	/* and now we have it all - initialize decoders */
	if (m_hasTheora)
	{
		theora_decode_init(&m_theoraState, &m_theoraInfo);
		m_theoraStateInitialized = true;
		debug(LOG_VIDEO, "Ogg logical stream %x is Theora %dx%d %.02f fps video",
		      (unsigned int) m_theoraSerial, (int) m_theoraInfo.width, (int) m_theoraInfo.height,
		      (double) m_theoraInfo.fps_numerator / m_theoraInfo.fps_denominator);
		if (m_theoraInfo.width != m_theoraInfo.frame_width || m_theoraInfo.height != m_theoraInfo.frame_height)
		{
			debug(LOG_VIDEO, "  Frame content is %dx%d with offset (%d,%d)", m_theoraInfo.frame_width,
			      m_theoraInfo.frame_height, m_theoraInfo.offset_x, m_theoraInfo.offset_y);
		}

		if (m_theoraInfo.pixelformat != OC_PF_420)
		{
			debug(LOG_ERROR, "Video not in YUV420 format: %s", m_provider->filename().toUtf8().c_str());
			return false;
		}

		int pp_level_max = 0;
		theora_control(&m_theoraState, TH_DECCTL_GET_PPLEVEL_MAX, &pp_level_max, sizeof(pp_level_max));
		int pp_level = pp_level_max;
		theora_control(&m_theoraState, TH_DECCTL_SET_PPLEVEL, &pp_level, sizeof(pp_level));

		m_videoMetadata.width = m_theoraInfo.frame_width;
		m_videoMetadata.height = m_theoraInfo.frame_height;
		m_videoMetadata.fps = (double) m_theoraInfo.fps_numerator / m_theoraInfo.fps_denominator;
	}

	if (m_hasVorbis)
	{
		std::array<std::pair<const uint8_t*, size_t>, 3> headerPackets;
		for (size_t i = 0; i < 3; ++i)
		{
			headerPackets[i] = { m_vorbisHeaderData[i].data(), m_vorbisHeaderData[i].size() };
		}
		m_audioDecoder = wzVorbisPacketDecoderCreate(headerPackets);
		if (!m_audioDecoder)
		{
			debug(LOG_ERROR, "Error parsing Vorbis stream headers; corrupt stream? %s", m_provider->filename().toUtf8().c_str());
			return false;
		}
		debug(LOG_VIDEO, "Ogg logical stream %x is Vorbis %u channel %u Hz audio",
		      (unsigned int) m_vorbisSerial, m_audioDecoder->channels(), m_audioDecoder->sampleRate());

		WZAudioTrackMetadata track;
		track.index = 0;
		track.languageCode = "und";
		track.isDefault = true;
		track.channels = m_audioDecoder->channels();
		track.sampleRate = m_audioDecoder->sampleRate();
		m_audioTracks.push_back(std::move(track));
	}

	return true;
}

bool OggTheoraDecoder::nextVideoFrame(WZVideoFrameYUV& out)
{
	if (!m_theoraStateInitialized)
	{
		return false;
	}

	ogg_packet op;
	for (;;)
	{
		if (ogg_stream_packetout(&m_theoraStream, &op) > 0)
		{
			/* theora is one in, one out... */
			theora_decode_packetin(&m_theoraState, &op);

			yuv_buffer yuv;
			if (theora_decode_YUVout(&m_theoraState, &yuv) != 0)
			{
				continue;
			}

			// hand out the visible region (offsets are 0 in practice for WZ videos)
			out.y = yuv.y + m_theoraInfo.offset_y * yuv.y_stride + m_theoraInfo.offset_x;
			out.u = yuv.u + (m_theoraInfo.offset_y / 2) * yuv.uv_stride + (m_theoraInfo.offset_x / 2);
			out.v = yuv.v + (m_theoraInfo.offset_y / 2) * yuv.uv_stride + (m_theoraInfo.offset_x / 2);
			out.yStride = yuv.y_stride;
			out.uvStride = yuv.uv_stride;
			out.width = m_videoMetadata.width;
			out.height = m_videoMetadata.height;
			out.pts = theora_granule_time(&m_theoraState, m_theoraState.granulepos);
			return true;
		}

		if (!pumpInput())
		{
			return false;
		}
	}
}

size_t OggTheoraDecoder::decodeAudio(int16_t *dest, size_t maxSamplesPerChannel, double& firstSamplePts)
{
	if (!m_audioDecoder)
	{
		return 0;
	}

	for (;;)
	{
		size_t samples = m_audioDecoder->pcmOut(dest, maxSamplesPerChannel);
		if (samples > 0)
		{
			firstSamplePts = static_cast<double>(m_audioSamplesOut) / m_audioDecoder->sampleRate();
			m_audioSamplesOut += samples;
			return samples;
		}

		ogg_packet op;
		if (ogg_stream_packetout(&m_vorbisStream, &op) > 0)
		{
			m_audioDecoder->submitPacket(op.packet, static_cast<size_t>(op.bytes));
			continue;
		}

		if (!pumpInput())
		{
			return 0;
		}
	}
}

} // anonymous namespace

std::unique_ptr<WZVideoDecoder> oggTheoraDecoderOpen(std::shared_ptr<VideoProvider> provider)
{
	if (!provider)
	{
		return nullptr;
	}
	auto decoder = std::make_unique<OggTheoraDecoder>(std::move(provider));
	if (!decoder->open())
	{
		return nullptr;
	}
	return decoder;
}
