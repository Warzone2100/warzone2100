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
#include "codec_packet.h"

#include <opus/opus_multistream.h>

#include <algorithm>
#include <cstring>
#include <vector>

namespace
{

// Maximum Opus frame duration is 120ms; at 48kHz that's 5760 samples per channel
constexpr int MAX_OPUS_FRAME_SAMPLES = 5760;
constexpr unsigned OPUS_OUTPUT_RATE = 48000;

/** The parsed fields of an OpusHead header (RFC 7845 section 5.1), which WebM
 * carries verbatim as the audio track's CodecPrivate data. */
struct OpusHeadInfo
{
	uint8_t channels = 0;
	uint16_t preSkip = 0;
	int16_t outputGainQ8 = 0;
	uint8_t mappingFamily = 0;
	uint8_t streamCount = 0;
	uint8_t coupledCount = 0;
	uint8_t mapping[255] = {0};
};

bool parseOpusHead(const uint8_t *data, size_t len, OpusHeadInfo& out)
{
	if (len < 19 || memcmp(data, "OpusHead", 8) != 0)
	{
		debug(LOG_ERROR, "Invalid OpusHead header (len %zu)", len);
		return false;
	}
	const uint8_t version = data[8];
	if ((version >> 4) != 0)	// upper nibble must be 0 for backwards-compatible versions
	{
		debug(LOG_ERROR, "Unsupported OpusHead version: %u", version);
		return false;
	}
	out.channels = data[9];
	out.preSkip = static_cast<uint16_t>(data[10] | (data[11] << 8));
	// bytes 12-15: input sample rate (informational only)
	out.outputGainQ8 = static_cast<int16_t>(data[16] | (data[17] << 8));
	out.mappingFamily = data[18];

	if (out.channels == 0)
	{
		debug(LOG_ERROR, "OpusHead: zero channels");
		return false;
	}

	if (out.mappingFamily == 0)
	{
		// RFC 7845: family 0 is mono/stereo, with an implicit mapping table
		if (out.channels > 2)
		{
			debug(LOG_ERROR, "OpusHead: mapping family 0 with %u channels", out.channels);
			return false;
		}
		out.streamCount = 1;
		out.coupledCount = static_cast<uint8_t>(out.channels - 1);
		out.mapping[0] = 0;
		out.mapping[1] = 1;
	}
	else
	{
		if (len < 21u + out.channels)
		{
			debug(LOG_ERROR, "OpusHead: truncated channel mapping table");
			return false;
		}
		out.streamCount = data[19];
		out.coupledCount = data[20];
		memcpy(out.mapping, &data[21], out.channels);
	}
	return true;
}

class WZOpusPacketDecoder final : public WZPacketAudioDecoder
{
public:
	~WZOpusPacketDecoder() override
	{
		if (m_decoder)
		{
			opus_multistream_decoder_destroy(m_decoder);
		}
	}

	bool init(const uint8_t *opusHead, size_t opusHeadLen)
	{
		if (!parseOpusHead(opusHead, opusHeadLen, m_head))
		{
			return false;
		}

		int err = OPUS_OK;
		m_decoder = opus_multistream_decoder_create(OPUS_OUTPUT_RATE, m_head.channels,
		                                            m_head.streamCount, m_head.coupledCount,
		                                            m_head.mapping, &err);
		if (err != OPUS_OK || !m_decoder)
		{
			debug(LOG_ERROR, "opus_multistream_decoder_create() failed: %s", opus_strerror(err));
			return false;
		}

		// OpusHead output gain is Q7.8 dB - the exact format OPUS_SET_GAIN expects
		if (m_head.outputGainQ8 != 0)
		{
			opus_multistream_decoder_ctl(m_decoder, OPUS_SET_GAIN(static_cast<opus_int32>(m_head.outputGainQ8)));
		}

		m_skipRemaining = m_head.preSkip;
		m_decodeBuf.resize(static_cast<size_t>(MAX_OPUS_FRAME_SAMPLES) * m_head.channels);
		return true;
	}

	unsigned channels() const override
	{
		return m_head.channels;
	}

	unsigned sampleRate() const override
	{
		return OPUS_OUTPUT_RATE;	// Opus always decodes at 48kHz
	}

	bool submitPacket(const uint8_t *data, size_t len) override
	{
		int samples = opus_multistream_decode(m_decoder, data, static_cast<opus_int32>(len),
		                                      m_decodeBuf.data(), MAX_OPUS_FRAME_SAMPLES, 0);
		if (samples < 0)
		{
			debug(LOG_WARNING, "opus_multistream_decode() failed: %s", opus_strerror(samples));
			return false;
		}

		// apply stream-start pre-skip (RFC 7845 section 4.2)
		int keepFrom = 0;
		if (m_skipRemaining > 0)
		{
			keepFrom = std::min(samples, static_cast<int>(m_skipRemaining));
			m_skipRemaining -= static_cast<uint32_t>(keepFrom);
		}

		m_pending.insert(m_pending.end(),
		                 m_decodeBuf.begin() + static_cast<ptrdiff_t>(keepFrom) * m_head.channels,
		                 m_decodeBuf.begin() + static_cast<ptrdiff_t>(samples) * m_head.channels);
		return true;
	}

	size_t pcmOut(int16_t *dest, size_t maxSamplesPerChannel) override
	{
		const size_t pendingSamples = (m_pending.size() - m_pendingRead) / m_head.channels;
		const size_t samplesToTake = std::min(pendingSamples, maxSamplesPerChannel);
		if (samplesToTake == 0)
		{
			return 0;
		}

		memcpy(dest, m_pending.data() + m_pendingRead, samplesToTake * m_head.channels * sizeof(int16_t));
		m_pendingRead += samplesToTake * m_head.channels;
		if (m_pendingRead >= m_pending.size())
		{
			m_pending.clear();
			m_pendingRead = 0;
		}
		return samplesToTake;
	}

	void flush() override
	{
		if (m_decoder)
		{
			opus_multistream_decoder_ctl(m_decoder, OPUS_RESET_STATE);
		}
		m_pending.clear();
		m_pendingRead = 0;
		m_skipRemaining = m_head.preSkip;
	}

private:
	OpusHeadInfo m_head;
	OpusMSDecoder *m_decoder = nullptr;
	std::vector<opus_int16> m_decodeBuf;	// one decoded packet (interleaved)
	std::vector<int16_t> m_pending;			// decoded-but-not-yet-drained PCM (interleaved)
	size_t m_pendingRead = 0;				// read offset into m_pending, in int16 units
	uint32_t m_skipRemaining = 0;
};

} // anonymous namespace

std::unique_ptr<WZPacketAudioDecoder> wzOpusPacketDecoderCreate(const uint8_t *opusHead, size_t opusHeadLen)
{
	auto decoder = std::make_unique<WZOpusPacketDecoder>();
	if (!decoder->init(opusHead, opusHeadLen))
	{
		return nullptr;
	}
	return decoder;
}
