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
#include "webm_video_codec.h"

#include <algorithm>
#include <cstring>
#include <thread>

WZWebmVideoCodec::~WZWebmVideoCodec()
{ }

unsigned wzWebmVideoCodecDecodeThreads()
{
	// hardware_concurrency() returns 0 when it can't tell
	return std::clamp(std::thread::hardware_concurrency(), 1u, 4u);
}

namespace
{

struct BackendEntry
{
	WZWebmCodecBackend id;
	const char *name;
	bool (*available)(WzString& detail);
	std::unique_ptr<WZWebmVideoCodec> (*create)(const char *codecId, unsigned width, unsigned height);
};

// Auto's preference order, and the fallback order once the requested backend has been tried.
const BackendEntry backendTable[] = {
#if defined(WZ_ENABLE_VIDEO_BACKEND_MF)
	{ WZWebmCodecBackend::MediaFoundation, "mediafoundation", wzMediaFoundationVideoCodecAvailable, wzCreateMediaFoundationVideoCodec },
#endif
	{ WZWebmCodecBackend::Libvpx, "libvpx", wzVpxVideoCodecAvailable, wzCreateVpxVideoCodec },
};

WZWebmCodecBackend preferredBackend = WZWebmCodecBackend::Auto;

} // anonymous namespace

void wzWebmSetPreferredCodecBackend(WZWebmCodecBackend backend)
{
	preferredBackend = backend;
}

WZWebmCodecBackend wzWebmPreferredCodecBackend()
{
	return preferredBackend;
}

bool wzWebmCodecBackendFromName(const WzString& name, WZWebmCodecBackend& out)
{
	if (name.compare("auto") == 0)
	{
		out = WZWebmCodecBackend::Auto;
		return true;
	}
	for (const auto& entry : backendTable)
	{
		if (name.compare(entry.name) == 0)
		{
			out = entry.id;
			return true;
		}
	}
	return false;
}

std::vector<const char*> wzWebmCodecBackendNames()
{
	std::vector<const char*> result;
	for (const auto& entry : backendTable)
	{
		result.push_back(entry.name);
	}
	return result;
}

static std::unique_ptr<WZWebmVideoCodec> tryCreate(const BackendEntry& entry, const char *codecId, unsigned width, unsigned height)
{
	WzString detail;
	if (!entry.available(detail))
	{
		debug(LOG_VIDEO, "Video decode backend '%s' is unavailable (%s)", entry.name, detail.toUtf8().c_str());
		return nullptr;
	}
	auto codec = entry.create(codecId, width, height);
	if (!codec)
	{
		debug(LOG_VIDEO, "Video decode backend '%s' did not initialize for %s", entry.name, codecId);
		return nullptr;
	}
	debug(LOG_VIDEO, "Initialized %s %s decoder (%ux%u) [%s]", codec->backendName(), codecId,
	      width, height, detail.toUtf8().c_str());
	return codec;
}

std::unique_ptr<WZWebmVideoCodec> wzWebmVideoCodecCreate(const char *codecId, unsigned width, unsigned height, WZWebmCodecBackend preference)
{
	const WZWebmCodecBackend wanted = (preference == WZWebmCodecBackend::Auto) ? backendTable[0].id : preference;

	// pass 0: the requested backend. pass 1: everything else, in registry order.
	for (int pass = 0; pass < 2; ++pass)
	{
		for (const auto& entry : backendTable)
		{
			if ((entry.id == wanted) != (pass == 0))
			{
				continue;
			}
			if (auto codec = tryCreate(entry, codecId, width, height))
			{
				return codec;
			}
		}
	}
	return nullptr;
}

std::unique_ptr<WZWebmVideoCodec> wzWebmVideoCodecCreateReplacement(const char *codecId, unsigned width, unsigned height, const WZWebmVideoCodec& failed)
{
	for (const auto& entry : backendTable)
	{
		if (strcmp(entry.name, failed.backendName()) == 0)
		{
			continue;
		}
		if (auto codec = tryCreate(entry, codecId, width, height))
		{
			return codec;
		}
	}
	return nullptr;
}
