/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2020  Warzone 2100 Project

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
/**
 * @file init.c
 *
 * Game initialisation routines.
 *
 */
#include "lib/framework/frame.h"

#include <string.h>

#include "lib/framework/frameresource.h"
#include "lib/framework/file.h"
#include "lib/framework/physfs_ext.h"
#include "3rdparty/physfs_memoryio.h"
#include "lib/framework/wzapp.h"
#include "lib/ivis_opengl/piemode.h"
#include "lib/ivis_opengl/piestate.h"
#include "lib/ivis_opengl/screen.h"
#include "lib/ivis_opengl/pieblitfunc.h"
#include "lib/ivis_opengl/tex.h"
#include "lib/ivis_opengl/imd.h"
#include "lib/netplay/netplay.h"
#include "lib/sound/audio_id.h"
#include "lib/sound/cdaudio.h"
#include "lib/sound/mixer.h"

#include "init.h"

#include "input/manager.h"
#include "advvis.h"
#include "atmos.h"
#include "campaigninfo.h"
#include "challenge.h"
#include "cmddroid.h"
#include "configuration.h"
#include "console.h"
#include "data.h"
#include "difficulty.h" // for "double up" and "biffer baker" cheats
#include "display.h"
#include "display3d.h"
#include "edit3d.h"
#include "effects.h"
#include "formation.h"
#include "fpath.h"
#include "frend.h"
#include "frontend.h"
#include "game.h"
#include "gateway.h"
#include "hci.h"
#include "intdisplay.h"
#include "levels.h"
#include "lighting.h"
#include "loadsave.h"
#include "loop.h"
#include "mapgrid.h"
#include "mechanics.h"
#include "miscimd.h"
#include "mission.h"
#include "modding.h"
#include "multiint.h"
#include "multigifts.h"
#include "multiplay.h"
#include "multistat.h"
#include "notifications.h"
#include "projectile.h"
#include "order.h"
#include "radar.h"
#include "research.h"
#include "lib/framework/cursors.h"
#include "text.h"
#include "transporter.h"
#include "warzoneconfig.h"
#include "main.h"
#include "wrappers.h"
#include "terrain.h"
#include "ingameop.h"
#include "qtscript.h"
#include "template.h"
#include "activity.h"
#include "spectatorwidgets.h"
#include "seqdisp.h"
#include "version.h"
#include "hci/teamstrategy.h"
#include "screens/guidescreen.h"
#include "titleui/widgets/gamebrowserform.h"
#include "wzapi.h"

#include "wzphysfszipioprovider.h"
#include <wzmaplib/map_package.h>

#include <algorithm>
#include <unordered_map>
#include <array>

static void initMiscVars();

static const char UserMusicPath[] = "music";

// FIXME Totally inappropriate place for this.
char fileLoadBuffer[FILE_LOAD_BUFFER_SIZE];

IMAGEFILE *FrontImages;

static std::vector<std::unique_ptr<wzSearchPath>> searchPathRegistry;
static std::vector<std::string> searchPathMountErrors;

static std::string inMemoryMapVirtualFilenameUID;
static std::shared_ptr<WzMapZipIO> inMemoryMapArchive;

static optional<std::vector<TerrainShaderQuality>> cachedAvailableTerrainShaderQualityTextures;

struct WZmapInfo
{
public:
	WZmapInfo(bool isMapMod, bool isRandom)
	: isMapMod(isMapMod)
	, isRandom(isRandom)
	{ }
public:
	bool isMapMod;
	bool isRandom;
};

typedef std::string MapName;
typedef std::unordered_map<MapName, WZmapInfo> WZMapInfo_Map;
WZMapInfo_Map WZ_Maps;

static std::string getFullModPath(MODS_PATHS type)
{
	switch (type)
	{
		case MODS_MUSIC:
			return version_getVersionedModsFolderPath("music");
		case MODS_GLOBAL:
			return version_getVersionedModsFolderPath("global");
		case MODS_AUTOLOAD:
			return version_getVersionedModsFolderPath("autoload");
		case MODS_CAMPAIGN:
			// With the new campaign mod packaging format, we no longer need a versioned path (as the mods contain version compatibility info)
			return "mods/campaign";
		case MODS_MULTIPLAY:
			return version_getVersionedModsFolderPath("multiplay");
		case MODS_PATHS_MAX:
			break;
	}
	return version_getVersionedModsFolderPath();
}

static std::array<std::string, MODS_PATHS_MAX> buildFullModsPaths()
{
	std::array<std::string, MODS_PATHS_MAX> result;
	for (size_t i = 0; i < MODS_PATHS_MAX; ++i)
	{
		result[i] = getFullModPath(static_cast<MODS_PATHS>(i));
	}
	return result;
}

const char* versionedModsPath(MODS_PATHS type)
{
	static std::array<std::string, MODS_PATHS_MAX> cachedFullModsPaths = buildFullModsPaths();
	return cachedFullModsPaths[type].c_str();
}

// Each module in the game should have a call from here to initialise
// any globals and statics to there default values each time the game
// or frontend restarts.
//
static bool InitialiseGlobals()
{
	frontendInitVars();	// Initialise frontend globals and statics.
	structureInitVars();
	if (!messageInitVars())
	{
		return false;
	}
	if (!researchInitVars())
	{
		return false;
	}
	featureInitVars();
	radarInitVars();
	Edit3DInitVars();

	return true;
}


bool loadLevFile(const std::string& filename, searchPathMode pathMode, bool ignoreWrf, char const *realFileName)
{
	char *pBuffer;
	UDWORD size;

	if (realFileName == nullptr)
	{
		debug(LOG_WZ, "Loading lev file: \"%s\", builtin\n", filename.c_str());
	}
	else
	{
		debug(LOG_WZ, "Loading lev file: \"%s\" from \"%s\"\n", filename.c_str(), realFileName);
	}

	if (!PHYSFS_exists(filename.c_str()) || !loadFile(filename.c_str(), &pBuffer, &size))
	{
		debug(LOG_ERROR, "File not found: %s\n", filename.c_str());
		return false; // only in NDEBUG case
	}
	if (!levParse(pBuffer, size, pathMode, ignoreWrf, realFileName))
	{
		debug(LOG_ERROR, "Parse error in %s\n", filename.c_str());
		free(pBuffer);
		return false;
	}
	free(pBuffer);

	return true;
}

bool loadLevFile_JSON(const std::string& mountPoint, const std::string& filename, searchPathMode pathMode, char const *realFileName)
{
	if (realFileName == nullptr)
	{
		debug(LOG_WZ, "Loading lev file: \"%s\", builtin\n", filename.c_str());
	}
	else
	{
		debug(LOG_WZ, "Loading lev file: \"%s\" from \"%s\"\n", filename.c_str(), realFileName);
	}

	if (!levParse_JSON(mountPoint, filename, pathMode, realFileName))
	{
		debug(LOG_ERROR, "Failed to load: %s\n", filename.c_str());
		return false;
	}

	return true;
}


static void cleanSearchPath()
{
	searchPathRegistry.clear();
	cachedAvailableTerrainShaderQualityTextures.reset();
}


/*!
 * Register searchPath above the path with next lower priority
 * For information about what can be a search path, refer to PhysFS documentation
 */
