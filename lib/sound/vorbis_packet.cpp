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

#include <vorbis/codec.h>

#include <algorithm>
#include <cmath>

WZPacketAudioDecoder::~WZPacketAudioDecoder()
{ }

namespace
{

class WZVorbisPacketDecoder final : public WZPacketAudioDecoder
{
public:
	WZVorbisPacketDecoder()
	{
		vorbis_info_init(&m_info);
		vorbis_comment_init(&m_comment);
	}

	~WZVorbisPacketDecoder() override
	{
		if (m_dspInitialized)
		{
			vorbis_block_clear(&m_block);
			vorbis_dsp_clear(&m_dsp);
		}
		vorbis_comment_clear(&m_comment);
		vorbis_info_clear(&m_info);
	}

	bool init(const std::array<std::pair<const uint8_t*, size_t>, 3>& headerPackets)
	{
		for (size_t i = 0; i < headerPackets.size(); ++i)
		{
			ogg_packet op = makePacket(headerPackets[i].first, headerPackets[i].second);
			op.b_o_s = (i == 0);
			op.packetno = static_cast<ogg_int64_t>(i);
			if (vorbis_synthesis_headerin(&m_info, &m_comment, &op) != 0)
			{
				debug(LOG_ERROR, "Error parsing Vorbis header packet %zu; corrupt stream?", i);
				return false;
			}
		}

		if (vorbis_synthesis_init(&m_dsp, &m_info) != 0)
		{
			debug(LOG_ERROR, "vorbis_synthesis_init() failed");
			return false;
		}
		vorbis_block_init(&m_dsp, &m_block);
		m_dspInitialized = true;
		return true;
	}

	unsigned channels() const override
	{
		return static_cast<unsigned>(m_info.channels);
	}

	unsigned sampleRate() const override
	{
		return static_cast<unsigned>(m_info.rate);
	}

	bool submitPacket(const uint8_t *data, size_t len) override
	{
		ogg_packet op = makePacket(data, len);
		op.packetno = m_packetno++;
		if (vorbis_synthesis(&m_block, &op) != 0)
		{
			return false;
		}
		vorbis_synthesis_blockin(&m_dsp, &m_block);
		return true;
	}

	size_t pcmOut(int16_t *dest, size_t maxSamplesPerChannel) override
	{
		float **pcm = nullptr;
		int samplesAvailable = vorbis_synthesis_pcmout(&m_dsp, &pcm);
		if (samplesAvailable <= 0)
		{
			return 0;
		}

		const int nChannels = m_info.channels;
		const int samplesToTake = static_cast<int>(std::min<size_t>(static_cast<size_t>(samplesAvailable), maxSamplesPerChannel));
		size_t idx = 0;
		for (int i = 0; i < samplesToTake; ++i)
		{
			for (int j = 0; j < nChannels; ++j)
			{
				int val = static_cast<int>(nearbyint(pcm[j][i] * 32767.f));
				if (val > 32767)
				{
					val = 32767;
				}
				else if (val < -32768)
				{
					val = -32768;
				}
				dest[idx++] = static_cast<int16_t>(val);
			}
		}

		vorbis_synthesis_read(&m_dsp, samplesToTake);
		return static_cast<size_t>(samplesToTake);
	}

	void flush() override
	{
		if (m_dspInitialized)
		{
			vorbis_synthesis_restart(&m_dsp);
		}
	}

private:
	static ogg_packet makePacket(const uint8_t *data, size_t len)
	{
		ogg_packet op;
		memset(&op, 0, sizeof(op));
		op.packet = const_cast<unsigned char *>(data);
		op.bytes = static_cast<long>(len);
		op.granulepos = -1;
		return op;
	}

private:
	vorbis_info m_info;
	vorbis_comment m_comment;
	vorbis_dsp_state m_dsp;
	vorbis_block m_block;
	bool m_dspInitialized = false;
	ogg_int64_t m_packetno = 3;	// data packets follow the 3 header packets
};

} // anonymous namespace

std::unique_ptr<WZPacketAudioDecoder> wzVorbisPacketDecoderCreate(
	const std::array<std::pair<const uint8_t*, size_t>, 3>& headerPackets)
{
	auto decoder = std::make_unique<WZVorbisPacketDecoder>();
	if (!decoder->init(headerPackets))
	{
		return nullptr;
	}
	return decoder;
}
