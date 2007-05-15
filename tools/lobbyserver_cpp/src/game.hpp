/*
    Warzone 2100 Lobbyserver, serves as a meeting place to set up games
    Copyright (C) 2007  Giel van Schijndel

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    $Revision$
    $Id$
    $HeadURL$
*/

#ifndef _INCLUDE_GAME_HPP_
#define _INCLUDE_GAME_HPP_

#if USE_GAME_HEADER
extern "C" {
#include <framework/frame.h>
#include <netplay/netplay.h>
}

#else
#define StringSize 64

typedef struct
{
    signed int dwSize;
    signed int dwFlags;
    char       host[16];
    signed int dwMaxPlayers;
    signed int dwCurrentPlayers;
    signed int dwUser1;
    signed int dwUser2;
    signed int dwUser3;
    signed int dwUser4;
} SESSIONDESC;

typedef struct
{
    char        name[StringSize];
    SESSIONDESC desc;
} GAMESTRUCT;
#endif

#include <string>

inline bool operator==(const SESSIONDESC& left, const SESSIONDESC& right)
{
    if (!(left.dwSize            == right.dwSize
       && left.dwFlags           == right.dwFlags
       && left.dwMaxPlayers      == right.dwMaxPlayers
       && left.dwCurrentPlayers  == right.dwCurrentPlayers
       && left.dwUser1           == right.dwUser1
       && left.dwUser2           == right.dwUser2
       && left.dwUser3           == right.dwUser3
       && left.dwUser4           == right.dwUser4))
        return false;

#ifdef USE_STRCMP
    return std::string(left.host) == std::string(right.host);
#else
    return std::equal(left.host, &left.host[sizeof(left.host)], right.host);
#endif
}

inline bool operator==(const GAMESTRUCT& left, const GAMESTRUCT& right)
{
    if (!(left.desc == right.desc))
        return false;

#ifdef USE_STRCMP
    return std::string(left.name) == std::string(right.name);
#else
    return std::equal(left.name, &left.name[sizeof(left.name)], right.name);
#endif
}

#endif // _INCLUDE_GAME_HPP_
