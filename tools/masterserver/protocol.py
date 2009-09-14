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
		if family == socket.AF_INET6:
			# Ensure that connecting to IPv6-mapped IPv4 addresses works as well
			s.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 0)
		yield s
	finally:
		s.close()

@contextmanager
def connection(host, port, family = socket.AF_UNSPEC, socktype = socket.SOCK_STREAM):
	addrs = socket.getaddrinfo(host, port, family, socktype)
	for num, addr in enumerate(addrs):
		(family, socktype, proto, canonname, sockaddr) = addr
		with _socket(family, socktype, proto) as s:
			timeout = s.gettimeout()
			s.settimeout(10.0)
			try:
				s.connect(sockaddr)
			except socket.timeout:
				if num == (len(addrs) - 1):
					raise
				continue
			s.settimeout(timeout)

			yield s
			return

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
	return struct.unpack(">I", struct.pack("<I", i))[0]

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

	def encodeGameID(gameId):
		pass

	def sendStatusMessage(host, status, message):
		pass

	def check(self, game, host):
		"""Check we can connect to the game's host."""
		try:
			logging.debug("Checking %s's vitality..." % host)
			with connection(host, self.gamePort) as s:
				return True
		except (socket.error, socket.herror, socket.gaierror, socket.timeout), e:
			logging.debug("%s did not respond: %s" % (host, e))
			return False

	def list(self, host):
		"""Retrieve a list of games from the lobby server."""
		with connection(host, self.lobbyPort) as s:
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

	def _encodeHost(self, game, hostnum = 0):
		if hostnum < len(game.hosts):
			return _encodeCString(game.hosts[hostnum], self.host_length)
		else:
			return _encodeCString(str(), self.host_length)

	def encodeSingle(self, game, out = str(), hideGameID = False):
		with writeable(out) as write:
			# Workaround the fact that the 2.0.x versions don't
			# perform endian swapping
			maxPlayers     = _swap_endianness(game.maxPlayers)
			currentPlayers = _swap_endianness(game.currentPlayers)

			write.write(self.gameFormat.pack(
				self._encodeName(game),
				game.size or self.size, game.flags,
				self._encodeHost(game),
				maxPlayers, currentPlayers, game.user1, game.user2, game.user3, game.user4))

			try:
				return write.getvalue()
			except AttributeError:
				return out

	def encodeMultiple(self, games, out = str(), hideGameID = False):
		with writeable(out) as write:
			write.write(self.countFormat.pack(len(games)))
			for game in games:
				self.encodeSingle(game, write, hideGameID)

			try:
				return write.getvalue()
			except AttributeError:
				return out

	def decodeSingle(self, input, game = Game(), offset = None):
		with readable(input) as read:
			decData = {'hosts': [None]}

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

			(decData['name'], game.size, game.flags, decData['hosts'][0], game.maxPlayers, game.currentPlayers,
				game.user1, game.user2, game.user3, game.user4) = data

			# Workaround the fact that the 2.0.x versions don't
			# perform endian swapping
			game.maxPlayers = _swap_endianness(game.maxPlayers)
			game.currentPlayers = _swap_endianness(game.currentPlayers)

			decData['name'] = decData['name'].strip("\0")
			decData['hosts'][0] = decData['hosts'][0].strip("\0")

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

	def encodeGameID(self, gameId, out = str()):
		"""Unsupported by this protocol version."""
		raise NotImplementedError

	def sendStatusMessage(self, host, status, message, out = str()):
		"""Unsupported by this protocol version."""
		raise NotImplementedError

class BinaryProtocol21(BinaryProtocol20):
	lobbyPort = BaseProtocol.lobbyPort['2.1']

	def encodeSingle(self, game, out = str(), hideGameID = False):
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
			decData = {'hosts': [None]}

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

			(decData['name'], game.size, game.flags, decData['hosts'][0], game.maxPlayers, game.currentPlayers,
				game.user1, game.user2, game.user3, game.user4) = data

			decData['name'] = decData['name'].strip("\0")
			decData['hosts'][0] = decData['hosts'][0].strip("\0")

			game.data.update(decData)
			return game

class BinaryProtocol22(BinaryProtocol21):
	# changes to >= 2.0 data
	host_length          = 40

	# >= 2.2 data
	extra_length         = 239
	versionstring_length = 64
	modlist_length       = 255

	gameFormat = struct.Struct('!I%dsii%ds6i%ds%ds%ds%ds%ds9I' % (BinaryProtocol21.name_length, host_length, host_length, host_length, extra_length, versionstring_length, modlist_length))

	gamePort = BaseProtocol.gamePort['2.2']
	lobbyPort = BaseProtocol.lobbyPort['2.2']

	def _encodeExtra(self, game):
		return _encodeCString(game.extra, self.extra_length)

	def _encodeVersionString(self, game):
		return _encodeCString(game.multiplayerVersion, self.versionstring_length)

	def encodeSingle(self, game, out = str(), hideGameID = False):
		with writeable(out) as write:
			if hideGameID:
				gameId = 0
			else:
				gameId = game.gameId
			write.write(self.gameFormat.pack(
				game.lobbyVersion,
				self._encodeName(game),
				game.size or self.size, game.flags,
				self._encodeHost(game, 0),
				game.maxPlayers, game.currentPlayers, game.user1, game.user2, game.user3, game.user4,
				self._encodeHost(game, 1),
				self._encodeHost(game, 2),
				self._encodeExtra(game),
				self._encodeVersionString(game),
				game.modlist,
				game.game_version_major, game.game_version_minor, game.private,
				game.pure, game.Mods, gameId, game.future2, game.future3, game.future4))

			try:
				return write.getvalue()
			except AttributeError:
				return out

	def decodeSingle(self, input, game = Game(), offset = None):
		with readable(input) as read:
			decData = {'hosts': [None, None, None]}

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

			(game.lobbyVersion, decData['name'], game.size, game.flags, decData['hosts'][0], game.maxPlayers, game.currentPlayers,
				game.user1, game.user2, game.user3, game.user4,
				decData['hosts'][1], decData['hosts'][2], game.extra, decData['multiplayer-version'], game.modlist,
				game.game_version_major, game.game_version_minor, game.private, game.pure, game.Mods, game.gameId,
				game.future2, game.future3, game.future4) = data

			for strKey in ['name', 'multiplayer-version']:
				decData[strKey] = decData[strKey].strip("\0")
			for i in xrange(len(decData['hosts'])):
				decData['hosts'][i] = decData['hosts'][i].strip("\0")
			decData['hosts'] = filter(bool, decData['hosts'])

			game.extra = game.extra.strip("\0")
			game.modlist = game.modlist.strip("\0")

			game.data.update(decData)
			return game

	def encodeGameID(self, gameId, out = str()):
		with writeable(out) as write:
			write.write(struct.pack('!I', gameId))

			try:
				return write.getvalue()
			except AttributeError:
				return out

	def sendStatusMessage(self, host, status, message, out = str()):
		with writeable(out) as write:
			write.write(struct.pack('!II%ds' % len(message), status, len(message), message))

			try:
				return write.getvalue()
			except AttributeError:
				return out

def Protocol(version = '2.2'):
	if   parse_version('2.0') <= parse_version(version) < parse_version('2.1'):
		return BinaryProtocol20(version)
	elif parse_version('2.1') <= parse_version(version) < parse_version('2.2'):
		return BinaryProtocol21(version)
	elif parse_version('2.2') <= parse_version(version):
		return BinaryProtocol22(version)
