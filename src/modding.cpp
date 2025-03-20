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
#include "lib/framework/frame.h"

#include "lib/exceptionhandler/dumpinfo.h"
#include "lib/framework/file.h"
#include "lib/framework/physfs_ext.h"
#include "lib/netplay/netplay.h"

#include "modding.h"

#include <string>
#include <vector>
#include <algorithm>

std::vector<std::string> global_mods;
std::vector<std::string> campaign_mods;
std::vector<std::string> multiplay_mods;

std::vector<std::string> override_mods;
std::string override_mod_list;
bool use_override_mods = false;

static std::vector<WzMods::LoadedMod> loaded_mods;
static std::string mod_list;
static std::vector<std::string> mod_names_list;
static std::vector<Sha256> mod_hash_list;


static void addLoadedMod(std::string modname, std::string filename, const std::string& fullRealPath);
static bool hasLoadedModRealPath(const std::string& fullRealPath);


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

WzString convertToPlatformDependentPath(const char *platformIndependentPath)
{
	WzString path(platformIndependentPath);

	// platform-independent notation uses "/" as the path separator
	if (WzString(PHYSFS_getDirSeparator()) != WzString("/"))
	{
		// replace all "/" with PHYSFS_getDirSeparator()
		path.replace("/", PHYSFS_getDirSeparator());
	}

	return path;
}

/*!
 * Tries to mount a list of directories, found in /basedir/subdir/<list>.
 * \param basedir Base directory (in platform-dependent notation)
 * \param subdir A subdirectory of basedir (in platform-independent notation - i.e. with "/" as the path separator)
 * \param appendToPath Whether to append or prepend
 * \param checkList List of directories to check. NULL means any.
 */
size_t addSubdirs(const char *basedir, const char *subdir, const bool appendToPath, std::vector<std::string> const *checkList, bool addToModList)
{
	size_t numAddedMods = 0;
	const WzString subdir_platformDependent = convertToPlatformDependentPath(subdir);
	std::string tmpFullModRealPath;
	WZ_PHYSFS_enumerateFiles(subdir, [&](const char *i) -> bool {
#ifdef DEBUG
		debug(LOG_NEVER, "Examining subdir: [%s]", i);
#endif // DEBUG
		if (i[0] != '.' && (!checkList || std::find(checkList->begin(), checkList->end(), i) != checkList->end()))
		{
			// platform-dependent notation
			tmpFullModRealPath = basedir;
			tmpFullModRealPath += subdir_platformDependent.toUtf8();
			tmpFullModRealPath += PHYSFS_getDirSeparator();
			tmpFullModRealPath += i;
#ifdef DEBUG
			debug(LOG_NEVER, "Adding [%s] to search path", tmpFullModRealPath.c_str());
#endif // DEBUG
			if (hasLoadedModRealPath(tmpFullModRealPath))
			{
				debug(LOG_INFO, "Already loaded: %s, skipping", tmpFullModRealPath.c_str());
				return true; // continue
			}
			if (PHYSFS_mount(tmpFullModRealPath.c_str(), NULL, appendToPath) != 0) // platform-dependent notation
			{
				numAddedMods++;
				if (addToModList)
				{
					std::string filename = astringf("%s/%s", subdir, i); // platform-independent notation
					addLoadedMod(i, std::move(filename), tmpFullModRealPath);
					char buf[256];
					snprintf(buf, sizeof(buf), "mod: %s", i);
					addDumpInfo(buf);
				}
			}
			else
			{
				// failed to mount mod
				code_part log_level = LOG_WZ;
				auto errorCode = PHYSFS_getLastErrorCode();
				switch (errorCode)
				{
				case PHYSFS_ERR_CORRUPT:
					log_level = LOG_ERROR;
					break;
				case PHYSFS_ERR_NOT_FOUND:
				default:
					log_level = LOG_WZ;
					break;
				}
				const char* pErrStr = PHYSFS_getErrorByCode(errorCode);
				if (!pErrStr)
				{
					pErrStr = "<unknown error>";
				}
				debug(log_level, "Failed to load mod from path: %s, %s", tmpFullModRealPath.c_str(), pErrStr);

			}
		}
		return true; // continue
	});
	return numAddedMods;
}

void removeSubdirs(const char *basedir, const char *subdir)
{
	ASSERT(basedir, "basedir is null");
	ASSERT(subdir, "subdir is null");
	const WzString subdir_platformDependent = convertToPlatformDependentPath(subdir);
	char tmpstr[PATH_MAX];
	WZ_PHYSFS_enumerateFiles(subdir, [&](const char *i) -> bool {
#ifdef DEBUG
		debug(LOG_NEVER, "Examining subdir: [%s]", i);
#endif // DEBUG
		snprintf(tmpstr, sizeof(tmpstr), "%s%s%s%s", basedir, subdir_platformDependent.toUtf8().c_str(), PHYSFS_getDirSeparator(), i);
#ifdef DEBUG
		debug(LOG_NEVER, "Removing [%s] from search path", tmpstr);
#endif // DEBUG
		if (!WZ_PHYSFS_unmount(tmpstr)) // platform-dependent notation
		{
#ifdef DEBUG	// spams a ton!
			debug(LOG_NEVER, "Couldn't remove %s from search path because %s", tmpstr, WZ_PHYSFS_getLastError());
#endif // DEBUG
		}
		return true; // continue
	});
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

bool hasOverrideMods()
{
	return !override_mods.empty();
}

void clearCampaignMods()
{
	campaign_mods.clear();
}

bool hasCampaignMods()
{
	return !campaign_mods.empty();
}

static void addLoadedMod(std::string modname, std::string filename, const std::string& fullRealPath)
{
	// Note, findHashOfFile won't work right now, since the search paths aren't set up until after all calls to addSubdirs, see rebuildSearchPath in init.cpp.
	loaded_mods.emplace_back(std::move(modname), std::move(filename), fullRealPath);
	mod_list.clear();
	mod_names_list.clear();
	mod_hash_list.clear();
}

static bool hasLoadedModRealPath(const std::string& fullRealPath)
{
	return std::any_of(loaded_mods.begin(), loaded_mods.end(), [fullRealPath](const WzMods::LoadedMod& loadedMod) -> bool {
		return loadedMod.fullRealPath == fullRealPath;
	});
}

void clearLoadedMods()
{
	loaded_mods.clear();
	mod_list.clear();
	mod_names_list.clear();
	mod_hash_list.clear();
}

std::vector<WzMods::LoadedMod> const &getLoadedMods()
{
	return loaded_mods;
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

std::vector<std::string> const &getModNamesList()
{
	if (mod_names_list.empty())
	{
		for (auto const &mod : loaded_mods)
		{
			mod_names_list.push_back(mod.name);
		}
		std::sort(mod_names_list.begin(), mod_names_list.end());
		mod_names_list.erase(std::unique(mod_names_list.begin(), mod_names_list.end()), mod_names_list.end());
	}
	return mod_names_list;
}

WzMods::LoadedMod::LoadedMod(const std::string& name, const std::string& filename, const std::string& fullRealPath)
: name(name)
, filename(filename)
, fullRealPath(fullRealPath)
{ }

Sha256& WzMods::LoadedMod::getHash()
{
	if (fileHash.isZero())
	{
		fileHash = findHashOfFile(filename.c_str());
	}
	return fileHash;
}

std::vector<Sha256> const &getModHashList()
{
	if (mod_hash_list.empty())
	{
		for (auto &mod : loaded_mods)
		{
			Sha256& hash = mod.getHash();
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
