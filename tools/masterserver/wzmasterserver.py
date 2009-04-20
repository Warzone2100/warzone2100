#!/usr/bin/env python
#
# This file is part of the Warzone 2100 Resurrection Project.
# Copyright (c) 2007-2009  Warzone 2100 Resurrection Project
#
# Warzone 2100 is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# You should have received a copy of the GNU General Public License
# along with Warzone 2100; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
#
# --------------------------------------------------------------------------
# MasterServer v0.1 by Gerard Krol (gerard_) and Tim Perrei (Kamaze)
#              v1.0 by Freddie Witherden (EvilGuru)
#              v2.0 by Gerhard Schaden (gschaden)
#              v2.0a by Buginator  (fixed endian issue)
#              v2.2 add motd, bigger GAMESTRUCT, private game notification
# --------------------------------------------------------------------------
#
################################################################################
# import from __future__
from __future__ import with_statement

#
################################################################################
#

__author__ = "Gerard Krol, Tim Perrei, Freddie Witherden, Gerhard Schaden, Dennis Schridde, Buginator"
__version__ = "2.2"
__bpydoc__ = """\
This script runs a Warzone 2100 2.2.x+ masterserver
"""

#
################################################################################
# Get the things we need.
import sys
import SocketServer
import struct
import socket
from threading import Lock, Thread, Event, Timer
import select
import logging
import cmd
#
################################################################################
# Settings.

gamePort  = 2100         # Gameserver port.
lobbyPort = 9990         # Lobby port.
checkInterval = 100      # Interval between requests causing a gamedb check

MOTDstring = None        # our message of the day (max 255 bytes)

logging.basicConfig(level = logging.DEBUG, format = "%(asctime)-15s %(levelname)s %(message)s")

#
################################################################################
# Game DB

gamedb = None
gamedblock = Lock()

class GameDB:
	global gamedblock

	def __init__(self):
		self.list = set()

	def __remove(self, g):
		# if g is not in the list, ignore the KeyError exception
		try:
			self.list.remove(g)
		except KeyError:
			pass

	def addGame(self, g):
		""" add a game """
		with gamedblock:
			self.list.add(g)

	def removeGame(self, g):
		""" remove a game from the list"""
		with gamedblock:
			self.__remove(g)

	# only games with a valid description
	def getGames(self):
		""" filter all games with a valid description """
		return filter(lambda x: x.description, self.list)

	def getAllGames(self):
		""" return all knwon games """
		return self.list

	def getGamesByHost(self, host):
		""" filter all games of a certain host"""
		return filter(lambda x: x.host == host, self.getGames())

	def checkGames(self):
		with gamedblock:
			gamesCount = len(self.getGames())
			logging.debug("Checking: %i game(s)" % (gamesCount))
			for game in self.getGames():
				if not game.check():
					logging.debug("Removing unreachable game: %s" % game)
					self.__remove(game)

	def listGames(self):
		with gamedblock:
			gamesCount = len(self.getGames())
			logging.debug("Gameserver list: %i game(s)" % (gamesCount))
			for game in self.getGames():
				logging.debug(" %s" % game)

#
################################################################################
# Game class
# NOTE: This must match exactly what we have defined for GAMESTRUCT in netplay.h
# The structure MUST be packed network byte order !

