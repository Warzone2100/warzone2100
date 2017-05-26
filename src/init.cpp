/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2017  Warzone 2100 Project

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
#include "lib/script/script.h"
#include "lib/sound/audio_id.h"
#include "lib/sound/cdaudio.h"
#include "lib/sound/mixer.h"

#include "init.h"

#include "advvis.h"
#include "atmos.h"
#include "challenge.h"
#include "cluster.h"
#include "cmddroid.h"
#include "configuration.h"
#include "console.h"
#include "data.h"
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
#include "projectile.h"
#include "order.h"
#include "radar.h"
#include "research.h"
#include "lib/framework/cursors.h"
#include "scriptextern.h"
#include "scriptfuncs.h"
#include "scripttabs.h"
#include "scriptvals.h"
#include "text.h"
#include "transporter.h"
#include "warzoneconfig.h"
#include "main.h"
#include "wrappers.h"
#include "terrain.h"
#include "ingameop.h"
#include "qtscript.h"
#include "template.h"

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


bool loadLevFile(const char *filename, searchPathMode datadir, bool ignoreWrf, char const *realFileName)
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
	if (!levParse(pBuffer, size, datadir, ignoreWrf, realFileName))
	{
		debug(LOG_ERROR, "Parse error in %s\n", filename);
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
				PHYSFS_removeFromSearchPath(tmpstr);
				sstrcpy(tmpstr, curSearchPath->path);
				sstrcat(tmpstr, "mp.wz");
				PHYSFS_removeFromSearchPath(tmpstr);

				// Remove plain dir
				PHYSFS_removeFromSearchPath(curSearchPath->path);

				// Remove base files
				sstrcpy(tmpstr, curSearchPath->path);
				sstrcat(tmpstr, "base");
				PHYSFS_removeFromSearchPath(tmpstr);
				sstrcpy(tmpstr, curSearchPath->path);
				sstrcat(tmpstr, "base.wz");
				PHYSFS_removeFromSearchPath(tmpstr);

				// remove video search path as well
				sstrcpy(tmpstr, curSearchPath->path);
				sstrcat(tmpstr, "sequences.wz");
				PHYSFS_removeFromSearchPath(tmpstr);
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
				PHYSFS_addToSearchPath(tmpstr, PHYSFS_APPEND);
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
				PHYSFS_addToSearchPath(curSearchPath->path, PHYSFS_APPEND);

				addSubdirs(curSearchPath->path, "mods/music", PHYSFS_APPEND, nullptr, false);
				addSubdirs(curSearchPath->path, "mods/global", PHYSFS_APPEND, use_override_mods ? &override_mods : &global_mods, true);
				addSubdirs(curSearchPath->path, "mods", PHYSFS_APPEND, use_override_mods ? &override_mods : &global_mods, true);
				addSubdirs(curSearchPath->path, "mods/autoload", PHYSFS_APPEND, use_override_mods ? &override_mods : nullptr, true);
				addSubdirs(curSearchPath->path, "mods/campaign", PHYSFS_APPEND, use_override_mods ? &override_mods : &campaign_mods, true);
				if (!PHYSFS_removeFromSearchPath(curSearchPath->path))
				{
					debug(LOG_WZ, "* Failed to remove path %s again", curSearchPath->path);
				}

				// Add plain dir
				PHYSFS_addToSearchPath(curSearchPath->path, PHYSFS_APPEND);

				// Add base files
				sstrcpy(tmpstr, curSearchPath->path);
				sstrcat(tmpstr, "base");
				PHYSFS_addToSearchPath(tmpstr, PHYSFS_APPEND);
				sstrcpy(tmpstr, curSearchPath->path);
				sstrcat(tmpstr, "base.wz");
				PHYSFS_addToSearchPath(tmpstr, PHYSFS_APPEND);

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
				PHYSFS_addToSearchPath(tmpstr, PHYSFS_APPEND);
				curSearchPath = curSearchPath->higherPriority;
			}
			// Add the selected map first, for mapmod support
			if (current_map != nullptr)
			{
				QString realPathAndDir = QString::fromUtf8(PHYSFS_getRealDir(current_map)) + current_map;
				realPathAndDir.replace('/', PHYSFS_getDirSeparator()); // Windows fix
				PHYSFS_addToSearchPath(realPathAndDir.toUtf8().constData(), PHYSFS_APPEND);
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
				PHYSFS_addToSearchPath(curSearchPath->path, PHYSFS_APPEND);
				addSubdirs(curSearchPath->path, "mods/music", PHYSFS_APPEND, nullptr, false);
				if (NetPlay.isHost || !NetPlay.bComms)
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
				PHYSFS_removeFromSearchPath(curSearchPath->path);

				// Add multiplay patches
				sstrcpy(tmpstr, curSearchPath->path);
				sstrcat(tmpstr, "mp");
				PHYSFS_addToSearchPath(tmpstr, PHYSFS_APPEND);
				sstrcpy(tmpstr, curSearchPath->path);
				sstrcat(tmpstr, "mp.wz");
				PHYSFS_addToSearchPath(tmpstr, PHYSFS_APPEND);

				// Add plain dir
				PHYSFS_addToSearchPath(curSearchPath->path, PHYSFS_APPEND);

				// Add base files
				sstrcpy(tmpstr, curSearchPath->path);
				sstrcat(tmpstr, "base");
				PHYSFS_addToSearchPath(tmpstr, PHYSFS_APPEND);
				sstrcpy(tmpstr, curSearchPath->path);
				sstrcat(tmpstr, "base.wz");
				PHYSFS_addToSearchPath(tmpstr, PHYSFS_APPEND);

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
		PHYSFS_removeFromSearchPath(PHYSFS_getWriteDir());
		PHYSFS_addToSearchPath(PHYSFS_getWriteDir(), PHYSFS_PREPEND);

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

typedef std::vector<std::string> MapFileList;
static MapFileList listMapFiles()
{
	MapFileList ret, filtered, oldSearchPath;

	char **subdirlist = PHYSFS_enumerateFiles("maps");

	for (char **i = subdirlist; *i != nullptr; ++i)
	{
		std::string wzfile = *i;
		if (*i[0] == '.' || wzfile.substr(wzfile.find_last_of('.') + 1) != "wz")
		{
			continue;
		}

		std::string realFileName = std::string("maps/") + *i;
		ret.push_back(realFileName);
	}
	PHYSFS_freeList(subdirlist);
	// save our current search path(s)
	debug(LOG_WZ, "Map search paths:");
	char **searchPath = PHYSFS_getSearchPath();
	for (char **i = searchPath; *i != nullptr; i++)
	{
		debug(LOG_WZ, "    [%s]", *i);
		oldSearchPath.push_back(*i);
		PHYSFS_removeFromSearchPath(*i);
	}
	PHYSFS_freeList(searchPath);

	for (const auto &realFileName : ret)
	{
		std::string realFilePathAndName = PHYSFS_getWriteDir() + realFileName;
		if (PHYSFS_addToSearchPath(realFilePathAndName.c_str(), PHYSFS_APPEND))
		{
			int unsafe = 0;
			char **filelist = PHYSFS_enumerateFiles("multiplay/maps");

			for (char **file = filelist; *file != nullptr; ++file)
			{
				std::string isDir = std::string("multiplay/maps/") + *file;
				if (PHYSFS_isDirectory(isDir.c_str()))
				{
					continue;
				}
				std::string checkfile = *file;
				debug(LOG_WZ, "checking ... %s", *file);
				if (checkfile.substr(checkfile.find_last_of('.') + 1) == "gam")
				{
					if (unsafe++ > 1)
					{
						debug(LOG_ERROR, "Map packs are not supported! %s NOT added.", realFilePathAndName.c_str());
						break;
					}
				}
			}
			PHYSFS_freeList(filelist);
			if (unsafe < 2)
			{
				filtered.push_back(realFileName);
			}
			PHYSFS_removeFromSearchPath(realFilePathAndName.c_str());
		}
		else
		{
			debug(LOG_POPUP, "Could not mount %s, because: %s.\nPlease delete or move the file specified.", realFilePathAndName.c_str(), PHYSFS_getLastError());
		}
	}

	// restore our search path(s) again
	for (const auto &restorePaths : oldSearchPath)
	{
		PHYSFS_addToSearchPath(restorePaths.c_str(), PHYSFS_APPEND);
	}
	debug(LOG_WZ, "Search paths restored");
	printSearchPath();

	return filtered;
}

// Map processing
struct WZmaps
{
	std::string MapName;
	bool isMapMod;
};

std::vector<struct WZmaps> WZ_Maps;

struct FindMap
{
	const std::string map;
	FindMap(const std::string &name) : map(name) {};
	bool operator()(const WZmaps &wzm) const
	{
		return wzm.MapName == map;
	};
};

bool CheckForMod(char *theMap)
{
	std::vector<struct WZmaps>::iterator it;
	if (theMap == nullptr)
	{
		return false;
	}
	it = std::find_if(WZ_Maps.begin(), WZ_Maps.end(), FindMap(theMap));
	if (it != WZ_Maps.end())
	{
		return it->isMapMod;
	}
	debug(LOG_ERROR, "Couldn't find map %s", theMap);

	return false;
}

// Mount the archive under the mountpoint, and enumerate the archive according to lookin
static bool CheckInMap(const char *archive, const char *mountpoint, const char *lookin)
{
	bool mapmod = false;

	if (!PHYSFS_mount(archive, mountpoint, PHYSFS_APPEND))
	{
		// We already checked to see if this was valid before, and now, something went seriously wrong.
		debug(LOG_FATAL, "Could not mount %s, because: %s. Please delete the file, and run the game again. Game will now exit.", archive, PHYSFS_getLastError());
		exit(-1);
	}

	std::string checkpath = lookin;
	checkpath.append("/");
	char **filelist = PHYSFS_enumerateFiles(lookin);
	for (char **file = filelist; *file != nullptr; ++file)
	{
		std::string checkfile = *file;
		if (PHYSFS_isDirectory((checkpath + checkfile).c_str()))
		{
			if (checkfile.compare("wrf") == 0 || checkfile.compare("stats") == 0 || checkfile.compare("components") == 0
			    || checkfile.compare("effects") == 0 || checkfile.compare("messages") == 0
			    || checkfile.compare("audio") == 0 || checkfile.compare("sequenceaudio") == 0 || checkfile.compare("misc") == 0
			    || checkfile.compare("features") == 0 || checkfile.compare("script") == 0 || checkfile.compare("structs") == 0
			    || checkfile.compare("tileset") == 0 || checkfile.compare("images") == 0 || checkfile.compare("texpages") == 0
			    || checkfile.compare("skirmish") == 0)
			{
				debug(LOG_WZ, "Detected: %s %s" , archive, checkfile.c_str());
				mapmod = true;
				break;
			}
		}
	}
	PHYSFS_freeList(filelist);

	if (!PHYSFS_removeFromSearchPath(archive))
	{
		debug(LOG_ERROR, "Could not unmount %s, %s", archive, PHYSFS_getLastError());
	}
	return mapmod;
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
	for (auto &realFileName : realFileNames)
	{
		bool mapmod = false;
		struct WZmaps CurrentMap;
		std::string realFilePathAndName = PHYSFS_getRealDir(realFileName.c_str()) + realFileName;

		PHYSFS_addToSearchPath(realFilePathAndName.c_str(), PHYSFS_APPEND);

		char **filelist = PHYSFS_enumerateFiles("");
		for (char **file = filelist; *file != nullptr; ++file)
		{
			std::string checkfile = *file;
			size_t len = strlen(*file);
			if (len > 10 && !strcasecmp(*file + (len - 10), ".addon.lev"))  // Do not add addon.lev again
			{
				loadLevFile(*file, mod_multiplay, true, realFileName.c_str());
			}
			// add support for X player maps using a new name to prevent conflicts.
			if (len > 13 && !strcasecmp(*file + (len - 13), ".xplayers.lev"))
			{
				loadLevFile(*file, mod_multiplay, true, realFileName.c_str());
			}
		}
		PHYSFS_freeList(filelist);

		if (PHYSFS_removeFromSearchPath(realFilePathAndName.c_str()) == 0)
		{
			debug(LOG_ERROR, "Could not unmount %s, %s", realFilePathAndName.c_str(), PHYSFS_getLastError());
		}

		mapmod = CheckInMap(realFilePathAndName.c_str(), "WZMap", "WZMap");
		if (!mapmod)
		{
			mapmod = CheckInMap(realFilePathAndName.c_str(), "WZMap", "WZMap/multiplay");
		}

		CurrentMap.MapName = realFileName;
		CurrentMap.isMapMod = mapmod;
		WZ_Maps.push_back(CurrentMap);
	}

	return true;
}

// ////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////////////////////////
// Called once on program startup.
//
bool systemInitialise()
{
	if (!widgInitialise())
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

	if (!audio_Init(droidAudioTrackStopped, war_getSoundEnabled()))
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

	if (!fpathInitialise())
	{
		return false;
	}

	// Initialize the iVis text rendering module
	wzSceneBegin("Main menu loop");
	iV_TextInit();

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

	shutdownEffectsSystem();
	wzSceneEnd(nullptr);  // Might want to end the "Main menu loop" or "Main game loop".
	keyClearMappings();

	// free up all the load functions (all the data should already have been freed)
	resReleaseAll();

	if (!multiShutdown()) // ajl. init net stuff
	{
		debug(LOG_FATAL, "Unable to multiShutdown() cleanly!");
		abort();
	}

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
	widgShutDown();
	fpathShutdown();
	mapShutdown();
	debug(LOG_MAIN, "shutting down everything else");
	pal_ShutDown();		// currently unused stub
	frameShutDown();	// close screen / SDL / resources / cursors / trig
	screenShutDown();
	closeConfig();		// "registry" close
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

	if (!scrTabInitialise())				// Initialise the script system
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

	// keymappings
	// clear out any existing mappings
	keyClearMappings();
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
	debug(LOG_WZ, "== Shuting down frontend ==");

	saveConfig();// save settings to registry.

	if (!mechanicsShutdown())
	{
		return false;
	}

	interfaceShutDown();
	scrShutDown();

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
	debug(LOG_WZ, "== stageOneInitalise ==");
	wzSceneEnd("Main menu loop");
	wzSceneBegin("Main game loop");

	// Initialise all globals and statics everwhere.
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

	if (!scrTabInitialise())	// Initialise the old wzscript system
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

	// do this here so that the very first mission has it initialised
	initRunData();

	gameTimeInit();
	eventTimeReset(gameTime / SCR_TICKRATE);

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

	scrShutDown();
	gridShutDown();

	debug(LOG_TEXTURE, "=== stageOneShutDown ===");
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

	debug(LOG_WZ, "== stageTwoInitalise ==");

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

	// keymappings
	keyClearMappings();
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
	STRUCTURE *psStr;
	UDWORD i;
	DROID		*psDroid;

	debug(LOG_WZ, "== stageThreeInitalise ==");

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

	if (bMultiPlayer)
	{
		multiGameInit();
		cmdDroidMultiExpBoost(true);
		initTemplates();
	}

	preProcessVisibility();

	prepareScripts(getLevelLoadType() == GTYPE_SAVE_MIDMISSION || getLevelLoadType() == GTYPE_SAVE_START);

	if (!fpathInitialise())
	{
		return false;
	}

	mapInit();
	clustInitialise();
	gridReset();

	//if mission screen is up, close it.
	if (MissionResUp)
	{
		intRemoveMissionResultNoAnim();
	}

	// Re-inititialise some static variables.

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

	if (getLevelLoadType() != GTYPE_SAVE_MIDMISSION)
	{
		if (getDebugMappingStatus())
		{
			triggerEventCheatMode(true);
		}
		eventFireCallbackTrigger((TRIGGER_TYPE)CALL_GAMEINIT);
		triggerEvent(TRIGGER_GAME_INIT);
	}

	return true;
}

/*****************************************************************************/
/*      Shutdown before any data is released                                 */

bool stageThreeShutDown()
{
	debug(LOG_WZ, "== stageThreeShutDown ==");

	hostlaunch = 0;

	removeSpotters();

	// There is an assymetry in scripts initialization and destruction, due
	// the many different ways scripts get loaded.
	if (!shutdownScripts())
	{
		return false;
	}

	challengesUp = false;
	challengeActive = false;
	isInGamePopupUp = false;

	shutdownTemplates();

	// make sure any button tips are gone.
	widgReset();

	audio_StopAll();

	if (bMultiPlayer)
	{
		multiGameShutdown();
	}

	cmdDroidMultiExpBoost(false);

	eventReset();

	// reset the script values system
	scrvReset();

	//call this here before mission data is released
	if (!missionShutDown())
	{
		return false;
	}

	scrExternReset();

	// reset the run data so that doesn't need to be initialised in the scripts
	initRunData();

	resetVTOLLandingPos();

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
	intResetPreviousObj();

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
