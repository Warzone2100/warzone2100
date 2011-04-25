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

#include "netlobby.h"

/**
 * Variables
 */
LobbyClient lobbyclient;

LobbyClient::LobbyClient()
{
	// Set defaults
	callId_ = 1;

	gameId_ = 0;
	socket_ = NULL;

	lastError_.code = LOBBY_NO_ERROR;
	lastError_.message = NULL;
}

void LobbyClient::stop()
{
	// remove the game from the masterserver.
	lobbyclient.delGame();
	lobbyclient.freeError();

	// Clear/free up games.
	games.clear();

	lobbyclient.disconnect();
}

LOBBY_ERROR LobbyClient::addGame(char** result, uint32_t port, uint32_t maxPlayers,
							const char* description, const char* versionstring,
							uint32_t game_version_major, uint32_t game_version_minor,
							bool isPrivate, const char* modlist,
							const char* mapname, const char* hostplayer)
{
	bson kwargs[1];
	bson_buffer buffer[1];

	if (gameId_ != 0)
		return LOBBY_NO_ERROR;

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
		// Prevents "addGame" until "delGame" gets called.
		gameId_ = -1;
		return lastError_.code;
	}
	bson_destroy(kwargs);

	if (callResult_.code != LOBBY_NO_ERROR)
	{
		debug(LOG_ERROR, "Received: server error %d: %s", callResult_.code, callResult_.result);

		// Prevents "addGame" until "delGame" gets called.
		gameId_ = -1;

		setError_(callResult_.code, _("Got server error %d!"), callResult_.code);
		freeCallResult_();
		return lastError_.code;
	}

	bson_iterator it;
	bson_iterator_init(&it, callResult_.result);

	bson_iterator_next(&it);
	if (bson_iterator_type(&it) != bson_int)
	{
		freeCallResult_();
		return setError_(LOBBY_INVALID_DATA, _("Received invalid addGame data."));
	}
	gameId_ = bson_iterator_int(&it);

	bson_iterator_next(&it);
	if (bson_iterator_type(&it) != bson_string)
	{
		freeCallResult_();
		return setError_(LOBBY_INVALID_DATA, _("Received invalid addGame data."));
	}
	asprintf(result, bson_iterator_string(&it));

	freeCallResult_();
	return LOBBY_NO_ERROR;
}

LOBBY_ERROR LobbyClient::delGame()
{
	bson kwargs[1];
	bson_buffer buffer[1];

	if (gameId_ == 0)
	{
		return LOBBY_NO_ERROR;
	}
	else if (gameId_ == -1)
	{
		// Clear an error.
		gameId_ = 0;
		return LOBBY_NO_ERROR;
	}

	bson_buffer_init(buffer);
	bson_append_int(buffer, "gameId", gameId_);
	bson_from_buffer(kwargs, buffer);

	if (call_("delGame", NULL, kwargs) != LOBBY_NO_ERROR)
		return lastError_.code;
	bson_destroy(kwargs);

	if (callResult_.code != LOBBY_NO_ERROR)
	{
		debug(LOG_ERROR, "Received: server error %d: %s.", callResult_.code, callResult_.result);
		setError_(callResult_.code, _("Got server error (%d)."), callResult_.code);
		freeCallResult_();
		return lastError_.code;

	}

	gameId_ = 0;

	freeCallResult_();
	return LOBBY_NO_ERROR;
}

LOBBY_ERROR LobbyClient::addPlayer(unsigned int index, const char* name, const char* ipaddress)
{
	bson kwargs[1];
	bson_buffer buffer[1];

	if (gameId_ == 0 || gameId_ == -1)
		return setError_(LOBBY_NO_GAME, _("Create a game first!"));

	bson_buffer_init(buffer);
	bson_append_int(buffer, "gameId", gameId_);
	bson_append_int(buffer, "slot", index);
	bson_append_string(buffer, "name", name);
	bson_append_string(buffer, "ipaddress", ipaddress);
	bson_from_buffer(kwargs, buffer);

	if (call_("addPlayer", NULL, kwargs) != LOBBY_NO_ERROR)
		return lastError_.code;
	bson_destroy(kwargs);

	if (callResult_.code != LOBBY_NO_ERROR)
	{
		debug(LOG_ERROR, "Received: server error %d: %s.", callResult_.code, callResult_.result);
		setError_(callResult_.code, _("Got server error (%d)."), callResult_.code);
		freeCallResult_();
		return lastError_.code;
	}

	freeCallResult_();
	return LOBBY_NO_ERROR;
}

