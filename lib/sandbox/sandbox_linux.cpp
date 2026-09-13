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

#include "sandbox_internal.h"
#include "landlock_compat.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <string>
#include <sys/prctl.h>
#include <unistd.h>

// Linux 3.5
#if !defined(PR_SET_NO_NEW_PRIVS)
# define PR_SET_NO_NEW_PRIVS 38
#endif

namespace wz::sandbox
{

/// Points any of stdin, stdout and stderr the process was started without at /dev/null, so the first file
/// the game opens cannot land on a standard descriptor.
static void openStandardDescriptors()
{
	for (int fd = STDIN_FILENO; fd <= STDERR_FILENO; ++fd)
	{
		if (fcntl(fd, F_GETFD) != -1 || errno != EBADF)
		{
			continue;
		}

		// Not O_CLOEXEC: gdb and crashpad_handler should inherit these
		const int opened = open("/dev/null", (fd == STDIN_FILENO) ? O_RDONLY : O_WRONLY);
		if (opened < 0)
		{
			logPending(LogLevel::Info, "descriptor %d left closed, /dev/null: %s", fd, strerror(errno));
			break;
		}
		if (opened != fd)
		{
			dup2(opened, fd);
			close(opened);
		}
	}
}

void backendHarden()
{
	if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0)
	{
		logPending(LogLevel::Info, "no_new_privs unavailable: %s", strerror(errno));
	}

	openStandardDescriptors();
}

constexpr uint64_t RIGHTS_RO = LANDLOCK_ACCESS_FS_READ_FILE | LANDLOCK_ACCESS_FS_READ_DIR;
constexpr uint64_t RIGHTS_ROX = RIGHTS_RO | LANDLOCK_ACCESS_FS_EXECUTE;
constexpr uint64_t RIGHTS_RW = RIGHTS_RO
	| LANDLOCK_ACCESS_FS_WRITE_FILE | LANDLOCK_ACCESS_FS_MAKE_REG | LANDLOCK_ACCESS_FS_MAKE_DIR
	| LANDLOCK_ACCESS_FS_MAKE_SYM | LANDLOCK_ACCESS_FS_MAKE_SOCK | LANDLOCK_ACCESS_FS_MAKE_FIFO
	| LANDLOCK_ACCESS_FS_REMOVE_FILE | LANDLOCK_ACCESS_FS_REMOVE_DIR
	| LANDLOCK_ACCESS_FS_REFER | LANDLOCK_ACCESS_FS_TRUNCATE;
constexpr uint64_t RIGHTS_DEV = RIGHTS_RO | LANDLOCK_ACCESS_FS_WRITE_FILE;
constexpr uint64_t RIGHTS_SOCKET = RIGHTS_RO | LANDLOCK_ACCESS_FS_MAKE_SOCK | LANDLOCK_ACCESS_FS_REMOVE_FILE;
constexpr uint64_t RIGHTS_ONE_FILE = RIGHTS_RO
	| LANDLOCK_ACCESS_FS_MAKE_REG | LANDLOCK_ACCESS_FS_WRITE_FILE | LANDLOCK_ACCESS_FS_TRUNCATE;

/// The rights the ruleset governs. Anything left out of it stays unrestricted.
static uint64_t handledAccess(int abi)
{
	uint64_t handled = LANDLOCK_ACCESS_FS_EXECUTE | LANDLOCK_ACCESS_FS_WRITE_FILE
		| LANDLOCK_ACCESS_FS_READ_FILE | LANDLOCK_ACCESS_FS_READ_DIR
		| LANDLOCK_ACCESS_FS_REMOVE_DIR | LANDLOCK_ACCESS_FS_REMOVE_FILE
		| LANDLOCK_ACCESS_FS_MAKE_CHAR | LANDLOCK_ACCESS_FS_MAKE_DIR | LANDLOCK_ACCESS_FS_MAKE_REG
		| LANDLOCK_ACCESS_FS_MAKE_SOCK | LANDLOCK_ACCESS_FS_MAKE_FIFO | LANDLOCK_ACCESS_FS_MAKE_BLOCK
		| LANDLOCK_ACCESS_FS_MAKE_SYM;

	if (abi >= 2)
	{
		handled |= LANDLOCK_ACCESS_FS_REFER;
	}
	if (abi >= 3)
	{
		handled |= LANDLOCK_ACCESS_FS_TRUNCATE;
	}
	return handled;
}

static void addPathRule(int rulesetFd, const std::string& path, uint64_t allowed)
{
	const int pathFd = open(path.c_str(), O_PATH | O_CLOEXEC);
	if (pathFd < 0)
	{
		if (errno != ENOENT)
		{
			logPending(LogLevel::Info, "no rule for %s: %s", path.c_str(), strerror(errno));
		}
		return;
	}

	landlock_path_beneath_attr attr = {};
	attr.allowed_access = allowed;
	attr.parent_fd = pathFd;
	if (landlock::addRule(rulesetFd, LANDLOCK_RULE_PATH_BENEATH, &attr, 0) != 0)
	{
		logPending(LogLevel::Info, "rule for %s rejected: %s", path.c_str(), strerror(errno));
	}
	close(pathFd);
}

