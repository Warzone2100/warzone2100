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
	VERSION = v -- set the global version

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

		_addPower = addPower
		function addPower(power, player)
			return _addPower(player, power) -- swap arguments
		end

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

		function _event.call_with_backtrace(f, arg)
			return f(unpack(arg)) -- no bactrace here!
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
				results = {coroutine.resume(co, handler, unpack(arg))}
			elseif type(handler) == 'thread' then
				co = handler
				-- run it
				results = {coroutine.resume(co, unpack(arg))}
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
			end

		end
	end
end
