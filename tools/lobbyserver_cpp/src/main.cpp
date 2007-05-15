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

#include <iostream>
#include <boost/thread/thread.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>

extern "C" {
#include <framework/frame.h>
#include <netplay/netplay.h>
}

const unsigned short lobbyPort = 9998;
const bool lobbyDev = true;

int start();

int main(int argc, char* argv[])
{
	//ThreadingTCPServer lobbyServer;

	//return lobbyServer(lobbyPort);

	try
	{
		return start();
	}
	catch (boost::asio::error& e)
	{
		std::cerr << "Boost::asio exception: " << e << std::endl;
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
	}

	return 1;
}

int start()
{
	boost::asio::io_service io_service;

	boost::asio::ip::tcp::acceptor acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v6(), lobbyPort));

	for (unsigned int count = 0; count != 10; ++count)
	{
		try
		{
			boost::asio::ip::tcp::socket socket(io_service);
			acceptor.accept(socket);

			std::cout << "Incoming connection from: " << socket.remote_endpoint() << "; on: " << socket.local_endpoint() << std::endl;

			std::cout << socket.remote_endpoint().address() << std::endl;

			boost::array<char, 5> buffer;

			size_t reply_length = boost::asio::read(socket, boost::asio::buffer(buffer, buffer.size() - 1));
			*(buffer.end() - 1) = 0;

			std::cout << "Received: " << buffer.data() << std::endl;

			boost::array<char, 5> cmdAddGame = {'a','d','d','g', 0};
			boost::array<char, 5> cmdListGames = {'l','i','s','t', 0};
			if (buffer == cmdAddGame)
			{
				// Debug
				if (lobbyDev)
					std::cout << "<- addg" << std::endl;

				boost::shared_ptr<GAMESTRUCT> newGameData(new GAMESTRUCT);

				reply_length = boost::asio::read(socket, boost::asio::buffer(newGameData.get(), sizeof(GAMESTRUCT)));

				if (lobbyDev)
					std::cout << "Game name: " << newGameData->name << "; host name: " << newGameData->desc.host << "; players/max: " << newGameData->desc.dwCurrentPlayers << "/" << newGameData->desc.dwMaxPlayers << std::endl;
			}
			else if (buffer == cmdListGames)
			{
				// Debug
				if (lobbyDev)
					std::cout << "<- list" << std::endl;
			}
		}
		catch (boost::asio::error& e)
		{
			if (!(e == boost::asio::error::eof))
				throw;

			std::cerr << "Premature EOF" << std::endl;
			--count;
			continue;
		}
	}

	return 0;
}
