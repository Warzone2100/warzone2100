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
// Standalone unit test for the VP9 bitstream helpers (no WZ dependencies).

#include "lib/sequence/vp9_bitstream.h"

#include <cstdio>
#include <cstdint>
#include <random>
#include <vector>

static int failures = 0;

#define CHECK(cond, ...) do { \
	if (!(cond)) { std::printf("FAIL %s:%d: ", __FILE__, __LINE__); std::printf(__VA_ARGS__); std::printf("\n"); ++failures; } \
} while (0)

/** Pack frames into a chunk with a valid superframe index. */
static std::vector<uint8_t> buildSuperframe(const std::vector<std::vector<uint8_t>>& frames, size_t magnitude)
{
	std::vector<uint8_t> chunk;
	for (const auto& frame : frames)
	{
		chunk.insert(chunk.end(), frame.begin(), frame.end());
	}
	const uint8_t marker = static_cast<uint8_t>(0xc0u | ((magnitude - 1) << 3) | (frames.size() - 1));
	chunk.push_back(marker);
	for (const auto& frame : frames)
	{
		for (size_t b = 0; b < magnitude; ++b)
		{
			chunk.push_back(static_cast<uint8_t>((frame.size() >> (b * 8)) & 0xff));
		}
	}
	chunk.push_back(marker);
	return chunk;
}

static std::vector<uint8_t> makeFrame(size_t len, uint8_t seed)
{
	std::vector<uint8_t> frame(len);
	frame[0] = 0x82;	// VP9 frame marker (0b10) in the top two bits
	for (size_t i = 1; i < len; ++i)
	{
		frame[i] = static_cast<uint8_t>(seed + i);
	}
	return frame;
}

