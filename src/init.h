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
/** @file
 *  Interface to the initialisation routines.
 */

#ifndef __INCLUDED_SRC_INIT_H__
#define __INCLUDED_SRC_INIT_H__

#include <vector>

struct IMAGEFILE;

// the size of the file loading buffer
// FIXME Totally inappropriate place for this.
#define FILE_LOAD_BUFFER_SIZE (1024*1024*4)
extern char fileLoadBuffer[];

bool systemInitialise(unsigned int horizScalePercentage, unsigned int vertScalePercentage);
void systemShutdown();
bool frontendInitialise(const char *ResourceFile);
bool frontendShutdown();
bool stageOneInitialise();
bool stageOneShutDown();
bool stageTwoInitialise();
bool stageTwoShutDown();
bool stageThreeInitialise();
bool stageThreeShutDown();

// Reset the game between campaigns
bool campaignReset();
// Reset the game when loading a save game
bool saveGameReset();

struct wzSearchPath
{
	std::string path;
	unsigned int priority = 0;
};

enum searchPathMode { mod_clean, mod_campaign, mod_multiplay, mod_override };

void registerSearchPath(const std::string& path, unsigned int priority);
void unregisterSearchPath(const std::string& path);
void debugOutputSearchPaths();
void debugOutputSearchPathMountErrors();
bool rebuildSearchPath(searchPathMode mode, bool force, const char *current_map = NULL, const char* current_map_mount_point = NULL);

bool buildMapList();
bool CheckForMod(char const *mapFile);
bool CheckForRandom(char const *mapFile, char const *mapDataFile0);
bool setSpecialInMemoryMap(std::vector<uint8_t>&& mapArchiveData);

bool loadLevFile(const std::string& filename, searchPathMode datadir, bool ignoreWrf, char const *realFileName);

extern IMAGEFILE	*FrontImages;

#endif // __INCLUDED_SRC_INIT_H__
