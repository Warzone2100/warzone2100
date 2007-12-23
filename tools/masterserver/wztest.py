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
# MasterServer Test v0.1 by Gerhard Schaden (gschaden)
# --------------------------------------------------------------------------
#
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

import wzmasterserver as wz
import socket
import logging
from threading import Timer
import SocketServer
import time
import struct

server="localhost"

# simulate a gameserver
class RequestHandler(SocketServer.ThreadingMixIn, SocketServer.StreamRequestHandler):
    def handle(self):
	logging.debug("got connection from lobbyserver")

def simulateGameServer():
	SocketServer.ThreadingTCPServer.allow_reuse_address = True
	tcpserver = SocketServer.ThreadingTCPServer(('0.0.0.0', wz.gamePort), RequestHandler)
	tcpserver.handle_request()
	tcpserver.server_close()


# thread with simulates adding a game
def doAddGame():
	
	# gamestrucuure
	g=wz.Game()
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

	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	logging.debug("connect to lobby")
	s.settimeout(10.0)
	s.connect((server, wz.lobbyPort))
	s.send("addg ")
	s.send(g.getData())
	#hold the game open for 10 seconds
	time.sleep(10)
	s.close()

#thread with simulates listing the available games
def doListGames():
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	logging.debug("connect to lobby")
	s.settimeout(10.0)
	s.connect((server, wz.lobbyPort))
	s.send("list ")
	n = struct.unpack("!I", s.recv(4))[0]
	logging.debug("read %d games" % n)
	for i in range(n):
		g = wz.Game()
		g.setData(s.recv(wz.gsSize))
		logging.debug("%s" % g)

	#hold the game open for 10 seconds
	s.close()

#start gameserver
t=Timer(0, simulateGameServer)
t.start()

#start add game thread
t=Timer(1, doAddGame)
t.start()

#start list Thread
t=Timer(3, doListGames)
t.start()
