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

from __future__ import with_statement
from contextlib import contextmanager
import socket
import struct

__all__ = ['masterserver_connection', 'game']

@contextmanager
def _socket(family = socket.AF_INET, type = socket.SOCK_STREAM, proto = 0):
    s = socket.socket(family, type, proto)
    try:
        yield s
    finally:
        s.close()

def _swap_endianness(i):
    return struct.unpack(">I", struct.pack("<I", i))

class masterserver_connection:
    host = None
    port = None

    def __init__(self, host, port):
        self.host = host
        self.port = port

    def list(self):
        with _socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect( (self.host, self.port) )

            self._send(s, "list")
            (count,) = self._recv(s, "!I")
            games = []
            for i in xrange(0,count):
                (description, size, flags, host, maxPlayers, currentPlayers,
                user1, user2, user3, user4 ) = self._recv(s, "!64sII16sIIIIII")
                description = description.strip("\0")
                host = host.strip("\0")
                # Workaround for the fact that some of the
                # 2.0.x versions don't perform endian swapping
                if maxPlayers > 100:
                    maxPlayers = _swap_endianness(maxPlayers)
                    currentPlayers = _swap_endianness(currentPlayers)
                g = game(description, host, maxPlayers, currentPlayers)
                games.append(g)

            return games

    def _send(self, s, command):
        s.send(command)
        s.send("\0")

    def _recv(self, s, format):
        data = struct.Struct(format)
        return data.unpack(s.recv(data.size))

class game:
    def __init__(self, description, host, maxPlayers, currentPlayers):
        self.description = description
        self.host = host
        self.maxPlayers = maxPlayers
        self.currentPlayers = currentPlayers

def main():
    lobbies = [(version, masterserver_connection("lobby.wz2100.net", port).list()) for (version, port) in [("2.0", 9998),
                                                                                                           ("2.1", 9997)]]
    for (version, lobby) in lobbies:
        print "%s lobby" % version
        print lobby

if __name__ == "__main__":
    main()
