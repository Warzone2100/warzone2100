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

#ifndef _REQUESTHANDLER_HPP_
#define _REQUESTHANDLER_HPP_

#include <boost/asio.hpp>

class GameLobby
{
    public:
        GameLobby();
        ~GameLobby();

	void handleRequest(boost::shared_ptr<boost::asio::ip::tcp::socket> socket);

    private:
        void addGame(boost::shared_ptr<boost::asio::ip::tcp::socket> socket);
        void listGames(boost::shared_ptr<boost::asio::ip::tcp::socket> socket);
        
    private:
        class impl;
        impl* pimpl;
};

#endif // _REQUESTHANDLER_HPP_
