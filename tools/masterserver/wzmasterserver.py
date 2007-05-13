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
import SocketServer
import thread
import struct

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
                
            # Fix the server address.
            gameHost = self.client_address[0]

            while len(gameHost) < 16:
                gameHost += '\0'

            currentGameData = None
            
            # and start receiving updates about the game
            while True:
                # Receive the gamestruct.
                try:
                    newGameData = self.rfile.read(gsSize)
                except:
                    newGameData = None
                
                # remove the previous data from the list
                if currentGameData:
                    listLock.acquire()
                    try:
                        if lobbyDev:
                            print "Removing game from", self.client_address[0]
                        gameList.remove(currentGameData)
                    finally:
                        listLock.release()
                        
                if len(newGameData) < gsSize:
                    # incomplete data
                    break
                        
                # Update the new gameData whith the gameHost
                currentGameData = newGameData[:ipOffset] + gameHost + newGameData[ipOffset+16:]

                # Put the game in the database
                listLock.acquire()
                try:
                    if lobbyDev:
                        print "  \- Adding game from", self.client_address[0]
                    gameList.add(currentGameData)
                finally:
                    listLock.release()

        # Get a game list.
        elif netCommand == 'list':
        
            # Debug
            if lobbyDev:
                print "<- list"
                print "  \- I know ", len(gameList), " games."

            # Lock the gamelist to prevent new games while output.
            listLock.acquire()
            
            # Transmit the length of the following list as unsigned integer (in network byte-order: big-endian).
            count = struct.pack('!I', len(gameList))
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
    
