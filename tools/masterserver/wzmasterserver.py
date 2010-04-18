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
__version__ = "2.3"
__bpydoc__ = """\
This script runs a Warzone 2100 2.3.x+ masterserver
"""

#
################################################################################
# Get the things we need.
import sys
from SocketServer import ThreadingTCPServer
import SocketServer
import struct
import socket
from threading import Lock, RLock, Thread, Event, Timer
from select import select
import logging
import traceback
import cmd
from game import *
from protocol import *
#
################################################################################
# Settings.

checkInterval = 100      # Interval between requests causing a gamedb check

MOTDstring = None        # our message of the day (max 255 bytes) (for 2.3 / trunk)
MOTD22string = None      # the EOL message for 2.2.x

logging.basicConfig(level = logging.DEBUG, format = "%(asctime)-15s %(levelname)s %(message)s")

#
################################################################################
# Game DB

gamedb = None

class MultiAddressGame(Game):
	def __init__(self, gameId):
		super(MultiAddressGame, self).__init__()
		self.addressCount = 0
		self.gameId = gameId

		assert self.gameId > 0, "Invalid game ID"

class GameDB:
	def __init__(self):
		self.list = set()
		self.index = {}
		self.lock = RLock()
		self.numgen = self.genNum()

	def genNum(self):
		while True:
			for n in xrange(1, pow(2, 31) - 1):
				if n != 0xBAD01 and n not in self.index:
					yield n

	def __remove(self, g):
		# if g is not in the list, ignore the KeyError exception
		try:
			self.list.remove(g)
		except KeyError:
			pass

	def addGame(self, g):
		""" add a game """
		with self.lock:
			try:
				g.addressCount += 1
			except AttributeError:
				self.list.add(g)

	def removeGame(self, g):
		""" remove a game from the list"""
		with self.lock:
			try:
				if g.addressCount == 0:
					self.__remove(g)
					del self.index[g.gameId]
				else:
					g.addressCount -= 1
			except AttributeError:
				self.__remove(g)

	def getGameID(self):
		with self.lock:
			g = MultiAddressGame(self.numgen.next())

			self.list.add(g)
			self.index[g.gameId] = g

			return g

	def getGameByID(self, gameId):
		with self.lock:
			return self.index[gameId]

	# only games with a valid description
	def getGames(self):
		""" filter all games with a valid description """
		for game in self.getAllGames():
			if game.description:
				yield game

	def getAllGames(self):
		""" return all known games """
		with self.lock:
			for game in self.list:
				yield game

	def getGamesByHost(self, host):
		""" filter all games of a certain host"""
		for game in self.getGames():
			if host in game.hosts:
				yield game

	def checkGames(self):
		with self.lock:
			games = list(self.getGames())
			logging.debug("Checking: %i game(s)" % (len(games)))
			for game in games:
				for host in game.hosts:
					if not host:
						continue

					if not protocol.check(game, host):
						logging.debug("Removing unreachable game: %s" % game)
						self.__remove(game)
						break

	def listGames(self):
		with self.lock:
			games = list(self.getGames())
			logging.debug("Gameserver list: %i game(s)" % (len(games)))
			for game in games:
				logging.debug(" %s" % game)

#
################################################################################
# Socket Handler.

requests = 0
requestlock = Lock()
protocol = Protocol()

class RequestHandler(SocketServer.ThreadingMixIn, SocketServer.StreamRequestHandler):
	SUCCESS_OK = 200

	CLIENT_ERROR_NOT_ACCEPTABLE = 406

	def sendStatusMessage(self, status, message):
		logging.debug("(%s) Sending response status (%d) and message: %s" % (self.gameHost, status, message))
		try:
			protocol.sendStatusMessage(self.gameHost, status, message, self.wfile)
		except NotImplementedError:
			# Ignore the case where sending of status messages isn't supported
			pass

	def __init__(self, request, client_address, server):
		self.request = request
		self.client_address = client_address
		self.server = server

		# Client's address
		self.gameHost = self.client_address[0]

		try:
			self.setup()
			try:
				self.handle()
			except struct.error, e:
				logging.warning("(%s) Data decoding error: %s" % (self.gameHost, traceback.format_exc()))
			except:
				logging.error("(%s) Unhandled exception: %s" % (self.gameHost, traceback.format_exc()))
				raise
			finally:
				self.finish()
		finally:
			sys.exc_traceback = None    # Help garbage collection

	def setup(self):
		for parent in (SocketServer.ThreadingMixIn, SocketServer.StreamRequestHandler):
			if hasattr(parent, 'setup'):
				parent.setup(self)
		self.g = None
		self.GameId = None

	def handle(self):
		global requests, requestlock, gamedb

		with requestlock:
			requests += 1
			if requests >= checkInterval:
				gamedb.checkGames()
				requests = 0

		while True:
			# Read the incoming command.
			netCommand = self.rfile.read(4)
			if netCommand == None:
				return

			# Skip the trailing NULL.
			self.rfile.read(1)

			#################################
			# Process the incoming command. #
			#################################

			logging.debug("(%s) Command: [%s] " % (self.gameHost, netCommand))
			# TODO: When releasing a new 2.2 version remove the "motd" command
			# Give the MOTD
			if netCommand == 'motd':
				self.wfile.write(MOTDstring[0:255])
				logging.debug("(%s) sending MOTD (%s)" % (self.gameHost, MOTDstring[0:255]))

			# Get a game ID (for multi address games)
			elif netCommand == 'gaId':
				self.GameId = gamedb.getGameID()
				self.GameId.requestHandlers = [self]
				self.GameId.hosts.append(self.gameHost)
				logging.debug("(%s) Created game ID: %d" % (self.gameHost, self.GameId.gameId))
				protocol.encodeGameID(self.GameId.gameId, self.wfile)
			# Add a game.
			elif netCommand == 'addg':
				# The host is valid
				logging.debug("(%s) Adding gameserver..." % self.gameHost)
				if self.GameId:
					self.g = self.GameId
					if self.g.requestHandlers[0] is not self:
						self.g.requestHandlers.append(self)
				else:
					# create a game object
					self.g = Game()
					self.g.requestHandlers = [self]
				# put it in the database
				gamedb.addGame(self.g)

				# and start receiving updates about the game
				while True:
					hosts = self.g.hosts
					newGameData = protocol.decodeSingle(self.rfile, self.g)
					if not newGameData:
						return
					self.g.hosts = hosts

					if not hasattr(self.g, 'addressCount') and self.g.gameId > 0:
						try:
							self.GameId = gamedb.getGameByID(self.g.gameId)
							logging.debug("(%s) Game with same ID already present, linking games...", (self.gameHost))

							# Apparently a game with this ID exists already, so remove this duplicate game
							gamedb.removeGame(self.g)
							self.g = None

							# Attach this request handler to the existing game
							(self.g, self.GameId) = (self.GameId, self.g)
							gamedb.addGame(self.g)
							self.g.hosts.append(self.gameHost)
						except KeyError:
							pass

					logging.debug("(%s) Updated game: %s" % (self.gameHost, self.g))
					#set gamehost
					if not len(self.g.hosts) or not self.g.hosts[0]:
						self.g.hosts.append(self.gameHost)

					if not protocol.check(self.g, self.gameHost):
						logging.debug("(%s) Removing unreachable game" % self.gameHost)
						self.sendStatusMessage(self.CLIENT_ERROR_NOT_ACCEPTABLE, 'Game unreachable, failed to open a connection to: [%s]:%d' % (self.gameHost, protocol.gamePort))
						return

					if self.g.lobbyVersion == 3:
					   self.sendStatusMessage(self.SUCCESS_OK, MOTDstring)
					else:
					   self.sendStatusMessage(self.SUCCESS_OK, MOTD22string)
