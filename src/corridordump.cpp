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
/** \file
 *  Headless corridor detector dump. See corridordump.h.
 */

#include "corridordump.h"

#include "lib/framework/frame.h"
#include "lib/framework/wzapp.h"
#include "lib/framework/physfs_ext.h"

#include "corridor_map.h"
#include "fpath.h"
#include "game_world.h"
#include "map.h"

#include <algorithm>
#include <string>

#include <nlohmann/json.hpp>

namespace
{

bool        g_active = false;
bool        g_done = false;
std::string g_mapName;

int32_t medianOf(std::vector<int32_t> v)
{
	if (v.empty())
	{
		return 0;
	}
	std::sort(v.begin(), v.end());
	return v[v.size() / 2];
}

// A full-map picture of the detection: blocked tiles as '#', open ground as
// space, and each corridor's centerline as a per-corridor character, so the
// skeleton can be eyeballed threading through the passages.
std::string renderAscii(const CorridorMap &cmap, const WorldMapState &mapState)
{
	static const char glyphs[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	std::string out;
	out.reserve(static_cast<size_t>(cmap.height) * static_cast<size_t>(cmap.width + 1));
	for (int y = 0; y < cmap.height; ++y)
	{
		for (int x = 0; x < cmap.width; ++x)
		{
			const int16_t cid = cmap.at(x, y);
			const size_t idx = static_cast<size_t>(y) * static_cast<size_t>(cmap.width) + static_cast<size_t>(x);
			const bool inSkel = !cmap.debugSkel.empty() && cmap.debugSkel[idx];
			const bool inNarrow = !cmap.debugNarrow.empty() && cmap.debugNarrow[idx];
			if (cid >= 0)
			{
				out.push_back(glyphs[cid % 36]);   // corridor centerline
			}
			else if (inNarrow)
			{
				out.push_back('-');   // narrow skeleton tile not on a kept corridor
			}
			else if (inSkel)
			{
				out.push_back('.');   // full skeleton in open space, an opening branch
			}
			else if (fpathBlockingTile(mapState, x, y, PROPULSION_TYPE_WHEELED))
			{
				out.push_back('#');
			}
			else
			{
				out.push_back(' ');
			}
		}
		out.push_back('\n');
	}
	return out;
}

void writeFile(const std::string &name, const std::string &contents)
{
	PHYSFS_file *fileHandle = PHYSFS_openWrite(name.c_str());
	if (fileHandle == nullptr)
	{
		debug(LOG_ERROR, "Failed to open %s for writing: %s", name.c_str(), WZ_PHYSFS_getLastError());
		return;
	}
	WZ_PHYSFS_writeBytes(fileHandle, contents.c_str(), static_cast<PHYSFS_uint32>(contents.size()));
	PHYSFS_close(fileHandle);
	const char *writeDir = PHYSFS_getWriteDir();
	debug(LOG_INFO, "corridordump: wrote %s%s", writeDir ? writeDir : "", name.c_str());
}

} // anonymous namespace

bool corridorDumpSelectMap(const char *mapName)
{
	if (mapName == nullptr || mapName[0] == '\0')
	{
		return false;
	}
	g_mapName = mapName;
	g_active = true;
	return true;
}

bool corridorDumpActive()
{
	return g_active;
}

const char *corridorDumpMapName()
{
	return g_mapName.c_str();
}

void corridorDumpUpdate()
{
	if (!g_active || g_done)
	{
		return;
	}
	g_done = true;

	if (!gameWorld.map.corridors)
	{
		debug(LOG_ERROR, "corridordump: no corridor map was built for %s", g_mapName.c_str());
		wzQuit(1);
		return;
	}
	const CorridorMap *cmap = gameWorld.map.corridors.get();

	nlohmann::json out = nlohmann::json::object();
	out["map"] = g_mapName;
	out["width"] = cmap->width;
	out["height"] = cmap->height;
	out["corridorCount"] = static_cast<uint32_t>(cmap->corridors.size());
	out["tileUnits"] = TILE_UNITS;
	nlohmann::json arr = nlohmann::json::array();
	for (const Corridor &c : cmap->corridors)
	{
		nlohmann::json j = nlohmann::json::object();
		j["id"] = c.id;
		j["mouthA"] = { c.mouthA.x, c.mouthA.y };
		j["mouthB"] = { c.mouthB.x, c.mouthB.y };
		j["lengthTiles"] = static_cast<uint32_t>(c.centerline.size());
		j["minWidthWorld"] = c.minWidth;
		j["minWidthTiles"] = static_cast<double>(c.minWidth) / TILE_UNITS;
		j["medianWidthWorld"] = medianOf(c.widthProfile);
		// Largest jump between consecutive centerline points, in tiles. A well
		// ordered path steps one tile at a time, so anything above ~1.5 is a
		// tracing discontinuity worth looking at.
		int32_t maxStep = 0;
		nlohmann::json pts = nlohmann::json::array();
		for (size_t k = 0; k < c.centerline.size(); ++k)
		{
			pts.push_back({ map_coord(c.centerline[k].x), map_coord(c.centerline[k].y) });
			if (k > 0)
			{
				maxStep = std::max(maxStep, iHypot(c.centerline[k] - c.centerline[k - 1]));
			}
		}
		j["maxStepTiles"] = static_cast<double>(maxStep) / TILE_UNITS;
		j["centerlineTiles"] = pts;
		arr.push_back(j);
	}
	out["corridors"] = arr;

	const std::string dumped = out.dump(2);
	fprintf(stdout, "%s\n", dumped.c_str());
	fflush(stdout);

	writeFile("corridordump_" + g_mapName + ".json", dumped);
	writeFile("corridordump_" + g_mapName + ".txt", renderAscii(*cmap, gameWorld.map));

	wzQuit(0);
}
