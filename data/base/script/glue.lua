--[[
	This file is part of Warzone 2100.
	Copyright (C) 2008-2009  Warzone Resurrection Project

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

-- this will make sure we can use the concatenation operator .. with booleans
__boolean_meta = {}
function __boolean_meta.__concat(a, b)
	if type(a) == 'boolean' then
		if a then
			a = 'true'
		else
			a = 'false'
		end
	end
	if type(b) == 'boolean' then
		if b then
			b = 'true'
		else
			b = 'false'
		end
	end
	return a..b
end
-- use the debug library as we can then set metatables for any type
debug.setmetatable(true, __boolean_meta)

-- A dofile that will prepend the path of the script that calls it
__dofile = dofile
function dofile(filename)
	local n, callingfile
	n = 2
	path = ""
	while true do
		info = debug.getinfo(n)
		if string.sub(info.source,1,1) == "@" then
			callingfile = string.sub(info.source,2)
			break
		end
		n = n + 1
	end
	if callingfile == "" then error("could not determine calling file") end
	path = callingfile
	while true do
		if string.sub(path,-1) == "/" or path == "" then break end
		path = string.sub(path,0,-2)
	end
	return __dofile(path..filename)
end
