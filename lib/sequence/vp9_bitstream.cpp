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

#include "vp9_bitstream.h"

// VP9 spec Annex B (superframes): the last byte of the chunk repeats as the first byte
// of a trailing index holding each frame's size.
void wzVp9SplitSuperframe(const uint8_t *data, size_t len, std::vector<WZVideoFrameSpan>& out)
{
	out.clear();
	if (!data || len == 0)
	{
		return;
	}

	const uint8_t marker = data[len - 1];
	if ((marker & 0xe0u) == 0xc0u)
	{
		const size_t frames = (marker & 0x7u) + 1;
		const size_t magnitude = ((marker >> 3) & 0x3u) + 1;
		const size_t indexSize = 2 + magnitude * frames;

		if (len >= indexSize && data[len - indexSize] == marker)
		{
			const size_t payload = len - indexSize;
			const uint8_t *entry = data + payload + 1;
			size_t offset = 0;
			bool valid = true;

			for (size_t i = 0; i < frames; ++i)
			{
				size_t frameSize = 0;
				for (size_t b = 0; b < magnitude; ++b)
				{
					frameSize |= static_cast<size_t>(*entry++) << (b * 8);
				}
				if (frameSize == 0 || frameSize > payload - offset)
				{
					valid = false;
					break;
				}
				out.push_back({ data + offset, frameSize });
				offset += frameSize;
			}

			if (valid)
			{
				return;
			}
			out.clear();
		}
	}

	out.push_back({ data, len });
}

namespace
{

class BitReader
{
public:
	BitReader(const uint8_t *data, size_t len) : m_data(data), m_bits(len * 8) { }

	bool read(unsigned count, unsigned& out)
	{
		out = 0;
		for (unsigned i = 0; i < count; ++i)
		{
			if (m_pos >= m_bits)
			{
				return false;
			}
			out = (out << 1) | ((m_data[m_pos >> 3] >> (7 - (m_pos & 7))) & 1u);
			++m_pos;
		}
		return true;
	}

private:
	const uint8_t *m_data;
	size_t m_bits;
	size_t m_pos = 0;
};

struct HeaderStart
{
	unsigned profile = 0;
	bool showExistingFrame = false;
	bool keyframe = false;
	bool showFrame = false;
	bool errorResilient = false;
};

// VP9 spec 6.2 "uncompressed header", up to error_resilient_mode
bool readHeaderStart(BitReader& bits, HeaderStart& h)
{
	unsigned value = 0, low = 0, high = 0;
	if (!bits.read(2, value) || value != 2 || !bits.read(1, low) || !bits.read(1, high))
	{
		return false;	// not a VP9 frame
	}
	h.profile = (high << 1) | low;
	if (h.profile == 3 && !bits.read(1, value))
	{
		return false;
	}
	if (!bits.read(1, value))
	{
		return false;
	}
	h.showExistingFrame = (value != 0);
	if (h.showExistingFrame)
	{
		return true;	// a display command: nothing else follows
	}
	unsigned frameType = 0, showFrame = 0, errorResilient = 0;
	if (!bits.read(1, frameType) || !bits.read(1, showFrame) || !bits.read(1, errorResilient))
	{
		return false;
	}
	h.keyframe = (frameType == 0);
	h.showFrame = (showFrame != 0);
	h.errorResilient = (errorResilient != 0);
	return true;
}

bool readSyncCode(BitReader& bits)
{
	unsigned a = 0, b = 0, c = 0;
	return bits.read(8, a) && bits.read(8, b) && bits.read(8, c) && a == 0x49 && b == 0x83 && c == 0x42;
}

} // anonymous namespace

bool wzVp9FrameIsKeyframe(const uint8_t *data, size_t len)
{
	if (!data || len == 0)
	{
		return false;
	}
	BitReader bits(data, len);
	HeaderStart h;
	return readHeaderStart(bits, h) && !h.showExistingFrame && h.keyframe;
}

bool wzVp9ReadColorConfig(const uint8_t *data, size_t len, WZVp9ColorConfig& out)
{
	if (!data || len == 0)
	{
		return false;
	}
	BitReader bits(data, len);
	HeaderStart h;
	if (!readHeaderStart(bits, h) || h.showExistingFrame)
	{
		return false;
	}

	unsigned value = 0;
	if (!h.keyframe)
	{
		unsigned intraOnly = 0;
		if (!h.showFrame && !bits.read(1, intraOnly))
		{
			return false;
		}
		if (!intraOnly)
		{
			return false;	// inter frames carry no color description
		}
		if ((!h.errorResilient && !bits.read(2, value)) || !readSyncCode(bits))	// reset_frame_context
		{
			return false;
		}
		if (h.profile == 0)
		{
			// profile 0 intra-only frames imply BT.601 studio range (as libvpx does)
			out.colorSpace = 1;
			out.fullRange = false;
			return true;
		}
	}
	else if (!readSyncCode(bits))
	{
		return false;
	}

	// color_config()
	if (h.profile >= 2 && !bits.read(1, value))	// ten_or_twelve_bit
	{
		return false;
	}
	unsigned colorSpace = 0, colorRange = 1;
	if (!bits.read(3, colorSpace))
	{
		return false;
	}
	if (colorSpace != 7 && !bits.read(1, colorRange))	// sRGB is always full range
	{
		return false;
	}
	out.colorSpace = colorSpace;
	out.fullRange = (colorRange != 0);
	return true;
}
