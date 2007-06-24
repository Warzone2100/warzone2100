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

#ifndef _INCLUDE_NETWORKING_NATTESTER_HPP_
#define _INCLUDE_NETWORKING_NATTESTER_HPP_

#include <boost/asio.hpp>
#include <boost/utility.hpp>

class NATTester : boost::noncopyable
{
    public:
        NATTester(boost::asio::ip::tcp::endpoint& endpoint_);

	bool reachable(unsigned int timeout_seconds);

    private:
        void handleConnect(const boost::asio::error& err);
        void handleTimeout();

    private:
        bool                          _finished;
        bool                          _reachable;
        boost::asio::io_service       _ios;
        boost::asio::ip::tcp:: socket _socket;
        boost::asio::deadline_timer   _timer;
};

#endif // _INCLUDE_NETWORKING_NATTESTER_HPP_
