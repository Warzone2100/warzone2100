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
/** @file
 *  Interface to the initialisation routines.
 */

#ifndef __INCLUDED_SRC_INIT_H__
#define __INCLUDED_SRC_INIT_H__

struct IMAGEFILE;

// the size of the file loading buffer
// FIXME Totally inappropriate place for this.
#define FILE_LOAD_BUFFER_SIZE (1024*1024*4)
extern char fileLoadBuffer[];

extern bool systemInitialise(void);
extern void systemShutdown(void);
extern bool frontendInitialise(const char *ResourceFile);
extern bool frontendShutdown(void);
extern bool stageOneInitialise(void);
extern bool stageOneShutDown(void);
extern bool stageTwoInitialise(void);
extern bool stageTwoShutDown(void);
extern bool stageThreeInitialise(void);
extern bool stageThreeShutDown(void);

// Reset the game between campaigns
extern bool campaignReset(void);
// Reset the game when loading a save game
extern bool saveGameReset(void);

struct wzSearchPath
{
	char path[PATH_MAX];
	unsigned int priority;
	wzSearchPath *higherPriority, * lowerPriority;
};

enum searchPathMode { mod_clean, mod_campaign, mod_multiplay, mod_override };

void registerSearchPath(const char path[], unsigned int priority);
bool rebuildSearchPath(searchPathMode mode, bool force, const char *current_map = NULL);

bool buildMapList(void);
bool CheckForMod(char *theMap);

bool loadLevFile(const char *filename, searchPathMode datadir, bool ignoreWrf, char const *realFileName);

extern IMAGEFILE	*FrontImages;

#endif // __INCLUDED_SRC_INIT_H__
