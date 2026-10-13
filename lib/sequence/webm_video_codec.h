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

#include "video_decoder.h"
#include "lib/framework/wzstring.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

enum class WZWebmCodecBackend
{
	Auto,
	Libvpx,
	MediaFoundation,
};

/** The codec-specific part of WebM video decoding */
class WZWebmVideoCodec
{
public:
	virtual ~WZWebmVideoCodec();

	virtual const char* backendName() const = 0;

	/** Feed one compressed frame, which may not produce output yet.
	 * \returns false on a decode error, not on "needs more input". */
	virtual bool decode(const uint8_t *data, size_t len, double pts) = 0;

	/** Get the next (borrowed) displayable frame, if the codec has one ready.
	 * Planes are valid until the next decode()/getFrame()/drain()/flush(). */
	virtual bool getFrame(WZVideoFrameYUV& out) = 0;

	/** Whether getFrame() sets out.pts itself (required of a backend that buffers frames) */
	virtual bool providesFramePts() const { return false; }

	/** No more input is coming: release anything still buffered to getFrame(). */
	virtual void drain() { }

	/** The input is about to jump (a seek): discard everything buffered.
	 * Decoding resumes from a key frame. */
	virtual void flush() { }

	/** Whether the codec has stopped decoding for good */
	virtual bool failed() const { return false; }

	/** Color description from the container - used where the bitstream lacks one */
	WZVideoColorMatrix containerColorMatrix = WZVideoColorMatrix::BT601;
	bool containerFullRange = false;
};

/** Create a codec for a Matroska CodecID ("V_VP8" / "V_VP9"), falling back through the registry on failure */
std::unique_ptr<WZWebmVideoCodec> wzWebmVideoCodecCreate(const char *codecId, unsigned width, unsigned height, WZWebmCodecBackend preference);

/** Create a codec from the first backend in the registry other than the one \p failed came from. */
std::unique_ptr<WZWebmVideoCodec> wzWebmVideoCodecCreateReplacement(const char *codecId, unsigned width, unsigned height, const WZWebmVideoCodec& failed);

std::vector<const char*> wzWebmCodecBackendNames();
bool wzWebmCodecBackendFromName(const WzString& name, WZWebmCodecBackend& out);

void wzWebmSetPreferredCodecBackend(WZWebmCodecBackend backend);
WZWebmCodecBackend wzWebmPreferredCodecBackend();

std::unique_ptr<WZWebmVideoCodec> wzCreateVpxVideoCodec(const char *codecId, unsigned width, unsigned height);
bool wzVpxVideoCodecAvailable(WzString& detail);

#if defined(WZ_ENABLE_VIDEO_BACKEND_MF)
std::unique_ptr<WZWebmVideoCodec> wzCreateMediaFoundationVideoCodec(const char *codecId, unsigned width, unsigned height);
bool wzMediaFoundationVideoCodecAvailable(WzString& detail);
#endif

/** Worker thread count for a backend's decoder: the hardware thread count, clamped to [1, 4] */
unsigned wzWebmVideoCodecDecodeThreads();
