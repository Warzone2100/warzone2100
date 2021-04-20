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
#include "keymap.h"
#include "levels.h"
#include "lighting.h"
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

#include <algorithm>
#include <unordered_map>

static void initMiscVars();

static const char UserMusicPath[] = "music";

// FIXME Totally inappropriate place for this.
char fileLoadBuffer[FILE_LOAD_BUFFER_SIZE];

IMAGEFILE *FrontImages;

static wzSearchPath *searchPathRegistry = nullptr;

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


bool loadLevFile(const char *filename, searchPathMode pathMode, bool ignoreWrf, char const *realFileName)
{
	char *pBuffer;
	UDWORD size;

	if (realFileName == nullptr)
	{
		debug(LOG_WZ, "Loading lev file: \"%s\", builtin\n", filename);
	}
	else
	{
		debug(LOG_WZ, "Loading lev file: \"%s\" from \"%s\"\n", filename, realFileName);
	}

	if (!PHYSFS_exists(filename) || !loadFile(filename, &pBuffer, &size))
	{
		debug(LOG_ERROR, "File not found: %s\n", filename);
		return false; // only in NDEBUG case
	}
	if (!levParse(pBuffer, size, pathMode, ignoreWrf, realFileName))
	{
		debug(LOG_ERROR, "Parse error in %s\n", filename);
		free(pBuffer);
		return false;
	}
	free(pBuffer);

	return true;
}


static void cleanSearchPath()
{
	wzSearchPath *curSearchPath = searchPathRegistry, * tmpSearchPath = nullptr;

	// Start at the lowest priority
	while (curSearchPath->lowerPriority)
	{
		curSearchPath = curSearchPath->lowerPriority;
	}

	while (curSearchPath)
	{
		tmpSearchPath = curSearchPath->higherPriority;
		free(curSearchPath);
		curSearchPath = tmpSearchPath;
	}
	searchPathRegistry = nullptr;
}


/*!
 * Register searchPath above the path with next lower priority
 * For information about what can be a search path, refer to PhysFS documentation
 */
void registerSearchPath(const char path[], unsigned int priority)
{
	wzSearchPath *curSearchPath = searchPathRegistry, * tmpSearchPath = nullptr;

	tmpSearchPath = (wzSearchPath *)malloc(sizeof(*tmpSearchPath));
	sstrcpy(tmpSearchPath->path, path);
	if (path[strlen(path) - 1] != *PHYSFS_getDirSeparator())
	{
		sstrcat(tmpSearchPath->path, PHYSFS_getDirSeparator());
	}
	tmpSearchPath->priority = priority;

	debug(LOG_WZ, "registerSearchPath: Registering %s at priority %i", path, priority);
	if (!curSearchPath)
	{
		searchPathRegistry = tmpSearchPath;
		searchPathRegistry->lowerPriority = nullptr;
		searchPathRegistry->higherPriority = nullptr;
		return;
	}

	while (curSearchPath->higherPriority && priority > curSearchPath->priority)
	{
		curSearchPath = curSearchPath->higherPriority;
	}
	while (curSearchPath->lowerPriority && priority < curSearchPath->priority)
	{
		curSearchPath = curSearchPath->lowerPriority;
	}

	if (priority < curSearchPath->priority)
	{
		tmpSearchPath->lowerPriority = curSearchPath->lowerPriority;
		tmpSearchPath->higherPriority = curSearchPath;
	}
	else
	{
		tmpSearchPath->lowerPriority = curSearchPath;
		tmpSearchPath->higherPriority = curSearchPath->higherPriority;
	}

	if (tmpSearchPath->lowerPriority)
	{
		tmpSearchPath->lowerPriority->higherPriority = tmpSearchPath;
	}
	if (tmpSearchPath->higherPriority)
	{
		tmpSearchPath->higherPriority->lowerPriority = tmpSearchPath;
	}
}


/*!
 * \brief Rebuilds the PHYSFS searchPath with mode specific subdirs
 *
 * Priority:
 * maps > mods > base > base.wz
 */
