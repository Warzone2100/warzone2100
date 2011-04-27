/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2011  Warzone 2100 Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef _netlobby_h
#define _netlobby_h

#include <vector>
#include <string>

#include "lib/framework/frame.h"
#include "lib/framework/string_ext.h"
#include "netsocket.h"
#include "bson/bson.h"

#define LOBBY_VERSION 4

/* NOTE: You also need to change this value in the
 * masterservers settings - session_size!
 */
#define LOBBY_SESSION_SIZE 16+1

/* We limit usernames here to 40 chars,
 * while the forums allow usernames with up to 255 characters.
 */
#define LOBBY_USERNAME_SIZE 40+1

enum LOBBY_ERROR
{
	LOBBY_NO_ERROR				=	0,

	// Copied from XMLRPC for socketrpc
	LOBBY_PARSE_ERROR			=	-32700,
	LOBBY_SERVER_ERROR			=	-32600,
	LOBBY_APPLICATION_ERROR		=	-32500,
	LOBBY_TRANSPORT_ERROR		=	-32300,

	// Specific errors.
	LOBBY_UNSUPPORTED_ENCODING	=	-32701,
	LOBBY_METHOD_NOT_FOUND		=	-32601,

	// Custom error codes.
	LOBBY_INVALID_DATA			= 	-500,
	LOBBY_LOGIN_REQUIRED		=	-405,
	LOBBY_WRONG_LOGIN 			=	-404,
	LOBBY_NOT_ACCEPTABLE 		=	-403,
	LOBBY_NO_GAME				=	-402,
};

// FIXME: Not sure if std::string is a good idea here,
// 		  as multitint is passing them direct to iV_DrawText.
struct LOBBY_GAME
{
	uint32_t port;							///< Port hosting on.
	std::string host;		///< IPv4, IPv6 or DNS Name of the host.
	std::string description;			///< Game Description.
	uint32_t currentPlayers;				///< Number of joined players.
	uint32_t maxPlayers;					///< Maximum number of players.
	std::string versionstring;	///< Version string.
	uint32_t game_version_major;			///< Minor NETCODE version.
	uint32_t game_version_minor;			///< Major NETCODE version.
	bool isPrivate;							///< Password protected?
	std::string modlist;		///< display string for mods.
	std::string  mapname;			///< name of map hosted.
	std::string  hostplayer;	///< hosts playername.
};

struct lobbyError
{
	LOBBY_ERROR code;
	char* message;
};

struct lobbyCallResult
{
	LOBBY_ERROR code;
	char* buffer;
	const char* result;
};

class LobbyClient {
	public:
		std::vector<LOBBY_GAME> games;

		LobbyClient();
		void stop();

		LOBBY_ERROR connect();
		bool disconnect();
		bool isConnected();
		LOBBY_ERROR login(const std::string& password);

		LOBBY_ERROR addGame(char** result, uint32_t port, uint32_t maxPlayers,
						const char* description, const char* versionstring,
						uint32_t gameversion__major, uint32_t gameversion__minor,
						bool isPassworded, const char* modlist,
						const char* mapname, const char* hostplayer);

		LOBBY_ERROR delGame();
		LOBBY_ERROR addPlayer(unsigned int index, const char* name, const char* username, const char* session);
		LOBBY_ERROR delPlayer(unsigned int index);
		LOBBY_ERROR updatePlayer(unsigned int index, const char* name);
		LOBBY_ERROR listGames(int maxGames);

		LobbyClient& setHost(const std::string& host) { host_ = host; return *this; }
		std::string getHost() const { return host_; }

		LobbyClient& setPort(const uint32_t& port) { port_ = port; return *this; }
		uint32_t getPort() { return port_; }

		bool isAuthenticated() { return isAuthenticated_; }

		LobbyClient& setUser(const std::string& user) { user_ = user; return *this; }
		std::string getUser() const { return user_; }

		LobbyClient& setToken(const std::string& token) { token_ = token; return *this; }
		std::string getToken() { return token_; }

		std::string getSession() const { return session_; }

		lobbyError* getError() { return &lastError_; }

		void freeError()
		{
			if (lastError_.code == LOBBY_NO_ERROR)
			{
				return;
			}

			lastError_.code = LOBBY_NO_ERROR;
			free(lastError_.message);
			lastError_.message = NULL;
		}

	private:
		int64_t gameId_;

		uint32_t callId_;

		std::string host_;
		uint32_t port_;

		std::string user_;
		std::string token_;
		std::string session_;

		Socket* socket_;

		bool isAuthenticated_;

		lobbyError lastError_;
		lobbyCallResult callResult_;

		LOBBY_ERROR call_(const char* command, bson* args, bson* kwargs);

		void freeCallResult_()
		{
			callResult_.code = LOBBY_NO_ERROR;
			callResult_.result = NULL;
			free(callResult_.buffer);
			callResult_.buffer = NULL;
		}

		LOBBY_ERROR setError_(const LOBBY_ERROR code, char * message, ...);
};

#endif // #ifndef _netlobby_h

extern LobbyClient lobbyclient;
