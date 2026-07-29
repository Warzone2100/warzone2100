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

#include <cstdint>
#include <cstddef>
#include <array>
#include <memory>
#include <utility>

/** Packet-level audio decoder.
 *
 * Unlike WZDecoder (codecs.h), which owns its input source and decodes an
 * entire file/stream, implementations of this interface are fed individual
 * compressed packets by an external demuxer (ex: the FMV container layer in
 * lib/sequence), and output interleaved int16 PCM in native endianness.
 */
class WZPacketAudioDecoder
{
public:
	virtual ~WZPacketAudioDecoder();

	WZPacketAudioDecoder(const WZPacketAudioDecoder&)            = delete;
	WZPacketAudioDecoder &operator=(const WZPacketAudioDecoder&) = delete;

	virtual unsigned channels() const = 0;
	/** Output sampling rate (may differ from the bitstream's original rate - ex: Opus always outputs 48000) */
	virtual unsigned sampleRate() const = 0;

	/** Feed one compressed packet; drain the decoded output with pcmOut().
	 * \returns false if the packet could not be parsed (safe to skip it and continue) */
	virtual bool submitPacket(const uint8_t *data, size_t len) = 0;

	/** Write up to maxSamplesPerChannel interleaved int16 samples to dest.
	 * \returns the number of samples (per channel) written - 0 means another packet is needed */
	virtual size_t pcmOut(int16_t *dest, size_t maxSamplesPerChannel) = 0;

	/** Reset decoder state, as at the start of the stream */
	virtual void flush() = 0;

protected:
	WZPacketAudioDecoder() = default;
};

/** Create a Vorbis packet decoder from the three Vorbis header packets
 * (identification, comment, setup), as extracted from an Ogg or WebM container.
 * \returns nullptr if the header packets fail to parse */
std::unique_ptr<WZPacketAudioDecoder> wzVorbisPacketDecoderCreate(
	const std::array<std::pair<const uint8_t*, size_t>, 3>& headerPackets);

/** Create an Opus packet decoder from an OpusHead header (RFC 7845), as carried
 * in a WebM audio track's CodecPrivate data. Handles pre-skip, output gain, and
 * multistream channel mappings. Output is always 48kHz.
 * \returns nullptr if the header fails to parse */
std::unique_ptr<WZPacketAudioDecoder> wzOpusPacketDecoderCreate(const uint8_t *opusHead, size_t opusHeadLen);
