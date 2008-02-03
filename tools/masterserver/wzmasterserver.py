#!/usr/bin/python2.5
#
# This file is part of the Warzone 2100 Resurrection Project.
# Copyright (c) 2007  Warzone 2100 Resurrection Project
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
# --------------------------------------------------------------------------
#
################################################################################
#

__author__ = "Gerard Krol, Tim Perrei, Freddie Witherden, Gerhard Schaden"
__version__ = "2.0"
__bpydoc__ = """\
This script runs a Warzone 2100 2.x masterserver
"""

#
################################################################################
# Get the things we need.

#import the new with statement, has to be the first expression
from __future__ import with_statement

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

gamePort  = 9999         # Gameserver port.
lobbyPort = 9998         # Lobby port.
gsSize    = 112          # Size of GAMESTRUCT in byte.

logging.basicConfig(level=logging.DEBUG, format="%(asctime)-15s %(levelname)s %(message)s")

#
################################################################################
# Game DB

gdb=None
gamedblock = Lock()
class GameDB:
	def __init__(self):
		self.list = set()

	def addGame(self, g):
		""" add a game """
		with gamedblock:
			self.list.add(g)

	def removeGame(self, g):
		""" remove a game from the list"""
		with gamedblock:
			# if g is not in thel list, ignore the KeyError exception
			try:
				self.list.remove(g)
			except KeyError:
				pass

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

	def listGames(self):
            with gamedblock:
                gamesCount=len(self.getGames())
                logging.debug("Gameserver list: %i game(s)" % (gamesCount))
                for game in self.getGames():
                    logging.debug(" %s" % game)
 


#
################################################################################
# Game class
	
class Game:
	""" class for a single game """
		
	def __init__(self):
		self.description = None
		self.size = None
		self.flags = None
		self.host = None
		self.maxPlayers = None
		self.currentPlayers = None
		self.user1 = None
		self.user2 = None
		self.user3 = None
		self.user4 = None

	def setData(self, d):
		""" decode the c-structure from the server into local varialbles"""
                (self.description, self.size, self.flags, self.host, self.maxPlayers, self.currentPlayers, 
			self.user1, self.user2, self.user3, self.user4 ) = struct.unpack("!64sII16sIIIIII", d)
		self.description=self.description.strip("\x00")
		self.host=self.host.strip("\x00")
		logging.debug("Game: %s %s %s %s" % ( self.host, self.description, self.maxPlayers, self.currentPlayers))

	def getData(self):
		""" use local variables and build a c-structure, for sending to the clients"""
		return struct.pack("64sII16sIIIIII", 
			self.description.ljust(64, "\x00"), 
			self.size, self.flags,
			self.host.ljust(16, "\x00"),
			self.maxPlayers, self.currentPlayers, self.user1, self.user2, self.user3, self.user4)
	
	def __str__(self):
		return "Game: %16s %s %s %s" % ( self.host, self.description, self.maxPlayers, self.currentPlayers)
	
#
################################################################################
# Socket Handler.

class RequestHandler(SocketServer.ThreadingMixIn, SocketServer.StreamRequestHandler):
    def handle(self):
	# client address
        gameHost = self.client_address[0]

    	# Read the incoming command.
        netCommand = self.rfile.read(4)

        # Skip the trailing NULL.
        self.rfile.read(1)

        #################################
        # Process the incoming command. #
        #################################

        logging.debug("Command(%s): %s" % (gameHost, netCommand))
        # Add a game.
        if netCommand == 'addg':

            # Check we can connect to the host
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            try:
                logging.debug("Checking gameserver's vitality...")
                s.settimeout(10.0)
                s.connect((gameHost, gamePort))
                s.close()
            except:
                logging.debug("Gameserver did not respond!")
                return
		
            # The host is valid
            logging.debug("Adding gameserver.")
            try:
		# create a game object
                g=Game()
		
		# put it in the database
                gdb.addGame(g)
    
                # and start receiving updates about the game
                while True:
                    newGameData = self.rfile.read(gsSize)
                    if not newGameData:
			logging.debug("End of gameserver")
                        return
    
                    #set Gamedata
                    g.setData(newGameData)
                    #set gamehost
                    g.host = gameHost
                    gdb.listGames()
    
            except KeyError:
               logging.warning("Communication error with %s" % g )
            finally:
                if g:
                    gdb.removeGame(g)
        # Get a game list.
        elif netCommand == 'list':

            # Lock the gamelist to prevent new games while output.
            with gamedblock:
                gamesCount=len(gdb.getGames())
                logging.debug("Gameserver list: %i game(s)" % (gamesCount))
    
                # Transmit the length of the following list as unsigned integer (in network byte-order: big-endian).
                count = struct.pack('!I', gamesCount)
                self.wfile.write(count)
    
                # Transmit the single games.
                for game in gdb.getGames():
                    logging.debug(" %s" % game)
                    self.wfile.write(game.getData())
        
        # If something unknown apperas.
        else:
            raise Exception("Recieved a unknown command: %s" % netCommand)

#
################################################################################
# The legendary Main.

if __name__ == '__main__':
    logging.info("Starting Warzone 2100 lobby server on port %d" % lobbyPort)
    gdb=GameDB()

    SocketServer.ThreadingTCPServer.allow_reuse_address = True
    tcpserver = SocketServer.ThreadingTCPServer(('0.0.0.0', lobbyPort), RequestHandler)
    try:
	while True:
            tcpserver.handle_request()
    except KeyboardInterrupt:
        pass
    logging.info("Shutting down lobby server, cleaning up")
    for game in gdb.getAllGames():
        game.requestHandler.finish()
    tcpserver.server_close()
