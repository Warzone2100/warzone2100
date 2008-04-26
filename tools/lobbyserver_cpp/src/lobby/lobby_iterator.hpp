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

#ifndef _LOBBY_ITERATOR_HPP_
#define _LOBBY_ITERATOR_HPP_

#include "lobby.hpp"

class GameLobby::iterator_interface
{
    public:
        typedef const GAMESTRUCT& const_reference;

        // Constructor
        iterator_interface(const GameLobby& lobby);

        class const_iterator
        {
            private:
                // Should only be constructed by iterator_interface
                const_iterator(const iterator_interface& interface, const std::list<GAMESTRUCT>::const_iterator& iter);
                friend class iterator_interface;

            public:
                bool operator==(const const_iterator& i) const;
                bool operator!=(const const_iterator& i) const;
                void operator++();

                const_reference operator*() const;

            private:
                const iterator_interface& _interface;
                std::list<GAMESTRUCT>::const_iterator _iter;
        };

        const_iterator begin() const;
        const_iterator end() const;

        std::size_t size() const;

    private:
        const GameLobby& _lobby;
        // This lock will be helt on _lobby from construction, and automatically released on destruction
        ReadWriteMutex::scoped_readonlylock lock;
};

#endif // _LOBBY_ITERATOR_HPP_