void registerSearchPath(const std::string& newPath, unsigned int priority)
{
	ASSERT_OR_RETURN(, !newPath.empty(), "Calling registerSearchPath with empty path, priority %u", priority);
	std::unique_ptr<wzSearchPath> tmpSearchPath = std::make_unique<wzSearchPath>();
	tmpSearchPath->path = newPath;
	if (!strEndsWith(tmpSearchPath->path, PHYSFS_getDirSeparator()))
	{
		tmpSearchPath->path += PHYSFS_getDirSeparator();
	}
	tmpSearchPath->priority = priority;

	debug(LOG_WZ, "registerSearchPath: Registering %s at priority %i", newPath.c_str(), priority);

	// insert in sorted order
	auto insert_pos = std::upper_bound(searchPathRegistry.begin(), searchPathRegistry.end(), tmpSearchPath, [](const std::unique_ptr<wzSearchPath>& a, const std::unique_ptr<wzSearchPath>& b) {
		return a->priority < b->priority;
	});
	searchPathRegistry.insert(insert_pos, std::move(tmpSearchPath));

	cachedAvailableTerrainShaderQualityTextures.reset();
}

void unregisterSearchPath(const std::string& path)
{
	for (auto it = searchPathRegistry.begin(); it != searchPathRegistry.end(); )
	{
		const auto& curSearchPath = *it;
		if (curSearchPath && (curSearchPath->path.compare(path) == 0))
		{
			// ignore this notification - remove from the list
			it = searchPathRegistry.erase(it);
			continue;
		}

		++it;
	}

	cachedAvailableTerrainShaderQualityTextures.reset();
}

std::list<std::string> getPhysFSSearchPathsAsStr()
{
	std::list<std::string> results;
	char **list = PHYSFS_getSearchPath();
	if (list == NULL)
	{
		return {};
	}
	for (char **i = list; *i != NULL; i++)
	{
		results.push_back(*i);
	}
	PHYSFS_freeList(list);
	return results;
}

void debugOutputSearchPaths()
{
	debug(LOG_INFO, "Search path registry:");
	size_t idx = 0;
	for (const auto& curSearchPath : searchPathRegistry)
	{
		debug(LOG_INFO, "- [%zu] [priority: %u]: %s", idx, curSearchPath->priority, curSearchPath->path.c_str());
		++idx;
	}
	debug(LOG_INFO, "Enumerated search paths:");
	auto searchPaths = getPhysFSSearchPathsAsStr();
	idx = 0;
	for (const auto& curSearchPath : searchPaths)
	{
		debug(LOG_INFO, "- [%zu]: %s", idx,  curSearchPath.c_str());
		++idx;
	}
}

void debugOutputSearchPathMountErrors()
{
	if (searchPathMountErrors.empty())
	{
		return;
	}
	debug(LOG_INFO, "Search path mount errors:");
	for (const auto& errorStr : searchPathMountErrors)
	{
		debug(LOG_INFO, "%s", errorStr.c_str());
	}
}

static void clearAllPhysFSSearchPaths()
{
	auto searchPaths = getPhysFSSearchPathsAsStr();
	if (searchPaths.empty())
	{
		return;
	}

	std::unordered_map<std::string, std::string> mountPathToErrorUnmounting;
	size_t searchPathsRemoved = 0;
	do
	{
		searchPathsRemoved = 0;
		mountPathToErrorUnmounting.clear();
		for (auto i = searchPaths.begin(); i != searchPaths.end();)
		{
			if (WZ_PHYSFS_unmount(i->c_str()) != 0)
			{
				++searchPathsRemoved;
				i = searchPaths.erase(i);
			}
			else
			{
				const char* pErrorStr = WZ_PHYSFS_getLastError();
				mountPathToErrorUnmounting[*i] = (pErrorStr) ? pErrorStr : "<unknown>";
				++i;
			}
		}
	} while (!searchPaths.empty() && searchPathsRemoved > 0);


	for (auto i = searchPaths.begin(); i != searchPaths.end(); i++)
	{
		auto it_error = mountPathToErrorUnmounting.find(*i);
		debug(LOG_WZ, "Unable to unmount search path: %s; (Reason: %s)", i->c_str(), (it_error != mountPathToErrorUnmounting.end()) ? it_error->second.c_str() : "");
	}
}

static void clearInMemoryMapFile(void *pData)
{
	ASSERT_OR_RETURN(, inMemoryMapArchive != nullptr, "Already freed??");
	ASSERT_OR_RETURN(, inMemoryMapArchive.get() == pData, "Unexpected pointer received?");
	if (!inMemoryMapVirtualFilenameUID.empty())
	{
		levRemoveDataSetByRealFileName(inMemoryMapVirtualFilenameUID.c_str(), nullptr);
		WZ_Maps.erase(inMemoryMapVirtualFilenameUID);
	}
	inMemoryMapVirtualFilenameUID.clear();
	inMemoryMapArchive.reset();
}

static bool WZ_PHYSFS_MountSearchPathWrapper(const char *newDir, const char *mountPoint, int appendToPath)
{
	ASSERT_OR_RETURN(false, newDir != nullptr, "Null newDir");
	if (PHYSFS_mount(newDir, mountPoint, appendToPath) != 0)
	{
		return true;
	}
	else
	{
		// mount failed
		auto errorCode = PHYSFS_getLastErrorCode();
		if (errorCode != PHYSFS_ERR_NOT_FOUND)
		{
			const char* errorStr = PHYSFS_getErrorByCode(errorCode);
			searchPathMountErrors.push_back(astringf("Failed to mount \"%s\" @ \"%s\": %s", newDir, (mountPoint) ? mountPoint : "/", (errorStr) ? errorStr : "<no details available?>"));
		}
		return false;
	}
}

struct RebuildSearchPathCommand
{
	searchPathMode mode = mod_clean;
	bool force = false;
	TerrainShaderQuality lastTerrainShaderQuality = TerrainShaderQuality::MEDIUM;
};

static RebuildSearchPathCommand lastCommand;

bool rebuildExistingSearchPathWithGraphicsOptionChange()
{
	return rebuildSearchPath(lastCommand.mode, false /* do not force */);
}

optional<std::string> getTerrainOverrideBaseSourcePath(TerrainShaderQuality quality)
 {
	switch (quality)
	{
		case TerrainShaderQuality::CLASSIC:
			return "terrain_overrides/classic";
		case TerrainShaderQuality::MEDIUM:
			return nullopt;
		case TerrainShaderQuality::NORMAL_MAPPING:
			return "terrain_overrides/high";
		case TerrainShaderQuality::UNINITIALIZED_PICK_DEFAULT:
			return nullopt;
	}
	return nullopt; // silence compiler warning
 }

optional<std::string> getCurrentTerrainOverrideBaseSourcePath()
{
	return getTerrainOverrideBaseSourcePath(getTerrainShaderQuality());
}