int main()
{
	std::vector<WZVideoFrameSpan> spans;

	// an ordinary frame passes through as a single span
	{
		auto frame = makeFrame(64, 1);
		wzVp9SplitSuperframe(frame.data(), frame.size(), spans);
		CHECK(spans.size() == 1, "plain frame split into %zu spans", spans.size());
		CHECK(spans[0].data == frame.data() && spans[0].len == frame.size(), "plain frame span mismatch");
	}

	// 1..8 frames, at every index magnitude wide enough to hold them
	for (size_t count = 1; count <= 8; ++count)
	{
		for (size_t magnitude = 1; magnitude <= 4; ++magnitude)
		{
			std::vector<std::vector<uint8_t>> frames;
			for (size_t i = 0; i < count; ++i)
			{
				frames.push_back(makeFrame(17 + i * 13, static_cast<uint8_t>(i)));
			}
			if (magnitude == 1 && frames.back().size() > 255)
			{
				continue;
			}
			auto chunk = buildSuperframe(frames, magnitude);

			wzVp9SplitSuperframe(chunk.data(), chunk.size(), spans);
			CHECK(spans.size() == count, "count=%zu mag=%zu: got %zu spans", count, magnitude, spans.size());
			if (spans.size() != count)
			{
				continue;
			}
			size_t offset = 0;
			for (size_t i = 0; i < count; ++i)
			{
				CHECK(spans[i].len == frames[i].size(), "count=%zu mag=%zu frame %zu: len %zu want %zu",
				      count, magnitude, i, spans[i].len, frames[i].size());
				CHECK(spans[i].data == chunk.data() + offset, "count=%zu mag=%zu frame %zu: wrong offset", count, magnitude, i);
				offset += frames[i].size();
			}
		}
	}

	// a chunk whose sizes do not fit the payload is handed over whole
	{
		std::vector<std::vector<uint8_t>> frames = { makeFrame(32, 1), makeFrame(32, 2) };
		auto chunk = buildSuperframe(frames, 1);
		chunk[chunk.size() - 3] = 0xff;		// first frame size now overruns
		wzVp9SplitSuperframe(chunk.data(), chunk.size(), spans);
		CHECK(spans.size() == 1 && spans[0].len == chunk.size(), "malformed index was not rejected");
	}

	// a marker byte that is not mirrored at the start of the index is not an index
	{
		std::vector<std::vector<uint8_t>> frames = { makeFrame(32, 1), makeFrame(32, 2) };
		auto chunk = buildSuperframe(frames, 1);
		chunk[chunk.size() - 4] ^= 0xff;	// break the leading marker
		wzVp9SplitSuperframe(chunk.data(), chunk.size(), spans);
		CHECK(spans.size() == 1 && spans[0].len == chunk.size(), "unmirrored marker was accepted");
	}

	// degenerate input
	{
		wzVp9SplitSuperframe(nullptr, 0, spans);
		CHECK(spans.empty(), "null input produced spans");
		const uint8_t forged = 0xc0;
		wzVp9SplitSuperframe(&forged, 1, spans);
		CHECK(spans.size() == 1 && spans[0].len == 1, "one-byte forged marker mishandled");
	}

	// every span must stay inside the buffer, whatever the input
	{
		std::vector<std::vector<uint8_t>> frames = { makeFrame(48, 1), makeFrame(64, 2), makeFrame(80, 3) };
		const auto base = buildSuperframe(frames, 2);
		std::mt19937 rng(1234);
		for (int trial = 0; trial < 20000; ++trial)
		{
			std::vector<uint8_t> chunk = base;
			switch (trial % 4)
			{
			case 0: chunk.resize(1 + (rng() % chunk.size())); break;
			case 1: chunk[rng() % chunk.size()] ^= static_cast<uint8_t>(1u << (rng() % 8)); break;
			case 2: chunk.assign(1 + rng() % 64, static_cast<uint8_t>(rng())); break;
			case 3: chunk.back() = static_cast<uint8_t>(0xc0u | (rng() & 0x3fu)); break;
			}
			wzVp9SplitSuperframe(chunk.data(), chunk.size(), spans);
			CHECK(!spans.empty(), "empty result for non-empty input");
			size_t total = 0;
			for (const auto& span : spans)
			{
				CHECK(span.data >= chunk.data() && span.data + span.len <= chunk.data() + chunk.size(),
				      "span escapes the buffer (trial %d)", trial);
				total += span.len;
			}
			CHECK(total <= chunk.size(), "spans cover more than the input (trial %d)", trial);
		}
	}

	// key frame detection from the uncompressed header
	{
		// profile 0: [10][profile_low=0][profile_high=0][show_existing][frame_type][show_frame][err]
		const uint8_t keyFrame[] = { 0x82, 0x49, 0x83 };		// show_existing=0, frame_type=0
		const uint8_t interFrame[] = { 0x86, 0x00, 0x00 };		// show_existing=0, frame_type=1
		const uint8_t showExisting[] = { 0x88, 0x00 };			// show_existing=1
		const uint8_t notVp9[] = { 0x00, 0x11, 0x22 };
		CHECK(wzVp9FrameIsKeyframe(keyFrame, sizeof(keyFrame)), "key frame not detected");
		CHECK(!wzVp9FrameIsKeyframe(interFrame, sizeof(interFrame)), "inter frame reported as key frame");
		CHECK(!wzVp9FrameIsKeyframe(showExisting, sizeof(showExisting)), "show_existing_frame reported as key frame");
		CHECK(!wzVp9FrameIsKeyframe(notVp9, sizeof(notVp9)), "non-VP9 data reported as key frame");
		CHECK(!wzVp9FrameIsKeyframe(nullptr, 0), "null input reported as key frame");
		CHECK(!wzVp9FrameIsKeyframe(keyFrame, 0), "zero length reported as key frame");
	}

	// color description from key frame and intra-only headers
	{
		// profile 0 key frame: [10][0][0][show_existing=0][key=0][show=1][err=0] + sync code + color_space(3) color_range(1)
		const uint8_t bt709Studio[] = { 0x82, 0x49, 0x83, 0x42, 0x40 };	// color_space 2, range 0
		const uint8_t bt601Full[] = { 0x82, 0x49, 0x83, 0x42, 0x30 };		// color_space 1, range 1
		const uint8_t srgb[] = { 0x82, 0x49, 0x83, 0x42, 0xe0 };			// color_space 7: always full range
		const uint8_t badSync[] = { 0x82, 0x49, 0x83, 0x43, 0x40 };
		const uint8_t truncated[] = { 0x82, 0x49, 0x83 };
		const uint8_t interFrame[] = { 0x86, 0x00, 0x00, 0x00, 0x00 };
		// profile 2 key frame: [10][0][1][0][0][1][0] + sync + ten_or_twelve_bit(1) color_space(3) color_range(1)
		const uint8_t profile2Bt709Full[] = { 0x92, 0x49, 0x83, 0x42, 0x28 };	// 0|010|1
		// profile 3 key frame: [10][1][1][reserved 0][0][0][1] [err 0] + sync + ...
		const uint8_t profile3Bt601Studio[] = { 0xb1, 0x24, 0xc1, 0xa1, 0x0b, 0x00 };	// shifted a bit by the reserved bit
		// profile 0 intra-only: show_frame=0, intra_only=1, reset_frame_context(2) + sync: implies BT.601 studio
		const uint8_t intraOnlyP0[] = { 0x84, 0x89, 0x30, 0x68, 0x40 };

		WZVp9ColorConfig c;
		CHECK(wzVp9ReadColorConfig(bt709Studio, sizeof(bt709Studio), c) && c.colorSpace == 2 && !c.fullRange, "BT.709 studio");
		CHECK(wzVp9ReadColorConfig(bt601Full, sizeof(bt601Full), c) && c.colorSpace == 1 && c.fullRange, "BT.601 full");
		CHECK(wzVp9ReadColorConfig(srgb, sizeof(srgb), c) && c.colorSpace == 7 && c.fullRange, "sRGB");
		CHECK(wzVp9ReadColorConfig(profile2Bt709Full, sizeof(profile2Bt709Full), c) && c.colorSpace == 2 && c.fullRange, "profile 2 BT.709 full");
		CHECK(wzVp9ReadColorConfig(profile3Bt601Studio, sizeof(profile3Bt601Studio), c) && c.colorSpace == 1 && !c.fullRange, "profile 3 BT.601 studio");
		CHECK(wzVp9ReadColorConfig(intraOnlyP0, sizeof(intraOnlyP0), c) && c.colorSpace == 1 && !c.fullRange, "profile 0 intra-only");
		CHECK(!wzVp9ReadColorConfig(badSync, sizeof(badSync), c), "bad sync code accepted");
		CHECK(!wzVp9ReadColorConfig(truncated, sizeof(truncated), c), "truncated header accepted");
		CHECK(!wzVp9ReadColorConfig(interFrame, sizeof(interFrame), c), "inter frame reported a color description");
		CHECK(!wzVp9ReadColorConfig(nullptr, 0, c), "null input");
	}

	std::printf("vp9_bitstream_test: %s\n", failures ? "FAILED" : "passed");
	return failures != 0;
}
