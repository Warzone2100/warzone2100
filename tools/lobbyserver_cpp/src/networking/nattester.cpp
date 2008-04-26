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

#include "nattester.hpp"
#include <boost/bind.hpp>

NATTester::NATTester(boost::asio::ip::tcp::endpoint& endpoint_) :
    _finished(false),
    _reachable(false),
    _ios(),
    _socket(_ios),
    _timer(_ios)
{
    _socket.async_connect(endpoint_, boost::bind(&NATTester::handleConnect, this, boost::asio::placeholders::error));
}

bool NATTester::reachable(unsigned int timeout_seconds)
{
    if (!_finished)
    {
        _timer.expires_from_now(boost::posix_time::seconds(timeout_seconds));
        _timer.async_wait(boost::bind(&NATTester::handleTimeout, this));
        _ios.run();
    }

    return _reachable;
}

void NATTester::handleConnect(const boost::asio::error& err)
{
    _reachable = err ? false : true;
    if (!err)
    {
        _socket.close();
    }

    _finished = true;
}

void NATTester::handleTimeout()
{
    _reachable = false;
    _socket.close();
    _finished = true;
}
