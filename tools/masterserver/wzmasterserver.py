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
# Socket Handler.

requests = 0
requestlock = Lock()
protocol = Protocol()

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
					g = Game()
					g.requestHandler = self
					# put it in the database
					gamedb.addGame(g)

					# and start receiving updates about the game
					while True:
						newGameData = protocol.decodeSingle(self.rfile, g)
						if not newGameData:
							logging.debug("(%s) Removing aborted game" % gameHost)
							return

						logging.debug("(%s) Updated game: %s" % (gameHost, g))
						#set gamehost
						g.host = gameHost

						if not protocol.check(g):
							logging.debug("(%s) Removing unreachable game" % gameHost)
							return

						gamedb.listGames()
				except struct.error, e:
					logging.warning("(%s) Data decoding error: %s" % (gameHost, traceback.format_exc()))
				except KeyError, e:
					logging.warning("(%s) Communication error: %s" % (gameHost, traceback.format_exc()))
				except:
					logging.error("(%s) Unhandled exception: %s" % (gameHost, traceback.format_exc()))
					raise
				finally:
					if g:
						gamedb.removeGame(g)
					break
			# Get a game list.
			elif netCommand == 'list':
				# Lock the gamelist to prevent new games while output.
				with gamedblock:
					try:
						gamesCount = len(gamedb.getGames())
						logging.debug("(%s) Gameserver list: %i game(s)" % (gameHost, gamesCount))

						# Transmit the games.
						for game in gamedb.getGames():
							logging.debug(" %s" % game)
						protocol.encodeMultiple(gamedb.getGames(), self.wfile)
						break
					except:
						logging.error("(%s) Unhandled exception: %s" % (gameHost, traceback.format_exc()))
						raise

			# If something unknown appears.
			else:
				raise Exception("(%s) Recieved a unknown command: %s" % (gameHost, netCommand))

#
################################################################################
# The legendary Main.

if __name__ == '__main__':
	logging.info("Starting Warzone 2100 lobby server on port %d" % (protocol.lobbyPort))

	# Read in the Message of the Day, max is 1 line, 255 chars
	in_file = open("motd.txt", "r")
	MOTDstring = in_file.read()
	in_file.close()
	logging.info("The MOTD is (%s)" % MOTDstring[0:255])


	gamedb = GameDB()

	SocketServer.ThreadingTCPServer.allow_reuse_address = True
	tcpserver = SocketServer.ThreadingTCPServer(('0.0.0.0', protocol.lobbyPort), RequestHandler)
	try:
		while True:
			tcpserver.handle_request()
	except KeyboardInterrupt:
		pass
	logging.info("Shutting down lobby server, cleaning up")
	for game in gamedb.getAllGames():
		game.requestHandler.finish()
	tcpserver.server_close()
