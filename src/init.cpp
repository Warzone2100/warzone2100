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
static std::vector<uint8_t> inMemoryMapArchiveData;
static size_t inMemoryMapArchiveMounted = 0;

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

enum MODS_PATHS: size_t
{
	MODS_MUSIC,
	MODS_GLOBAL,
	MODS_AUTOLOAD,
	MODS_CAMPAIGN,
	MODS_MULTIPLAY,
	MODS_PATHS_MAX
};

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
			return version_getVersionedModsFolderPath("campaign");
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

static const char* versionedModsPath(MODS_PATHS type)
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
	statsInitVars();
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
}


/*!
 * Register searchPath above the path with next lower priority
 * For information about what can be a search path, refer to PhysFS documentation
 */
void registerSearchPath(const std::string& newPath, unsigned int priority)
{
	ASSERT_OR_RETURN(, !newPath.empty(), "Calling registerSearchPath with empty path, priority %u", priority);
	std::unique_ptr<wzSearchPath> tmpSearchPath = std::unique_ptr<wzSearchPath>(new wzSearchPath());
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
	ASSERT_OR_RETURN(, !inMemoryMapArchiveData.empty(), "Already freed??");
	ASSERT_OR_RETURN(, inMemoryMapArchiveData.data() == pData, "Unexpected pointer received?");
	if (!inMemoryMapVirtualFilenameUID.empty())
	{
		levRemoveDataSetByRealFileName(inMemoryMapVirtualFilenameUID.c_str(), nullptr);
		WZ_Maps.erase(inMemoryMapVirtualFilenameUID);
	}
	inMemoryMapVirtualFilenameUID.clear();
	inMemoryMapArchiveData.clear();
	if (inMemoryMapArchiveMounted > 0)
	{
		--inMemoryMapArchiveMounted;
	}
}

static bool WZ_PHYSFS_MountSearchPathWrapper(const char *newDir, const char *mountPoint, int appendToPath)
{
	if (PHYSFS_mount(newDir, mountPoint, appendToPath) != 0)
	{
		return true;
	}
	else
	{
		// mount failed
#if defined(WZ_PHYSFS_2_1_OR_GREATER)
		auto errorCode = PHYSFS_getLastErrorCode();
		if (errorCode != PHYSFS_ERR_NOT_FOUND)
		{
			const char* errorStr = PHYSFS_getErrorByCode(errorCode);
			searchPathMountErrors.push_back(astringf("Failed to mount \"%s\" @ \"%s\": %s", newDir, mountPoint, (errorStr) ? errorStr : "<no details available?>"));
		}
#endif
		return false;
	}
}

/*!
 * \brief Rebuilds the PHYSFS searchPath with mode specific subdirs
 *
 * Priority:
 * maps > mods > base > base.wz
 */
