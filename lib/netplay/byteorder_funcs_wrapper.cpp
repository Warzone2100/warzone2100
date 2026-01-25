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

#include "lib/netplay/byteorder_funcs_wrapper.h"

#include "lib/framework/wzglobal.h"

// bring in the original `htonl`/`htons`/`ntohs`/`htohl` functions
#if defined WZ_OS_WIN
# include <winsock.h>
#else // *NIX / *BSD variants
# include <arpa/inet.h>
#endif

uint32_t wz_htonl(uint32_t hostlong)
{
	return htonl(hostlong);
}

uint16_t wz_htons(uint16_t hostshort)
{
	return htons(hostshort);
}

uint32_t wz_ntohl(uint32_t netlong)
{
	return ntohl(netlong);
}

uint16_t wz_ntohs(uint16_t netshort)
{
	return ntohs(netshort);
}
