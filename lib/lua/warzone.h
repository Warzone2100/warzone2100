/*
	This file is part of Warzone 2100.
	Copyright (C) 2005-2007  Warzone Resurrection Project

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
*/

#ifndef _lua_warzone_h
#define _lua_warzone_h

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

extern int luaWZ_dofile(lua_State *L, const char *filename);
extern int luaWZ_dofile_threaded(lua_State *L, const char *filename);
extern void luaWZ_openlibs(lua_State *L);
extern int luaWZ_pcall_backtrace(lua_State *L, int args, int ret);
extern void luaWZ_pcall_backtrace_threaded(lua_State *L);
extern int luaWZ_loadfile(lua_State *L, const char *filename);

extern bool luaL_checkboolean(lua_State* L, int param);

extern int luaWZ_checkplayer (lua_State *L, int narg);

extern const char *luaWZ_terminated;

extern void luaWZ_setintfield (lua_State *L, const char *index, int value);
extern void luaWZ_setpointerfield (lua_State *L, const char *index, void * ptr);
extern void luaWZ_setstringfield (lua_State *L, const char *index, const char * string);
extern void luaWZ_setnumberfield (lua_State *L, const char *index, double value);

#endif