std::vector<TerrainShaderQuality> getAvailableTerrainShaderQualityTextures()
{
	// Cached result, which is invalidated with any calls to: cleanSearchPath / registerSearchPath / unregisterSearchPath
	if (cachedAvailableTerrainShaderQualityTextures.has_value())
	{
		return cachedAvailableTerrainShaderQualityTextures.value();
	}

	std::vector<TerrainShaderQuality> result;
	auto allQualities = getAllTerrainShaderQualityOptions();

	auto checkValidTerrainOverridesPath = [](const std::string& terrainOverridesPlatformDependentPath) -> bool {
		if (PHYSFS_mount(terrainOverridesPlatformDependentPath.c_str(), "WZTerrainOverrideTest", PHYSFS_APPEND) == 0)
		{
			return false;
		}

		// Get the actual mount point for this archive
		// (Why? Because it could have been mounted *before* this check at a different location, if it's the currently-selected terrain override pack and is present...)
		const char* mountPoint = PHYSFS_getMountPoint(terrainOverridesPlatformDependentPath.c_str());
		std::string actualMountPoint = (mountPoint) ? mountPoint : "WZTerrainOverrideTest";
		if (!strEndsWith(actualMountPoint, "/"))
		{
			actualMountPoint += "/";
		}

		bool validTerrainOverridesPackage = false;

		// Sanity check: For texpages and/or tileset folder(s)
		std::string checkFolder = actualMountPoint + "texpages";
		if (WZ_PHYSFS_isDirectory(checkFolder.c_str()))
		{
			validTerrainOverridesPackage = true;
		}
		checkFolder = actualMountPoint + "tileset";
		if (!validTerrainOverridesPackage && WZ_PHYSFS_isDirectory(checkFolder.c_str()))
		{
			validTerrainOverridesPackage = true;
		}

		if (!WZ_PHYSFS_unmount(terrainOverridesPlatformDependentPath.c_str()))
		{
			debug(LOG_WZ, "* Failed to remove path %s again", terrainOverridesPlatformDependentPath.c_str());
		}

		return validTerrainOverridesPackage;
	};

	std::string tmpstr;
	for (const auto& quality : allQualities)
	{
		auto textureOverrideBaseSourcePath = getTerrainOverrideBaseSourcePath(quality);
		bool foundTexturePack = false;
		if (textureOverrideBaseSourcePath.has_value())
		{
			// Check all search paths for this folder or .wz file
			for (const auto& curSearchPath : searchPathRegistry)
			{
				// Check <path> (folder)
				tmpstr = curSearchPath->path + textureOverrideBaseSourcePath.value();
				if (checkValidTerrainOverridesPath(tmpstr))
				{
					foundTexturePack = true;
					break;
				}

				// Check <path>.wz
				tmpstr += ".wz";
				if (checkValidTerrainOverridesPath(tmpstr))
				{
					foundTexturePack = true;
					break;
				}
			}
		}
		else
		{
			// quality doesn’t need a separate texture pack - always available
			foundTexturePack = true;
		}

		if (foundTexturePack)
		{
			result.push_back(quality);
		}
	}

	cachedAvailableTerrainShaderQualityTextures = result;

	return result;
}

/*!
 * \brief Rebuilds the PHYSFS searchPath with mode specific subdirs
 *
 * Priority:
 * maps > mods > base > base.wz
 */
