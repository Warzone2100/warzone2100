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

/// <summary>
/// Utility function to store a 16-bit value (host byte order) into a (possibly unaligned) destination
/// using network byte order (i.e., big endian).
/// </summary>
/// <param name="dst">Destination byte buffer. Does not need to be properly aligned to uint16_t type requirements.</param>
/// <param name="src">Source 16-bit value (host byte order).</param>
inline void wz_htons_store_unaligned(uint8_t* dst, uint16_t src)
{
	dst[0] = static_cast<uint8_t>(src >> 8);
	dst[1] = static_cast<uint8_t>(src);
}

/// <summary>
/// Utility function to load a 16-bit value (network byte order) from a (possibly unaligned) source
/// into a destination (host byte order).
/// </summary>
/// <param name="dst">Destination 16-bit value (host byte order).</param>
/// <param name="src">Source byte buffer. Does not need to be properly aligned to uint16_t type requirements.</param>
inline void wz_ntohs_load_unaligned(uint16_t& dst, const uint8_t* src)
{
	dst = (src[0] << 8) | src[1];
}
