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

#include "requesthandler.hpp"
#include <iostream>
#include <boost/thread/recursive_mutex.hpp>
#include "lobby/lobby.hpp"
#include "lobby/game.hpp"
#include <algorithm>
#include <boost/bind.hpp>

extern boost::recursive_mutex cout_mutex;
extern boost::recursive_mutex cerr_mutex;
#define lobbyDev true


class lobbyprotocol::requestHandler::impl
{
    public:
        impl(boost::shared_ptr<GameLobby> lobby) :
            _lobby(lobby)
        {
        }

        void addGame(boost::shared_ptr<boost::asio::ip::tcp::socket> socket)
        {
            // Debug
            if (lobbyDev)
            {
                boost::recursive_mutex::scoped_lock lock(cout_mutex);
                std::cout << "<- addg" << std::endl;
            }

            // Reserve a lock to attach the game of this session to, its destructor
            // will automatically remove the game if we leave this scope for
            // whatever reason (usually loss of connection)
            GameLobby::gameLock lobbiedGame(*_lobby);

            for (bool firstRun = true;; firstRun = false)
            {
                GAMESTRUCT newGameData;
                try
                {
                    // Attempt to receive the game struct
                    boost::asio::read(*socket, boost::asio::buffer(&newGameData, sizeof(GAMESTRUCT)));

                    // Update the game struct's host (usually not set by client, so we'll have to do this)
                    strncpy(newGameData.desc.host, socket->remote_endpoint().address().to_string().c_str(), sizeof(newGameData.desc.host));
                    newGameData.desc.host[sizeof(newGameData.desc.host) - 1] = 0;

                    // Attach this game to our GameLobby lock, this'll automatically add the game to the lobby list
                    lobbiedGame = newGameData;
                }
                catch (boost::asio::error& e)
                {
                    if (e == boost::asio::error::eof)
                    {
                        boost::recursive_mutex::scoped_lock lock(cerr_mutex);
                        std::cerr << "Connection terminated" << std::endl;
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

                // Debug, dump the current list of games on screen
                if (lobbyDev)
                    std::for_each(_lobby->begin(), _lobby->end(), printGame);
            }
        }

        static inline void sendGame(boost::shared_ptr<boost::asio::ip::tcp::socket> socket, const GAMESTRUCT& game)
        {
            boost::asio::write(*socket, boost::asio::buffer(&game, sizeof(game)));
        }

        void listGames(boost::shared_ptr<boost::asio::ip::tcp::socket> socket)
        {
            GameLobby::const_iterator beginIter(_lobby->begin()), endIter(_lobby->end());
            const unsigned int gameCount = endIter - beginIter;

            // Debug
            if (lobbyDev)
            {
                boost::recursive_mutex::scoped_lock lock(cout_mutex);
                std::cout << "<- list"
                          << "  \\- I know " << gameCount << " games." << std::endl;
            }

            // First write out a little endian unsigned 32bit long integer as game count
            boost::asio::write(*socket, boost::asio::buffer(&gameCount, sizeof(gameCount)));
            // Now send all members of the game list to the client
            std::for_each(beginIter, endIter, boost::bind(sendGame, socket, _1));
        }

        void handleRequest(boost::shared_ptr<boost::asio::ip::tcp::socket> socket)
        {
            try
            {
                if (lobbyDev)
                {
                    boost::recursive_mutex::scoped_lock lock(cout_mutex);
                    std::cout << "Incoming connection from: " << socket->remote_endpoint() << "; on: " << socket->local_endpoint() << std::endl;
                }

                // Read out the method/command
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
                    // No command recognized, just dump it on screen
                    boost::recursive_mutex::scoped_lock lock(cerr_mutex);
                    std::cerr << "Unrecognized command: \"" << commandToString(buffer) << "\"" << std::endl;
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

        static inline std::string commandToString(const boost::array<char, 5>& command)
        {
            // This is the buffer we will copy from, it is one byte longer as the command itself so that we can append a '\0' to it
            boost::array<char, sizeof(command) + 1> buffer;

            std::copy(command.begin(), command.end(), buffer.begin());
            // Set last char to '\0' for safety
            buffer.back() = 0;

            std::string tmpString(buffer.data());

            // If there is a '\0' contained in the command, output it as the last character
            if (strlen(buffer.data()) < sizeof(command))
                tmpString += "\\0";

            return tmpString;
        }

    private:
        boost::shared_ptr<GameLobby> _lobby;
};

namespace lobbyprotocol
{
    requestHandler::requestHandler(boost::shared_ptr<GameLobby> lobby) :
        pimpl(new impl(lobby))
    {
    }

    requestHandler::~requestHandler()
    {
        delete pimpl;
    }

    void requestHandler::operator()(boost::shared_ptr<boost::asio::ip::tcp::socket> socket)
    {
        pimpl->handleRequest(socket);
    }
}