bool rebuildSearchPath(searchPathMode mode, bool force)
{
	std::string tmpstr;
	static searchPathMode current_mode = mod_clean;
	auto currentTerrainShaderQuality = getTerrainShaderQuality();

	if (mode != current_mode
		|| force
		|| lastCommand.lastTerrainShaderQuality != currentTerrainShaderQuality
		|| (use_override_mods && override_mod_list != getModList()))
	{
		if (mode != mod_clean)
		{
			rebuildSearchPath(mod_clean, false);
		}

		current_mode = mode;
		lastCommand.mode = mode; // store this separately, so it doesn't get overridden by anything below
		lastCommand.force = force;
		lastCommand.lastTerrainShaderQuality = currentTerrainShaderQuality;

		auto terrainQualityOverrideBasePath = getCurrentTerrainOverrideBaseSourcePath();
		bool loadedTerrainTextureOverrides = false;

		switch (mode)
		{
		case mod_clean:
			debug(LOG_WZ, "Cleaning up");
			clearLoadedMods();

			for (const auto& curSearchPath : searchPathRegistry)
			{
#ifdef DEBUG
				debug(LOG_WZ, "Removing [%s] from search path", curSearchPath->path.c_str());
#endif // DEBUG
				// Remove maps and mods
				removeSubdirs(curSearchPath->path.c_str(), "maps");
				removeSubdirs(curSearchPath->path.c_str(), versionedModsPath(MODS_MUSIC));
				removeSubdirs(curSearchPath->path.c_str(), versionedModsPath(MODS_GLOBAL));
				removeSubdirs(curSearchPath->path.c_str(), versionedModsPath(MODS_AUTOLOAD));
				removeSubdirs(curSearchPath->path.c_str(), versionedModsPath(MODS_CAMPAIGN));
				removeSubdirs(curSearchPath->path.c_str(), versionedModsPath(MODS_MULTIPLAY));
				removeSubdirs(curSearchPath->path.c_str(), "mods/downloads"); // not versioned

				// Remove multiplay patches
				tmpstr = curSearchPath->path + "mp";
				WZ_PHYSFS_unmount(tmpstr.c_str());
				tmpstr = curSearchPath->path + "mp.wz";
				WZ_PHYSFS_unmount(tmpstr.c_str());

				// Remove plain dir
				WZ_PHYSFS_unmount(curSearchPath->path.c_str());

				// Remove base files
				tmpstr = curSearchPath->path + "base";
				WZ_PHYSFS_unmount(tmpstr.c_str());
				tmpstr = curSearchPath->path + "base.wz";
				WZ_PHYSFS_unmount(tmpstr.c_str());

				// remove video search path as well
				tmpstr = curSearchPath->path + "sequences.wz";
				WZ_PHYSFS_unmount(tmpstr.c_str());
			}

			// This should properly remove all paths, but testing is needed to ensure that all supported versions of PhysFS behave as expected
			// For now, keep the old code above as well as this new method
			clearAllPhysFSSearchPaths();
			searchPathMountErrors.clear();
			break;
		case mod_campaign:
			debug(LOG_WZ, "*** Switching to campaign mods ***");
			clearLoadedMods();

			for (const auto& curSearchPath : searchPathRegistry)
			{
				// make sure videos override included files
				tmpstr = curSearchPath->path + "sequences.wz";
				WZ_PHYSFS_MountSearchPathWrapper(tmpstr.c_str(), NULL, PHYSFS_APPEND);
			}

			for (const auto& curSearchPath : searchPathRegistry)
			{
#ifdef DEBUG
				debug(LOG_WZ, "Adding [%s] to search path", curSearchPath->path.c_str());
#endif // DEBUG
				// Add global and campaign mods
				WZ_PHYSFS_MountSearchPathWrapper(curSearchPath->path.c_str(), NULL, PHYSFS_APPEND);

				addSubdirs(curSearchPath->path.c_str(), versionedModsPath(MODS_MUSIC), PHYSFS_APPEND, nullptr, false);
				addSubdirs(curSearchPath->path.c_str(), versionedModsPath(MODS_GLOBAL), PHYSFS_APPEND, use_override_mods ? &override_mods : &global_mods, true);
				addSubdirs(curSearchPath->path.c_str(), versionedModsPath(MODS_AUTOLOAD), PHYSFS_APPEND, use_override_mods ? &override_mods : nullptr, true);
				addSubdirs(curSearchPath->path.c_str(), versionedModsPath(MODS_CAMPAIGN), PHYSFS_APPEND, use_override_mods ? &override_mods : &campaign_mods, true);
				if (!WZ_PHYSFS_unmount(curSearchPath->path.c_str()))
				{
					debug(LOG_WZ, "* Failed to remove path %s again", curSearchPath->path.c_str());
				}

				// Add plain dir
				WZ_PHYSFS_MountSearchPathWrapper(curSearchPath->path.c_str(), NULL, PHYSFS_APPEND);

				if (terrainQualityOverrideBasePath.has_value() && !loadedTerrainTextureOverrides)
				{
					// Add terrain quality override files
					tmpstr = curSearchPath->path + terrainQualityOverrideBasePath.value();
					loadedTerrainTextureOverrides = WZ_PHYSFS_MountSearchPathWrapper(tmpstr.c_str(), NULL, PHYSFS_APPEND) || loadedTerrainTextureOverrides;
					tmpstr += ".wz";
					loadedTerrainTextureOverrides = WZ_PHYSFS_MountSearchPathWrapper(tmpstr.c_str(), NULL, PHYSFS_APPEND) || loadedTerrainTextureOverrides;
					if (loadedTerrainTextureOverrides)
					{
						debug(LOG_INFO, "Loaded terrain overrides from: %s", curSearchPath->path.c_str());
					}
				}

				// Add base files
				tmpstr = curSearchPath->path + "base";
				WZ_PHYSFS_MountSearchPathWrapper(tmpstr.c_str(), NULL, PHYSFS_APPEND);
				tmpstr = curSearchPath->path + "base.wz";
				WZ_PHYSFS_MountSearchPathWrapper(tmpstr.c_str(), NULL, PHYSFS_APPEND);
			}
			break;
		case mod_multiplay:
			debug(LOG_WZ, "*** Switching to multiplay mods ***");
			clearLoadedMods();

			for (const auto& curSearchPath : searchPathRegistry)
			{
				// make sure videos override included files
				tmpstr = curSearchPath->path + "sequences.wz";
				WZ_PHYSFS_MountSearchPathWrapper(tmpstr.c_str(), NULL, PHYSFS_APPEND);
			}
			for (const auto& curSearchPath : searchPathRegistry)
			{
#ifdef DEBUG
				debug(LOG_WZ, "Adding [%s] to search path", curSearchPath->path.c_str());
#endif // DEBUG
				// Add global and multiplay mods
				WZ_PHYSFS_MountSearchPathWrapper(curSearchPath->path.c_str(), NULL, PHYSFS_APPEND);
				addSubdirs(curSearchPath->path.c_str(), versionedModsPath(MODS_MUSIC), PHYSFS_APPEND, nullptr, false);

				// Only load if we are host or singleplayer (Initial mod load relies on this, too)
				if (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER || !NetPlay.bComms)
				{
					addSubdirs(curSearchPath->path.c_str(), versionedModsPath(MODS_GLOBAL), PHYSFS_APPEND, use_override_mods ? &override_mods : &global_mods, true);
					addSubdirs(curSearchPath->path.c_str(), versionedModsPath(MODS_AUTOLOAD), PHYSFS_APPEND, use_override_mods ? &override_mods : nullptr, true);
					addSubdirs(curSearchPath->path.c_str(), versionedModsPath(MODS_MULTIPLAY), PHYSFS_APPEND, use_override_mods ? &override_mods : &multiplay_mods, true);
				}
				else
				{
					std::vector<std::string> hashList;
					for (Sha256 &hash : game.modHashes)
					{
						hashList = {hash.toString()};
						addSubdirs(curSearchPath->path.c_str(), "mods/downloads", PHYSFS_APPEND, &hashList, true); // not versioned
					}
				}
				WZ_PHYSFS_unmount(curSearchPath->path.c_str());

				// Add multiplay patches
				tmpstr = curSearchPath->path + "mp";
				WZ_PHYSFS_MountSearchPathWrapper(tmpstr.c_str(), NULL, PHYSFS_APPEND);
				tmpstr = curSearchPath->path + "mp.wz";
				WZ_PHYSFS_MountSearchPathWrapper(tmpstr.c_str(), NULL, PHYSFS_APPEND);

				// Add plain dir
				WZ_PHYSFS_MountSearchPathWrapper(curSearchPath->path.c_str(), NULL, PHYSFS_APPEND);

				if (terrainQualityOverrideBasePath.has_value() && !loadedTerrainTextureOverrides)
				{
					// Add terrain quality override files
					tmpstr = curSearchPath->path + terrainQualityOverrideBasePath.value();
					loadedTerrainTextureOverrides = WZ_PHYSFS_MountSearchPathWrapper(tmpstr.c_str(), NULL, PHYSFS_APPEND) || loadedTerrainTextureOverrides;
					tmpstr += ".wz";
					loadedTerrainTextureOverrides = WZ_PHYSFS_MountSearchPathWrapper(tmpstr.c_str(), NULL, PHYSFS_APPEND) || loadedTerrainTextureOverrides;
					if (loadedTerrainTextureOverrides)
					{
						debug(LOG_INFO, "Loaded terrain overrides from: %s", curSearchPath->path.c_str());
					}
				}

				// Add base files
				tmpstr = curSearchPath->path + "base";
				WZ_PHYSFS_MountSearchPathWrapper(tmpstr.c_str(), NULL, PHYSFS_APPEND);
				tmpstr = curSearchPath->path + "base.wz";
				WZ_PHYSFS_MountSearchPathWrapper(tmpstr.c_str(), NULL, PHYSFS_APPEND);
			}
			break;
		default:
			debug(LOG_ERROR, "Can't switch to unknown mods %i", mode);
			return false;
		}
		if (use_override_mods && mode != mod_clean)
		{
			if (getModList() != override_mod_list)
			{
				debug(LOG_POPUP, _("The required mod could not be loaded: %s\n\nWarzone will try to load the game without it."), override_mod_list.c_str());
			}
		}

		// User's home dir must be first so we always see what we write
		WZ_PHYSFS_unmount(PHYSFS_getWriteDir());
		WZ_PHYSFS_MountSearchPathWrapper(PHYSFS_getWriteDir(), NULL, PHYSFS_PREPEND);

#ifdef DEBUG
		printSearchPath();
#endif // DEBUG

		if (mode != mod_clean)
		{
			gfx_api::loadTextureCompressionOverrides(); // reload these as mods may have overridden the file!
		}

		if (terrainQualityOverrideBasePath.has_value() && (mode == mod_campaign || mode == mod_multiplay))
		{
			if (!loadedTerrainTextureOverrides)
			{
				debug(LOG_INFO, "Failed to load expected terrain quality overrides: %s", (terrainQualityOverrideBasePath.value().c_str()));
			}
		}

		ActivityManager::instance().rebuiltSearchPath();
	}
	return true;
}

struct MapFileListPath
{
public:
	MapFileListPath(const std::string& platformIndependentNotation, const std::string& platformDependentNotation)
	: platformIndependent(platformIndependentNotation)
	, platformDependent(platformDependentNotation)
	{ }
	std::string platformIndependent;
	std::string platformDependent;
};
typedef std::vector<MapFileListPath> MapFileList;
static MapFileList listMapFiles()
{
	MapFileList ret, filtered;
	std::vector<std::string> oldSearchPath;

	WZ_PHYSFS_enumerateFiles("maps", [&](const char *i) -> bool {
		std::string wzfile = i;
		if (i[0] == '.' || wzfile.substr(wzfile.find_last_of('.') + 1) != "wz")
		{
			return true; // continue;
		}

		std::string realFileName_platformIndependent = std::string("maps") + "/" + i;
		std::string realFileName_platformDependent = std::string("maps") + PHYSFS_getDirSeparator() + i;
		ret.push_back(MapFileListPath(realFileName_platformIndependent, realFileName_platformDependent));
		return true; // continue
	});

	return ret;
}

// Map processing

static inline WZMapInfo_Map::iterator findMap(char const *name)
{
	return name != nullptr? WZ_Maps.find(name) : WZ_Maps.end();
}

bool CheckForMod(char const *mapFile)
{
	auto it = findMap(mapFile);
	if (it != WZ_Maps.end())
	{
		return it->second.isMapMod;
	}
	if (mapFile != nullptr)
	{
		debug(LOG_ERROR, "Couldn't find map %s", mapFile);
	}

	return false;
}

