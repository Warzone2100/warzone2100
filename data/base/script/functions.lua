--[[
	This file is part of Warzone 2100.
	Copyright (C) 2009  Warzone Resurrection Project

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

-- Functions for counting things in a range
-- all objects
function numFriendlyWeapObjInRange(me, x, y, range, vtols, finished)
	local count
	count =         numFriendlyWeapDroidsInRange(me, x, y, range, vtols)
	count = count + numFriendlyWeapStructsInRange(me, x, y, range, finished) 
	return count
end

function numPlayerWeapObjInRange(player, me, x, y, range, vtols, finished)
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

