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

/// The backend for platforms with no self-confinement facility that fits.

#include "sandbox_internal.h"

#if defined(__FreeBSD__)
# include <cerrno>
# include <cstring>
# include <sys/procctl.h>
# include <unistd.h>
#endif

namespace wz::sandbox
{

void backendHarden(Tier requested)
{
	(void)requested;

#if defined(__FreeBSD__) && defined(PROC_NO_NEW_PRIVS_CTL)
	int enable = PROC_NO_NEW_PRIVS_ENABLE;
	if (procctl(P_PID, getpid(), PROC_NO_NEW_PRIVS_CTL, &enable) != 0)
	{
		logPending(LogLevel::Info, "no_new_privs unavailable: %s", strerror(errno));
	}
#endif
}

Status backendConfine(const Plan& plan, Tier requested)
{
	(void)plan;
	(void)requested;

	Status result;
	result.detail = "not supported on this platform";
	return result;
}

} // namespace wz::sandbox
