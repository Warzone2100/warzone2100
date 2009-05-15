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
import socket
import logging
#
################################################################################
# Settings.

__all__ = ['Game']

#
################################################################################
# Game class
# NOTE: This must match exactly what we have defined for GAMESTRUCT in netplay.h
# The structure MUST be packed network byte order !

class Game(object):
	""" class for a single game """

	def _setDescription(self, v):
		self.data['name'] = unicode(v)

	def _setHosts(self, v):
		# Make sure that 'v' is a list
		self.data['hosts'] = list(v)

	def _setMaxPlayers(self, v):
		self.data['maxPlayers'] = int(v)

	def _setCurrentPlayers(self, v):
		self.data['currentPlayers'] = int(v)

	def _setLobbyVersion(self, v):
		self.data['lobby-version'] = int(v)

	def _setMultiplayerVersion(self, v):
		self.data['multiplayer-version'] = unicode(v)

	def _setWarzoneVersion(self, v):
		self.data['warzone-version'] = unicode(v)

	def _setPure(self, v):
		self.data['pure'] = bool(v)

	def _setPrivate(self, v):
		self.data['private'] = bool(v)

	def _setAnnounce(self, v):
		self.data['announced'] = not bool(v)

	def _setGameId(self, v):
		self.data['gameId'] = int(v)

	description        = property(fget = lambda self:    self.data['name'],
	                              fset = lambda self, v: self._setDescription(v))

	hosts              = property(fget = lambda self:    self.data['hosts'],
	                              fset = lambda self, v: self._setHosts(v))

	maxPlayers         = property(fget = lambda self:    self.data['maxPlayers'],
	                              fset = lambda self, v: self._setMaxPlayers(v))

	currentPlayers     = property(fget = lambda self:    self.data['currentPlayers'],
	                              fset = lambda self, v: self._setCurrentPlayers(v))

	lobbyVersion       = property(fget = lambda self:    self.data['lobby-version'],
	                              fset = lambda self, v: self._setLobbyVersion(v))

	multiplayerVersion = property(fget = lambda self:    self.data['multiplayer-version'],
	                              fset = lambda self, v: self._setMultiplayerVersion(v))

	warzoneVersion     = property(fget = lambda self:    self.data['warzone-version'],
	                              fset = lambda self, v: self._setWarzoneVersion(v))

	pure               = property(fget = lambda self:    self.data['pure'],
	                              fset = lambda self, v: self._setPure(v))

	private            = property(fget = lambda self:    self.data['private'],
	                              fset = lambda self, v: self._setPrivate(v))

	announce           = property(fget = lambda self: not self.private and (not 'announced' in self.data or not self.data['announced']),
	                              fset = lambda self, v: self._setAnnounce(v))

	gameId             = property(fget = lambda self:    self.data['gameId'],
	                              fset = lambda self, v: self._setGameId(v))
	def __init__(self):
		self.data = {'name':                None,
		             'hosts':               [],
		             'maxPlayers':          None,
		             'currentPlayers':      None,
		             'lobby-version':       None,
		             'multiplayer-version': None,
		             'warzone-version':     None,
		             'pure':                None,
		             'private':             None,
		             'gameId':              0}
		self.size = None
		self.flags = None
		self.user1 = None
		self.user2 = None
		self.user3 = None
		self.user4 = None
		self.misc = None				# 64  byte misc string
		self.extra = None				# 255 byte extra string (future use)
		self.modlist = None			# 255 byte string
		self.game_version_major = None	# game major version
		self.game_version_minor = None	# game minor version
		self.Mods = None				# number of concatenated mods they have (list of mods is in modlist)
		self.future2 = None			# for future use
		self.future3 = None			# for future use
		self.future4 = None			# for future use

	def __str__(self):
		if self.private == 1:
		   return "(private) Game: %s %s %s %s" % (self.hosts, self.description, self.maxPlayers, self.currentPlayers)
		else:
		   return "Game: %s %s %s %s" % (self.hosts, self.description, self.maxPlayers, self.currentPlayers)
