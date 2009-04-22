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
################################################################################
# import from __future__
from __future__ import with_statement

#
################################################################################
# Get the things we need.
import struct
from game import *

__all__ = ['Protocol', 'BinaryProtocol']

class Protocol(object):
	# Size of a single game is undefined at compile time
	size = None

	def encodeSingle(data):
		pass

	def encodeMultiple(data):
		pass

	def decodeSingle(data):
		pass

	def decodeMultiple(data):
		pass

class BinaryProtocol(Protocol):
	name_length          = 64
	host_length          = 16
	misc_length          = 64
	extra_length         = 255
	versionstring_length = 64
	modlist_length       = 255
	# Binary struct format to use (GAMESTRUCT)
	gameFormat = '!%dsII%ds6I%ds%ds%ds%ds10I' % (name_length, host_length, misc_length, extra_length, versionstring_length, modlist_length)

	# Size of a single game = sizeof(GAMESTRUCT)
	size = struct.calcsize(gameFormat)

	countFormat = '!I'
	countSize = struct.calcsize(countFormat)

	def _encodeName(self, game):
		return game.description[:self.name_length - 1].ljust(self.name_length, "\0")

	def _encodeHost(self, game):
		return game.host[:self.host_length - 1].ljust(self.host_length, "\0")

	def _encodeMisc(self, game):
		return game.misc[:self.misc_length - 1].ljust(self.misc_length, "\0")

	def _encodeExtra(self, game):
		return game.extra[:self.extra_length - 1].ljust(self.extra_length, "\0")

	def _encodeVersionString(self, game):
		return game.multiplayerVersion[:self.versionstring_length - 1].ljust(self.versionstring_length, "\0")

	def encodeSingle(self, game):
		return struct.pack(self.gameFormat,
			self._encodeName(game),
			game.size or self.size, game.flags,
			self._encodeHost(game),
			game.maxPlayers, game.currentPlayers, game.user1, game.user2, game.user3, game.user4,
			self._encodeMisc(game),
			self._encodeExtra(game),
			self._encodeVersionString(game),
			game.modlist,
			game.lobbyVersion, game.game_version_major, game.game_version_minor, game.private,
			game.pure, game.Mods, game.future1, game.future2, game.future3, game.future4)

	def encodeMultiple(self, games):
		message = struct.pack(self.countFormat, len(games))
		for game in games:
			message += self.encodeSingle(game)
		return message

	def decodeSingle(self, data, game = Game(None)):
		decData = {}

		(decData['name'], game.size, game.flags, decData['host'], game.maxPlayers, game.currentPlayers,
			game.user1, game.user2, game.user3, game.user4,
			game.misc, game.extra, decData['multiplayer-version'], game.modlist, game.lobbyVersion,
			game.game_version_major, game.game_version_minor, game.private, game.pure, game.Mods, game.future1,
			game.future2, game.future3, game.future4) = struct.unpack(self.gameFormat, data)

		for strKey in ['name', 'host', 'multiplayer-version']:
			decData[strKey] = decData[strKey].strip("\0")

		game.misc = game.misc.strip("\0")
		game.extra = game.extra.strip("\0")
		game.modlist = game.modlist.strip("\0")

		game.data.update(decData)
		return game

	def decodeMultiple(self, data):
		countMsg = data[:countSize]
		data = data[countSize:]
		count = struct.pack(self.countFormat, countMsg)
		return [self.decodeSingle(data[size * i: size * i + size]) for i in range(count)]
