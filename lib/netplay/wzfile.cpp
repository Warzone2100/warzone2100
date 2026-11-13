// SPDX-License-Identifier: GPL-3.0-or-later WITH AdditionRef-WZ2100-GPL-3.0-Section-7-1.0-or-later

/*
	This file is part of Warzone 2100.
	Copyright (C) 2024-2026  Warzone 2100 Project (https://github.com/Warzone2100)

	Warzone 2100 is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Additional terms under Section 7 of the GNU General Public License
	version 3 apply to this file: version 1.0 of the Warzone 2100
	additional terms, or (at your option) any later version published by
	the Warzone 2100 Project, set out in COPYING.SECTION7 in the
	top-level directory of the source distribution. This notice must be
	kept intact.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100. If not, see <https://www.gnu.org/licenses/>.
*/
/**
 * @file wzfile.cpp
 *
 * `WZFile` thin wrapper over physfs file handle. Used for network code related to
 * downloading files.
 */

#include "wzfile.h"

static inline bool physfs_file_safe_close_impl(PHYSFS_file* f)
{
	if (!f)
	{
		return false;
	}
	if (!PHYSFS_isInit())
	{
		return false;
	}
	PHYSFS_close(f);
	return true;
}

void physfs_file_safe_close(PHYSFS_file* f)
{
	physfs_file_safe_close_impl(f);
}

WZFile::~WZFile()
{ }

bool WZFile::closeFile()
{
	if (!handle_)
	{
		return false;
	}
	PHYSFS_file* file = handle_.release();
	return physfs_file_safe_close_impl(file);
}