LOBBY_ERROR LobbyClient::delPlayer(unsigned int index)
{
	bson kwargs[1];
	bson_buffer buffer[1];

	if (gameId_ == 0 || gameId_ == -1)
		return setError_(LOBBY_NO_GAME, _("Create a game first!"));

	bson_buffer_init(buffer);
	bson_append_int(buffer, "gameId", gameId_);
	bson_append_int(buffer, "slot", index);
	bson_from_buffer(kwargs, buffer);

	if (call_("delPlayer", NULL, kwargs) != LOBBY_NO_ERROR)
		return lastError_.code;
	bson_destroy(kwargs);

	if (callResult_.code != LOBBY_NO_ERROR)
	{
		debug(LOG_ERROR, "Received: server error %d: %s.", callResult_.code, callResult_.result);
		setError_(callResult_.code, _("Got server error (%d)."), callResult_.code);
		freeCallResult_();
		return lastError_.code;
	}

	freeCallResult_();
	return LOBBY_NO_ERROR;
}

LOBBY_ERROR LobbyClient::updatePlayer(unsigned int index, const char* name)
{
	bson kwargs[1];
	bson_buffer buffer[1];

	if (gameId_ == 0 || gameId_ == -1)
		return setError_(LOBBY_NO_GAME, _("Create a game first!"));

	bson_buffer_init(buffer);
	bson_append_int(buffer, "gameId", gameId_);
	bson_append_int(buffer, "slot", index);
	bson_append_string(buffer, "name", name);
	bson_from_buffer(kwargs, buffer);

	if (call_("updatePlayer", NULL, kwargs) != LOBBY_NO_ERROR)
		return lastError_.code;
	bson_destroy(kwargs);

	if (callResult_.code != LOBBY_NO_ERROR)
	{
		debug(LOG_ERROR, "Received: server error %d: %s.", callResult_.code, callResult_.result);
		setError_(callResult_.code, _("Got server error (%d)."), callResult_.code);
		freeCallResult_();
		return lastError_.code;
	}

	freeCallResult_();
	return LOBBY_NO_ERROR;
}

LOBBY_ERROR LobbyClient::listGames(int maxGames)
{
	bson_iterator it, gameIt;
	const char * key;
	uint32_t gameCount = 0;
	LOBBY_GAME game;

	// Clear old games.
	games.clear();

	// Run "list" and retrieve its result
	if (call_("list", NULL, NULL) != LOBBY_NO_ERROR)
		return lastError_.code;

	//Print the result to stdout.
	//bson_print_raw(callResult_.result, 0);

	// Translate the result into LOBBY_GAMEs
	bson_iterator_init(&it, callResult_.result);
	while (bson_iterator_next(&it) && gameCount <= maxGames)
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

			games.push_back(game);
			gameCount++;
		}
	}

	freeCallResult_();
	return LOBBY_NO_ERROR;
}

inline bool LobbyClient::isConnected()
{
	return (socket_ != NULL && !socketReadDisconnected(socket_));
}

bool LobbyClient::connect()
{
	char version[sizeof("version") + sizeof(uint32_t)];
	char* p_version = version;

	if (isConnected())
		return true;

	// Build the initial version command.
	strlcpy(p_version, "version", sizeof("version"));
	p_version+= sizeof("version");
	*(uint32_t*)p_version = htonl(LOBBY_VERSION);

	callId_ = 0;

	// Resolve the hostname
	SocketAddress *const hosts = resolveHost(host_.c_str(), port_);

	// Resolve failed?
	if (hosts == NULL)
	{
		debug(LOG_ERROR, "Cannot resolve masterserver \"%s\": %s.", host_.c_str(), strSockError(getSockErr()));
		return setError_(LOBBY_TRANSPORT_ERROR, _("Could not resolve masterserver name (%s)!"), host_.c_str());
	}

	// try each address from resolveHost until we successfully connect.
	socket_ = socketOpenAny(hosts, 1500);
	deleteSocketAddress(hosts);

	// No address succeeded or couldn't send version data
	if (socket_ == NULL || writeAll(socket_, version, sizeof(version)) == SOCKET_ERROR)
	{
		debug(LOG_ERROR, "Cannot connect to masterserver \"%s:%d\": %s", host_.c_str(), port_, strSockError(getSockErr()));
		return setError_(LOBBY_TRANSPORT_ERROR, _("Could not communicate with lobby server! Is TCP port %u open for outgoing traffic?"), port_);
	}

	return true;
}

