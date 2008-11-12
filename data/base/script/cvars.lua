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

C = {}
_Cvars = {} -- to store all private stuff in
_Cvars.Cid = {}
_Cvars.meta = {}
-- metamethods to allow C variables to be accessed like C.var
function _Cvars.meta.__index(table, key)
	return getCVar(_Cvars.Cid[key][1])
end
function _Cvars.meta.__newindex(table, key, value)
	if not _Cvars.Cid[key][2] then
		setCVar(_Cvars.Cid[key][1], value)
	else
		error("attempt to set readonly C value")
	end
end
setmetatable(C, _Cvars.meta)

-- called by C to register an id
function registerCVariable(name, id, read_only)
	_Cvars.Cid[name] = {id, read_only}
end
