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

--[[
	This file provides compatibility functions.
	Use version(<script compatibility version>) at the start of your scripts to provide fallbacks and wrappers.
]]--

function version(v)
	local current_version = 1

	-- check we support this version
	if v > current_version then
		error("This script is written using script version "..v.." while this build of Warzone 2100 only supports versions 0-"..current_version)
	end

	if v < 1 then
		-- Version 0 are the scripts that are converted using wz2lua
	
		-- These are script api functions which are written in Lua

		function setLandingZone(x1, y1, x2, y2)
			setNoGoArea(x1, y1, x2, y2, 0)
		end

		function random(upper)
			if upper < 1 then
				error('cannot call random with a negative value', 2)
			end
			return math.random(0, upper-1)
		end
		
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

		function modulo(a, b)
			return a % b
		end

		function Array(a, b)
			local new_array = {}
			local i,j
			if not b then
				for i=0,a do
					new_array[i] = 0
				end
			else
				for i=0,a do
					new_array[i] = {}
					for j=0,b do
						new_array[i][j] = 0
					end
				end
			end
			return new_array
		end

		function initEnumDroids(player, seen_by)
			_initEnumDroids_iterator = visibleDroids(player, seen_by)
		end

		function enumDroid()
			local droid
			droid = _initEnumDroids_iterator()
			return droid
		end

		function initEnumStruct(anystruct, stat, player, seen_by)
			if anystruct then stat = nil end
			if seen_by < 0 then seen_by = player end
			_initEnumStruct_iterator = visibleStructures(stat, player, seen_by)
		end

		function enumStruct()
			local structure
			structure = _initEnumStruct_iterator()
			return structure
		end
	end
end