bool rebuildSearchPath(searchPathMode mode, bool force, const char *current_map)
{
	static searchPathMode current_mode = mod_clean;
	static std::string current_current_map;
	wzSearchPath *curSearchPath = searchPathRegistry;
	char tmpstr[PATH_MAX] = "\0";

	if (mode != current_mode || (current_map != nullptr ? current_map : "") != current_current_map || force ||
	    (use_override_mods && override_mod_list != getModList()))
	{
		if (mode != mod_clean)
		{
			rebuildSearchPath(mod_clean, false);
		}

		current_mode = mode;
		current_current_map = current_map != nullptr ? current_map : "";

		// Start at the lowest priority
		while (curSearchPath->lowerPriority)
		{
			curSearchPath = curSearchPath->lowerPriority;
		}

		switch (mode)
		{
		case mod_clean:
			debug(LOG_WZ, "Cleaning up");
			clearLoadedMods();

			while (curSearchPath)
			{
#ifdef DEBUG
				debug(LOG_WZ, "Removing [%s] from search path", curSearchPath->path);
#endif // DEBUG
				// Remove maps and mods
				removeSubdirs(curSearchPath->path, "maps");
				removeSubdirs(curSearchPath->path, "mods/music");
				removeSubdirs(curSearchPath->path, "mods/global");
				removeSubdirs(curSearchPath->path, "mods");
				removeSubdirs(curSearchPath->path, "mods/autoload");
				removeSubdirs(curSearchPath->path, "mods/campaign");
				removeSubdirs(curSearchPath->path, "mods/multiplay");
				removeSubdirs(curSearchPath->path, "mods/downloads");

				// Remove multiplay patches
				sstrcpy(tmpstr, curSearchPath->path);
				sstrcat(tmpstr, "mp");
				WZ_PHYSFS_unmount(tmpstr);
				sstrcpy(tmpstr, curSearchPath->path);
				sstrcat(tmpstr, "mp.wz");
				WZ_PHYSFS_unmount(tmpstr);

				// Remove plain dir
				WZ_PHYSFS_unmount(curSearchPath->path);

				// Remove base files
				sstrcpy(tmpstr, curSearchPath->path);
				sstrcat(tmpstr, "base");
				WZ_PHYSFS_unmount(tmpstr);
				sstrcpy(tmpstr, curSearchPath->path);
				sstrcat(tmpstr, "base.wz");
				WZ_PHYSFS_unmount(tmpstr);

				// remove video search path as well
				sstrcpy(tmpstr, curSearchPath->path);
				sstrcat(tmpstr, "sequences.wz");
				WZ_PHYSFS_unmount(tmpstr);
				curSearchPath = curSearchPath->higherPriority;
			}
			break;
		case mod_campaign:
			debug(LOG_WZ, "*** Switching to campaign mods ***");
			clearLoadedMods();

			while (curSearchPath)
			{
				// make sure videos override included files
				sstrcpy(tmpstr, curSearchPath->path);
				sstrcat(tmpstr, "sequences.wz");
				PHYSFS_mount(tmpstr, NULL, PHYSFS_APPEND);
				curSearchPath = curSearchPath->higherPriority;
			}
			curSearchPath = searchPathRegistry;
			while (curSearchPath->lowerPriority)
			{
				curSearchPath = curSearchPath->lowerPriority;
			}
			while (curSearchPath)
			{
#ifdef DEBUG
				debug(LOG_WZ, "Adding [%s] to search path", curSearchPath->path);
#endif // DEBUG
				// Add global and campaign mods
				PHYSFS_mount(curSearchPath->path, NULL, PHYSFS_APPEND);

				addSubdirs(curSearchPath->path, "mods/music", PHYSFS_APPEND, nullptr, false);
				addSubdirs(curSearchPath->path, "mods/global", PHYSFS_APPEND, use_override_mods ? &override_mods : &global_mods, true);
				addSubdirs(curSearchPath->path, "mods", PHYSFS_APPEND, use_override_mods ? &override_mods : &global_mods, true);
				addSubdirs(curSearchPath->path, "mods/autoload", PHYSFS_APPEND, use_override_mods ? &override_mods : nullptr, true);
				addSubdirs(curSearchPath->path, "mods/campaign", PHYSFS_APPEND, use_override_mods ? &override_mods : &campaign_mods, true);
				if (!WZ_PHYSFS_unmount(curSearchPath->path))
				{
					debug(LOG_WZ, "* Failed to remove path %s again", curSearchPath->path);
				}

				// Add plain dir
				PHYSFS_mount(curSearchPath->path, NULL, PHYSFS_APPEND);

				// Add base files
				sstrcpy(tmpstr, curSearchPath->path);
				sstrcat(tmpstr, "base");
				PHYSFS_mount(tmpstr, NULL, PHYSFS_APPEND);
				sstrcpy(tmpstr, curSearchPath->path);
				sstrcat(tmpstr, "base.wz");
				PHYSFS_mount(tmpstr, NULL, PHYSFS_APPEND);

				curSearchPath = curSearchPath->higherPriority;
			}
			break;
		case mod_multiplay:
			debug(LOG_WZ, "*** Switching to multiplay mods ***");
			clearLoadedMods();

			while (curSearchPath)
			{
				// make sure videos override included files
				sstrcpy(tmpstr, curSearchPath->path);
				sstrcat(tmpstr, "sequences.wz");
				PHYSFS_mount(tmpstr, NULL, PHYSFS_APPEND);
				curSearchPath = curSearchPath->higherPriority;
			}
			// Add the selected map first, for mapmod support
			if (current_map != nullptr)
			{
				WzString realPathAndDir = WzString::fromUtf8(PHYSFS_getRealDir(current_map)) + current_map;
				realPathAndDir.replace("/", PHYSFS_getDirSeparator()); // Windows fix
				PHYSFS_mount(realPathAndDir.toUtf8().c_str(), NULL, PHYSFS_APPEND);
			}
			curSearchPath = searchPathRegistry;
			while (curSearchPath->lowerPriority)
			{
				curSearchPath = curSearchPath->lowerPriority;
			}
			while (curSearchPath)
			{
#ifdef DEBUG
				debug(LOG_WZ, "Adding [%s] to search path", curSearchPath->path);
#endif // DEBUG
				// Add global and multiplay mods
				PHYSFS_mount(curSearchPath->path, NULL, PHYSFS_APPEND);
				addSubdirs(curSearchPath->path, "mods/music", PHYSFS_APPEND, nullptr, false);

				// Only load if we are host or singleplayer (Initial mod load relies on this, too)
				if (ingame.side == InGameSide::HOST_OR_SINGLEPLAYER || !NetPlay.bComms)
				{
					addSubdirs(curSearchPath->path, "mods/global", PHYSFS_APPEND, use_override_mods ? &override_mods : &global_mods, true);
					addSubdirs(curSearchPath->path, "mods", PHYSFS_APPEND, use_override_mods ? &override_mods : &global_mods, true);
					addSubdirs(curSearchPath->path, "mods/autoload", PHYSFS_APPEND, use_override_mods ? &override_mods : nullptr, true);
					addSubdirs(curSearchPath->path, "mods/multiplay", PHYSFS_APPEND, use_override_mods ? &override_mods : &multiplay_mods, true);
				}
				else
				{
					std::vector<std::string> hashList;
					for (Sha256 &hash : game.modHashes)
					{
						hashList = {hash.toString()};
						addSubdirs(curSearchPath->path, "mods/downloads", PHYSFS_APPEND, &hashList, true);
					}
				}
				WZ_PHYSFS_unmount(curSearchPath->path);

				// Add multiplay patches
				sstrcpy(tmpstr, curSearchPath->path);
				sstrcat(tmpstr, "mp");
				PHYSFS_mount(tmpstr, NULL, PHYSFS_APPEND);
				sstrcpy(tmpstr, curSearchPath->path);
				sstrcat(tmpstr, "mp.wz");
				PHYSFS_mount(tmpstr, NULL, PHYSFS_APPEND);

				// Add plain dir
				PHYSFS_mount(curSearchPath->path, NULL, PHYSFS_APPEND);

				// Add base files
				sstrcpy(tmpstr, curSearchPath->path);
				sstrcat(tmpstr, "base");
				PHYSFS_mount(tmpstr, NULL, PHYSFS_APPEND);
				sstrcpy(tmpstr, curSearchPath->path);
				sstrcat(tmpstr, "base.wz");
				PHYSFS_mount(tmpstr, NULL, PHYSFS_APPEND);

				curSearchPath = curSearchPath->higherPriority;
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
		PHYSFS_mount(PHYSFS_getWriteDir(), NULL, PHYSFS_PREPEND);

#ifdef DEBUG
		printSearchPath();
#endif // DEBUG
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
			int unsafe = 0;
			WZ_PHYSFS_enumerateFiles("multiplay/maps", [&unsafe, &realFilePathAndName](const char *file) -> bool {
				std::string isDir = std::string("multiplay/maps/") + file;
				if (WZ_PHYSFS_isDirectory(isDir.c_str()))
				{
					return true; // continue;
				}
				std::string checkfile = file;
				debug(LOG_WZ, "checking ... %s", file);
				if (checkfile.substr(checkfile.find_last_of('.') + 1) == "gam")
				{
					if (unsafe++ > 1)
					{
						debug(LOG_ERROR, "Map packs are not supported! %s NOT added.", realFilePathAndName.c_str());
						return false; // break;
					}
				}
				return true; // continue
			});
			if (unsafe < 2)
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
struct WZmapInfo
{
	bool isMapMod;
	bool isRandom;
};

typedef std::string MapName;
typedef std::unordered_map<MapName, WZmapInfo> WZMapInfo_Map;
WZMapInfo_Map WZ_Maps;

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
static std::pair<bool, bool> CheckInMap(const char *archive, const char *mountpoint, const std::vector<const char *>& lookin_list)
{
	bool mapmod = false;
	bool isRandom = false;

	if (!PHYSFS_mount(archive, mountpoint, PHYSFS_APPEND))
	{
		// We already checked to see if this was valid before, and now, something went seriously wrong.
		debug(LOG_FATAL, "Could not mount %s, because: %s. Please delete the file, and run the game again. Game will now exit.", archive, WZ_PHYSFS_getLastError());
		exit(-1);
	}

	for (auto lookin : lookin_list)
	{
		std::string checkpath = lookin;
		checkpath.append("/");
		WZ_PHYSFS_enumerateFiles(lookin, [&](const char *file) -> bool {
			std::string checkfile = file;
			if (WZ_PHYSFS_isDirectory((checkpath + checkfile).c_str()))
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

		std::string maps = checkpath + "/multiplay/maps";
		WZ_PHYSFS_enumerateFiles(maps.c_str(), [&](const char *file) -> bool {
			if (WZ_PHYSFS_isDirectory((maps + "/" + file).c_str()) && PHYSFS_exists((maps + "/" + file + "/game.js").c_str()))
			{
				isRandom = true;
				return false; // break;
			}
			return true; // continue
		});
	}

	if (!WZ_PHYSFS_unmount(archive))
	{
		debug(LOG_ERROR, "Could not unmount %s, %s", archive, WZ_PHYSFS_getLastError());
	}
	return {mapmod, isRandom};
}

bool buildMapList()
{
	if (!loadLevFile("gamedesc.lev", mod_campaign, false, nullptr))
	{
		return false;
	}
	loadLevFile("addon.lev", mod_multiplay, false, nullptr);
	WZ_Maps.clear();
	MapFileList realFileNames = listMapFiles();
	const std::vector<const char *> lookin_list = { "WZMap", "WZMap/multiplay" };
	for (auto &realFileName : realFileNames)
	{
		struct WZmapInfo CurrentMap;
		const char * pRealDirStr = PHYSFS_getRealDir(realFileName.platformIndependent.c_str());
		if (!pRealDirStr)
		{
			debug(LOG_ERROR, "Failed to find realdir for: %s", realFileName.platformIndependent.c_str());
			continue; // skip
		}
		std::string realFilePathAndName = pRealDirStr + realFileName.platformDependent;

		PHYSFS_mount(realFilePathAndName.c_str(), NULL, PHYSFS_APPEND);

		WZ_PHYSFS_enumerateFiles("", [&](const char *file) -> bool {
			size_t len = strlen(file);
			if (len > 10 && !strcasecmp(file + (len - 10), ".addon.lev"))  // Do not add addon.lev again
			{
				loadLevFile(file, mod_multiplay, true, realFileName.platformIndependent.c_str());
			}
			// add support for X player maps using a new name to prevent conflicts.
			if (len > 13 && !strcasecmp(file + (len - 13), ".xplayers.lev"))
			{
				loadLevFile(file, mod_multiplay, true, realFileName.platformIndependent.c_str());
			}
			return true; // continue
		});

		if (WZ_PHYSFS_unmount(realFilePathAndName.c_str()) == 0)
		{
			debug(LOG_ERROR, "Could not unmount %s, %s", realFilePathAndName.c_str(), WZ_PHYSFS_getLastError());
		}

		auto chk = CheckInMap(realFilePathAndName.c_str(), "WZMap", lookin_list);

		const std::string& MapName = realFileName.platformIndependent;
		CurrentMap.isMapMod = chk.first;
		CurrentMap.isRandom = chk.second;
		WZ_Maps.insert(WZMapInfo_Map::value_type(MapName, std::move(CurrentMap)));
	}

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Called once on program startup.
//
bool systemInitialise(float horizScaleFactor, float vertScaleFactor)
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
	iV_TextInit(horizScaleFactor, vertScaleFactor);

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
	pie_ShutdownRadar();
	clearLoadedMods();
	flushConsoleMessages();

	shutdownEffectsSystem();
	wzSceneEnd(nullptr);  // Might want to end the "Main menu loop" or "Main game loop".
	keyMappings.clear();

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
	/* Shift the interface initialisation here temporarily so that it
		can pick up the stats after they have been loaded */
	if (!intInitialise())
	{
		return false;
	}

	// reinitialise key mappings
	keyInitMappings(false);

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

	interfaceShutDown();

	//do this before shutting down the iV library
	resReleaseAllData();

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
	keyInitMappings(false);

	// Set the default uncoloured cursor here, since it looks slightly
	// better for menus and such.
	wzSetCursor(CURSOR_DEFAULT);

	SetFormAudioIDs(ID_SOUND_WINDOWOPEN, ID_SOUND_WINDOWCLOSE);

	// Setup game queues.
	// Don't ask why this doesn't go in stage three. In fact, don't even ask me what stage one/two/three is supposed to mean, it seems about as descriptive as stage doStuff, stage doMoreStuff and stage doEvenMoreStuff...
	debug(LOG_MAIN, "Init game queues, I am %d.", selectedPlayer);
	sendQueuedDroidInfo();  // Discard any pending orders which could later get flushed into the game queue.
	for (i = 0; i < MAX_PLAYERS; ++i)
	{
		NETinitQueue(NETgameQueue(i));

		if (!myResponsibility(i))
		{
			NETsetNoSendOverNetwork(NETgameQueue(i));
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

	shutdown3DView();

	return true;
}

bool stageThreeInitialise()
{
	STRUCTURE *psStr;
	UDWORD i;
	DROID *psDroid;
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

	/* decide if we have to create teams, ONLY in multiplayer mode!*/
	if (bMultiPlayer && alliancesSharedVision(game.alliance))
	{
		createTeamAlliances();

		/* Update ally vision for pre-placed structures and droids */
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (i != selectedPlayer)
			{
				/* Structures */
				for (psStr = apsStructLists[i]; psStr; psStr = psStr->psNext)
				{
					if (aiCheckAlliances(psStr->player, selectedPlayer))
					{
						visTilesUpdate((BASE_OBJECT *)psStr);
					}
				}

				/* Droids */
				for (psDroid = apsDroidLists[i]; psDroid; psDroid = psDroid->psNext)
				{
					if (aiCheckAlliances(psDroid->player, selectedPlayer))
					{
						visTilesUpdate((BASE_OBJECT *)psDroid);
					}
				}
			}
		}
	}

	countUpdate();

	if (getLevelLoadType() == GTYPE_SAVE_MIDMISSION || getLevelLoadType() == GTYPE_SAVE_START)
	{
		triggerEvent(TRIGGER_GAME_LOADED);
	}
	else
	{
		if (getDebugMappingStatus())
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

	for (unsigned n = 0; n < MAX_PLAYERS; ++n)
	{
		processDebugMappings(n, false);
	}
}
