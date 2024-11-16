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

#include <string>
#include "lib/framework/wzglobal.h"

enum class WZ_Command_Interface
{
	None,
	StdIn_Interface,
	Unix_Socket,
};

// used from clparse:
void configSetCmdInterface(WZ_Command_Interface mode, std::string value);

void cmdInterfaceThreadInit();
void cmdInterfaceThreadShutdown();

bool wz_command_interface_enabled();

#if defined(WZ_CC_MINGW)
#include <cstdio> // For __MINGW_PRINTF_FORMAT define
void wz_command_interface_output(const char *str, ...) WZ_DECL_FORMAT(__MINGW_PRINTF_FORMAT, 1, 2);
#else
void wz_command_interface_output(const char *str, ...) WZ_DECL_FORMAT(printf, 1, 2);
#endif

void wz_command_interface_output_str(const char *str);

void wz_command_interface_output_room_status_json();
