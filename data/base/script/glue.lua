--[[
	This file is part of Warzone 2100.
	Copyright (C) 2005-2008  Warzone Resurrection Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
]]--


-- FIXME: HACKS
structChoice = {} -- bug in skirmish scripts, this variable should have been PRIVATE in the .slo

-- to obtain information about undefined members
__undefined_meta = {}
function __undefined_meta.__index(table, key)
	local name
	val = rawget(table, key)
	if not val then
		name = _save.search(table)
		if name then
			name = "\""..name.."\""
		else
			name = "<local table>"
		end
		error(name .. " does not contain member \"" .. key .. "\"", 2)
	end
	return val
end

-- Some legacy names
EnumDroid = enumDroid
InitEnumDroids = initEnumDroids

-- This are script api functions which are written in Lua

function setLandingZone(x1, y1, x2, y2)
    setNoGoArea(x1, y1, x2, y2, 0)
end

function random(upper)
	return math.random(0, upper-1)
end
math.randomseed(os.time())

function distBetweenTwoPoints(x1, y1, x2, y2)
	return math.sqrt( math.pow(x1-x2, 2) + math.pow(y1-y2, 2) )
end

function ASSERT(check, message, me)
	if not check then error("Player "..me..": "..message, 2) end
end

function objToStructure(object)
	if not object.type == "structure" then error("object is not a structure", 2) end
	return object
end

function objToDroid(object)
	if not object.type == "droid" then error("object is not a droid", 2) end
	return object
end

-- Functions for counting things in a range
-- all objects
function scrNumFriendlyWeapObjInRange(me, x, y, range, vtols, finished)
	local count
	count =         numFriendlyWeapDroidsInRange(me, x, y, range, vtols)
	count = count + numFriendlyWeapStructsInRange(me, x, y, range, finished) 
	return count
end

function scrNumPlayerWeapObjInRange(player, me, x, y, range, vtols, finished)
	local count
	count =         numPlayerWeapDroidsInRange(player, me, x, y, range, vtols)
	count = count + numPlayerWeapStructsInRange(player, me, x, y, range, finished) 
	return count
end

-- droids
function numWeapDroidsInRange(me, x, y, range, vtols, friendly)
	local count = 0
	local ally
	for player=0,MAX_PLAYERS-1 do
		ally = allianceExistsBetween(me, player) or me == player
		if (ally and friendly) or (not ally and not friendly) then
			count = count + numPlayerWeapDroidsInRange(player, me, x, y, range, vtols) 
		end
	end
	return count
end
function numEnemyWeapDroidsInRange(me, x, y, range, vtols)
	return numWeapDroidsInRange(me, x, y, range, vtols, false)
end
function numFriendlyWeapDroidsInRange(me, x, y, range, vtols)
	return numWeapDroidsInRange(me, x, y, range, vtols, true)
end

-- structs
function numWeapStructsInRange(me, x, y, range, finished, friendly)
	local count = 0
	local ally
	for player=0,MAX_PLAYERS-1 do
		ally = allianceExistsBetween(me, player) or me == player
		if (ally and friendly) or (not ally and not friendly) then
			count = count + numPlayerWeapStructsInRange(player, me, x, y, range, finished) 
		end
	end
	return count
end
function numEnemyWeapStructsInRange(me, x, y, range, finished)
	return numWeapStructsInRange(me, x, y, range, finished, false)
end
function numFriendlyWeapStructsInRange(me, x, y, range, finished)
	return numWeapStructsInRange(me, x, y, range, finished, true)
end