bool LobbyClient::disconnect()
{
	if (!isConnected())
		return true;

	socketClose(socket_);
	socket_ = NULL;

	return true;
}

LOBBY_ERROR LobbyClient::call_(const char* command, bson* args, bson* kwargs)
{
	bson bson[1];
	bson_buffer buffer[1];
	int bsize;
	char *call;
	uint32_t resSize;

	// Connect to the masterserver
	if (connect() != true)
		return lastError_.code;

	callId_ += 1;

	debug(LOG_NET, "Calling \"%s\" on the mastserver", command);

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

	bsize = bson_size(bson);

	call = (char*) malloc(sizeof(uint32_t) + bsize);
	*(uint32_t*)call = htonl(bsize);
	memcpy(call + sizeof(uint32_t), bson->data, bsize);

	if (writeAll(socket_, call, sizeof(uint32_t) + bsize) == SOCKET_ERROR
		|| readAll(socket_, &resSize, sizeof(resSize), 300) != sizeof(resSize))
	{
		connect(); // FIXME: Should check why readAll failed.

		if (writeAll(socket_, call, sizeof(uint32_t) + bsize) == SOCKET_ERROR
			|| readAll(socket_, &resSize, sizeof(resSize), 300) != sizeof(resSize))
		{
			debug(LOG_ERROR, "Failed sending command \"%s\" to the masterserver: %s.", command, strSockError(getSockErr()));
			return setError_(LOBBY_TRANSPORT_ERROR, _("Failed to communicate with the masterserver."));
		}
	}

	free(call);
	bson_destroy(bson);

	resSize = ntohl(resSize);

	callResult_.buffer = (char*) malloc(resSize);
	if (readAll(socket_, callResult_.buffer, resSize, 900) != resSize)
	{
		debug(LOG_ERROR, "Failed reading the result for \"%s\" from the masterserver: %s.", command, strSockError(getSockErr()));

		free(callResult_.buffer);
		return setError_(LOBBY_TRANSPORT_ERROR, _("Failed to communicate with the masterserver."));
	}

	// Get the first object (bson_array)
	bson_iterator it;
	bson_iterator_init(&it, callResult_.buffer);
	bson_iterator_next(&it);
	if (bson_iterator_type(&it) != bson_array)
	{
		debug(LOG_ERROR, "Received invalid bson data (no array first): %d.", bson_iterator_type(&it));

		free(callResult_.buffer);
		return setError_(LOBBY_INVALID_DATA, _("Failed to communicate with the masterserver."));
	}
	bson_iterator_init(&it, bson_iterator_value(&it));
	bson_iterator_next(&it);

	if (bson_iterator_type(&it) != bson_int)
	{
		debug(LOG_ERROR, "Received invalid bson data (no int first): %d.", bson_iterator_type(&it));

		free(callResult_.buffer);
		return setError_(LOBBY_INVALID_DATA, _("Failed to communicate with the masterserver."));
	}

	callResult_.code = (LOBBY_ERROR)bson_iterator_int(&it);
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

	return LOBBY_NO_ERROR;
}

LOBBY_ERROR LobbyClient::setError_(const LOBBY_ERROR code, char * message, ...)
{
	   va_list ap;
	   int count;

	   lastError_.code = code;

	   va_start(ap, message);
			   count = vasprintf(&lastError_.message, message, ap);
	   va_end(ap);

	   if (count == -1)
			   lastError_.message = NULL;

	   return code;
}
