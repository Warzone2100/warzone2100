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
from contextlib import contextmanager
import logging
import socket
import struct
from StringIO import StringIO
from pkg_resources import parse_version
from game import *

__all__ = ['Protocol']

@contextmanager
def _socket(family = socket.AF_INET, type = socket.SOCK_STREAM, proto = 0):
	s = socket.socket(family, type, proto)
	try:
		yield s
	finally:
		s.close()

def _encodeCString(string, buf_len):
	# If we haven't got a string return an empty one
	if not string:
		return str('\0' * buf_len)

	# Make sure the string fits
	string = string[:buf_len - 1]

	# Add a terminating NUL and fill the rest of the buffer with NUL bytes
	string = string.ljust(buf_len, "\0")

	# Make sure *not* to use a unicode string
	return str(string)

def _swap_endianness(i):
	return struct.unpack(">I", struct.pack("<I", i))

@contextmanager
def writeable(output):
	if   hasattr(output, 'write'):
		# File IO
		yield output
	else:
		try:
			# Socket IO
			io = output.makefile()
			yield io
		except AttributeError:
			# String, lets use StringIO instead
			io = StringIO(output)
			yield io
		finally:
			io.close()

@contextmanager
def readable(input):
	if   hasattr(input, 'read'):
		# File IO
		yield input
	else:
		try:
			# Socket IO
			io = input.makefile()
			yield io
		except AttributeError:
			# String IO (no, not the StringIO class, that would have been matched as "File IO")
			io = None
			yield io
		finally:
			if io:
				io.close()

class BaseProtocol(object):
	# Gameserver port.
	gamePort = {'2.0': 9999,
	            '2.2': 2100}
	# Lobby server port.
	lobbyPort = {'2.0': 9998,
	             '2.1': 9997,
	             '2.2': 9990}

	def __init__(self, version):
		self.version = version

	def encodeSingle(data):
		pass

	def encodeMultiple(data):
		pass

	def decodeSingle(data, game = Game()):
		pass

	def decodeMultiple(data):
		pass

	def check(self, game):
		"""Check we can connect to the game's host."""
		with _socket() as s:
			try:
				logging.debug("Checking %s's vitality..." % game.host)
				s.settimeout(10.0)
				s.connect((game.host, self.gamePort))
				return True
			except (socket.error, socket.herror, socket.gaierror, socket.timeout), e:
				logging.debug("%s did not respond: %s" % (game.host, e))
				return False

	def list(self, host):
		"""Retrieve a list of games from the lobby server."""
		with _socket() as s:
			s.settimeout(10.0)
			s.connect((host, self.lobbyPort))

			s.send("list\0")
			for game in self.decodeMultiple(s):
				yield game

class BinaryProtocol20(BaseProtocol):
	# Binary struct format to use (GAMESTRUCT)

	# >= 2.0 data
	name_length          = 64
	host_length          = 16

	gameFormat = struct.Struct('!%dsII%ds6I' % (name_length, host_length))

	countFormat = struct.Struct('!I')

	size = property(fget = lambda self: self.gameFormat.size)

	gamePort = BaseProtocol.gamePort['2.0']
	lobbyPort = BaseProtocol.lobbyPort['2.0']

	def _encodeName(self, game):
		return _encodeCString(game.description, self.name_length)

	def _encodeHost(self, game):
		return _encodeCString(game.host, self.host_length)

	def encodeSingle(self, game, out = str()):
		with writeable(out) as write:
			# Workaround the fact that the 2.0.x versions don't
			# perform endian swapping
			maxPlayers     = _swap_endianness(maxPlayers)
			currentPlayers = _swap_endianness(currentPlayers)

			write.write(self.gameFormat.pack(
				self._encodeName(game),
				game.size or self.size, game.flags,
				self._encodeHost(game),
				maxPlayers, currentPlayers, game.user1, game.user2, game.user3, game.user4))

			try:
				return write.getvalue()
			except AttributeError:
				return out

	def encodeMultiple(self, games, out = str()):
		with writeable(out) as write:
			write.write(self.countFormat.pack(len(games)))
			for game in games:
				self.encodeSingle(game, write)

			try:
				return write.getvalue()
			except AttributeError:
				return out

	def decodeSingle(self, input, game = Game(), offset = None):
		with readable(input) as read:
			decData = {}

			if   offset != None and read == None:
				def unpack():
					return self.gameFormat.unpack_from(out, offset)
			elif not read:
				def unpack():
					return self.gameFormat.unpack(input)
			else:
				def unpack():
					data = read.read(self.size)
					if len(data) != self.size:
						return None
					return self.gameFormat.unpack(data)

			data = unpack()
			if not data:
				return None

			(decData['name'], game.size, game.flags, decData['host'], game.maxPlayers, game.currentPlayers,
				game.user1, game.user2, game.user3, game.user4) = data

			# Workaround the fact that the 2.0.x versions don't
			# perform endian swapping
			game.maxPlayers = _swap_endianness(game.maxPlayers)
			game.currentPlayers = _swap_endianness(game.currentPlayers)

			for strKey in ['name', 'host']:
				decData[strKey] = decData[strKey].strip("\0")

			game.data.update(decData)
			return game

	def decodeMultiple(self, input):
		with readable(input) as read:
			if read:
				(count,) = self.countFormat.unpack(read.read(self.countFormat.size))
			else:
				(count,) = self.countFormat.unpack_from(input)

			for i in xrange(count):
				game = self.decodeSingle(read or input, offset=(self.size * i + self.countFormat.size))
				if game is None:
					return
				yield game

