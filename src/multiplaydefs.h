/*
	This file is part of Warzone 2100.
	Copyright (C) 2023  Warzone 2100 Project

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

#ifndef __INCLUDED_SRC_MULTIPLAYDEFS_H__
#define __INCLUDED_SRC_MULTIPLAYDEFS_H__

#include "lib/framework/frame.h"
#include "lib/framework/types.h"
#include "lib/netplay/nettypes.h"

enum class PLAYER_LEAVE_MODE : uint8_t
{
	DESTROY_RESOURCES = 0,
	SPLIT_WITH_TEAM = 1
};

constexpr PLAYER_LEAVE_MODE PLAYER_LEAVE_MODE_MAX = PLAYER_LEAVE_MODE::SPLIT_WITH_TEAM;

constexpr PLAYER_LEAVE_MODE PLAYER_LEAVE_MODE_DEFAULT = PLAYER_LEAVE_MODE::SPLIT_WITH_TEAM;

#endif // __INCLUDED_SRC_MULTIPLAYDEFS_H__
