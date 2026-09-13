// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version, as published
	by the Warzone 2100 Project. They are provided in COPYING.SECTION7 in
	the top-level directory of the source distribution. This notice must
	be kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/

#include "sandbox_internal.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
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

Status backendConfine(const Plan& plan, Tier requested)
{
	(void)plan;
	(void)requested;

	Status result;
	result.detail = "not implemented";
	return result;
}

} // namespace wz::sandbox