/// The GPU driver writes its shader cache as the game process, so the ruleset has to cover it.
static void addShaderCacheRules(int rulesetFd, uint64_t handled)
{
	const char* home = getenv("HOME");
	const char* xdgCache = getenv("XDG_CACHE_HOME");

	std::string cacheDir;
	if (xdgCache != nullptr && xdgCache[0] != '\0')
	{
		cacheDir = xdgCache;
	}
	else if (home != nullptr && home[0] != '\0')
	{
		cacheDir = std::string(home) + "/.cache";
	}

	if (!cacheDir.empty())
	{
		for (const char* name : { "mesa_shader_cache", "mesa_shader_cache_db", "radv_builtin_shaders", "nvidia" })
		{
			addPathRule(rulesetFd, cacheDir + "/" + name, RIGHTS_RW & handled);
		}
	}
	if (home != nullptr && home[0] != '\0')
	{
		addPathRule(rulesetFd, std::string(home) + "/.nv", RIGHTS_RW & handled);
	}
}

static void addWriteTierRules(int rulesetFd, const Plan& plan, uint64_t handled)
{
	// Everything stays readable, which is what covers the Plan's readRoots and execRoots
	addPathRule(rulesetFd, "/", RIGHTS_ROX & handled);

	addPathRule(rulesetFd, "/dev", RIGHTS_DEV & handled);
	addPathRule(rulesetFd, "/dev/shm", RIGHTS_RW & handled);
	addPathRule(rulesetFd, "/tmp", RIGHTS_RW & handled);

	const char* runtimeDir = getenv("XDG_RUNTIME_DIR");
	if (runtimeDir != nullptr && runtimeDir[0] != '\0')
	{
		addPathRule(rulesetFd, runtimeDir, RIGHTS_RW & handled);
	}

	addShaderCacheRules(rulesetFd, handled);

	for (const std::string& path : plan.writeRoots)
	{
		addPathRule(rulesetFd, path, RIGHTS_RW & handled);
	}
	for (const std::string& path : plan.socketDirs)
	{
		addPathRule(rulesetFd, path, RIGHTS_SOCKET & handled);
	}
	for (const std::string& path : plan.writeFileParents)
	{
		addPathRule(rulesetFd, path, RIGHTS_ONE_FILE & handled);
	}
}

static bool isSingleThreaded()
{
	DIR* tasks = opendir("/proc/self/task");
	if (tasks == nullptr)
	{
		return true;
	}

	int count = 0;
	while (const dirent* entry = readdir(tasks))
	{
		if (entry->d_name[0] != '.')
		{
			++count;
		}
	}
	closedir(tasks);
	return count <= 1;
}

Status backendConfine(const Plan& plan, Tier requested)
{
	Status result;

	if (requested == Tier::Off)
	{
		result.detail = "disabled";
		return result;
	}

	const int abi = landlock::createRuleset(nullptr, 0, LANDLOCK_CREATE_RULESET_VERSION);
	if (abi < 0)
	{
		result.detail = std::string("landlock unavailable: ") + strerror(errno);
		return result;
	}

	if (!isSingleThreaded())
	{
		logPending(LogLevel::Error, "confine() ran with threads already started, so only this one is confined");
	}
	if (requested == Tier::Strict)
	{
		logPending(LogLevel::Info, "the strict tier is not implemented, applying the write tier");
	}

	const uint64_t handled = handledAccess(abi);
	const landlock::RulesetAttr attr{ handled };
	const int rulesetFd = landlock::createRuleset(&attr, sizeof(attr), 0);
	if (rulesetFd < 0)
	{
		result.detail = std::string("ruleset: ") + strerror(errno);
		logPending(LogLevel::Error, "landlock_create_ruleset: %s", strerror(errno));
		return result;
	}

	addWriteTierRules(rulesetFd, plan, handled);

	prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0);

	const uint32_t restrictFlags = (abi >= 8) ? LANDLOCK_RESTRICT_SELF_TSYNC : 0;
	if (landlock::restrictSelf(rulesetFd, restrictFlags) != 0)
	{
		result.detail = std::string("restrict: ") + strerror(errno);
		logPending(LogLevel::Error, "landlock_restrict_self: %s", strerror(errno));
		close(rulesetFd);
		return result;
	}
	close(rulesetFd);

	result.active = true;
	result.effective = Tier::WriteConfined;
	result.backend = "landlock";
	result.detail = "ABI " + std::to_string(abi);
	return result;
}

} // namespace wz::sandbox
