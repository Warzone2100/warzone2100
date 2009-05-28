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


--[[
General persistance framework.
All globals created after inclusion of save.lua will be saved.
]]--



_save = {}
_save.alreadythere = {}
_save.oldglobals = {}

_save.counter = 0

-- returns the name of f, call like search(some_object)
function _save.search(f, t, searched)
	t = t or _G
	searched = searched or {}
	searched[t] = true
	for n, v in pairs(t) do
		if v == f then
			return n
		end
		if type(v) == 'table' and not searched[v] then
			local r = _save.search(f, v, searched)
			if r then return tostring(n)..'.'..r end
		end
	end
end

-- returns a representation of the following types: nil, number, boolean, string, function
function _save.save_basic(value, object_to_name)
	if value == nil then
		return 'nil'
	elseif type(value) == 'number' or type(value) == 'boolean' then
		return tostring(value)
	elseif type(value) == 'string' then
		return string.format('%q', value)
	elseif type(value) == 'function' then
		-- check if the function was defined using loadstring
		local info = debug.getinfo(value)
		if info.source then
			if info.what ~= 'C' and string.sub(info.source,1,1) ~= '@' then
				-- hurray, a function defined by loadstring
				return 'loadstring("'..info.source..'")'
			end
		end
		-- perhaps we already know about it
		local r = object_to_name[value]
		if r then return r end
		-- last resort
		return 'unknownFunction'
	end
	error('invalid basic type:'..type(value))
end

-- create a mapping from object -> name for easy lookup later
function _save.fill_object_to_name(object_to_name, t, seen, parent)
	seen = seen or {}
	seen[t] = true
	-- FIXME: should be breadth first search so that we get the shortest variable names
	for k,v in pairs(t) do
		local add = not object_to_name[v]
		if not add and type(v) == 'function' then
			-- hack to find the name of the original function
			local info = debug.getinfo(v)
			if false and string.sub(info.source,1,1) == '@' then -- disable for now
				local filename = string.sub(info.source, 2)
				-- read specified line
				local currentline = 1
				local name 
				for line in io.lines(filename) do
					if currentline == info.linedefined then
						name = string.match(line, 'function[ \\t]+([a-zA-Z0-9_.]+)')
						if not name then
							name = string.match(line, '([a-zA-Z0-9_.]+)[ \\t]*=[ \\t]*function')
						end
						break
					end
					currentline = currentline + 1
				end
				-- now check if this is a valid name
				if name and loadstring('return '..name)() == v then
					object_to_name[v] = name
					add = false
				end
			end
		end
		if add then
			-- ok, we can add this to the mapping
			if type(k) == 'string' then
				if parent then
					object_to_name[v] = parent..'.'..k
				else
					object_to_name[v] = k
				end
			else
				if parent then
					local name = object_to_name[k]
					-- fallback to search
					if not name then name = _save.search(k) end
					if name then
						object_to_name[v] = parent..'['..name..']'
					end
				else
					object_to_name[v] = 'nil'
				end
			end
		end
	end
	-- recurse into children
	for k,v in pairs(t) do
		if type(v) == 'table' and not seen[v] then
			_save.fill_object_to_name(object_to_name, v, seen, object_to_name[v])
		end
	end
end

-- save value as name
function save (name, value, saved, object_to_name)
	if _save.oldglobals[name] == value then
		return ''
	end

	local basic = {['string'] = true; ['number'] = true; ['function'] = true; ['boolean']=true}
	if (not saved or not pairs(saved)(saved)) and saved ~= _save.alreadythere then
		-- copy the alreadythere table
		if not saved then saved = {} end
		for k,v in pairs(_save.alreadythere) do
			saved[k] = v
		end
	end
	if not object_to_name then
		object_to_name = {}
		_save.fill_object_to_name(object_to_name, _G)
	end
	if not name then
		error('no name for '..value)
	end
	local r = name..' = '
	if value == nil or basic[type(value)] then
		local setval = _save.save_basic(value, object_to_name)
		if setval ~= name then 
			return r..setval..'\n'
		else
			return ''
		end
	elseif type(value) == 'table' then
		if saved[value] then
			if name == saved[value] or saved[value] == 'package.loaded.'..name then
				return ''
			else
				return r..saved[value]..'\n'
			end
		end
		saved[value] = name
		r = r..'{}\n'
		local r2 = ''
		local make_new_table = true
		for k,v in pairs(value) do
			local fieldname, fieldnamedot
			if name == '_G' then
				fieldnamedot = ''
			else
				fieldnamedot = name..'.'
			end
			if type(k) == 'string' then
				fieldname = fieldnamedot..k
			elseif type(k) == 'table' then
				if saved[k] then
					fieldname = name..'['..saved[k]..']'
				else
					local tablename = object_to_name[k]
					if tablename then
						r2 = r2..save(tablename, k, saved, object_to_name)
					end
					if not tablename then
						_save.counter = _save.counter + 1
						tablename = '_table_'.._save.counter
						local s = save(tablename, k, saved, object_to_name)
						if s == '' then make_new_table = false end
						r2 = r2..s
					end
					fieldname = name..'['..tablename..']'
				end
			else
				fieldname = string.format("%s[%s]", name, _save.save_basic(k, object_to_name))
			end
			local s = save(fieldname, v, saved, object_to_name)
			if s == '' then make_new_table = false end
			r2 = r2..s
		end
		local mt = getmetatable(value)
		if mt then
			local metatablename
			metatablename = saved[mt]
			if not metatablename then
				metatablename = object_to_name[mt]
				if metatablename then
					r2 = r2..save(metatablename, mt, saved, object_to_name)
				end
			end
			if not metatablename then
				_save.counter = _save.counter + 1
				metatablename = '__metatable_'.._save.counter
				r2 = r2..save(metatablename, mt, saved, object_to_name)
			end
			r2 = r2..'setmetatable('..name..', '..metatablename..')\n'
		end
		if make_new_table then
			return r..r2
		else
			return r2
		end
	else
		return r..'cannot save a ' .. type(value)..'\n'
	end
end

function onload() -- dummy
end

-- returns a string containing lua code that will recreate the state
function saveall()
	local result = save('_G', _G)
	-- now fix everything up again
	onload()
	return 'version('..VERSION..')\n'..result
end

for k,v in pairs(_G) do
	if type(k) == 'string' and (type(v) == 'string' or type(v) == 'number') then
		_save.oldglobals[k] = v
	end
end

-- this is to prevent standard lua globals from showing up
save('_G', _G, _save.alreadythere)
_save.alreadythere[_G] = nil

--[[
void_function = function () return 0 end
getStructureByID = void_function
getGameTime = void_function
CALL_GAMEINIT = 333
test = startEvnt
dofile('data/script/event.lua')
dofile('data/script/cvars.lua')
dofile('data/script/glue.lua')
dofile('data/script/text/cam1a.lua')

--_Cvars.meta[1] = 1
registerCVariable('test', 1)

print(saveall())
print('---------------------------')
two = {}
print(save('C', C, two))
print(save('_Cvars', _Cvars, two))



print(save('two', two))
print('-----------------')
--_save.alreadythere[_save.alreadythere] = nil
--print(save('_save.alreadythere', _save.alreadythere))

for k,v in pairs(callback_list) do
	print(tostring(k)..' '..tostring(v))
end
]]--
