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

#ifndef _LOBBY_HPP_
#define _LOBBY_HPP_

#include "read_write_mutex.hpp"
#include "game.hpp"
#include <list>

class GameLobby
{
    public:
        ~GameLobby();

        class gameLock : boost::noncopyable
        {
            public:
                gameLock(GameLobby& lobby);
                gameLock(GameLobby& lobby, const GAMESTRUCT& game);
                ~gameLock();

                gameLock& operator=(const GAMESTRUCT& game);
                void clear();

                bool operator==(const GAMESTRUCT& game);
                bool operator!=(const GAMESTRUCT& game);

            private:
                GameLobby& _lobby;

                std::list<GAMESTRUCT>::iterator _iter;
                bool iterValid;
        };

        class const_iterator
        {
            public:
                typedef const GAMESTRUCT& const_reference;

            private:
                // Should only be constructed by GameLobby
                const_iterator(const GameLobby& lobby, const std::list<GAMESTRUCT>::const_iterator& iter);

            public:
                const_iterator(const const_iterator& org);

            private:
                // Private copy assignment operator, to lazy to implement it right now
                const_iterator& operator=(const const_iterator& org);

            public:
                bool operator==(const const_iterator& i) const;
                bool operator!=(const const_iterator& i) const;
                void operator++();

                std::size_t operator-(const const_iterator& i) const;

                const_reference operator*() const;

            private:
                const GameLobby& _lobby;
                ReadWriteMutex::scoped_readonlylock lock;

                std::list<GAMESTRUCT>::const_iterator _iter;

            friend class GameLobby;
        };

        const_iterator begin() const;
        const_iterator end() const;

        std::size_t size() const;

    private:
        ReadWriteMutex _mutex;
        std::list<GAMESTRUCT> _games;
};

#endif // _LOBBY_HPP_