bool CheckForRandom(char const *mapFile, char const *mapDataFile0)
{
	auto it = findMap(mapFile);
	if (it != WZ_Maps.end())
	{
		return it->second.isRandom;
	}

	if (mapFile != nullptr) {
		debug(LOG_ERROR, "Couldn't find map %s", mapFile);
	}
	else if (mapDataFile0 != nullptr && strlen(mapDataFile0) > 4)
	{
		std::string fn = mapDataFile0;
		fn.resize(fn.size() - 4);
		fn += "/game.js";
		return PHYSFS_exists(fn.c_str());
	}

	return false;
}

static inline WZmapInfo CheckInMap(WzMap::MapPackage& mapPackage)
{
	bool mapmod = mapPackage.packageType() == WzMap::MapPackage::MapPackageType::Map_Mod;
	bool isRandom = mapPackage.isScriptGeneratedMap();
	return WZmapInfo(mapmod, isRandom);
}

bool setSpecialInMemoryMap(std::vector<uint8_t>&& mapArchiveData)
{
	ASSERT_OR_RETURN(false, !mapArchiveData.empty(), "Null map archive data passed?");
	ASSERT_OR_RETURN(false, inMemoryMapArchive == nullptr, "In-memory map archive already set?");

	// calculate a hash for the map data
	Sha256 mapHash = sha256Sum(mapArchiveData.data(), mapArchiveData.size());

	// generate a new unique filename UID for the in-memory map
	inMemoryMapVirtualFilenameUID = "<in-memory>::mapArchive::" + mapHash.toString();

	// open the in-memory zip archive
	std::unique_ptr<std::vector<uint8_t>> pMapArchiveData = std::make_unique<std::vector<uint8_t>>(std::move(mapArchiveData));
	inMemoryMapArchive = WzMapZipIO::openZipArchiveMemory(std::move(pMapArchiveData), false);
	if (!inMemoryMapArchive)
	{
		debug(LOG_ERROR, "Failed to mount - corrupt / invalid map archive: %s", inMemoryMapVirtualFilenameUID.c_str());
		inMemoryMapVirtualFilenameUID.clear();
		return false;
	}

	// open the map package
	auto mapPackage = WzMap::MapPackage::loadPackage("", nullptr, inMemoryMapArchive);
	if (!mapPackage)
	{
		debug(LOG_ERROR, "Failed to mount - corrupt / invalid map file: %s", inMemoryMapVirtualFilenameUID.c_str());
		inMemoryMapVirtualFilenameUID.clear();
		inMemoryMapArchive.reset();
		return false;
	}

	// load it into the level-loading system
	// (and put it at the front of the list, to ensure it's enumerated over any local copy of the map)
	if (!levAddWzMap(mapPackage->levelDetails(), mod_multiplay, inMemoryMapVirtualFilenameUID.c_str(), true))
	{
		// Failed to enumerate contents - corrupt map archive
		debug(LOG_ERROR, "Failed to enumerate - corrupt / invalid map file: %s", inMemoryMapVirtualFilenameUID.c_str());
		inMemoryMapVirtualFilenameUID.clear();
		inMemoryMapArchive.reset();
		return false;
	}

	auto WZmapInfoResult = CheckInMap(*mapPackage);
	WZ_Maps.insert(WZMapInfo_Map::value_type(inMemoryMapVirtualFilenameUID, WZmapInfoResult));

	// fix-up level hash
	levSetFileHashByRealFileName(inMemoryMapVirtualFilenameUID.c_str(), mapHash);

	return true;
}

std::shared_ptr<WzMapZipIO> getSpecialInMemoryMapArchive(const std::string& mapName)
{
	if (inMemoryMapVirtualFilenameUID.empty())
	{
		// no special in-memory map file loaded
		return nullptr;
	}
	ASSERT_OR_RETURN(nullptr, inMemoryMapVirtualFilenameUID == mapName, "mapName (%s) does not match currently-loaded in-memory map: %s", mapName.c_str(), inMemoryMapVirtualFilenameUID.c_str());
	return inMemoryMapArchive;
}

bool clearSpecialInMemoryMap()
{
	if (inMemoryMapVirtualFilenameUID.empty())
	{
		return false;
	}

	if (inMemoryMapArchive == nullptr)
	{
		return false;
	}

	debug(LOG_INFO, "Clearing in-memory map archive");
	clearInMemoryMapFile(inMemoryMapArchive.get());
	return true;
}

class WzMapLoadDebugLogger : public WzMap::LoggingProtocol
{
public:
	virtual ~WzMapLoadDebugLogger() { }
	virtual void printLog(WzMap::LoggingProtocol::LogLevel level, const char *function, int line, const char *str) override
	{
		code_part logPart = LOG_MAP;
		switch (level)
		{
			case WzMap::LoggingProtocol::LogLevel::Info_Verbose:
				logPart = LOG_NEVER;
				break;
			case WzMap::LoggingProtocol::LogLevel::Info:
				logPart = LOG_NEVER;
				break;
			case WzMap::LoggingProtocol::LogLevel::Warning:
				logPart = LOG_MAP;
				break;
			case WzMap::LoggingProtocol::LogLevel::Error:
				logPart = (m_logErrors) ? LOG_ERROR : LOG_MAP;
				break;
		}
		if (enabled_debug[logPart])
		{
			_debug(line, logPart, function, "%s", str);
		}
	}
	void setLogErrors(bool enabled)
	{
		m_logErrors = enabled;
	}
private:
	bool m_logErrors = false;
};