class Game(object):
	""" class for a single game """

	def _setDescription(self, v):
		self.data['name'] = unicode(v)

	def _setHost(self, v):
		self.data['host'] = v

	def _setMaxPlayers(self, v):
		self.data['maxPlayers'] = int(v)

	def _setCurrentPlayers(self, v):
		self.data['currentPlayers'] = int(v)

	def _setLobbyVersion(self, v):
		self.data['lobby-version'] = int(v)

	def _setMultiplayerVersion(self, v):
		self.data['multiplayer-version'] = unicode(v)

	def _setWarzoneVersion(self, v):
		self.data['warzone-version'] = unicode(v)

	def _setPure(self, v):
		self.data['pure'] = bool(v)

	def _setPrivate(self, v):
		self.data['private'] = bool(v)

	description        = property(fget = lambda self:    self.data['name'],
	                              fset = lambda self, v: self._setDescription(v))

	host               = property(fget = lambda self:    self.data['host'],
	                              fset = lambda self, v: self._setHost(v))

	maxPlayers         = property(fget = lambda self:    self.data['maxPlayers'],
	                              fset = lambda self, v: self._setMaxPlayers(v))

	currentPlayers     = property(fget = lambda self:    self.data['currentPlayers'],
	                              fset = lambda self, v: self._setCurrentPlayers(v))

	lobbyVersion       = property(fget = lambda self:    self.data['lobby-version'],
	                              fset = lambda self, v: self._setLobbyVersion(v))

	multiplayerVersion = property(fget = lambda self:    self.data['multiplayer-version'],
	                              fset = lambda self, v: self._setMultiplayerVersion(v))

	warzoneVersion     = property(fget = lambda self:    self.data['warzone-version'],
	                              fset = lambda self, v: self._setWarzoneVersion(v))

	pure               = property(fget = lambda self:    self.data['pure'],
	                              fset = lambda self, v: self._setPure(v))

	private = property(fget = lambda self:    self.data['private'],
	                   fset = lambda self, v: self._setPrivate(v))

	def __init__(self, requestHandler):
		self.data = {'name':                None,
		             'host':                None,
		             'maxPlayers':          None,
		             'currentPlayers':      None,
		             'lobby-version':       None,
		             'multiplayer-version': None,
		             'warzone-version':     None,
		             'pure':                None,
		             'private':             None}
		self.size = None
		self.flags = None
		self.user1 = None
		self.user2 = None
		self.user3 = None
		self.user4 = None
		self.misc = None				# 64  byte misc string
		self.extra = None				# 255 byte extra string (future use)
		self.modlist = None			# 255 byte string
		self.game_version_major = None	# game major version
		self.game_version_minor = None	# game minor version
		self.Mods = None				# number of concatenated mods they have (list of mods is in modlist)
		self.future1 = None			# for future use
		self.future2 = None			# for future use
		self.future3 = None			# for future use
		self.future4 = None			# for future use

		self.requestHandler = requestHandler

	def __str__(self):
		if self.private == 1:
		   return "(private) Game: %16s %s %s %s" % ( self.host, self.description, self.maxPlayers, self.currentPlayers)
		else:
		   return "Game: %16s %s %s %s" % ( self.host, self.description, self.maxPlayers, self.currentPlayers)

	def check(self):
		# Check we can connect to the host
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		try:
			logging.debug("Checking %s's vitality..." % self.host)
			s.settimeout(10.0)
			s.connect((self.host, gamePort))
			s.close()
			return True
		except:
			logging.debug("%s did not respond!" % self.host)
			return False

class Protocol(object):
	# Size of a single game is undefined at compile time
	size = None

	def encodeSingle(data):
		pass

	def encodeMultiple(data):
		pass

	def decodeSingle(data):
		pass

	def decodeMultiple(data):
		pass

class BinaryProtocol(Protocol):
	name_length          = 64
	host_length          = 16
	misc_length          = 64
	extra_length         = 255
	versionstring_length = 64
	modlist_length       = 255
	# Binary struct format to use (GAMESTRUCT)
	gameFormat = '!%dsII%ds6I%ds%ds%ds%ds10I' % (name_length, host_length, misc_length, extra_length, versionstring_length, modlist_length)

	# Size of a single game = sizeof(GAMESTRUCT)
	size = struct.calcsize(gameFormat)

	countFormat = '!I'
	countSize = struct.calcsize(countFormat)

	listSeparator = ' '

	def _encodeName(self, game):
		return game.description[:self.name_length - 1].ljust(self.name_length, "\0")

	def _encodeHost(self, game):
		return game.host[:self.host_length - 1].ljust(self.host_length, "\0")

	def _encodeMisc(self, game):
		return game.misc[:self.misc_length - 1].ljust(self.misc_length, "\0")

	def _encodeExtra(self, game):
		return game.extra[:self.extra_length - 1].ljust(self.extra_length, "\0")

	def _encodeVersionString(self, game):
		return game.multiplayerVersion[:self.versionstring_length - 1].ljust(self.versionstring_length, "\0")

	def encodeSingle(self, game):
		return struct.pack(self.gameFormat,
			self._encodeName(game),
			game.size or self.size, game.flags,
			self._encodeHost(game),
			game.maxPlayers, game.currentPlayers, game.user1, game.user2, game.user3, game.user4,
			self._encodeMisc(game),
			self._encodeExtra(game),
			self._encodeVersionString(game),
			game.modlist,
			game.lobbyVersion, game.game_version_major, game.game_version_minor, game.private,
			game.pure, game.Mods, game.future1, game.future2, game.future3, game.future4)

	def encodeMultiple(self, games):
		message = struct.pack(self.countFormat, len(games))
		for game in games:
			message += self.encodeSingle(game)
		return message

	def decodeSingle(self, data, game = Game(None)):
		decData = {}

		(decData['name'], game.size, game.flags, decData['host'], game.maxPlayers, game.currentPlayers,
			game.user1, game.user2, game.user3, game.user4,
			game.misc, game.extra, decData['multiplayer-version'], game.modlist, game.lobbyVersion,
			game.game_version_major, game.game_version_minor, game.private, game.pure, game.Mods, game.future1,
			game.future2, game.future3, game.future4) = struct.unpack(self.gameFormat, data)

		for strKey in ['name', 'host', 'multiplayer-version']:
			decData[strKey] = decData[strKey].strip("\0")

		game.misc = game.misc.strip("\0")
		game.extra = game.extra.strip("\0")
		game.modlist = game.modlist.strip("\0")

		game.data.update(decData)
		return game

	def decodeMultiple(self, data):
		countMsg = data[:countSize]
		data = data[countSize:]
		count = struct.pack(self.countFormat, countMsg)
		return [self.decodeSingle(data[size * i: size * i + size]) for i in range(count)]


