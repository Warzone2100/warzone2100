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

static inline std::string str_repeat(const std::string& src, unsigned int count)
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
static inline std::string to_string (const T& t)
{
    std::stringstream ss;
    ss << t;
    return ss.str();
}

static inline void printGame(const GAMESTRUCT& game)
{
    std::string gameName(game.name);
    std::string gameSize(to_string(game.desc.dwSize));
    std::string gameFlags(to_string(game.desc.dwFlags));
    std::string gameHost(game.desc.host);
    std::string gameMaxPlayers(to_string(game.desc.dwMaxPlayers));
    std::string gameCurrentPlayers(to_string(game.desc.dwCurrentPlayers));
    std::string gameUser1(to_string(game.desc.dwUser1));
    std::string gameUser2(to_string(game.desc.dwUser2));
    std::string gameUser3(to_string(game.desc.dwUser3));
    std::string gameUser4(to_string(game.desc.dwUser4));

    unsigned int targetLength = std::max(std::max(std::max(std::max(std::max(std::max(std::max(std::max(std::max(gameName.length() + 2, gameSize.length()), gameFlags.length()), gameHost.length() + 2), gameMaxPlayers.length()), gameCurrentPlayers.length()), gameUser1.length()), gameUser2.length()), gameUser3.length()), gameUser4.length());

    boost::recursive_mutex::scoped_lock lock(cout_mutex);

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
