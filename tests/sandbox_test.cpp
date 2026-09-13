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

/**
 * Checks for lib/sandbox. Build with -DWZ_BUILD_SANDBOX_TEST=ON and run the binary.
 *
 * The confinement checks run in a child process: once a tier is applied it cannot be lifted, and the parent
 * still has to remove the temporary directory it made under $HOME.
 */

#include "lib/sandbox/sandbox.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static int failures = 0;

static void check(bool passed, const char* what)
{
	printf("%-58s %s\n", what, passed ? "ok" : "FAILED");
	if (!passed)
	{
		++failures;
	}
}

static bool canWrite(const std::string& path)
{
	const int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (fd < 0)
	{
		return false;
	}
	close(fd);
	unlink(path.c_str());
	return true;
}

static bool canRead(const char* path)
{
	const int fd = open(path, O_RDONLY);
	if (fd < 0)
	{
		return false;
	}
	close(fd);
	return true;
}

static void checkTierParsing()
{
	wz::sandbox::Tier tier = wz::sandbox::Tier::Strict;
	check(wz::sandbox::parseTier("off", tier) && tier == wz::sandbox::Tier::Off, "parseTier(\"off\")");
	tier = wz::sandbox::Tier::Strict;
	check(wz::sandbox::parseTier("0", tier) && tier == wz::sandbox::Tier::Off, "parseTier(\"0\")");
	check(wz::sandbox::parseTier("write", tier) && tier == wz::sandbox::Tier::WriteConfined, "parseTier(\"write\")");
	check(wz::sandbox::parseTier("strict", tier) && tier == wz::sandbox::Tier::Strict, "parseTier(\"strict\")");

	tier = wz::sandbox::Tier::WriteConfined;
	check(!wz::sandbox::parseTier("bogus", tier) && tier == wz::sandbox::Tier::WriteConfined, "parseTier(\"bogus\")");
	check(!wz::sandbox::parseTier(nullptr, tier), "parseTier(nullptr)");

	check(strcmp(wz::sandbox::tierName(wz::sandbox::Tier::Off), "off") == 0, "tierName(Off)");
	check(strcmp(wz::sandbox::tierName(wz::sandbox::Tier::WriteConfined), "write") == 0, "tierName(WriteConfined)");
	check(strcmp(wz::sandbox::tierName(wz::sandbox::Tier::Strict), "strict") == 0, "tierName(Strict)");
}

static int runConfinedChecks(const std::string& writeRoot, const std::string& homeOutside)
{
	wz::sandbox::Plan plan;
	plan.writeRoots.push_back(writeRoot);

	const wz::sandbox::Status status = wz::sandbox::confine(plan, wz::sandbox::Tier::WriteConfined);
	for (const wz::sandbox::LogLine& line : wz::sandbox::pendingLog())
	{
		printf("  sandbox: %s\n", line.text.c_str());
	}

	if (!status.active)
	{
		printf("SKIP: no confinement applied (%s)\n", status.detail.c_str());
		return 0;
	}
	printf("confined with %s (%s)\n", status.backend.c_str(), status.detail.c_str());

	check(canWrite(writeRoot + "/probe"), "write inside the write root is allowed");
	check(!canWrite(homeOutside), "write elsewhere under $HOME is denied");
	check(errno == EACCES, "the denial is EACCES");
	check(canRead("/etc/hostname"), "reads outside the write root still work");
	check(wz::sandbox::status().active, "status() reports the applied confinement");

	return failures;
}

int main()
{
	checkTierParsing();

	const char* home = getenv("HOME");
	if (home == nullptr || home[0] == '\0')
	{
		printf("SKIP: no $HOME, cannot test confinement\n");
		return failures != 0;
	}

	std::string writeRoot = std::string(home) + "/.wz-sandbox-test-XXXXXX";
	if (mkdtemp(&writeRoot[0]) == nullptr)
	{
		printf("SKIP: could not create a temporary directory: %s\n", strerror(errno));
		return failures != 0;
	}
	const std::string homeOutside = std::string(home) + "/.wz-sandbox-test-outside";

	fflush(stdout);   // or the child inherits this output and prints it again
	const pid_t child = fork();
	if (child == 0)
	{
		const int childFailures = runConfinedChecks(writeRoot, homeOutside);
		fflush(stdout);   // _exit() does not flush, and stdout is fully buffered when it is not a terminal
		_exit(childFailures != 0);
	}

	int status = 0;
	waitpid(child, &status, 0);

	unlink(homeOutside.c_str());
	rmdir(writeRoot.c_str());

	const bool childFailed = !WIFEXITED(status) || WEXITSTATUS(status) != 0;
	printf("%s\n", (failures == 0 && !childFailed) ? "PASS" : "FAIL");
	return (failures == 0 && !childFailed) ? 0 : 1;
}
