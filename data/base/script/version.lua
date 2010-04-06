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

	Versions (even are releases, odd ones support both n-1 and n+1 functions but will warn about deprecation):
	0: The scripts that shipped with the original version of Warzone
	1: Compatibility mode, will warn about functions that are removed or changed in version 2
	2: Changes to the script API that were done while going from wzscript to Lua.
]]--

function deprecate(f, message)
	return function(...) warning("function ".. debug.getinfo(1).name .." is deprecated: "..message, 2); f(...) end
end

function version(v)
	local current_version = 2

	-- check we support this version
	if v < 0 or v > current_version then
		error("This script is written using script version "..v.." while this build of Warzone 2100 only supports versions 0-"..current_version)
	end
	-- did we already select a script version?
	if VERSION then
		if VERSION ~= v then
			warning("Loading a script with version "..v..", but the previous script loaded in this state required version "..VERSION..": ", 2)
			-- now check which version to select
			if math.abs(VERSION - v) > 2 or
			   (math.abs(VERSION - v) > 1 and (VERSION % 2 ~= 0 or v % 2 ~= 0)) then
				print("versions differ too much, keeping the previous version ("..VERSION..")")
				v = VERSION
			else
				-- if two stable versions, select the compatibility mode
				if VERSION % 2 == 0 and v % 2 == 0 then
					v = (VERSION + v) / 2
					print("selecting compatibility mode (version "..v..")")
				else
					-- now select the compatibility mode one
					if VERSION % 2 > 0 then
						v = VERSION
					end
					print("selected compatibility version "..v)
				end
			end
		end
	end
	-- warn for compatibility mode
	if v % 2 > 0 then
		print("WARNING: Running in version ".. (v-1) .. " compatibility mode")
		print("         Please fully convert to version " .. (v+1) .. " before releasing this script")
	end
	VERSION = v -- set the global version

	if v < 2 then
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
			_initEnumStruct_iterator = visibleStructuresOfType(stat, player, seen_by)
		end

		function enumStruct()
			local structure
			structure = _initEnumStruct_iterator()
			return structure
		end

		initGetFeature_iterators = {}
		function initGetFeature(feature_type, seen_by, bucket)
			initGetFeature_iterators[bucket] = visibleFeatures(seen_by, feature_type)
		end

		function getFeature(bucket)
			local feature
			for feature in initGetFeature_iterators[bucket] do
				if not structureOnLocation(feature.x, feature.y) and
				   not fireOnLocation(feature.x, feature.y) then
					return feature
				end
			end
			return nil
		end
		getFeatureB = getFeature

		-------------------------------------------------
		-- event stuff (to support the "pause" function)
		-- pause execution of an event handler for a certain time
		-- WARNING: when the game is saved, this function will return immedeately
		-- use only "for effect", to make it less obvious that you are acting on
		-- the destruction of a building, or the fact that an unit has been spotted
		-- if you need a reliable pause, use delayedEvent
		function pause(time)
			--print('entering pause')
			if not _event.saving then
				coroutine.yield('pause', time)
			end
			--print('leaving pause')
		end

		function _event.call_with_backtrace(f, ...)
			local arg = {...}
			local run_function = function () return f(unpack(arg)) end
			local result, returns = _event.pack(xpcall(run_function, debug.traceback))
			if not result then
				--[[ HACK: try to remove the last useless lines
				local index = string.find(returns[1], "[C]: in function 'xpcall'", 1, true)
				if index then
					print(string.sub(returns[1], 1, index-19))
				else
					print(returns[1])
				end
				return nil
				]]
				print(returns[1])
			else
				return returns
			end
		end

		-- run the handler as a coroutine to intercept the yields produced by pause
		-- handler can be a function or a thread, the thread will be resumed
		function _event.run_event(handler, ...)
			--[[if parameters then
				print('run_event')
				for i,v in ipairs(parameters) do
					print(tostring(i)..':'..tostring(v))
				end
			end]]--
			local results, co
			parameters = parameters or {}
			-- first make sure we have a coroutine
			if type(handler) == 'function' then
				co = coroutine.create(_event.disable_run_enable)
				-- run it
				results = {coroutine.resume(co, handler, ...)}
			elseif type(handler) == 'thread' then
				co = handler
				-- run it
				results = {coroutine.resume(co, ...)}
			else
				error('handler is not a function or a thread')
			end

			-- check what to do with it
			if coroutine.status(co) == 'suspended' then
				-- it called yield, well, what does it want

				if results[2] == 'pause' then
					-- pause(time)
					-- schedule it to be resumed after the specified time
					delayedEvent(co, results[3])
				elseif results[2] == 'sound' then
					print('inserting sound finished')
					callbackEvent(co, CALL_SOUND_FINISHED)
				else
					error('yield returned unsupported request '..results[2])
				end
			elseif coroutine.status(co) == 'dead' and type(handler) == 'thread' then
				_event.event_list[handler].enabled = false
			elseif coroutine.status(co) == 'dead' and results[1] == false and results [2] ~= nil then
				--print('error in script: '..results[2])
				error(results[2])
			end

		end
	end

	if v == 1 and not __DEPRECATED_VERSION_1__ then
		__DEPRECATED_VERSION_1__ = true
		setLandingZone       = deprecate(setLandingZone, "use setNoGoArea")
		random               = deprecate(random, "use math.random")
		distBetweenTwoPoints = deprecate(distBetweenTwoPoints, "do the calculations yourself")
		ASSERT               = deprecate(ASSERT, "use \"if not <condition> then error(...)\"")
		objToStructure       = deprecate(objToStructure, "use <object>.type to see if it has the correct one")
		objToDroid           = deprecate(objToDroid, "use <object>.type to see if it has the correct one")
		modulo               = deprecate(modulo, "use \"a % b\"")
		Array                = deprecate(Array, "use the Lua list definition syntax")
		initEnumDroids       = deprecate(initEnumDroids, "use the droid iterator functions (droids/visibleDroids)")
		enumDroid            = deprecate(enumDroid, "use the droid iterator functions (droids/visibleDroids)")
		initEnumStruct       = deprecate(initEnumStruct, "use the structure iterator functions")
		enumStruct           = deprecate(enumStruct, "use the structure iterator functions")
		initGetFeature       = deprecate(initGetFeature, "use the feature iterator functions")
		getFeature           = deprecate(getFeature, "use the feature iterator functions")
		pause                = deprecate(pause, "use delayedEvent")
	end
end
