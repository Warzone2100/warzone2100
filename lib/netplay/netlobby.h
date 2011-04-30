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

#include <QtNetwork>
#include "bson/bson.h"

namespace Lobby
{
	const qint32 PROTOCOL = 4;

	/* NOTE: You also need to change this value in the
	 * masterservers settings - session_size!
	 */
	const int SESSION_SIZE = 16+1;

	/* We limit usernames here to 40 chars,
	 * while the forums allows usernames with up to 255 characters.
	 */
	const int USERNAME_SIZE = 40+1;

	enum RETURN_CODES
	{
		NO_ERROR				=	0,

		// Copied from XMLRPC for socketrpc
		PARSE_ERROR				=	-32700,
		SERVER_ERROR			=	-32600,
		APPLICATION_ERROR		=	-32500,
		TRANSPORT_ERROR			=	-32300,
		// Specific errors.
		UNSUPPORTED_ENCODING	=	-32701,
		METHOD_NOT_FOUND		=	-32601,

		// Custom error codes.
		INVALID_DATA			= 	-500,
		LOGIN_REQUIRED			=	-405,
		WRONG_LOGIN 			=	-404,
		NOT_ACCEPTABLE 			=	-403,
		NO_GAME					=	-402,
	};

	// FIXME: Not sure if std::string is a good idea here,
	// 		  as multiint is passing them to iV_DrawText.
	struct GAME
	{
		uint32_t port;						///< Port hosting on.
		std::string host;					///< IPv4, IPv6 or DNS Name of the host.
		std::string description;			///< Game Description.
		uint32_t currentPlayers;			///< Number of joined players.
		uint32_t maxPlayers;				///< Maximum number of players.
		std::string versionstring;			///< Version string.
		uint32_t game_version_major;		///< Minor NETCODE version.
		uint32_t game_version_minor;		///< Major NETCODE version.
		bool isPrivate;						///< Password protected?
		std::string modlist;				///< display string for mods.
		std::string  mapname;				///< name of map hosted.
		std::string  hostplayer;			///< hosts playername.
	};

	struct ERROR
	{
		RETURN_CODES code;
		QString message;
	};

	struct CALL_RESULT
	{
		RETURN_CODES code;
		char* buffer;
		const char* result;
	};

	class Client {
		public:
			std::vector<GAME> games;

			Client();
			void stop();

			RETURN_CODES connect();
			bool disconnect();
			bool isConnected();
			RETURN_CODES login(const std::string& password);

			RETURN_CODES addGame(char** result, const uint32_t port, const uint32_t maxPlayers,
								const char* description, const char* versionstring,
								const uint32_t game_version_major, const uint32_t game_version_minor,
								const bool isPrivate, const char* modlist,
								const char* mapname, const char* hostplayer);

			RETURN_CODES delGame();
			RETURN_CODES addPlayer(const unsigned int index, const char* name, const char* username, const char* session);
			RETURN_CODES delPlayer(const unsigned int index);
			RETURN_CODES updatePlayer(const unsigned int index, const char* name);
			RETURN_CODES listGames(const int maxGames);

			Client& setHost(const QString& host) { host_ = host; return *this; }
			QString getHost() const { return host_; }

			Client& setPort(const quint16& port) { port_ = port; return *this; }
			quint16 getPort() { return port_; }

			bool isAuthenticated() { return isAuthenticated_; }

			Client& setUser(const std::string& user) { user_ = user; return *this; }
			std::string getUser() const { return user_; }

			Client& setToken(const std::string& token) { token_ = token; return *this; }
			std::string getToken() { return token_; }

			std::string getSession() const { return session_; }

			ERROR* getError() { return &lastError_; }

			void freeError()
			{
				lastError_.code = NO_ERROR;
				lastError_.message.clear();
			}

		private:
			int64_t gameId_;

			uint32_t callId_;

			QString host_;
			quint16 port_;

			std::string user_;
			std::string token_;
			std::string session_;

			QTcpSocket socket_;

			bool isAuthenticated_;

			ERROR lastError_;
			CALL_RESULT callResult_;

			RETURN_CODES call_(const char* command, const bson* args, const bson* kwargs);

			void freeCallResult_()
			{
				callResult_.code = NO_ERROR;
				callResult_.result = NULL;
				delete callResult_.buffer;
			}

			RETURN_CODES setError_(const RETURN_CODES code, const char * message, ...);
	}; // class Client

} // namespace Lobby

#endif // #ifndef _netlobby_h
