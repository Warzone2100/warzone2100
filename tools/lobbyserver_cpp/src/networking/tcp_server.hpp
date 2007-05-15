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

#ifndef _INCLUDE_NETWORKING_TCP_SERVER_HPP_
#define _INCLUDE_NETWORKING_TCP_SERVER_HPP_

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/utility.hpp>

class TCPServer : boost::noncopyable
{
    public:
        typedef boost::function<void (boost::shared_ptr<boost::asio::ip::tcp::socket>)> connectionHandler;

        TCPServer(boost::shared_ptr<boost::asio::io_service> io_service, const connectionHandler& handler);
        ~TCPServer();

        void listen(unsigned short port);

    private:
        class impl;
        impl* pimpl;
};

#endif // _INCLUDE_NETWORKING_TCP_SERVER_HPP_
