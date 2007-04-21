#!/usr/bin/python
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
################################################################################
# Get the things we need.

import sys

try:
    import SocketServer
except:
    print "Couldn't load module SocketServer! -- Aborting..."
    sys.exit(1)
    
try:
    import thread
except:
    print "Couldn't load module thread! -- Aborting..."
    sys.exit(1)
    
try:
    import struct
except:
    print "Couldn't load module struct! -- Aborting..."
    sys.exit(1)

#
################################################################################
# Settings.

lobbyPort = 9998         # Lobby port.
lobbyDev  = True         # Enable debugging.
gsSize    = 112          # Size of GAMESTRUCT in byte.
ipOffset  = 64+4+4       # 64 byte StringSize + SDWORD + SDWORD
gameList  = set()        # Holds the list.
listLock  = thread.allocate_lock()


#
################################################################################
# Socket Handler.
 
class RequestHandler(SocketServer.ThreadingMixIn, SocketServer.StreamRequestHandler):
    def handle(self):
    	# Read the incoming command.
        netCommand = self.rfile.read(4)
        
        # Skip the trailing NULL.
        self.rfile.read(1)

        #################################
        # Process the incoming command. #
        #################################

        # Add a game.
        if netCommand == 'addg':

            # Debug
            if lobbyDev:
                print "<- addg"

            # Recive the gamestruct.
            gameData = self.rfile.read(gsSize)

            # Fix the server address.
            gameHost = self.client_address[0]

            while len(gameHost) < 16:
                gameHost += '\0'

            # Update the gameData whith the fresh gameHost.
            gameData = gameData[:ipOffset] + gameHost + gameData[ipOffset+16:]

            # Put the game in the database.
            listLock.acquire()
            gameList.add(gameData)
            listLock.release()

            # Wait until it is done.
            self.rfile.read(1)
            gameList.remove(gameData)

        # Get a game list.
        elif netCommand == 'list':
        
            # Debug
            if lobbyDev:
                print "<- list"
                print "  \- I know ", len(gameList), " games."

            # Lock the gamelist to prevent new games while output.
            listLock.acquire()
            
            # Transmit the length of the following list as integer.
            count = struct.pack('i', len(gameList))
            self.wfile.write(count)
            
            # Transmit the single games.
            for game in gameList:
                self.wfile.write(game)
                
            # Remove the lock.
            listLock.release()

        # If something unknown apperas.
        else:
            print "Recieved a unknown command: ", netCommand 


#
################################################################################
# The legendary Main.

if __name__ == '__main__':
    print "Starting Warzone 2100 lobby server on port ", lobbyPort
    
    tcpserver = SocketServer.ThreadingTCPServer(('0.0.0.0', lobbyPort), RequestHandler)
    #tcpserver.allow_reuse_address = True
    tcpserver.serve_forever()