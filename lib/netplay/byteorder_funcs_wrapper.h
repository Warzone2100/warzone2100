/*
	This file is part of Warzone 2100.
	Copyright (C) 2024  Warzone 2100 Project

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

#include <stdint.h>

///
/// byteorder functions wrappers for WZ just to avoid polluting all places,
/// where these functions are needed, with conditional includes of <arpa/inet.h>
/// and winsock headers.
///

uint32_t wz_htonl(uint32_t hostlong);
uint16_t wz_htons(uint16_t hostshort);
uint32_t wz_ntohl(uint32_t netlong);
uint16_t wz_ntohs(uint16_t netshort);
