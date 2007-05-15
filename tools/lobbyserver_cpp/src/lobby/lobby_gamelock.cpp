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

GameLobby::gameLock::gameLock(GameLobby& lobby) :
    _lobby(lobby),
    iterValid(false)
{
}

GameLobby::gameLock::gameLock(GameLobby& lobby, const GAMESTRUCT& game) :
    _lobby(lobby),
    iterValid(false)
{
    *this = game;
}

GameLobby::gameLock::~gameLock()
{
    clear();
}

GameLobby::gameLock& GameLobby::gameLock::operator=(const GAMESTRUCT& game)
{
    // Check to see if the new game isn't the same as the old one
    if (*this == game)
        return *this;

    // If we already have a game then remove it first
    clear();

    {
        ReadWriteMutex::scoped_lock lock(_lobby._mutex);

        // Insert our game into the list
        _iter = _lobby._games.insert(_lobby._games.begin(), game);
        iterValid = true;
    }

    return *this;
}

void GameLobby::gameLock::clear()
{
    if (iterValid)
    {
        ReadWriteMutex::scoped_lock lock(_lobby._mutex);

        _lobby._games.erase(_iter);

        iterValid = false;
    }
}

bool GameLobby::gameLock::operator==(const GAMESTRUCT& game)
{
    if (!iterValid)
        return false;

    ReadWriteMutex::scoped_readonlylock lock(_lobby._mutex);

    return *_iter == game;
}

bool GameLobby::gameLock::operator!=(const GAMESTRUCT& game)
{
    return !(*this == game);
}
