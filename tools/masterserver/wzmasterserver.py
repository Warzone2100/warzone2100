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

MOTDstring = None        # our message of the day (max 255 bytes)

logging.basicConfig(level = logging.DEBUG, format = "%(asctime)-15s %(levelname)s %(message)s")

#
################################################################################
# Game DB

gamedb = None

class GameDB:
	def __init__(self):
		self.list = set()
		self.lock = RLock()

	def __remove(self, g):
		# if g is not in the list, ignore the KeyError exception
		try:
			self.list.remove(g)
		except KeyError:
			pass

	def addGame(self, g):
		""" add a game """
		with self.lock:
			self.list.add(g)

	def removeGame(self, g):
		""" remove a game from the list"""
		with self.lock:
			self.__remove(g)

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
			if game.host == host:
				yield game

	def checkGames(self):
		with self.lock:
			games = list(self.getGames())
			logging.debug("Checking: %i game(s)" % (len(games)))
			for game in games:
				if not protocol.check(game):
					logging.debug("Removing unreachable game: %s" % game)
					self.__remove(game)

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
	def __init__(self, request, client_address, server):
		self.request = request
		self.client_address = client_address
		self.server = server

		# Client's address
		gameHost = self.client_address[0]

		try:
			self.setup()
			try:
				self.handle()
			except struct.error, e:
				logging.warning("(%s) Data decoding error: %s" % (gameHost, traceback.format_exc()))
			except:
				logging.error("(%s) Unhandled exception: %s" % (gameHost, traceback.format_exc()))
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

	def handle(self):
		global requests, requestlock, gamedb

		with requestlock:
			requests += 1
			if requests >= checkInterval:
				gamedb.checkGames()
				requests = 0

		# client address
		gameHost = self.client_address[0]


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

			logging.debug("(%s) Command: [%s] " % (gameHost, netCommand))
			# Give the MOTD
			if netCommand == 'motd':
				self.wfile.write(MOTDstring[0:255])
				logging.debug("(%s) sending MOTD (%s)" % (gameHost, MOTDstring[0:255]))

			# Add a game.
			elif netCommand == 'addg':
				# The host is valid
				logging.debug("(%s) Adding gameserver..." % gameHost)

				# create a game object
				self.g = Game()
				self.g.requestHandler = self
				# put it in the database
				gamedb.addGame(self.g)

				# and start receiving updates about the game
				while True:
					newGameData = protocol.decodeSingle(self.rfile, self.g)
					if not newGameData:
						logging.debug("(%s) Removing aborted game" % gameHost)
						return

					logging.debug("(%s) Updated game: %s" % (gameHost, self.g))
					#set gamehost
					self.g.host = gameHost

					if not protocol.check(self.g):
						logging.debug("(%s) Removing unreachable game" % gameHost)
						return

					gamedb.listGames()
				return
			# Get a game list.
			elif netCommand == 'list':
				# Lock the gamelist to prevent new games while output.
				with gamedb.lock:
					games = list(gamedb.getGames())
					logging.debug("(%s) Gameserver list: %i game(s)" % (gameHost, len(games)))

					# Transmit the games.
					for game in games:
						logging.debug(" %s" % game)
					protocol.encodeMultiple(games, self.wfile)
				return

			# If something unknown appears.
			else:
				raise Exception("(%s) Received an unknown command: %s" % (gameHost, netCommand))

	def finish(self):
		if self.g:
			gamedb.removeGame(self.g)
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
		self.socket.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 1)
		SocketServer.TCPServer.server_bind(self)

class ThreadingTCP6Server(SocketServer.ThreadingMixIn, TCP6Server): pass

#
################################################################################
# The legendary Main.

if __name__ == '__main__':
	logging.info("Starting Warzone 2100 lobby server on port %d" % (protocol.lobbyPort))

	# Read in the Message of the Day, max is 1 line, 255 chars
	MOTDstring = "The message of the day!!!"
	logging.info("The MOTD is (%s)" % MOTDstring[0:255])


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
		game.requestHandler.finish()
	for tcpserver in tcpservers:
		tcpserver.server_close()
