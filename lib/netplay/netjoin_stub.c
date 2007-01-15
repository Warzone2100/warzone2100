/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
/*
 * Net join.
 * join related stuff
 */

#include "lib/framework/frame.h"
#include "netplay.h"

SDWORD NETgetGameFlagsUnjoined(UDWORD gameid, UDWORD flag)
{
	switch(flag)
	{
	case 1:
		return NetPlay.games[gameid].desc.dwUser1;
		break;
	case 2:
		return NetPlay.games[gameid].desc.dwUser2;
		break;
	case 3:
		return NetPlay.games[gameid].desc.dwUser3;
		break;
	case 4:
		return NetPlay.games[gameid].desc.dwUser4;
		break;
	default:
		debug( LOG_ERROR, "Invalid flag for NETgetGameFlagsUnjoined in netplay lib" );
		abort();
		break;
	}
	return 0;
}