bool rebuildSearchPath(searchPathMode mode, bool force, const char *current_map, const char* current_map_mount_point)
{
	static searchPathMode current_mode = mod_clean;
	static std::string current_current_map;
	std::string tmpstr;

	if (mode != current_mode || (current_map != nullptr ? current_map : "") != current_current_map || force ||
	    (use_override_mods && override_mod_list != getModList()))
	{
		if (mode != mod_clean)
		{
			rebuildSearchPath(mod_clean, false);
		}

		current_mode = mode;
		current_current_map = current_map != nullptr ? current_map : "";

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
			// Add the selected map first, for mapmod support
			if (current_map != nullptr)
			{
				if (inMemoryMapVirtualFilenameUID.empty() || current_map != inMemoryMapVirtualFilenameUID)
				{
					// mount it as a normal physical map path
					WzString realPathAndDir = WzString::fromUtf8(PHYSFS_getRealDir(current_map)) + current_map;
					realPathAndDir.replace("/", PHYSFS_getDirSeparator()); // Windows fix
					WZ_PHYSFS_MountSearchPathWrapper(realPathAndDir.toUtf8().c_str(), current_map_mount_point, PHYSFS_APPEND);
				}
				else if (!inMemoryMapArchiveData.empty())
				{
					// mount the in-memory map archive as a virtual file
					if (PHYSFS_mountMemory_fixed(inMemoryMapArchiveData.data(), inMemoryMapArchiveData.size(), clearInMemoryMapFile, inMemoryMapVirtualFilenameUID.c_str(), current_map_mount_point, PHYSFS_APPEND) != 0)
					{
						inMemoryMapArchiveMounted++;
					}
				}
				else
				{
					debug(LOG_ERROR, "Specified virtual map file, but no data?");
				}
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
			clearOverrideMods();
			current_mode = mod_override;
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

		ActivityManager::instance().rebuiltSearchPath();
	}
	else if (use_override_mods)
	{
		// override mods are already the same as current mods, so no need to do anything
		clearOverrideMods();
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

	// save our current search path(s)
	debug(LOG_WZ, "Map search paths:");
	char **searchPath = PHYSFS_getSearchPath();
	for (char **i = searchPath; *i != nullptr; i++)
	{
		debug(LOG_WZ, "    [%s]", *i);
		oldSearchPath.push_back(*i);
		WZ_PHYSFS_unmount(*i);
	}
	PHYSFS_freeList(searchPath);

	for (const auto &realFileName : ret)
	{
		std::string realFilePathAndName = PHYSFS_getWriteDir() + realFileName.platformDependent;
		if (PHYSFS_mount(realFilePathAndName.c_str(), NULL, PHYSFS_APPEND))
		{
			size_t numMapGamFiles = 0;
			size_t numMapFolders = 0;
			const std::string baseMapsPath("multiplay/maps/");
			bool enumSuccess = WZ_PHYSFS_enumerateFiles("multiplay/maps", [&numMapGamFiles, &numMapFolders, &baseMapsPath, &realFilePathAndName](const char *file) -> bool {
				if (!file)
				{
					return true; // continue
				}
				std::string isDir = baseMapsPath + file;
				if (WZ_PHYSFS_isDirectory(isDir.c_str()))
				{
					if (baseMapsPath.size() == isDir.size())
					{
						// for some reason, file is an empty string - just skip
						return true; // continue;
					}
					if (++numMapFolders > 1)
					{
						debug(LOG_ERROR, "Map packs are not supported! %s NOT added.", realFilePathAndName.c_str());
						return false; // break;
					}
					return true; // continue;
				}
				std::string checkfile = file;
				debug(LOG_WZ, "checking ... %s", file);
				if (checkfile.substr(checkfile.find_last_of('.') + 1) == "gam")
				{
					if (++numMapGamFiles > 1)
					{
						debug(LOG_ERROR, "Map packs are not supported! %s NOT added.", realFilePathAndName.c_str());
						return false; // break;
					}
				}
				return true; // continue
			});
			if (!enumSuccess)
			{
				// Failed to enumerate contents - corrupt map archive (ignore it)
				debug(LOG_ERROR, "Failed to enumerate - ignoring corrupt map file: %s", realFilePathAndName.c_str());
				numMapGamFiles = std::numeric_limits<size_t>::max() - 1;
				numMapFolders = std::numeric_limits<size_t>::max() - 1;
			}
			if (numMapGamFiles < 2 && numMapFolders < 2)
			{
				filtered.push_back(realFileName);
			}
			WZ_PHYSFS_unmount(realFilePathAndName.c_str());
		}
		else
		{
			debug(LOG_POPUP, "Could not mount %s, because: %s.\nPlease delete or move the file specified.", realFilePathAndName.c_str(), WZ_PHYSFS_getLastError());
		}
	}

	// restore our search path(s) again
	for (const auto &restorePaths : oldSearchPath)
	{
		PHYSFS_mount(restorePaths.c_str(), NULL, PHYSFS_APPEND);
	}
	debug(LOG_WZ, "Search paths restored");
	printSearchPath();

	return filtered;
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

// Mount the archive under the mountpoint, and enumerate the archive according to lookin
static inline optional<WZmapInfo> CheckInMap(const char *archive, const std::string& mountpoint, const std::vector<std::string>& lookin_list)
{
	bool mapmod = false;
	bool isRandom = false;

	for (auto lookin_subdir : lookin_list)
	{
		std::string lookin = mountpoint;
		if (!lookin_subdir.empty())
		{
			lookin.append("/");
			lookin.append(lookin_subdir);
		}
		bool enumResult = WZ_PHYSFS_enumerateFiles(lookin.c_str(), [&](const char *file) -> bool {
			std::string checkfile = file;
			if (WZ_PHYSFS_isDirectory((lookin + "/" + checkfile).c_str()))
			{
				if (checkfile.compare("wrf") == 0 || checkfile.compare("stats") == 0 || checkfile.compare("components") == 0
					|| checkfile.compare("effects") == 0 || checkfile.compare("messages") == 0
					|| checkfile.compare("audio") == 0 || checkfile.compare("sequenceaudio") == 0 || checkfile.compare("misc") == 0
					|| checkfile.compare("features") == 0 || checkfile.compare("script") == 0 || checkfile.compare("structs") == 0
					|| checkfile.compare("tileset") == 0 || checkfile.compare("images") == 0 || checkfile.compare("texpages") == 0
					|| checkfile.compare("skirmish") == 0 || checkfile.compare("shaders") == 0 || checkfile.compare("fonts") == 0
					|| checkfile.compare("icons") == 0)
				{
					debug(LOG_WZ, "Detected: %s %s" , archive, checkfile.c_str());
					mapmod = true;
					return false; // break;
				}
			}
			return true; // continue
		});
		if (!enumResult)
		{
			// failed to enumerate - just exit out
			return nullopt;
		}

		std::string maps = lookin + "/multiplay/maps";
		enumResult = WZ_PHYSFS_enumerateFiles(maps.c_str(), [&](const char *file) -> bool {
			if (WZ_PHYSFS_isDirectory((maps + "/" + file).c_str()) && PHYSFS_exists((maps + "/" + file + "/game.js").c_str()))
			{
				isRandom = true;
				return false; // break;
			}
			return true; // continue
		});
		if (!enumResult)
		{
			// failed to enumerate - just exit out
			return nullopt;
		}
	}

	return WZmapInfo(mapmod, isRandom);
}

static std::vector<std::string> map_lookin_list = { "", "multiplay" };

// Process a map that has been mounted in the PhysFS virtual filesystem
//
// Verifies the index data, determines attributes, and adds to the level loading system so it can be loaded
//
// - archive: Directory or archive added to the path, in platform-dependent notation
// - realFileName_platformIndependent:
//     For actual map archives, this is the platform independent unique filename + parent path for the map
//     For "virtual" map archives this is basically a lookup key that should be unique
// - mountPoint: Location in the interpolated PhysFS tree that this archive was "mounted" (in platform-independent notation)
bool processMap(const char* archive, const char* realFileName_platformIndependent, const std::string& mountPoint, bool rejectMapMods = false)
{
	auto WZmapInfoResult = CheckInMap(archive, mountPoint, map_lookin_list);
	if (!WZmapInfoResult.has_value())
	{
		// failed to enumerate contents
		return false;
	}
	if (rejectMapMods && WZmapInfoResult.value().isMapMod)
	{
		debug(LOG_WZ, "Rejecting map mod: %s", archive);
		return false;
	}

	WzMapPhysFSIO mapIO;

	bool containsMap = false;

	// First pass: Look for new level.json (which are in multiplay/maps/<map name>)
	std::string mapsDirPath = mapIO.pathJoin(mountPoint.c_str(), "multiplay/maps");
	bool enumSuccess = WZ_PHYSFS_enumerateFolders(mapsDirPath, [&](const char *folder) -> bool {
		if (!folder) { return true; }
		if (*folder == '\0') { return true; }

		std::string levelJSONPath = std::string("multiplay/maps/") + folder + "/level.json";
		if (PHYSFS_exists(WzString::fromUtf8(mountPoint + "/" + levelJSONPath)) && loadLevFile_JSON(mountPoint, levelJSONPath, mod_multiplay, realFileName_platformIndependent))
		{
			containsMap = true;
			return false; // stop enumerating
		}
		return true;
	});
	if (!enumSuccess)
	{
		// Failed to enumerate contents - corrupt map archive
		return false;
	}

	// Or "flattened" self-contained maps (where level.json is in the root)
	if (!containsMap)
	{
		if (PHYSFS_exists(WzString::fromUtf8(mountPoint + "/" + "level.json")) && loadLevFile_JSON(mountPoint, "level.json", mod_multiplay, realFileName_platformIndependent))
		{
			containsMap = true;
		}
	}

	if (containsMap)
	{
		std::string MapName = realFileName_platformIndependent;
		WZ_Maps.insert(WZMapInfo_Map::value_type(MapName, WZmapInfoResult.value()));
		return true;
	}

	// Second pass: Look for older / classic maps (with *.addon.lev / *.xplayers.lev in the root)
	enumSuccess = WZ_PHYSFS_enumerateFiles(mountPoint.c_str(), [&](const char *file) -> bool {
		size_t len = strlen(file);
		if ((len > 10 && !strcasecmp(file + (len - 10), ".addon.lev"))  // Do not add addon.lev again // <--- Err, what? The code has loaded .addon.lev for a while...
			|| (len > 13 && !strcasecmp(file + (len - 13), ".xplayers.lev"))) // add support for X player maps using a new name to prevent conflicts.
		{
			std::string fullPath = mountPoint + "/" + file;
			if (loadLevFile(mountPoint + "/" + file, mod_multiplay, true, realFileName_platformIndependent))
			{
				containsMap = true;
				return false; // stop enumerating
			}
		}
		return true; // continue
	});

	if (!enumSuccess)
	{
		// Failed to enumerate contents - corrupt map archive
		return false;
	}

	if (containsMap)
	{
		std::string MapName = realFileName_platformIndependent;
		WZ_Maps.insert(WZMapInfo_Map::value_type(MapName, WZmapInfoResult.value()));
		return true;
	}

	return false;
}

#if defined(HAS_PHYSFS_IO_SUPPORT)
bool setSpecialInMemoryMap(std::vector<uint8_t>&& mapArchiveData)
{
	ASSERT_OR_RETURN(false, !mapArchiveData.empty(), "Null map archive data passed?");
	ASSERT_OR_RETURN(false, inMemoryMapArchiveMounted == 0, "In-memory map archive already mounted");

	// calculate a hash for the map data
	Sha256 mapHash = sha256Sum(mapArchiveData.data(), mapArchiveData.size());

	// generate a new unique filename UID for the in-memory map
	inMemoryMapVirtualFilenameUID = "<in-memory>::mapArchive::" + mapHash.toString();

	// store the map archive data
	inMemoryMapArchiveData = std::move(mapArchiveData);

	// try to mount the in-memory archive
	if (PHYSFS_mountMemory_fixed(inMemoryMapArchiveData.data(), inMemoryMapArchiveData.size(), nullptr, inMemoryMapVirtualFilenameUID.c_str(), "WZMap", PHYSFS_APPEND) == 0)
	{
		// Failed to mount data - corrupt map archive
		debug(LOG_ERROR, "Failed to mount - corrupt / invalid map file: %s", inMemoryMapVirtualFilenameUID.c_str());
		inMemoryMapVirtualFilenameUID.clear();
		inMemoryMapArchiveData.clear();
		return false;
	}

	// load it into the level-loading system
	if (!processMap(inMemoryMapVirtualFilenameUID.c_str(), inMemoryMapVirtualFilenameUID.c_str(), "WZMap", true))
	{
		// Failed to enumerate contents - corrupt map archive
		debug(LOG_ERROR, "Failed to enumerate - corrupt / invalid map file: %s", inMemoryMapVirtualFilenameUID.c_str());
		inMemoryMapVirtualFilenameUID.clear();
		inMemoryMapArchiveData.clear();
		return false;
	}

	if (WZ_PHYSFS_unmount(inMemoryMapVirtualFilenameUID.c_str()) == 0)
	{
		debug(LOG_ERROR, "Could not unmount %s, %s", inMemoryMapVirtualFilenameUID.c_str(), WZ_PHYSFS_getLastError());
	}

	// fix-up level hash
	levSetFileHashByRealFileName(inMemoryMapVirtualFilenameUID.c_str(), mapHash);

	return true;
}
#else
bool setSpecialInMemoryMap(std::vector<uint8_t>&& mapArchiveData)
{
	// Sadly, the version of PhysFS used for compilation is too old
	debug(LOG_INFO, "The version of PhysFS used for compilation is too old, and does not support PHYSFS_Io");
	return false;
}
#endif

bool buildMapList()
{
	if (!loadLevFile("gamedesc.lev", mod_campaign, false, nullptr))
	{
		return false;
	}
	loadLevFile("addon.lev", mod_multiplay, false, nullptr);
	WZ_Maps.clear();
	if (!inMemoryMapArchiveMounted && !inMemoryMapArchiveData.empty())
	{
		debug(LOG_INFO, "Clearing in-memory map archive (since it isn't currently loaded)");
		clearInMemoryMapFile(inMemoryMapArchiveData.data());
	}
	MapFileList realFileNames = listMapFiles();
	for (auto &realFileName : realFileNames)
	{
		const char * pRealDirStr = PHYSFS_getRealDir(realFileName.platformIndependent.c_str());
		if (!pRealDirStr)
		{
			debug(LOG_ERROR, "Failed to find realdir for: %s", realFileName.platformIndependent.c_str());
			continue; // skip
		}
		std::string realFilePathAndName = pRealDirStr + realFileName.platformDependent;

		if (PHYSFS_mount(realFilePathAndName.c_str(), "WZMap", PHYSFS_APPEND) == 0)
		{
			debug(LOG_POPUP, "Could not mount %s, because: %s.\nPlease delete or move the file specified.", realFilePathAndName.c_str(), WZ_PHYSFS_getLastError());
			continue; // skip
		}

		if (!processMap(realFilePathAndName.c_str(), realFileName.platformIndependent.c_str(), "WZMap"))
		{
			// Failed to enumerate contents - corrupt map archive
			debug(LOG_ERROR, "Failed to enumerate - corrupt / invalid map file: %s", realFilePathAndName.c_str());
		}

		if (WZ_PHYSFS_unmount(realFilePathAndName.c_str()) == 0)
		{
			debug(LOG_ERROR, "Could not unmount %s, %s", realFilePathAndName.c_str(), WZ_PHYSFS_getLastError());
		}
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

	if (audio_Disabled() == false && !audio_Shutdown())
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

	//do this before shutting down the iV library
	resReleaseAllData();
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

	if (audio_Disabled() == false)
	{
		sound_CheckAllUnloaded();
	}

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

	// Use mod_multiplay as the default (campaign might have set it to mod_singleplayer)
	rebuildSearchPath(mod_multiplay, true);
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

			if (!myResponsibility(i))
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


	if (!ShutdownRadar())
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

	return true;
}

bool stageThreeInitialise()
{
	bool fromSave = (getSaveGameType() == GTYPE_SAVE_START || getSaveGameType() == GTYPE_SAVE_MIDMISSION);

	debug(LOG_WZ, "== stageThreeInitialise ==");

	loopMissionState = LMS_NORMAL;

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

	resizeRadar();

	setAllPauseStates(false);

	countUpdate();

	if (getLevelLoadType() == GTYPE_SAVE_MIDMISSION || getLevelLoadType() == GTYPE_SAVE_START)
	{
		triggerEvent(TRIGGER_GAME_LOADED);
	}
	else
	{
		const DebugInputManager& dbgInputManager = gInputManager.debugManager();
		if (dbgInputManager.debugMappingsAllowed())
		{
			triggerEventCheatMode(true);
		}
		triggerEvent(TRIGGER_GAME_INIT);
		playerBuiltHQ = structureExists(selectedPlayer, REF_HQ, true, false);
	}

	// Start / randomize in-game music
	cdAudio_PlayTrack(SONG_INGAME);

	return true;
}

/*****************************************************************************/
/*      Shutdown before any data is released                                 */

bool stageThreeShutDown()
{
	debug(LOG_WZ, "== stageThreeShutDown ==");

	setHostLaunch(HostLaunch::Normal);

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