# After a few weeks, we will force 2.2.x clients to upgrade, by closing the connection after MOTD is sent.
#					   self.sendStatusMessage(self.CLIENT_ERROR_NOT_ACCEPTABLE, MOTD22string)
#					   return
					gamedb.listGames()
				return
			# Get a game list.
			elif netCommand == 'list':
				# Lock the gamelist to prevent new games while output.
				with gamedb.lock:
					games = list(gamedb.getGames())
					logging.debug("(%s) Gameserver list: %i game(s)" % (self.gameHost, len(games)))

					# Transmit the games.
					for game in games:
						logging.debug(" %s" % game)
					protocol.encodeMultiple(games, self.wfile, hideGameID=True)
				return

			# If something unknown appears.
			else:
				raise Exception("(%s) Received an unknown command: %s" % (self.gameHost, netCommand))

	def finish(self):
		if self.g:
			self.g.hosts = filter(lambda s: s != self.gameHost, self.g.hosts)
			if not filter(None, self.g.hosts):
				logging.debug("(%s) Removing aborted game" % self.gameHost)
			gamedb.removeGame(self.g)
		if self.GameId:
			gamedb.removeGame(self.GameId)
		for parent in (SocketServer.ThreadingMixIn, SocketServer.StreamRequestHandler):
			if hasattr(parent, 'finish'):
				parent.finish(self)

class TCP6Server(SocketServer.TCPServer):
	"""Class to enable listening on both IPv4 and IPv6 addresses."""

	address_family = socket.AF_INET6

	def server_bind(self):
		"""Called by constructor to bind the socket.

		Overridden to enable IPV6_V6ONLY.

		"""
		try:
			self.socket.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 1)
		except AttributeError:
			# Apparently IPV6_V6ONLY isn't supported on this
			# platform. Lets hope that IPv6 mapping of IPv4
			# addresses isn't supported either...
			pass
		SocketServer.TCPServer.server_bind(self)

class ThreadingTCP6Server(SocketServer.ThreadingMixIn, TCP6Server): pass

#
################################################################################
# The legendary Main.

if __name__ == '__main__':
	logging.info("Starting Warzone 2100 lobby server on port %d" % (protocol.lobbyPort))

	# Read in the Message of the Day, max is 1 line, 255 chars
	#for the 2.3 / trunk clients
	in_file = open("motd.txt", "r")
	MOTDstring = in_file.read()
	in_file.close()

	#for the 2.2.x client (farewell message)
	in_file = open("motd2.2.txt", "r")
	MOTD22string = in_file.read()
	in_file.close()

	logging.info("The MOTD(2.2) is (%s)" % MOTD22string)
	logging.info("The MOTD is (%s)" % MOTDstring)


	gamedb = GameDB()

	ThreadingTCPServer.allow_reuse_address = True
	ThreadingTCP6Server.allow_reuse_address = True
	tcpservers = [ThreadingTCPServer(('0.0.0.0', protocol.lobbyPort), RequestHandler),
	              ThreadingTCP6Server(('::', protocol.lobbyPort), RequestHandler)]
	try:
		while True:
			(readyServers, wlist, xlist) = select(tcpservers, [], [])
			for tcpserver in readyServers:
				tcpserver.handle_request()
	except KeyboardInterrupt:
		pass
	logging.info("Shutting down lobby server, cleaning up")
	for game in gamedb.getAllGames():
		for requestHandler in game.requestHandlers:
			requestHandler.finish()
	for tcpserver in tcpservers:
		tcpserver.server_close()
