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
#include "video_decoder.h"
#include "ogg_decoder.h"
#if defined(WZ_ENABLE_WEBM)
# include "webm_decoder.h"
#endif

#include <cstring>

WZVideoDecoder::~WZVideoDecoder()
{ }

bool videoDecoderWebmSupported()
{
#if defined(WZ_ENABLE_WEBM)
	return true;
#else
	return false;
#endif
}

std::unique_ptr<WZVideoDecoder> videoDecoderOpen(std::shared_ptr<VideoProvider> provider)
{
	if (!provider)
	{
		return nullptr;
	}

	// sniff the container format from the first bytes (don't trust file extensions)
	uint8_t magic[4] = {0};
	if (!provider->seek(0) || provider->read(magic, sizeof(magic)) != sizeof(magic) || !provider->seek(0))
	{
		debug(LOG_ERROR, "Unable to read video file header: %s", provider->filename().toUtf8().c_str());
		return nullptr;
	}

	if (memcmp(magic, "OggS", 4) == 0)
	{
		return oggTheoraDecoderOpen(std::move(provider));
	}

	static const uint8_t ebmlMagic[4] = { 0x1A, 0x45, 0xDF, 0xA3 };
	if (memcmp(magic, ebmlMagic, 4) == 0)
	{
		// EBML magic: a WebM (Matroska) container
#if defined(WZ_ENABLE_WEBM)
		return webmVideoDecoderOpen(std::move(provider));
#else
		debug(LOG_ERROR, "This build does not include WebM video support: %s", provider->filename().toUtf8().c_str());
		return nullptr;
#endif
	}

	debug(LOG_ERROR, "Unrecognized video container format: %s", provider->filename().toUtf8().c_str());
	return nullptr;
}
