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

#include "game.hpp"
#include <iostream>
#include <boost/thread/recursive_mutex.hpp>
#include <sstream>

extern boost::recursive_mutex cout_mutex;
extern boost::recursive_mutex cerr_mutex;

static inline std::string str_repeat(const std::string& src, unsigned int count)
{
    std::string tmp;
    tmp.reserve(src.length() * count);
    for (unsigned int i = 0; i != count; ++i)
    {
        tmp += src;
    }

    return tmp;
}

template <class T>
static inline std::string to_string(const T& t)
{
    std::stringstream ss;
    ss << t;
    return ss.str();
}

void printGame(const GAMESTRUCT& game)
{
    std::string gameName(game.name);
    std::string gameSize(to_string(game.desc.dwSize));
    std::string gameFlags(to_string(game.desc.dwFlags));
    std::string gameHost(game.desc.host);
    std::string gameMaxPlayers(to_string(game.desc.dwMaxPlayers));
    std::string gameCurrentPlayers(to_string(game.desc.dwCurrentPlayers));
    std::string gameUser1(to_string(game.desc.dwUserFlags[0]));
    std::string gameUser2(to_string(game.desc.dwUserFlags[1]));
    std::string gameUser3(to_string(game.desc.dwUserFlags[2]));
    std::string gameUser4(to_string(game.desc.dwUserFlags[3]));

    unsigned int targetLength = std::max(std::max(std::max(std::max(std::max(std::max(std::max(std::max(std::max(gameName.length() + 2, gameSize.length()), gameFlags.length()), gameHost.length() + 2), gameMaxPlayers.length()), gameCurrentPlayers.length()), gameUser1.length()), gameUser2.length()), gameUser3.length()), gameUser4.length());

    boost::recursive_mutex::scoped_lock lock (cout_mutex);

    std::cout << "Game info:\n"
              << "+------------------" << str_repeat("-", targetLength) << "-+\n"
              << "| Name           | \"" << gameName << "\"" << str_repeat(" ", targetLength - gameName.length() - 2) << " |\n"
              << "| Size           | " << str_repeat(" ", targetLength - gameSize.length()) << gameSize << " |\n"
              << "| Flags          | " << str_repeat(" ", targetLength - gameFlags.length()) << gameFlags << " |\n"
              << "| Host           | \"" << gameHost << "\"" << str_repeat(" ", targetLength - gameHost.length() - 2) << " |\n"
              << "| MaxPlayers     | " << str_repeat(" ", targetLength - gameMaxPlayers.length()) << gameMaxPlayers << " |\n"
              << "| CurrentPlayers | " << str_repeat(" ", targetLength - gameCurrentPlayers.length()) << gameCurrentPlayers << " |\n"
              << "| User1          | " << str_repeat(" ", targetLength - gameUser1.length()) << gameUser1 << " |\n"
              << "| User2          | " << str_repeat(" ", targetLength - gameUser2.length()) << gameUser2 << " |\n"
              << "| User3          | " << str_repeat(" ", targetLength - gameUser3.length()) << gameUser3 << " |\n"
              << "| User4          | " << str_repeat(" ", targetLength - gameUser4.length()) << gameUser4 << " |\n"
              << "+------------------" << str_repeat("-", targetLength) << "-+" << std::endl;
}