class BinaryProtocol21(BinaryProtocol20):
	lobbyPort = BaseProtocol.lobbyPort['2.1']

	def encodeSingle(self, game, out = str()):
		with writeable(out) as write:
			write.write(self.gameFormat.pack(
				self._encodeName(game),
				game.size or self.size, game.flags,
				self._encodeHost(game),
				game.maxPlayers, game.currentPlayers, game.user1, game.user2, game.user3, game.user4))

			try:
				return write.getvalue()
			except AttributeError:
				return out

	def decodeSingle(self, input, game = Game(), offset = None):
		with readable(input) as read:
			decData = {}

			if   offset != None and read == None:
				def unpack():
					return self.gameFormat.unpack_from(out, offset)
			elif not read:
				def unpack():
					return self.gameFormat.unpack(input)
			else:
				def unpack():
					data = read.read(self.size)
					if len(data) != self.size:
						return None
					return self.gameFormat.unpack(data)

			data = unpack()
			if not data:
				return None

			(decData['name'], game.size, game.flags, decData['host'], game.maxPlayers, game.currentPlayers,
				game.user1, game.user2, game.user3, game.user4) = data

			for strKey in ['name', 'host']:
				decData[strKey] = decData[strKey].strip("\0")

			game.data.update(decData)
			return game

class BinaryProtocol22(BinaryProtocol21):
	# >= 2.2 data
	misc_length          = 64
	extra_length         = 255
	versionstring_length = 64
	modlist_length       = 255

	gameFormat = struct.Struct(BinaryProtocol21.gameFormat.format + '%ds%ds%ds%ds10I' % (misc_length, extra_length, versionstring_length, modlist_length))

	gamePort = BaseProtocol.gamePort['2.2']
	lobbyPort = BaseProtocol.lobbyPort['2.2']

	def _encodeMisc(self, game):
		return _encodeCString(game.misc, self.misc_length)

	def _encodeExtra(self, game):
		return _encodeCString(game.extra, self.extra_length)

	def _encodeVersionString(self, game):
		return _encodeCString(game.multiplayerVersion, self.versionstring_length)

	def encodeSingle(self, game, out = str()):
		with writeable(out) as write:
			write.write(self.gameFormat.pack(
				self._encodeName(game),
				game.size or self.size, game.flags,
				self._encodeHost(game),
				game.maxPlayers, game.currentPlayers, game.user1, game.user2, game.user3, game.user4,
				self._encodeMisc(game),
				self._encodeExtra(game),
				self._encodeVersionString(game),
				game.modlist,
				game.lobbyVersion, game.game_version_major, game.game_version_minor, game.private,
				game.pure, game.Mods, game.future1, game.future2, game.future3, game.future4))

			try:
				return write.getvalue()
			except AttributeError:
				return out

	def decodeSingle(self, input, game = Game(), offset = None):
		with readable(input) as read:
			decData = {}

			if   offset != None and read == None:
				def unpack():
					return self.gameFormat.unpack_from(out, offset)
			elif not read:
				def unpack():
					return self.gameFormat.unpack(input)
			else:
				def unpack():
					data = read.read(self.size)
					if len(data) != self.size:
						return None
					return self.gameFormat.unpack(data)

			data = unpack()
			if not data:
				return None

			(decData['name'], game.size, game.flags, decData['host'], game.maxPlayers, game.currentPlayers,
				game.user1, game.user2, game.user3, game.user4,
				game.misc, game.extra, decData['multiplayer-version'], game.modlist, game.lobbyVersion,
				game.game_version_major, game.game_version_minor, game.private, game.pure, game.Mods, game.future1,
				game.future2, game.future3, game.future4) = data

			for strKey in ['name', 'host', 'multiplayer-version']:
				decData[strKey] = decData[strKey].strip("\0")

			game.misc = game.misc.strip("\0")
			game.extra = game.extra.strip("\0")
			game.modlist = game.modlist.strip("\0")

			game.data.update(decData)
			return game

def Protocol(version = '2.2'):
	if   parse_version('2.0') <= parse_version(version) < parse_version('2.1'):
		return BinaryProtocol20(version)
	elif parse_version('2.1') <= parse_version(version) < parse_version('2.2'):
		return BinaryProtocol21(version)
	elif parse_version('2.2') <= parse_version(version):
		return BinaryProtocol22(version)
