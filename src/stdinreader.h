/*
	This file is part of Warzone 2100.
	Copyright (C) 2020-2021  Warzone 2100 Project

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

#pragma once

void stdInThreadInit();
void stdInThreadShutdown();

#include "lib/framework/wzglobal.h"

#if defined(WZ_CC_MINGW)
#include <cstdio> // For __MINGW_PRINTF_FORMAT define
void wz_command_interface_output(const char *str, ...) WZ_DECL_FORMAT(__MINGW_PRINTF_FORMAT, 1, 2);
#else
void wz_command_interface_output(const char *str, ...) WZ_DECL_FORMAT(printf, 1, 2);
#endif
