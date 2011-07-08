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
#include <QtCore/QtEndian>
#include <QtCore/QFile>

#include <QtNetwork/QSslConfiguration>

#include "netlobby.h"

namespace Lobby
{
	Client::Client()
	{
		// Set defaults
		gameId_ = 0;

		lastError_.code = LOBBY_NO_ERROR;

		isAuthenticated_ = false;
		useSSL_ = false;
		useAuth_ = false;
	}

	void Client::stop()
	{
		debug(LOG_LOBBY, "Stopping");

		// remove the game from the masterserver,
		delGame();
		freeError();

		// and disconnect.
		disconnect();

		delete socket_;
	}

	RETURN_CODES Client::addGame(char** result, const uint32_t port, const uint32_t maxPlayers,
								const char* description, const char* versionstring,
								const uint32_t game_version_major, const uint32_t game_version_minor,
								const bool isPrivate, const char* modlist,
								const char* mapname, const char* hostplayer)
	{
		bson kwargs[1];
		bson_buffer buffer[1];

		if (gameId_ != 0)
		{
			// Ignore server errors here.
			if (delGame() != LOBBY_NO_ERROR)
			{
				freeError();
			}
		}

		bson_buffer_init(buffer);
		bson_append_int(buffer, "port", port);
		bson_append_string(buffer, "description", description);
		bson_append_int(buffer, "currentPlayers", 1);
		bson_append_int(buffer, "maxPlayers", maxPlayers);
		bson_append_string(buffer, "multiVer", versionstring);
		bson_append_int(buffer, "wzVerMajor", game_version_major);
		bson_append_int(buffer, "wzVerMinor", game_version_minor);
		bson_append_bool(buffer, "isPrivate", isPrivate);
		bson_append_string(buffer, "modlist", modlist);
		bson_append_string(buffer, "mapname", mapname);
		bson_append_string(buffer, "hostplayer", hostplayer);

		bson_from_buffer(kwargs, buffer);

		if (call_("addGame", NULL, kwargs) != LOBBY_NO_ERROR)
		{
			bson_destroy(kwargs);
			return lastError_.code;
		}
		bson_destroy(kwargs);

		bson_iterator it;
		bson_iterator_init(&it, callResult_.result);

		bson_iterator_next(&it);
		if (bson_iterator_type(&it) == bson_long)
		{
			gameId_ = bson_iterator_long(&it);
		}
		else if (bson_iterator_type(&it) == bson_int)
		{
			gameId_ = (int64_t)bson_iterator_int(&it);
		}
		else
		{
			freeCallResult_();
			return setError_(INVALID_DATA, _("Received invalid addGame data."));
		}

		bson_iterator_next(&it);
		if (bson_iterator_type(&it) != bson_string)
		{
			freeCallResult_();
			return setError_(INVALID_DATA, _("Received invalid addGame data."));
		}
		asprintf(result, bson_iterator_string(&it));

		freeCallResult_();
		return LOBBY_NO_ERROR;
	}

	RETURN_CODES Client::delGame()
	{
		bson kwargs[1];
		bson_buffer buffer[1];

		if (gameId_ == 0)
		{
			return LOBBY_NO_ERROR;
		}

		bson_buffer_init(buffer);
		bson_append_int(buffer, "gameId", gameId_);
		bson_from_buffer(kwargs, buffer);

		if (call_("delGame", NULL, kwargs) != LOBBY_NO_ERROR)
		{
			// Ignore a server side error and unset the local game.
			gameId_ = 0;

			bson_destroy(kwargs);
			return lastError_.code;
		}
		bson_destroy(kwargs);

		gameId_ = 0;

		freeCallResult_();
		return LOBBY_NO_ERROR;
	}

