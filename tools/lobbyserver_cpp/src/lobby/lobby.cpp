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
#include <boost/thread/recursive_mutex.hpp>
#include <boost/array.hpp>
#include <boost/utility.hpp>
#include "game.hpp"
#include <boost/bind.hpp>
#include <list>
#include "read_write_mutex.hpp"

#define lobbyDev true
extern boost::recursive_mutex cout_mutex;
extern boost::recursive_mutex cerr_mutex;

class GameLobby::impl : boost::noncopyable
{
    public:
        class gameLock : boost::noncopyable
        {
            public:
                inline gameLock(impl* Pimpl) :
                    pimpl(Pimpl),
                    iterValid(false)
                {
                }

                inline gameLock(impl* Pimpl, const GAMESTRUCT& game) :
                    pimpl(Pimpl),
                    iterValid(false)
                {
                    *this = game;
                }

                inline ~gameLock()
                {
                    clear();
                }

                inline gameLock& operator=(const GAMESTRUCT& game)
                {
                    // Check to see if the new game isn't the same as the old one
                    if (*this == game)
                        return *this;

                    // If we already have a game then remove it first
                    clear();

                    {
                        ReadWriteMutex::scoped_lock lock(pimpl->_mutex);

                        // Insert our game into the list
                        _iter = pimpl->_games.insert(pimpl->_games.begin(), game);
                        iterValid = true;
                    }

                    return *this;
                }

                inline void clear()
                {
                    if (iterValid)
                    {
                        ReadWriteMutex::scoped_lock lock(pimpl->_mutex);

                        pimpl->_games.erase(_iter);

                        iterValid = false;
                    }
                }

                inline bool operator==(const GAMESTRUCT& game)
                {
                    if (!iterValid)
                        return false;

                    ReadWriteMutex::scoped_readonlylock lock(pimpl->_mutex);

                    return *_iter == game;
                }
                
                inline bool operator!=(const GAMESTRUCT& game)
                {
                    return !(*this == game);
                }

            private:
                impl* pimpl;

                std::list<GAMESTRUCT>::iterator _iter;
                bool iterValid;
        };

        inline void printAllGames()
        {
            ReadWriteMutex::scoped_readonlylock lock(_mutex);

            for_each(_games.begin(), _games.end(), printGame);
        }

        inline void listGames(boost::shared_ptr<boost::asio::ip::tcp::socket> socket)
        {
            ReadWriteMutex::scoped_readonlylock lock(_mutex);

            const unsigned int gameCount = _games.size();

            boost::asio::write(*socket, boost::asio::buffer(&gameCount, sizeof(gameCount)));

            for_each(_games.begin(), _games.end(), boost::bind(sendGame, socket, _1));
        }

        static inline void sendGame(boost::shared_ptr<boost::asio::ip::tcp::socket> socket, const GAMESTRUCT& game)
        {
            boost::asio::write(*socket, boost::asio::buffer(&game, sizeof(game)));
        }

    private:
        ReadWriteMutex _mutex;
        std::list<GAMESTRUCT> _games;
};

GameLobby::GameLobby() :
    pimpl(new impl)
{
}

GameLobby::~GameLobby()
{
    delete pimpl;
}

void GameLobby::addGame(boost::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
    // Debug
    if (lobbyDev)
    {
        boost::recursive_mutex::scoped_lock lock(cout_mutex);
        std::cout << "<- addg" << std::endl;
    }

    impl::gameLock lobbiedGame(pimpl);

    for (bool firstRun = true;; firstRun = false)
    {
        GAMESTRUCT newGameData;
        try
        {
            boost::asio::read(*socket, boost::asio::buffer(&newGameData, sizeof(GAMESTRUCT)));

	    strncpy(newGameData.desc.host, socket->remote_endpoint().address().to_string().c_str(), sizeof(newGameData.desc.host));
	    newGameData.desc.host[sizeof(newGameData.desc.host) - 1] = 0;

            lobbiedGame = newGameData;
        }
        catch (boost::asio::error& e)
        {
            if (e == boost::asio::error::eof)
            {
                boost::recursive_mutex::scoped_lock lock(cerr_mutex);
                std::cerr << "EOF" << std::endl;
                return;
            }
            else if (e == boost::asio::error::connection_aborted)
            {
                boost::recursive_mutex::scoped_lock lock(cerr_mutex);
                std::cerr << "Connection aborted" << std::endl;
                return;
            }
            else
            {
                throw;
            }
        }

        // Debug
        if (lobbyDev && !firstRun)
        {
            boost::recursive_mutex::scoped_lock lock(cout_mutex);
            std::cout << "<- addg update" << std::endl;
        }

        // Debug
        if (lobbyDev)
            pimpl->printAllGames();
    }
}

void GameLobby::listGames(boost::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
    // Debug
    if (lobbyDev)
    {
        boost::recursive_mutex::scoped_lock lock(cout_mutex);
        std::cout << "<- list" << std::endl;
    }

    static const GAMESTRUCT dummyGame = {"Tha Dummy Game!!!!", {48, 0, "192.168.1.11", 8, 5, 14, 0, 0, 0}};

    impl::gameLock lobbiedGame(pimpl, dummyGame);

    pimpl->listGames(socket);
}

static inline void printCommandSafe(const boost::array<char, 5>& command)
{
    // This is the buffer we will print from, it is one byte longer as the command itself so that we can append a '\0' to it
    boost::array<char, sizeof(command) + 1> buffer;

    std::copy(command.begin(), command.end(), buffer.begin());
    // Set last char to '\0'
    *(buffer.end() - 1) = 0;

    boost::recursive_mutex::scoped_lock lock(cout_mutex);

    std::cout << "Received: \"" << buffer.data();

    // If there is a '\0' contained in the command, output it as the last character
    if (strlen(buffer.data()) < sizeof(command))
        std::cout << "\\0";

    std::cout << "\"" << std::endl;
}

void GameLobby::handleRequest(boost::shared_ptr<boost::asio::ip::tcp::socket> socket)
{
    try
    {
        if (lobbyDev)
        {
            boost::recursive_mutex::scoped_lock lock(cout_mutex);
            std::cout << "Incoming connection from: " << socket->remote_endpoint() << "; on: " << socket->local_endpoint() << std::endl;
        }

        boost::array<char, 5> buffer;

        boost::asio::read(*socket, boost::asio::buffer(buffer));

        if (std::equal(buffer.begin(), buffer.end(), "addg"))
        {
            addGame(socket);
        }
        else if (std::equal(buffer.begin(), buffer.end(), "list"))
        {
            listGames(socket);
        }
        else
        {
            printCommandSafe(buffer);
        }
    }
    catch (boost::asio::error& e)
    {
        boost::recursive_mutex::scoped_lock lock(cerr_mutex);
        std::cerr << "handleRequest: Boost::asio exception: " << e << std::endl;
    }
    catch (std::exception& e)
    {
        boost::recursive_mutex::scoped_lock lock(cerr_mutex);
        std::cerr << "handleRequest: Exception: " << e.what() << std::endl;
    }
}