bool buildMapList(bool campaignOnly)
{
	if (!loadLevFile("gamedesc.lev", mod_campaign, false, nullptr))
	{
		return false;
	}
	if (!campaignOnly)
	{
		loadLevFile("addon.lev", mod_multiplay, false, nullptr);
	}
	WZ_Maps.clear();
	if (inMemoryMapArchive != nullptr)
	{
		debug(LOG_INFO, "Clearing in-memory map archive");
		clearInMemoryMapFile(inMemoryMapArchive.get());
	}
	if (campaignOnly)
	{
		return true;
	}
	MapFileList realFileNames = listMapFiles();
	auto debugLoggerInstance = std::make_shared<WzMapLoadDebugLogger>();
	for (auto &realFileName : realFileNames)
	{
		const char * pRealDirStr = PHYSFS_getRealDir(realFileName.platformIndependent.c_str());
		if (!pRealDirStr)
		{
			debug(LOG_ERROR, "Failed to find realdir for: %s", realFileName.platformIndependent.c_str());
			continue; // skip
		}
		std::string realFilePathAndName = pRealDirStr + realFileName.platformDependent;

		auto zipReadSource = WzZipIOPHYSFSSourceReadProvider::make(realFileName.platformIndependent);
		if (!zipReadSource)
		{
			debug(LOG_ERROR, "Failed to open: %s", realFileName.platformIndependent.c_str());
			continue;
		}

		debugLoggerInstance->setLogErrors(true);
		auto mapZipIO = WzMapZipIO::openZipArchiveReadIOProvider(zipReadSource, debugLoggerInstance.get());
		if (!mapZipIO)
		{
			debug(LOG_INFO, "Failed to open archive: %s.\nPlease delete or move the file specified.", realFilePathAndName.c_str());
			continue;
		}
		debugLoggerInstance->setLogErrors(false);
		auto mapPackage = WzMap::MapPackage::loadPackage("", debugLoggerInstance, mapZipIO);
		if (!mapPackage)
		{
			debug(LOG_INFO, "Failed to load %s.\nPlease delete or move the file specified.", realFilePathAndName.c_str());
			continue;
		}

		if (!levAddWzMap(mapPackage->levelDetails(), mod_multiplay, realFileName.platformIndependent.c_str()))
		{
			debug(LOG_ERROR, "Corrupt / invalid map file: %s", realFilePathAndName.c_str());
			continue;
		}

		auto WZmapInfoResult = CheckInMap(*mapPackage);
		WZ_Maps.insert(WZMapInfo_Map::value_type(realFileName.platformIndependent, WZmapInfoResult));
	}

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Called once on program startup.
//
bool systemInitialise(unsigned int horizScalePercentage, unsigned int vertScalePercentage)
{
	if (!widgInitialise())
	{
		return false;
	}

	if (!notificationsInitialize())
	{
		return false;
	}

	buildMapList();

	// Initialize render engine
	if (!pie_Initialise())
	{
		debug(LOG_ERROR, "Unable to initialise renderer");
		return false;
	}

	if (!audio_Init(droidAudioTrackStopped, war_GetHRTFMode(), war_getSoundEnabled()))
	{
		debug(LOG_SOUND, "Continuing without audio");
	}
	if (war_getSoundEnabled() && war_GetMusicEnabled())
	{
		cdAudio_Open(UserMusicPath);
	}
	else
	{
		debug(LOG_SOUND, "Music disabled");
	}

	if (!dataInitLoadFuncs()) // Pass all the data loading functions to the framework library
	{
		return false;
	}

	// Initialize the iVis text rendering module
	wzSceneBegin("Main menu loop");
	iV_TextInit(horizScalePercentage, vertScalePercentage);

	pie_InitRadar();

	readAIs();
	// Reset config and order if default not found (user messed with config or maybe a mod AI is now missing).
	const std::string configDefaultAI = defaultSkirmishAI;
	if (configDefaultAI != getDefaultSkirmishAI())
	{
		readAIs();
	}

	initTerrainShaderType();

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Called once at program shutdown.
//
void systemShutdown()
{
	if (bLoadSaveUp)
	{
		closeLoadSaveOnShutdown(); // TODO: Ideally this would not be required here (refactor loadsave.cpp / frontend.cpp?)
	}

	NETclose();

	seqReleaseAll();

	pie_ShutdownRadar();
	clearLoadedMods();
	shutdownConsoleMessages();

	shutdownEffectsSystem();
	wzSceneEnd(nullptr);  // Might want to end the "Main menu loop" or "Main game loop".
	gInputManager.shutdown();

	// free up all the load functions (all the data should already have been freed)
	resReleaseAll();

	if (!multiShutdown()) // ajl. init net stuff
	{
		debug(LOG_FATAL, "Unable to multiShutdown() cleanly!");
		abort();
	}

	// shut down various databases
	shutdownKnownPlayers();

	debug(LOG_MAIN, "shutting down audio subsystems");

	debug(LOG_MAIN, "shutting down CD audio");
	cdAudio_Close();

	if (!audio_Disabled() && !audio_Shutdown())
	{
		debug(LOG_FATAL, "Unable to audio_Shutdown() cleanly!");
		abort();
	}

	debug(LOG_MAIN, "shutting down graphics subsystem");
	levShutDown();
	notificationsShutDown();
	widgShutDown();
	fpathShutdown();
	mapShutdown();
	modelShutdown();
	debug(LOG_MAIN, "shutting down everything else");
	pal_ShutDown();		// currently unused stub
	frameShutDown();	// close screen / SDL / resources / cursors / trig
	screenShutDown();
	shutdownLobbyBrowserFetches();
	netplayShutDown();	// MUST come after widgShutDown (as widget screens might have connections, etc)
	gfx_api::context::get().shutdown();
	cleanSearchPath();	// clean PHYSFS search paths
	debug_exit();		// cleanup debug routines
	PHYSFS_deinit();	// cleanup PHYSFS (If failure, state of PhysFS is undefined, and probably badly screwed up.)
	// NOTE: Exception handler is cleaned via atexit(ExchndlShutdown);
}

/***************************************************************************/

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Called At Frontend Startup.

bool frontendInitialise(const char *ResourceFile)
{
	frontendIsShuttingDown();

	debug(LOG_WZ, "== Initializing frontend == : %s", ResourceFile);

	if (!InitialiseGlobals())				// Initialise all globals and statics everywhere.
	{
		return false;
	}

	if (!stringsInitialise())				// Initialise the string system
	{
		return false;
	}

	if (!objInitialise())					// Initialise the object system
	{
		return false;
	}

	if (!allocPlayerPower())	 //set up the PlayerPower for each player - this should only be done ONCE now
	{
		return false;
	}

	debug(LOG_MAIN, "frontEndInitialise: loading resource file .....");
	if (!resLoad(ResourceFile, 0))
	{
		//need the object heaps to have been set up before loading in the save game
		return false;
	}

	if (!dispInitialise())					// Initialise the display system
	{
		return false;
	}

	FrontImages = (IMAGEFILE *)resGetData("IMG", "frontend.img");
	if (FrontImages == nullptr)
	{
		std::string errorMessage = astringf(_("Unable to load: %s."), "frontend.img");
		if (!getLoadedMods().empty())
		{
			errorMessage += " ";
			errorMessage += _("Please remove all incompatible mods.");
		}
		debug(LOG_FATAL, "%s", errorMessage.c_str());
		return false;
	}

	/* Shift the interface initialisation here temporarily so that it
		can pick up the stats after they have been loaded */
	if (!intInitialise())
	{
		return false;
	}

	// reinitialise key mappings
	gInputManager.resetMappings(false, gKeyFuncConfig);

	// Set the default uncoloured cursor here, since it looks slightly
	// better for menus and such.
	wzSetCursor(CURSOR_DEFAULT);

	SetFormAudioIDs(-1, ID_SOUND_WINDOWCLOSE);			// disable the open noise since distorted in 3dfx builds.

	initMiscVars();

	gameTimeInit();

	// hit me with some funky beats....
	cdAudio_PlayTrack(SONG_FRONTEND);

	return true;
}


bool frontendShutdown()
{
	debug(LOG_WZ, "== Shutting down frontend ==");

	saveConfig();// save settings to registry.

	if (!mechanicsShutdown())
	{
		return false;
	}

	changeTitleUI(nullptr);
	if (challengesUp)
	{
		closeChallenges(); // TODO: Ideally this would not be required here (refactor challenge.cpp / frontend.cpp?)
	}
	if (bLoadSaveUp)
	{
		closeLoadSaveOnShutdown(); // TODO: Ideally this would not be required here (refactor loadsave.cpp / frontend.cpp?)
	}
	interfaceShutDown();
	shutdownGameGuide();

	//do this before shutting down the iV library
	resReleaseAllData();
	FrontImages = nullptr;
	frontendIsShuttingDown();

	if (!objShutdown())
	{
		return false;
	}

	ResearchRelease();

	debug(LOG_TEXTURE, "=== frontendShutdown ===");
	modelShutdown();
	pie_TexShutDown();
	pie_TexInit(); // ready for restart
	freeComponentLists();
	statsShutDown();

	return true;
}


/******************************************************************************/
/*                       Initialisation before data is loaded                 */



bool stageOneInitialise()
{
	debug(LOG_WZ, "== stageOneInitialise ==");
	wzSceneEnd("Main menu loop");
	wzSceneBegin("Main game loop");

	// Initialise all globals and statics everywhere.
	if (!InitialiseGlobals())
	{
		return false;
	}

	if (!stringsInitialise())	/* Initialise the string system */
	{
		return false;
	}

	if (!objInitialise())		/* Initialise the object system */
	{
		return false;
	}

	if (!droidInit())
	{
		return false;
	}

	if (!initViewData())
	{
		return false;
	}

	if (!grpInitialise())
	{
		return false;
	}

	if (!aiInitialise())		/* Initialise the AI system */ // pregame
	{
		return false;
	}

	if (!allocPlayerPower())	/*set up the PlayerPower for each player - this should only be done ONCE now*/
	{
		return false;
	}

	if (!formationInitialise())		// Initialise the formation system
	{
		return false;
	}

	// initialise the visibility stuff
	if (!visInitialise())
	{
		return false;
	}

	if (!proj_InitSystem())
	{
		return false;
	}

	if (!gridInitialise())
	{
		return false;
	}

	initMission();
	initTransporters();
	scriptInit();

	gameTimeInit();
	transitionInit();
	resetScroll();

	strategyPlansInit();

	return true;
}


/******************************************************************************/
/*                       Shutdown after data is released                      */

bool stageOneShutDown()
{
	debug(LOG_WZ, "== stageOneShutDown ==");

	atmosSetWeatherType(WT_NONE); // reset weather and free its data
	wzPerfShutdown();

	pie_FreeShaders();

	if (!audio_Disabled())
	{
		sound_CheckAllUnloaded();
	}

	shutdownGameGuide();

	proj_Shutdown();

	releaseMission();

	if (!aiShutdown())
	{
		return false;
	}

	if (!objShutdown())
	{
		return false;
	}

	grpShutDown();

	formationShutDown();

	ResearchRelease();

	//free up the gateway stuff?
	gwShutDown();

	shutdownTerrain();

	if (!mapShutdown())
	{
		return false;
	}

	gridShutDown();

	debug(LOG_TEXTURE, "== stageOneShutDown ==");
	modelShutdown();
	pie_TexShutDown();

	bool needsLevReload = (hasOverrideMods() || hasCampaignMods()) && (game.type == LEVEL_TYPE::CAMPAIGN);
	clearOverrideMods();
	clearCampaignMods();
	clearCamTweakOptions();

	// Use mod_multiplay as the default (campaign might have set it to mod_singleplayer)
	rebuildSearchPath(mod_multiplay, true);

	if (needsLevReload)
	{
		// Clear & reload level list
		levShutDown();
		levInitialise();
		pal_Init(); // Update palettes.
		if (!buildMapList(false))
		{
			debug(LOG_FATAL, "Failed to rebuild map / level list?");
		}
	}

	clearSpecialInMemoryMap();

	pie_TexInit(); // restart it

	initMiscVars();
	reinitFactionsMapping();
	wzSceneEnd("Main game loop");
	wzSceneBegin("Main menu loop");

	return true;
}


// ////////////////////////////////////////////////////////////////////////////
// Initialise after the base data is loaded but before final level data is loaded

bool stageTwoInitialise()
{
	int i;

	debug(LOG_WZ, "== stageTwoInitialise ==");

	// prevent "double up" and "biffer baker" cheats from messing up damage modifiers
	resetDamageModifiers();

	// make sure we clear on loading; this a bad hack to fix a bug when
	// loading a savegame where we are building a lassat
	for (i = 0; i < MAX_PLAYERS; i++)
	{
		setLasSatExists(false, i);
	}

	if (!dispInitialise())		/* Initialise the display system */
	{
		return false;
	}

	if (!initMiscImds())			/* Set up the explosions */
	{
		debug(LOG_FATAL, "Can't find all the explosions graphics?");
		abort();
		return false;
	}

	if (!cmdDroidInit())
	{
		return false;
	}

	/* Shift the interface initialisation here temporarily so that it
		can pick up the stats after they have been loaded */

	if (!intInitialise())
	{
		return false;
	}

	if (!initMessage())			/* Initialise the message heaps */
	{
		return false;
	}

	if (!gwInitialise())
	{
		return false;
	}

	if (!initScripts())		// Initialise the new javascript system
	{
		return false;
	}

	// reinitialise key mappings
	gInputManager.resetMappings(false, gKeyFuncConfig);

	if (NETisReplay())
	{
		// Debug keybinds start enabled for replays
		gInputManager.contexts().set(InputContext::DEBUG_MISC, InputContext::State::ACTIVE);
	}

	// Set the default uncoloured cursor here, since it looks slightly
	// better for menus and such.
	wzSetCursor(CURSOR_DEFAULT);

	SetFormAudioIDs(ID_SOUND_WINDOWOPEN, ID_SOUND_WINDOWCLOSE);

	// Setup game queues.
	// Don't ask why this doesn't go in stage three. In fact, don't even ask me what stage one/two/three is supposed to mean, it seems about as descriptive as stage doStuff, stage doMoreStuff and stage doEvenMoreStuff...
	debug(LOG_MAIN, "Init game queues, I am %d.", selectedPlayer);
	sendQueuedDroidInfo();  // Discard any pending orders which could later get flushed into the game queue.
	if (!NETisReplay())
	{
		for (i = 0; i < MAX_CONNECTED_PLAYERS; ++i)
		{
			NETinitQueue(NETgameQueue(i));

			if (!myResponsibility(i) || !NetPlay.bComms)
			{
				NETsetNoSendOverNetwork(NETgameQueue(i));
			}
		}
	}

	// NOTE:
	// - Loading savegames calls `fPathDroidRoute` (from `loadSaveDroid`) before `stageThreeInitialise`
	//   is called, hence removing this call to `fpathInitialise` will currently break loading certain
	//   savegames (if they interact with the fPath system).
	if (!fpathInitialise())
	{
		return false;
	}

	debug(LOG_MAIN, "stageTwoInitialise: done");

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Free up after level specific data has been released but before base data is released
//
bool stageTwoShutDown()
{
	debug(LOG_WZ, "== stageTwoShutDown ==");

	shutdown3DView_FullReset();

	fpathShutdown();

	cdAudio_Stop();

	freeAllStructs();
	freeAllDroids();
	freeAllFeatures();
	freeAllFlagPositions();

	if (!messageShutdown())
	{
		return false;
	}

	if (!mechanicsShutdown())
	{
		return false;
	}

	interfaceShutDown();

	cmdDroidShutDown();

	//free up the gateway stuff?
	gwShutDown();

	if (!mapShutdown())
	{
		return false;
	}

	clearCampaignName();

	return true;
}

static void displayLoadingErrors()
{
	std::vector<std::string> loadingErrors;

	// warn the user if a mod might have corrupted models / files
	auto modelLoadingErrorCount = getModelLoadingErrorCount();
	if (modelLoadingErrorCount > 0)
	{
		debug(LOG_INFO, "Failure to load %zu game model(s) - please remove any outdated or broken mods", modelLoadingErrorCount);
		loadingErrors.push_back(astringf(_("Failure to load %zu game model(s)"), modelLoadingErrorCount));
	}
	auto modelTextureLoadingFailures = getModelTextureLoadingFailuresCount();
	if (modelTextureLoadingFailures > 0)
	{
		debug(LOG_INFO, "Failed to load textures for %zu models - please remove any outdated or broken mods", modelTextureLoadingFailures);
		loadingErrors.push_back(astringf(_("Failure to load textures for %zu model(s)"), modelTextureLoadingFailures));
	}
	auto statModelLoadingFailures = getStatModelLoadingFailures();
	if (statModelLoadingFailures > 0)
	{
		debug(LOG_INFO, "Failed to load model for %zu stats entries - please remove any outdated or broken mods", statModelLoadingFailures);
		loadingErrors.push_back(astringf(_("Failure to load model for %zu stat(s)"), statModelLoadingFailures));
	}

	if (!loadingErrors.empty())
	{
		std::string modelErrorDescription = _("Warzone 2100 encountered error(s) loading game data:");
		for (const auto& error : loadingErrors)
		{
			modelErrorDescription += "\n";
			modelErrorDescription += "- ";
			modelErrorDescription += error;
		}
		modelErrorDescription += "\n\n";
		if (!getLoadedMods().empty())
		{
			modelErrorDescription += _("Please try removing any new mods - they may have issues or be incompatible with this version.");
			modelErrorDescription += "\n\n";
			modelErrorDescription += _("Loaded mod(s):");
			modelErrorDescription += "\n";
			modelErrorDescription += std::string("- ") + getModList();
		}
		else
		{
			modelErrorDescription += _("Base game files may be corrupt or outdated - please try reinstalling the game.");
		}
		wzDisplayDialog(Dialog_Error, _("Error Loading Game Data"), modelErrorDescription.c_str());
	}
}

bool stageThreeInitialise()
{
	bool fromSave = (getSaveGameType() == GTYPE_SAVE_START || getSaveGameType() == GTYPE_SAVE_MIDMISSION);

	debug(LOG_WZ, "== stageThreeInitialise ==");

	loopMissionState = LMS_NORMAL;

	// preload model textures for current tileset
	size_t modelTilesetIdx = static_cast<size_t>(currentMapTileset);
	modelUpdateTilesetIdx(modelTilesetIdx);
	enumerateLoadedModels([](const std::string &modelName, iIMDBaseShape &s){
		for (const iIMDShape *pDisplayShape = s.displayModel(); pDisplayShape != nullptr; pDisplayShape = pDisplayShape->next.get())
		{
			pDisplayShape->getTextures();
		}
	});
	resDoResLoadCallback();		// do callback.

	if (!InitRadar()) 	// After resLoad cause it needs the game palette initialised.
	{
		return false;
	}

	// reset the clock to normal speed
	gameTimeResetMod();

	if (!init3DView())	// Initialise 3d view stuff. After resLoad cause it needs the game palette initialised.
	{
		return false;
	}

	effectResetUpdates();
	initLighting(0, 0, mapWidth, mapHeight);
	pie_InitLighting();
	getCurrentLightmapData().reset(mapWidth, mapHeight);

	if (fromSave)
	{
		// these two lines are the biggest hack in the world.
		// the reticule seems to get detached from 'reticuleup'
		// this forces it back in sync...
		intRemoveReticule();
		intAddReticule();
	}

	if (bMultiPlayer)
	{
		multiGameInit();
		initTemplates();
	}

	preProcessVisibility();

	prepareScripts(getLevelLoadType() == GTYPE_SAVE_MIDMISSION || getLevelLoadType() == GTYPE_SAVE_START);

	if (!fpathInitialise())
	{
		return false;
	}

	mapInit();
	gridReset();

	//if mission screen is up, close it.
	if (MissionResUp)
	{
		intRemoveMissionResultNoAnim();
	}

	// Re-inititialise some static variables.

	bInTutorial = false;
	rangeOnScreen = false;

	if (fromSave && ActivityManager::instance().getCurrentGameMode() == ActivitySink::GameMode::CHALLENGE)
	{
		challengeActive = true;
	}

	// add radar to interface screen, and resize
	intAddRadarWidget();
	resizeRadar();

	setAllPauseStates(false);

	countUpdate();

	if (getLevelLoadType() == GTYPE_SAVE_MIDMISSION || getLevelLoadType() == GTYPE_SAVE_START)
	{
		executeFnAndProcessScriptQueuedRemovals([]() { triggerEvent(TRIGGER_GAME_LOADED); });
	}
	else
	{
		initNoGoAreas();

		const DebugInputManager& dbgInputManager = gInputManager.debugManager();
		if (dbgInputManager.debugMappingsAllowed())
		{
			Cheated = true;
			triggerEventCheatMode(true);
			gInputManager.contexts().set(InputContext::DEBUG_MISC, InputContext::State::ACTIVE);
		}

		executeFnAndProcessScriptQueuedRemovals([]() { triggerEvent(TRIGGER_GAME_INIT); });
		playerBuiltHQ = structureExists(selectedPlayer, REF_HQ, true, false);
	}

	// Start / randomize in-game music
	cdAudio_PlayTrack(SONG_INGAME);

	// always start off with a refresh of the groups UI data
	intGroupsChanged();

	// warn the user if a mod might have corrupted models / files
	displayLoadingErrors();

	if (bMultiPlayer)
	{
		cameraToHome(selectedPlayer, false, fromSave);
		playerResponding(); // say howdy!
		if (NetPlay.bComms && !NETisReplay())
		{
			multiStartScreenInit();
		}
	}

	// Call once again to update counts (if modified by earlier wzapi events)
	countUpdate(false);

	return true;
}

/*****************************************************************************/
/*      Shutdown before any data is released                                 */

bool stageThreeShutDown()
{
	debug(LOG_WZ, "== stageThreeShutDown ==");

	resetHostLaunch();

	removeSpotters();

	// There is an asymmetry in scripts initialization and destruction, due
	// the many different ways scripts get loaded.
	if (!shutdownScripts())
	{
		return false;
	}

	specStatsViewShutdown();

	challengesUp = false;
	challengeActive = false;
	isInGamePopupUp = false;
	InGameOpUp = false;
	bInTutorial = false;

	shutdownTemplates();

	// make sure any button tips are gone.
	widgReset();

	if (!ShutdownRadar())
	{
		debug(LOG_FATAL, "Failed to shutdown radar");
	}

	audio_StopAll();

	if (bMultiPlayer)
	{
		multiGameShutdown();
	}

	//call this here before mission data is released
	if (!missionShutDown())
	{
		return false;
	}

	setScriptWinLoseVideo(PLAY_NONE);

	shutdown3DView();

	return true;
}

// Reset the game between campaigns
bool campaignReset()
{
	debug(LOG_MAIN, "campaignReset");
	gwShutDown();
	mapShutdown();
	shutdownTerrain();
	// when the terrain textures are reloaded we need to reset the radar
	// otherwise it will end up as a terrain texture somehow
	ShutdownRadar();
	InitRadar();
	return true;
}

// Reset the game when loading a save game
bool saveGameReset()
{
	debug(LOG_MAIN, "saveGameReset");

	cdAudio_Stop();

	freeAllStructs();
	freeAllDroids();
	freeAllFeatures();
	freeAllFlagPositions();
	initMission();
	initTransporters();

	//free up the gateway stuff?
	gwShutDown();
	intResetScreen(true);

	if (!mapShutdown())
	{
		return false;
	}

	freeMessages();

	return true;
}

// --- Miscellaneous Initialisation stuff that really should be done each restart
static void initMiscVars()
{
	selectedPlayer = 0;
	realSelectedPlayer = 0;
	godMode = false;

	radarOnScreen = true;
	radarPermitted = true;
	allowDesign = true;
	includeRedundantDesigns = false;
	enableConsoleDisplay(true);

	DebugInputManager& dbgInputManager = gInputManager.debugManager();
	for (unsigned n = 0; n < MAX_PLAYERS; ++n)
	{
		dbgInputManager.setPlayerWantsDebugMappings(n, false);
	}
}