	RETURN_CODES Client::addPlayer(const unsigned int index, const char* name,
									   const char* username, const char* session)
	{
		bson kwargs[1];
		bson_buffer buffer[1];

		if (gameId_ == 0 || gameId_ == -1)
		{
			return setError_(NO_GAME, _("Create a game first!"));
		}

		bson_buffer_init(buffer);
		bson_append_int(buffer, "gameId", gameId_);
		bson_append_int(buffer, "slot", index);
		bson_append_string(buffer, "name", name);
		bson_append_string(buffer, "username", username);
		bson_append_string(buffer, "session", session);
		bson_from_buffer(kwargs, buffer);

		if (call_("addPlayer", NULL, kwargs) != LOBBY_NO_ERROR)
		{
			bson_destroy(kwargs);
			return lastError_.code;
		}
		bson_destroy(kwargs);

		freeCallResult_();
		return LOBBY_NO_ERROR;
	}

	RETURN_CODES Client::delPlayer(const unsigned int index)
	{
		bson kwargs[1];
		bson_buffer buffer[1];

		if (gameId_ == 0 || gameId_ == -1)
		{
			return setError_(NO_GAME, _("Create a game first!"));
		}

		bson_buffer_init(buffer);
		bson_append_int(buffer, "gameId", gameId_);
		bson_append_int(buffer, "slot", index);
		bson_from_buffer(kwargs, buffer);

		if (call_("delPlayer", NULL, kwargs) != LOBBY_NO_ERROR)
		{
			bson_destroy(kwargs);
			return lastError_.code;
		}
		bson_destroy(kwargs);

		freeCallResult_();
		return LOBBY_NO_ERROR;
	}

	RETURN_CODES Client::updatePlayer(const unsigned int index, const char* name)
	{
		bson kwargs[1];
		bson_buffer buffer[1];

		if (gameId_ == 0 || gameId_ == -1)
		{
			return setError_(NO_GAME, _("Create a game first!"));
		}

		bson_buffer_init(buffer);
		bson_append_int(buffer, "gameId", gameId_);
		bson_append_int(buffer, "slot", index);
		bson_append_string(buffer, "name", name);
		bson_from_buffer(kwargs, buffer);

		if (call_("updatePlayer", NULL, kwargs) != LOBBY_NO_ERROR)
		{
			bson_destroy(kwargs);
			return lastError_.code;
		}
		bson_destroy(kwargs);

		freeCallResult_();
		return LOBBY_NO_ERROR;
	}

	RETURN_CODES Client::listGames(const int maxGames)
	{
		bson_iterator it, gameIt;
		const char * key;
		uint32_t gameCount = 0;
		GAME game;

		// Clear old games.
		games.clear();

		// Run "list" and retrieve its result
		if (call_("list", NULL, NULL) != LOBBY_NO_ERROR)
			return lastError_.code;

		//Print the result to stdout.
		//bson_print_raw(callResult_.result, 0);

		// Translate the result into GAMEs
		bson_iterator_init(&it, callResult_.result);
		while (bson_iterator_next(&it) && gameCount < maxGames)
		{
			// new Game
			if (bson_iterator_type(&it) == bson_object)
			{
				bson_iterator_init(&gameIt, bson_iterator_value(&it));
				while (bson_iterator_next(&gameIt))
				{
					key = bson_iterator_key(&gameIt);

					if (strcmp(key, "port") == 0)
					{
						game.port = (uint32_t)bson_iterator_int(&gameIt);
					}
					else if (strcmp(key, "host") == 0) // Generated at server side.
					{
						game.host = bson_iterator_string(&gameIt);
					}
					if (strcmp(key, "description") == 0)
					{
						game.description = bson_iterator_string(&gameIt);
					}
					else if (strcmp(key, "currentPlayers") == 0)
					{
						game.currentPlayers = bson_iterator_int(&gameIt);
					}
					else if (strcmp(key, "maxPlayers") == 0)
					{
						game.maxPlayers = bson_iterator_int(&gameIt);
					}
					else if (strcmp(key, "multiVer") == 0)
					{
						game.versionstring = bson_iterator_string(&gameIt);
					}
					else if (strcmp(key, "wzVerMajor") == 0)
					{
						game.game_version_major = (uint32_t)bson_iterator_int(&gameIt);
					}
					else if (strcmp(key, "wzVerMinor") == 0)
					{
						game.game_version_minor = (uint32_t)bson_iterator_int(&gameIt);
					}
					else if (strcmp(key, "isPrivate") == 0)
					{
						game.isPrivate = bson_iterator_bool(&gameIt);
					}
					else if (strcmp(key, "modlist") == 0)
					{
						game.modlist = bson_iterator_string(&gameIt);
					}
					else if (strcmp(key, "mapname") == 0)
					{
						game.mapname = bson_iterator_string(&gameIt);
					}
					else if (strcmp(key, "hostplayer") == 0)
					{
						game.hostplayer = bson_iterator_string(&gameIt);
					}
				}

				games.append(game);
				gameCount++;
			}
		}

		freeCallResult_();
		return LOBBY_NO_ERROR;
	}

