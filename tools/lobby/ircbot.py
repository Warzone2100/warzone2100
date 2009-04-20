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

__author__  = "The Warzone Resurrection Project"
__version__ = "0.0.1"
__license__ = "GPL"

import re
import socket
import struct
import threading
import time
from client import *

irc_server          = "irc.freenode.net"
irc_port            = 6667
irc_serve_channels  = ['#warzone2100-games']
irc_silent_channels = ['#warzone2100']
irc_nick            = "wzlobbybot"
irc_nickpass        = None

def main():
    bot = Bot(irc_server, irc_port, irc_serve_channels, irc_silent_channels, irc_nick, irc_nickpass, "lobby.wz2100.net", 9997)
    bot.start()

class Bot:
    def __init__(self, ircServer, ircPort, ircServeChannels, ircSilentChannels, ircNick, nickPass, lobbyServer, lobbyPort):
        self.commands     = BotCommands()
        self.commands.bot = self
        self.irc          = bot_connection(self.commands, ircServer, ircPort, ircServeChannels, ircSilentChannels, ircNick, nickPass)
        self.lobby        = masterserver_connection(lobbyServer, lobbyPort, version='2.1')
        self.check        = change_notifier(self.irc, self.lobby)

    def start(self):
        self.check.start()
        try:
            while True:
                self.irc.readAndHandle()
        except KeyboardInterrupt:
            print "Shutting down. Waiting for lobby change notifier to terminate."
            self.stop()
            raise

    def stop(self):
        self.check.stop()
        self.check.join()

    def write(self, text, channels = None):
        return self.irc.write(text, channels)

class BotCommands:
    def ping(self, nick, channel):
        self.bot.write("%s: pong" % (nick), [channel])

    def help(self, nick, channel):
        self.bot.write("%s: I'm a bot that shows information from the \x02Warzone 2100\x02 lobby server. I was created by %s. For information about commands you can try: \"commands\"" % (nick, __author__), [channel])

    # Alias 'info' to 'help'
    info = help

    def commands(self, nick, channel):
        self.bot.write("%s: ping: pong, help/info: general information about this bot, list: show which games are currenly being hosted" % (nick), [channel])

    def list(self, nick, channel):
        try:
            games = self.bot.lobby.list(allowException = True)
            if not games:
                self.bot.write("%s: No games in lobby" % (nick), [channel])
            else:
                if len(games) == 1:
                    message = "1 game hosted: "
                else:
                    message = "%i games hosted: " % (len(games))
                l = ["\x02%s\x02 [%i/%i]" % (g.description, g.currentPlayers, g.maxPlayers) for g in games]
                message += ", ".join(l)
                self.bot.write("%s: %s" % (nick, message), [channel])
        except socket.timeout:
            self.bot.write("%s: Failed to communicate with the lobby (%s:%d)" % (nick, self.bot.lobby.host, self.bot.lobby.port), [channel])

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
        self.stopChecking = threading.Event()

    def stop(self):
        self.stopChecking.set()

    def run(self):
        self.stopChecking.clear()
        while not self.stopChecking.isSet():
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

            # Wait until we're requested to stop or we're ready for the next lobby update
            self.stopChecking.wait(10)

class bot_connection:
    nick = None
    connection = None

    def __init__(self, bot, server, port, serve_channels, silent_channels, nick, nickpass = None):
        self.connection = irc_connection(bot, server, port, serve_channels, silent_channels, nick, nickpass)
        self.nick = nick

    def readAndHandle(self):
        """Read a message from IRC and handle it."""
        return self.connection.readAndHandle()

    def write(self, line, channels = None):
        return self.connection.write(line, channels)

class irc_connection:
    def __init__(self, bot, server, port, serve_channels, silent_channels, nick, nickpass = None):
        self.bot = bot
        self.s = line_socket(server, port)
        self.nick = nick
        self.serve_channels  = serve_channels
        self.silent_channels = silent_channels
        self.s.write("NICK "+nick)
        self.s.write("USER %s 0 * :Warzone 2100 Lobby Bot ( http://wz2100.net/ )" % (nick))
        for channel in self.serve_channels + self.silent_channels:
            self.join(channel)
        if nickpass:
            self.privmsg('NICKSERV', 'IDENTIFY %s' % (nickpass))

        serve_channel_re = r'(?P<channel>%s)' % ('|'.join([re.escape(channel) for channel in self.serve_channels]))

        self.private_channel_message = re.compile(r'^:(?P<nick>[^!]+)\S*\s+PRIVMSG (?P<channel>#[^:]+) :[ \t,:.?!]*%s[ \t,:.?!]*(?P<message>.*?)[ \t,:.?!]*$' % (nick))
        self.chan_mode_re = re.compile(r'^:(?P<nick>[^!]+)\S*\s+MODE %s (?P<mode>(?:[-+][oOpPiIsStTnNmMlLbBvVkK])+) %s\s*$' % (serve_channel_re, nick))

        self.on_op = {}

    def invite(self, user, channel):
        self.as_op(lambda x: self.s.write("INVITE %s %s" % (user, channel)), channel)

    def join(self, channel):
        self.s.write("JOIN %s" % (channel))

    def notice(self, recipient, message):
        self.s.write("NOTICE %s :%s" % (recipient, message))

    def privmsg(self, recipient, message):
        self.s.write("PRIVMSG %s :%s" % (recipient, message))

    def as_op(self, functor, channel):
        self.op(channel)
        if not channel in self.on_op:
            self.on_op[channel] = []
        self.on_op[channel].append(functor)

    def op(self, channel):
        self.privmsg('CHANSERV', 'OP %s %s' % (channel, self.nick))

    def deop(self, channel):
        self.privmsg('CHANSERV', 'OP %s -%s' % (channel, self.nick))

    def readAndHandle(self):
        """Read a message from IRC and handle it."""
        line = self.s.read()

        m = self.chan_mode_re.match(line)
        if m:
            mode    = m.group('mode')
            channel = m.group('channel')
            print "Mode change \"%s\"" % (mode)

            l = mode.rfind('o')
            if l <= 0:
                l = mode.rfind('O')

            if l > 0 and mode[l - 1] == '+':
                for op in self.on_op[channel]:
                    op(0)
                self.on_op[channel] = []
                self.deop(channel)

        m = self.private_channel_message.match(line)
        if m:
            nick    = m.group('nick')
            channel = m.group('channel')
            message = m.group('message')
            if channel in self.serve_channels:
                func = None
                try:
                    func = getattr(self.bot, message)
                except AttributeError:
                    self.privmsg(channel, '%s: Unknown command \'%s\'. Try \'commands\'.' % (nick, message))

                if func:
                    func(nick, channel)
            else:
                # Not the serviced channel, point the user at the correct channel
                self.notice(nick, 'Sorry %s, I will not provide my services in this channel. Please find me in one of %s' % (nick, ', '.join(self.serve_channels)))
                for channel in self.serve_channels:
                    self.invite(nick, channel)

    def write(self, line, channels = None):
        if not channels:
            channels = self.serve_channels
        for channel in channels:
            self.privmsg(channel, line)


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


if __name__ == "__main__":
    main()
