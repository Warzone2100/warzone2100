#!/usr/bin/env python
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
# MasterServer Test v0.1 by Gerhard Schaden (gschaden)
# --------------------------------------------------------------------------
#
################################################################################
# import from __future__
from __future__ import with_statement

################################################################################
#

__author__ = "Gerhard Schaden"
__version__ = "0.1"
__bpydoc__ = """\
This script simulates a Warzone 2100 2.x client to test the masterserver
"""

#
################################################################################
#

import logging
from threading import Timer
import SocketServer
import time
import struct
from game import *
from protocol import *
from protocol import connection

server="lobby.wz2100.net"
protocol = Protocol()
logging.basicConfig(level = logging.DEBUG, format = "%(asctime)-15s %(levelname)s %(message)s")

# simulate a gameserver
class RequestHandler(SocketServer.ThreadingMixIn, SocketServer.StreamRequestHandler):
    def handle(self):
	logging.debug("got connection from lobbyserver")

def simulateGameServer():
	SocketServer.ThreadingTCPServer.allow_reuse_address = True
	tcpserver = SocketServer.ThreadingTCPServer(('0.0.0.0', protocol.gamePort), RequestHandler)
	tcpserver.handle_request()
	tcpserver.server_close()

# thread with simulates adding a game
def doAddGame():
	# gamestrucuure
	g = Game()
	g.description = "description"
	g.size = 0
	g.flags = 0
	g.host = "1.1.1.1"
	g.maxPlayers = 8
	g.currentPlayers = 1
	g.user1 = 0
	g.user2 = 0
	g.user3 = 0
	g.user4 = 0
	g.multiplayerVersion = protocol.version
	g.modlist = '\0'
	g.lobbyVersion = 2
	g.game_version_major = int(protocol.version[0])
	g.game_version_minor = int(protocol.version[2])
	g.private = 0
	g.pure = 0
	g.Mods = 0
	g.future1 = 0
	g.future2 = 0
	g.future3 = 0
	g.future4 = 0

	logging.debug("connect to lobby")
	with connection(server, protocol.lobbyPort) as s:
		s.send("addg\0")
		protocol.encodeSingle(g, s)
		#hold the game open for 10 seconds
		time.sleep(10)

#thread with simulates listing the available games
def doListGames():
	logging.debug("Listing games...")
	for game in protocol.list(server):
		logging.debug("Game: %s" % (game))

#start gameserver
t=Timer(0, simulateGameServer)
t.start()

#start add game thread
t=Timer(1, doAddGame)
t.start()

#start list Thread
t=Timer(3, doListGames)
t.start()
