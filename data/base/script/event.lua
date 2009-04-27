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
	if event == nil then error("nil event") end
	new_event.type = 'callback'
	new_event.id = event
	new_event.enabled = true
	_event.event_list[handler] = new_event
end

function deactivateEvent(handler)
	if not _event.event_list[handler] then
		error('trying to deactivate an unknown event handler: '.._save.search(handler), 2)
	else
		_event.event_list[handler].enabled = false
		_event.event_list[handler].enabled_stored = false
	end
end

-- will process all conditional events, executed every frame
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
						_event.event_list[handler].enabled = false
					else
						event.check_time = event.check_time + event.time
					end
					-- and update
					_event.run_event(handler, event.parameters)
				end
			end
		end
	end
end
callbackEvent(processEvents, CALL_EVERY_FRAME)

---------------------------------
-- Pretty backtraces
function _event.pack(a, ...)
	return a, {...}
end

function _event.call_with_backtrace(f, ...) -- has an override in version.lua
	local arg = {...}
	local run_function = function () return f(unpack(arg)) end
	local result, returns = _event.pack(xpcall(run_function, debug.traceback))
	if not result then
		-- HACK: try to remove the last useless lines
		local index = string.find(returns[1], "[C]: in function 'xpcall'", 1, true)
		if index then
			print(string.sub(returns[1], 1, index-19))
		else
			print(returns[1])
		end
		return nil
	else
		return returns
	end
end

-- wrapper to make sure that an event handler is not executing twice at the
-- same time
function _event.disable_run_enable(handler, ...)
	local old = _event.event_list[handler]
	if _event.event_list[handler] then -- could be deactivated
		_event.event_list[handler].enabled_stored = _event.event_list[handler].enabled
		_event.event_list[handler].enabled = false
	end

	local results = {_event.call_with_backtrace(handler, ...)}

	if _event.event_list[handler] and old == _event.event_list[handler] then -- could be deactivated or reactivated
		_event.event_list[handler].enabled = _event.event_list[handler].enabled_stored
	end
	return unpack(results)
end

function _event.run_event(handler, ...) -- has an override in version.lua
	_event.disable_run_enable(handler, ...)
end

-- called from C when a callback event occurs
function doCallbacksFor(event, ...)
	for h, e in pairs(_event.event_list) do
		if e.type == 'callback' and e.enabled and e.id == event then
			_event.run_event(h, ...)
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
