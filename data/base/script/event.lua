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

_event = {}
_event.event_list = {}

-- supported events
function conditionalEvent( handler, expression, time, rep, parameters)
	local new_handler = {}
	new_handler.type = 'conditional'
	if type(expression) == 'string' then
		new_handler.expression = loadstring('return ' .. expression)
	else
		new_handler.expression = expression
	end
	new_handler.time = time
	new_handler.check_time = time+getGameTime()
	if rep == nil then
		rep = true
	end
	new_handler.rep = rep
	new_handler.parameters = parameters
	new_handler.enabled = true
	_event.event_list[handler] = new_handler
end

-- will execute <handler> every <time> seconds
function repeatingEvent(handler, time)
	conditionalEvent( handler, nil, time, true)
end
-- will wait <time> seconds and then execute <handler>
function delayedEvent(handler, time)
	conditionalEvent( handler, nil, time, false)
end

function callbackEvent(handler, event)
	--print('adding callback for '..event)
	local new_event = {}
	new_event.type = 'callback'
	new_event.id = event
	new_event.enabled = true
	_event.event_list[handler] = new_event
end

function deactivateEvent(handler)
	_event.event_list[handler] = nil
end

-- will process all conditional events, executed every tick
function processEvents()
	local now = getGameTime()
	-- iterate over all of them
	for handler, event in pairs(_event.event_list) do
		-- check if we need to check the time
		if event.type == 'conditional' and event.enabled then
			-- is it time?
			if now > event.check_time then
				-- check the condition
				if not event.expression or event.expression() then
					-- update the trigger
					if event.rep == false then
						_event.event_list[handler] = nil
					else
						event.check_time = event.check_time + event.time
					end
					-- and update
					run_event(handler, event.parameters)
				end
			end
		end
	end
end

-- wrapper to make sure that an event handler is not executing twice at the
-- same time
function _event.disable_run_enable(handler, ...)
	if _event.event_list[handler] then -- could be deactivated
		_event.event_list[handler].enabled = false
	end
	--[[if arg then
		print('disable_run_enable')
		for i,v in ipairs(arg) do
			print(tostring(i)..':'..tostring(v))
		end
	end]]--
	local results = {handler(unpack(arg))}
	if _event.event_list[handler] then -- could be deactivated
		_event.event_list[handler].enabled = true
	end
	return unpack(results)
end

-- run the handler as a coroutine to intercept the yields produced by pause
-- handler can be a function or a thread, the thread will be resumed
function run_event(handler, parameters)
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
		results = {coroutine.resume(co, handler, unpack(parameters))}
	elseif type(handler) == 'thread' then
		co = handler
		-- run it
		results = {coroutine.resume(co, unpack(parameters))}
	else
		error('handler is not a function or a thread')
	end
	if results[1] ~= true then
		print(results[2])
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
		_event.event_list[handler] = nil
	end
end

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

-- called every frame by C
function tick()
	processEvents()
end

-- called from C when a callback event occurs
function doCallbacksFor(event, ...)
	--print('callback for '..event)
	for h, e in pairs(_event.event_list) do
		--print('iterating...')
		--print(e.type)
		--print(e.enabled)
		--print(e.id)
		if e.type == 'callback' and e.enabled and e.id == event then
			--[[print('found handler, calling')
			for i,v in ipairs(arg) do
				print(tostring(i)..':'..tostring(v))
			end]]--
			run_event(h, arg)
		end
	end
end

-- hook into the save system to get rid of the coroutines
_event.old_saveall = saveall
function saveall()
	-- make sure that we won't pause
	_event.saving = true
	-- iterate over them
	for handler, event in pairs(_event.event_list) do
		if type(handler) == 'thread' then
			-- run until done
			while coroutine.status(handler) ~= 'dead' do
				coroutine.resume(handler)
			end
			-- remove
			_event.event_list[handler] = nil
		elseif event.enabled == false then
			-- remove the inactive ones
			_event.event_list[handler] = nil
		end
	end
	_event.saving = false
	
	return _event.old_saveall()
end

--[[ Devurandom compatibility functions
event = {}

event.active = {}

function event.link(time, handler)
	local f
	f = function()
			t = handler()
			if t then delayedEvent(f, t) end
		end
	delayedEvent(f, time)
	event.active[handler] = f
end

function event.unlink(handler)
	deactivateEvent(event.active[handler])
	event.active[handler] = nil
end

function event.every(time, handler) -- every 'time' ticks call 'handler'
	return time, 	function() -- return 2 values, initial execution time + eventhandler
						handler() -- execute the eventhandler
						return time -- fire again in 'time' ticks
					end
end

function event.wait(time, handler)
	return time, handler
end
]]--
