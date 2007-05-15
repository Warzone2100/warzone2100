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

#include "tcp_server.hpp"
#include <boost/bind.hpp>

class TCPServer::impl
{
    public:
        impl(boost::shared_ptr<boost::asio::io_service> io_service, const connectionHandler& handler) :
            _io_service(io_service),
            _handler(handler)
        {
        }

        void listen(boost::asio::ip::tcp::endpoint endpoint)
        {
            boost::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor(new boost::asio::ip::tcp::acceptor(*_io_service, endpoint.protocol()));
            acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
            acceptor->bind(endpoint);
            acceptor->listen();

            startAccept(acceptor);
        }

        void startAccept(boost::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor)
        {
            boost::shared_ptr<boost::asio::ip::tcp::socket> socket(new boost::asio::ip::tcp::socket(*_io_service));

            acceptor->async_accept(*socket,
              boost::bind(&TCPServer::impl::handleAccept, this, acceptor, socket, boost::asio::placeholders::error));
        }

        void handleAccept(boost::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor,
                          boost::shared_ptr<boost::asio::ip::tcp::socket> socket,
                          const boost::asio::error& error)
        {
            if (!error)
            {
                _handler(socket);
                startAccept(acceptor);
            }
            else
            {
                throw;
            }
        }

    private:
        // Only purpose of this pointer is to make sure the io_service
        // object has lifetime of at least equal to the acceptors'.
        const boost::shared_ptr<boost::asio::io_service> _io_service;

        const connectionHandler _handler;
};

TCPServer::TCPServer(boost::shared_ptr<boost::asio::io_service> io_service, const connectionHandler& handler) :
    pimpl(new impl(io_service, handler))
{
}

TCPServer::~TCPServer()
{
    delete pimpl;
}

void TCPServer::listen(unsigned short port)
{
    // Listen on the specified port for incoming connections via IPv4
    pimpl->listen(boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));

    // Listen on the specified port for incoming connections via IPv6
    pimpl->listen(boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v6(), port));
}
