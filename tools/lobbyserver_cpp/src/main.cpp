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

std::string str_repeat(const std::string& src, unsigned int count)
{
	std::string tmp;
	tmp.reserve(src.length() * count);
	for (unsigned int i = 0; i != count; ++i)
	{
		tmp += src;
	}

	return tmp;
}

template <class T>
inline std::string to_string (const T& t)
{
    std::stringstream ss;
    ss << t;
    return ss.str();
}

void printGame(boost::shared_ptr<GAMESTRUCT> game)
{
	std::string gameName(game->name);
	std::string gameSize(to_string(game->desc.dwSize));
	std::string gameFlags(to_string(game->desc.dwFlags));
	std::string gameHost(game->desc.host);
	std::string gameMaxPlayers(to_string(game->desc.dwMaxPlayers));
	std::string gameCurrentPlayers(to_string(game->desc.dwCurrentPlayers));
	std::string gameUser1(to_string(game->desc.dwUser1));
	std::string gameUser2(to_string(game->desc.dwUser2));
	std::string gameUser3(to_string(game->desc.dwUser3));
	std::string gameUser4(to_string(game->desc.dwUser4));

	unsigned int targetLength = std::max(std::max(std::max(std::max(std::max(std::max(std::max(std::max(std::max(gameName.length() + 2, gameSize.length()), gameFlags.length()), gameHost.length() + 2), gameMaxPlayers.length()), gameCurrentPlayers.length()), gameUser1.length()), gameUser2.length()), gameUser3.length()), gameUser4.length());

	std::cout << "Game info:\n"
	          << "+------------------" << str_repeat("-", targetLength) << "-+\n"
	          << "| Name           | \"" << gameName << "\"" << str_repeat(" ", targetLength - gameName.length() - 2) << " |\n"
	          << "| Size           | " << str_repeat(" ", targetLength - gameSize.length()) << gameSize << " |\n"
	          << "| Flags          | " << str_repeat(" ", targetLength - gameFlags.length()) << gameFlags << " |\n"
	          << "| Host           | \"" << gameHost << "\"" << str_repeat(" ", targetLength - gameHost.length() - 2) << " |\n"
	          << "| MaxPlayers     | " << str_repeat(" ", targetLength - gameMaxPlayers.length()) << gameMaxPlayers << " |\n"
	          << "| CurrentPlayers | " << str_repeat(" ", targetLength - gameCurrentPlayers.length()) << gameCurrentPlayers << " |\n"
	          << "| User1          | " << str_repeat(" ", targetLength - gameUser1.length()) << gameUser1 << " |\n"
	          << "| User2          | " << str_repeat(" ", targetLength - gameUser2.length()) << gameUser2 << " |\n"
	          << "| User3          | " << str_repeat(" ", targetLength - gameUser3.length()) << gameUser3 << " |\n"
	          << "| User4          | " << str_repeat(" ", targetLength - gameUser4.length()) << gameUser4 << " |\n"
	          << "+------------------" << str_repeat("-", targetLength) << "-+" << std::endl;
}

int start()
{
	boost::asio::io_service io_service;

	boost::asio::ip::tcp::acceptor acceptor(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), lobbyPort));

	for (unsigned int count = 0; count != 10; ++count)
	{
		try
		{
			boost::asio::ip::tcp::socket socket(io_service);
			acceptor.accept(socket);

			std::cout << "Incoming connection from: " << socket.remote_endpoint() << "; on: " << socket.local_endpoint() << std::endl;

			std::cout << socket.remote_endpoint().address() << std::endl;

			boost::array<char, 5> buffer;

			size_t reply_length = boost::asio::read(socket, boost::asio::buffer(buffer));

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
					printGame(newGameData);
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
