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

-- structure code
--[[
_glue.structuremt = {}
_glue.structuremt.__eq = function(a,b)
	return a.ptr == b.ptr
end]]--
--[[
_glue.tracked_structs = {}
function getStructureByID(id, varname)
	local s = _getStructureByID(id)
	--setmetatable(s, _glue.structuremt)
	if varname ~= nil then
		-- add to the list so we can nil it later
		print('getStructureByID: adding to check list')
		_glue.tracked_structs[s] = varname
	end
	return s
end
function _glue.structDestroyed(structure)
	print('_glue.structDestroyed')
	for s, varname in pairs(_glue.tracked_structs) do
		if s.ptr == structure.ptr then
			print('setting '..varname..' to nil')
			_G[varname] = nil
			_glue.tracked_structs[s] = nil
			return
		end
	end
end
callbackEvent(_glue.structDestroyed, CALL_STRUCT_DESTROYED)
]]--
-- temporary
--mapWidth = 64
--mapHeight = 64
