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

#include "lobby.hpp"

GameLobby::~GameLobby()
{
    // Make sure all pending operations are finished before closing down, by acquiring a write lock first
    ReadWriteMutex::scoped_lock lock (_mutex);
}

GameLobby::const_iterator GameLobby::begin() const
{
    ReadWriteMutex::scoped_readonlylock lock (_mutex);
    return GameLobby::const_iterator(*this, _games.begin());
}

GameLobby::const_iterator GameLobby::end() const
{
    return const_iterator(*this, _games.end());
}

std::size_t GameLobby::size() const
{
    ReadWriteMutex::scoped_readonlylock lock (_mutex);

    return _games.size();
}
