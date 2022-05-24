/*
	This file is part of Warzone 2100.
	Copyright (C) 2022  Warzone 2100 Project

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

#include "../include/wzmaplib/map_version.h"
#include <wzmaplib_internal/map_config_internal.h>

namespace WzMap {

const char* wzmaplib_version_string()
{
	return WZMAPLIB_VERSION_STRING;
}

unsigned int wzmaplib_version_major()
{
	return WZMAPLIB_VERSION_MAJOR;
}

unsigned int wzmaplib_version_minor()
{
	return WZMAPLIB_VERSION_MINOR;
}

unsigned int wzmaplib_version_rev()
{
	return WZMAPLIB_VERSION_REV;
}

} // namespace WzMap
