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
#include "lib/framework/frame.h"

#include "lib/exceptionhandler/dumpinfo.h"
#include "lib/framework/file.h"
#include "lib/framework/physfs_ext.h"
#include "lib/netplay/netplay.h"

#include "modding.h"

#include <string>
#include <vector>

namespace {
	struct LoadedMod
	{
		std::string name;
		std::string filename;
	};
}

std::vector<std::string> global_mods;
std::vector<std::string> campaign_mods;
std::vector<std::string> multiplay_mods;

std::vector<std::string> override_mods;
std::string override_mod_list;
bool use_override_mods = false;

static std::vector<LoadedMod> loaded_mods;
static std::string mod_list;
static std::vector<Sha256> mod_hash_list;


static void addLoadedMod(std::string modname, std::string filename);


static inline std::vector<std::string> split(std::string const &str, std::string const &sep)
{
	std::vector<std::string> strs;
	if (str.empty())
	{
		return strs;
	}
	std::string::size_type begin = 0;
	std::string::size_type end = str.find(sep);
	while (end != std::string::npos)
	{
		strs.push_back(str.substr(begin, end - begin));
		begin = end + sep.size();
		end = str.find(sep, begin);
	}
	strs.push_back(str.substr(begin));
	return strs;
}

static inline std::string join(std::vector<std::string> const &strs, std::string const &sep)
{
	std::string str;
	bool first = true;
	for (auto const &s : strs)
	{
		if (!first)
		{
			str += sep;
		}
		str += s;
		first = false;
	}
	return str;
}


/*!
 * Tries to mount a list of directories, found in /basedir/subdir/<list>.
 * \param basedir Base directory
 * \param subdir A subdirectory of basedir
 * \param appendToPath Whether to append or prepend
 * \param checkList List of directories to check. NULL means any.
 */
void addSubdirs(const char *basedir, const char *subdir, const bool appendToPath, std::vector<std::string> const *checkList, bool addToModList)
{
	char **subdirlist = PHYSFS_enumerateFiles(subdir);
	char **i = subdirlist;
	while (*i != nullptr)
	{
#ifdef DEBUG
		debug(LOG_NEVER, "Examining subdir: [%s]", *i);
#endif // DEBUG
		if (*i[0] != '.' && (!checkList || std::find(checkList->begin(), checkList->end(), *i) != checkList->end()))
		{
			char tmpstr[PATH_MAX];
			snprintf(tmpstr, sizeof(tmpstr), "%s%s%s%s", basedir, subdir, PHYSFS_getDirSeparator(), *i);
#ifdef DEBUG
			debug(LOG_NEVER, "Adding [%s] to search path", tmpstr);
#endif // DEBUG
			if (addToModList)
			{
				std::string filename = astringf("%s%s%s", subdir, PHYSFS_getDirSeparator(), *i);
				addLoadedMod(*i, std::move(filename));
				char buf[256];
				snprintf(buf, sizeof(buf), "mod: %s", *i);
				addDumpInfo(buf);
			}
			PHYSFS_mount(tmpstr, NULL, appendToPath);
		}
		i++;
	}
	PHYSFS_freeList(subdirlist);
}

void removeSubdirs(const char *basedir, const char *subdir)
{
	char tmpstr[PATH_MAX];
	char **subdirlist = PHYSFS_enumerateFiles(subdir);
	char **i = subdirlist;
	while (*i != nullptr)
	{
#ifdef DEBUG
		debug(LOG_NEVER, "Examining subdir: [%s]", *i);
#endif // DEBUG
		snprintf(tmpstr, sizeof(tmpstr), "%s%s%s%s", basedir, subdir, PHYSFS_getDirSeparator(), *i);
#ifdef DEBUG
		debug(LOG_NEVER, "Removing [%s] from search path", tmpstr);
#endif // DEBUG
		if (!PHYSFS_removeFromSearchPath(tmpstr))
		{
#ifdef DEBUG	// spams a ton!
			debug(LOG_NEVER, "Couldn't remove %s from search path because %s", tmpstr, PHYSFS_getLastError());
#endif // DEBUG
		}
		i++;
	}
	PHYSFS_freeList(subdirlist);
}

void printSearchPath()
{
	debug(LOG_WZ, "Search paths:");
	char **searchPath = PHYSFS_getSearchPath();
	for (char **i = searchPath; *i != nullptr; i++)
	{
		debug(LOG_WZ, "    [%s]", *i);
	}
	PHYSFS_freeList(searchPath);
}

void setOverrideMods(char *modlist)
{
	override_mods = split(modlist, ", ");
	override_mod_list = modlist;
	use_override_mods = true;
}

void clearOverrideMods()
{
	override_mods.clear();
	override_mod_list.clear();
	use_override_mods = false;
}

static void addLoadedMod(std::string modname, std::string filename)
{
	// Note, findHashOfFile won't work right now, since the search paths aren't set up until after all calls to addSubdirs, see rebuildSearchPath in init.cpp.
	loaded_mods.push_back({std::move(modname), std::move(filename)});
}

void clearLoadedMods()
{
	loaded_mods.clear();
	mod_list.clear();
}

std::string const &getModList()
{
	if (mod_list.empty())
	{
		// Construct mod list.
		std::vector<std::string> mods;
		for (auto const &mod : loaded_mods)
		{
			mods.push_back(mod.name);
		}
		std::sort(mods.begin(), mods.end());
		mods.erase(std::unique(mods.begin(), mods.end()), mods.end());
		mod_list = join(mods, ", ");
		mod_list.resize(std::min<size_t>(mod_list.size(), modlist_string_size - 1));  // Probably not needed.
	}
	return mod_list;
}

std::vector<Sha256> const &getModHashList()
{
	if (mod_hash_list.empty())
	{
		for (auto const &mod : loaded_mods)
		{
			Sha256 hash = findHashOfFile(mod.filename.c_str());
			debug(LOG_WZ, "Mod[%s]: %s\n", hash.toString().c_str(), mod.filename.c_str());
			mod_hash_list.push_back(hash);
		}
	}
	return mod_hash_list;
}

std::string getModFilename(Sha256 const &hash)
{
	for (auto const &mod : loaded_mods)
	{
		Sha256 foundHash = findHashOfFile(mod.filename.c_str());
		if (foundHash == hash)
		{
			return mod.filename;
		}
	}
	return {};
}
