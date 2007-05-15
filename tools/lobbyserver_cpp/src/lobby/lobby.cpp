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
#include "game.hpp"

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

GameLobby::~GameLobby()
{
    // Make sure all pending operations are finished before closing down, by acquiring a write lock first
    ReadWriteMutex::scoped_lock lock(_mutex);
}

GameLobby::const_iterator::const_iterator(const GameLobby& lobby, const std::list<GAMESTRUCT>::const_iterator& iter) :
    _lobby(lobby),
    lock(_lobby._mutex),
    _iter(iter)
{
}

GameLobby::const_iterator::const_iterator(const const_iterator& org) :
    _lobby(org._lobby),
    lock(_lobby._mutex),
    _iter(org._iter)
{
}

bool GameLobby::const_iterator::operator==(const const_iterator& i) const
{
    if (&_lobby != &i._lobby)
        return false;

    return _iter == i._iter;
}

bool GameLobby::const_iterator::operator!=(const const_iterator& i) const
{
    return !(*this == i);
}

void GameLobby::const_iterator::operator++()
{
    ++_iter;
}

std::size_t GameLobby::const_iterator::operator-(const const_iterator& i) const
{
    // This needs to be a comparison between two iterators of the same GameLobby
    if (&_lobby != &i._lobby)
        return 0;

    // The current iterator needs to be ==end()
    // the other (i) ==begin()
    if (_iter == _lobby._games.end() && i._iter == _lobby._games.begin()
     || _iter == _lobby._games.begin() && i._iter == _lobby._games.end())
        return _lobby.size();

    return 0;
}

GameLobby::const_iterator::const_reference GameLobby::const_iterator::operator*() const
{
    return *_iter;
}

GameLobby::const_iterator GameLobby::begin() const
{
    ReadWriteMutex::scoped_readonlylock lock(_mutex);
    return GameLobby::const_iterator(*this, _games.begin());
}

GameLobby::const_iterator GameLobby::end() const
{
    return const_iterator(*this, _games.end());
}

std::size_t GameLobby::size() const
{
    ReadWriteMutex::scoped_readonlylock lock(_mutex);

    return _games.size();
}
