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
 * Each tier's checks run in their own child process: confinement cannot be lifted once applied, and the
 * parent still has to remove the temporary directory it made under $HOME.
 */

#include "lib/sandbox/sandbox.h"

#include <sys/prctl.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fcntl.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/syscall.h>
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

/// SECCOMP_MODE_FILTER out of /proc/self/status, or 0 when there is no filter.
static int seccompMode()
{
	FILE* status = fopen("/proc/self/status", "r");
	if (status == nullptr)
	{
		return 0;
	}

	int mode = 0;
	char line[256];
	while (fgets(line, sizeof(line), status) != nullptr)
	{
		if (sscanf(line, "Seccomp: %d", &mode) == 1)
		{
			break;
		}
	}
	fclose(status);
	return mode;
}

/// A child of a filtered process inherits the filter, so these two have to keep working.
static bool childCanExec()
{
	const pid_t child = fork();
	if (child == 0)
	{
		execl("/bin/true", "true", static_cast<char*>(nullptr));
		_exit(127);
	}

	int status = 0;
	waitpid(child, &status, 0);
	return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static bool childCanPtrace()
{
	const pid_t child = fork();
	if (child == 0)
	{
		const long result = ptrace(PTRACE_TRACEME, 0, nullptr, nullptr);
		_exit((result == -1 && errno == EPERM) ? 1 : 0);
	}

	int status = 0;
	waitpid(child, &status, 0);
	return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static int runHardenChecks()
{
	wz::sandbox::harden(wz::sandbox::Tier::WriteConfined);
	for (const wz::sandbox::LogLine& line : wz::sandbox::pendingLog())
	{
		printf("  sandbox: %s\n", line.text.c_str());
	}

	check(prctl(PR_GET_NO_NEW_PRIVS, 0, 0, 0, 0) == 1, "no_new_privs is set");
	check(fcntl(STDIN_FILENO, F_GETFD) != -1, "stdin is open");
	check(fcntl(STDOUT_FILENO, F_GETFD) != -1, "stdout is open");
	check(fcntl(STDERR_FILENO, F_GETFD) != -1, "stderr is open");

	if (seccompMode() != 2)
	{
		printf("SKIP: no syscall filter installed\n");
		return failures;
	}
	printf("syscall filter installed\n");

#if defined(__NR_keyctl)
	errno = 0;
	check(syscall(__NR_keyctl, 0, 0, 0, 0, 0) == -1 && errno == EPERM, "keyctl is denied with EPERM");
#endif
	check(canRead("/etc/hostname"), "an ordinary open still works");
	check(childCanExec(), "a child can still execve");
	check(childCanPtrace(), "a child can still ptrace");

	return failures;
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

static int runConfinedChecks(wz::sandbox::Tier tier, const std::string& writeRoot,
                             const std::string& homeOutside, const std::string& homeReadable)
{
	const char* tierText = wz::sandbox::tierName(tier);

	wz::sandbox::Plan plan;
	plan.writeRoots.push_back(writeRoot);

	const wz::sandbox::Status status = wz::sandbox::confine(plan, tier);
	for (const wz::sandbox::LogLine& line : wz::sandbox::pendingLog())
	{
		printf("  sandbox: %s\n", line.text.c_str());
	}

	if (!status.active)
	{
		printf("SKIP %s: no confinement applied (%s)\n", tierText, status.detail.c_str());
		return 0;
	}
	printf("confined with %s (%s), tier %s\n", status.backend.c_str(), status.detail.c_str(), tierText);

	check(status.effective == tier, "status() reports the tier that was requested");
	check(canWrite(writeRoot + "/probe"), "write inside the write root is allowed");
	check(!canWrite(homeOutside), "write elsewhere under $HOME is denied");
	check(errno == EACCES, "the denial is EACCES");
	check(canRead("/etc/hostname"), "reads outside the write root still work");

	// The tiers differ only in whether the rest of $HOME stays readable
	const bool readable = canRead(homeReadable.c_str());
	if (tier == wz::sandbox::Tier::Strict)
	{
		check(!readable, "reads elsewhere under $HOME are denied");
	}
	else
	{
		check(readable, "reads elsewhere under $HOME are allowed");
	}

	return failures;
}

static bool runTierInChild(wz::sandbox::Tier tier, const std::string& writeRoot,
                           const std::string& homeOutside, const std::string& homeReadable)
{
	fflush(stdout);   // or the child inherits this output and prints it again
	const pid_t child = fork();
	if (child == 0)
	{
		const int childFailures = runConfinedChecks(tier, writeRoot, homeOutside, homeReadable);
		fflush(stdout);   // _exit() does not flush, and stdout is fully buffered when it is not a terminal
		_exit(childFailures != 0);
	}

	int status = 0;
	waitpid(child, &status, 0);
	return WIFEXITED(status) && WEXITSTATUS(status) == 0;
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
	const std::string homeReadable = std::string(home) + "/.wz-sandbox-test-readable";
	const int readableFd = open(homeReadable.c_str(), O_WRONLY | O_CREAT, 0600);
	if (readableFd < 0)
	{
		printf("SKIP: could not create %s: %s\n", homeReadable.c_str(), strerror(errno));
		return failures != 0;
	}
	close(readableFd);

	fflush(stdout);
	const pid_t hardenChild = fork();
	if (hardenChild == 0)
	{
		const int hardenFailures = runHardenChecks();
		fflush(stdout);
		_exit(hardenFailures != 0);
	}
	int hardenStatus = 0;
	waitpid(hardenChild, &hardenStatus, 0);
	bool tiersPassed = WIFEXITED(hardenStatus) && WEXITSTATUS(hardenStatus) == 0;

	tiersPassed = runTierInChild(wz::sandbox::Tier::WriteConfined, writeRoot, homeOutside, homeReadable) && tiersPassed;
	tiersPassed = runTierInChild(wz::sandbox::Tier::Strict, writeRoot, homeOutside, homeReadable) && tiersPassed;

	unlink(homeReadable.c_str());
	unlink(homeOutside.c_str());
	rmdir(writeRoot.c_str());

	printf("%s\n", (failures == 0 && tiersPassed) ? "PASS" : "FAIL");
	return (failures == 0 && tiersPassed) ? 0 : 1;
}