#
################################################################################
# Socket Handler.

requests = 0
requestlock = Lock()
protocol = BinaryProtocol()

class RequestHandler(SocketServer.ThreadingMixIn, SocketServer.StreamRequestHandler):
	def handle(self):
		global requests, requestlock, gamedb

		with requestlock:
			requests += 1
			if requests >= checkInterval:
				gamedb.checkGames()

		# client address
		gameHost = self.client_address[0]


		while True:
			# Read the incoming command.
			netCommand = self.rfile.read(4)
			if netCommand == None:
				break

			# Skip the trailing NULL.
			self.rfile.read(1)

			#################################
			# Process the incoming command. #
			#################################

			logging.debug("(%s) Command: [%s] " % (gameHost, netCommand))
			# Give the MOTD
			if netCommand == 'motd':
				self.wfile.write(MOTDstring[0:255])
				logging.debug("(%s) sending MOTD (%s)" % (gameHost, MOTDstring[0:255]))

			# Add a game.
			elif netCommand == 'addg':
				# The host is valid
				logging.debug("(%s) Adding gameserver..." % gameHost)
				try:
					# create a game object
					g = Game(self)
					# put it in the database
					gamedb.addGame(g)

					# and start receiving updates about the game
					while True:
						newGameData = self.rfile.read(protocol.size)
						if not newGameData:
							logging.debug("(%s) Removing aborted game" % gameHost)
							return

						logging.debug("(%s) Updating game..." % gameHost)
						#set Gamedata
						protocol.decodeSingle(newGameData, g)
						logging.debug(g)
						#set gamehost
						g.host = gameHost

						if not g.check():
							logging.debug("(%s) Removing unreachable game" % gameHost)
							return

						gamedb.listGames()
				except struct.error, e:
					logging.warning("(%s) Data decoding error: %s" % (gameHost, e))
				except KeyError, e:
					logging.warning("(%s) Communication error: %s" % (gameHost, e))
				finally:
					if g:
						gamedb.removeGame(g)
					break
			# Get a game list.
			elif netCommand == 'list':
				# Lock the gamelist to prevent new games while output.
				with gamedblock:
					gamesCount = len(gamedb.getGames())
					logging.debug("(%s) Gameserver list: %i game(s)" % (gameHost, gamesCount))

					# Transmit the games.
					for game in gamedb.getGames():
						logging.debug(" %s" % game)
					self.wfile.write(protocol.encodeMultiple(gamedb.getGames()))
					break

			# If something unknown appears.
			else:
				raise Exception("(%s) Recieved a unknown command: %s" % (gameHost, netCommand))

#
################################################################################
# The legendary Main.

if __name__ == '__main__':
	logging.info("Starting Warzone 2100 lobby server on port %d" % lobbyPort)

	# Read in the Message of the Day, max is 1 line, 255 chars
	in_file = open("motd.txt", "r")
	MOTDstring = in_file.read()
	in_file.close()
	logging.info("The MOTD is (%s)" % MOTDstring[0:255])


	gamedb = GameDB()

	SocketServer.ThreadingTCPServer.allow_reuse_address = True
	tcpserver = SocketServer.ThreadingTCPServer(('0.0.0.0', lobbyPort), RequestHandler)
	try:
		while True:
			tcpserver.handle_request()
	except KeyboardInterrupt:
		pass
	logging.info("Shutting down lobby server, cleaning up")
	for game in gamedb.getAllGames():
		game.requestHandler.finish()
	tcpserver.server_close()
