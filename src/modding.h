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

#ifndef __INCLUDED_SRC_MODDING_H__
#define __INCLUDED_SRC_MODDING_H__

#include "lib/framework/crc.h"

#include <string>
#include <vector>


void addSubdirs(const char *basedir, const char *subdir, const bool appendToPath, std::vector<std::string> const *checkList, bool addToModList);
void removeSubdirs(const char *basedir, const char *subdir);
void printSearchPath();

void setOverrideMods(char *modlist);
void clearOverrideMods();

void clearLoadedMods();
std::string const &getModList();
std::vector<Sha256> const &getModHashList();
std::string getModFilename(Sha256 const &hash);

extern std::vector<std::string> global_mods;
extern std::vector<std::string> campaign_mods;
extern std::vector<std::string> multiplay_mods;

extern std::vector<std::string> override_mods;
extern std::string override_mod_list;
extern bool use_override_mods;

#endif // __INCLUDED_SRC_MODDING_H__
