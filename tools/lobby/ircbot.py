#!/usr/bin/env python
# -*- coding: utf-8 -*-
# vim: set et sts=4 sw=4 encoding=utf-8:
#
# This file is part of Warzone 2100.
# Copyright (C) 2009  Gerard Krol
# Copyright (C) 2009  Warzone Resurrection Project
#
# Warzone 2100 is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Warzone 2100 is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Warzone 2100; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
#

__author__  = "Warzone Resurrection Project"
__version__ = "0.0.1"
__license__ = "GPL"

import socket
import struct
import threading
import time

irc_server  = "irc.freenode.net"
irc_port    = 6667
irc_channel = "#warzone2100"
irc_nick    = "wzlobbybot"

def main():
    irc = bot_connection(irc_server, irc_port, irc_channel, irc_nick)
    lobby = masterserver_connection("lobby.wz2100.net", 9997)
    check = change_notifier(irc, lobby)
    check.start()
    while True:
        message = irc.read()
        if message == "ping":
            irc.write("pong")
        if message == "help" or message == "info":
            irc.write("I'm a bot that shows information from the \x02Warzone 2100\x02 lobby server. I was created by Gerard Krol. For information about commands you can try: \"commands\"")
        if message == "commands":
            irc.write("ping: pong, help/info: general information about this bot, list: show which games are currenly being hosted")
        if message == "list":
            games = lobby.list()
            if not games:
                irc.write("No games in lobby")
            else:
                if len(games) == 1:
                    message = "1 game hosted: "
                else:
                    message = "%i games hosted: " % (len(games))
                l = ["\x02%s\x02 [%i/%i]" % (g.description, g.currentPlayers, g.maxPlayers) for g in games]
                message += ", ".join(l)
                irc.write(message)
        # TODO: if message.startswith("show") # join a game and show information about it

class change_notifier(threading.Thread):
    display_game_updated = False
    display_game_closed = False
    timestamps = {}

    def __init__(self, irc, lobby):
        threading.Thread.__init__(self)
        self.irc = irc
        self.lobby = lobby
        self.games = None

    def run(self):
        while True:
            new_games = self.lobby.list()
            if self.games == None:
                self.games = {}
                for g in new_games:
                    self.games[g.host] = g
            else:
                new_game_dict = {}
                for g in new_games:
                    if g.host in self.games and g.announce == True:
                        g.announce = False
                        message = "New game: \x02%s\x02 (%i players)" % (g.description, g.maxPlayers)
                        if not g.host in self.timestamps:
                            self.irc.write(message)
                        else:
                            if self.timestamps[g.host] + 300 < time.time():
                                self.irc.write(message)
                        self.timestamps[g.host] = time.time()
                    else:
                        if self.display_game_updated:
                            if ((g.description != self.games[g.host].description) or
                               (g.maxPlayers  != self.games[g.host].maxPlayers)  or
                               (g.currentPlayers  != self.games[g.host].currentPlayers)):
                                self.irc.write("Updated game: %s [%i/%i]" % (g.description, g.currentPlayers, g.maxPlayers))
                    new_game_dict[g.host] = g
                if self.display_game_closed:
                    for h, g in self.games.iteritems():
                        if h not in new_game_dict:
                            self.irc.write("Game closed: %s [%i/%i]" % (g.description, g.currentPlayers, g.maxPlayers))
                self.games = new_game_dict
            time.sleep(10)

class masterserver_connection:
    host = None
    port = None

    def __init__(self, host, port):
        self.host = host
        self.port = port

    def list(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect( (self.host, self.port) )

        self.send(s, "list")
        (count,) = self.recv(s, "!I")
        games = []
        for i in xrange(0,count):
            (description, size, flags, host, maxPlayers, currentPlayers,
            user1, user2, user3, user4 ) = self.recv(s, "!64sII16sIIIIII")
            description = description.strip("\0")
            host = host.strip("\0")
            if maxPlayers > 100:
                maxPlayers = swap_endianness(maxPlayers)
                currentPlayers = swap_endianness(currentPlayers)
            g = game(description, host, maxPlayers, currentPlayers)
            games.append(g)

        s.close()
        return games

    def send(self, s, command):
        s.send(command)
        s.send("\0")

    def recv(self, s, format):
        data = struct.Struct(format)
        return data.unpack(s.recv(data.size))

class game:
    announce = True

    def __init__(self, description, host, maxPlayers, currentPlayers):
        self.description = description
        self.host = host
        self.maxPlayers = maxPlayers
        self.currentPlayers = currentPlayers

class bot_connection:
    nick = None
    connection = None

    def __init__(self, server, port, channel, nick):
        self.connection = irc_connection(server, port, channel, nick)
        self.nick = nick

    def read(self):
        while True:
            line = self.connection.read()
            if line.find(self.nick) >= 0:
                return line.replace(self.nick, "").strip(" \t,:.?!")

    def write(self, line):
        self.connection.write(line)

class irc_connection:
    s = None
    channel = None

    def __init__(self, server, port, channel, nick):
        self.s = line_socket(server, port)
        self.channel = channel
        self.s.write("NICK "+nick)
        self.s.write("USER "+nick+" 0 * :Lobby Bot")
        self.s.write("JOIN "+channel)

    def read(self):
        while True:
            line = self.s.read()
            parts = line.split("PRIVMSG "+self.channel+" :")
            if len(parts) > 1:
                return parts[1]

    def write(self, line):
        self.s.write("PRIVMSG "+self.channel+" :"+line)


class line_socket:
    buffer = ""
    s = None

    def __init__(self, host, port):
        self.s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.s.connect( (host, port) )

    def read(self):
        while True:
            pos = self.buffer.find("\r\n")
            if pos > 0:
                line = self.buffer[:pos]
                self.buffer = self.buffer[pos+2:]
                print "read:", line
                return  line
            else:
                self.buffer = self.buffer + self.s.recv(1)

    def write(self, line):
        print "write:", line
        self.s.send(line);
        self.s.send("\r\n");


def swap_endianness(i):
    return struct.unpack(">I", struct.pack("<I", i))

if __name__ = "__main__":
    main()
