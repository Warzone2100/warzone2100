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

_group = {}
_group.old_saveall = saveall
function saveall()
	_group.convert_to_ids()
	return _group.old_saveall()
end

function _group.convert_to_ids(t, seen)
	t = t or _G
	seen = seen or {}
	if seen[t] then return end
	seen[t] = true
	for k,v in pairs(t) do
		if v ~= C then
			if type(v) == 'table' then
				if v.type == 'group' then
					t[k] = {} -- empty it
					t[k].type = 'savedgroup'
					t[k].ids = {}
					initIterateGroup(v)
					local droid = iterateGroup(v)
					while droid do
						--print('saving droid '..tostring(droid.id))
						table.insert(t[k].ids, droid.id)
						droid = iterateGroup(v)
					end
				else
					_group.convert_to_ids(v, seen)
				end
			end
		end
	end
end

_group.old_onload = onload
function onload()
	_group.convert_from_ids()
	_group.old_onload()
end

function _group.convert_from_ids(t, seen)
	t = t or _G
	seen = seen or {}
	if seen[t] then return end
	seen[t] = true
	for k,v in pairs(t) do
		if v ~= C then
			if type(v) == 'table' then
				if v.type == 'savedgroup' then
					t[k] = Group()
					for i, id in ipairs(v.ids) do
						groupAddDroid(t[k], id)
					end
				else
					_group.convert_from_ids(v, seen)
				end
			end
		end
	end
end