	inline bool Client::isConnected()
	{
		return (socket_ && socket_->state() == QAbstractSocket::ConnectedState);
	}

	Client& Client::addCACertificate(const QString& path)
	{
#if defined(NO_SSL)
		debug(LOG_LOBBY, "Cannot add an SSL Certificate as SSL is not compiled in.");
		return *this;
#else
		debug(LOG_LOBBY, "Adding the CA certificate %s.", path.toUtf8().constData());

		QFile cafile(path);
		if (!cafile.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			debug(LOG_ERROR, "Cannot open the CA certificate %s!", path.toUtf8().constData());
			return *this;
		}

		QSslCertificate certificate(&cafile, QSsl::Pem);
		if (!certificate.isValid())
		{
			debug(LOG_ERROR, "Failed to load the CA certificate %s!", path.toUtf8().constData());
			return *this;
		}
		cafile.close();

		debug(LOG_LOBBY, "Cert common name: %s", certificate.subjectInfo(QSslCertificate::CommonName).toUtf8().constData());

		cacerts_.append(certificate);

		return *this;
#endif
	}

	RETURN_CODES Client::connect()
	{
		if (isConnected())
		{
			return LOBBY_NO_ERROR;
		}

		isAuthenticated_ = false;

		callId_ = 0;

#if defined(NO_SSL)
		socket_ = new QTcpSocket();
		socket_->connectToHost(host_, port_);
#else
		socket_ = new QSslSocket();
		// Connect
		if (useSSL_ == false)
		{
			debug(LOG_LOBBY, "Connecting to \"[%s]:%d\".", host_.toUtf8().constData(), port_);
			socket_->connectToHost(host_, port_);
		}
		else
		{
			debug(LOG_LOBBY, "Connecting to \"[%s]:%d\" with SSL enabled.", host_.toUtf8().constData(), port_);
			socket_->setCaCertificates(cacerts_);
			socket_->connectToHostEncrypted(host_, port_);
		}
#endif

#if defined(NO_SSL)
		if (!socket_->waitForConnected())
		{
#else
		if ((useSSL_ && !socket_->waitForEncrypted())
			|| (!useSSL_ && !socket_->waitForConnected()))
		{
#endif
			debug(LOG_ERROR, "Cannot connect to lobby \"[%s]:%d\": %s.",
				  host_.toUtf8().constData(), port_, socket_->errorString().toUtf8().constData());
			return setError_(TRANSPORT_ERROR, "");
		}

		// Build the initial version command.
		uchar buf[sizeof(qint32)] = { "\0" };
		qToBigEndian<qint32>(PROTOCOL, buf);
		QByteArray version;
		version.append("version", sizeof("version"));
		version.append((char *)buf, sizeof(buf));

		// Send Version command
		if (socket_->write(version) == -1)
		{
			debug(LOG_ERROR, "Cannot send version string to lobby \"%s:%d\": %s",
				  host_.toUtf8().constData(), port_, socket_->errorString().toUtf8().constData());
			return setError_(TRANSPORT_ERROR, "");
		}

		// At last try to login
		return login("");
	}

	RETURN_CODES Client::login(const QString& password)
	{
		bson kwargs[1];
		bson_buffer buffer[1];
		const char * key;

		if (isAuthenticated_ == true)
		{
			return LOBBY_NO_ERROR;
		}
		else if (useAuth_ == false)
		{
			debug(LOG_LOBBY, "Authentication is disabled.");
			return LOBBY_NO_ERROR;
		}

		bson_buffer_init(buffer);
		bson_append_string(buffer, "username", user_.toUtf8().constData());

		if (!token_.isEmpty())
		{
			bson_append_string(buffer, "token", token_.toUtf8().constData());
		}
		else if (!password.isEmpty())
		{
			token_.clear();
			bson_append_string(buffer, "password", password.toUtf8().constData());
		}
		else
		{
			bson_buffer_destroy(buffer);

			debug(LOG_LOBBY, "Not trying to login no password/token supplied.");

			// Do not return an error for internal use.
			return LOBBY_NO_ERROR;
		}

		bson_from_buffer(kwargs, buffer);

		if (call_("login", NULL, kwargs) != LOBBY_NO_ERROR)
		{
			// Reset login credentials on a wrong login
			if (lastError_.code == WRONG_LOGIN)
			{
				debug(LOG_LOBBY, "Login failed!");
				token_.clear();
				session_.clear();
				isAuthenticated_ = false;
			}
			return lastError_.code;
		}
		bson_destroy(kwargs);

		token_.clear();
		session_.clear();

		bson_iterator it;
		bson_iterator_init(&it, callResult_.result);
		while (bson_iterator_next(&it))
		{
			key = bson_iterator_key(&it);

			if (strcmp(key, "token") == 0)
			{
				token_ = bson_iterator_string(&it);
			}
			else if (strcmp(key, "session") == 0)
			{
				session_ = bson_iterator_string(&it);
			}
		}

		if (token_.isEmpty() || session_.isEmpty())
		{
			debug(LOG_LOBBY, "Login failed!");
			freeCallResult_();
			return setError_(INVALID_DATA, _("Received invalid login data."));
		}

		debug(LOG_LOBBY, "Received Session \"%s\"", session_.toUtf8().constData());

		isAuthenticated_ = true;

		freeCallResult_();
		return LOBBY_NO_ERROR;
	}

	RETURN_CODES Client::logout()
	{
		// Remove a maybe hosted game.
		if (delGame() != LOBBY_NO_ERROR)
		{
			freeError();
		}

		// Tell the lobby that we want to logout.
		if (call_("logout", NULL, NULL) != LOBBY_NO_ERROR)
		{
			// Clear auth data.
			useAuth_ = false;
			user_.clear();
			token_.clear();
			session_.clear();
			return lastError_.code;
		}

		// Clear auth data.
		useAuth_ = false;
		user_.clear();
		token_.clear();
		session_.clear();

		freeCallResult_();
		return LOBBY_NO_ERROR;
	}

	bool Client::disconnect()
	{
		if (!isConnected())
		{
			return true;
		}

		socket_->close();

		// Rest call id.
		callId_ = 0;

		// clear/free up games,
		games.clear();

		// clear auth data,
		session_.clear();
		isAuthenticated_ = false;

		return true;
	}

	RETURN_CODES Client::call_(const char* command, const bson* args, const bson* kwargs)
	{
		bson bson[1];
		bson_buffer buffer[1];

		// Connect to the lobby
		if (isConnected() != true && connect() != LOBBY_NO_ERROR)
		{
			return lastError_.code;
		}

		callId_ += 1;

		debug(LOG_LOBBY, "Calling \"%s\" on the lobby", command);

		bson_buffer_init(buffer);
		bson_append_start_array(buffer, "call");
		{
			// Command
			bson_append_string(buffer, "0", command);

			// Arguments
			if (args == NULL)
			{
				// Add a empty "array"/"list"
				bson_append_start_array(buffer, "1");
				bson_append_finish_object(buffer);
			}
			else
			{
				bson_append_bson(buffer, "1", args);
			}

			// Keyword Arguments
			if (kwargs == NULL)
			{
				// Add a empty "object"/"dict"
				bson_append_start_object(buffer, "2");
				bson_append_finish_object(buffer);
			}
			else
			{
				// Keyword arguments
				bson_append_bson(buffer, "2", kwargs);
			}

			// Call id
			bson_append_int(buffer, "3", callId_);
		}
		bson_append_finish_object(buffer);
		bson_from_buffer(bson, buffer);

		QByteArray block;
		QDataStream out(&block, QIODevice::WriteOnly);
		out.setVersion(QDataStream::Qt_4_0);
		out.setByteOrder(QDataStream::BigEndian);
		out.writeBytes(bson->data, bson_size(bson));
		bson_destroy(bson);

		if (socket_->write(block) == -1)
		{
			debug(LOG_ERROR, "Failed sending command \"%s\" to the lobby: %s.",
					command, socket_->errorString().toUtf8().constData());
			return setError_(TRANSPORT_ERROR, _("Failed to communicate with the lobby."));
		}

		quint32 blockSize;
		QDataStream in(socket_);
		in.setVersion(QDataStream::Qt_4_0);
		in.setByteOrder(QDataStream::BigEndian);
		while (socket_->bytesAvailable() < sizeof(quint16))
		{
			if (!socket_->waitForReadyRead())
			{
				debug(LOG_ERROR, "Failed reading the results size for \"%s\" from the lobby: %s.",
						command, socket_->errorString().toUtf8().constData());
				return setError_(TRANSPORT_ERROR, _("Failed to communicate with the lobby."));
			}

			in >> blockSize;

			while (socket_->bytesAvailable() < blockSize)
			{
				if (!socket_->waitForReadyRead())
				{
					debug(LOG_ERROR, "Failed reading the result for \"%s\" from the lobby: %s.",
							command, socket_->errorString().toUtf8().constData());
					return setError_(TRANSPORT_ERROR, _("Failed to communicate with the lobby."));
				}
			}
		}
		callResult_.buffer = new char[blockSize];
		in.readRawData(callResult_.buffer, blockSize);

		// Get the first object (bson_array)
		bson_iterator it;
		bson_iterator_init(&it, callResult_.buffer);
		bson_iterator_next(&it);
		if (bson_iterator_type(&it) != bson_array)
		{
			debug(LOG_ERROR, "Received invalid bson data (no array first): %d.", bson_iterator_type(&it));

			delete callResult_.buffer;
			return setError_(INVALID_DATA, _("Failed to communicate with the lobby."));
		}
		bson_iterator_init(&it, bson_iterator_value(&it));
		bson_iterator_next(&it);

		if (bson_iterator_type(&it) != bson_int)
		{
			debug(LOG_ERROR, "Received invalid bson data (no int first): %d.", bson_iterator_type(&it));

			delete callResult_.buffer;
			return setError_(INVALID_DATA, _("Failed to communicate with the lobby."));
		}

		callResult_.code = (RETURN_CODES)bson_iterator_int(&it);

		bson_iterator_next(&it);
		bson_type type = bson_iterator_type(&it);
		if (type == bson_string)
		{
			callResult_.result = bson_iterator_string(&it);
		}
		else if (type == bson_object || type == bson_array)
		{
			callResult_.result = bson_iterator_value(&it);
		}

		if (callResult_.code != LOBBY_NO_ERROR)
		{
			debug(LOG_LOBBY, "Received: server error %d: %s", callResult_.code, callResult_.result);

			setError_(callResult_.code, _("Got server error: %s"), callResult_.result);
			freeCallResult_();
			return lastError_.code;
		}

		return LOBBY_NO_ERROR;
	}

	RETURN_CODES Client::setError_(const RETURN_CODES code, const char* message, ...)
	{
        char buffer[256];      

        lastError_.code = code;

        va_list ap;
        va_start(ap, message);
            vsprintf(buffer, message, ap);
            lastError_.message = buffer;
        va_end(ap);

        return code;
	}

} // namespace Lobby
